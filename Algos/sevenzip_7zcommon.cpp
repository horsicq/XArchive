/* XArchive amalgamation of 7-Zip 26.01 -- CPP/7zip/Common support.
 *
 * 21 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/7zip/Common/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

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

/* ---- CPP/7zip/Common/RegisterCodec.h ---- */
// RegisterCodec.h

#ifndef ZIP7_INC_REGISTER_CODEC_H
#define ZIP7_INC_REGISTER_CODEC_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

typedef void * (*CreateCodecP)();

struct CCodecInfo
{
  CreateCodecP CreateDecoder;
  CreateCodecP CreateEncoder;
  CMethodId Id;
  const char *Name;
  UInt32 NumStreams;
  bool IsFilter;
};

void RegisterCodec(const CCodecInfo *codecInfo) throw();


#define REGISTER_CODEC_CREATE_2(name, cls, i) static void *name() { return (void *)(i *)(new cls); }
#define REGISTER_CODEC_CREATE(name, cls) REGISTER_CODEC_CREATE_2(name, cls, ICompressCoder)

#define REGISTER_CODEC_NAME(x) CRegisterCodec ## x
#define REGISTER_CODEC_VAR(x) static const CCodecInfo g_CodecInfo_ ## x =

#define REGISTER_CODEC(x) struct REGISTER_CODEC_NAME(x) { \
    REGISTER_CODEC_NAME(x)() { RegisterCodec(&g_CodecInfo_ ## x); }}; \
    static REGISTER_CODEC_NAME(x) g_RegisterCodec_ ## x;


#define REGISTER_CODECS_NAME(x) CRegisterCodecs ## x
#define REGISTER_CODECS_VAR static const CCodecInfo g_CodecsInfo[] =

#define REGISTER_CODECS(x) struct REGISTER_CODECS_NAME(x) { \
    REGISTER_CODECS_NAME(x)() { for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_CodecsInfo); i++) \
    RegisterCodec(&g_CodecsInfo[i]); }}; \
    static REGISTER_CODECS_NAME(x) g_RegisterCodecs;


#define REGISTER_CODEC_2(x, crDec, crEnc, id, name) \
    REGISTER_CODEC_VAR(x) \
    { crDec, crEnc, id, name, 1, false }; \
    REGISTER_CODEC(x)


#ifdef Z7_EXTRACT_ONLY
  #define REGISTER_CODEC_E(x, clsDec, clsEnc, id, name) \
    REGISTER_CODEC_CREATE(CreateDec, clsDec) \
    REGISTER_CODEC_2(x, CreateDec, NULL, id, name)
#else
  #define REGISTER_CODEC_E(x, clsDec, clsEnc, id, name) \
    REGISTER_CODEC_CREATE(CreateDec, clsDec) \
    REGISTER_CODEC_CREATE(CreateEnc, clsEnc) \
    REGISTER_CODEC_2(x, CreateDec, CreateEnc, id, name)
#endif



#define REGISTER_FILTER_CREATE(name, cls) REGISTER_CODEC_CREATE_2(name, cls, ICompressFilter)

#define REGISTER_FILTER_ITEM(crDec, crEnc, id, name) \
    { crDec, crEnc, id, name, 1, true }

#define REGISTER_FILTER(x, crDec, crEnc, id, name) \
    REGISTER_CODEC_VAR(x) \
    REGISTER_FILTER_ITEM(crDec, crEnc, id, name); \
    REGISTER_CODEC(x)

#ifdef Z7_EXTRACT_ONLY
  #define REGISTER_FILTER_E(x, clsDec, clsEnc, id, name) \
    REGISTER_FILTER_CREATE(x ## _CreateDec, clsDec) \
    REGISTER_FILTER(x, x ## _CreateDec, NULL, id, name)
#else
  #define REGISTER_FILTER_E(x, clsDec, clsEnc, id, name) \
    REGISTER_FILTER_CREATE(x ## _CreateDec, clsDec) \
    REGISTER_FILTER_CREATE(x ## _CreateEnc, clsEnc) \
    REGISTER_FILTER(x, x ## _CreateDec, x ## _CreateEnc, id, name)
#endif



struct CHasherInfo
{
  IHasher * (*CreateHasher)();
  CMethodId Id;
  const char *Name;
  UInt32 DigestSize;
};

void RegisterHasher(const CHasherInfo *hasher) throw();

#define REGISTER_HASHER_NAME(x) CRegHasher_ ## x

#define REGISTER_HASHER(cls, id, name, size) \
    Z7_COM7F_IMF2(UInt32, cls::GetDigestSize()) { return size; } \
    static IHasher *CreateHasherSpec() { return new cls(); } \
    static const CHasherInfo g_HasherInfo = { CreateHasherSpec, id, name, size }; \
    struct REGISTER_HASHER_NAME(cls) { REGISTER_HASHER_NAME(cls)() { RegisterHasher(&g_HasherInfo); }}; \
    static REGISTER_HASHER_NAME(cls) g_RegisterHasher;

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

/* ---- CPP/7zip/Common/MemBlocks.h ---- */
// MemBlocks.h

#ifndef ZIP7_INC_MEM_BLOCKS_H
#define ZIP7_INC_MEM_BLOCKS_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

class CMemBlockManager
{
  void *_data;
  size_t _blockSize;
  void *_headFree;
public:
  CMemBlockManager(size_t blockSize = (1 << 20)): _data(NULL), _blockSize(blockSize), _headFree(NULL) {}
  ~CMemBlockManager() { FreeSpace(); }

  bool AllocateSpace_bool(size_t numBlocks);
  void FreeSpace();
  size_t GetBlockSize() const { return _blockSize; }
  void *AllocateBlock();
  void FreeBlock(void *p);
};


class CMemBlockManagerMt: public CMemBlockManager
{
  NWindows::NSynchronization::CCriticalSection _criticalSection;
public:
  SYNC_OBJ_DECL(Synchro)
  NWindows::NSynchronization::CSemaphore_WFMO Semaphore;

  CMemBlockManagerMt(size_t blockSize = (1 << 20)): CMemBlockManager(blockSize) {}
  ~CMemBlockManagerMt() { FreeSpace(); }

  HRESULT AllocateSpace(size_t numBlocks, size_t numNoLockBlocks);
  HRESULT AllocateSpaceAlways(size_t desiredNumberOfBlocks, size_t numNoLockBlocks = 0);
  void FreeSpace();
  void *AllocateBlock();
  void FreeBlock(void *p, bool lockMode = true);
  // WRes ReleaseLockedBlocks_WRes(unsigned number) { return Semaphore.Release(number); }
};


class CMemBlocks
{
  void Free(CMemBlockManagerMt *manager);
public:
  CRecordVector<void *> Blocks;
  UInt64 TotalSize;
  
  CMemBlocks(): TotalSize(0) {}

  void FreeOpt(CMemBlockManagerMt *manager);
  HRESULT WriteToStream(size_t blockSize, ISequentialOutStream *outStream) const;
};

struct CMemLockBlocks: public CMemBlocks
{
  bool LockMode;

  CMemLockBlocks(): LockMode(true) {}
  void Free(CMemBlockManagerMt *memManager);
  void FreeBlock(unsigned index, CMemBlockManagerMt *memManager);
  // HRESULT SwitchToNoLockMode(CMemBlockManagerMt *memManager);
  void Detach(CMemLockBlocks &blocks, CMemBlockManagerMt *memManager);
};

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

/* ---- CPP/7zip/Common/OffsetStream.h ---- */
// OffsetStream.h

#ifndef ZIP7_INC_OFFSET_STREAM_H
#define ZIP7_INC_OFFSET_STREAM_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_NOQIB_1(
  COffsetOutStream
  , IOutStream
)
  Z7_IFACE_COM7_IMP(ISequentialOutStream)

  CMyComPtr<IOutStream> _stream;
  UInt64 _offset;
public:
  HRESULT Init(IOutStream *stream, UInt64 offset);
};

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

/* ---- CPP/7zip/Common/OutMemStream.h ---- */
// OutMemStream.h

#ifndef ZIP7_INC_OUT_MEM_STREAM_H
#define ZIP7_INC_OUT_MEM_STREAM_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_NOQIB_1(
  COutMemStream
  , IOutStream
)
  Z7_IFACE_COM7_IMP(ISequentialOutStream)

  CMemBlockManagerMt *_memManager;
  size_t _curBlockPos;
  unsigned _curBlockIndex;
  bool _realStreamMode;

  bool _unlockEventWasSent;
  NWindows::NSynchronization::CAutoResetEvent_WFMO StopWritingEvent;
  NWindows::NSynchronization::CAutoResetEvent_WFMO WriteToRealStreamEvent;
  // NWindows::NSynchronization::CAutoResetEvent NoLockEvent;

  HRESULT StopWriteResult;
  CMemLockBlocks Blocks;

  CMyComPtr<ISequentialOutStream> OutSeqStream;
  CMyComPtr<IOutStream> OutStream;

  UInt64 GetPos() const { return (UInt64)_curBlockIndex * _memManager->GetBlockSize() + _curBlockPos; }

public:

  HRESULT CreateEvents(SYNC_PARAM_DECL(synchro))
  {
    WRes wres = StopWritingEvent.CreateIfNotCreated_Reset(SYNC_WFMO(synchro));
    if (wres == 0)
      wres = WriteToRealStreamEvent.CreateIfNotCreated_Reset(SYNC_WFMO(synchro));
    return HRESULT_FROM_WIN32(wres);
  }

  void SetOutStream(IOutStream *outStream)
  {
    OutStream = outStream;
    OutSeqStream = outStream;
  }

  void SetSeqOutStream(ISequentialOutStream *outStream)
  {
    OutStream = NULL;
    OutSeqStream = outStream;
  }

  void ReleaseOutStream()
  {
    OutStream.Release();
    OutSeqStream.Release();
  }

  COutMemStream(CMemBlockManagerMt *memManager):
      _memManager(memManager)
  {
    /*
    #ifndef _WIN32
    StopWritingEvent._sync       =
    WriteToRealStreamEvent._sync =  &memManager->Synchro;
    #endif
    */
  }

  ~COutMemStream() { Free(); }
  void Free();

  void Init();
  HRESULT WriteToRealStream();

  void DetachData(CMemLockBlocks &blocks);

  bool WasUnlockEventSent() const { return _unlockEventWasSent; }

  void SetRealStreamMode()
  {
    _unlockEventWasSent = true;
    WriteToRealStreamEvent.Set();
  }

  /*
  void SetNoLockMode()
  {
    _unlockEventWasSent = true;
    NoLockEvent.Set();
  }
  */

  void StopWriting(HRESULT res)
  {
    StopWriteResult = res;
    StopWritingEvent.Set();
  }
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

/* ---- CPP/7zip/Common/ProgressMt.h ---- */
// ProgressMt.h

#ifndef ZIP7_INC_PROGRESSMT_H
#define ZIP7_INC_PROGRESSMT_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

class CMtCompressProgressMixer
{
  CMyComPtr<ICompressProgressInfo> _progress;
  CRecordVector<UInt64> InSizes;
  CRecordVector<UInt64> OutSizes;
  UInt64 TotalInSize;
  UInt64 TotalOutSize;
public:
  NWindows::NSynchronization::CCriticalSection CriticalSection;
  void Init(unsigned numItems, ICompressProgressInfo *progress);
  void Reinit(unsigned index);
  HRESULT SetRatioInfo(unsigned index, const UInt64 *inSize, const UInt64 *outSize);
};


Z7_CLASS_IMP_NOQIB_1(
  CMtCompressProgress
  , ICompressProgressInfo
)
  unsigned _index;
  CMtCompressProgressMixer *_progress;
public:
  void Init(CMtCompressProgressMixer *progress, unsigned index)
  {
    _progress = progress;
    _index = index;
  }
  void Reinit() { _progress->Reinit(_index); }
};

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

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Common/CWrappers.cpp ================ */
// CWrappers.c

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

SRes HRESULT_To_SRes(HRESULT res, SRes defaultRes) throw()
{
  switch (res)
  {
    case S_OK: return SZ_OK;
    case E_OUTOFMEMORY: return SZ_ERROR_MEM;
    case E_INVALIDARG: return SZ_ERROR_PARAM;
    case E_ABORT: return SZ_ERROR_PROGRESS;
    case S_FALSE: return SZ_ERROR_DATA;
    case E_NOTIMPL: return SZ_ERROR_UNSUPPORTED;
    default: break;
  }
  return defaultRes;
}


HRESULT SResToHRESULT(SRes res) throw()
{
  switch (res)
  {
    case SZ_OK: return S_OK;
    
    case SZ_ERROR_DATA:
    case SZ_ERROR_CRC:
    case SZ_ERROR_INPUT_EOF:
    case SZ_ERROR_ARCHIVE:
    case SZ_ERROR_NO_ARCHIVE:
      return S_FALSE;
    
    case SZ_ERROR_MEM: return E_OUTOFMEMORY;
    case SZ_ERROR_PARAM: return E_INVALIDARG;
    case SZ_ERROR_PROGRESS: return E_ABORT;
    case SZ_ERROR_UNSUPPORTED: return E_NOTIMPL;
    // case SZ_ERROR_OUTPUT_EOF:
    // case SZ_ERROR_READ:
    // case SZ_ERROR_WRITE:
    // case SZ_ERROR_THREAD:
    // case SZ_ERROR_ARCHIVE:
    // case SZ_ERROR_NO_ARCHIVE:
    // return E_FAIL;
    default: break;
  }
  if (res < 0)
    return res;
  return E_FAIL;
}


#define PROGRESS_UNKNOWN_VALUE ((UInt64)(Int64)-1)

#define CONVERT_PR_VAL(x) (x == PROGRESS_UNKNOWN_VALUE ? NULL : &x)


static SRes CompressProgress(ICompressProgressPtr pp, UInt64 inSize, UInt64 outSize) throw()
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CCompressProgressWrap)
  p->Res = p->Progress->SetRatioInfo(CONVERT_PR_VAL(inSize), CONVERT_PR_VAL(outSize));
  return HRESULT_To_SRes(p->Res, SZ_ERROR_PROGRESS);
}

void CCompressProgressWrap::Init(ICompressProgressInfo *progress) throw()
{
  vt.Progress = CompressProgress;
  Progress = progress;
  Res = SZ_OK;
}

static const UInt32 kStreamStepSize = (UInt32)1 << 31;

static SRes MyRead(ISeqInStreamPtr pp, void *data, size_t *size) throw()
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSeqInStreamWrap)
  UInt32 curSize = ((*size < kStreamStepSize) ? (UInt32)*size : kStreamStepSize);
  p->Res = (p->Stream->Read(data, curSize, &curSize));
  *size = curSize;
  p->Processed += curSize;
  if (p->Res == S_OK)
    return SZ_OK;
  return HRESULT_To_SRes(p->Res, SZ_ERROR_READ);
}

static size_t MyWrite(ISeqOutStreamPtr pp, const void *data, size_t size) throw()
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSeqOutStreamWrap)
  if (p->Stream)
  {
    p->Res = WriteStream(p->Stream, data, size);
    if (p->Res != 0)
      return 0;
  }
  else
    p->Res = S_OK;
  p->Processed += size;
  return size;
}


void CSeqInStreamWrap::Init(ISequentialInStream *stream) throw()
{
  vt.Read = MyRead;
  Stream = stream;
  Processed = 0;
  Res = S_OK;
}

void CSeqOutStreamWrap::Init(ISequentialOutStream *stream) throw()
{
  vt.Write = MyWrite;
  Stream = stream;
  Res = SZ_OK;
  Processed = 0;
}


static SRes InStreamWrap_Read(ISeekInStreamPtr pp, void *data, size_t *size) throw()
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSeekInStreamWrap)
  UInt32 curSize = ((*size < kStreamStepSize) ? (UInt32)*size : kStreamStepSize);
  p->Res = p->Stream->Read(data, curSize, &curSize);
  *size = curSize;
  return (p->Res == S_OK) ? SZ_OK : SZ_ERROR_READ;
}

static SRes InStreamWrap_Seek(ISeekInStreamPtr pp, Int64 *offset, ESzSeek origin) throw()
{
  Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CSeekInStreamWrap)
  UInt32 moveMethod;
  /* we need (int)origin to eliminate the clang warning:
     default label in switch which covers all enumeration values
     [-Wcovered-switch-default */
  switch ((int)origin)
  {
    case SZ_SEEK_SET: moveMethod = STREAM_SEEK_SET; break;
    case SZ_SEEK_CUR: moveMethod = STREAM_SEEK_CUR; break;
    case SZ_SEEK_END: moveMethod = STREAM_SEEK_END; break;
    default: return SZ_ERROR_PARAM;
  }
  UInt64 newPosition;
  p->Res = p->Stream->Seek(*offset, moveMethod, &newPosition);
  *offset = (Int64)newPosition;
  return (p->Res == S_OK) ? SZ_OK : SZ_ERROR_READ;
}

void CSeekInStreamWrap::Init(IInStream *stream) throw()
{
  Stream = stream;
  vt.Read = InStreamWrap_Read;
  vt.Seek = InStreamWrap_Seek;
  Res = S_OK;
}


/* ---------- CByteInBufWrap ---------- */

void CByteInBufWrap::Free() throw()
{
  ::MidFree(Buf);
  Buf = NULL;
}

bool CByteInBufWrap::Alloc(UInt32 size) throw()
{
  if (!Buf || size != Size)
  {
    Free();
    Lim = Cur = Buf = (Byte *)::MidAlloc((size_t)size);
    Size = size;
  }
  return (Buf != NULL);
}

Byte CByteInBufWrap::ReadByteFromNewBlock() throw()
{
  if (!Extra && Res == S_OK)
  {
    UInt32 avail;
    Res = Stream->Read(Buf, Size, &avail);
    Processed += (size_t)(Cur - Buf);
    Cur = Buf;
    Lim = Buf + avail;
    if (avail != 0)
      return *Cur++;
  }
  Extra = true;
  return 0;
}

// #pragma GCC diagnostic ignored "-Winvalid-offsetof"

static Byte Wrap_ReadByte(IByteInPtr pp) throw()
{
  CByteInBufWrap *p = Z7_CONTAINER_FROM_VTBL_CLS(pp, CByteInBufWrap, vt);
  // Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CByteInBufWrap)
  if (p->Cur != p->Lim)
    return *p->Cur++;
  return p->ReadByteFromNewBlock();
}

CByteInBufWrap::CByteInBufWrap() throw(): Buf(NULL)
{
  vt.Read = Wrap_ReadByte;
}



/* ---------- CByteOutBufWrap ---------- */

/*
void CLookToSequentialWrap::Free() throw()
{
  ::MidFree(BufBase);
  BufBase = NULL;
}

bool CLookToSequentialWrap::Alloc(UInt32 size) throw()
{
  if (!BufBase || size != Size)
  {
    Free();
    BufBase = (Byte *)::MidAlloc((size_t)size);
    Size = size;
  }
  return (BufBase != NULL);
}
*/

/*
EXTERN_C_BEGIN

void CLookToSequentialWrap_Look(ILookInSeqStreamPtr pp)
{
  CLookToSequentialWrap *p = (CLookToSequentialWrap *)pp->Obj;

  if (p->Extra || p->Res != S_OK)
    return;
  {
    UInt32 avail;
    p->Res = p->Stream->Read(p->BufBase, p->Size, &avail);
    p->Processed += avail;
    pp->Buf = p->BufBase;
    pp->Limit = pp->Buf + avail;
    if (avail == 0)
      p->Extra = true;
  }
}

EXTERN_C_END
*/


/* ---------- CByteOutBufWrap ---------- */

void CByteOutBufWrap::Free() throw()
{
  ::MidFree(Buf);
  Buf = NULL;
}

bool CByteOutBufWrap::Alloc(size_t size) throw()
{
  if (!Buf || size != Size)
  {
    Free();
    Buf = (Byte *)::MidAlloc(size);
    Size = size;
  }
  return (Buf != NULL);
}

HRESULT CByteOutBufWrap::Flush() throw()
{
  if (Res == S_OK)
  {
    const size_t size = (size_t)(Cur - Buf);
    Res = WriteStream(Stream, Buf, size);
    if (Res == S_OK)
      Processed += size;
    // else throw 11;
  }
  Cur = Buf; // reset pointer for later Wrap_WriteByte()
  return Res;
}

static void Wrap_WriteByte(IByteOutPtr pp, Byte b) throw()
{
  CByteOutBufWrap *p = Z7_CONTAINER_FROM_VTBL_CLS(pp, CByteOutBufWrap, vt);
  // Z7_CONTAINER_FROM_VTBL_TO_DECL_VAR_pp_vt_p(CByteOutBufWrap)
  Byte *dest = p->Cur;
  *dest = b;
  p->Cur = ++dest;
  if (dest == p->Lim)
    p->Flush();
}

CByteOutBufWrap::CByteOutBufWrap() throw(): Buf(NULL), Size(0)
{
  vt.Write = Wrap_WriteByte;
}


/* ---------- CLookOutWrap ---------- */

/*
void CLookOutWrap::Free() throw()
{
  ::MidFree(Buf);
  Buf = NULL;
}

bool CLookOutWrap::Alloc(size_t size) throw()
{
  if (!Buf || size != Size)
  {
    Free();
    Buf = (Byte *)::MidAlloc(size);
    Size = size;
  }
  return (Buf != NULL);
}

static size_t LookOutWrap_GetOutBuf(ILookOutStreamPtr pp, void **buf) throw()
{
  CLookOutWrap *p = Z7_CONTAINER_FROM_VTBL_CLS(pp, CLookOutWrap, vt);
  *buf = p->Buf;
  return p->Size;
}

static size_t LookOutWrap_Write(ILookOutStreamPtr pp, size_t size) throw()
{
  CLookOutWrap *p = Z7_CONTAINER_FROM_VTBL_CLS(pp, CLookOutWrap, vt);
  if (p->Res == S_OK && size != 0)
  {
    p->Res = WriteStream(p->Stream, p->Buf, size);
    if (p->Res == S_OK)
    {
      p->Processed += size;
      return size;
    }
  }
  return 0;
}

CLookOutWrap::CLookOutWrap() throw(): Buf(NULL), Size(0)
{
  vt.GetOutBuf = LookOutWrap_GetOutBuf;
  vt.Write = LookOutWrap_Write;
}
*/

/* ================ unit: CPP/7zip/Common/CreateCoder.cpp ================ */
// CreateCoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

static const unsigned kNumCodecsMax = 64;
extern
unsigned g_NumCodecs;
unsigned g_NumCodecs = 0;
extern
const CCodecInfo *g_Codecs[];
const CCodecInfo *g_Codecs[kNumCodecsMax];

// We use g_ExternalCodecs in other stages.
#ifdef Z7_EXTERNAL_CODECS
/*
extern CExternalCodecs g_ExternalCodecs;
#define CHECK_GLOBAL_CODECS \
    if (!_externalCodecs || !_externalCodecs->IsSet()) _externalCodecs = &g_ExternalCodecs;
*/
#define CHECK_GLOBAL_CODECS
#endif


void RegisterCodec(const CCodecInfo *codecInfo) throw()
{
  if (g_NumCodecs < kNumCodecsMax)
    g_Codecs[g_NumCodecs++] = codecInfo;
}

static const unsigned kNumHashersMax = 32;
extern
unsigned g_NumHashers;
unsigned g_NumHashers = 0;
extern
const CHasherInfo *g_Hashers[];
const CHasherInfo *g_Hashers[kNumHashersMax];

void RegisterHasher(const CHasherInfo *hashInfo) throw()
{
  if (g_NumHashers < kNumHashersMax)
    g_Hashers[g_NumHashers++] = hashInfo;
}


#ifdef Z7_EXTERNAL_CODECS

static HRESULT ReadNumberOfStreams(ICompressCodecsInfo *codecsInfo, UInt32 index, PROPID propID, UInt32 &res)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(codecsInfo->GetProperty(index, propID, &prop))
  if (prop.vt == VT_EMPTY)
    res = 1;
  else if (prop.vt == VT_UI4)
    res = prop.ulVal;
  else
    return E_INVALIDARG;
  return S_OK;
}

static HRESULT ReadIsAssignedProp(ICompressCodecsInfo *codecsInfo, UInt32 index, PROPID propID, bool &res)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(codecsInfo->GetProperty(index, propID, &prop))
  if (prop.vt == VT_EMPTY)
    res = true;
  else if (prop.vt == VT_BOOL)
    res = VARIANT_BOOLToBool(prop.boolVal);
  else
    return E_INVALIDARG;
  return S_OK;
}

HRESULT CExternalCodecs::Load()
{
  Codecs.Clear();
  Hashers.Clear();

  if (GetCodecs)
  {
    CCodecInfoEx info;
    
    UString s;
    UInt32 num;
    RINOK(GetCodecs->GetNumMethods(&num))
    
    for (UInt32 i = 0; i < num; i++)
    {
      NWindows::NCOM::CPropVariant prop;
      
      RINOK(GetCodecs->GetProperty(i, NMethodPropID::kID, &prop))
      if (prop.vt != VT_UI8)
        continue; // old Interface
      info.Id = prop.uhVal.QuadPart;
      
      prop.Clear();
      
      info.Name.Empty();
      RINOK(GetCodecs->GetProperty(i, NMethodPropID::kName, &prop))
      if (prop.vt == VT_BSTR)
        info.Name.SetFromWStr_if_Ascii(prop.bstrVal);
      else if (prop.vt != VT_EMPTY)
        continue;
      
      RINOK(ReadNumberOfStreams(GetCodecs, i, NMethodPropID::kPackStreams, info.NumStreams))
      {
        UInt32 numUnpackStreams = 1;
        RINOK(ReadNumberOfStreams(GetCodecs, i, NMethodPropID::kUnpackStreams, numUnpackStreams))
        if (numUnpackStreams != 1)
          continue;
      }
      RINOK(ReadIsAssignedProp(GetCodecs, i, NMethodPropID::kEncoderIsAssigned, info.EncoderIsAssigned))
      RINOK(ReadIsAssignedProp(GetCodecs, i, NMethodPropID::kDecoderIsAssigned, info.DecoderIsAssigned))
      RINOK(ReadIsAssignedProp(GetCodecs, i, NMethodPropID::kIsFilter, info.IsFilter))
      
      Codecs.Add(info);
    }
  }
  
  if (GetHashers)
  {
    UInt32 num = GetHashers->GetNumHashers();
    CHasherInfoEx info;
    
    for (UInt32 i = 0; i < num; i++)
    {
      NWindows::NCOM::CPropVariant prop;

      RINOK(GetHashers->GetHasherProp(i, NMethodPropID::kID, &prop))
      if (prop.vt != VT_UI8)
        continue;
      info.Id = prop.uhVal.QuadPart;
      
      prop.Clear();
      
      info.Name.Empty();
      RINOK(GetHashers->GetHasherProp(i, NMethodPropID::kName, &prop))
      if (prop.vt == VT_BSTR)
        info.Name.SetFromWStr_if_Ascii(prop.bstrVal);
      else if (prop.vt != VT_EMPTY)
        continue;
      
      Hashers.Add(info);
    }
  }
  
  return S_OK;
}

#endif


int FindMethod_Index(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const AString &name,
    bool encode,
    CMethodId &methodId,
    UInt32 &numStreams,
    bool &isFilter)
{
  unsigned i;
  for (i = 0; i < g_NumCodecs; i++)
  {
    const CCodecInfo &codec = *g_Codecs[i];
    if ((encode ? codec.CreateEncoder : codec.CreateDecoder)
        && StringsAreEqualNoCase_Ascii(name, codec.Name))
    {
      methodId = codec.Id;
      numStreams = codec.NumStreams;
      isFilter = codec.IsFilter;
      return (int)i;
    }
  }
  
  #ifdef Z7_EXTERNAL_CODECS
  
  CHECK_GLOBAL_CODECS

  if (_externalCodecs)
    for (i = 0; i < _externalCodecs->Codecs.Size(); i++)
    {
      const CCodecInfoEx &codec = _externalCodecs->Codecs[i];
      if ((encode ? codec.EncoderIsAssigned : codec.DecoderIsAssigned)
          && StringsAreEqualNoCase_Ascii(name, codec.Name))
      {
        methodId = codec.Id;
        numStreams = codec.NumStreams;
        isFilter = codec.IsFilter;
        return (int)(g_NumCodecs + i);
      }
    }
  
  #endif
  
  return -1;
}


static int FindMethod_Index(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode)
{
  unsigned i;
  for (i = 0; i < g_NumCodecs; i++)
  {
    const CCodecInfo &codec = *g_Codecs[i];
    if (codec.Id == methodId && (encode ? codec.CreateEncoder : codec.CreateDecoder))
      return (int)i;
  }
  
  #ifdef Z7_EXTERNAL_CODECS
  
  CHECK_GLOBAL_CODECS

  if (_externalCodecs)
    for (i = 0; i < _externalCodecs->Codecs.Size(); i++)
    {
      const CCodecInfoEx &codec = _externalCodecs->Codecs[i];
      if (codec.Id == methodId && (encode ? codec.EncoderIsAssigned : codec.DecoderIsAssigned))
        return (int)(g_NumCodecs + i);
    }
  
  #endif
  
  return -1;
}


bool FindMethod(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId,
    AString &name)
{
  name.Empty();
 
  unsigned i;
  for (i = 0; i < g_NumCodecs; i++)
  {
    const CCodecInfo &codec = *g_Codecs[i];
    if (methodId == codec.Id)
    {
      name = codec.Name;
      return true;
    }
  }
  
  #ifdef Z7_EXTERNAL_CODECS

  CHECK_GLOBAL_CODECS

  if (_externalCodecs)
    for (i = 0; i < _externalCodecs->Codecs.Size(); i++)
    {
      const CCodecInfoEx &codec = _externalCodecs->Codecs[i];
      if (methodId == codec.Id)
      {
        name = codec.Name;
        return true;
      }
    }
  
  #endif
  
  return false;
}

bool FindHashMethod(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const AString &name,
    CMethodId &methodId)
{
  unsigned i;
  for (i = 0; i < g_NumHashers; i++)
  {
    const CHasherInfo &codec = *g_Hashers[i];
    if (StringsAreEqualNoCase_Ascii(name, codec.Name))
    {
      methodId = codec.Id;
      return true;
    }
  }
  
  #ifdef Z7_EXTERNAL_CODECS

  CHECK_GLOBAL_CODECS

  if (_externalCodecs)
    for (i = 0; i < _externalCodecs->Hashers.Size(); i++)
    {
      const CHasherInfoEx &codec = _externalCodecs->Hashers[i];
      if (StringsAreEqualNoCase_Ascii(name, codec.Name))
      {
        methodId = codec.Id;
        return true;
      }
    }
  
  #endif
  
  return false;
}

void GetHashMethods(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CRecordVector<CMethodId> &methods)
{
  methods.ClearAndSetSize(g_NumHashers);
  unsigned i;
  for (i = 0; i < g_NumHashers; i++)
    methods[i] = (*g_Hashers[i]).Id;
  
  #ifdef Z7_EXTERNAL_CODECS
  
  CHECK_GLOBAL_CODECS

  if (_externalCodecs)
    for (i = 0; i < _externalCodecs->Hashers.Size(); i++)
      methods.Add(_externalCodecs->Hashers[i].Id);
  
  #endif
}



HRESULT CreateCoder_Index(
    DECL_EXTERNAL_CODECS_LOC_VARS
    unsigned i, bool encode,
    CMyComPtr<ICompressFilter> &filter,
    CCreatedCoder &cod)
{
  cod.IsExternal = false;
  cod.IsFilter = false;
  cod.NumStreams = 1;

  if (i < g_NumCodecs)
  {
    const CCodecInfo &codec = *g_Codecs[i];
    // if (codec.Id == methodId)
    {
      if (encode)
      {
        if (codec.CreateEncoder)
        {
          void *p = codec.CreateEncoder();
          if (codec.IsFilter) filter = (ICompressFilter *)p;
          else if (codec.NumStreams == 1) cod.Coder = (ICompressCoder *)p;
          else { cod.Coder2 = (ICompressCoder2 *)p; cod.NumStreams = codec.NumStreams; }
          return S_OK;
        }
      }
      else
        if (codec.CreateDecoder)
        {
          void *p = codec.CreateDecoder();
          if (codec.IsFilter) filter = (ICompressFilter *)p;
          else if (codec.NumStreams == 1) cod.Coder = (ICompressCoder *)p;
          else { cod.Coder2 = (ICompressCoder2 *)p; cod.NumStreams = codec.NumStreams; }
          return S_OK;
        }
    }
  }

  #ifdef Z7_EXTERNAL_CODECS

  CHECK_GLOBAL_CODECS
  
  if (_externalCodecs)
  {
    i -= g_NumCodecs;
    cod.IsExternal = true;
    if (i < _externalCodecs->Codecs.Size())
    {
      const CCodecInfoEx &codec = _externalCodecs->Codecs[i];
      // if (codec.Id == methodId)
      {
        if (encode)
        {
          if (codec.EncoderIsAssigned)
          {
            if (codec.NumStreams == 1)
            {
              const HRESULT res = _externalCodecs->GetCodecs->CreateEncoder(i, &IID_ICompressCoder, (void **)&cod.Coder);
              if (res != S_OK && res != E_NOINTERFACE && res != CLASS_E_CLASSNOTAVAILABLE)
                return res;
              if (cod.Coder)
                return res;
              return _externalCodecs->GetCodecs->CreateEncoder(i, &IID_ICompressFilter, (void **)&filter);
            }
            cod.NumStreams = codec.NumStreams;
            return _externalCodecs->GetCodecs->CreateEncoder(i, &IID_ICompressCoder2, (void **)&cod.Coder2);
          }
        }
        else
          if (codec.DecoderIsAssigned)
          {
            if (codec.NumStreams == 1)
            {
              const HRESULT res = _externalCodecs->GetCodecs->CreateDecoder(i, &IID_ICompressCoder, (void **)&cod.Coder);
              if (res != S_OK && res != E_NOINTERFACE && res != CLASS_E_CLASSNOTAVAILABLE)
                return res;
              if (cod.Coder)
                return res;
              return _externalCodecs->GetCodecs->CreateDecoder(i, &IID_ICompressFilter, (void **)&filter);
            }
            cod.NumStreams = codec.NumStreams;
            return _externalCodecs->GetCodecs->CreateDecoder(i, &IID_ICompressCoder2, (void **)&cod.Coder2);
          }
      }
    }
  }
  #endif

  return S_OK;
}


HRESULT CreateCoder_Index(
    DECL_EXTERNAL_CODECS_LOC_VARS
    unsigned index, bool encode,
    CCreatedCoder &cod)
{
  CMyComPtr<ICompressFilter> filter;
  const HRESULT res = CreateCoder_Index(
      EXTERNAL_CODECS_LOC_VARS
      index, encode,
      filter, cod);
  
  if (filter)
  {
    cod.IsFilter = true;
    CFilterCoder *coderSpec = new CFilterCoder(encode);
    cod.Coder = coderSpec;
    coderSpec->Filter = filter;
  }
  
  return res;
}


HRESULT CreateCoder_Id(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode,
    CMyComPtr<ICompressFilter> &filter,
    CCreatedCoder &cod)
{
  const int index = FindMethod_Index(EXTERNAL_CODECS_LOC_VARS methodId, encode);
  if (index < 0)
    return S_OK;
  return CreateCoder_Index(EXTERNAL_CODECS_LOC_VARS (unsigned)index, encode, filter, cod);
}


HRESULT CreateCoder_Id(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode,
    CCreatedCoder &cod)
{
  CMyComPtr<ICompressFilter> filter;
  const HRESULT res = CreateCoder_Id(
      EXTERNAL_CODECS_LOC_VARS
      methodId, encode,
      filter, cod);
  
  if (filter)
  {
    cod.IsFilter = true;
    CFilterCoder *coderSpec = new CFilterCoder(encode);
    cod.Coder = coderSpec;
    coderSpec->Filter = filter;
  }
  
  return res;
}


HRESULT CreateCoder_Id(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode,
    CMyComPtr<ICompressCoder> &coder)
{
  CCreatedCoder cod;
  const HRESULT res = CreateCoder_Id(
      EXTERNAL_CODECS_LOC_VARS
      methodId, encode,
      cod);
  coder = cod.Coder;
  return res;
}

HRESULT CreateFilter(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId, bool encode,
    CMyComPtr<ICompressFilter> &filter)
{
  CCreatedCoder cod;
  return CreateCoder_Id(
      EXTERNAL_CODECS_LOC_VARS
      methodId, encode,
      filter, cod);
}


HRESULT CreateHasher(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CMethodId methodId,
    AString &name,
    CMyComPtr<IHasher> &hasher)
{
  name.Empty();

  unsigned i;
  for (i = 0; i < g_NumHashers; i++)
  {
    const CHasherInfo &codec = *g_Hashers[i];
    if (codec.Id == methodId)
    {
      hasher = codec.CreateHasher();
      name = codec.Name;
      break;
    }
  }

  #ifdef Z7_EXTERNAL_CODECS

  CHECK_GLOBAL_CODECS

  if (!hasher && _externalCodecs)
    for (i = 0; i < _externalCodecs->Hashers.Size(); i++)
    {
      const CHasherInfoEx &codec = _externalCodecs->Hashers[i];
      if (codec.Id == methodId)
      {
        name = codec.Name;
        return _externalCodecs->GetHashers->CreateHasher((UInt32)i, &hasher);
      }
    }

  #endif

  return S_OK;
}

/* ================ unit: CPP/7zip/Common/FilterCoder.cpp ================ */
// FilterCoder.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef _WIN32
  #define alignedMidBuffer_Alloc g_MidAlloc
#else
  #define alignedMidBuffer_Alloc g_AlignedAlloc
#endif

CAlignedMidBuffer::~CAlignedMidBuffer()
{
  ISzAlloc_Free(&alignedMidBuffer_Alloc, _buf);
}

void CAlignedMidBuffer::AllocAligned(size_t size)
{
  ISzAlloc_Free(&alignedMidBuffer_Alloc, _buf);
  _buf = (Byte *)ISzAlloc_Alloc(&alignedMidBuffer_Alloc, size);
}

/*
  AES filters need 16-bytes alignment for HARDWARE-AES instructions.
  So we call IFilter::Filter(, size), where (size != 16 * N) only for last data block.

  AES-CBC filters need data size aligned for 16-bytes.
  So the encoder can add zeros to the end of original stream.

  Some filters (BCJ and others) don't process data at the end of stream in some cases.
  So the encoder and decoder write such last bytes without change.

  Most filters process all data, if we send aligned size to filter.
     But  BCJ filter can process up 4 bytes less than sent size.
     And ARMT filter can process    2 bytes less than sent size.
*/


static const UInt32 kBufSize = 1 << 21;

Z7_COM7F_IMF(CFilterCoder::SetInBufSize(UInt32 , UInt32 size)) { _inBufSize = size; return S_OK; }
Z7_COM7F_IMF(CFilterCoder::SetOutBufSize(UInt32 , UInt32 size)) { _outBufSize = size; return S_OK; }

HRESULT CFilterCoder::Alloc()
{
  UInt32 size = MyMin(_inBufSize, _outBufSize);
  /* minimal bufSize is 16 bytes for AES and IA64 filter.
     bufSize for AES must be aligned for 16 bytes.
     We use (1 << 12) min size to support future aligned filters. */
  const UInt32 kMinSize = 1 << 12;
  size &= ~(UInt32)(kMinSize - 1);
  if (size < kMinSize)
    size = kMinSize;
  // size = (1 << 12); // + 117; // for debug
  if (!_buf || _bufSize != size)
  {
    AllocAligned(size);
    if (!_buf)
      return E_OUTOFMEMORY;
    _bufSize = size;
  }
  return S_OK;
}

HRESULT CFilterCoder::Init_and_Alloc()
{
  RINOK(Filter->Init())
  return Alloc();
}

CFilterCoder::CFilterCoder(bool encodeMode):
    _bufSize(0),
    _inBufSize(kBufSize),
    _outBufSize(kBufSize),
    _encodeMode(encodeMode),
    _outSize_Defined(false),
    _outSize(0),
    _nowPos64(0)
  {}


Z7_COM7F_IMF(CFilterCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  RINOK(Init_and_Alloc())

  /*
     It's expected that BCJ/ARMT filter can process up to 4 bytes less
     than sent data size. For such BCJ/ARMT cases with non-filtered data we:
       - write some filtered data to output stream
       - move non-written data (filtered and non-filtered data) to start of buffer
       - read more new data from input stream to position after end of non-filtered data
       - call Filter() for concatenated data in buffer.

     For all cases, even for cases with partial filtering (BCJ/ARMT),
     we try to keep real/virtual alignment for all operations
       (memmove, Read(), Filter(), Write()).
     We use (kAlignSize=64) alignmnent that is larger than (16-bytes)
     required for AES filter alignment.

     AES-CBC uses 16-bytes blocks, that is simple case for processing here,
     if we call Filter() for aligned size for all calls except of last call (last block).
     And now there are no filters that use blocks with non-power2 size,
     but we try to support such non-power2 filters too here at Code().
  */
    
  UInt64 prev = 0;
  UInt64 nowPos64 = 0;
  bool inputFinished = false;
  UInt32 readPos = 0;
  UInt32 filterPos = 0;

  while (!outSize || nowPos64 < *outSize)
  {
    HRESULT hres = S_OK;
    if (!inputFinished)
    {
      size_t processedSize = _bufSize - readPos;
      /* for AES filters we need at least max(16, kAlignSize) bytes in buffer.
         But we try to read full buffer to reduce the number of Filter() and Write() calls.
      */
      hres = ReadStream(inStream, _buf + readPos, &processedSize);
      readPos += (UInt32)processedSize;
      inputFinished = (readPos != _bufSize);
      if (hres != S_OK)
      {
        // do we need to stop encoding after reading error?
        // if (_encodeMode) return hres;
        inputFinished = true;
      }
    }

    if (readPos == 0)
      return hres;

    /* we set (needMoreInput = true), if it's block-filter (like AES-CBC)
         that needs more data for current block filtering:
       We read full input buffer with Read(), and _bufSize is aligned,
       So the possible cases when we set (needMoreInput = true) are:
         1) decode : filter needs more data after the end of input stream.
           another cases are possible for non-power2-block-filter,
           because buffer size is not aligned for filter_non_power2_block_size:
         2) decode/encode : filter needs more data from non-finished input stream
         3) encode        : filter needs more space for zeros after the end of input stream
    */
    bool needMoreInput = false;

    while (readPos != filterPos)
    {
      /* Filter() is allowed to process part of data.
         Here we use the loop to filter as max as possible.
         when we call Filter(data, size):
         if (size < 16), AES-CTR filter uses internal 16-byte buffer.
         new (since v23.00) AES-CTR filter allows (size < 16) for non-last block,
         but it will work less efficiently than calls with aligned (size).
         We still support old (before v23.00) AES-CTR filters here.
         We have aligned (size) for AES-CTR, if it's not last block.
         We have aligned (readPos) for any filter, if (!inputFinished).
         We also meet the requirements for (data) pointer in Filter() call:
         {
           (virtual_stream_offset % aligment_size) == (data_ptr % aligment_size)
           (aligment_size == 2^N)
           (aligment_size  >= 16)
         }
      */
      const UInt32 cur = Filter->Filter(_buf + filterPos, readPos - filterPos);
      if (cur == 0)
        break;
      const UInt32 f = filterPos + cur;
      if (cur > readPos - filterPos)
      {
        // AES-CBC
        if (hres != S_OK)
          break;

        if (!_encodeMode
            || cur > _bufSize - filterPos
            || !inputFinished)
        {
          /* (cur > _bufSize - filterPos) is unexpected for AES filter, if _bufSize is multiply of 16.
             But we support this case, if some future filter will use block with non-power2-size.
          */
          needMoreInput = true;
          break;
        }

        /* (_encodeMode && inputFinished).
           We add zero bytes as pad in current block after the end of read data. */
        Byte *buf = _buf;
        do
          buf[readPos] = 0;
        while (++readPos != f);
        // (readPos) now is (size_of_real_input_data + size_of_zero_pad)
        if (cur != Filter->Filter(buf + filterPos, cur))
          return E_FAIL;
      }
      filterPos = f;
    }

    UInt32 size = filterPos;
    if (hres == S_OK)
    {
      /* If we need more Read() or Filter() calls, then we need to Write()
         some data and move unwritten data to get additional space in buffer.
         We try to keep alignment for data moves, Read(), Filter() and Write() calls.
      */
      const UInt32 kAlignSize = 1 << 6;
      const UInt32 alignedFiltered = filterPos & ~(kAlignSize - 1);
      if (inputFinished)
      {
        if (!needMoreInput)
          size = readPos; // for risc/bcj filters in last block we write data after filterPos.
        else if (_encodeMode)
          size = alignedFiltered; // for non-power2-block-encode-filter
      }
      else
        size = alignedFiltered;
    }

    {
      UInt32 writeSize = size;
      if (outSize)
      {
        const UInt64 rem = *outSize - nowPos64;
        if (writeSize > rem)
          writeSize = (UInt32)rem;
      }
      RINOK(WriteStream(outStream, _buf, writeSize))
      nowPos64 += writeSize;
    }

    if (hres != S_OK)
      return hres;

    if (inputFinished)
    {
      if (readPos == size)
        return hres;
      if (!_encodeMode)
      {
        // block-decode-filter (AES-CBS) has non-full last block
        // we don't want unaligned data move for more iterations with this error case.
        return S_FALSE;
      }
    }

    if (size == 0)
    {
      // it's unexpected that we have no any move in this iteration.
      return E_FAIL;
    }
    // if (size != 0)
    {
      if (filterPos < size)
        return E_FAIL; // filterPos = 0; else
      filterPos -= size;
      readPos -= size;
      if (readPos != 0)
        memmove(_buf, _buf + size, readPos);
    }
    // printf("\nnowPos64=%x, readPos=%x, filterPos=%x\n", (unsigned)nowPos64, (unsigned)readPos, (unsigned)filterPos);

    if (progress && (nowPos64 - prev) >= (1 << 22))
    {
      prev = nowPos64;
      RINOK(progress->SetRatioInfo(&nowPos64, &nowPos64))
    }
  }

  return S_OK;
}



// ---------- Write to Filter ----------

Z7_COM7F_IMF(CFilterCoder::SetOutStream(ISequentialOutStream *outStream))
{
  _outStream = outStream;
  return S_OK;
}

Z7_COM7F_IMF(CFilterCoder::ReleaseOutStream())
{
  _outStream.Release();
  return S_OK;
}

HRESULT CFilterCoder::Flush2()
{
  while (_convSize != 0)
  {
    UInt32 num = _convSize;
    if (_outSize_Defined)
    {
      const UInt64 rem = _outSize - _nowPos64;
      if (num > rem)
        num = (UInt32)rem;
      if (num == 0)
        return k_My_HRESULT_WritingWasCut;
    }
    
    UInt32 processed = 0;
    const HRESULT res = _outStream->Write(_buf + _convPos, num, &processed);
    if (processed == 0)
      return res != S_OK ? res : E_FAIL;
    
    _convPos += processed;
    _convSize -= processed;
    _nowPos64 += processed;
    RINOK(res)
  }
    
  const UInt32 convPos = _convPos;
  if (convPos != 0)
  {
    const UInt32 num = _bufPos - convPos;
    Byte *buf = _buf;
    for (UInt32 i = 0; i < num; i++)
      buf[i] = buf[convPos + i];
    _bufPos = num;
    _convPos = 0;
  }
    
  return S_OK;
}

Z7_COM7F_IMF(CFilterCoder::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  
  while (size != 0)
  {
    RINOK(Flush2())

    // _convSize is 0
    // _convPos is 0
    // _bufPos is small

    if (_bufPos != _bufSize)
    {
      UInt32 num = MyMin(size, _bufSize - _bufPos);
      memcpy(_buf + _bufPos, data, num);
      size -= num;
      data = (const Byte *)data + num;
      if (processedSize)
        *processedSize += num;
      _bufPos += num;
      if (_bufPos != _bufSize)
        continue;
    }

    // _bufPos == _bufSize
    _convSize = Filter->Filter(_buf, _bufPos);
    
    if (_convSize == 0)
      break;
    if (_convSize > _bufPos)
    {
      // that case is not possible.
      _convSize = 0;
      return E_FAIL;
    }
  }

  return S_OK;
}

Z7_COM7F_IMF(CFilterCoder::OutStreamFinish())
{
  for (;;)
  {
    RINOK(Flush2())
    if (_bufPos == 0)
      break;
    const UInt32 convSize = Filter->Filter(_buf, _bufPos);
    _convSize = convSize;
    UInt32 bufPos = _bufPos;
    if (convSize == 0)
      _convSize = bufPos;
    else if (convSize > bufPos)
    {
      // AES
      if (convSize > _bufSize)
      {
        _convSize = 0;
        return E_FAIL;
      }
      if (!_encodeMode)
      {
        _convSize = 0;
        return S_FALSE;
      }
      Byte *buf = _buf;
      for (; bufPos < convSize; bufPos++)
        buf[bufPos] = 0;
      _bufPos = bufPos;
      _convSize = Filter->Filter(_buf, bufPos);
      if (_convSize != _bufPos)
        return E_FAIL;
    }
  }
  
  CMyComPtr<IOutStreamFinish> finish;
  _outStream.QueryInterface(IID_IOutStreamFinish, &finish);
  if (finish)
    return finish->OutStreamFinish();
  return S_OK;
}

// ---------- Init functions ----------

Z7_COM7F_IMF(CFilterCoder::InitEncoder())
{
  InitSpecVars();
  return Init_and_Alloc();
}

HRESULT CFilterCoder::Init_NoSubFilterInit()
{
  InitSpecVars();
  return Alloc();
}

Z7_COM7F_IMF(CFilterCoder::SetOutStreamSize(const UInt64 *outSize))
{
  InitSpecVars();
  if (outSize)
  {
    _outSize = *outSize;
    _outSize_Defined = true;
  }
  return Init_and_Alloc();
}

// ---------- Read from Filter ----------

Z7_COM7F_IMF(CFilterCoder::SetInStream(ISequentialInStream *inStream))
{
  _inStream = inStream;
  return S_OK;
}

Z7_COM7F_IMF(CFilterCoder::ReleaseInStream())
{
  _inStream.Release();
  return S_OK;
}


Z7_COM7F_IMF(CFilterCoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  
  while (size != 0)
  {
    if (_convSize != 0)
    {
      if (size > _convSize)
        size = _convSize;
      if (_outSize_Defined)
      {
        const UInt64 rem = _outSize - _nowPos64;
        if (size > rem)
          size = (UInt32)rem;
      }
      memcpy(data, _buf + _convPos, size);
      _convPos += size;
      _convSize -= size;
      _nowPos64 += size;
      if (processedSize)
        *processedSize = size;
      break;
    }
  
    const UInt32 convPos = _convPos;
    if (convPos != 0)
    {
      const UInt32 num = _bufPos - convPos;
      Byte *buf = _buf;
      for (UInt32 i = 0; i < num; i++)
        buf[i] = buf[convPos + i];
      _bufPos = num;
      _convPos = 0;
    }
    
    {
      size_t readSize = _bufSize - _bufPos;
      const HRESULT res = ReadStream(_inStream, _buf + _bufPos, &readSize);
      _bufPos += (UInt32)readSize;
      RINOK(res)
    }
    
    const UInt32 convSize = Filter->Filter(_buf, _bufPos);
    _convSize = convSize;
    
    UInt32 bufPos = _bufPos;

    if (convSize == 0)
    {
      if (bufPos == 0)
        break;
      // BCJ
      _convSize = bufPos;
      continue;
    }
    
    if (convSize > bufPos)
    {
      // AES
      if (convSize > _bufSize)
        return E_FAIL;
      if (!_encodeMode)
        return S_FALSE;
      Byte *buf = _buf;
      do
        buf[bufPos] = 0;
      while (++bufPos != convSize);
      _bufPos = bufPos;
      _convSize = Filter->Filter(_buf, convSize);
      if (_convSize != _bufPos)
        return E_FAIL;
    }
  }
 
  return S_OK;
}


#ifndef Z7_NO_CRYPTO

Z7_COM7F_IMF(CFilterCoder::CryptoSetPassword(const Byte *data, UInt32 size))
  { return _setPassword->CryptoSetPassword(data, size); }

Z7_COM7F_IMF(CFilterCoder::SetKey(const Byte *data, UInt32 size))
  { return _cryptoProperties->SetKey(data, size); }

Z7_COM7F_IMF(CFilterCoder::SetInitVector(const Byte *data, UInt32 size))
  { return _cryptoProperties->SetInitVector(data, size); }

#endif


#ifndef Z7_EXTRACT_ONLY

Z7_COM7F_IMF(CFilterCoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *properties, UInt32 numProperties))
  { return _setCoderProperties->SetCoderProperties(propIDs, properties, numProperties); }

Z7_COM7F_IMF(CFilterCoder::WriteCoderProperties(ISequentialOutStream *outStream))
  { return _writeCoderProperties->WriteCoderProperties(outStream); }

Z7_COM7F_IMF(CFilterCoder::SetCoderPropertiesOpt(const PROPID *propIDs,
    const PROPVARIANT *properties, UInt32 numProperties))
  { return _setCoderPropertiesOpt->SetCoderPropertiesOpt(propIDs, properties, numProperties); }

/*
Z7_COM7F_IMF(CFilterCoder::ResetSalt()
  { return _cryptoResetSalt->ResetSalt(); }
*/

Z7_COM7F_IMF(CFilterCoder::ResetInitVector())
  { return _cryptoResetInitVector->ResetInitVector(); }

#endif


Z7_COM7F_IMF(CFilterCoder::SetDecoderProperties2(const Byte *data, UInt32 size))
  { return _setDecoderProperties2->SetDecoderProperties2(data, size); }

/* ================ unit: CPP/7zip/Common/InBuffer.cpp ================ */
// InBuffer.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

CInBufferBase::CInBufferBase() throw():
  _buf(NULL),
  _bufLim(NULL),
  _bufBase(NULL),
  _stream(NULL),
  _processedSize(0),
  _bufSize(0),
  _wasFinished(false),
  NumExtraBytes(0)
{}

bool CInBuffer::Create(size_t bufSize) throw()
{
  const unsigned kMinBlockSize = 1;
  if (bufSize < kMinBlockSize)
    bufSize = kMinBlockSize;
  if (_bufBase != NULL && _bufSize == bufSize)
    return true;
  Free();
  _bufSize = bufSize;
  _bufBase = (Byte *)::MidAlloc(bufSize);
  return (_bufBase != NULL);
}

void CInBuffer::Free() throw()
{
  ::MidFree(_bufBase);
  _bufBase = NULL;
}

void CInBufferBase::Init() throw()
{
  _processedSize = 0;
  _buf = _bufBase;
  _bufLim = _buf;
  _wasFinished = false;
  #ifdef Z7_NO_EXCEPTIONS
  ErrorCode = S_OK;
  #endif
  NumExtraBytes = 0;
}

bool CInBufferBase::ReadBlock()
{
  #ifdef Z7_NO_EXCEPTIONS
  if (ErrorCode != S_OK)
    return false;
  #endif
  if (_wasFinished)
    return false;
  _processedSize += (size_t)(_buf - _bufBase);
  _buf = _bufBase;
  _bufLim = _bufBase;
  UInt32 processed;
  // FIX_ME: we can improve it to support (_bufSize >= (1 << 32))
  const HRESULT result = _stream->Read(_bufBase, (UInt32)_bufSize, &processed);
  #ifdef Z7_NO_EXCEPTIONS
  ErrorCode = result;
  #else
  if (result != S_OK)
    throw CInBufferException(result);
  #endif
  _bufLim = _buf + processed;
  _wasFinished = (processed == 0);
  return !_wasFinished;
}

bool CInBufferBase::ReadByte_FromNewBlock(Byte &b)
{
  if (!ReadBlock())
  {
    // 22.00: we don't increment (NumExtraBytes) here
    // NumExtraBytes++;
    b = 0xFF;
    return false;
  }
  b = *_buf++;
  return true;
}

Byte CInBufferBase::ReadByte_FromNewBlock()
{
  if (!ReadBlock())
  {
    NumExtraBytes++;
    return 0xFF;
  }
  return *_buf++;
}

size_t CInBufferBase::ReadBytesPart(Byte *buf, size_t size)
{
  if (size == 0)
    return 0;
  size_t rem = (size_t)(_bufLim - _buf);
  if (rem == 0)
  {
    if (!ReadBlock())
      return 0;
    rem = (size_t)(_bufLim - _buf);
  }
  if (size > rem)
      size = rem;
  memcpy(buf, _buf, size);
  _buf += size;
  return size;
}

size_t CInBufferBase::ReadBytes(Byte *buf, size_t size)
{
  size_t num = 0;
  for (;;)
  {
    const size_t rem = (size_t)(_bufLim - _buf);
    if (size <= rem)
    {
      if (size != 0)
      {
        memcpy(buf, _buf, size);
        _buf += size;
        num += size;
      }
      return num;
    }
    if (rem != 0)
    {
      memcpy(buf, _buf, rem);
      _buf += rem;
      buf += rem;
      num += rem;
      size -= rem;
    }
    if (!ReadBlock())
      return num;
  }

  /*
  if ((size_t)(_bufLim - _buf) >= size)
  {
    const Byte *src = _buf;
    for (size_t i = 0; i < size; i++)
      buf[i] = src[i];
    _buf += size;
    return size;
  }
  for (size_t i = 0; i < size; i++)
  {
    if (_buf >= _bufLim)
      if (!ReadBlock())
        return i;
    buf[i] = *_buf++;
  }
  return size;
  */
}

size_t CInBufferBase::Skip(size_t size)
{
  size_t processed = 0;
  for (;;)
  {
    const size_t rem = (size_t)(_bufLim - _buf);
    if (rem >= size)
    {
      _buf += size;
      return processed + size;
    }
    _buf += rem;
    processed += rem;
    size -= rem;
    if (!ReadBlock())
      return processed;
  }
}

/* ================ unit: CPP/7zip/Common/InOutTempBuffer.cpp ================ */
// InOutTempBuffer.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#ifdef USE_InOutTempBuffer_FILE

// amalgamation: header emitted in prologue

#define kTempFilePrefixString FTEXT("7zt")
/*
  Total buffer size limit, if we use temp file scheme:
    32-bit:  16 MiB = 1 MiB *   16 buffers
    64-bit:   4 GiB = 1 MiB * 4096 buffers
*/
static const size_t kNumBufsMax = (size_t)1 << (sizeof(size_t) * 2 - 4);

#endif

static const size_t kInOutTempBuf_kBufSize = (size_t)1 << 20;


CInOutTempBuffer::CInOutTempBuffer():
    _size(0),
    _bufs(NULL),
    _numBufs(0),
    _numFilled(0)
{
 #ifdef USE_InOutTempBuffer_FILE
  _tempFile_Created = false;
  _useMemOnly = false;
  _crc = CRC_INIT_VAL;
 #endif
}

CInOutTempBuffer::~CInOutTempBuffer()
{
  for (size_t i = 0; i < _numBufs; i++)
    MyFree(_bufs[i]);
  MyFree(_bufs);
}


void *CInOutTempBuffer::GetBuf(size_t index)
{
  if (index >= _numBufs)
  {
    const size_t num = (_numBufs == 0 ? 16 : _numBufs * 2);
    void **p = (void **)MyRealloc(_bufs, num * sizeof(void *));
    if (!p)
      return NULL;
    _bufs = p;
    memset(p + _numBufs, 0, (num - _numBufs) * sizeof(void *));
    _numBufs = num;
  }
  
  void *buf = _bufs[index];
  if (!buf)
  {
    buf = MyAlloc(kInOutTempBuf_kBufSize);
    if (buf)
      _bufs[index] = buf;
  }
  return buf;
}


HRESULT CInOutTempBuffer::Write_HRESULT(const void *data, UInt32 size)
{
  if (size == 0)
    return S_OK;
  
 #ifdef USE_InOutTempBuffer_FILE
  if (!_tempFile_Created)
 #endif
  for (;;)  // loop for additional attemp to allocate memory after file creation error
  {
   #ifdef USE_InOutTempBuffer_FILE
    bool allocError = false;
   #endif
  
    for (;;)  // loop for writing to buffers
    {
      const size_t index = (size_t)(_size / kInOutTempBuf_kBufSize);
      
     #ifdef USE_InOutTempBuffer_FILE
      if (index >= kNumBufsMax && !_useMemOnly)
        break;
     #endif
    
      void *buf = GetBuf(index);
      if (!buf)
      {
       #ifdef USE_InOutTempBuffer_FILE
        if (!_useMemOnly)
        {
          allocError = true;
          break;
        }
       #endif
        return E_OUTOFMEMORY;
      }
      
      const size_t offset = (size_t)(_size) & (kInOutTempBuf_kBufSize - 1);
      size_t cur = kInOutTempBuf_kBufSize - offset;
      if (cur > size)
        cur = size;
      memcpy((Byte *)buf + offset, data, cur);
      _size += cur;
      if (index >= _numFilled)
        _numFilled = index + 1;
      data = (const void *)((const Byte *)data + cur);
      size -= (UInt32)cur;
      if (size == 0)
        return S_OK;
    }

   #ifdef USE_InOutTempBuffer_FILE
   #ifndef _WIN32
    _outFile.mode_for_Create = 0600;  // only owner will have the rights to access this file
   #endif
    if (_tempFile.CreateRandomInTempFolder(kTempFilePrefixString, &_outFile))
    {
      _tempFile_Created = true;
      break;
    }
    _useMemOnly = true;
    if (allocError)
      return GetLastError_noZero_HRESULT();
   #endif
  }

 #ifdef USE_InOutTempBuffer_FILE
  if (!_outFile.WriteFull(data, size))
    return GetLastError_noZero_HRESULT();
  _crc = CrcUpdate(_crc, data, size);
  _size += size;
  return S_OK;
 #endif
}


HRESULT CInOutTempBuffer::WriteToStream(ISequentialOutStream *stream)
{
  UInt64 rem = _size;
  // if (rem == 0) return S_OK;

  const size_t numFilled = _numFilled;
  _numFilled = 0;

  for (size_t i = 0; i < numFilled; i++)
  {
    if (rem == 0)
      return E_FAIL;
    size_t cur = kInOutTempBuf_kBufSize;
    if (cur > rem)
      cur = (size_t)rem;
    RINOK(WriteStream(stream, _bufs[i], cur))
    rem -= cur;
   #ifdef USE_InOutTempBuffer_FILE
    // we will use _bufs[0] later for writing from temp file
    if (i != 0 || !_tempFile_Created)
   #endif
    {
      MyFree(_bufs[i]);
      _bufs[i] = NULL;
    }
  }


 #ifdef USE_InOutTempBuffer_FILE

  if (rem == 0)
    return _tempFile_Created ? E_FAIL : S_OK;

  if (!_tempFile_Created)
    return E_FAIL;

  if (!_outFile.Close())
    return GetLastError_noZero_HRESULT();

  HRESULT hres;
  void *buf = GetBuf(0); // index
  if (!buf)
    hres = E_OUTOFMEMORY;
  else
  {
    NWindows::NFile::NIO::CInFile inFile;
    if (!inFile.Open(_tempFile.GetPath()))
      hres = GetLastError_noZero_HRESULT();
    else
    {
      UInt32 crc = CRC_INIT_VAL;
      for (;;)
      {
        size_t processed;
        if (!inFile.ReadFull(buf, kInOutTempBuf_kBufSize, processed))
        {
          hres = GetLastError_noZero_HRESULT();
          break;
        }
        if (processed == 0)
        {
          // we compare crc without CRC_GET_DIGEST
          hres = (_crc == crc ? S_OK : E_FAIL);
          break;
        }
        size_t n = processed;
        if (n > rem)
          n = (size_t)rem;
        hres = WriteStream(stream, buf, n);
        if (hres != S_OK)
          break;
        crc = CrcUpdate(crc, buf, n);
        rem -= n;
        if (n != processed)
        {
          hres = E_FAIL;
          break;
        }
      }
    }
  }

  // _tempFile.DisableDeleting(); // for debug
  _tempFile.Remove();
  RINOK(hres)

 #endif

  return rem == 0 ? S_OK : E_FAIL;
}

/* ================ unit: CPP/7zip/Common/LimitedStreams.cpp ================ */
// LimitedStreams.cpp

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue

Z7_COM7F_IMF(CLimitedSequentialInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 realProcessedSize = 0;
  {
    const UInt64 rem = _size - _pos;
    if (size > rem)
      size = (UInt32)rem;
  }
  HRESULT result = S_OK;
  if (size != 0)
  {
    result = _stream->Read(data, size, &realProcessedSize);
    _pos += realProcessedSize;
    if (realProcessedSize == 0)
      _wasFinished = true;
  }
  if (processedSize)
    *processedSize = realProcessedSize;
  return result;
}

Z7_COM7F_IMF(CLimitedInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= _size)
  {
    // 9.31: Fixed. Windows doesn't return error in ReadFile and IStream->Read in that case.
    return S_OK;
    // return (_virtPos == _size) ? S_OK: E_FAIL; // ERROR_HANDLE_EOF
  }
  {
    const UInt64 rem = _size - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
  }
  UInt64 newPos = _startOffset + _virtPos;
  if (newPos != _physPos)
  {
    _physPos = newPos;
    RINOK(SeekToPhys())
  }
  HRESULT res = _stream->Read(data, size, &size);
  if (processedSize)
    *processedSize = size;
  _physPos += size;
  _virtPos += size;
  return res;
}

Z7_COM7F_IMF(CLimitedInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += _size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

HRESULT CreateLimitedInStream(IInStream *inStream, UInt64 pos, UInt64 size, ISequentialInStream **resStream)
{
  *resStream = NULL;
  CLimitedInStream *streamSpec = new CLimitedInStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  streamSpec->SetStream(inStream);
  RINOK(streamSpec->InitAndSeek(pos, size))
  streamSpec->SeekToStart();
  *resStream = streamTemp.Detach();
  return S_OK;
}

Z7_COM7F_IMF(CClusterInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= Size)
    return S_OK;
  {
    UInt64 rem = Size - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
  }
  if (size == 0)
    return S_OK;

  if (_curRem == 0)
  {
    const UInt32 blockSize = (UInt32)1 << BlockSizeLog;
    const UInt32 virtBlock = (UInt32)(_virtPos >> BlockSizeLog);
    const UInt32 offsetInBlock = (UInt32)_virtPos & (blockSize - 1);
    const UInt32 phyBlock = Vector[virtBlock];
    
    UInt64 newPos = StartOffset + ((UInt64)phyBlock << BlockSizeLog) + offsetInBlock;
    if (newPos != _physPos)
    {
      _physPos = newPos;
      RINOK(SeekToPhys())
    }

    _curRem = blockSize - offsetInBlock;
    
    for (unsigned i = 1; i < 64 && (virtBlock + i) < (UInt32)Vector.Size() && phyBlock + i == Vector[virtBlock + i]; i++)
      _curRem += (UInt32)1 << BlockSizeLog;
  }
  
  if (size > _curRem)
    size = _curRem;
  HRESULT res = Stream->Read(data, size, &size);
  if (processedSize)
    *processedSize = size;
  _physPos += size;
  _virtPos += size;
  _curRem -= size;
  return res;
}
 
Z7_COM7F_IMF(CClusterInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
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
  if (_virtPos != (UInt64)offset)
    _curRem = 0;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = (UInt64)offset;
  return S_OK;
}


Z7_COM7F_IMF(CExtentsStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  const UInt64 virt = _virtPos;
  if (virt >= Extents.Back().Virt)
    return S_OK;
  if (size == 0)
    return S_OK;
  
  unsigned left = _prevExtentIndex;
  if (virt <  Extents[left].Virt ||
      virt >= Extents[left + 1].Virt)
  {
    left = 0;
    unsigned right = Extents.Size() - 1;
    for (;;)
    {
      const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      if (mid == left)
        break;
      if (virt < Extents[mid].Virt)
        right = mid;
      else
        left = mid;
    }
    _prevExtentIndex = left;
  }
  
  {
    const UInt64 rem = Extents[left + 1].Virt - virt;
    if (size > rem)
      size = (UInt32)rem;
  }
  
  const CSeekExtent &extent = Extents[left];
  
  if (extent.Is_ZeroFill())
  {
    memset(data, 0, size);
    _virtPos += size;
    if (processedSize)
      *processedSize = size;
    return S_OK;
  }

  {
    const UInt64 phy = extent.Phy + (virt - extent.Virt);
    if (_phyPos != phy)
    {
      _phyPos = (UInt64)0 - 1;  // we don't trust seek_pos in case of error
      RINOK(InStream_SeekSet(Stream, phy))
      _phyPos = phy;
    }
  }
  
  const HRESULT res = Stream->Read(data, size, &size);
  _virtPos += size;
  if (res == S_OK)
    _phyPos += size;
  else
    _phyPos = (UInt64)0 - 1;  // we don't trust seek_pos in case of error
  if (processedSize)
    *processedSize = size;
  return res;
}


Z7_COM7F_IMF(CExtentsStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += Extents.Back().Virt; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}


Z7_COM7F_IMF(CLimitedSequentialOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  HRESULT result = S_OK;
  if (processedSize)
    *processedSize = 0;
  if (size > _size)
  {
    if (_size == 0)
    {
      _overflow = true;
      if (!_overflowIsAllowed)
        return E_FAIL;
      if (processedSize)
        *processedSize = size;
      return S_OK;
    }
    size = (UInt32)_size;
  }
  if (_stream)
    result = _stream->Write(data, size, &size);
  _size -= size;
  if (processedSize)
    *processedSize = size;
  return result;
}


Z7_COM7F_IMF(CTailInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 cur;
  HRESULT res = Stream->Read(data, size, &cur);
  if (processedSize)
    *processedSize = cur;
  _virtPos += cur;
  return res;
}
  
Z7_COM7F_IMF(CTailInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END:
    {
      UInt64 pos = 0;
      RINOK(Stream->Seek(offset, STREAM_SEEK_END, &pos))
      if (pos < Offset)
        return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
      _virtPos = pos - Offset;
      if (newPosition)
        *newPosition = _virtPos;
      return S_OK;
    }
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = _virtPos;
  return InStream_SeekSet(Stream, Offset + _virtPos);
}

Z7_COM7F_IMF(CLimitedCachedInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= _size)
  {
    // 9.31: Fixed. Windows doesn't return error in ReadFile and IStream->Read in that case.
    return S_OK;
    // return (_virtPos == _size) ? S_OK: E_FAIL; // ERROR_HANDLE_EOF
  }
  UInt64 rem = _size - _virtPos;
  if (rem < size)
    size = (UInt32)rem;

  UInt64 newPos = _startOffset + _virtPos;
  UInt64 offsetInCache = newPos - _cachePhyPos;
  HRESULT res = S_OK;
  if (newPos >= _cachePhyPos &&
      offsetInCache <= _cacheSize &&
      size <= _cacheSize - (size_t)offsetInCache)
  {
    if (size != 0)
      memcpy(data, _cache + (size_t)offsetInCache, size);
  }
  else
  {
    if (newPos != _physPos)
    {
      _physPos = newPos;
      RINOK(SeekToPhys())
    }
    res = _stream->Read(data, size, &size);
    _physPos += size;
  }
  if (processedSize)
    *processedSize = size;
  _virtPos += size;
  return res;
}

Z7_COM7F_IMF(CLimitedCachedInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += _size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

Z7_COM7F_IMF(CTailOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 cur;
  HRESULT res = Stream->Write(data, size, &cur);
  if (processedSize)
    *processedSize = cur;
  _virtPos += cur;
  if (_virtSize < _virtPos)
    _virtSize = _virtPos;
  return res;
}
  
Z7_COM7F_IMF(CTailOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += _virtSize; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = _virtPos;
  return Stream->Seek((Int64)(Offset + _virtPos), STREAM_SEEK_SET, NULL);
}

Z7_COM7F_IMF(CTailOutStream::SetSize(UInt64 newSize))
{
  _virtSize = newSize;
  return Stream->SetSize(Offset + newSize);
}

/* ================ unit: CPP/7zip/Common/LockedStream.cpp ================ */
// LockedStream.cpp

// amalgamation: header emitted in prologue

/* ================ unit: CPP/7zip/Common/MemBlocks.cpp ================ */
// MemBlocks.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

bool CMemBlockManager::AllocateSpace_bool(size_t numBlocks)
{
  FreeSpace();
  if (numBlocks == 0)
  {
    return true;
    // return false;
  }
  if (_blockSize < sizeof(void *))
    return false;
  const size_t totalSize = numBlocks * _blockSize;
  if (totalSize / _blockSize != numBlocks)
    return false;
  _data = ::MidAlloc(totalSize);
  if (!_data)
    return false;
  Byte *p = (Byte *)_data;
  for (size_t i = 0; i + 1 < numBlocks; i++, p += _blockSize)
    *(Byte **)(void *)p = (p + _blockSize);
  *(Byte **)(void *)p = NULL;
  _headFree = _data;
  return true;
}

void CMemBlockManager::FreeSpace()
{
  ::MidFree(_data);
  _data = NULL;
  _headFree= NULL;
}

void *CMemBlockManager::AllocateBlock()
{
  void *p = _headFree;
  if (p)
    _headFree = *(void **)p;
  return p;
}

void CMemBlockManager::FreeBlock(void *p)
{
  if (!p)
    return;
  *(void **)p = _headFree;
  _headFree = p;
}


// #include <stdio.h>

HRESULT CMemBlockManagerMt::AllocateSpace(size_t numBlocks, size_t numNoLockBlocks)
{
  if (numNoLockBlocks > numBlocks)
    return E_INVALIDARG;
  const size_t numLockBlocks = numBlocks - numNoLockBlocks;
  UInt32 maxCount = (UInt32)numLockBlocks;
  if (maxCount != numLockBlocks)
    return E_OUTOFMEMORY;
  if (!CMemBlockManager::AllocateSpace_bool(numBlocks))
    return E_OUTOFMEMORY;
  // we need (maxCount = 1), if we want to create non-use empty Semaphore
  if (maxCount == 0)
    maxCount = 1;

  // printf("\n Synchro.Create() \n");
  WRes wres;
  #ifndef _WIN32
  Semaphore.Close();
  wres = Synchro.Create();
  if (wres != 0)
    return HRESULT_FROM_WIN32(wres);
  wres = Semaphore.Create(&Synchro, (UInt32)numLockBlocks, maxCount);
  #else
  wres = Semaphore.OptCreateInit((UInt32)numLockBlocks, maxCount);
  #endif
  
  return HRESULT_FROM_WIN32(wres);
}


HRESULT CMemBlockManagerMt::AllocateSpaceAlways(size_t desiredNumberOfBlocks, size_t numNoLockBlocks)
{
  // desiredNumberOfBlocks = 0; // for debug
  if (numNoLockBlocks > desiredNumberOfBlocks)
    return E_INVALIDARG;
  for (;;)
  {
    // if (desiredNumberOfBlocks == 0) return E_OUTOFMEMORY;
    const HRESULT hres = AllocateSpace(desiredNumberOfBlocks, numNoLockBlocks);
    if (hres != E_OUTOFMEMORY)
      return hres;
    if (desiredNumberOfBlocks == numNoLockBlocks)
      return E_OUTOFMEMORY;
    desiredNumberOfBlocks = numNoLockBlocks + ((desiredNumberOfBlocks - numNoLockBlocks) >> 1);
  }
}

void CMemBlockManagerMt::FreeSpace()
{
  Semaphore.Close();
  CMemBlockManager::FreeSpace();
}

void *CMemBlockManagerMt::AllocateBlock()
{
  // Semaphore.Lock();
  NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
  return CMemBlockManager::AllocateBlock();
}

void CMemBlockManagerMt::FreeBlock(void *p, bool lockMode)
{
  if (!p)
    return;
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    CMemBlockManager::FreeBlock(p);
  }
  if (lockMode)
    Semaphore.Release();
}



void CMemBlocks::Free(CMemBlockManagerMt *manager)
{
  while (Blocks.Size() > 0)
  {
    manager->FreeBlock(Blocks.Back());
    Blocks.DeleteBack();
  }
  TotalSize = 0;
}

void CMemBlocks::FreeOpt(CMemBlockManagerMt *manager)
{
  Free(manager);
  Blocks.ClearAndFree();
}

HRESULT CMemBlocks::WriteToStream(size_t blockSize, ISequentialOutStream *outStream) const
{
  UInt64 totalSize = TotalSize;
  for (unsigned blockIndex = 0; totalSize > 0; blockIndex++)
  {
    size_t curSize = blockSize;
    if (curSize > totalSize)
      curSize = (size_t)totalSize;
    if (blockIndex >= Blocks.Size())
      return E_FAIL;
    RINOK(WriteStream(outStream, Blocks[blockIndex], curSize))
    totalSize -= curSize;
  }
  return S_OK;
}


void CMemLockBlocks::FreeBlock(unsigned index, CMemBlockManagerMt *memManager)
{
  memManager->FreeBlock(Blocks[index], LockMode);
  Blocks[index] = NULL;
}

void CMemLockBlocks::Free(CMemBlockManagerMt *memManager)
{
  while (Blocks.Size() > 0)
  {
    FreeBlock(Blocks.Size() - 1, memManager);
    Blocks.DeleteBack();
  }
  TotalSize = 0;
}

/*
HRes CMemLockBlocks::SwitchToNoLockMode(CMemBlockManagerMt *memManager)
{
  if (LockMode)
  {
    if (Blocks.Size() > 0)
    {
      RINOK(memManager->ReleaseLockedBlocks(Blocks.Size()));
    }
    LockMode = false;
  }
  return 0;
}
*/

void CMemLockBlocks::Detach(CMemLockBlocks &blocks, CMemBlockManagerMt *memManager)
{
  blocks.Free(memManager);
  blocks.LockMode = LockMode;
  UInt64 totalSize = 0;
  const size_t blockSize = memManager->GetBlockSize();
  FOR_VECTOR (i, Blocks)
  {
    if (totalSize < TotalSize)
      blocks.Blocks.Add(Blocks[i]);
    else
      FreeBlock(i, memManager);
    Blocks[i] = NULL;
    totalSize += blockSize;
  }
  blocks.TotalSize = TotalSize;
  Free(memManager);
}

/* ================ unit: CPP/7zip/Common/MethodId.cpp ================ */
// MethodId.cpp

// amalgamation: header emitted in prologue

/* ================ unit: CPP/7zip/Common/MethodProps.cpp ================ */
// MethodProps.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

using namespace NWindows;

UInt64 Calc_From_Val_Percents(UInt64 val, UInt64 percents)
{
  // if (percents == 0) return 0;
  const UInt64 q = percents / 100;
  const UInt32 r = (UInt32)(percents % 100);
  UInt64 res = 0;
  
  if (q != 0)
  {
    if (val > (UInt64)(Int64)-1 / q)
      return (UInt64)(Int64)-1;
    res = val * q;
  }

  if (r != 0)
  {
    UInt64 v2;
    if (val <= (UInt64)(Int64)-1 / r)
      v2 = val * r / 100;
    else
      v2 = val / 100 * r;
    res += v2;
    if (res < v2)
      return (UInt64)(Int64)-1;
  }
  
  return res;
}


bool StringToBool(const wchar_t *s, bool &res)
{
  if (s[0] == 0 || (s[0] == '+' && s[1] == 0) || StringsAreEqualNoCase_Ascii(s, "ON"))
  {
    res = true;
    return true;
  }
  if ((s[0] == '-' && s[1] == 0) || StringsAreEqualNoCase_Ascii(s, "OFF"))
  {
    res = false;
    return true;
  }
  return false;
}

HRESULT PROPVARIANT_to_bool(const PROPVARIANT &prop, bool &dest)
{
  switch (prop.vt)
  {
    case VT_EMPTY: dest = true; return S_OK;
    case VT_BOOL: dest = (prop.boolVal != VARIANT_FALSE); return S_OK;
    case VT_BSTR: return StringToBool(prop.bstrVal, dest) ? S_OK : E_INVALIDARG;
    default: break;
  }
  return E_INVALIDARG;
}

unsigned ParseStringToUInt32(const UString &srcString, UInt32 &number)
{
  const wchar_t *start = srcString;
  const wchar_t *end;
  number = ConvertStringToUInt32(start, &end);
  return (unsigned)(end - start);
}

static unsigned ParseStringToUInt64(const UString &srcString, UInt64 &number)
{
  const wchar_t *start = srcString;
  const wchar_t *end;
  number = ConvertStringToUInt64(start, &end);
  return (unsigned)(end - start);
}

HRESULT ParsePropToUInt32(const UString &name, const PROPVARIANT &prop, UInt32 &resValue)
{
  // =VT_UI4
  // =VT_EMPTY : it doesn't change (resValue), and returns S_OK
  // {stringUInt32}=VT_EMPTY

  if (prop.vt == VT_UI4)
  {
    if (!name.IsEmpty())
      return E_INVALIDARG;
    resValue = prop.ulVal;
    return S_OK;
  }
  if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  if (name.IsEmpty())
    return S_OK;
  UInt32 v;
  if (ParseStringToUInt32(name, v) != name.Len())
    return E_INVALIDARG;
  resValue = v;
  return S_OK;
}



HRESULT ParseMtProp2(const UString &name, const PROPVARIANT &prop, UInt32 &numThreads, bool &force)
{
  force = false;
  UString s;
  if (name.IsEmpty())
  {
    if (prop.vt == VT_UI4)
    {
      numThreads = prop.ulVal;
      force = true;
      return S_OK;
    }
    bool val;
    HRESULT res = PROPVARIANT_to_bool(prop, val);
    if (res == S_OK)
    {
      if (!val)
      {
        numThreads = 1;
        force = true;
      }
      // force = true; for debug
      // "(VT_BOOL = VARIANT_TRUE)" set "force = false" and doesn't change numThreads
      return S_OK;
    }
    if (prop.vt != VT_BSTR)
      return res;
    s.SetFromBstr(prop.bstrVal);
    if (s.IsEmpty())
      return E_INVALIDARG;
  }
  else
  {
    if (prop.vt != VT_EMPTY)
      return E_INVALIDARG;
    s = name;
  }

  s.MakeLower_Ascii();
  const wchar_t *start = s;
  UInt32 v = numThreads;

  /* we force up, if threads number specified
     only `d` will force it down */
  bool force_loc = true;
  for (;;)
  {
    const wchar_t c = *start;
    if (!c)
      break;
    if (c == 'd')
    {
      force_loc = false;  // force down
      start++;
      continue;
    }
    if (c == 'u')
    {
      force_loc = true;   // force up
      start++;
      continue;
    }
    bool isPercent = false;
    if (c == 'p')
    {
      isPercent = true;
      start++;
    }
    const wchar_t *end;
    v = ConvertStringToUInt32(start, &end);
    if (end == start)
      return E_INVALIDARG;
    if (isPercent)
      v = numThreads * v / 100;
    start = end;
  }

  numThreads = v;
  force = force_loc;
  return S_OK;
}



static HRESULT SetLogSizeProp(UInt64 number, NCOM::CPropVariant &destProp)
{
  if (number >= 64)
    return E_INVALIDARG;
  UInt32 val32;
  if (number < 32)
    val32 = (UInt32)1 << (unsigned)number;
  /*
  else if (number == 32 && reduce_4GB_to_32bits)
    val32 = (UInt32)(Int32)-1;
  */
  else
  {
    destProp = (UInt64)((UInt64)1 << (unsigned)number);
    return S_OK;
  }
  destProp = (UInt32)val32;
  return S_OK;
}


static HRESULT StringToDictSize(const UString &s, NCOM::CPropVariant &destProp)
{
  /* if (reduce_4GB_to_32bits) we can reduce (4 GiB) property to (4 GiB - 1).
     to fit the value to UInt32 for clients that do not support 64-bit values */

  const wchar_t *end;
  const UInt64 number = ConvertStringToUInt64(s, &end);
  const unsigned numDigits = (unsigned)(end - s.Ptr());
  if (numDigits == 0 || s.Len() > numDigits + 1)
    return E_INVALIDARG;
  
  if (s.Len() == numDigits)
    return SetLogSizeProp(number, destProp);
  
  unsigned numBits;
  
  switch (MyCharLower_Ascii(s[numDigits]))
  {
    case 'b': numBits =  0; break;
    case 'k': numBits = 10; break;
    case 'm': numBits = 20; break;
    case 'g': numBits = 30; break;
    default: return E_INVALIDARG;
  }
  
  const UInt64 range4g = ((UInt64)1 << (32 - numBits));
  if (number < range4g)
    destProp = (UInt32)((UInt32)number << numBits);
  /*
  else if (number == range4g && reduce_4GB_to_32bits)
    destProp = (UInt32)(Int32)-1;
  */
  else if (numBits == 0)
    destProp = (UInt64)number;
  else if (number >= ((UInt64)1 << (64 - numBits)))
    return E_INVALIDARG;
  else
    destProp = (UInt64)((UInt64)number << numBits);
  return S_OK;
}


static HRESULT PROPVARIANT_to_DictSize(const PROPVARIANT &prop, NCOM::CPropVariant &destProp)
{
  if (prop.vt == VT_UI4)
    return SetLogSizeProp(prop.ulVal, destProp);

  if (prop.vt == VT_BSTR)
  {
    UString s;
    s = prop.bstrVal;
    return StringToDictSize(s, destProp);
  }
  return E_INVALIDARG;
}


void CProps::AddProp32(PROPID propid, UInt32 val)
{
  CProp &prop = Props.AddNew();
  prop.IsOptional = true;
  prop.Id = propid;
  prop.Value = (UInt32)val;
}

void CProps::AddPropBool(PROPID propid, bool val)
{
  CProp &prop = Props.AddNew();
  prop.IsOptional = true;
  prop.Id = propid;
  prop.Value = val;
}

class CCoderProps
{
  PROPID *_propIDs;
  NCOM::CPropVariant *_props;
  unsigned _numProps;
  unsigned _numPropsMax;
public:
  CCoderProps(unsigned numPropsMax):
      _propIDs(NULL),
      _props(NULL),
      _numProps(0),
      _numPropsMax(numPropsMax)
  {
    _propIDs = new PROPID[numPropsMax];
    _props = new NCOM::CPropVariant[numPropsMax];
  }
  ~CCoderProps()
  {
    delete []_propIDs;
    delete []_props;
  }
  void AddProp(const CProp &prop);
  HRESULT SetProps(ICompressSetCoderProperties *setCoderProperties)
  {
    return setCoderProperties->SetCoderProperties(_propIDs, _props, _numProps);
  }
};

void CCoderProps::AddProp(const CProp &prop)
{
  if (_numProps >= _numPropsMax)
    throw 1;
  _propIDs[_numProps] = prop.Id;
  _props[_numProps] = prop.Value;
  _numProps++;
}

HRESULT CProps::SetCoderProps(ICompressSetCoderProperties *scp, const UInt64 *dataSizeReduce) const
{
  return SetCoderProps_DSReduce_Aff(scp, dataSizeReduce, NULL, NULL, NULL);
}

HRESULT CProps::SetCoderProps_DSReduce_Aff(
    ICompressSetCoderProperties *scp,
    const UInt64 *dataSizeReduce,
    const UInt64 *affinity,
    const UInt32 *affinityGroup,
    const UInt64 *affinityInGroup) const
{
  CCoderProps coderProps(Props.Size()
      + (dataSizeReduce ? 1 : 0)
      + (affinity ? 1 : 0)
      + (affinityGroup ? 1 : 0)
      + (affinityInGroup ? 1 : 0)
      );
  FOR_VECTOR (i, Props)
    coderProps.AddProp(Props[i]);
  if (dataSizeReduce)
  {
    CProp prop;
    prop.Id = NCoderPropID::kReduceSize;
    prop.Value = *dataSizeReduce;
    coderProps.AddProp(prop);
  }
  if (affinity)
  {
    CProp prop;
    prop.Id = NCoderPropID::kAffinity;
    prop.Value = *affinity;
    coderProps.AddProp(prop);
  }
  if (affinityGroup)
  {
    CProp prop;
    prop.Id = NCoderPropID::kThreadGroup;
    prop.Value = *affinityGroup;
    coderProps.AddProp(prop);
  }
  if (affinityInGroup)
  {
    CProp prop;
    prop.Id = NCoderPropID::kAffinityInGroup;
    prop.Value = *affinityInGroup;
    coderProps.AddProp(prop);
  }
  return coderProps.SetProps(scp);
}


int CMethodProps::FindProp(PROPID id) const
{
  for (unsigned i = Props.Size(); i != 0;)
    if (Props[--i].Id == id)
      return (int)i;
  return -1;
}

unsigned CMethodProps::GetLevel() const
{
  int i = FindProp(NCoderPropID::kLevel);
  if (i < 0)
    return 5;
  if (Props[(unsigned)i].Value.vt != VT_UI4)
    return 9;
  UInt32 level = Props[(unsigned)i].Value.ulVal;
  return level > 9 ? 9 : (unsigned)level;
}

struct CNameToPropID
{
  VARTYPE VarType;
  const char *Name;
};


// the following are related to NCoderPropID::EEnum values
// NCoderPropID::k_NUM_DEFINED
static const CNameToPropID g_NameToPropID[] =
{
  { VT_UI4, "" },
  { VT_UI4, "d" },
  { VT_UI4, "mem" },
  { VT_UI4, "o" },
  { VT_UI8, "c" },
  { VT_UI4, "pb" },
  { VT_UI4, "lc" },
  { VT_UI4, "lp" },
  { VT_UI4, "fb" },
  { VT_BSTR, "mf" },
  { VT_UI4, "mc" },
  { VT_UI4, "pass" },
  { VT_UI4, "a" },
  { VT_UI4, "mt" },
  { VT_BOOL, "eos" },
  { VT_UI4, "x" },
  { VT_UI8, "reduce" },
  { VT_UI8, "expect" },
  { VT_UI8, "cc" }, // "cc" in v23,  "b" in v22.01
  { VT_UI4, "check" },
  { VT_BSTR, "filter" },
  { VT_UI8, "memuse" },
  { VT_UI8, "aff" },
  { VT_UI4, "offset" },
  { VT_UI4, "zhb" }
  /*
  , { VT_UI4, "tgn" }, // kNumThreadGroups
  , { VT_UI4, "tgi" }, // kThreadGroup
  , { VT_UI8, "tga" }, // kAffinityInGroup
  */
  /*
  ,
  // { VT_UI4, "zhc" },
  // { VT_UI4, "zhd" },
  // { VT_UI4, "zcb" },
  { VT_UI4, "dc" },
  { VT_UI4, "zx" },
  { VT_UI4, "zf" },
  { VT_UI4, "zmml" },
  { VT_UI4, "zov" },
  { VT_BOOL, "zmfr" },
  { VT_BOOL, "zle" }, // long enable
  // { VT_UI4, "zldb" },
  { VT_UI4, "zld" },
  { VT_UI4, "zlhb" },
  { VT_UI4, "zlmml" },
  { VT_UI4, "zlbb" },
  { VT_UI4, "zlhrb" },
  { VT_BOOL, "zwus" },
  { VT_BOOL, "zshp" },
  { VT_BOOL, "zshs" },
  { VT_BOOL, "zshe" },
  { VT_BOOL, "zshg" },
  { VT_UI4, "zpsm" }
  */
  // { VT_UI4, "mcb" }, // mc log version
  // { VT_UI4, "ztlen" },  // fb ?
};

/*
#if defined(static_assert) || (defined(__cplusplus) && __cplusplus >= 200410L) || (defined(_MSC_VER) && _MSC_VER >= 1600)

#if (defined(__cplusplus) && __cplusplus < 201103L) \
    && defined(__clang__) && __clang_major__ >= 4
#pragma GCC diagnostic ignored "-Wc11-extensions"
#endif
  static_assert(Z7_ARRAY_SIZE(g_NameToPropID) == NCoderPropID::k_NUM_DEFINED,
    "g_NameToPropID doesn't match NCoderPropID enum");
#endif
*/

static int FindPropIdExact(const UString &name)
{
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_NameToPropID); i++)
    if (StringsAreEqualNoCase_Ascii(name, g_NameToPropID[i].Name))
      return (int)i;
  return -1;
}

static bool ConvertProperty(const PROPVARIANT &srcProp, VARTYPE varType, NCOM::CPropVariant &destProp)
{
  if (varType == srcProp.vt)
  {
    destProp = srcProp;
    return true;
  }

  if (varType == VT_UI8 && srcProp.vt == VT_UI4)
  {
    destProp = (UInt64)srcProp.ulVal;
    return true;
  }

  if (varType == VT_BOOL)
  {
    bool res;
    if (PROPVARIANT_to_bool(srcProp, res) != S_OK)
      return false;
    destProp = res;
    return true;
  }
  if (srcProp.vt == VT_EMPTY)
  {
    destProp = srcProp;
    return true;
  }
  return false;
}
    
static void SplitParams(const UString &srcString, UStringVector &subStrings)
{
  subStrings.Clear();
  UString s;
  unsigned len = srcString.Len();
  if (len == 0)
    return;
  for (unsigned i = 0; i < len; i++)
  {
    wchar_t c = srcString[i];
    if (c == L':')
    {
      subStrings.Add(s);
      s.Empty();
    }
    else
      s += c;
  }
  subStrings.Add(s);
}

static void SplitParam(const UString &param, UString &name, UString &value)
{
  int eqPos = param.Find(L'=');
  if (eqPos >= 0)
  {
    name.SetFrom(param, (unsigned)eqPos);
    value = param.Ptr((unsigned)(eqPos + 1));
    return;
  }
  unsigned i;
  for (i = 0; i < param.Len(); i++)
  {
    wchar_t c = param[i];
    if (c >= L'0' && c <= L'9')
      break;
  }
  name.SetFrom(param, i);
  value = param.Ptr(i);
}

static bool IsLogSizeProp(PROPID propid)
{
  switch (propid)
  {
    case NCoderPropID::kDictionarySize:
    case NCoderPropID::kUsedMemorySize:
    case NCoderPropID::kBlockSize:
    case NCoderPropID::kBlockSize2:
    /*
    case NCoderPropID::kChainSize:
    case NCoderPropID::kLdmWindowSize:
    */
    // case NCoderPropID::kReduceSize:
      return true;
    default: break;
  }
  return false;
}

HRESULT CMethodProps::SetParam(const UString &name, const UString &value)
{
  int index = FindPropIdExact(name);
  if (index < 0)
  {
    // 'b' was used as NCoderPropID::kBlockSize2 before v23
    if (!name.IsEqualTo_Ascii_NoCase("b") || value.Find(L':') >= 0)
      return E_INVALIDARG;
    index = NCoderPropID::kBlockSize2;
  }
  const CNameToPropID &nameToPropID = g_NameToPropID[(unsigned)index];
  CProp prop;
  prop.Id = (unsigned)index;

  if (IsLogSizeProp(prop.Id))
  {
    RINOK(StringToDictSize(value, prop.Value))
  }
  else
  {
    NCOM::CPropVariant propValue;
    if (nameToPropID.VarType == VT_BSTR)
      propValue = value;
    else if (nameToPropID.VarType == VT_BOOL)
    {
      bool res;
      if (!StringToBool(value, res))
        return E_INVALIDARG;
      propValue = res;
    }
    else if (!value.IsEmpty())
    {
      if (nameToPropID.VarType == VT_UI4)
      {
        UInt32 number;
        if (ParseStringToUInt32(value, number) == value.Len())
          propValue = number;
        else
          propValue = value;
      }
      else if (nameToPropID.VarType == VT_UI8)
      {
        UInt64 number;
        if (ParseStringToUInt64(value, number) == value.Len())
          propValue = number;
        else
          propValue = value;
      }
      else
        propValue = value;
    }
    if (!ConvertProperty(propValue, nameToPropID.VarType, prop.Value))
      return E_INVALIDARG;
  }
  Props.Add(prop);
  return S_OK;
}

HRESULT CMethodProps::ParseParamsFromString(const UString &srcString)
{
  UStringVector params;
  SplitParams(srcString, params);
  FOR_VECTOR (i, params)
  {
    const UString &param = params[i];
    UString name, value;
    SplitParam(param, name, value);
    RINOK(SetParam(name, value))
  }
  return S_OK;
}

HRESULT CMethodProps::ParseParamsFromPROPVARIANT(const UString &realName, const PROPVARIANT &value)
{
  if (realName.Len() == 0)
  {
    // [empty]=method
    return E_INVALIDARG;
  }
  if (value.vt == VT_EMPTY)
  {
    // {realName}=[empty]
    UString name, valueStr;
    SplitParam(realName, name, valueStr);
    return SetParam(name, valueStr);
  }
  
  // {realName}=value
  const int index = FindPropIdExact(realName);
  if (index < 0)
    return E_INVALIDARG;
  const CNameToPropID &nameToPropID = g_NameToPropID[(unsigned)index];
  CProp prop;
  prop.Id = (unsigned)index;
  
  if (IsLogSizeProp(prop.Id))
  {
    RINOK(PROPVARIANT_to_DictSize(value, prop.Value))
  }
  else
  {
    if (!ConvertProperty(value, nameToPropID.VarType, prop.Value))
      return E_INVALIDARG;
  }
  Props.Add(prop);
  return S_OK;
}


static UInt64 GetMemoryUsage_LZMA(UInt32 dict, bool isBt, UInt32 numThreads)
{
  UInt32 hs = dict - 1;
  hs |= (hs >> 1);
  hs |= (hs >> 2);
  hs |= (hs >> 4);
  hs |= (hs >> 8);
  hs >>= 1;
  if (hs >= (1 << 24))
    hs >>= 1;
  hs |= (1 << 16) - 1;
  // if (numHashBytes >= 5)
  if (!isBt)
    hs |= (256 << 10) - 1;
  hs++;
  UInt64 size1 = (UInt64)hs * 4;
  size1 += (UInt64)dict * 4;
  if (isBt)
    size1 += (UInt64)dict * 4;
  size1 += (2 << 20);
  
  if (numThreads > 1 && isBt)
    size1 += (2 << 20) + (4 << 20);
  return size1;
}

static const UInt32 kLzmaMaxDictSize = (UInt32)15 << 28;

UInt64 CMethodProps::Get_Lzma_MemUsage(bool addSlidingWindowSize) const
{
  const UInt64 dicSize = Get_Lzma_DicSize();
  const bool isBt = Get_Lzma_MatchFinder_IsBt();
  const UInt32 dict32 = (dicSize >= kLzmaMaxDictSize ? kLzmaMaxDictSize : (UInt32)dicSize);
  const UInt32 numThreads = Get_Lzma_NumThreads();
  UInt64 size = GetMemoryUsage_LZMA(dict32, isBt, numThreads);
  
  if (addSlidingWindowSize)
  {
    const UInt32 kBlockSizeMax = (UInt32)0 - (UInt32)(1 << 16);
    UInt64 blockSize = (UInt64)dict32 + (1 << 16)
        + (numThreads > 1 ? (1 << 20) : 0);
    blockSize += (blockSize >> (blockSize < ((UInt32)1 << 30) ? 1 : 2));
    if (blockSize >= kBlockSizeMax)
      blockSize = kBlockSizeMax;
    size += blockSize;
  }
  return size;
}




HRESULT COneMethodInfo::ParseMethodFromString(const UString &s)
{
  MethodName.Empty();
  int splitPos = s.Find(L':');
  {
    UString temp = s;
    if (splitPos >= 0)
      temp.DeleteFrom((unsigned)splitPos);
    if (!temp.IsAscii())
      return E_INVALIDARG;
    MethodName.SetFromWStr_if_Ascii(temp);
  }
  if (splitPos < 0)
    return S_OK;
  PropsString = s.Ptr((unsigned)(splitPos + 1));
  return ParseParamsFromString(PropsString);
}

HRESULT COneMethodInfo::ParseMethodFromPROPVARIANT(const UString &realName, const PROPVARIANT &value)
{
  if (!realName.IsEmpty() && !StringsAreEqualNoCase_Ascii(realName, "m"))
    return ParseParamsFromPROPVARIANT(realName, value);
  // -m{N}=method
  if (value.vt != VT_BSTR)
    return E_INVALIDARG;
  UString s;
  s = value.bstrVal;
  return ParseMethodFromString(s);
}

/* ================ unit: CPP/7zip/Common/OffsetStream.cpp ================ */
// OffsetStream.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

HRESULT COffsetOutStream::Init(IOutStream *stream, UInt64 offset)
{
  _offset = offset;
  _stream = stream;
  return _stream->Seek((Int64)offset, STREAM_SEEK_SET, NULL);
}

Z7_COM7F_IMF(COffsetOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  return _stream->Write(data, size, processedSize);
}

Z7_COM7F_IMF(COffsetOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  if (seekOrigin == STREAM_SEEK_SET)
  {
    if (offset < 0)
      return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
    offset += _offset;
  }
  UInt64 absoluteNewPosition = 0; // =0 for gcc-10
  const HRESULT result = _stream->Seek(offset, seekOrigin, &absoluteNewPosition);
  if (newPosition)
    *newPosition = absoluteNewPosition - _offset;
  return result;
}

Z7_COM7F_IMF(COffsetOutStream::SetSize(UInt64 newSize))
{
  return _stream->SetSize(_offset + newSize);
}

/* ================ unit: CPP/7zip/Common/OutBuffer.cpp ================ */
// OutBuffer.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

bool COutBuffer::Create(UInt32 bufSize) throw()
{
  const UInt32 kMinBlockSize = 1;
  if (bufSize < kMinBlockSize)
    bufSize = kMinBlockSize;
  if (_buf && _bufSize == bufSize)
    return true;
  Free();
  _bufSize = bufSize;
  _buf = (Byte *)::MidAlloc(bufSize);
  return (_buf != NULL);
}

void COutBuffer::Free() throw()
{
  ::MidFree(_buf);
  _buf = NULL;
}

void COutBuffer::Init() throw()
{
  _streamPos = 0;
  _limitPos = _bufSize;
  _pos = 0;
  _processedSize = 0;
  _overDict = false;
  #ifdef Z7_NO_EXCEPTIONS
  ErrorCode = S_OK;
  #endif
}

UInt64 COutBuffer::GetProcessedSize() const throw()
{
  UInt64 res = _processedSize + _pos - _streamPos;
  if (_streamPos > _pos)
    res += _bufSize;
  return res;
}


HRESULT COutBuffer::FlushPart() throw()
{
  // _streamPos < _bufSize
  UInt32 size = (_streamPos >= _pos) ? (_bufSize - _streamPos) : (_pos - _streamPos);
  HRESULT result = S_OK;
  #ifdef Z7_NO_EXCEPTIONS
  result = ErrorCode;
  #endif
  if (_buf2)
  {
    memcpy(_buf2, _buf + _streamPos, size);
    _buf2 += size;
  }

  if (_stream
      #ifdef Z7_NO_EXCEPTIONS
      && (ErrorCode == S_OK)
      #endif
     )
  {
    UInt32 processedSize = 0;
    result = _stream->Write(_buf + _streamPos, size, &processedSize);
    size = processedSize;
  }
  _streamPos += size;
  if (_streamPos == _bufSize)
    _streamPos = 0;
  if (_pos == _bufSize)
  {
    _overDict = true;
    _pos = 0;
  }
  _limitPos = (_streamPos > _pos) ? _streamPos : _bufSize;
  _processedSize += size;
  return result;
}

HRESULT COutBuffer::Flush() throw()
{
  #ifdef Z7_NO_EXCEPTIONS
  if (ErrorCode != S_OK)
    return ErrorCode;
  #endif

  while (_streamPos != _pos)
  {
    const HRESULT result = FlushPart();
    if (result != S_OK)
      return result;
  }
  return S_OK;
}

void COutBuffer::FlushWithCheck()
{
  const HRESULT result = Flush();
  #ifdef Z7_NO_EXCEPTIONS
  ErrorCode = result;
  #else
  if (result != S_OK)
    throw COutBufferException(result);
  #endif
}

/* ================ unit: CPP/7zip/Common/OutMemStream.cpp ================ */
// OutMemStream.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

void COutMemStream::Free()
{
  Blocks.Free(_memManager);
  Blocks.LockMode = true;
}

void COutMemStream::Init()
{
  WriteToRealStreamEvent.Reset();
  _unlockEventWasSent = false;
  _realStreamMode = false;
  Free();
  _curBlockPos = 0;
  _curBlockIndex = 0;
}

void COutMemStream::DetachData(CMemLockBlocks &blocks)
{
  Blocks.Detach(blocks, _memManager);
  Free();
}


HRESULT COutMemStream::WriteToRealStream()
{
  RINOK(Blocks.WriteToStream(_memManager->GetBlockSize(), OutSeqStream))
  Blocks.Free(_memManager);
  return S_OK;
}


Z7_COM7F_IMF(COutMemStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  if (_realStreamMode)
    return OutSeqStream->Write(data, size, processedSize);
  if (processedSize)
    *processedSize = 0;
  while (size != 0)
  {
    if (_curBlockIndex < Blocks.Blocks.Size())
    {
      Byte *p = (Byte *)Blocks.Blocks[_curBlockIndex] + _curBlockPos;
      size_t curSize = _memManager->GetBlockSize() - _curBlockPos;
      if (size < curSize)
        curSize = size;
      memcpy(p, data, curSize);
      if (processedSize)
        *processedSize += (UInt32)curSize;
      data = (const void *)((const Byte *)data + curSize);
      size -= (UInt32)curSize;
      _curBlockPos += curSize;

      const UInt64 pos64 = GetPos();
      if (pos64 > Blocks.TotalSize)
        Blocks.TotalSize = pos64;
      if (_curBlockPos == _memManager->GetBlockSize())
      {
        _curBlockIndex++;
        _curBlockPos = 0;
      }
      continue;
    }

    const NWindows::NSynchronization::CHandle_WFMO events[3] =
      { StopWritingEvent, WriteToRealStreamEvent, /* NoLockEvent, */ _memManager->Semaphore };
    const DWORD waitResult = NWindows::NSynchronization::WaitForMultiObj_Any_Infinite(
        ((Blocks.LockMode /* && _memManager->Semaphore.IsCreated() */) ? 3 : 2), events);
    
    // printf("\n 1- outMemStream %d\n", waitResult - WAIT_OBJECT_0);

    switch (waitResult)
    {
      case (WAIT_OBJECT_0 + 0):
        return StopWriteResult;
      case (WAIT_OBJECT_0 + 1):
      {
        _realStreamMode = true;
        RINOK(WriteToRealStream())
        UInt32 processedSize2;
        const HRESULT res = OutSeqStream->Write(data, size, &processedSize2);
        if (processedSize)
          *processedSize += processedSize2;
        return res;
      }
      case (WAIT_OBJECT_0 + 2):
      {
        // it has bug: no write.
        /*
        if (!Blocks.SwitchToNoLockMode(_memManager))
          return E_FAIL;
        */
        break;
      }
      default:
      {
        if (waitResult == WAIT_FAILED)
        {
          const DWORD res = ::GetLastError();
          if (res != 0)
            return HRESULT_FROM_WIN32(res);
        }
        return E_FAIL;
      }
    }
    void *p = _memManager->AllocateBlock();
    if (!p)
      return E_FAIL;
    Blocks.Blocks.Add(p);
  }
  return S_OK;
}

Z7_COM7F_IMF(COutMemStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  if (_realStreamMode)
  {
    if (!OutStream)
      return E_FAIL;
    return OutStream->Seek(offset, seekOrigin, newPosition);
  }
  if (seekOrigin == STREAM_SEEK_CUR)
  {
    if (offset != 0)
      return E_NOTIMPL;
  }
  else if (seekOrigin == STREAM_SEEK_SET)
  {
    if (offset != 0)
      return E_NOTIMPL;
    _curBlockIndex = 0;
    _curBlockPos = 0;
  }
  else
    return E_NOTIMPL;
  if (newPosition)
    *newPosition = GetPos();
  return S_OK;
}

Z7_COM7F_IMF(COutMemStream::SetSize(UInt64 newSize))
{
  if (_realStreamMode)
  {
    if (!OutStream)
      return E_FAIL;
    return OutStream->SetSize(newSize);
  }
  Blocks.TotalSize = newSize;
  return S_OK;
}

/* ================ unit: CPP/7zip/Common/ProgressMt.cpp ================ */
// ProgressMt.h

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

void CMtCompressProgressMixer::Init(unsigned numItems, ICompressProgressInfo *progress)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
  InSizes.Clear();
  OutSizes.Clear();
  for (unsigned i = 0; i < numItems; i++)
  {
    InSizes.Add(0);
    OutSizes.Add(0);
  }
  TotalInSize = 0;
  TotalOutSize = 0;
  _progress = progress;
}

void CMtCompressProgressMixer::Reinit(unsigned index)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
  InSizes[index] = 0;
  OutSizes[index] = 0;
}

HRESULT CMtCompressProgressMixer::SetRatioInfo(unsigned index, const UInt64 *inSize, const UInt64 *outSize)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
  if (inSize)
  {
    const UInt64 diff = *inSize - InSizes[index];
    InSizes[index] = *inSize;
    TotalInSize += diff;
  }
  if (outSize)
  {
    const UInt64 diff = *outSize - OutSizes[index];
    OutSizes[index] = *outSize;
    TotalOutSize += diff;
  }
  if (_progress)
    return _progress->SetRatioInfo(&TotalInSize, &TotalOutSize);
  return S_OK;
}


Z7_COM7F_IMF(CMtCompressProgress::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  return _progress->SetRatioInfo(_index, inSize, outSize);
}

/* ================ unit: CPP/7zip/Common/ProgressUtils.cpp ================ */
// ProgressUtils.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

CLocalProgress::CLocalProgress():
    SendRatio(true),
    SendProgress(true),
    ProgressOffset(0),
    InSize(0),
    OutSize(0)
  {}

void CLocalProgress::Init(IProgress *progress, bool inSizeIsMain)
{
  _ratioProgress.Release();
  _progress = progress;
  _progress.QueryInterface(IID_ICompressProgressInfo, &_ratioProgress);
  _inSizeIsMain = inSizeIsMain;
}

Z7_COM7F_IMF(CLocalProgress::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  UInt64 inSize2 = InSize;
  UInt64 outSize2 = OutSize;
  
  if (inSize)
    inSize2 += (*inSize);
  if (outSize)
    outSize2 += (*outSize);
  
  if (SendRatio && _ratioProgress)
  {
    RINOK(_ratioProgress->SetRatioInfo(&inSize2, &outSize2))
  }
  
  if (SendProgress)
  {
    inSize2 += ProgressOffset;
    outSize2 += ProgressOffset;
    return _progress->SetCompleted(_inSizeIsMain ? &inSize2 : &outSize2);
  }
  
  return S_OK;
}

HRESULT CLocalProgress::SetCur()
{
  return SetRatioInfo(NULL, NULL);
}

/* ================ unit: CPP/7zip/Common/PropId.cpp ================ */
// PropId.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// VARTYPE
const Byte k7z_PROPID_To_VARTYPE[kpid_NUM_DEFINED] =
{
  VT_EMPTY,
  VT_UI4,
  VT_UI4,
  VT_BSTR,
  VT_BSTR,
  VT_BSTR,
  VT_BOOL,
  VT_UI8,
  VT_UI8,
  VT_UI4,
  VT_FILETIME,
  VT_FILETIME,
  VT_FILETIME,
  VT_BOOL,
  VT_BOOL,
  VT_BOOL,
  VT_BOOL,
  VT_BOOL,
  VT_UI4,
  VT_UI4,
  VT_BSTR,
  VT_BOOL,
  VT_BSTR,
  VT_BSTR,
  VT_BSTR,
  VT_BSTR,
  VT_BSTR,
  VT_UI8,
  VT_BSTR,
  VT_UI8,
  VT_BSTR,
  VT_UI8,
  VT_UI8,
  VT_BSTR, // or VT_UI8 kpidUnpackVer
  VT_UI4, // or VT_UI8 kpidVolume
  VT_BOOL,
  VT_UI8,
  VT_UI8,
  VT_UI8,
  VT_UI8,
  VT_UI4,
  VT_BOOL,
  VT_BOOL,
  VT_BSTR,
  VT_UI8,
  VT_UI8,
  VT_UI4, // kpidChecksum
  VT_BSTR,
  VT_UI8,
  VT_BSTR, // or VT_UI8 kpidId
  VT_BSTR,
  VT_BSTR,
  VT_UI4,
  VT_UI4,
  VT_BSTR,
  VT_BSTR,
  VT_UI8,
  VT_UI8,
  VT_UI4,
  VT_BSTR,
  VT_BSTR,
  VT_BSTR,
  VT_BSTR, // kpidNtSecure
  VT_BOOL,
  VT_BOOL,
  VT_BOOL,
  VT_BOOL,
  VT_BSTR, // SHA-1
  VT_BSTR, // SHA-256
  VT_BSTR,
  VT_UI8,
  VT_UI4,
  VT_UI4,
  VT_BSTR,
  VT_UI8,
  VT_UI8,
  VT_UI8,
  VT_UI8,
  VT_UI8,
  VT_UI8,
  VT_UI8,
  VT_BSTR,
  VT_BSTR,
  VT_BSTR,
  VT_BOOL,
  VT_BOOL,
  VT_BOOL,
  VT_UI8,
  VT_UI8,
  VT_BSTR, // kpidNtReparse
  VT_BSTR,
  VT_UI8,
  VT_UI8,
  VT_BOOL,
  VT_BSTR,
  VT_BSTR,
  VT_BSTR,
  VT_BOOL,
  VT_FILETIME, // kpidChangeTime
  VT_UI4,
  VT_UI4,
  VT_UI4,
  VT_UI4,
  VT_UI4,
  VT_UI4  // kpidDevMinor
};

/* ================ unit: CPP/7zip/Common/StreamBinder.cpp ================ */
// StreamBinder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_COM_1(
  CBinderInStream
  , ISequentialInStream
)
  CStreamBinder *_binder;
public:
  ~CBinderInStream() { _binder->CloseRead_CallOnce(); }
  CBinderInStream(CStreamBinder *binder): _binder(binder) {}
};

Z7_COM7F_IMF(CBinderInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
  { return _binder->Read(data, size, processedSize); }


Z7_CLASS_IMP_COM_1(
  CBinderOutStream
  , ISequentialOutStream
)
  CStreamBinder *_binder;
public:
  ~CBinderOutStream() { _binder->CloseWrite(); }
  CBinderOutStream(CStreamBinder *binder): _binder(binder) {}
};

Z7_COM7F_IMF(CBinderOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
  { return _binder->Write(data, size, processedSize); }


static HRESULT Event_Create_or_Reset(NWindows::NSynchronization::CAutoResetEvent &event)
{
  const WRes wres = event.CreateIfNotCreated_Reset();
  return HRESULT_FROM_WIN32(wres);
}

HRESULT CStreamBinder::Create_ReInit()
{
  RINOK(Event_Create_or_Reset(_canRead_Event))
  // RINOK(Event_Create_or_Reset(_canWrite_Event))

  // _canWrite_Semaphore.Close();
  // we need at least 3 items of maxCount: 1 for normal unlock in Read(), 2 items for unlock in CloseRead_CallOnce()
  _canWrite_Semaphore.OptCreateInit(0, 3);

  // _readingWasClosed = false;
  _readingWasClosed2 = false;

  _waitWrite = true;
  _bufSize = 0;
  _buf = NULL;
  ProcessedSize = 0;
  // WritingWasCut = false;
  return S_OK;
}


void CStreamBinder::CreateStreams2(CMyComPtr<ISequentialInStream> &inStream, CMyComPtr<ISequentialOutStream> &outStream)
{
  inStream = new CBinderInStream(this);
  outStream = new CBinderOutStream(this);
}

// (_canRead_Event && _bufSize == 0) means that stream is finished.

HRESULT CStreamBinder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (size != 0)
  {
    if (_waitWrite)
    {
      WRes wres = _canRead_Event.Lock();
      if (wres != 0)
        return HRESULT_FROM_WIN32(wres);
      _waitWrite = false;
    }
    if (size > _bufSize)
      size = _bufSize;
    if (size != 0)
    {
      memcpy(data, _buf, size);
      _buf = ((const Byte *)_buf) + size;
      ProcessedSize += size;
      if (processedSize)
        *processedSize = size;
      _bufSize -= size;

      /*
      if (_bufSize == 0), then we have read whole buffer
      we have two ways here:
        - if we       check (_bufSize == 0) here, we unlock Write only after full data Reading - it reduces the number of syncs
        - if we don't check (_bufSize == 0) here, we unlock Write after partial data Reading
      */
      if (_bufSize == 0)
      {
        _waitWrite = true;
        // _canWrite_Event.Set();
        _canWrite_Semaphore.Release();
      }
    }
  }
  return S_OK;
}


HRESULT CStreamBinder::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;

  if (!_readingWasClosed2)
  {
    _buf = data;
    _bufSize = size;
    _canRead_Event.Set();
    
    /*
    _canWrite_Event.Lock();
    if (_readingWasClosed)
      _readingWasClosed2 = true;
    */

    _canWrite_Semaphore.Lock();

    // _bufSize : is remain size that was not read
    size -= _bufSize;

    // size : is size of data that was read
    if (size != 0)
    {
      // if some data was read, then we report that size and return
      if (processedSize)
        *processedSize = size;
      return S_OK;
    }
    _readingWasClosed2 = true;
  }

  // WritingWasCut = true;
  return k_My_HRESULT_WritingWasCut;
}

/* ================ unit: CPP/7zip/Common/StreamObjects.cpp ================ */
// StreamObjects.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_COM7F_IMF(CBufferInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  if (_pos >= Buf.Size())
    return S_OK;
  size_t rem = Buf.Size() - (size_t)_pos;
  if (rem > size)
    rem = (size_t)size;
  memcpy(data, (const Byte *)Buf + (size_t)_pos, rem);
  _pos += rem;
  if (processedSize)
    *processedSize = (UInt32)rem;
  return S_OK;
}

Z7_COM7F_IMF(CBufferInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _pos; break;
    case STREAM_SEEK_END: offset += Buf.Size(); break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _pos = (UInt64)offset;
  if (newPosition)
    *newPosition = (UInt64)offset;
  return S_OK;
}

Z7_COM7F_IMF(CBufInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  if (_pos >= _size)
    return S_OK;
  size_t rem = _size - (size_t)_pos;
  if (rem > size)
    rem = (size_t)size;
  memcpy(data, _data + (size_t)_pos, rem);
  _pos += rem;
  if (processedSize)
    *processedSize = (UInt32)rem;
  return S_OK;
}

Z7_COM7F_IMF(CBufInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _pos; break;
    case STREAM_SEEK_END: offset += _size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _pos = (UInt64)offset;
  if (newPosition)
    *newPosition = (UInt64)offset;
  return S_OK;
}

void Create_BufInStream_WithReference(const void *data, size_t size, IUnknown *ref, ISequentialInStream **stream)
{
  *stream = NULL;
  CBufInStream *inStreamSpec = new CBufInStream;
  CMyComPtr<ISequentialInStream> streamTemp = inStreamSpec;
  inStreamSpec->Init((const Byte *)data, size, ref);
  *stream = streamTemp.Detach();
}

void Create_BufInStream_WithNewBuffer(const void *data, size_t size, ISequentialInStream **stream)
{
  *stream = NULL;
  CBufferInStream *inStreamSpec = new CBufferInStream;
  CMyComPtr<ISequentialInStream> streamTemp = inStreamSpec;
  inStreamSpec->Buf.CopyFrom((const Byte *)data, size);
  inStreamSpec->Init();
  *stream = streamTemp.Detach();
}

void CByteDynBuffer::Free() throw()
{
  MyFree(_buf);
  _buf = NULL;
  _capacity = 0;
}

bool CByteDynBuffer::EnsureCapacity(size_t cap) throw()
{
  if (cap <= _capacity)
    return true;
  const size_t cap2 = _capacity + _capacity / 4;
  if (cap < cap2)
    cap = cap2;
  Byte *buf = (Byte *)MyRealloc(_buf, cap);
  if (!buf)
    return false;
  _buf = buf;
  _capacity = cap;
  return true;
}

Byte *CDynBufSeqOutStream::GetBufPtrForWriting(size_t addSize)
{
  addSize += _size;
  if (addSize < _size)
    return NULL;
  if (!_buffer.EnsureCapacity(addSize))
    return NULL;
  return (Byte *)_buffer + _size;
}

void CDynBufSeqOutStream::CopyToBuffer(CByteBuffer &dest) const
{
  dest.CopyFrom((const Byte *)_buffer, _size);
}

Z7_COM7F_IMF(CDynBufSeqOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  Byte *buf = GetBufPtrForWriting(size);
  if (!buf)
    return E_OUTOFMEMORY;
  memcpy(buf, data, size);
  UpdateSize(size);
  if (processedSize)
    *processedSize = size;
  return S_OK;
}

Z7_COM7F_IMF(CBufPtrSeqOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  size_t rem = _size - _pos;
  if (rem > size)
    rem = (size_t)size;
  if (rem != 0)
  {
    memcpy(_buffer + _pos, data, rem);
    _pos += rem;
  }
  if (processedSize)
    *processedSize = (UInt32)rem;
  return (rem != 0 || size == 0) ? S_OK : E_FAIL;
}

Z7_COM7F_IMF(CSequentialOutStreamSizeCount::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 realProcessedSize;
  HRESULT result = _stream->Write(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize)
    *processedSize = realProcessedSize;
  return result;
}

static const UInt64 kEmptyTag = (UInt64)(Int64)-1;

void CCachedInStream::Free() throw()
{
  MyFree(_tags);
  _tags = NULL;
  MidFree(_data);
  _data = NULL;
}

bool CCachedInStream::Alloc(unsigned blockSizeLog, unsigned numBlocksLog) throw()
{
  unsigned sizeLog = blockSizeLog + numBlocksLog;
  if (sizeLog >= sizeof(size_t) * 8)
    return false;
  size_t dataSize = (size_t)1 << sizeLog;
  if (!_data || dataSize != _dataSize)
  {
    MidFree(_data);
    _data = (Byte *)MidAlloc(dataSize);
    if (!_data)
      return false;
    _dataSize = dataSize;
  }
  if (!_tags || numBlocksLog != _numBlocksLog)
  {
    MyFree(_tags);
    _tags = (UInt64 *)MyAlloc(sizeof(UInt64) << numBlocksLog);
    if (!_tags)
      return false;
    _numBlocksLog = numBlocksLog;
  }
  _blockSizeLog = blockSizeLog;
  return true;
}

void CCachedInStream::Init(UInt64 size) throw()
{
  _size = size;
  _pos = 0;
  const size_t numBlocks = (size_t)1 << _numBlocksLog;
  for (size_t i = 0; i < numBlocks; i++)
    _tags[i] = kEmptyTag;
}

Z7_COM7F_IMF(CCachedInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  if (_pos >= _size)
    return S_OK;

  {
    const UInt64 rem = _size - _pos;
    if (size > rem)
      size = (UInt32)rem;
  }

  while (size != 0)
  {
    const UInt64 cacheTag = _pos >> _blockSizeLog;
    const size_t cacheIndex = (size_t)cacheTag & (((size_t)1 << _numBlocksLog) - 1);
    Byte *p = _data + (cacheIndex << _blockSizeLog);

    if (_tags[cacheIndex] != cacheTag)
    {
      _tags[cacheIndex] = kEmptyTag;
      const UInt64 remInBlock = _size - (cacheTag << _blockSizeLog);
      size_t blockSize = (size_t)1 << _blockSizeLog;
      if (blockSize > remInBlock)
        blockSize = (size_t)remInBlock;
      
      RINOK(ReadBlock(cacheTag, p, blockSize))
      
      _tags[cacheIndex] = cacheTag;
    }
    
    const size_t kBlockSize = (size_t)1 << _blockSizeLog;
    const size_t offset = (size_t)_pos & (kBlockSize - 1);
    UInt32 cur = size;
    const size_t rem = kBlockSize - offset;
    if (cur > rem)
      cur = (UInt32)rem;
    
    memcpy(data, p + offset, cur);
    
    if (processedSize)
      *processedSize += cur;
    data = (void *)((const Byte *)data + cur);
    _pos += cur;
    size -= cur;
  }

  return S_OK;
}

 
Z7_COM7F_IMF(CCachedInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _pos; break;
    case STREAM_SEEK_END: offset += _size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _pos = (UInt64)offset;
  if (newPosition)
    *newPosition = (UInt64)offset;
  return S_OK;
}

/* ================ unit: CPP/7zip/Common/StreamUtils.cpp ================ */
// StreamUtils.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static const UInt32 kBlockSize = ((UInt32)1 << 31);


HRESULT InStream_SeekToBegin(IInStream *stream) throw()
{
  return InStream_SeekSet(stream, 0);
}


HRESULT InStream_AtBegin_GetSize(IInStream *stream, UInt64 &sizeRes) throw()
{
#ifdef _WIN32
  {
    Z7_DECL_CMyComPtr_QI_FROM(
        IStreamGetSize,
        streamGetSize, stream)
    if (streamGetSize && streamGetSize->GetSize(&sizeRes) == S_OK)
      return S_OK;
  }
#endif
  const HRESULT hres = InStream_GetSize_SeekToEnd(stream, sizeRes);
  const HRESULT hres2 = InStream_SeekToBegin(stream);
  return hres != S_OK ? hres : hres2;
}


HRESULT InStream_GetPos_GetSize(IInStream *stream, UInt64 &curPosRes, UInt64 &sizeRes) throw()
{
  RINOK(InStream_GetPos(stream, curPosRes))
#ifdef _WIN32
  {
    Z7_DECL_CMyComPtr_QI_FROM(
        IStreamGetSize,
        streamGetSize, stream)
    if (streamGetSize && streamGetSize->GetSize(&sizeRes) == S_OK)
      return S_OK;
  }
#endif
  const HRESULT hres = InStream_GetSize_SeekToEnd(stream, sizeRes);
  const HRESULT hres2 = InStream_SeekSet(stream, curPosRes);
  return hres != S_OK ? hres : hres2;
}



HRESULT ReadStream(ISequentialInStream *stream, void *data, size_t *processedSize) throw()
{
  size_t size = *processedSize;
  *processedSize = 0;
  while (size != 0)
  {
    UInt32 curSize = (size < kBlockSize) ? (UInt32)size : kBlockSize;
    UInt32 processedSizeLoc;
    HRESULT res = stream->Read(data, curSize, &processedSizeLoc);
    *processedSize += processedSizeLoc;
    data = (void *)((Byte *)data + processedSizeLoc);
    size -= processedSizeLoc;
    RINOK(res)
    if (processedSizeLoc == 0)
      return S_OK;
  }
  return S_OK;
}

HRESULT ReadStream_FALSE(ISequentialInStream *stream, void *data, size_t size) throw()
{
  size_t processedSize = size;
  RINOK(ReadStream(stream, data, &processedSize))
  return (size == processedSize) ? S_OK : S_FALSE;
}

HRESULT ReadStream_FAIL(ISequentialInStream *stream, void *data, size_t size) throw()
{
  size_t processedSize = size;
  RINOK(ReadStream(stream, data, &processedSize))
  return (size == processedSize) ? S_OK : E_FAIL;
}

HRESULT WriteStream(ISequentialOutStream *stream, const void *data, size_t size) throw()
{
  while (size != 0)
  {
    UInt32 curSize = (size < kBlockSize) ? (UInt32)size : kBlockSize;
    UInt32 processedSizeLoc;
    HRESULT res = stream->Write(data, curSize, &processedSizeLoc);
    data = (const void *)((const Byte *)data + processedSizeLoc);
    size -= processedSizeLoc;
    RINOK(res)
    if (processedSizeLoc == 0)
      return E_FAIL;
  }
  return S_OK;
}

/* ================ unit: CPP/7zip/Common/UniqBlocks.cpp ================ */
// UniqBlocks.cpp

// amalgamation: header emitted in prologue

#include <string.h>

// amalgamation: header emitted in prologue

unsigned CUniqBlocks::AddUniq(const Byte *data, size_t size)
{
  unsigned left = 0, right = Sorted.Size();
  while (left != right)
  {
    const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
    const unsigned index = Sorted[mid];
    const CByteBuffer &buf = Bufs[index];
    const size_t sizeMid = buf.Size();
    if (size < sizeMid)
      right = mid;
    else if (size > sizeMid)
      left = mid + 1;
    else
    {
      if (size == 0)
        return index;
      const int cmp = memcmp(data, buf, size);
      if (cmp == 0)
        return index;
      if (cmp < 0)
        right = mid;
      else
        left = mid + 1;
    }
  }
  unsigned index = Bufs.Size();
  Sorted.Insert(left, index);
  Bufs.AddNew().CopyFrom(data, size);
  return index;
}

UInt64 CUniqBlocks::GetTotalSizeInBytes() const
{
  UInt64 size = 0;
  FOR_VECTOR (i, Bufs)
    size += Bufs[i].Size();
  return size;
}

void CUniqBlocks::GetReverseMap()
{
  unsigned num = Sorted.Size();
  BufIndexToSortedIndex.ClearAndSetSize(num);
  unsigned *p = &BufIndexToSortedIndex[0];
  const unsigned *sorted = &Sorted[0];
  for (unsigned i = 0; i < num; i++)
    p[sorted[i]] = i;
}

/* ================ unit: CPP/7zip/Common/VirtThread.cpp ================ */
// VirtThread.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static THREAD_FUNC_DECL CoderThread(void *p)
{
  for (;;)
  {
    CVirtThread *t = (CVirtThread *)p;
    t->StartEvent.Lock();
    if (t->Exit)
      return THREAD_FUNC_RET_ZERO;
    t->Execute();
    t->FinishedEvent.Set();
  }
}

WRes CVirtThread::Create()
{
  RINOK_WRes(StartEvent.CreateIfNotCreated_Reset())
  RINOK_WRes(FinishedEvent.CreateIfNotCreated_Reset())
  // StartEvent.Reset();
  // FinishedEvent.Reset();
  Exit = false;
  if (Thread.IsCreated())
    return S_OK;
  return Thread.Create(CoderThread, this);
}

WRes CVirtThread::Start()
{
  Exit = false;
  return StartEvent.Set();
}

void CVirtThread::WaitThreadFinish()
{
  Exit = true;
  if (StartEvent.IsCreated())
    StartEvent.Set();
  if (Thread.IsCreated())
  {
    Thread.Wait_Close();
  }
}
