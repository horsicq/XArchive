/* XArchive amalgamation of 7-Zip 26.01 -- CPP/7zip/Archive standalone handler.
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

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Archive/ElfHandler.cpp ================ */
// ElfHandler.cpp

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

// #define Z7_ELF_SHOW_DETAILS

using namespace NWindows;

static UInt16 Get16(const Byte *p, bool be) { if (be) return GetBe16a(p); return GetUi16a(p); }
static UInt32 Get32(const Byte *p, bool be) { if (be) return GetBe32a(p); return GetUi32a(p); }
static UInt64 Get64(const Byte *p, bool be) { if (be) return GetBe64a(p); return GetUi64a(p); }

#define G16(offs, v) v = Get16(p + (offs), be)
#define G32(offs, v) v = Get32(p + (offs), be)
#define G64(offs, v) v = Get64(p + (offs), be)

namespace NArchive {
namespace NElf {

/*
ELF Structure example:
{
  Header
  Program header table (is used at runtime) (list of segment metadata records)
  {
    Segment (Read)
      Segment : PT_PHDR : header table itself
      Segment : PT_INTERP
      Segment : PT_NOTE
      .rela.dyn (RELA, ALLOC)
    Segment (Execute/Read)
      .text section (PROGBITS, SHF_ALLOC | SHF_EXECINSTR)
    Segment (Read)
      .rodata (PROGBITS, SHF_ALLOC | SHF_WRITE)
      Segment : PT_GNU_EH_FRAME
        .eh_frame_hdr
      .eh_frame
      .gcc_except_table
    ...
    Segment (Write/Read) (VaSize > Size)
      Segment (Read) : PT_GNU_RELRO
      Segment (Write/Read)
      .data
      .bss (Size == 0) (VSize != 0)
  }
  .comment (VA == 0)
  .shstrtab (VA == 0)
  Section header table (the data for linking and relocation)
}

  Last top level segment contains .bss section that requires additional VA space.
  So (VaSize > Size) for that segment.

  Segments can be unsorted (by offset) in table.
  Top level segments has Type=PT_LOAD : "Loadable segment".
  Top level segments usually are aligned for page size (4 KB).
  Another segments (non PT_LOAD segments) are inside PT_LOAD segments.

  (VA-offset == 0) is possible for some sections and segments at the beginning of file.
  (VA-offset == 4KB*N) for most sections and segments where (Size != 0),
  (VA-offset != 4KB*N) for .bss section (last section), because (Size == 0),
    and that section is not mapped from image file.
  Some files contain additional "virtual" 4 KB page in VA space after
  end of data of top level segments (PT_LOAD) before new top level segments.
  So (VA-offset) value can increase by 4 KB step.
*/

#define ELF_CLASS_32 1
#define ELF_CLASS_64 2

#define ELF_DATA_2LSB 1
#define ELF_DATA_2MSB 2

static const unsigned kHeaderSize32 = 0x34;
static const unsigned kHeaderSize64 = 0x40;

static const unsigned kSegmentSize32 = 0x20;
static const unsigned kSegmentSize64 = 0x38;

static const unsigned kSectionSize32 = 0x28;
static const unsigned kSectionSize64 = 0x40;

struct CHeader
{
  bool Mode64;
  bool Be;
  Byte Os;
  Byte AbiVer;

  UInt16 Type;
  UInt16 Machine;
  // UInt32 Version;

  // UInt64 EntryVa;
  UInt64 ProgOffset;
  UInt64 SectOffset;
  UInt32 Flags;
  UInt16 HeaderSize;
  UInt16 SegmentEntrySize;
  UInt16 NumSegments;
  UInt16 SectionEntrySize;
  UInt16 NumSections;
  UInt16 NamesSectIndex;

  bool Parse(const Byte *p);

  UInt32 GetHeadersSize() const { return (UInt32)HeaderSize +
      (UInt32)NumSegments * SegmentEntrySize +
      (UInt32)NumSections * SectionEntrySize; }
};

bool CHeader::Parse(const Byte *p)
{
  switch (p[4])
  {
    case ELF_CLASS_32: Mode64 = false; break;
    case ELF_CLASS_64: Mode64 = true; break;
    default: return false;
  }
  bool be;
  switch (p[5])
  {
    case ELF_DATA_2LSB: be = false; break;
    case ELF_DATA_2MSB: be = true; break;
    default: return false;
  }
  Be = be;
  if (p[6] != 1) // Version
    return false;
  Os = p[7];
  // AbiVer = p[8];
  for (int i = 9; i < 16; i++)
    if (p[i] != 0)
      return false;

  G16(0x10, Type);
  G16(0x12, Machine);
  if (Get32(p + 0x14, be) != 1) // Version
    return false;

  if (Mode64)
  {
    // G64(0x18, EntryVa);
    G64(0x20, ProgOffset); // == kHeaderSize64 == 0x40 usually
    G64(0x28, SectOffset);
    p += 0x30;
    // we expect that fields are aligned
    if (ProgOffset & 7) return false;
    if (SectOffset & 7) return false;
  }
  else
  {
    // G32(0x18, EntryVa);
    G32(0x1C, ProgOffset); // == kHeaderSize32 == 0x34 usually
    G32(0x20, SectOffset);
    p += 0x24;
    if (ProgOffset & 3) return false;
    if (SectOffset & 3) return false;
  }

  G32(0, Flags);
  G16(4, HeaderSize);
  if (HeaderSize != (Mode64 ? kHeaderSize64 : kHeaderSize32))
    return false;

  G16(6, SegmentEntrySize);
  G16(8, NumSegments);
  G16(10, SectionEntrySize);
  G16(12, NumSections);
  G16(14, NamesSectIndex);

  if (ProgOffset < HeaderSize && (ProgOffset || NumSegments)) return false;
  if (SectOffset < HeaderSize && (SectOffset || NumSections)) return false;

  if (SegmentEntrySize == 0) { if (NumSegments) return false; }
  else if (SegmentEntrySize != (Mode64 ? kSegmentSize64 : kSegmentSize32)) return false;

  if (SectionEntrySize == 0) { if (NumSections) return false; }
  else if (SectionEntrySize != (Mode64 ? kSectionSize64 : kSectionSize32)) return false;

  return true;
}


#define PT_PHDR 6  // The program header table itself.
#define PT_GNU_STACK 0x6474e551

static const CUInt32PCharPair g_SegnmentTypes[] =
{
  { 0, "Unused" },
  { 1, "Loadable segment" },
  { 2, "Dynamic linking tables" },
  { 3, "Program interpreter path name" },
  { 4, "Note section" },
  { 5, "SHLIB" },
  { 6, "Program header table" },
  { 7, "TLS" },
  { 0x6474e550, "GNU_EH_FRAME" },
  { PT_GNU_STACK, "GNU_STACK" },
  { 0x6474e552, "GNU_RELRO" }
};


static const char * const g_SegmentFlags[] =
{
    "Execute"
  , "Write"
  , "Read"
};

struct CSegment
{
  UInt32 Type;
  UInt32 Flags;
  UInt64 Offset;
  UInt64 Va;
  UInt64 Size;  // size in file
  UInt64 VSize; // size in memory
#ifdef Z7_ELF_SHOW_DETAILS
  UInt64 Pa;    // usually == Va, or == 0
  UInt64 Align; // if (Align != 0), condition must be met:
                //   (VSize % Align == Offset % Alig)
#endif
  void UpdateTotalSize(UInt64 &totalSize)
  {
    const UInt64 t = Offset + Size;
    if (totalSize < t)
        totalSize = t;
  }
  void Parse(const Byte *p, bool mode64, bool be);
};

void CSegment::Parse(const Byte *p, bool mode64, bool be)
{
  G32(0, Type);
  if (mode64)
  {
    G32(4, Flags);
    G64(8, Offset);
    G64(0x10, Va);
    G64(0x20, Size);
    G64(0x28, VSize);
#ifdef Z7_ELF_SHOW_DETAILS
    G64(0x18, Pa);
    G64(0x30, Align);
#endif
  }
  else
  {
    G32(4, Offset);
    G32(8, Va);
    G32(0x10, Size);
    G32(0x14, VSize);
    G32(0x18, Flags);
#ifdef Z7_ELF_SHOW_DETAILS
    G32(0x0C, Pa);
    G32(0x1C, Align);
#endif
  }
}

// Section_index = 0 means NO section

#define SHN_UNDEF 0

// Section types

/*
#define SHT_NULL           0
#define SHT_PROGBITS       1
#define SHT_SYMTAB         2
#define SHT_STRTAB         3
#define SHT_RELA           4
#define SHT_HASH           5
#define SHT_DYNAMIC        6
#define SHT_NOTE           7
*/
#define SHT_NOBITS         8
/*
#define SHT_REL            9
#define SHT_SHLIB         10
#define SHT_DYNSYM        11
#define SHT_UNKNOWN12     12
#define SHT_UNKNOWN13     13
#define SHT_INIT_ARRAY    14
#define SHT_FINI_ARRAY    15
#define SHT_PREINIT_ARRAY 16
#define SHT_GROUP         17
#define SHT_SYMTAB_SHNDX  18
*/

static const CUInt32PCharPair g_SectTypes[] =
{
  { 0, "NULL" },
  { 1, "PROGBITS" },
  { 2, "SYMTAB" },
  { 3, "STRTAB" },
  { 4, "RELA" },
  { 5, "HASH" },
  { 6, "DYNAMIC" },
  { 7, "NOTE" },
  { 8, "NOBITS" },
  { 9, "REL" },
  { 10, "SHLIB" },
  { 11, "DYNSYM" },
  { 12, "UNKNOWN12" },
  { 13, "UNKNOWN13" },
  { 14, "INIT_ARRAY" },
  { 15, "FINI_ARRAY" },
  { 16, "PREINIT_ARRAY" },
  { 17, "GROUP" },
  { 18, "SYMTAB_SHNDX" },

  { 0x6ffffff5, "GNU_ATTRIBUTES" },
  { 0x6ffffff6, "GNU_HASH" },
  { 0x6ffffffd, "GNU_verdef" },
  { 0x6ffffffe, "GNU_verneed" },
  { 0x6fffffff, "GNU_versym" },
  // { 0x70000001, "X86_64_UNWIND" },
  { 0x70000001, "ARM_EXIDX" },
  { 0x70000002, "ARM_PREEMPTMAP" },
  { 0x70000003, "ARM_ATTRIBUTES" },
  { 0x70000004, "ARM_DEBUGOVERLAY" },
  { 0x70000005, "ARM_OVERLAYSECTION" }
};


// SHF_ flags
static const CUInt32PCharPair g_SectionFlags[] =
{
  { 0, "WRITE" },
  { 1, "ALLOC" },
  { 2, "EXECINSTR" },

  { 4, "MERGE" },
  { 5, "STRINGS" },
  { 6, "INFO_LINK" },
  { 7, "LINK_ORDER" },
  { 8, "OS_NONCONFORMING" },
  { 9, "GROUP" },
  { 10, "TLS" },
  { 11, "COMPRESSED" },
  { 12, "DP_SECTION" },
  { 13, "XCORE_SHF_CP_SECTION" },
  { 28, "64_LARGE" },
};

struct CSection
{
  UInt32 Name;
  UInt32 Type;
  UInt64 Flags;
  UInt64 Va;
  UInt64 Offset;
  UInt64 VSize;
  UInt32 Link;
  UInt32 Info;
  UInt64 AddrAlign;
  UInt64 EntSize;

  UInt64 GetSize() const { return Type == SHT_NOBITS ? 0 : VSize; }

  void UpdateTotalSize(UInt64 &totalSize)
  {
    const UInt64 t = Offset + GetSize();
    if (totalSize < t)
        totalSize = t;
  }
  bool Parse(const Byte *p, bool mode64, bool be);
};

bool CSection::Parse(const Byte *p, bool mode64, bool be)
{
  G32(0, Name);
  G32(4, Type);
  if (mode64)
  {
    G64(0x08, Flags);
    G64(0x10, Va);
    G64(0x18, Offset);
    G64(0x20, VSize);
    G32(0x28, Link);
    G32(0x2C, Info);
    G64(0x30, AddrAlign);
    G64(0x38, EntSize);
  }
  else
  {
    G32(0x08, Flags);
    G32(0x0C, Va);
    G32(0x10, Offset);
    G32(0x14, VSize);
    G32(0x18, Link);
    G32(0x1C, Info);
    G32(0x20, AddrAlign);
    G32(0x24, EntSize);
  }
  if (EntSize >= ((UInt32)1 << 31))
    return false;
  if (EntSize >= ((UInt32)1 << 10) &&
      EntSize >= VSize &&
      VSize != 0)
    return false;
  return true;
}


static const char * const g_Machines[] =
{
    "None"
  , "AT&T WE 32100"
  , "SPARC"
  , "Intel 386"
  , "Motorola 68000"
  , "Motorola 88000"
  , "Intel 486"
  , "Intel i860"
  , "MIPS"
  , "IBM S/370"
  , "MIPS RS3000 LE"
  , "RS6000"
  , NULL
  , NULL
  , NULL
  , "PA-RISC"
  , "nCUBE"
  , "Fujitsu VPP500"
  , "SPARC 32+"
  , "Intel i960"
  , "PowerPC"
  , "PowerPC 64-bit"
  , "IBM S/390"
  , "SPU"
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , "NEX v800"
  , "Fujitsu FR20"
  , "TRW RH-32"
  , "Motorola RCE"
  , "ARM"
  , "Alpha-STD"
  , "Hitachi SH"
  , "SPARC-V9"
  , "Siemens Tricore"
  , "ARC"
  , "H8/300"
  , "H8/300H"
  , "H8S"
  , "H8/500"
  , "IA-64"
  , "Stanford MIPS-X"
  , "Motorola ColdFire"
  , "M68HC12"
  , "Fujitsu MMA"
  , "Siemens PCP"
  , "Sony nCPU"
  , "Denso NDR1"
  , "Motorola StarCore"
  , "Toyota ME16"
  , "ST100"
  , "Advanced Logic TinyJ"
  , "AMD64"
  , "Sony DSP"
  , NULL
  , NULL
  , "Siemens FX66"
  , "ST9+"
  , "ST7"
  , "MC68HC16"
  , "MC68HC11"
  , "MC68HC08"
  , "MC68HC05"
  , "Silicon Graphics SVx"
  , "ST19"
  , "Digital VAX"
  , "Axis CRIS"
  , "Infineon JAVELIN"
  , "Element 14 FirePath"
  , "LSI ZSP"
  , "MMIX"
  , "HUANY"
  , "SiTera Prism"
  , "Atmel AVR"
  , "Fujitsu FR30"
  , "Mitsubishi D10V"
  , "Mitsubishi D30V"
  , "NEC v850"
  , "Mitsubishi M32R"
  , "Matsushita MN10300"
  , "Matsushita MN10200"
  , "picoJava"
  , "OpenRISC"
  , "ARC Tangent-A5"
  , "Tensilica Xtensa"
  , "Alphamosaic VideoCore"
  , "Thompson MM GPP"
  , "National Semiconductor 32K"
  , "Tenor Network TPC"
  , "Trebia SNP 1000"
  , "ST200"
  , "Ubicom IP2xxx"
  , "MAX"
  , "NS CompactRISC"
  , "Fujitsu F2MC16"
  , "TI msp430"
  , "Blackfin (DSP)"
  , "SE S1C33"
  , "Sharp embedded"
  , "Arca RISC"
  , "Unicore"
  , "eXcess"
  , "DXP"
  , "Altera Nios II"
  , "NS CRX"
  , "Motorola XGATE"
  , "Infineon C16x/XC16x"
  , "Renesas M16C"
  , "Microchip Technology dsPIC30F"
  , "Freescale CE"
  , "Renesas M32C"
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , "Altium TSK3000"
  , "Freescale RS08"
  , "Analog Devices SHARC"
  , "Cyan Technology eCOG2"
  , "Sunplus S+core7 RISC"
  , "NJR 24-bit DSP"
  , "Broadcom VideoCore III"
  , "Lattice FPGA"
  , "SE C17"
  , "TI TMS320C6000"
  , "TI TMS320C2000"
  , "TI TMS320C55x"
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , "STM 64bit VLIW Data Signal"
  , "Cypress M8C"
  , "Renesas R32C"
  , "NXP TriMedia"
  , "Qualcomm Hexagon"
  , "Intel 8051"
  , "STMicroelectronics STxP7x"
  , "Andes"
  , "Cyan Technology eCOG1X"
  , "Dallas Semiconductor MAXQ30"
  , "NJR 16-bit DSP"
  , "M2000"
  , "Cray NV2"
  , "Renesas RX"
  , "Imagination Technologies META"
  , "MCST Elbrus"
  , "Cyan Technology eCOG16"
  , "National Semiconductor CR16"
  , "Freescale ETPUnit"
  , "Infineon SLE9X"
  , "Intel L10M"
  , "Intel K10M"
  , NULL
  , "ARM64"
  , NULL
  , "Atmel AVR32"
  , "STM8"
  , "Tilera TILE64"
  , "Tilera TILEPro"
  , "Xilinx MicroBlaze"
  , "NVIDIA CUDA"
  , "Tilera TILE-Gx"
  , "CloudShield"
  , "KIPO-KAIST Core-A 1st"
  , "KIPO-KAIST Core-A 2nd"
  , "Synopsys ARCompact V2"
  , "Open8"
  , "Renesas RL78"
  , "Broadcom VideoCore V"
  , "Renesas 78KOR"
  , "Freescale 56800EX" // 200
};

static const CUInt32PCharPair g_MachinePairs[] =
{
  { 243, "RISC-V" },
  { 258, "LoongArch" },
  { 0x9026, "Alpha" },  // EM_ALPHA_EXP, obsolete, (used by NetBSD/alpha) (written in the absence of an ABI)
  { 0xbaab, "Xilinx MicroBlaze" }
};

static const CUInt32PCharPair g_OS[] =
{
  { 0, "None" },
  { 1, "HP-UX" },
  { 2, "NetBSD" },
  { 3, "Linux" },
  { 4, "Hurd" },

  { 6, "Solaris" },
  { 7, "AIX" },
  { 8, "IRIX" },
  { 9, "FreeBSD" },
  { 10, "TRU64" },
  { 11, "Novell Modesto" },
  { 12, "OpenBSD" },
  { 13, "OpenVMS" },
  { 14, "HP NSK" },
  { 15, "AROS" },
  { 16, "FenixOS" },
  { 17, "CloudABI" },
  { 18, "OpenVOS" },
  { 64, "Bare-metal TMS320C6000" },
  { 65, "Linux TMS320C6000" },
  { 97, "ARM" },
  { 255, "Standalone" }
};

#define k_Machine_MIPS 8
#define k_Machine_ARM 40
#define k_Machine_RISCV 243

/*
#define EF_ARM_ABIMASK        0xFF000000
#define EF_ARM_BE8            0x00800000
#define EF_ARM_GCCMASK        0x00400FFF
#define EF_ARM_ABI_FLOAT_SOFT 0x00000200
#define EF_ARM_ABI_FLOAT_HARD 0x00000400
*/

static const CUInt32PCharPair g_ARM_Flags[] =
{
  { 1, "HasEntry" },
  { 9, "SF" },
  { 10, "HF" },
  { 23, "BE8" }
};


static const CUInt32PCharPair g_MIPS_Flags[] =
{
  { 0, "NOREORDER" },
  { 1, "PIC" },
  { 2, "CPIC" },
  { 3, "XGOT" },
  { 4, "64BIT_WHIRL" },
  { 5, "ABI2" },
  { 6, "ABI_ON32" },
  { 10, "NAN2008" },
  { 25, "MicroMIPS" },
  { 26, "M16" },
  { 27, "MDMX" }
};

static const char * const g_RISCV_Flags[] =
{
  "RVC",
  NULL,
  NULL,
  "RVE",
  "TSO"
};


// #define ET_NONE 0
#define ET_REL  1
// #define ET_EXEC 2
#define ET_DYN  3
// #define ET_CORE 4

static const char * const g_Types[] =
{
    "None"
  , "Relocatable file"
  , "Executable file"
  , "Shared object file"
  , "Core file"
};




Z7_CLASS_IMP_CHandler_IInArchive_1(
    IArchiveAllowTail
)
  CRecordVector<CSegment> _segments;
  CRecordVector<CSection> _sections;
  CByteBuffer _namesData;
  CMyComPtr<IInStream> _inStream;
  UInt64 _totalSize;
  CHeader _header;
  bool _headersError;
  bool _allowTail;
  bool _stackFlags_Defined;
  UInt32 _stackFlags;

  void GetSectionName(UInt32 index, NCOM::CPropVariant &prop, bool showNULL) const;
  HRESULT Open2(IInStream *stream);
public:
  CHandler(): _allowTail(false) {}
};

void CHandler::GetSectionName(UInt32 index, NCOM::CPropVariant &prop, bool showNULL) const
{
  if (index >= _sections.Size())
    prop = index; // it's possible for some file, but maybe it's ERROR case
  else
  {
    const CSection &section = _sections[index];
    const UInt32 offset = section.Name;
    if (index == SHN_UNDEF /* && section.Type == SHT_NULL && offset == 0 */)
    {
      if (showNULL)
        prop = "NULL";
      return;
    }
    const Byte *p = _namesData;
    const size_t size = _namesData.Size();
    for (size_t i = offset; i < size; i++)
      if (p[i] == 0)
      {
        prop = (const char *)(p + offset);
        return;
      }
    prop = "ERROR";
  }
}

static const Byte kArcProps[] =
{
  kpidCpu,
  kpidBit64,
  kpidBigEndian,
  kpidHostOS,
  kpidCharacts,
  kpidComment,
  kpidHeadersSize
};

enum
{
  kpidLinkSection = kpidUserDefined,
  kpidInfoSection,
  kpidEntrySize
#ifdef Z7_ELF_SHOW_DETAILS
  // , kpidAlign
  , kpidPa
  , kpidDelta
  , kpidOffsetEnd
#endif
};

static const CStatProp kProps[] =
{
  { NULL, kpidPath, VT_BSTR },
  { NULL, kpidSize, VT_UI8 },
  { NULL, kpidVirtualSize, VT_UI8 },
  { NULL, kpidOffset, VT_UI8 },
  { NULL, kpidVa, VT_UI8 },
  { NULL, kpidType, VT_BSTR },
  { NULL, kpidCharacts, VT_BSTR }
#ifdef Z7_ELF_SHOW_DETAILS
  // , { "Align", kpidAlign, VT_UI8 }
  , { NULL, kpidClusterSize, VT_UI8 }
  , { "PA", kpidPa, VT_UI8 }
  , { "End offset", kpidOffsetEnd, VT_UI8 }
  , { "Delta (VA-Offset)", kpidDelta, VT_UI8 }
#endif
  , { "Entry Size", kpidEntrySize, VT_UI8}
  , { "Link Section", kpidLinkSection, VT_BSTR}
  , { "Info Section", kpidInfoSection, VT_BSTR}
};

IMP_IInArchive_Props_WITH_NAME
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: prop = _totalSize; break;
    case kpidHeadersSize: prop = _header.GetHeadersSize(); break;
    case kpidBit64: if (_header.Mode64) prop = _header.Mode64; break;
    case kpidBigEndian: if (_header.Be) prop = _header.Be; break;
    case kpidShortComment:

    case kpidCpu:
    {
      AString s;
      if (_header.Machine < Z7_ARRAY_SIZE(g_Machines))
      {
        const char *name = g_Machines[_header.Machine];
        if (name)
          s = name;
      }
      if (s.IsEmpty())
        s = TypePairToString(g_MachinePairs, Z7_ARRAY_SIZE(g_MachinePairs), _header.Machine);
      UInt32 flags = _header.Flags;
      if (flags)
      {
        s.Add_Space();
        if (_header.Machine == k_Machine_ARM)
        {
          s += FlagsToString(g_ARM_Flags, Z7_ARRAY_SIZE(g_ARM_Flags), flags & (((UInt32)1 << 24) - 1));
          s += " ABI:";
          s.Add_UInt32(flags >> 24);
        }
        else if (_header.Machine == k_Machine_MIPS)
        {
          const UInt32 ver = flags >> 28;
          s.Add_Char('v');
          s.Add_UInt32(ver);
          flags &= ((UInt32)1 << 28) - 1;
          const UInt32 abi = (flags >> 12) & 7;
          if (abi)
          {
            s += " ABI:";
            s.Add_UInt32(abi);
          }
          flags &= ~((UInt32)7 << 12);
          s.Add_Space();
          s += FlagsToString(g_MIPS_Flags, Z7_ARRAY_SIZE(g_MIPS_Flags), flags);
        }
        else if (_header.Machine == k_Machine_RISCV)
        {
          s += "FLOAT_";
          const UInt32 fl = (flags >> 1) & 3;
          /*
          static const char * const g_RISCV_Flags_Float[] =
            { "SOFT", "SINGLE", "DOUBLE", "QUAD" };
          s += g_RISCV_Flags_Float[fl];
          */
          if (fl)
            s.Add_UInt32(16u << fl);
          else
            s += "SOFT";
          s.Add_Space();
          flags &= ~(UInt32)6;
          s += FlagsToString(g_RISCV_Flags, Z7_ARRAY_SIZE(g_RISCV_Flags), flags);
        }
#if 0
#define k_Machine_LOONGARCH 258
        else if (_header.Machine == k_Machine_LOONGARCH)
        {
          s += "ABI:";
          s.Add_UInt32((flags >> 6) & 3);
          s.Add_Dot();
          s.Add_UInt32((flags >> 3) & 7);
          s.Add_Dot();
#if 1
          s.Add_UInt32(flags & 7);
#else
          static const char k_LoongArch_Float_Type[8] = { '0', 's', 'f', 'd', '4' ,'5', '6', '7' };
          s.Add_Char(k_LoongArch_Float_Type[flags & 7]);
#endif
          flags &= ~(UInt32)0xff;
          if (flags)
          {
            s.Add_Colon();
            char sz[16];
            ConvertUInt32ToHex(flags, sz);
            s += sz;
          }
        }
#endif
        else
        {
          char sz[16];
          ConvertUInt32ToHex(flags, sz);
          s += sz;
        }
      }
      prop = s;
      break;
    }

    case kpidHostOS: PAIR_TO_PROP(g_OS, _header.Os, prop); break;
    case kpidCharacts: TYPE_TO_PROP(g_Types, _header.Type, prop); break;
    case kpidComment:
    {
      AString s;
      if (_stackFlags_Defined)
      {
        s += "STACK: ";
        s += FlagsToString(g_SegmentFlags, Z7_ARRAY_SIZE(g_SegmentFlags), _stackFlags);
        s.Add_LF();
        /*
        if (_header.EntryVa)
        {
          s += "Entry point: 0x";
          char temp[16 + 4];
          ConvertUInt64ToHex(_header.EntryVa, temp);
          s += temp;
          s.Add_LF();
        }
        */
      }
      if (_header.NumSegments)
      {
        s += "Segments: ";
        s.Add_UInt32(_header.NumSegments);
        s.Add_LF();
      }
      if (_header.NumSections)
      {
        s += "Sections: ";
        s.Add_UInt32(_header.NumSections);
        s.Add_LF();
      }
      prop = s;
      break;
    }
    case kpidExtension:
    {
      const char *s = NULL;
      if (_header.Type == ET_DYN)
        s = "so";
      else if (_header.Type == ET_REL)
        s = "o";
      if (s)
        prop = s;
      break;
    }
    // case kpidIsSelfExe: prop = (_header.Type != ET_DYN)  && (_header.Type == ET_REL); break;
    case kpidErrorFlags:
    {
      UInt32 flags = 0;
      if (_headersError) flags |= kpv_ErrorFlags_HeadersError;
      if (flags != 0)
        prop = flags;
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
  NCOM::CPropVariant prop;
  if (index < _segments.Size())
  {
    const CSegment &item = _segments[index];
    switch (propID)
    {
      case kpidPath:
      {
        char sz[16];
        ConvertUInt32ToString(index, sz);
        prop = sz;
        break;
      }
      case kpidOffset: prop = item.Offset; break;
      case kpidVa: prop = item.Va; break;
#ifdef Z7_ELF_SHOW_DETAILS
      case kpidDelta: if (item.Va) { prop = item.Va - item.Offset; } break;
      case kpidOffsetEnd: prop = item.Offset + item.Size; break;
      case kpidPa: prop = item.Pa; break;
      case kpidClusterSize: prop = item.Align; break;
#endif
      case kpidSize:
      case kpidPackSize: prop = (UInt64)item.Size; break;
      case kpidVirtualSize: prop = (UInt64)item.VSize; break;
      case kpidType: PAIR_TO_PROP(g_SegnmentTypes, item.Type, prop); break;
      case kpidCharacts: FLAGS_TO_PROP(g_SegmentFlags, item.Flags, prop); break;
    }
  }
  else
  {
    index -= _segments.Size();
    const CSection &item = _sections[index];
    switch (propID)
    {
      case kpidPath: GetSectionName(index, prop, true); break;
      case kpidOffset: prop = item.Offset; break;
      case kpidVa: prop = item.Va; break;
#ifdef Z7_ELF_SHOW_DETAILS
      case kpidDelta: if (item.Va) { prop = item.Va - item.Offset; } break;
      case kpidOffsetEnd: prop = item.Offset + item.GetSize(); break;
#endif
      case kpidSize:
      case kpidPackSize: prop = (UInt64)(item.Type == SHT_NOBITS ? 0 : item.VSize); break;
      case kpidVirtualSize: prop = item.GetSize(); break;
      case kpidType: PAIR_TO_PROP(g_SectTypes, item.Type, prop); break;
      case kpidCharacts: FLAGS_TO_PROP(g_SectionFlags, (UInt32)item.Flags, prop); break;
      // case kpidAlign: prop = item.Align; break;
      case kpidLinkSection: GetSectionName(item.Link, prop, false); break;
      case kpidInfoSection: GetSectionName(item.Info, prop, false); break;
      case kpidEntrySize: prop = (UInt64)item.EntSize; break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::Open2(IInStream *stream)
{
  {
    const UInt32 kStartSize = kHeaderSize64;
    UInt64 h64[kStartSize / 8];
    RINOK(ReadStream_FALSE(stream, h64, kStartSize))
    const Byte *h = (const Byte *)(const void *)h64;
    if (GetUi32a(h) != 0x464c457f)
      return S_FALSE;
    if (!_header.Parse(h))
      return S_FALSE;
  }

  _totalSize = _header.HeaderSize;

  bool addSegments = false;
  bool addSections = false;
  // first section usually is NULL (with zero offsets and zero sizes).
  if (_header.NumSegments == 0 || _header.NumSections > 1)
    addSections = true;
  else
    addSegments = true;
#ifdef Z7_ELF_SHOW_DETAILS
  addSections = true;
  addSegments = true;
#endif

  if (_header.NumSegments)
  {
    if (_header.ProgOffset > (UInt64)1 << 60) return S_FALSE;
    RINOK(InStream_SeekSet(stream, _header.ProgOffset))
    const size_t size = (size_t)_header.SegmentEntrySize * _header.NumSegments;
    CByteArr buf(size);
    RINOK(ReadStream_FALSE(stream, buf, size))
    {
      const UInt64 total = _header.ProgOffset + size;
      if (_totalSize < total)
          _totalSize = total;
    }
    if (addSegments)
      _segments.ClearAndReserve(_header.NumSegments);
    const Byte *p = buf;
    for (unsigned i = 0; i < _header.NumSegments; i++, p += _header.SegmentEntrySize)
    {
      CSegment seg;
      seg.Parse(p, _header.Mode64, _header.Be);
      seg.UpdateTotalSize(_totalSize);
      if (seg.Type == PT_GNU_STACK && !_stackFlags_Defined)
      {
        _stackFlags = seg.Flags;
        _stackFlags_Defined = true;
      }
      if (addSegments
          // we don't show program header table segment
          && seg.Type != PT_PHDR
          )
        _segments.AddInReserved(seg);
    }
  }

  if (_header.NumSections)
  {
    if (_header.SectOffset > (UInt64)1 << 60) return S_FALSE;
    RINOK(InStream_SeekSet(stream, _header.SectOffset))
    const size_t size = (size_t)_header.SectionEntrySize * _header.NumSections;
    CByteArr buf(size);
    RINOK(ReadStream_FALSE(stream, buf, size))
    {
      const UInt64 total = _header.SectOffset + size;
      if (_totalSize < total)
          _totalSize = total;
    }
    if (addSections)
      _sections.ClearAndReserve(_header.NumSections);
    const Byte *p = buf;
    for (unsigned i = 0; i < _header.NumSections; i++, p += _header.SectionEntrySize)
    {
      CSection sect;
      if (!sect.Parse(p, _header.Mode64, _header.Be))
      {
        _headersError = true;
        return S_FALSE;
      }
      sect.UpdateTotalSize(_totalSize);
      if (addSections)
        _sections.AddInReserved(sect);
    }
  }

  if (addSections)
  {
    if (_header.NamesSectIndex < _sections.Size())
    {
      const CSection &sect = _sections[_header.NamesSectIndex];
      const UInt64 size = sect.GetSize();
      if (size && size < ((UInt64)1 << 31)
          && (Int64)sect.Offset >= 0)
      {
        _namesData.Alloc((size_t)size);
        RINOK(InStream_SeekSet(stream, sect.Offset))
        RINOK(ReadStream_FALSE(stream, _namesData, (size_t)size))
      }
    }
    /*
    // we cannot delete "NULL" sections,
    // because we have links to sections array via indexes
    for (int i = _sections.Size() - 1; i >= 0; i--)
      if (_sections[i].Type == SHT_NULL)
        _items.Delete(i);
    */
  }

  if (!_allowTail)
  {
    UInt64 fileSize;
    RINOK(InStream_GetSize_SeekToEnd(stream, fileSize))
    if (fileSize > _totalSize)
      return S_FALSE;
  }

  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */))
{
  COM_TRY_BEGIN
  Close();
  RINOK(Open2(inStream))
  _inStream = inStream;
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _totalSize = 0;
  _headersError = false;
  _stackFlags_Defined = false;

  _inStream.Release();
  _segments.Clear();
  _sections.Clear();
  _namesData.Free();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _segments.Size() + _sections.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _segments.Size() + _sections.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const UInt32 index = allFilesMode ? i : indices[i];
    totalSize += (index < _segments.Size()) ?
        _segments[index].Size :
        _sections[index - _segments.Size()].GetSize();
  }
  RINOK(extractCallback->SetTotal(totalSize))

  totalSize = 0;
  UInt64 currentItemSize;
  
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> inStream;
  inStream->SetStream(_inStream);

  for (i = 0;; i++, totalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    UInt64 offset;
    if (index < _segments.Size())
    {
      const CSegment &item = _segments[index];
      currentItemSize = item.Size;
      offset = item.Offset;
    }
    else
    {
      const CSection &item = _sections[index - _segments.Size()];
      currentItemSize = item.GetSize();
      offset = item.Offset;
    }
    {
      CMyComPtr<ISequentialOutStream> outStream;
      RINOK(extractCallback->GetStream(index, &outStream, askMode))
      if (!testMode && !outStream)
        continue;
      RINOK(extractCallback->PrepareOperation(askMode))
      RINOK(InStream_SeekSet(_inStream, offset))
      inStream->Init(currentItemSize);
      RINOK(copyCoder.Interface()->Code(inStream, outStream, NULL, NULL, lps))
    }
    RINOK(extractCallback->SetOperationResult(copyCoder->TotalSize == currentItemSize ?
        NExtract::NOperationResult::kOK:
        NExtract::NOperationResult::kDataError))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::AllowTail(Int32 allowTail))
{
  _allowTail = IntToBool(allowTail);
  return S_OK;
}

static const Byte k_Signature[] = { 0x7F, 'E', 'L', 'F' };

REGISTER_ARC_I(
  "ELF", "elf", NULL, 0xDE,
  k_Signature,
  0,
  NArcInfoFlags::kPreArc,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/ExtHandler.cpp ================ */
// ExtHandler.cpp

// amalgamation: header emitted in prologue

// #define SHOW_DEBUG_INFO

// #include <stdio.h>
// #define PRF2(x) x

#define PRF2(x)

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
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

using namespace NWindows;

UInt32 LzhCrc16Update(UInt32 crc, const void *data, size_t size);

namespace NArchive {
namespace NExt {

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

#define LE_16(offs, dest) dest = Get16(p + (offs));
#define LE_32(offs, dest) dest = Get32(p + (offs));
#define LE_64(offs, dest) dest = Get64(p + (offs));

#define HI_16(offs, dest) dest |= (((UInt32)Get16(p + (offs))) << 16);
#define HI_32(offs, dest) dest |= (((UInt64)Get32(p + (offs))) << 32);

/*
static UInt32 g_Crc32CTable[256];

static struct CInitCrc32C
{
  CInitCrc32C()
  {
    for (unsigned i = 0; i < 256; i++)
    {
      UInt32 r = i;
      unsigned j;
      for (j = 0; j < 8; j++)
        r = (r >> 1) ^ (0x82F63B78 & ~((r & 1) - 1));
      g_Crc32CTable[i] = r;
    }
  }
} g_InitCrc32C;

#define CRC32C_INIT_VAL 0xFFFFFFFF
#define CRC32C_GET_DIGEST(crc) ((crc) ^ CRC_INIT_VAL)
#define CRC32C_UPDATE_BYTE(crc, b) (g_Crc32CTable[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

static UInt32 Crc32C_Update(UInt32 crc, Byte const *data, size_t size)
{
  for (size_t i = 0; i < size; i++)
    crc = CRC32C_UPDATE_BYTE(crc, data[i]);
  return crc;
}

static UInt32 Crc32C_Calc(Byte const *data, size_t size)
{
  return Crc32C_Update(CRC32C_INIT_VAL, data, size);
}
*/


#define CRC16_INIT_VAL 0xFFFF

#define Crc16Update(crc, data, size)  LzhCrc16Update(crc, data, size)

static UInt32 Crc16Calc(Byte const *data, size_t size)
{
  return Crc16Update(CRC16_INIT_VAL, data, size);
}


#define EXT4_GOOD_OLD_INODE_SIZE 128
#define EXT_NODE_SIZE_MIN 128


// inodes numbers
 
// #define k_INODE_BAD          1  // Bad blocks
#define k_INODE_ROOT         2  // Root dir
// #define k_INODE_USR_QUOTA    3  // User quota
// #define k_INODE_GRP_QUOTA    4  // Group quota
// #define k_INODE_BOOT_LOADER  5  // Boot loader
// #define k_INODE_UNDEL_DIR    6  // Undelete dir
#define k_INODE_RESIZE       7  // Reserved group descriptors
// #define k_INODE_JOURNAL      8  // Journal

// First non-reserved inode for old ext4 filesystems
#define k_INODE_GOOD_OLD_FIRST  11

static const char * const k_SysInode_Names[] =
{
    "0"
  , "Bad"
  , "Root"
  , "UserQuota"
  , "GroupQuota"
  , "BootLoader"
  , "Undelete"
  , "Resize"
  , "Journal"
  , "Exclude"
  , "Replica"
};

static const char * const kHostOS[] =
{
    "Linux"
  , "Hurd"
  , "Masix"
  , "FreeBSD"
  , "Lites"
};

static const char * const g_FeatureCompat_Flags[] =
{
    "DIR_PREALLOC"
  , "IMAGIC_INODES"
  , "HAS_JOURNAL"
  , "EXT_ATTR"
  , "RESIZE_INODE"
  , "DIR_INDEX"
  , "LAZY_BG" // not in Linux
  , NULL // { 7, "EXCLUDE_INODE" // not used
  , NULL // { 8, "EXCLUDE_BITMAP" // not in kernel
  , "SPARSE_SUPER2"
};
  

#define EXT4_FEATURE_INCOMPAT_FILETYPE (1 << 1)
#define EXT4_FEATURE_INCOMPAT_64BIT (1 << 7)

static const char * const g_FeatureIncompat_Flags[] =
{
    "COMPRESSION"
  , "FILETYPE"
  , "RECOVER" /* Needs recovery */
  , "JOURNAL_DEV" /* Journal device */
  , "META_BG"
  , NULL
  , "EXTENTS" /* extents support */
  , "64BIT"
  , "MMP"
  , "FLEX_BG"
  , "EA_INODE" /* EA in inode */
  , NULL
  , "DIRDATA" /* data in dirent */
  , "BG_USE_META_CSUM" /* use crc32c for bg */
  , "LARGEDIR" /* >2GB or 3-lvl htree */
  , "INLINE_DATA" /* data in inode */
  , "ENCRYPT" // 16
};


static const UInt32 RO_COMPAT_GDT_CSUM = 1 << 4;
static const UInt32 RO_COMPAT_METADATA_CSUM = 1 << 10;

static const char * const g_FeatureRoCompat_Flags[] =
{
    "SPARSE_SUPER"
  , "LARGE_FILE"
  , "BTREE_DIR"
  , "HUGE_FILE"
  , "GDT_CSUM"
  , "DIR_NLINK"
  , "EXTRA_ISIZE"
  , "HAS_SNAPSHOT"
  , "QUOTA"
  , "BIGALLOC"
  , "METADATA_CSUM"
  , "REPLICA"
  , "READONLY" // 12
};



static const UInt32 k_NodeFlags_HUGE = (UInt32)1 << 18;
static const UInt32 k_NodeFlags_EXTENTS = (UInt32)1 << 19;


static const char * const g_NodeFlags[] =
{
    "SECRM"
  , "UNRM"
  , "COMPR"
  , "SYNC"
  , "IMMUTABLE"
  , "APPEND"
  , "NODUMP"
  , "NOATIME"
  , "DIRTY"
  , "COMPRBLK"
  , "NOCOMPR"
  , "ENCRYPT"
  , "INDEX"
  , "IMAGIC"
  , "JOURNAL_DATA"
  , "NOTAIL"
  , "DIRSYNC"
  , "TOPDIR"
  , "HUGE_FILE"
  , "EXTENTS"
  , NULL
  , "EA_INODE"
  , "EOFBLOCKS"
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , "INLINE_DATA" // 28
};


enum
{
  k_Type_UNKNOWN,
  k_Type_REG_FILE,
  k_Type_DIR,
  k_Type_CHRDEV,
  k_Type_BLKDEV,
  k_Type_FIFO,
  k_Type_SOCK,
  k_Type_SYMLINK
};

static const UInt16 k_TypeToMode[] =
{
  0,
  MY_LIN_S_IFREG,
  MY_LIN_S_IFDIR,
  MY_LIN_S_IFCHR,
  MY_LIN_S_IFBLK,
  MY_LIN_S_IFIFO,
  MY_LIN_S_IFSOCK,
  MY_LIN_S_IFLNK
};


#define EXT4_GOOD_OLD_REV 0  // old (original) format
// #define EXT4_DYNAMIC_REV 1  // V2 format with dynamic inode sizes
 
struct CHeader
{
  unsigned BlockBits;
  unsigned ClusterBits;

  UInt32 NumInodes;
  UInt64 NumBlocks;
  // UInt64 NumBlocksSuper;
  UInt64 NumFreeBlocks;
  UInt32 NumFreeInodes;
  // UInt32 FirstDataBlock;

  UInt32 BlocksPerGroup;
  UInt32 ClustersPerGroup;
  UInt32 InodesPerGroup;

  UInt32 MountTime;
  UInt32 WriteTime;

  // UInt16 NumMounts;
  // UInt16 NumMountsMax;
  
  // UInt16 State;
  // UInt16 Errors;
  // UInt16 MinorRevLevel;

  UInt32 LastCheckTime;
  // UInt32 CheckInterval;
  UInt32 CreatorOs;
  UInt32 RevLevel;

  // UInt16 DefResUid;
  // UInt16 DefResGid;

  UInt32 FirstInode;
  UInt16 InodeSize;
  UInt16 BlockGroupNr;
  UInt32 FeatureCompat;
  UInt32 FeatureIncompat;
  UInt32 FeatureRoCompat;
  Byte Uuid[16];
  char VolName[16];
  char LastMount[64];
  // UInt32 BitmapAlgo;

  UInt32 JournalInode;
  UInt16 GdSize; // = 64 if 64-bit();
  UInt32 CTime;
  UInt16 MinExtraISize;
  // UInt16 WantExtraISize;
  // UInt32 Flags;
  // Byte LogGroupsPerFlex;
  // Byte ChecksumType;

  UInt64 WrittenKB;

  bool IsOldRev() const { return RevLevel == EXT4_GOOD_OLD_REV; }

  UInt64 GetNumGroups() const { return (NumBlocks + BlocksPerGroup - 1) / BlocksPerGroup; }
  UInt64 GetNumGroups2() const { return ((UInt64)NumInodes + InodesPerGroup - 1) / InodesPerGroup; }

  bool IsThereFileType() const { return (FeatureIncompat & EXT4_FEATURE_INCOMPAT_FILETYPE) != 0; }
  bool Is64Bit() const { return (FeatureIncompat & EXT4_FEATURE_INCOMPAT_64BIT) != 0; }
  bool UseGdtChecksum() const { return (FeatureRoCompat & RO_COMPAT_GDT_CSUM) != 0; }
  bool UseMetadataChecksum() const { return (FeatureRoCompat & RO_COMPAT_METADATA_CSUM) != 0; }

  UInt64 GetPhySize() const { return NumBlocks << BlockBits; }

  bool Parse(const Byte *p);
};


static int inline GetLog(UInt32 num)
{
  for (unsigned i = 0; i < 32; i++)
    if (((UInt32)1 << i) == num)
      return (int)i;
  return -1;
}

static bool inline IsEmptyData(const Byte *data, unsigned size)
{
  for (unsigned i = 0; i < size; i++)
    if (data[i] != 0)
      return false;
  return true;
}


bool CHeader::Parse(const Byte *p)
{
  if (GetUi16(p + 0x38) != 0xEF53)
    return false;
  
  LE_32 (0x18, BlockBits)
  LE_32 (0x1C, ClusterBits)
  
  if (ClusterBits != 0 && BlockBits != ClusterBits)
    return false;

  if (BlockBits > 16 - 10)
    return false;
  BlockBits += 10;
  
  LE_32 (0x00, NumInodes)
  LE_32 (0x04, NumBlocks)
  // LE_32 (0x08, NumBlocksSuper);
  LE_32 (0x0C, NumFreeBlocks)
  LE_32 (0x10, NumFreeInodes)

  if (NumInodes < 2 || NumInodes <= NumFreeInodes)
    return false;

  UInt32 FirstDataBlock;
  LE_32 (0x14, FirstDataBlock)
  if (FirstDataBlock != (unsigned)(BlockBits == 10 ? 1 : 0))
    return false;
    
  LE_32 (0x20, BlocksPerGroup)
  LE_32 (0x24, ClustersPerGroup)

  if (BlocksPerGroup != ClustersPerGroup)
    return false;
  if (BlocksPerGroup == 0)
    return false;
  if (BlocksPerGroup != ((UInt32)1 << (BlockBits + 3)))
  {
    // it's allowed in ext2
    // return false;
  }

  LE_32 (0x28, InodesPerGroup)

  if (InodesPerGroup < 1 || InodesPerGroup > NumInodes)
    return false;
  
  LE_32 (0x2C, MountTime)
  LE_32 (0x30, WriteTime)
  
  // LE_16 (0x34, NumMounts);
  // LE_16 (0x36, NumMountsMax);
  
  // LE_16 (0x3A, State);
  // LE_16 (0x3C, Errors);
  // LE_16 (0x3E, MinorRevLevel);
  
  LE_32 (0x40, LastCheckTime)
  // LE_32 (0x44, CheckInterval);
  LE_32 (0x48, CreatorOs)
  LE_32 (0x4C, RevLevel)
  
  // LE_16 (0x50, DefResUid);
  // LE_16 (0x52, DefResGid);
  
  FirstInode = k_INODE_GOOD_OLD_FIRST;
  InodeSize = EXT4_GOOD_OLD_INODE_SIZE;

  if (!IsOldRev())
  {
    LE_32 (0x54, FirstInode)
    LE_16 (0x58, InodeSize)
    if (FirstInode < k_INODE_GOOD_OLD_FIRST)
      return false;
    if (InodeSize > ((UInt32)1 << BlockBits)
        || InodeSize < EXT_NODE_SIZE_MIN
        || GetLog(InodeSize) < 0)
      return false;
  }

  LE_16 (0x5A, BlockGroupNr)
  LE_32 (0x5C, FeatureCompat)
  LE_32 (0x60, FeatureIncompat)
  LE_32 (0x64, FeatureRoCompat)
  
  memcpy(Uuid, p + 0x68, sizeof(Uuid));
  memcpy(VolName, p + 0x78, sizeof(VolName));
  memcpy(LastMount, p + 0x88, sizeof(LastMount));
  
  // LE_32 (0xC8, BitmapAlgo);
  
  LE_32 (0xE0, JournalInode)
  
  LE_16 (0xFE, GdSize)
  
  LE_32 (0x108, CTime)

  if (Is64Bit())
  {
    HI_32(0x150, NumBlocks)
    // HI_32(0x154, NumBlocksSuper);
    HI_32(0x158, NumFreeBlocks)
  }

  if (NumBlocks >= (UInt64)1 << (63 - BlockBits))
    return false;


  LE_16(0x15C, MinExtraISize)
  // LE_16(0x15E, WantExtraISize);
  // LE_32(0x160, Flags);
  // LE_16(0x164, RaidStride);
  // LE_16(0x166, MmpInterval);
  // LE_64(0x168, MmpBlock);
  
  // LogGroupsPerFlex = p[0x174];
  // ChecksumType = p[0x175];

  LE_64 (0x178, WrittenKB)

  // LE_32(0x194, ErrorCount);
  // LE_32(0x198, ErrorTime);
  // LE_32(0x19C, ErrorINode);
  // LE_32(0x1A0, ErrorBlock);
  
  if (NumBlocks < 1)
    return false;
  if (NumFreeBlocks > NumBlocks)
    return false;

  if (GetNumGroups() != GetNumGroups2())
    return false;

  return true;
}


struct CGroupDescriptor
{
  UInt64 BlockBitmap;
  UInt64 InodeBitmap;
  UInt64 InodeTable;
  UInt32 NumFreeBlocks;
  UInt32 NumFreeInodes;
  UInt32 DirCount;
  
  UInt16 Flags;
  
  UInt64 ExcludeBitmap;
  UInt32 BlockBitmap_Checksum;
  UInt32 InodeBitmap_Checksum;
  UInt32 UnusedCount;
  UInt16 Checksum;

  void Parse(const Byte *p, unsigned size);
};

void CGroupDescriptor::Parse(const Byte *p, unsigned size)
{
  LE_32 (0x00, BlockBitmap)
  LE_32 (0x04, InodeBitmap)
  LE_32 (0x08, InodeTable)
  LE_16 (0x0C, NumFreeBlocks)
  LE_16 (0x0E, NumFreeInodes)
  LE_16 (0x10, DirCount)
  LE_16 (0x12, Flags)
  LE_32 (0x14, ExcludeBitmap)
  LE_16 (0x18, BlockBitmap_Checksum)
  LE_16 (0x1A, InodeBitmap_Checksum)
  LE_16 (0x1C, UnusedCount)
  LE_16 (0x1E, Checksum)
  
  if (size >= 64)
  {
    p += 0x20;
    HI_32 (0x00, BlockBitmap)
    HI_32 (0x04, InodeBitmap)
    HI_32 (0x08, InodeTable)
    HI_16 (0x0C, NumFreeBlocks)
    HI_16 (0x0E, NumFreeInodes)
    HI_16 (0x10, DirCount)
    HI_16 (0x12, UnusedCount) // instead of Flags
    HI_32 (0x14, ExcludeBitmap)
    HI_16 (0x18, BlockBitmap_Checksum)
    HI_16 (0x1A, InodeBitmap_Checksum)
    // HI_16 (0x1C, Reserved);
  }
}


static const unsigned kNodeBlockFieldSize = 60;

struct CExtentTreeHeader
{
  UInt16 NumEntries;
  UInt16 MaxEntries;
  UInt16 Depth;
  // UInt32 Generation;

  bool Parse(const Byte *p)
  {
    LE_16 (0x02, NumEntries)
    LE_16 (0x04, MaxEntries)
    LE_16 (0x06, Depth)
    // LE_32 (0x08, Generation);
    return Get16(p) == 0xF30A; // magic
  }
};

struct CExtentIndexNode
{
  UInt32 VirtBlock;
  UInt64 PhyLeaf;

  void Parse(const Byte *p)
  {
    LE_32 (0x00, VirtBlock)
    LE_32 (0x04, PhyLeaf)
    PhyLeaf |= (((UInt64)Get16(p + 8)) << 32);
    // unused 16-bit field (at offset 0x0A) can be not zero in some cases. Why?
  }
};

struct CExtent
{
  UInt32 VirtBlock;
  UInt16 Len;
  bool IsInited;
  UInt64 PhyStart;

  UInt32 GetVirtEnd() const { return VirtBlock + Len; }
  bool IsLenOK() const { return VirtBlock + Len >= VirtBlock; }

  void Parse(const Byte *p)
  {
    LE_32 (0x00, VirtBlock)
    LE_16 (0x04, Len)
    IsInited = true;
    if (Len > (UInt32)0x8000)
    {
      IsInited = false;
      Len = (UInt16)(Len - (UInt32)0x8000);
    }
    LE_32 (0x08, PhyStart)
    UInt16 hi;
    LE_16 (0x06, hi)
    PhyStart |= ((UInt64)hi << 32);
  }
};



struct CExtTime
{
  UInt32 Val;
  UInt32 Extra;
};

struct CNode
{
  Int32 ParentNode;   // in _refs[], -1 if not dir
  int ItemIndex;      // in _items[]   , (set as >=0 only for if (IsDir())
  int SymLinkIndex;   // in _symLinks[]
  int DirIndex;       // in _dirs[]

  UInt16 Mode;
  UInt32 Uid; // fixed 21.02
  UInt32 Gid; // fixed 21.02
  // UInt16 Checksum;
  
  UInt64 FileSize;
  CExtTime MTime;
  CExtTime ATime;
  CExtTime CTime;
  CExtTime ChangeTime;
  // CExtTime DTime;

  UInt64 NumBlocks;
  UInt32 NumLinks;
  UInt32 Flags;

  UInt32 NumLinksCalced;

  Byte Block[kNodeBlockFieldSize];
  
  CNode():
      ParentNode(-1),
      ItemIndex(-1),
      SymLinkIndex(-1),
      DirIndex(-1),
      NumLinksCalced(0)
        {}

  bool IsFlags_HUGE()    const { return (Flags & k_NodeFlags_HUGE) != 0; }
  bool IsFlags_EXTENTS() const { return (Flags & k_NodeFlags_EXTENTS) != 0; }

  bool IsDir()     const { return MY_LIN_S_ISDIR(Mode); }
  bool IsRegular() const { return MY_LIN_S_ISREG(Mode); }
  bool IsLink()    const { return MY_LIN_S_ISLNK(Mode); }

  bool Parse(const Byte *p, const CHeader &_h);
};


bool CNode::Parse(const Byte *p, const CHeader &_h)
{
  MTime.Extra = 0;
  ATime.Extra = 0;
  CTime.Extra = 0;
  CTime.Val = 0;
  ChangeTime.Extra = 0;
  // DTime.Extra = 0;

  LE_16 (0x00, Mode)
  LE_16 (0x02, Uid)
  LE_32 (0x04, FileSize)
  LE_32 (0x08, ATime.Val)
  LE_32 (0x0C, ChangeTime.Val)
  LE_32 (0x10, MTime.Val)
  // LE_32 (0x14, DTime.Val);
  LE_16 (0x18, Gid)
  LE_16 (0x1A, NumLinks)

  LE_32 (0x1C, NumBlocks)
  LE_32 (0x20, Flags)
  // LE_32 (0x24, Union osd1);
  
  memcpy(Block, p + 0x28, kNodeBlockFieldSize);
  
  // LE_32 (0x64, Generation);  // File version (for NFS)
  // LE_32 (0x68, ACL);
  
  {
    UInt32 highSize;
    LE_32 (0x6C, highSize) // In ext2/3 this field was named i_dir_acl
   
    if (IsRegular()) // do we need that check ?
      FileSize |= ((UInt64)highSize << 32);
  }

  // UInt32 fragmentAddress;
  // LE_32 (0x70, fragmentAddress);
  
  // osd2
  {
    // Linux;
    // ext2:
    // Byte FragmentNumber = p[0x74];
    // Byte FragmentSize = p[0x74 + 1];
    
    // ext4:
    UInt32 numBlocksHigh;
    LE_16 (0x74, numBlocksHigh)
    NumBlocks |= (UInt64)numBlocksHigh << 32;
    
    HI_16 (0x74 + 4, Uid)
    HI_16 (0x74 + 6, Gid)
    /*
    UInt32 checksum;
    LE_16 (0x74 + 8, checksum);
    checksum = checksum;
    */
  }

  // 0x74: Byte Union osd2[12];

  if (_h.InodeSize > 128)
  {
    // InodeSize is power of 2, so the following check is not required:
    // if (_h.InodeSize < 128 + 2) return false;
    UInt16 extra_isize;
    LE_16 (0x80, extra_isize)
    if (128 + extra_isize > _h.InodeSize)
      return false;
    if (extra_isize >= 0x1C)
    {
      // UInt16 checksumUpper;
      // LE_16 (0x82, checksumUpper);
      LE_32 (0x84, ChangeTime.Extra)
      LE_32 (0x88, MTime.Extra)
      LE_32 (0x8C, ATime.Extra)
      LE_32 (0x90, CTime.Val)
      LE_32 (0x94, CTime.Extra)
      // LE_32 (0x98, VersionHi)
    }
  }

  PRF(printf("size = %5d", (unsigned)FileSize));

  return true;
}


struct CItem
{
  unsigned Node;        // in _refs[]
  int ParentNode;       // in _refs[]
  int SymLinkItemIndex; // in _items[], if the Node contains SymLink to existing dir
  Byte Type;
  
  AString Name;

  CItem():
      Node(0),
      ParentNode(-1),
      SymLinkItemIndex(-1),
      Type(k_Type_UNKNOWN)
        {}
  
  void Clear()
  {
    Node = 0;
    ParentNode = -1;
    SymLinkItemIndex = -1;
    Type = k_Type_UNKNOWN;
    Name.Empty();
  }

  bool IsDir() const { return Type == k_Type_DIR; }
  // bool IsNotDir() const { return Type != k_Type_DIR && Type != k_Type_UNKNOWN; }
};



static const unsigned kNumTreeLevelsMax = 6; // must be >= 3


Z7_CLASS_IMP_CHandler_IInArchive_2(
  IArchiveGetRawProps,
  IInArchiveGetStream
)
  CObjectVector<CItem> _items;
  CIntVector _refs;                 // iNode -> (index in _nodes). if (_refs[iNode] < 0), that node is not filled
  CRecordVector<CNode> _nodes;
  CObjectVector<CUIntVector> _dirs; // each CUIntVector contains indexes in _items[] only for dir items;
  AStringVector _symLinks;
  AStringVector _auxItems;
  int _auxSysIndex;
  int _auxUnknownIndex;

  CMyComPtr<IInStream> _stream;
  UInt64 _phySize;
  bool _isArc;
  bool _headersError;
  bool _headersWarning;
  bool _linksError;
  
  bool _isUTF;
  
  CHeader _h;

  IArchiveOpenCallback *_openCallback;
  UInt64 _totalRead;
  UInt64 _totalReadPrev;

  CByteBuffer _tempBufs[kNumTreeLevelsMax];

  
  HRESULT CheckProgress2()
  {
    const UInt64 numFiles = _items.Size();
    return _openCallback->SetCompleted(&numFiles, &_totalRead);
  }

  HRESULT CheckProgress()
  {
    HRESULT res = S_OK;
    if (_openCallback)
    {
      if (_totalRead - _totalReadPrev >= ((UInt32)1 << 20))
      {
        _totalReadPrev = _totalRead;
        res = CheckProgress2();
      }
    }
    return res;
  }

  
  int GetParentAux(const CItem &item) const
  {
    if (item.Node < _h.FirstInode && _auxSysIndex >= 0)
      return _auxSysIndex;
    return _auxUnknownIndex;
  }

  HRESULT SeekAndRead(IInStream *inStream, UInt64 block, Byte *data, size_t size);
  HRESULT ParseDir(const Byte *data, size_t size, unsigned iNodeDir);
  int FindTargetItem_for_SymLink(unsigned dirNode, const AString &path) const;

  HRESULT FillFileBlocks2(UInt32 block, unsigned level, unsigned numBlocks, CRecordVector<UInt32> &blocks);
  HRESULT FillFileBlocks(const Byte *p, unsigned numBlocks, CRecordVector<UInt32> &blocks);
  HRESULT FillExtents(const Byte *p, size_t size, CRecordVector<CExtent> &extents, int parentDepth);

  HRESULT GetStream_Node(unsigned nodeIndex, ISequentialInStream **stream);
  HRESULT ExtractNode(unsigned nodeIndex, CByteBuffer &data);

  void ClearRefs();
  HRESULT Open2(IInStream *inStream);

  void GetPath(unsigned index, AString &s) const;
  bool GetPackSize(unsigned index, UInt64 &res) const;
};



HRESULT CHandler::ParseDir(const Byte *p, size_t size, unsigned iNodeDir)
{
  bool isThereSelfLink = false;

  PRF(printf("\n\n========= node = %5d    size = %5d", (unsigned)iNodeDir, (unsigned)size));

  CNode &nodeDir = _nodes[_refs[iNodeDir]];
  nodeDir.DirIndex = (int)_dirs.Size();
  CUIntVector &dir = _dirs.AddNew();
  int parentNode = -1;

  CItem item;

  for (;;)
  {
    if (size == 0)
      break;
    if (size < 8)
      return S_FALSE;
    UInt32 iNode;
    LE_32 (0x00, iNode)
    unsigned recLen;
    LE_16 (0x04, recLen)
    const unsigned nameLen = p[6];
    const Byte type = p[7];

    if (recLen > size)
      return S_FALSE;
    if (nameLen + 8 > recLen)
      return S_FALSE;

    if (iNode >= _refs.Size())
      return S_FALSE;

    item.Clear();

    if (_h.IsThereFileType())
      item.Type = type;
    else if (type != 0)
      return S_FALSE;

    item.ParentNode = (int)iNodeDir;
    item.Node = iNode;
    item.Name.SetFrom_CalcLen((const char *)(p + 8), nameLen);
  
    p += recLen;
    size -= recLen;
   
    if (item.Name.Len() != nameLen)
      return S_FALSE;
    
    if (_isUTF)
    {
      // 21.07 : we force UTF8
      // _isUTF = CheckUTF8_AString(item.Name);
    }

    if (iNode == 0)
    {
      /*
      ext3 deleted??
      if (item.Name.Len() != 0)
        return S_FALSE;
      */

      PRF(printf("\n EMPTY %6d %d %s", (unsigned)recLen, (unsigned)type, (const char *)item.Name));
      if (type == 0xDE)
      {
        // checksum
      }
      continue;
    }

    const int nodeIndex = _refs[iNode];
    if (nodeIndex < 0)
      return S_FALSE;
    CNode &node = _nodes[nodeIndex];

    if (_h.IsThereFileType() && type != 0)
    {
      if (type >= Z7_ARRAY_SIZE(k_TypeToMode))
        return S_FALSE;
      if (k_TypeToMode[type] != (node.Mode & MY_LIN_S_IFMT))
        return S_FALSE;
    }

    node.NumLinksCalced++;
    
    PRF(printf("\n%s %6d %s", item.IsDir() ? "DIR  " : "     ", (unsigned)item.Node, (const char *)item.Name));
    
    if (item.Name[0] == '.')
    {
      if (item.Name[1] == 0)
      {
        if (isThereSelfLink)
          return S_FALSE;
        isThereSelfLink = true;
        if (iNode != iNodeDir)
          return S_FALSE;
        continue;
      }
      
      if (item.Name[1] == '.' && item.Name[2] == 0)
      {
        if (parentNode >= 0)
          return S_FALSE;
        if (!node.IsDir())
          return S_FALSE;
        if (iNode == iNodeDir && iNode != k_INODE_ROOT)
          return S_FALSE;
        
        parentNode = (int)iNode;

        if (nodeDir.ParentNode < 0)
          nodeDir.ParentNode = (int)iNode;
        else if ((unsigned)nodeDir.ParentNode != iNode)
          return S_FALSE;

        continue;
      }
    }

    if (iNode == iNodeDir)
      return S_FALSE;

    if (parentNode < 0)
      return S_FALSE;

    if (node.IsDir())
    {
      if (node.ParentNode < 0)
        node.ParentNode = (int)iNodeDir;
      else if ((unsigned)node.ParentNode != iNodeDir)
        return S_FALSE;
      const unsigned itemIndex = _items.Size();
      dir.Add(itemIndex);
      node.ItemIndex = (int)itemIndex;
    }

    _items.Add(item);
  }

  if (parentNode < 0 || !isThereSelfLink)
    return S_FALSE;

  return S_OK;
}


static int CompareItemsNames(const unsigned *p1, const unsigned *p2, void *param)
{
  const CObjectVector<CItem> &items = *(const CObjectVector<CItem> *)param;
  return strcmp(items[*p1].Name, items[*p2].Name);
}


int CHandler::FindTargetItem_for_SymLink(unsigned iNode, const AString &path) const
{
  unsigned pos = 0;
  
  if (path.IsEmpty())
    return -1;
  
  if (path[0] == '/')
  {
    iNode = k_INODE_ROOT;
    if (iNode >= _refs.Size())
      return -1;
    pos = 1;
  }

  AString s;

  while (pos != path.Len())
  {
    const CNode &node = _nodes[_refs[iNode]];
    const int slash = path.Find('/', pos);
    
    if (slash < 0)
    {
      s = path.Ptr(pos);
      pos = path.Len();
    }
    else
    {
      s.SetFrom(path.Ptr(pos), (unsigned)slash - pos);
      pos = (unsigned)slash + 1;
    }
    
    if (s[0] == '.')
    {
      if (s[1] == 0)
        continue;
      else if (s[1] == '.' && s[2] == 0)
      {
        if (node.ParentNode < 0)
          return -1;
        if (iNode == k_INODE_ROOT)
          return -1;
        iNode = (unsigned)node.ParentNode;
        continue;
      }
    }
    
    if (node.DirIndex < 0)
      return -1;

    const CUIntVector &dir = _dirs[node.DirIndex];
    
    /*
    for (unsigned i = 0;; i++)
    {
      if (i >= dir.Size())
        return -1;
      const CItem &item = _items[dir[i]];
      if (item.Name == s)
      {
        iNode = item.Node;
        break;
      }
    }
    */

    unsigned left = 0, right = dir.Size();
    for (;;)
    {
      if (left == right)
        return -1;
      const unsigned mid = (unsigned)(((size_t)left + (size_t)right) / 2);
      const CItem &item = _items[dir[mid]];
      const int comp = strcmp(s, item.Name);
      if (comp == 0)
      {
        iNode = item.Node;
        break;
      }
      if (comp < 0)
        right = mid;
      else
        left = mid + 1;
    }
  }

  return _nodes[_refs[iNode]].ItemIndex;
}


HRESULT CHandler::SeekAndRead(IInStream *inStream, UInt64 block, Byte *data, size_t size)
{
  if (block == 0 || block >= _h.NumBlocks)
    return S_FALSE;
  if (((size + ((size_t)1 << _h.BlockBits) - 1) >> _h.BlockBits) > _h.NumBlocks - block)
    return S_FALSE;
  RINOK(InStream_SeekSet(inStream, (UInt64)block << _h.BlockBits))
  _totalRead += size;
  return ReadStream_FALSE(inStream, data, size);
}


static const unsigned kHeaderSize = 2 * 1024;
static const unsigned kHeaderDataOffset = 1024;

HRESULT CHandler::Open2(IInStream *inStream)
{
  {
    Byte buf[kHeaderSize];
    RINOK(ReadStream_FALSE(inStream, buf, kHeaderSize))
    if (!_h.Parse(buf + kHeaderDataOffset))
      return S_FALSE;
    if (_h.BlockGroupNr != 0)
      return S_FALSE; // it's just copy of super block
  }
  
  {
    // ---------- Read groups and nodes ----------
    
    unsigned numGroups;
    {
      const UInt64 numGroups64 = _h.GetNumGroups();
      if (numGroups64 > (UInt32)1 << 31)
        return S_FALSE;
      numGroups = (unsigned)numGroups64;
    }

    unsigned gdBits = 5;
    if (_h.Is64Bit())
    {
      if (_h.GdSize != 64)
        return S_FALSE;
      gdBits = 6;
    }

    _isArc = true;
    _phySize = _h.GetPhySize();

    if (_openCallback)
    {
      RINOK(_openCallback->SetTotal(NULL, &_phySize))
    }

    UInt64 fileSize = 0;
    RINOK(InStream_GetSize_SeekToEnd(inStream, fileSize))

    CRecordVector<CGroupDescriptor> groups;

    {
      // ---------- Read groups ----------

      CByteBuffer gdBuf;
      const size_t gdBufSize = (size_t)numGroups << gdBits;
      if ((gdBufSize >> gdBits) != numGroups)
        return S_FALSE;
      gdBuf.Alloc(gdBufSize);
      RINOK(SeekAndRead(inStream, (_h.BlockBits <= 10 ? 2 : 1), gdBuf, gdBufSize))

      for (unsigned i = 0; i < numGroups; i++)
      {
        CGroupDescriptor gd;
        
        const Byte *p = gdBuf + ((size_t)i << gdBits);
        const unsigned gd_Size = (unsigned)1 << gdBits;
        gd.Parse(p, gd_Size);
        
        if (_h.UseMetadataChecksum())
        {
          // use CRC32c
        }
        else if (_h.UseGdtChecksum())
        {
          UInt32 crc = Crc16Calc(_h.Uuid, sizeof(_h.Uuid));
          Byte i_le[4];
          SetUi32(i_le, i)
          crc = Crc16Update(crc, i_le, 4);
          crc = Crc16Update(crc, p, 32 - 2);
          if (gd_Size != 32)
            crc = Crc16Update(crc, p + 32, gd_Size - 32);
          if (crc != gd.Checksum)
            return S_FALSE;
        }
        
        groups.Add(gd);
      }
    }

    {
      // ---------- Read nodes ----------

      if (_h.NumInodes < _h.NumFreeInodes)
        return S_FALSE;

      UInt32 numNodes = _h.InodesPerGroup;
      if (numNodes > _h.NumInodes)
        numNodes = _h.NumInodes;
      const size_t nodesDataSize = (size_t)numNodes * _h.InodeSize;
      
      if (nodesDataSize / _h.InodeSize != numNodes)
        return S_FALSE;

      // that code to reduce false detecting cases
      if (nodesDataSize > fileSize)
      {
        if (numNodes > (1 << 24))
          return S_FALSE;
      }
      
      const UInt32 numReserveInodes = _h.NumInodes - _h.NumFreeInodes + 1;
      // numReserveInodes = _h.NumInodes + 1;
      if (numReserveInodes != 0)
      {
        _nodes.Reserve(numReserveInodes);
        _refs.Reserve(numReserveInodes);
      }
      
      CByteBuffer nodesData;
      nodesData.Alloc(nodesDataSize);
      
      CByteBuffer nodesMap;
      const size_t blockSize = (size_t)1 << _h.BlockBits;
      nodesMap.Alloc(blockSize);
      
      unsigned globalNodeIndex = 0;
      // unsigned numEmpty = 0;
      unsigned numEmpty_in_Maps = 0;

      FOR_VECTOR (gi, groups)
      {
        if (globalNodeIndex >= _h.NumInodes)
          break;

        const CGroupDescriptor &gd = groups[gi];
        
        PRF(printf("\n\ng%6d block = %6x\n", gi, (unsigned)gd.InodeTable));
        
        RINOK(SeekAndRead(inStream, gd.InodeBitmap, nodesMap, blockSize))
        RINOK(SeekAndRead(inStream, gd.InodeTable, nodesData, nodesDataSize))

        unsigned numEmpty_in_Map = 0;
        
        for (size_t n = 0; n < numNodes && globalNodeIndex < _h.NumInodes; n++, globalNodeIndex++)
        {
          if ((nodesMap[n >> 3] & ((unsigned)1 << (n & 7))) == 0)
          {
            numEmpty_in_Map++;
            continue;
          }

          const Byte *p = nodesData + (size_t)n * _h.InodeSize;
          if (IsEmptyData(p, _h.InodeSize))
          {
            if (globalNodeIndex + 1 >= _h.FirstInode)
            {
              _headersError = true;
              // return S_FALSE;
            }
            continue;
          }

          CNode node;
              
          PRF(printf("\nnode = %5d ", (unsigned)n));

          if (!node.Parse(p, _h))
            return S_FALSE;
  
          // PRF(printf("\n %6d", (unsigned)n));
          /*
            SetUi32(p + 0x7C, 0)
            SetUi32(p + 0x82, 0)
            
            UInt32 crc = Crc32C_Calc(_h.Uuid, sizeof(_h.Uuid));
            Byte i_le[4];
            SetUi32(i_le, n);
            crc = Crc32C_Update(crc, i_le, 4);
            crc = Crc32C_Update(crc, p, _h.InodeSize);
            if (crc != node.Checksum) return S_FALSE;
          */

          while (_refs.Size() < globalNodeIndex + 1)
          {
            // numEmpty++;
            _refs.Add(-1);
          }
          
          _refs.Add((int)_nodes.Add(node));
        }

        
        numEmpty_in_Maps += numEmpty_in_Map;
        
        if (numEmpty_in_Map != gd.NumFreeInodes)
        {
          _headersWarning = true;
          // return S_FALSE;
        }
      }
      
      if (numEmpty_in_Maps != _h.NumFreeInodes)
      {
        // some ext2 examples has incorrect value in _h.NumFreeInodes.
        // so we disable check;
        _headersWarning = true;
      }

      if (_refs.Size() <= k_INODE_ROOT)
        return S_FALSE;

      // printf("\n numReserveInodes = %6d, _refs.Size() = %d, numEmpty = %7d\n", numReserveInodes, _refs.Size(), (unsigned)numEmpty);
    }
  }

  _stream = inStream; // we need stream for dir nodes

  {
    // ---------- Read Dirs ----------

    CByteBuffer dataBuf;

    FOR_VECTOR (i, _refs)
    {
      const int nodeIndex = _refs[i];
      {
        if (nodeIndex < 0)
          continue;
        const CNode &node = _nodes[nodeIndex];
        if (!node.IsDir())
          continue;
      }
      RINOK(ExtractNode((unsigned)nodeIndex, dataBuf))
      if (dataBuf.Size() == 0)
      {
        // _headersError = true;
        return S_FALSE;
      }
      else
      {
        RINOK(ParseDir(dataBuf, dataBuf.Size(), i))
      }
      RINOK(CheckProgress())
    }

    const int ref = _refs[k_INODE_ROOT];
    if (ref < 0 || _nodes[ref].ParentNode != k_INODE_ROOT)
      return S_FALSE;
  }

  {
    // ---------- Check NumLinks and unreferenced dir nodes ----------
  
    FOR_VECTOR (i, _refs)
    {
      int nodeIndex = _refs[i];
      if (nodeIndex < 0)
        continue;
      const CNode &node = _nodes[nodeIndex];

      if (node.NumLinks != node.NumLinksCalced)
      {
        if (node.NumLinks != 1 || node.NumLinksCalced != 0
            // ) && i >= _h.FirstInode
            )
        {
          _linksError = true;
          // return S_FALSE;
        }
      }

      if (!node.IsDir())
        continue;

      if (node.ParentNode < 0)
      {
        if (i >= _h.FirstInode)
          return S_FALSE;
        continue;
      }
    }
  }

  {
    // ---------- Check that there is no loops in parents list ----------

    unsigned numNodes = _refs.Size();
    CIntArr UsedByNode(numNodes);
    {
      {
        for (unsigned i = 0; i < numNodes; i++)
          UsedByNode[i] = -1;
      }
    }

    FOR_VECTOR (i, _refs)
    {
      {
        int nodeIndex = _refs[i];
        if (nodeIndex < 0)
          continue;
        const CNode &node = _nodes[nodeIndex];
        if (node.ParentNode < 0 // not dir
            || i == k_INODE_ROOT)
          continue;
      }

      unsigned c = i;

      for (;;)
      {
        const int nodeIndex = _refs[c];
        if (nodeIndex < 0)
          return S_FALSE;
        CNode &node = _nodes[nodeIndex];
        
        if (UsedByNode[c] != -1)
        {
          if ((unsigned)UsedByNode[c] == i)
            return S_FALSE;
          break;
        }
        
        UsedByNode[c] = (int)i;
        if (node.ParentNode < 0 || node.ParentNode == k_INODE_ROOT)
          break;
        if ((unsigned)node.ParentNode == i)
          return S_FALSE;
        c = (unsigned)node.ParentNode;
      }
    }
  }
  
  {
    // ---------- Fill SymLinks data ----------

    AString s;
    CByteBuffer data;

    unsigned i;
    for (i = 0; i < _refs.Size(); i++)
    {
      const int nodeIndex = _refs[i];
      if (nodeIndex < 0)
        continue;
      CNode &node = _nodes[nodeIndex];
      if (!node.IsLink())
        continue;
      if (node.FileSize > ((UInt32)1 << 14))
        continue;
      if (ExtractNode((unsigned)nodeIndex, data) == S_OK && data.Size() != 0)
      {
        s.SetFrom_CalcLen((const char *)(const Byte *)data, (unsigned)data.Size());
        if (s.Len() == data.Size())
          node.SymLinkIndex = (int)_symLinks.Add(s);
        RINOK(CheckProgress())
      }
    }

    for (i = 0; i < _dirs.Size(); i++)
    {
      _dirs[i].Sort(CompareItemsNames, (void *)&_items);
    }

    unsigned prev = 0;
    unsigned complex = 0;

    for (i = 0; i < _items.Size(); i++)
    {
      CItem &item = _items[i];
      const int sym = _nodes[_refs[item.Node]].SymLinkIndex;
      if (sym >= 0 && item.ParentNode >= 0)
      {
        item.SymLinkItemIndex = FindTargetItem_for_SymLink((unsigned)item.ParentNode, _symLinks[sym]);
        if (_openCallback)
        {
          complex++;
          if (complex - prev >= (1 << 10))
          {
            prev = complex;
            RINOK(CheckProgress2())
          }
        }
      }
    }
  }

  {
    // ---------- Add items and aux folders for unreferenced files ----------

    bool useSys = false;
    bool useUnknown = false;

    FOR_VECTOR (i, _refs)
    {
      const int nodeIndex = _refs[i];
      if (nodeIndex < 0)
        continue;
      const CNode &node = _nodes[nodeIndex];
      
      if (node.NumLinksCalced == 0 /* || i > 100 && i < 150 */) // for debug
      {
        CItem item;
        item.Node = i;

        // we don't know how to work with k_INODE_RESIZE node (strange FileSize and Block values).
        // so we ignore it;
        
        if (i == k_INODE_RESIZE)
          continue;
        
        if (node.FileSize == 0)
          continue;

        if (i < _h.FirstInode)
        {
          if (item.Node < Z7_ARRAY_SIZE(k_SysInode_Names))
            item.Name = k_SysInode_Names[item.Node];
          useSys = true;
        }
        else
          useUnknown = true;
        
        if (item.Name.IsEmpty())
          item.Name.Add_UInt32(item.Node);

        _items.Add(item);
      }
    }

    if (useSys)
      _auxSysIndex = (int)_auxItems.Add((AString)"[SYS]");
    if (useUnknown)
      _auxUnknownIndex = (int)_auxItems.Add((AString)"[UNKNOWN]");
  }

  return S_OK;
}


Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  {
    Close();
    HRESULT res;
    try
    {
      _openCallback = callback;
      res = Open2(stream);
    }
    catch(...)
    {
      ClearRefs();
      throw;
    }
    
    if (res != S_OK)
    {
      ClearRefs();
      return res;
    }
    _stream = stream;
  }
  return S_OK;
  COM_TRY_END
}


void CHandler::ClearRefs()
{
  _stream.Release();
  _items.Clear();
  _nodes.Clear();
  _refs.Clear();
  _auxItems.Clear();
  _symLinks.Clear();
  _dirs.Clear();
  _auxSysIndex = -1;
  _auxUnknownIndex = -1;
}


Z7_COM7F_IMF(CHandler::Close())
{
  _totalRead = 0;
  _totalReadPrev = 0;
  _phySize = 0;
  _isArc = false;
  _headersError = false;
  _headersWarning = false;
  _linksError = false;
  _isUTF = true;

  ClearRefs();
  return S_OK;
}


static void ChangeSeparatorsInName(char *s, unsigned num)
{
  for (unsigned i = 0; i < num; i++)
  {
    char c = s[i];
    if (c == CHAR_PATH_SEPARATOR || c == '/')
      s[i] = '_';
  }
}


void CHandler::GetPath(unsigned index, AString &s) const
{
  s.Empty();

  if (index >= _items.Size())
  {
    s = _auxItems[index - _items.Size()];
    return;
  }
  
  for (;;)
  {
    const CItem &item = _items[index];
    if (!s.IsEmpty())
      s.InsertAtFront(CHAR_PATH_SEPARATOR);
    s.Insert(0, item.Name);
    // 18.06
    ChangeSeparatorsInName(s.GetBuf(), item.Name.Len());

    if (item.ParentNode == k_INODE_ROOT)
      return;

    if (item.ParentNode < 0)
    {
      int aux = GetParentAux(item);
      if (aux < 0)
        break;
      s.InsertAtFront(CHAR_PATH_SEPARATOR);
      s.Insert(0, _auxItems[aux]);
      return;
    }

    const CNode &node = _nodes[_refs[item.ParentNode]];
    if (node.ItemIndex < 0)
      return;
    index = (unsigned)node.ItemIndex;

    if (s.Len() > ((UInt32)1 << 16))
    {
      s.Insert(0, "[LONG]" STRING_PATH_SEPARATOR);
      return;
    }
  }
}


bool CHandler::GetPackSize(unsigned index, UInt64 &totalPack) const
{
  if (index >= _items.Size())
  {
    totalPack = 0;
    return false;
  }

  const CItem &item = _items[index];
  const CNode &node = _nodes[_refs[item.Node]];

  // if (!node.IsFlags_EXTENTS())
  {
    totalPack = (UInt64)node.NumBlocks << (node.IsFlags_HUGE() ? _h.BlockBits : 9);
    return true;
  }

  /*
  CExtentTreeHeader eth;
  if (!eth.Parse(node.Block))
    return false;
  if (eth.NumEntries > 3)
    return false;
  if (!eth.Depth == 0)
    return false;

  UInt64 numBlocks = 0;
  {
    for (unsigned i = 0; i < eth.NumEntries; i++)
    {
      CExtent e;
      e.Parse(node.Block + 12 + i * 12);
      // const CExtent &e = node.leafs[i];
      if (e.IsInited)
        numBlocks += e.Len;
    }
  }

  totalPack = numBlocks << _h.BlockBits;
  return true;
  */
}


Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _items.Size() + _auxItems.Size();
  return S_OK;
}

enum
{
  kpidMountTime = kpidUserDefined,
  kpidLastCheckTime,
  kpidRevision,
  kpidINodeSize,
  kpidLastMount,
  kpidFeatureIncompat,
  kpidFeatureRoCompat,
  kpidWrittenKB

  // kpidGroupSize,

  // kpidChangeTime = kpidUserDefined + 256,
  // kpidDTime
};

static const UInt32 kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidPosixAttrib,
  kpidMTime,
  kpidCTime,
  kpidATime,
  // kpidChangeTime,
  // kpidDTime,
  kpidINode,
  kpidLinks,
  kpidSymLink,
  kpidCharacts,
  kpidUserId,
  kpidGroupId
};


static const CStatProp kArcProps[] =
{
  { NULL, kpidHeadersSize, VT_BSTR },
  // { NULL, kpidFileSystem, VT_BSTR },
  // kpidMethod,
  { NULL, kpidClusterSize, VT_UI4 },
  // { "Group Size", kpidGroupSize, VT_UI8 },
  { NULL, kpidFreeSpace, VT_UI8 },
  
  { NULL, kpidMTime, VT_FILETIME },
  { NULL, kpidCTime, VT_FILETIME },
  { "Mount Time", kpidMountTime, VT_FILETIME },
  { "Last Check Time", kpidLastCheckTime, VT_FILETIME },
  
  { NULL, kpidHostOS, VT_BSTR},
  { "Revision", kpidRevision, VT_UI4},
  { "inode Size", kpidINodeSize, VT_UI4},
  { NULL, kpidCodePage, VT_BSTR},
  { NULL, kpidVolumeName, VT_BSTR},
  { "Last Mounted", kpidLastMount, VT_BSTR},
  { NULL, kpidId, VT_BSTR},
  { NULL, kpidCharacts, VT_BSTR },
  { "Incompatible Features", kpidFeatureIncompat, VT_BSTR },
  { "Readonly-compatible Features", kpidFeatureRoCompat, VT_BSTR },
  { "Written KiB", kpidWrittenKB, VT_UI8 }
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME

static void StringToProp(bool isUTF, const char *s, unsigned size, NCOM::CPropVariant &prop)
{
  UString u;
  AString a;
  a.SetFrom_CalcLen(s, size);
  if (!isUTF || !ConvertUTF8ToUnicode(a, u))
    MultiByteToUnicodeString2(u, a);
  prop = u;
}

static void UnixTimeToProp(UInt32 val, NCOM::CPropVariant &prop)
{
  if (val != 0)
    PropVariant_SetFrom_UnixTime(prop, val);
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
 
  NCOM::CPropVariant prop;
  
  switch (propID)
  {
    /*
    case kpidFileSystem:
    {
      AString res = "Ext4";
      prop = res;
      break;
    }
    */
    
    case kpidIsTree: prop = true; break;
    case kpidIsAux: prop = true; break;
    case kpidINode: prop = true; break;

    case kpidClusterSize: prop = (UInt32)1 << _h.BlockBits; break;
    // case kpidGroupSize: prop = (UInt64)_h.BlocksPerGroup << _h.BlockBits; break;
    
    case kpidFreeSpace: prop = (UInt64)_h.NumFreeBlocks << _h.BlockBits; break;

    case kpidCTime: UnixTimeToProp(_h.CTime, prop); break;
    case kpidMTime: UnixTimeToProp(_h.WriteTime, prop); break;
    case kpidMountTime: UnixTimeToProp(_h.MountTime, prop); break;
    case kpidLastCheckTime: UnixTimeToProp(_h.LastCheckTime, prop); break;

    case kpidHostOS:
    {
      TYPE_TO_PROP(kHostOS, _h.CreatorOs, prop);
      break;
    }
    
    case kpidRevision: prop = _h.RevLevel; break;

    case kpidINodeSize: prop = (UInt32)_h.InodeSize; break;

    case kpidId:
    {
      if (!IsEmptyData(_h.Uuid, sizeof(_h.Uuid)))
      {
        char s[sizeof(_h.Uuid) * 2 + 2];
        ConvertDataToHex_Lower(s, _h.Uuid, sizeof(_h.Uuid));
        prop = s;
      }
      break;
    }

    case kpidCodePage: if (_isUTF) prop = "UTF-8"; break;
    
    case kpidShortComment:
    case kpidVolumeName:
        StringToProp(_isUTF, _h.VolName, sizeof(_h.VolName), prop); break;
    
    case kpidLastMount: StringToProp(_isUTF, _h.LastMount, sizeof(_h.LastMount), prop); break;

    case kpidCharacts: FLAGS_TO_PROP(g_FeatureCompat_Flags, _h.FeatureCompat, prop); break;
    case kpidFeatureIncompat: FLAGS_TO_PROP(g_FeatureIncompat_Flags, _h.FeatureIncompat, prop); break;
    case kpidFeatureRoCompat: FLAGS_TO_PROP(g_FeatureRoCompat_Flags, _h.FeatureRoCompat, prop); break;
    case kpidWrittenKB: if (_h.WrittenKB != 0) prop = _h.WrittenKB; break;

    case kpidPhySize: prop = _phySize; break;

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_linksError) v |= kpv_ErrorFlags_HeadersError;
      if (_headersError) v |= kpv_ErrorFlags_HeadersError;
      if (!_stream && v == 0 && _isArc)
        v = kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }

    case kpidWarningFlags:
    {
      UInt32 v = 0;
      if (_headersWarning) v |= kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }
  }
  
  prop.Detach(value);
  return S_OK;

  COM_TRY_END
}


/*
static const Byte kRawProps[] =
{
  // kpidSha1,
};
*/

Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *numProps))
{
  // *numProps = Z7_ARRAY_SIZE(kRawProps);
  *numProps = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32 /* index */, BSTR *name, PROPID *propID))
{
  // *propID = kRawProps[index];
  *propID = 0;
  *name = NULL;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *parentType))
{
  *parentType = NParentType::kDir;
  *parent = (UInt32)(Int32)-1;

  if (index >= _items.Size())
    return S_OK;

  const CItem &item = _items[index];
  
  if (item.ParentNode < 0)
  {
    const int aux = GetParentAux(item);
    if (aux >= 0)
      *parent = _items.Size() + (unsigned)aux;
  }
  else
  {
    const int itemIndex = _nodes[_refs[item.ParentNode]].ItemIndex;
    if (itemIndex >= 0)
      *parent = (unsigned)itemIndex;
  }
  
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;

  if (propID == kpidName && _isUTF)
  {
    if (index < _items.Size())
    {
      const AString &s = _items[index].Name;
      if (!s.IsEmpty())
      {
        *data = (void *)(const char *)s;
        *dataSize = (UInt32)s.Len() + 1;
        *propType = NPropDataType::kUtf8z;
      }
      return S_OK;
    }
    else
    {
      const AString &s = _auxItems[index - _items.Size()];
      {
        *data = (void *)(const char *)s;
        *dataSize = (UInt32)s.Len() + 1;
        *propType = NPropDataType::kUtf8z;
      }
      return S_OK;
    }
  }

  return S_OK;
}


static void ExtTimeToProp(const CExtTime &t, NCOM::CPropVariant &prop)
{
  if (t.Val == 0 && t.Extra == 0)
    return;

  FILETIME ft;
  unsigned low100ns = 0;
  // if (t.Extra != 0)
  {
    // 1901-2446 :
    Int64 v = (Int64)(Int32)t.Val;
    v += (UInt64)(t.Extra & 3) << 32;  // 2 low bits are offset for main timestamp
    UInt64 ft64 = NTime::UnixTime64_To_FileTime64(v);
    const UInt32 ns = (t.Extra >> 2);
    if (ns < 1000000000)
    {
      ft64 += ns / 100;
      low100ns = (unsigned)(ns % 100);
    }
    ft.dwLowDateTime = (DWORD)ft64;
    ft.dwHighDateTime = (DWORD)(ft64 >> 32);
  }
  /*
  else
  {
    // 1901-2038 : that code is good for ext4 and compatibility with Extra
    NTime::UnixTime64ToFileTime((Int32)t.Val, ft); // for

    // 1970-2106 : that code is good if timestamp is used as unsigned 32-bit
    // are there such systems?
    // NTime::UnixTimeToFileTime(t.Val, ft); // for
  }
  */
  prop.SetAsTimeFrom_FT_Prec_Ns100(ft, k_PropVar_TimePrec_1ns, low100ns);
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  if (index >= _items.Size())
  {
    switch (propID)
    {
      case kpidPath:
      case kpidName:
      {
        prop = _auxItems[index - _items.Size()];
        break;
      }
      case kpidIsDir: prop = true; break;
      case kpidIsAux: prop = true; break;
    }
  }
  else
  {

  const CItem &item = _items[index];
  const CNode &node = _nodes[_refs[item.Node]];
  bool isDir = node.IsDir();

  switch (propID)
  {
    case kpidPath:
    {
      UString u;
      {
        AString s;
        GetPath(index, s);
        if (!_isUTF || !ConvertUTF8ToUnicode(s, u))
          MultiByteToUnicodeString2(u, s);
      }
      prop = u;
      break;
    }
    
    case kpidName:
    {
      {
        UString u;
        {
          if (!_isUTF || !ConvertUTF8ToUnicode(item.Name, u))
            MultiByteToUnicodeString2(u, item.Name);
        }
        prop = u;
      }
      break;
    }
    
    case kpidIsDir:
    {
      bool isDir2 = isDir;
      if (item.SymLinkItemIndex >= 0)
        isDir2 = _nodes[_refs[_items[item.SymLinkItemIndex].Node]].IsDir();
      prop = isDir2;
      break;
    }

    case kpidSize: if (!isDir) prop = node.FileSize; break;
    
    case kpidPackSize:
      if (!isDir)
      {
        UInt64 size;
        if (GetPackSize(index, size))
          prop = size;
      }
      break;

    case kpidPosixAttrib:
    {
      /*
      if (node.Type != 0 && node.Type < Z7_ARRAY_SIZE(k_TypeToMode))
        prop = (UInt32)(node.Mode & 0xFFF) | k_TypeToMode[node.Type];
      */
      prop = (UInt32)(node.Mode);
      break;
    }

    case kpidMTime: ExtTimeToProp(node.MTime, prop); break;
    case kpidCTime: ExtTimeToProp(node.CTime, prop); break;
    case kpidATime: ExtTimeToProp(node.ATime, prop); break;
    // case kpidDTime: ExtTimeToProp(node.DTime, prop); break;
    case kpidChangeTime: ExtTimeToProp(node.ChangeTime, prop); break;
    case kpidUserId: prop = (UInt32)node.Uid; break;
    case kpidGroupId: prop = (UInt32)node.Gid; break;
    case kpidLinks: prop = node.NumLinks; break;
    case kpidINode: prop = (UInt32)item.Node; break;
    case kpidStreamId: if (!isDir) prop = (UInt32)item.Node; break;
    case kpidCharacts: FLAGS_TO_PROP(g_NodeFlags, (UInt32)node.Flags, prop); break;

    case kpidSymLink:
    {
      if (node.SymLinkIndex >= 0)
      {
        UString u;
        {
          const AString &s = _symLinks[node.SymLinkIndex];
          if (!_isUTF || !ConvertUTF8ToUnicode(s, u))
            MultiByteToUnicodeString2(u, s);
        }
        prop = u;
      }
      break;
    }
  }

  }

  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_CLASS_IMP_IInStream(CClusterInStream2
)
  UInt64 _virtPos;
  UInt64 _physPos;
  UInt32 _curRem;
public:
  unsigned BlockBits;
  UInt64 Size;
  CMyComPtr<IInStream> Stream;
  CRecordVector<UInt32> Vector;

  HRESULT SeekToPhys() { return InStream_SeekSet(Stream, _physPos); }

  HRESULT InitAndSeek()
  {
    _curRem = 0;
    _virtPos = 0;
    _physPos = 0;
    if (Vector.Size() > 0)
    {
      _physPos = (Vector[0] << BlockBits);
      return SeekToPhys();
    }
    return S_OK;
  }
};


Z7_COM7F_IMF(CClusterInStream2::Read(void *data, UInt32 size, UInt32 *processedSize))
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
    const UInt32 blockSize = (UInt32)1 << BlockBits;
    const UInt32 virtBlock = (UInt32)(_virtPos >> BlockBits);
    const UInt32 offsetInBlock = (UInt32)_virtPos & (blockSize - 1);
    const UInt32 phyBlock = Vector[virtBlock];
    
    if (phyBlock == 0)
    {
      UInt32 cur = blockSize - offsetInBlock;
      if (cur > size)
        cur = size;
      memset(data, 0, cur);
      _virtPos += cur;
      if (processedSize)
        *processedSize = cur;
      return S_OK;
    }
    
    UInt64 newPos = ((UInt64)phyBlock << BlockBits) + offsetInBlock;
    if (newPos != _physPos)
    {
      _physPos = newPos;
      RINOK(SeekToPhys())
    }
    
    _curRem = blockSize - offsetInBlock;
    
    for (unsigned i = 1; i < 64 && (virtBlock + i) < (UInt32)Vector.Size() && phyBlock + i == Vector[virtBlock + i]; i++)
      _curRem += (UInt32)1 << BlockBits;
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
 
Z7_COM7F_IMF(CClusterInStream2::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
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


Z7_CLASS_IMP_IInStream(
  CExtInStream
)
  UInt64 _virtPos;
  UInt64 _phyPos;
public:
  unsigned BlockBits;
  UInt64 Size;
  CMyComPtr<IInStream> Stream;
  CRecordVector<CExtent> Extents;

  HRESULT StartSeek()
  {
    _virtPos = 0;
    _phyPos = 0;
    return InStream_SeekSet(Stream, _phyPos);
  }
};

Z7_COM7F_IMF(CExtInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
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

  UInt32 blockIndex = (UInt32)(_virtPos >> BlockBits);
  
  unsigned left = 0, right = Extents.Size();
  for (;;)
  {
    unsigned mid = (left + right) / 2;
    if (mid == left)
      break;
    if (blockIndex < Extents[mid].VirtBlock)
      right = mid;
    else
      left = mid;
  }

  {
    const CExtent &extent = Extents[left];
    if (blockIndex < extent.VirtBlock)
      return E_FAIL;
    UInt32 bo = blockIndex - extent.VirtBlock;
    if (bo >= extent.Len)
      return E_FAIL;

    UInt32 offset = ((UInt32)_virtPos & (((UInt32)1 << BlockBits) - 1));
    UInt32 remBlocks = extent.Len - bo;
    UInt64 remBytes = ((UInt64)remBlocks << BlockBits);
    remBytes -= offset;

    if (size > remBytes)
      size = (UInt32)remBytes;

    if (!extent.IsInited)
    {
      memset(data, 0, size);
      _virtPos += size;
      if (processedSize)
        *processedSize = size;
      return S_OK;
    }

    const UInt64 phyBlock = extent.PhyStart + bo;
    const UInt64 phy = (phyBlock << BlockBits) + offset;
    
    if (phy != _phyPos)
    {
      RINOK(InStream_SeekSet(Stream, phy))
      _phyPos = phy;
    }
    
    UInt32 realProcessSize = 0;
    
    const HRESULT res = Stream->Read(data, size, &realProcessSize);
    
    _phyPos += realProcessSize;
    _virtPos += realProcessSize;
    if (processedSize)
      *processedSize = realProcessSize;
    return res;
  }
}


Z7_COM7F_IMF(CExtInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
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



HRESULT CHandler::FillFileBlocks2(UInt32 block, unsigned level, unsigned numBlocks, CRecordVector<UInt32> &blocks)
{
  const size_t blockSize = (size_t)1 << _h.BlockBits;
  CByteBuffer &tempBuf = _tempBufs[level];
  tempBuf.Alloc(blockSize);

  PRF2(printf("\n level = %d, block = %7d", level, (unsigned)block));

  RINOK(SeekAndRead(_stream, block, tempBuf, blockSize))

  const Byte *p = tempBuf;
  size_t num = (size_t)1 << (_h.BlockBits - 2);

  for (size_t i = 0; i < num; i++)
  {
    if (blocks.Size() == numBlocks)
      break;
    UInt32 val = GetUi32(p + 4 * i);
    if (val >= _h.NumBlocks)
      return S_FALSE;

    if (level != 0)
    {
      if (val == 0)
      {
        /*
        size_t num = (size_t)1 << ((_h.BlockBits - 2) * (level));
        PRF2(printf("\n num empty = %3d", (unsigned)num));
        for (size_t k = 0; k < num; k++)
        {
          blocks.Add(0);
          if (blocks.Size() == numBlocks)
            return S_OK;
        }
        continue;
        */
        return S_FALSE;
      }
      
      RINOK(FillFileBlocks2(val, level - 1, numBlocks, blocks))
      continue;
    }
    
    PRF2(printf("\n i = %3d,  blocks.Size() = %6d, block = %5d ", i, blocks.Size(), (unsigned)val));

    PRF(printf("\n i = %3d,  start = %5d ", (unsigned)i, (unsigned)val));
    
    blocks.Add(val);
  }

  return S_OK;
}


static const unsigned kNumDirectNodeBlocks = 12;

HRESULT CHandler::FillFileBlocks(const Byte *p, unsigned numBlocks, CRecordVector<UInt32> &blocks)
{
  // ext2 supports zero blocks (blockIndex == 0).

  blocks.ClearAndReserve(numBlocks);

  for (unsigned i = 0; i < kNumDirectNodeBlocks; i++)
  {
    if (i == numBlocks)
      return S_OK;
    UInt32 val = GetUi32(p + 4 * i);
    if (val >= _h.NumBlocks)
      return S_FALSE;
    blocks.Add(val);
  }
  
  for (unsigned level = 0; level < 3; level++)
  {
    if (blocks.Size() == numBlocks)
      break;
    UInt32 val = GetUi32(p + 4 * (kNumDirectNodeBlocks + level));
    if (val >= _h.NumBlocks)
      return S_FALSE;

    if (val == 0)
    {
      /*
      size_t num = (size_t)1 << ((_h.BlockBits - 2) * (level + 1));
      for (size_t k = 0; k < num; k++)
      {
        blocks.Add(0);
        if (blocks.Size() == numBlocks)
          return S_OK;
      }
      continue;
      */
      return S_FALSE;
    }

    RINOK(FillFileBlocks2(val, level, numBlocks, blocks))
  }
  
  return S_OK;
}


static void AddSkipExtents(CRecordVector<CExtent> &extents, UInt32 virtBlock, UInt32 numBlocks)
{
  while (numBlocks != 0)
  {
    UInt32 len = numBlocks;
    const UInt32 kLenMax = (UInt32)1 << 15;
    if (len > kLenMax)
      len = kLenMax;
    CExtent e;
    e.VirtBlock = virtBlock;
    e.Len = (UInt16)len;
    e.IsInited = false;
    e.PhyStart = 0;
    extents.Add(e);
    virtBlock += len;
    numBlocks -= len;
  }
}

static bool UpdateExtents(CRecordVector<CExtent> &extents, UInt32 block)
{
  if (extents.IsEmpty())
  {
    if (block == 0)
      return true;
    AddSkipExtents(extents, 0, block);
    return true;
  }

  const CExtent &prev = extents.Back();
  if (block < prev.VirtBlock)
    return false;
  UInt32 prevEnd = prev.GetVirtEnd();
  if (block == prevEnd)
    return true;
  AddSkipExtents(extents, prevEnd, block - prevEnd);
  return true;
}


HRESULT CHandler::FillExtents(const Byte *p, size_t size, CRecordVector<CExtent> &extents, int parentDepth)
{
  CExtentTreeHeader eth;
  if (!eth.Parse(p))
    return S_FALSE;

  if (parentDepth >= 0 && eth.Depth != parentDepth - 1) // (eth.Depth >= parentDepth)
    return S_FALSE;

  if (12 + 12 * (size_t)eth.NumEntries > size)
    return S_FALSE;

  if (eth.Depth >= kNumTreeLevelsMax)
    return S_FALSE;

  if (eth.Depth == 0)
  {
    for (unsigned i = 0; i < eth.NumEntries; i++)
    {
      CExtent e;
      e.Parse(p + 12 + i * 12);
      if (e.PhyStart == 0
          || e.PhyStart > _h.NumBlocks
          || e.PhyStart + e.Len > _h.NumBlocks
          || !e.IsLenOK())
        return S_FALSE;
      if (!UpdateExtents(extents, e.VirtBlock))
        return S_FALSE;
      extents.Add(e);
    }
    
    return S_OK;
  }

  const size_t blockSize = (size_t)1 << _h.BlockBits;
  CByteBuffer &tempBuf = _tempBufs[eth.Depth];
  tempBuf.Alloc(blockSize);

  for (unsigned i = 0; i < eth.NumEntries; i++)
  {
    CExtentIndexNode e;
    e.Parse(p + 12 + i * 12);

    if (e.PhyLeaf == 0 || e.PhyLeaf >= _h.NumBlocks)
      return S_FALSE;

    if (!UpdateExtents(extents, e.VirtBlock))
      return S_FALSE;

    RINOK(SeekAndRead(_stream, e.PhyLeaf, tempBuf, blockSize))
    RINOK(FillExtents(tempBuf, blockSize, extents, eth.Depth))
  }

  return S_OK;
}


HRESULT CHandler::GetStream_Node(unsigned nodeIndex, ISequentialInStream **stream)
{
  COM_TRY_BEGIN

  *stream = NULL;

  const CNode &node = _nodes[nodeIndex];

  if (!node.IsFlags_EXTENTS())
  {
    // maybe sparse file can have (node.NumBlocks == 0) ?

    /* The following code doesn't work correctly for some CentOS images,
       where there are nodes with inline data and (node.NumBlocks != 0).
       If you know better way to detect inline data, please notify 7-Zip developers. */

    if (node.NumBlocks == 0 && node.FileSize < kNodeBlockFieldSize)
    {
      Create_BufInStream_WithNewBuffer(node.Block, (size_t)node.FileSize, stream);
      return S_OK;
    }
  }

  if (node.FileSize >= ((UInt64)1 << 63))
    return S_FALSE;

  CMyComPtr<IInStream> streamTemp;
  
  const UInt64 numBlocks64 = (node.FileSize + (UInt64)(((UInt32)1 << _h.BlockBits) - 1)) >> _h.BlockBits;
  
  if (node.IsFlags_EXTENTS())
  {
    if ((UInt32)numBlocks64 != numBlocks64)
      return S_FALSE;

    CExtInStream *streamSpec = new CExtInStream;
    streamTemp = streamSpec;
    
    streamSpec->BlockBits = _h.BlockBits;
    streamSpec->Size = node.FileSize;
    streamSpec->Stream = _stream;
    
    RINOK(FillExtents(node.Block, kNodeBlockFieldSize, streamSpec->Extents, -1))

    UInt32 end = 0;
    if (!streamSpec->Extents.IsEmpty())
      end = streamSpec->Extents.Back().GetVirtEnd();
    if (end < numBlocks64)
    {
      AddSkipExtents(streamSpec->Extents, end, (UInt32)(numBlocks64 - end));
      // return S_FALSE;
    }

    RINOK(streamSpec->StartSeek())
  }
  else
  {
    {
      UInt64 numBlocks2 = numBlocks64;

      if (numBlocks64 > kNumDirectNodeBlocks)
      {
        UInt64 rem = numBlocks64 - kNumDirectNodeBlocks;
        const unsigned refBits = (_h.BlockBits - 2);
        const size_t numRefsInBlocks = (size_t)1 << refBits;
        numBlocks2++;
        if (rem > numRefsInBlocks)
        {
          numBlocks2++;
          const UInt64 numL2 = (rem - 1) >> refBits;
          numBlocks2 += numL2;
          if (numL2 > numRefsInBlocks)
          {
            numBlocks2++;
            numBlocks2 += (numL2 - 1) >> refBits;
          }
        }
      }

      const unsigned specBits = (node.IsFlags_HUGE() ? 0 : _h.BlockBits - 9);
      const UInt32 specMask = ((UInt32)1 << specBits) - 1;
      if ((node.NumBlocks & specMask) != 0)
        return S_FALSE;
      const UInt64 numBlocks64_from_header = node.NumBlocks >> specBits;
      if (numBlocks64_from_header < numBlocks2)
      {
        // why (numBlocks64_from_header > numBlocks2) in some cases?
        // return S_FALSE;
      }
    }

    unsigned numBlocks = (unsigned)numBlocks64;
    if (numBlocks != numBlocks64)
      return S_FALSE;

    CClusterInStream2 *streamSpec = new CClusterInStream2;
    streamTemp = streamSpec;

    streamSpec->BlockBits = _h.BlockBits;
    streamSpec->Size = node.FileSize;
    streamSpec->Stream = _stream;

    RINOK(FillFileBlocks(node.Block, numBlocks, streamSpec->Vector))
    streamSpec->InitAndSeek();
  }

  *stream = streamTemp.Detach();

  return S_OK;

  COM_TRY_END
}


HRESULT CHandler::ExtractNode(unsigned nodeIndex, CByteBuffer &data)
{
  data.Free();
  const CNode &node = _nodes[nodeIndex];
  size_t size = (size_t)node.FileSize;
  if (size != node.FileSize)
    return S_FALSE;
  CMyComPtr<ISequentialInStream> inSeqStream;
  RINOK(GetStream_Node(nodeIndex, &inSeqStream))
  if (!inSeqStream)
    return S_FALSE;
  data.Alloc(size);
  _totalRead += size;
  return ReadStream_FALSE(inSeqStream, data, size);
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _items.Size() + _auxItems.Size();
  if (numItems == 0)
    return S_OK;
  
  UInt64 totalSize = 0;
  UInt32 i;

  for (i = 0; i < numItems; i++)
  {
    const UInt32 index = allFilesMode ? i : indices[i];
    if (index >= _items.Size())
      continue;
    const CItem &item = _items[index];
    const CNode &node = _nodes[_refs[item.Node]];
    if (!node.IsDir())
      totalSize += node.FileSize;
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

    int opRes;
   {
    CMyComPtr<ISequentialOutStream> outStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    
    const UInt32 index = allFilesMode ? i : indices[i];
    
    RINOK(extractCallback->GetStream(index, &outStream, askMode))

    if (index >= _items.Size())
    {
      RINOK(extractCallback->PrepareOperation(askMode))
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      continue;
    }

    const CItem &item = _items[index];
    const CNode &node = _nodes[_refs[item.Node]];

    if (node.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode))
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      continue;
    }

    UInt64 unpackSize = node.FileSize;
    totalSize += unpackSize;
    UInt64 packSize;
    if (GetPackSize(index, packSize))
      totalPackSize += packSize;

    if (!testMode && !outStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode))

    opRes = NExtract::NOperationResult::kDataError;
    {
      CMyComPtr<ISequentialInStream> inSeqStream;
      HRESULT hres = GetStream(index, &inSeqStream);
      if (hres == S_FALSE || !inSeqStream)
      {
        if (hres == E_OUTOFMEMORY)
          return hres;
        opRes = NExtract::NOperationResult::kUnsupportedMethod;
      }
      else
      {
        RINOK(hres)
        {
          hres = copyCoder.Interface()->Code(inSeqStream, outStream, NULL, NULL, lps);
          if (hres == S_OK)
          {
            if (copyCoder->TotalSize == unpackSize)
              opRes = NExtract::NOperationResult::kOK;
          }
          else if (hres == E_NOTIMPL)
          {
            opRes = NExtract::NOperationResult::kUnsupportedMethod;
          }
          else if (hres != S_FALSE)
          {
            RINOK(hres)
          }
        }
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
  *stream = NULL;
  if (index >= _items.Size())
    return S_FALSE;
  return GetStream_Node((unsigned)_refs[_items[index].Node], stream);
}


API_FUNC_IsArc IsArc_Ext_PhySize(const Byte *p, size_t size, UInt64 *phySize);
API_FUNC_IsArc IsArc_Ext_PhySize(const Byte *p, size_t size, UInt64 *phySize)
{
  if (phySize)
    *phySize = 0;
  if (size < kHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  CHeader h;
  if (!h.Parse(p + kHeaderDataOffset))
    return k_IsArc_Res_NO;
  if (phySize)
    *phySize = h.GetPhySize();
  return k_IsArc_Res_YES;
}


API_FUNC_IsArc IsArc_Ext(const Byte *p, size_t size);
API_FUNC_IsArc IsArc_Ext(const Byte *p, size_t size)
{
  return IsArc_Ext_PhySize(p, size, NULL);
}


static const Byte k_Signature[] = { 0x53, 0xEF };

REGISTER_ARC_I(
  "Ext", "ext ext2 ext3 ext4 img", NULL, 0xC7,
  k_Signature,
  0x438,
  0,
  IsArc_Ext)

}}

/* ================ unit: CPP/7zip/Archive/FatHandler.cpp ================ */
// FatHandler.cpp

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

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get16a(p) GetUi16a(p)
#define Get32a(p) GetUi32a(p)

#if 0
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif

namespace NArchive {
namespace NFat {

static const UInt32 kFatItemUsedByDirMask = (UInt32)1 << 31;

struct CHeader
{
  Byte NumFatBits;
  Byte SectorSizeLog;
  Byte SectorsPerClusterLog;
  Byte ClusterSizeLog;
  Byte NumFats;
  Byte MediaType;

  bool VolFieldsDefined;
  bool HeadersWarning;

  UInt32 FatSize;
  UInt32 BadCluster;

  UInt16 NumReservedSectors;
  UInt32 NumSectors;
  UInt32 NumFatSectors;
  UInt32 RootDirSector;
  UInt32 NumRootDirSectors;
  UInt32 DataSector;

  UInt16 SectorsPerTrack;
  UInt16 NumHeads;
  UInt32 NumHiddenSectors;
  
  UInt32 VolId;
  // Byte VolName[11];
  // Byte FileSys[8];
  // Byte OemName[5];

  // 32-bit FAT
  UInt16 Flags;
  UInt16 FsInfoSector;
  UInt32 RootCluster;

  bool IsFat32() const { return NumFatBits == 32; }
  UInt64 GetPhySize() const { return (UInt64)NumSectors << SectorSizeLog; }
  UInt32 SectorSize() const { return (UInt32)1 << SectorSizeLog; }
  UInt32 ClusterSize() const { return (UInt32)1 << ClusterSizeLog; }
  UInt32 ClusterToSector(UInt32 c) const { return DataSector + ((c - 2) << SectorsPerClusterLog); }
  UInt32 IsEoc(UInt32 c) const { return c > BadCluster; }
  UInt32 IsEocAndUnused(UInt32 c) const { return c > BadCluster && (c & kFatItemUsedByDirMask) == 0; }
  UInt32 IsValidCluster(UInt32 c) const { return c >= 2 && c < FatSize; }
  UInt32 SizeToSectors(UInt32 size) const { return (size + SectorSize() - 1) >> SectorSizeLog; }
  UInt32 CalcFatSizeInSectors() const { return SizeToSectors((FatSize * (NumFatBits / 4) + 1) / 2); }

  UInt32 GetFatSector() const
  {
    UInt32 index = (IsFat32() && (Flags & 0x80) != 0) ? (Flags & 0xF) : 0;
    if (index > NumFats)
      index = 0;
    return NumReservedSectors + index * NumFatSectors;
  }

  UInt64 GetFilePackSize(UInt32 unpackSize) const
  {
    UInt64 mask = ClusterSize() - 1;
    return (unpackSize + mask) & ~mask;
  }

  UInt32 GetNumClusters(UInt32 size) const
    { return (UInt32)(((UInt64)size + ClusterSize() - 1) >> ClusterSizeLog); }

  bool Parse(const Byte *p);
};


static const unsigned kHeaderSize = 512;

API_FUNC_IsArc IsArc_Fat(const Byte *p, size_t size);
API_FUNC_IsArc IsArc_Fat(const Byte *p, size_t size)
{
  if (size < kHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  CHeader h;
  return h.Parse(p) ? k_IsArc_Res_YES : k_IsArc_Res_NO;
}

bool CHeader::Parse(const Byte *p)
{
  if (Get16(p + 0x1FE) != 0xAA55)
    return false;

  HeadersWarning = false;

  int codeOffset = 0;
  switch (p[0])
  {
    case 0xE9: codeOffset = 3 + (Int16)Get16(p + 1); break;
    case 0xEB: if (p[2] != 0x90) return false; codeOffset = 2 + (signed char)p[1]; break;
    default: return false;
  }
  {
    {
      const unsigned num = Get16(p + 11);
      unsigned i = 9;
      unsigned m = 1 << i;
      for (;;)
      {
        if (m == num)
          break;
        m <<= 1;
        if (++i > 12)
          return false;
      }
      SectorSizeLog = (Byte)i;
    }
    {
      const unsigned num = p[13];
      unsigned i = 0;
      unsigned m = 1 << i;
      for (;;)
      {
        if (m == num)
          break;
        m <<= 1;
        if (++i > 7)
          return false;
      }
      SectorsPerClusterLog = (Byte)i;
      i += SectorSizeLog;
      ClusterSizeLog = (Byte)i;
      // (2^15 = 32 KB is safe cluster size that is suported by all system.
      // (2^16 = 64 KB is supported by some systems
      // (128 KB / 256 KB) can be created by some tools, but it is not supported by many tools.
      if (i > 18) // 256 KB
        return false;
    }
  }

  NumReservedSectors = Get16(p + 14);
  if (NumReservedSectors == 0)
    return false;

  NumFats = p[16];
  if (NumFats < 1 || NumFats > 4)
    return false;

  // we also support images that contain 0 in offset field.
  const bool isOkOffset = (codeOffset == 0)
      || (codeOffset == (p[0] == 0xEB ? 2 : 3));

  const unsigned numRootDirEntries = Get16(p + 17);
  if (numRootDirEntries == 0)
  {
    if (codeOffset < 90 && !isOkOffset)
      return false;
    NumFatBits = 32;
    NumRootDirSectors = 0;
  }
  else
  {
    // Some FAT12s don't contain VolFields
    if (codeOffset < 62 - 24 && !isOkOffset)
      return false;
    NumFatBits = 0;
    const unsigned mask = (1u << (SectorSizeLog - 5)) - 1;
    if (numRootDirEntries & mask)
      return false;
    NumRootDirSectors = (numRootDirEntries /* + mask */) >> (SectorSizeLog - 5);
  }

  NumSectors = Get16(p + 19);
  if (NumSectors == 0)
    NumSectors = Get32(p + 32);
  /*
  // mkfs.fat could create fat32 image with 16-bit number of sectors.
  // v23: we allow 16-bit value for number of sectors in fat32.
  else if (IsFat32())
    return false;
  */
  MediaType = p[21];
  NumFatSectors = Get16(p + 22);
  SectorsPerTrack = Get16(p + 24);
  NumHeads = Get16(p + 26);
  NumHiddenSectors = Get32(p + 28);

  // memcpy(OemName, p + 3, 5);

  int curOffset = 36;
  p += 36;
  if (IsFat32())
  {
    if (NumFatSectors != 0)
      return false;
    NumFatSectors = Get32(p);
    if (NumFatSectors >= (1 << 24))
      return false;

    Flags = Get16(p + 4);
    if (Get16(p + 6) != 0)
      return false;
    RootCluster = Get32(p + 8);
    FsInfoSector = Get16(p + 12);
    for (unsigned i = 16; i < 28; i++)
      if (p[i] != 0)
        return false;
    p += 28;
    curOffset += 28;
  }

  // DriveNumber = p[0];
  VolFieldsDefined = false;
  if (codeOffset >= curOffset + 3)
  {
    VolFieldsDefined = (p[2] == 0x29); // ExtendedBootSig
    if (VolFieldsDefined)
    {
      if (codeOffset < curOffset + 26)
        return false;
      VolId = Get32(p + 3);
      // memcpy(VolName, p + 7, 11);
      // memcpy(FileSys, p + 18, 8);
    }
  }

  if (NumFatSectors == 0)
    return false;
  RootDirSector = NumReservedSectors + NumFatSectors * NumFats;
  DataSector = RootDirSector + NumRootDirSectors;
  if (NumSectors < DataSector)
    return false;
  const UInt32 numDataSectors = NumSectors - DataSector;
  const UInt32 numClusters = numDataSectors >> SectorsPerClusterLog;
  
  BadCluster = 0x0FFFFFF7;
  // v23: we support unusual (< 0xFFF5) numClusters values in fat32 systems
  if (NumFatBits != 32)
  {
    if (numClusters >= 0xFFF5)
      return false;
    NumFatBits = (Byte)(numClusters < 0xFF5 ? 12 : 16);
    BadCluster &= (((UInt32)1 << NumFatBits) - 1);
  }

  FatSize = numClusters + 2;
  if (FatSize > BadCluster)
    return false;
  if (CalcFatSizeInSectors() > NumFatSectors)
  {
    /* some third-party program can create such FAT image, where
       size of FAT table (NumFatSectors from headers) is smaller than
       required value that is calculated from calculated (FatSize) value.
       Another FAT unpackers probably ignore that error.
       v23.02: we also ignore that error, and
       we recalculate (FatSize) value from (NumFatSectors).
       New (FatSize) will be smaller than original "full" (FatSize) value.
       So we will have some unused clusters at the end of archive.
    */
    FatSize = (UInt32)(((UInt64)NumFatSectors << (3 + SectorSizeLog)) / NumFatBits);
    HeadersWarning = true;
  }
  return true;
}



class CItem
{
  Z7_CLASS_NO_COPY(CItem)
public:
  UInt32 Size;
  Byte Attrib;
  Byte CTime2;
  UInt16 ADate;
  CByteBuffer LongName; // if LongName.Size() == 0 : no long name
                        // if LongName.Size() != 0 : it's NULL terminated UTF16-LE string.
  char DosName[11];
  Byte Flags;
  UInt32 MTime;
  UInt32 CTime;
  UInt32 Cluster;
  Int32 Parent;

  CItem() {}

  // NT uses Flags to store Low Case status
  bool NameIsLow() const { return (Flags & 0x8) != 0; }
  bool ExtIsLow() const { return (Flags & 0x10) != 0; }
  bool IsDir() const { return (Attrib & 0x10) != 0; }
  void GetShortName(UString &dest) const;
  void GetName(UString &name) const;
};


static char *CopyAndTrim(char *dest, const char *src,
    unsigned size, unsigned toLower)
{
  do
  {
    if (src[(size_t)size - 1] != ' ')
    {
      const unsigned range = toLower ? 'Z' - 'A' + 1 : 0;
      do
      {
        unsigned c = (Byte)*src++;
        if ((unsigned)(c - 'A') < range)
          c += 0x20;
        *dest++ = (char)c;
      }
      while (--size);
      break;
    }
  }
  while (--size);
  *dest = 0;
  return dest;
}


static void FatStringToUnicode(UString &dest, const char *s)
{
  MultiByteToUnicodeString2(dest, AString(s), CP_OEMCP);
}

void CItem::GetShortName(UString &shortName) const
{
  char s[16];
  char *dest = CopyAndTrim(s, DosName, 8, NameIsLow());
  *dest++ = '.';
  char *dest2 = CopyAndTrim(dest, DosName + 8, 3, ExtIsLow());
  if (dest == dest2)
    dest[-1] = 0;
  FatStringToUnicode(shortName, s);
}



// numWords != 0
static unsigned ParseLongName(UInt16 *buf, unsigned numWords)
{
  unsigned i;
  for (i = 0; i < numWords; i++)
  {
    const unsigned c = buf[i];
    if (c == 0)
      break;
    if (c == 0xFFFF)
      return 0;
  }
  if (i == 0)
    return 0;
  buf[i] = 0;
  numWords -= i;
  i++;
  if (numWords > 1)
  {
    numWords--;
    buf += i;
    do
      if (*buf++ != 0xFFFF)
        return 0;
    while (--numWords);
  }
  return i; // it includes NULL terminator
}


void CItem::GetName(UString &name) const
{
  if (LongName.Size() >= 2)
  {
    const Byte * const p = LongName;
    const unsigned numWords = ((unsigned)LongName.Size() - 2) / 2;
    wchar_t *dest = name.GetBuf(numWords);
    for (unsigned i = 0; i < numWords; i++)
      dest[i] = (wchar_t)Get16(p + (size_t)i * 2);
    name.ReleaseBuf_SetEnd(numWords);
  }
  else
    GetShortName(name);
  if (name.IsEmpty()) // it's unexpected
    name = '_';
  NItemName::NormalizeSlashes_in_FileName_for_OsPath(name);
}


static void GetVolName(const char dosName[11], NWindows::NCOM::CPropVariant &prop)
{
  char s[12];
  CopyAndTrim(s, dosName, 11, false);
  UString u;
  FatStringToUnicode(u, AString(s));
  prop = u;
}


struct CDatabase
{
  CObjectVector<CItem> Items;
  UInt32 *Fat;
  CHeader Header;
  CMyComPtr<IInStream> InStream;
  IArchiveOpenCallback *OpenCallback;
  CAlignedBuffer ByteBuf;
  CByteBuffer LfnBuf;

  UInt32 NumFreeClusters;
  UInt32 NumDirClusters;
  UInt64 NumCurUsedBytes;
  UInt64 PhySize;

  UInt32 Vol_MTime;
  char VolLabel[11];
  bool VolItem_Defined;

  CDatabase(): Fat(NULL) {}
  ~CDatabase() { ClearAndClose(); }

  void Clear();
  void ClearAndClose();
  HRESULT OpenProgressFat(bool changeTotal = true);
  HRESULT OpenProgress();

  void GetItemPath(UInt32 index, UString &s) const;
  HRESULT Open();
  HRESULT ReadDir(Int32 parent, UInt32 cluster, unsigned level);

  UInt64 GetHeadersSize() const
  {
    return (UInt64)(Header.DataSector + (NumDirClusters << Header.SectorsPerClusterLog)) << Header.SectorSizeLog;
  }
  HRESULT SeekToSector(UInt32 sector);
  HRESULT SeekToCluster(UInt32 cluster) { return SeekToSector(Header.ClusterToSector(cluster)); }
};


HRESULT CDatabase::SeekToSector(UInt32 sector)
{
  return InStream_SeekSet(InStream, (UInt64)sector << Header.SectorSizeLog);
}

void CDatabase::Clear()
{
  PhySize = 0;
  VolItem_Defined = false;
  NumDirClusters = 0;
  NumCurUsedBytes = 0;

  Items.Clear();
  delete []Fat;
  Fat = NULL;
}

void CDatabase::ClearAndClose()
{
  Clear();
  InStream.Release();
}

HRESULT CDatabase::OpenProgressFat(bool changeTotal)
{
  if (!OpenCallback)
    return S_OK;
  if (changeTotal)
  {
    const UInt64 numTotalBytes = (Header.CalcFatSizeInSectors() << Header.SectorSizeLog) +
        ((UInt64)(Header.FatSize - NumFreeClusters) << Header.ClusterSizeLog);
    RINOK(OpenCallback->SetTotal(NULL, &numTotalBytes))
  }
  return OpenCallback->SetCompleted(NULL, &NumCurUsedBytes);
}

HRESULT CDatabase::OpenProgress()
{
  if (!OpenCallback)
    return S_OK;
  const UInt64 numItems = Items.Size();
  return OpenCallback->SetCompleted(&numItems, &NumCurUsedBytes);
}

void CDatabase::GetItemPath(UInt32 index, UString &s) const
{
  UString name;
  for (;;)
  {
    const CItem &item = Items[index];
    item.GetName(name);
    if (item.Parent >= 0)
      name.InsertAtFront(WCHAR_PATH_SEPARATOR);
    s.Insert(0, name);
    index = (UInt32)item.Parent;
    if (item.Parent < 0)
      break;
  }
}


HRESULT CDatabase::ReadDir(Int32 parent, UInt32 cluster, unsigned level)
{
  const unsigned startIndex = Items.Size();
  if (startIndex >= (1 << 30) || level > 256)
    return S_FALSE;

  UInt32 blockSize = Header.ClusterSize();
  const bool clusterMode = (Header.IsFat32() || parent >= 0);
  if (!clusterMode)
  {
    blockSize = Header.SectorSize();
    RINOK(SeekToSector(Header.RootDirSector))
  }

  ByteBuf.Alloc(blockSize);

  const unsigned k_NumLfnRecords_MAX = 20; // 260 symbols limit (strict limit)
  // const unsigned k_NumLfnRecords_MAX = 0x40 - 1; // 1260 symbols limit (relaxed limit)
  const unsigned k_NumLfnBytes_in_Record = 13 * 2;
  // we reserve 2 additional bytes for NULL terminator
  LfnBuf.Alloc(k_NumLfnRecords_MAX * k_NumLfnBytes_in_Record + 2 * 1);
  
  UInt32 curDirBytes_read = 0;
  UInt32 sectorIndex = 0;
  unsigned num_lfn_records = 0;
  unsigned lfn_RecordIndex = 0;
  int checkSum = -1;
  bool is_record_error = false;

  for (UInt32 pos = blockSize;; pos += 32)
  {
    if (pos == blockSize)
    {
      pos = 0;

      if (clusterMode)
      {
        if (Header.IsEoc(cluster))
          break;
        if (!Header.IsValidCluster(cluster))
          return S_FALSE;
        PRF(printf("\nCluster = %4X", cluster));
        RINOK(SeekToCluster(cluster))
        const UInt32 newCluster = Fat[cluster];
        if (newCluster & kFatItemUsedByDirMask)
          return S_FALSE;
        Fat[cluster] |= kFatItemUsedByDirMask;
        cluster = newCluster;
        NumDirClusters++;
        if ((NumDirClusters & 0xFF) == 0)
        {
          RINOK(OpenProgress())
        }
        NumCurUsedBytes += Header.ClusterSize();
      }
      else if (sectorIndex++ >= Header.NumRootDirSectors)
        break;
      
      // if (curDirBytes_read > (1u << 28)) // do we need some relaxed limit for non-MS FATs?
      if (curDirBytes_read >= (1u << 21)) // 2MB limit from FAT specification.
        return S_FALSE;
      RINOK(ReadStream_FALSE(InStream, ByteBuf, blockSize))
      curDirBytes_read += blockSize;
    }
  
    if (is_record_error)
    {
      Header.HeadersWarning = true;
      num_lfn_records = 0;
      lfn_RecordIndex = 0;
      checkSum = -1;
    }

    const Byte * const p = ByteBuf + pos;
   
    if (p[0] == 0)
    {
      /*
      // FreeDOS formats FAT partition with cluster chain longer than required.
      if (clusterMode && !Header.IsEoc(cluster))
        return S_FALSE;
      */
      break;
    }

    is_record_error = true;

    if (p[0] == 0xE5)
    {
      // deleted entry
      if (num_lfn_records == 0)
        is_record_error = false;
      continue;
    }
    // else
    {
      const Byte attrib = p[11];
      // maybe we can use more strick check : (attrib == 0xF) ?
      if ((attrib & 0x3F) == 0xF)
      {
        // long file name (LFN) entry
        const unsigned longIndex = p[0] & 0x3F;
        if (longIndex == 0
            || longIndex > k_NumLfnRecords_MAX
            || p[0] > 0x7F
            || Get16a(p + 26) != 0 // LDIR_FstClusLO
            )
        {
          return S_FALSE;
          // break;
        }
        const bool isLast = (p[0] & 0x40) != 0;
        if (num_lfn_records == 0)
        {
          if (!isLast)
            continue; // orphan
          num_lfn_records = longIndex;
        }
        else if (isLast || longIndex != lfn_RecordIndex)
        {
          return S_FALSE;
          // break;
        }
        
        lfn_RecordIndex = longIndex - 1;
        
        if (p[12] == 0)
        {
          Byte * const dest = LfnBuf + k_NumLfnBytes_in_Record * lfn_RecordIndex;
          memcpy(dest,          p +  1, 5 * 2);
          memcpy(dest +  5 * 2, p + 14, 6 * 2);
          memcpy(dest + 11 * 2, p + 28, 2 * 2);
          if (isLast)
            checkSum = p[13];
          if (checkSum == p[13])
            is_record_error = false;
          // else return S_FALSE;
          continue;
        }
        // else
        checkSum = -1; // we will ignore LfnBuf in this case
        continue;
      }
      
      if (lfn_RecordIndex)
      {
        Header.HeadersWarning = true;
        // return S_FALSE;
      }
      // lfn_RecordIndex = 0;

      const unsigned type_in_attrib = attrib & 0x18;
      if (type_in_attrib == 0x18)
      {
        // invalid directory record (both flags are set: dir_flag and volume_flag)
        return S_FALSE;
        // break;
        // continue;
      }
      if (type_in_attrib == 8) // volume_flag
      {
        if (!VolItem_Defined && level == 0)
        {
          VolItem_Defined = true;
          memcpy(VolLabel, p, 11);
          Vol_MTime = Get32(p + 22);
          is_record_error = false;
        }
      }
      else if (memcmp(p, ".          ", 11) == 0
            || memcmp(p, "..         ", 11) == 0)
      {
        if (num_lfn_records == 0 && type_in_attrib == 0x10) // dir_flag
          is_record_error = false;
      }
      else
      {
        CItem &item = Items.AddNew();
        memcpy(item.DosName, p, 11);
        if (item.DosName[0] == 5)
          item.DosName[0] = (char)(Byte)0xE5; // 0xE5 is valid KANJI lead byte value.
        item.Attrib = attrib;
        item.Flags = p[12];
        item.Size = Get32a(p + 28);
        item.Cluster = Get16a(p + 26);
        if (Header.NumFatBits > 16)
          item.Cluster |= ((UInt32)Get16a(p + 20) << 16);
        else
        {
          // OS/2 and WinNT probably can store EA (extended atributes) in that field.
        }
        item.CTime = Get32(p + 14);
        item.CTime2 = p[13];
        item.ADate = Get16a(p + 18);
        item.MTime = Get32(p + 22);
        item.Parent = parent;
        {
          if (!item.IsDir())
            NumCurUsedBytes += Header.GetFilePackSize(item.Size);
          // PRF(printf("\n%7d: %S", Items.Size(), GetItemPath(Items.Size() - 1)));
          PRF(printf("\n%7d" /* ": %S" */, Items.Size() /* , item.GetShortName() */ );)
        }
        if (num_lfn_records == 0)
          is_record_error = false;
        else if (checkSum >= 0 && lfn_RecordIndex == 0)
        {
          Byte sum = 0;
          for (unsigned i = 0; i < 11; i++)
            sum = (Byte)((sum << 7) + (sum >> 1) + (Byte)item.DosName[i]);
          if (sum == checkSum)
          {
            const unsigned numWords = ParseLongName((UInt16 *)(void *)(Byte *)LfnBuf,
                num_lfn_records * k_NumLfnBytes_in_Record / 2);
            if (numWords > 1)
            {
              // numWords includes NULL terminator
              item.LongName.CopyFrom(LfnBuf, numWords * 2);
              is_record_error = false;
            }
          }
        }
        
        if (
            // item.LongName.Size() < 20 ||  // for debug
            item.LongName.Size() <= 2 * 1
            && memcmp(p, "           ", 11) == 0)
        {
          char s[16 + 16];
          const size_t numChars = (size_t)(ConvertUInt32ToString(
              Items.Size() - 1 - startIndex,
              MyStpCpy(s, "[NONAME]-")) - s) + 1;
          item.LongName.Alloc(numChars * 2);
          for (size_t i = 0; i < numChars; i++)
          {
            SetUi16a(item.LongName + i * 2, (Byte)s[i])
          }
          Header.HeadersWarning = true;
        }
      }
      num_lfn_records = 0;
    }
  }

  if (is_record_error)
    Header.HeadersWarning = true;

  const unsigned finishIndex = Items.Size();
  for (unsigned i = startIndex; i < finishIndex; i++)
  {
    const CItem &item = Items[i];
    if (item.IsDir())
    {
      PRF(printf("\n---- %c%c%c%c%c", item.DosName[0], item.DosName[1], item.DosName[2], item.DosName[3], item.DosName[4]));
      RINOK(ReadDir((int)i, item.Cluster, level + 1))
    }
  }
  return S_OK;
}



HRESULT CDatabase::Open()
{
  Clear();
  bool numFreeClusters_Defined = false;
  {
    UInt32 buf32[kHeaderSize / 4];
    RINOK(ReadStream_FALSE(InStream, buf32, kHeaderSize))
    if (!Header.Parse((Byte *)(void *)buf32))
      return S_FALSE;
    UInt64 fileSize;
    RINOK(InStream_GetSize_SeekToEnd(InStream, fileSize))

    /* we comment that check to support truncated images */
    /*
    if (fileSize < Header.GetPhySize())
      return S_FALSE;
    */

    if (Header.IsFat32())
    {
      if (((UInt32)Header.FsInfoSector << Header.SectorSizeLog) + kHeaderSize <= fileSize
          && SeekToSector(Header.FsInfoSector) == S_OK
          && ReadStream_FALSE(InStream, buf32, kHeaderSize) == S_OK
          && 0xaa550000 == Get32a(buf32 + 508 / 4)
          && 0x41615252 == Get32a(buf32)
          && 0x61417272 == Get32a(buf32 + 484 / 4))
      {
        NumFreeClusters = Get32a(buf32 + 488 / 4);
        numFreeClusters_Defined = (NumFreeClusters <= Header.FatSize);
      }
      else
        Header.HeadersWarning = true;
    }
  }

  // numFreeClusters_Defined = false; // to recalculate NumFreeClusters
  if (!numFreeClusters_Defined)
    NumFreeClusters = 0;

  CByteBuffer byteBuf;
  Fat = new UInt32[Header.FatSize];

  RINOK(OpenProgressFat())
  RINOK(SeekToSector(Header.GetFatSector()))
  if (Header.NumFatBits == 32)
  {
    const UInt32 kBufSize = 1 << 15;
    byteBuf.Alloc(kBufSize);
    for (UInt32 i = 0;;)
    {
      UInt32 size = Header.FatSize - i;
      if (size == 0)
        break;
      const UInt32 kBufSize32 = kBufSize / 4;
      if (size > kBufSize32)
        size = kBufSize32;
      const UInt32 readSize = Header.SizeToSectors(size * 4) << Header.SectorSizeLog;
      RINOK(ReadStream_FALSE(InStream, byteBuf, readSize))
      NumCurUsedBytes += readSize;

      const UInt32 *src = (const UInt32 *)(const void *)(const Byte *)byteBuf;
      UInt32 *dest = Fat + i;
      const UInt32 *srcLim = src + size;
      if (numFreeClusters_Defined)
        do
          *dest++ = Get32a(src) & 0x0FFFFFFF;
        while (++src != srcLim);
      else
      {
        UInt32 numFreeClusters = 0;
        do
        {
          const UInt32 v = Get32a(src) & 0x0FFFFFFF;
          *dest++ = v;
          numFreeClusters += (UInt32)(v - 1) >> 31;
        }
        while (++src != srcLim);
        NumFreeClusters += numFreeClusters;
      }
      i += size;
      if ((i & 0xFFFFF) == 0)
      {
        RINOK(OpenProgressFat(!numFreeClusters_Defined))
      }
    }
  }
  else
  {
    const UInt32 kBufSize = (UInt32)Header.CalcFatSizeInSectors() << Header.SectorSizeLog;
    NumCurUsedBytes += kBufSize;
    byteBuf.Alloc(kBufSize);
    Byte *p = byteBuf;
    RINOK(ReadStream_FALSE(InStream, p, kBufSize))
    const UInt32 fatSize = Header.FatSize;
    UInt32 *fat = &Fat[0];
    if (Header.NumFatBits == 16)
      for (UInt32 j = 0; j < fatSize; j++)
        fat[j] = Get16a(p + j * 2);
    else
      for (UInt32 j = 0; j < fatSize; j++)
        fat[j] = (Get16(p + j * 3 / 2) >> ((j & 1) << 2)) & 0xFFF;

    if (!numFreeClusters_Defined)
    {
      UInt32 numFreeClusters = 0;
      for (UInt32 i = 0; i < fatSize; i++)
        numFreeClusters += (UInt32)(fat[i] - 1) >> 31;
      NumFreeClusters = numFreeClusters;
    }
  }

  RINOK(OpenProgressFat())

  if ((Fat[0] & 0xFF) != Header.MediaType)
  {
    // that case can mean error in FAT,
    // but xdf file: (MediaType == 0xF0 && Fat[0] == 0xFF9)
    // 19.00: so we use non-strict check
    if ((Fat[0] & 0xFF) < 0xF0)
      return S_FALSE;
  }

  RINOK(ReadDir(-1, Header.RootCluster, 0))

  PhySize = Header.GetPhySize();
  return S_OK;
}



Z7_class_CHandler_final:
  public IInArchive,
  public IArchiveGetRawProps,
  public IInArchiveGetStream,
  public CMyUnknownImp,
  CDatabase
{
  Z7_IFACES_IMP_UNK_3(IInArchive, IArchiveGetRawProps, IInArchiveGetStream)
};

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  const CItem &item = Items[index];
  CClusterInStream *streamSpec = new CClusterInStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  streamSpec->Stream = InStream;
  streamSpec->StartOffset = Header.DataSector << Header.SectorSizeLog;
  streamSpec->BlockSizeLog = Header.ClusterSizeLog;
  streamSpec->Size = item.Size;

  const UInt32 numClusters = Header.GetNumClusters(item.Size);
  streamSpec->Vector.ClearAndReserve(numClusters);
  UInt32 cluster = item.Cluster;
  UInt32 size = item.Size;

  if (size == 0)
  {
    if (cluster != 0)
      return S_FALSE;
  }
  else
  {
    const UInt32 clusterSize = Header.ClusterSize();
    for (;; size -= clusterSize)
    {
      if (!Header.IsValidCluster(cluster))
        return S_FALSE;
      streamSpec->Vector.AddInReserved(cluster - 2);
      cluster = Fat[cluster];
      if (size <= clusterSize)
        break;
    }
    if (!Header.IsEocAndUnused(cluster))
      return S_FALSE;
  }
  RINOK(streamSpec->InitAndSeek())
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}



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
  kpidShortName
  // , kpidCharacts
};

enum
{
  kpidNumFats = kpidUserDefined
  // kpidOemName,
  // kpidVolName,
  // kpidFileSysType
};

static const CStatProp kArcProps[] =
{
  { NULL, kpidFileSystem, VT_BSTR},
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidFreeSpace, VT_UI8},
  { NULL, kpidHeadersSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidVolumeName, VT_BSTR},

  { "FATs", kpidNumFats, VT_UI4},
  { NULL, kpidSectorSize, VT_UI4},
  { NULL, kpidId, VT_UI4},
  // { "OEM Name", kpidOemName, VT_BSTR},
  // { "Volume Name", kpidVolName, VT_BSTR},
  // { "File System Type", kpidFileSysType, VT_BSTR}
  // { NULL, kpidSectorsPerTrack, VT_UI4},
  // { NULL, kpidNumHeads, VT_UI4},
  // { NULL, kpidHiddenSectors, VT_UI4}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME


static void FatTimeToProp(UInt32 dosTime, UInt32 ms10, NWindows::NCOM::CPropVariant &prop)
{
  FILETIME localFileTime, utc;
  if (NWindows::NTime::DosTime_To_FileTime(dosTime, localFileTime))
    if (LocalFileTimeToFileTime(&localFileTime, &utc))
    {
      UInt64 t64 = (((UInt64)utc.dwHighDateTime) << 32) + utc.dwLowDateTime;
      t64 += ms10 * 100000;
      utc.dwLowDateTime = (DWORD)t64;
      utc.dwHighDateTime = (DWORD)(t64 >> 32);
      prop.SetAsTimeFrom_FT_Prec(utc, k_PropVar_TimePrec_Base + 2);
    }
}

/*
static void StringToProp(const Byte *src, unsigned size, NWindows::NCOM::CPropVariant &prop)
{
  char dest[32];
  memcpy(dest, src, size);
  dest[size] = 0;
  prop = FatStringToUnicode(dest);
}

#define STRING_TO_PROP(s, p) StringToProp(s, sizeof(s), prop)
*/

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidFileSystem:
    {
      char s[16];
      s[0] = 'F';
      s[1] = 'A';
      s[2] = 'T';
      ConvertUInt32ToString(Header.NumFatBits, s + 3);
      prop = s;
      break;
    }
    case kpidClusterSize: prop = Header.ClusterSize(); break;
    case kpidPhySize: prop = PhySize; break;
    case kpidFreeSpace: prop = (UInt64)NumFreeClusters << Header.ClusterSizeLog; break;
    case kpidHeadersSize: prop = GetHeadersSize(); break;
    case kpidMTime: if (VolItem_Defined) PropVariant_SetFrom_DosTime(prop, Vol_MTime); break;
    case kpidShortComment:
    case kpidVolumeName: if (VolItem_Defined) GetVolName(VolLabel, prop); break;
    case kpidNumFats: if (Header.NumFats != 2) prop = Header.NumFats; break;
    case kpidSectorSize: prop = (UInt32)1 << Header.SectorSizeLog; break;
    // case kpidSectorsPerTrack: prop = Header.SectorsPerTrack; break;
    // case kpidNumHeads: prop = Header.NumHeads; break;
    // case kpidOemName: STRING_TO_PROP(Header.OemName, prop); break;
    case kpidId: if (Header.VolFieldsDefined) prop = Header.VolId; break;
    case kpidIsTree: prop = true; break;
    // case kpidVolName: if (Header.VolFieldsDefined) STRING_TO_PROP(Header.VolName, prop); break;
    // case kpidFileSysType: if (Header.VolFieldsDefined) STRING_TO_PROP(Header.FileSys, prop); break;
    // case kpidHiddenSectors: prop = Header.NumHiddenSectors; break;
    case kpidWarningFlags:
    {
      UInt32 v = 0;
      if (Header.HeadersWarning) v |= kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *numProps))
{
  *numProps = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32 /* index */ , BSTR *name, PROPID *propID))
{
  *name = NULL;
  *propID = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *parentType))
{
  *parentType = NParentType::kDir;
  int par = -1;
  if (index < Items.Size())
    par = Items[index].Parent;
  *parent = (UInt32)(Int32)par;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;

  if (index < Items.Size()
      && propID == kpidName)
  {
    CByteBuffer &buf = Items[index].LongName;
    const UInt32 size = (UInt32)buf.Size();
    if (size != 0)
    {
      *dataSize = size;
      *propType = NPropDataType::kUtf16z;
      *data = (const void *)(const Byte *)buf;
    }
  }
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItem &item = Items[index];
  switch (propID)
  {
    case kpidPath:
    case kpidName:
    case kpidShortName:
    {
      UString s;
      if (propID == kpidPath)
        GetItemPath(index, s);
      else if (propID == kpidName)
        item.GetName(s);
      else
        item.GetShortName(s);
      prop = s;
      break;
    }
/*
    case kpidCharacts:
    {
      if (item.LongName.Size())
        prop = "LFN";
      break;
    }
*/
    case kpidIsDir: prop = item.IsDir(); break;
    case kpidMTime: PropVariant_SetFrom_DosTime(prop, item.MTime); break;
    case kpidCTime: FatTimeToProp(item.CTime, item.CTime2, prop); break;
    case kpidATime: PropVariant_SetFrom_DosTime(prop, ((UInt32)item.ADate << 16)); break;
    case kpidAttrib: prop = (UInt32)item.Attrib; break;
    case kpidSize: if (!item.IsDir()) prop = item.Size; break;
    case kpidPackSize: if (!item.IsDir()) prop = Header.GetFilePackSize(item.Size); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  {
    OpenCallback = callback;
    InStream = stream;
    HRESULT res;
    try
    {
      res = CDatabase::Open();
      if (res == S_OK)
        return S_OK;
    }
    catch(...)
    {
      Close();
      throw;
    }
    Close();
    return res;
  }
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  ClearAndClose();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == (UInt32)(Int32)-1)
  {
    indices = NULL;
    numItems = Items.Size();
    if (numItems == 0)
      return S_OK;
  }
  else
  {
    if (numItems == 0)
      return S_OK;
    if (!indices)
      return E_INVALIDARG;
  }
  UInt64 totalSize = 0;
  {
    UInt32 i = 0;
    do
    {
      UInt32 index = i;
      if (indices)
        index = indices[i];
      const CItem &item = Items[index];
      if (!item.IsDir())
        totalSize += item.Size;
    }
    while (++i != numItems);
  }
  RINOK(extractCallback->SetTotal(totalSize))

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;

  UInt32 i;
  for (i = 0;; i++)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i == numItems)
      break;
    int res;
    {
      CMyComPtr<ISequentialOutStream> realOutStream;
      const Int32 askMode = testMode ?
          NExtract::NAskMode::kTest :
          NExtract::NAskMode::kExtract;
      UInt32 index = i;
      if (indices)
        index = indices[i];
      const CItem &item = Items[index];
      RINOK(extractCallback->GetStream(index, &realOutStream, askMode))
        
      if (item.IsDir())
      {
        RINOK(extractCallback->PrepareOperation(askMode))
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
        continue;
      }
      
      totalPackSize += Header.GetFilePackSize(item.Size);
      totalSize += item.Size;
      
      if (!testMode && !realOutStream)
        continue;
      RINOK(extractCallback->PrepareOperation(askMode))
      res = NExtract::NOperationResult::kDataError;
      CMyComPtr<ISequentialInStream> inStream;
      const HRESULT hres = GetStream(index, &inStream);
      if (hres != S_FALSE)
      {
        RINOK(hres)
        if (inStream)
        {
          RINOK(copyCoder.Interface()->Code(inStream, realOutStream, NULL, NULL, lps))
          if (copyCoder->TotalSize == item.Size)
            res = NExtract::NOperationResult::kOK;
        }
      }
    }
    RINOK(extractCallback->SetOperationResult(res))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = Items.Size();
  return S_OK;
}

static const Byte k_Signature[] = { 0x55, 0xAA };

REGISTER_ARC_I(
  "FAT", "fat img", NULL, 0xDA,
  k_Signature,
  0x1FE,
  0,
  IsArc_Fat)

}}

/* ================ unit: CPP/7zip/Archive/FlvHandler.cpp ================ */
// FlvHandler.cpp

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

#define GetBe24(p) ( \
    ((UInt32)((const Byte *)(p))[0] << 16) | \
    ((UInt32)((const Byte *)(p))[1] <<  8) | \
             ((const Byte *)(p))[2] )

// #define Get16(p) GetBe16(p)
#define Get24(p) GetBe24(p)
#define Get32(p) GetBe32(p)

namespace NArchive {
namespace NFlv {

// static const UInt32 kFileSizeMax = (UInt32)1 << 30;
static const UInt32 kNumChunksMax = (UInt32)1 << 23;

static const UInt32 kTagHeaderSize = 11;

static const Byte kFlag_Video = 1;
static const Byte kFlag_Audio = 4;

static const Byte kType_Audio = 8;
static const Byte kType_Video = 9;
static const Byte kType_Meta = 18;
static const unsigned kNumTypes = 19;

struct CItem
{
  CByteBuffer Data;
  Byte Type;
};

struct CItem2
{
  Byte Type;
  Byte SubType;
  Byte Props;
  bool SameSubTypes;
  unsigned NumChunks;
  size_t Size;

  CReferenceBuf *BufSpec;
  CMyComPtr<IUnknown> RefBuf;

  bool IsAudio() const { return Type == kType_Audio; }
};


Z7_CLASS_IMP_CHandler_IInArchive_1(
  IInArchiveGetStream
)
  CMyComPtr<IInStream> _stream;
  CObjectVector<CItem2> _items2;
  CByteBuffer _metadata;
  bool _isRaw;
  UInt64 _phySize;

  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *callback);
  // AString GetComment();
};

static const Byte kProps[] =
{
  kpidSize,
  kpidNumBlocks,
  kpidComment
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

static const char * const g_AudioTypes[16] =
{
    "pcm"
  , "adpcm"
  , "mp3"
  , "pcm_le"
  , "nellymoser16"
  , "nellymoser8"
  , "nellymoser"
  , "g711a"
  , "g711m"
  , "audio9"
  , "aac"
  , "speex"
  , "audio12"
  , "audio13"
  , "mp3"
  , "audio15"
};

static const char * const g_VideoTypes[16] =
{
    "video0"
  , "jpeg"
  , "h263"
  , "screen"
  , "vp6"
  , "vp6alpha"
  , "screen2"
  , "avc"
  , "video8"
  , "video9"
  , "video10"
  , "video11"
  , "video12"
  , "video13"
  , "video14"
  , "video15"
};

static const char * const g_Rates[4] =
{
    "5.5 kHz"
  , "11 kHz"
  , "22 kHz"
  , "44 kHz"
};

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  NWindows::NCOM::CPropVariant prop;
  const CItem2 &item = _items2[index];
  switch (propID)
  {
    case kpidExtension:
      prop = _isRaw ?
        (item.IsAudio() ? g_AudioTypes[item.SubType] : g_VideoTypes[item.SubType]) :
        (item.IsAudio() ? "audio.flv" : "video.flv");
      break;
    case kpidSize:
    case kpidPackSize:
      prop = (UInt64)item.Size;
      break;
    case kpidNumBlocks: prop = (UInt32)item.NumChunks; break;
    case kpidComment:
    {
      char sz[64];
      char *s = MyStpCpy(sz, (item.IsAudio() ? g_AudioTypes[item.SubType] : g_VideoTypes[item.SubType]) );
      if (item.IsAudio())
      {
        *s++ = ' ';
        s = MyStpCpy(s, g_Rates[(item.Props >> 2) & 3]);
        s = MyStpCpy(s, (item.Props & 2) ? " 16-bit" : " 8-bit");
        s = MyStpCpy(s, (item.Props & 1) ? " stereo" : " mono");
      }
      prop = sz;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

/*
AString CHandler::GetComment()
{
  const Byte *p = _metadata;
  size_t size = _metadata.Size();
  AString res;
  if (size > 0)
  {
    p++;
    size--;
    for (;;)
    {
      if (size < 2)
        break;
      int len = Get16(p);
      p += 2;
      size -= 2;
      if (len == 0 || (size_t)len > size)
        break;
      {
        AString temp;
        temp.SetFrom_CalcLen((const char *)p, len);
        if (!res.IsEmpty())
          res += '\n';
        res += temp;
      }
      p += len;
      size -= len;
      if (size < 1)
        break;
      Byte type = *p++;
      size--;
      bool ok = false;
      switch (type)
      {
        case 0:
        {
          if (size < 8)
            break;
          ok = true;
          Byte reverse[8];
          for (int i = 0; i < 8; i++)
          {
            bool little_endian = 1;
            if (little_endian)
              reverse[i] = p[7 - i];
            else
              reverse[i] = p[i];
          }
          double d = *(double *)reverse;
          char temp[32];
          sprintf(temp, " = %.3f", d);
          res += temp;
          p += 8;
          size -= 8;
          break;
        }
        case 8:
        {
          if (size < 4)
            break;
          ok = true;
          // UInt32 numItems = Get32(p);
          p += 4;
          size -= 4;
          break;
        }
      }
      if (!ok)
        break;
    }
  }
  return res;
}
*/

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  // COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    // case kpidComment: prop = GetComment(); break;
    case kpidPhySize: prop = (UInt64)_phySize; break;
    case kpidIsNotArcType: prop = true; break;
  }
  prop.Detach(value);
  return S_OK;
  // COM_TRY_END
}

HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *callback)
{
  const UInt32 kHeaderSize = 13;
  Byte header[kHeaderSize];
  RINOK(ReadStream_FALSE(stream, header, kHeaderSize))
  if (header[0] != 'F' ||
      header[1] != 'L' ||
      header[2] != 'V' ||
      header[3] != 1 ||
      (header[4] & 0xFA) != 0)
    return S_FALSE;
  UInt64 offset = Get32(header + 5);
  if (offset != 9 || Get32(header + 9) != 0)
    return S_FALSE;
  offset = kHeaderSize;
 
  CInBuffer inBuf;
  if (!inBuf.Create(1 << 15))
    return E_OUTOFMEMORY;
  inBuf.SetStream(stream);

  CObjectVector<CItem> items;
  int lasts[kNumTypes];
  unsigned i;
  for (i = 0; i < kNumTypes; i++)
    lasts[i] = -1;

  _phySize = offset;
  for (;;)
  {
    Byte buf[kTagHeaderSize];
    CItem item;
    if (inBuf.ReadBytes(buf, kTagHeaderSize) != kTagHeaderSize)
      break;
    item.Type = buf[0];
    UInt32 size = Get24(buf + 1);
    if (size < 1)
      break;
    // item.Time = Get24(buf + 4);
    // item.Time |= (UInt32)buf[7] << 24;
    if (Get24(buf + 8) != 0) // streamID
      break;

    UInt32 curSize = kTagHeaderSize + size + 4;
    item.Data.Alloc(curSize);
    memcpy(item.Data, buf, kTagHeaderSize);
    if (inBuf.ReadBytes(item.Data + kTagHeaderSize, size) != size)
      break;
    if (inBuf.ReadBytes(item.Data + kTagHeaderSize + size, 4) != 4)
      break;

    if (Get32(item.Data + kTagHeaderSize + size) != kTagHeaderSize + size)
      break;

    offset += curSize;
    
    // printf("\noffset = %6X type = %2d time = %6d size = %6d", (UInt32)offset, item.Type, item.Time, item.Size);

    if (item.Type == kType_Meta)
    {
      // _metadata = item.Buf;
    }
    else
    {
      if (item.Type != kType_Audio && item.Type != kType_Video)
        break;
      if (items.Size() >= kNumChunksMax)
        return S_FALSE;
      Byte firstByte = item.Data[kTagHeaderSize];
      Byte subType, props;
      if (item.Type == kType_Audio)
      {
        subType = (Byte)(firstByte >> 4);
        props = (Byte)(firstByte & 0xF);
      }
      else
      {
        subType = (Byte)(firstByte & 0xF);
        props = (Byte)(firstByte >> 4);
      }
      int last = lasts[item.Type];
      if (last < 0)
      {
        CItem2 item2;
        item2.RefBuf = item2.BufSpec = new CReferenceBuf;
        item2.Size = curSize;
        item2.Type = item.Type;
        item2.SubType = subType;
        item2.Props = props;
        item2.NumChunks = 1;
        item2.SameSubTypes = true;
        lasts[item.Type] = (int)_items2.Add(item2);
      }
      else
      {
        CItem2 &item2 = _items2[last];
        if (subType != item2.SubType)
          item2.SameSubTypes = false;
        item2.Size += curSize;
        item2.NumChunks++;
      }
      items.Add(item);
    }
    _phySize = offset;
    if (callback && (items.Size() & 0xFF) == 0)
    {
      RINOK(callback->SetCompleted(NULL, &offset))
    }
  }
  if (items.IsEmpty())
    return S_FALSE;

  _isRaw = (_items2.Size() == 1);
  for (i = 0; i < _items2.Size(); i++)
  {
    CItem2 &item2 = _items2[i];
    CByteBuffer &itemBuf = item2.BufSpec->Buf;
    if (_isRaw)
    {
      if (!item2.SameSubTypes)
        return S_FALSE;
      itemBuf.Alloc((size_t)item2.Size - (size_t)(kTagHeaderSize + 4 + 1) * item2.NumChunks);
      item2.Size = 0;
    }
    else
    {
      itemBuf.Alloc(kHeaderSize + (size_t)item2.Size);
      memcpy(itemBuf, header, kHeaderSize);
      itemBuf[4] = item2.IsAudio() ? kFlag_Audio : kFlag_Video;
      item2.Size = kHeaderSize;
    }
  }

  for (i = 0; i < items.Size(); i++)
  {
    const CItem &item = items[i];
    CItem2 &item2 = _items2[lasts[item.Type]];
    size_t size = item.Data.Size();
    const Byte *src = item.Data;
    if (_isRaw)
    {
      src += kTagHeaderSize + 1;
      size -= (kTagHeaderSize + 4 + 1);
    }
    if (size != 0)
    {
      memcpy(item2.BufSpec->Buf + item2.Size, src, size);
      item2.Size += size;
    }
  }
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  Close();
  HRESULT res;
  try
  {
    res = Open2(inStream, callback);
    if (res == S_OK)
      _stream = inStream;
  }
  catch(...) { res = S_FALSE; }
  if (res != S_OK)
  {
    Close();
    return S_FALSE;
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _phySize = 0;
  _stream.Release();
  _items2.Clear();
  // _metadata.SetCapacity(0);
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
    totalSize += _items2[allFilesMode ? i : indices[i]].Size;
  extractCallback->SetTotal(totalSize);

  totalSize = 0;
  
  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    CMyComPtr<ISequentialOutStream> outStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    const CItem2 &item = _items2[index];
    RINOK(extractCallback->GetStream(index, &outStream, askMode))
    totalSize += item.Size;
    if (!testMode && !outStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode))
    if (outStream)
    {
      RINOK(WriteStream(outStream, item.BufSpec->Buf, item.BufSpec->Buf.Size()))
    }
    RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  CBufInStream *streamSpec = new CBufInStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  streamSpec->Init(_items2[index].BufSpec);
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

static const Byte k_Signature[] = { 'F', 'L', 'V', 1, };

REGISTER_ARC_I(
  "FLV", "flv", NULL, 0xD6,
  k_Signature,
  0,
  0,
  NULL)

}}
