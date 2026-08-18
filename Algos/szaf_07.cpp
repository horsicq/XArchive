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

/* ---- C/SwapBytes.h ---- */
/* SwapBytes.h -- Byte Swap conversion filter
2023-04-02 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_SWAP_BYTES_H
#define ZIP7_INC_SWAP_BYTES_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

void z7_SwapBytes2(UInt16 *data, size_t numItems);
void z7_SwapBytes4(UInt32 *data, size_t numItems);
void z7_SwapBytesPrepare(void);

EXTERN_C_END

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

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Archive/MachoHandler.cpp ================ */
// MachoHandler.cpp

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

static UInt32 Get32(const Byte *p, bool be) { if (be) return GetBe32(p); return GetUi32(p); }
static UInt64 Get64(const Byte *p, bool be) { if (be) return GetBe64(p); return GetUi64(p); }

using namespace NWindows;
using namespace NCOM;

namespace NArchive {
namespace NMacho {

#define CPU_ARCH_ABI64 (1 << 24)
#define CPU_TYPE_386    7
#define CPU_TYPE_ARM   12
#define CPU_TYPE_SPARC 14
#define CPU_TYPE_PPC   18

#define CPU_SUBTYPE_I386_ALL 3

// #define CPU_TYPE_PPC64 (CPU_ARCH_ABI64 | CPU_TYPE_PPC)
#define CPU_TYPE_AMD64 (CPU_ARCH_ABI64 | CPU_TYPE_386)
#define CPU_TYPE_ARM64 (CPU_ARCH_ABI64 | CPU_TYPE_ARM)

#define CPU_SUBTYPE_LIB64 ((UInt32)1 << 31)

#define CPU_SUBTYPE_POWERPC_970 100

static const char * const k_PowerPc_SubTypes[] =
{
  NULL
  , "601"
  , "602"
  , "603"
  , "603e"
  , "603ev"
  , "604"
  , "604e"
  , "620"
  , "750"
  , "7400"
  , "7450"
};

static const CUInt32PCharPair g_CpuPairs[] =
{
  { CPU_TYPE_AMD64, "x64" },
  { CPU_TYPE_ARM64, "ARM64" },
  { CPU_TYPE_386, "x86" },
  { CPU_TYPE_ARM, "ARM" },
  { CPU_TYPE_SPARC, "SPARC" },
  { CPU_TYPE_PPC, "PowerPC" }
};


#define CMD_SEGMENT_32 1
#define CMD_SEGMENT_64 0x19

#define SECT_TYPE_MASK 0x000000FF
#define SECT_ATTR_MASK 0xFFFFFF00

#define SECT_ATTR_ZEROFILL 1

static const char * const g_SectTypes[] =
{
    "REGULAR"
  , "ZEROFILL"
  , "CSTRINGS"
  , "4BYTE_LITERALS"
  , "8BYTE_LITERALS"
  , "LITERAL_POINTERS"
  , "NON_LAZY_SYMBOL_POINTERS"
  , "LAZY_SYMBOL_POINTERS"
  , "SYMBOL_STUBS"
  , "MOD_INIT_FUNC_POINTERS"
  , "MOD_TERM_FUNC_POINTERS"
  , "COALESCED"
  , "GB_ZEROFILL"
  , "INTERPOSING"
  , "16BYTE_LITERALS"
  , "DTRACE_DOF"
  , "LAZY_DYLIB_SYMBOL_POINTERS"
  , "THREAD_LOCAL_REGULAR"
  , "THREAD_LOCAL_ZEROFILL"
  , "THREAD_LOCAL_VARIABLES"
  , "THREAD_LOCAL_VARIABLE_POINTERS"
  , "THREAD_LOCAL_INIT_FUNCTION_POINTERS"
};

enum EFileType
{
  kType_OBJECT = 1,
  kType_EXECUTE,
  kType_FVMLIB,
  kType_CORE,
  kType_PRELOAD,
  kType_DYLIB,
  kType_DYLINKER,
  kType_BUNDLE,
  kType_DYLIB_STUB,
  kType_DSYM
};

static const char * const g_FileTypes[] =
{
    "0"
  , "OBJECT"
  , "EXECUTE"
  , "FVMLIB"
  , "CORE"
  , "PRELOAD"
  , "DYLIB"
  , "DYLINKER"
  , "BUNDLE"
  , "DYLIB_STUB"
  , "DSYM"
};


static const char * const g_ArcFlags[] =
{
    "NOUNDEFS"
  , "INCRLINK"
  , "DYLDLINK"
  , "BINDATLOAD"
  , "PREBOUND"
  , "SPLIT_SEGS"
  , "LAZY_INIT"
  , "TWOLEVEL"
  , "FORCE_FLAT"
  , "NOMULTIDEFS"
  , "NOFIXPREBINDING"
  , "PREBINDABLE"
  , "ALLMODSBOUND"
  , "SUBSECTIONS_VIA_SYMBOLS"
  , "CANONICAL"
  , "WEAK_DEFINES"
  , "BINDS_TO_WEAK"
  , "ALLOW_STACK_EXECUTION"
  , "ROOT_SAFE"
  , "SETUID_SAFE"
  , "NO_REEXPORTED_DYLIBS"
  , "PIE"
  , "DEAD_STRIPPABLE_DYLIB"
  , "HAS_TLV_DESCRIPTORS"
  , "NO_HEAP_EXECUTION"
};


static const CUInt32PCharPair g_SectFlags[] =
{
  { 31, "PURE_INSTRUCTIONS" },
  { 30, "NO_TOC" },
  { 29, "STRIP_STATIC_SYMS" },
  { 28, "NO_DEAD_STRIP" },
  { 27, "LIVE_SUPPORT" },
  { 26, "SELF_MODIFYING_CODE" },
  { 25, "DEBUG" },
  { 10, "SOME_INSTRUCTIONS" },
  {  9, "EXT_RELOC" },
  {  8, "LOC_RELOC" }
};


// VM_PROT_*
static const char * const g_SegmentProt[] =
{
    "READ"
  , "WRITE"
  , "EXECUTE"
  /*
  , "NO_CHANGE"
  , "COPY"
  , "TRUSTED"
  , "IS_MASK"
  */
};

// SG_*

static const char * const g_SegmentFlags[] =
{
    "SG_HIGHVM"
  , "SG_FVMLIB"
  , "SG_NORELOC"
  , "SG_PROTECTED_VERSION_1"
  , "SG_READ_ONLY"
};

static const unsigned kNameSize = 16;

struct CSegment
{
  char Name[kNameSize];
  UInt32 MaxProt;
  UInt32 InitProt;
  UInt32 Flags;
};

struct CSection
{
  char Name[kNameSize];
  // char SegName[kNameSize];
  UInt64 Va;
  UInt64 Pa;
  UInt64 VSize;
  UInt64 PSize;

  UInt32 Align;
  UInt32 Flags;
  unsigned SegmentIndex;
  bool IsDummy;

  CSection(): IsDummy(false) {}
  // UInt64 GetPackSize() const { return Flags == SECT_ATTR_ZEROFILL ? 0 : Size; }
  UInt64 GetPackSize() const { return PSize; }
};


Z7_CLASS_IMP_CHandler_IInArchive_1(
  IArchiveAllowTail
)
  CMyComPtr<IInStream> _inStream;
  CObjectVector<CSegment> _segments;
  CObjectVector<CSection> _sections;
  bool _allowTail;
  bool _mode64;
  bool _be;
  UInt32 _cpuType;
  UInt32 _cpuSubType;
  UInt32 _type;
  UInt32 _flags;
  UInt32 _headersSize;
  UInt64 _totalSize;

  HRESULT Open2(ISequentialInStream *stream);
public:
  CHandler(): _allowTail(false) {}
};

static const Byte kArcProps[] =
{
  kpidCpu,
  kpidBit64,
  kpidBigEndian,
  kpidCharacts,
  kpidHeadersSize
};

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidPackSize,
  kpidCharacts,
  kpidOffset,
  kpidVa,
  kpidClusterSize // Align
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  CPropVariant prop;
  switch (propID)
  {
    case kpidShortComment:
    case kpidCpu:
    {
      AString s;
      const UInt32 cpu = _cpuType & ~(UInt32)CPU_ARCH_ABI64;
      UInt32 flag64 = _cpuType & (UInt32)CPU_ARCH_ABI64;
      {
        s.Add_UInt32(cpu);
        for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_CpuPairs); i++)
        {
          const CUInt32PCharPair &pair = g_CpuPairs[i];
          if (pair.Value == cpu || pair.Value == _cpuType)
          {
            if (pair.Value == _cpuType)
              flag64 = 0;
            s = pair.Name;
            break;
          }
        }
       
        if (flag64 != 0)
          s.Add_OptSpaced("64-bit");
        else if ((_cpuSubType & CPU_SUBTYPE_LIB64) && _cpuType != CPU_TYPE_AMD64)
          s.Add_OptSpaced("64-bit-lib");
      }
      const UInt32 t = _cpuSubType & ~(UInt32)CPU_SUBTYPE_LIB64;
      if (t != 0 && (t != CPU_SUBTYPE_I386_ALL || cpu != CPU_TYPE_386))
      {
        const char *n = NULL;
        if (cpu == CPU_TYPE_PPC)
        {
          if (t == CPU_SUBTYPE_POWERPC_970)
            n = "970";
          else if (t < Z7_ARRAY_SIZE(k_PowerPc_SubTypes))
            n = k_PowerPc_SubTypes[t];
        }
        s.Add_Space();
        if (n)
          s += n;
        else
          s.Add_UInt32(t);
      }
      prop = s;
      break;
    }
    case kpidCharacts:
    {
      // TYPE_TO_PROP(g_FileTypes, _type, prop); break;
      AString res (TypeToString(g_FileTypes, Z7_ARRAY_SIZE(g_FileTypes), _type));
      const AString s (FlagsToString(g_ArcFlags, Z7_ARRAY_SIZE(g_ArcFlags), _flags));
      if (!s.IsEmpty())
      {
        res.Add_Space();
        res += s;
      }
      prop = res;
      break;
    }
    case kpidPhySize:  prop = _totalSize; break;
    case kpidHeadersSize:  prop = _headersSize; break;
    case kpidBit64:  if (_mode64) prop = _mode64; break;
    case kpidBigEndian:  if (_be) prop = _be; break;
    case kpidExtension:
    {
      const char *ext = NULL;
      if (_type == kType_OBJECT)
        ext = "o";
      else if (_type == kType_BUNDLE)
        ext = "bundle";
      else if (_type == kType_DYLIB)
        ext = "dylib"; // main shared library usually does not have extension
      if (ext)
        prop = ext;
      break;
    }
    // case kpidIsSelfExe: prop = (_type == kType_EXECUTE); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

static void AddName(AString &s, const char *name)
{
  char temp[kNameSize + 1];
  memcpy(temp, name, kNameSize);
  temp[kNameSize] = 0;
  s += temp;
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  CPropVariant prop;
  const CSection &item = _sections[index];
  switch (propID)
  {
    case kpidPath:
    {
      AString s;
      AddName(s, _segments[item.SegmentIndex].Name);
      if (!item.IsDummy)
      {
        // CSection::SegName and CSegment::Name are same in real cases.
        // AddName(s, item.SegName);
        AddName(s, item.Name);
      }
      prop = MultiByteToUnicodeString(s);
      break;
    }
    case kpidSize:  /* prop = (UInt64)item.VSize; break; */
    case kpidPackSize:  prop = (UInt64)item.GetPackSize(); break;
    case kpidCharacts:
    {
      AString res;
      {
        if (!item.IsDummy)
        {
          {
            const AString s (TypeToString(g_SectTypes, Z7_ARRAY_SIZE(g_SectTypes), item.Flags & SECT_TYPE_MASK));
            if (!s.IsEmpty())
            {
              res.Add_OptSpaced("sect_type:");
              res.Add_OptSpaced(s);
            }
          }
          {
            const AString s (FlagsToString(g_SectFlags, Z7_ARRAY_SIZE(g_SectFlags), item.Flags & SECT_ATTR_MASK));
            if (!s.IsEmpty())
            {
              res.Add_OptSpaced("sect_flags:");
              res.Add_OptSpaced(s);
            }
          }
        }
        const CSegment &seg = _segments[item.SegmentIndex];
        {
          const AString s (FlagsToString(g_SegmentFlags, Z7_ARRAY_SIZE(g_SegmentFlags), seg.Flags));
          if (!s.IsEmpty())
          {
            res.Add_OptSpaced("seg_flags:");
            res.Add_OptSpaced(s);
          }
        }
        {
          const AString s (FlagsToString(g_SegmentProt, Z7_ARRAY_SIZE(g_SegmentProt), seg.MaxProt));
          if (!s.IsEmpty())
          {
            res.Add_OptSpaced("max_prot:");
            res.Add_OptSpaced(s);
          }
        }
        {
          const AString s (FlagsToString(g_SegmentProt, Z7_ARRAY_SIZE(g_SegmentProt), seg.InitProt));
          if (!s.IsEmpty())
          {
            res.Add_OptSpaced("init_prot:");
            res.Add_OptSpaced(s);
          }
        }
      }
      if (!res.IsEmpty())
        prop = res;
      break;
    }
    case kpidOffset:  prop = item.Pa; break;
    case kpidVa:  prop = item.Va; break;
    case kpidClusterSize:  prop = (UInt32)1 << item.Align; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


HRESULT CHandler::Open2(ISequentialInStream *stream)
{
  const UInt32 kStartHeaderSize = 7 * 4;

  Byte header[kStartHeaderSize];
  RINOK(ReadStream_FALSE(stream, header, kStartHeaderSize))
  bool be, mode64;
  switch (GetUi32(header))
  {
    case 0xCEFAEDFE:  be = true; mode64 = false; break;
    case 0xCFFAEDFE:  be = true; mode64 = true; break;
    case 0xFEEDFACE:  be = false; mode64 = false; break;
    case 0xFEEDFACF:  be = false; mode64 = true; break;
    default: return S_FALSE;
  }
  
  _mode64 = mode64;
  _be = be;

  const UInt32 numCommands = Get32(header + 0x10, be);
  const UInt32 commandsSize = Get32(header + 0x14, be);

  if (numCommands == 0)
    return S_FALSE;

  if (commandsSize > (1 << 24) ||
      numCommands > (1 << 21) ||
      numCommands * 8 > commandsSize)
    return S_FALSE;

  _cpuType = Get32(header + 4, be);
  _cpuSubType = Get32(header + 8, be);
  _type = Get32(header + 0xC, be);
  _flags = Get32(header + 0x18, be);

  /*
  // Probably the sections are in first commands. So we can reduce the number of commands.
  bool reduceCommands = false;
  const UInt32 kNumReduceCommands = 16;
  if (numCommands > kNumReduceCommands)
  {
    reduceCommands = true;
    numCommands = kNumReduceCommands;
  }
  */

  UInt32 startHeaderSize = kStartHeaderSize;
  if (mode64)
    startHeaderSize += 4;
  _headersSize = startHeaderSize + commandsSize;
  _totalSize = _headersSize;
  CByteArr buffer(_headersSize);
  RINOK(ReadStream_FALSE(stream, buffer + kStartHeaderSize, _headersSize - kStartHeaderSize))
  const Byte *buf = buffer + startHeaderSize;
  size_t size = _headersSize - startHeaderSize;
  for (UInt32 cmdIndex = 0; cmdIndex < numCommands; cmdIndex++)
  {
    if (size < 8)
      return S_FALSE;
    UInt32 cmd = Get32(buf, be);
    UInt32 cmdSize = Get32(buf + 4, be);
    if (cmdSize < 8)
      return S_FALSE;
    if (size < cmdSize)
      return S_FALSE;
    if (cmd == CMD_SEGMENT_32 || cmd == CMD_SEGMENT_64)
    {
      UInt32 offs = (cmd == CMD_SEGMENT_64) ? 0x48 : 0x38;
      if (cmdSize < offs)
        break;

      UInt64 vmAddr, vmSize, phAddr, phSize;

      {
        if (cmd == CMD_SEGMENT_64)
        {
          vmAddr = Get64(buf + 0x18, be);
          vmSize = Get64(buf + 0x20, be);
          phAddr = Get64(buf + 0x28, be);
          phSize = Get64(buf + 0x30, be);
        }
        else
        {
          vmAddr = Get32(buf + 0x18, be);
          vmSize = Get32(buf + 0x1C, be);
          phAddr = Get32(buf + 0x20, be);
          phSize = Get32(buf + 0x24, be);
        }
        {
          UInt64 totalSize = phAddr + phSize;
          if (totalSize < phAddr)
            return S_FALSE;
          if (_totalSize < totalSize)
            _totalSize = totalSize;
        }
      }
      
      CSegment seg;
      seg.MaxProt = Get32(buf + offs - 16, be);
      seg.InitProt = Get32(buf + offs - 12, be);
      seg.Flags = Get32(buf + offs - 4, be);
      memcpy(seg.Name, buf + 8, kNameSize);
      _segments.Add(seg);

      UInt32 numSections = Get32(buf + offs - 8, be);
      if (numSections > (1 << 8))
        return S_FALSE;

      if (numSections == 0)
      {
        CSection &sect = _sections.AddNew();
        sect.IsDummy = true;
        sect.SegmentIndex = _segments.Size() - 1;
        sect.Va = vmAddr;
        sect.PSize = phSize;
        sect.VSize = vmSize;
        sect.Pa = phAddr;
        sect.Align = 0;
        sect.Flags = 0;
      }
      else do
      {
        const UInt32 headSize = (cmd == CMD_SEGMENT_64) ? 0x50 : 0x44;
        const Byte *p = buf + offs;
        if (cmdSize - offs < headSize)
          break;
        CSection &sect = _sections.AddNew();
        unsigned f32Offset;
        if (cmd == CMD_SEGMENT_64)
        {
          sect.Va    = Get64(p + 0x20, be);
          sect.VSize = Get64(p + 0x28, be);
          f32Offset = 0x30;
        }
        else
        {
          sect.Va    = Get32(p + 0x20, be);
          sect.VSize = Get32(p + 0x24, be);
          f32Offset = 0x28;
        }
        sect.Pa    = Get32(p + f32Offset, be);
        sect.Align = Get32(p + f32Offset + 4, be);
        // sect.reloff = Get32(p + f32Offset + 8, be);
        // sect.nreloc = Get32(p + f32Offset + 12, be);
        sect.Flags = Get32(p + f32Offset + 16, be);
        if ((sect.Flags & SECT_TYPE_MASK) == SECT_ATTR_ZEROFILL)
          sect.PSize = 0;
        else
          sect.PSize = sect.VSize;
        memcpy(sect.Name, p, kNameSize);
        // memcpy(sect.SegName, p + kNameSize, kNameSize);
        sect.SegmentIndex = _segments.Size() - 1;
        offs += headSize;
      }
      while (--numSections);

      if (offs != cmdSize)
        return S_FALSE;
    }
    buf += cmdSize;
    size -= cmdSize;
  }
  // return (reduceCommands || (size == 0)) ? S_OK : S_FALSE;
  if (size != 0)
    return S_FALSE;

  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */))
{
  COM_TRY_BEGIN
  Close();
  RINOK(Open2(inStream))
  if (!_allowTail)
  {
    UInt64 fileSize;
    RINOK(InStream_GetSize_SeekToEnd(inStream, fileSize))
    if (fileSize > _totalSize)
      return S_FALSE;
  }
  _inStream = inStream;
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _totalSize = 0;
  _inStream.Release();
  _sections.Clear();
  _segments.Clear();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _sections.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _sections.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _sections[allFilesMode ? i : indices[i]].GetPackSize();
  extractCallback->SetTotal(totalSize);

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_inStream);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur())
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    const CSection &item = _sections[index];
    currentItemSize = item.GetPackSize();

    CMyComPtr<ISequentialOutStream> outStream;
    RINOK(extractCallback->GetStream(index, &outStream, askMode))
    if (!testMode && !outStream)
      continue;
    
    RINOK(extractCallback->PrepareOperation(askMode))
    RINOK(InStream_SeekSet(_inStream, item.Pa))
    streamSpec->Init(currentItemSize);
    RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress))
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(copyCoderSpec->TotalSize == currentItemSize ?
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

static const Byte k_Signature[] = {
  4, 0xCE, 0xFA, 0xED, 0xFE,
  4, 0xCF, 0xFA, 0xED, 0xFE,
  4, 0xFE, 0xED, 0xFA, 0xCE,
  4, 0xFE, 0xED, 0xFA, 0xCF };

REGISTER_ARC_I(
  "MachO", "macho", NULL, 0xDF,
  k_Signature,
  0,
  NArcInfoFlags::kMultiSignature |
  NArcInfoFlags::kPreArc,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/MbrHandler.cpp ================ */
// MbrHandler.cpp

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

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

using namespace NWindows;

namespace NArchive {

namespace NFat {
API_FUNC_IsArc IsArc_Fat(const Byte *p, size_t size);
}

namespace NMbr {

struct CChs
{
  Byte Head;
  Byte SectCyl;
  Byte Cyl8;
  
  UInt32 GetSector() const { return SectCyl & 0x3F; }
  UInt32 GetCyl() const { return ((UInt32)SectCyl >> 6 << 8) | Cyl8; }
  void ToString(NCOM::CPropVariant &prop) const;

  void Parse(const Byte *p)
  {
    Head = p[0];
    SectCyl = p[1];
    Cyl8 = p[2];
  }
  bool Check() const { return GetSector() > 0; }
};


// Chs in some MBRs contains only low bits of "Cyl number". So we disable check.
/*
#define RINOZ(x) { int _t_ = (x); if (_t_ != 0) return _t_; }
static int CompareChs(const CChs &c1, const CChs &c2)
{
  RINOZ(MyCompare(c1.GetCyl(), c2.GetCyl()));
  RINOZ(MyCompare(c1.Head, c2.Head));
  return MyCompare(c1.GetSector(), c2.GetSector());
}
*/

void CChs::ToString(NCOM::CPropVariant &prop) const
{
  AString s;
  s.Add_UInt32(GetCyl());
  s.Add_Minus();
  s.Add_UInt32(Head);
  s.Add_Minus();
  s.Add_UInt32(GetSector());
  prop = s;
}

struct CPartition
{
  Byte Status;
  CChs BeginChs;
  Byte Type;
  CChs EndChs;
  UInt32 Lba;
  UInt32 NumBlocks;

  CPartition() { memset (this, 0, sizeof(*this)); }
  
  bool IsEmpty() const { return Type == 0; }
  bool IsExtended() const { return Type == 5 || Type == 0xF; }
  UInt32 GetLimit() const { return Lba + NumBlocks; }
  // bool IsActive() const { return Status == 0x80; }
  UInt64 GetPos(unsigned sectorSizeLog) const { return (UInt64)Lba << sectorSizeLog; }
  UInt64 GetSize(unsigned sectorSizeLog) const { return (UInt64)NumBlocks << sectorSizeLog; }

  bool CheckLbaLimits() const { return (UInt32)0xFFFFFFFF - Lba >= NumBlocks; }
  bool Parse(const Byte *p)
  {
    Status = p[0];
    BeginChs.Parse(p + 1);
    Type = p[4];
    EndChs.Parse(p + 5);
    Lba = GetUi32(p + 8);
    NumBlocks = GetUi32(p + 12);
    if (Type == 0)
      return true;
    if (Status != 0 && Status != 0x80)
      return false;
    return BeginChs.Check()
       && EndChs.Check()
       // && CompareChs(BeginChs, EndChs) <= 0
       && NumBlocks > 0
       && CheckLbaLimits();
  }

  #ifdef SHOW_DEBUG_INFO
  void Print() const
  {
    NCOM::CPropVariant prop, prop2;
    BeginChs.ToString(prop);
    EndChs.ToString(prop2);
    printf("   %2x %2x %8X %8X %12S %12S", (int)Status, (int)Type, Lba, NumBlocks, prop.bstrVal, prop2.bstrVal);
  }
  #endif
};

struct CPartType
{
  UInt32 Id;
  const char *Ext;
  const char *Name;
};

#define kFat "fat"

/*
if we use "format" command in Windows 10 for existing partition:
  - if we format to ExFAT, it sets type=7
  - if we format to UDF, it doesn't change type from previous value.
*/
  
static const unsigned kType_Windows_NTFS = 7;

static const CPartType kPartTypes[] =
{
  { 0x01, kFat, "FAT12" },
  { 0x04, kFat, "FAT16 DOS 3.0+" },
  { 0x05, NULL, "Extended" },
  { 0x06, kFat, "FAT16 DOS 3.31+" },
  { 0x07, "ntfs", "NTFS" },
  { 0x0B, kFat, "FAT32" },
  { 0x0C, kFat, "FAT32-LBA" },
  { 0x0E, kFat, "FAT16-LBA" },
  { 0x0F, NULL, "Extended-LBA" },
  { 0x11, kFat, "FAT12-Hidden" },
  { 0x14, kFat, "FAT16-Hidden < 32 MB" },
  { 0x16, kFat, "FAT16-Hidden >= 32 MB" },
  { 0x1B, kFat, "FAT32-Hidden" },
  { 0x1C, kFat, "FAT32-LBA-Hidden" },
  { 0x1E, kFat, "FAT16-LBA-WIN95-Hidden" },
  { 0x27, "ntfs", "NTFS-WinRE" },
  { 0x82, NULL, "Solaris x86 / Linux swap" },
  { 0x83, NULL, "Linux" },
  { 0x8E, "lvm", "Linux LVM" },
  { 0xA5, NULL, "BSD slice" },
  { 0xBE, NULL, "Solaris 8 boot" },
  { 0xBF, NULL, "New Solaris x86" },
  { 0xC2, NULL, "Linux-Hidden" },
  { 0xC3, NULL, "Linux swap-Hidden" },
  { 0xEE, NULL, "GPT" },
  { 0xEE, NULL, "EFI" }
};

static int FindPartType(UInt32 type)
{
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(kPartTypes); i++)
    if (kPartTypes[i].Id == type)
      return (int)i;
  return -1;
}

struct CItem
{
  bool IsReal;
  bool IsPrim;
  bool WasParsed;
  const char *FileSystem;
  UInt64 Size;
  CPartition Part;

  CItem():
      WasParsed(false),
      FileSystem(NULL)
      {}
};


Z7_class_CHandler_final: public CHandlerCont
{
  Z7_IFACE_COM7_IMP(IInArchive_Cont)

  CObjectVector<CItem> _items;
  UInt64 _totalSize;
  CByteBuffer _buffer;

  UInt32 _signature;
  unsigned _sectorSizeLog;

  virtual int GetItem_ExtractInfo(UInt32 index, UInt64 &pos, UInt64 &size) const Z7_override Z7_final
  {
    const CItem &item = _items[index];
    pos = item.Part.GetPos(_sectorSizeLog);
    size = item.Size;
    return NExtract::NOperationResult::kOK;
  }

  HRESULT ReadTables(IInStream *stream, UInt32 baseLba, UInt32 lba, unsigned level);
};

/*
static bool IsEmptyBuffer(const Byte *data, size_t size)
{
  for (unsigned i = 0; i < size; i++)
    if (data[i] != 0)
      return false;
  return true;
}
*/

const char *GetFileSystem(ISequentialInStream *stream, UInt64 partitionSize);

HRESULT CHandler::ReadTables(IInStream *stream, UInt32 baseLba, UInt32 lba, unsigned level)
{
  if (level >= 128 || _items.Size() >= 128)
    return S_FALSE;

  const unsigned kNumHeaderParts = 4;
  CPartition parts[kNumHeaderParts];

  if (level == 0)
    _sectorSizeLog = 9;
  const UInt32 kSectorSize = (UInt32)1 << _sectorSizeLog;
  UInt32 bufSize  = kSectorSize;
  if (level == 0 && _totalSize >= (1 << 12))
    bufSize = (1 << 12);
  _buffer.Alloc(bufSize);
  {
    Byte *buf = _buffer;
    const UInt64 newPos = (UInt64)lba << _sectorSizeLog;
    if (newPos + bufSize > _totalSize)
      return S_FALSE;
    RINOK(InStream_SeekSet(stream, newPos))
    RINOK(ReadStream_FALSE(stream, buf, bufSize))
    if (buf[0x1FE] != 0x55 || buf[0x1FF] != 0xAA)
      return S_FALSE;
    if (level == 0)
      _signature = GetUi32(buf + 0x1B8);
    for (unsigned i = 0; i < kNumHeaderParts; i++)
      if (!parts[i].Parse(buf + 0x1BE + 16 * i))
        return S_FALSE;
  }

  // 23.02: now we try to detect 4kn format (4 KB sectors case)
  if (level == 0)
  // if (_totalSize >= (1 << 28)) // we don't expect small images with 4 KB sectors
  if (bufSize >= (1 << 12))
  {
    UInt32 lastLim = 0;
    UInt32 firstLba = 0;
    UInt32 numBlocks = 0; // in first partition
    for (unsigned i = 0; i < kNumHeaderParts; i++)
    {
      const CPartition &part = parts[i];
      if (part.IsEmpty())
        continue;
      if (firstLba == 0 && part.NumBlocks != 0)
      {
        firstLba = part.Lba;
        numBlocks = part.NumBlocks;
      }
      const UInt32 newLim = part.GetLimit();
      if (newLim < lastLim)
        return S_FALSE;
      lastLim = newLim;
    }
    if (lastLim != 0)
    {
      const UInt64 lim12 = (UInt64)lastLim << 12;
      if (lim12 <= _totalSize
          // && _totalSize - lim12 < (1 << 28) // we can try to exclude false positive cases
          )
      // if (IsEmptyBuffer(&_buffer[(1 << 9)], (1 << 12) - (1 << 9)))
      if (InStream_SeekSet(stream, (UInt64)firstLba << 12) == S_OK)
      if (GetFileSystem(stream, (UInt64)numBlocks << 12))
        _sectorSizeLog = 12;
    }
  }

  PRF(printf("\n# %8X", lba));

  UInt32 limLba = lba + 1;
  if (limLba == 0)
    return S_FALSE;

  for (unsigned i = 0; i < kNumHeaderParts; i++)
  {
    CPartition &part = parts[i];
    
    if (part.IsEmpty())
      continue;
    PRF(printf("\n   %2d ", (unsigned)level));
    #ifdef SHOW_DEBUG_INFO
    part.Print();
    #endif
    
    unsigned numItems = _items.Size();
    UInt32 newLba = lba + part.Lba;
    
    if (part.IsExtended())
    {
      // if (part.Type == 5) // Check it!
      newLba = baseLba + part.Lba;
      if (newLba < limLba)
        return S_FALSE;
      const HRESULT res = ReadTables(stream, level < 1 ? newLba : baseLba, newLba, level + 1);
      if (res != S_FALSE && res != S_OK)
        return res;
    }
    if (newLba < limLba)
      return S_FALSE;
    part.Lba = newLba;
    if (!part.CheckLbaLimits())
      return S_FALSE;

    CItem n;
    n.Part = part;
    bool addItem = false;
    if (numItems == _items.Size())
    {
      n.IsPrim = (level == 0);
      n.IsReal = true;
      addItem = true;
    }
    else
    {
      const CItem &back = _items.Back();
      const UInt32 backLimit = back.Part.GetLimit();
      const UInt32 partLimit = part.GetLimit();
      if (backLimit < partLimit)
      {
        n.IsReal = false;
        n.Part.Lba = backLimit;
        n.Part.NumBlocks = partLimit - backLimit;
        addItem = true;
      }
    }
    if (addItem)
    {
      if (n.Part.GetLimit() < limLba)
        return S_FALSE;
      limLba = n.Part.GetLimit();
      n.Size = n.Part.GetSize(_sectorSizeLog);
      _items.Add(n);
    }
  }
  return S_OK;
}


static const Byte k_Ntfs_Signature[] = { 'N', 'T', 'F', 'S', ' ', ' ', ' ', ' ', 0 };

static bool Is_Ntfs(const Byte *p)
{
  if (p[0x1FE] != 0x55 || p[0x1FF] != 0xAA)
    return false;
  switch (p[0])
  {
    case 0xE9: /* codeOffset = 3 + (Int16)Get16(p + 1); */ break;
    case 0xEB: if (p[2] != 0x90) return false; /* codeOffset = 2 + (int)(signed char)p[1]; */ break;
    default: return false;
  }
  return memcmp(p + 3, k_Ntfs_Signature, Z7_ARRAY_SIZE(k_Ntfs_Signature)) == 0;
}

static const Byte k_ExFat_Signature[] = { 'E', 'X', 'F', 'A', 'T', ' ', ' ', ' ' };

static bool Is_ExFat(const Byte *p)
{
  if (p[0x1FE] != 0x55 || p[0x1FF] != 0xAA)
    return false;
  if (p[0] != 0xEB || p[1] != 0x76 || p[2] != 0x90)
    return false;
  return memcmp(p + 3, k_ExFat_Signature, Z7_ARRAY_SIZE(k_ExFat_Signature)) == 0;
}

static bool AllAreZeros(const Byte *p, size_t size)
{
  for (size_t i = 0; i < size; i++)
    if (p[i] != 0)
      return false;
  return true;
}

static const UInt32 k_Udf_StartPos = 0x8000;
static const Byte k_Udf_Signature[] = { 0, 'B', 'E', 'A', '0', '1', 1, 0 };

static bool Is_Udf(const Byte *p)
{
  return memcmp(p + k_Udf_StartPos, k_Udf_Signature, sizeof(k_Udf_Signature)) == 0;
}


const char *GetFileSystem(ISequentialInStream *stream, UInt64 partitionSize)
{
  const size_t kHeaderSize = 1 << 9;
  if (partitionSize >= kHeaderSize)
  {
    Byte buf[kHeaderSize];
    if (ReadStream_FAIL(stream, buf, kHeaderSize) == S_OK)
    {
      // NTFS is expected default filesystem for (Type == 7)
      if (Is_Ntfs(buf))
        return "NTFS";
      if (Is_ExFat(buf))
        return "exFAT";
      if (NFat::IsArc_Fat(buf, kHeaderSize))
        return "FAT";
      const size_t kHeaderSize2 = k_Udf_StartPos + (1 << 9);
      if (partitionSize >= kHeaderSize2)
      {
        if (AllAreZeros(buf, kHeaderSize))
        {
          CByteBuffer buffer(kHeaderSize2);
          // memcpy(buffer, buf, kHeaderSize);
          if (ReadStream_FAIL(stream, buffer + kHeaderSize, kHeaderSize2 - kHeaderSize) == S_OK)
            if (Is_Udf(buffer))
              return "UDF";
        }
      }
    }
  }
  return NULL;
}


Z7_COM7F_IMF(CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */))
{
  COM_TRY_BEGIN
  Close();
  RINOK(InStream_GetSize_SeekToEnd(stream, _totalSize))
  RINOK(ReadTables(stream, 0, 0, 0))
  if (_items.IsEmpty())
    return S_FALSE;
  {
    const UInt32 lbaLimit = _items.Back().Part.GetLimit();
    const UInt64 lim = (UInt64)lbaLimit << _sectorSizeLog;
    if (lim < _totalSize)
    {
      CItem n;
      n.Part.Lba = lbaLimit;
      n.Size = _totalSize - lim;
      n.IsReal = false;
      _items.Add(n);
    }
  }

  FOR_VECTOR (i, _items)
  {
    CItem &item = _items[i];
    if (item.Part.Type != kType_Windows_NTFS)
      continue;
    if (InStream_SeekSet(stream, item.Part.GetPos(_sectorSizeLog)) != S_OK)
      continue;
    item.FileSystem = GetFileSystem(stream, item.Size);
    item.WasParsed = true;
  }

  _stream = stream;
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::Close())
{
  _totalSize = 0;
  _items.Clear();
  _stream.Release();
  return S_OK;
}


enum
{
  kpidPrimary = kpidUserDefined,
  kpidBegChs,
  kpidEndChs
};

static const CStatProp kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidFileSystem, VT_BSTR},
  { NULL, kpidOffset, VT_UI8},
  { "Primary", kpidPrimary, VT_BOOL},
  { "Begin CHS", kpidBegChs, VT_BSTR},
  { "End CHS", kpidEndChs, VT_BSTR}
};

static const Byte kArcProps[] =
{
  kpidSectorSize,
  kpidId
};

IMP_IInArchive_Props_WITH_NAME
IMP_IInArchive_ArcProps


Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidMainSubfile:
    {
      int mainIndex = -1;
      FOR_VECTOR (i, _items)
        if (_items[i].IsReal)
        {
          if (mainIndex >= 0)
          {
            mainIndex = -1;
            break;
          }
          mainIndex = (int)i;
        }
      if (mainIndex >= 0)
        prop = (UInt32)(Int32)mainIndex;
      break;
    }
    case kpidPhySize: prop = _totalSize; break;
    case kpidSectorSize: prop = (UInt32)((UInt32)1 << _sectorSizeLog); break;
    case kpidId: prop = _signature; break;
  }
  prop.Detach(value);
  return S_OK;
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

  const CItem &item = _items[index];
  const CPartition &part = item.Part;
  switch (propID)
  {
    case kpidPath:
    {
      AString s;
      s.Add_UInt32(index);
      if (item.IsReal)
      {
        s.Add_Dot();
        const char *ext = NULL;
        if (item.FileSystem)
        {
          ext = "";
          AString fs (item.FileSystem);
          fs.MakeLower_Ascii();
          s += fs;
        }
        else if (!item.WasParsed)
        {
          const int typeIndex = FindPartType(part.Type);
          if (typeIndex >= 0)
            ext = kPartTypes[(unsigned)typeIndex].Ext;
        }
        if (!ext)
          ext = "img";
        s += ext;
      }
      prop = s;
      break;
    }
    case kpidFileSystem:
      if (item.IsReal)
      {
        char s[32];
        ConvertUInt32ToString(part.Type, s);
        const char *res = s;
        if (item.FileSystem)
        {
          // strcat(s, "-");
          // strcpy(s, item.FileSystem);
          res = item.FileSystem;
        }
        else if (!item.WasParsed)
        {
          const int typeIndex = FindPartType(part.Type);
          if (typeIndex >= 0 && kPartTypes[(unsigned)typeIndex].Name)
            res = kPartTypes[(unsigned)typeIndex].Name;
        }
        prop = res;
      }
      break;
    case kpidSize:
    case kpidPackSize: prop = item.Size; break;
    case kpidOffset: prop = part.GetPos(_sectorSizeLog); break;
    case kpidPrimary: if (item.IsReal) prop = item.IsPrim; break;
    case kpidBegChs: if (item.IsReal) part.BeginChs.ToString(prop); break;
    case kpidEndChs: if (item.IsReal) part.EndChs.ToString(prop); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


  // 3, { 1, 1, 0 },
  // 2, { 0x55, 0x1FF },

REGISTER_ARC_I_NO_SIG(
  "MBR", "mbr", NULL, 0xDB,
  0,
  NArcInfoFlags::kPureStartOpen,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/MslzHandler.cpp ================ */
// MslzHandler.cpp

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
namespace NMslz {

static const UInt32 kUnpackSizeMax = 0xFFFFFFE0;

Z7_CLASS_IMP_CHandler_IInArchive_1(
  IArchiveOpenSeq
)
  CMyComPtr<IInStream> _inStream;
  CMyComPtr<ISequentialInStream> _seqStream;

  bool _isArc;
  bool _needSeekToStart;
  bool _dataAfterEnd;
  bool _needMoreInput;

  bool _packSize_Defined;
  bool _unpackSize_Defined;

  UInt32 _unpackSize;
  UInt64 _packSize;
  UInt64 _originalFileSize;
  UString _name;

  void ParseName(Byte replaceByte, IArchiveOpenCallback *callback);
};

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidPackSize,
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
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidExtension: prop = "mslz"; break;
    case kpidIsNotArcType: prop = true; break;
    case kpidPhySize: if (_packSize_Defined) prop = _packSize; break;
    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_needMoreInput) v |= kpv_ErrorFlags_UnexpectedEnd;
      if (_dataAfterEnd) v |= kpv_ErrorFlags_DataAfterEnd;
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
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPath: if (!_name.IsEmpty()) prop = _name; break;
    case kpidSize: if (_unpackSize_Defined || _inStream) prop = _unpackSize; break;
    case kpidPackSize: if (_packSize_Defined || _inStream) prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

static const unsigned kSignatureSize = 9;
static const unsigned kHeaderSize = kSignatureSize + 1 + 4;
#define MSLZ_SIGNATURE { 0x53, 0x5A, 0x44, 0x44, 0x88, 0xF0, 0x27, 0x33, 0x41 }
// old signature: 53 5A 20 88 F0 27 33
static const Byte kSignature[kSignatureSize] = MSLZ_SIGNATURE;

// we support only 3 chars strings here
static const char * const g_Exts[] =
{
    "bin"
  , "dll"
  , "exe"
  , "kmd"
  , "pdf"
  , "sys"
};

void CHandler::ParseName(Byte replaceByte, IArchiveOpenCallback *callback)
{
  if (!callback)
    return;
  Z7_DECL_CMyComPtr_QI_FROM(IArchiveOpenVolumeCallback, volumeCallback, callback)
  if (!volumeCallback)
    return;

  NWindows::NCOM::CPropVariant prop;
  if (volumeCallback->GetProperty(kpidName, &prop) != S_OK || prop.vt != VT_BSTR)
    return;

  UString s = prop.bstrVal;
  if (s.IsEmpty() ||
      s.Back() != L'_')
    return;
  
  s.DeleteBack();
  _name = s;
   
  if (replaceByte == 0)
  {
    if (s.Len() < 3 || s[s.Len() - 3] != '.')
      return;
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_Exts); i++)
    {
      const char *ext = g_Exts[i];
      if (s[s.Len() - 2] == (Byte)ext[0] &&
          s[s.Len() - 1] == (Byte)ext[1])
      {
        replaceByte = (Byte)ext[2];
        break;
      }
    }
  }
  
  if (replaceByte >= 0x20 && replaceByte < 0x80)
    _name.Add_Char((char)replaceByte);
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  {
    Close();
    _needSeekToStart = true;
    Byte buffer[kHeaderSize];
    RINOK(ReadStream_FALSE(stream, buffer, kHeaderSize))
    if (memcmp(buffer, kSignature, kSignatureSize) != 0)
      return S_FALSE;
    _unpackSize = GetUi32(buffer + 10);
    if (_unpackSize > kUnpackSizeMax)
      return S_FALSE;
    RINOK(InStream_GetSize_SeekToEnd(stream, _originalFileSize))
    _packSize = _originalFileSize;

    ParseName(buffer[kSignatureSize], callback);

    _isArc = true;
    _unpackSize_Defined = true;
    _inStream = stream;
    _seqStream = stream;
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _originalFileSize = 0;
  _packSize = 0;
  _unpackSize = 0;

  _isArc = false;
  _needSeekToStart = false;
  _dataAfterEnd = false;
  _needMoreInput = false;

  _packSize_Defined = false;
  _unpackSize_Defined =  false;

  _seqStream.Release();
  _inStream.Release();
  _name.Empty();
  return S_OK;
}

// MslzDec is modified LZSS algorithm of Haruhiko Okumura:
//   maxLen = 18; Okumura
//   maxLen = 16; MS

#define PROGRESS_AND_WRITE \
  if ((dest & kMask) == 0) { if (outStream) RINOK(WriteStream(outStream, buf, kBufSize)); \
    if ((dest & ((1 << 20) - 1)) == 0) \
    if (progress) \
      { UInt64 inSize = inStream.GetProcessedSize(); UInt64 outSize = dest; \
        RINOK(progress->SetRatioInfo(&inSize, &outSize)); }}

static HRESULT MslzDec(CInBuffer &inStream, ISequentialOutStream *outStream, UInt32 unpackSize, bool &needMoreData, ICompressProgressInfo *progress)
{
  const unsigned kBufSize = (1 << 12);
  const unsigned kMask = kBufSize - 1;
  Byte buf[kBufSize];
  UInt32 dest = 0;
  memset(buf, ' ', kBufSize);
  
  while (dest < unpackSize)
  {
    Byte b;
    if (!inStream.ReadByte(b))
    {
      needMoreData = true;
      return S_FALSE;
    }
    
    for (unsigned mask = (unsigned)b | 0x100; mask > 1 && dest < unpackSize; mask >>= 1)
    {
      if (!inStream.ReadByte(b))
      {
        needMoreData = true;
        return S_FALSE;
      }
  
      if (mask & 1)
      {
        buf[dest++ & kMask] = b;
        PROGRESS_AND_WRITE
      }
      else
      {
        Byte b1;
        if (!inStream.ReadByte(b1))
        {
          needMoreData = true;
          return S_FALSE;
        }
        const unsigned kMaxLen = 16; // 18 in Okumura's code.
        unsigned src = (((((unsigned)b1 & 0xF0) << 4) | b) + kMaxLen) & kMask;
        unsigned len = (b1 & 0xF) + 3;
        if (len > kMaxLen || dest + len > unpackSize)
          return S_FALSE;
    
        do
        {
          buf[dest++ & kMask] = buf[src++ & kMask];
          PROGRESS_AND_WRITE
        }
        while (--len != 0);
      }
    }
  }
  
  if (outStream)
    RINOK(WriteStream(outStream, buf, dest & kMask))
  return S_OK;
}

Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  COM_TRY_BEGIN
  Close();
  _isArc = true;
  _seqStream = stream;
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  // RINOK(extractCallback->SetTotal(_unpackSize))
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

  if (_needSeekToStart)
  {
    if (!_inStream)
      return E_FAIL;
    RINOK(InStream_SeekToBegin(_inStream))
  }
  else
    _needSeekToStart = true;

  opRes = NExtract::NOperationResult::kDataError;

  bool isArc = false;
  bool needMoreInput = false;
  try
  {
    CInBuffer s;
    if (!s.Create(1 << 20))
      return E_OUTOFMEMORY;
    s.SetStream(_seqStream);
    s.Init();
    
    Byte buffer[kHeaderSize];
    if (s.ReadBytes(buffer, kHeaderSize) == kHeaderSize)
    {
      UInt32 unpackSize;
      if (memcmp(buffer, kSignature, kSignatureSize) == 0)
      {
        unpackSize = GetUi32(buffer + 10);
        if (unpackSize <= kUnpackSizeMax)
        {
          const HRESULT result = MslzDec(s, outStream, unpackSize, needMoreInput, lps);
          if (result == S_OK)
            opRes = NExtract::NOperationResult::kOK;
          else if (result != S_FALSE)
            return result;
          _unpackSize = unpackSize;
          _unpackSize_Defined = true;

          _packSize = s.GetProcessedSize();
          _packSize_Defined = true;

          if (_inStream && _packSize < _originalFileSize)
            _dataAfterEnd = true;
          
          isArc = true;
        }
      }
    }
  }
  catch (CInBufferException &e) { return e.ErrorCode; }

  _isArc = isArc;
  if (isArc)
    _needMoreInput = needMoreInput;
  if (!_isArc)
    opRes = NExtract::NOperationResult::kIsNotArc;
  else if (_needMoreInput)
    opRes = NExtract::NOperationResult::kUnexpectedEnd;
  else if (_dataAfterEnd)
    opRes = NExtract::NOperationResult::kDataAfterEnd;
  }
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}

REGISTER_ARC_I(
  "MsLZ", "mslz", NULL, 0xD5,
  kSignature,
  0,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/MubHandler.cpp ================ */
// MubHandler.cpp

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
using namespace NCOM;

namespace NArchive {
namespace NMub {

#define MACH_CPU_ARCH_ABI64 ((UInt32)1 << 24)
#define MACH_CPU_TYPE_386    7
#define MACH_CPU_TYPE_ARM   12
#define MACH_CPU_TYPE_SPARC 14
#define MACH_CPU_TYPE_PPC   18

#define MACH_CPU_TYPE_PPC64 (MACH_CPU_ARCH_ABI64 | MACH_CPU_TYPE_PPC)
#define MACH_CPU_TYPE_AMD64 (MACH_CPU_ARCH_ABI64 | MACH_CPU_TYPE_386)
#define MACH_CPU_TYPE_ARM64 (MACH_CPU_ARCH_ABI64 | MACH_CPU_TYPE_ARM)

#define MACH_CPU_SUBTYPE_LIB64 ((UInt32)1 << 31)

#define MACH_CPU_SUBTYPE_I386_ALL 3

struct CItem
{
  UInt32 Type;
  UInt32 SubType;
  UInt32 Offset;
  UInt32 Size;
  UInt32 Align;
};

static const UInt32 kNumFilesMax = 6;

Z7_class_CHandler_final: public CHandlerCont
{
  Z7_IFACE_COM7_IMP(IInArchive_Cont)

  // UInt64 _startPos;
  UInt64 _phySize;
  UInt32 _numItems;
  bool _bigEndian;
  CItem _items[kNumFilesMax];

  HRESULT Open2(IInStream *stream);

  virtual int GetItem_ExtractInfo(UInt32 index, UInt64 &pos, UInt64 &size) const Z7_override
  {
    const CItem &item = _items[index];
    pos = item.Offset;
    size = item.Size;
    return NExtract::NOperationResult::kOK;
  }
};

static const Byte kArcProps[] =
{
  kpidBigEndian
};

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidOffset,
  kpidClusterSize // Align
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  PropVariant_Clear(value);
  switch (propID)
  {
    case kpidBigEndian: PropVarEm_Set_Bool(value, _bigEndian); break;
    case kpidPhySize: PropVarEm_Set_UInt64(value, _phySize); break;
  }
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  PropVariant_Clear(value);
  const CItem &item = _items[index];
  switch (propID)
  {
    case kpidExtension:
    {
      char temp[32];
      const char *ext = NULL;
      switch (item.Type)
      {
        case MACH_CPU_TYPE_386:   ext = "x86";   break;
        case MACH_CPU_TYPE_ARM:   ext = "arm";   break;
        case MACH_CPU_TYPE_SPARC: ext = "sparc"; break;
        case MACH_CPU_TYPE_PPC:   ext = "ppc";   break;
        case MACH_CPU_TYPE_AMD64: ext = "x64";   break;
        case MACH_CPU_TYPE_ARM64: ext = "arm64"; break;
        case MACH_CPU_TYPE_PPC64: ext = "ppc64"; break;
        default:
          temp[0] = 'c';
          temp[1] = 'p';
          temp[2] = 'u';
          char *p = ConvertUInt32ToString(item.Type & ~MACH_CPU_ARCH_ABI64, temp + 3);
          if (item.Type & MACH_CPU_ARCH_ABI64)
            MyStringCopy(p, "_64");
          break;
      }
      if (ext)
        MyStringCopy(temp, ext);
      if (item.SubType != 0)
      if ((item.Type != MACH_CPU_TYPE_386 &&
           item.Type != MACH_CPU_TYPE_AMD64)
           || (item.SubType & ~(UInt32)MACH_CPU_SUBTYPE_LIB64) != MACH_CPU_SUBTYPE_I386_ALL
         )
      {
        unsigned pos = MyStringLen(temp);
        temp[pos++] = '-';
        ConvertUInt32ToString(item.SubType, temp + pos);
      }
      return PropVarEm_Set_Str(value, temp);
    }
    case kpidSize:
    case kpidPackSize:
      PropVarEm_Set_UInt64(value, item.Size);
      break;
    case kpidOffset:
      PropVarEm_Set_UInt64(value, item.Offset);
      break;
    case kpidClusterSize:
      PropVarEm_Set_UInt32(value, (UInt32)1 << item.Align);
      break;
  }
  return S_OK;
}

HRESULT CHandler::Open2(IInStream *stream)
{
  // RINOK(InStream_GetPos(stream, _startPos));

  const UInt32 kHeaderSize = 2;
  const UInt32 kRecordSize = 5;
  const UInt32 kBufSize = kHeaderSize + kNumFilesMax * kRecordSize;
  UInt32 buf[kBufSize];
  size_t processed = kBufSize * 4;
  RINOK(ReadStream(stream, buf, &processed))
  processed >>= 2;
  if (processed < kHeaderSize)
    return S_FALSE;
  
  bool be;
  switch (buf[0])
  {
    case Z7_CONV_BE_TO_NATIVE_CONST32(0xCAFEBABE): be = true; break;
    case Z7_CONV_BE_TO_NATIVE_CONST32(0xB9FAF10E): be = false; break;
    default: return S_FALSE;
  }
  _bigEndian = be;
  if (
      #if defined(MY_CPU_BE)
        !
      #endif
        be)
    z7_SwapBytes4(&buf[1], processed - 1);
  const UInt32 num = buf[1];
  if (num > kNumFilesMax || processed < kHeaderSize + num * kRecordSize)
    return S_FALSE;
  if (num == 0)
    return S_FALSE;
  UInt64 endPosMax = kHeaderSize;

  for (UInt32 i = 0; i < num; i++)
  {
    const UInt32 *p = buf + kHeaderSize + i * kRecordSize;
    CItem &sb = _items[i];
    sb.Type = p[0];
    sb.SubType = p[1];
    sb.Offset = p[2];
    sb.Size = p[3];
    const UInt32 align = p[4];
    sb.Align = align;
    if (align > 31)
      return S_FALSE;
    if (sb.Offset < kHeaderSize + num * kRecordSize)
      return S_FALSE;
    if ((sb.Type & ~MACH_CPU_ARCH_ABI64) >= 0x100 ||
        (sb.SubType & ~MACH_CPU_SUBTYPE_LIB64) >= 0x100)
      return S_FALSE;

    const UInt64 endPos = (UInt64)sb.Offset + sb.Size;
    if (endPosMax < endPos)
      endPosMax = endPos;
  }
  _numItems = num;
  _phySize = endPosMax;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */))
{
  COM_TRY_BEGIN
  Close();
  try
  {
    if (Open2(inStream) != S_OK)
      return S_FALSE;
    _stream = inStream;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _stream.Release();
  _numItems = 0;
  _phySize = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _numItems;
  return S_OK;
}

namespace NBe {

static const Byte k_Signature[] = {
    7, 0xCA, 0xFE, 0xBA, 0xBE, 0, 0, 0,
    4, 0xB9, 0xFA, 0xF1, 0x0E };

REGISTER_ARC_I(
  "Mub", "mub", NULL, 0xE2,
  k_Signature,
  0,
  NArcInfoFlags::kMultiSignature,
  NULL)

}

}}

/* ================ unit: CPP/7zip/Archive/NtfsHandler.cpp ================ */
// NtfsHandler.cpp

// amalgamation: header emitted in prologue

// #define SHOW_DEBUG_INFO
// #define SHOW_DEBUG_INFO2

#if defined(SHOW_DEBUG_INFO) || defined(SHOW_DEBUG_INFO2)
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

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#define PRF_UTF16(x) PRF(printf("%S", x))
#else
#define PRF(x)
#define PRF_UTF16(x)
#endif

#ifdef SHOW_DEBUG_INFO2
#define PRF2(x) x
#else
#define PRF2(x)
#endif

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

#define G16(p, dest) dest = Get16(p)
#define G32(p, dest) dest = Get32(p)
#define G64(p, dest) dest = Get64(p)

using namespace NWindows;

namespace NArchive {
namespace Ntfs {

static const wchar_t * const kVirtualFolder_System = L"[SYSTEM]";
static const wchar_t * const kVirtualFolder_Lost_Normal = L"[LOST]";
static const wchar_t * const kVirtualFolder_Lost_Deleted = L"[UNKNOWN]";

static const unsigned kNumSysRecs = 16;

static const unsigned kRecIndex_Volume    = 3;
static const unsigned kRecIndex_RootDir   = 5;
static const unsigned kRecIndex_BadClus   = 8;
static const unsigned kRecIndex_Security  = 9;

struct CHeader
{
  unsigned SectorSizeLog;
  unsigned ClusterSizeLog;
  unsigned MftRecordSizeLog;
  // Byte MediaType;
  // UInt32 NumHiddenSectors;
  UInt64 NumSectors;
  UInt64 NumClusters;
  UInt64 MftCluster;
  UInt64 SerialNumber;
  // UInt16 SectorsPerTrack;
  // UInt16 NumHeads;

  UInt64 GetPhySize_Clusters() const { return NumClusters << ClusterSizeLog; }
  UInt64 GetPhySize_Max() const { return (NumSectors + 1) << SectorSizeLog; }
  UInt32 ClusterSize() const { return (UInt32)1 << ClusterSizeLog; }
  bool Parse(const Byte *p);
};

static int GetLog(UInt32 num)
{
  for (int i = 0; i < 31; i++)
    if (((UInt32)1 << i) == num)
      return i;
  return -1;
}

bool CHeader::Parse(const Byte *p)
{
  if (p[0x1FE] != 0x55 || p[0x1FF] != 0xAA)
    return false;

  // int codeOffset = 0;
  switch (p[0])
  {
    case 0xE9: /* codeOffset = 3 + (Int16)Get16(p + 1); */ break;
    case 0xEB: if (p[2] != 0x90) return false; /* codeOffset = 2 + (int)(signed char)p[1]; */ break;
    default: return false;
  }
  unsigned sectorsPerClusterLog;

  if (memcmp(p + 3, "NTFS    ", 8) != 0)
    return false;
  {
    {
      const int t = GetLog(Get16(p + 11));
      if (t < 9 || t > 12)
        return false;
      SectorSizeLog = (unsigned)t;
    }
    {
      const unsigned v = p[13];
      if (v <= 0x80)
      {
        const int t = GetLog(v);
        if (t < 0)
          return false;
        sectorsPerClusterLog = (unsigned)t;
      }
      else
        sectorsPerClusterLog = 0x100 - v;
      ClusterSizeLog = SectorSizeLog + sectorsPerClusterLog;
      if (ClusterSizeLog > 21)
        return false;
    }
  }

  for (int i = 14; i < 21; i++)
    if (p[i] != 0)
      return false;

  // F8 : a hard disk
  // F0 : high-density 3.5-inch floppy disk
  if (p[21] != 0xF8) // MediaType = Fixed_Disk
    return false;
  if (Get16(p + 22) != 0) // NumFatSectors
    return false;
  // G16(p + 24, SectorsPerTrack); // 63 usually
  // G16(p + 26, NumHeads); // 255
  // G32(p + 28, NumHiddenSectors); // 63 (XP) / 2048 (Vista and win7) / (0 on media that are not partitioned ?)
  if (Get32(p + 32) != 0) // NumSectors32
    return false;

  // DriveNumber = p[0x24];
  if (p[0x25] != 0) // CurrentHead
    return false;
  /*
  NTFS-HDD:   p[0x26] = 0x80
  NTFS-FLASH: p[0x26] = 0
  */
  if (p[0x26] != 0x80 && p[0x26] != 0) // ExtendedBootSig
    return false;
  if (p[0x27] != 0) // reserved
    return false;
  
  NumSectors = Get64(p + 0x28);
  if (NumSectors >= ((UInt64)1 << (62 - SectorSizeLog)))
    return false;

  NumClusters = NumSectors >> sectorsPerClusterLog;

  G64(p + 0x30, MftCluster);   // $MFT.
  // G64(p + 0x38, Mft2Cluster);
  G64(p + 0x48, SerialNumber); // $MFTMirr

  /*
    numClusters_per_MftRecord:
    numClusters_per_IndexBlock:
    only low byte from 4 bytes is used. Another 3 high bytes are zeros.
      If the number is positive (number < 0x80),
          then it represents the number of clusters.
      If the number is negative (number >= 0x80),
          then the size of the file record is 2 raised to the absolute value of this number.
          example: (0xF6 == -10) means 2^10 = 1024 bytes.
  */
  {
    UInt32 numClusters_per_MftRecord;
    G32(p + 0x40, numClusters_per_MftRecord);
    if (numClusters_per_MftRecord >= 0x100 || numClusters_per_MftRecord == 0)
      return false;
    if (numClusters_per_MftRecord < 0x80)
    {
      const int t = GetLog(numClusters_per_MftRecord);
      if (t < 0)
        return false;
      MftRecordSizeLog = (unsigned)t + ClusterSizeLog;
    }
    else
      MftRecordSizeLog = 0x100 - numClusters_per_MftRecord;
    // what exact MFT record sizes are possible and supported by Windows?
    // do we need to change this limit here?
    const unsigned k_MftRecordSizeLog_MAX = 12;
    if (MftRecordSizeLog > k_MftRecordSizeLog_MAX)
      return false;
    if (MftRecordSizeLog < SectorSizeLog)
      return false;
  }
  {
    UInt32 numClusters_per_IndexBlock;
    G32(p + 0x44, numClusters_per_IndexBlock);
    return (numClusters_per_IndexBlock < 0x100);
  }
}

struct CMftRef
{
  UInt64 Val;
  
  UInt64 GetIndex() const { return Val & (((UInt64)1 << 48) - 1); }
  UInt16 GetNumber() const { return (UInt16)(Val >> 48); }
  bool IsBaseItself() const { return Val == 0; }

  CMftRef(): Val(0) {}
};

#define ATNAME(n) ATTR_TYPE_ ## n
#define DEF_ATTR_TYPE(v, n) ATNAME(n) = v

enum
{
  DEF_ATTR_TYPE(0x00, UNUSED),
  DEF_ATTR_TYPE(0x10, STANDARD_INFO),
  DEF_ATTR_TYPE(0x20, ATTRIBUTE_LIST),
  DEF_ATTR_TYPE(0x30, FILE_NAME),
  DEF_ATTR_TYPE(0x40, OBJECT_ID),
  DEF_ATTR_TYPE(0x50, SECURITY_DESCRIPTOR),
  DEF_ATTR_TYPE(0x60, VOLUME_NAME),
  DEF_ATTR_TYPE(0x70, VOLUME_INFO),
  DEF_ATTR_TYPE(0x80, DATA),
  DEF_ATTR_TYPE(0x90, INDEX_ROOT),
  DEF_ATTR_TYPE(0xA0, INDEX_ALLOCATION),
  DEF_ATTR_TYPE(0xB0, BITMAP),
  DEF_ATTR_TYPE(0xC0, REPARSE_POINT),
  DEF_ATTR_TYPE(0xD0, EA_INFO),
  DEF_ATTR_TYPE(0xE0, EA),
  DEF_ATTR_TYPE(0xF0, PROPERTY_SET),
  DEF_ATTR_TYPE(0x100, LOGGED_UTILITY_STREAM),
  DEF_ATTR_TYPE(0x1000, FIRST_USER_DEFINED_ATTRIBUTE)
};


/* WinXP-64:
    Probably only one short name (dos name) per record is allowed.
    There are no short names for hard links.
   The pair (Win32,Dos) can be in any order.
   Posix name can be after or before Win32 name
*/

// static const Byte kFileNameType_Posix     = 0; // for hard links
static const Byte kFileNameType_Win32     = 1; // after Dos name
static const Byte kFileNameType_Dos       = 2; // short name
static const Byte kFileNameType_Win32Dos  = 3; // short and full name are same

struct CFileNameAttr
{
  CMftRef ParentDirRef;

  // Probably these timestamps don't contain some useful timestamps. So we don't use them
  // UInt64 CTime;
  // UInt64 MTime;
  // UInt64 ThisRecMTime;  // xp-64: the time of previous name change (not last name change. why?)
  // UInt64 ATime;
  // UInt64 AllocatedSize;
  // UInt64 DataSize;
  // UInt16 PackedEaSize;
  UString2 Name;
  UInt32 Attrib;
  Byte NameType;
  
  bool IsDos() const { return NameType == kFileNameType_Dos; }
  bool IsWin32() const { return (NameType == kFileNameType_Win32); }

  bool Parse(const Byte *p, unsigned size);

  CFileNameAttr():
      Attrib(0),
      NameType(0)
      {}
};

static void GetString(const Byte *p, const unsigned len, UString2 &res)
{
  if (len == 0 && res.IsEmpty())
    return;
  wchar_t *s = res.GetBuf(len);
  unsigned i;
  for (i = 0; i < len; i++)
  {
    const wchar_t c = Get16(p + i * 2);
    if (c == 0)
      break;
    s[i] = c;
  }
  s[i] = 0;
  res.ReleaseBuf_SetLen(i);
}

bool CFileNameAttr::Parse(const Byte *p, unsigned size)
{
  if (size < 0x42)
    return false;
  G64(p + 0x00, ParentDirRef.Val);
  // G64(p + 0x08, CTime);
  // G64(p + 0x10, MTime);
  // G64(p + 0x18, ThisRecMTime);
  // G64(p + 0x20, ATime);
  // G64(p + 0x28, AllocatedSize);
  // G64(p + 0x30, DataSize);
  G32(p + 0x38, Attrib);
  // G16(p + 0x3C, PackedEaSize);
  NameType = p[0x41];
  const unsigned len = p[0x40];
  if (0x42 + len * 2 > size)
    return false;
  if (len != 0)
    GetString(p + 0x42, len, Name);
  return true;
}

struct CSiAttr
{
  UInt64 CTime;
  UInt64 MTime;
  UInt64 ThisRecMTime;
  UInt64 ATime;
  UInt32 Attrib;

  /*
  UInt32 MaxVersions;
  UInt32 Version;
  UInt32 ClassId;
  UInt32 OwnerId;
  */
  UInt32 SecurityId; // SecurityId = 0 is possible ?
  // UInt64 QuotaCharged;

  bool Parse(const Byte *p, unsigned size);

  CSiAttr():
      CTime(0),
      MTime(0),
      ThisRecMTime(0),
      ATime(0),
      Attrib(0),
      SecurityId(0)
      {}
};


bool CSiAttr::Parse(const Byte *p, unsigned size)
{
  if (size < 0x24)
    return false;
  G64(p + 0x00, CTime);
  G64(p + 0x08, MTime);
  G64(p + 0x10, ThisRecMTime);
  G64(p + 0x18, ATime);
  G32(p + 0x20, Attrib);
  SecurityId = 0;
  if (size >= 0x38)
    G32(p + 0x34, SecurityId);
  return true;
}

static const UInt64 kEmptyExtent = (UInt64)(Int64)-1;

struct CExtent
{
  UInt64 Virt;
  UInt64 Phy;

  bool IsEmpty() const { return Phy == kEmptyExtent; }
};

struct CVolInfo
{
  Byte MajorVer;
  Byte MinorVer;
  // UInt16 Flags;

  bool Parse(const Byte *p, unsigned size);
};

bool CVolInfo::Parse(const Byte *p, unsigned size)
{
  if (size < 12)
    return false;
  MajorVer = p[8];
  MinorVer = p[9];
  // Flags = Get16(p + 10);
  return true;
}

struct CAttr
{
  UInt32 Type;

  Byte NonResident;

  // Non-Resident
  Byte CompressionUnit;

  // UInt32 Len;
  UString2 Name;
  // UInt16 Flags;
  // UInt16 Instance;
  CByteBuffer Data;

  // Non-Resident
  UInt64 LowVcn;
  UInt64 HighVcn;
  UInt64 AllocatedSize;
  UInt64 Size;
  UInt64 PackSize;
  UInt64 InitializedSize;

  // Resident
  // UInt16 ResidentFlags;

  bool IsCompressionUnitSupported() const { return CompressionUnit == 0 || CompressionUnit == 4; }

  UInt32 Parse(const Byte *p, unsigned size);
  bool ParseFileName(CFileNameAttr &a) const { return a.Parse(Data, (unsigned)Data.Size()); }
  bool ParseSi(CSiAttr &a) const { return a.Parse(Data, (unsigned)Data.Size()); }
  bool ParseVolInfo(CVolInfo &a) const { return a.Parse(Data, (unsigned)Data.Size()); }
  bool ParseExtents(CRecordVector<CExtent> &extents, UInt64 numClustersMax, unsigned compressionUnit) const;
  UInt64 GetSize() const { return NonResident ? Size : Data.Size(); }
  UInt64 GetPackSize() const
  {
    if (!NonResident)
      return Data.Size();
    if (CompressionUnit != 0)
      return PackSize;
    return AllocatedSize;
  }
};

#define RINOZ(x) { int _tt_ = (x); if (_tt_ != 0) return _tt_; }

static int CompareAttr(void *const *elem1, void *const *elem2, void *)
{
  const CAttr &a1 = *(*((const CAttr *const *)elem1));
  const CAttr &a2 = *(*((const CAttr *const *)elem2));
  RINOZ(MyCompare(a1.Type, a2.Type))
  if (a1.Name.IsEmpty())
  {
    if (!a2.Name.IsEmpty())
      return -1;
  }
  else if (a2.Name.IsEmpty())
    return 1;
  else
  {
    RINOZ(a1.Name.Compare(a2.Name.GetRawPtr()))
  }
  return MyCompare(a1.LowVcn, a2.LowVcn);
}

UInt32 CAttr::Parse(const Byte *p, unsigned size)
{
  if (size < 4)
    return 0;
  G32(p, Type);
  if (Type == 0xFFFFFFFF)
    return 8; // required size is 4, but attributes are 8 bytes aligned. So we return 8
  if (size < 0x18)
    return 0;

  PRF(printf(" T=%2X", Type));
  
  UInt32 len = Get32(p + 4);
  PRF(printf(" L=%3d", len));
  if (len > size)
    return 0;
  if ((len & 7) != 0)
    return 0;
  NonResident = p[8];
  {
    unsigned nameLen = p[9];
    UInt32 nameOffset = Get16(p + 0x0A);
    if (nameLen != 0)
    {
      if (nameOffset + nameLen * 2 > len)
        return 0;
      GetString(p + nameOffset, nameLen, Name);
      PRF(printf(" N="));
      PRF_UTF16(Name);
    }
  }

  // G16(p + 0x0C, Flags);
  // G16(p + 0x0E, Instance);
  // PRF(printf(" F=%4X", Flags));
  // PRF(printf(" Inst=%d", Instance));

  UInt32 dataSize;
  UInt32 offs;
  
  if (NonResident)
  {
    if (len < 0x40)
      return 0;
    PRF(printf(" NR"));
    G64(p + 0x10, LowVcn);
    G64(p + 0x18, HighVcn);
    G64(p + 0x28, AllocatedSize);
    G64(p + 0x30, Size);
    G64(p + 0x38, InitializedSize);
    G16(p + 0x20, offs);
    CompressionUnit = p[0x22];

    PackSize = Size;
    if (CompressionUnit != 0)
    {
      if (len < 0x48)
        return 0;
      G64(p + 0x40, PackSize);
      PRF(printf(" PS=%I64x", PackSize));
    }

    // PRF(printf("\n"));
    PRF(printf(" ASize=%4I64d", AllocatedSize));
    PRF(printf(" Size=%I64d", Size));
    PRF(printf(" IS=%I64d", InitializedSize));
    PRF(printf(" Low=%I64d", LowVcn));
    PRF(printf(" High=%I64d", HighVcn));
    PRF(printf(" CU=%d", (unsigned)CompressionUnit));
    dataSize = len - offs;
  }
  else
  {
    if (len < 0x18)
      return 0;
    G32(p + 0x10, dataSize);
    G16(p + 0x14, offs);
    // G16(p + 0x16, ResidentFlags);
    PRF(printf(" RES"));
    PRF(printf(" dataSize=%3d", dataSize));
    // PRF(printf(" ResFlags=%4X", ResidentFlags));
  }
  
  if (offs > len || dataSize > len || len - dataSize < offs)
    return 0;
  
  Data.CopyFrom(p + offs, dataSize);
  
  #ifdef SHOW_DEBUG_INFO
  PRF(printf("  : "));
  for (unsigned i = 0; i < Data.Size(); i++)
  {
    PRF(printf(" %02X", (unsigned)Data[i]));
  }
  #endif
  
  return len;
}


bool CAttr::ParseExtents(CRecordVector<CExtent> &extents, UInt64 numClustersMax, unsigned compressionUnit) const
{
  const Byte *p = Data;
  unsigned size = (unsigned)Data.Size();
  UInt64 vcn = LowVcn;
  UInt64 lcn = 0;
  const UInt64 highVcn1 = HighVcn + 1;
  
  if (LowVcn != extents.Back().Virt || highVcn1 > (UInt64)1 << 63)
    return false;

  extents.DeleteBack();

  PRF2(printf("\n# ParseExtents # LowVcn = %4I64X # HighVcn = %4I64X", LowVcn, HighVcn));

  while (size > 0)
  {
    const unsigned b = *p++;
    size--;
    if (b == 0)
      break;
    unsigned num = b & 0xF;
    if (num == 0 || num > 8 || num > size)
      return false;

    UInt64 vSize = 0;
    {
      unsigned i = num;
      do vSize = (vSize << 8) | p[--i]; while (i);
    }
    if (vSize == 0)
      return false;
    p += num;
    size -= num;
    if ((highVcn1 - vcn) < vSize)
      return false;

    CExtent e;
    e.Virt = vcn;
    vcn += vSize;

    num = b >> 4;
    if (num > 8 || num > size)
      return false;
    
    if (num == 0)
    {
      // Sparse
      
      /* if Unit is compressed, it can have many Elements for each compressed Unit:
         and last Element for unit MUST be without LCN.
           Element 0: numCompressedClusters2, LCN_0
           Element 1: numCompressedClusters2, LCN_1
           ...
           Last Element : (16 - total_clusters_in_previous_elements), no LCN
      */
      
      // sparse is not allowed for (compressionUnit == 0) ? Why ?
      if (compressionUnit == 0)
        return false;

      e.Phy = kEmptyExtent;
    }
    else
    {
      Int64 v = (signed char)p[num - 1];
      {
        for (unsigned i = num - 1; i != 0;)
          v = (v << 8) | p[--i];
      }
      p += num;
      size -= num;
      lcn = (UInt64)((Int64)lcn + v);
      if (lcn > numClustersMax)
        return false;
      e.Phy = lcn;
    }
    
    extents.Add(e);
  }

  CExtent e;
  e.Phy = kEmptyExtent;
  e.Virt = vcn;
  extents.Add(e);
  return (highVcn1 == vcn);
}


static const UInt64 kEmptyTag = (UInt64)(Int64)-1;

static const unsigned kNumCacheChunksLog = 1;
static const size_t kNumCacheChunks = (size_t)1 << kNumCacheChunksLog;

Z7_CLASS_IMP_IInStream(
  CInStream
)
  UInt64 _virtPos;
  UInt64 _physPos;
  UInt64 _curRem;
  bool _sparseMode;
public:
  bool InUse;
private:
  unsigned _chunkSizeLog;
  CByteBuffer _inBuf;
  CByteBuffer _outBuf;
public:
  UInt64 Size;
  UInt64 InitializedSize;
  unsigned BlockSizeLog;
  unsigned CompressionUnit;
  CRecordVector<CExtent> Extents;
  CMyComPtr<IInStream> Stream;
private:
  UInt64 _tags[kNumCacheChunks];

  HRESULT SeekToPhys() { return InStream_SeekSet(Stream, _physPos); }
  UInt32 GetCuSize() const { return (UInt32)1 << (BlockSizeLog + CompressionUnit); }
public:
  HRESULT InitAndSeek(unsigned compressionUnit)
  {
    CompressionUnit = compressionUnit;
    _chunkSizeLog = BlockSizeLog + CompressionUnit;
    if (compressionUnit != 0)
    {
      UInt32 cuSize = GetCuSize();
      _inBuf.Alloc(cuSize);
      _outBuf.Alloc(kNumCacheChunks << _chunkSizeLog);
    }
    for (size_t i = 0; i < kNumCacheChunks; i++)
      _tags[i] = kEmptyTag;

    _sparseMode = false;
    _curRem = 0;
    _virtPos = 0;
    _physPos = 0;
    const CExtent &e = Extents[0];
    if (!e.IsEmpty())
      _physPos = e.Phy << BlockSizeLog;
    return SeekToPhys();
  }
};

static size_t Lznt1Dec(Byte *dest, size_t outBufLim, size_t destLen, const Byte *src, size_t srcLen)
{
  size_t destSize = 0;
  while (destSize < destLen)
  {
    if (srcLen < 2 || (destSize & 0xFFF) != 0)
      break;
    UInt32 comprSize;
    {
      const UInt32 v = Get16(src);
      if (v == 0)
        break;
      src += 2;
      srcLen -= 2;
      comprSize = (v & 0xFFF) + 1;
      if (comprSize > srcLen)
        break;
      srcLen -= comprSize;
      if ((v & 0x8000) == 0)
      {
        if (comprSize != (1 << 12))
          break;
        memcpy(dest + destSize, src, comprSize);
        src += comprSize;
        destSize += comprSize;
        continue;
      }
    }
    {
      if (destSize + (1 << 12) > outBufLim || (src[0] & 1) != 0)
        return 0;
      unsigned numDistBits = 4;
      UInt32 sbOffset = 0;
      UInt32 pos = 0;

      do
      {
        comprSize--;
        for (UInt32 mask = src[pos++] | 0x100; mask > 1 && comprSize > 0; mask >>= 1)
        {
          if ((mask & 1) == 0)
          {
            if (sbOffset >= (1 << 12))
              return 0;
            dest[destSize++] = src[pos++];
            sbOffset++;
            comprSize--;
          }
          else
          {
            if (comprSize < 2)
              return 0;
            const UInt32 v = Get16(src + pos);
            pos += 2;
            comprSize -= 2;

            while (((sbOffset - 1) >> numDistBits) != 0)
              numDistBits++;

            UInt32 len = (v & (0xFFFF >> numDistBits)) + 3;
            if (sbOffset + len > (1 << 12))
              return 0;
            UInt32 dist = (v >> (16 - numDistBits));
            if (dist >= sbOffset)
              return 0;
            const size_t offs = 1 + dist;
            Byte *p = dest + destSize - offs;
            destSize += len;
            sbOffset += len;
            const Byte *lim = p + len;
            p[offs] = *p; ++p;
            p[offs] = *p; ++p;
            do
              p[offs] = *p;
            while (++p != lim);
          }
        }
      }
      while (comprSize > 0);
      src += pos;
    }
  }
  return destSize;
}

Z7_COM7F_IMF(CInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= Size)
    return (Size == _virtPos) ? S_OK: E_FAIL;
  if (size == 0)
    return S_OK;
  {
    const UInt64 rem = Size - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
  }

  if (_virtPos >= InitializedSize)
  {
    memset((Byte *)data, 0, size);
    _virtPos += size;
    *processedSize = size;
    return S_OK;
  }

  {
    const UInt64 rem = InitializedSize - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
  }

  while (_curRem == 0)
  {
    const UInt64 cacheTag = _virtPos >> _chunkSizeLog;
    const size_t cacheIndex = (size_t)cacheTag & (kNumCacheChunks - 1);
    
    if (_tags[cacheIndex] == cacheTag)
    {
      const size_t chunkSize = (size_t)1 << _chunkSizeLog;
      const size_t offset = (size_t)_virtPos & (chunkSize - 1);
      size_t cur = chunkSize - offset;
      if (cur > size)
        cur = size;
      memcpy(data, _outBuf + (cacheIndex << _chunkSizeLog) + offset, cur);
      *processedSize = (UInt32)cur;
      _virtPos += cur;
      return S_OK;
    }

    PRF2(printf("\nVirtPos = %6d", _virtPos));
    
    const UInt32 comprUnitSize = (UInt32)1 << CompressionUnit;
    const UInt64 virtBlock = _virtPos >> BlockSizeLog;
    const UInt64 virtBlock2 = virtBlock & ~((UInt64)comprUnitSize - 1);
    
    unsigned left = 0, right = Extents.Size();
    for (;;)
    {
      unsigned mid = (left + right) / 2;
      if (mid == left)
        break;
      if (virtBlock2 < Extents[mid].Virt)
        right = mid;
      else
        left = mid;
    }
    
    bool isCompressed = false;
    const UInt64 virtBlock2End = virtBlock2 + comprUnitSize;
    if (CompressionUnit != 0)
      for (unsigned i = left; i < Extents.Size(); i++)
      {
        const CExtent &e = Extents[i];
        if (e.Virt >= virtBlock2End)
          break;
        if (e.IsEmpty())
        {
          isCompressed = true;
          break;
        }
      }

    unsigned i;
    for (i = left; Extents[i + 1].Virt <= virtBlock; i++);
    
    _sparseMode = false;
    if (!isCompressed)
    {
      const CExtent &e = Extents[i];
      UInt64 newPos = (e.Phy << BlockSizeLog) + _virtPos - (e.Virt << BlockSizeLog);
      if (newPos != _physPos)
      {
        _physPos = newPos;
        RINOK(SeekToPhys())
      }
      UInt64 next = Extents[i + 1].Virt;
      if (next > virtBlock2End)
        next &= ~((UInt64)comprUnitSize - 1);
      next <<= BlockSizeLog;
      if (next > Size)
        next = Size;
      _curRem = next - _virtPos;
      break;
    }
    
    bool thereArePhy = false;
    
    for (unsigned i2 = left; i2 < Extents.Size(); i2++)
    {
      const CExtent &e = Extents[i2];
      if (e.Virt >= virtBlock2End)
        break;
      if (!e.IsEmpty())
      {
        thereArePhy = true;
        break;
      }
    }
    
    if (!thereArePhy)
    {
      _curRem = (Extents[i + 1].Virt << BlockSizeLog) - _virtPos;
      _sparseMode = true;
      break;
    }
    
    size_t offs = 0;
    UInt64 curVirt = virtBlock2;
    
    for (i = left; i < Extents.Size(); i++)
    {
      const CExtent &e = Extents[i];
      if (e.IsEmpty())
        break;
      if (e.Virt >= virtBlock2End)
        return S_FALSE;
      const UInt64 newPos = (e.Phy + (curVirt - e.Virt)) << BlockSizeLog;
      if (newPos != _physPos)
      {
        _physPos = newPos;
        RINOK(SeekToPhys())
      }
      UInt64 numChunks = Extents[i + 1].Virt - curVirt;
      if (curVirt + numChunks > virtBlock2End)
        numChunks = virtBlock2End - curVirt;
      const size_t compressed = (size_t)numChunks << BlockSizeLog;
      RINOK(ReadStream_FALSE(Stream, _inBuf + offs, compressed))
      curVirt += numChunks;
      _physPos += compressed;
      offs += compressed;
    }
    
    const size_t destLenMax = GetCuSize();
    size_t destLen = destLenMax;
    const UInt64 rem = Size - (virtBlock2 << BlockSizeLog);
    if (destLen > rem)
      destLen = (size_t)rem;

    Byte *dest = _outBuf + (cacheIndex << _chunkSizeLog);
    const size_t destSizeRes = Lznt1Dec(dest, destLenMax, destLen, _inBuf, offs);
    _tags[cacheIndex] = cacheTag;

    // some files in Vista have destSize > destLen
    if (destSizeRes < destLen)
    {
      memset(dest, 0, destLenMax);
      if (InUse)
        return S_FALSE;
    }
  }
  
  if (size > _curRem)
    size = (UInt32)_curRem;
  HRESULT res = S_OK;
  if (_sparseMode)
    memset(data, 0, size);
  else
  {
    res = Stream->Read(data, size, &size);
    _physPos += size;
  }
  if (processedSize)
    *processedSize = size;
  _virtPos += size;
  _curRem -= size;
  return res;
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
  if (_virtPos != (UInt64)offset)
  {
    _curRem = 0;
    _virtPos = (UInt64)offset;
  }
  if (newPosition)
    *newPosition = (UInt64)offset;
  return S_OK;
}

static HRESULT DataParseExtents(unsigned clusterSizeLog, const CObjectVector<CAttr> &attrs,
    unsigned attrIndex, unsigned attrIndexLim, UInt64 numPhysClusters, CRecordVector<CExtent> &Extents)
{
  {
    CExtent e;
    e.Virt = 0;
    e.Phy = kEmptyExtent;
    Extents.Add(e);
  }
  
  const CAttr &attr0 = attrs[attrIndex];

  /*
  if (attrs[attrIndexLim - 1].HighVcn + 1 != (attr0.AllocatedSize >> clusterSizeLog))
  {
  }
  */

  if (attr0.AllocatedSize < attr0.Size ||
      (attrs[attrIndexLim - 1].HighVcn + 1) != (attr0.AllocatedSize >> clusterSizeLog) ||
      (attr0.AllocatedSize & ((1 << clusterSizeLog) - 1)) != 0)
    return S_FALSE;
  
  for (unsigned i = attrIndex; i < attrIndexLim; i++)
    if (!attrs[i].ParseExtents(Extents, numPhysClusters, attr0.CompressionUnit))
      return S_FALSE;

  UInt64 packSizeCalc = 0;
  FOR_VECTOR (k, Extents)
  {
    CExtent &e = Extents[k];
    if (!e.IsEmpty())
      packSizeCalc += (Extents[k + 1].Virt - e.Virt) << clusterSizeLog;
    PRF2(printf("\nSize = %4I64X", Extents[k + 1].Virt - e.Virt));
    PRF2(printf("  Pos = %4I64X", e.Phy));
  }
  
  if (attr0.CompressionUnit != 0)
  {
    if (packSizeCalc != attr0.PackSize)
      return S_FALSE;
  }
  else
  {
    if (packSizeCalc != attr0.AllocatedSize)
      return S_FALSE;
  }
  return S_OK;
}

struct CDataRef
{
  unsigned Start;
  unsigned Num;
};

static const UInt32 kMagic_FILE = 0x454C4946;
static const UInt32 kMagic_BAAD = 0x44414142;

// 22.02: we support some rare case magic values:
static const UInt32 kMagic_INDX = 0x58444e49;
static const UInt32 kMagic_HOLE = 0x454c4f48;
static const UInt32 kMagic_RSTR = 0x52545352;
static const UInt32 kMagic_RCRD = 0x44524352;
static const UInt32 kMagic_CHKD = 0x444b4843;
static const UInt32 kMagic_FFFFFFFF = 0xFFFFFFFF;


struct CMftRec
{
  UInt32 Magic;
  // UInt64 Lsn;
  UInt16 SeqNumber;  // Number of times this mft record has been reused
  UInt16 Flags;
  // UInt16 LinkCount;
  // UInt16 NextAttrInstance;
  CMftRef BaseMftRef;
  // UInt32 ThisRecNumber;
  
  UInt32 MyNumNameLinks;
  int MyItemIndex; // index in Items[] of main item  for that record, or -1 if there is no item for that record

  CObjectVector<CAttr> DataAttrs;
  CObjectVector<CFileNameAttr> FileNames;
  CRecordVector<CDataRef> DataRefs;
  // CAttr SecurityAttr;

  CSiAttr SiAttr;
  
  CByteBuffer ReparseData;

  int FindWin32Name_for_DosName(unsigned dosNameIndex) const
  {
    const CFileNameAttr &cur = FileNames[dosNameIndex];
    if (cur.IsDos())
      for (unsigned i = 0; i < FileNames.Size(); i++)
      {
        const CFileNameAttr &next = FileNames[i];
        if (next.IsWin32() && cur.ParentDirRef.Val == next.ParentDirRef.Val)
          return (int)i;
      }
    return -1;
  }

  int FindDosName(unsigned nameIndex) const
  {
    const CFileNameAttr &cur = FileNames[nameIndex];
    if (cur.IsWin32())
      for (unsigned i = 0; i < FileNames.Size(); i++)
      {
        const CFileNameAttr &next = FileNames[i];
        if (next.IsDos() && cur.ParentDirRef.Val == next.ParentDirRef.Val)
          return (int)i;
      }
    return -1;
  }

  /*
  bool IsAltStream(int dataIndex) const
  {
    return dataIndex >= 0 && (
      (IsDir() ||
      !DataAttrs[DataRefs[dataIndex].Start].Name.IsEmpty()));
  }
  */

  void MoveAttrsFrom(CMftRec &src)
  {
    DataAttrs += src.DataAttrs;
    FileNames += src.FileNames;
    src.DataAttrs.ClearAndFree();
    src.FileNames.ClearAndFree();
  }

  UInt64 GetPackSize() const
  {
    UInt64 res = 0;
    FOR_VECTOR (i, DataRefs)
      res += DataAttrs[DataRefs[i].Start].GetPackSize();
    return res;
  }

  bool Parse(Byte *p, unsigned sectorSizeLog, UInt32 numSectors, UInt32 recNumber, CObjectVector<CAttr> *attrs);

  bool Is_Magic_Empty() const
  {
    // what exact Magic values are possible for empty and unused records?
    const UInt32 k_Magic_Unused_MAX = 5; // 22.02
    return (Magic <= k_Magic_Unused_MAX);
  }
  bool Is_Magic_FILE() const { return (Magic == kMagic_FILE); }
  // bool Is_Magic_BAAD() const { return (Magic == kMagic_BAAD); }
  bool Is_Magic_CanIgnore() const
  {
    return Is_Magic_Empty()
        || Magic == kMagic_BAAD
        || Magic == kMagic_INDX
        || Magic == kMagic_HOLE
        || Magic == kMagic_RSTR
        || Magic == kMagic_RCRD
        || Magic == kMagic_CHKD
        || Magic == kMagic_FFFFFFFF;
  }

  bool InUse() const { return (Flags & 1) != 0; }
  bool IsDir() const { return (Flags & 2) != 0; }

  void ParseDataNames();
  HRESULT GetStream(IInStream *mainStream, int dataIndex,
      unsigned clusterSizeLog, UInt64 numPhysClusters, IInStream **stream) const;
  unsigned GetNumExtents(int dataIndex, unsigned clusterSizeLog, UInt64 numPhysClusters) const;

  UInt64 GetSize(unsigned dataIndex) const { return DataAttrs[DataRefs[dataIndex].Start].GetSize(); }

  CMftRec():
      SeqNumber(0),
      Flags(0),
      MyNumNameLinks(0),
      MyItemIndex(-1) {}
};

void CMftRec::ParseDataNames()
{
  DataRefs.Clear();
  DataAttrs.Sort(CompareAttr, NULL);

  for (unsigned i = 0; i < DataAttrs.Size();)
  {
    CDataRef ref;
    ref.Start = i;
    for (i++; i < DataAttrs.Size(); i++)
      if (DataAttrs[ref.Start].Name != DataAttrs[i].Name)
        break;
    ref.Num = i - ref.Start;
    DataRefs.Add(ref);
  }
}

HRESULT CMftRec::GetStream(IInStream *mainStream, int dataIndex,
    unsigned clusterSizeLog, UInt64 numPhysClusters, IInStream **destStream) const
{
  *destStream = NULL;
  CBufferInStream *streamSpec = new CBufferInStream;
  CMyComPtr<IInStream> streamTemp = streamSpec;

  if (dataIndex >= 0)
  if ((unsigned)dataIndex < DataRefs.Size())
  {
    const CDataRef &ref = DataRefs[dataIndex];
    unsigned numNonResident = 0;
    unsigned i;
    for (i = ref.Start; i < ref.Start + ref.Num; i++)
      if (DataAttrs[i].NonResident)
        numNonResident++;

    const CAttr &attr0 = DataAttrs[ref.Start];
      
    if (numNonResident != 0 || ref.Num != 1)
    {
      if (numNonResident != ref.Num || !attr0.IsCompressionUnitSupported())
        return S_FALSE;
      CInStream *ss = new CInStream;
      CMyComPtr<IInStream> streamTemp2 = ss;
      RINOK(DataParseExtents(clusterSizeLog, DataAttrs, ref.Start, ref.Start + ref.Num, numPhysClusters, ss->Extents))
      ss->Size = attr0.Size;
      ss->InitializedSize = attr0.InitializedSize;
      ss->Stream = mainStream;
      ss->BlockSizeLog = clusterSizeLog;
      ss->InUse = InUse();
      RINOK(ss->InitAndSeek(attr0.CompressionUnit))
      *destStream = streamTemp2.Detach();
      return S_OK;
    }
  
    streamSpec->Buf = attr0.Data;
  }

  streamSpec->Init();
  *destStream = streamTemp.Detach();
  return S_OK;
}

unsigned CMftRec::GetNumExtents(int dataIndex, unsigned clusterSizeLog, UInt64 numPhysClusters) const
{
  if (dataIndex < 0)
    return 0;
  {
    const CDataRef &ref = DataRefs[dataIndex];
    unsigned numNonResident = 0;
    unsigned i;
    for (i = ref.Start; i < ref.Start + ref.Num; i++)
      if (DataAttrs[i].NonResident)
        numNonResident++;

    const CAttr &attr0 = DataAttrs[ref.Start];
      
    if (numNonResident != 0 || ref.Num != 1)
    {
      if (numNonResident != ref.Num || !attr0.IsCompressionUnitSupported())
        return 0; // error;
      CRecordVector<CExtent> extents;
      if (DataParseExtents(clusterSizeLog, DataAttrs, ref.Start, ref.Start + ref.Num, numPhysClusters, extents) != S_OK)
        return 0; // error;
      return extents.Size() - 1;
    }
    // if (attr0.Data.Size() != 0)
    //   return 1;
    return 0;
  }
}

bool CMftRec::Parse(Byte *p, unsigned sectorSizeLog, UInt32 numSectors, UInt32 recNumber,
    CObjectVector<CAttr> *attrs)
{
  G32(p, Magic);
  if (!Is_Magic_FILE())
    return Is_Magic_CanIgnore();
  
  {
    UInt32 usaOffset;
    UInt32 numUsaItems;
    G16(p + 0x04, usaOffset);
    G16(p + 0x06, numUsaItems);
      
    /* NTFS stores (usn) to 2 last bytes in each sector (before writing record to disk).
       Original values of these two bytes are stored in table.
       So we restore original data from table */

    if ((usaOffset & 1) != 0
        || usaOffset + numUsaItems * 2 > ((UInt32)1 << sectorSizeLog) - 2
        || numUsaItems == 0
        || numUsaItems - 1 != numSectors)
      return false;

    if (usaOffset >= 0x30) // NTFS 3.1+
    {
      UInt32 iii = Get32(p + 0x2C);
      if (iii != recNumber)
      {
        // ntfs-3g probably writes 0 (that probably is incorrect value) to this field for unused records.
        // so we support that "bad" case.
        if (iii != 0)
          return false;
      }
    }
    
    UInt16 usn = Get16(p + usaOffset);
    // PRF(printf("\nusn = %d", usn));
    for (UInt32 i = 1; i < numUsaItems; i++)
    {
      void *pp = p + (i << sectorSizeLog) - 2;
      if (Get16(pp) != usn)
        return false;
      SetUi16(pp, Get16(p + usaOffset + i * 2))
    }
  }

  // G64(p + 0x08, Lsn);
  G16(p + 0x10, SeqNumber);
  // G16(p + 0x12, LinkCount);
  // PRF(printf(" L=%d", LinkCount));
  const UInt32 attrOffs = Get16(p + 0x14);
  G16(p + 0x16, Flags);
  PRF(printf(" F=%4X", Flags));

  const UInt32 bytesInUse = Get32(p + 0x18);
  const UInt32 bytesAlloc = Get32(p + 0x1C);
  G64(p + 0x20, BaseMftRef.Val);
  if (BaseMftRef.Val != 0)
  {
    PRF(printf("  BaseRef=%d", (int)BaseMftRef.Val));
    // return false; // Check it;
  }
  // G16(p + 0x28, NextAttrInstance);

  UInt32 limit = numSectors << sectorSizeLog;
  if (attrOffs >= limit
      || (attrOffs & 7) != 0
      || (bytesInUse & 7) != 0
      || bytesInUse > limit
      || bytesAlloc != limit)
    return false;

  limit = bytesInUse;

  for (UInt32 t = attrOffs;;)
  {
    if (t >= limit)
      return false;

    CAttr attr;
    // PRF(printf("\n  %2d:", Attrs.Size()));
    PRF(printf("\n"));
    UInt32 len = attr.Parse(p + t, limit - t);
    if (len == 0 || limit - t < len)
      return false;
    t += len;
    if (attr.Type == 0xFFFFFFFF)
    {
      if (t != limit)
        return false;
      break;
    }
    switch (attr.Type)
    {
      case ATTR_TYPE_FILE_NAME:
      {
        CFileNameAttr fna;
        if (!attr.ParseFileName(fna))
          return false;
        FileNames.Add(fna);
        PRF(printf("  flags = %4x\n  ", (int)fna.NameType));
        PRF_UTF16(fna.Name);
        break;
      }
      case ATTR_TYPE_STANDARD_INFO:
        if (!attr.ParseSi(SiAttr))
          return false;
        break;
      case ATTR_TYPE_DATA:
        DataAttrs.Add(attr);
        break;
      case ATTR_TYPE_REPARSE_POINT:
        ReparseData = attr.Data;
        break;
      /*
      case ATTR_TYPE_SECURITY_DESCRIPTOR:
        SecurityAttr = attr;
        break;
      */
      default:
        if (attrs)
          attrs->Add(attr);
        break;
    }
  }

  return true;
}

/*
  NTFS probably creates empty DATA_ATTRIBUTE for empty file,
  But it doesn't do it for
    $Secure (:$SDS),
    $Extend\$Quota
    $Extend\$ObjId
    $Extend\$Reparse
*/
  
static const int k_Item_DataIndex_IsEmptyFile = -1; // file without unnamed data stream
static const int k_Item_DataIndex_IsDir = -2;

// static const int k_ParentFolderIndex_Root = -1;
static const int k_ParentFolderIndex_Lost = -2;
static const int k_ParentFolderIndex_Deleted = -3;

struct CItem
{
  unsigned RecIndex;  // index in Recs array
  unsigned NameIndex; // index in CMftRec::FileNames

  int DataIndex;      /* index in CMftRec::DataRefs
                         -1: file without unnamed data stream
                         -2: for directories */
                         
  int ParentFolder;   /* index in Items array
                         -1: for root items
                         -2: [LOST] folder
                         -3: [UNKNOWN] folder (deleted lost) */
  int ParentHost;     /* index in Items array, if it's AltStream
                         -1: if it's not AltStream */
  
  CItem(): DataIndex(k_Item_DataIndex_IsDir), ParentFolder(-1), ParentHost(-1) {}
  
  bool IsAltStream() const { return ParentHost != -1; }
  bool IsDir() const { return DataIndex == k_Item_DataIndex_IsDir; }
        // check it !!!
        // probably NTFS for empty file still creates empty DATA_ATTRIBUTE
        // But it doesn't do it for $Secure:$SDS
};

struct CDatabase
{
  CRecordVector<CItem> Items;
  CObjectVector<CMftRec> Recs;
  CMyComPtr<IInStream> InStream;
  CHeader Header;
  UInt64 PhySize;

  IArchiveOpenCallback *OpenCallback;

  CByteBuffer ByteBuf;

  CObjectVector<CAttr> VolAttrs;

  CByteBuffer SecurData;
  CRecordVector<size_t> SecurOffsets;

  // bool _headerWarning;
  bool ThereAreAltStreams;

  bool _showSystemFiles;
  bool _showDeletedFiles;
  CObjectVector<UString2> VirtFolderNames;
  UString EmptyString;

  int _systemFolderIndex;
  int _lostFolderIndex_Normal;
  int _lostFolderIndex_Deleted;

  void InitProps()
  {
    _showSystemFiles = true;
    // we show SystemFiles by default since it's difficult to track $Extend\* system files
    // it must be fixed later
    _showDeletedFiles = false;
  }

  CDatabase() { InitProps(); }
  ~CDatabase() { ClearAndClose(); }

  void Clear();
  void ClearAndClose();

  void GetItemPath(unsigned index, NCOM::CPropVariant &path) const;
  HRESULT Open();

  HRESULT SeekToCluster(UInt64 cluster);

  int Find_DirItem_For_MftRec(UInt64 recIndex) const
  {
    if (recIndex >= Recs.Size())
      return -1;
    const CMftRec &rec = Recs[(unsigned)recIndex];
    if (!rec.IsDir())
      return -1;
    return rec.MyItemIndex;
    /*
    unsigned left = 0, right = Items.Size();
    while (left != right)
    {
      unsigned mid = (left + right) / 2;
      const CItem &item = Items[mid];
      UInt64 midValue = item.RecIndex;
      if (recIndex == midValue)
      {
        // if item is not dir (file or alt stream we don't return it)
        // if (item.DataIndex < 0)
        if (item.IsDir())
          return mid;
        right = mid;
      }
      else if (recIndex < midValue)
        right = mid;
      else
        left = mid + 1;
    }
    return -1;
    */
  }

  bool FindSecurityDescritor(UInt32 id, UInt64 &offset, UInt32 &size) const;
  
  HRESULT ParseSecuritySDS_2();
  void ParseSecuritySDS()
  {
    HRESULT res = ParseSecuritySDS_2();
    if (res != S_OK)
    {
      SecurOffsets.Clear();
      SecurData.Free();
    }
  }

};

HRESULT CDatabase::SeekToCluster(UInt64 cluster)
{
  return InStream_SeekSet(InStream, cluster << Header.ClusterSizeLog);
}

void CDatabase::Clear()
{
  Items.Clear();
  Recs.Clear();
  SecurOffsets.Clear();
  SecurData.Free();
  VirtFolderNames.Clear();
  _systemFolderIndex = -1;
  _lostFolderIndex_Normal = -1;
  _lostFolderIndex_Deleted = -1;
  ThereAreAltStreams = false;
  // _headerWarning = false;
  PhySize = 0;
}

void CDatabase::ClearAndClose()
{
  Clear();
  InStream.Release();
}


static void CopyName(wchar_t *dest, const wchar_t *src)
{
  for (;;)
  {
    wchar_t c = *src++;
    // 18.06
    if (c == '\\' || c == '/')
      c = '_';
    *dest++ = c;
    if (c == 0)
      return;
  }
}

void CDatabase::GetItemPath(unsigned index, NCOM::CPropVariant &path) const
{
  const CItem *item = &Items[index];
  unsigned size = 0;
  const CMftRec &rec = Recs[item->RecIndex];
  size += rec.FileNames[item->NameIndex].Name.Len();

  bool isAltStream = item->IsAltStream();

  if (isAltStream)
  {
    const CAttr &data = rec.DataAttrs[rec.DataRefs[item->DataIndex].Start];
    if (item->RecIndex == kRecIndex_RootDir)
    {
      wchar_t *s = path.AllocBstr(data.Name.Len() + 1);
      s[0] = L':';
      if (!data.Name.IsEmpty())
        CopyName(s + 1, data.Name.GetRawPtr());
      return;
    }

    size += data.Name.Len();
    size++;
  }

  for (unsigned i = 0;; i++)
  {
    if (i > 256)
    {
      path = "[TOO-LONG]";
      return;
    }
    const wchar_t *servName;
    if (item->RecIndex < kNumSysRecs
        /* && item->RecIndex != kRecIndex_RootDir */)
      servName = kVirtualFolder_System;
    else
    {
      int index2 = item->ParentFolder;
      if (index2 >= 0)
      {
        item = &Items[index2];
        size += Recs[item->RecIndex].FileNames[item->NameIndex].Name.Len() + 1;
        continue;
      }
      if (index2 == -1)
        break;
      servName = (index2 == k_ParentFolderIndex_Lost) ?
          kVirtualFolder_Lost_Normal :
          kVirtualFolder_Lost_Deleted;
    }
    size += MyStringLen(servName) + 1;
    break;
  }

  wchar_t *s = path.AllocBstr(size);
  
  item = &Items[index];

  bool needColon = false;
  if (isAltStream)
  {
    const UString2 &name = rec.DataAttrs[rec.DataRefs[item->DataIndex].Start].Name;
    if (!name.IsEmpty())
    {
      size -= name.Len();
      CopyName(s + size, name.GetRawPtr());
    }
    s[--size] = ':';
    needColon = true;
  }

  {
    const UString2 &name = rec.FileNames[item->NameIndex].Name;
    unsigned len = name.Len();
    if (len != 0)
      CopyName(s + size - len, name.GetRawPtr());
    if (needColon)
      s[size] =  ':';
    size -= len;
  }

  for (;;)
  {
    const wchar_t *servName;
    if (item->RecIndex < kNumSysRecs
        /* && && item->RecIndex != kRecIndex_RootDir */)
      servName = kVirtualFolder_System;
    else
    {
      int index2 = item->ParentFolder;
      if (index2 >= 0)
      {
        item = &Items[index2];
        const UString2 &name = Recs[item->RecIndex].FileNames[item->NameIndex].Name;
        unsigned len = name.Len();
        size--;
        if (len != 0)
        {
          size -= len;
          CopyName(s + size, name.GetRawPtr());
        }
        s[size + len] = WCHAR_PATH_SEPARATOR;
        continue;
      }
      if (index2 == -1)
        break;
      servName = (index2 == k_ParentFolderIndex_Lost) ?
          kVirtualFolder_Lost_Normal :
          kVirtualFolder_Lost_Deleted;
    }
    MyStringCopy(s, servName);
    s[MyStringLen(servName)] = WCHAR_PATH_SEPARATOR;
    break;
  }
}

bool CDatabase::FindSecurityDescritor(UInt32 item, UInt64 &offset, UInt32 &size) const
{
  offset = 0;
  size = 0;
  unsigned left = 0, right = SecurOffsets.Size();
  while (left != right)
  {
    unsigned mid = (left + right) / 2;
    size_t offs = SecurOffsets[mid];
    UInt32 midValue = Get32(((const Byte *)SecurData) + offs + 4);
    if (item == midValue)
    {
      offset = Get64((const Byte *)SecurData + offs + 8) + 20;
      size = Get32((const Byte *)SecurData + offs + 16) - 20;
      return true;
    }
    if (item < midValue)
      right = mid;
    else
      left = mid + 1;
  }
  return false;
}

/*
static int CompareIDs(const size_t *p1, const size_t *p2, void *data)
{
  UInt32 id1 = Get32(((const Byte *)data) + *p1 + 4);
  UInt32 id2 = Get32(((const Byte *)data) + *p2 + 4);
  return MyCompare(id1, id2);
}
*/

// security data contains duplication copy after each 256 KB.
static const unsigned kSecureDuplicateStepBits = 18;

HRESULT CDatabase::ParseSecuritySDS_2()
{
  const Byte *p = SecurData;
  size_t size = SecurData.Size();
  const size_t kDuplicateStep = (size_t)1 << kSecureDuplicateStepBits;
  const size_t kDuplicateMask = kDuplicateStep - 1;
  size_t lim = MyMin(size, kDuplicateStep);
  UInt32 idPrev = 0;
  for (size_t pos = 0; pos < size && size - pos >= 20;)
  {
    UInt32 id = Get32(p + pos + 4);
    UInt64 offs = Get64(p + pos + 8);
    UInt32 entrySize = Get32(p + pos + 16);
    if (offs == pos && entrySize >= 20 && lim - pos >= entrySize)
    {
      if (id <= idPrev)
        return S_FALSE;
      idPrev = id;
      SecurOffsets.Add(pos);
      pos += entrySize;
      pos = (pos + 0xF) & ~(size_t)0xF;
      if ((pos & kDuplicateMask) != 0)
        continue;
    }
    else
      pos = (pos + kDuplicateStep) & ~kDuplicateMask;
    pos += kDuplicateStep;
    lim = pos + kDuplicateStep;
    if (lim >= size)
      lim = size;
  }
  // we checked that IDs are sorted, so we don't need Sort
  // SecurOffsets.Sort(CompareIDs, (void *)p);
  return S_OK;
}

HRESULT CDatabase::Open()
{
  Clear();

  /* NTFS layout:
     1) main part (as specified by NumClusters). Only that part is available, if we open "\\.\c:"
     2) additional empty sectors (as specified by NumSectors)
     3) the copy of first sector (boot sector)
    
     We support both cases:
      - the file with only main part
      - full file (as raw data on partition), including the copy
        of first sector (boot sector) at the end of data
     
     We don't support the case, when only the copy of boot sector
     at the end was detected as NTFS signature.
  */
  
  {
    const UInt32 kHeaderSize = 512;
    Byte buf[kHeaderSize];
    RINOK(ReadStream_FALSE(InStream, buf, kHeaderSize))
    if (!Header.Parse(buf))
      return S_FALSE;
    
    UInt64 fileSize;
    RINOK(InStream_GetSize_SeekToEnd(InStream, fileSize))
    PhySize = Header.GetPhySize_Clusters();
    if (fileSize < PhySize)
      return S_FALSE;
    
    UInt64 phySizeMax = Header.GetPhySize_Max();
    if (fileSize >= phySizeMax)
    {
      RINOK(InStream_SeekSet(InStream, Header.NumSectors << Header.SectorSizeLog))
      Byte buf2[kHeaderSize];
      if (ReadStream_FALSE(InStream, buf2, kHeaderSize) == S_OK)
      {
        if (memcmp(buf, buf2, kHeaderSize) == 0)
          PhySize = phySizeMax;
        // else _headerWarning = true;
      }
    }
  }
 
  SeekToCluster(Header.MftCluster);

  // we use ByteBuf for records reading.
  // so the size of ByteBuf must be >= mftRecordSize
  const size_t recSize = (size_t)1 << Header.MftRecordSizeLog;
  const size_t kBufSize = MyMax((size_t)(1 << 15), recSize);
  ByteBuf.Alloc(kBufSize);
  RINOK(ReadStream_FALSE(InStream, ByteBuf, recSize))
  {
    const UInt32 allocSize = Get32(ByteBuf + 0x1C);
    if (allocSize != recSize)
      return S_FALSE;
  }
  // MftRecordSizeLog >= SectorSizeLog
  const UInt32 numSectorsInRec = 1u << (Header.MftRecordSizeLog - Header.SectorSizeLog);
  CMyComPtr<IInStream> mftStream;
  CMftRec mftRec;
  {
    if (!mftRec.Parse(ByteBuf, Header.SectorSizeLog, numSectorsInRec, 0, NULL))
      return S_FALSE;
    if (!mftRec.Is_Magic_FILE())
      return S_FALSE;
    mftRec.ParseDataNames();
    if (mftRec.DataRefs.IsEmpty())
      return S_FALSE;
    RINOK(mftRec.GetStream(InStream, 0, Header.ClusterSizeLog, Header.NumClusters, &mftStream))
    if (!mftStream)
      return S_FALSE;
  }

  // CObjectVector<CAttr> SecurityAttrs;

  const UInt64 mftSize = mftRec.DataAttrs[0].Size;
  if ((mftSize >> 4) > Header.GetPhySize_Clusters())
    return S_FALSE;

  {
    const UInt64 numFiles = mftSize >> Header.MftRecordSizeLog;
    if (numFiles > (1 << 30))
      return S_FALSE;
    if (OpenCallback)
    {
      RINOK(OpenCallback->SetTotal(&numFiles, &mftSize))
    }
    Recs.ClearAndReserve((unsigned)numFiles);
  }
  
  for (UInt64 pos64 = 0;;)
  {
    if (OpenCallback)
    {
      const UInt64 numFiles = Recs.Size();
      if ((numFiles & 0x3FFF) == 0)
      {
        RINOK(OpenCallback->SetCompleted(&numFiles, &pos64))
      }
    }
    size_t readSize = kBufSize;
    {
      const UInt64 rem = mftSize - pos64;
      if (readSize > rem)
        readSize = (size_t)rem;
    }
    if (readSize < recSize)
      break;
    RINOK(ReadStream_FALSE(mftStream, ByteBuf, readSize))
    pos64 += readSize;

    for (size_t i = 0; readSize >= recSize; i += recSize, readSize -= recSize)
    {
      PRF(printf("\n---------------------"));
      PRF(printf("\n%5d:", Recs.Size()));
      
      Byte *p = ByteBuf + i;
      CMftRec rec;

      CObjectVector<CAttr> *attrs = NULL;
      unsigned recIndex = Recs.Size();
      switch (recIndex)
      {
        case kRecIndex_Volume: attrs = &VolAttrs; break;
        // case kRecIndex_Security: attrs = &SecurityAttrs; break;
      }

      if (!rec.Parse(p, Header.SectorSizeLog, numSectorsInRec, (UInt32)Recs.Size(), attrs))
        return S_FALSE;
      Recs.Add(rec);
    }
  }

  /*
  // that code looks too complex. And we can get security info without index parsing
  for (i = 0; i < SecurityAttrs.Size(); i++)
  {
    const CAttr &attr = SecurityAttrs[i];
    if (attr.Name.IsEqualTo("$SII"))
    {
      if (attr.Type == ATTR_TYPE_INDEX_ROOT)
      {
        const Byte *data = attr.Data;
        size_t size = attr.Data.Size();

        // Index Root
        UInt32 attrType = Get32(data);
        UInt32 collationRule = Get32(data + 4);
        UInt32 indexAllocationEtrySizeSize = Get32(data + 8);
        UInt32 clustersPerIndexRecord = Get32(data + 0xC);
        data += 0x10;

        // Index Header
        UInt32 firstEntryOffset = Get32(data);
        UInt32 totalSize = Get32(data + 4);
        UInt32 allocSize = Get32(data + 8);
        UInt32 flags = Get32(data + 0xC);

        int num = 0;
        for (int j = 0 ; j < num; j++)
        {
          if (Get32(data) != 0x1414 || // offset and size
              Get32(data + 4) != 0 ||
              Get32(data + 8) != 0x428) // KeySize / EntrySize
            break;
          UInt32 flags = Get32(data + 12);
          UInt32 id = Get32(data + 0x10);
          if (id = Get32(data + 0x18))
            break;
          UInt32 descriptorOffset = Get64(data + 0x1C);
          UInt32 descriptorSize = Get64(data + 0x24);
          data += 0x28;
        }
        // break;
      }
    }
  }
  */

  unsigned i;
  
  for (i = 0; i < Recs.Size(); i++)
  {
    CMftRec &rec = Recs[i];
    if (!rec.Is_Magic_FILE())
      continue;

    if (!rec.BaseMftRef.IsBaseItself())
    {
      const UInt64 refIndex = rec.BaseMftRef.GetIndex();
      if (refIndex >= Recs.Size())
        return S_FALSE;
      CMftRec &refRec = Recs[(unsigned)refIndex];
      if (!refRec.Is_Magic_FILE())
        continue;

      bool moveAttrs = (refRec.SeqNumber == rec.BaseMftRef.GetNumber() && refRec.BaseMftRef.IsBaseItself());
      if (rec.InUse() && refRec.InUse())
      {
        if (!moveAttrs)
          return S_FALSE;
      }
      else if (rec.InUse() || refRec.InUse())
        moveAttrs = false;
      if (moveAttrs)
        refRec.MoveAttrsFrom(rec);
    }
  }

  for (i = 0; i < Recs.Size(); i++)
  {
    CMftRec &rec = Recs[i];
    if (!rec.Is_Magic_FILE())
      continue;
    rec.ParseDataNames();
  }
  
  for (i = 0; i < Recs.Size(); i++)
  {
    CMftRec &rec = Recs[i];
    if (!rec.Is_Magic_FILE() || !rec.BaseMftRef.IsBaseItself())
      continue;
    if (i < kNumSysRecs && !_showSystemFiles)
      continue;
    if (!rec.InUse() && !_showDeletedFiles)
      continue;

    rec.MyNumNameLinks = rec.FileNames.Size();
    
    // printf("\n%4d: ", i);
    
    /* Actually DataAttrs / DataRefs are sorted by name.
       It can not be more than one unnamed stream in DataRefs
       And indexOfUnnamedStream <= 0.
    */

    int indexOfUnnamedStream = -1;
    if (!rec.IsDir())
    {
      FOR_VECTOR (di, rec.DataRefs)
        if (rec.DataAttrs[rec.DataRefs[di].Start].Name.IsEmpty())
        {
          indexOfUnnamedStream = (int)di;
          break;
        }
    }

    if (rec.FileNames.IsEmpty())
    {
      bool needShow = true;
      if (i < kNumSysRecs)
      {
        needShow = false;
        FOR_VECTOR (di, rec.DataRefs)
          if (rec.GetSize(di) != 0)
          {
            needShow = true;
            break;
          }
      }
      if (needShow)
      {
        CFileNameAttr &fna = rec.FileNames.AddNew();
        // we set incorrect ParentDirRef, that will place item to [LOST] folder
        fna.ParentDirRef.Val = (UInt64)(Int64)-1;
        char s[16 + 16];
        ConvertUInt32ToString(i, MyStpCpy(s, "[NONAME]-"));
        fna.Name.SetFromAscii(s);
        fna.NameType = kFileNameType_Win32Dos;
        fna.Attrib = 0;
      }
    }

    // bool isMainName = true;

    FOR_VECTOR (t, rec.FileNames)
    {
      #ifdef SHOW_DEBUG_INFO
      const CFileNameAttr &fna = rec.FileNames[t];
      #endif
      PRF(printf("\n %4d ", (int)fna.NameType));
      PRF_UTF16(fna.Name);
      // PRF(printf("  | "));

      if (rec.FindWin32Name_for_DosName(t) >= 0)
      {
        rec.MyNumNameLinks--;
        continue;
      }
      
      CItem item;
      item.NameIndex = t;
      item.RecIndex = i;
      item.DataIndex = rec.IsDir() ?
          k_Item_DataIndex_IsDir :
            (indexOfUnnamedStream < 0 ?
          k_Item_DataIndex_IsEmptyFile :
          indexOfUnnamedStream);
      
      if (rec.MyItemIndex < 0)
        rec.MyItemIndex = (int)Items.Size();
      item.ParentHost = (int)Items.Add(item);
      
      /* we can use that code to reduce the number of alt streams:
         it will not show how alt streams for hard links. */
      // if (!isMainName) continue; isMainName = false;

      // unsigned numAltStreams = 0;

      FOR_VECTOR (di, rec.DataRefs)
      {
        if (!rec.IsDir() && (int)di == indexOfUnnamedStream)
          continue;

        const UString2 &subName = rec.DataAttrs[rec.DataRefs[di].Start].Name;
        
        PRF(printf("\n alt stream: "));
        PRF_UTF16(subName);

        {
          // $BadClus:$Bad is sparse file for all clusters. So we skip it.
          if (i == kRecIndex_BadClus && subName == L"$Bad")
            continue;
        }

        // numAltStreams++;
        ThereAreAltStreams = true;
        item.DataIndex = (int)di;
        Items.Add(item);
      }
    }
  }
  
  if (Recs.Size() > kRecIndex_Security)
  {
    const CMftRec &rec = Recs[kRecIndex_Security];
    FOR_VECTOR (di, rec.DataRefs)
    {
      const CAttr &attr = rec.DataAttrs[rec.DataRefs[di].Start];
      if (attr.Name == L"$SDS")
      {
        CMyComPtr<IInStream> sdsStream;
        RINOK(rec.GetStream(InStream, (int)di, Header.ClusterSizeLog, Header.NumClusters, &sdsStream))
        if (sdsStream)
        {
          const UInt64 size64 = attr.GetSize();
          if (size64 < (UInt32)1 << 29)
          {
            size_t size = (size_t)size64;
            if ((((size + 1) >> kSecureDuplicateStepBits) & 1) != 0)
            {
              size -= (1 << kSecureDuplicateStepBits);
              SecurData.Alloc(size);
              if (ReadStream_FALSE(sdsStream, SecurData, size) == S_OK)
              {
                ParseSecuritySDS();
                break;
              }
            }
          }
        }
        break;
      }
    }
  }

  bool thereAreUnknownFolders_Normal = false;
  bool thereAreUnknownFolders_Deleted = false;

  for (i = 0; i < Items.Size(); i++)
  {
    CItem &item = Items[i];
    const CMftRec &rec = Recs[item.RecIndex];
    const CFileNameAttr &fn = rec.FileNames[item.NameIndex];
    const CMftRef &parentDirRef = fn.ParentDirRef;
    const UInt64 refIndex = parentDirRef.GetIndex();
    if (refIndex == kRecIndex_RootDir)
      item.ParentFolder = -1;
    else
    {
      int index = Find_DirItem_For_MftRec(refIndex);
      if (index < 0 ||
          Recs[Items[index].RecIndex].SeqNumber != parentDirRef.GetNumber())
      {
        if (Recs[item.RecIndex].InUse())
        {
          thereAreUnknownFolders_Normal = true;
          index = k_ParentFolderIndex_Lost;
        }
        else
        {
          thereAreUnknownFolders_Deleted = true;
          index = k_ParentFolderIndex_Deleted;
        }
      }
      item.ParentFolder = index;
    }
  }
  
  unsigned virtIndex = Items.Size();
  if (_showSystemFiles)
  {
    _systemFolderIndex = (int)(virtIndex++);
    VirtFolderNames.Add(kVirtualFolder_System);
  }
  if (thereAreUnknownFolders_Normal)
  {
    _lostFolderIndex_Normal = (int)(virtIndex++);
    VirtFolderNames.Add(kVirtualFolder_Lost_Normal);
  }
  if (thereAreUnknownFolders_Deleted)
  {
    _lostFolderIndex_Deleted = (int)(virtIndex++);
    VirtFolderNames.Add(kVirtualFolder_Lost_Deleted);
  }

  return S_OK;
}

Z7_class_CHandler_final:
  public IInArchive,
  public IArchiveGetRawProps,
  public IInArchiveGetStream,
  public ISetProperties,
  public CMyUnknownImp,
  public CDatabase
{
  Z7_IFACES_IMP_UNK_4(
      IInArchive,
      IArchiveGetRawProps,
      IInArchiveGetStream,
      ISetProperties)
};

Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *numProps))
{
  *numProps = 2;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *propID))
{
  *name = NULL;
  *propID = index == 0 ? kpidNtReparse : kpidNtSecure;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *parentType))
{
  *parentType = NParentType::kDir;
  int par = -1;

  if (index < Items.Size())
  {
    const CItem &item = Items[index];
    
    if (item.ParentHost >= 0)
    {
      *parentType = NParentType::kAltStream;
      par = (item.RecIndex == kRecIndex_RootDir ? -1 : item.ParentHost);
    }
    else if (item.RecIndex < kNumSysRecs)
    {
      if (_showSystemFiles)
        par = _systemFolderIndex;
    }
    else if (item.ParentFolder >= 0)
      par = item.ParentFolder;
    else if (item.ParentFolder == k_ParentFolderIndex_Lost)
      par = _lostFolderIndex_Normal;
    else if (item.ParentFolder == k_ParentFolderIndex_Deleted)
      par = _lostFolderIndex_Deleted;
  }
  *parent = (UInt32)(Int32)par;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;

  if (propID == kpidName)
  {
    #ifdef MY_CPU_LE
    const UString2 *s;
    if (index >= Items.Size())
      s = &VirtFolderNames[index - Items.Size()];
    else
    {
      const CItem &item = Items[index];
      const CMftRec &rec = Recs[item.RecIndex];
      if (item.IsAltStream())
        s = &rec.DataAttrs[rec.DataRefs[item.DataIndex].Start].Name;
      else
        s = &rec.FileNames[item.NameIndex].Name;
    }
    if (s->IsEmpty())
      *data = (const wchar_t *)EmptyString;
    else
      *data = s->GetRawPtr();
    *dataSize = (s->Len() + 1) * (UInt32)sizeof(wchar_t);
    *propType = PROP_DATA_TYPE_wchar_t_PTR_Z_LE;
    #endif
    return S_OK;
  }

  if (propID == kpidNtReparse)
  {
    if (index >= Items.Size())
      return S_OK;
    const CItem &item = Items[index];
    const CMftRec &rec = Recs[item.RecIndex];
    const CByteBuffer &reparse = rec.ReparseData;

    if (reparse.Size() != 0)
    {
      *dataSize = (UInt32)reparse.Size();
      *propType = NPropDataType::kRaw;
      *data = (const Byte *)reparse;
    }
  }

  if (propID == kpidNtSecure)
  {
    if (index >= Items.Size())
      return S_OK;
    const CItem &item = Items[index];
    const CMftRec &rec = Recs[item.RecIndex];
    if (rec.SiAttr.SecurityId > 0)
    {
      UInt64 offset;
      UInt32 size;
      if (FindSecurityDescritor(rec.SiAttr.SecurityId, offset, size))
      {
        *dataSize = size;
        *propType = NPropDataType::kRaw;
        *data = (const Byte *)SecurData + offset;
      }
    }
  }
  
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  if (index >= Items.Size())
    return S_OK;
  IInStream *stream2;
  const CItem &item = Items[index];
  const CMftRec &rec = Recs[item.RecIndex];
  HRESULT res = rec.GetStream(InStream, item.DataIndex, Header.ClusterSizeLog, Header.NumClusters, &stream2);
  *stream = (ISequentialInStream *)stream2;
  return res;
  COM_TRY_END
}

/*
enum
{
  kpidLink2 = kpidUserDefined,
  kpidLinkType,
  kpidRecMTime,
  kpidRecMTime2,
  kpidMTime2,
  kpidCTime2,
  kpidATime2
};

static const CStatProp kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},

  // { NULL, kpidLink, VT_BSTR},
  
  // { "Link 2", kpidLink2, VT_BSTR},
  // { "Link Type", kpidLinkType, VT_UI2},
  { NULL, kpidINode, VT_UI8},
 
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME},
  
  // { "Record Modified", kpidRecMTime, VT_FILETIME},

  // { "Modified 2", kpidMTime2, VT_FILETIME},
  // { "Created 2", kpidCTime2, VT_FILETIME},
  // { "Accessed 2", kpidATime2, VT_FILETIME},
  // { "Record Modified 2", kpidRecMTime2, VT_FILETIME},

  { NULL, kpidAttrib, VT_UI4},
  { NULL, kpidNumBlocks, VT_UI4},
  { NULL, kpidIsDeleted, VT_BOOL},
};
*/

static const Byte kProps[] =
{
  kpidPath,
  kpidIsDir,
  kpidSize,
  kpidPackSize,
  kpidMTime,
  kpidCTime,
  kpidATime,
  kpidChangeTime,
  kpidAttrib,
  kpidLinks,
  kpidINode,
  kpidNumBlocks,
  kpidNumAltStreams,
  kpidIsAltStream,
  kpidShortName,
  kpidIsDeleted
};

enum
{
  kpidRecordSize = kpidUserDefined
};

static const CStatProp kArcProps[] =
{
  { NULL, kpidVolumeName, VT_BSTR},
  { NULL, kpidFileSystem, VT_BSTR},
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidSectorSize, VT_UI4},
  { "MFT Record Size", kpidRecordSize, VT_UI4},
  { NULL, kpidHeadersSize, VT_UI8},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidId, VT_UI8},
};

/*
static const Byte kArcProps[] =
{
  kpidVolumeName,
  kpidFileSystem,
  kpidClusterSize,
  kpidHeadersSize,
  kpidCTime,

  kpidSectorSize,
  kpidId
  // kpidSectorsPerTrack,
  // kpidNumHeads,
  // kpidHiddenSectors
};
*/

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME

static void NtfsTimeToProp(UInt64 t, NCOM::CPropVariant &prop)
{
  FILETIME ft;
  ft.dwLowDateTime = (DWORD)t;
  ft.dwHighDateTime = (DWORD)(t >> 32);
  prop = ft;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  const CMftRec *volRec = (Recs.Size() > kRecIndex_Volume ? &Recs[kRecIndex_Volume] : NULL);

  switch (propID)
  {
    case kpidClusterSize: prop = Header.ClusterSize(); break;
    case kpidPhySize: prop = PhySize; break;
    /*
    case kpidHeadersSize:
    {
      UInt64 val = 0;
      for (unsigned i = 0; i < kNumSysRecs; i++)
      {
        printf("\n%2d: %8I64d ", i, Recs[i].GetPackSize());
        if (i == 8)
          i = i
        val += Recs[i].GetPackSize();
      }
      prop = val;
      break;
    }
    */
    case kpidCTime: if (volRec) NtfsTimeToProp(volRec->SiAttr.CTime, prop); break;
    case kpidMTime: if (volRec) NtfsTimeToProp(volRec->SiAttr.MTime, prop); break;
    case kpidShortComment:
    case kpidVolumeName:
    {
      FOR_VECTOR (i, VolAttrs)
      {
        const CAttr &attr = VolAttrs[i];
        if (attr.Type == ATTR_TYPE_VOLUME_NAME)
        {
          UString2 name;
          GetString(attr.Data, (unsigned)attr.Data.Size() / 2, name);
          if (!name.IsEmpty())
            prop = name.GetRawPtr();
          break;
        }
      }
      break;
    }
    case kpidFileSystem:
    {
      AString s ("NTFS");
      FOR_VECTOR (i, VolAttrs)
      {
        const CAttr &attr = VolAttrs[i];
        if (attr.Type == ATTR_TYPE_VOLUME_INFO)
        {
          CVolInfo vi;
          if (attr.ParseVolInfo(vi))
          {
            s.Add_Space();
            s.Add_UInt32(vi.MajorVer);
            s.Add_Dot();
            s.Add_UInt32(vi.MinorVer);
          }
          break;
        }
      }
      prop = s;
      break;
    }
    case kpidSectorSize: prop = (UInt32)1 << Header.SectorSizeLog; break;
    case kpidRecordSize: prop = (UInt32)1 << Header.MftRecordSizeLog; break;
    case kpidId: prop = Header.SerialNumber; break;

    case kpidIsTree: prop = true; break;
    case kpidIsDeleted: prop = _showDeletedFiles; break;
    case kpidIsAltStream: prop = ThereAreAltStreams; break;
    case kpidIsAux: prop = true; break;
    case kpidINode: prop = true; break;

    case kpidWarning:
      if (_lostFolderIndex_Normal >= 0)
        prop = "There are lost files";
      break;

    /*
    case kpidWarningFlags:
    {
      UInt32 flags = 0;
      if (_headerWarning)
        flags |= k_ErrorFlags_HeadersError;
      if (flags != 0)
        prop = flags;
      break;
    }
    */
      
    // case kpidMediaType: prop = Header.MediaType; break;
    // case kpidSectorsPerTrack: prop = Header.SectorsPerTrack; break;
    // case kpidNumHeads: prop = Header.NumHeads; break;
    // case kpidHiddenSectors: prop = Header.NumHiddenSectors; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  if (index >= Items.Size())
  {
    switch (propID)
    {
      case kpidName:
      case kpidPath:
        prop = VirtFolderNames[index - Items.Size()].GetRawPtr();
        break;
      case kpidIsDir: prop = true; break;
      case kpidIsAux: prop = true; break;
      case kpidIsDeleted:
        if ((int)index == _lostFolderIndex_Deleted)
          prop = true;
        break;
    }
    prop.Detach(value);
    return S_OK;
  }

  const CItem &item = Items[index];
  const CMftRec &rec = Recs[item.RecIndex];

  const CAttr *data= NULL;
  if (item.DataIndex >= 0)
    data = &rec.DataAttrs[rec.DataRefs[item.DataIndex].Start];

  // const CFileNameAttr *fn = &rec.FileNames[item.NameIndex];
  /*
  if (rec.FileNames.Size() > 0)
    fn = &rec.FileNames[0];
  */

  switch (propID)
  {
    case kpidPath:
      GetItemPath(index, prop);
      break;

    /*
    case kpidLink:
      if (!rec.ReparseAttr.SubsName.IsEmpty())
      {
        prop = rec.ReparseAttr.SubsName;
      }
      break;
    case kpidLink2:
      if (!rec.ReparseAttr.PrintName.IsEmpty())
      {
        prop = rec.ReparseAttr.PrintName;
      }
      break;

    case kpidLinkType:
      if (rec.ReparseAttr.Tag != 0)
      {
        prop = (rec.ReparseAttr.Tag & 0xFFFF);
      }
      break;
    */
    
    case kpidINode:
    {
      // const CMftRec &rec = Recs[item.RecIndex];
      // prop = ((UInt64)rec.SeqNumber << 48) | item.RecIndex;
      prop = (UInt32)item.RecIndex;
      break;
    }
    case kpidStreamId:
    {
      if (item.DataIndex >= 0)
        prop = ((UInt64)item.RecIndex << 32) | (unsigned)item.DataIndex;
      break;
    }

    case kpidName:
    {
      const UString2 *s;
      if (item.IsAltStream())
        s = &rec.DataAttrs[rec.DataRefs[item.DataIndex].Start].Name;
      else
        s = &rec.FileNames[item.NameIndex].Name;
      if (s->IsEmpty())
        prop = (const wchar_t *)EmptyString;
      else
        prop = s->GetRawPtr();
      break;
    }

    case kpidShortName:
    {
      if (!item.IsAltStream())
      {
        int dosNameIndex = rec.FindDosName(item.NameIndex);
        if (dosNameIndex >= 0)
        {
          const UString2 &s = rec.FileNames[dosNameIndex].Name;
          if (s.IsEmpty())
            prop = (const wchar_t *)EmptyString;
          else
            prop = s.GetRawPtr();
        }
      }
      break;
    }

    case kpidIsDir: prop = item.IsDir(); break;
    case kpidIsAltStream: prop = item.IsAltStream(); break;
    case kpidIsDeleted: prop = !rec.InUse(); break;
    case kpidIsAux: prop = false; break;

    case kpidMTime: NtfsTimeToProp(rec.SiAttr.MTime, prop); break;
    case kpidCTime: NtfsTimeToProp(rec.SiAttr.CTime, prop); break;
    case kpidATime: NtfsTimeToProp(rec.SiAttr.ATime, prop); break;
    case kpidChangeTime: NtfsTimeToProp(rec.SiAttr.ThisRecMTime, prop); break;

    /*
    case kpidMTime2: if (fn) NtfsTimeToProp(fn->MTime, prop); break;
    case kpidCTime2: if (fn) NtfsTimeToProp(fn->CTime, prop); break;
    case kpidATime2: if (fn) NtfsTimeToProp(fn->ATime, prop); break;
    case kpidRecMTime2: if (fn) NtfsTimeToProp(fn->ThisRecMTime, prop); break;
    */
      
    case kpidAttrib:
    {
      UInt32 attrib;
      /* WinXP-64: The CFileNameAttr::Attrib is not updated  after some changes. Why?
         CSiAttr:attrib is updated better. So we use CSiAttr:Sttrib */
      /*
      if (fn)
        attrib = fn->Attrib;
      else
      */
        attrib = rec.SiAttr.Attrib;
      if (item.IsDir())
        attrib |= FILE_ATTRIBUTE_DIRECTORY;

      /* some system entries can contain extra flags (Index View).
      // 0x10000000   (Directory)
      // 0x20000000   FILE_ATTR_VIEW_INDEX_PRESENT MFT_RECORD_IS_VIEW_INDEX (Index View)
      But we don't need them */
      attrib &= 0xFFFF;

      prop = attrib;
      break;
    }
    case kpidLinks: if (rec.MyNumNameLinks != 1) prop = rec.MyNumNameLinks; break;
    
    case kpidNumAltStreams:
    {
      if (!item.IsAltStream())
      {
        unsigned num = rec.DataRefs.Size();
        if (num > 0)
        {
          if (!rec.IsDir() && rec.DataAttrs[rec.DataRefs[0].Start].Name.IsEmpty())
            num--;
          if (num > 0)
            prop = (UInt32)num;
        }
      }
      break;
    }
    
    case kpidSize: if (data) prop = data->GetSize(); else if (!item.IsDir()) prop = (UInt64)0; break;
    case kpidPackSize: if (data) prop = data->GetPackSize(); else if (!item.IsDir()) prop = (UInt64)0; break;
    case kpidNumBlocks: if (data) prop = (UInt32)rec.GetNumExtents(item.DataIndex, Header.ClusterSizeLog, Header.NumClusters); break;
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
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = Items.Size();
  if (numItems == 0)
    return S_OK;
  UInt32 i;
  UInt64 totalSize = 0;
  for (i = 0; i < numItems; i++)
  {
    const UInt32 index = allFilesMode ? i : indices[i];
    if (index >= (UInt32)Items.Size())
      continue;
    const CItem &item = Items[allFilesMode ? i : indices[i]];
    const CMftRec &rec = Recs[item.RecIndex];
    if (item.DataIndex >= 0)
      totalSize += rec.GetSize((unsigned)item.DataIndex);
  }
  RINOK(extractCallback->SetTotal(totalSize))

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;
  
  UInt32 clusterSize = Header.ClusterSize();
  CByteBuffer buf(clusterSize);

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyComPtr2_Create<ISequentialOutStream, CDummyOutStream> outStream;

  for (i = 0;; i++)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;

    CMyComPtr<ISequentialOutStream> realOutStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode))

    if (index >= (UInt32)Items.Size() || Items[index].IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode))
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      continue;
    }

    const CItem &item = Items[index];

    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode))

    outStream->SetStream(realOutStream);
    realOutStream.Release();
    outStream->Init();

    const CMftRec &rec = Recs[item.RecIndex];

    int res = NExtract::NOperationResult::kDataError;
    {
      CMyComPtr<IInStream> inStream;
      HRESULT hres = rec.GetStream(InStream, item.DataIndex, Header.ClusterSizeLog, Header.NumClusters, &inStream);
      if (hres == S_FALSE)
        res = NExtract::NOperationResult::kUnsupportedMethod;
      else
      {
        RINOK(hres)
        if (inStream)
        {
          hres = copyCoder.Interface()->Code(inStream, outStream, NULL, NULL, lps);
          if (hres != S_OK &&  hres != S_FALSE)
          {
            RINOK(hres)
          }
          if (/* copyCoderSpec->TotalSize == item.GetSize() && */ hres == S_OK)
            res = NExtract::NOperationResult::kOK;
        }
      }
    }
    if (item.DataIndex >= 0)
    {
      const CAttr &data = rec.DataAttrs[rec.DataRefs[item.DataIndex].Start];
      totalPackSize += data.GetPackSize();
      totalSize += data.GetSize();
    }
    outStream->ReleaseStream();
    RINOK(extractCallback->SetOperationResult(res))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = Items.Size() + VirtFolderNames.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  InitProps();

  for (UInt32 i = 0; i < numProps; i++)
  {
    const wchar_t *name = names[i];
    const PROPVARIANT &prop = values[i];

    if (StringsAreEqualNoCase_Ascii(name, "ld"))
    {
      RINOK(PROPVARIANT_to_bool(prop, _showDeletedFiles))
    }
    else if (StringsAreEqualNoCase_Ascii(name, "ls"))
    {
      RINOK(PROPVARIANT_to_bool(prop, _showSystemFiles))
    }
    else if (IsString1PrefixedByString2_NoCase_Ascii(name, "mt"))
    {
    }
    else if (IsString1PrefixedByString2_NoCase_Ascii(name, "memuse"))
    {
    }
    else
      return E_INVALIDARG;
  }
  return S_OK;
}

static const Byte k_Signature[] = { 'N', 'T', 'F', 'S', ' ', ' ', ' ', ' ', 0 };

REGISTER_ARC_I(
  "NTFS", "ntfs img", NULL, 0xD9,
  k_Signature,
  3,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/PeHandler.cpp ================ */
// PeHandler.cpp

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

// amalgamation: header emitted in prologue

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

#define G16(offs, v) v = Get16(p + (offs))
#define G32(offs, v) v = Get32(p + (offs))
#define G32_signed(offs, v) v = (Int32)Get32(p + (offs))
#define G64(offs, v) v = Get64(p + (offs))

#define RINOZ(x) { int _tt_ = (x); if (_tt_ != 0) return _tt_; }

using namespace NWindows;

namespace NArchive {
namespace NPe {

static const UInt32 k_Signature32 = 0x00004550;

static HRESULT CalcCheckSum(ISequentialInStream *stream, UInt32 size, UInt32 excludePos, UInt32 &res)
{
  const UInt32 kBufSizeMax = (UInt32)1 << 15;
  UInt32 bufSize = kBufSizeMax;
  CByteBuffer buffer(bufSize);
  Byte *buf = buffer;
  UInt32 sum = 0;
  UInt32 pos = 0;
  for (;;)
  {
    UInt32 rem = size - pos;
    if (rem > bufSize)
      rem = bufSize;
    if (rem == 0)
      break;
    size_t processed = rem;
    RINOK(ReadStream(stream, buf, &processed))
    
    for (unsigned j = 0; j < 4; j++)
    {
      UInt32 e = excludePos + j;
      if (pos <= e)
      {
        e -= pos;
        if (e < processed)
          buf[e] = 0;
      }
    }

    const unsigned kStep = (1 << 4);
    {
      for (size_t i = processed; (i & (kStep - 1)) != 0; i++)
        buf[i] = 0;
    }
    {
      const Byte *buf2 = buf;
      const Byte *bufLimit = buf + processed;
      UInt64 sum2 = 0;
      for (; buf2 < bufLimit; buf2 += kStep)
      {
        UInt64 sum3 = (UInt64)Get32(buf2)
            + Get32(buf2 + 4)
            + Get32(buf2 + 8)
            + Get32(buf2 + 12);
        sum2 += sum3;
      }
      sum2 = (UInt32)(sum2) + (UInt64)(sum2 >> 32);
      UInt32 sum3 = ((UInt32)sum2 + (UInt32)(sum2 >> 32));
      sum += (sum3 & 0xFFFF) + (sum3 >> 16);
      sum = (sum & 0xFFFF) + (sum >> 16);
      sum = (sum & 0xFFFF) + (sum >> 16);
    }

    pos += (UInt32)processed;
    if (rem != processed)
      break;
  }
  res = sum + pos;
  return S_OK;
}


struct CVersion
{
  UInt16 Major;
  UInt16 Minor;

  void Parse(const Byte *p)
  {
    G16(0, Major);
    G16(2, Minor);
  }
  void ToProp(NCOM::CPropVariant &prop);
};

void CVersion::ToProp(NCOM::CPropVariant &prop)
{
  char sz[32];
  ConvertUInt32ToString(Major, sz);
  unsigned len = MyStringLen(sz);
  sz[len] = '.';
  ConvertUInt32ToString(Minor, sz + len + 1);
  prop = sz;
}

static const unsigned kCoffHeaderSize = 20;
static const unsigned kPeHeaderSize = 4 + kCoffHeaderSize;
static const unsigned k_OptHeader32_Size_MIN = 96;
static const unsigned k_OptHeader64_Size_MIN = 112;

static const UInt32 PE_IMAGE_FILE_DLL  = (1 << 13);

struct CHeader
{
  UInt16 Machine;
  UInt16 NumSections;
  UInt32 Time;
  UInt32 PointerToSymbolTable;
  UInt32 NumSymbols;
  UInt16 OptHeaderSize;
  UInt16 Flags;

  void ParseBase(const Byte *p);
  bool ParseCoff(const Byte *p);
  bool ParsePe(const Byte *p);
  bool IsDll() const { return (Flags & PE_IMAGE_FILE_DLL) != 0; }
};

void CHeader::ParseBase(const Byte *p)
{
  G16( 0, Machine);
  G16( 2, NumSections);
  G32( 4, Time);
  G32( 8, PointerToSymbolTable);
  G32(12, NumSymbols);
  G16(16, OptHeaderSize);
  G16(18, Flags);
}

bool CHeader::ParsePe(const Byte *p)
{
  if (Get32(p) != k_Signature32)
    return false;
  ParseBase(p + 4);
  return OptHeaderSize >= k_OptHeader32_Size_MIN;
}

struct CDirLink
{
  UInt32 Va;
  UInt32 Size;
  
  CDirLink(): Va(0), Size(0) {}
  void Parse(const Byte *p)
  {
    G32(0, Va);
    G32(4, Size);
  }
};


// IMAGE_DIRECTORY_ENTRY_*
static const char * const g_Dir_Names[] =
{
    "EXPORT"
  , "IMPORT"
  , "RESOURCE"
  , "EXCEPTION"
  , "SECURITY"
  , "BASERELOC"
  , "DEBUG"
  , "ARCHITECTURE" // "COPYRIGHT"
  , "GLOBALPTR"
  , "TLS"
  , "LOAD_CONFIG"
  , "BOUND_IMPORT"
  , "IAT"
  , "DELAY_IMPORT"
  , "COM_DESCRIPTOR"
};

enum
{
  kDirLink_EXCEPTION = 3,
  kDirLink_Certificate = 4,
  kDirLink_BASERELOC = 5,
  kDirLink_Debug = 6
};

static const UInt32 kNumDirItemsMax = 16;

struct CDebugEntry
{
  UInt32 Flags;
  UInt32 Time;
  CVersion Ver;
  UInt32 Type;
  UInt32 Size;
  UInt32 Va;
  UInt32 Pa;
  
  void Parse(const Byte *p)
  {
    G32(0, Flags);
    G32(4, Time);
    Ver.Parse(p + 8);
    G32(12, Type);
    G32(16, Size);
    G32(20, Va);
    G32(24, Pa);
  }
};

static const UInt32 k_CheckSum_Field_Offset = 64;

static const UInt32 PE_OptHeader_Magic_32 = 0x10B;
static const UInt32 PE_OptHeader_Magic_64 = 0x20B;

static const UInt32 k_SubSystems_EFI_First = 10;
static const UInt32 k_SubSystems_EFI_Last = 13;

struct COptHeader
{
  UInt16 Magic;
  Byte LinkerVerMajor;
  Byte LinkerVerMinor;

  UInt32 CodeSize;
  UInt32 InitDataSize;
  UInt32 UninitDataSize;
  
  // UInt32 AddressOfEntryPoint;
  // UInt32 BaseOfCode; //  VA(.text) == 0x1000 in most cases
  // UInt32 BaseOfData32;
  UInt64 ImageBase;

  UInt32 SectAlign;
  UInt32 FileAlign;

  CVersion OsVer;
  CVersion ImageVer;
  CVersion SubsysVer;
  
  UInt32 ImageSize;
  UInt32 HeadersSize;
  UInt32 CheckSum;
  UInt16 SubSystem;
  UInt16 DllCharacts;

  UInt64 StackReserve;
  UInt64 StackCommit;
  UInt64 HeapReserve;
  UInt64 HeapCommit;

  UInt32 NumDirItems;
  CDirLink DirItems[kNumDirItemsMax];

  bool Is64Bit() const { return Magic == PE_OptHeader_Magic_64; }
  bool Parse(const Byte *p, UInt32 size);

  int GetNumFileAlignBits() const
  {
    for (unsigned i = 0; i < 32; i++)
      if (((UInt32)1 << i) == FileAlign)
        return (int)i;
    return -1;
  }

  bool IsSybSystem_EFI() const
  {
    return
        SubSystem >= k_SubSystems_EFI_First &&
        SubSystem <= k_SubSystems_EFI_Last;
  }
};

// size is 16-bit
bool COptHeader::Parse(const Byte *p, UInt32 size)
{
  if (size < k_OptHeader32_Size_MIN)
    return false;
  Magic = Get16(p);
  switch (Magic)
  {
    case PE_OptHeader_Magic_32:
    case PE_OptHeader_Magic_64:
      break;
    default:
      return false;
  }
  LinkerVerMajor = p[2];
  LinkerVerMinor = p[3];
  
  G32( 4, CodeSize);
  G32( 8, InitDataSize);
  G32(12, UninitDataSize);
  // G32(16, AddressOfEntryPoint);
  // G32(20, BaseOfCode);
  
  G32(32, SectAlign);
  G32(36, FileAlign);

  OsVer.Parse(p + 40);
  ImageVer.Parse(p + 44);
  SubsysVer.Parse(p + 48);

  // reserved = Get32(p + 52);

  G32(56, ImageSize);
  G32(60, HeadersSize);
  G32(64, CheckSum);
  G16(68, SubSystem);
  G16(70, DllCharacts);

  UInt32 pos;
  if (Is64Bit())
  {
    if (size < k_OptHeader64_Size_MIN)
      return false;
    // BaseOfData32 = 0;
    G64(24, ImageBase);
    G64(72, StackReserve);
    G64(80, StackCommit);
    G64(88, HeapReserve);
    G64(96, HeapCommit);
    pos = 108;
  }
  else
  {
    // G32(24, BaseOfData32);
    G32(28, ImageBase);
    G32(72, StackReserve);
    G32(76, StackCommit);
    G32(80, HeapReserve);
    G32(84, HeapCommit);
    pos = 92;
  }

  UInt32 numDirItems;
  G32(pos, numDirItems);
  NumDirItems = numDirItems;
  if (numDirItems > (1 << 13))
    return false;
  pos += 4;
  if (pos + 8 * numDirItems > size)
    return false;
  memset((void *)DirItems, 0, sizeof(DirItems));
  if (numDirItems > kNumDirItemsMax)
      numDirItems = kNumDirItemsMax;
  for (UInt32 i = 0; i < numDirItems; i++)
    DirItems[i].Parse(p + pos + i * 8);
  return true;
}

static const UInt32 kSectionSize = 40;

struct CSection
{
  AString Name;

  UInt32 ExtractSize;
  UInt32 VSize;
  UInt32 Va;
  UInt32 PSize;
  UInt32 Pa;
  UInt32 Flags;
  UInt32 Time;
  // UInt16 NumRelocs; // is set to zero for executable images
  bool IsRealSect;
  bool IsDebug;
  bool IsAdditionalSection;

  CSection():
    ExtractSize(0),
    IsRealSect(false),
    IsDebug(false),
    IsAdditionalSection(false)
    // , NumRelocs(0)
    {}

  void Set_Size_for_all(UInt32 size)
  {
    PSize = VSize = ExtractSize = size;
  }

  UInt32 GetSize_Extract() const
  {
    return ExtractSize;
  }

  void UpdateTotalSize(UInt32 &totalSize) const
  {
    const UInt32 t = Pa + PSize;
    if (totalSize < t)
        totalSize = t;
  }
  
  void Parse(const Byte *p);

  int Compare(const CSection &s) const
  {
    RINOZ(MyCompare(Pa, s.Pa))
    const UInt32 size1 = GetSize_Extract();
    const UInt32 size2 = s.GetSize_Extract();
    return MyCompare(size1, size2);
  }
};

static const unsigned kNameSize = 8;

static void GetName(const Byte *name, AString &res)
{
  res.SetFrom_CalcLen((const char *)name, kNameSize);
}

void CSection::Parse(const Byte *p)
{
  GetName(p, Name);
  G32( 8, VSize);
  G32(12, Va);
  G32(16, PSize);
  G32(20, Pa);
  // G16(32, NumRelocs);
  G32(36, Flags);
  // v24.08: we extract only useful data (without extra padding bytes).
  // VSize == 0 is not expected, but we support that case too.
  // return (VSize && VSize < PSize) ? VSize : PSize;
  ExtractSize = (VSize && VSize < PSize) ? VSize : PSize;
}



// IMAGE_FILE_*

static const CUInt32PCharPair g_HeaderCharacts[] =
{
  {  1, "Executable" },
  { 13, "DLL" },
  {  8, "32-bit" },
  {  5, "LargeAddress" },
  {  0, "NoRelocs" },
  {  2, "NoLineNums" },
  {  3, "NoLocalSyms" },
  {  4, "AggressiveWsTrim" },
  {  9, "NoDebugInfo" },
  { 10, "RemovableRun" },
  { 11, "NetRun" },
  { 12, "System" },
  { 14, "UniCPU" },
  {  7, "Little-Endian" },
  { 15, "Big-Endian" }
};

// IMAGE_DLLCHARACTERISTICS_*

static const char * const g_DllCharacts[] =
{
    NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , "HighEntropyVA"
  , "Relocated"
  , "Integrity"
  , "NX-Compatible"
  , "NoIsolation"
  , "NoSEH"
  , "NoBind"
  , "AppContainer"
  , "WDM"
  , "GuardCF"
  , "TerminalServerAware"
};


// IMAGE_SCN_* constants:

static const char * const g_SectFlags[] =
{
    NULL
  , NULL
  , NULL
  , "NoPad"
  , NULL
  , "Code"
  , "InitializedData"
  , "UninitializedData"
  , "Other"
  , "Comments"
  , NULL // OVER
  , "Remove"
  , "COMDAT"
  , NULL
  , "NO_DEFER_SPEC_EXC"
  , "GP" // MEM_FARDATA
  , NULL // SYSHEAP
  , "PURGEABLE" // 16BIT
  , "LOCKED"
  , "PRELOAD"
  , NULL
  , NULL
  , NULL
  , NULL
  , "ExtendedRelocations"
  , "Discardable"
  , "NotCached"
  , "NotPaged"
  , "Shared"
  , "Execute"
  , "Read"
  , "Write"
};

static const CUInt32PCharPair g_MachinePairs[] =
{
  { 0x014C, "x86" },
  { 0x014D, "I860" },
  { 0x0162, "MIPS-R3000" },
  { 0x0166, "MIPS-R4000" },
  { 0x0168, "MIPS-R10000" },
  { 0x0169, "MIPS-V2" },
  { 0x0184, "Alpha" },
  { 0x01A2, "SH3" },
  { 0x01A3, "SH3-DSP" },
  { 0x01A4, "SH3E" },
  { 0x01A6, "SH4" },
  { 0x01A8, "SH5" },
  { 0x01C0, "ARM" },
  { 0x01C2, "ARM-Thumb" },
  { 0x01C4, "ARM-NT" },
  { 0x01D3, "AM33" },
  { 0x01F0, "PPC" },
  { 0x01F1, "PPC-FP" },
  { 0x01F2, "PPC-BE" },
  { 0x0200, "IA-64" },
  { 0x0266, "MIPS-16" },
  { 0x0284, "Alpha-64" },
  { 0x0366, "MIPS-FPU" },
  { 0x0466, "MIPS-FPU16" },
  { 0x0520, "TriCore" },
  { 0x0CEF, "CEF" },
  { 0x0EBC, "EFI" },
  { 0x5032, "RISCV32" },
  { 0x5064, "RISCV64" },
//  { 0x5128, "RISCV128" },
  { 0x6232, "LOONGARCH32" },
  { 0x6264, "LOONGARCH64" },
  { 0x8664, "x64" },
  { 0x9041, "M32R" },
  { 0xA641, "ARM64EC" },
  { 0xA64e, "ARM64X" },
  { 0xAA64, "ARM64" },
  { 0xC0EE, "CEE" }
};

static const char * const g_SubSystems[] =
{
    "Unknown"
  , "Native"
  , "Windows GUI"
  , "Windows CUI"
  , NULL // "Old Windows CE"
  , "OS2"
  , NULL
  , "Posix"
  , "Win9x"
  , "Windows CE"
  , "EFI"
  , "EFI Boot"
  , "EFI Runtime"
  , "EFI ROM"
  , "XBOX"
  , NULL
  , "Windows Boot"
  , "XBOX Catalog" // 17
};

static const char * const g_ResTypes[] =
{
    NULL
  , "CURSOR"
  , "BITMAP"
  , "ICON"
  , "MENU"
  , "DIALOG"
  , "STRING"
  , "FONTDIR"
  , "FONT"
  , "ACCELERATOR"
  , "RCDATA"
  , "MESSAGETABLE"
  , "GROUP_CURSOR"
  , NULL
  , "GROUP_ICON"
  , NULL
  , "VERSION"
  , "DLGINCLUDE"
  , NULL
  , "PLUGPLAY"
  , "VXD"
  , "ANICURSOR"
  , "ANIICON"
  , "HTML"
  , "MANIFEST"
};

static const UInt32 kFlag = (UInt32)1 << 31;
static const UInt32 kMask = ~kFlag;

struct CTableItem
{
  UInt32 Offset;
  UInt32 ID;
};


static const UInt32 kBmpHeaderSize = 14;
static const UInt32 kIconHeaderSize = 22;

struct CResItem
{
  UInt32 Type;
  UInt32 ID;
  UInt32 Lang;

  UInt32 Size;
  UInt32 Offset;

  UInt32 HeaderSize;
  Byte Header[kIconHeaderSize]; // it must be enough for max size header.
  bool Enabled;

  bool IsNameEqual(const CResItem &item) const { return Lang == item.Lang; }
  UInt32 GetSize() const { return Size + HeaderSize; }
  bool IsBmp() const { return Type == 2; }
  bool IsIcon() const { return Type == 3; }
  bool IsString() const { return Type == 6; }
  bool IsRcData() const { return Type == 10; }
  bool IsVersion() const { return Type == 16; }
  bool IsRcDataOrUnknown() const { return IsRcData() || Type > 64; }
};

struct CTextFile
{
  CByteDynamicBuffer Buf;

  size_t FinalSize() const { return Buf.GetPos(); }

  void AddChar(char c);
  void AddWChar(UInt16 c);
  void AddWChar_Smart(UInt16 c);
  void NewLine();
  void AddString(const char *s);
  void AddSpaces(int num);
  void AddBytes(const Byte *p, size_t size)
  {
    Buf.AddData(p, size);
  }
  
  void OpenBlock(int num)
  {
    AddSpaces(num);
    AddChar('{');
    NewLine();
  }
  void CloseBlock(int num)
  {
    AddSpaces(num);
    AddChar('}');
    NewLine();
  }
};

void CTextFile::AddChar(char c)
{
  Byte *p = Buf.GetCurPtrAndGrow(2);
  p[0] = (Byte)c;
  p[1] = 0;
}

void CTextFile::AddWChar(UInt16 c)
{
  Byte *p = Buf.GetCurPtrAndGrow(2);
  SetUi16(p, c)
}

void CTextFile::AddWChar_Smart(UInt16 c)
{
  if (c == '\n')
  {
    AddChar('\\');
    c = 'n';
  }
  AddWChar(c);
}

void CTextFile::NewLine()
{
  AddChar(0x0D);
  AddChar(0x0A);
}

void CTextFile::AddString(const char *s)
{
  for (;; s++)
  {
    char c = *s;
    if (c == 0)
      return;
    AddChar(c);
  }
}

void CTextFile::AddSpaces(int num)
{
  for (int i = 0; i < num; i++)
    AddChar(' ');
}

struct CStringItem: public CTextFile
{
  UInt32 Lang;
};

struct CByteBuffer_WithLang: public CByteBuffer
{
  UInt32 Lang;
};


struct CMixItem
{
  int SectionIndex;
  int ResourceIndex;
  int StringIndex;
  int VersionIndex;

  CMixItem(): SectionIndex(-1), ResourceIndex(-1), StringIndex(-1), VersionIndex(-1) {}
  bool IsSectionItem() const { return ResourceIndex < 0 && StringIndex < 0 && VersionIndex < 0; }
};

struct CUsedBitmap
{
  CByteBuffer Buf;
public:
  void Alloc(size_t size)
  {
    size = (size + 7) / 8;
    Buf.Alloc(size);
    memset(Buf, 0, size);
  }
  
  void Free()
  {
    Buf.Free();
  }
  
  bool SetRange(size_t from, unsigned size)
  {
    for (unsigned i = 0; i < size; i++)
    {
      size_t pos = (from + i) >> 3;
      Byte mask = (Byte)(1 << ((from + i) & 7));
      Byte b = Buf[pos];
      if ((b & mask) != 0)
        return false;
      Buf[pos] = (Byte)(b | mask);
    }
    return true;
  }
};
 
struct CStringKeyValue
{
  UString Key;
  UString Value;
};


Z7_CLASS_IMP_CHandler_IInArchive_2(
  IInArchiveGetStream,
  IArchiveAllowTail
)
  CMyComPtr<IInStream> _stream;
  CObjectVector<CSection> _sections;
  CHeader _header;
  UInt32 _totalSize;
  Int32 _mainSubfile;

  CRecordVector<CMixItem> _mixItems;
  CRecordVector<CResItem> _items;
  CObjectVector<CStringItem> _strings;
  CObjectVector<CByteBuffer_WithLang> _versionFiles;
  UString _versionFullString;
  UString _versionShortString;
  UString _originalFilename;
  CObjectVector<CStringKeyValue> _versionKeys;

  CByteBuffer _buf;
  bool _oneLang;
  UString _resourcesPrefix;
  CUsedBitmap _usedRes;
  // bool _parseResources;
  bool _checksumError;
  bool _sectionsError;

  bool IsOpt() const { return _header.OptHeaderSize != 0; }

  COptHeader _optHeader;

  bool _coffMode;
  bool _allowTail;

  HRESULT LoadDebugSections(IInStream *stream, bool &thereIsSection);
  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *callback);

  void AddResNameToString(UString &s, UInt32 id) const;
  void AddLangPrefix(UString &s, UInt32 lang) const;
  HRESULT ReadString(UInt32 offset, UString &dest) const;
  HRESULT ReadTable(UInt32 offset, CRecordVector<CTableItem> &items);
  bool ParseStringRes(UInt32 id, UInt32 lang, const Byte *src, UInt32 size);
  HRESULT OpenResources(unsigned sectIndex, IInStream *stream, IArchiveOpenCallback *callback);
  void CloseResources();


  bool CheckItem(const CSection &sect, const CResItem &item, size_t offset) const
  {
    return item.Offset >= sect.Va && offset <= _buf.Size() && _buf.Size() - offset >= item.Size;
  }

public:
  CHandler(bool coffMode = false):
        _coffMode(coffMode),
        _allowTail(coffMode)
        {}
};


enum
{
  kpidSectAlign = kpidUserDefined,
  kpidFileAlign,
  kpidLinkerVer,
  kpidOsVer,
  kpidImageVer,
  kpidSubsysVer,
  kpidCodeSize,
  kpidImageSize,
  kpidInitDataSize,
  kpidUnInitDataSize,
  kpidHeadersSizeUnInitDataSize,
  kpidSubSystem,
  kpidDllCharacts,
  kpidStackReserve,
  kpidStackCommit,
  kpidHeapReserve,
  kpidHeapCommit
  // , kpidImageBase
  // , kpidAddressOfEntryPoint
  // , kpidBaseOfCode
  // , kpidBaseOfData32
};

static const CStatProp kArcProps[] =
{
  // { NULL, kpidWarning, VT_BSTR},
  { NULL, kpidCpu, VT_BSTR},
  { NULL, kpidBit64, VT_BOOL},
  { NULL, kpidCharacts, VT_BSTR},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidHeadersSize, VT_UI4},
  { NULL, kpidChecksum, VT_UI4},
  { NULL, kpidName, VT_BSTR},

  { "Image Size", kpidImageSize, VT_UI4},
  { "Section Alignment", kpidSectAlign, VT_UI4},
  { "File Alignment", kpidFileAlign, VT_UI4},
  { "Code Size", kpidCodeSize, VT_UI4},
  { "Initialized Data Size", kpidInitDataSize, VT_UI4},
  { "Uninitialized Data Size", kpidUnInitDataSize, VT_UI4},
  { "Linker Version", kpidLinkerVer, VT_BSTR},
  { "OS Version", kpidOsVer, VT_BSTR},
  { "Image Version", kpidImageVer, VT_BSTR},
  { "Subsystem Version", kpidSubsysVer, VT_BSTR},
  { "Subsystem", kpidSubSystem, VT_BSTR},
  { "DLL Characteristics", kpidDllCharacts, VT_BSTR},
  { "Stack Reserve", kpidStackReserve, VT_UI8},
  { "Stack Commit", kpidStackCommit, VT_UI8},
  { "Heap Reserve", kpidHeapReserve, VT_UI8},
  { "Heap Commit", kpidHeapCommit, VT_UI8},
  { NULL, kpidVa, VT_UI8 }, // "Image Base", kpidImageBase, VT_UI8
  { NULL, kpidComment, VT_BSTR}
  
  // , { "Address Of Entry Point", kpidAddressOfEntryPoint, VT_UI8}
  // , { "Base Of Code", kpidBaseOfCode, VT_UI8}
  // , { "Base Of Data", kpidBaseOfData32, VT_UI8}
};

// #define kpid_NumRelocs 250

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidPackSize,
  kpidVirtualSize,
  kpidCharacts,
  kpidOffset,
  kpidVa
  // , kpid_NumRelocs
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME

static void TimeToProp(UInt32 unixTime, NCOM::CPropVariant &prop)
{
  if (unixTime != 0)
    PropVariant_SetFrom_UnixTime(prop, unixTime);
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: prop = _totalSize; break;
    case kpidComment:
    {
      UString s (_versionFullString);
      s.Add_LF();
      s += "Data Directories: ";
      s.Add_UInt32(_optHeader.NumDirItems);
      s.Add_LF();
      s.Add_Char('{');
      s.Add_LF();
      for (unsigned i = 0; i < _optHeader.NumDirItems
          && i < Z7_ARRAY_SIZE(_optHeader.DirItems); i++)
      {
        const CDirLink &di = _optHeader.DirItems[i];
        if (di.Va == 0 && di.Size == 0)
          continue;
        s += "index=";
        s.Add_UInt32(i);

        if (i < Z7_ARRAY_SIZE(g_Dir_Names))
        {
          s += " name=";
          s += g_Dir_Names[i];
        }
        s += " VA=0x";
        char temp[16];
        ConvertUInt32ToHex(di.Va, temp);
        s += temp;
        s += " Size=";
        s.Add_UInt32(di.Size);
        s.Add_LF();
      }
      s.Add_Char('}');
      s.Add_LF();
      prop = s;
      break;
    }
    case kpidShortComment:
      if (!_versionShortString.IsEmpty())
        prop = _versionShortString;
      else
      {
        PAIR_TO_PROP(g_MachinePairs, _header.Machine, prop);
      }
      break;

    case kpidName: if (!_originalFilename.IsEmpty()) prop = _originalFilename; break;
      
    // case kpidIsSelfExe: prop = !_header.IsDll(); break;
    // case kpidError:
    case kpidWarning: if (_checksumError) prop = "Checksum error"; break;

    case kpidWarningFlags:
    {
      UInt32 v = 0;
      if (_sectionsError) v |= kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }

    case kpidCpu: PAIR_TO_PROP(g_MachinePairs, _header.Machine, prop); break;
    case kpidMTime:
    case kpidCTime: TimeToProp(_header.Time, prop); break;
    case kpidCharacts: FLAGS_TO_PROP(g_HeaderCharacts, _header.Flags, prop); break;
    case kpidMainSubfile: if (_mainSubfile >= 0) prop = (UInt32)_mainSubfile; break;
    
    default:
    if (IsOpt())
    switch (propID)
    {

    case kpidSectAlign: prop = _optHeader.SectAlign; break;
    case kpidFileAlign: prop = _optHeader.FileAlign; break;
    case kpidLinkerVer:
    {
      CVersion v = { _optHeader.LinkerVerMajor, _optHeader.LinkerVerMinor };
      v.ToProp(prop);
      break;
    }
  
    case kpidOsVer: _optHeader.OsVer.ToProp(prop); break;
    case kpidImageVer: _optHeader.ImageVer.ToProp(prop); break;
    case kpidSubsysVer: _optHeader.SubsysVer.ToProp(prop); break;
    case kpidCodeSize: prop = _optHeader.CodeSize; break;
    case kpidInitDataSize: prop = _optHeader.InitDataSize; break;
    case kpidUnInitDataSize: prop = _optHeader.UninitDataSize; break;
    case kpidImageSize: prop = _optHeader.ImageSize; break;
    case kpidHeadersSize: prop = _optHeader.HeadersSize; break;
    case kpidChecksum: prop = _optHeader.CheckSum; break;

    case kpidExtension:
      if (_header.IsDll())
        prop = "dll";
      else if (_optHeader.IsSybSystem_EFI())
        prop = "efi";
      break;

    case kpidBit64: if (_optHeader.Is64Bit()) prop = true; break;
    case kpidSubSystem: TYPE_TO_PROP(g_SubSystems, _optHeader.SubSystem, prop); break;

    case kpidDllCharacts: FLAGS_TO_PROP(g_DllCharacts, _optHeader.DllCharacts, prop); break;
    case kpidStackReserve: prop = _optHeader.StackReserve; break;
    case kpidStackCommit: prop = _optHeader.StackCommit; break;
    case kpidHeapReserve: prop = _optHeader.HeapReserve; break;
    case kpidHeapCommit: prop = _optHeader.HeapCommit; break;
    case kpidVa: prop = _optHeader.ImageBase; break; // kpidImageBase:
    // case kpidAddressOfEntryPoint: prop = _optHeader.AddressOfEntryPoint; break;
    // case kpidBaseOfCode: prop = _optHeader.BaseOfCode; break;
    // case kpidBaseOfData32: if (!_optHeader.Is64Bit()) prop = _optHeader.BaseOfData32; break;
  }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::ReadString(UInt32 offset, UString &dest) const
{
  if ((offset & 1) != 0 || offset >= _buf.Size())
    return S_FALSE;
  size_t rem = _buf.Size() - offset;
  if (rem < 2)
    return S_FALSE;
  unsigned len = Get16(_buf + offset);
  if ((rem - 2) / 2 < len)
    return S_FALSE;
  dest.Empty();
  wchar_t *destBuf = dest.GetBuf(len);
  offset += 2;
  const Byte *src = _buf + offset;
  unsigned i;
  for (i = 0; i < len; i++)
  {
    wchar_t c = (wchar_t)Get16(src + i * 2);
    if (c == 0)
      break;
    destBuf[i] = c;
  }
  destBuf[i] = 0;
  dest.ReleaseBuf_SetLen(i);
  return S_OK;
}

void CHandler::AddResNameToString(UString &s, UInt32 id) const
{
  if ((id & kFlag) != 0)
  {
    UString name;
    if (ReadString(id & kMask, name) == S_OK)
    {
      const wchar_t *str = L"[]";
      if (name.Len() > 1 && name[0] == '"' && name.Back() == '"')
      {
        if (name.Len() != 2)
        {
          name.DeleteBack();
          str = name.Ptr(1);
        }
      }
      else if (!name.IsEmpty())
        str = name;
      s += str;
      return;
    }
  }
  s.Add_UInt32(id);
}

void CHandler::AddLangPrefix(UString &s, UInt32 lang) const
{
  if (!_oneLang)
  {
    AddResNameToString(s, lang);
    s.Add_PathSepar();
  }
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  const CMixItem &mixItem = _mixItems[index];
  if (mixItem.StringIndex >= 0)
  {
    const CStringItem &item = _strings[mixItem.StringIndex];
    switch (propID)
    {
      case kpidPath:
      {
        UString s = _resourcesPrefix;
        AddLangPrefix(s, item.Lang);
        s += "string.txt";
        prop = s;
        break;
      }
      case kpidSize:
      case kpidPackSize:
        prop = (UInt64)item.FinalSize(); break;
    }
  }
  else if (mixItem.VersionIndex >= 0)
  {
    const CByteBuffer_WithLang &item = _versionFiles[mixItem.VersionIndex];
    switch (propID)
    {
      case kpidPath:
      {
        UString s = _resourcesPrefix;
        AddLangPrefix(s, item.Lang);
        s += "version.txt";
        prop = s;
        break;
      }
      case kpidSize:
      case kpidPackSize:
        prop = (UInt64)item.Size(); break;
    }
  }
  else if (mixItem.ResourceIndex >= 0)
  {
    const CResItem &item = _items[mixItem.ResourceIndex];
    switch (propID)
    {
      case kpidPath:
      {
        UString s = _resourcesPrefix;
        AddLangPrefix(s, item.Lang);
        {
          const char *p = NULL;
          if (item.Type < Z7_ARRAY_SIZE(g_ResTypes))
            p = g_ResTypes[item.Type];
          if (p)
            s += p;
          else
            AddResNameToString(s, item.Type);
        }
        s.Add_PathSepar();
        AddResNameToString(s, item.ID);
        if (item.HeaderSize != 0)
        {
          if (item.IsBmp())
            s += ".bmp";
          else if (item.IsIcon())
            s += ".ico";
        }
        prop = s;
        break;
      }
      case kpidSize: prop = (UInt64)item.GetSize(); break;
      case kpidPackSize: prop = (UInt64)item.Size; break;
    }
  }
  else
  {
    const CSection &item = _sections[mixItem.SectionIndex];
    switch (propID)
    {
      case kpidPath:
      {
        AString s = item.Name;
        s.Replace('/', '_');
        s.Replace('\\', '_');
        prop = MultiByteToUnicodeString(s);
        break;
      }
      case kpidSize: prop = (UInt64)item.GetSize_Extract(); break;
      // case kpid_NumRelocs: prop = (UInt32)item.NumRelocs; break;
      case kpidPackSize: prop = (UInt64)item.PSize; break;
      case kpidVirtualSize: prop = (UInt64)item.VSize; break;
      case kpidOffset: prop = item.Pa; break;
      case kpidVa: if (item.IsRealSect) prop = item.Va; break;
      case kpidMTime:
      case kpidCTime:
        TimeToProp(item.IsDebug ? item.Time : _header.Time, prop); break;
      case kpidCharacts:
       if (item.IsRealSect)
       {
         UInt32 flags = item.Flags;
         const UInt32 MY_IMAGE_SCN_ALIGN_MASK = 0x00F00000;
         AString s = FlagsToString(g_SectFlags, Z7_ARRAY_SIZE(g_SectFlags), item.Flags & ~MY_IMAGE_SCN_ALIGN_MASK);
         const UInt32 align = ((flags >> 20) & 0xF);
         if (align != 0)
         {
           char sz[32];
           ConvertUInt32ToString(1 << (align - 1), sz);
           s.Add_Space();
           s += "align_";
           s += sz;
         }
         prop = s;
       }
       break;
      case kpidZerosTailIsAllowed: if (!item.IsRealSect) prop = true; break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::LoadDebugSections(IInStream *stream, bool &thereIsSection)
{
  thereIsSection = false;
  const CDirLink &debugLink = _optHeader.DirItems[kDirLink_Debug];
  if (debugLink.Size == 0)
    return S_OK;
  const unsigned kEntrySize = 28;
  UInt32 numItems = debugLink.Size / kEntrySize;
  if (numItems > 16)
    return S_FALSE;

  // MAC's EFI file: numItems can be incorrect. Only first CDebugEntry entry is correct.
  // debugLink.Size = kEntrySize + some_data, pointed by entry[0].
  if (numItems * kEntrySize != debugLink.Size)
  {
    // return S_FALSE;
    if (numItems > 1)
      numItems = 1;
  }
  
  UInt64 pa = 0;
  unsigned i;
  for (i = 0; i < _sections.Size(); i++)
  {
    const CSection &sect = _sections[i];
    if (sect.Va <= debugLink.Va && debugLink.Va + debugLink.Size <= sect.Va + sect.PSize)
    {
      pa = sect.Pa + (debugLink.Va - sect.Va);
      break;
    }
  }
  if (i == _sections.Size())
  {
    // Exe for ARM requires S_OK
    // return S_FALSE;
    return S_OK;
  }
  
  CByteBuffer buffer(debugLink.Size);
  Byte *buf = buffer;
  
  RINOK(InStream_SeekSet(stream, pa))
  RINOK(ReadStream_FALSE(stream, buf, debugLink.Size))

  for (i = 0; i < numItems; i++)
  {
    CDebugEntry de;
    de.Parse(buf);

    if (de.Size == 0)
      continue;
    
    UInt32 totalSize = de.Pa + de.Size;
    if (totalSize > _totalSize)
    {
      _totalSize = totalSize;
      thereIsSection = true;

      CSection &sect = _sections.AddNew();
      sect.Name = ".debug";
      sect.Name.Add_UInt32(i);
      sect.IsDebug = true;
      sect.Time = de.Time;
      sect.Va = de.Va;
      sect.Pa = de.Pa;
      sect.Set_Size_for_all(de.Size);
    }
    buf += kEntrySize;
  }

  return S_OK;
}

HRESULT CHandler::ReadTable(UInt32 offset, CRecordVector<CTableItem> &items)
{
  if ((offset & 3) != 0 || offset >= _buf.Size())
    return S_FALSE;
  size_t rem = _buf.Size() - offset;
  if (rem < 16)
    return S_FALSE;
  unsigned numNameItems = Get16(_buf + offset + 12);
  unsigned numIdItems = Get16(_buf + offset + 14);
  unsigned numItems = numNameItems + numIdItems;
  if ((rem - 16) / 8 < numItems)
    return S_FALSE;
  if (!_usedRes.SetRange(offset, 16 + numItems * 8))
    return S_FALSE;
  offset += 16;
  items.ClearAndReserve(numItems);
  for (unsigned i = 0; i < numItems; i++, offset += 8)
  {
    const Byte *buf = _buf + offset;
    CTableItem item;
    item.ID = Get32(buf + 0);
    if ((bool)((item.ID & kFlag) != 0) != (bool)(i < numNameItems))
      return S_FALSE;
    item.Offset = Get32(buf + 4);
    items.AddInReserved(item);
  }
  return S_OK;
}

static const UInt32 kFileSizeMax = (UInt32)1 << 31;
static const unsigned kNumResItemsMax = (unsigned)1 << 23;
static const unsigned kNumStringLangsMax = 256;

// BITMAPINFOHEADER
struct CBitmapInfoHeader
{
  // UInt32 HeaderSize;
  UInt32 XSize;
  Int32 YSize;
  UInt16 Planes;
  UInt16 BitCount;
  UInt32 Compression;
  UInt32 SizeImage;

  bool Parse(const Byte *p, size_t size);
};

static const UInt32 kBitmapInfoHeader_Size = 0x28;

bool CBitmapInfoHeader::Parse(const Byte *p, size_t size)
{
  if (size < kBitmapInfoHeader_Size || Get32(p) != kBitmapInfoHeader_Size)
    return false;
  G32( 4, XSize);
  G32_signed( 8, YSize);
  G16(12, Planes);
  G16(14, BitCount);
  G32(16, Compression);
  G32(20, SizeImage);
  return true;
}

static UInt32 GetImageSize(UInt32 xSize, UInt32 ySize, UInt32 bitCount)
{
  return ((xSize * bitCount + 7) / 8 + 3) / 4 * 4 * ySize;
}
  
static UInt32 SetBitmapHeader(Byte *dest, const Byte *src, UInt32 size)
{
  CBitmapInfoHeader h;
  if (!h.Parse(src, size))
    return 0;
  if (h.YSize < 0)
    h.YSize = -h.YSize;
  if (h.XSize > (1 << 26)
      || h.YSize > (1 << 26)
      || h.YSize < 0
      || h.Planes != 1 || h.BitCount > 32)
    return 0;
  if (h.SizeImage == 0)
  {
    if (h.Compression != 0) // BI_RGB
      return 0;
    h.SizeImage = GetImageSize(h.XSize, (UInt32)h.YSize, h.BitCount);
  }
  UInt32 totalSize = kBmpHeaderSize + size;
  UInt32 offBits = totalSize - h.SizeImage;
  // BITMAPFILEHEADER
  SetUi16(dest, 0x4D42)
  SetUi32(dest + 2, totalSize)
  SetUi32(dest + 6, 0)
  SetUi32(dest + 10, offBits)
  return kBmpHeaderSize;
}

static UInt32 SetIconHeader(Byte *dest, const Byte *src, UInt32 size)
{
  CBitmapInfoHeader h;
  if (!h.Parse(src, size))
    return 0;
  if (h.YSize < 0)
    h.YSize = -h.YSize;
  if (h.XSize > (1 << 26)
      || h.YSize > (1 << 26)
      || h.YSize < 0
      || h.Planes != 1
      || h.Compression != 0) // BI_RGB
    return 0;

  const UInt32 numBitCount = h.BitCount;
  if (numBitCount != 1 &&
      numBitCount != 4 &&
      numBitCount != 8 &&
      numBitCount != 24 &&
      numBitCount != 32)
    return 0;

  if ((h.YSize & 1) != 0)
    return 0;
  h.YSize /= 2;
  if (h.XSize > 0x100 || h.YSize > 0x100)
    return 0;

  UInt32 imageSize;
  // imageSize is not correct if AND mask array contains zeros
  // in this case it is equal image1Size

  // UInt32 imageSize = h.SizeImage;
  // if (imageSize == 0)
  // {
    const UInt32 image1Size = GetImageSize(h.XSize, (UInt32)h.YSize, h.BitCount);
    const UInt32 image2Size = GetImageSize(h.XSize, (UInt32)h.YSize, 1);
    imageSize = image1Size + image2Size;
  // }
  UInt32 numColors = 0;
  if (numBitCount < 16)
    numColors = 1 << numBitCount;

  SetUi16(dest, 0) // Reserved
  SetUi16(dest + 2, 1) // RES_ICON
  SetUi16(dest + 4, 1) // ResCount

  dest[6] = (Byte)h.XSize; // Width
  dest[7] = (Byte)h.YSize; // Height
  dest[8] = (Byte)numColors; // ColorCount
  dest[9] = 0; // Reserved
  
  SetUi32(dest + 10, 0) // Reserved1 / Reserved2

  UInt32 numQuadsBytes = numColors * 4;
  UInt32 BytesInRes = kBitmapInfoHeader_Size + numQuadsBytes + imageSize;
  SetUi32(dest + 14, BytesInRes)
  SetUi32(dest + 18, kIconHeaderSize)

  /*
  Description = DWORDToString(xSize) +
      kDelimiterChar + DWORDToString(ySize) +
      kDelimiterChar + DWORDToString(numBitCount);
  */
  return kIconHeaderSize;
}

bool CHandler::ParseStringRes(UInt32 id, UInt32 lang, const Byte *src, UInt32 size)
{
  if ((size & 1) != 0)
    return false;

  unsigned i;
  for (i = 0; i < _strings.Size(); i++)
    if (_strings[i].Lang == lang)
      break;
  if (i == _strings.Size())
  {
    if (_strings.Size() >= kNumStringLangsMax)
      return false;
    CStringItem &item = _strings.AddNew();
    item.Lang = lang;
  }
  
  CStringItem &item = _strings[i];
  id = (id - 1) << 4;
  UInt32 pos = 0;
  for (i = 0; i < 16; i++)
  {
    if (size - pos < 2)
      return false;
    UInt32 len = Get16(src + pos);
    pos += 2;
    if (len != 0)
    {
      if (size - pos < len * 2)
        return false;
      char temp[32];
      ConvertUInt32ToString(id + i, temp);
      size_t tempLen = strlen(temp);
      size_t j;
      for (j = 0; j < tempLen; j++)
        item.AddChar(temp[j]);
      item.AddChar('\t');
      for (j = 0; j < len; j++, pos += 2)
        item.AddWChar_Smart(Get16(src + pos));
      item.NewLine();
    }
  }
  if (size == pos)
    return true;
  
  // Some rare case files have additional ZERO.
  if (size == pos + 2 && Get16(src + pos) == 0)
    return true;
  
  return false;
}


// ---------- VERSION ----------

static const UInt32 kMy_VS_FFI_SIGNATURE = 0xFEEF04BD;

struct CMy_VS_FIXEDFILEINFO
{
  // UInt32 Signature;
  // UInt32 StrucVersion;
  UInt32 VersionMS;
  UInt32 VersionLS;
  UInt32 ProductVersionMS;
  UInt32 ProductVersionLS;
  UInt32 FlagsMask;
  UInt32 Flags;
  UInt32 OS;
  UInt32 Type;
  UInt32 Subtype;
  UInt32 DateMS;
  UInt32 DateLS;

  bool Parse(const Byte *p);
  void PrintToTextFile(CTextFile &f, CObjectVector<CStringKeyValue> &keys);
};

bool CMy_VS_FIXEDFILEINFO::Parse(const Byte *p)
{
  if (Get32(p) != kMy_VS_FFI_SIGNATURE) // signature;
    return false;
  // G32(0x04, StrucVersion);
  G32(0x08, VersionMS);
  G32(0x0C, VersionLS);
  G32(0x10, ProductVersionMS);
  G32(0x14, ProductVersionLS);
  G32(0x18, FlagsMask);
  G32(0x1C, Flags);
  G32(0x20, OS);
  G32(0x24, Type);
  G32(0x28, Subtype);
  G32(0x2C, DateMS);
  G32(0x40, DateLS);
  return true;
}

static void PrintUInt32(CTextFile &f, UInt32 v)
{
  char s[16];
  ConvertUInt32ToString(v, s);
  f.AddString(s);
}

static inline void PrintUInt32(UString &dest, UInt32 v)
{
  dest.Add_UInt32(v);
}

static void PrintHex(CTextFile &f, UInt32 val)
{
  char temp[16];
  temp[0] = '0';
  temp[1] = 'x';
  ConvertUInt32ToHex(val, temp + 2);
  f.AddString(temp);
}

static void PrintVersion(CTextFile &f, UInt32 ms, UInt32 ls)
{
  PrintUInt32(f, HIWORD(ms));  f.AddChar(',');
  PrintUInt32(f, LOWORD(ms));  f.AddChar(',');
  PrintUInt32(f, HIWORD(ls));  f.AddChar(',');
  PrintUInt32(f, LOWORD(ls));
}

static void PrintVersion(UString &s, UInt32 ms, UInt32 ls)
{
  PrintUInt32(s, HIWORD(ms));  s.Add_Dot();
  PrintUInt32(s, LOWORD(ms));  s.Add_Dot();
  PrintUInt32(s, HIWORD(ls));  s.Add_Dot();
  PrintUInt32(s, LOWORD(ls));
}

static const char * const k_VS_FileFlags[] =
{
    "DEBUG"
  , "PRERELEASE"
  , "PATCHED"
  , "PRIVATEBUILD"
  , "INFOINFERRED"
  , "SPECIALBUILD"
};

static const CUInt32PCharPair k_VS_FileOS[] =
{
  {  0x10001, "VOS_DOS_WINDOWS16" },
  {  0x10004, "VOS_DOS_WINDOWS32" },
  {  0x20002, "VOS_OS216_PM16" },
  {  0x30003, "VOS_OS232_PM32" },
  {  0x40004, "VOS_NT_WINDOWS32" }
};

static const char * const k_VS_FileOS_High[] =
{
    "VOS_UNKNOWN"
  , "VOS_DOS"
  , "VOS_OS216"
  , "VOS_OS232"
  , "VOS_NT"
  , "VOS_WINCE"
};

static const UInt32 kMY_VFT_DRV  = 3;
static const UInt32 kMY_VFT_FONT = 4;

static const char * const k_VS_FileOS_Low[] =
{
    "VOS__BASE"
  , "VOS__WINDOWS16"
  , "VOS__PM16"
  , "VOS__PM32"
  , "VOS__WINDOWS32"
};

static const char * const k_VS_FileType[] =
{
    "VFT_UNKNOWN"
  , "VFT_APP"
  , "VFT_DLL"
  , "VFT_DRV"
  , "VFT_FONT"
  , "VFT_VXD"
  , "0x6"
  , "VFT_STATIC_LIB"
};

// Subtype for VFT_DRV Type
static const char * const k_VS_FileSubType_DRV[] =
{
    "0"
  , "PRINTER"
  , "KEYBOARD"
  , "LANGUAGE"
  , "DISPLAY"
  , "MOUSE"
  , "NETWORK"
  , "SYSTEM"
  , "INSTALLABLE"
  , "SOUND"
  , "COMM"
  , "INPUTMETHOD"
  , "VERSIONED_PRINTER"
};

// Subtype for VFT_FONT Type
static const char * const k_VS_FileSubType_FONT[] =
{
    "0"
  , "VFT2_FONT_RASTER"
  , "VFT2_FONT_VECTOR"
  , "VFT2_FONT_TRUETYPE"
};

static int FindKey(CObjectVector<CStringKeyValue> &v, const char *key)
{
  FOR_VECTOR (i, v)
    if (v[i].Key.IsEqualTo(key))
      return (int)i;
  return -1;
}

static void AddToUniqueUStringVector(CObjectVector<CStringKeyValue> &v, const UString &key, const UString &value)
{
  bool needInsert = false;
  unsigned i;
  for (i = 0; i < v.Size(); i++)
  {
    if (v[i].Key == key)
    {
      if (v[i].Value == value)
        return;
      needInsert = true;
    }
    else if (needInsert)
      break;
  }
  CStringKeyValue &pair = v.InsertNew(i);
  pair.Key = key;
  pair.Value = value;
}

void CMy_VS_FIXEDFILEINFO::PrintToTextFile(CTextFile &f, CObjectVector<CStringKeyValue> &keys)
{
  f.AddString("FILEVERSION    ");
  PrintVersion(f, VersionMS, VersionLS);
  f.NewLine();

  f.AddString("PRODUCTVERSION ");
  PrintVersion(f, ProductVersionMS, ProductVersionLS);
  f.NewLine();

  {
    UString s;
    PrintVersion(s, VersionMS, VersionLS);
    AddToUniqueUStringVector(keys, L"FileVersion", s);
  }
  {
    UString s;
    PrintVersion(s, ProductVersionMS, ProductVersionLS);
    AddToUniqueUStringVector(keys, L"ProductVersion", s);
  }
 
  f.AddString("FILEFLAGSMASK  ");
  PrintHex(f, FlagsMask);
  f.NewLine();

  f.AddString("FILEFLAGS      ");
  {
    bool wasPrinted = false;
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(k_VS_FileFlags); i++)
    {
      if ((Flags & ((UInt32)1 << i)) != 0)
      {
        if (wasPrinted)
          f.AddString(" | ");
        f.AddString("VS_FF_");
        f.AddString(k_VS_FileFlags[i]);
        wasPrinted = true;
      }
    }
    UInt32 v = Flags & ~(((UInt32)1 << Z7_ARRAY_SIZE(k_VS_FileFlags)) - 1);
    if (v != 0 || !wasPrinted)
    {
      if (wasPrinted)
        f.AddString(" | ");
      PrintHex(f, v);
    }
  }
  f.NewLine();

  // OS = 0x111230;
  f.AddString("FILEOS         ");
  unsigned i;
  for (i = 0; i < Z7_ARRAY_SIZE(k_VS_FileOS); i++)
  {
    const CUInt32PCharPair &pair = k_VS_FileOS[i];
    if (OS == pair.Value)
    {
      // continue;
      // f.AddString("VOS_");
      f.AddString(pair.Name);
      break;
    }
  }
  if (i == Z7_ARRAY_SIZE(k_VS_FileOS))
  {
    UInt32 high = OS >> 16;
    if (high < Z7_ARRAY_SIZE(k_VS_FileOS_High))
      f.AddString(k_VS_FileOS_High[high]);
    else
      PrintHex(f, high << 16);
    UInt32 low = OS & 0xFFFF;
    if (low != 0)
    {
      f.AddString(" | ");
      if (low < Z7_ARRAY_SIZE(k_VS_FileOS_Low))
        f.AddString(k_VS_FileOS_Low[low]);
      else
        PrintHex(f, low);
    }
  }
  f.NewLine();

  f.AddString("FILETYPE       ");
  if (Type < Z7_ARRAY_SIZE(k_VS_FileType))
    f.AddString(k_VS_FileType[Type]);
  else
    PrintHex(f, Type);
  f.NewLine();

  f.AddString("FILESUBTYPE    ");
  bool needPrintSubType = true;
  if (Type == kMY_VFT_DRV)
  {
    if (Subtype != 0 && Subtype < Z7_ARRAY_SIZE(k_VS_FileSubType_DRV))
    {
      f.AddString("VFT2_DRV_");
      f.AddString(k_VS_FileSubType_DRV[Subtype]);
      needPrintSubType = false;
    }
  }
  else if (Type == kMY_VFT_FONT)
  {
    if (Subtype != 0 && Subtype < Z7_ARRAY_SIZE(k_VS_FileSubType_FONT))
    {
      f.AddString(k_VS_FileSubType_FONT[Subtype]);
      needPrintSubType = false;
    }
  }
  if (needPrintSubType)
    PrintHex(f, Subtype);
  f.NewLine();
}

static void CopyToUString(const Byte *p, UString &s)
{
  for (;;)
  {
    const wchar_t c = (wchar_t)Get16(p);
    p += 2;
    if (c == 0)
      return;
    s += c;
  }
}

static void CopyToUString_ByLen16(const Byte *p, unsigned numChars16, UString &s)
{
  for (; numChars16; numChars16--)
  {
    const wchar_t c = (wchar_t)Get16(p);
    p += 2;
    s += c;
  }
}

static bool CompareWStrStrings(const Byte *p, const char *s)
{
  unsigned pos = 0;
  for (;;)
  {
    const Byte c = (Byte)*s++;
    if (Get16(p + pos) != c)
      return false;
    pos += 2;
    if (c == 0)
      return true;
  }
}

struct CVersionBlock
{
  UInt32 TotalLen;
  UInt32 ValueLen;
  unsigned IsTextValue;
  unsigned StrSize;

  bool Parse(const Byte *p, UInt32 size);
};

static int Get_Utf16Str_Len_InBytes(const Byte *p, size_t size)
{
  unsigned pos = 0;
  for (;;)
  {
    if (pos + 1 >= size)
      return -1;
    if (Get16(p + pos) == 0)
      return (int)pos;
    pos += 2;
  }
}

static int Get_Utf16Str_Len_InBytes_AllowNonZeroTail(const Byte *p, size_t size)
{
  unsigned pos = 0;
  for (;;)
  {
    if (pos + 1 >= size)
    {
      if (pos == size)
        return (int)pos;
      return -1;
    }
    if (Get16(p + pos) == 0)
      return (int)pos;
    pos += 2;
  }
}

static const unsigned k_ResoureBlockHeader_Size = 6;

bool CVersionBlock::Parse(const Byte *p, UInt32 size)
{
  if (size < k_ResoureBlockHeader_Size)
    return false;
  TotalLen = Get16(p);
  ValueLen = Get16(p + 2);
  if (TotalLen < k_ResoureBlockHeader_Size || TotalLen > size)
    return false;
  IsTextValue = Get16(p + 4);
  if (IsTextValue > 1)
    return false;
  StrSize = 0;
  const int t = Get_Utf16Str_Len_InBytes(p + k_ResoureBlockHeader_Size,
      TotalLen - k_ResoureBlockHeader_Size);
  if (t < 0)
    return false;
  StrSize = (unsigned)t;
  return true;
}

static void AddParamString(CTextFile &f, const Byte *p, size_t sLen)
{
  f.AddChar(' ');
  f.AddChar('\"');
  f.AddBytes(p, sLen);
  f.AddChar('\"');
}

static bool ParseVersion(const Byte *p, UInt32 size, CTextFile &f, CObjectVector<CStringKeyValue> &keys)
{
  UInt32 pos;
  {
    const unsigned k_sizeof_VS_FIXEDFILEINFO = 13 * 4;

    CVersionBlock vb;
    if (!vb.Parse(p, size))
      return false;
    if (vb.ValueLen != k_sizeof_VS_FIXEDFILEINFO) // maybe 0 is allowed here?
      return false;
    if (vb.IsTextValue)
      return false;
    pos = k_ResoureBlockHeader_Size;
    if (!CompareWStrStrings(p + pos, "VS_VERSION_INFO"))
      return false;
    pos += vb.StrSize + 2;
    pos += (4 - pos) & 3;
    if (pos + vb.ValueLen > vb.TotalLen)
      return false;
    /* sometimes resource contains zeros in remainder.
       So we don't check that size != vb.TotalLen
    // if (size != vb.TotalLen) return false;
    */
    if (size > vb.TotalLen)
        size = vb.TotalLen;
    CMy_VS_FIXEDFILEINFO FixedFileInfo;
    if (!FixedFileInfo.Parse(p + pos))
      return false;
    FixedFileInfo.PrintToTextFile(f, keys);
    pos += vb.ValueLen;
  }
  
  f.OpenBlock(0);
  
  for (;;)
  {
    pos += (4 - pos) & 3;
    if (pos >= size)
      break;
    
    CVersionBlock vb;
    if (!vb.Parse(p + pos, size - pos))
      return false;
    if (vb.ValueLen != 0)
      return false;
    const UInt32 endPos = pos + vb.TotalLen;
    pos += k_ResoureBlockHeader_Size;
    
    f.AddSpaces(2);
    f.AddString("BLOCK");
    AddParamString(f, p + pos, vb.StrSize);
    
    f.NewLine();
    f.OpenBlock(2);
    
    if (CompareWStrStrings(p + pos, "VarFileInfo"))
    {
      pos += vb.StrSize + 2;
      for (;;)
      {
        pos += (4 - pos) & 3;
        if (pos >= endPos)
          break;
        CVersionBlock vb2;
        if (!vb2.Parse(p + pos, endPos - pos))
          return false;
        const UInt32 endPos2 = pos + vb2.TotalLen;
        if (vb2.IsTextValue)
          return false;
        pos += k_ResoureBlockHeader_Size;
        f.AddSpaces(4);
        f.AddString("VALUE");
        AddParamString(f, p + pos, vb2.StrSize);
        if (!CompareWStrStrings(p + pos, "Translation"))
          return false;
        pos += vb2.StrSize + 2;
        pos += (4 - pos) & 3;
        if (pos + vb2.ValueLen != endPos2)
          return false;
        if ((vb2.ValueLen & 3) != 0)
          return false;
        UInt32 num = (vb2.ValueLen >> 2);
        for (; num != 0; num--, pos += 4)
        {
          const UInt32 dw = Get32(p + pos);
          const UInt32 lang = LOWORD(dw);
          const UInt32 codePage = HIWORD(dw);

          f.AddString(", ");
          PrintHex(f, lang);
          f.AddString(", ");
          PrintUInt32(f, codePage);
        }
        f.NewLine();
      }
    }
    else
    {
      if (!CompareWStrStrings(p + pos, "StringFileInfo"))
        return false;
      pos += vb.StrSize + 2;
      for (;;)
      {
        pos += (4 - pos) & 3;
        if (pos >= endPos)
          break;
        CVersionBlock vb2;
        if (!vb2.Parse(p + pos, endPos - pos))
          return false;
        const UInt32 endPos2 = pos + vb2.TotalLen;
        if (vb2.ValueLen != 0)
          return false;
        pos += k_ResoureBlockHeader_Size;

        f.AddSpaces(4);
        f.AddString("BLOCK");
        AddParamString(f, p + pos, vb2.StrSize);
        pos += vb2.StrSize + 2;

        f.NewLine();
        f.OpenBlock(4);

        for (;;)
        {
          pos += (4 - pos) & 3;
          if (pos >= endPos2)
            break;

          CVersionBlock vb3;
          if (!vb3.Parse(p + pos, endPos2 - pos))
            return false;
          // ValueLen is a number of 16-bit characters (usually it includes zero tail character).
          const UInt32 endPos3 = pos + vb3.TotalLen;
          pos += k_ResoureBlockHeader_Size;

          // we don't write string if it's not text
          if (vb3.IsTextValue)
          {
            f.AddSpaces(6);
            f.AddString("VALUE");
            AddParamString(f, p + pos, vb3.StrSize);
            UString key;
            UString value;
            CopyToUString(p + pos, key);
            pos += vb3.StrSize + 2;

            pos += (4 - pos) & 3;
            if (vb3.ValueLen != 0 && pos /* + 2 */ <= endPos3)
            {
              f.AddChar(',');
              f.AddSpaces((34 - (int)vb3.StrSize) / 2);
              // vb3.TotalLen for some PE files (not from msvc) doesn't include tail zero at the end of Value string.
              // we allow that minor error.
              const int sLen = Get_Utf16Str_Len_InBytes_AllowNonZeroTail(p + pos, endPos3 - pos);
              if (sLen < 0)
                return false;
              /*
              if (vb3.ValueLen - 1 != (unsigned)sLen / 2 &&
                  vb3.ValueLen     != (unsigned)sLen / 2)
                return false;
              */
              AddParamString(f, p + pos, (unsigned)sLen);
              CopyToUString_ByLen16(p + pos, (unsigned)sLen / 2, value);
              // pos += (unsigned)sLen + 2;
            }
            AddToUniqueUStringVector(keys, key, value);
          }
          pos = endPos3;
          f.NewLine();
        }
        pos = endPos2;
        f.CloseBlock(4);
      }
    }
    f.CloseBlock(2);
    pos = endPos;
  }

  f.CloseBlock(0);
  return true;
}


HRESULT CHandler::OpenResources(unsigned sectionIndex, IInStream *stream, IArchiveOpenCallback *callback)
{
  const CSection &sect = _sections[sectionIndex];
  size_t fileSize = sect.PSize;
  {
    size_t fileSizeMin = sect.PSize;
    
    if (sect.VSize < sect.PSize)
    {
      fileSize = fileSizeMin = sect.VSize;
      const int numBits = _optHeader.GetNumFileAlignBits();
      if (numBits > 0)
      {
        const UInt32 mask = ((UInt32)1 << numBits) - 1;
        const size_t end = (size_t)((sect.VSize + mask) & (UInt32)~mask);
        if (end > sect.VSize)
        {
          if (end <= sect.PSize)
            fileSize = end;
          else
            fileSize = sect.PSize;
        }
      }
    }

    if (fileSize > kFileSizeMax)
      return S_FALSE;

    {
      const UInt64 fileSize64 = fileSize;
      if (callback)
        RINOK(callback->SetTotal(NULL, &fileSize64))
    }
    
    RINOK(InStream_SeekSet(stream, sect.Pa))
    
    _buf.Alloc(fileSize);
    
    size_t pos;
    
    for (pos = 0; pos < fileSize;)
    {
      {
        const UInt64 offset64 = pos;
        if (callback)
          RINOK(callback->SetCompleted(NULL, &offset64))
      }
      size_t rem = MyMin(fileSize - pos, (size_t)(1 << 22));
      RINOK(ReadStream(stream, _buf + pos, &rem))
      if (rem == 0)
      {
        if (pos < fileSizeMin)
          return S_FALSE;
        break;
      }
      pos += rem;
    }
    
    if (pos < fileSize)
      memset(_buf + pos, 0, fileSize - pos);
  }
  
  _usedRes.Alloc(fileSize);
  CRecordVector<CTableItem> specItems;
  RINOK(ReadTable(0, specItems))

  _oneLang = true;
  bool stringsOk = true;
  size_t maxOffset = 0;
  
  FOR_VECTOR (i, specItems)
  {
    const CTableItem &item1 = specItems[i];
    if ((item1.Offset & kFlag) == 0)
      return S_FALSE;

    CRecordVector<CTableItem> specItems2;
    RINOK(ReadTable(item1.Offset & kMask, specItems2))

    FOR_VECTOR (j, specItems2)
    {
      const CTableItem &item2 = specItems2[j];
      if ((item2.Offset & kFlag) == 0)
        return S_FALSE;
      
      CRecordVector<CTableItem> specItems3;
      RINOK(ReadTable(item2.Offset & kMask, specItems3))
      
      CResItem item;
      item.Type = item1.ID;
      item.ID = item2.ID;
      
      FOR_VECTOR (k, specItems3)
      {
        if (_items.Size() >= kNumResItemsMax)
          return S_FALSE;
        const CTableItem &item3 = specItems3[k];
        if ((item3.Offset & kFlag) != 0)
          return S_FALSE;
        if (item3.Offset >= _buf.Size() || _buf.Size() - item3.Offset < 16)
          return S_FALSE;
        const Byte *buf = _buf + item3.Offset;
        item.Lang = item3.ID;
        item.Offset = Get32(buf + 0);
        item.Size = Get32(buf + 4);
        // UInt32 codePage = Get32(buf + 8);
        if (Get32(buf + 12) != 0)
          return S_FALSE;
        if (!_items.IsEmpty() && _oneLang && !item.IsNameEqual(_items.Back()))
          _oneLang = false;

        item.HeaderSize = 0;
      
        size_t offset = item.Offset - sect.Va;
        if (offset > maxOffset)
          maxOffset = offset;
        if (offset + item.Size > maxOffset)
          maxOffset = offset + item.Size;

        if (CheckItem(sect, item, offset))
        {
          const Byte *data = _buf + offset;
          if (item.IsBmp())
            item.HeaderSize = SetBitmapHeader(item.Header, data, item.Size);
          else if (item.IsIcon())
            item.HeaderSize = SetIconHeader(item.Header, data, item.Size);
          else if (item.IsString())
          {
            if (stringsOk)
              stringsOk = ParseStringRes(item.ID, item.Lang, data, item.Size);
          }
        }

        if (item.IsVersion())
        {
          if (offset > _buf.Size() || _buf.Size() - offset < item.Size)
            continue;
          CTextFile f;
          if (ParseVersion((const Byte *)_buf + offset, item.Size, f, _versionKeys))
          {
            CMixItem mixItem;
            mixItem.VersionIndex = (int)_versionFiles.Size();
            mixItem.SectionIndex = (int)sectionIndex; // check it !!!!
            CByteBuffer_WithLang &vf = _versionFiles.AddNew();
            vf.Lang = item.Lang;
            vf.CopyFrom(f.Buf, f.Buf.GetPos());
            _mixItems.Add(mixItem);
            continue;
          }
          // PrintError("ver.Parse error");
        }
  
        item.Enabled = true;
        _items.Add(item);
      }
    }
  }
  
  if (stringsOk && !_strings.IsEmpty())
  {
    unsigned i;
    for (i = 0; i < _items.Size(); i++)
    {
      CResItem &item = _items[i];
      if (item.IsString())
        item.Enabled = false;
    }
    for (i = 0; i < _strings.Size(); i++)
    {
      if (_strings[i].FinalSize() == 0)
        continue;
      CMixItem mixItem;
      mixItem.StringIndex = (int)i;
      mixItem.SectionIndex = (int)sectionIndex;
      _mixItems.Add(mixItem);
    }
  }

  _usedRes.Free();

  {
    // PSize can be much larger than VSize in some exe installers.
    // it contains archive data after PE resources.
    // So we need to use PSize here!
    if (maxOffset < sect.PSize)
    {
      size_t end = fileSize;

      // we skip Zeros to start of aligned block
      size_t i;
      for (i = maxOffset; i < end; i++)
        if (_buf[i] != 0)
          break;
      if (i == end)
        maxOffset = end;
      
      CSection sect2;
      sect2.Flags = 0;
      sect2.Pa = sect.Pa + (UInt32)maxOffset;
      sect2.Va = sect.Va + (UInt32)maxOffset;

      // 9.29: we use sect.PSize instead of sect.VSize to support some CAB-SFX
      // the code for .rsrc_2 is commented.
      sect2.PSize = sect.PSize - (UInt32)maxOffset;

      if (sect2.PSize != 0)
      {
        sect2.ExtractSize = sect2.VSize = sect2.PSize;
        sect2.Name = ".rsrc_1";
        sect2.Time = 0;
        sect2.IsAdditionalSection = true;
        _sections.Add(sect2);
      }
    }
  }

  return S_OK;
}


bool CHeader::ParseCoff(const Byte *p)
{
  ParseBase(p);
  if (PointerToSymbolTable < kCoffHeaderSize)
    return false;
  if (NumSymbols >= (1 << 24))
    return false;
  if (OptHeaderSize != 0 && OptHeaderSize < k_OptHeader32_Size_MIN)
    return false;

  // 18.04: we reduce false detections
  if (NumSections == 0 && OptHeaderSize == 0)
    return false;

  for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_MachinePairs); i++)
    if (Machine == g_MachinePairs[i].Value)
      return true;
  if (Machine == 0)
    return true;

  return false;
}


static inline bool CheckPeOffset(UInt32 pe)
{
  // ((pe & 7) == 0) is for most PE files. But there is unusual EFI-PE file that uses unaligned pe value.
  return pe >= 0x40 && pe <= 0x1000 /* && (pe & 7) == 0 */ ;
}

static const unsigned kStartSize = 0x40;

API_FUNC_static_IsArc IsArc_Pe(const Byte *p, size_t size)
{
  if (size < 2)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != 'M' || p[1] != 'Z')
    return k_IsArc_Res_NO;
  if (size < kStartSize)
    return k_IsArc_Res_NEED_MORE;
  UInt32 pe = Get32(p + 0x3C);
  if (!CheckPeOffset(pe))
    return k_IsArc_Res_NO;
  if (pe + kPeHeaderSize > size)
    return k_IsArc_Res_NEED_MORE;
  CHeader header;
  if (!header.ParsePe(p + pe))
    return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}

HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *callback)
{
  UInt32 coffOffset = 0;
  if (_coffMode)
  {
    Byte h[kCoffHeaderSize];
    RINOK(ReadStream_FALSE(stream, h, kCoffHeaderSize))
    if (!_header.ParseCoff(h))
      return S_FALSE;
  }
  else
  {
    UInt32 _peOffset;
    {
      Byte h[kStartSize];
      RINOK(ReadStream_FALSE(stream, h, kStartSize))
      if (h[0] != 'M' || h[1] != 'Z')
        return S_FALSE;
        /* most of PE files contain 0x0090 at offset 2.
        But some rare PE files contain another values. So we don't use that check.
      if (Get16(h + 2) != 0x90) return false; */
      _peOffset = Get32(h + 0x3C);
      if (!CheckPeOffset(_peOffset))
        return S_FALSE;
      coffOffset = _peOffset + 4;
    }
    {
      Byte h[kPeHeaderSize];
      RINOK(InStream_SeekSet(stream, _peOffset))
      RINOK(ReadStream_FALSE(stream, h, kPeHeaderSize))
      if (!_header.ParsePe(h))
        return S_FALSE;
    }
  }

  const UInt32 optStart = coffOffset + kCoffHeaderSize;
  const UInt32 bufSize = _header.OptHeaderSize + (UInt32)_header.NumSections * kSectionSize;
  _totalSize = optStart + bufSize;
  CByteBuffer buffer(bufSize);

  RINOK(ReadStream_FALSE(stream, buffer, bufSize))
  
  // memset((void *)&_optHeader, 0, sizeof(_optHeader));
  if (_header.OptHeaderSize != 0)
  if (!_optHeader.Parse(buffer, _header.OptHeaderSize))
    return S_FALSE;

  UInt32 pos = _header.OptHeaderSize;
  unsigned i;
  for (i = 0; i < _header.NumSections; i++, pos += kSectionSize)
  {
    CSection &sect = _sections.AddNew();
    sect.Parse(buffer + pos);
    sect.IsRealSect = true;
    if (sect.Name.IsEqualTo(".reloc"))
    {
      const CDirLink &dl = _optHeader.DirItems[kDirLink_BASERELOC];
      if (dl.Va == sect.Va &&
          dl.Size <= sect.PSize)
        sect.ExtractSize = dl.Size;
    }
    else if (sect.Name.IsEqualTo(".pdata"))
    {
      const CDirLink &dl = _optHeader.DirItems[kDirLink_EXCEPTION];
      if (dl.Va == sect.Va &&
          dl.Size <= sect.PSize)
        sect.ExtractSize = dl.Size;
    }
    
    /* PE pre-file in .hxs file has errors:
         PSize of resource is larger than real size.
         So it overlaps next ".its" section.
       7-zip before 22.02: we corrected it.

       22.02: another bad case is possible in incorrect pe (exe) file:
         PSize in .rsrc section is correct,
         but next .reloc section has incorrect (Pa) that overlaps with .rsrc.
    */

    if (i != 0)
    {
      const CSection &prev = _sections[i - 1];
      if (prev.Pa < sect.Pa
          && prev.Pa + prev.PSize > sect.Pa
          && sect.PSize != 0
          && prev.PSize != 0)
      {
        _sectionsError = true;
        // PRF(printf("\n !!!! Section correction: %s\n ", prev.Name));

        /* we corrected that case in 7-zip before 22.02: */
        // prev.PSize = sect.Pa - prev.Pa;

        /* 22.02: here we can try to change bad section position to expected postion.
           but original Windows code probably will not do same things. */
        // if (prev.PSize <= sect.Va - prev.Va) sect.Pa = prev.Pa + prev.PSize;
      }
    }
    /* last ".its" section in hxs file has incorrect sect.PSize.
       7-zip before 22.02: we reduced section to real sect.VSize */
    /*
    if (sect.VSize == 24 && sect.PSize == 512 && i == (unsigned)_header.NumSections - 1)
      sect.PSize = sect.VSize;
    */
  }

  for (i = 0; i < _sections.Size(); i++)
    _sections[i].UpdateTotalSize(_totalSize);

  bool thereISDebug = false;
  if (IsOpt())
  {
  RINOK(LoadDebugSections(stream, thereISDebug))

  const CDirLink &certLink = _optHeader.DirItems[kDirLink_Certificate];
  if (certLink.Size != 0)
  {
    CSection &sect = _sections.AddNew();
    sect.Name = "CERTIFICATE";
    sect.Va = 0;
    sect.Pa = certLink.Va;
    sect.Set_Size_for_all(certLink.Size);
    sect.UpdateTotalSize(_totalSize);
  }

  if (thereISDebug)
  {
    /* sometime there is some data after debug section.
       We don't see any reference in exe file to that data.
       But we suppose that it's part of EXE file */

    const UInt32 kAlign = 1 << 12;
    UInt32 alignPos = _totalSize & (kAlign - 1);
    if (alignPos != 0)
    {
      UInt32 size = kAlign - alignPos;
      RINOK(InStream_SeekSet(stream, _totalSize))
      buffer.Alloc(kAlign);
      Byte *buf = buffer;
      size_t processed = size;
      RINOK(ReadStream(stream, buf, &processed))

      /*
      if (processed != 0)
      {
        printf("\ndata after debug %d, %d \n", (int)size, (int)processed);
        fflush(stdout);
      }
      */

      size_t k;
      for (k = 0; k < processed; k++)
        if (buf[k] != 0)
          break;
      if (processed < size && processed < 100)
        _totalSize += (UInt32)processed;
      else if (((_totalSize + k) & 0x1FF) == 0 || processed < size)
        _totalSize += (UInt32)k;
    }
  }
  }

  if (_header.NumSymbols > 0 && _header.PointerToSymbolTable >= optStart)
  {
    if (_header.NumSymbols >= (1 << 24))
      return S_FALSE;
    UInt32 size = _header.NumSymbols * 18;
    RINOK(InStream_SeekSet(stream, (UInt64)_header.PointerToSymbolTable + size))
    Byte buf[4];
    RINOK(ReadStream_FALSE(stream, buf, 4))
    UInt32 size2 = Get32(buf);
    if (size2 >= (1 << 28))
      return S_FALSE;
    size += size2;

    CSection &sect = _sections.AddNew();
    sect.Name = "COFF_SYMBOLS";
    sect.Va = 0;
    sect.Pa = _header.PointerToSymbolTable;
    sect.Set_Size_for_all(size);
    sect.UpdateTotalSize(_totalSize);
  }

  {
    CObjectVector<CSection> sections = _sections;
    sections.Sort();
    UInt32 limit = (1 << 12);
    unsigned num = 0;
    FOR_VECTOR (k, sections)
    {
      const CSection &s = sections[k];
      if (s.Pa > limit)
      {
        CSection &s2 = _sections.AddNew();
        s2.Pa = s2.Va = limit;
        s2.Set_Size_for_all(s.Pa - limit);
        s2.IsAdditionalSection = true;
        s2.Name.Add_Char('[');
        s2.Name.Add_UInt32(num++);
        s2.Name.Add_Char(']');
        limit = s.Pa;
      }
      UInt32 next = s.Pa + s.PSize;
      if (next < s.Pa)
        break;
      if (next >= limit)
        limit = next;
    }
  }


  if (IsOpt())
  if (_optHeader.CheckSum != 0)
  {
    RINOK(InStream_SeekToBegin(stream))
    UInt32 checkSum = 0;
    RINOK(CalcCheckSum(stream, _totalSize, optStart + k_CheckSum_Field_Offset, checkSum))
    _checksumError = (checkSum != _optHeader.CheckSum);
  }


  if (!_allowTail)
  {
    UInt64 fileSize;
    RINOK(InStream_GetSize_SeekToEnd(stream, fileSize))
    if (fileSize > _totalSize)
      return S_FALSE;
  }

  bool _parseResources = true;
  // _parseResources = false; // for debug

  UInt64 mainSize = 0, mainSize2 = 0;

  for (i = 0; i < _sections.Size(); i++)
  {
    const CSection &sect = _sections[i];
    if (IsOpt())
    if (_parseResources && sect.Name.IsEqualTo(".rsrc"))
    {
      // 20.01: we try to parse only first copy of .rsrc section.
      _parseResources = false;
      const unsigned numMixItems = _mixItems.Size();
      HRESULT res = OpenResources(i, stream, callback);
      if (res == S_OK)
      {
        _resourcesPrefix = sect.Name.Ptr();
        _resourcesPrefix.Add_PathSepar();
        FOR_VECTOR (j, _items)
        {
          const CResItem &item = _items[j];
          if (item.Enabled)
          {
            CMixItem mixItem;
            mixItem.SectionIndex = (int)i;
            mixItem.ResourceIndex = (int)j;
            if (item.IsRcDataOrUnknown())
            {
              if (item.Size >= mainSize)
              {
                mainSize2 = mainSize;
                mainSize = item.Size;
                _mainSubfile = (Int32)(int)_mixItems.Size();
              }
              else if (item.Size >= mainSize2)
                mainSize2 = item.Size;
            }
            _mixItems.Add(mixItem);
          }
        }
        // 9.29: .rsrc_2 code was commented.
        // .rsrc_1 now must include that .rsrc_2 block.
        /*
        if (sect.PSize > sect.VSize)
        {
          int numBits = _optHeader.GetNumFileAlignBits();
          if (numBits >= 0)
          {
            UInt32 mask = (1 << numBits) - 1;
            UInt32 end = ((sect.VSize + mask) & ~mask);

            if (sect.PSize > end)
            {
              CSection &sect2 = _sections.AddNew();
              sect2.Flags = 0;
              sect2.Pa = sect.Pa + end;
              sect2.Va = sect.Va + end;
              sect2.PSize = sect.PSize - end;
              sect2.VSize = sect2.PSize;
              sect2.Name = ".rsrc_2";
              sect2.Time = 0;
              sect2.IsAdditionalSection = true;
            }
          }
        }
        */
        continue;
      }
      if (res != S_FALSE)
        return res;
      _mixItems.DeleteFrom(numMixItems);
      CloseResources();
    }

    if (sect.IsAdditionalSection)
    {
      if (sect.PSize >= mainSize)
      {
        mainSize2 = mainSize;
        mainSize = sect.PSize;
        _mainSubfile = (Int32)(int)_mixItems.Size();
      }
      else if (sect.PSize >= mainSize2)
        mainSize2 = sect.PSize;
    }
    
    CMixItem mixItem;
    mixItem.SectionIndex = (int)i;
    _mixItems.Add(mixItem);
  }
  
  if (mainSize2 >= (1 << 20) && mainSize < mainSize2 * 2)
    _mainSubfile = -1;

  for (i = 0; i < _mixItems.Size(); i++)
  {
    const CMixItem &mixItem = _mixItems[i];
    if (mixItem.StringIndex < 0 && mixItem.ResourceIndex < 0 && _sections[mixItem.SectionIndex].Name.IsEqualTo("_winzip_"))
    {
      _mainSubfile = (Int32)(int)i;
      break;
    }
  }

  for (i = 0; i < _versionKeys.Size(); i++)
  {
    if (i != 0)
      _versionFullString.Add_LF();
    const CStringKeyValue &k = _versionKeys[i];
    _versionFullString += k.Key;
    _versionFullString += ": ";
    _versionFullString += k.Value;
  }

  {
    int keyIndex = FindKey(_versionKeys, "OriginalFilename");
    if (keyIndex >= 0)
      _originalFilename = _versionKeys[keyIndex].Value;
  }
  {
    int keyIndex = FindKey(_versionKeys, "FileDescription");
    if (keyIndex >= 0)
      _versionShortString = _versionKeys[keyIndex].Value;
  }
  {
    int keyIndex = FindKey(_versionKeys, "FileVersion");
    if (keyIndex >= 0)
    {
      _versionShortString.Add_Space();
      _versionShortString += _versionKeys[keyIndex].Value;
    }
  }

  return S_OK;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  Close();
  RINOK(Open2(inStream, callback))
  _stream = inStream;
  return S_OK;
  COM_TRY_END
}

void CHandler::CloseResources()
{
  _usedRes.Free();
  _items.Clear();
  _strings.Clear();
  _versionFiles.Clear();
  _buf.Free();
  _versionFullString.Empty();
  _versionShortString.Empty();
  _originalFilename.Empty();
  _versionKeys.Clear();
}

Z7_COM7F_IMF(CHandler::Close())
{
  _totalSize = 0;
  _checksumError = false;
  _sectionsError = false;
  _mainSubfile = -1;

  _stream.Release();
  _sections.Clear();
  _mixItems.Clear();
  CloseResources();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _mixItems.Size();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = _mixItems.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const CMixItem &mixItem = _mixItems[allFilesMode ? i : indices[i]];
    UInt64 size;
    if (mixItem.StringIndex >= 0)
      size = _strings[mixItem.StringIndex].FinalSize();
    else if (mixItem.VersionIndex >= 0)
      size = _versionFiles[mixItem.VersionIndex].Size();
    else if (mixItem.ResourceIndex >= 0)
      size = _items[mixItem.ResourceIndex].GetSize();
    else
      size = _sections[mixItem.SectionIndex].GetSize_Extract();
    totalSize += size;
  }
  RINOK(extractCallback->SetTotal(totalSize))

  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> inStream;
  inStream->SetStream(_stream);

  totalSize = 0;
  UInt64 currentItemSize;
  
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

    CMyComPtr<ISequentialOutStream> outStream;
    RINOK(extractCallback->GetStream(index, &outStream, askMode))
    const CMixItem &mixItem = _mixItems[index];

    const CSection &sect = _sections[mixItem.SectionIndex];
    bool isOk = true;
    if (mixItem.StringIndex >= 0)
    {
      const CStringItem &item = _strings[mixItem.StringIndex];
      currentItemSize = item.FinalSize();
      if (!testMode && !outStream)
        continue;

      RINOK(extractCallback->PrepareOperation(askMode))
      if (outStream)
        RINOK(WriteStream(outStream, item.Buf, item.FinalSize()))
    }
    else if (mixItem.VersionIndex >= 0)
    {
      const CByteBuffer &item = _versionFiles[mixItem.VersionIndex];
      currentItemSize = item.Size();
      if (!testMode && !outStream)
        continue;

      RINOK(extractCallback->PrepareOperation(askMode))
      if (outStream)
        RINOK(WriteStream(outStream, item, item.Size()))
    }
    else if (mixItem.ResourceIndex >= 0)
    {
      const CResItem &item = _items[mixItem.ResourceIndex];
      currentItemSize = item.GetSize();
      if (!testMode && !outStream)
        continue;

      RINOK(extractCallback->PrepareOperation(askMode))
      size_t offset = item.Offset - sect.Va;
      if (!CheckItem(sect, item, offset))
        isOk = false;
      else if (outStream)
      {
        if (item.HeaderSize != 0)
          RINOK(WriteStream(outStream, item.Header, item.HeaderSize))
        RINOK(WriteStream(outStream, _buf + offset, item.Size))
      }
    }
    else
    {
      currentItemSize = sect.GetSize_Extract();
      if (!testMode && !outStream)
        continue;
      
      RINOK(extractCallback->PrepareOperation(askMode))
      RINOK(InStream_SeekSet(_stream, sect.Pa))
      inStream->Init(currentItemSize);
      RINOK(copyCoder.Interface()->Code(inStream, outStream, NULL, NULL, lps))
      isOk = (copyCoder->TotalSize == currentItemSize);
    }
    
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(isOk ?
        NExtract::NOperationResult::kOK :
        NExtract::NOperationResult::kDataError))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;

  const CMixItem &mixItem = _mixItems[index];
  const CSection &sect = _sections[mixItem.SectionIndex];
  if (mixItem.IsSectionItem())
    return CreateLimitedInStream(_stream, sect.Pa, sect.GetSize_Extract(), stream);

  CBufInStream *inStreamSpec = new CBufInStream;
  CMyComPtr<ISequentialInStream> streamTemp = inStreamSpec;
  CReferenceBuf *referenceBuf = new CReferenceBuf;
  CMyComPtr<IUnknown> ref = referenceBuf;
  if (mixItem.StringIndex >= 0)
  {
    const CStringItem &item = _strings[mixItem.StringIndex];
    referenceBuf->Buf.CopyFrom(item.Buf, item.FinalSize());
  }
  else if (mixItem.VersionIndex >= 0)
  {
    const CByteBuffer &item = _versionFiles[mixItem.VersionIndex];
    referenceBuf->Buf.CopyFrom(item, item.Size());
  }
  else
  {
    const CResItem &item = _items[mixItem.ResourceIndex];
    size_t offset = item.Offset - sect.Va;
    if (!CheckItem(sect, item, offset))
      return S_FALSE;
    if (item.HeaderSize == 0)
    {
      CBufInStream *streamSpec = new CBufInStream;
      CMyComPtr<IInStream> streamTemp2 = streamSpec;
      streamSpec->Init(_buf + offset, item.Size, (IInArchive *)this);
      *stream = streamTemp2.Detach();
      return S_OK;
    }
    referenceBuf->Buf.Alloc(item.HeaderSize + item.Size);
    memcpy(referenceBuf->Buf, item.Header, item.HeaderSize);
    if (item.Size != 0)
      memcpy(referenceBuf->Buf + item.HeaderSize, _buf + offset, item.Size);
  }
  inStreamSpec->Init(referenceBuf);

  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::AllowTail(Int32 allowTail))
{
  _allowTail = IntToBool(allowTail);
  return S_OK;
}

static const Byte k_Signature[] = { 'M', 'Z' };

REGISTER_ARC_I(
  "PE", "exe dll sys", NULL, 0xDD,
  k_Signature,
  0,
  NArcInfoFlags::kPreArc,
  IsArc_Pe)

}

namespace NCoff {

API_FUNC_static_IsArc IsArc_Coff(const Byte *p, size_t size)
{
  if (size < NPe::kCoffHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  NPe::CHeader header;
  if (!header.ParseCoff(p))
    return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}

/*
static const Byte k_Signature[] =
{
    2, 0x4C, 0x01, // x86
    2, 0x64, 0x86, // x64
    2, 0x64, 0xAA  // ARM64
};
REGISTER_ARC_I_CLS(
*/

REGISTER_ARC_I_CLS_NO_SIG(
  NPe::CHandler(true),
  "COFF", "obj", NULL, 0xC6,
  // k_Signature,
  0,
  // NArcInfoFlags::kMultiSignature |
  NArcInfoFlags::kStartOpen,
  IsArc_Coff)
}


namespace NTe {

// Terse Executable (TE) image

struct CDataDir
{
  UInt32 Va;
  UInt32 Size;

  void Parse(const Byte *p)
  {
    G32(0, Va);
    G32(4, Size);
  }
};

static const UInt32 kHeaderSize = 40;

static bool FindValue(const CUInt32PCharPair *pairs, unsigned num, UInt32 value)
{
  for (unsigned i = 0; i < num; i++)
    if (pairs[i].Value == value)
      return true;
  return false;
}

#define MY_FIND_VALUE(pairs, val) FindValue(pairs, Z7_ARRAY_SIZE(pairs), val)
#define MY_FIND_VALUE_2(strings, val) (val < Z7_ARRAY_SIZE(strings) && strings[val])
 
static const UInt32 kNumSection_MAX = 32;

struct CHeader
{
  UInt16 Machine;
  Byte NumSections;
  Byte SubSystem;
  UInt16 StrippedSize;
  /*
  UInt32 AddressOfEntryPoint;
  UInt32 BaseOfCode;
  UInt64 ImageBase;
  */
  CDataDir DataDir[2]; // base relocation and debug directory

  bool ConvertPa(UInt32 &pa) const
  {
    if (pa < StrippedSize)
      return false;
    pa = pa - StrippedSize + kHeaderSize;
    return true;
  }
  bool Parse(const Byte *p);
};

bool CHeader::Parse(const Byte *p)
{
  NumSections = p[4];
  if (NumSections > kNumSection_MAX)
    return false;
  SubSystem = p[5];
  G16(2, Machine);
  G16(6, StrippedSize);
  /*
  G32(8, AddressOfEntryPoint);
  G32(12, BaseOfCode);
  G64(16, ImageBase);
  */
  for (unsigned i = 0; i < 2; i++)
  {
    CDataDir &dd = DataDir[i];
    dd.Parse(p + 24 + i * 8);
    if (dd.Size >= ((UInt32)1 << 28))
      return false;
  }
  return
      MY_FIND_VALUE(NPe::g_MachinePairs, Machine) &&
      MY_FIND_VALUE_2(NPe::g_SubSystems, SubSystem);
}

API_FUNC_static_IsArc IsArc_Te(const Byte *p, size_t size)
{
  if (size < 2)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != 'V' || p[1] != 'Z')
    return k_IsArc_Res_NO;
  if (size < kHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  
  CHeader h;
  if (!h.Parse(p))
    return k_IsArc_Res_NO;
  return k_IsArc_Res_YES;
}
}


struct CSection
{
  Byte Name[NPe::kNameSize];

  UInt32 ExtractSize;
  UInt32 VSize;
  UInt32 Va;
  UInt32 PSize;
  UInt32 Pa;
  UInt32 Flags;
  // UInt16 NumRelocs;

  void Parse(const Byte *p)
  {
    memcpy(Name, p, NPe::kNameSize);
    G32(8, VSize);
    G32(12, Va);
    G32(16, PSize);
    G32(20, Pa);
    // G32(p + 32, NumRelocs);
    G32(36, Flags);
    ExtractSize = (VSize && VSize < PSize) ? VSize : PSize;
  }

  bool Check() const
  {
    return
        Pa <= ((UInt32)1 << 30) &&
        PSize <= ((UInt32)1 << 30);
  }

  UInt32 GetSize_Extract() const
  {
    return ExtractSize;
  }

  void UpdateTotalSize(UInt32 &totalSize)
  {
    const UInt32 t = Pa + PSize;
    if (totalSize < t)
        totalSize = t;
  }
};


Z7_CLASS_IMP_CHandler_IInArchive_2(
  IInArchiveGetStream,
  IArchiveAllowTail
)
  CRecordVector<CSection> _items;
  CMyComPtr<IInStream> _stream;
  UInt32 _totalSize;
  bool _allowTail;
  CHeader _h;
  
  HRESULT Open2(IInStream *stream);
public:
  CHandler(): _allowTail(false) {}
};

static const Byte kProps[] =
{
  kpidPath,
  kpidSize,
  kpidPackSize,
  kpidVirtualSize,
  kpidCharacts,
  kpidOffset,
  kpidVa
};

enum
{
  kpidSubSystem = kpidUserDefined
  // , kpidImageBase
};

static const CStatProp kArcProps[] =
{
  // { NULL, kpidHeadersSize, VT_UI4 },
  { NULL, kpidCpu, VT_BSTR},
  { "Subsystem", kpidSubSystem, VT_BSTR },
  // { "Image Base", kpidImageBase, VT_UI8 }
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_WITH_NAME

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: prop = _totalSize; break;
    case kpidCpu: PAIR_TO_PROP(NPe::g_MachinePairs, _h.Machine, prop); break;
    case kpidSubSystem: TYPE_TO_PROP(NPe::g_SubSystems, _h.SubSystem, prop); break;
    /*
    case kpidImageBase: prop = _h.ImageBase; break;
    case kpidAddressOfEntryPoint: prop = _h.AddressOfEntryPoint; break;
    case kpidBaseOfCode: prop = _h.BaseOfCode; break;
    */
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  {
    const CSection &item = _items[index];
    switch (propID)
    {
      case kpidPath:
      {
        AString name;
        NPe::GetName(item.Name, name);
        prop = MultiByteToUnicodeString(name);
        break;
      }
      case kpidSize: prop = (UInt64)item.GetSize_Extract(); break;
      case kpidPackSize: prop = (UInt64)item.PSize; break;
      case kpidVirtualSize: prop = (UInt64)item.VSize; break;
      case kpidOffset: prop = item.Pa; break;
      case kpidVa: prop = item.Va; break;
      case kpidCharacts: FLAGS_TO_PROP(NPe::g_SectFlags, item.Flags, prop); break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::Open2(IInStream *stream)
{
  Byte h[kHeaderSize];
  RINOK(ReadStream_FALSE(stream, h, kHeaderSize))
  if (h[0] != 'V' || h[1] != 'Z')
    return S_FALSE;
  if (!_h.Parse(h))
    return S_FALSE;

  UInt32 headerSize = NPe::kSectionSize * (UInt32)_h.NumSections;
  CByteArr buf(headerSize);
  RINOK(ReadStream_FALSE(stream, buf, headerSize))
  headerSize += kHeaderSize;

  _totalSize = headerSize;
  _items.ClearAndReserve(_h.NumSections);
  for (UInt32 i = 0; i < _h.NumSections; i++)
  {
    CSection sect;
    sect.Parse(buf + i * NPe::kSectionSize);
    if (!_h.ConvertPa(sect.Pa))
      return S_FALSE;
    if (sect.Pa < headerSize)
      return S_FALSE;
    if (!sect.Check())
      return S_FALSE;
    _items.AddInReserved(sect);
    sect.UpdateTotalSize(_totalSize);
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
  // try
  {
    if (Open2(inStream) != S_OK)
      return S_FALSE;
    _stream = inStream;
  }
  // catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _totalSize = 0;
  _stream.Release();
  _items.Clear();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = _items.Size();
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
    totalSize += _items[allFilesMode ? i : indices[i]].GetSize_Extract();
  RINOK(extractCallback->SetTotal(totalSize))

  CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> inStream;
  inStream->SetStream(_stream);

  totalSize = 0;

  for (i = 0;; i++)
  {
    lps->InSize = lps->OutSize = totalSize;
    RINOK(lps->SetCur())
    if (i >= numItems)
      break;
    int opRes;
    {
    CMyComPtr<ISequentialOutStream> realOutStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    const UInt32 index = allFilesMode ? i : indices[i];
    const CSection &item = _items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode))
    const UInt32 size = item.GetSize_Extract();
    totalSize += size;
    
    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode))
    RINOK(InStream_SeekSet(_stream, item.Pa))
    inStream->Init(size);
    RINOK(copyCoder.Interface()->Code(inStream, realOutStream, NULL, NULL, lps))

      opRes = (copyCoder->TotalSize == size) ?
          NExtract::NOperationResult::kOK : (copyCoder->TotalSize < size) ?
          NExtract::NOperationResult::kUnexpectedEnd :
          NExtract::NOperationResult::kDataError;
    }
    RINOK(extractCallback->SetOperationResult(opRes))
  }
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  const CSection &item = _items[index];
  return CreateLimitedInStream(_stream, item.Pa, item.GetSize_Extract(), stream);
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::AllowTail(Int32 allowTail))
{
  _allowTail = IntToBool(allowTail);
  return S_OK;
}

static const Byte k_Signature[] = { 'V', 'Z' };

REGISTER_ARC_I(
  "TE", "te", NULL, 0xCF,
  k_Signature,
  0,
  NArcInfoFlags::kPreArc,
  IsArc_Te)

}
}

/* ================ unit: CPP/7zip/Archive/PpmdHandler.cpp ================ */
/* PpmdHandler.cpp -- PPMd format handler
2020 : Igor Pavlov : Public domain
This code is based on:
  PPMd var.H (2001) / var.I (2002): Dmitry Shkarin : Public domain
  Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

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
namespace NPpmd {

static const UInt32 kBufSize = (1 << 20);

struct CBuf
{
  Byte *Buf;
  
  CBuf(): Buf(NULL) {}
  ~CBuf() { ::MidFree(Buf); }
  bool Alloc()
  {
    if (!Buf)
      Buf = (Byte *)::MidAlloc(kBufSize);
    return (Buf != NULL);
  }
};

static const UInt32 kHeaderSize = 16;
static const UInt32 kSignature = 0x84ACAF8F;
static const unsigned kNewHeaderVer = 8;

struct CItem
{
  UInt32 Attrib;
  UInt32 Time;
  AString Name;
  
  unsigned Order;
  unsigned MemInMB;
  unsigned Ver;
  unsigned Restor;

  HRESULT ReadHeader(ISequentialInStream *s, UInt32 &headerSize);
  bool IsSupported() const
  {
    return (Ver == 7 && Order >= PPMD7_MIN_ORDER)
        || (Ver == 8 && Order >= PPMD8_MIN_ORDER && Restor < PPMD8_RESTORE_METHOD_UNSUPPPORTED);
  }
};

HRESULT CItem::ReadHeader(ISequentialInStream *s, UInt32 &headerSize)
{
  Byte h[kHeaderSize];
  RINOK(ReadStream_FALSE(s, h, kHeaderSize))
  if (GetUi32(h) != kSignature)
    return S_FALSE;
  Attrib = GetUi32(h + 4);
  Time = GetUi32(h + 12);
  const unsigned info = GetUi16(h + 8);
  Order = (info & 0xF) + 1;
  MemInMB = ((info >> 4) & 0xFF) + 1;
  Ver = info >> 12;

  if (Ver < 6 || Ver > 11) return S_FALSE;
 
  UInt32 nameLen = GetUi16(h + 10);
  Restor = nameLen >> 14;
  if (Restor > 2)
    return S_FALSE;
  if (Ver >= kNewHeaderVer)
    nameLen &= 0x3FFF;
  if (nameLen > (1 << 9))
    return S_FALSE;
  char *name = Name.GetBuf(nameLen);
  const HRESULT res = ReadStream_FALSE(s, name, nameLen);
  Name.ReleaseBuf_CalcLen(nameLen);
  headerSize = kHeaderSize + nameLen;
  return res;
}


Z7_CLASS_IMP_CHandler_IInArchive_1(
  IArchiveOpenSeq
)
  CItem _item;
  UInt32 _headerSize;
  bool _packSize_Defined;
  UInt64 _packSize;
  CMyComPtr<ISequentialInStream> _stream;

  void GetVersion(NCOM::CPropVariant &prop);
};

static const Byte kProps[] =
{
  kpidPath,
  kpidMTime,
  kpidAttrib,
  kpidMethod
};

static const Byte kArcProps[] =
{
  kpidMethod
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

void CHandler::GetVersion(NCOM::CPropVariant &prop)
{
  AString s ("PPMd");
  s.Add_Char((char)('A' + _item.Ver));
  s += ":o";
  s.Add_UInt32(_item.Order);
  s += ":mem";
  s.Add_UInt32(_item.MemInMB);
  s.Add_Char('m');
  if (_item.Ver >= kNewHeaderVer && _item.Restor != 0)
  {
    s += ":r";
    s.Add_UInt32(_item.Restor);
  }
  prop = s;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: if (_packSize_Defined) prop = _packSize; break;
    case kpidMethod: GetVersion(prop); break;
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
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPath: prop = MultiByteToUnicodeString(_item.Name, CP_ACP); break;
    case kpidMTime:
    {
      // time can be in Unix format ???
      FILETIME utc;
      if (NTime::DosTime_To_FileTime(_item.Time, utc))
        prop = utc;
      break;
    }
    case kpidAttrib: prop = _item.Attrib; break;
    case kpidPackSize: if (_packSize_Defined) prop = _packSize; break;
    case kpidMethod: GetVersion(prop); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *))
{
  return OpenSeq(stream);
}

Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  COM_TRY_BEGIN
  HRESULT res;
  try
  {
    Close();
    res = _item.ReadHeader(stream, _headerSize);
  }
  catch(...) { res = S_FALSE; }
  if (res == S_OK)
    _stream = stream;
  else
    Close();
  return res;
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  _packSize = 0;
  _packSize_Defined = false;
  _stream.Release();
  return S_OK;
}



struct CPpmdCpp
{
  unsigned Ver;
  CPpmd7 _ppmd7;
  CPpmd8 _ppmd8;
  
  CPpmdCpp(unsigned version)
  {
    Ver = version;
    Ppmd7_Construct(&_ppmd7);
    Ppmd8_Construct(&_ppmd8);
  }

  ~CPpmdCpp()
  {
    Ppmd7_Free(&_ppmd7, &g_BigAlloc);
    Ppmd8_Free(&_ppmd8, &g_BigAlloc);
  }

  bool Alloc(UInt32 memInMB)
  {
    memInMB <<= 20;
    if (Ver == 7)
      return Ppmd7_Alloc(&_ppmd7, memInMB, &g_BigAlloc) != 0;
    return Ppmd8_Alloc(&_ppmd8, memInMB, &g_BigAlloc) != 0;
  }

  void Init(unsigned order, unsigned restor)
  {
    if (Ver == 7)
      Ppmd7_Init(&_ppmd7, order);
    else
      Ppmd8_Init(&_ppmd8, order, restor);
  }
    
  bool InitRc(CByteInBufWrap *inStream)
  {
    if (Ver == 7)
    {
      _ppmd7.rc.dec.Stream = &inStream->vt;
      return (Ppmd7a_RangeDec_Init(&_ppmd7.rc.dec) != 0);
    }
    else
    {
      _ppmd8.Stream.In = &inStream->vt;
      return Ppmd8_Init_RangeDec(&_ppmd8) != 0;
    }
  }

  bool IsFinishedOK()
  {
    if (Ver == 7)
      return Ppmd7z_RangeDec_IsFinishedOK(&_ppmd7.rc.dec);
    return Ppmd8_RangeDec_IsFinishedOK(&_ppmd8);
  }
};


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  // extractCallback->SetTotal(_packSize);
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

  CByteInBufWrap inBuf;
  if (!inBuf.Alloc(1 << 20))
    return E_OUTOFMEMORY;
  inBuf.Stream = _stream;

  CBuf outBuf;
  if (!outBuf.Alloc())
    return E_OUTOFMEMORY;

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, true);

  CPpmdCpp ppmd(_item.Ver);
  if (!ppmd.Alloc(_item.MemInMB))
    return E_OUTOFMEMORY;
  
  opRes = NExtract::NOperationResult::kUnsupportedMethod;

  if (_item.IsSupported())
  {
    opRes = NExtract::NOperationResult::kDataError;
    
    ppmd.Init(_item.Order, _item.Restor);
    inBuf.Init();
    UInt64 outSize = 0;
    
    if (ppmd.InitRc(&inBuf) && !inBuf.Extra && inBuf.Res == S_OK)
    for (;;)
    {
      lps->InSize = _packSize = inBuf.GetProcessed();
      lps->OutSize = outSize;
      RINOK(lps->SetCur())

      size_t i;
      int sym = 0;

      Byte *buf = outBuf.Buf;
      if (ppmd.Ver == 7)
      {
        for (i = 0; i < kBufSize; i++)
        {
          sym = Ppmd7a_DecodeSymbol(&ppmd._ppmd7);
          if (inBuf.Extra || sym < 0)
            break;
          buf[i] = (Byte)sym;
        }
      }
      else
      {
        for (i = 0; i < kBufSize; i++)
        {
          sym = Ppmd8_DecodeSymbol(&ppmd._ppmd8);
          if (inBuf.Extra || sym < 0)
            break;
          buf[i] = (Byte)sym;
        }
      }

      outSize += i;
      _packSize = _headerSize + inBuf.GetProcessed();
      _packSize_Defined = true;
      if (realOutStream)
      {
        RINOK(WriteStream(realOutStream, outBuf.Buf, i))
      }

      if (inBuf.Extra)
      {
        opRes = NExtract::NOperationResult::kUnexpectedEnd;
        break;
      }

      if (sym < 0)
      {
        if (sym == -1 && ppmd.IsFinishedOK())
          opRes = NExtract::NOperationResult::kOK;
        break;
      }
    }
    
    RINOK(inBuf.Res)
  }
}
  return extractCallback->SetOperationResult(opRes);
}


static const Byte k_Signature[] = { 0x8F, 0xAF, 0xAC, 0x84 };

REGISTER_ARC_I(
  "Ppmd", "pmd", NULL, 0xD,
  k_Signature,
  0,
  0,
  NULL)

}}

/* ================ unit: CPP/7zip/Archive/QcowHandler.cpp ================ */
// QcowHandler.cpp

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

#define Get32(p) GetBe32a(p)
#define Get64(p) GetBe64a(p)

using namespace NWindows;

namespace NArchive {
namespace NQcow {

static const Byte k_Signature[] =  { 'Q', 'F', 'I', 0xFB, 0, 0, 0 };

/*
VA to PA maps:
  high bits (L1) :              : index in L1 (_dir) : _dir[high_index] points to Table.
  mid bits  (L2) : _numMidBits  : index in Table, Table[index] points to cluster start offset in arc file.
  low bits       : _clusterBits : offset inside cluster.
*/

Z7_class_CHandler_final: public CHandlerImg
{
  Z7_IFACE_COM7_IMP(IInArchive_Img)
  Z7_IFACE_COM7_IMP(IInArchiveGetStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)

  unsigned _clusterBits;
  unsigned _numMidBits;
  UInt64 _compressedFlag;

  CObjArray2<UInt32> _dir;
  CAlignedBuffer _table;
  CByteBuffer _cache;
  CByteBuffer _cacheCompressed;
  UInt64 _cacheCluster;

  UInt64 _comprPos;
  size_t _comprSize;

  bool _needCompression;
  bool _isArc;
  bool _unsupported;
  Byte _compressionType;

  UInt64 _phySize;

  CMyComPtr2<ISequentialInStream, CBufInStream> _bufInStream;
  CMyComPtr2<ISequentialOutStream, CBufPtrSeqOutStream> _bufOutStream;
  CMyComPtr2<ICompressCoder, NCompress::NDeflate::NDecoder::CCOMCoder> _deflateDecoder;

  UInt32 _version;
  UInt32 _cryptMethod;
  UInt64 _incompatFlags;
  
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
};


static const UInt32 kEmptyDirItem = (UInt32)0 - 1;

Z7_COM7F_IMF(CHandler::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  // printf("\nRead _virtPos = %6d  size = %6d\n", (UInt32)_virtPos, size);
  if (_virtPos >= _size)
    return S_OK;
  {
    const UInt64 rem = _size - _virtPos;
    if (size > rem)
      size = (UInt32)rem;
    if (size == 0)
      return S_OK;
  }
 
  for (;;)
  {
    const UInt64 cluster = _virtPos >> _clusterBits;
    const size_t clusterSize = (size_t)1 << _clusterBits;
    const size_t lowBits = (size_t)_virtPos & (clusterSize - 1);
    {
      const size_t rem = clusterSize - lowBits;
      if (size > rem)
        size = (UInt32)rem;
    }
    if (cluster == _cacheCluster)
    {
      memcpy(data, _cache + lowBits, size);
      break;
    }
   
    const UInt64 high = cluster >> _numMidBits;
 
    if (high < _dir.Size())
    {
      const UInt32 tabl = _dir[(size_t)high];
      if (tabl != kEmptyDirItem)
      {
        const size_t midBits = (size_t)cluster & (((size_t)1 << _numMidBits) - 1);
        const Byte *p = _table + ((((size_t)tabl << _numMidBits) + midBits) << 3);
        UInt64 v = Get64(p);
        
        if (v)
        {
          if (v & _compressedFlag)
          {
            if (_version <= 1)
              return E_FAIL;
            /*
            the example of table record for 12-bit clusters (4KB uncompressed):
              2 bits : isCompressed status
              (4 == _clusterBits - 8) bits : (num_sectors - 1)
                  packSize = num_sectors * 512;
                  it uses one additional bit over unpacked cluster_bits.
              (49 == 61 - _clusterBits) bits : offset of 512-byte sector
              9 bits : offset in 512-byte sector
            */
            const unsigned numOffsetBits = 62 - (_clusterBits - 8);
            const UInt64 offset = v & (((UInt64)1 << 62) - 1);
            const size_t dataSize = ((size_t)(offset >> numOffsetBits) + 1) << 9;
            UInt64 sectorOffset = offset & (((UInt64)1 << numOffsetBits) - (1 << 9));
            const UInt64 offset2inCache = sectorOffset - _comprPos;
            
            // _comprPos is aligned for 512-bytes
            // we try to use previous _cacheCompressed that contains compressed data
            // that was read for previous unpacking

            if (sectorOffset >= _comprPos && offset2inCache < _comprSize)
            {
              if (offset2inCache)
              {
                _comprSize -= (size_t)offset2inCache;
                memmove(_cacheCompressed, _cacheCompressed + (size_t)offset2inCache, _comprSize);
                _comprPos = sectorOffset;
              }
              sectorOffset += _comprSize;
            }
            else
            {
              _comprPos = sectorOffset;
              _comprSize = 0;
            }
            
            if (dataSize > _comprSize)
            {
              if (sectorOffset != _posInArc)
              {
                // printf("\nDeflate-Seek %12I64x %12I64x\n", sectorOffset, sectorOffset - _posInArc);
                RINOK(Seek2(sectorOffset))
              }
              if (_cacheCompressed.Size() < dataSize)
                return E_FAIL;
              const size_t dataSize3 = dataSize - _comprSize;
              size_t dataSize2 = dataSize3;
              // printf("\n\n=======\nReadStream = %6d _comprPos = %6d \n", (UInt32)dataSize2, (UInt32)_comprPos);
              const HRESULT hres = ReadStream(Stream, _cacheCompressed + _comprSize, &dataSize2);
              _posInArc += dataSize2;
              RINOK(hres)
              if (dataSize2 != dataSize3)
                return E_FAIL;
              _comprSize += dataSize2;
            }
            
            const size_t kSectorMask = (1 << 9) - 1;
            const size_t offsetInSector = (size_t)offset & kSectorMask;
            _bufInStream->Init(_cacheCompressed + offsetInSector, dataSize - offsetInSector);
            _cacheCluster = (UInt64)(Int64)-1;
            if (_cache.Size() < clusterSize)
              return E_FAIL;
            _bufOutStream->Init(_cache, clusterSize);
            // Do we need to use smaller block than clusterSize for last cluster?
            const UInt64 blockSize64 = clusterSize;
            HRESULT res = _deflateDecoder.Interface()->Code(_bufInStream, _bufOutStream, NULL, &blockSize64, NULL);
            /*
            if (_bufOutStreamSpec->GetPos() != clusterSize)
              memset(_cache + _bufOutStreamSpec->GetPos(), 0, clusterSize - _bufOutStreamSpec->GetPos());
            */
            if (res == S_OK)
              if (!_deflateDecoder->IsFinished()
                  || _bufOutStream->GetPos() != clusterSize)
                res = S_FALSE;
            RINOK(res)
            _cacheCluster = cluster;
            continue;
            /*
            memcpy(data, _cache + lowBits, size);
            break;
            */
          }

          // version_3 supports zero clusters
          if (((UInt32)v & 511) != 1)
          {
            v &= _compressedFlag - 1;
            v += lowBits;
            if (v != _posInArc)
            {
              // printf("\n%12I64x\n", v - _posInArc);
              RINOK(Seek2(v))
            }
            const HRESULT res = Stream->Read(data, size, &size);
            _posInArc += size;
            _virtPos += size;
            if (processedSize)
              *processedSize = size;
            return res;
          }
        }
      }
    }
    
    memset(data, 0, size);
    break;
  }

  _virtPos += size;
  if (processedSize)
    *processedSize = size;
  return S_OK;
}


static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

static const Byte kArcProps[] =
{
  kpidClusterSize,
  kpidSectorSize, // actually we need variable to show table size
  kpidHeadersSize,
  kpidUnpackVer,
  kpidMethod,
  kpidCharacts
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static const CUInt32PCharPair g_IncompatFlags_Characts[] =
{
  {  0, "Dirty" },
  {  1, "Corrupt" },
  {  2, "External_Data_File" },
  {  3, "Compression" },
  {  4, "Extended_L2" }
};

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;

  switch (propID)
  {
    case kpidMainSubfile: prop = (UInt32)0; break;
    case kpidClusterSize: prop = (UInt32)1 << _clusterBits; break;
    case kpidSectorSize: prop = (UInt32)1 << (_numMidBits + 3); break;
    case kpidHeadersSize: prop = _table.Size() + (UInt64)_dir.Size() * 8; break;
    case kpidPhySize: if (_phySize) prop = _phySize; break;
    case kpidUnpackVer: prop = _version; break;
    case kpidCharacts:
    {
      if (_incompatFlags)
      {
        AString s ("incompatible: ");
        // we need to show also high 32-bits.
        s += FlagsToString(g_IncompatFlags_Characts,
            Z7_ARRAY_SIZE(g_IncompatFlags_Characts), (UInt32)_incompatFlags);
        prop = s;
      }
      break;
    }
    case kpidMethod:
    {
      AString s;

      if (_compressionType)
      {
        if (_compressionType == 1)
          s += "ZSTD";
        else
        {
          s += "Compression:";
          s.Add_UInt32(_compressionType);
        }
      }
      else if (_needCompression)
        s.Add_OptSpaced("Deflate");

      if (_cryptMethod)
      {
        s.Add_Space_if_NotEmpty();
        if (_cryptMethod == 1)
          s += "AES";
        if (_cryptMethod == 2)
          s += "LUKS";
        else
        {
          s += "Encryption:";
          s.Add_UInt32(_cryptMethod);
        }
      }
      if (!s.IsEmpty())
        prop = s;
      break;
    }

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_unsupported) v |= kpv_ErrorFlags_UnsupportedMethod;
      // if (_headerError) v |= kpv_ErrorFlags_HeadersError;
      if (!Stream && v == 0)
        v = kpv_ErrorFlags_HeadersError;
      if (v)
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


HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *openCallback)
{
  UInt64 buf64[0x70 / 8];
  RINOK(ReadStream_FALSE(stream, buf64, sizeof(buf64)))
  const void *buf = (const void *)buf64;
  // signature: { 'Q', 'F', 'I', 0xFB }
  if (*(const UInt32 *)buf != Z7_CONV_BE_TO_NATIVE_CONST32(0x514649fb))
    return S_FALSE;
  _version = Get32((const Byte *)(const void *)buf64 + 4);
  if (_version < 1 || _version > 3)
    return S_FALSE;
  
  const UInt64 k_UncompressedSize_MAX = (UInt64)1 << 60;
  const UInt64 k_CompressedSize_MAX   = (UInt64)1 << 60;

  _size = Get64((const Byte *)(const void *)buf64 + 0x18);
  if (_size > k_UncompressedSize_MAX)
    return S_FALSE;
  size_t l1Size;
  UInt32 headerSize;

  if (_version == 1)
  {
    // _mTime = Get32((const Byte *)(const void *)buf64 + 0x14); // is unused in most images
    _clusterBits = ((const Byte *)(const void *)buf64)[0x20];
    _numMidBits  = ((const Byte *)(const void *)buf64)[0x21];
    if (_clusterBits < 9 || _clusterBits > 30)
      return S_FALSE;
    if (_numMidBits < 1 || _numMidBits > 28)
      return S_FALSE;
    _cryptMethod = Get32((const Byte *)(const void *)buf64 + 0x24);
    const unsigned numBits2 = _clusterBits + _numMidBits;
    const UInt64 l1Size64 = (_size + (((UInt64)1 << numBits2) - 1)) >> numBits2;
    if (l1Size64 > ((UInt32)1 << 31))
      return S_FALSE;
    l1Size = (size_t)l1Size64;
    headerSize = 0x30;
  }
  else
  {
    _clusterBits = Get32((const Byte *)(const void *)buf64 + 0x14);
    if (_clusterBits < 9 || _clusterBits > 30)
      return S_FALSE;
    _numMidBits = _clusterBits - 3;
    _cryptMethod = Get32((const Byte *)(const void *)buf64 + 0x20);
    l1Size = Get32((const Byte *)(const void *)buf64 + 0x24);
    headerSize = 0x48;
    if (_version >= 3)
    {
      _incompatFlags = Get64((const Byte *)(const void *)buf64 + 0x48);
      // const UInt64 CompatFlags    = Get64((const Byte *)(const void *)buf64 + 0x50);
      // const UInt64 AutoClearFlags = Get64((const Byte *)(const void *)buf64 + 0x58);
      // const UInt32 RefCountOrder = Get32((const Byte *)(const void *)buf64 + 0x60);
      headerSize = 0x68;
      const UInt32 headerSize2  = Get32((const Byte *)(const void *)buf64 + 0x64);
      if (headerSize2 > (1u << 30))
        return S_FALSE;
      if (headerSize < headerSize2)
          headerSize = headerSize2;
      if (headerSize2 >= 0x68 + 1)
        _compressionType = ((const Byte *)(const void *)buf64)[0x68];
    }

    const UInt64 refOffset = Get64((const Byte *)(const void *)buf64 + 0x30); // must be aligned for cluster
    const UInt32 refClusters = Get32((const Byte *)(const void *)buf64 + 0x38);
    // UInt32 numSnapshots = Get32((const Byte *)(const void *)buf64 + 0x3C);
    // UInt64 snapshotsOffset = Get64((const Byte *)(const void *)buf64 + 0x40); // must be aligned for cluster
    /*
    if (numSnapshots)
      return S_FALSE;
    */
    if (refClusters)
    {
      if (refOffset > k_CompressedSize_MAX)
        return S_FALSE;
      const UInt64 numBytes = (UInt64)refClusters << _clusterBits;
      const UInt64 end = refOffset + numBytes;
      if (end > k_CompressedSize_MAX)
        return S_FALSE;
      /*
      CByteBuffer refs;
      refs.Alloc(numBytes);
      RINOK(InStream_SeekSet(stream, refOffset))
      RINOK(ReadStream_FALSE(stream, refs, numBytes));
      */
      if (_phySize < end)
          _phySize = end;
      /*
      for (size_t i = 0; i < numBytes; i += 2)
      {
        UInt32 v = GetBe16((const Byte *)refs + (size_t)i);
        if (v == 0)
          continue;
      }
      */
    }
  }

  const UInt64 l1Offset = Get64((const Byte *)(const void *)buf64 + 0x28); // must be aligned for cluster ?
  if (l1Offset < headerSize || l1Offset > k_CompressedSize_MAX)
    return S_FALSE;
  if (_phySize < headerSize)
      _phySize = headerSize;

  // we use 32 MiB limit for L1 size, as QEMU with QCOW_MAX_L1_SIZE limit.
  if (l1Size > (1u << 22)) // if (l1Size > (1u << (sizeof(size_t) * 8 - 4)))
    return S_FALSE;

  _isArc = true;
  {
    const UInt64 backOffset = Get64((const Byte *)(const void *)buf64 + 8);
    // UInt32 backSize = Get32((const Byte *)(const void *)buf64 + 0x10);
    if (backOffset)
    {
      _unsupported = true;
      return S_FALSE;
    }
  }

  UInt64 fileSize = 0;
  RINOK(InStream_GetSize_SeekToBegin(stream, fileSize))

  const size_t clusterSize = (size_t)1 << _clusterBits;
  const size_t t1SizeBytes = (size_t)l1Size << 3;
  {
    const UInt64 end = l1Offset + t1SizeBytes;
    if (end > k_CompressedSize_MAX)
      return S_FALSE;
    // we need to use align end for empty qcow files
    // some files has no cluster alignment padding at the end
    // but has sector alignment
    // end = (end + clusterSize - 1) >> _clusterBits << _clusterBits;
    if (_phySize < end)
        _phySize = end;
    if (end > fileSize)
      return S_FALSE;
    if (_phySize < fileSize)
    {
      const UInt64 end2 = (end + 511) & ~(UInt64)511;
      if (end2 == fileSize)
        _phySize = end2;
    }
  }
  CObjArray<UInt64> table64(l1Size);
  {
    RINOK(InStream_SeekSet(stream, l1Offset))
    RINOK(ReadStream_FALSE(stream, table64, t1SizeBytes))
  }

  _compressedFlag = (_version <= 1) ? ((UInt64)1 << 63) : ((UInt64)1 << 62);
  const UInt64 offsetMask = _compressedFlag - 1;
  const size_t midSize = (size_t)1 << (_numMidBits + 3);
  size_t numTables = 0;
  size_t i;

  for (i = 0; i < l1Size; i++)
  {
    const UInt64 v = Get64(table64 + (size_t)i) & offsetMask;
    if (!v)
      continue;
    numTables++;
    const UInt64 end = v + midSize;
    if (end > k_CompressedSize_MAX)
      return S_FALSE;
    if (_phySize < end)
        _phySize = end;
    if (end > fileSize)
      return S_FALSE;
  }

  if (numTables)
  {
    const size_t size = (size_t)numTables << (_numMidBits + 3);
    if (size >> (_numMidBits + 3) != numTables)
      return E_OUTOFMEMORY;
    _table.Alloc(size);
    if (!_table.IsAllocated())
      return E_OUTOFMEMORY;
    if (openCallback)
    {
      const UInt64 totalBytes = size;
      RINOK(openCallback->SetTotal(NULL, &totalBytes))
    }
  }

  _dir.SetSize((unsigned)l1Size);

  UInt32 curTable = 0;

  for (i = 0; i < l1Size; i++)
  {
    Byte *buf2;
    {
      const UInt64 v = Get64(table64 + (size_t)i) & offsetMask;
      if (v == 0)
      {
        _dir[i] = kEmptyDirItem;
        continue;
      }
      _dir[i] = curTable;
      const size_t tableOffset = (size_t)curTable << (_numMidBits + 3);
      buf2 = (Byte *)_table + tableOffset;
      curTable++;
      if (openCallback && (tableOffset & 0xFFFFF) == 0)
      {
        const UInt64 numBytes = tableOffset;
        RINOK(openCallback->SetCompleted(NULL, &numBytes))
      }
      RINOK(InStream_SeekSet(stream, v))
      RINOK(ReadStream_FALSE(stream, buf2, midSize))
    }

    for (size_t k = 0; k < midSize; k += 8)
    {
      const UInt64 v = Get64((const Byte *)buf2 + (size_t)k);
      if (v == 0)
        continue;
      UInt64 offset = v & offsetMask;
      size_t dataSize = clusterSize;
      
      if (v & _compressedFlag)
      {
        if (_version <= 1)
        {
          const unsigned numOffsetBits = 63 - _clusterBits;
          dataSize = ((size_t)(offset >> numOffsetBits) + 1) << 9;
          offset &= ((UInt64)1 << numOffsetBits) - 1;
          dataSize = 0; // why ?
          // offset &= ~(((UInt64)1 << 9) - 1);
        }
        else
        {
          const unsigned numOffsetBits = 62 - (_clusterBits - 8);
          dataSize = ((size_t)(offset >> numOffsetBits) + 1) << 9;
          offset &= ((UInt64)1 << numOffsetBits) - (1 << 9);
        }
        _needCompression = true;
      }
      else
      {
        const UInt32 low = (UInt32)v & 511;
        if (low)
        {
          // version_3 supports zero clusters
          if (_version < 3 || low != 1)
          {
            _unsupported = true;
            return S_FALSE;
          }
        }
      }
      
      const UInt64 end = offset + dataSize;
      if (_phySize < end)
          _phySize = end;
    }
  }

  if (curTable != numTables)
    return E_FAIL;

  if (_cryptMethod)
    _unsupported = true;
  if (_needCompression && _version <= 1) // that case was not implemented
    _unsupported = true;
  if (_compressionType)
    _unsupported = true;

  Stream = stream;
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Close())
{
  _table.Free();
  _dir.Free();
  // _cache.Free();
  // _cacheCompressed.Free();
  _phySize = 0;

  _cacheCluster = (UInt64)(Int64)-1;
  _comprPos = 0;
  _comprSize = 0;

  _needCompression = false;
  _isArc = false;
  _unsupported = false;

  _compressionType = 0;
  _incompatFlags = 0;

  // CHandlerImg:
  Clear_HandlerImg_Vars();
  Stream.Release();
  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream))
{
  COM_TRY_BEGIN
  *stream = NULL;
  if (_unsupported || !Stream)
    return S_FALSE;
  if (_needCompression)
  {
    if (_version <= 1 || _compressionType)
      return S_FALSE;
    _bufInStream.Create_if_Empty();
    _bufOutStream.Create_if_Empty();
    _deflateDecoder.Create_if_Empty();
    _deflateDecoder->Set_NeedFinishInput(true);
    const size_t clusterSize = (size_t)1 << _clusterBits;
    _cache.AllocAtLeast(clusterSize);
    _cacheCompressed.AllocAtLeast(clusterSize * 2);
  }
  CMyComPtr<ISequentialInStream> streamTemp = this;
  RINOK(InitAndSeek())
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}


REGISTER_ARC_I(
  "QCOW", "qcow qcow2 qcow2c", NULL, 0xCA,
  k_Signature,
  0,
  0,
  NULL)

}}
