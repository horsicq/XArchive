/* XArchive amalgamation of 7-Zip 26.01 -- CPP/7zip/Archive standalone handler.
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

/* ---- CPP/7zip/Archive/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

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

/* ---- CPP/7zip/Archive/Common/InStreamWithCRC.h ---- */
// InStreamWithCRC.h

#ifndef ZIP7_INC_IN_STREAM_WITH_CRC_H
#define ZIP7_INC_IN_STREAM_WITH_CRC_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_NOQIB_2(
  CSequentialInStreamWithCRC
  , ISequentialInStream
  , IStreamGetSize
)
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
  UInt32 _crc;
  bool _wasFinished;
  UInt64 _fullSize;
public:
  
  CSequentialInStreamWithCRC():
    _fullSize((UInt64)(Int64)-1)
    {}

  void SetStream(ISequentialInStream *stream) { _stream = stream; }
  void SetFullSize(UInt64 fullSize) { _fullSize = fullSize; }
  void Init()
  {
    _size = 0;
    _crc = CRC_INIT_VAL;
    _wasFinished = false;
  }
  void ReleaseStream() { _stream.Release(); }
  UInt32 GetCRC() const { return CRC_GET_DIGEST(_crc); }
  UInt64 GetSize() const { return _size; }
  bool WasFinished() const { return _wasFinished; }
};


Z7_CLASS_IMP_IInStream(
  CInStreamWithCRC
)
  CMyComPtr<IInStream> _stream;
  UInt64 _size;
  UInt32 _crc;
  // bool _wasFinished;
public:
  void SetStream(IInStream *stream) { _stream = stream; }
  void Init()
  {
    _size = 0;
    // _wasFinished = false;
    _crc = CRC_INIT_VAL;
  }
  void ReleaseStream() { _stream.Release(); }
  UInt32 GetCRC() const { return CRC_GET_DIGEST(_crc); }
  UInt64 GetSize() const { return _size; }
  // bool WasFinished() const { return _wasFinished; }
};

#endif

/* ---- CPP/7zip/Archive/Common/OutStreamWithCRC.h ---- */
// OutStreamWithCRC.h

#ifndef ZIP7_INC_OUT_STREAM_WITH_CRC_H
#define ZIP7_INC_OUT_STREAM_WITH_CRC_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithCRC
  , ISequentialOutStream
)
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
  UInt32 _crc;
  bool _calculate;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init(bool calculate = true)
  {
    _size = 0;
    _calculate = calculate;
    _crc = CRC_INIT_VAL;
  }
  void EnableCalc(bool calculate) { _calculate = calculate; }
  void InitCRC() { _crc = CRC_INIT_VAL; }
  UInt64 GetSize() const { return _size; }
  UInt32 GetCRC() const { return CRC_GET_DIGEST(_crc); }
};

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

/* ---- CPP/7zip/Compress/LzfseDecoder.h ---- */
// LzfseDecoder.h

#ifndef ZIP7_INC_LZFSE_DECODER_H
#define ZIP7_INC_LZFSE_DECODER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzfse {

Z7_CLASS_IMP_NOQIB_1(
  CDecoder
  , ICompressCoder
)
  CLzOutWindow m_OutWindowStream;
  CInBuffer m_InStream;
  CByteBuffer _literals;
  CByteBuffer _buffer;

  class CCoderReleaser
  {
    CDecoder *m_Coder;
  public:
    bool NeedFlush;
    CCoderReleaser(CDecoder *coder): m_Coder(coder), NeedFlush(true) {}
    ~CCoderReleaser()
    {
      if (NeedFlush)
        m_Coder->m_OutWindowStream.Flush();
    }
  };
  friend class CCoderReleaser;

  HRESULT GetUInt32(UInt32 &val);

  HRESULT DecodeUncompressed(UInt32 unpackSize);
  HRESULT DecodeLzvn(UInt32 unpackSize, UInt32 packSize);
  HRESULT DecodeLzfse(UInt32 unpackSize, Byte version);

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
public:
  bool LzvnMode;

  CDecoder():
    LzvnMode(false)
    {}

  // sizes are checked in Code()
  // UInt64 GetInputProcessedSize() const { return m_InStream.GetProcessedSize(); }
  // UInt64 GetOutputProcessedSize() const { return m_OutWindowStream.GetProcessedSize(); }
};

}}

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

/* ---- CPP/7zip/Archive/HfsHandler.h ---- */
// HfsHandler.h

#ifndef ZIP7_INC_HFS_HANDLER_H
#define ZIP7_INC_HFS_HANDLER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace NHfs {

static const UInt32 k_decmpfs_HeaderSize = 16;

struct CCompressHeader
{
  UInt64 UnpackSize;
  UInt32 Method;
  Byte DataPos;
  bool IsCorrect;
  bool IsSupported;
  bool IsResource;

  bool IsMethod_Compressed_Inline() const { return DataPos == k_decmpfs_HeaderSize; }
  bool IsMethod_Uncompressed_Inline() const { return DataPos == k_decmpfs_HeaderSize + 1; }
  bool IsMethod_Resource() const { return IsResource; }

  void Parse(const Byte *p, size_t size);

  void Clear()
  {
    UnpackSize = 0;
    Method = 0;
    DataPos = 0;
    IsCorrect = false;
    IsSupported = false;
    IsResource = false;
  }

  CCompressHeader() { Clear(); }

  void MethodToProp(NWindows::NCOM::CPropVariant &prop) const;
};

void MethodsMaskToProp(UInt32 methodsMask, NWindows::NCOM::CPropVariant &prop);


class CDecoder
{
  CMyComPtr2_Create<ICompressCoder, NCompress::NZlib::CDecoder> _zlibDecoder;
  CMyComPtr2_Create<ICompressCoder, NCompress::NLzfse::CDecoder> _lzfseDecoder;

  CByteBuffer _tableBuf;
  CByteBuffer _buf;

  HRESULT ExtractResourceFork_ZLIB(
      ISequentialInStream *inStream, ISequentialOutStream *realOutStream,
      UInt64 forkSize, UInt64 unpackSize,
      UInt64 progressStart, IArchiveExtractCallback *extractCallback);

  HRESULT ExtractResourceFork_LZFSE(
      ISequentialInStream *inStream, ISequentialOutStream *realOutStream,
      UInt64 forkSize, UInt64 unpackSize,
      UInt64 progressStart, IArchiveExtractCallback *extractCallback);

  HRESULT ExtractResourceFork_ZBM(
      ISequentialInStream *inStream, ISequentialOutStream *realOutStream,
      UInt64 forkSize, UInt64 unpackSize,
      UInt64 progressStart, IArchiveExtractCallback *extractCallback);

public:

  HRESULT Extract(
      ISequentialInStream *inStreamFork, ISequentialOutStream *realOutStream,
      UInt64 forkSize,
      const CCompressHeader &compressHeader,
      const CByteBuffer *data,
      UInt64 progressStart, IArchiveExtractCallback *extractCallback,
      int &opRes);

  CDecoder(bool IsAdlerOptional);
};

}}

#endif

/* ---- CPP/Common/DynamicBuffer.h ---- */
// Common/DynamicBuffer.h

#ifndef ZIP7_INC_COMMON_DYNAMIC_BUFFER_H
#define ZIP7_INC_COMMON_DYNAMIC_BUFFER_H

#include <string.h>

// amalgamation: header emitted in prologue

template <class T> class CDynamicBuffer
{
  T *_items;
  size_t _size;
  size_t _pos;

  CDynamicBuffer(const CDynamicBuffer &buffer);
  void operator=(const CDynamicBuffer &buffer);

  void Grow(size_t size)
  {
    size_t delta = _size >= 64 ? _size : 64;
    if (delta < size)
      delta = size;
    size_t newCap = _size + delta;
    if (newCap < delta)
    {
      newCap = _size + size;
      if (newCap < size)
        throw 20120116;
    }

    T *newBuffer = new T[newCap];
    if (_pos != 0)
      memcpy(newBuffer, _items, _pos * sizeof(T));
    delete []_items;
    _items = newBuffer;
    _size = newCap;
  }

public:
  CDynamicBuffer(): _items(NULL), _size(0), _pos(0) {}
  // operator T *() { return _items; }
  operator const T *() const { return _items; }
  ~CDynamicBuffer() { delete []_items; }

  void Free()
  {
    delete []_items;
    _items = NULL;
    _size = 0;
    _pos = 0;
  }

  T *GetCurPtrAndGrow(size_t addSize)
  {
    size_t rem = _size - _pos;
    if (rem < addSize)
      Grow(addSize - rem);
    T *res = _items + _pos;
    _pos += addSize;
    return res;
  }

  void AddData(const T *data, size_t size)
  {
    memcpy(GetCurPtrAndGrow(size), data, size * sizeof(T));
  }

  size_t GetPos() const { return _pos; }

  // void Empty() { _pos = 0; }
};

typedef CDynamicBuffer<Byte> CByteDynamicBuffer;

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

/* ---- CPP/7zip/Compress/BcjCoder.h ---- */
// BcjCoder.h

#ifndef ZIP7_INC_COMPRESS_BCJ_CODER_H
#define ZIP7_INC_COMPRESS_BCJ_CODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBcj {

/* CCoder in old versions used another constructor parameter CCoder(int encode).
   And some code called it as CCoder(0).
   We have changed constructor parameter type.
   So we have changed the name of class also to CCoder2. */

Z7_CLASS_IMP_COM_1(
  CCoder2
  , ICompressFilter
)
  UInt32 _pc;
  UInt32 _state;
  z7_Func_BranchConvSt _convFunc;
public:
  CCoder2(z7_Func_BranchConvSt convFunc):
      _pc(0),
      _state(Z7_BRANCH_CONV_ST_X86_STATE_INIT_VAL),
      _convFunc(convFunc)
    {}
};

}}

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

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Archive/GptHandler.cpp ================ */
// GptHandler.cpp

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

using namespace NWindows;

namespace NArchive {

namespace NMbr {
const char *GetFileSystem(ISequentialInStream *stream, UInt64 partitionSize);
}

namespace NFat {
API_FUNC_IsArc IsArc_Fat(const Byte *p, size_t size);
}

namespace NGpt {

static const unsigned k_SignatureSize = 12;
static const Byte k_Signature[k_SignatureSize] =
    { 'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T', 0, 0, 1, 0 };

static const CUInt32PCharPair g_PartitionFlags[] =
{
  { 0, "Sys" },
  { 1, "Ignore" },
  { 2, "Legacy" },
  { 60, "Win-Read-only" },
  { 62, "Win-Hidden" },
  { 63, "Win-Not-Automount" }
};

static const unsigned kNameLen = 36;

struct CPartition
{
  Byte Type[16];
  Byte Id[16];
  UInt64 FirstLba;
  UInt64 LastLba;
  UInt64 Flags;
  const char *Ext; // detected later
  Byte Name[kNameLen * 2];

  bool IsUnused() const
  {
    for (unsigned i = 0; i < 16; i++)
      if (Type[i] != 0)
        return false;
    return true;
  }

  UInt64 GetSize(unsigned sectorSizeLog) const { return (LastLba - FirstLba + 1) << sectorSizeLog; }
  UInt64 GetPos(unsigned sectorSizeLog) const { return FirstLba << sectorSizeLog; }
  UInt64 GetEnd(unsigned sectorSizeLog) const { return (LastLba + 1) << sectorSizeLog; }

  void Parse(const Byte *p)
  {
    memcpy(Type, p, 16);
    memcpy(Id, p + 16, 16);
    FirstLba = Get64(p + 32);
    LastLba = Get64(p + 40);
    Flags = Get64(p + 48);
    memcpy(Name, p + 56, kNameLen * 2);
    Ext = NULL;
  }
};


struct CPartType
{
  UInt32 Id;
  const char *Ext;
  const char *Type;
};

static const CPartType kPartTypes[] =
{
  // { 0x0, NULL, "Unused" },

  { 0x21686148, NULL, "BIOS Boot" },

  { 0xC12A7328, NULL, "EFI System" },
  { 0x024DEE41, NULL, "MBR" },
      
  { 0xE3C9E316, NULL, "Windows MSR" },
  { 0xEBD0A0A2, NULL, "Windows BDP" },
  { 0x5808C8AA, NULL, "Windows LDM Metadata" },
  { 0xAF9B60A0, NULL, "Windows LDM Data" },
  { 0xDE94BBA4, NULL, "Windows Recovery" },
  // { 0x37AFFC90, NULL, "IBM GPFS" },
  // { 0xE75CAF8F, NULL, "Windows Storage Spaces" },

  { 0x0FC63DAF, NULL, "Linux Data" },
  { 0x0657FD6D, NULL, "Linux Swap" },
  { 0x44479540, NULL, "Linux root (x86)" },
  { 0x4F68BCE3, NULL, "Linux root (x86-64)" },
  { 0x69DAD710, NULL, "Linux root (ARM)" },
  { 0xB921B045, NULL, "Linux root (ARM64)" },
  { 0x993D8D3D, NULL, "Linux root (IA-64)" },
  

  { 0x83BD6B9D, NULL, "FreeBSD Boot" },
  { 0x516E7CB4, NULL, "FreeBSD Data" },
  { 0x516E7CB5, NULL, "FreeBSD Swap" },
  { 0x516E7CB6, "ufs", "FreeBSD UFS" },
  { 0x516E7CB8, NULL, "FreeBSD Vinum" },
  { 0x516E7CB8, "zfs", "FreeBSD ZFS" },

  { 0x48465300, "hfsx", "HFS+" },
  { 0x7C3457EF, "apfs", "APFS" },
};

static int FindPartType(const Byte *guid)
{
  const UInt32 val = Get32(guid);
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(kPartTypes); i++)
    if (kPartTypes[i].Id == val)
      return (int)i;
  return -1;
}


static void RawLeGuidToString_Upper(const Byte *g, char *s)
{
  RawLeGuidToString(g, s);
  // MyStringUpper_Ascii(s);
}


Z7_class_CHandler_final: public CHandlerCont
{
  Z7_IFACE_COM7_IMP(IInArchive_Cont)

  CRecordVector<CPartition> _items;
  UInt64 _totalSize;
  unsigned _sectorSizeLog;
  Byte Guid[16];

  CByteBuffer _buffer;

  HRESULT Open2(IInStream *stream);

  virtual int GetItem_ExtractInfo(UInt32 index, UInt64 &pos, UInt64 &size) const Z7_override
  {
    const CPartition &item = _items[index];
    pos = item.GetPos(_sectorSizeLog);
    size = item.GetSize(_sectorSizeLog);
    return NExtract::NOperationResult::kOK;
  }
};


HRESULT CHandler::Open2(IInStream *stream)
{
  const unsigned kBufSize = 2 << 12;
  _buffer.Alloc(kBufSize);
  RINOK(ReadStream_FALSE(stream, _buffer, kBufSize))
  const Byte *buf = _buffer;
  if (buf[0x1FE] != 0x55 || buf[0x1FF] != 0xAA)
    return S_FALSE;
  {
    for (unsigned sectorSizeLog = 9;; sectorSizeLog += 3)
    {
      if (sectorSizeLog > 12)
        return S_FALSE;
      if (memcmp(buf + ((size_t)1 << sectorSizeLog), k_Signature, k_SignatureSize) == 0)
      {
        buf += ((size_t)1 << sectorSizeLog);
        _sectorSizeLog = sectorSizeLog;
        break;
      }
    }
  }
  const UInt32 kSectorSize = 1u << _sectorSizeLog;
  {
    // if (Get32(buf + 8) != 0x10000) return S_FALSE; // revision
    const UInt32 headerSize = Get32(buf + 12); // = 0x5C usually
    if (headerSize > kSectorSize)
      return S_FALSE;
    const UInt32 crc = Get32(buf + 0x10);
    SetUi32(_buffer + kSectorSize + 0x10, 0)
    if (CrcCalc(_buffer + kSectorSize, headerSize) != crc)
      return S_FALSE;
  }
  // UInt32 reserved = Get32(buf + 0x14);
  const UInt64 curLba = Get64(buf + 0x18);
  if (curLba != 1)
    return S_FALSE;
  const UInt64 backupLba = Get64(buf + 0x20);
  // UInt64 firstUsableLba = Get64(buf + 0x28);
  // UInt64 lastUsableLba = Get64(buf + 0x30);
  memcpy(Guid, buf + 0x38, 16);
  const UInt64 tableLba = Get64(buf + 0x48);
  if (tableLba < 2 || (tableLba >> (63 - _sectorSizeLog)) != 0)
    return S_FALSE;
  const UInt32 numEntries = Get32(buf + 0x50);
  if (numEntries > (1 << 16))
    return S_FALSE;
  const UInt32 entrySize = Get32(buf + 0x54); // = 128 usually
  if (entrySize < 128 || entrySize > (1 << 12))
    return S_FALSE;
  const UInt32 entriesCrc = Get32(buf + 0x58);
  
  const UInt32 tableSize = entrySize * numEntries;
  const UInt32 tableSizeAligned = (tableSize + kSectorSize - 1) & ~(kSectorSize - 1);
  _buffer.Alloc(tableSizeAligned);
  const UInt64 tableOffset = tableLba * kSectorSize;
  RINOK(InStream_SeekSet(stream, tableOffset))
  RINOK(ReadStream_FALSE(stream, _buffer, tableSizeAligned))
  
  if (CrcCalc(_buffer, tableSize) != entriesCrc)
    return S_FALSE;
  
  _totalSize = tableOffset + tableSizeAligned;
  
  for (UInt32 i = 0; i < numEntries; i++)
  {
    CPartition item;
    item.Parse(_buffer + i * entrySize);
    if (item.IsUnused())
      continue;
    if (item.LastLba < item.FirstLba)
      return S_FALSE;
    if ((item.LastLba >> (63 - _sectorSizeLog)) != 0)
      return S_FALSE;
    const UInt64 endPos = item.GetEnd(_sectorSizeLog);
    if (_totalSize < endPos)
      _totalSize = endPos;
    _items.Add(item);
  }

  _buffer.Free();
  {
    if ((backupLba >> (63 - _sectorSizeLog)) != 0)
      return S_FALSE;
    const UInt64 end = (backupLba + 1) * kSectorSize;
    if (_totalSize < end)
      _totalSize = end;
  }
  {
    UInt64 fileEnd;
    RINOK(InStream_GetSize_SeekToEnd(stream, fileEnd))
    
    if (_totalSize < fileEnd)
    {
      const UInt64 rem = fileEnd - _totalSize;
      const UInt64 kRemMax = 1 << 22;
      if (rem <= kRemMax)
      {
        RINOK(InStream_SeekSet(stream, _totalSize))
        bool areThereNonZeros = false;
        UInt64 numZeros = 0;
        if (ReadZeroTail(stream, areThereNonZeros, numZeros, kRemMax) == S_OK)
          if (!areThereNonZeros)
            _totalSize += numZeros;
      }
    }
  }

  return S_OK;
}


Z7_COM7F_IMF(CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */))
{
  COM_TRY_BEGIN
  Close();
  RINOK(Open2(stream))
  _stream = stream;

  FOR_VECTOR (fileIndex, _items)
  {
    CPartition &item = _items[fileIndex];
    const int typeIndex = FindPartType(item.Type);
    if (typeIndex < 0)
      continue;
    const CPartType &t = kPartTypes[(unsigned)typeIndex];
    if (t.Ext)
    {
      item.Ext = t.Ext;
      continue;
    }
    if (t.Type && IsString1PrefixedByString2_NoCase_Ascii(t.Type, "Windows"))
    {
      CMyComPtr<ISequentialInStream> inStream;
      if (
          // ((IInArchiveGetStream *)this)->
          GetStream(fileIndex, &inStream) == S_OK && inStream)
      {
        const char *fs = NMbr::GetFileSystem(inStream, item.GetSize(_sectorSizeLog));
        if (fs)
          item.Ext = fs;
      }
    }
  }

  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _sectorSizeLog = 0;
  _totalSize = 0;
  memset(Guid, 0, sizeof(Guid));
  _items.Clear();
  _stream.Release();
  return S_OK;
}

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidFileSystem,
  kpidCharacts,
  kpidOffset,
  kpidId
};

static const Byte kArcProps[] =
{
  kpidSectorSize,
  kpidId
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMainSubfile:
    {
      if (_items.Size() == 1)
        prop = (UInt32)0;
      break;
    }
    case kpidPhySize: prop = _totalSize; break;
    case kpidSectorSize: prop = (UInt32)((UInt32)1 << _sectorSizeLog); break;
    case kpidId:
    {
      char s[48];
      RawLeGuidToString_Upper(Guid, s);
      prop = s;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _items.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  const CPartition &item = _items[index];

  switch (propID)
  {
    case kpidPath:
    {
      // Windows BDP partitions can have identical names.
      // So we add the partition number at front
      UString s;
      s.Add_UInt32(index);
      {
        UString s2;
        for (unsigned i = 0; i < kNameLen; i++)
        {
          wchar_t c = (wchar_t)Get16(item.Name + i * 2);
          if (c == 0)
            break;
          s2 += c;
        }
        if (!s2.IsEmpty())
        {
          s.Add_Dot();
          s += s2;
        }
      }
      {
        s.Add_Dot();
        if (item.Ext)
        {
          AString fs (item.Ext);
          fs.MakeLower_Ascii();
          s += fs;
        }
        else
          s += "img";
      }
      prop = s;
      break;
    }
    
    case kpidSize:
    case kpidPackSize: prop = item.GetSize(_sectorSizeLog); break;
    case kpidOffset: prop = item.GetPos(_sectorSizeLog); break;

    case kpidFileSystem:
    {
      char s[48];
      const char *res;
      const int typeIndex = FindPartType(item.Type);
      if (typeIndex >= 0 && kPartTypes[(unsigned)typeIndex].Type)
        res = kPartTypes[(unsigned)typeIndex].Type;
      else
      {
        RawLeGuidToString_Upper(item.Type, s);
        res = s;
      }
      prop = res;
      break;
    }

    case kpidId:
    {
      char s[48];
      RawLeGuidToString_Upper(item.Id, s);
      prop = s;
      break;
    }

    case kpidCharacts: FLAGS64_TO_PROP(g_PartitionFlags, item.Flags, prop); break;
  }

  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

// we suppport signature only for 512-bytes sector.
REGISTER_ARC_I(
  "GPT", "gpt mbr", NULL, 0xCB,
  k_Signature,
  1 << 9,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/GzHandler.cpp ================ */
// GzHandler.cpp

// amalgamation: header emitted in prologue

// #include  <stdio.h>

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

#define Get32(p) GetUi32(p)

using namespace NWindows;

using namespace NCompress;
using namespace NDeflate;

namespace NArchive {
namespace NGz {

  static const Byte kSignature_0 = 0x1F;
  static const Byte kSignature_1 = 0x8B;
  static const Byte kSignature_2 = 8; //  NCompressionMethod::kDeflate

  // Latest versions of gzip program don't write comment field to gz archive.
  // We also don't write comment field to gz archive.

  namespace NFlags
  {
    // const Byte kIsText  = 1 << 0;
    const Byte kCrc     = 1 << 1;
    const Byte kExtra   = 1 << 2;
    const Byte kName    = 1 << 3;
    const Byte kComment = 1 << 4;
    const Byte kReserved = 0xE0;
  }
  
  namespace NExtraFlags
  {
    const Byte kMaximum = 2;
    const Byte kFastest = 4;
  }
  
  namespace NHostOS
  {
    enum EEnum
    {
      kFAT = 0,
      kAMIGA,
      kVMS,
      kUnix,
      kVM_CMS,
      kAtari,
      kHPFS,
      kMac,
      kZ_System,
      kCPM,
      kTOPS20,
      kNTFS,
      kQDOS,
      kAcorn,
      kVFAT,
      kMVS,
      kBeOS,
      kTandem,
      
      kUnknown = 255
    };
  }

static const char * const kHostOSes[] =
{
    "FAT"
  , "AMIGA"
  , "VMS"
  , "Unix"
  , "VM/CMS"
  , "Atari"
  , "HPFS"
  , "Macintosh"
  , "Z-System"
  , "CP/M"
  , "TOPS-20"
  , "NTFS"
  , "SMS/QDOS"
  , "Acorn"
  , "VFAT"
  , "MVS"
  , "BeOS"
  , "Tandem"
  , "OS/400"
  , "OS/X"
};


class CItem
{
  bool TestFlag(Byte flag) const { return (Flags & flag) != 0; }
public:
  Byte Flags;
  Byte ExtraFlags;
  Byte HostOS;
  UInt32 Time;
  UInt32 Crc;
  UInt32 Size32;

  AString Name;
  AString Comment;
  // CByteBuffer Extra;

  CItem():
    Flags(0),
    ExtraFlags(0),
    HostOS(0),
    Time(0),
    Crc(0),
    Size32(0) {}

  void Clear()
  {
    Name.Empty();
    Comment.Empty();
    // Extra.Free();
  }

  void CopyMetaPropsFrom(const CItem &a)
  {
    Flags = a.Flags;
    HostOS = a.HostOS;
    Time = a.Time;
    Name = a.Name;
    Comment = a.Comment;
    // Extra = a.Extra;
  }

  void CopyDataPropsFrom(const CItem &a)
  {
    ExtraFlags = a.ExtraFlags;
    Crc = a.Crc;
    Size32 = a.Size32;
  }
  
  // bool IsText() const { return TestFlag(NFlags::kIsText); }
  bool HeaderCrcIsPresent() const { return TestFlag(NFlags::kCrc); }
  bool ExtraFieldIsPresent() const { return TestFlag(NFlags::kExtra); }
  bool NameIsPresent() const { return TestFlag(NFlags::kName); }
  bool CommentIsPresent() const { return TestFlag(NFlags::kComment); }
  bool IsSupported() const { return (Flags & NFlags::kReserved) == 0; }

  HRESULT ReadHeader(NDecoder::CCOMCoder *stream);
  HRESULT ReadFooter1(NDecoder::CCOMCoder *stream);
  HRESULT ReadFooter2(ISequentialInStream *stream);

  HRESULT WriteHeader(ISequentialOutStream *stream);
  HRESULT WriteFooter(ISequentialOutStream *stream);
};

static HRESULT ReadBytes(NDecoder::CCOMCoder *stream, Byte *data, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
    data[i] = stream->ReadAlignedByte();
  return stream->InputEofError() ? S_FALSE : S_OK;
}

static HRESULT SkipBytes(NDecoder::CCOMCoder *stream, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
    stream->ReadAlignedByte();
  return stream->InputEofError() ? S_FALSE : S_OK;
}

static HRESULT ReadUInt16(NDecoder::CCOMCoder *stream, UInt32 &value /* , UInt32 &crc */)
{
  value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = stream->ReadAlignedByte();
    if (stream->InputEofError())
      return S_FALSE;
    // crc = CRC_UPDATE_BYTE(crc, b);
    value |= ((UInt32)(b) << (8 * i));
  }
  return S_OK;
}

static HRESULT ReadString(NDecoder::CCOMCoder *stream, AString &s, size_t limit /* , UInt32 &crc */)
{
  s.Empty();
  for (size_t i = 0; i < limit; i++)
  {
    const Byte b = stream->ReadAlignedByte();
    if (stream->InputEofError())
      return S_FALSE;
    // crc = CRC_UPDATE_BYTE(crc, b);
    if (b == 0)
      return S_OK;
    s.Add_Char((char)b);
  }
  return S_FALSE;
}

static UInt32 Is_Deflate(const Byte *p, size_t size)
{
  if (size < 1)
    return k_IsArc_Res_NEED_MORE;
  Byte b = *p;
  p++;
  size--;
  unsigned type = ((unsigned)b >> 1) & 3;
  if (type == 3)
    return k_IsArc_Res_NO;
  if (type == 0)
  {
    // Stored (uncompreessed data)
    if ((b >> 3) != 0)
      return k_IsArc_Res_NO;
    if (size < 4)
      return k_IsArc_Res_NEED_MORE;
    UInt16 r = (UInt16)~GetUi16(p + 2);
    if (GetUi16(p) != r)
      return k_IsArc_Res_NO;
  }
  else if (type == 2)
  {
    // Dynamic Huffman
    if (size < 1)
      return k_IsArc_Res_NEED_MORE;
    if ((*p & 0x1F) + 1 > 30) // numDistLevels
      return k_IsArc_Res_NO;
  }
  return k_IsArc_Res_YES;
}

static const unsigned kNameMaxLen = 1 << 12;
static const unsigned kCommentMaxLen = 1 << 16;

API_FUNC_static_IsArc IsArc_Gz(const Byte *p, size_t size)
{
  if (size < 10)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != kSignature_0 ||
      p[1] != kSignature_1 ||
      p[2] != kSignature_2)
    return k_IsArc_Res_NO;

  const Byte flags = p[3];
  if ((flags & NFlags::kReserved) != 0)
    return k_IsArc_Res_NO;

  const Byte extraFlags = p[8];
  // maybe that flag can have another values for some gz archives?
  if (extraFlags != 0 &&
      extraFlags != NExtraFlags::kMaximum &&
      extraFlags != NExtraFlags::kFastest)
    return k_IsArc_Res_NO;

  size -= 10;
  p += 10;

  if ((flags & NFlags::kExtra) != 0)
  {
    if (size < 2)
      return k_IsArc_Res_NEED_MORE;
    unsigned xlen = GetUi16(p);
    size -= 2;
    p += 2;
    while (xlen != 0)
    {
      if (xlen < 4)
        return k_IsArc_Res_NO;
      if (size < 4)
        return k_IsArc_Res_NEED_MORE;
      const unsigned len = GetUi16(p + 2);
      size -= 4;
      xlen -= 4;
      p += 4;
      if (len > xlen)
        return k_IsArc_Res_NO;
      if (len > size)
        return k_IsArc_Res_NEED_MORE;
      size -= len;
      xlen -= len;
      p += len;
    }
  }

  if ((flags & NFlags::kName) != 0)
  {
    size_t limit = kNameMaxLen;
    if (limit > size)
      limit = size;
    size_t i;
    for (i = 0; i < limit && p[i] != 0; i++);
    if (i == size)
      return k_IsArc_Res_NEED_MORE;
    if (i == limit)
      return k_IsArc_Res_NO;
    i++;
    p += i;
    size -= i;
  }

  if ((flags & NFlags::kComment) != 0)
  {
    size_t limit = kCommentMaxLen;
    if (limit > size)
      limit = size;
    size_t i;
    for (i = 0; i < limit && p[i] != 0; i++);
    if (i == size)
      return k_IsArc_Res_NEED_MORE;
    if (i == limit)
      return k_IsArc_Res_NO;
    i++;
    p += i;
    size -= i;
  }
  
  if ((flags & NFlags::kCrc) != 0)
  {
    if (size < 2)
      return k_IsArc_Res_NEED_MORE;
    p += 2;
    size -= 2;
  }

  return Is_Deflate(p, size);
}
}

HRESULT CItem::ReadHeader(NDecoder::CCOMCoder *stream)
{
  Clear();

  // Header-CRC field had another meaning in old version of gzip!
  // UInt32 crc = CRC_INIT_VAL;
  Byte buf[10];

  RINOK(ReadBytes(stream, buf, 10))
  
  if (buf[0] != kSignature_0 ||
      buf[1] != kSignature_1 ||
      buf[2] != kSignature_2)
    return S_FALSE;

  Flags = buf[3];
  if (!IsSupported())
    return S_FALSE;

  Time = Get32(buf + 4);
  ExtraFlags = buf[8];
  HostOS = buf[9];

  // crc = CrcUpdate(crc, buf, 10);
  
  if (ExtraFieldIsPresent())
  {
    UInt32 xlen;
    RINOK(ReadUInt16(stream, xlen /* , crc */))
    RINOK(SkipBytes(stream, xlen))
    // Extra.SetCapacity(xlen);
    // RINOK(ReadStream_FALSE(stream, Extra, xlen));
    // crc = CrcUpdate(crc, Extra, xlen);
  }
  if (NameIsPresent())
    RINOK(ReadString(stream, Name, kNameMaxLen /* , crc */))
  if (CommentIsPresent())
    RINOK(ReadString(stream, Comment, kCommentMaxLen /* , crc */))

  if (HeaderCrcIsPresent())
  {
    UInt32 headerCRC;
    // UInt32 dummy = 0;
    RINOK(ReadUInt16(stream, headerCRC /* , dummy */))
    /*
    if ((UInt16)CRC_GET_DIGEST(crc) != headerCRC)
      return S_FALSE;
    */
  }
  return stream->InputEofError() ? S_FALSE : S_OK;
}

HRESULT CItem::ReadFooter1(NDecoder::CCOMCoder *stream)
{
  Byte buf[8];
  RINOK(ReadBytes(stream, buf, 8))
  Crc = Get32(buf);
  Size32 = Get32(buf + 4);
  return stream->InputEofError() ? S_FALSE : S_OK;
}

HRESULT CItem::ReadFooter2(ISequentialInStream *stream)
{
  Byte buf[8];
  RINOK(ReadStream_FALSE(stream, buf, 8))
  Crc = Get32(buf);
  Size32 = Get32(buf + 4);
  return S_OK;
}

HRESULT CItem::WriteHeader(ISequentialOutStream *stream)
{
  Byte buf[10];
  buf[0] = kSignature_0;
  buf[1] = kSignature_1;
  buf[2] = kSignature_2;
  buf[3] = (Byte)(Flags & NFlags::kName);
  // buf[3] |= NFlags::kCrc;
  SetUi32(buf + 4, Time)
  buf[8] = ExtraFlags;
  buf[9] = HostOS;
  RINOK(WriteStream(stream, buf, 10))
  // crc = CrcUpdate(CRC_INIT_VAL, buf, 10);
  if (NameIsPresent())
  {
    // crc = CrcUpdate(crc, (const char *)Name, Name.Len() + 1);
    RINOK(WriteStream(stream, (const char *)Name, Name.Len() + 1))
  }
  // SetUi16(buf, (UInt16)CRC_GET_DIGEST(crc));
  // RINOK(WriteStream(stream, buf, 2));
  return S_OK;
}

HRESULT CItem::WriteFooter(ISequentialOutStream *stream)
{
  Byte buf[8];
  SetUi32(buf, Crc)
  SetUi32(buf + 4, Size32)
  return WriteStream(stream, buf, 8);
}

Z7_CLASS_IMP_CHandler_IInArchive_3(
  IArchiveOpenSeq,
  IOutArchive,
  ISetProperties
)
  CItem _item;

  bool _isArc;
  bool _needSeekToStart;
  bool _dataAfterEnd;
  bool _needMoreInput;

  bool _packSize_Defined;
  bool _unpackSize_Defined;
  bool _numStreams_Defined;

  UInt64 _packSize;
  UInt64 _unpackSize; // real unpack size (NOT from footer)
  UInt64 _numStreams;
  UInt64 _headerSize; // only start header (without footer)
  
  CMyComPtr<IInStream> _stream;
  CMyComPtr2<ICompressCoder, NDecoder::CCOMCoder> _decoder;

  CSingleMethodProps _props;
  CHandlerTimeOptions _timeOptions;

public:
  CHandler():
      _isArc(false)
      {}
  
  void CreateDecoder()
  {
    _decoder.Create_if_Empty();
  }
};

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidPackSize,
  kpidMTime,
  kpidHostOS,
  kpidCRC
  // kpidComment
};

static const Byte kArcProps[] =
{
  kpidHeadersSize,
  kpidNumStreams
};


IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: if (_packSize_Defined) prop = _packSize; break;
    case kpidUnpackSize: if (_unpackSize_Defined) prop = _unpackSize; break;
    case kpidNumStreams: if (_numStreams_Defined) prop = _numStreams; break;
    case kpidHeadersSize: if (_headerSize != 0) prop = _headerSize; break;
    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_needMoreInput) v |= kpv_ErrorFlags_UnexpectedEnd;
      if (_dataAfterEnd) v |= kpv_ErrorFlags_DataAfterEnd;
      prop = v;
      break;
    }
    case kpidName:
      if (_item.NameIsPresent())
      {
        UString s = MultiByteToUnicodeString(_item.Name, CP_ACP);
        s += ".gz";
        prop = s;
      }
      break;
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

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPath:
      if (_item.NameIsPresent())
        prop = MultiByteToUnicodeString(_item.Name, CP_ACP);
      break;
    // case kpidComment: if (_item.CommentIsPresent()) prop = MultiByteToUnicodeString(_item.Comment, CP_ACP); break;
    case kpidMTime:
      // gzip specification: MTIME = 0 means no time stamp is available.
      if (_item.Time != 0)
        PropVariant_SetFrom_UnixTime(prop, _item.Time);
      break;
    case kpidTimeType:
      if (_item.Time != 0)
        prop = (UInt32)NFileTimeType::kUnix;
      break;
    case kpidSize:
    {
      if (_unpackSize_Defined)
        prop = _unpackSize;
      else if (_stream)
        prop = (UInt64)_item.Size32;
      break;
    }
    case kpidPackSize:
    {
      if (_packSize_Defined || _stream)
        prop = _packSize;
      break;
    }
    case kpidHostOS: TYPE_TO_PROP(kHostOSes, _item.HostOS, prop); break;
    case kpidCRC: if (_stream) prop = _item.Crc; break;
    default: break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
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
    UInt64 files = 0;
    UInt64 value = Offset + *inSize;
    return Callback->SetCompleted(&files, &value);
  }
  return S_OK;
}

/*
*/

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *))
{
  COM_TRY_BEGIN
  RINOK(OpenSeq(stream))
  _isArc = false;
  UInt64 endPos;
  RINOK(stream->Seek(-8, STREAM_SEEK_END, &endPos))
  _packSize = endPos + 8;
  RINOK(_item.ReadFooter2(stream))
  _stream = stream;
  _isArc = true;
  _needSeekToStart = true;
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  COM_TRY_BEGIN
  try
  {
    Close();
    CreateDecoder();
    _decoder->SetInStream(stream);
    _decoder->InitInStream(true);
    RINOK(_item.ReadHeader(_decoder.ClsPtr()))
    if (_decoder->InputEofError())
      return S_FALSE;
    _headerSize = _decoder->GetInputProcessedSize();
    _isArc = true;
    return S_OK;
  }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _isArc = false;
  _needSeekToStart = false;
  _dataAfterEnd = false;
  _needMoreInput = false;

  _packSize_Defined = false;
  _unpackSize_Defined = false;
  _numStreams_Defined = false;

  _packSize = 0;
  _headerSize = 0;
  
  _stream.Release();
  if (_decoder)
    _decoder->ReleaseInStream();
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

  if (_packSize_Defined)
    RINOK(extractCallback->SetTotal(_packSize))
  // UInt64 currentTotalPacked = 0;
  // RINOK(extractCallback->SetCompleted(&currentTotalPacked));
  Int32 retResult;
 {
  CMyComPtr<ISequentialOutStream> realOutStream;
  const Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode))
  if (!testMode && !realOutStream)
    return S_OK;

  RINOK(extractCallback->PrepareOperation(askMode))

  CreateDecoder();

  CMyComPtr2_Create<ISequentialOutStream, COutStreamWithCRC> outStream;
  outStream->SetStream(realOutStream);
  outStream->Init();
  // realOutStream.Release();

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, true);

  bool needReadFirstItem = _needSeekToStart;
  
  if (_needSeekToStart)
  {
    if (!_stream)
      return E_FAIL;
    RINOK(InStream_SeekToBegin(_stream))
    _decoder->InitInStream(true);
    // printf("\nSeek");
  }
  else
    _needSeekToStart = true;

  bool firstItem = true;

  UInt64 packSize = _decoder->GetInputProcessedSize();
  // printf("\npackSize = %d", (unsigned)packSize);

  UInt64 unpackedSize = 0;
  UInt64 numStreams = 0;

  bool crcError = false;

  HRESULT result = S_OK;

  try {
  
  for (;;)
  {
    lps->InSize = packSize;
    lps->OutSize = unpackedSize;

    RINOK(lps->SetCur())

    CItem item;
    
    if (!firstItem || needReadFirstItem)
    {
      result = item.ReadHeader(_decoder.ClsPtr());

      if (result != S_OK && result != S_FALSE)
        return result;
    
      if (_decoder->InputEofError())
        result = S_FALSE;

      if (result != S_OK && firstItem)
      {
        _isArc = false;
        break;
      }

      if (packSize == _decoder->GetStreamSize())
      {
        result = S_OK;
        break;
      }

      if (result != S_OK)
      {
        _dataAfterEnd = true;
        break;
      }
    }
    
    numStreams++;
    firstItem = false;

    const UInt64 startOffset = outStream->GetSize();
    outStream->InitCRC();

    result = _decoder->CodeResume(outStream, NULL, lps);

    packSize = _decoder->GetInputProcessedSize();
    unpackedSize = outStream->GetSize();

    if (result != S_OK && result != S_FALSE)
      return result;

    if (_decoder->InputEofError())
    {
      packSize = _decoder->GetStreamSize();
      _needMoreInput = true;
      result = S_FALSE;
    }

    if (result != S_OK)
      break;

    _decoder->AlignToByte();
    
    result = item.ReadFooter1(_decoder.ClsPtr());

    packSize = _decoder->GetInputProcessedSize();

    if (result != S_OK && result != S_FALSE)
      return result;

    if (result != S_OK)
    {
      if (_decoder->InputEofError())
      {
        _needMoreInput = true;
        result = S_FALSE;
      }
      break;
    }

    if (item.Crc != outStream->GetCRC() ||
        item.Size32 != (UInt32)(unpackedSize - startOffset))
    {
      crcError = true;
      result = S_FALSE;
      break;
    }

    // break; // we can use break, if we need only first stream
  }

  } catch(const CInBufferException &e) { return e.ErrorCode; }

  if (!firstItem)
  {
    _packSize = packSize;
    _unpackSize = unpackedSize;
    _numStreams = numStreams;

    _packSize_Defined = true;
    _unpackSize_Defined = true;
    _numStreams_Defined = true;
  }

  // outStream.Release();

  retResult = NExtract::NOperationResult::kDataError;

  if (!_isArc)
    retResult = NExtract::NOperationResult::kIsNotArc;
  else if (_needMoreInput)
    retResult = NExtract::NOperationResult::kUnexpectedEnd;
  else if (crcError)
    retResult = NExtract::NOperationResult::kCRCError;
  else if (_dataAfterEnd)
    retResult = NExtract::NOperationResult::kDataAfterEnd;
  else if (result == S_FALSE)
    retResult = NExtract::NOperationResult::kDataError;
  else if (result == S_OK)
    retResult = NExtract::NOperationResult::kOK;
  else
    return result;
 }

  return extractCallback->SetOperationResult(retResult);
  COM_TRY_END
}

static const Byte kHostOS =
  #ifdef _WIN32
  NHostOS::kFAT;
  #else
  NHostOS::kUnix;
  #endif


/*
static HRESULT ReportItemProp(IArchiveUpdateCallbackArcProp *reportArcProp, PROPID propID, const PROPVARIANT *value)
{
  return reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, 0, propID, value);
}

static HRESULT ReportArcProp(IArchiveUpdateCallbackArcProp *reportArcProp, PROPID propID, const PROPVARIANT *value)
{
  return reportArcProp->ReportProp(NEventIndexType::kArcProp, 0, propID, value);
}

static HRESULT ReportArcProps(IArchiveUpdateCallbackArcProp *reportArcProp,
    const CItem &item,
    bool needTime,
    bool needCrc,
    const UInt64 *unpackSize)
{
  NCOM::CPropVariant timeProp;
  NCOM::CPropVariant sizeProp;
  if (needTime)
  {
    FILETIME ft;
    NTime::UnixTimeToFileTime(item.Time, ft);
    timeProp.SetAsTimeFrom_FT_Prec(ft, k_PropVar_TimePrec_Unix);
  }
  if (unpackSize)
  {
    sizeProp = *unpackSize;
    RINOK(ReportItemProp(reportArcProp, kpidSize, &sizeProp));
  }
  if (needCrc)
  {
    NCOM::CPropVariant prop;
    prop = item.Crc;
    RINOK(ReportItemProp(reportArcProp, kpidCRC, &prop));
  }
  {
    RINOK(ReportItemProp(reportArcProp, kpidMTime, &timeProp));
  }
 
  RINOK(reportArcProp->ReportFinished(NEventIndexType::kOutArcIndex, 0, NArchive::NUpdate::NOperationResult::kOK));

  if (unpackSize)
  {
    RINOK(ReportArcProp(reportArcProp, kpidSize, &sizeProp));
  }
  {
    RINOK(ReportArcProp(reportArcProp, kpidComboMTime, &timeProp));
  }
  return S_OK;
}
*/

static HRESULT UpdateArchive(
    ISequentialOutStream *outStream,
    UInt64 unpackSize,
    CItem &item,
    const CSingleMethodProps &props,
    const CHandlerTimeOptions &timeOptions,
    IArchiveUpdateCallback *updateCallback
    // , IArchiveUpdateCallbackArcProp *reportArcProp
    )
{
  UInt64 unpackSizeReal;
  {
  CMyComPtr<ISequentialInStream> fileInStream;

  RINOK(updateCallback->GetStream(0, &fileInStream))

  if (!fileInStream)
    return S_FALSE;

  {
    Z7_DECL_CMyComPtr_QI_FROM(
        IStreamGetProps,
        getProps, fileInStream)
    if (getProps)
    {
      FILETIME mTime;
      UInt64 size;
      if (getProps->GetProps(&size, NULL, NULL, &mTime, NULL) == S_OK)
      {
        unpackSize = size;
        if (timeOptions.Write_MTime.Val)
          NTime::FileTime_To_UnixTime(mTime, item.Time);
      }
    }
  }

  UInt64 complexity = 0;
  RINOK(updateCallback->SetTotal(unpackSize))
  RINOK(updateCallback->SetCompleted(&complexity))

  CMyComPtr2_Create<ISequentialInStream, CSequentialInStreamWithCRC> crcStream;
  crcStream->SetStream(fileInStream);
  crcStream->Init();

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(updateCallback, true);
  
  item.ExtraFlags = props.GetLevel() >= 7 ?
      NExtraFlags::kMaximum :
      NExtraFlags::kFastest;

  item.HostOS = kHostOS;

  RINOK(item.WriteHeader(outStream))

  CMyComPtr2_Create<ICompressCoder, NEncoder::CCOMCoder> deflateEncoder;

  RINOK(props.SetCoderProps(deflateEncoder.ClsPtr(), NULL))
  RINOK(deflateEncoder.Interface()->Code(crcStream, outStream, NULL, NULL, lps))

  item.Crc = crcStream->GetCRC();
  unpackSizeReal = crcStream->GetSize();
  item.Size32 = (UInt32)unpackSizeReal;
  RINOK(item.WriteFooter(outStream))
  }
  /*
  if (reportArcProp)
  {
    RINOK(ReportArcProps(reportArcProp,
        item,
        props._Write_MTime, // item.Time != 0,
        true, // writeCrc
        &unpackSizeReal));
  }
  */
  return updateCallback->SetOperationResult(NUpdate::NOperationResult::kOK);
}


Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *timeType))
{
  /*
    if (_item.Time != 0)
    {
      we set NFileTimeType::kUnix in precision,
      and we return NFileTimeType::kUnix in kpidTimeType
      so GetFileTimeType() value is not used in any version of 7-zip.
    }
    else // (_item.Time == 0)
    {
      kpidMTime and kpidTimeType are not defined
      before 22.00 : GetFileTimeType() value is used in GetUpdatePairInfoList();
             22.00 : GetFileTimeType() value is not used
    }
  */

  UInt32 t;
  t = NFileTimeType::kUnix;
  if (_isArc ? (_item.Time == 0) : !_timeOptions.Write_MTime.Val)
  {
    t = GET_FileTimeType_NotDefined_for_GetFileTimeType;
    // t = k_PropVar_TimePrec_1ns; // failed in 7-Zip 21
    // t = (UInt32)(Int32)NFileTimeType::kNotDefined; // failed in 7-Zip 21
  }
  *timeType = t;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback))
{
  COM_TRY_BEGIN

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

  // Z7_DECL_CMyComPtr_QI_FROM(IArchiveUpdateCallbackArcProp, reportArcProp, updateCallback)

  CItem newItem;
  
  if (!IntToBool(newProps))
  {
    newItem.CopyMetaPropsFrom(_item);
  }
  else
  {
    newItem.HostOS = kHostOS;
    if (_timeOptions.Write_MTime.Val)
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidMTime, &prop))
      if (prop.vt == VT_FILETIME)
        NTime::FileTime_To_UnixTime(prop.filetime, newItem.Time);
      else if (prop.vt == VT_EMPTY)
        newItem.Time = 0;
      else
        return E_INVALIDARG;
    }
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidPath, &prop))
      if (prop.vt == VT_BSTR)
      {
        UString name = prop.bstrVal;
        int slashPos = name.ReverseFind_PathSepar();
        if (slashPos >= 0)
          name.DeleteFrontal((unsigned)(slashPos + 1));
        newItem.Name = UnicodeStringToMultiByte(name, CP_ACP);
        if (!newItem.Name.IsEmpty())
          newItem.Flags |= NFlags::kName;
      }
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }
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
    return UpdateArchive(outStream, size, newItem, _props, _timeOptions, updateCallback);
  }

  if (indexInArchive != 0)
    return E_INVALIDARG;

  if (!_stream)
    return E_NOTIMPL;

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(updateCallback, true);

  Z7_DECL_CMyComPtr_QI_FROM(
      IArchiveUpdateCallbackFile,
      opCallback, updateCallback)
  if (opCallback)
  {
    RINOK(opCallback->ReportOperation(
        NEventIndexType::kInArcIndex, 0,
        NUpdateNotifyOp::kReplicate))
  }

  newItem.CopyDataPropsFrom(_item);

  UInt64 offset = 0;
  if (IntToBool(newProps))
  {
    newItem.WriteHeader(outStream);
    offset += _headerSize;
  }
  RINOK(InStream_SeekSet(_stream, offset))

  /*
  if (reportArcProp)
    ReportArcProps(reportArcProp, newItem,
        _props._Write_MTime,
        false, // writeCrc
        NULL); // unpacksize
  */

  return NCompress::CopyStream(_stream, outStream, lps);

  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  _timeOptions.Init();
  _props.Init();

  for (UInt32 i = 0; i < numProps; i++)
  {
    UString name = names[i];
    name.MakeLower_Ascii();
    if (name.IsEmpty())
      return E_INVALIDARG;
    const PROPVARIANT &value = values[i];
    {
      bool processed = false;
      RINOK(_timeOptions.Parse(name, value, processed))
      if (processed)
      {
        if (_timeOptions.Write_CTime.Val ||
            _timeOptions.Write_ATime.Val)
          return E_INVALIDARG;
        if (   _timeOptions.Prec != (UInt32)(Int32)-1
            && _timeOptions.Prec != k_PropVar_TimePrec_0
            && _timeOptions.Prec != k_PropVar_TimePrec_Unix
            && _timeOptions.Prec != k_PropVar_TimePrec_HighPrec
            && _timeOptions.Prec != k_PropVar_TimePrec_Base)
          return E_INVALIDARG;
        continue;
      }
    }
    RINOK(_props.SetProperty(name, value))
  }
  return S_OK;
}

static const Byte k_Signature[] = { kSignature_0, kSignature_1, kSignature_2 };

REGISTER_ARC_IO(
  "gzip", "gz gzip tgz tpz apk", "* * .tar .tar .tar", 0xEF,
  k_Signature, 0,
    NArcInfoFlags::kKeepName
  | NArcInfoFlags::kMTime
  | NArcInfoFlags::kMTime_Default
  , TIME_PREC_TO_ARC_FLAGS_MASK (NFileTimeType::kUnix)
  | TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT (NFileTimeType::kUnix)
  , IsArc_Gz)

}}

/* ================ unit: CPP/7zip/Archive/HandlerCont.cpp ================ */
// HandlerCont.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {

namespace NExt {
API_FUNC_IsArc IsArc_Ext(const Byte *p, size_t size);
}

Z7_COM7F_IMF(CHandlerCont::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
  {
    RINOK(GetNumberOfItems(&numItems))
  }
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    UInt64 pos, size;
    GetItem_ExtractInfo(allFilesMode ? i : indices[i], pos, size);
    totalSize += size;
  }
  RINOK(extractCallback->SetTotal(totalSize))

  totalSize = 0;
  
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> streamSpec;
  streamSpec->SetStream(_stream);
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

  for (i = 0;; i++)
  {
    lps->InSize = totalSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;

    CMyComPtr<ISequentialOutStream> outStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    
    RINOK(extractCallback->GetStream(index, &outStream, askMode))

    UInt64 pos, size;
    int opRes = GetItem_ExtractInfo(index, pos, size);
    totalSize += size;
    if (!testMode && !outStream)
      continue;
    
    RINOK(extractCallback->PrepareOperation(askMode))

    if (opRes == NExtract::NOperationResult::kOK)
    {
      RINOK(InStream_SeekSet(_stream, pos))
      streamSpec->Init(size);
    
      RINOK(copyCoder.Interface()->Code(streamSpec, outStream, NULL, NULL, lps))
      
      opRes = NExtract::NOperationResult::kDataError;
      if (copyCoder->TotalSize == size)
        opRes = NExtract::NOperationResult::kOK;
      else if (copyCoder->TotalSize < size)
        opRes = NExtract::NOperationResult::kUnexpectedEnd;
    }
    
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandlerCont::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  UInt64 pos, size;
  if (GetItem_ExtractInfo(index, pos, size) != NExtract::NOperationResult::kOK)
    return S_FALSE;
  return CreateLimitedInStream(_stream, pos, size, stream);
  COM_TRY_END
}



CHandlerImg::CHandlerImg()
{
  Clear_HandlerImg_Vars();
}

Z7_COM7F_IMF(CHandlerImg::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += _size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
  {
    if (newPosition)
      *newPosition = _virtPos;
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  }
  _virtPos = (UInt64)offset;
  if (newPosition)
    *newPosition = (UInt64)offset;
  return S_OK;
}

static const Byte k_GDP_Signature[] =
    { 'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T', 0, 0, 1, 0 };
// static const Byte k_Ext_Signature[] = { 0x53, 0xEF };
// static const unsigned k_Ext_Signature_offset = 0x438;

static const char *GetImgExt(ISequentialInStream *stream)
{
  // const size_t kHeaderSize_for_Ext = (1 << 11); // for ext
  const size_t kHeaderSize = 2 << 12; // for 4 KB sector GPT
  Byte buf[kHeaderSize];
  size_t processed = kHeaderSize;
  if (ReadStream(stream, buf, &processed) == S_OK)
  {
    if (processed >= kHeaderSize)
    if (buf[0x1FE] == 0x55 && buf[0x1FF] == 0xAA)
    {
      for (unsigned k = (1 << 9); k <= (1u << 12); k <<= 3)
        if (memcmp(buf + k, k_GDP_Signature, sizeof(k_GDP_Signature)) == 0)
          return "gpt";
      return "mbr";
    }
    if (NExt::IsArc_Ext(buf, processed) == k_IsArc_Res_YES)
      return "ext";
  }
  return NULL;
}

void CHandlerImg::CloseAtError()
{
  Stream.Release();
}

void CHandlerImg::Clear_HandlerImg_Vars()
{
  _imgExt = NULL;
  _size = 0;
  ClearStreamVars();
  Reset_VirtPos();
  Reset_PosInArc();
}

Z7_COM7F_IMF(CHandlerImg::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * openCallback))
{
  COM_TRY_BEGIN
  {
    Close();
    HRESULT res;
    try
    {
      res = Open2(stream, openCallback);
      if (res == S_OK)
      {
        CMyComPtr<ISequentialInStream> inStream;
        const HRESULT res2 = GetStream(0, &inStream);
        if (res2 == S_OK && inStream)
          _imgExt = GetImgExt(inStream);
        // _imgExt = GetImgExt(this); // for debug
        /*  we reset (_virtPos) to support cases, if some code will
            call Read() from Handler object instead of GetStream() object. */
        Reset_VirtPos();
        // optional: we reset (_posInArc). if real seek position of stream will be changed in external code
        Reset_PosInArc();
        // optional: here we could also reset seek positions in parent streams..
        return S_OK;
      }
    }
    catch(...)
    {
      CloseAtError();
      throw;
    }
    CloseAtError();
    return res;
  }
  COM_TRY_END
}

Z7_COM7F_IMF(CHandlerImg::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = 1;
  return S_OK;
}


Z7_CLASS_IMP_NOQIB_1(
  CHandlerImgProgress
  , ICompressProgressInfo
)
public:
  CHandlerImg &Handler;
  CMyComPtr<ICompressProgressInfo> _ratioProgress;

  CHandlerImgProgress(CHandlerImg &handler) : Handler(handler) {}
};


Z7_COM7F_IMF(CHandlerImgProgress::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  UInt64 inSize2;
  if (Handler.Get_PackSizeProcessed(inSize2))
    inSize = &inSize2;
  return _ratioProgress->SetRatioInfo(inSize, outSize);
}
  

Z7_COM7F_IMF(CHandlerImg::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  RINOK(extractCallback->SetTotal(_size))
  CMyComPtr<ISequentialOutStream> outStream;
  const Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &outStream, askMode))
  if (!testMode && !outStream)
    return S_OK;
  RINOK(extractCallback->PrepareOperation(askMode))

  int opRes = NExtract::NOperationResult::kDataError;
  
  ClearStreamVars();
  
  CMyComPtr<ISequentialInStream> inStream;
  HRESULT hres = GetStream(0, &inStream);
  if (hres == S_FALSE)
    hres = E_NOTIMPL;

  if (hres == S_OK && inStream)
  {
    CLocalProgress *lps = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> progress = lps;
    lps->Init(extractCallback, false);
    
    if (Init_PackSizeProcessed())
    {
      CHandlerImgProgress *imgProgressSpec = new CHandlerImgProgress(*this);
      CMyComPtr<ICompressProgressInfo> imgProgress = imgProgressSpec;
      imgProgressSpec->_ratioProgress = progress;
      progress.Release();
      progress = imgProgress;
    }

    CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

    hres = copyCoder.Interface()->Code(inStream, outStream, NULL, &_size, progress);
    if (hres == S_OK)
    {
      if (copyCoder->TotalSize == _size)
        opRes = NExtract::NOperationResult::kOK;
      
      if (_stream_unavailData)
        opRes = NExtract::NOperationResult::kUnavailable;
      else if (_stream_unsupportedMethod)
        opRes = NExtract::NOperationResult::kUnsupportedMethod;
      else if (_stream_dataError)
        opRes = NExtract::NOperationResult::kDataError;
      else if (copyCoder->TotalSize < _size)
        opRes = NExtract::NOperationResult::kUnexpectedEnd;
    }
  }

  inStream.Release();
  outStream.Release();
  
  if (hres != S_OK)
  {
    if (hres == S_FALSE)
      opRes = NExtract::NOperationResult::kDataError;
    else if (hres == E_NOTIMPL)
      opRes = NExtract::NOperationResult::kUnsupportedMethod;
    else
      return hres;
  }
 
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}

}

/* ================ unit: CPP/7zip/Archive/HfsHandler.cpp ================ */
// HfsHandler.cpp

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

/* if HFS_SHOW_ALT_STREAMS is defined, the handler will show attribute files
   and resource forks. In most cases it looks useless. So we disable it. */
 
#define HFS_SHOW_ALT_STREAMS

#define Get16(p) GetBe16(p)
#define Get32(p) GetBe32(p)
#define Get64(p) GetBe64(p)

#define Get16a(p) GetBe16a(p)
#define Get32a(p) GetBe32a(p)

namespace NArchive {
namespace NHfs {

static const char * const kResFileName = "rsrc"; // "com.apple.ResourceFork";

struct CExtent
{
  UInt32 Pos;
  UInt32 NumBlocks;
};

struct CIdExtents
{
  UInt32 ID;
  UInt32 StartBlock;
  CRecordVector<CExtent> Extents;
};

struct CFork
{
  UInt64 Size;
  UInt32 NumBlocks;
  // UInt32 ClumpSize;
  CRecordVector<CExtent> Extents;

  CFork(): Size(0), NumBlocks(0) {}

  void Parse(const Byte *p);

  bool IsEmpty() const { return Size == 0 && NumBlocks == 0 && Extents.Size() == 0; }

  UInt32 Calc_NumBlocks_from_Extents() const;
  bool Check_NumBlocks() const;

  bool Check_Size_with_NumBlocks(unsigned blockSizeLog) const
  {
    return Size <= ((UInt64)NumBlocks << blockSizeLog);
  }

  bool IsOk(unsigned blockSizeLog) const
  {
    // we don't check cases with extra (empty) blocks in last extent
    return Check_NumBlocks() && Check_Size_with_NumBlocks(blockSizeLog);
  }

  bool Upgrade(const CObjectVector<CIdExtents> &items, UInt32 id);
  bool UpgradeAndTest(const CObjectVector<CIdExtents> &items, UInt32 id, unsigned blockSizeLog)
  {
    if (!Upgrade(items, id))
      return false;
    return IsOk(blockSizeLog);
  }
};

static const unsigned kNumFixedExtents = 8;
static const unsigned kForkRecSize = 16 + kNumFixedExtents * 8;


void CFork::Parse(const Byte *p)
{
  Extents.Clear();
  Size = Get64(p);
  // ClumpSize = Get32(p + 8);
  NumBlocks = Get32(p + 12);
  p += 16;
  for (unsigned i = 0; i < kNumFixedExtents; i++, p += 8)
  {
    CExtent e;
    e.Pos = Get32(p);
    e.NumBlocks = Get32(p + 4);
    if (e.NumBlocks != 0)
      Extents.Add(e);
  }
}

UInt32 CFork::Calc_NumBlocks_from_Extents() const
{
  UInt32 num = 0;
  FOR_VECTOR (i, Extents)
    num += Extents[i].NumBlocks;
  return num;
}

bool CFork::Check_NumBlocks() const
{
  UInt32 num = NumBlocks;
  FOR_VECTOR (i, Extents)
  {
    const UInt32 cur = Extents[i].NumBlocks;
    if (num < cur)
      return false;
    num -= cur;
  }
  return num == 0;
}

struct CIdIndexPair
{
  UInt32 ID;
  unsigned Index;

  int Compare(const CIdIndexPair &a) const;
};

#define RINOZ(x) { const int _t_ = (x); if (_t_ != 0) return _t_; }

int CIdIndexPair::Compare(const CIdIndexPair &a) const
{
  RINOZ(MyCompare(ID, a.ID))
  return MyCompare(Index, a.Index);
}

static int FindItemIndex(const CRecordVector<CIdIndexPair> &items, UInt32 id)
{
  unsigned left = 0, right = items.Size();
  while (left != right)
  {
    const unsigned mid = (left + right) / 2;
    const UInt32 midVal = items[mid].ID;
    if (id == midVal)
      return (int)items[mid].Index;
    if (id < midVal)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

static int Find_in_IdExtents(const CObjectVector<CIdExtents> &items, UInt32 id)
{
  unsigned left = 0, right = items.Size();
  while (left != right)
  {
    const unsigned mid = (left + right) / 2;
    const UInt32 midVal = items[mid].ID;
    if (id == midVal)
      return (int)mid;
    if (id < midVal)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

bool CFork::Upgrade(const CObjectVector<CIdExtents> &items, UInt32 id)
{
  const int index = Find_in_IdExtents(items, id);
  if (index < 0)
    return true;
  const CIdExtents &item = items[index];
  if (Calc_NumBlocks_from_Extents() != item.StartBlock)
    return false;
  Extents += item.Extents;
  return true;
}


struct CVolHeader
{
  unsigned BlockSizeLog;
  UInt32 NumFiles;
  UInt32 NumFolders;
  UInt32 NumBlocks;
  UInt32 NumFreeBlocks;

  bool Is_Hsfx_ver5;
  // UInt32 Attr;
  // UInt32 LastMountedVersion;
  // UInt32 JournalInfoBlock;

  UInt32 CTime;
  UInt32 MTime;
  // UInt32 BackupTime;
  // UInt32 CheckedTime;
  
  // UInt32 WriteCount;
  // UInt32 FinderInfo[8];
  // UInt64 VolID;

  UInt64 GetPhySize() const { return (UInt64)NumBlocks << BlockSizeLog; }
  UInt64 GetFreeSize() const { return (UInt64)NumFreeBlocks << BlockSizeLog; }
  bool IsHfsX() const { return Is_Hsfx_ver5; }
};

inline void HfsTimeToFileTime(UInt32 hfsTime, FILETIME &ft)
{
  UInt64 v = ((UInt64)3600 * 24 * (365 * 303 + 24 * 3) + hfsTime) * 10000000;
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}

enum ERecordType
{
  RECORD_TYPE_FOLDER = 1,
  RECORD_TYPE_FILE,
  RECORD_TYPE_FOLDER_THREAD,
  RECORD_TYPE_FILE_THREAD
};



// static const UInt32 kMethod_1_NO_COMPRESSION = 1; // in xattr
static const UInt32 kMethod_ZLIB_ATTR = 3;
static const UInt32 kMethod_ZLIB_RSRC = 4;
// static const UInt32 kMethod_DEDUP = 5; // de-dup within the generation store
// macos 10.10
static const UInt32 kMethod_LZVN_ATTR = 7;
static const UInt32 kMethod_LZVN_RSRC = 8;
static const UInt32 kMethod_COPY_ATTR = 9;
static const UInt32 kMethod_COPY_RSRC = 10;
// macos 10.11
// static const UInt32 kMethod_LZFSE_ATTR = 11;
static const UInt32 kMethod_LZFSE_RSRC = 12;

// static const UInt32 kMethod_ZBM_RSRC = 14;

static const char * const g_Methods[] =
{
    NULL
  , NULL
  , NULL
  , "ZLIB-attr"
  , "ZLIB-rsrc"
  , NULL
  , NULL
  , "LZVN-attr"
  , "LZVN-rsrc"
  , "COPY-attr"
  , "COPY-rsrc"
  , "LZFSE-attr"
  , "LZFSE-rsrc"
  , NULL
  , "ZBM-rsrc"
};


static const Byte k_COPY_Uncompressed_Marker = 0xcc;
static const Byte k_LZVN_Uncompressed_Marker = 6;

void CCompressHeader::Parse(const Byte *p, size_t dataSize)
{
  Clear();

  if (dataSize < k_decmpfs_HeaderSize)
    return;
  if (GetUi32(p) != 0x636D7066) // magic == "fpmc"
    return;
  Method = GetUi32(p + 4);
  UnpackSize = GetUi64(p + 8);
  dataSize -= k_decmpfs_HeaderSize;
  IsCorrect = true;
  
  if (   Method == kMethod_ZLIB_RSRC
      || Method == kMethod_COPY_RSRC
      || Method == kMethod_LZVN_RSRC
      || Method == kMethod_LZFSE_RSRC
      // || Method == kMethod_ZBM_RSRC // for debug
      )
  {
    IsResource = true;
    if (dataSize == 0)
      IsSupported = (
          Method != kMethod_LZFSE_RSRC &&
          Method != kMethod_COPY_RSRC);
    return;
  }
  
  if (   Method == kMethod_ZLIB_ATTR
      || Method == kMethod_COPY_ATTR
      || Method == kMethod_LZVN_ATTR
      // || Method == kMethod_LZFSE_ATTR
    )
  {
    if (dataSize == 0)
      return;
    const Byte b = p[k_decmpfs_HeaderSize];
    if (   (Method == kMethod_ZLIB_ATTR && (b & 0xf) == 0xf)
        || (Method == kMethod_COPY_ATTR && b == k_COPY_Uncompressed_Marker)
        || (Method == kMethod_LZVN_ATTR && b == k_LZVN_Uncompressed_Marker))
    {
      dataSize--;
      // if (UnpackSize > dataSize)
      if (UnpackSize != dataSize)
        return;
      DataPos = k_decmpfs_HeaderSize + 1;
      IsSupported = true;
    }
    else
    {
      if (Method != kMethod_COPY_ATTR)
        IsSupported = true;
      DataPos = k_decmpfs_HeaderSize;
    }
  }
}


void CCompressHeader::MethodToProp(NWindows::NCOM::CPropVariant &prop) const
{
  if (!IsCorrect)
    return;
  const UInt32 method = Method;
  const char *p = NULL;
  if (method < Z7_ARRAY_SIZE(g_Methods))
    p = g_Methods[method];
  AString s;
  if (p)
    s = p;
  else
    s.Add_UInt32(method);
  // if (!IsSupported) s += "-unsuported";
  prop = s;
}

void MethodsMaskToProp(UInt32 methodsMask, NWindows::NCOM::CPropVariant &prop)
{
  FLAGS_TO_PROP(g_Methods, methodsMask, prop);
}


struct CItem
{
  UString Name;
  
  UInt32 ParentID;

  UInt16 Type;
  UInt16 FileMode;
  // UInt16 Flags;
  // UInt32 Valence;
  UInt32 ID;
  UInt32 CTime;
  UInt32 MTime;
  UInt32 AttrMTime;
  UInt32 ATime;
  // UInt32 BackupDate;

  /*
  UInt32 OwnerID;
  UInt32 GroupID;
  Byte AdminFlags;
  Byte OwnerFlags;
  union
  {
    UInt32  iNodeNum;
    UInt32  LinkCount;
    UInt32  RawDevice;
  } special;

  UInt32 FileType;
  UInt32 FileCreator;
  UInt16 FinderFlags;
  UInt16 Point[2];
  */

  CFork DataFork;
  CFork ResourceFork;

  // for compressed attribute (decmpfs)
  int decmpfs_AttrIndex;
  CCompressHeader CompressHeader;

  CItem():
      decmpfs_AttrIndex(-1)
      {}
  bool IsDir() const { return Type == RECORD_TYPE_FOLDER; }
  // const CFork *GetFork(bool isResource) const { return (isResource ? &ResourceFork: &DataFork); }
};


struct CAttr
{
  UInt32 ID;
  bool Fork_defined;
  
  // UInt32 Size;    // for (Fork_defined == false) case
  // size_t DataPos; // for (Fork_defined == false) case
  CByteBuffer Data;

  CFork Fork;

  UString Name;

  UInt64 GetSize() const
  {
    if (Fork_defined)
      return Fork.Size;
    return Data.Size();
  }

  CAttr():
      Fork_defined(false)
      // Size(0),
      // DataPos(0),
      {}
};


static const int kAttrIndex_Item     = -1;
static const int kAttrIndex_Resource = -2;

struct CRef
{
  unsigned ItemIndex;
  int AttrIndex;
  int Parent;

  CRef(): AttrIndex(kAttrIndex_Item), Parent(-1) {}
  bool IsResource() const { return AttrIndex == kAttrIndex_Resource; }
  bool IsAltStream() const { return AttrIndex != kAttrIndex_Item; }
  bool IsItem() const { return AttrIndex == kAttrIndex_Item; }
};


class CDatabase
{
  HRESULT ReadFile(const CFork &fork, CByteBuffer &buf, IInStream *inStream);
  HRESULT LoadExtentFile(const CFork &fork, IInStream *inStream, CObjectVector<CIdExtents> *overflowExtentsArray);
  HRESULT LoadAttrs(const CFork &fork, IInStream *inStream, IArchiveOpenCallback *progress);
  HRESULT LoadCatalog(const CFork &fork, const CObjectVector<CIdExtents> *overflowExtentsArray, IInStream *inStream, IArchiveOpenCallback *progress);
  bool Parse_decmpgfs(unsigned attrIndex, CItem &item, bool &skip);
public:
  CRecordVector<CRef> Refs;
  CObjectVector<CItem> Items;
  CObjectVector<CAttr> Attrs;

  // CByteBuffer AttrBuf;

  CVolHeader Header;
  bool HeadersError;
  bool UnsupportedFeature;
  bool ThereAreAltStreams;
  // bool CaseSensetive;
  UInt32 MethodsMask;
  UString ResFileName;

  UInt64 SpecOffset;
  // UInt64 PhySize;
  UInt64 PhySize2;
  UInt64 ArcFileSize;

  void Clear()
  {
    SpecOffset = 0;
    // PhySize = 0;
    PhySize2 = 0;
    ArcFileSize = 0;
    MethodsMask = 0;
    HeadersError = false;
    UnsupportedFeature = false;
    ThereAreAltStreams = false;
    // CaseSensetive = false;

    Refs.Clear();
    Items.Clear();
    Attrs.Clear();
    // AttrBuf.Free();
  }

  UInt64 Get_UnpackSize_of_Ref(const CRef &ref) const
  {
    if (ref.AttrIndex >= 0)
      return Attrs[ref.AttrIndex].GetSize();
    const CItem &item = Items[ref.ItemIndex];
    if (ref.IsResource())
      return item.ResourceFork.Size;
    if (item.IsDir())
      return 0;
    else if (item.CompressHeader.IsCorrect)
      return item.CompressHeader.UnpackSize;
    return item.DataFork.Size;
  }

  void GetItemPath(unsigned index, NWindows::NCOM::CPropVariant &path) const;
  HRESULT Open2(IInStream *inStream, IArchiveOpenCallback *progress);
};

enum
{
  kHfsID_Root                  = 1,
  kHfsID_RootFolder            = 2,
  kHfsID_ExtentsFile           = 3,
  kHfsID_CatalogFile           = 4,
  kHfsID_BadBlockFile          = 5,
  kHfsID_AllocationFile        = 6,
  kHfsID_StartupFile           = 7,
  kHfsID_AttributesFile        = 8,
  kHfsID_RepairCatalogFile     = 14,
  kHfsID_BogusExtentFile       = 15,
  kHfsID_FirstUserCatalogNode  = 16
};

void CDatabase::GetItemPath(unsigned index, NWindows::NCOM::CPropVariant &path) const
{
  unsigned len = 0;
  const unsigned kNumLevelsMax = (1 << 10);
  unsigned cur = index;
  unsigned i;
  
  for (i = 0; i < kNumLevelsMax; i++)
  {
    const CRef &ref = Refs[cur];
    const UString *s;

    if (ref.IsResource())
      s = &ResFileName;
    else if (ref.AttrIndex >= 0)
      s = &Attrs[ref.AttrIndex].Name;
    else
      s = &Items[ref.ItemIndex].Name;

    len += s->Len();
    len++;
    cur = (unsigned)ref.Parent;
    if (ref.Parent < 0)
      break;
  }
  
  len--;
  wchar_t *p = path.AllocBstr(len);
  p[len] = 0;
  cur = index;
  
  for (;;)
  {
    const CRef &ref = Refs[cur];
    const UString *s;
    wchar_t delimChar = L':';

    if (ref.IsResource())
      s = &ResFileName;
    else if (ref.AttrIndex >= 0)
      s = &Attrs[ref.AttrIndex].Name;
    else
    {
      delimChar = WCHAR_PATH_SEPARATOR;
      s = &Items[ref.ItemIndex].Name;
    }

    unsigned curLen = s->Len();
    len -= curLen;
    
    const wchar_t *src = (const wchar_t *)*s;
    wchar_t *dest = p + len;
    for (unsigned j = 0; j < curLen; j++)
    {
      wchar_t c = src[j];
      // 18.06
      if (c == CHAR_PATH_SEPARATOR || c == '/')
        c = '_';
      dest[j] = c;
    }
    
    if (len == 0)
      break;
    p[--len] = delimChar;
    cur = (unsigned)ref.Parent;
  }
}

// Actually we read all blocks. It can be larger than fork.Size

HRESULT CDatabase::ReadFile(const CFork &fork, CByteBuffer &buf, IInStream *inStream)
{
  if (fork.NumBlocks >= Header.NumBlocks)
    return S_FALSE;
  if (((ArcFileSize - SpecOffset) >> Header.BlockSizeLog) + 1 < fork.NumBlocks)
    return S_FALSE;

  const size_t totalSize = (size_t)fork.NumBlocks << Header.BlockSizeLog;
  if ((totalSize >> Header.BlockSizeLog) != fork.NumBlocks)
    return S_FALSE;
  buf.Alloc(totalSize);
  UInt32 curBlock = 0;
  FOR_VECTOR (i, fork.Extents)
  {
    if (curBlock >= fork.NumBlocks)
      return S_FALSE;
    const CExtent &e = fork.Extents[i];
    if (e.Pos > Header.NumBlocks ||
        e.NumBlocks > fork.NumBlocks - curBlock ||
        e.NumBlocks > Header.NumBlocks - e.Pos)
      return S_FALSE;
    RINOK(InStream_SeekSet(inStream, SpecOffset + ((UInt64)e.Pos << Header.BlockSizeLog)))
    RINOK(ReadStream_FALSE(inStream,
        (Byte *)buf + ((size_t)curBlock << Header.BlockSizeLog),
        (size_t)e.NumBlocks << Header.BlockSizeLog))
    curBlock += e.NumBlocks;
  }
  return S_OK;
}

static const unsigned kNodeDescriptor_Size = 14;

struct CNodeDescriptor
{
  UInt32 fLink;
  // UInt32 bLink;
  Byte Kind;
  // Byte Height;
  unsigned NumRecords;

  bool Parse(const Byte *p, unsigned nodeSizeLog);
};


bool CNodeDescriptor::Parse(const Byte *p, unsigned nodeSizeLog)
{
  fLink = Get32(p);
  // bLink = Get32(p + 4);
  Kind = p[8];
  // Height = p[9];
  NumRecords = Get16(p + 10);

  const size_t nodeSize = (size_t)1 << nodeSizeLog;
  if (kNodeDescriptor_Size + ((UInt32)NumRecords + 1) * 2 > nodeSize)
    return false;
  const size_t limit = nodeSize - ((UInt32)NumRecords + 1) * 2;

  p += nodeSize - 2;

  for (unsigned i = 0; i < NumRecords; i++)
  {
    const UInt32 offs = Get16(p);
    p -= 2;
    const UInt32 offsNext = Get16(p);
    if (offs < kNodeDescriptor_Size
        || offs >= offsNext
        || offsNext > limit)
      return false;
  }
  return true;
}

struct CHeaderRec
{
  // UInt16 TreeDepth;
  // UInt32 RootNode;
  // UInt32 LeafRecords;
  UInt32 FirstLeafNode;
  // UInt32 LastLeafNode;
  unsigned NodeSizeLog;
  // UInt16 MaxKeyLength;
  UInt32 TotalNodes;
  // UInt32 FreeNodes;
  // UInt16 Reserved1;
  // UInt32 ClumpSize;
  // Byte BtreeType;
  // Byte KeyCompareType;
  // UInt32 Attributes;
  // UInt32 Reserved3[16];
  
  HRESULT Parse2(const CByteBuffer &buf);
};

HRESULT CHeaderRec::Parse2(const CByteBuffer &buf)
{
  if (buf.Size() < kNodeDescriptor_Size + 0x2A + 16 * 4)
    return S_FALSE;
  const Byte * p = (const Byte *)buf + kNodeDescriptor_Size;
  // TreeDepth = Get16(p);
  // RootNode = Get32(p + 2);
  // LeafRecords = Get32(p + 6);
  FirstLeafNode = Get32(p + 0xA);
  // LastLeafNode = Get32(p + 0xE);
  const UInt32 nodeSize = Get16(p + 0x12);

  unsigned i;
  for (i = 9; ((UInt32)1 << i) != nodeSize; i++)
    if (i == 16)
      return S_FALSE;
  NodeSizeLog = i;

  // MaxKeyLength = Get16(p + 0x14);
  TotalNodes = Get32(p + 0x16);
  // FreeNodes = Get32(p + 0x1A);
  // Reserved1 = Get16(p + 0x1E);
  // ClumpSize = Get32(p + 0x20);
  // BtreeType = p[0x24];
  // KeyCompareType = p[0x25];
  // Attributes = Get32(p + 0x26);
  /*
  for (int i = 0; i < 16; i++)
    Reserved3[i] = Get32(p + 0x2A + i * 4);
  */

  if ((buf.Size() >> NodeSizeLog) < TotalNodes)
    return S_FALSE;

  return S_OK;
}


static const Byte kNodeType_Leaf   = 0xFF;
// static const Byte kNodeType_Index  = 0;
// static const Byte kNodeType_Header = 1;
// static const Byte kNodeType_Mode   = 2;

static const Byte kExtentForkType_Data = 0;
static const Byte kExtentForkType_Resource = 0xFF;

/* It loads data extents from Extents Overflow File
   Most dmg installers are not fragmented. So there are no extents in Overflow File. */

HRESULT CDatabase::LoadExtentFile(const CFork &fork, IInStream *inStream, CObjectVector<CIdExtents> *overflowExtentsArray)
{
  if (fork.NumBlocks == 0)
    return S_OK;
  CByteBuffer buf;
  RINOK(ReadFile(fork, buf, inStream))
  const Byte *p = (const Byte *)buf;

  // CNodeDescriptor nodeDesc;
  // nodeDesc.Parse(p);
  CHeaderRec hr;
  RINOK(hr.Parse2(buf))

  UInt32 node = hr.FirstLeafNode;
  if (node == 0)
    return S_OK;
  if (hr.TotalNodes == 0)
    return S_FALSE;

  CByteArr usedBuf(hr.TotalNodes);
  memset(usedBuf, 0, hr.TotalNodes);

  while (node != 0)
  {
    if (node >= hr.TotalNodes || usedBuf[node] != 0)
      return S_FALSE;
    usedBuf[node] = 1;

    const size_t nodeOffset = (size_t)node << hr.NodeSizeLog;
    CNodeDescriptor desc;
    if (!desc.Parse(p + nodeOffset, hr.NodeSizeLog))
      return S_FALSE;
    if (desc.Kind != kNodeType_Leaf)
      return S_FALSE;
    
    UInt32 endBlock = 0;

    for (unsigned i = 0; i < desc.NumRecords; i++)
    {
      const UInt32 nodeSize = ((UInt32)1 << hr.NodeSizeLog);
      const Byte *r = p + nodeOffset + nodeSize - i * 2;
      const UInt32 offs = Get16(r - 2);
      UInt32 recSize = Get16(r - 4) - offs;
      const unsigned kKeyLen = 10;

      if (recSize != 2 + kKeyLen + kNumFixedExtents * 8)
        return S_FALSE;

      r = p + nodeOffset + offs;
      if (Get16(r) != kKeyLen)
        return S_FALSE;
      
      const Byte forkType = r[2];
      unsigned forkTypeIndex;
      if (forkType == kExtentForkType_Data)
        forkTypeIndex = 0;
      else if (forkType == kExtentForkType_Resource)
        forkTypeIndex = 1;
      else
        continue;
      CObjectVector<CIdExtents> &overflowExtents = overflowExtentsArray[forkTypeIndex];

      const UInt32 id = Get32(r + 4);
      const UInt32 startBlock = Get32(r + 8);
      r += 2 + kKeyLen;
      
      bool needNew = true;
      
      if (overflowExtents.Size() != 0)
      {
        CIdExtents &e = overflowExtents.Back();
        if (e.ID == id)
        {
          if (endBlock != startBlock)
            return S_FALSE;
          needNew = false;
        }
      }
      
      if (needNew)
      {
        CIdExtents &e = overflowExtents.AddNew();
        e.ID = id;
        e.StartBlock = startBlock;
        endBlock = startBlock;
      }
      
      CIdExtents &e = overflowExtents.Back();
      
      for (unsigned k = 0; k < kNumFixedExtents; k++, r += 8)
      {
        CExtent ee;
        ee.Pos = Get32(r);
        ee.NumBlocks = Get32(r + 4);
        if (ee.NumBlocks != 0)
        {
          e.Extents.Add(ee);
          endBlock += ee.NumBlocks;
        }
      }
    }

    node = desc.fLink;
  }
  return S_OK;
}

static void LoadName(const Byte *data, unsigned len, UString &dest)
{
  wchar_t *p = dest.GetBuf(len);
  unsigned i;
  for (i = 0; i < len; i++)
  {
    const wchar_t c = Get16(data + i * 2);
    if (c == 0)
      break;
    p[i] = c;
  }
  p[i] = 0;
  dest.ReleaseBuf_SetLen(i);
}

static bool IsNameEqualTo(const Byte *data, const char *name)
{
  for (unsigned i = 0;; i++)
  {
    const char c = name[i];
    if (c == 0)
      return true;
    if (Get16(data + i * 2) != (Byte)c)
      return false;
  }
}

static const UInt32 kAttrRecordType_Inline = 0x10;
static const UInt32 kAttrRecordType_Fork = 0x20;
// static const UInt32 kAttrRecordType_Extents = 0x30;

HRESULT CDatabase::LoadAttrs(const CFork &fork, IInStream *inStream, IArchiveOpenCallback *progress)
{
  if (fork.NumBlocks == 0)
    return S_OK;

  CByteBuffer AttrBuf;
  RINOK(ReadFile(fork, AttrBuf, inStream))
  const Byte *p = (const Byte *)AttrBuf;

  // CNodeDescriptor nodeDesc;
  // nodeDesc.Parse(p);
  CHeaderRec hr;
  RINOK(hr.Parse2(AttrBuf))
  
  // CaseSensetive = (Header.IsHfsX() && hr.KeyCompareType == 0xBC);

  UInt32 node = hr.FirstLeafNode;
  if (node == 0)
    return S_OK;
  if (hr.TotalNodes == 0)
    return S_FALSE;

  CByteArr usedBuf(hr.TotalNodes);
  memset(usedBuf, 0, hr.TotalNodes);

  CFork resFork;

  while (node != 0)
  {
    if (node >= hr.TotalNodes || usedBuf[node] != 0)
      return S_FALSE;
    usedBuf[node] = 1;
    
    const size_t nodeOffset = (size_t)node << hr.NodeSizeLog;
    CNodeDescriptor desc;
    if (!desc.Parse(p + nodeOffset, hr.NodeSizeLog))
      return S_FALSE;
    if (desc.Kind != kNodeType_Leaf)
      return S_FALSE;
    
    for (unsigned i = 0; i < desc.NumRecords; i++)
    {
      const UInt32 nodeSize = ((UInt32)1 << hr.NodeSizeLog);
      const Byte *r = p + nodeOffset + nodeSize - i * 2;
      const UInt32 offs = Get16(r - 2);
      UInt32 recSize = Get16(r - 4) - offs;
      const unsigned kHeadSize = 14;
      if (recSize < kHeadSize)
        return S_FALSE;

      r = p + nodeOffset + offs;
      const UInt32 keyLen = Get16(r);

      // UInt16 pad = Get16(r + 2);
      const UInt32 fileID = Get32(r + 4);
      const unsigned startBlock = Get32(r + 8);
      if (startBlock != 0)
      {
        // that case is still unsupported
        UnsupportedFeature = true;
        continue;
      }
      const unsigned nameLen = Get16(r + 12);

      if (keyLen + 2 > recSize ||
          keyLen != kHeadSize - 2 + nameLen * 2)
        return S_FALSE;
      r += kHeadSize;
      recSize -= kHeadSize;

      const Byte *name = r;
      r += nameLen * 2;
      recSize -= nameLen * 2;

      if (recSize < 4)
        return S_FALSE;

      const UInt32 recordType = Get32(r);

      if (progress && (Attrs.Size() & 0xFFF) == 0)
      {
        const UInt64 numFiles = 0;
        RINOK(progress->SetCompleted(&numFiles, NULL))
      }

      if (Attrs.Size() >= ((UInt32)1 << 31))
        return S_FALSE;

      CAttr &attr = Attrs.AddNew();
      attr.ID = fileID;
      LoadName(name, nameLen, attr.Name);

      if (recordType == kAttrRecordType_Fork)
      {
        // 22.00 : some hfs files contain it;
        /* spec: If the attribute has more than 8 extents, there will be additional
            records (of type kAttrRecordType_Extents) for this attribute. */
        if (recSize != 8 + kForkRecSize)
          return S_FALSE;
        if (Get32(r + 4) != 0) // reserved
          return S_FALSE;
        attr.Fork.Parse(r + 8);
        attr.Fork_defined = true;
        continue;
      }
      else if (recordType != kAttrRecordType_Inline)
      {
        UnsupportedFeature = true;
        continue;
      }

      const unsigned kRecordHeaderSize = 16;
      if (recSize < kRecordHeaderSize)
        return S_FALSE;
      if (Get32(r + 4) != 0 || Get32(r + 8) != 0) // reserved
        return S_FALSE;
      const UInt32 dataSize = Get32(r + 12);
      
      r += kRecordHeaderSize;
      recSize -= kRecordHeaderSize;
      
      if (recSize < dataSize)
        return S_FALSE;

      attr.Data.CopyFrom(r, dataSize);
      // attr.DataPos = nodeOffset + offs + 2 + keyLen + kRecordHeaderSize;
      // attr.Size = dataSize;
    }

    node = desc.fLink;
  }
  return S_OK;
}


bool CDatabase::Parse_decmpgfs(unsigned attrIndex, CItem &item, bool &skip)
{
  const CAttr &attr = Attrs[attrIndex];
  skip = false;
  if (item.CompressHeader.IsCorrect || !item.DataFork.IsEmpty())
    return false;

  item.CompressHeader.Parse(attr.Data, attr.Data.Size());

  if (item.CompressHeader.IsCorrect)
  {
    item.decmpfs_AttrIndex = (int)attrIndex;
    skip = true;
    if (item.CompressHeader.Method < sizeof(MethodsMask) * 8)
      MethodsMask |= ((UInt32)1 << item.CompressHeader.Method);
  }

  return true;
}


HRESULT CDatabase::LoadCatalog(const CFork &fork, const CObjectVector<CIdExtents> *overflowExtentsArray, IInStream *inStream, IArchiveOpenCallback *progress)
{
  CByteBuffer buf;
  RINOK(ReadFile(fork, buf, inStream))
  const Byte *p = (const Byte *)buf;

  // CNodeDescriptor nodeDesc;
  // nodeDesc.Parse(p);
  CHeaderRec hr;
  RINOK(hr.Parse2(buf))

  CRecordVector<CIdIndexPair> IdToIndexMap;

  const unsigned reserveSize = (unsigned)(Header.NumFolders + 1 + Header.NumFiles);

  const unsigned kBasicRecSize = 0x58;
  const unsigned kMinRecSize = kBasicRecSize + 10;

  if ((UInt64)reserveSize * kMinRecSize < buf.Size())
  {
    Items.ClearAndReserve(reserveSize);
    Refs.ClearAndReserve(reserveSize);
    IdToIndexMap.ClearAndReserve(reserveSize);
  }
 
  // CaseSensetive = (Header.IsHfsX() && hr.KeyCompareType == 0xBC);

  CByteArr usedBuf(hr.TotalNodes);
  if (hr.TotalNodes != 0)
    memset(usedBuf, 0, hr.TotalNodes);

  CFork resFork;

  UInt32 node = hr.FirstLeafNode;
  UInt32 numFiles = 0;
  UInt32 numFolders = 0;
  
  while (node != 0)
  {
    if (node >= hr.TotalNodes || usedBuf[node] != 0)
      return S_FALSE;
    usedBuf[node] = 1;
    
    const size_t nodeOffset = (size_t)node << hr.NodeSizeLog;
    CNodeDescriptor desc;
    if (!desc.Parse(p + nodeOffset, hr.NodeSizeLog))
      return S_FALSE;
    if (desc.Kind != kNodeType_Leaf)
      return S_FALSE;
    
    for (unsigned i = 0; i < desc.NumRecords; i++)
    {
      const UInt32 nodeSize = (1 << hr.NodeSizeLog);
      const Byte *r = p + nodeOffset + nodeSize - i * 2;
      const UInt32 offs = Get16(r - 2);
      UInt32 recSize = Get16(r - 4) - offs;
      if (recSize < 6)
        return S_FALSE;

      r = p + nodeOffset + offs;
      UInt32 keyLen = Get16(r);
      UInt32 parentID = Get32(r + 2);
      if (keyLen < 6 || (keyLen & 1) != 0 || keyLen + 2 > recSize)
        return S_FALSE;
      r += 6;
      recSize -= 6;
      keyLen -= 6;
      
      unsigned nameLen = Get16(r);
      if (nameLen * 2 != (unsigned)keyLen)
        return S_FALSE;
      r += 2;
      recSize -= 2;
     
      r += nameLen * 2;
      recSize -= nameLen * 2;

      if (recSize < 2)
        return S_FALSE;
      UInt16 type = Get16(r);

      if (type != RECORD_TYPE_FOLDER &&
          type != RECORD_TYPE_FILE)
        continue;

      if (recSize < kBasicRecSize)
        return S_FALSE;

      CItem &item = Items.AddNew();
      item.ParentID = parentID;
      item.Type = type;
      // item.Flags = Get16(r + 2);
      // item.Valence = Get32(r + 4);
      item.ID = Get32(r + 8);
      {
        const Byte *name = r - (nameLen * 2);
        LoadName(name, nameLen, item.Name);
        if (item.Name.Len() <= 1)
        {
          if (item.Name.IsEmpty() && nameLen == 21)
          {
            if (GetUi32(name) == 0 &&
                GetUi32(name + 4) == 0 &&
                IsNameEqualTo(name + 8, "HFS+ Private Data"))
            {
              // it's folder for "Hard Links" files
              item.Name = "[HFS+ Private Data]";
            }
          }

          // Some dmg files have ' ' folder item.
          if (item.Name.IsEmpty() || item.Name[0] == L' ')
            item.Name = "[]";
        }
      }

      item.CTime = Get32(r + 0xC);
      item.MTime = Get32(r + 0x10);
      item.AttrMTime = Get32(r + 0x14);
      item.ATime = Get32(r + 0x18);
      // item.BackupDate = Get32(r + 0x1C);
      
      /*
      item.OwnerID = Get32(r + 0x20);
      item.GroupID = Get32(r + 0x24);
      item.AdminFlags = r[0x28];
      item.OwnerFlags = r[0x29];
      */
      item.FileMode = Get16(r + 0x2A);
      /*
      item.special.iNodeNum = Get16(r + 0x2C); // or .linkCount
      item.FileType = Get32(r + 0x30);
      item.FileCreator = Get32(r + 0x34);
      item.FinderFlags = Get16(r + 0x38);
      item.Point[0] = Get16(r + 0x3A); // v
      item.Point[1] = Get16(r + 0x3C); // h
      */

      // const refIndex = Refs.Size();
      CIdIndexPair pair;
      pair.ID = item.ID;
      pair.Index = Items.Size() - 1;
      IdToIndexMap.Add(pair);

      recSize -= kBasicRecSize;
      r += kBasicRecSize;
      if (item.IsDir())
      {
        numFolders++;
        if (recSize != 0)
          return S_FALSE;
      }
      else
      {
        numFiles++;
        if (recSize != kForkRecSize * 2)
          return S_FALSE;
        
        item.DataFork.Parse(r);

        if (!item.DataFork.UpgradeAndTest(overflowExtentsArray[0], item.ID, Header.BlockSizeLog))
          HeadersError = true;
  
        item.ResourceFork.Parse(r + kForkRecSize);
        if (!item.ResourceFork.IsEmpty())
        {
          if (!item.ResourceFork.UpgradeAndTest(overflowExtentsArray[1], item.ID, Header.BlockSizeLog))
            HeadersError = true;
          // ThereAreAltStreams = true;
        }
      }
      if (progress && (Items.Size() & 0xFFF) == 0)
      {
        const UInt64 numItems = Items.Size();
        RINOK(progress->SetCompleted(&numItems, NULL))
      }
    }
    node = desc.fLink;
  }

  if (Header.NumFiles != numFiles ||
      Header.NumFolders + 1 != numFolders)
    HeadersError = true;

  IdToIndexMap.Sort2();
  {
    for (unsigned i = 1; i < IdToIndexMap.Size(); i++)
      if (IdToIndexMap[i - 1].ID == IdToIndexMap[i].ID)
        return S_FALSE;
  }

 
  CBoolArr skipAttr(Attrs.Size());
  {
    for (unsigned i = 0; i < Attrs.Size(); i++)
      skipAttr[i] = false;
  }

  {
    FOR_VECTOR (i, Attrs)
    {
      const CAttr &attr = Attrs[i];

      const int itemIndex = FindItemIndex(IdToIndexMap, attr.ID);
      if (itemIndex < 0)
      {
        HeadersError = true;
        continue;
      }
  
      if (attr.Name.IsEqualTo("com.apple.decmpfs"))
      {
        if (!Parse_decmpgfs(i, Items[itemIndex], skipAttr[i]))
          HeadersError = true;
      }
    }
  }

  IdToIndexMap.ClearAndReserve(Items.Size());

  {
    FOR_VECTOR (i, Items)
    {
      const CItem &item = Items[i];

      CIdIndexPair pair;
      pair.ID = item.ID;
      pair.Index = Refs.Size();
      IdToIndexMap.Add(pair);

      CRef ref;
      ref.ItemIndex = i;
      Refs.Add(ref);
      
      #ifdef HFS_SHOW_ALT_STREAMS
     
      if (item.ResourceFork.IsEmpty())
        continue;
      if (item.CompressHeader.IsSupported && item.CompressHeader.IsMethod_Resource())
        continue;

      ThereAreAltStreams = true;
      ref.AttrIndex = kAttrIndex_Resource;
      ref.Parent = (int)(Refs.Size() - 1);
      Refs.Add(ref);
      
      #endif
    }
  }

  IdToIndexMap.Sort2();

  {
    FOR_VECTOR (i, Refs)
    {
      CRef &ref = Refs[i];
      if (ref.IsResource())
        continue;
      const CItem &item = Items[ref.ItemIndex];
      ref.Parent = FindItemIndex(IdToIndexMap, item.ParentID);
      if (ref.Parent >= 0)
      {
        if (!Items[Refs[ref.Parent].ItemIndex].IsDir())
        {
          ref.Parent = -1;
          HeadersError = true;
        }
      }
    }
  }

  #ifdef HFS_SHOW_ALT_STREAMS
  {
    FOR_VECTOR (i, Attrs)
    {
      if (skipAttr[i])
        continue;
      const CAttr &attr = Attrs[i];

      const int refIndex = FindItemIndex(IdToIndexMap, attr.ID);
      if (refIndex < 0)
      {
        HeadersError = true;
        continue;
      }

      ThereAreAltStreams = true;

      CRef ref;
      ref.AttrIndex = (int)i;
      ref.Parent = refIndex;
      ref.ItemIndex = Refs[refIndex].ItemIndex;
      Refs.Add(ref);
    }
  }
  #endif

  return S_OK;
}

static const unsigned kHeaderPadSize = 1 << 10;
static const unsigned kMainHeaderSize = 512;
static const unsigned kHfsHeaderSize = kHeaderPadSize + kMainHeaderSize;

static const unsigned k_Signature_LE16_HFS_BD = 'B' + ((unsigned)'D' << 8);
static const unsigned k_Signature_LE16_HPLUS  = 'H' + ((unsigned)'+' << 8);
static const UInt32   k_Signature_LE32_HFSP_VER4 = 'H' + ((UInt32)'+' << 8) + ((UInt32)4 << 24);
static const UInt32   k_Signature_LE32_HFSX_VER5 = 'H' + ((UInt32)'X' << 8) + ((UInt32)5 << 24);

API_FUNC_static_IsArc IsArc_HFS(const Byte *p, size_t size)
{
  if (size < kHfsHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  p += kHeaderPadSize;
  const UInt32 sig = GetUi32(p);
  if (sig != k_Signature_LE32_HFSP_VER4)
  if (sig != k_Signature_LE32_HFSX_VER5)
  if ((UInt16)sig != k_Signature_LE16_HFS_BD
      || GetUi16(p + 0x7c) != k_Signature_LE16_HPLUS)
    return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}

HRESULT CDatabase::Open2(IInStream *inStream, IArchiveOpenCallback *progress)
{
  Clear();
  UInt32 buf32[kHfsHeaderSize / 4];
  RINOK(ReadStream_FALSE(inStream, buf32, kHfsHeaderSize))
  const Byte *p = (const Byte *)buf32 + kHeaderPadSize;
  CVolHeader &h = Header;

  if (GetUi16a(p) == k_Signature_LE16_HFS_BD)
  {
    /*
    It's header for old HFS format.
    We don't support old HFS format, but we support
    special HFS volume that contains embedded HFS+ volume.
    HFS MDB : Master directory block
    HFS VIB : Volume information block
    some old images contain boot data with "LK" signature at start of buf32.
    */
#if 1
    // here we check first bytes of archive,
    // because start data can contain signature of some another
    // archive type that could have priority over HFS.
    const void *buf_ptr = (const void *)buf32;
    const unsigned sig = GetUi16a(buf_ptr);
    if (sig != 'L' + ((unsigned)'K' << 8))
    {
      // some old HFS (non HFS+) files have no "LK" signature,
      // but have non-zero data after 2 first bytes in start 1 KiB.
      if (sig != 0)
        return S_FALSE;
/*
      for (unsigned i = 0; i < kHeaderPadSize / 4; i++)
        if (buf32[i] != 0)
          return S_FALSE;
*/
    }
#endif
    if (GetUi16a(p + 0x7c) != k_Signature_LE16_HPLUS) // signature of embedded HFS+ volume
      return S_FALSE;
    /*
    h.CTime = Get32(p + 0x2);
    h.MTime = Get32(p + 0x6);
    
    h.NumFiles = Get32(p + 0x54);
    h.NumFolders = Get32(p + 0x58);
    
    if (h.NumFolders > ((UInt32)1 << 29) ||
        h.NumFiles > ((UInt32)1 << 30))
      return S_FALSE;
    if (progress)
    {
      UInt64 numFiles = (UInt64)h.NumFiles + h.NumFolders + 1;
      RINOK(progress->SetTotal(&numFiles, NULL))
    }
    h.NumFreeBlocks = Get16(p + 0x22);
    */
    
    // v24.09: blockSize in old HFS image can be non-power of 2.
    const UInt32 blockSize = Get32a(p + 0x14); // drAlBlkSiz
    if (blockSize == 0 || (blockSize & 0x1ff))
      return S_FALSE;
    const unsigned numBlocks = Get16a(p + 0x12); // drNmAlBlks
    // UInt16 drFreeBks = Get16a(p + 0x22); // number of unused allocation blocks
    /*
    we suppose that it has the following layout:
    {
      start data with header
      blocks[h.NumBlocks]
      end data with header (probably size_of_footer <= blockSize).
    }
    */
    // PhySize2 = ((UInt64)numBlocks + 2) * blockSize;
    const unsigned sector_of_FirstBlock = Get16a(p + 0x1c); // drAlBlSt : first allocation block in volume
    const UInt32 startBlock = Get16a(p + 0x7c + 2);
    const UInt32 blockCount = Get16a(p + 0x7c + 4);
    SpecOffset = (UInt32)sector_of_FirstBlock << 9; // it's 32-bit here
    PhySize2 = SpecOffset + (UInt64)numBlocks * blockSize;
    SpecOffset += (UInt64)startBlock * blockSize;
    // before v24.09: // SpecOffset = (UInt64)(1 + startBlock) * blockSize;
    const UInt64 phy = SpecOffset + (UInt64)blockCount * blockSize;
    if (PhySize2 < phy)
        PhySize2 = phy;
    UInt32 tail = 1 << 10; // at least 1 KiB tail (for footer MDB) is expected.
    if (tail < blockSize)
        tail = blockSize;
    RINOK(InStream_GetSize_SeekToEnd(inStream, ArcFileSize))
    if (ArcFileSize > PhySize2 &&
        ArcFileSize - PhySize2 <= tail)
    {
      // data after blocks[h.NumBlocks] must contain another copy of MDB.
      // In example where blockSize is not power of 2, we have
      //   (ArcFileSize - PhySize2) < blockSize.
      // We suppose that data after blocks[h.NumBlocks] is part of HFS archive.
      // Maybe we should scan for footer MDB data (in last 1 KiB)?
      PhySize2 = ArcFileSize;
    }
    RINOK(InStream_SeekSet(inStream, SpecOffset))
    RINOK(ReadStream_FALSE(inStream, buf32, kHfsHeaderSize))
  }

  // HFS+ / HFSX volume header (starting from offset==1024):
  {
    // v24.09: we use strict condition test for pair signature(Version):
    // H+(4), HX(5):
    const UInt32 sig = GetUi32a(p);
    // h.Version = Get16(p + 2);
    h.Is_Hsfx_ver5 = false;
    if (sig != k_Signature_LE32_HFSP_VER4)
    {
      if (sig != k_Signature_LE32_HFSX_VER5)
        return S_FALSE;
      h.Is_Hsfx_ver5 = true;
    }
  }
  {
    const UInt32 blockSize = Get32a(p + 0x28);
    unsigned i;
    for (i = 9; ((UInt32)1 << i) != blockSize; i++)
      if (i == 31)
        return S_FALSE;
    h.BlockSizeLog = i;
  }
#if 1
  // HFS Plus DOCs: The first 1024 bytes are reserved for use as boot blocks
  // v24.09: we don't check starting 1 KiB before old (HFS MDB) block ("BD" signture) .
  //     but we still check starting 1 KiB before HFS+ / HFSX volume header.
  // are there HFS+ / HFSX images with non-zero data in this reserved area?
  {
    for (unsigned i = 0; i < kHeaderPadSize / 4; i++)
      if (buf32[i] != 0)
        return S_FALSE;
  }
#endif
  // h.Attr = Get32a(p + 4);
  // h.LastMountedVersion = Get32a(p + 8);
  // h.JournalInfoBlock = Get32a(p + 0xC);
  h.CTime = Get32a(p + 0x10);
  h.MTime = Get32a(p + 0x14);
  // h.BackupTime = Get32a(p + 0x18);
  // h.CheckedTime = Get32a(p + 0x1C);
  h.NumFiles = Get32a(p + 0x20);
  h.NumFolders = Get32a(p + 0x24);
  if (h.NumFolders > ((UInt32)1 << 29) ||
      h.NumFiles > ((UInt32)1 << 30))
    return S_FALSE;

  RINOK(InStream_GetSize_SeekToEnd(inStream, ArcFileSize))
  if (progress)
  {
    const UInt64 numFiles = (UInt64)h.NumFiles + h.NumFolders + 1;
    RINOK(progress->SetTotal(&numFiles, NULL))
  }

  h.NumBlocks = Get32a(p + 0x2C);
  h.NumFreeBlocks = Get32a(p + 0x30);
  /*
  h.NextCalatlogNodeID = Get32(p + 0x40);
  h.WriteCount = Get32(p + 0x44);
  for (i = 0; i < 6; i++)
    h.FinderInfo[i] = Get32(p + 0x50 + i * 4);
  h.VolID = Get64(p + 0x68);
  */

  ResFileName = kResFileName;

  CFork extentsFork, catalogFork, attrFork;
  // allocationFork.Parse(p + 0x70 + 0x50 * 0);
  extentsFork.Parse(p + 0x70 + 0x50 * 1);
  catalogFork.Parse(p + 0x70 + 0x50 * 2);
  attrFork.Parse   (p + 0x70 + 0x50 * 3);
  // startupFork.Parse(p + 0x70 + 0x50 * 4);

  CObjectVector<CIdExtents> overflowExtents[2];
  if (!extentsFork.IsOk(Header.BlockSizeLog))
    HeadersError = true;
  else
  {
    const HRESULT res = LoadExtentFile(extentsFork, inStream, overflowExtents);
    if (res == S_FALSE)
      HeadersError = true;
    else if (res != S_OK)
      return res;
  }

  if (!catalogFork.UpgradeAndTest(overflowExtents[0], kHfsID_CatalogFile, Header.BlockSizeLog))
    return S_FALSE;
  
  if (!attrFork.UpgradeAndTest(overflowExtents[0], kHfsID_AttributesFile, Header.BlockSizeLog))
    HeadersError = true;
  else
  {
    if (attrFork.Size != 0)
      RINOK(LoadAttrs(attrFork, inStream, progress))
  }
  
  RINOK(LoadCatalog(catalogFork, overflowExtents, inStream, progress))

  // PhySize = Header.GetPhySize();
  return S_OK;
}



Z7_class_CHandler_final:
  public IInArchive,
  public IArchiveGetRawProps,
  public IInArchiveGetStream,
  public CMyUnknownImp,
  public CDatabase
{
  Z7_IFACES_IMP_UNK_3(
      IInArchive,
      IArchiveGetRawProps,
      IInArchiveGetStream)

  CMyComPtr<IInStream> _stream;
  HRESULT GetForkStream(const CFork &fork, ISequentialInStream **stream);
};

static const Byte kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidCTime,
  kpidMTime,
  kpidATime,
  kpidChangeTime,
  kpidPosixAttrib,
  /*
  kpidUserId,
  kpidGroupId,
  */
#ifdef HFS_SHOW_ALT_STREAMS
  kpidIsAltStream,
#endif
  kpidMethod
};

static const Byte kArcProps[] =
{
  kpidMethod,
  kpidCharacts,
  kpidClusterSize,
  kpidFreeSpace,
  kpidCTime,
  kpidMTime
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static void HfsTimeToProp(UInt32 hfsTime, NWindows::NCOM::CPropVariant &prop)
{
  if (hfsTime == 0)
    return;
  FILETIME ft;
  HfsTimeToFileTime(hfsTime, ft);
  prop.SetAsTimeFrom_FT_Prec(ft, k_PropVar_TimePrec_Base);
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidExtension: prop = Header.IsHfsX() ? "hfsx" : "hfs"; break;
    case kpidMethod: prop = Header.IsHfsX() ? "HFSX" : "HFS+"; break;
    case kpidCharacts: MethodsMaskToProp(MethodsMask, prop); break;
    case kpidPhySize:
    {
      UInt64 v = SpecOffset + Header.GetPhySize(); // PhySize;
      if (v < PhySize2)
        v = PhySize2;
      prop = v;
      break;
    }
    case kpidClusterSize: prop = (UInt32)1 << Header.BlockSizeLog; break;
    case kpidFreeSpace: prop = (UInt64)Header.GetFreeSize(); break;
    case kpidMTime: HfsTimeToProp(Header.MTime, prop); break;
    case kpidCTime:
    {
      if (Header.CTime != 0)
      {
        FILETIME localFt, ft;
        HfsTimeToFileTime(Header.CTime, localFt);
        if (LocalFileTimeToFileTime(&localFt, &ft))
          prop.SetAsTimeFrom_FT_Prec(ft, k_PropVar_TimePrec_Base);
      }
      break;
    }
    case kpidIsTree: prop = true; break;
    case kpidErrorFlags:
    {
      UInt32 flags = 0;
      if (HeadersError) flags |= kpv_ErrorFlags_HeadersError;
      if (UnsupportedFeature) flags |= kpv_ErrorFlags_UnsupportedFeature;
      if (flags != 0)
        prop = flags;
      break;
    }
    case kpidIsAltStream: prop = ThereAreAltStreams; break;
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

Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32 /* index */, BSTR *name, PROPID *propID))
{
  *name = NULL;
  *propID = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *parentType))
{
  const CRef &ref = Refs[index];
  *parentType = ref.IsAltStream() ?
      NParentType::kAltStream :
      NParentType::kDir;
  *parent = (UInt32)(Int32)ref.Parent;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;
  #ifdef MY_CPU_LE
  if (propID == kpidName)
  {
    const CRef &ref = Refs[index];
    const UString *s;
    if (ref.IsResource())
      s = &ResFileName;
    else if (ref.AttrIndex >= 0)
      s = &Attrs[ref.AttrIndex].Name;
    else
      s = &Items[ref.ItemIndex].Name;
    *data = (const wchar_t *)(*s);
    *dataSize = (s->Len() + 1) * (UInt32)sizeof(wchar_t);
    *propType = PROP_DATA_TYPE_wchar_t_PTR_Z_LE;
    return S_OK;
  }
  #else
  UNUSED_VAR(index)
  UNUSED_VAR(propID)
  #endif
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CRef &ref = Refs[index];
  const CItem &item = Items[ref.ItemIndex];
  switch (propID)
  {
    case kpidPath: GetItemPath(index, prop); break;
    case kpidName:
    {
      const UString *s;
      if (ref.IsResource())
        s = &ResFileName;
      else if (ref.AttrIndex >= 0)
        s = &Attrs[ref.AttrIndex].Name;
      else
        s = &item.Name;
      prop = *s;
      break;
    }
    case kpidPackSize:
      {
        UInt64 size;
        if (ref.AttrIndex >= 0)
          size = Attrs[ref.AttrIndex].GetSize();
        else if (ref.IsResource())
          size = (UInt64)item.ResourceFork.NumBlocks << Header.BlockSizeLog;
        else if (item.IsDir())
          break;
        else if (item.CompressHeader.IsCorrect)
        {
          if (item.CompressHeader.IsMethod_Resource())
            size = (UInt64)item.ResourceFork.NumBlocks << Header.BlockSizeLog;
          else if (item.decmpfs_AttrIndex >= 0)
          {
            // size = item.PackSize;
            const CAttr &attr = Attrs[item.decmpfs_AttrIndex];
            size = attr.Data.Size() - item.CompressHeader.DataPos;
          }
          else
            size = 0;
        }
        else
          size = (UInt64)item.DataFork.NumBlocks << Header.BlockSizeLog;
        prop = size;
        break;
      }
    case kpidSize:
      {
        UInt64 size;
        if (ref.AttrIndex >= 0)
          size = Attrs[ref.AttrIndex].GetSize();
        else if (ref.IsResource())
          size = item.ResourceFork.Size;
        else if (item.IsDir())
          break;
        else if (item.CompressHeader.IsCorrect)
          size = item.CompressHeader.UnpackSize;
        else
          size = item.DataFork.Size;
        prop = size;
        break;
      }
    case kpidIsDir: prop = (ref.IsItem() && item.IsDir()); break;
    case kpidIsAltStream: prop = ref.IsAltStream(); break;
    case kpidCTime: HfsTimeToProp(item.CTime, prop); break;
    case kpidMTime: HfsTimeToProp(item.MTime, prop); break;
    case kpidATime: HfsTimeToProp(item.ATime, prop); break;
    case kpidChangeTime: HfsTimeToProp(item.AttrMTime, prop); break;
    case kpidPosixAttrib: if (ref.IsItem()) prop = (UInt32)item.FileMode; break;
    /*
    case kpidUserId: prop = (UInt32)item.OwnerID; break;
    case kpidGroupId: prop = (UInt32)item.GroupID; break;
    */

    case kpidMethod:
      if (ref.IsItem())
        item.CompressHeader.MethodToProp(prop);
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  Close();
  RINOK(Open2(inStream, callback))
  _stream = inStream;
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _stream.Release();
  Clear();
  return S_OK;
}

static const UInt32 kCompressionBlockSize = 1 << 16;

CDecoder::CDecoder(bool IsAdlerOptional)
{
  /* Some new hfs files contain zlib resource fork without Adler checksum.
     We do not know how we must detect case where there is Adler
     checksum or there is no Adler checksum.
  */
  _zlibDecoder->IsAdlerOptional = IsAdlerOptional;
  _lzfseDecoder->LzvnMode = true;
}

HRESULT CDecoder::ExtractResourceFork_ZLIB(
    ISequentialInStream *inStream, ISequentialOutStream *outStream,
    UInt64 forkSize, UInt64 unpackSize,
    UInt64 progressStart, IArchiveExtractCallback *extractCallback)
{
  const unsigned kHeaderSize = 0x100 + 8;

  const size_t kBufSize = kCompressionBlockSize;
  _buf.Alloc(kBufSize + 0x10); // we need 1 additional bytes for uncompressed chunk header

  RINOK(ReadStream_FALSE(inStream, _buf, kHeaderSize))
  Byte *buf = _buf;
  const UInt32 dataPos = Get32(buf);
  const UInt32 mapPos = Get32(buf + 4);
  const UInt32 dataSize = Get32(buf + 8);
  const UInt32 mapSize = Get32(buf + 12);

  const UInt32 kResMapSize = 50;

  if (mapSize != kResMapSize
      || dataPos > mapPos
      || dataSize != mapPos - dataPos
      || mapSize > forkSize
      || mapPos != forkSize - mapSize)
    return S_FALSE;
  
  const UInt32 dataSize2 = Get32(buf + 0x100);
  if (4 + dataSize2 != dataSize
      || dataSize2 < 8
      || dataSize2 > dataSize)
    return S_FALSE;
  
  const UInt32 numBlocks = GetUi32(buf + 0x100 + 4);
  if (((dataSize2 - 4) >> 3) < numBlocks)
    return S_FALSE;
  {
    const UInt64 up = unpackSize + kCompressionBlockSize - 1;
    if (up < unpackSize || up / kCompressionBlockSize != numBlocks)
      return S_FALSE;
  }

  const UInt32 tableSize = (numBlocks << 3);

  _tableBuf.AllocAtLeast(tableSize);

  RINOK(ReadStream_FALSE(inStream, _tableBuf, tableSize))
  const Byte *tableBuf = _tableBuf;
  
  UInt32 prev = 4 + tableSize;

  UInt32 i;
  for (i = 0; i < numBlocks; i++)
  {
    const UInt32 offs = GetUi32(tableBuf + i * 8);
    const UInt32 size = GetUi32(tableBuf + i * 8 + 4);
    if (size == 0
        || prev != offs
        || offs > dataSize2
        || size > dataSize2 - offs)
      return S_FALSE;
    prev = offs + size;
  }

  if (prev != dataSize2)
    return S_FALSE;

  CMyComPtr2_Create<ISequentialInStream, CBufInStream> bufInStream;

  // bool padError = false;
  UInt64 outPos = 0;

  for (i = 0; i < numBlocks; i++)
  {
    const UInt64 rem = unpackSize - outPos;
    if (rem == 0)
      return S_FALSE;
    UInt32 blockSize = kCompressionBlockSize;
    if (rem < kCompressionBlockSize)
      blockSize = (UInt32)rem;

    const UInt32 size = GetUi32(tableBuf + i * 8 + 4);

    if (size > kCompressionBlockSize + 1)
      return S_FALSE;

    RINOK(ReadStream_FALSE(inStream, buf, size))

    if ((buf[0] & 0xF) == 0xF)
    {
      // (buf[0] = 0xff) is marker of uncompressed block in APFS
      // that code was not tested in HFS
      if (size - 1 != blockSize)
        return S_FALSE;

      if (outStream)
      {
        RINOK(WriteStream(outStream, buf + 1, blockSize))
      }
    }
    else
    {
      const UInt64 blockSize64 = blockSize;
      bufInStream->Init(buf, size);
      RINOK(_zlibDecoder.Interface()->Code(bufInStream, outStream, NULL, &blockSize64, NULL))
      if (_zlibDecoder->GetOutputProcessedSize() != blockSize)
        return S_FALSE;
      const UInt64 inSize = _zlibDecoder->GetInputProcessedSize();
      if (inSize != size)
      {
        if (inSize > size)
          return S_FALSE;
        // apfs file can contain junk (non-zeros) after data block.
        /*
        if (!padError)
        {
          const Byte *p = buf + (UInt32)inSize;
          const Byte *e = p + (size - (UInt32)inSize);
          do
          {
            if (*p != 0)
            {
              padError = true;
              break;
            }
          }
          while (++p != e);
        }
        */
      }
    }
    
    outPos += blockSize;
    if ((i & 0xFF) == 0)
    {
      const UInt64 progressPos = progressStart + outPos;
      RINOK(extractCallback->SetCompleted(&progressPos))
    }
  }

  if (outPos != unpackSize)
    return S_FALSE;

  // if (padError) return S_FALSE;

  /* We check Resource Map
     Are there HFS files with another values in Resource Map ??? */

  RINOK(ReadStream_FALSE(inStream, buf, mapSize))
  const UInt32 types = Get16(buf + 24);
  const UInt32 names = Get16(buf + 26);
  const UInt32 numTypes = Get16(buf + 28);
  if (numTypes != 0 || types != 28 || names != kResMapSize)
    return S_FALSE;
  const UInt32 resType = Get32(buf + 30);
  const UInt32 numResources = Get16(buf + 34);
  const UInt32 resListOffset = Get16(buf + 36);
  if (resType != 0x636D7066) // cmpf
    return S_FALSE;
  if (numResources != 0 || resListOffset != 10)
    return S_FALSE;

  const UInt32 entryId = Get16(buf + 38);
  const UInt32 nameOffset = Get16(buf + 40);
  // Byte attrib = buf[42];
  const UInt32 resourceOffset = Get32(buf + 42) & 0xFFFFFF;
  if (entryId != 1 || nameOffset != 0xFFFF || resourceOffset != 0)
    return S_FALSE;

  return S_OK;
}



HRESULT CDecoder::ExtractResourceFork_LZFSE(
    ISequentialInStream *inStream, ISequentialOutStream *outStream,
    UInt64 forkSize, UInt64 unpackSize,
    UInt64 progressStart, IArchiveExtractCallback *extractCallback)
{
  const UInt32 kNumBlocksMax = (UInt32)1 << 29;
  if (unpackSize >= (UInt64)kNumBlocksMax * kCompressionBlockSize)
    return S_FALSE;
  const UInt32 numBlocks = (UInt32)((unpackSize + kCompressionBlockSize - 1) / kCompressionBlockSize);
  const UInt32 numBlocks2 = numBlocks + 1;
  const UInt32 tableSize = (numBlocks2 << 2);
  if (tableSize > forkSize)
    return S_FALSE;
  _tableBuf.AllocAtLeast(tableSize);
  RINOK(ReadStream_FALSE(inStream, _tableBuf, tableSize))
  const Byte *tableBuf = _tableBuf;
  
  {
    UInt32 prev = GetUi32(tableBuf);
    if (prev != tableSize)
      return S_FALSE;
    for (UInt32 i = 1; i < numBlocks2; i++)
    {
      const UInt32 offs = GetUi32(tableBuf + i * 4);
      if (offs <= prev)
        return S_FALSE;
      prev = offs;
    }
    if (prev != forkSize)
      return S_FALSE;
  }

  const size_t kBufSize = kCompressionBlockSize;
  _buf.Alloc(kBufSize + 0x10); // we need 1 additional bytes for uncompressed chunk header

  CMyComPtr2_Create<ISequentialInStream, CBufInStream> bufInStream;

  UInt64 outPos = 0;

  for (UInt32 i = 0; i < numBlocks; i++)
  {
    const UInt64 rem = unpackSize - outPos;
    if (rem == 0)
      return S_FALSE;
    UInt32 blockSize = kCompressionBlockSize;
    if (rem < kCompressionBlockSize)
      blockSize = (UInt32)rem;

    const UInt32 size =
        GetUi32(tableBuf + i * 4 + 4) -
        GetUi32(tableBuf + i * 4);

    if (size > kCompressionBlockSize + 1)
      return S_FALSE;

    RINOK(ReadStream_FALSE(inStream, _buf, size))
    const Byte *buf = _buf;

    if (buf[0] == k_LZVN_Uncompressed_Marker)
    {
      if (size - 1 != blockSize)
        return S_FALSE;
      if (outStream)
      {
        RINOK(WriteStream(outStream, buf + 1, blockSize))
      }
    }
    else
    {
      const UInt64 blockSize64 = blockSize;
      const UInt64 packSize64 = size;
      bufInStream->Init(buf, size);
      RINOK(_lzfseDecoder.Interface()->Code(bufInStream, outStream, &packSize64, &blockSize64, NULL))
      // in/out sizes were checked in Code()
    }
    
    outPos += blockSize;
    if ((i & 0xFF) == 0)
    {
      const UInt64 progressPos = progressStart + outPos;
      RINOK(extractCallback->SetCompleted(&progressPos))
    }
  }

  return S_OK;
}


/*
static UInt32 GetUi24(const Byte *p)
{
  return p[0] + ((UInt32)p[1] << 8) + ((UInt32)p[2] << 24);
}

HRESULT CDecoder::ExtractResourceFork_ZBM(
    ISequentialInStream *inStream, ISequentialOutStream *outStream,
    UInt64 forkSize, UInt64 unpackSize,
    UInt64 progressStart, IArchiveExtractCallback *extractCallback)
{
  const UInt32 kNumBlocksMax = (UInt32)1 << 29;
  if (unpackSize >= (UInt64)kNumBlocksMax * kCompressionBlockSize)
    return S_FALSE;
  const UInt32 numBlocks = (UInt32)((unpackSize + kCompressionBlockSize - 1) / kCompressionBlockSize);
  const UInt32 numBlocks2 = numBlocks + 1;
  const UInt32 tableSize = (numBlocks2 << 2);
  if (tableSize > forkSize)
    return S_FALSE;
  _tableBuf.AllocAtLeast(tableSize);
  RINOK(ReadStream_FALSE(inStream, _tableBuf, tableSize));
  const Byte *tableBuf = _tableBuf;
  
  {
    UInt32 prev = GetUi32(tableBuf);
    if (prev != tableSize)
      return S_FALSE;
    for (UInt32 i = 1; i < numBlocks2; i++)
    {
      const UInt32 offs = GetUi32(tableBuf + i * 4);
      if (offs <= prev)
        return S_FALSE;
      prev = offs;
    }
    if (prev != forkSize)
      return S_FALSE;
  }

  const size_t kBufSize = kCompressionBlockSize;
  _buf.Alloc(kBufSize + 0x10); // we need 1 additional bytes for uncompressed chunk header

  CBufInStream *bufInStream = new CBufInStream;
  CMyComPtr<ISequentialInStream> bufInStream = bufInStream;

  UInt64 outPos = 0;

  for (UInt32 i = 0; i < numBlocks; i++)
  {
    const UInt64 rem = unpackSize - outPos;
    if (rem == 0)
      return S_FALSE;
    UInt32 blockSize = kCompressionBlockSize;
    if (rem < kCompressionBlockSize)
      blockSize = (UInt32)rem;

    const UInt32 size =
        GetUi32(tableBuf + i * 4 + 4) -
        GetUi32(tableBuf + i * 4);

    // if (size > kCompressionBlockSize + 1)
    if (size > blockSize + 1)
      return S_FALSE; // we don't expect it, because encode will use uncompressed chunk

    RINOK(ReadStream_FALSE(inStream, _buf, size));
    const Byte *buf = _buf;

    // (size != 0)
    // if (size == 0) return S_FALSE;

    if (buf[0] == 0xFF) // uncompressed marker
    {
      if (size != blockSize + 1)
        return S_FALSE;
      if (outStream)
      {
        RINOK(WriteStream(outStream, buf + 1, blockSize));
      }
    }
    else
    {
      if (size < 4)
        return S_FALSE;
      if (buf[0] != 'Z' ||
          buf[1] != 'B' ||
          buf[2] != 'M' ||
          buf[3] != 9)
        return S_FALSE;
      // for debug:
      unsigned packPos = 4;
      unsigned unpackPos = 0;
      unsigned packRem = size - packPos;
      for (;;)
      {
        if (packRem < 6)
          return S_FALSE;
        const UInt32 packSize = GetUi24(buf + packPos);
        const UInt32 chunkUnpackSize = GetUi24(buf + packPos + 3);
        if (packSize < 6)
          return S_FALSE;
        if (packSize > packRem)
          return S_FALSE;
        if (chunkUnpackSize > blockSize - unpackPos)
          return S_FALSE;
        packPos += packSize;
        packRem -= packSize;
        unpackPos += chunkUnpackSize;
        if (packSize == 6)
        {
          if (chunkUnpackSize != 0)
            return S_FALSE;
          break;
        }
        if (packSize >= chunkUnpackSize + 6)
        {
          if (packSize > chunkUnpackSize + 6)
            return S_FALSE;
          // uncompressed chunk;
        }
        else
        {
          // compressed chunk
          const Byte *t = buf + packPos - packSize + 6;
          UInt32 r = packSize - 6;
          if (r < 9)
            return S_FALSE;
          const UInt32 v0 = GetUi24(t);
          const UInt32 v1 = GetUi24(t + 3);
          const UInt32 v2 = GetUi24(t + 6);
          if (v0 > v1 || v1 > v2 || v2 > packSize)
            return S_FALSE;
          // here we need the code that will decompress ZBM chunk
        }
      }

      if (unpackPos != blockSize)
        return S_FALSE;

      UInt32 size1 = size;
      if (size1 > kCompressionBlockSize)
      {
        size1 = kCompressionBlockSize;
        // return S_FALSE;
      }
      if (outStream)
      {
        RINOK(WriteStream(outStream, buf, size1))

        const UInt32 kTempSize = 1 << 16;
        Byte temp[kTempSize];
        memset(temp, 0, kTempSize);

        for (UInt32 k = size1; k < kCompressionBlockSize; k++)
        {
          UInt32 cur = kCompressionBlockSize - k;
          if (cur > kTempSize)
            cur = kTempSize;
          RINOK(WriteStream(outStream, temp, cur))
          k += cur;
        }
      }

      // const UInt64 blockSize64 = blockSize;
      // const UInt64 packSize64 = size;
      // bufInStream->Init(buf, size);
      // RINOK(_zbmDecoderSpec->Code(bufInStream, outStream, &packSize64, &blockSize64, NULL));
      // in/out sizes were checked in Code()
    }
    
    outPos += blockSize;
    if ((i & 0xFF) == 0)
    {
      const UInt64 progressPos = progressStart + outPos;
      RINOK(extractCallback->SetCompleted(&progressPos));
    }
  }

  return S_OK;
}
*/

HRESULT CDecoder::Extract(
    ISequentialInStream *inStreamFork, ISequentialOutStream *realOutStream,
    UInt64 forkSize,
    const CCompressHeader &compressHeader,
    const CByteBuffer *data,
    UInt64 progressStart, IArchiveExtractCallback *extractCallback,
    int &opRes)
{
  opRes = NExtract::NOperationResult::kDataError;

  if (compressHeader.IsMethod_Uncompressed_Inline())
  {
    const size_t packSize = data->Size() - compressHeader.DataPos;
    if (realOutStream)
    {
      RINOK(WriteStream(realOutStream, *data + compressHeader.DataPos, packSize))
    }
    opRes = NExtract::NOperationResult::kOK;
    return S_OK;
  }

  if (compressHeader.Method == kMethod_ZLIB_ATTR ||
      compressHeader.Method == kMethod_LZVN_ATTR)
  {
    CMyComPtr2_Create<ISequentialInStream, CBufInStream> bufInStream;
    const size_t packSize = data->Size() - compressHeader.DataPos;
    bufInStream->Init(*data + compressHeader.DataPos, packSize);
    
    if (compressHeader.Method == kMethod_ZLIB_ATTR)
    {
      const HRESULT hres = _zlibDecoder.Interface()->Code(bufInStream, realOutStream,
          NULL, &compressHeader.UnpackSize, NULL);
      if (hres == S_OK)
        if (_zlibDecoder->GetOutputProcessedSize() == compressHeader.UnpackSize
            && _zlibDecoder->GetInputProcessedSize() == packSize)
          opRes = NExtract::NOperationResult::kOK;
      return hres;
    }
    {
      const UInt64 packSize64 = packSize;
      const HRESULT hres = _lzfseDecoder.Interface()->Code(bufInStream, realOutStream,
          &packSize64, &compressHeader.UnpackSize, NULL);
      if (hres == S_OK)
      {
        // in/out sizes were checked in Code()
        opRes = NExtract::NOperationResult::kOK;
      }
      return hres;
    }
  }

  HRESULT hres;
  if (compressHeader.Method == NHfs::kMethod_ZLIB_RSRC)
  {
    hres = ExtractResourceFork_ZLIB(
        inStreamFork, realOutStream,
        forkSize, compressHeader.UnpackSize,
        progressStart, extractCallback);
    // for debug:
    // hres = NCompress::CopyStream(inStreamFork, realOutStream, NULL);
  }
  else if (compressHeader.Method == NHfs::kMethod_LZVN_RSRC)
  {
    hres = ExtractResourceFork_LZFSE(
        inStreamFork, realOutStream,
        forkSize, compressHeader.UnpackSize,
        progressStart, extractCallback);
  }
  /*
  else if (compressHeader.Method == NHfs::kMethod_ZBM_RSRC)
  {
    hres = ExtractResourceFork_ZBM(
        inStreamFork, realOutStream,
        forkSize, compressHeader.UnpackSize,
        progressStart, extractCallback);
  }
  */
  else
  {
    opRes = NExtract::NOperationResult::kUnsupportedMethod;
    hres = S_FALSE;
  }

  if (hres == S_OK)
    opRes = NExtract::NOperationResult::kOK;
  return hres;
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = Refs.Size();
  if (numItems == 0)
    return S_OK;
  UInt32 i;
  UInt64 totalSize = 0;
  for (i = 0; i < numItems; i++)
  {
    const CRef &ref = Refs[allFilesMode ? i : indices[i]];
    totalSize += Get_UnpackSize_of_Ref(ref);
  }
  RINOK(extractCallback->SetTotal(totalSize))

  UInt64 currentTotalSize = 0, currentItemSize = 0;
  
  const size_t kBufSize = kCompressionBlockSize;
  CByteBuffer buf(kBufSize + 0x10); // we need 1 additional bytes for uncompressed chunk header

  // there are hfs without adler in zlib.
  CDecoder decoder(true); // IsAdlerOptional

  for (i = 0;; i++, currentTotalSize += currentItemSize)
  {
    RINOK(extractCallback->SetCompleted(&currentTotalSize))
    if (i >= numItems)
      break;
    const UInt32 index = allFilesMode ? i : indices[i];
    const CRef &ref = Refs[index];
    const CItem &item = Items[ref.ItemIndex];
    currentItemSize = Get_UnpackSize_of_Ref(ref);

    int opRes;
   {
    CMyComPtr<ISequentialOutStream> realOutStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode))

    if (ref.IsItem() && item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode))
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      continue;
    }
    if (!testMode && !realOutStream)
      continue;
    
    RINOK(extractCallback->PrepareOperation(askMode))
    
    UInt64 pos = 0;
    opRes = NExtract::NOperationResult::kDataError;
    const CFork *fork = NULL;
    
    if (ref.AttrIndex >= 0)
    {
      const CAttr &attr = Attrs[ref.AttrIndex];
      if (attr.Fork_defined && attr.Data.Size() == 0)
        fork = &attr.Fork;
      else
      {
        opRes = NExtract::NOperationResult::kOK;
        if (realOutStream)
        {
          RINOK(WriteStream(realOutStream,
              // AttrBuf + attr.Pos, attr.Size
              attr.Data, attr.Data.Size()
              ))
        }
      }
    }
    else if (ref.IsResource())
      fork = &item.ResourceFork;
    else if (item.CompressHeader.IsSupported)
    {
      CMyComPtr<ISequentialInStream> inStreamFork;
      UInt64 forkSize = 0;
      const CByteBuffer *decmpfs_Data = NULL;

      if (item.CompressHeader.IsMethod_Resource())
      {
        const CFork &resourceFork = item.ResourceFork;
        forkSize = resourceFork.Size;
        GetForkStream(resourceFork, &inStreamFork);
      }
      else
      {
        const CAttr &attr = Attrs[item.decmpfs_AttrIndex];
        decmpfs_Data = &attr.Data;
      }

      if (inStreamFork || decmpfs_Data)
      {
        const HRESULT hres = decoder.Extract(
            inStreamFork, realOutStream,
            forkSize,
            item.CompressHeader,
            decmpfs_Data,
            currentTotalSize, extractCallback,
            opRes);
        if (hres != S_FALSE && hres != S_OK)
          return hres;
      }
    }
    else if (item.CompressHeader.IsCorrect)
      opRes = NExtract::NOperationResult::kUnsupportedMethod;
    else
      fork = &item.DataFork;
    
    if (fork)
    {
      if (fork->IsOk(Header.BlockSizeLog))
      {
        opRes = NExtract::NOperationResult::kOK;
        unsigned extentIndex;
        for (extentIndex = 0; extentIndex < fork->Extents.Size(); extentIndex++)
        {
          if (opRes != NExtract::NOperationResult::kOK)
            break;
          if (fork->Size == pos)
            break;
          const CExtent &e = fork->Extents[extentIndex];
          RINOK(InStream_SeekSet(_stream, SpecOffset + ((UInt64)e.Pos << Header.BlockSizeLog)))
          UInt64 extentRem = (UInt64)e.NumBlocks << Header.BlockSizeLog;
          while (extentRem != 0)
          {
            const UInt64 rem = fork->Size - pos;
            if (rem == 0)
            {
              // Here we check that there are no extra (empty) blocks in last extent.
              if (extentRem >= ((UInt64)1 << Header.BlockSizeLog))
                opRes = NExtract::NOperationResult::kDataError;
              break;
            }
            size_t cur = kBufSize;
            if (cur > rem)
              cur = (size_t)rem;
            if (cur > extentRem)
              cur = (size_t)extentRem;
            RINOK(ReadStream(_stream, buf, &cur))
            if (cur == 0)
            {
              opRes = NExtract::NOperationResult::kDataError;
              break;
            }
            if (realOutStream)
            {
              RINOK(WriteStream(realOutStream, buf, cur))
            }
            pos += cur;
            extentRem -= cur;
            const UInt64 processed = currentTotalSize + pos;
            RINOK(extractCallback->SetCompleted(&processed))
          }
        }
        if (extentIndex != fork->Extents.Size() || fork->Size != pos)
          opRes = NExtract::NOperationResult::kDataError;
      }
    }
   }
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = Refs.Size();
  return S_OK;
}

HRESULT CHandler::GetForkStream(const CFork &fork, ISequentialInStream **stream)
{
  *stream = NULL;
  
  if (!fork.IsOk(Header.BlockSizeLog))
    return S_FALSE;

  CMyComPtr2<ISequentialInStream, CExtentsStream> extentStream;
  extentStream.Create_if_Empty();

  UInt64 rem = fork.Size;
  UInt64 virt = 0;

  FOR_VECTOR (i, fork.Extents)
  {
    const CExtent &e = fork.Extents[i];
    if (e.NumBlocks == 0)
      continue;
    UInt64 cur = ((UInt64)e.NumBlocks << Header.BlockSizeLog);
    if (cur > rem)
    {
      cur = rem;
      if (i != fork.Extents.Size() - 1)
        return S_FALSE;
    }
    CSeekExtent se;
    se.Phy = SpecOffset + ((UInt64)e.Pos << Header.BlockSizeLog);
    se.Virt = virt;
    virt += cur;
    rem -= cur;
    extentStream->Extents.Add(se);
  }
  
  if (rem != 0)
    return S_FALSE;
  
  CSeekExtent se;
  se.Phy = 0; // = SpecOffset ?
  se.Virt = virt;
  extentStream->Extents.Add(se);
  extentStream->Stream = _stream;
  extentStream->Init();
  *stream = extentStream.Detach();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  *stream = NULL;

  const CRef &ref = Refs[index];
  const CFork *fork = NULL;
  if (ref.AttrIndex >= 0)
  {
    const CAttr &attr = Attrs[ref.AttrIndex];
    if (!attr.Fork_defined || attr.Data.Size() != 0)
      return S_FALSE;
    fork = &attr.Fork;
  }
  else
  {
    const CItem &item = Items[ref.ItemIndex];
    if (ref.IsResource())
      fork = &item.ResourceFork;
    else if (item.IsDir())
      return S_FALSE;
    else if (item.CompressHeader.IsCorrect)
      return S_FALSE;
    else
      fork = &item.DataFork;
  }
  return GetForkStream(*fork, stream);
}

static const Byte k_Signature[] = {
    2, 'B', 'D',
    4, 'H', '+', 0, 4,
    4, 'H', 'X', 0, 5 };

REGISTER_ARC_I(
  "HFS", "hfs hfsx", NULL, 0xE3,
  k_Signature,
  kHeaderPadSize,
  NArcInfoFlags::kMultiSignature,
  IsArc_HFS)

}}

/* ================ unit: CPP/7zip/Archive/IhexHandler.cpp ================ */
// IhexHandler.cpp

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

namespace NArchive {
namespace NIhex {

/* We still don't support files with custom record types: 20, 22: used by Samsung */

struct CBlock
{
  CByteDynamicBuffer Data;
  UInt32 Offset;
};


Z7_CLASS_IMP_CHandler_IInArchive_0

  bool _isArc;
  bool _needMoreInput;
  bool _dataError;
  
  UInt64 _phySize;

  CObjectVector<CBlock> _blocks;
};

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidVa
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _blocks.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: if (_phySize != 0) prop = _phySize; break;
    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_needMoreInput) v |= kpv_ErrorFlags_UnexpectedEnd;
      if (_dataError) v |= kpv_ErrorFlags_DataError;
      prop = v;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CBlock &block = _blocks[index];
  switch (propID)
  {
    case kpidSize: prop = (UInt64)block.Data.GetPos(); break;
    case kpidVa: prop = block.Offset; break;
    case kpidPath:
    {
      if (_blocks.Size() != 1)
      {
        char s[16];
        ConvertUInt32ToString(index, s);
        prop = s;
      }
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


static int Parse(const Byte *p)
{
  unsigned v0 = p[0];  Z7_PARSE_HEX_DIGIT(v0, return -1;)
  unsigned v1 = p[1];  Z7_PARSE_HEX_DIGIT(v1, return -1;)
  return (int)((v0 << 4) | v1);
}
  
#define kType_Data 0
#define kType_Eof  1
#define kType_Seg  2
// #define kType_CsIp 3
#define kType_High 4
// #define kType_Ip32 5

#define kType_MAX  5

// we don't want to read files with big number of spaces between records
// it's our limitation (out of specification):
static const unsigned k_NumSpaces_LIMIT = 16 + 1;

#define IS_LINE_DELIMITER(c) (/* (c) == 0 || */ (c) == 10 || (c) == 13)

API_FUNC_static_IsArc IsArc_Ihex(const Byte *p, size_t size)
{
  if (size < 1)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != ':')
    return k_IsArc_Res_NO;
  p++;
  size--;

  const unsigned kNumLinesToCheck = 3; // 1 line is OK also, but we don't want false detection

  for (unsigned j = 0; j < kNumLinesToCheck; j++)
  {
    if (size < 4 * 2)
      return k_IsArc_Res_NEED_MORE;
    
    const int num = Parse(p);
    if (num < 0)
      return k_IsArc_Res_NO;
    
    const int type = Parse(p + 6);
    if (type < 0 || type > kType_MAX)
      return k_IsArc_Res_NO;
    
    unsigned numChars = ((unsigned)num + 5) * 2;
    unsigned sum = 0;
    
    for (unsigned i = 0; i < numChars; i += 2)
    {
      if (i + 2 > size)
        return k_IsArc_Res_NEED_MORE;
      int v = Parse(p + i);
      if (v < 0)
        return k_IsArc_Res_NO;
      sum += (unsigned)v;
    }
    
    if (sum & 0xFF)
      return k_IsArc_Res_NO;

    if (type == kType_Data)
    {
      // we don't want to open :0000000000 files
      if (num == 0)
        return k_IsArc_Res_NO;
    }
    else
    {
      if (type == kType_Eof)
      {
        if (num != 0)
          return k_IsArc_Res_NO;
        return k_IsArc_Res_YES;
      }
      if (p[2] != 0 ||
          p[3] != 0 ||
          p[4] != 0 ||
          p[5] != 0)
        return k_IsArc_Res_NO;
      if (type == kType_Seg || type == kType_High)
      {
        if (num != 2)
          return k_IsArc_Res_NO;
      }
      else
      {
        if (num != 4)
          return k_IsArc_Res_NO;
      }
    }
    
    p += numChars;
    size -= numChars;
    
    unsigned numSpaces = k_NumSpaces_LIMIT;
    for (;;)
    {
      if (size == 0)
        return k_IsArc_Res_NEED_MORE;
      const Byte b = *p++;
      size--;
      if (b == ':')
        break;
      if (--numSpaces == 0 || !IS_LINE_DELIMITER(b))
        return k_IsArc_Res_NO;
    }
  }
  
  return k_IsArc_Res_YES;
}
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *openCallback))
{
  COM_TRY_BEGIN
  {
  Close();
  try
  {
    const unsigned kStartSize = (2 + (256 + 5) + 2) * 2;
    Byte temp[kStartSize];
    {
      size_t size = kStartSize;
      RINOK(ReadStream(stream, temp, &size))
      const UInt32 isArcRes = IsArc_Ihex(temp, size);
      if (isArcRes == k_IsArc_Res_NO)
        return S_FALSE;
      if (isArcRes == k_IsArc_Res_NEED_MORE && size != kStartSize)
        return S_FALSE;
    }
    _isArc = true;

    RINOK(InStream_SeekToBegin(stream))
    CInBuffer s;
    if (!s.Create(1 << 15))
      return E_OUTOFMEMORY;
    s.SetStream(stream);
    s.Init();
    {
      Byte b;
      if (!s.ReadByte(b))
      {
        _needMoreInput = true;
        return S_FALSE;
      }
      if (b != ':')
      {
        _dataError = true;
        return S_FALSE;
      }
    }

    UInt32 globalOffset = 0;
    const UInt32 k_progressStep = 1 << 24;
    UInt64 progressNext = k_progressStep;

    for (;;)
    {
      if (s.ReadBytes(temp, 2) != 2)
      {
        _needMoreInput = true;
        return S_FALSE;
      }
      const int num = Parse(temp);
      if (num < 0)
      {
        _dataError = true;
        return S_FALSE;
      }
      {
        const size_t numPairs = (unsigned)num + 4;
        const size_t numBytes = numPairs * 2;
        if (s.ReadBytes(temp, numBytes) != numBytes)
        {
          _needMoreInput = true;
          return S_FALSE;
        }
        unsigned sum = (unsigned)num;
        for (size_t i = 0; i < numPairs; i++)
        {
          const int a = Parse(temp + i * 2);
          if (a < 0)
          {
            _dataError = true;
            return S_FALSE;
          }
          temp[i] = (Byte)a;
          sum += (unsigned)a;
        }
        if (sum & 0xFF)
        {
          _dataError = true;
          return S_FALSE;
        }
      }

      const unsigned type = temp[2];
      if (type > kType_MAX)
      {
        _dataError = true;
        return S_FALSE;
      }

      const UInt32 a = GetBe16(temp);

      if (type == kType_Data)
      {
        if (num == 0)
        {
          // we don't want to open :0000000000 files
          // maybe it can mean EOF in old-style files?
          _dataError = true;
          return S_FALSE;
        }
        // if (num != 0)
        {
          const UInt32 offs = globalOffset + a;
          CBlock *block = NULL;
          if (!_blocks.IsEmpty())
          {
            block = &_blocks.Back();
            if (block->Offset + block->Data.GetPos() != offs)
              block = NULL;
          }
          if (!block)
          {
            block = &_blocks.AddNew();
            block->Offset = offs;
          }
          block->Data.AddData(temp + 3, (unsigned)num);
        }
      }
      else
      {
        if (a != 0) // from description: the address field is typically 0.
        {
          _dataError = true;
          return S_FALSE;
        }
        if (type == kType_Eof)
        {
          if (num != 0)
          {
            _dataError = true;
            return S_FALSE;
          }
          _phySize = s.GetProcessedSize();
          Byte b;
          if (s.ReadByte(b))
          {
            if (b == 10)
              _phySize++;
            else if (b == 13)
            {
              _phySize++;
              if (s.ReadByte(b))
              {
                if (b == 10)
                  _phySize++;
              }
            }
          }
          return S_OK;
        }
  
        if (type == kType_Seg || type == kType_High)
        {
          if (num != 2)
          {
            _dataError = true;
            return S_FALSE;
          }
          // here we use optimization trick for num shift calculation: (type == kType_Seg ? 4 : 16)
          globalOffset = (UInt32)GetBe16(temp + 3) << (1u << type);
        }
        else
        {
          if (num != 4)
          {
            _dataError = true;
            return S_FALSE;
          }
        }
      }

      if (openCallback)
      {
        const UInt64 processed = s.GetProcessedSize();
        if (processed >= progressNext)
        {
          progressNext = processed + k_progressStep;
          const UInt64 numFiles = _blocks.Size();
          RINOK(openCallback->SetCompleted(&numFiles, &processed))
        }
      }

      unsigned numSpaces = k_NumSpaces_LIMIT;
      for (;;)
      {
        Byte b;
        if (!s.ReadByte(b))
        {
          _needMoreInput = true;
          return S_FALSE;
        }
        if (b == ':')
          break;
        if (--numSpaces == 0 || !IS_LINE_DELIMITER(b))
        {
          _dataError = true;
          return S_FALSE;
        }
      }
    }
  }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  }
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::Close())
{
  _phySize = 0;
  _isArc = false;
  _needMoreInput = false;
  _dataError = false;
  _blocks.Clear();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _blocks.Size();
  if (numItems == 0)
    return S_OK;

  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _blocks[allFilesMode ? i : indices[i]].Data.GetPos();
  RINOK(extractCallback->SetTotal(totalSize))

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);

  for (i = 0;; i++)
  {
    lps->InSize = lps->OutSize;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;
    const UInt32 index = allFilesMode ? i : indices[i];
    const CByteDynamicBuffer &data = _blocks[index].Data;
    lps->OutSize += data.GetPos();
    {
      CMyComPtr<ISequentialOutStream> realOutStream;
      const Int32 askMode = testMode ?
          NExtract::NAskMode::kTest :
          NExtract::NAskMode::kExtract;
      RINOK(extractCallback->GetStream(index, &realOutStream, askMode))
      if (!testMode && !realOutStream)
        continue;
      RINOK(extractCallback->PrepareOperation(askMode))
      if (realOutStream)
        RINOK(WriteStream(realOutStream, (const Byte *)data, data.GetPos()))
    }
    RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
  }
  
  return S_OK;
  COM_TRY_END
}

// k_Signature: { ':' }

REGISTER_ARC_I_NO_SIG(
  "IHex", "ihex", NULL, 0xCD,
  0,
  NArcInfoFlags::kStartOpen,
  IsArc_Ihex)

}}

/* ================ unit: CPP/7zip/Archive/LpHandler.cpp ================ */
// LpHandler.cpp

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
#define Get64(p) GetUi64(p)

#define G16(_offs_, dest) dest = Get16(p + (_offs_));
#define G32(_offs_, dest) dest = Get32(p + (_offs_));
#define G64(_offs_, dest) dest = Get64(p + (_offs_));

using namespace NWindows;

namespace NArchive {

namespace NExt {
API_FUNC_IsArc IsArc_Ext_PhySize(const Byte *p, size_t size, UInt64 *phySize);
}

namespace NLp {

/*
Android 10+ use Android's Dynamic Partitions to allow the
different read-only system partitions (e.g. system, vendor, product)
to share the same pool of storage space (as LVM in Linux).
Name for partition: "super" (for GPT) or "super.img" (for file).
Dynamic Partition Tools: lpmake
All partitions that are A/B-ed should be named as follows (slots are always named a, b, etc.):
boot_a, boot_b, system_a, system_b, vendor_a, vendor_b.
*/

#define LP_METADATA_MAJOR_VERSION 10
// #define LP_METADATA_MINOR_VERSION_MIN 0
// #define LP_METADATA_MINOR_VERSION_MAX 2

// #define LP_SECTOR_SIZE 512
static const unsigned kSectorSizeLog = 9;

/* Amount of space reserved at the start of every super partition to avoid
 * creating an accidental boot sector. */
#define LP_PARTITION_RESERVED_BYTES 4096
#define LP_METADATA_GEOMETRY_SIZE 4096
#define LP_METADATA_HEADER_MAGIC 0x414C5030

static const unsigned k_SignatureSize = 8;
static const Byte k_Signature[k_SignatureSize] =
  { 0x67, 0x44, 0x6c, 0x61, 0x34, 0, 0, 0 };

// The length (36) is the same as the maximum length of a GPT partition name.
static const unsigned kNameLen = 36;

static void AddName36ToString(AString &s, const char *name, bool strictConvert)
{
  for (unsigned i = 0; i < kNameLen; i++)
  {
    char c = name[i];
    if (c == 0)
      return;
    if (strictConvert && c < 32)
      c = '_';
    s += c;
  }
}


static const unsigned k_Geometry_Size = 0x34;

// LpMetadataGeometry
struct CGeometry
{
  // UInt32 magic;
  // UInt32 struct_size;
  // Byte checksum[32];   /*  SHA256 checksum of this struct, with this field set to 0. */
  
  /* Maximum amount of space a single copy of the metadata can use,
     a multiple of LP_SECTOR_SIZE. */
  UInt32 metadata_max_size;
  
  /* Number of copies of the metadata to keep.
     For Non-A/B: 1, For A/B: 2, for A/B/C: 3.
     A backup copy of each slot is kept */
  UInt32 metadata_slot_count;
  
  /* minimal alignment for partition and extent sizes, a multiple of LP_SECTOR_SIZE. */
  UInt32 logical_block_size;

  bool Parse(const Byte *p)
  {
    G32 (40, metadata_max_size)
    G32 (44, metadata_slot_count)
    G32 (48, logical_block_size)
    if (metadata_slot_count == 0 || metadata_slot_count >= ((UInt32)1 << 20))
      return false;
    if (metadata_max_size == 0)
      return false;
    if ((metadata_max_size & (((UInt32)1 << kSectorSizeLog) - 1)) != 0)
      return false;
    return true;
  }

  UInt64 GetTotalMetadataSize() const
  {
    // there are 2 copies of GEOMETRY and METADATA slots
    return LP_PARTITION_RESERVED_BYTES +
           LP_METADATA_GEOMETRY_SIZE * 2 +
           ((UInt64)metadata_max_size * metadata_slot_count) * 2;
  }
};



// LpMetadataTableDescriptor
struct CDescriptor
{
  UInt32 offset;        /*  Location of the table, relative to end of the metadata header. */
  UInt32 num_entries;   /*  Number of entries in the table. */
  UInt32 entry_size;    /*  Size of each entry in the table, in bytes. */

  void Parse(const Byte *p)
  {
    G32 (0, offset)
    G32 (4, num_entries)
    G32 (8, entry_size)
  }
 
  bool CheckLimits(UInt32 limit) const
  {
    if (entry_size == 0)
      return false;
    const UInt32 size = num_entries * entry_size;
    if (size / entry_size != num_entries)
      return false;
    if (offset > limit || limit - offset < size)
      return false;
    return true;
  }
};


// #define LP_PARTITION_ATTR_NONE 0x0
// #define LP_PARTITION_ATTR_READONLY (1 << 0)

/* This flag is only intended to be used with super_empty.img and super.img on
 * retrofit devices. On these devices there are A and B super partitions, and
 * we don't know ahead of time which slot the image will be applied to.
 *
 * If set, the partition name needs a slot suffix applied. The slot suffix is
 * determined by the metadata slot number (0 = _a, 1 = _b).
 */
// #define LP_PARTITION_ATTR_SLOT_SUFFIXED (1 << 1)

/* This flag is applied automatically when using MetadataBuilder::NewForUpdate.
 * It signals that the partition was created (or modified) for a snapshot-based
 * update. If this flag is not present, the partition was likely flashed via
 * fastboot.
 */
// #define LP_PARTITION_ATTR_UPDATED (1 << 2)

/* This flag marks a partition as disabled. It should not be used or mapped. */
// #define LP_PARTITION_ATTR_DISABLED (1 << 3)

static const char * const g_PartitionAttr[] =
{
    "READONLY"
  , "SLOT_SUFFIXED"
  , "UPDATED"
  , "DISABLED"
};

static unsigned const k_MetaPartition_Size = 52;

// LpMetadataPartition
struct CPartition
{
  /* ASCII characters: alphanumeric or _. at least one ASCII character,
     (name) must be unique across all partition names. */
  char name[kNameLen];
  
  UInt32 attributes;   /* (LP_PARTITION_ATTR_*). */

  /* Index of the first extent owned by this partition. The extent will
   * start at logical sector 0. Gaps between extents are not allowed. */
  UInt32 first_extent_index;
  
  /* Number of extents in the partition. Every partition must have at least one extent. */
  UInt32 num_extents;

  /* Group this partition belongs to. */
  UInt32 group_index;

  void Parse(const Byte *p)
  {
    memcpy(name, p, kNameLen);
    G32 (36, attributes)
    G32 (40, first_extent_index)
    G32 (44, num_extents)
    G32 (48, group_index)
  }

  // calced properties:
  UInt32 MethodsMask;
  UInt64 NumSectors;
  UInt64 NumSectors_Pack;
  const char *Ext;

  UInt64 GetSize() const { return NumSectors << kSectorSizeLog; }
  UInt64 GetPackSize() const { return NumSectors_Pack << kSectorSizeLog; }
  
  CPartition():
      MethodsMask(0),
      NumSectors(0),
      NumSectors_Pack(0),
      Ext(NULL)
      {}
};




#define LP_TARGET_TYPE_LINEAR 0
/* This extent is a dm-zero target. The index is ignored and must be 0. */
#define LP_TARGET_TYPE_ZERO 1

static const char * const g_Methods[] =
{
    "RAW" // "LINEAR"
  , "ZERO"
};

static unsigned const k_MetaExtent_Size = 24;

// LpMetadataExtent
struct CExtent
{
  UInt64 num_sectors;  /*  Length in 512-byte sectors. */
  UInt32 target_type;  /*  Target type for device-mapper (LP_TARGET_TYPE_*). */

  /* for LINEAR: The sector on the physical partition that this extent maps onto.
     for ZERO:   must be 0. */
  UInt64 target_data;

  /* for LINEAR: index into the block devices table.
     for ZERO:   must be 0. */
  UInt32 target_source;

  bool IsRAW() const { return target_type == LP_TARGET_TYPE_LINEAR; }

  void Parse(const Byte *p)
  {
    G64 (0, num_sectors)
    G32 (8, target_type)
    G64 (12, target_data)
    G32 (20, target_source)
  }
};


/* This flag is only intended to be used with super_empty.img and super.img on
 * retrofit devices. If set, the group needs a slot suffix to be interpreted
 * correctly. The suffix is automatically applied by ReadMetadata().
 */
// #define LP_GROUP_SLOT_SUFFIXED (1 << 0)
static unsigned const k_Group_Size = 48;

// LpMetadataPartitionGroup
struct CGroup
{
  char name[kNameLen];
  UInt32 flags;  /* (LP_GROUP_*). */
  UInt64 maximum_size; /* Maximum size in bytes. If 0, the group has no maximum size. */

  void Parse(const Byte *p)
  {
    memcpy(name, p, kNameLen);
    G32 (36, flags)
    G64 (40, maximum_size)
  }
};




/* This flag is only intended to be used with super_empty.img and super.img on
 * retrofit devices. On these devices there are A and B super partitions, and
 * we don't know ahead of time which slot the image will be applied to.
 *
 * If set, the block device needs a slot suffix applied before being used with
 * IPartitionOpener. The slot suffix is determined by the metadata slot number
 * (0 = _a, 1 = _b).
 */
// #define LP_BLOCK_DEVICE_SLOT_SUFFIXED (1 << 0)

static unsigned const k_Device_Size = 64;

/* This struct defines an entry in the block_devices table. There must be at
 * least one device, and the first device must represent the partition holding
 * the super metadata.
 */
// LpMetadataBlockDevice
struct CDevice
{
    /* 0: First usable sector for allocating logical partitions. this will be
     * the first sector after the initial geometry blocks, followed by the
     * space consumed by metadata_max_size*metadata_slot_count*2.
     */
  UInt64 first_logical_sector;

    /* 8: Alignment for defining partitions or partition extents. For example,
     * an alignment of 1MiB will require that all partitions have a size evenly
     * divisible by 1MiB, and that the smallest unit the partition can grow by
     * is 1MiB.
     *
     * Alignment is normally determined at runtime when growing or adding
     * partitions. If for some reason the alignment cannot be determined, then
     * this predefined alignment in the geometry is used instead. By default
     * it is set to 1MiB.
     */
  UInt32 alignment;

    /* 12: Alignment offset for "stacked" devices. For example, if the "super"
     * partition itself is not aligned within the parent block device's
     * partition table, then we adjust for this in deciding where to place
     * |first_logical_sector|.
     *
     * Similar to |alignment|, this will be derived from the operating system.
     * If it cannot be determined, it is assumed to be 0.
     */
  UInt32 alignment_offset;

    /* 16: Block device size, as specified when the metadata was created. This
     * can be used to verify the geometry against a target device.
     */
  UInt64 size;

    /* 24: Partition name in the GPT*/
  char partition_name[kNameLen];

    /* 60: Flags (see LP_BLOCK_DEVICE_* flags below). */
  UInt32 flags;

  void Parse(const Byte *p)
  {
    memcpy(partition_name, p + 24, kNameLen);
    G64 (0, first_logical_sector)
    G32 (8, alignment)
    G32 (12, alignment_offset)
    G64 (16, size)
    G32 (60, flags)
  }
};


/* This device uses Virtual A/B. Note that on retrofit devices, the expanded
 * header may not be present.
 */
// #define LP_HEADER_FLAG_VIRTUAL_AB_DEVICE 0x1

static const char * const g_Header_Flags[] =
{
  "VIRTUAL_AB"
};


static const unsigned k_LpMetadataHeader10_size = 128;
static const unsigned k_LpMetadataHeader12_size = 256;

struct LpMetadataHeader
{
  /*  0: Four bytes equal to LP_METADATA_HEADER_MAGIC. */
  UInt32 magic;
  
  /*  4: Version number required to read this metadata. If the version is not
   * equal to the library version, the metadata should be considered
   * incompatible.
   */
  UInt16 major_version;
  
  /*  6: Minor version. A library supporting newer features should be able to
   * read metadata with an older minor version. However, an older library
   * should not support reading metadata if its minor version is higher.
   */
  UInt16 minor_version;
  
  /*  8: The size of this header struct. */
  UInt32 header_size;
  
  /* 12: SHA256 checksum of the header, up to |header_size| bytes, computed as
   * if this field were set to 0.
   */
  // Byte header_checksum[32];
  
  /* 44: The total size of all tables. This size is contiguous; tables may not
   * have gaps in between, and they immediately follow the header.
   */
  UInt32 tables_size;
  
  /* 48: SHA256 checksum of all table contents. */
  Byte tables_checksum[32];
  
  /* 80: Partition table descriptor. */
  CDescriptor partitions;
  /* 92: Extent table descriptor. */
  CDescriptor extents;
  /* 104: Updateable group descriptor. */
  CDescriptor groups;
  /* 116: Block device table. */
  CDescriptor block_devices;
  
  /* Everything past here is header version 1.2+, and is only included if
   * needed. When liblp supporting >= 1.2 reads a < 1.2 header, it must
   * zero these additional fields.
   */
  
  /* 128: See LP_HEADER_FLAG_ constants for possible values. Header flags are
   * independent of the version number and intended to be informational only.
   * New flags can be added without bumping the version.
   */
  // UInt32 flags;
  
  /* 132: Reserved (zero), pad to 256 bytes. */
  // Byte reserved[124];

  void Parse128(const Byte *p)
  {
    G32 (0, magic)
    G16 (4, major_version)
    G16 (6, minor_version)
    G32 (8, header_size)
    // Byte header_checksum[32];
    G32 (44, tables_size)
    memcpy (tables_checksum, p + 48, 32);
    partitions.Parse(p + 80);
    extents.Parse(p + 92);
    groups.Parse(p + 104);
    block_devices.Parse(p + 116);
    /* Everything past here is header version 1.2+, and is only included if
     * needed. When liblp supporting >= 1.2 reads a < 1.2 header, it must
     * zero these additional fields.
     */
  }
};


static bool CheckSha256(const Byte *data, size_t size, const Byte *checksum)
{
  MY_ALIGN (16)
  CSha256 sha;
  Sha256_Init(&sha);
  Sha256_Update(&sha, data, size);
  MY_ALIGN (16)
  Byte calced[32];
  Sha256_Final(&sha, calced);
  return memcmp(checksum, calced, 32) == 0;
}

static bool CheckSha256_csOffset(Byte *data, size_t size, unsigned hashOffset)
{
  MY_ALIGN (4)
  Byte checksum[32];
  Byte *shaData = &data[hashOffset];
  memcpy(checksum, shaData, 32);
  memset(shaData, 0, 32);
  return CheckSha256(data, size, checksum);
}



Z7_CLASS_IMP_CHandler_IInArchive_1(
  IInArchiveGetStream
)
  CRecordVector<CPartition> _items;
  CRecordVector<CExtent> Extents;

  CMyComPtr<IInStream> _stream;
  UInt64 _totalSize;
  // UInt64 _usedSize;
  // UInt64 _headersSize;

  CGeometry geom;
  UInt16 Major_version;
  UInt16 Minor_version;
  UInt32 Flags;

  Int32 _mainFileIndex;
  UInt32 MethodsMask;
  bool _headerWarning;
  AString GroupsString;
  AString DevicesString;
  AString DeviceArcName;

  HRESULT Open2(IInStream *stream);
};


static void AddComment_UInt64(AString &s, const char *name, UInt64 val)
{
  s.Add_Space();
  s += name;
  s += '=';
  s.Add_UInt64(val);
}


static bool IsBufZero(const Byte *data, size_t size)
{
  for (size_t i = 0; i < size; i += 4)
    if (*(const UInt32 *)(const void *)(data + i) != 0)
      return false;
  return true;
}


HRESULT CHandler::Open2(IInStream *stream)
{
  RINOK(InStream_SeekSet(stream, LP_PARTITION_RESERVED_BYTES))
  {
    MY_ALIGN (4)
    Byte buf[k_Geometry_Size];
    RINOK(ReadStream_FALSE(stream, buf, k_Geometry_Size))
    if (memcmp(buf, k_Signature, k_SignatureSize) != 0)
      return S_FALSE;
    if (!geom.Parse(buf))
      return S_FALSE;
    if (!CheckSha256_csOffset(buf, k_Geometry_Size, 8))
      return S_FALSE;
  }

  CByteBuffer buffer;
  RINOK(InStream_SeekToBegin(stream))
  buffer.Alloc(LP_METADATA_GEOMETRY_SIZE * 2);
  {
    // buffer.Size() >= LP_PARTITION_RESERVED_BYTES
    RINOK(ReadStream_FALSE(stream, buffer, LP_PARTITION_RESERVED_BYTES))
    if (!IsBufZero(buffer, LP_PARTITION_RESERVED_BYTES))
    {
      _headerWarning = true;
      // return S_FALSE;
    }
  }

  RINOK(ReadStream_FALSE(stream, buffer, LP_METADATA_GEOMETRY_SIZE * 2))
  // we check that 2 copies of GEOMETRY are identical:
  if (memcmp(buffer, buffer + LP_METADATA_GEOMETRY_SIZE, LP_METADATA_GEOMETRY_SIZE) != 0
      || !IsBufZero(buffer + k_Geometry_Size, LP_METADATA_GEOMETRY_SIZE - k_Geometry_Size))
  {
    _headerWarning = true;
    // return S_FALSE;
  }

  RINOK(ReadStream_FALSE(stream, buffer, k_LpMetadataHeader10_size))
  LpMetadataHeader header;
  header.Parse128(buffer);
  if (header.magic != LP_METADATA_HEADER_MAGIC ||
      header.major_version != LP_METADATA_MAJOR_VERSION ||
      header.header_size < k_LpMetadataHeader10_size)
    return S_FALSE;
  Flags = 0;
  if (header.header_size > k_LpMetadataHeader10_size)
  {
    if (header.header_size != k_LpMetadataHeader12_size)
      return S_FALSE;
    RINOK(ReadStream_FALSE(stream, buffer + k_LpMetadataHeader10_size,
        header.header_size - k_LpMetadataHeader10_size))
    Flags = Get32(buffer + k_LpMetadataHeader10_size);
  }
  Major_version = header.major_version;
  Minor_version = header.minor_version;

  if (!CheckSha256_csOffset(buffer, header.header_size, 12))
    return S_FALSE;

  if (geom.metadata_max_size < header.tables_size ||
      geom.metadata_max_size - header.tables_size < header.header_size)
    return S_FALSE;

  buffer.AllocAtLeast(header.tables_size);
  RINOK(ReadStream_FALSE(stream, buffer, header.tables_size))

  const UInt64 totalMetaSize = geom.GetTotalMetadataSize();
  // _headersSize = _totalSize;
  _totalSize = totalMetaSize;

  if (!CheckSha256(buffer, header.tables_size, header.tables_checksum))
    return S_FALSE;

  {
    const CDescriptor &d = header.partitions;
    if (!d.CheckLimits(header.tables_size))
      return S_FALSE;
    if (d.entry_size != k_MetaPartition_Size)
      return S_FALSE;
    for (UInt32 i = 0; i < d.num_entries; i++)
    {
      CPartition part;
      part.Parse(buffer + d.offset + i * d.entry_size);
      const UInt32 extLimit = part.first_extent_index + part.num_extents;
      if (extLimit < part.first_extent_index ||
          extLimit > header.extents.num_entries ||
          part.group_index >= header.groups.num_entries)
        return S_FALSE;
      _items.Add(part);
    }
  }
  {
    const CDescriptor &d = header.extents;
    if (!d.CheckLimits(header.tables_size))
      return S_FALSE;
    if (d.entry_size != k_MetaExtent_Size)
      return S_FALSE;
    for (UInt32 i = 0; i < d.num_entries; i++)
    {
      CExtent e;
      e.Parse(buffer + d.offset + i * d.entry_size);
      // if (e.target_type > LP_TARGET_TYPE_ZERO) return S_FALSE;
      if (e.IsRAW())
      {
        if (e.target_source >= header.block_devices.num_entries)
          return S_FALSE;
        const UInt64 endSector = e.target_data + e.num_sectors;
        const UInt64 endOffset = endSector << kSectorSizeLog;
        if (_totalSize < endOffset)
          _totalSize = endOffset;
      }
      MethodsMask |= (UInt32)1 << e.target_type;
      Extents.Add(e);
    }
  }
  
  // _usedSize = _totalSize;
  {
    const CDescriptor &d = header.groups;
    if (!d.CheckLimits(header.tables_size))
      return S_FALSE;
    if (d.entry_size != k_Group_Size)
      return S_FALSE;
    AString s;
    for (UInt32 i = 0; i < d.num_entries; i++)
    {
      CGroup g;
      g.Parse(buffer + d.offset + i * d.entry_size);
      if (_totalSize < g.maximum_size)
        _totalSize = g.maximum_size;
      s += "  ";
      AddName36ToString(s, g.name, true);
      AddComment_UInt64(s, "maximum_size", g.maximum_size);
      AddComment_UInt64(s, "flags", g.flags);
      s.Add_LF();
    }
    GroupsString = s;
  }

  {
    const CDescriptor &d = header.block_devices;
    if (!d.CheckLimits(header.tables_size))
      return S_FALSE;
    if (d.entry_size != k_Device_Size)
      return S_FALSE;
    AString s;
    // CRecordVector<CDevice> devices;
    for (UInt32 i = 0; i < d.num_entries; i++)
    {
      CDevice v;
      v.Parse(buffer + d.offset + i * d.entry_size);
      // if (i == 0)
      {
        // it's super_device is first device;
        if (totalMetaSize > (v.first_logical_sector << kSectorSizeLog))
          return S_FALSE;
      }
      if (_totalSize < v.size)
        _totalSize = v.size;
      s += "  ";
      if (i == 0)
        AddName36ToString(DeviceArcName, v.partition_name, true);
      // devices.Add(v);
      AddName36ToString(s, v.partition_name, true);
      AddComment_UInt64(s, "size", v.size);
      AddComment_UInt64(s, "first_logical_sector", v.first_logical_sector);
      AddComment_UInt64(s, "alignment", v.alignment);
      AddComment_UInt64(s, "alignment_offset", v.alignment_offset);
      AddComment_UInt64(s, "flags", v.flags);
      s.Add_LF();
    }
    DevicesString = s;
  }

  {
    FOR_VECTOR (i, _items)
    {
      CPartition &part = _items[i];
      if (part.first_extent_index > Extents.Size() ||
          part.num_extents > Extents.Size() - part.first_extent_index)
        return S_FALSE;

      UInt64 numSectors = 0;
      UInt64 numSectors_Pack = 0;
      UInt32 methods = 0;
      for (UInt32 k = 0; k < part.num_extents; k++)
      {
        const CExtent &e = Extents[part.first_extent_index + k];
        numSectors += e.num_sectors;
        if (e.IsRAW())
          numSectors_Pack += e.num_sectors;
        methods |= (UInt32)1 << e.target_type;
      }
      part.NumSectors = numSectors;
      part.NumSectors_Pack = numSectors_Pack;
      part.MethodsMask = methods;
    }
  }

  return S_OK;
}


Z7_COM7F_IMF(CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */))
{
  COM_TRY_BEGIN
  Close();
  RINOK(Open2(stream))
  _stream = stream;

  int mainFileIndex = -1;
  unsigned numNonEmptyParts = 0;

  FOR_VECTOR (fileIndex, _items)
  {
    CPartition &item = _items[fileIndex];
    if (item.NumSectors != 0)
    {
      mainFileIndex = (int)fileIndex;
      numNonEmptyParts++;
      CMyComPtr<ISequentialInStream> parseStream;
      if (GetStream(fileIndex, &parseStream) == S_OK && parseStream)
      {
        const size_t kParseSize = 1 << 11;
        Byte buf[kParseSize];
        if (ReadStream_FAIL(parseStream, buf, kParseSize) == S_OK)
        {
          UInt64 extSize;
          if (NExt::IsArc_Ext_PhySize(buf, kParseSize, &extSize) == k_IsArc_Res_YES)
            if (extSize == item.GetSize())
              item.Ext = "ext";
        }
      }
    }
  }
  if (numNonEmptyParts == 1)
    _mainFileIndex = mainFileIndex;

  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::Close())
{
  _totalSize = 0;
  // _usedSize = 0;
  // _headersSize = 0;
  _items.Clear();
  Extents.Clear();
  _stream.Release();
  _mainFileIndex = -1;
  _headerWarning = false;
  MethodsMask = 0;
  GroupsString.Empty();
  DevicesString.Empty();
  DeviceArcName.Empty();
  return S_OK;
}


static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidPackSize,
  kpidCharacts,
  kpidMethod,
  kpidNumBlocks,
  kpidOffset
};

static const Byte kArcProps[] =
{
  kpidUnpackVer,
  kpidMethod,
  kpidClusterSize,
  // kpidHeadersSize,
  // kpidFreeSpace,
  kpidName,
  kpidComment
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMainSubfile:
    {
      if (_mainFileIndex >= 0)
        prop = (UInt32)_mainFileIndex;
      break;
    }
    case kpidPhySize: prop = _totalSize; break;

    // case kpidFreeSpace: if (_usedSize != 0) prop = _totalSize - _usedSize; break;
    // case kpidHeadersSize: prop = _headersSize; break;

    case kpidMethod:
    {
      const UInt32 m = MethodsMask;
      if (m != 0)
      {
        FLAGS_TO_PROP(g_Methods, m, prop);
      }
      break;
    }

    case kpidUnpackVer:
    {
      AString s;
      s.Add_UInt32(Major_version);
      s.Add_Dot();
      s.Add_UInt32(Minor_version);
      prop = s;
      break;
    }

    case kpidClusterSize:
      prop = geom.logical_block_size;
      break;

    case kpidComment:
    {
      AString s;

      s += "metadata_slot_count: ";
      s.Add_UInt32(geom.metadata_slot_count);
      s.Add_LF();

      s += "metadata_max_size: ";
      s.Add_UInt32(geom.metadata_max_size);
      s.Add_LF();

      if (Flags != 0)
      {
        s += "flags: ";
        s += FlagsToString(g_Header_Flags, Z7_ARRAY_SIZE(g_Header_Flags), Flags);
        s.Add_LF();
      }

      if (!GroupsString.IsEmpty())
      {
        s += "Groups:";
        s.Add_LF();
        s += GroupsString;
      }
      
      if (!DevicesString.IsEmpty())
      {
        s += "BlockDevices:";
        s.Add_LF();
        s += DevicesString;
      }
      
      if (!s.IsEmpty())
        prop = s;
      break;
    }

    case kpidName:
      if (!DeviceArcName.IsEmpty())
        prop = DeviceArcName + ".lpimg";
      break;

    case kpidWarningFlags:
      if (_headerWarning)
      {
        UInt32 v = kpv_ErrorFlags_HeadersError;
        prop = v;
      }
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _items.Size();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  const CPartition &item = _items[index];

  switch (propID)
  {
    case kpidPath:
    {
      AString s;
      AddName36ToString(s, item.name, false);
      if (s.IsEmpty())
        s.Add_UInt32(index);
      if (item.num_extents != 0)
      {
        s.Add_Dot();
        s += (item.Ext ? item.Ext : "img");
      }
      prop = s;
      break;
    }
    
    case kpidSize: prop = item.GetSize(); break;
    case kpidPackSize: prop = item.GetPackSize(); break;
    case kpidNumBlocks: prop = item.num_extents; break;
    case kpidMethod:
    {
      const UInt32 m = item.MethodsMask;
      if (m != 0)
      {
        FLAGS_TO_PROP(g_Methods, m, prop);
      }
      break;
    }
    case kpidOffset:
      if (item.num_extents != 0)
        if (item.first_extent_index < Extents.Size())
          prop = Extents[item.first_extent_index].target_data << kSectorSizeLog;
      break;

    case kpidCharacts:
    {
      AString s;
      s += "group:";
      s.Add_UInt32(item.group_index);
      s.Add_Space();
      s += FlagsToString(g_PartitionAttr, Z7_ARRAY_SIZE(g_PartitionAttr), item.attributes);
      prop = s;
      break;
    }
  }

  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}



Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  
  const CPartition &item = _items[index];

  if (item.first_extent_index > Extents.Size()
      || item.num_extents > Extents.Size() - item.first_extent_index)
    return S_FALSE;

  if (item.num_extents == 0)
    return CreateLimitedInStream(_stream, 0, 0, stream);

  if (item.num_extents == 1)
  {
    const CExtent &e = Extents[item.first_extent_index];
    if (e.IsRAW())
    {
      const UInt64 pos = e.target_data << kSectorSizeLog;
      if ((pos >> kSectorSizeLog) != e.target_data)
        return S_FALSE;
      const UInt64 size = item.GetSize();
      if (pos + size < pos)
        return S_FALSE;
      return CreateLimitedInStream(_stream, pos, size, stream);
    }
  }

  CExtentsStream *extentStreamSpec = new CExtentsStream();
  CMyComPtr<ISequentialInStream> extentStream = extentStreamSpec;

  // const unsigned kNumDebugExtents = 10;
  extentStreamSpec->Extents.Reserve(item.num_extents + 1
      // + kNumDebugExtents
      );

  UInt64 virt = 0;
  for (UInt32 k = 0; k < item.num_extents; k++)
  {
    const CExtent &e = Extents[item.first_extent_index + k];

    CSeekExtent se;
    {
      const UInt64 numSectors = e.num_sectors;
      if (numSectors == 0)
      {
        continue;
        // return S_FALSE;
      }
      const UInt64 numBytes = numSectors << kSectorSizeLog;
      if ((numBytes >> kSectorSizeLog) != numSectors)
        return S_FALSE;
      if (numBytes >= ((UInt64)1 << 63) - virt)
        return S_FALSE;
      
      se.Virt = virt;
      virt += numBytes;
    }

    const UInt64 phySector = e.target_data;
    if (e.target_type == LP_TARGET_TYPE_ZERO)
    {
      if (phySector != 0)
        return S_FALSE;
      se.SetAs_ZeroFill();
    }
    else if (e.target_type == LP_TARGET_TYPE_LINEAR)
    {
      se.Phy = phySector << kSectorSizeLog;
      if ((se.Phy >> kSectorSizeLog) != phySector)
        return S_FALSE;
      if (se.Phy >= ((UInt64)1 << 63))
        return S_FALSE;
    }
    else
      return S_FALSE;

    extentStreamSpec->Extents.AddInReserved(se);

    /*
    {
      // for debug
      const UInt64 kAdd = (e.num_sectors << kSectorSizeLog) / kNumDebugExtents;
      for (unsigned i = 0; i < kNumDebugExtents; i++)
      {
        se.Phy += kAdd;
        // se.Phy += (UInt64)1 << 63; // for debug
        // se.Phy += 1; // for debug
        se.Virt += kAdd;
        extentStreamSpec->Extents.AddInReserved(se);
      }
    }
    */
  }

  CSeekExtent se;
  se.Phy = 0;
  se.Virt = virt;
  extentStreamSpec->Extents.Add(se);
  extentStreamSpec->Stream = _stream;
  extentStreamSpec->Init();
  *stream = extentStream.Detach();
  
  return S_OK;
  COM_TRY_END
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
    const UInt32 index = allFilesMode ? i : indices[i];
    totalSize += _items[index].GetSize();
  }
  extractCallback->SetTotal(totalSize);

  totalSize = 0;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = totalSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    CMyComPtr<ISequentialOutStream> outStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    
    RINOK(extractCallback->GetStream(index, &outStream, askMode))

    const UInt64 size = _items[index].GetSize();
    totalSize += size;
    if (!testMode && !outStream)
      continue;
    
    RINOK(extractCallback->PrepareOperation(askMode))

    CMyComPtr<ISequentialInStream> inStream;
    const HRESULT hres = GetStream(index, &inStream);
    int opRes = NExtract::NOperationResult::kUnsupportedMethod;
    if (hres != S_FALSE)
    {
      if (hres != S_OK)
        return hres;
      RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress))
      opRes = NExtract::NOperationResult::kDataError;
      if (copyCoderSpec->TotalSize == size)
        opRes = NExtract::NOperationResult::kOK;
      else if (copyCoderSpec->TotalSize < size)
        opRes = NExtract::NOperationResult::kUnexpectedEnd;
    }
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  
  return S_OK;
  COM_TRY_END
}


REGISTER_ARC_I(
  "LP", "lpimg img", NULL, 0xc1,
  k_Signature,
  LP_PARTITION_RESERVED_BYTES,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/LzhHandler.cpp ================ */
// LzhHandler.cpp

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
using namespace NTime;

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)


// CRC-16 (-IBM, -ANSI). The poly is 0x8005 (x^16 + x^15 + x^2 + 1)

static const UInt16 kCrc16Poly = 0xA001;

MY_ALIGN(64)
static UInt16 g_LzhCrc16Table[256];

#define CRC16_UPDATE_BYTE(crc, b) (g_LzhCrc16Table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

UInt32 LzhCrc16Update(UInt32 crc, const void *data, size_t size);
UInt32 LzhCrc16Update(UInt32 crc, const void *data, size_t size)
{
  const Byte *p = (const Byte *)data;
  const Byte *pEnd = p + size;
  for (; p != pEnd; p++)
    crc = CRC16_UPDATE_BYTE(crc, *p);
  return crc;
}

static struct CLzhCrc16TableInit
{
  CLzhCrc16TableInit()
  {
    for (UInt32 i = 0; i < 256; i++)
    {
      UInt32 r = i;
      for (unsigned j = 0; j < 8; j++)
        r = (r >> 1) ^ (kCrc16Poly & ((UInt32)0 - (r & 1)));
      g_LzhCrc16Table[i] = (UInt16)r;
    }
  }
} g_LzhCrc16TableInit;


namespace NArchive {
namespace NLzh{

const unsigned kMethodIdSize = 5;

const Byte kExtIdFileName = 0x01;
const Byte kExtIdDirName  = 0x02;
const Byte kExtIdUnixTime = 0x54;

struct CExtension
{
  Byte Type;
  CByteBuffer Data;
  
  AString GetString() const
  {
    AString s;
    s.SetFrom_CalcLen((const char *)(const Byte *)Data, (unsigned)Data.Size());
    return s;
  }
};

const UInt32 kBasicPartSize = 22;

API_FUNC_static_IsArc IsArc_Lzh(const Byte *p, size_t size)
{
  if (size < 2 + kBasicPartSize)
    return k_IsArc_Res_NEED_MORE;
  if (p[2] != '-' || p[3] != 'l'  || p[4] != 'h' || p[6] != '-')
    return k_IsArc_Res_NO;
  Byte n = p[5];
  if (n != 'd')
    if (n < '0' || n > '7')
      return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}

struct CItem
{
  AString Name;
  Byte Method[kMethodIdSize];
  Byte Attributes;
  Byte Level;
  Byte OsId;
  UInt32 PackSize;
  UInt32 Size;
  UInt32 ModifiedTime;
  UInt16 CRC;
  CObjectVector<CExtension> Extensions;

  bool IsValidMethod() const  { return (Method[0] == '-' && Method[1] == 'l' && Method[4] == '-'); }
  bool IsLhMethod() const  {return (IsValidMethod() && Method[2] == 'h'); }
  bool IsDir() const {return (IsLhMethod() && Method[3] == 'd'); }

  bool IsCopyMethod() const
  {
    return (IsLhMethod() && Method[3] == '0') ||
      (IsValidMethod() && Method[2] == 'z' && Method[3] == '4');
  }
  
  bool IsLh1GroupMethod() const
  {
    if (!IsLhMethod())
      return false;
    switch (Method[3])
    {
      case '1':
        return true;
    }
    return false;
  }
  
  bool IsLh4GroupMethod() const
  {
    if (!IsLhMethod())
      return false;
    switch (Method[3])
    {
      case '4':
      case '5':
      case '6':
      case '7':
        return true;
    }
    return false;
  }
  
  unsigned GetNumDictBits() const
  {
    if (!IsLhMethod())
      return 0;
    switch (Method[3])
    {
      case '1': return 12;
      case '2': return 13;
      case '3': return 13;
      case '4': return 12;
      case '5': return 13;
      case '6': return 15;
      case '7': return 16;
    }
    return 0;
  }

  int FindExt(Byte type) const
  {
    FOR_VECTOR (i, Extensions)
      if (Extensions[i].Type == type)
        return (int)i;
    return -1;
  }
  
  bool GetUnixTime(UInt32 &value) const
  {
    value = 0;
    int index = FindExt(kExtIdUnixTime);
    if (index < 0
        || Extensions[index].Data.Size() < 4)
    {
      if (Level == 2)
      {
        value = ModifiedTime;
        return true;
      }
      return false;
    }
    const Byte *data = (const Byte *)(Extensions[index].Data);
    value = GetUi32(data);
    return true;
  }

  AString GetDirName() const
  {
    int index = FindExt(kExtIdDirName);
    if (index < 0)
      return AString();
    return Extensions[index].GetString();
  }

  AString GetFileName() const
  {
    int index = FindExt(kExtIdFileName);
    if (index < 0)
      return Name;
    return Extensions[index].GetString();
  }

  AString GetName() const
  {
    AString s (GetDirName());
    const char kDirSeparator = '\\';
    // check kDirSeparator in Linux
    s.Replace((char)(Byte)0xFF, kDirSeparator);
    if (!s.IsEmpty() && s.Back() != kDirSeparator)
      s += kDirSeparator;
    s += GetFileName();
    return s;
  }
};

static const Byte *ReadUInt16(const Byte *p, UInt16 &v)
{
  v = Get16(p);
  return p + 2;
}

static const Byte *ReadString(const Byte *p, size_t size, AString &s)
{
  s.Empty();
  for (size_t i = 0; i < size; i++)
  {
    const Byte c = p[i];
    if (c == 0)
      break;
    s += (char)c;
  }
  return p + size;
}

static Byte CalcSum(const Byte *data, size_t size)
{
  Byte sum = 0;
  for (size_t i = 0; i < size; i++)
    sum = (Byte)(sum + data[i]);
  return sum;
}

static HRESULT GetNextItem(ISequentialInStream *stream, bool &filled, CItem &item)
{
  filled = false;

  size_t processedSize = 2;
  Byte startHeader[2];
  RINOK(ReadStream(stream, startHeader, &processedSize))
  if (processedSize == 0)
    return S_OK;
  if (processedSize == 1)
    return (startHeader[0] == 0) ? S_OK: S_FALSE;
  if (startHeader[0] == 0 && startHeader[1] == 0)
    return S_OK;

  Byte header[256];
  processedSize = kBasicPartSize;
  RINOK(ReadStream(stream, header, &processedSize))
  if (processedSize != kBasicPartSize)
    return (startHeader[0] == 0) ? S_OK: S_FALSE;

  const Byte *p = header;
  memcpy(item.Method, p, kMethodIdSize);
  if (!item.IsValidMethod())
    return S_OK;
  p += kMethodIdSize;
  item.PackSize = Get32(p);
  item.Size = Get32(p + 4);
  item.ModifiedTime = Get32(p + 8);
  item.Attributes = p[12];
  item.Level = p[13];
  p += 14;
  if (item.Level > 2)
    return S_FALSE;
  UInt32 headerSize;
  if (item.Level < 2)
  {
    headerSize = startHeader[0];
    if (headerSize < kBasicPartSize)
      return S_FALSE;
    RINOK(ReadStream_FALSE(stream, header + kBasicPartSize, headerSize - kBasicPartSize))
    if (startHeader[1] != CalcSum(header, headerSize))
      return S_FALSE;
    const size_t nameLength = *p++;
    if ((size_t)(p - header) + nameLength + 2 > headerSize)
      return S_FALSE;
    p = ReadString(p, nameLength, item.Name);
  }
  else
    headerSize = startHeader[0] | ((UInt32)startHeader[1] << 8);
  p = ReadUInt16(p, item.CRC);
  if (item.Level != 0)
  {
    if (item.Level == 2)
    {
      RINOK(ReadStream_FALSE(stream, header + kBasicPartSize, 2))
    }
    if ((size_t)(p - header) + 3 > headerSize)
      return S_FALSE;
    item.OsId = *p++;
    UInt16 nextSize;
    p = ReadUInt16(p, nextSize);
    while (nextSize != 0)
    {
      if (nextSize < 3)
        return S_FALSE;
      if (item.Level == 1)
      {
        if (item.PackSize < nextSize)
          return S_FALSE;
        item.PackSize -= nextSize;
      }
      if (item.Extensions.Size() >= (1 << 8))
        return S_FALSE;
      CExtension ext;
      RINOK(ReadStream_FALSE(stream, &ext.Type, 1))
      nextSize = (UInt16)(nextSize - 3);
      ext.Data.Alloc(nextSize);
      RINOK(ReadStream_FALSE(stream, (Byte *)ext.Data, nextSize))
      item.Extensions.Add(ext);
      Byte hdr2[2];
      RINOK(ReadStream_FALSE(stream, hdr2, 2))
      ReadUInt16(hdr2, nextSize);
    }
  }
  filled = true;
  return S_OK;
}


static const CUInt32PCharPair g_OsPairs[] =
{
  {   0, "MS-DOS" },
  { 'M', "MS-DOS" },
  { '2', "OS/2" },
  { '9', "OS9" },
  { 'K', "OS/68K" },
  { '3', "OS/386" },
  { 'H', "HUMAN" },
  { 'U', "UNIX" },
  { 'C', "CP/M" },
  { 'F', "FLEX" },
  { 'm', "Mac" },
  { 'R', "Runser" },
  { 'T', "TownsOS" },
  { 'X', "XOSK" },
  { 'w', "Windows 95" },
  { 'W', "Windows NT" },
  { 'J', "Java VM" }
};


static const Byte kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidMTime,
  // kpidAttrib,
  kpidCRC,
  kpidMethod,
  kpidHostOS
};


Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithCRC
  , ISequentialOutStream
)
  UInt32 _crc;
  CMyComPtr<ISequentialOutStream> _stream;
public:
  void Init(ISequentialOutStream *stream)
  {
    _stream = stream;
    _crc = 0;
  }
  void ReleaseStream() { _stream.Release(); }
  UInt32 GetCRC() const { return _crc; }
};

Z7_COM7F_IMF(COutStreamWithCRC::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  HRESULT res = S_OK;
  if (_stream)
    res = _stream->Write(data, size, &size);
  _crc = LzhCrc16Update(_crc, data, size);
  if (processedSize)
    *processedSize = size;
  return res;
}


struct CItemEx: public CItem
{
  UInt64 DataPosition;
};


Z7_CLASS_IMP_CHandler_IInArchive_0

  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _stream;
  UInt64 _phySize;
  UInt32 _errorFlags;
  bool _isArc;
public:
  CHandler();
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

CHandler::CHandler() {}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _items.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: prop = _phySize; break;
   
    case kpidErrorFlags:
      UInt32 v = _errorFlags;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      prop = v;
      break;
  }
  prop.Detach(value);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  const CItemEx &item = _items[index];
  switch (propID)
  {
    case kpidPath:
    {
      UString s = NItemName::WinPathToOsPath(MultiByteToUnicodeString(item.GetName(), CP_OEMCP));
      if (!s.IsEmpty())
      {
        if (s.Back() == WCHAR_PATH_SEPARATOR)
          s.DeleteBack();
        prop = s;
      }
      break;
    }
    case kpidIsDir:  prop = item.IsDir(); break;
    case kpidSize:   prop = (UInt64)item.Size; break;
    case kpidPackSize:  prop = (UInt64)item.PackSize; break;
    case kpidCRC:  prop = (UInt32)item.CRC; break;
    case kpidHostOS:  PAIR_TO_PROP(g_OsPairs, item.OsId, prop); break;
    case kpidMTime:
    {
      UInt32 unixTime;
      if (item.GetUnixTime(unixTime))
        PropVariant_SetFrom_UnixTime(prop, unixTime);
      else
        PropVariant_SetFrom_DosTime(prop, item.ModifiedTime);
      break;
    }
    // case kpidAttrib:  prop = (UInt32)item.Attributes; break;
    case kpidMethod:
    {
      char method2[kMethodIdSize + 1];
      method2[kMethodIdSize] = 0;
      memcpy(method2, item.Method, kMethodIdSize);
      prop = method2;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  Close();
  try
  {
    _items.Clear();

    UInt64 endPos;
    bool needSetTotal = true;

    RINOK(InStream_AtBegin_GetSize(stream, endPos))

    for (;;)
    {
      CItemEx item;
      bool filled;
      const HRESULT res = GetNextItem(stream, filled, item);
      RINOK(InStream_GetPos(stream, item.DataPosition))
      if (res == S_FALSE)
      {
        _errorFlags = kpv_ErrorFlags_HeadersError;
        break;
      }

      if (res != S_OK)
        return S_FALSE;
      _phySize = item.DataPosition;
      if (!filled)
        break;
      _items.Add(item);

      _isArc = true;

      UInt64 newPostion;
      RINOK(stream->Seek(item.PackSize, STREAM_SEEK_CUR, &newPostion))
      if (newPostion > endPos)
      {
        _phySize = endPos;
        _errorFlags = kpv_ErrorFlags_UnexpectedEnd;
        break;
      }
      _phySize = newPostion;
      if (callback)
      {
        if (needSetTotal)
        {
          RINOK(callback->SetTotal(NULL, &endPos))
          needSetTotal = false;
        }
        if (_items.Size() % 100 == 0)
        {
          const UInt64 numFiles = _items.Size();
          const UInt64 numBytes = item.DataPosition;
          RINOK(callback->SetCompleted(&numFiles, &numBytes))
        }
      }
    }
    if (_items.IsEmpty())
      return S_FALSE;

    _stream = stream;
  }
  catch(...)
  {
    return S_FALSE;
  }
  COM_TRY_END
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Close())
{
  _isArc = false;
  _phySize = 0;
  _errorFlags = 0;
  _items.Clear();
  _stream.Release();
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
  UInt64 totalUnPacked = 0 /* , totalPacked = 0 */;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const CItemEx &item = _items[allFilesMode ? i : indices[i]];
    totalUnPacked += item.Size;
    // totalPacked += item.PackSize;
  }
  RINOK(extractCallback->SetTotal(totalUnPacked))

  UInt32 cur_Unpacked, cur_Packed;

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyUniquePtr<NCompress::NLzh::NDecoder::CCoder> lzhDecoder;
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> inStream;
  inStream->SetStream(_stream);

  for (i = 0;; i++,
      lps->OutSize += cur_Unpacked,
      lps->InSize += cur_Packed)
  {
    cur_Unpacked = 0;
    cur_Packed = 0;
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
      const CItemEx &item = _items[index];
      RINOK(extractCallback->GetStream(index, &realOutStream, askMode))
        
      if (item.IsDir())
      {
        // if (!testMode)
        {
          RINOK(extractCallback->PrepareOperation(askMode))
          RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
        }
        continue;
      }
      
      if (!testMode && !realOutStream)
        continue;
      
      RINOK(extractCallback->PrepareOperation(askMode))
      cur_Unpacked = item.Size;
      cur_Packed = item.PackSize;

      CMyComPtr2_Create<ISequentialOutStream, COutStreamWithCRC> outStream;
      outStream->Init(realOutStream);
      realOutStream.Release();
      
      RINOK(InStream_SeekSet(_stream, item.DataPosition))

      inStream->Init(item.PackSize);

      HRESULT res = S_OK;
      opRes = NExtract::NOperationResult::kOK;

      if (item.IsCopyMethod())
      {
        res = copyCoder.Interface()->Code(inStream, outStream, NULL, NULL, lps);
        if (res == S_OK && copyCoder->TotalSize != item.PackSize)
          res = S_FALSE;
      }
      else if (item.IsLh4GroupMethod())
      {
        lzhDecoder.Create_if_Empty();
        // lzhDecoder->FinishMode = true;
        lzhDecoder->SetDictSize((UInt32)1 << item.GetNumDictBits());
        res = lzhDecoder->Code(inStream, outStream, cur_Unpacked, lps);
        if (res == S_OK && lzhDecoder->GetInputProcessedSize() != item.PackSize)
          res = S_FALSE;
      }
      /*
      else if (item.IsLh1GroupMethod())
      {
        if (!lzh1Decoder)
        {
          lzh1DecoderSpec = new NCompress::NLzh1::NDecoder::CCoder;
          lzh1Decoder = lzh1DecoderSpec;
        }
        lzh1DecoderSpec->SetDictionary(item.GetNumDictBits());
        res = lzh1Decoder->Code(inStream, outStream, NULL, &cur_Unpacked, progress);
      }
      */
      else
        opRes = NExtract::NOperationResult::kUnsupportedMethod;

      if (opRes == NExtract::NOperationResult::kOK)
      {
        if (res == S_FALSE)
          opRes = NExtract::NOperationResult::kDataError;
        else
        {
          RINOK(res)
          if (outStream->GetCRC() != item.CRC)
            opRes = NExtract::NOperationResult::kCRCError;
        }
      }
    }
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  return S_OK;
  COM_TRY_END
}

static const Byte k_Signature[] = { '-', 'l', 'h' };

REGISTER_ARC_I(
  "Lzh", "lzh lha", NULL, 6,
  k_Signature,
  2,
  0,
  IsArc_Lzh)

}}

/* ================ unit: CPP/7zip/Archive/LzmaHandler.cpp ================ */
// LzmaHandler.cpp

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
namespace NLzma {

static bool CheckDicSize(const Byte *p)
{
  UInt32 dicSize = GetUi32(p);
  if (dicSize == 1)
    return true;
  for (unsigned i = 0; i <= 30; i++)
    if (dicSize == ((UInt32)2 << i) || dicSize == ((UInt32)3 << i))
      return true;
  return (dicSize == 0xFFFFFFFF);
}

static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize,
  kpidMethod
};

static const Byte kArcProps[] =
{
  kpidNumStreams,
  kpidMethod
};

struct CHeader
{
  UInt64 Size;
  Byte FilterID;
  Byte LzmaProps[5];

  Byte GetProp() const { return LzmaProps[0]; }
  UInt32 GetDicSize() const { return GetUi32(LzmaProps + 1); }
  bool HasSize() const { return (Size != (UInt64)(Int64)-1); }
  bool Parse(const Byte *buf, bool isThereFilter);
};

bool CHeader::Parse(const Byte *buf, bool isThereFilter)
{
  FilterID = 0;
  if (isThereFilter)
    FilterID = buf[0];
  const Byte *sig = buf + (isThereFilter ? 1 : 0);
  for (int i = 0; i < 5; i++)
    LzmaProps[i] = sig[i];
  Size = GetUi64(sig + 5);
  return
    LzmaProps[0] < 5 * 5 * 9 &&
    FilterID < 2 &&
    (!HasSize() || Size < ((UInt64)1 << 56))
    && CheckDicSize(LzmaProps + 1);
}

class CDecoder Z7_final
{
  CMyComPtr<ISequentialOutStream> _bcjStream;
  CFilterCoder *_filterCoder;
public:
  CMyComPtr2<ICompressCoder, NCompress::NLzma::CDecoder> _lzmaDecoder;

  ~CDecoder();
  HRESULT Create(bool filtered, ISequentialInStream *inStream);

  HRESULT Code(const CHeader &header, ISequentialOutStream *outStream, ICompressProgressInfo *progress);

  UInt64 GetInputProcessedSize() const { return _lzmaDecoder->GetInputProcessedSize(); }

  void ReleaseInStream() { if (_lzmaDecoder) _lzmaDecoder->ReleaseInStream(); }

  HRESULT ReadInput(Byte *data, UInt32 size, UInt32 *processedSize)
    { return _lzmaDecoder->ReadFromInputStream(data, size, processedSize); }
};

HRESULT CDecoder::Create(bool filteredMode, ISequentialInStream *inStream)
{
  _lzmaDecoder.Create_if_Empty();
  _lzmaDecoder->FinishStream = true;

  if (filteredMode)
  {
    if (!_bcjStream)
    {
      _filterCoder = new CFilterCoder(false);
      CMyComPtr<ICompressCoder> coder = _filterCoder;
      _filterCoder->Filter = new NCompress::NBcj::CCoder2(z7_BranchConvSt_X86_Dec);
      _bcjStream = _filterCoder;
    }
  }

  return _lzmaDecoder->SetInStream(inStream);
}

CDecoder::~CDecoder()
{
  ReleaseInStream();
}

HRESULT CDecoder::Code(const CHeader &header, ISequentialOutStream *outStream,
    ICompressProgressInfo *progress)
{
  if (header.FilterID > 1)
    return E_NOTIMPL;

  RINOK(_lzmaDecoder->SetDecoderProperties2(header.LzmaProps, 5))

  bool filteredMode = (header.FilterID == 1);

  if (filteredMode)
  {
    RINOK(_filterCoder->SetOutStream(outStream))
    outStream = _bcjStream;
    RINOK(_filterCoder->SetOutStreamSize(NULL))
  }

  const UInt64 *Size = header.HasSize() ? &header.Size : NULL;
  HRESULT res = _lzmaDecoder->CodeResume(outStream, Size, progress);

  if (filteredMode)
  {
    {
      HRESULT res2 = _filterCoder->OutStreamFinish();
      if (res == S_OK)
        res = res2;
    }
    HRESULT res2 = _filterCoder->ReleaseOutStream();
    if (res == S_OK)
      res = res2;
  }
  
  RINOK(res)

  if (header.HasSize())
    if (_lzmaDecoder->GetOutputProcessedSize() != header.Size)
      return S_FALSE;

  return S_OK;
}


Z7_CLASS_IMP_CHandler_IInArchive_1(
  IArchiveOpenSeq
)
  bool _lzma86;
  bool _isArc;
  bool _needSeekToStart;
  bool _dataAfterEnd;
  bool _needMoreInput;
  bool _unsupported;
  bool _dataError;

  bool _packSize_Defined;
  bool _unpackSize_Defined;
  bool _numStreams_Defined;

  CHeader _header;
  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;
  
  UInt64 _packSize;
  UInt64 _unpackSize;
  UInt64 _numStreams;

  void GetMethod(NCOM::CPropVariant &prop);

  unsigned GetHeaderSize() const { return 5 + 8 + (_lzma86 ? 1 : 0); }
public:
  CHandler(bool lzma86) { _lzma86 = lzma86; }
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: if (_packSize_Defined) prop = _packSize; break;
    case kpidNumStreams: if (_numStreams_Defined) prop = _numStreams; break;
    case kpidUnpackSize: if (_unpackSize_Defined) prop = _unpackSize; break;
    case kpidMethod: GetMethod(prop); break;
    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_needMoreInput) v |= kpv_ErrorFlags_UnexpectedEnd;
      if (_dataAfterEnd) v |= kpv_ErrorFlags_DataAfterEnd;
      if (_unsupported) v |= kpv_ErrorFlags_UnsupportedMethod;
      if (_dataError) v |= kpv_ErrorFlags_DataError;
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


static char * DictSizeToString(UInt32 val, char *s)
{
  for (unsigned i = 0; i < 32; i++)
    if (((UInt32)1 << i) == val)
      return ::ConvertUInt32ToString(i, s);
  char c = 'b';
       if ((val & ((1 << 20) - 1)) == 0) { val >>= 20; c = 'm'; }
  else if ((val & ((1 << 10) - 1)) == 0) { val >>= 10; c = 'k'; }
  s = ::ConvertUInt32ToString(val, s);
  *s++ = c;
  *s = 0;
  return s;
}

static char *AddProp32(char *s, const char *name, UInt32 v)
{
  *s++ = ':';
  s = MyStpCpy(s, name);
  return ::ConvertUInt32ToString(v, s);
}

void CHandler::GetMethod(NCOM::CPropVariant &prop)
{
  if (!_stream)
    return;

  char sz[64];
  char *s = sz;
  if (_header.FilterID != 0)
    s = MyStpCpy(s, "BCJ ");
  s = MyStpCpy(s, "LZMA:");
  s = DictSizeToString(_header.GetDicSize(), s);
  
  UInt32 d = _header.GetProp();
  // if (d != 0x5D)
  {
    UInt32 lc = d % 9;
    d /= 9;
    UInt32 pb = d / 5;
    UInt32 lp = d % 5;
    if (lc != 3) s = AddProp32(s, "lc", lc);
    if (lp != 0) s = AddProp32(s, "lp", lp);
    if (pb != 2) s = AddProp32(s, "pb", pb);
  }
  prop = sz;
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidSize: if (_stream && _header.HasSize()) prop = _header.Size; break;
    case kpidPackSize: if (_packSize_Defined) prop = _packSize; break;
    case kpidMethod: GetMethod(prop); break;
    default: break;
  }
  prop.Detach(value);
  return S_OK;
}

API_FUNC_static_IsArc IsArc_Lzma(const Byte *p, size_t size)
{
  const UInt32 kHeaderSize = 1 + 4 + 8;
  if (size < kHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] >= 5 * 5 * 9)
    return k_IsArc_Res_NO;
  const UInt64 unpackSize = GetUi64(p + 1 + 4);
  if (unpackSize != (UInt64)(Int64)-1)
  {
    if (unpackSize >= ((UInt64)1 << 56))
      return k_IsArc_Res_NO;
  }
  if (unpackSize != 0)
  {
    if (size < kHeaderSize + 2)
      return k_IsArc_Res_NEED_MORE;
    if (p[kHeaderSize] != 0)
      return k_IsArc_Res_NO;
    if (unpackSize != (UInt64)(Int64)-1)
    {
      if ((p[kHeaderSize + 1] & 0x80) != 0)
        return k_IsArc_Res_NO;
    }
  }
  if (!CheckDicSize(p + 1))
    // return k_IsArc_Res_YES_LOW_PROB;
    return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}

API_FUNC_static_IsArc IsArc_Lzma86(const Byte *p, size_t size)
{
  if (size < 1)
    return k_IsArc_Res_NEED_MORE;
  Byte filterID = p[0];
  if (filterID != 0 && filterID != 1)
    return k_IsArc_Res_NO;
  return IsArc_Lzma(p + 1, size - 1);
}
}



Z7_COM7F_IMF(CHandler::Open(IInStream *inStream, const UInt64 *, IArchiveOpenCallback *))
{
  Close();
  
  const unsigned headerSize = GetHeaderSize();
  const UInt32 kBufSize = 1 << 7;
  Byte buf[kBufSize];
  size_t processedSize = kBufSize;
  RINOK(ReadStream(inStream, buf, &processedSize))
  if (processedSize < headerSize + 2)
    return S_FALSE;
  if (!_header.Parse(buf, _lzma86))
    return S_FALSE;
  const Byte *start = buf + headerSize;
  if (start[0] != 0 /* || (start[1] & 0x80) != 0 */ ) // empty stream with EOS is not 0x80
    return S_FALSE;

  RINOK(InStream_GetSize_SeekToEnd(inStream, _packSize))

  SizeT srcLen = (SizeT)processedSize - headerSize;

  if (srcLen > 10
      && _header.Size == 0
      // && _header.FilterID == 0
      && _header.LzmaProps[0] == 0
      )
    return S_FALSE;

  const UInt32 outLimit = 1 << 11;
  Byte outBuf[outLimit];

  SizeT outSize = outLimit;
  if (outSize > _header.Size)
    outSize = (SizeT)_header.Size;
  SizeT destLen = outSize;
  ELzmaStatus status;
  
  SRes res = LzmaDecode(outBuf, &destLen, start, &srcLen,
      _header.LzmaProps, 5, LZMA_FINISH_ANY,
      &status, &g_Alloc);
  
  if (res != SZ_OK)
    if (res != SZ_ERROR_INPUT_EOF)
      return S_FALSE;

  _isArc = true;
  _stream = inStream;
  _seqStream = inStream;
  _needSeekToStart = true;
  return S_OK;
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
  _dataAfterEnd = false;
  _needMoreInput = false;
  _unsupported = false;
  _dataError = false;

  _packSize_Defined = false;
  _unpackSize_Defined = false;
  _numStreams_Defined = false;

  _packSize = 0;

  _stream.Release();
  _seqStream.Release();
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
    const UInt64 val = Offset + *inSize;
    return Callback->SetCompleted(&files, &val);
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

  if (_packSize_Defined)
    RINOK(extractCallback->SetTotal(_packSize))
    
  Int32 opResult;
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
  realOutStream.Release();

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

  CDecoder decoder;
  RINOK(decoder.Create(_lzma86, _seqStream))
 
  bool firstItem = true;

  UInt64 packSize = 0;
  UInt64 unpackSize = 0;
  UInt64 numStreams = 0;

  bool dataAfterEnd = false;
  
  HRESULT hres = S_OK;

  for (;;)
  {
    lps->InSize = packSize;
    lps->OutSize = unpackSize;
    RINOK(lps->SetCur())

    const UInt32 kBufSize = 1 + 5 + 8;
    Byte buf[kBufSize];
    const UInt32 headerSize = GetHeaderSize();
    UInt32 processed;
    RINOK(decoder.ReadInput(buf, headerSize, &processed))
    if (processed != headerSize)
    {
      if (processed != 0)
        dataAfterEnd = true;
      break;
    }
  
    CHeader st;
    if (!st.Parse(buf, _lzma86))
    {
      dataAfterEnd = true;
      break;
    }
    numStreams++;
    firstItem = false;

    hres = decoder.Code(st, outStream, lps);

    packSize = decoder.GetInputProcessedSize();
    unpackSize = outStream->GetSize();
    
    if (hres == E_NOTIMPL)
    {
      _unsupported = true;
      hres = S_FALSE;
      break;
    }
    if (hres == S_FALSE)
      break;
    RINOK(hres)
  }

  if (firstItem)
  {
    _isArc = false;
    hres = S_FALSE;
  }
  else if (hres == S_OK || hres == S_FALSE)
  {
    if (dataAfterEnd)
      _dataAfterEnd = true;
    else if (decoder._lzmaDecoder->NeedsMoreInput())
      _needMoreInput = true;

    _packSize = packSize;
    _unpackSize = unpackSize;
    _numStreams = numStreams;
  
    _packSize_Defined = true;
    _unpackSize_Defined = true;
    _numStreams_Defined = true;
  }
  
  opResult = NExtract::NOperationResult::kOK;

  if (!_isArc)
    opResult = NExtract::NOperationResult::kIsNotArc;
  else if (_needMoreInput)
    opResult = NExtract::NOperationResult::kUnexpectedEnd;
  else if (_unsupported)
    opResult = NExtract::NOperationResult::kUnsupportedMethod;
  else if (_dataAfterEnd)
    opResult = NExtract::NOperationResult::kDataAfterEnd;
  else if (hres == S_FALSE)
    opResult = NExtract::NOperationResult::kDataError;
  else if (hres == S_OK)
    opResult = NExtract::NOperationResult::kOK;
  else
    return hres;

  // outStream.Release();
 }
  return extractCallback->SetOperationResult(opResult);

  COM_TRY_END
}

namespace NLzmaAr {

// 2, { 0x5D, 0x00 },

REGISTER_ARC_I_CLS_NO_SIG(
  CHandler(false),
  "lzma", "lzma", NULL, 0xA,
  0,
  NArcInfoFlags::kStartOpen |
  NArcInfoFlags::kKeepName,
  IsArc_Lzma)
 
}

namespace NLzma86Ar {

REGISTER_ARC_I_CLS_NO_SIG(
  CHandler(true),
  "lzma86", "lzma86", NULL, 0xB,
  0,
  NArcInfoFlags::kKeepName,
  IsArc_Lzma86)
 
}

}}
