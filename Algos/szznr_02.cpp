/* XArchive amalgamation of 7-Zip 26.01 -- Archive Zip/Nsis/Rar readers.
 *
 * 2 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/7zip/Archive/Nsis/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

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

/* ---- CPP/Common/DynLimBuf.h ---- */
// Common/DynLimBuf.h

#ifndef ZIP7_INC_COMMON_DYN_LIM_BUF_H
#define ZIP7_INC_COMMON_DYN_LIM_BUF_H

#include <string.h>

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

class CDynLimBuf
{
  Byte *_chars;
  size_t _pos;
  size_t _size;
  size_t _sizeLimit;
  bool _error;

  CDynLimBuf(const CDynLimBuf &s);

  // ---------- forbidden functions ----------
  CDynLimBuf &operator+=(wchar_t c);

public:
  CDynLimBuf(size_t limit) throw();
  ~CDynLimBuf() { MyFree(_chars); }

  size_t Len() const { return _pos; }
  bool IsError() const { return _error; }
  void Empty() { _pos = 0; _error = false; }

  operator const Byte *() const { return _chars; }
  // const char *Ptr() const { return _chars; }

  CDynLimBuf &operator+=(char c) throw();
  CDynLimBuf &operator+=(const char *s) throw();
};


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

/* ---- CPP/7zip/Archive/Nsis/NsisDecode.h ---- */
// NsisDecode.h

#ifndef ZIP7_INC_NSIS_DECODE_H
#define ZIP7_INC_NSIS_DECODE_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace NNsis {

namespace NMethodType
{
  enum EEnum
  {
    kCopy,
    kDeflate,
    kBZip2,
    kLZMA,
    kZstd
  };
}

/* 7-Zip installers 4.38 - 9.08 used modified version of NSIS that
   supported BCJ filter for better compression ratio.
   We support such modified NSIS archives. */

class CDecoder
{
  NMethodType::EEnum _curMethod; // method of created decoder

  CFilterCoder *_filter;
  CMyComPtr<ISequentialInStream> _filterInStream;
  CMyComPtr<ISequentialInStream> _codecInStream;
  CMyComPtr<ISequentialInStream> _decoderInStream;

  NCompress::NBZip2::CNsisDecoder *_bzDecoder;
  NCompress::NDeflate::NDecoder::CCOMCoder *_deflateDecoder;
  NCompress::NLzma::CDecoder *_lzmaDecoder;
  NCompress::NZstd::CDecoder *_zstdDecoder;

public:
  CMyComPtr<IInStream> InputStream; // for non-solid
  UInt64 StreamPos; // the pos in unpacked for solid, the pos in Packed for non-solid
  
  NMethodType::EEnum Method;
  bool FilterFlag;
  bool Solid;
  bool IsNsisDeflate;
  
  CByteBuffer Buffer; // temp buf

  CDecoder():
      FilterFlag(false),
      Solid(true),
      IsNsisDeflate(true)
  {
    _bzDecoder = NULL;
    _deflateDecoder = NULL;
    _lzmaDecoder = NULL;
    _zstdDecoder = NULL;
  }

  void Release()
  {
    _filterInStream.Release();
    _codecInStream.Release();
    _decoderInStream.Release();
    InputStream.Release();

    _bzDecoder = NULL;
    _deflateDecoder = NULL;
    _lzmaDecoder = NULL;
    _zstdDecoder = NULL;
  }

  UInt64 GetInputProcessedSize() const;
  
  HRESULT Init(ISequentialInStream *inStream, bool &useFilter);

  HRESULT Read(void *data, size_t *processedSize)
  {
    return ReadStream(_decoderInStream, data, processedSize);
  }


  HRESULT SetToPos(UInt64 pos, ICompressProgressInfo *progress); // for solid
  HRESULT Decode(CByteBuffer *outBuf, bool unpackSizeDefined, UInt32 unpackSize,
      ISequentialOutStream *realOutStream, ICompressProgressInfo *progress,
      UInt32 &packSizeRes, UInt32 &unpackSizeRes);
};

}}

#endif

/* ---- CPP/7zip/Archive/Nsis/NsisIn.h ---- */
// NsisIn.h

#ifndef ZIP7_INC_ARCHIVE_NSIS_IN_H
#define ZIP7_INC_ARCHIVE_NSIS_IN_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

/* If NSIS_SCRIPT is defined, it will decompile NSIS script to [NSIS].nsi file.
   The code is much larger in that case. */
 
// #define NSIS_SCRIPT

namespace NArchive {
namespace NNsis {

const size_t kScriptSizeLimit = 1 << 27;

const unsigned kSignatureSize = 16;
extern const Byte kSignature[kSignatureSize];
#define NSIS_SIGNATURE { 0xEF, 0xBE, 0xAD, 0xDE, 'N', 'u', 'l', 'l', 's', 'o', 'f', 't', 'I', 'n', 's', 't' }

const UInt32 kFlagsMask = 0xF;
namespace NFlags
{
  const UInt32 kUninstall = 1;
  const UInt32 kSilent = 2;
  const UInt32 kNoCrc = 4;
  const UInt32 kForceCrc = 8;
  // NSISBI fork flags:
  const UInt32 k_BI_LongOffset = 16;
  const UInt32 k_BI_ExternalFileSupport = 32;
  const UInt32 k_BI_ExternalFile = 64;
  const UInt32 k_BI_IsStubInstaller = 128;
}

struct CFirstHeader
{
  UInt32 Flags;
  UInt32 HeaderSize;
  UInt32 ArcSize;

  bool ThereIsCrc() const
  {
    return
        (Flags & NFlags::kForceCrc) != 0 ||
        (Flags & NFlags::kNoCrc) == 0;
  }

  UInt32 GetDataSize() const { return ArcSize - (ThereIsCrc() ? 4 : 0); }
};


struct CBlockHeader
{
  UInt32 Offset;
  UInt32 Num;

  void Parse(const Byte *p, unsigned bhoSize);
};

struct CItem
{
  bool IsEmptyFile;
  bool IsCompressed;
  bool Size_Defined;
  bool CompressedSize_Defined;
  bool EstimatedSize_Defined;
  bool Attrib_Defined;
  bool IsUninstaller;
  // bool UseFilter;
  
  UInt32 Attrib;
  UInt32 Pos;       // = 0, if (IsEmptyFile == true)
  UInt32 Size;
  UInt32 CompressedSize;
  UInt32 EstimatedSize;
  UInt32 DictionarySize;
  UInt32 PatchSize; // for Uninstaller.exe
  int Prefix; // - 1 means no prefix

  FILETIME MTime;
  AString NameA;
  UString NameU;
  
  bool Is_PatchedUninstaller() const { return PatchSize != 0; }
  
  CItem():
      IsEmptyFile(false),
      IsCompressed(true),
      Size_Defined(false),
      CompressedSize_Defined(false),
      EstimatedSize_Defined(false),
      Attrib_Defined(false),
      IsUninstaller(false),
      // UseFilter(false),
      Attrib(0),
      Pos(0),
      Size(0),
      CompressedSize(0),
      EstimatedSize(0),
      DictionarySize(1),
      PatchSize(0),
      Prefix(-1)
  {
    MTime.dwLowDateTime = 0;
    MTime.dwHighDateTime = 0;
  }

  /*
  bool IsINSTDIR() const
  {
    return (PrefixA.Len() >= 3 || PrefixU.Len() >= 3);
  }
  */
};

enum ENsisType
{
  k_NsisType_Nsis2,
  k_NsisType_Nsis3,
  k_NsisType_Park1, // Park 2.46.1-
  k_NsisType_Park2, // Park 2.46.2  : GetFontVersion
  k_NsisType_Park3  // Park 2.46.3+ : GetFontName
};

#ifdef NSIS_SCRIPT

struct CSection
{
  UInt32 InstallTypes; // bits set for each of the different install_types, if any.
  UInt32 Flags; // SF_* - defined above
  UInt32 StartCmdIndex; // code;
  UInt32 NumCommands; // code_size;
  UInt32 SizeKB;
  UInt32 Name;

  void Parse(const Byte *data);
};

struct CLicenseFile
{
  UInt32 Offset;
  UInt32 Size;
  AString Name;
  CByteBuffer Text;
};

#endif

class CInArchive
{
public:
  #ifdef NSIS_SCRIPT
  CDynLimBuf Script;
  #endif
  CByteBuffer _data;
  CObjectVector<CItem> Items;
  bool IsUnicode;
  bool Is64Bit;
private:
  UInt32 _stringsPos;     // relative to _data
  UInt32 NumStringChars;
  size_t _size;           // it's Header Size

  AString Raw_AString;
  UString Raw_UString;

  ENsisType NsisType;
  bool IsNsis200; // NSIS 2.03 and before
  bool IsNsis225; // NSIS 2.25 and before
  bool LogCmdIsEnabled;
  int BadCmd; // -1: no bad command; in another cases lowest bad command id

  bool IsPark() const { return NsisType >= k_NsisType_Park1; }
  bool IsNsis3_OrHigher() const { return NsisType == k_NsisType_Nsis3; }

  UInt64 _fileSize;
  
  bool _headerIsCompressed;
  UInt32 _nonSolidStartOffset;

  #ifdef NSIS_SCRIPT
  
  CByteBuffer strUsed;

  CBlockHeader bhPages;
  CBlockHeader bhSections;
  CBlockHeader bhCtlColors;
  CBlockHeader bhData;
  UInt32 AfterHeaderSize;
  CByteBuffer _afterHeader;

  UInt32 SectionSize;
  const Byte *_mainLang;
  UInt32 _numLangStrings;
  AString LangComment;
  CRecordVector<UInt32> langStrIDs;
  UInt32 numOnFunc;
  UInt32 onFuncOffset;
  // CRecordVector<UInt32> OnFuncs;
  unsigned _numRootLicenses;
  CRecordVector<UInt32> noParseStringIndexes;
  AString _tempString_for_GetVar;
  AString _tempString_for_AddFuncName;
  AString _tempString;

  #endif


public:
  CMyComPtr<IInStream> _stream; // it's limited stream that contains only NSIS archive
  UInt64 StartOffset;           // offset in original stream.
  UInt64 DataStreamOffset;      // = sizeof(FirstHeader) = offset of Header in _stream

  bool IsArc;

  CDecoder Decoder;
  CByteBuffer ExeStub;
  CFirstHeader FirstHeader;
  NMethodType::EEnum Method;
  UInt32 DictionarySize;
  bool IsSolid;
  bool UseFilter;
  bool FilterFlag;
  
  bool IsInstaller;
  AString Name;
  AString BrandingText;
  UStringVector UPrefixes;
  AStringVector APrefixes;

  #ifdef NSIS_SCRIPT
  CObjectVector<CLicenseFile> LicenseFiles;
  #endif

private:
  void GetShellString(AString &s, unsigned index1, unsigned index2);
  void GetNsisString_Raw(const Byte *s);
  void GetNsisString_Unicode_Raw(const Byte *s);
  void ReadString2_Raw(UInt32 pos);
  bool IsGoodString(UInt32 param) const;
  bool AreTwoParamStringsEqual(UInt32 param1, UInt32 param2) const;

  void Add_LangStr(AString &res, UInt32 id);

  #ifdef NSIS_SCRIPT

  void Add_UInt(UInt32 v);
  void AddLicense(UInt32 param, Int32 langID);

  void Add_LangStr_Simple(UInt32 id);
  void Add_FuncName(const UInt32 *labels, UInt32 index);
  void AddParam_Func(const UInt32 *labels, UInt32 index);
  void Add_LabelName(UInt32 index);

  void Add_Color2(UInt32 v);
  void Add_ColorParam(UInt32 v);
  void Add_Color(UInt32 index);

  void Add_ButtonID(UInt32 buttonID);

  void Add_ShowWindow_Cmd(UInt32 cmd);
  void Add_TypeFromList(const char * const *table, unsigned tableSize, UInt32 type);
  void Add_ExecFlags(UInt32 flagsType);
  void Add_SectOp(UInt32 opType);

  void Add_Var(UInt32 index);
  void AddParam_Var(UInt32 value);
  void AddParam_UInt(UInt32 value);

  void Add_GotoVar(UInt32 param);
  void Add_GotoVar1(UInt32 param);
  void Add_GotoVars2(const UInt32 *params);


 
  bool PrintSectionBegin(const CSection &sect, unsigned index);
  void PrintSectionEnd();

  void GetNsisString(AString &res, const Byte *s);
  void GetNsisString_Unicode(AString &res, const Byte *s);
  UInt32 GetNumUsedVars() const;
  void ReadString2(AString &s, UInt32 pos);

  void MessageBox_MB_Part(UInt32 param);
  void AddParam(UInt32 pos);
  void AddOptionalParam(UInt32 pos);
  void AddParams(const UInt32 *params, unsigned num);
  void AddPageOption1(UInt32 param, const char *name);
  void AddPageOption(const UInt32 *params, unsigned num, const char *name);
  void AddOptionalParams(const UInt32 *params, unsigned num);
  void AddRegRoot(UInt32 value);

 
  void ClearLangComment();
  void Separator();
  void Space();
  void Tab();
  void Tab(bool commented);
  void BigSpaceComment();
  void SmallSpaceComment();
  void AddCommentAndString(const char *s);
  void AddError(const char *s);
  void AddErrorLF(const char *s);
  void CommentOpen();
  void CommentClose();
  void AddLF();
  void AddQuotes();
  void TabString(const char *s);
  void AddStringLF(const char *s);
  void NewLine();
  void PrintNumComment(const char *name, UInt32 value);
  void Add_QuStr(const AString &s);
  void SpaceQuStr(const AString &s);
  bool CompareCommands(const Byte *rawCmds, const Byte *sequence, size_t numCommands);

  #endif

  #ifdef NSIS_SCRIPT
  unsigned GetNumSupportedCommands() const;
  #endif

  UInt32 GetCmd(UInt32 a);
  void FindBadCmd(const CBlockHeader &bh, const Byte *);
  void DetectNsisType(const CBlockHeader &bh, const Byte *);

  HRESULT ReadEntries(const CBlockHeader &bh);
  HRESULT SortItems();
  HRESULT Parse();
  HRESULT Open2(const Byte *data, size_t size);
  void Clear2();

  void GetVar2(AString &res, UInt32 index);
  void GetVar(AString &res, UInt32 index);
  Int32 GetVarIndex(UInt32 strPos) const;
  Int32 GetVarIndex(UInt32 strPos, UInt32 &resOffset) const;
  Int32 GetVarIndexFinished(UInt32 strPos, Byte endChar, UInt32 &resOffset) const;
  bool IsVarStr(UInt32 strPos, UInt32 varIndex) const;
  bool IsAbsolutePathVar(UInt32 strPos) const;
  void SetItemName(CItem &item, UInt32 strPos);

public:
  HRESULT Open(IInStream *inStream, const UInt64 *maxCheckStartPosition);
  AString GetFormatDescription() const;
  HRESULT InitDecoder()
  {
    bool useFilter;
    return Decoder.Init(_stream, useFilter);
  }

  HRESULT SeekTo(UInt64 pos)
  {
    return InStream_SeekSet(_stream, pos);
  }

  HRESULT SeekTo_DataStreamOffset()
  {
    return SeekTo(DataStreamOffset);
  }

  HRESULT SeekToNonSolidItem(unsigned index)
  {
    return SeekTo(GetPosOfNonSolidItem(index));
  }

  void Clear();

  bool IsDirectString_Equal(UInt32 offset, const char *s) const;
  /*
  UInt64 GetDataPos(unsigned index)
  {
    const CItem &item = Items[index];
    return GetOffset() + FirstHeader.HeaderSize + item.Pos;
  }
  */

  UInt64 GetPosOfSolidItem(unsigned index) const
  {
    const CItem &item = Items[index];
    return 4 + (UInt64)FirstHeader.HeaderSize + item.Pos;
  }
  
  UInt64 GetPosOfNonSolidItem(unsigned index) const
  {
    const CItem &item = Items[index];
    return DataStreamOffset + _nonSolidStartOffset + 4 + item.Pos;
  }

  void Release()
  {
    Decoder.Release();
  }

  bool IsTruncated() const { return (_fileSize - StartOffset < FirstHeader.ArcSize); }

  UString GetReducedName(unsigned index) const
  {
    const CItem &item = Items[index];

    UString s;
    if (item.Prefix >= 0)
    {
      if (IsUnicode)
        s = UPrefixes[item.Prefix];
      else
        s = MultiByteToUnicodeString(APrefixes[item.Prefix]);
      if (s.Len() > 0)
        if (s.Back() != L'\\')
          s.Add_Char('\\');
    }

    if (IsUnicode)
    {
      s += item.NameU;
      if (item.NameU.IsEmpty())
        s += "file";
    }
    else
    {
      s += MultiByteToUnicodeString(item.NameA);
      if (item.NameA.IsEmpty())
        s += "file";
    }
    
    const char * const kRemoveStr = "$INSTDIR\\";
    if (s.IsPrefixedBy_Ascii_NoCase(kRemoveStr))
    {
      s.Delete(0, MyStringLen(kRemoveStr));
      if (s[0] == L'\\')
        s.DeleteFrontal(1);
    }
    if (item.Is_PatchedUninstaller() && ExeStub.Size() == 0)
      s += ".nsis";
    return s;
  }

  UString ConvertToUnicode(const AString &s) const;

  CInArchive()
    #ifdef NSIS_SCRIPT
      : Script(kScriptSizeLimit)
    #endif
    {}
};

}}
  
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

/* ---- CPP/7zip/Archive/Nsis/NsisHandler.h ---- */
// NSisHandler.h

#ifndef ZIP7_INC_NSIS_HANDLER_H
#define ZIP7_INC_NSIS_HANDLER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NNsis {

Z7_CLASS_IMP_CHandler_IInArchive_0

  CInArchive _archive;
  AString _methodString;

  bool GetUncompressedSize(unsigned index, UInt32 &size) const;
  bool GetCompressedSize(unsigned index, UInt32 &size) const;

  // AString GetMethod(NMethodType::EEnum method, bool useItemFilter, UInt32 dictionary) const;
};

}}

#endif

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Archive/Nsis/NsisIn.cpp ================ */
// NsisIn.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)

// #define NUM_SPEED_TESTS 1000

namespace NArchive {
namespace NNsis {

static const size_t kInputBufSize = 1 << 20;

const Byte kSignature[kSignatureSize] = NSIS_SIGNATURE;
static const UInt32 kMask_IsCompressed = (UInt32)1 << 31;

static const unsigned kNumCommandParams = 6;
static const unsigned kCmdSize = 4 + kNumCommandParams * 4;

#ifdef NSIS_SCRIPT
#define CR_LF "\x0D\x0A"
#endif

static const char * const kErrorStr = "$_ERROR_STR_";

#define RINOZ(x) { int _tt_ = (x); if (_tt_ != 0) return _tt_; }


/* There are several versions of NSIS:
   1) Original NSIS:
        NSIS-2 ANSI
        NSIS-3 ANSI
        NSIS-3 Unicode
   2) NSIS from Jim Park that extends old NSIS-2 to Unicode support:
        NSIS-Park-(1,2,3) ANSI
        NSIS-Park-(1,2,3) Unicode

   The command IDs layout is slightly different for different versions.
   Also there are additional "log" versions of NSIS that support EW_LOG.
   We use the layout of "NSIS-3 Unicode" without "log" as main layout.
   And we transfer the command IDs to main layout, if another layout is detected. */


enum
{
  EW_INVALID_OPCODE,
  EW_RET,               // Return
  EW_NOP,               // Nop, Goto
  EW_ABORT,             // Abort
  EW_QUIT,              // Quit
  EW_CALL,              // Call, InitPluginsDir
  EW_UPDATETEXT,        // DetailPrint
  EW_SLEEP,             // Sleep
  EW_BRINGTOFRONT,      // BringToFront
  EW_CHDETAILSVIEW,     // SetDetailsView
  EW_SETFILEATTRIBUTES, // SetFileAttributes
  EW_CREATEDIR,         // CreateDirectory, SetOutPath
  EW_IFFILEEXISTS,      // IfFileExists
  EW_SETFLAG,           // SetRebootFlag, ...
  EW_IFFLAG,            // IfAbort, IfSilent, IfErrors, IfRebootFlag
  EW_GETFLAG,           // GetInstDirError, GetErrorLevel
  EW_RENAME,            // Rename
  EW_GETFULLPATHNAME,   // GetFullPathName
  EW_SEARCHPATH,        // SearchPath
  EW_GETTEMPFILENAME,   // GetTempFileName
  EW_EXTRACTFILE,       // File
  EW_DELETEFILE,        // Delete
  EW_MESSAGEBOX,        // MessageBox
  EW_RMDIR,             // RMDir
  EW_STRLEN,            // StrLen
  EW_ASSIGNVAR,         // StrCpy
  EW_STRCMP,            // StrCmp
  EW_READENVSTR,        // ReadEnvStr, ExpandEnvStrings
  EW_INTCMP,            // IntCmp, IntCmpU
  EW_INTOP,             // IntOp
  EW_INTFMT,            // IntFmt/Int64Fmt
  EW_PUSHPOP,           // Push/Pop/Exchange
  EW_FINDWINDOW,        // FindWindow
  EW_SENDMESSAGE,       // SendMessage
  EW_ISWINDOW,          // IsWindow
  EW_GETDLGITEM,        // GetDlgItem
  EW_SETCTLCOLORS,      // SetCtlColors
  EW_SETBRANDINGIMAGE,  // SetBrandingImage / LoadAndSetImage
  EW_CREATEFONT,        // CreateFont
  EW_SHOWWINDOW,        // ShowWindow, EnableWindow, HideWindow
  EW_SHELLEXEC,         // ExecShell
  EW_EXECUTE,           // Exec, ExecWait
  EW_GETFILETIME,       // GetFileTime
  EW_GETDLLVERSION,     // GetDLLVersion

  // EW_GETFONTVERSION, // Park : 2.46.2
  // EW_GETFONTNAME,    // Park : 2.46.3
 
  EW_REGISTERDLL,       // RegDLL, UnRegDLL, CallInstDLL
  EW_CREATESHORTCUT,    // CreateShortCut
  EW_COPYFILES,         // CopyFiles
  EW_REBOOT,            // Reboot
  EW_WRITEINI,          // WriteINIStr, DeleteINISec, DeleteINIStr, FlushINI
  EW_READINISTR,        // ReadINIStr
  EW_DELREG,            // DeleteRegValue, DeleteRegKey
  EW_WRITEREG,          // WriteRegStr, WriteRegExpandStr, WriteRegBin, WriteRegDWORD
  EW_READREGSTR,        // ReadRegStr, ReadRegDWORD
  EW_REGENUM,           // EnumRegKey, EnumRegValue
  EW_FCLOSE,            // FileClose
  EW_FOPEN,             // FileOpen
  EW_FPUTS,             // FileWrite, FileWriteByte
  EW_FGETS,             // FileRead, FileReadByte

  // Park
  // EW_FPUTWS,            // FileWriteUTF16LE, FileWriteWord
  // EW_FGETWS,            // FileReadUTF16LE, FileReadWord
  
  EW_FSEEK,             // FileSeek
  EW_FINDCLOSE,         // FindClose
  EW_FINDNEXT,          // FindNext
  EW_FINDFIRST,         // FindFirst
  EW_WRITEUNINSTALLER,  // WriteUninstaller
  
  // Park : since 2.46.3 the log is enabled in main Park version
  // EW_LOG,               // LogSet, LogText

  EW_SECTIONSET,        // Get*, Set*
  EW_INSTTYPESET,       // InstTypeSetText, InstTypeGetText, SetCurInstType, GetCurInstType

  /*
  // before v3.06 nsis it was so:
  // instructions not actually implemented in exehead, but used in compiler.
  EW_GETLABELADDR,      // both of these get converted to EW_ASSIGNVAR
  EW_GETFUNCTIONADDR,
  */
  
  // v3.06 and later it was changed to:
  EW_GETOSINFO,
  EW_RESERVEDOPCODE,
  
  EW_LOCKWINDOW,        // LockWindow
  
  // 2 unicode commands available only in Unicode archive
  EW_FPUTWS,            // FileWriteUTF16LE, FileWriteWord
  EW_FGETWS,            // FileReadUTF16LE, FileReadWord

  /*
  // since v3.06 the fllowing IDs codes was moved here:
  // Opcodes listed here are not actually used in exehead. No exehead opcodes should be present after these!
  EW_GETLABELADDR,      // --> EW_ASSIGNVAR
  EW_GETFUNCTIONADDR,   // --> EW_ASSIGNVAR
  */

  // The following IDs are not IDs in real order.
  // We just need some IDs to translate eny extended layout to main layout.

  EW_LOG,               // LogSet, LogText

  // Park
  EW_FINDPROC,          // FindProc
  
  EW_GETFONTVERSION,    // GetFontVersion
  EW_GETFONTNAME,       // GetFontName

  kNumCmds
};



struct CCommandInfo
{
  Byte NumParams;
};

static const CCommandInfo k_Commands[kNumCmds] =
{
  { 0 }, // "Invalid" },
  { 0 }, // Return
  { 1 }, // Nop, Goto
  { 1 }, // "Abort" },
  { 0 }, // "Quit" },
  { 2 }, // Call
  { 6 }, // "DetailPrint" }, // 1 param in new versions, 6 in old NSIS versions
  { 1 }, // "Sleep" },
  { 0 }, // "BringToFront" },
  { 2 }, // "SetDetailsView" },
  { 2 }, // "SetFileAttributes" },
  { 3 }, // CreateDirectory, SetOutPath
  { 3 }, // "IfFileExists" },
  { 3 }, // SetRebootFlag, ...
  { 4 }, // "If" }, // IfAbort, IfSilent, IfErrors, IfRebootFlag
  { 2 }, // "Get" }, // GetInstDirError, GetErrorLevel
  { 4 }, // "Rename" },
  { 3 }, // "GetFullPathName" },
  { 2 }, // "SearchPath" },
  { 2 }, // "GetTempFileName" },
  { 6 }, // "File"
  { 2 }, // "Delete" },
  { 6 }, // "MessageBox" },
  { 2 }, // "RMDir" },
  { 2 }, // "StrLen" },
  { 4 }, // StrCpy, GetCurrentAddress
  { 5 }, // "StrCmp" },
  { 3 }, // ReadEnvStr, ExpandEnvStrings
  { 6 }, // "IntCmp" },
  { 4 }, // "IntOp" },
  { 4 }, // "IntFmt" }, EW_INTFMT
  { 6 }, // Push, Pop, Exch // it must be 3 params. But some multi-command write garbage.
  { 5 }, // "FindWindow" },
  { 6 }, // "SendMessage" },
  { 3 }, // "IsWindow" },
  { 3 }, // "GetDlgItem" },
  { 2 }, // "SetCtlColors" },
  { 4 }, // "SetBrandingImage" } // LoadAndSetImage
  { 5 }, // "CreateFont" },
  { 4 }, // ShowWindow, EnableWindow, HideWindow
  { 6 }, // "ExecShell" },
  { 3 }, // "Exec" }, // Exec, ExecWait
  { 3 }, // "GetFileTime" },
  { 4 }, // "GetDLLVersion" },
  { 6 }, // RegDLL, UnRegDLL, CallInstDLL // it must be 5 params. But some multi-command write garbage.
  { 6 }, // "CreateShortCut" },
  { 4 }, // "CopyFiles" },
  { 1 }, // "Reboot" },
  { 5 }, // WriteINIStr, DeleteINISec, DeleteINIStr, FlushINI
  { 4 }, // "ReadINIStr" },
  { 5 }, // "DeleteReg" }, // DeleteRegKey, DeleteRegValue
  { 6 }, // "WriteReg" },  // WriteRegStr, WriteRegExpandStr, WriteRegBin, WriteRegDWORD
  { 5 }, // "ReadReg" }, // ReadRegStr, ReadRegDWORD
  { 5 }, // "EnumReg" }, // EnumRegKey, EnumRegValue
  { 1 }, // "FileClose" },
  { 4 }, // "FileOpen" },
  { 3 }, // "FileWrite" }, // FileWrite, FileWriteByte
  { 4 }, // "FileRead" }, // FileRead, FileReadByte
  { 4 }, // "FileSeek" },
  { 1 }, // "FindClose" },
  { 2 }, // "FindNext" },
  { 3 }, // "FindFirst" },
  { 4 }, // "WriteUninstaller" },
  { 5 }, // "Section" },  // ***
  { 4 }, // InstTypeSetText, InstTypeGetText, SetCurInstType, GetCurInstType
  
  // { 6 }, // "GetLabelAddr" }, // before 3.06
  { 6 }, // "GetOsInfo" }, GetKnownFolderPath, ReadMemory, // v3.06+
  
  { 2 }, // "GetFunctionAddress" }, // before 3.06

  { 1 }, // "LockWindow" },
  { 4 }, // "FileWrite" }, // FileWriteUTF16LE, FileWriteWord
  { 4 }, // "FileRead" }, // FileReadUTF16LE, FileReadWord
  
  { 2 }, // "Log" }, // LogSet, LogText
  // Park
  { 2 }, // "FindProc" },
  { 2 }, // "GetFontVersion" },
  { 2 }, // "GetFontName" }
};

#ifdef NSIS_SCRIPT

static const char * const k_CommandNames[kNumCmds] =
{
    "Invalid"
  , NULL // Return
  , NULL // Nop, Goto
  , "Abort"
  , "Quit"
  , NULL // Call
  , "DetailPrint" // 1 param in new versions, 6 in old NSIS versions
  , "Sleep"
  , "BringToFront"
  , "SetDetailsView"
  , "SetFileAttributes"
  , NULL // CreateDirectory, SetOutPath
  , "IfFileExists"
  , NULL // SetRebootFlag, ...
  , "If" // IfAbort, IfSilent, IfErrors, IfRebootFlag
  , "Get" // GetInstDirError, GetErrorLevel
  , "Rename"
  , "GetFullPathName"
  , "SearchPath"
  , "GetTempFileName"
  , NULL // File
  , "Delete"
  , "MessageBox"
  , "RMDir"
  , "StrLen"
  , NULL // StrCpy, GetCurrentAddress
  , "StrCmp"
  , NULL // ReadEnvStr, ExpandEnvStrings
  , NULL // IntCmp / Int64Cmp / EW_INTCMP
  , "IntOp"
  , NULL // IntFmt / Int64Fmt / EW_INTFMT
  , NULL // Push, Pop, Exch // it must be 3 params. But some multi-command write garbage.
  , "FindWindow"
  , "SendMessage"
  , "IsWindow"
  , "GetDlgItem"
  , "SetCtlColors"
  , "SetBrandingImage"
  , "CreateFont"
  , NULL // ShowWindow, EnableWindow, HideWindow
  , "ExecShell"
  , "Exec" // Exec, ExecWait
  , "GetFileTime"
  , "GetDLLVersion"
  , NULL // RegDLL, UnRegDLL, CallInstDLL // it must be 5 params. But some multi-command write garbage.
  , "CreateShortCut"
  , "CopyFiles"
  , "Reboot"
  , NULL // WriteINIStr, DeleteINISec, DeleteINIStr, FlushINI
  , "ReadINIStr"
  , "DeleteReg" // DeleteRegKey, DeleteRegValue
  , "WriteReg"  // WriteRegStr, WriteRegExpandStr, WriteRegBin, WriteRegDWORD
  , "ReadReg" // ReadRegStr, ReadRegDWORD
  , "EnumReg" // EnumRegKey, EnumRegValue
  , "FileClose"
  , "FileOpen"
  , "FileWrite" // FileWrite, FileWriteByte
  , "FileRead" // FileRead, FileReadByte
  , "FileSeek"
  , "FindClose"
  , "FindNext"
  , "FindFirst"
  , "WriteUninstaller"
  , "Section"  // ***
  , NULL // InstTypeSetText, InstTypeGetText, SetCurInstType, GetCurInstType
  
  , NULL // "GetOsInfo" // , "GetLabelAddr" //
  , "GetFunctionAddress"
  
  , "LockWindow"
  , "FileWrite" // FileWriteUTF16LE, FileWriteWord
  , "FileRead" // FileReadUTF16LE, FileReadWord
  
  , "Log" // LogSet, LogText

  // Park
  , "FindProc"
  , "GetFontVersion"
  , "GetFontName"
};

#endif

/* NSIS can use one name for two CSIDL_*** and CSIDL_COMMON_*** items (CurrentUser / AllUsers)
   Some NSIS shell names are not identical to WIN32 CSIDL_* names.
   NSIS doesn't use some CSIDL_* values. But we add name for all CSIDL_ (marked with '+'). */

static const char * const kShellStrings[] =
{
    "DESKTOP"     // +
  , "INTERNET"    // +
  , "SMPROGRAMS"  // CSIDL_PROGRAMS
  , "CONTROLS"    // +
  , "PRINTERS"    // +
  , "DOCUMENTS"   // CSIDL_PERSONAL
  , "FAVORITES"   // CSIDL_FAVORITES
  , "SMSTARTUP"   // CSIDL_STARTUP
  , "RECENT"      // CSIDL_RECENT
  , "SENDTO"      // CSIDL_SENDTO
  , "BITBUCKET"   // +
  , "STARTMENU"
  , NULL          // CSIDL_MYDOCUMENTS = CSIDL_PERSONAL
  , "MUSIC"       // CSIDL_MYMUSIC
  , "VIDEOS"      // CSIDL_MYVIDEO
  , NULL
  , "DESKTOP"     // CSIDL_DESKTOPDIRECTORY
  , "DRIVES"      // +
  , "NETWORK"     // +
  , "NETHOOD"
  , "FONTS"
  , "TEMPLATES"
  , "STARTMENU"   // CSIDL_COMMON_STARTMENU
  , "SMPROGRAMS"  // CSIDL_COMMON_PROGRAMS
  , "SMSTARTUP"   // CSIDL_COMMON_STARTUP
  , "DESKTOP"     // CSIDL_COMMON_DESKTOPDIRECTORY
  , "APPDATA"     // CSIDL_APPDATA         !!! "QUICKLAUNCH"
  , "PRINTHOOD"
  , "LOCALAPPDATA"
  , "ALTSTARTUP"
  , "ALTSTARTUP"  // CSIDL_COMMON_ALTSTARTUP
  , "FAVORITES"   // CSIDL_COMMON_FAVORITES
  , "INTERNET_CACHE"
  , "COOKIES"
  , "HISTORY"
  , "APPDATA"     // CSIDL_COMMON_APPDATA
  , "WINDIR"
  , "SYSDIR"
  , "PROGRAM_FILES" // +
  , "PICTURES"    // CSIDL_MYPICTURES
  , "PROFILE"
  , "SYSTEMX86" // +
  , "PROGRAM_FILESX86" // +
  , "PROGRAM_FILES_COMMON" // +
  , "PROGRAM_FILES_COMMONX8" // +  CSIDL_PROGRAM_FILES_COMMONX86
  , "TEMPLATES"   // CSIDL_COMMON_TEMPLATES
  , "DOCUMENTS"   // CSIDL_COMMON_DOCUMENTS
  , "ADMINTOOLS"  // CSIDL_COMMON_ADMINTOOLS
  , "ADMINTOOLS"  // CSIDL_ADMINTOOLS
  , "CONNECTIONS" // +
  , NULL
  , NULL
  , NULL
  , "MUSIC"       // CSIDL_COMMON_MUSIC
  , "PICTURES"    // CSIDL_COMMON_PICTURES
  , "VIDEOS"      // CSIDL_COMMON_VIDEO
  , "RESOURCES"
  , "RESOURCES_LOCALIZED"
  , "COMMON_OEM_LINKS" // +
  , "CDBURN_AREA"
  , NULL // unused
  , "COMPUTERSNEARME" // +
};


static inline void UIntToString(AString &s, UInt32 v)
{
  s.Add_UInt32(v);
}

#ifdef NSIS_SCRIPT

void CInArchive::Add_UInt(UInt32 v)
{
  char sz[16];
  ConvertUInt32ToString(v, sz);
  Script += sz;
}

static void Add_SignedInt(CDynLimBuf &s, Int32 v)
{
  char sz[32];
  ConvertInt64ToString(v, sz);
  s += sz;
}

static void Add_Hex(CDynLimBuf &s, UInt32 v)
{
  char sz[16];
  sz[0] = '0';
  sz[1] = 'x';
  ConvertUInt32ToHex(v, sz + 2);
  s += sz;
}

static UInt32 GetUi16Str_Len(const Byte *p)
{
  const Byte *pp = p;
  for (; *pp != 0 || *(pp + 1) != 0; pp += 2);
  return (UInt32)((pp - p) >> 1);
}

void CInArchive::AddLicense(UInt32 param, Int32 langID)
{
  Space();
  if (param >= NumStringChars ||
      param + 1 >= NumStringChars)
  {
    Script += kErrorStr;
    return;
  }
  strUsed[param] = 1;

  const UInt32 start = _stringsPos + (IsUnicode ? param * 2 : param);
  const UInt32 offset = start + (IsUnicode ? 2 : 1);
  {
    FOR_VECTOR (i, LicenseFiles)
    {
      const CLicenseFile &lic = LicenseFiles[i];
      if (offset == lic.Offset)
      {
        Script += lic.Name;
        return;
      }
    }
  }
  AString fileName ("[LICENSE]");
  if (langID >= 0)
  {
    fileName += "\\license-";
    // LangId_To_String(fileName, langID);
    UIntToString(fileName, (UInt32)langID);
  }
  else if (++_numRootLicenses > 1)
  {
    fileName.Add_Minus();
    UIntToString(fileName, _numRootLicenses);
  }
  const Byte *sz = (_data + start);
  const unsigned marker = IsUnicode ? Get16(sz) : *sz;
  const bool isRTF = (marker == 2);
  fileName += isRTF ? ".rtf" : ".txt"; // if (*sz == 1) it's text;
  Script += fileName;

  CLicenseFile &lic = LicenseFiles.AddNew();
  lic.Name = fileName;
  lic.Offset = offset;
  if (!IsUnicode)
    lic.Size = (UInt32)strlen((const char *)sz + 1);
  else
  {
    sz += 2;
    const UInt32 len = GetUi16Str_Len(sz);
    lic.Size = len * 2;
    if (isRTF)
    {
      lic.Text.Alloc((size_t)len);
      for (UInt32 i = 0; i < len; i++, sz += 2)
      {
        unsigned c = Get16(sz);
        if (c >= 256)
          c = '?';
        lic.Text[i] = (Byte)(c);
      }
      lic.Size = len;
      lic.Offset = 0;
    }
  }
}

#endif


#ifdef NSIS_SCRIPT
#define Z7_NSIS_WIN_GENERIC_READ    ((UInt32)1 << 31)
#endif
#define Z7_NSIS_WIN_GENERIC_WRITE   ((UInt32)1 << 30)
#ifdef NSIS_SCRIPT
#define Z7_NSIS_WIN_GENERIC_EXECUTE ((UInt32)1 << 29)
#define Z7_NSIS_WIN_GENERIC_ALL     ((UInt32)1 << 28)
#endif

#ifdef NSIS_SCRIPT
#define Z7_NSIS_WIN_CREATE_NEW        1
#endif
#define Z7_NSIS_WIN_CREATE_ALWAYS     2
#ifdef NSIS_SCRIPT
#define Z7_NSIS_WIN_OPEN_EXISTING     3
#define Z7_NSIS_WIN_OPEN_ALWAYS       4
#define Z7_NSIS_WIN_TRUNCATE_EXISTING 5
#endif


// #define kVar_CMDLINE    20
#define kVar_INSTDIR    21
#define kVar_OUTDIR     22
#define kVar_EXEDIR     23
// #define kVar_LANGUAGE   24
#define kVar_TEMP       25
#define kVar_PLUGINSDIR 26
#define kVar_EXEPATH    27  // NSIS 2.26+
// #define kVar_EXEFILE    28  // NSIS 2.26+

#define kVar_HWNDPARENT_225 27
#ifdef NSIS_SCRIPT
#define kVar_HWNDPARENT     29
#endif

// #define kVar__CLICK 30
#define kVar_Spec_OUTDIR_225  29  // NSIS 2.04 - 2.25
#define kVar_Spec_OUTDIR      31  // NSIS 2.26+


static const char * const kVarStrings[] =
{
    "CMDLINE"
  , "INSTDIR"
  , "OUTDIR"
  , "EXEDIR"
  , "LANGUAGE"
  , "TEMP"
  , "PLUGINSDIR"
  , "EXEPATH"   // NSIS 2.26+
  , "EXEFILE"   // NSIS 2.26+
  , "HWNDPARENT"
  , "_CLICK"    // is set from page->clicknext
  , "_OUTDIR"   // NSIS 2.04+
};

static const unsigned kNumInternalVars = 20 + Z7_ARRAY_SIZE(kVarStrings);

#define GET_NUM_INTERNAL_VARS (IsNsis200 ? kNumInternalVars - 3 : IsNsis225 ? kNumInternalVars - 2 : kNumInternalVars)

void CInArchive::GetVar2(AString &res, UInt32 index)
{
  if (index < 20)
  {
    if (index >= 10)
    {
      res += 'R';
      index -= 10;
    }
    UIntToString(res, index);
  }
  else
  {
    unsigned numInternalVars = GET_NUM_INTERNAL_VARS;
    if (index < numInternalVars)
    {
      if (IsNsis225 && index >= kVar_EXEPATH)
        index += 2;
      res += kVarStrings[index - 20];
    }
    else
    {
      res += '_';
      UIntToString(res, index - numInternalVars);
      res += '_';
    }
  }
}

void CInArchive::GetVar(AString &res, UInt32 index)
{
  res += '$';
  GetVar2(res, index);
}

#ifdef NSIS_SCRIPT

void CInArchive::Add_Var(UInt32 index)
{
  _tempString_for_GetVar.Empty();
  GetVar(_tempString_for_GetVar, index);
  Script += _tempString_for_GetVar;
}

void CInArchive::AddParam_Var(UInt32 index)
{
  Space();
  Add_Var(index);
}

void CInArchive::AddParam_UInt(UInt32 value)
{
  Space();
  Add_UInt(value);
}

#endif


#define NS_CODE_SKIP    252
#define NS_CODE_VAR     253
#define NS_CODE_SHELL   254
// #define NS_CODE_LANG    255

// #define NS_3_CODE_LANG  1
#define NS_3_CODE_SHELL 2
#define NS_3_CODE_VAR   3
#define NS_3_CODE_SKIP  4

#define PARK_CODE_SKIP  0xE000
#define PARK_CODE_VAR   0xE001
#define PARK_CODE_SHELL 0xE002
#define PARK_CODE_LANG  0xE003

#define IS_NS_SPEC_CHAR(c) ((c) >= NS_CODE_SKIP)
#define IS_PARK_SPEC_CHAR(c) ((c) >= PARK_CODE_SKIP && (c) <= PARK_CODE_LANG)

#define DECODE_NUMBER_FROM_2_CHARS(c0, c1) (((unsigned)(c0) & 0x7F) | (((unsigned)((c1) & 0x7F)) << 7))
#define CONVERT_NUMBER_NS_3_UNICODE(n) n = ((n & 0x7F) | (((n >> 8) & 0x7F) << 7))
#define CONVERT_NUMBER_PARK(n) n &= 0x7FFF


static bool AreStringsEqual_16and8(const Byte *p16, const char *p8)
{
  for (;;)
  {
    unsigned c16 = Get16(p16); p16 += 2;
    unsigned c = (Byte)(*p8++);
    if (c16 != c)
      return false;
    if (c == 0)
      return true;
  }
}

void CInArchive::GetShellString(AString &s, unsigned index1, unsigned index2)
{
  // zeros are not allowed here.
  // if (index1 == 0 || index2 == 0) throw 333;

  if ((index1 & 0x80) != 0)
  {
    unsigned offset = (index1 & 0x3F);

    /* NSIS reads registry string:
         keyName   = HKLM Software\\Microsoft\\Windows\\CurrentVersion
         mask      = KEY_WOW64_64KEY, If 64-bit flag in index1 is set
         valueName = string(offset)
       If registry reading is failed, NSIS uses second parameter (index2)
       to read string. The recursion is possible in that case in NSIS.
       We don't parse index2 string. We only set strUsed status for that
       string (but without recursion). */

    if (offset >= NumStringChars)
    {
      s += kErrorStr;
      return;
    }
    
    #ifdef NSIS_SCRIPT
    strUsed[offset] = 1;
    if (index2 < NumStringChars)
      strUsed[index2] = 1;
    #endif

    const Byte *p = (const Byte *)(_data + _stringsPos);
    int id = -1;
    if (IsUnicode)
    {
      p += offset * 2;
      if (AreStringsEqual_16and8(p, "ProgramFilesDir"))
        id = 0;
      else if (AreStringsEqual_16and8(p, "CommonFilesDir"))
        id = 1;
    }
    else
    {
      p += offset;
      if (strcmp((const char *)p, "ProgramFilesDir") == 0)
        id = 0;
      else if (strcmp((const char *)p, "CommonFilesDir") == 0)
        id = 1;
    }

    s += ((id >= 0) ? (id == 0 ? "$PROGRAMFILES" : "$COMMONFILES") :
      "$_ERROR_UNSUPPORTED_VALUE_REGISTRY_");
    // s += ((index1 & 0x40) != 0) ? "64" : "32";
    if ((index1 & 0x40) != 0)
      s += "64";

    if (id < 0)
    {
      s += '(';
      if (IsUnicode)
      {
        for (unsigned i = 0; i < 256; i++)
        {
          wchar_t c = Get16(p + i * 2);
          if (c == 0)
            break;
          if (c < 0x80)
            s += (char)c;
        }
      }
      else
        s += (const char *)p;
      s += ')';
    }
    return;
  }

  s += '$';
  if (index1 < Z7_ARRAY_SIZE(kShellStrings))
  {
    const char *sz = kShellStrings[index1];
    if (sz)
    {
      s += sz;
      return;
    }
  }
  if (index2 < Z7_ARRAY_SIZE(kShellStrings))
  {
    const char *sz = kShellStrings[index2];
    if (sz)
    {
      s += sz;
      return;
    }
  }
  s += "_ERROR_UNSUPPORTED_SHELL_";
  s += '[';
  UIntToString(s, index1);
  s += ',';
  UIntToString(s, index2);
  s += ']';
}

#ifdef NSIS_SCRIPT

void CInArchive::Add_LangStr_Simple(UInt32 id)
{
  Script += "LSTR_";
  Add_UInt(id);
}

#endif

void CInArchive::Add_LangStr(AString &res, UInt32 id)
{
  #ifdef NSIS_SCRIPT
  langStrIDs.Add(id);
  #endif
  res += "$(LSTR_";
  UIntToString(res, id);
  res += ')';
}

void CInArchive::GetNsisString_Raw(const Byte *s)
{
  Raw_AString.Empty();

  if (NsisType != k_NsisType_Nsis3)
  {
    for (;;)
    {
      Byte c = *s++;
      if (c == 0)
        return;
      if (IS_NS_SPEC_CHAR(c))
      {
        Byte c0 = *s++;
        if (c0 == 0)
          return;
        if (c != NS_CODE_SKIP)
        {
          Byte c1 = *s++;
          if (c1 == 0)
            return;
          
          if (c == NS_CODE_SHELL)
            GetShellString(Raw_AString, c0, c1);
          else
          {
            unsigned n = DECODE_NUMBER_FROM_2_CHARS(c0, c1);
            if (c == NS_CODE_VAR)
              GetVar(Raw_AString, n);
            else //  if (c == NS_CODE_LANG)
              Add_LangStr(Raw_AString, n);
          }
          continue;
        }
        c = c0;
      }
      Raw_AString += (char)c;
    }
  }

  // NSIS-3 ANSI
  for (;;)
  {
    Byte c = *s++;
    if (c <= NS_3_CODE_SKIP)
    {
      if (c == 0)
        return;
      Byte c0 = *s++;
      if (c0 == 0)
        return;
      if (c != NS_3_CODE_SKIP)
      {
        Byte c1 = *s++;
        if (c1 == 0)
          return;
        
        if (c == NS_3_CODE_SHELL)
          GetShellString(Raw_AString, c0, c1);
        else
        {
          unsigned n = DECODE_NUMBER_FROM_2_CHARS(c0, c1);
          if (c == NS_3_CODE_VAR)
            GetVar(Raw_AString, n);
          else // if (c == NS_3_CODE_LANG)
            Add_LangStr(Raw_AString, n);
        }
        continue;
      }
      c = c0;
    }
    Raw_AString += (char)c;
  }
}

#ifdef NSIS_SCRIPT

void CInArchive::GetNsisString(AString &res, const Byte *s)
{
  for (;;)
  {
    Byte c = *s++;
    if (c == 0)
      return;
    if (NsisType != k_NsisType_Nsis3)
    {
      if (IS_NS_SPEC_CHAR(c))
      {
        Byte c0 = *s++;
        if (c0 == 0)
          return;
        if (c != NS_CODE_SKIP)
        {
          Byte c1 = *s++;
          if (c1 == 0)
            return;
          if (c == NS_CODE_SHELL)
            GetShellString(res, c0, c1);
          else
          {
            unsigned n = DECODE_NUMBER_FROM_2_CHARS(c0, c1);
            if (c == NS_CODE_VAR)
              GetVar(res, n);
            else // if (c == NS_CODE_LANG)
              Add_LangStr(res, n);
          }
          continue;
        }
        c = c0;
      }
    }
    else
    {
      // NSIS-3 ANSI
      if (c <= NS_3_CODE_SKIP)
      {
        Byte c0 = *s++;
        if (c0 == 0)
          return;
        if (c0 == 0)
          break;
        if (c != NS_3_CODE_SKIP)
        {
          Byte c1 = *s++;
          if (c1 == 0)
            return;
          if (c == NS_3_CODE_SHELL)
            GetShellString(res, c0, c1);
          else
          {
            unsigned n = DECODE_NUMBER_FROM_2_CHARS(c0, c1);
            if (c == NS_3_CODE_VAR)
              GetVar(res, n);
            else // if (c == NS_3_CODE_LANG)
              Add_LangStr(res, n);
          }
          continue;
        }
        c = c0;
      }
    }

    {
      const char *e;
           if (c ==   9) e = "$\\t";
      else if (c ==  10) e = "$\\n";
      else if (c ==  13) e = "$\\r";
      else if (c == '"') e = "$\\\"";
      else if (c == '$') e = "$$";
      else
      {
        res += (char)c;
        continue;
      }
      res += e;
      continue;
    }
  }
}

#endif

void CInArchive::GetNsisString_Unicode_Raw(const Byte *p)
{
  Raw_UString.Empty();

  if (IsPark())
  {
    for (;;)
    {
      unsigned c = Get16(p);
      p += 2;
      if (c == 0)
        break;
      if (c < 0x80)
      {
        Raw_UString.Add_Char((char)c);
        continue;
      }
      
      if (IS_PARK_SPEC_CHAR(c))
      {
        unsigned n = Get16(p);
        p += 2;
        if (n == 0)
          break;
        if (c != PARK_CODE_SKIP)
        {
          Raw_AString.Empty();
          if (c == PARK_CODE_SHELL)
            GetShellString(Raw_AString, n & 0xFF, n >> 8);
          else
          {
            CONVERT_NUMBER_PARK(n);
            if (c == PARK_CODE_VAR)
              GetVar(Raw_AString, n);
            else // if (c == PARK_CODE_LANG)
              Add_LangStr(Raw_AString, n);
          }
          Raw_UString += Raw_AString.Ptr(); // check it !
          continue;
        }
        c = n;
      }
      
      Raw_UString += (wchar_t)c;
    }
    
    return;
  }

  // NSIS-3 Unicode
  for (;;)
  {
    unsigned c = Get16(p);
    p += 2;
    if (c > NS_3_CODE_SKIP)
    {
      Raw_UString += (wchar_t)c;
      continue;
    }
    if (c == 0)
      break;

    unsigned n = Get16(p);
    p += 2;
    if (n == 0)
      break;
    if (c == NS_3_CODE_SKIP)
    {
      Raw_UString += (wchar_t)n;
      continue;
    }

    Raw_AString.Empty();
    if (c == NS_3_CODE_SHELL)
      GetShellString(Raw_AString, n & 0xFF, n >> 8);
    else
    {
      CONVERT_NUMBER_NS_3_UNICODE(n);
      if (c == NS_3_CODE_VAR)
        GetVar(Raw_AString, n);
      else // if (c == NS_3_CODE_LANG)
        Add_LangStr(Raw_AString, n);
    }
    Raw_UString += Raw_AString.Ptr();
  }
}

#ifdef NSIS_SCRIPT

static const Byte kUtf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

void CInArchive::GetNsisString_Unicode(AString &res, const Byte *p)
{
  for (;;)
  {
    unsigned c = Get16(p);
    p += 2;
    if (c == 0)
      break;
    if (IsPark())
    {
      if (IS_PARK_SPEC_CHAR(c))
      {
        unsigned n = Get16(p);
        p += 2;
        if (n == 0)
          break;
        if (c != PARK_CODE_SKIP)
        {
          if (c == PARK_CODE_SHELL)
            GetShellString(res, n & 0xFF, n >> 8);
          else
          {
            CONVERT_NUMBER_PARK(n);
            if (c == PARK_CODE_VAR)
              GetVar(res, n);
            else // if (c == PARK_CODE_LANG)
              Add_LangStr(res, n);
          }
          continue;
        }
        c = n;
      }
    }
    else
    {
      // NSIS-3 Unicode
      if (c <= NS_3_CODE_SKIP)
      {
        unsigned n = Get16(p);
        p += 2;
        if (n == 0)
          break;
        if (c != NS_3_CODE_SKIP)
        {
          if (c == NS_3_CODE_SHELL)
            GetShellString(res, n & 0xFF, n >> 8);
          else
          {
            CONVERT_NUMBER_NS_3_UNICODE(n);
            if (c == NS_3_CODE_VAR)
              GetVar(res, n);
            else // if (c == NS_3_CODE_LANG)
              Add_LangStr(res, n);
          }
          continue;
        }
        c = n;
      }
    }

    if (c < 0x80)
    {
      const char *e;
           if (c ==   9) e = "$\\t";
      else if (c ==  10) e = "$\\n";
      else if (c ==  13) e = "$\\r";
      else if (c == '"') e = "$\\\"";
      else if (c == '$') e = "$$";
      else
      {
        res += (char)c;
        continue;
      }
      res += e;
      continue;
    }

    UInt32 value = c;
    /*
    if (value >= 0xD800 && value < 0xE000)
    {
      UInt32 c2;
      if (value >= 0xDC00 || srcPos == srcLen)
        break;
      c2 = src[srcPos++];
      if (c2 < 0xDC00 || c2 >= 0xE000)
        break;
      value = (((value - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
    }
    */
    unsigned numAdds;
    for (numAdds = 1; numAdds < 5; numAdds++)
      if (value < (((UInt32)1) << (numAdds * 5 + 6)))
        break;
    res += (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));
    do
    {
      numAdds--;
      res += (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
      // destPos++;
    }
    while (numAdds != 0);

    // AddToUtf8(res, c);
  }
}

#endif

void CInArchive::ReadString2_Raw(UInt32 pos)
{
  Raw_AString.Empty();
  Raw_UString.Empty();
  if ((Int32)pos < 0)
    Add_LangStr(Raw_AString, (UInt32)-((Int32)pos + 1));
  else if (pos >= NumStringChars)
  {
    Raw_AString += kErrorStr;
    // UIntToString(Raw_AString, pos);
  }
  else
  {
    if (IsUnicode)
      GetNsisString_Unicode_Raw(_data + _stringsPos + pos * 2);
    else
      GetNsisString_Raw(_data + _stringsPos + pos);
    return;
  }
  Raw_UString = Raw_AString.Ptr();
}

bool CInArchive::IsGoodString(UInt32 param) const
{
  if (param >= NumStringChars)
    return false;
  if (param == 0)
    return true;
  const Byte *p = _data + _stringsPos;
  unsigned c;
  if (IsUnicode)
    c = Get16(p + param * 2 - 2);
  else
    c = p[param - 1];
  // some files have '\\' character before string?
  return (c == 0 || c == '\\');
}

bool CInArchive::AreTwoParamStringsEqual(UInt32 param1, UInt32 param2) const
{
  if (param1 == param2)
    return true;

  /* NSIS-3.0a1 probably contains bug, so it can use 2 different strings
     with same content. So we check real string also.
     Also it's possible to check identical postfix parts of strings. */

  if (param1 >= NumStringChars ||
      param2 >= NumStringChars)
    return false;

  const Byte *p = _data + _stringsPos;

  if (IsUnicode)
  {
    const Byte *p1 = p + param1 * 2;
    const Byte *p2 = p + param2 * 2;
    for (;;)
    {
      UInt16 c = Get16(p1);
      if (c != Get16(p2))
        return false;
      if (c == 0)
        return true;
      p1 += 2;
      p2 += 2;
    }
  }
  else
  {
    const Byte *p1 = p + param1;
    const Byte *p2 = p + param2;
    for (;;)
    {
      Byte c = *p1++;
      if (c != *p2++)
        return false;
      if (c == 0)
        return true;
    }
  }
}

#ifdef NSIS_SCRIPT

UInt32 CInArchive::GetNumUsedVars() const
{
  UInt32 numUsedVars = 0;
  const Byte *data = (const Byte *)_data + _stringsPos;
  unsigned npi = 0;
  for (UInt32 i = 0; i < NumStringChars;)
  {
    bool process = true;
    if (npi < noParseStringIndexes.Size() && noParseStringIndexes[npi] == i)
    {
      process = false;
      npi++;
    }
    
    if (IsUnicode)
    {
      if (IsPark())
      {
        for (;;)
        {
          unsigned c = Get16(data + i * 2);
          i++;
          if (c == 0)
            break;
          if (IS_PARK_SPEC_CHAR(c))
          {
            UInt32 n = Get16(data + i * 2);
            i++;
            if (n == 0)
              break;
            if (process && c == PARK_CODE_VAR)
            {
              CONVERT_NUMBER_PARK(n);
              n++;
              if (numUsedVars < n)
                numUsedVars = n;
            }
          }
        }
      }
      else // NSIS-3 Unicode
      {
        for (;;)
        {
          unsigned c = Get16(data + i * 2);
          i++;
          if (c == 0)
            break;
          if (c > NS_3_CODE_SKIP)
            continue;
          UInt32 n = Get16(data + i * 2);
          i++;
          if (n == 0)
            break;
          if (process && c == NS_3_CODE_VAR)
          {
            CONVERT_NUMBER_NS_3_UNICODE(n);
            n++;
            if (numUsedVars < n)
              numUsedVars = n;
          }
        }
      }
    }
    else // not Unicode (ANSI)
    {
      if (NsisType != k_NsisType_Nsis3)
      {
        for (;;)
        {
          Byte c = data[i++];
          if (c == 0)
            break;
          if (IS_NS_SPEC_CHAR(c))
          {
            Byte c0 = data[i++];
            if (c0 == 0)
              break;
            if (c == NS_CODE_SKIP)
              continue;
            Byte c1 = data[i++];
            if (c1 == 0)
              break;
            if (process && c == NS_CODE_VAR)
            {
              UInt32 n = DECODE_NUMBER_FROM_2_CHARS(c0, c1);
              n++;
              if (numUsedVars < n)
                numUsedVars = n;
            }
          }
        }
      }
      else
      {
        // NSIS-3 ANSI
        for (;;)
        {
          Byte c = data[i++];
          if (c == 0)
            break;
          if (c > NS_3_CODE_SKIP)
            continue;

          Byte c0 = data[i++];
          if (c0 == 0)
            break;
          if (c == NS_3_CODE_SKIP)
            continue;
          Byte c1 = data[i++];
          if (c1 == 0)
            break;
          if (process && c == NS_3_CODE_VAR)
          {
            UInt32 n = DECODE_NUMBER_FROM_2_CHARS(c0, c1);
            n++;
            if (numUsedVars < n)
              numUsedVars = n;
          }
        }
      }
    }
  }
  return numUsedVars;
}

void CInArchive::ReadString2(AString &s, UInt32 pos)
{
  if ((Int32)pos < 0)
  {
    Add_LangStr(s, (UInt32)-((Int32)pos + 1));
    return;
  }

  if (pos >= NumStringChars)
  {
    s += kErrorStr;
    // UIntToString(s, pos);
    return;
  }

  #ifdef NSIS_SCRIPT
  strUsed[pos] = 1;
  #endif

  if (IsUnicode)
    GetNsisString_Unicode(s, _data + _stringsPos + pos * 2);
  else
    GetNsisString(s, _data + _stringsPos + pos);
}

#endif

#ifdef NSIS_SCRIPT

// #define DEL_DIR     1
#define DEL_RECURSE 2
#define DEL_REBOOT  4
// #define DEL_SIMPLE  8

void CInArchive::AddRegRoot(UInt32 val)
{
  Space();
  const char *s;
  switch (val)
  {
    case 0:  s = "SHCTX"; break;
    case 0x80000000:  s = "HKCR"; break;
    case 0x80000001:  s = "HKCU"; break;
    case 0x80000002:  s = "HKLM"; break;
    case 0x80000003:  s = "HKU";  break;
    case 0x80000004:  s = "HKPD"; break;
    case 0x80000005:  s = "HKCC"; break;
    case 0x80000006:  s = "HKDD"; break;
    case 0x80000050:  s = "HKPT"; break;
    case 0x80000060:  s = "HKPN"; break;
    default:
      // Script += " RRRRR ";
      // throw 1;
      Add_Hex(Script, val); return;
  }
  Script += s;
}

static const char * const g_WinAttrib[] =
{
    "READONLY"
  , "HIDDEN"
  , "SYSTEM"
  , NULL
  , "DIRECTORY"
  , "ARCHIVE"
  , "DEVICE"
  , "NORMAL"
  , "TEMPORARY"
  , "SPARSE_FILE"
  , "REPARSE_POINT"
  , "COMPRESSED"
  , "OFFLINE"
  , "NOT_CONTENT_INDEXED"
  , "ENCRYPTED"
  , NULL
  , "VIRTUAL"
};

#define FLAGS_DELIMITER '|'

static void FlagsToString2(CDynLimBuf &s, const char * const *table, unsigned num, UInt32 flags)
{
  bool filled = false;
  for (unsigned i = 0; i < num; i++)
  {
    UInt32 f = (UInt32)1 << i;
    if ((flags & f) != 0)
    {
      const char *name = table[i];
      if (name)
      {
        if (filled)
          s += FLAGS_DELIMITER;
        filled = true;
        s += name;
        flags &= ~f;
      }
    }
  }
  if (flags != 0)
  {
    if (filled)
      s += FLAGS_DELIMITER;
    Add_Hex(s, flags);
  }
}

static bool DoesNeedQuotes(const char *s)
{
  {
    char c = s[0];
    if (c == 0 || c == '#' || c == ';' || (c == '/' && s[1] == '*'))
      return true;
  }
  for (;;)
  {
    char c = *s++;
    if (c == 0)
      return false;
    if (c == ' ')
      return true;
  }
}

void CInArchive::Add_QuStr(const AString &s)
{
  bool needQuotes = DoesNeedQuotes(s);
  if (needQuotes)
    Script += '\"';
  Script += s;
  if (needQuotes)
    Script += '\"';
}

void CInArchive::SpaceQuStr(const AString &s)
{
  Space();
  Add_QuStr(s);
}

void CInArchive::AddParam(UInt32 pos)
{
  _tempString.Empty();
  ReadString2(_tempString, pos);
  SpaceQuStr(_tempString);
}

void CInArchive::AddParams(const UInt32 *params, unsigned num)
{
  for (unsigned i = 0; i < num; i++)
    AddParam(params[i]);
}

void CInArchive::AddOptionalParam(UInt32 pos)
{
  if (pos != 0)
    AddParam(pos);
}

static unsigned GetNumParams(const UInt32 *params, unsigned num)
{
  for (; num > 0 && params[num - 1] == 0; num--);
  return num;
}
 
void CInArchive::AddOptionalParams(const UInt32 *params, unsigned num)
{
  AddParams(params, GetNumParams(params, num));
}


static const UInt32 CMD_REF_Goto    = (1 << 0);
static const UInt32 CMD_REF_Call    = (1 << 1);
static const UInt32 CMD_REF_Pre     = (1 << 2);
static const UInt32 CMD_REF_Show    = (1 << 3);
static const UInt32 CMD_REF_Leave   = (1 << 4);
static const UInt32 CMD_REF_OnFunc  = (1 << 5);
static const UInt32 CMD_REF_Section = (1 << 6);
static const UInt32 CMD_REF_InitPluginDir = (1 << 7);
// static const UInt32 CMD_REF_Creator = (1 << 5); // CMD_REF_Pre is used instead
static const unsigned CMD_REF_OnFunc_NumShifts = 28; // it uses for onFunc too
static const unsigned CMD_REF_Page_NumShifts = 16; // it uses for onFunc too
static const UInt32 CMD_REF_Page_Mask   = 0x0FFF0000;
static const UInt32 CMD_REF_OnFunc_Mask = 0xF0000000;

inline bool IsPageFunc(UInt32 flag)
{
  return (flag & (CMD_REF_Pre | CMD_REF_Show | CMD_REF_Leave)) != 0;
}

inline bool IsFunc(UInt32 flag)
{
  // return (flag & (CMD_REF_Pre | CMD_REF_Show | CMD_REF_Leave | CMD_REF_OnFunc)) != 0;
  return (flag & (CMD_REF_Call | CMD_REF_Pre | CMD_REF_Show | CMD_REF_Leave | CMD_REF_OnFunc)) != 0;
}

inline bool IsProbablyEndOfFunc(UInt32 flag)
{
  return (flag != 0 && flag != CMD_REF_Goto);
}

static const char * const kOnFunc[] =
{
    "Init"
  , "InstSuccess"
  , "InstFailed"
  , "UserAbort"
  , "GUIInit"
  , "GUIEnd"
  , "MouseOverSection"
  , "VerifyInstDir"
  , "SelChange"
  , "RebootFailed"
};

void CInArchive::Add_FuncName(const UInt32 *labels, UInt32 index)
{
  UInt32 mask = labels[index];
  if (mask & CMD_REF_OnFunc)
  {
    Script += ".on";
    Script += kOnFunc[labels[index] >> CMD_REF_OnFunc_NumShifts];
  }
  else if (mask & CMD_REF_InitPluginDir)
  {
    /*
    if (!IsInstaller)
      Script += "un."
    */
    Script += "Initialize_____Plugins";
  }
  else
  {
    Script += "func_";
    Add_UInt(index);
  }
}

void CInArchive::AddParam_Func(const UInt32 *labels, UInt32 index)
{
  Space();
  if ((Int32)index >= 0)
    Add_FuncName(labels, index);
  else
    AddQuotes();
}


void CInArchive::Add_LabelName(UInt32 index)
{
  Script += "label_";
  Add_UInt(index);
}

// param != 0
void CInArchive::Add_GotoVar(UInt32 param)
{
  Space();
  if ((Int32)param < 0)
    Add_Var((UInt32)-((Int32)param + 1));
  else
    Add_LabelName(param - 1);
}

void CInArchive::Add_GotoVar1(UInt32 param)
{
  if (param == 0)
    Script += " 0";
  else
    Add_GotoVar(param);
}

void CInArchive::Add_GotoVars2(const UInt32 *params)
{
  Add_GotoVar1(params[0]);
  if (params[1] != 0)
    Add_GotoVar(params[1]);
}

static bool NoLabels(const UInt32 *labels, UInt32 num)
{
  for (UInt32 i = 0; i < num; i++)
    if (labels[i] != 0)
      return false;
  return true;
}

static const char * const k_REBOOTOK = " /REBOOTOK";

#define Z7_NSIS_WIN_MB_ABORTRETRYIGNORE 2
#define Z7_NSIS_WIN_MB_RETRYCANCEL      5

static const char * const k_MB_Buttons[] =
{
    "OK"
  , "OKCANCEL"
  , "ABORTRETRYIGNORE"
  , "YESNOCANCEL"
  , "YESNO"
  , "RETRYCANCEL"
  , "CANCELTRYCONTINUE"
};

#define Z7_NSIS_WIN_MB_ICONSTOP   (1 << 4)

static const char * const k_MB_Icons[] =
{
    NULL
  , "ICONSTOP"
  , "ICONQUESTION"
  , "ICONEXCLAMATION"
  , "ICONINFORMATION"
};

static const char * const k_MB_Flags[] =
{
    "HELP"
  , "NOFOCUS"
  , "SETFOREGROUND"
  , "DEFAULT_DESKTOP_ONLY"
  , "TOPMOST"
  , "RIGHT"
  , "RTLREADING"
  // , "SERVICE_NOTIFICATION" // unsupported. That bit is used for NSIS purposes
};

#define Z7_NSIS_WIN_IDCANCEL 2
#define Z7_NSIS_WIN_IDIGNORE 5

static const char * const k_Button_IDs[] =
{
    "0"
  , "IDOK"
  , "IDCANCEL"
  , "IDABORT"
  , "IDRETRY"
  , "IDIGNORE"
  , "IDYES"
  , "IDNO"
  , "IDCLOSE"
  , "IDHELP"
  , "IDTRYAGAIN"
  , "IDCONTINUE"
};

void CInArchive::Add_ButtonID(UInt32 buttonID)
{
  Space();
  if (buttonID < Z7_ARRAY_SIZE(k_Button_IDs))
    Script += k_Button_IDs[buttonID];
  else
  {
    Script += "Button_";
    Add_UInt(buttonID);
  }
}

bool CInArchive::IsDirectString_Equal(UInt32 offset, const char *s) const
{
  if (offset >= NumStringChars)
    return false;
  if (IsUnicode)
    return AreStringsEqual_16and8(_data + _stringsPos + offset * 2, s);
  else
    return strcmp((const char *)(const Byte *)_data + _stringsPos + offset, s) == 0;
}

static bool StringToUInt32(const char *s, UInt32 &res)
{
  const char *end;
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    res = ConvertHexStringToUInt32(s + 2, &end);
  else
    res = ConvertStringToUInt32(s, &end);
  return (*end == 0);
}

static const unsigned k_CtlColors32_Size = 24;
static const unsigned k_CtlColors64_Size = 28;

#define GET_CtlColors_SIZE(is64) ((is64) ? k_CtlColors64_Size : k_CtlColors32_Size)

struct CNsis_CtlColors
{
  UInt32 text; // COLORREF
  UInt32 bkc;  // COLORREF
  UInt32 lbStyle;
  UInt32 bkb; // HBRUSH
  Int32 bkmode;
  Int32 flags;
  UInt32 bkb_hi32;

  void Parse(const Byte *p, bool is64);
};

void CNsis_CtlColors::Parse(const Byte *p, bool is64)
{
  text = Get32(p);
  bkc = Get32(p + 4);
  if (is64)
  {
    bkb = Get32(p + 8);
    bkb_hi32 = Get32(p + 12);
    lbStyle = Get32(p + 16);
    p += 4;
  }
  else
  {
    lbStyle = Get32(p + 8);
    bkb = Get32(p + 12);
  }
  bkmode = (Int32)Get32(p + 16);
  flags = (Int32)Get32(p + 20);
}

// Win32 constants
#define Z7_NSIS_WIN_TRANSPARENT 1
// #define Z7_NSIS_WIN_OPAQUE      2

// text/bg colors
#define kColorsFlags_TEXT     1
#define kColorsFlags_TEXT_SYS 2
#define kColorsFlags_BK       4
#define kColorsFlags_BK_SYS   8
#define kColorsFlags_BKB     16

void CInArchive::Add_Color2(UInt32 v)
{
  v = ((v & 0xFF) << 16) | (v & 0xFF00) | ((v >> 16) & 0xFF);
  char sz[32];
  for (int i = 5; i >= 0; i--)
  {
    unsigned t = v & 0xF;
    v >>= 4;
    sz[i] = (char)(((t < 10) ? ('0' + t) : ('A' + (t - 10))));
  }
  sz[6] = 0;
  Script += sz;
}

void CInArchive::Add_ColorParam(UInt32 v)
{
  Space();
  Add_Color2(v);
}

void CInArchive::Add_Color(UInt32 v)
{
  Script += "0x";
  Add_Color2(v);
}

#define Z7_NSIS_WIN_SW_HIDE 0
#define Z7_NSIS_WIN_SW_SHOWNORMAL 1

#define Z7_NSIS_WIN_SW_SHOWMINIMIZED 2
#define Z7_NSIS_WIN_SW_SHOWMINNOACTIVE 7
#define Z7_NSIS_WIN_SW_SHOWNA 8

static const char * const kShowWindow_Commands[] =
{
    "HIDE"
  , "SHOWNORMAL"     // "NORMAL"
  , "SHOWMINIMIZED"
  , "SHOWMAXIMIZED"  // "MAXIMIZE"
  , "SHOWNOACTIVATE"
  , "SHOW"
  , "MINIMIZE"
  , "SHOWMINNOACTIVE"
  , "SHOWNA"
  , "RESTORE"
  , "SHOWDEFAULT"
  , "FORCEMINIMIZE"  // "MAX"
};

static void Add_ShowWindow_Cmd_2(AString &s, UInt32 cmd)
{
  if (cmd < Z7_ARRAY_SIZE(kShowWindow_Commands))
  {
    s += "SW_";
    s += kShowWindow_Commands[cmd];
  }
  else
    UIntToString(s, cmd);
}

void CInArchive::Add_ShowWindow_Cmd(UInt32 cmd)
{
  if (cmd < Z7_ARRAY_SIZE(kShowWindow_Commands))
  {
    Script += "SW_";
    Script += kShowWindow_Commands[cmd];
  }
  else
    Add_UInt(cmd);
}

void CInArchive::Add_TypeFromList(const char * const *table, unsigned tableSize, UInt32 type)
{
  if (type < tableSize)
    Script += table[type];
  else
  {
    Script += '_';
    Add_UInt(type);
  }
}

#define ADD_TYPE_FROM_LIST(table, type) Add_TypeFromList(table, Z7_ARRAY_SIZE(table), type)

enum
{
  k_ExecFlags_AutoClose,
  k_ExecFlags_ShellVarContext,
  k_ExecFlags_Errors,
  k_ExecFlags_Abort,
  k_ExecFlags_RebootFlag,
  k_ExecFlags_reboot_called,
  k_ExecFlags_cur_insttype,
  k_ExecFlags_plugin_api_version,
  k_ExecFlags_Silent,
  k_ExecFlags_InstDirError,
  k_ExecFlags_rtl,
  k_ExecFlags_ErrorLevel,
  k_ExecFlags_RegView,
  k_ExecFlags_DetailsPrint = 13
};

// Names for NSIS exec_flags_t structure vars
static const char * const kExecFlags_VarsNames[] =
{
    "AutoClose" // autoclose;
  , "ShellVarContext" // all_user_var;
  , "Errors" // exec_error;
  , "Abort" // abort;
  , "RebootFlag" // exec_reboot; // NSIS_SUPPORT_REBOOT
  , "reboot_called" // reboot_called; // NSIS_SUPPORT_REBOOT
  , "cur_insttype" // XXX_cur_insttype; // depreacted
  , "plugin_api_version" // plugin_api_version; // see NSISPIAPIVER_CURR
                          // used to be XXX_insttype_changed
  , "Silent" // silent; // NSIS_CONFIG_SILENT_SUPPORT
  , "InstDirError" // instdir_error;
  , "rtl" // rtl;
  , "ErrorLevel" // errlvl;
  , "RegView" // alter_reg_view;
  , "DetailsPrint" // status_update;
};

void CInArchive::Add_ExecFlags(UInt32 flagsType)
{
  ADD_TYPE_FROM_LIST(kExecFlags_VarsNames, flagsType);
}


// ---------- Page ----------

// page flags
#define PF_CANCEL_ENABLE 4
#define PF_LICENSE_FORCE_SELECTION 32
#define PF_LICENSE_NO_FORCE_SELECTION 64
#define PF_PAGE_EX 512
#define PF_DIR_NO_BTN_DISABLE 1024
/*
#define PF_LICENSE_SELECTED 1
#define PF_NEXT_ENABLE 2
#define PF_BACK_SHOW 8
#define PF_LICENSE_STREAM 16
#define PF_NO_NEXT_FOCUS 128
#define PF_BACK_ENABLE 256
*/

// page window proc
enum
{
  PWP_LICENSE,
  PWP_SELCOM,
  PWP_DIR,
  PWP_INSTFILES,
  PWP_UNINST,
  PWP_COMPLETED,
  PWP_CUSTOM
};

static const char * const kPageTypes[] =
{
    "license"
  , "components"
  , "directory"
  , "instfiles"
  , "uninstConfirm"
  , "COMPLETED"
  , "custom"
};

#define SET_FUNC_REF(x, flag) if ((Int32)(x) >= 0 && (x) < bh.Num) \
  { labels[x] = (labels[x] & ~CMD_REF_Page_Mask) | ((flag) | (pageIndex << CMD_REF_Page_NumShifts)); }

// #define IDD_LICENSE  102
#define IDD_LICENSE_FSRB 108
#define IDD_LICENSE_FSCB 109

void CInArchive::AddPageOption1(UInt32 param, const char *name)
{
  if (param == 0)
    return;
  TabString(name);
  AddParam(param);
  NewLine();
}

void CInArchive::AddPageOption(const UInt32 *params, unsigned num, const char *name)
{
  num = GetNumParams(params, num);
  if (num == 0)
    return;
  TabString(name);
  AddParams(params, num);
  NewLine();
}

void CInArchive::Separator()
{
  AddLF();
  AddCommentAndString("--------------------");
  AddLF();
}

void CInArchive::Space()
{
  Script += ' ';
}

void CInArchive::Tab()
{
  Script += "  ";
}

void CInArchive::Tab(bool commented)
{
  Script += commented ? "    ; " : "  ";
}

void CInArchive::BigSpaceComment()
{
  Script += "    ; ";
}

void CInArchive::SmallSpaceComment()
{
  Script += " ; ";
}

void CInArchive::AddCommentAndString(const char *s)
{
  Script += "; ";
  Script += s;
}

void CInArchive::AddError(const char *s)
{
  BigSpaceComment();
  Script += "!!! ERROR: ";
  Script += s;
}

void CInArchive::AddErrorLF(const char *s)
{
  AddError(s);
  AddLF();
}

void CInArchive::CommentOpen()
{
  AddStringLF("/*");
}

void CInArchive::CommentClose()
{
  AddStringLF("*/");
}

void CInArchive::AddLF()
{
  Script += CR_LF;
}

void CInArchive::AddQuotes()
{
  Script += "\"\"";
}

void CInArchive::TabString(const char *s)
{
  Tab();
  Script += s;
}

void CInArchive::AddStringLF(const char *s)
{
  Script += s;
  AddLF();
}

// ---------- Section ----------

static const char * const kSection_VarsNames[] =
{
    "Text"
  , "InstTypes"
  , "Flags"
  , "Code"
  , "CodeSize"
  , "Size" // size in KB
};

void CInArchive::Add_SectOp(UInt32 opType)
{
  ADD_TYPE_FROM_LIST(kSection_VarsNames, opType);
}

void CSection::Parse(const Byte *p)
{
  Name = Get32(p);
  InstallTypes = Get32(p + 4);
  Flags = Get32(p + 8);
  StartCmdIndex = Get32(p + 12);
  NumCommands = Get32(p + 16);
  SizeKB = Get32(p + 20);
}

// used for section->flags
#define SF_SELECTED   (1 << 0)
#define SF_SECGRP     (1 << 1)
#define SF_SECGRPEND  (1 << 2)
#define SF_BOLD       (1 << 3)
#define SF_RO         (1 << 4)
#define SF_EXPAND     (1 << 5)
/*
#define SF_PSELECTED  (1 << 6)
#define SF_TOGGLED    (1 << 7)
#define SF_NAMECHG    (1 << 8)
*/

bool CInArchive::PrintSectionBegin(const CSection &sect, unsigned index)
{
  AString name;
  if (sect.Flags & SF_BOLD)
    name += '!';
  AString s2;
  ReadString2(s2, sect.Name);
  if (!IsInstaller)
  {
    if (!StringsAreEqualNoCase_Ascii(s2, "uninstall"))
      name += "un.";
  }
  name += s2;
  
  if (sect.Flags & SF_SECGRPEND)
  {
    AddStringLF("SectionGroupEnd");
    return true;
  }

  if (sect.Flags & SF_SECGRP)
  {
    Script += "SectionGroup";
    if (sect.Flags & SF_EXPAND)
      Script += " /e";
    SpaceQuStr(name);
    Script += "    ; Section";
    AddParam_UInt(index);
    NewLine();
    return true;
  }

  Script += "Section";
  if ((sect.Flags & SF_SELECTED) == 0)
    Script += " /o";
  if (!name.IsEmpty())
    SpaceQuStr(name);
  
  /*
  if (!name.IsEmpty())
    Script += ' ';
  else
  */
  SmallSpaceComment();
  Script += "Section_";
  Add_UInt(index);

  /*
  Script += " ; flags = ";
  Add_Hex(Script, sect.Flags);
  */

  NewLine();

  if (sect.SizeKB != 0)
  {
    // probably we must show AddSize, only if there is additional size.
    Tab();
    AddCommentAndString("AddSize");
    AddParam_UInt(sect.SizeKB);
    AddLF();
  }

  bool needSectionIn =
      (sect.Name != 0 && sect.InstallTypes != 0) ||
      (sect.Name == 0 && sect.InstallTypes != 0xFFFFFFFF);
  if (needSectionIn || (sect.Flags & SF_RO) != 0)
  {
    TabString("SectionIn");
    UInt32 instTypes = sect.InstallTypes;
    for (unsigned i = 0; i < 32; i++, instTypes >>= 1)
      if ((instTypes & 1) != 0)
      {
        AddParam_UInt(i + 1);
      }
    if ((sect.Flags & SF_RO) != 0)
      Script += " RO";
    AddLF();
  }
  return false;
}

void CInArchive::PrintSectionEnd()
{
  AddStringLF("SectionEnd");
  AddLF();
}

// static const unsigned kOnFuncShift = 4;

void CInArchive::ClearLangComment()
{
  langStrIDs.Clear();
}

void CInArchive::PrintNumComment(const char *name, UInt32 value)
{
  // size_t len = Script.Len();
  AddCommentAndString(name);
  Script += ": ";
  Add_UInt(value);
  AddLF();
  /*
  len = Script.Len() - len;
  char sz[16];
  ConvertUInt32ToString(value, sz);
  len += MyStringLen(sz);
  for (; len < 20; len++)
    Space();
  AddStringLF(sz);
  */
}


void CInArchive::NewLine()
{
  if (!langStrIDs.IsEmpty())
  {
    BigSpaceComment();
    for (unsigned i = 0; i < langStrIDs.Size() && i < 20; i++)
    {
      /*
      if (i != 0)
        Script += ' ';
      */
      UInt32 langStr = langStrIDs[i];
      if (langStr >= _numLangStrings)
      {
        AddError("langStr");
        break;
      }
      UInt32 param = Get32(_mainLang + langStr * 4);
      if (param != 0)
        AddParam(param);
    }
    ClearLangComment();
  }
  AddLF();
}

static const UInt32 kPageSize = 16 * 4;

static const char * const k_SetOverwrite_Modes[] =
{
    "on"
  , "off"
  , "try"
  , "ifnewer"
  , "ifdiff"
  // "lastused"
};


void CInArchive::MessageBox_MB_Part(UInt32 param)
{
  {
    UInt32 v = param & 0xF;
    Script += " MB_";
    if (v < Z7_ARRAY_SIZE(k_MB_Buttons))
      Script += k_MB_Buttons[v];
    else
    {
      Script += "Buttons_";
      Add_UInt(v);
    }
  }
  {
    UInt32 icon = (param >> 4) & 0x7;
    if (icon != 0)
    {
      Script += "|MB_";
      if (icon < Z7_ARRAY_SIZE(k_MB_Icons) && k_MB_Icons[icon])
        Script += k_MB_Icons[icon];
      else
      {
        Script += "Icon_";
        Add_UInt(icon);
      }
    }
  }
  if ((param & 0x80) != 0)
    Script += "|MB_USERICON";
  {
    UInt32 defButton = (param >> 8) & 0xF;
    if (defButton != 0)
    {
      Script += "|MB_DEFBUTTON";
      Add_UInt(defButton + 1);
    }
  }
  {
    UInt32 modal = (param >> 12) & 0x3;
    if (modal == 1) Script += "|MB_SYSTEMMODAL";
    else if (modal == 2) Script += "|MB_TASKMODAL";
    else if (modal == 3) Script += "|0x3000";
    UInt32 flags = (param >> 14);
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(k_MB_Flags); i++)
      if ((flags & (1 << i)) != 0)
      {
        Script += "|MB_";
        Script += k_MB_Flags[i];
      }
  }
}

#define GET_CMD_PARAM(ppp, index) Get32((ppp) + 4 + (index) * 4)

static const Byte k_InitPluginDir_Commands[] =
  { 13, 26, 31, 13, 19, 21, 11, 14, 25, 31, 1, 22, 4, 1 };

bool CInArchive::CompareCommands(const Byte *rawCmds, const Byte *sequence, size_t numCommands)
{
  for (UInt32 kkk = 0; kkk < numCommands; kkk++, rawCmds += kCmdSize)
    if (GetCmd(Get32(rawCmds)) != sequence[kkk])
      return false;
  return true;
}


static const UInt32 kSectionSize_base = 6 * 4;
// static const UInt32 kSectionSize_8bit = kSectionSize_base + 1024;
// static const UInt32 kSectionSize_16bit = kSectionSize_base + 1024 * 2;
// static const UInt32 kSectionSize_16bit_Big = kSectionSize_base + 8196 * 2;
// 8196 is default string length in NSIS-Unicode since 2.37.3

#endif


static void AddString(AString &dest, const char *src)
{
  dest.Add_Space_if_NotEmpty();
  dest += src;
}

AString CInArchive::GetFormatDescription() const
{
  AString s ("NSIS-");
  char c;
  if (IsPark())
  {
    s += "Park-";
    c = '1';
         if (NsisType == k_NsisType_Park2) c = '2';
    else if (NsisType == k_NsisType_Park3) c = '3';
  }
  else
  {
    c = '2';
    if (NsisType == k_NsisType_Nsis3)
      c = '3';
  }
  s += c;
  if (IsNsis200)
    s += ".00";
  else if (IsNsis225)
    s += ".25";

  if (IsUnicode)
    AddString(s, "Unicode");
  
  if (Is64Bit)
    AddString(s, "64-bit");

  if (LogCmdIsEnabled)
    AddString(s, "log");

  if (BadCmd >= 0)
  {
    AddString(s, "BadCmd=");
    UIntToString(s, (UInt32)BadCmd);
  }
  return s;
}

#ifdef NSIS_SCRIPT

static const unsigned kNumAdditionalParkCmds = 3;

unsigned CInArchive::GetNumSupportedCommands() const
{
  unsigned numCmds = IsPark() ? (unsigned)kNumCmds : (unsigned)(kNumCmds) - kNumAdditionalParkCmds;
  if (!LogCmdIsEnabled)
    numCmds--;
  if (!IsUnicode)
    numCmds -= 2;
  return numCmds;
}

#endif

UInt32 CInArchive::GetCmd(UInt32 a)
{
  if (!IsPark())
  {
    if (!LogCmdIsEnabled)
      return a;
    if (a < EW_SECTIONSET)
      return a;
    if (a == EW_SECTIONSET)
      return EW_LOG;
    return a - 1;
  }

  if (a < EW_REGISTERDLL)
    return a;
  if (NsisType >= k_NsisType_Park2)
  {
    if (a == EW_REGISTERDLL) return EW_GETFONTVERSION;
    a--;
  }
  if (NsisType >= k_NsisType_Park3)
  {
    if (a == EW_REGISTERDLL) return EW_GETFONTNAME;
    a--;
  }
  if (a >= EW_FSEEK)
  {
    if (IsUnicode)
    {
      if (a == EW_FSEEK) return EW_FPUTWS;
      if (a == EW_FSEEK + 1) return EW_FPUTWS + 1;
      a -= 2;
    }
    
    if (a >= EW_SECTIONSET && LogCmdIsEnabled)
    {
      if (a == EW_SECTIONSET)
        return EW_LOG;
      return a - 1;
    }
    if (a == EW_FPUTWS)
      return EW_FINDPROC;
    // if (a > EW_FPUTWS) return 0;
  }
  return a;
}

void CInArchive::FindBadCmd(const CBlockHeader &bh, const Byte *p)
{
  BadCmd = -1;
  
  for (UInt32 kkk = 0; kkk < bh.Num; kkk++, p += kCmdSize)
  {
    const UInt32 id = GetCmd(Get32(p));
    if (id >= kNumCmds)
      continue;
    if (BadCmd >= 0 && id >= (unsigned)BadCmd)
      continue;
    unsigned i;
    if (IsNsis3_OrHigher())
    {
      if (id == EW_RESERVEDOPCODE)
      {
        BadCmd = (int)id;
        continue;
      }
    }
    else
    {
      // if (id == EW_GETLABELADDR || id == EW_GETFUNCTIONADDR)
      if (id == EW_RESERVEDOPCODE || id == EW_GETOSINFO)
      {
        BadCmd = (int)id;
        continue;
      }
    }
    for (i = 6; i != 0; i--)
    {
      const UInt32 param = Get32(p + i * 4);
      if (param != 0)
        break;
    }
    if (id == EW_FINDPROC && i == 0)
    {
      BadCmd = (int)id;
      continue;
    }
    if (k_Commands[id].NumParams < i)
      BadCmd = (int)id;
  }
}

/* We calculate the number of parameters in commands to detect
   layout of commands. It's not very good way.
   If you know simpler and more robust way to detect Version and layout,
   please write to 7-Zip forum */

void CInArchive::DetectNsisType(const CBlockHeader &bh, const Byte *p)
{
  bool strongPark = false;
  bool strongNsis = false;

  if (NumStringChars > 2)
  {
    const Byte *strData = _data + _stringsPos;
    if (IsUnicode)
    {
      UInt32 num = NumStringChars - 2;
      for (UInt32 i = 0; i < num; i++)
      {
        if (Get16(strData + i * 2) == 0)
        {
          unsigned c2 = Get16(strData + 2 + i * 2);
          // it can be TXT/RTF with marker char (1 or 2). so we must check next char
          // if (c2 <= NS_3_CODE_SKIP && c2 != NS_3_CODE_SHELL)
          if (c2 == NS_3_CODE_VAR)
          {
            // 18.06: fixed: is it correct ?
            // if ((Get16(strData + 3 + i * 2) & 0x8000) != 0)
            if ((Get16(strData + 4 + i * 2) & 0x8080) == 0x8080)
            {
              NsisType = k_NsisType_Nsis3;
              strongNsis = true;
              break;
            }
          }
        }
      }
      if (!strongNsis)
      {
        NsisType = k_NsisType_Park1;
        strongPark = true;
      }
    }
    else
    {
      UInt32 num = NumStringChars - 2;
      for (UInt32 i = 0; i < num; i++)
      {
        if (strData[i] == 0)
        {
          Byte c2 = strData[i + 1];
          // it can be TXT/RTF with marker char (1 or 2). so we must check next char
          // for marker=1 (txt)
          if (c2 == NS_3_CODE_VAR)
            // if (c2 <= NS_3_CODE_SKIP && c2 != NS_3_CODE_SHELL && c2 != 1)
          {
            if ((strData[i + 2] & 0x80) != 0)
            {
              // const char *p2 = (const char *)(strData + i + 1);
              // p2 = p2;
              NsisType = k_NsisType_Nsis3;
              strongNsis = true;
              break;
            }
          }
        }
      }
    }
  }

  if (NsisType == k_NsisType_Nsis2 && !IsUnicode)
  {
    const Byte *p2 = p;

    for (UInt32 kkk = 0; kkk < bh.Num; kkk++, p2 += kCmdSize)
    {
      UInt32 cmd = GetCmd(Get32(p2));
      if (cmd != EW_GETDLGITEM &&
          cmd != EW_ASSIGNVAR)
        continue;
      
      UInt32 params[kNumCommandParams];
      for (unsigned i = 0; i < kNumCommandParams; i++)
        params[i] = Get32(p2 + 4 + 4 * i);

      if (cmd == EW_GETDLGITEM)
      {
        // we can use also EW_SETCTLCOLORS
        if (IsVarStr(params[1], kVar_HWNDPARENT_225))
        {
          IsNsis225 = true;
          if (params[0] == kVar_Spec_OUTDIR_225)
          {
            IsNsis200 = true;
            break;
          }
        }
      }
      else // if (cmd == EW_ASSIGNVAR)
      {
        if (params[0] == kVar_Spec_OUTDIR_225 &&
            params[2] == 0 &&
            params[3] == 0 &&
            IsVarStr(params[1], kVar_OUTDIR))
          IsNsis225 = true;
      }
    }
  }

  bool parkVer_WasDetected = false;

  if (!strongNsis && !IsNsis225 && !IsNsis200)
  {
    // it must be before FindBadCmd(bh, p);
    unsigned mask = 0;

    unsigned numInsertMax = IsUnicode ? 4 : 2;

    const Byte *p2 = p;
      
    for (UInt32 kkk = 0; kkk < bh.Num; kkk++, p2 += kCmdSize)
    {
      const UInt32 cmd = Get32(p2); // we use original (not converted) command

      if (cmd < EW_WRITEUNINSTALLER ||
          cmd > EW_WRITEUNINSTALLER + numInsertMax)
        continue;

      UInt32 params[kNumCommandParams];
      for (unsigned i = 0; i < kNumCommandParams; i++)
        params[i] = Get32(p2 + 4 + 4 * i);

      if (params[4] != 0 ||
          params[5] != 0 ||
          params[0] <= 1 ||
          params[3] <= 1)
        continue;

      const UInt32 altParam = params[3];
      if (!IsGoodString(params[0]) ||
          !IsGoodString(altParam))
        continue;

      UInt32 additional = 0;
      if (GetVarIndexFinished(altParam, '\\', additional) != kVar_INSTDIR)
        continue;
      if (AreTwoParamStringsEqual(altParam + additional, params[0]))
      {
        const unsigned numInserts = cmd - EW_WRITEUNINSTALLER;
        mask |= ((unsigned)1 << numInserts);
      }
    }

    if (mask == 1)
    {
      parkVer_WasDetected = true; // it can be original NSIS or Park-1
    }
    else if (mask != 0)
    {
      ENsisType newType = NsisType;
      if (IsUnicode)
        switch (mask)
        {
          case (1 << 3): newType = k_NsisType_Park2; break;
          case (1 << 4): newType = k_NsisType_Park3; break;
        }
      else
        switch (mask)
        {
          case (1 << 1): newType = k_NsisType_Park2; break;
          case (1 << 2): newType = k_NsisType_Park3; break;
        }
      if (newType != NsisType)
      {
        parkVer_WasDetected = true;
        NsisType = newType;
      }
    }
  }

  FindBadCmd(bh, p);

  /*
  if (strongNsis)
    return;
  */

  if (BadCmd < EW_REGISTERDLL)
    return;

  /*
  // in ANSI archive we don't check Park and log version
  if (!IsUnicode)
    return;
  */
  
  // We can support Park-ANSI archives, if we remove if (strongPark) check
  if (strongPark && !parkVer_WasDetected)
  {
    if (BadCmd < EW_SECTIONSET)
    {
      NsisType = k_NsisType_Park3;
      LogCmdIsEnabled = true; // version 3 is provided with log enabled
      FindBadCmd(bh, p);
      if (BadCmd > 0 && BadCmd < EW_SECTIONSET)
      {
        NsisType = k_NsisType_Park2;
        LogCmdIsEnabled = false;
        FindBadCmd(bh, p);
        if (BadCmd > 0 && BadCmd < EW_SECTIONSET)
        {
          NsisType = k_NsisType_Park1;
          FindBadCmd(bh, p);
        }
      }
    }
  }

  if (BadCmd >= EW_SECTIONSET)
  {
    LogCmdIsEnabled = !LogCmdIsEnabled;
    FindBadCmd(bh, p);
    if (BadCmd >= EW_SECTIONSET && LogCmdIsEnabled)
    {
      LogCmdIsEnabled = false;
      FindBadCmd(bh, p);
    }
  }
}

Int32 CInArchive::GetVarIndex(UInt32 strPos) const
{
  if (strPos >= NumStringChars)
    return -1;
  
  if (IsUnicode)
  {
    if (NumStringChars - strPos < 3 * 2)
      return -1;
    const Byte *p = _data + _stringsPos + strPos * 2;
    unsigned code = Get16(p);
    if (IsPark())
    {
      if (code != PARK_CODE_VAR)
        return -1;
      UInt32 n = Get16(p + 2);
      if (n == 0)
        return -1;
      CONVERT_NUMBER_PARK(n);
      return (Int32)n;
    }
    
    // NSIS-3
    {
      if (code != NS_3_CODE_VAR)
        return -1;
      UInt32 n = Get16(p + 2);
      if (n == 0)
        return -1;
      CONVERT_NUMBER_NS_3_UNICODE(n);
      return (Int32)n;
    }
  }
  
  if (NumStringChars - strPos < 4)
    return -1;
  
  const Byte *p = _data + _stringsPos + strPos;
  unsigned c = *p;
  if (NsisType == k_NsisType_Nsis3)
  {
    if (c != NS_3_CODE_VAR)
      return -1;
  }
  else if (c != NS_CODE_VAR)
    return -1;

  const unsigned c0 = p[1];
  if (c0 == 0)
    return -1;
  const unsigned c1 = p[2];
  if (c1 == 0)
    return -1;
  return (Int32)DECODE_NUMBER_FROM_2_CHARS(c0, c1);
}

Int32 CInArchive::GetVarIndex(UInt32 strPos, UInt32 &resOffset) const
{
  resOffset = 0;
  Int32 varIndex = GetVarIndex(strPos);
  if (varIndex < 0)
    return varIndex;
  if (IsUnicode)
  {
    if (NumStringChars - strPos < 2 * 2)
      return -1;
    resOffset = 2;
  }
  else
  {
    if (NumStringChars - strPos < 3)
      return -1;
    resOffset = 3;
  }
  return varIndex;
}

Int32 CInArchive::GetVarIndexFinished(UInt32 strPos, Byte endChar, UInt32 &resOffset) const
{
  resOffset = 0;
  Int32 varIndex = GetVarIndex(strPos);
  if (varIndex < 0)
    return varIndex;
  if (IsUnicode)
  {
    if (NumStringChars - strPos < 3 * 2)
      return -1;
    const Byte *p = _data + _stringsPos + strPos * 2;
    if (Get16(p + 4) != endChar)
      return -1;
    resOffset = 3;
  }
  else
  {
    if (NumStringChars - strPos < 4)
      return -1;
    const Byte *p = _data + _stringsPos + strPos;
    if (p[3] != endChar)
      return -1;
    resOffset = 4;
  }
  return varIndex;
}

bool CInArchive::IsVarStr(UInt32 strPos, UInt32 varIndex) const
{
  if (varIndex > (UInt32)0x7FFF)
    return false;
  UInt32 resOffset;
  return GetVarIndexFinished(strPos, 0, resOffset) == (Int32)varIndex;
}

bool CInArchive::IsAbsolutePathVar(UInt32 strPos) const
{
  Int32 varIndex = GetVarIndex(strPos);
  if (varIndex < 0)
    return false;
  switch (varIndex)
  {
    case kVar_INSTDIR:
    case kVar_EXEDIR:
    case kVar_TEMP:
    case kVar_PLUGINSDIR:
      return true;
  }
  return false;
}

#define IS_LETTER_CHAR(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

// We use same check as in NSIS decoder
static bool IsDrivePath(const wchar_t *s) { return IS_LETTER_CHAR(s[0]) && s[1] == ':' /* && s[2] == '\\' */ ; }
static bool IsDrivePath(const char *s)    { return IS_LETTER_CHAR(s[0]) && s[1] == ':' /* && s[2] == '\\' */ ; }

static bool IsAbsolutePath(const wchar_t *s)
{
  return (s[0] == WCHAR_PATH_SEPARATOR && s[1] == WCHAR_PATH_SEPARATOR) || IsDrivePath(s);
}

static bool IsAbsolutePath(const char *s)
{
  return (s[0] == CHAR_PATH_SEPARATOR && s[1] == CHAR_PATH_SEPARATOR) || IsDrivePath(s);
}

void CInArchive::SetItemName(CItem &item, UInt32 strPos)
{
  ReadString2_Raw(strPos);
  const bool isAbs = IsAbsolutePathVar(strPos);
  if (IsUnicode)
  {
    item.NameU = Raw_UString;
    if (!isAbs && !IsAbsolutePath(Raw_UString))
      item.Prefix = (int)UPrefixes.Size() - 1;
  }
  else
  {
    item.NameA = Raw_AString;
    if (!isAbs && !IsAbsolutePath(Raw_AString))
      item.Prefix = (int)APrefixes.Size() - 1;
  }
}

HRESULT CInArchive::ReadEntries(const CBlockHeader &bh)
{
  #ifdef NSIS_SCRIPT
  CDynLimBuf &s = Script;

  CObjArray<UInt32> labels;
  labels.Alloc(bh.Num);
  memset(labels, 0, bh.Num * sizeof(UInt32));

  {
    const Byte *p = _data;
    UInt32 i;
    for (i = 0; i < numOnFunc; i++)
    {
      UInt32 func = Get32(p + onFuncOffset + 4 * i);
      if (func < bh.Num)
        labels[func] = (labels[func] & ~CMD_REF_OnFunc_Mask) | (CMD_REF_OnFunc | (i << CMD_REF_OnFunc_NumShifts));
    }
  }

  /*
  {
    for (int i = 0; i < OnFuncs.Size(); i++)
    {
      UInt32 address = OnFuncs[i] >> kOnFuncShift;
      if (address < bh.Num)
    }
  }
  */

  if (bhPages.Num != 0)
  {
    Separator();
    PrintNumComment("PAGES", bhPages.Num);

    if (bhPages.Num > (1 << 12)
        || bhPages.Offset > _size
        || bhPages.Num * kPageSize > _size - bhPages.Offset)
    {
      AddErrorLF("Pages error");
    }
    else
    {

    AddLF();
    const Byte *p = _data + bhPages.Offset;
    
    for (UInt32 pageIndex = 0; pageIndex < bhPages.Num; pageIndex++, p += kPageSize)
    {
      UInt32 dlgID = Get32(p);
      UInt32 wndProcID = Get32(p + 4);
      UInt32 preFunc = Get32(p + 8);
      UInt32 showFunc = Get32(p + 12);
      UInt32 leaveFunc = Get32(p + 16);
      UInt32 flags = Get32(p + 20);
      UInt32 caption = Get32(p + 24);
      // UInt32 back = Get32(p + 28);
      UInt32 next = Get32(p + 32);
      // UInt32 clickNext = Get32(p + 36);
      // UInt32 cancel = Get32(p + 40);
      UInt32 params[5];
      for (int i = 0; i < 5; i++)
        params[i] = Get32(p + 44 + 4 * i);

      SET_FUNC_REF(preFunc, CMD_REF_Pre)
      SET_FUNC_REF(showFunc, CMD_REF_Show)
      SET_FUNC_REF(leaveFunc, CMD_REF_Leave)

      if (wndProcID == PWP_COMPLETED)
        CommentOpen();

      AddCommentAndString("Page ");
      Add_UInt(pageIndex);
      AddLF();

      if (flags & PF_PAGE_EX)
      {
        s += "PageEx ";
        if (!IsInstaller)
          s += "un.";
      }
      else
        s += IsInstaller ? "Page " : "UninstPage ";

      if (wndProcID < Z7_ARRAY_SIZE(kPageTypes))
        s += kPageTypes[wndProcID];
      else
        Add_UInt(wndProcID);


      bool needCallbacks = (
          (Int32)preFunc >= 0 ||
          (Int32)showFunc >= 0 ||
          (Int32)leaveFunc >= 0);

      if (flags & PF_PAGE_EX)
      {
        AddLF();
        if (needCallbacks)
          TabString("PageCallbacks");
      }

      if (needCallbacks)
      {
        AddParam_Func(labels, preFunc); // it's creator_function for PWP_CUSTOM
        if (wndProcID != PWP_CUSTOM)
        {
          AddParam_Func(labels, showFunc);
        }
        AddParam_Func(labels, leaveFunc);
      }

      if ((flags & PF_PAGE_EX) == 0)
      {
        // AddOptionalParam(caption);
        if (flags & PF_CANCEL_ENABLE)
          s += " /ENABLECANCEL";
        AddLF();
      }
      else
      {
        AddLF();
        AddPageOption1(caption, "Caption");
      }

        if (wndProcID == PWP_LICENSE)
        {
          if ((flags & PF_LICENSE_NO_FORCE_SELECTION) != 0 ||
              (flags & PF_LICENSE_FORCE_SELECTION) != 0)
          {
            TabString("LicenseForceSelection ");
            if (flags & PF_LICENSE_NO_FORCE_SELECTION)
              s += "off";
            else
            {
              if (dlgID == IDD_LICENSE_FSCB)
                s += "checkbox";
              else if (dlgID == IDD_LICENSE_FSRB)
                s += "radiobuttons";
              else
                Add_UInt(dlgID);
              AddOptionalParams(params + 2, 2);
            }
            NewLine();
          }

          if (params[0] != 0 || next != 0)
          {
            TabString("LicenseText");
            AddParam(params[0]);
            AddOptionalParam(next);
            NewLine();
          }
          if (params[1] != 0)
          {
            TabString("LicenseData");
            if ((Int32)params[1] < 0)
              AddParam(params[1]);
            else
              AddLicense(params[1], -1);
            ClearLangComment();
            NewLine();
          }
        }
        else if (wndProcID == PWP_SELCOM)
          AddPageOption(params, 3, "ComponentsText");
        else if (wndProcID == PWP_DIR)
        {
          AddPageOption(params, 4, "DirText");
          if (params[4] != 0)
          {
            TabString("DirVar");
            AddParam_Var(params[4] - 1);
            AddLF();
          }
          if (flags & PF_DIR_NO_BTN_DISABLE)
          {
            TabString("DirVerify leave");
            AddLF();
          }

        }
        else if (wndProcID == PWP_INSTFILES)
        {
          AddPageOption1(params[2], "CompletedText");
          AddPageOption1(params[1], "DetailsButtonText");
        }
        else if (wndProcID == PWP_UNINST)
        {
          if (params[4] != 0)
          {
            TabString("DirVar");
            AddParam_Var(params[4] - 1);
            AddLF();
          }
          AddPageOption(params, 2, "UninstallText");
        }

      if (flags & PF_PAGE_EX)
      {
        s += "PageExEnd";
        NewLine();
      }
      if (wndProcID == PWP_COMPLETED)
        CommentClose();
      NewLine();
    }
    }
  }

  CObjArray<CSection> Sections;

  {
    Separator();
    PrintNumComment("SECTIONS", bhSections.Num);
    PrintNumComment("COMMANDS", bh.Num);
    AddLF();

    if (bhSections.Num > (1 << 15)
        // || bhSections.Offset > _size
        // || (bhSections.Num * SectionSize > _size - bhSections.Offset)
      )
    {
      AddErrorLF("Sections error");
    }
    else if (bhSections.Num != 0)
    {
      Sections.Alloc((unsigned)bhSections.Num);
      const Byte *p = _data + bhSections.Offset;
      for (UInt32 i = 0; i < bhSections.Num; i++, p += SectionSize)
      {
        CSection &section = Sections[i];
        section.Parse(p);
        if (section.StartCmdIndex < bh.Num)
          labels[section.StartCmdIndex] |= CMD_REF_Section;
      }
    }
  }

  #endif

  const Byte *p;
  UInt32 kkk;

  #ifdef NSIS_SCRIPT

  p = _data + bh.Offset;

  for (kkk = 0; kkk < bh.Num; kkk++, p += kCmdSize)
  {
    UInt32 commandId = GetCmd(Get32(p));
    UInt32 mask;
    switch (commandId)
    {
      case EW_NOP:          mask = 1 << 0; break;
      case EW_IFFILEEXISTS: mask = 3 << 1; break;
      case EW_IFFLAG:       mask = 3 << 0; break;
      case EW_MESSAGEBOX:   mask = 5 << 3; break;
      case EW_STRCMP:       mask = 3 << 2; break;
      case EW_INTCMP:       mask = 7 << 2; break;
      case EW_ISWINDOW:     mask = 3 << 1; break;
      case EW_CALL:
      {
        if (Get32(p + 4 + 4) == 1) // it's Call :Label
        {
          mask = 1 << 0;
          break;
        }
        UInt32 param0 = Get32(p + 4);
        if ((Int32)param0 > 0)
          labels[param0 - 1] |= CMD_REF_Call;
        continue;
      }
      default: continue;
    }
    for (unsigned i = 0; mask != 0; i++, mask >>= 1)
      if (mask & 1)
      {
        UInt32 param = Get32(p + 4 + 4 * i);
        if ((Int32)param > 0 && (Int32)param <= (Int32)bh.Num)
          labels[param - 1] |= CMD_REF_Goto;
      }
  }

  int InitPluginsDir_Start = -1;
  int InitPluginsDir_End = -1;
  p = _data + bh.Offset;
  for (kkk = 0; kkk < bh.Num; kkk++, p += kCmdSize)
  {
    UInt32 flg = labels[kkk];
    /*
    if (IsFunc(flg))
    {
      AddLF();
      for (int i = 0; i < 14; i++)
      {
        UInt32 commandId = GetCmd(Get32(p + kCmdSize * i));
        s += ", ";
        UIntToString(s, commandId);
      }
      AddLF();
    }
    */
    if (IsFunc(flg)
        && bh.Num - kkk >= Z7_ARRAY_SIZE(k_InitPluginDir_Commands)
        && CompareCommands(p, k_InitPluginDir_Commands, Z7_ARRAY_SIZE(k_InitPluginDir_Commands)))
    {
      InitPluginsDir_Start = (int)kkk;
      InitPluginsDir_End = (int)(kkk + Z7_ARRAY_SIZE(k_InitPluginDir_Commands));
      labels[kkk] |= CMD_REF_InitPluginDir;
      break;
    }
  }

  #endif

  // AString prefixA_Temp;
  // UString prefixU_Temp;


  // const UInt32 kFindS = 158;

  #ifdef NSIS_SCRIPT

  UInt32 curSectionIndex = 0;
  // UInt32 lastSectionEndCmd = 0xFFFFFFFF;
  bool sectionIsOpen = false;
  // int curOnFunc = 0;
  bool onFuncIsOpen = false;

  /*
  for (unsigned yyy = 0; yyy + 3 < _data.Size(); yyy++)
  {
    UInt32 val = Get32(_data + yyy);
    if (val == kFindS)
      val = val;
  }
  */

  UInt32 overwrite_State = 0; // "SetOverwrite on"
  Int32 allowSkipFiles_State = -1; // -1: on, -2: off, >=0 : RAW value
  UInt32 endCommentIndex = 0;

  unsigned numSupportedCommands = GetNumSupportedCommands();

  #endif

  p = _data + bh.Offset;
  
  UString spec_outdir_U;
  AString spec_outdir_A;

  UPrefixes.Add(UString("$INSTDIR"));
  APrefixes.Add(AString("$INSTDIR"));

  p = _data + bh.Offset;

  unsigned spec_outdir_VarIndex = IsNsis225 ?
      kVar_Spec_OUTDIR_225 :
      kVar_Spec_OUTDIR;

  for (kkk = 0; kkk < bh.Num; kkk++, p += kCmdSize)
  {
    UInt32 commandId;
    UInt32 params[kNumCommandParams];
    commandId = GetCmd(Get32(p));
    {
      for (unsigned i = 0; i < kNumCommandParams; i++)
      {
        params[i] = Get32(p + 4 + 4 * i);
        /*
        if (params[i] == kFindS)
          i = i;
        */
      }
    }

    #ifdef NSIS_SCRIPT

    bool IsSectionGroup = false;
    while (curSectionIndex < bhSections.Num)
    {
      const CSection &sect = Sections[curSectionIndex];
      if (sectionIsOpen)
      {
        if (sect.StartCmdIndex + sect.NumCommands + 1 != kkk)
          break;
        PrintSectionEnd();
        sectionIsOpen = false;
        // lastSectionEndCmd = kkk;
        curSectionIndex++;
        continue;
      }
      if (sect.StartCmdIndex != kkk)
        break;
      if (PrintSectionBegin(sect, curSectionIndex))
      {
        IsSectionGroup = true;
        curSectionIndex++;
        // do we need to flush prefixes in new section?
        // FlushOutPathPrefixes();
      }
      else
        sectionIsOpen = true;
    }

    /*
    if (curOnFunc < OnFuncs.Size())
    {
      if ((OnFuncs[curOnFunc] >> kOnFuncShift) == kkk)
      {
        s += "Function .on";
        s += kOnFunc[OnFuncs[curOnFunc++] & ((1 << kOnFuncShift) - 1)];
        AddLF();
        onFuncIsOpen = true;
      }
    }
    */

    if (labels[kkk] != 0 && labels[kkk] != CMD_REF_Section)
    {
      UInt32 flg = labels[kkk];
      if (IsFunc(flg))
      {
        if ((int)kkk == InitPluginsDir_Start)
          CommentOpen();

        onFuncIsOpen = true;
        s += "Function ";
        Add_FuncName(labels, kkk);
        if (IsPageFunc(flg))
        {
          BigSpaceComment();
          s += "Page ";
          Add_UInt((flg & CMD_REF_Page_Mask) >> CMD_REF_Page_NumShifts);
          // if (flg & CMD_REF_Creator) s += ", Creator";
          if (flg & CMD_REF_Leave) s += ", Leave";
          if (flg & CMD_REF_Pre) s += ", Pre";
          if (flg & CMD_REF_Show) s += ", Show";
        }
        AddLF();
      }
      if (flg & CMD_REF_Goto)
      {
        Add_LabelName(kkk);
        s += ':';
        AddLF();
      }
    }

    if (commandId != EW_RET)
    {
      Tab(kkk < endCommentIndex);
    }
      
    /*
    UInt32 originalCmd = Get32(p);
    if (originalCmd >= EW_REGISTERDLL)
    {
      UIntToString(s, originalCmd);
      s += ' ';
      if (originalCmd != commandId)
      {
        UIntToString(s, commandId);
        s += ' ';
      }
    }
    */

    unsigned numSkipParams = 0;

    if (commandId < Z7_ARRAY_SIZE(k_Commands) && commandId < numSupportedCommands)
    {
      numSkipParams = k_Commands[commandId].NumParams;
      const char *sz = k_CommandNames[commandId];
      if (sz)
        s += sz;
    }
    else
    {
      s += "Command";
      Add_UInt(commandId);
      /* We don't show wrong commands that use overlapped ids.
         So we change commandId to big value */
      if (commandId < (1 << 12))
        commandId += (1 << 12);
    }

    #endif

    switch (commandId)
    {
      case EW_CREATEDIR:
      {
        bool isSetOutPath = (params[1] != 0);

        if (isSetOutPath)
        {
          UInt32 par0 = params[0];
          
          UInt32 resOffset;
          Int32 idx = GetVarIndex(par0, resOffset);
          if (idx == (Int32)spec_outdir_VarIndex ||
              idx == kVar_OUTDIR)
            par0 += resOffset;

          ReadString2_Raw(par0);

          if (IsUnicode)
          {
            if (idx == (Int32)spec_outdir_VarIndex)
              Raw_UString.Insert(0, spec_outdir_U);
            else if (idx == kVar_OUTDIR)
              Raw_UString.Insert(0, UPrefixes.Back());
            UPrefixes.Add(Raw_UString);
          }
          else
          {
            if (idx == (Int32)spec_outdir_VarIndex)
              Raw_AString.Insert(0, spec_outdir_A);
            else if (idx == kVar_OUTDIR)
              Raw_AString.Insert(0, APrefixes.Back());
            APrefixes.Add(Raw_AString);
          }
        }
        
        #ifdef NSIS_SCRIPT
        s += isSetOutPath ? "SetOutPath" : "CreateDirectory";
        AddParam(params[0]);
        if (params[2] != 0) // 2.51+ & 3.0b3+
        {
          SmallSpaceComment();
          s += "CreateRestrictedDirectory";
        }
        #endif
        
        break;
      }


      case EW_ASSIGNVAR:
      {
        if (params[0] == spec_outdir_VarIndex)
        {
          spec_outdir_U.Empty();
          spec_outdir_A.Empty();
          if (IsVarStr(params[1], kVar_OUTDIR) &&
              params[2] == 0 &&
              params[3] == 0)
          {
            spec_outdir_U = UPrefixes.Back(); // outdir_U;
            spec_outdir_A = APrefixes.Back(); // outdir_A;
          }
        }

        #ifdef NSIS_SCRIPT
        
        if (params[2] == 0 &&
            params[3] == 0 &&
            params[4] == 0 &&
            params[5] == 0 &&
            params[1] != 0 &&
            params[1] < NumStringChars)
        {
          char sz[16];
          ConvertUInt32ToString(kkk + 1, sz);
          if (IsDirectString_Equal(params[1], sz))
          {
            // we suppose that it's GetCurrentAddress command
            // but there is probability that it's StrCpy command
            s += "GetCurrentAddress";
            AddParam_Var(params[0]);
            SmallSpaceComment();
          }
        }
        s += "StrCpy";
        AddParam_Var(params[0]);
        AddParam(params[1]);

        AddOptionalParams(params + 2, 2);

        #endif

        break;
      }

      case EW_EXTRACTFILE:
      {
        CItem &item = Items.AddNew();

        UInt32 par1 = params[1];

        SetItemName(item, par1);
          
        item.Pos = params[2];
        item.MTime.dwLowDateTime = params[3];
        item.MTime.dwHighDateTime = params[4];
        
        #ifdef NSIS_SCRIPT
        
        {
          UInt32 overwrite = params[0] & 0x7;
          if (overwrite != overwrite_State)
          {
            s += "SetOverwrite ";
            ADD_TYPE_FROM_LIST(k_SetOverwrite_Modes, overwrite);
            overwrite_State = overwrite;
            AddLF();
            Tab(kkk < endCommentIndex);
          }
        }

        {
          UInt32 nsisMB = params[0] >> 3;
          if ((Int32)nsisMB != allowSkipFiles_State)
          {
            UInt32 mb = nsisMB & ((1 << 20) - 1);  // old/new NSIS
            UInt32 b1 = nsisMB >> 21;  // NSIS 2.06+
            UInt32 b2 = nsisMB >> 20;  // NSIS old
            Int32 asf = (Int32)nsisMB;
            if (mb == (Z7_NSIS_WIN_MB_ABORTRETRYIGNORE | Z7_NSIS_WIN_MB_ICONSTOP) && (b1 == Z7_NSIS_WIN_IDIGNORE || b2 == Z7_NSIS_WIN_IDIGNORE))
              asf = -1;
            else if (mb == (Z7_NSIS_WIN_MB_RETRYCANCEL | Z7_NSIS_WIN_MB_ICONSTOP) && (b1 == Z7_NSIS_WIN_IDCANCEL || b2 == Z7_NSIS_WIN_IDCANCEL))
              asf = -2;
            else
            {
              AddCommentAndString("AllowSkipFiles [Overwrite]: ");
              MessageBox_MB_Part(mb);
              if (b1 != 0)
              {
                s += " /SD";
                Add_ButtonID(b1);
              }
            }
            if (asf != allowSkipFiles_State)
            {
              if (asf < 0)
              {
                s += "AllowSkipFiles ";
                s += (asf == -1) ? "on" : "off";
              }
              AddLF();
              Tab(kkk < endCommentIndex);
            }
            allowSkipFiles_State = (Int32)nsisMB;
          }
        }
          
        s += "File";
        AddParam(params[1]);

        /* params[5] contains link to LangString (negative value)
           with NLF_FILE_ERROR or NLF_FILE_ERROR_NOIGNORE message for MessageBox.
           We don't need to print it. */
        
        #endif
        
        if (IsVarStr(par1, 10)) // is $R0
        {
          // we parse InstallLib macro in 7-Zip installers
          unsigned kBackOffset = 28;
          if (kkk > 1)
          {
            // detect old version of InstallLib macro
            if (Get32(p - 1 * kCmdSize) == EW_NOP) // goto command
              kBackOffset -= 2;
          }

          if (kkk > kBackOffset)
          {
            const Byte *p2 = p - kBackOffset * kCmdSize;
            UInt32 cmd = Get32(p2);
            if (cmd == EW_ASSIGNVAR)
            {
              UInt32 pars[6];
              for (int i = 0; i < 6; i++)
                pars[i] = Get32(p2 + i * 4 + 4);
              if (pars[0] == 10 + 4 && pars[2] == 0 && pars[3] == 0) // 10 + 4 means $R4
              {
                item.Prefix = -1;
                item.NameA.Empty();
                item.NameU.Empty();
                SetItemName(item, pars[1]);
                // maybe here we can restore original item name, if new name is empty
              }
            }
          }
        }
        /* UInt32 allowIgnore = params[5]; */
        break;
      }

      case EW_SETFILEATTRIBUTES:
      {
        if (kkk > 0 && Get32(p - kCmdSize) == EW_EXTRACTFILE)
        {
          if (params[0] == Get32(p - kCmdSize + 4 + 4 * 1)) // compare with PrevCmd.Params[1]
          {
            CItem &item = Items.Back();
            item.Attrib_Defined = true;
            item.Attrib = params[1];
          }
        }
        #ifdef NSIS_SCRIPT
        AddParam(params[0]);
        Space();
        FlagsToString2(s, g_WinAttrib, Z7_ARRAY_SIZE(g_WinAttrib), params[1]);
        #endif
        break;
      }

      case EW_WRITEUNINSTALLER:
      {
        /* NSIS 2.29+ writes alternative path to params[3]
             "$INSTDIR\\" + Str(params[0])
           NSIS installer uses alternative path, if main path
           from params[0] is not absolute path */

        const bool pathOk = (params[0] > 0) && IsGoodString(params[0]);

        if (!pathOk)
        {
          #ifdef NSIS_SCRIPT
          AddError("bad path");
          #endif
          break;
        }

      #ifdef NSIS_SCRIPT

        bool altPathOk = true;

        const UInt32 altParam = params[3];
        if (altParam != 0)
        {
          altPathOk = false;
          UInt32 additional = 0;
          if (GetVarIndexFinished(altParam, '\\', additional) == kVar_INSTDIR)
            altPathOk = AreTwoParamStringsEqual(altParam + additional, params[0]);
        }

        AddParam(params[0]);

        /*
        for (int i = 1; i < 3; i++)
          AddParam_UInt(params[i]);
        */

        if (params[3] != 0)
        {
          SmallSpaceComment();
          AddParam(params[3]);
        }
        
        if (!altPathOk)
        {
          #ifdef NSIS_SCRIPT
          AddError("alt path error");
          #endif
        }

      #endif

        if (BadCmd >= 0 && BadCmd <= EW_WRITEUNINSTALLER)
        {
          /* We don't support cases with incorrect installer commands.
             Such bad installer item can break unpacking for other items. */
          #ifdef NSIS_SCRIPT
          AddError("SKIP possible BadCmd");
          #endif
          break;
        }

        CItem &item = Items.AddNew();

        SetItemName(item, params[0]);

        item.Pos = params[1];
        item.PatchSize = params[2];
        item.IsUninstaller = true;
        const UInt32 param3 = params[3];
        if (param3 != 0 && item.Prefix != -1)
        {
          /* (item.Prefix != -1) case means that param[0] path was not absolute.
              So we use params[3] in that case, as original nsis */
          SetItemName(item, param3);
        }
        /* UNINSTALLER file doesn't use directory prefixes.
           So we remove prefix: */
        item.Prefix = -1;
        
        /*
        // we can add second time to test the code
        CItem item2 = item;
        item2.NameU += L'2';
        item2.NameA += '2';
        Items.Add(item2);
        */

        break;
      }

      #ifdef NSIS_SCRIPT
      
      case EW_RET:
      {
        // bool needComment = false;
        if (onFuncIsOpen)
        {
          if (kkk == bh.Num - 1 || IsProbablyEndOfFunc(labels[kkk + 1]))
          {
            AddStringLF("FunctionEnd");

            if ((int)kkk + 1 == InitPluginsDir_End)
              CommentClose();
            AddLF();
            onFuncIsOpen = false;
            // needComment = true;
            break;
          }
        }
        // if (!needComment)
            if (IsSectionGroup)
              break;
          if (sectionIsOpen)
          {
            const CSection &sect = Sections[curSectionIndex];
            if (sect.StartCmdIndex + sect.NumCommands == kkk)
            {
              PrintSectionEnd();
              sectionIsOpen = false;
              curSectionIndex++;
              break;
            }

            // needComment = true;
            // break;
          }

        /*
        if (needComment)
          s += "  ;";
        */
        TabString("Return");
        AddLF();
        break;
      }

      case EW_NOP:
      {
        if (params[0] == 0)
          s += "Nop";
        else
        {
          s += "Goto";
          Add_GotoVar(params[0]);
        }
        break;
      }

      case EW_ABORT:
      {
        AddOptionalParam(params[0]);
        break;
      }

      case EW_CALL:
      {
        if (kkk + 1 < bh.Num && GetCmd(Get32(p + kCmdSize)) == EW_EXTRACTFILE)
        {
          UInt32 par1 = GET_CMD_PARAM(p + kCmdSize, 1);

          UInt32 pluginPar = 0;

          if (GetVarIndexFinished(par1, '\\', pluginPar) == kVar_PLUGINSDIR)
          {
            pluginPar += par1;
            UInt32 commandId2 = GetCmd(Get32(p + kCmdSize * 2));
            if (commandId2 == EW_SETFLAG || commandId2 == EW_UPDATETEXT)
            {
              UInt32 i;
              for (i = kkk + 3; i < bh.Num; i++)
              {
                const Byte *pCmd = p + kCmdSize * (i - kkk);
                UInt32 commandId3 = GetCmd(Get32(pCmd));
                if (commandId3 != EW_PUSHPOP
                    || GET_CMD_PARAM(pCmd, 1) != 0
                    || GET_CMD_PARAM(pCmd, 2) != 0)
                  break;
              }
              if (i < bh.Num)
              {
                const Byte *pCmd = p + kCmdSize * (i - kkk);

                // UInt32 callDll_Param = GET_CMD_PARAM(pCmd, 0);
                // UInt32 file_Param = GET_CMD_PARAM(p + kCmdSize, 1);

                if (GetCmd(Get32(pCmd)) == EW_REGISTERDLL &&
                    AreTwoParamStringsEqual(
                      GET_CMD_PARAM(pCmd, 0),
                      GET_CMD_PARAM(p + kCmdSize, 1)))
                {
                  // params[4] = 1 means GetModuleHandle attempt before default LoadLibraryEx;
                  /// new versions of NSIS use params[4] = 1 for Plugin command
                  if (GET_CMD_PARAM(pCmd, 2) == 0
                    // && GET_CMD_PARAM(pCmd, 4) != 0
                    )
                  {
                    {
                      AString s2;
                      ReadString2(s2, pluginPar);
                      if (s2.Len() >= 4 &&
                          StringsAreEqualNoCase_Ascii(s2.RightPtr(4), ".dll"))
                        s2.DeleteFrom(s2.Len() - 4);
                      s2 += "::";
                      AString func;
                      ReadString2(func, GET_CMD_PARAM(pCmd, 1));
                      s2 += func;
                      Add_QuStr(s2);

                      if (GET_CMD_PARAM(pCmd, 3) == 1)
                        s += " /NOUNLOAD";

                      for (UInt32 j = i - 1; j >= kkk + 3; j--)
                      {
                        const Byte *pCmd2 = p + kCmdSize * (j - kkk);
                        AddParam(GET_CMD_PARAM(pCmd2, 0));
                      }
                      NewLine();
                      Tab(true);
                      endCommentIndex = i + 1;
                    }
                  }
                }
              }
            }
          }
        }
        {
          const Byte *nextCmd = p + kCmdSize;
          UInt32 commandId2 = GetCmd(Get32(nextCmd));
          if (commandId2 == EW_SETFLAG
              && GET_CMD_PARAM(nextCmd, 0) == k_ExecFlags_DetailsPrint
              && GET_CMD_PARAM(nextCmd, 2) != 0) // is "lastused"
            // || commandId2 == EW_UPDATETEXT)
          {
            if ((Int32)params[0] > 0 && labels[params[0] - 1] & CMD_REF_InitPluginDir)
            {
              s += "InitPluginsDir";
              AddLF();
              Tab(true);
              endCommentIndex = kkk + 2;
            }
          }
        }
        
        s += "Call ";
        if ((Int32)params[0] < 0)
          Add_Var((UInt32)-((Int32)params[0] + 1));
        else if (params[0] == 0)
          s += '0';
        else
        {
          const UInt32 val = params[0] - 1;
          if (params[1] == 1) // it's Call :Label
          {
            s += ':';
            Add_LabelName(val);
          }
          else // if (params[1] == 0) // it's Call Func
            Add_FuncName(labels, val);
        }
        break;
      }

      case EW_UPDATETEXT:
      case EW_SLEEP:
      {
        AddParam(params[0]);
        break;
      }
      
      case EW_CHDETAILSVIEW:
      {
             if (params[0] == Z7_NSIS_WIN_SW_SHOWNA && params[1] == Z7_NSIS_WIN_SW_HIDE) s += " show";
        else if (params[1] == Z7_NSIS_WIN_SW_SHOWNA && params[0] == Z7_NSIS_WIN_SW_HIDE) s += " hide";
        else
          for (int i = 0; i < 2; i++)
          {
            Space();
            Add_ShowWindow_Cmd(params[i]);
          }
        break;
      }

      case EW_IFFILEEXISTS:
      {
        AddParam(params[0]);
        Add_GotoVars2(&params[1]);
        break;
      }

      case EW_SETFLAG:
      {
        AString temp;
        ReadString2(temp, params[1]);
        if (params[0] == k_ExecFlags_Errors && params[2] == 0)
        {
          s += (temp.Len() == 1 && temp[0] == '0') ? "ClearErrors" : "SetErrors";
          break;
        }
        s += "Set";
        Add_ExecFlags(params[0]);

        if (params[2] != 0)
        {
          s += " lastused";
          break;
        }
        UInt32 v;
        if (StringToUInt32(temp, v))
        {
          const char *s2 = NULL;
          switch (params[0])
          {
            case k_ExecFlags_AutoClose:
            case k_ExecFlags_RebootFlag:
              if (v < 2) { s2 = (v == 0) ? "false" : "true"; }  break;
            case k_ExecFlags_ShellVarContext:
              if (v < 2) { s2 = (v == 0) ? "current" : "all"; }  break;
            case k_ExecFlags_Silent:
              if (v < 2) { s2 = (v == 0) ? "normal" : "silent"; }  break;
            case k_ExecFlags_RegView:
                   if (v ==   0) s2 = "32";
              else if (v == 256) s2 = "64";
              break;
            case k_ExecFlags_DetailsPrint:
                   if (v == 0) s2 = "both";
              else if (v == 2) s2 = "textonly";
              else if (v == 4) s2 = "listonly";
              else if (v == 6) s2 = "none";
              break;
          }
          if (s2)
          {
            s += ' ';
            s += s2;
            break;
          }
        }
        SpaceQuStr(temp);
        break;
      }

      case EW_IFFLAG:
      {
        Add_ExecFlags(params[2]);
        Add_GotoVars2(&params[0]);
        /*
        const unsigned kIfErrors = 2;
        if (params[2] != kIfErrors && params[3] != 0xFFFFFFFF ||
            params[2] == kIfErrors && params[3] != 0)
        {
          s += " # FLAG &= ";
          AddParam_UInt(params[3]);
        }
        */
        break;
      }

      case EW_GETFLAG:
      {
        Add_ExecFlags(params[1]);
        AddParam_Var(params[0]);
        break;
      }

      case EW_RENAME:
      {
        if (params[2] != 0)
          s += k_REBOOTOK;
        AddParams(params, 2);
        if (params[3] != 0)
        {
          SmallSpaceComment();
          AddParam(params[3]); // rename comment for log file
        }
        break;
      }
      
      case EW_GETFULLPATHNAME:
      {
        if (params[2] == 0)
          s += " /SHORT";
        AddParam_Var(params[1]);
        AddParam(params[0]);
        break;
      }

      case EW_SEARCHPATH:
      case EW_STRLEN:
      {
        AddParam_Var(params[0]);
        AddParam(params[1]);
        break;
      }

      case EW_GETTEMPFILENAME:
      {
        AddParam_Var(params[0]);
        AString temp;
        ReadString2(temp, params[1]);
        if (!temp.IsEqualTo("$TEMP"))
          SpaceQuStr(temp);
        break;
      }

      case EW_DELETEFILE:
      {
        UInt32 flag = params[1];
        if ((flag & DEL_REBOOT) != 0)
          s += k_REBOOTOK;
        AddParam(params[0]);
        break;
      }

      case EW_MESSAGEBOX:
      {
        MessageBox_MB_Part(params[0]);
        AddParam(params[1]);
        {
          UInt32 buttonID = (params[0] >> 21); // NSIS 2.06+
          if (buttonID != 0)
          {
            s += " /SD";
            Add_ButtonID(buttonID);
          }
        }
        for (int i = 2; i < 6; i += 2)
          if (params[i] != 0)
          {
            Add_ButtonID(params[i]);
            Add_GotoVar1(params[i + 1]);
          }
        break;
      }

      case EW_RMDIR:
      {
        UInt32 flag = params[1];
        if ((flag & DEL_RECURSE) != 0)
          s += " /r";
        if ((flag & DEL_REBOOT) != 0)
          s += k_REBOOTOK;
        AddParam(params[0]);
        break;
      }

      case EW_STRCMP:
      {
        if (params[4] != 0)
          s += 'S';
        AddParams(params, 2);
        Add_GotoVars2(&params[2]);
        break;
      }

      case EW_READENVSTR:
      {
        s += (params[2] != 0) ?
          "ReadEnvStr" :
          "ExpandEnvStrings";
        AddParam_Var(params[0]);
        AString temp;
        ReadString2(temp, params[1]);
        if (params[2] != 0 &&temp.Len() >= 2 && temp[0] == '%' && temp.Back() == '%')
        {
          temp.DeleteBack();
          temp.Delete(0);
        }
        SpaceQuStr(temp);
        break;
      }

      case EW_INTCMP:
      {
        s += "Int";
        const UInt32 param5 = params[5];
        if (param5 & 0x8000)
          s += "64"; // v3.03+
        s += "Cmp";
        if (IsNsis3_OrHigher() ? (param5 & 1) : (param5 != 0))
          s += 'U';
        AddParams(params, 2);
        Add_GotoVar1(params[2]);
        if (params[3] != 0 || params[4] != 0)
          Add_GotoVars2(params + 3);
        break;
      }

      case EW_INTOP:
      {
        AddParam_Var(params[0]);
        const char * const kOps = "+-*/|&^!|&%<>>"; // NSIS 2.01+
                        // "+-*/|&^!|&%";   // NSIS 2.0b4+
                        // "+-*/|&^~!|&%";  // NSIS old
        const UInt32 opIndex = params[3];
        const char c = (opIndex < 14) ? kOps[opIndex] : '?';
        const char c2 = (opIndex < 8 || opIndex == 10) ? (char)0 : c;
        const int numOps = (opIndex == 7) ? 1 : 2;
        AddParam(params[1]);
        if (numOps == 2 && c == '^' && IsDirectString_Equal(params[2], "0xFFFFFFFF"))
          s += " ~    ;";
        Space();
        s += c;
        if (numOps != 1)
        {
          if (opIndex == 13) // v3.03+ : operation ">>>"
            s += c;
          if (c2 != 0)
            s += c2;
          AddParam(params[2]);
        }
        break;
      }

      case EW_INTFMT:
      {
        if (params[3])
          s += "Int64Fmt";  // v3.03+
        else
          s += "IntFmt";
        AddParam_Var(params[0]);
        AddParams(params + 1, 2);
        break;
      }

      case EW_PUSHPOP:
      {
        if (params[2] != 0)
        {
          s += "Exch";
          if (params[2] != 1)
            AddParam_UInt(params[2]);
        }
        else if (params[1] != 0)
        {
          s += "Pop";
          AddParam_Var(params[0]);
        }
        else
        {
          if (NoLabels(labels + kkk + 1, 2)
              && Get32(p + kCmdSize) == EW_PUSHPOP // Exch"
              && GET_CMD_PARAM(p + kCmdSize, 2) == 1
              && Get32(p + kCmdSize * 2) == EW_PUSHPOP // Pop $VAR
              && GET_CMD_PARAM(p + kCmdSize * 2, 1) != 0)
          {
            if (IsVarStr(params[0], GET_CMD_PARAM(p + kCmdSize * 2, 0)))
            {
              s += "Exch";
              AddParam(params[0]);
              NewLine();
              Tab(true);
              endCommentIndex = kkk + 3;
            }
          }
          s += "Push";
          AddParam(params[0]);
        }
        break;
      }

      case EW_FINDWINDOW:
      {
        AddParam_Var(params[0]);
        AddParam(params[1]);
        AddOptionalParams(params + 2, 3);
        break;
      }

      case EW_SENDMESSAGE:
      {
        // SendMessage: 6 [output, hwnd, msg, wparam, lparam, [wparamstring?1:0 | lparamstring?2:0 | timeout<<2]
        AddParam(params[1]);

        const char *w = NULL;
        AString t;
        ReadString2(t, params[2]);
        UInt32 wm;
        if (StringToUInt32(t, wm))
        {
          switch (wm)
          {
            case 0x0C: w = "SETTEXT"; break;
            case 0x10: w = "CLOSE"; break;
            case 0x30: w = "SETFONT"; break;
          }
        }
        if (w)
        {
          s += " ${WM_";
          s += w;
          s += '}';
        }
        else
          SpaceQuStr(t);
        
        UInt32 spec = params[5];
        for (unsigned i = 0; i < 2; i++)
        {
          AString s2;
          if (spec & ((UInt32)1 << i))
            s2 += "STR:";
          ReadString2(s2, params[3 + i]);
          SpaceQuStr(s2);
        }

        if ((Int32)params[0] >= 0)
          AddParam_Var(params[0]);

        spec >>= 2;
        if (spec != 0)
        {
          s += " /TIMEOUT=";
          Add_UInt(spec);
        }
        break;
      }

      case EW_ISWINDOW:
      {
        AddParam(params[0]);
        Add_GotoVars2(&params[1]);
        break;
      }

      case EW_GETDLGITEM:
      {
        AddParam_Var(params[0]);
        AddParams(params + 1, 2);
        break;
      }
      
      case EW_SETCTLCOLORS:
      {
        AddParam(params[0]);

        UInt32 offset = params[1];
        
        if (_size < bhCtlColors.Offset
           || _size - bhCtlColors.Offset < offset
           || _size - bhCtlColors.Offset - offset < GET_CtlColors_SIZE(Is64Bit))
        {
          AddError("bad offset");
          break;
        }

        const Byte *p2 = _data + bhCtlColors.Offset + offset;
        CNsis_CtlColors colors;
        colors.Parse(p2, Is64Bit);

        if ((colors.flags & kColorsFlags_BK_SYS) != 0 ||
            (colors.flags & kColorsFlags_TEXT_SYS) != 0)
          s += " /BRANDING";
        
        AString bk;
        bool bkc = false;
        if (colors.bkmode == Z7_NSIS_WIN_TRANSPARENT)
          bk += " transparent";
        else if (colors.flags & kColorsFlags_BKB)
        {
          if ((colors.flags & kColorsFlags_BK_SYS) == 0 &&
              (colors.flags & kColorsFlags_BK) != 0)
            bkc = true;
        }
        if ((colors.flags & kColorsFlags_TEXT) != 0 || !bk.IsEmpty() || bkc)
        {
          Space();
          if ((colors.flags & kColorsFlags_TEXT_SYS) != 0 || (colors.flags & kColorsFlags_TEXT) == 0)
            AddQuotes();
          else
            Add_Color(colors.text);
        }
        s += bk;
        if (bkc)
        {
          Space();
          Add_Color(colors.bkc);
        }

        break;
      }

      // case EW_LOADANDSETIMAGE:
      case EW_SETBRANDINGIMAGE:
      {
        s += " /IMGID=";
        Add_UInt(params[1]);
        if (params[2] == 1)
          s += " /RESIZETOFIT";
        AddParam(params[0]);
        break;
      }

      case EW_CREATEFONT:
      {
        AddParam_Var(params[0]);
        AddParam(params[1]);
        AddOptionalParams(params + 2, 2);
        if (params[4] & 1) s += " /ITALIC";
        if (params[4] & 2) s += " /UNDERLINE";
        if (params[4] & 4) s += " /STRIKE";
        break;
      }

      case EW_SHOWWINDOW:
      {
        AString hw, sw;
        ReadString2(hw, params[0]);
        ReadString2(sw, params[1]);
        if (params[3] != 0)
          s += "EnableWindow";
        else
        {
          UInt32 val;
          bool valDefined = false;
          if (StringToUInt32(sw, val))
          {
            if (val < Z7_ARRAY_SIZE(kShowWindow_Commands))
            {
              sw.Empty();
              sw += "${";
              Add_ShowWindow_Cmd_2(sw, val);
              sw += '}';
              valDefined = true;
            }
          }
          bool isHwndParent = IsVarStr(params[0], IsNsis225 ? kVar_HWNDPARENT_225 : kVar_HWNDPARENT);
          if (params[2] != 0)
          {
            if (valDefined && val == 0 && isHwndParent)
            {
              s += "HideWindow";
              break;
            }
          }
          if (valDefined && val == 5 && isHwndParent &&
              kkk + 1 < bh.Num && GetCmd(Get32(p + kCmdSize)) == EW_BRINGTOFRONT)
          {
            s += "  ; ";
          }
          s += "ShowWindow";
        }
        SpaceQuStr(hw);
        SpaceQuStr(sw);
        break;
      }

      case EW_SHELLEXEC:
      {
        AddParams(params, 2);
        if (params[2] != 0 || params[3] != Z7_NSIS_WIN_SW_SHOWNORMAL)
        {
          AddParam(params[2]);
          if (params[3] != Z7_NSIS_WIN_SW_SHOWNORMAL)
          {
            Space();
            Add_ShowWindow_Cmd(params[3]);
          }
        }
        if (params[5] != 0)
        {
          s += "    ;";
          AddParam(params[5]); // it's tatus text update
        }
        break;
      }

      case EW_EXECUTE:
      {
        if (params[2] != 0)
          s += "Wait";
        AddParam(params[0]);
        if (params[2] != 0)
          if ((Int32)params[1] >= 0)
            AddParam_Var(params[1]);
        break;
      }
      
      case EW_GETFILETIME:
      case EW_GETDLLVERSION:
      {
        if (commandId == EW_GETDLLVERSION)
          if (params[3] == 2)
            s += " /ProductVersion";  // v3.08+
        AddParam(params[2]);
        AddParam_Var(params[0]);
        AddParam_Var(params[1]);
        break;
      }

      case EW_REGISTERDLL:
      {
        AString func;
        ReadString2(func, params[1]);
        bool printFunc = true;
        // params[4] = 1; for plugin command
        if (params[2] == 0)
        {
          s += "CallInstDLL";
          AddParam(params[0]);
          if (params[3] == 1)
            s += " /NOUNLOAD";
        }
        else
        {
          if (func.IsEqualTo("DllUnregisterServer"))
          {
            s += "UnRegDLL";
            printFunc = false;
          }
          else
          {
            s += "RegDLL";
            if (func.IsEqualTo("DllRegisterServer"))
              printFunc = false;
          }
          AddParam(params[0]);
        }
        if (printFunc)
          SpaceQuStr(func);
        break;
      }

      case EW_CREATESHORTCUT:
      {
        unsigned numParams;
        #define IsNsis3d0b3_OrHigher() 0 // change it
        const unsigned v3_0b3 = IsNsis3d0b3_OrHigher();
        for (numParams = 6; numParams > 2; numParams--)
          if (params[numParams - 1] != 0)
            break;

        const UInt32 spec = params[4];
        const unsigned sw_shift = v3_0b3 ? 12 : 8;
        const UInt32 sw_mask = v3_0b3 ? 0x7000 : 0x7F;
        if (spec & 0x8000) // NSIS 3.0b0
          s += " /NoWorkingDir";

        AddParams(params, numParams > 4 ? 4 : numParams);
        if (numParams <= 4)
          break;

        UInt32 icon = (spec & (v3_0b3 ? 0xFFF : 0xFF));
        Space();
        if (icon != 0)
          Add_UInt(icon);
        else
          AddQuotes();

        if ((spec >> sw_shift) == 0 && numParams < 6)
          break;
        UInt32 sw = (spec >> sw_shift) & sw_mask;
        Space();
        // NSIS encoder replaces these names:
        if (sw == Z7_NSIS_WIN_SW_SHOWMINNOACTIVE)
          sw = Z7_NSIS_WIN_SW_SHOWMINIMIZED;
        if (sw == 0)
          AddQuotes();
        else
          Add_ShowWindow_Cmd(sw);
        
        UInt32 modKey = spec >> 24;
        UInt32 key = (spec >> 16) & 0xFF;

        if (modKey == 0 && key == 0)
        {
          if (numParams < 6)
            break;
          Space();
          AddQuotes();
        }
        else
        {
          Space();
          if (modKey & 1) s += "SHIFT|"; // HOTKEYF_SHIFT
          if (modKey & 2) s += "CONTROL|";
          if (modKey & 4) s += "ALT|";
          if (modKey & 8) s += "EXT|";
          
          const unsigned kMy_VK_F1 = 0x70;
          if (key >= kMy_VK_F1 && key <= kMy_VK_F1 + 23)
          {
            s += 'F';
            Add_UInt(key - kMy_VK_F1 + 1);
          }
          else if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9'))
            s += (char)key;
          else
          {
            s += "Char_";
            Add_UInt(key);
          }
        }
        AddOptionalParam(params[5]); // description
        break;
      }

      case EW_COPYFILES:
      {
        if (params[2] & 0x04) s += " /SILENT"; // FOF_SILENT
        if (params[2] & 0x80) s += " /FILESONLY"; // FOF_FILESONLY
        AddParams(params, 2);
        if (params[3] != 0)
        {
          s += "    ;";
          AddParam(params[3]); // status text update
        }
        break;
      }

      case EW_REBOOT:
      {
        if (params[0] != 0xbadf00d)
          s += " ; Corrupted ???";
        else if (kkk + 1 < bh.Num && GetCmd(Get32(p + kCmdSize)) == EW_QUIT)
          endCommentIndex = kkk + 2;
        break;
      }

      case EW_WRITEINI:
      {
        unsigned numAlwaysParams = 0;
        if (params[0] == 0)  // Section
          s += "FlushINI";
        else if (params[4] != 0)
        {
          s += "WriteINIStr";
          numAlwaysParams = 3;
        }
        else
        {
          s += "DeleteINI";
          s += (params[1] == 0) ? "Sec" : "Str";
          numAlwaysParams = 1;
        }
        AddParam(params[3]); // filename
        // Section, EntryName, Value
        AddParams(params, numAlwaysParams);
        AddOptionalParams(params + numAlwaysParams, 3 - numAlwaysParams);
        break;
      }

      case EW_READINISTR:
      {
        AddParam_Var(params[0]);
        AddParam(params[3]); // FileName
        AddParams(params +1, 2); // Section, EntryName
        break;
      }

      case EW_DELREG:
      {
        // NSIS 2.00 used another scheme!
        
        if (params[4] == 0)
          s += "Value";
        else
        {
          s += "Key";
          if (params[4] & 2)
            s += " /ifempty";
          // TODO: /ifnosubkeys, /ifnovalues
        }
        AddRegRoot(params[1]);
        AddParam(params[2]);
        AddOptionalParam(params[3]);
        break;
      }

      case EW_WRITEREG:
      {
        const char *s2 = NULL;
        switch (params[4])
        {
          case 1: s2 = "Str"; break;
          case 2: s2 = "ExpandStr"; break; // maybe unused
          case 3: s2 = "Bin"; break;
          case 4: s2 = "DWORD"; break;
          default:
            s += '?';
            Add_UInt(params[4]);
        }
        if (params[4] == 1 && params[5] == 2)
          s2 = "ExpandStr";
        if (params[4] == 3 && params[5] == 7)
          s2 = "MultiStr"; // v3.02+
        if (s2)
          s += s2;
        AddRegRoot(params[0]);
        AddParams(params + 1, 2); // keyName, valueName
        if (params[4] != 3)
          AddParam(params[3]); // value
        else
        {
          // Binary data.
          Space();
          UInt32 offset = params[3];
          bool isSupported = false;
          if (AfterHeaderSize >= 4
              && bhData.Offset <= AfterHeaderSize - 4
              && offset <= AfterHeaderSize - 4 - bhData.Offset)
          {
            // we support it for solid archives.
            const Byte *p2 = _afterHeader + bhData.Offset + offset;
            UInt32 size = Get32(p2);
            if (size <= AfterHeaderSize - 4 - bhData.Offset - offset)
            {
              for (UInt32 i = 0; i < size; i++)
              {
                Byte b = (p2 + 4)[i];
                unsigned t;
                t = (b >> 4); s += (char)(((t < 10) ? ('0' + t) : ('A' + (t - 10))));
                t = (b & 15); s += (char)(((t < 10) ? ('0' + t) : ('A' + (t - 10))));
              }
              isSupported = true;
            }
          }
          if (!isSupported)
          {
            // we must read from file here;
            s += "data[";
            Add_UInt(offset);
            s += " ... ]";
            s += "  ; !!! Unsupported";
          }
        }
        break;
      }

      case EW_READREGSTR:
      {
        s += (params[4] == 1) ? "DWORD" : "Str";
        AddParam_Var(params[0]);
        AddRegRoot(params[1]);
        AddParams(params + 2, 2);
        break;
      }

      case EW_REGENUM:
      {
        s += (params[4] != 0) ? "Key" : "Value";
        AddParam_Var(params[0]);
        AddRegRoot(params[1]);
        AddParams(params + 2, 2);
        break;
      }

      case EW_FCLOSE:
      case EW_FINDCLOSE:
      {
        AddParam_Var(params[0]);
        break;
      }

    #endif

      case EW_FOPEN:
      {
        /*
          the pattern for empty files is following:
          FileOpen $0 "file_name" w
          FileClose $0
        */

        const UInt32 acc = params[1]; // dwDesiredAccess
        const UInt32 creat = params[2]; // dwCreationDisposition
        if (creat == Z7_NSIS_WIN_CREATE_ALWAYS && acc == Z7_NSIS_WIN_GENERIC_WRITE)
        {
          if (kkk + 1 < bh.Num)
          if (Get32(p + kCmdSize) == EW_FCLOSE)
          if (Get32(p + kCmdSize + 4) == params[0])
          {
            CItem &item = Items.AddNew();
            item.IsEmptyFile = true;
            SetItemName(item, params[3]);
          }
        }

      #ifdef NSIS_SCRIPT

        AddParam_Var(params[0]);
        AddParam(params[3]);
        if (acc == 0 && creat == 0)
          break;
        char cc = 0;
             if (creat == Z7_NSIS_WIN_OPEN_EXISTING && acc == Z7_NSIS_WIN_GENERIC_READ)
          cc = 'r';
        else if (creat == Z7_NSIS_WIN_CREATE_ALWAYS && acc == Z7_NSIS_WIN_GENERIC_WRITE)
          cc = 'w';
        else if (creat == Z7_NSIS_WIN_OPEN_ALWAYS && (acc == (Z7_NSIS_WIN_GENERIC_WRITE | Z7_NSIS_WIN_GENERIC_READ)))
          cc = 'a';
        // cc = 0;
        if (cc != 0)
        {
          Space();
          s += cc;
          break;
        }

        if (acc & Z7_NSIS_WIN_GENERIC_READ)     s += " GENERIC_READ";
        if (acc & Z7_NSIS_WIN_GENERIC_WRITE)    s += " GENERIC_WRITE";
        if (acc & Z7_NSIS_WIN_GENERIC_EXECUTE)  s += " GENERIC_EXECUTE";
        if (acc & Z7_NSIS_WIN_GENERIC_ALL)      s += " GENERIC_ALL";
        
        const char *s2 = NULL;
        switch (creat)
        {
          case Z7_NSIS_WIN_CREATE_NEW:        s2 = "CREATE_NEW"; break;
          case Z7_NSIS_WIN_CREATE_ALWAYS:     s2 = "CREATE_ALWAYS"; break;
          case Z7_NSIS_WIN_OPEN_EXISTING:     s2 = "OPEN_EXISTING"; break;
          case Z7_NSIS_WIN_OPEN_ALWAYS:       s2 = "OPEN_ALWAYS"; break;
          case Z7_NSIS_WIN_TRUNCATE_EXISTING: s2 = "TRUNCATE_EXISTING"; break;
        }
        Space();
        if (s2)
          s += s2;
        else
          Add_UInt(creat);
      #endif

        break;
      }

    #ifdef NSIS_SCRIPT
      
      case EW_FPUTS:
      case EW_FPUTWS:
      {
        if (commandId == EW_FPUTWS)
          s += (params[2] == 0) ? "UTF16LE" : "Word";
        else if (params[2] != 0)
          s += "Byte";
        if (params[2] == 0 && params[3])
          s += " /BOM"; // v3.0b3+
        AddParam_Var(params[0]);
        AddParam(params[1]);
        break;
      }

      case EW_FGETS:
      case EW_FGETWS:
      {
        if (commandId == EW_FPUTWS)
          s += (params[3] == 0) ? "UTF16LE" : "Word";
        if (params[3] != 0)
          s += "Byte";
        AddParam_Var(params[0]);
        AddParam_Var(params[1]);
        AString maxLenStr;
        ReadString2(maxLenStr, params[2]);
        UInt32 maxLen;
        if (StringToUInt32(maxLenStr, maxLen))
        {
          if (maxLen == 1 && params[3] != 0)
            break;
          if (maxLen == 1023 && params[3] == 0) // NSIS_MAX_STRLEN - 1; can be other value!!
            break;
        }
        SpaceQuStr(maxLenStr);
        break;
      }

      case EW_FSEEK:
      {
        AddParam_Var(params[0]);
        AddParam(params[2]);
        if (params[3] == 1) s += " CUR"; // FILE_CURRENT
        if (params[3] == 2) s += " END"; // FILE_END
        if ((Int32)params[1] >= 0)
        {
          if (params[3] == 0) s += " SET"; // FILE_BEGIN
          AddParam_Var(params[1]);
        }
        break;
      }

      case EW_FINDNEXT:
      {
        AddParam_Var(params[1]);
        AddParam_Var(params[0]);
        break;
      }

      case EW_FINDFIRST:
      {
        AddParam_Var(params[1]);
        AddParam_Var(params[0]);
        AddParam(params[2]);
        break;
      }

      case EW_LOG:
      {
        if (params[0] != 0)
        {
          s += "Set ";
          s += (params[1] == 0) ? "off" : "on";
        }
        else
        {
          s += "Text";
          AddParam(params[1]);
        }
        break;
      }

      case EW_SECTIONSET:
      {
        if ((Int32)params[2] >= 0)
        {
          s += "Get";
          Add_SectOp(params[2]);
          AddParam(params[0]);
          AddParam_Var(params[1]);
        }
        else
        {
          s += "Set";
          const UInt32 t = (UInt32)(-(Int32)params[2] - 1);
          Add_SectOp(t);
          AddParam(params[0]);
          AddParam(params[t == 0 ? 4 : 1]);

          // params[3] != 0 means call SectionFlagsChanged in installer
          // used by SECTIONSETFLAGS command
        }
        break;
      }

      case EW_INSTTYPESET:
      {
        unsigned numQwParams = 0;
        const char *s2;
        if (params[3] == 0)
        {
          if (params[2] == 0)
          {
            s2 = "InstTypeGetText";
            numQwParams = 1;
          }
          else
          {
            s2 = "InstTypeSetText";
            numQwParams = 2;
          }
        }
        else
        {
          if (params[2] == 0)
            s2 = "GetCurInstType";
          else
          {
            s2 = "SetCurInstType";
            numQwParams = 1;
          }
        }
        s += s2;
        AddParams(params, numQwParams);
        if (params[2] == 0)
          AddParam_Var(params[1]);
        break;
      }

      case EW_GETOSINFO:
      {
        if (IsNsis3_OrHigher())
        {
          // v3.06+
          if (params[3] == 0) // GETOSINFO_KNOWNFOLDER
          {
            s += "GetKnownFolderPath";
            AddParam_Var(params[1]);
            AddParam(params[2]);
            break;
          }
          else if (params[3] == 1) // GETOSINFO_READMEMORY
          {
            s += "ReadMemory";
            AddParam_Var(params[1]);
            AddParam(params[2]);
            AddParam(params[4]);
            // if (params[2].IsEqualTo("0")) AddCommentAndString("GetWinVer");
          }
          else
            s += "GetOsInfo";
          break;
        }
        s += "GetLabelAddr"; //  before v3.06+
        break;
      }
      
      case EW_LOCKWINDOW:
      {
        s += (params[0] == 0) ? " on" : " off";
        break;
      }

      case EW_FINDPROC:
      {
        AddParam_Var(params[0]);
        AddParam(params[1]);
        break;
      }

      default:
      {
        numSkipParams = 0;
      }
      #endif
    }
    
    #ifdef NSIS_SCRIPT

    unsigned numParams = kNumCommandParams;

    for (; numParams > 0; numParams--)
      if (params[numParams - 1] != 0)
        break;

    if (numParams > numSkipParams)
    {
      s += " ; !!!! Unknown Params: ";
      unsigned i;
      for (i = 0; i < numParams; i++)
        AddParam(params[i]);

      s += "   ;";
      
      for (i = 0; i < numParams; i++)
      {
        Space();
        UInt32 v = params[i];
        if (v > 0xFFF00000)
          Add_SignedInt(s, (Int32)v);
        else
          Add_UInt(v);
      }
    }

    NewLine();
    
    #endif
  }

  #ifdef NSIS_SCRIPT

  if (sectionIsOpen)
  {
    if (curSectionIndex < bhSections.Num)
    {
      const CSection &sect = Sections[curSectionIndex];
      if (sect.StartCmdIndex + sect.NumCommands + 1 == kkk)
      {
        PrintSectionEnd();
        sectionIsOpen = false;
        // lastSectionEndCmd = kkk;
        curSectionIndex++;
      }
    }
  }
  
  while (curSectionIndex < bhSections.Num)
  {
    const CSection &sect = Sections[curSectionIndex];
    if (sectionIsOpen)
    {
      if (sect.StartCmdIndex + sect.NumCommands != kkk)
        AddErrorLF("SECTION ERROR");
      PrintSectionEnd();
      sectionIsOpen = false;
      curSectionIndex++;
    }
    else
    {
      if (PrintSectionBegin(sect, curSectionIndex))
        curSectionIndex++;
      else
        sectionIsOpen = true;
    }
  }
  
  #endif

  return S_OK;
}

static int CompareItems(void *const *p1, void *const *p2, void *param)
{
  const CItem &i1 = **(const CItem *const *)p1;
  const CItem &i2 = **(const CItem *const *)p2;
  RINOZ(MyCompare(i1.Pos, i2.Pos))

  /* In another code we check CItem::Pos after each solid item.
     So here we place empty files before all non empty files */
  if (i1.IsEmptyFile)
  {
    if (!i2.IsEmptyFile)
      return -1;
  }
  else if (i2.IsEmptyFile)
    return 1;

  const CInArchive *inArchive = (const CInArchive *)param;
  if (inArchive->IsUnicode)
  {
    if (i1.Prefix != i2.Prefix)
    {
      if (i1.Prefix < 0) return -1;
      if (i2.Prefix < 0) return 1;
      RINOZ(
          inArchive->UPrefixes[i1.Prefix].Compare(
          inArchive->UPrefixes[i2.Prefix]))
    }
    RINOZ(i1.NameU.Compare(i2.NameU))
  }
  else
  {
    if (i1.Prefix != i2.Prefix)
    {
      if (i1.Prefix < 0) return -1;
      if (i2.Prefix < 0) return 1;
      RINOZ(strcmp(
          inArchive->APrefixes[i1.Prefix],
          inArchive->APrefixes[i2.Prefix]))
    }
    RINOZ(strcmp(i1.NameA, i2.NameA))
  }
  return 0;
}

HRESULT CInArchive::SortItems()
{
  {
    Items.Sort(CompareItems, (void *)this);
    unsigned i;

    for (i = 0; i + 1 < Items.Size(); i++)
    {
      const CItem &i1 = Items[i];
      if (i1.IsEmptyFile)
        continue;
      const CItem &i2 = Items[i + 1];
      if (i1.Pos != i2.Pos)
        continue;

      if (IsUnicode)
      {
        if (i1.NameU != i2.NameU) continue;
        if (i1.Prefix != i2.Prefix)
        {
          if (i1.Prefix < 0 || i2.Prefix < 0) continue;
          if (UPrefixes[i1.Prefix] != UPrefixes[i2.Prefix]) continue;
        }
      }
      else
      {
        if (i1.NameA != i2.NameA) continue;
        if (i1.Prefix != i2.Prefix)
        {
          if (i1.Prefix < 0 || i2.Prefix < 0) continue;
          if (APrefixes[i1.Prefix] != APrefixes[i2.Prefix]) continue;
        }
      }
      Items.Delete(i + 1);
      i--;
    }
    
    for (i = 0; i < Items.Size(); i++)
    {
      CItem &item = Items[i];
      if (item.IsEmptyFile)
        continue;
      const UInt32 curPos = item.Pos + 4;
      for (unsigned nextIndex = i + 1; nextIndex < Items.Size(); nextIndex++)
      {
        const CItem &nextItem = Items[nextIndex];
        // if (nextItem.IsEmptyFile) continue;
        const UInt32 nextPos = nextItem.Pos;
        if (curPos <= nextPos)
        {
          item.EstimatedSize_Defined = true;
          item.EstimatedSize = nextPos - curPos;
          break;
        }
      }
    }
    
    if (!IsSolid)
    {
      for (i = 0; i < Items.Size(); i++)
      {
        CItem &item = Items[i];
        if (item.IsEmptyFile)
          continue;
        RINOK(SeekToNonSolidItem(i))
        const UInt32 kSigSize = 4 + 1 + 1 + 4; // size,[flag],prop,dict
        BYTE sig[kSigSize];
        size_t processedSize = kSigSize;
        RINOK(ReadStream(_stream, sig, &processedSize))
        if (processedSize < 4)
          return S_FALSE;
        UInt32 size = Get32(sig);
        if ((size & kMask_IsCompressed) != 0)
        {
          item.IsCompressed = true;
          size &= ~kMask_IsCompressed;
          if (Method == NMethodType::kLZMA)
          {
            if (processedSize < 9)
              return S_FALSE;
            /*
            if (FilterFlag)
              item.UseFilter = (sig[4] != 0);
            */
            item.DictionarySize = Get32(sig + 4 + 1 + (FilterFlag ? 1 : 0));
          }
        }
        else
        {
          item.IsCompressed = false;
          item.Size = size;
          item.Size_Defined = true;
        }
        item.CompressedSize = size;
        item.CompressedSize_Defined = true;
      }
    }
  }
  return S_OK;
}

#ifdef NSIS_SCRIPT
// Flags for common_header.flags
// #define CH_FLAGS_DETAILS_SHOWDETAILS 1
// #define CH_FLAGS_DETAILS_NEVERSHOW 2
#define CH_FLAGS_PROGRESS_COLORED 4
#define CH_FLAGS_SILENT 8
#define CH_FLAGS_SILENT_LOG 16
#define CH_FLAGS_AUTO_CLOSE 32
// #define CH_FLAGS_DIR_NO_SHOW 64  // unused now
#define CH_FLAGS_NO_ROOT_DIR 128
#define CH_FLAGS_COMP_ONLY_ON_CUSTOM 256
#define CH_FLAGS_NO_CUSTOM 512

static const char * const k_PostStrings[] =
{
    "install_directory_auto_append"
  , "uninstchild"     // NSIS 2.25+, used by uninstaller:
  , "uninstcmd"       // NSIS 2.25+, used by uninstaller:
  , "wininit"         // NSIS 2.25+, used by move file on reboot
};
#endif


void CBlockHeader::Parse(const Byte *p, unsigned bhoSize)
{
  if (bhoSize == 12)
  {
    // UInt64 a = GetUi64(p);
    if (GetUi32(p + 4) != 0)
      throw 1;
  }
  Offset = GetUi32(p);
  Num = GetUi32(p + bhoSize - 4);
}

#define PARSE_BH(k, bh) bh.Parse (p1 + 4 + bhoSize * k, bhoSize)


HRESULT CInArchive::Parse()
{
  // UInt32 offset = ReadUInt32();
  // ???? offset == FirstHeader.HeaderSize
  const Byte * const p1 = _data;

  if (_size < 4 + 12 * 8)
    Is64Bit = false;
  else
  {
    Is64Bit = true;
    // here we test high 32-bit of possible UInt64 CBlockHeader::Offset field
    for (int k = 0; k < 8; k++)
      if (GetUi32(p1 + 4 + 12 * k + 4) != 0)
        Is64Bit = false;
  }

  const unsigned bhoSize = Is64Bit ? 12 : 8;
  if (_size < 4 + bhoSize * 8)
    return S_FALSE;

  CBlockHeader bhEntries, bhStrings, bhLangTables;

  PARSE_BH (2, bhEntries);
  PARSE_BH (3, bhStrings);
  PARSE_BH (4, bhLangTables);

  #ifdef NSIS_SCRIPT

  CBlockHeader bhFont;
  PARSE_BH (0, bhPages);
  PARSE_BH (1, bhSections);
  PARSE_BH (5, bhCtlColors);
  PARSE_BH (6, bhFont);
  PARSE_BH (7, bhData);

  #endif

  _stringsPos = bhStrings.Offset;
  if (_stringsPos > _size
      || bhLangTables.Offset > _size
      || bhEntries.Offset > _size)
    return S_FALSE;
  {
    if (bhLangTables.Offset < bhStrings.Offset)
      return S_FALSE;
    const UInt32 stringTableSize = bhLangTables.Offset - bhStrings.Offset;
    if (stringTableSize < 2)
      return S_FALSE;
    const Byte *strData = _data + _stringsPos;
    if (strData[stringTableSize - 1] != 0)
      return S_FALSE;
    IsUnicode = (Get16(strData) == 0);
    NumStringChars = stringTableSize;
    if (IsUnicode)
    {
      if ((stringTableSize & 1) != 0)
        return S_FALSE;
      NumStringChars >>= 1;
      if (strData[stringTableSize - 2] != 0)
        return S_FALSE;
    }
  }

  if (bhEntries.Num > (1 << 25))
    return S_FALSE;
  if (bhEntries.Num * kCmdSize > _size - bhEntries.Offset)
    return S_FALSE;

  DetectNsisType(bhEntries, _data + bhEntries.Offset);

  Decoder.IsNsisDeflate = (NsisType != k_NsisType_Nsis3);
  
  // some NSIS files (that are not detected as k_NsisType_Nsis3)
  // use original (non-NSIS) Deflate
  // How to detect these cases?

  // Decoder.IsNsisDeflate = false;


  #ifdef NSIS_SCRIPT

  {
    AddCommentAndString("NSIS script");
    if (IsUnicode)
      Script += " (UTF-8)";
    Space();
    Script += GetFormatDescription();
    AddLF();
  }
  {
    AddCommentAndString(IsInstaller ? "Install" : "Uninstall");
    AddLF();
  }

  AddLF();
  if (Is64Bit)
    AddStringLF("Target AMD64-Unicode"); // TODO: Read PE machine type and use the correct CPU type
  else if (IsUnicode)
    AddStringLF("Unicode true");
  else if (IsNsis3_OrHigher())
    AddStringLF("Unicode false"); // Unicode is the default in 3.07+

  if (Method != NMethodType::kCopy)
  {
    const char *m = NULL;
    switch ((int)Method)
    {
      case NMethodType::kDeflate: m = "zlib"; break;
      case NMethodType::kBZip2: m = "bzip2"; break;
      case NMethodType::kLZMA: m = "lzma"; break;
      case NMethodType::kZstd: m = "zstd"; break;
      default: break;
    }
    Script += "SetCompressor";
    if (IsSolid)
      Script += " /SOLID";
    if (m)
    {
      Space();
      Script += m;
    }
    AddLF();
  }
  if (Method == NMethodType::kLZMA)
  {
    // if (DictionarySize != (8 << 20))
    {
      Script += "SetCompressorDictSize";
      AddParam_UInt(DictionarySize >> 20);
      AddLF();
    }
  }

  Separator();
  PrintNumComment("HEADER SIZE", FirstHeader.HeaderSize);
  // if (bhPages.Offset != 300 && bhPages.Offset != 288)
  if (bhPages.Offset != 0)
  {
    PrintNumComment("START HEADER SIZE", bhPages.Offset);
  }

  if (bhSections.Num > 0)
  {
    if (bhEntries.Offset < bhSections.Offset)
      return S_FALSE;
    SectionSize = (bhEntries.Offset - bhSections.Offset) / bhSections.Num;
    if (bhSections.Offset + bhSections.Num * SectionSize != bhEntries.Offset)
      return S_FALSE;
    if (SectionSize < kSectionSize_base)
      return S_FALSE;
    UInt32 maxStringLen = SectionSize - kSectionSize_base;
    if (IsUnicode)
    {
      if ((maxStringLen & 1) != 0)
        return S_FALSE;
      maxStringLen >>= 1;
    }
    // if (maxStringLen != 1024)
    {
      if (maxStringLen == 0)
        PrintNumComment("SECTION SIZE", SectionSize);
      else
        PrintNumComment("MAX STRING LENGTH", maxStringLen);
    }
  }
  
  PrintNumComment("STRING CHARS", NumStringChars);
  // PrintNumComment("LANG TABLE SIZE", bhCtlColors.Offset - bhLangTables.Offset);
  
  if (bhCtlColors.Offset > _size)
    AddErrorLF("Bad COLORS TABLE");
  // PrintNumComment("COLORS TABLE SIZE", bhFont.Offset - bhCtlColors.Offset);
  if (bhCtlColors.Num != 0)
    PrintNumComment("COLORS Num", bhCtlColors.Num);

  // bhData uses offset in _afterHeader (not in _data)
  // PrintNumComment("FONT TABLE SIZE", bhData.Offset - bhFont.Offset);
  if (bhFont.Num != 0)
    PrintNumComment("FONTS Num", bhFont.Num);

  // PrintNumComment("DATA SIZE", FirstHeader.HeaderSize - bhData.Offset);
  if (bhData.Num != 0)
    PrintNumComment("DATA NUM", bhData.Num);

  AddLF();

  AddStringLF("OutFile [NSIS].exe");
  AddStringLF("!include WinMessages.nsh");

  AddLF();

  strUsed.Alloc(NumStringChars);
  memset(strUsed, 0, NumStringChars);

  {
    UInt32 ehFlags = Get32(p1);
    UInt32 showDetails = ehFlags & 3;// CH_FLAGS_DETAILS_SHOWDETAILS & CH_FLAGS_DETAILS_NEVERSHOW;
    if (showDetails >= 1 && showDetails <= 2)
    {
      Script += IsInstaller ? "ShowInstDetails" : "ShowUninstDetails";
      Script += (showDetails == 1) ? " show" : " nevershow";
      AddLF();
    }
    if (ehFlags & CH_FLAGS_PROGRESS_COLORED) AddStringLF("InstProgressFlags colored" );
    if ((ehFlags & (CH_FLAGS_SILENT | CH_FLAGS_SILENT_LOG)) != 0)
    {
      Script += IsInstaller ? "SilentInstall " : "SilentUnInstall ";
      Script += (ehFlags & CH_FLAGS_SILENT_LOG) ? "silentlog" : "silent";
      AddLF();
    }
    if (ehFlags & CH_FLAGS_AUTO_CLOSE) AddStringLF("AutoCloseWindow true");
    if ((ehFlags & CH_FLAGS_NO_ROOT_DIR) == 0) AddStringLF("AllowRootDirInstall true");
    if (ehFlags & CH_FLAGS_NO_CUSTOM) AddStringLF("InstType /NOCUSTOM");
    if (ehFlags & CH_FLAGS_COMP_ONLY_ON_CUSTOM) AddStringLF("InstType /COMPONENTSONLYONCUSTOM");
  }

  // Separator();
  // AddLF();

  Int32 licenseLangIndex = -1;
  {
    const Byte *pp = _data + bhPages.Offset;

    for (UInt32 pageIndex = 0; pageIndex < bhPages.Num; pageIndex++, pp += kPageSize)
    {
      UInt32 wndProcID = Get32(pp + 4);
      UInt32 param1 = Get32(pp + 44 + 4 * 1);
      if (wndProcID != PWP_LICENSE || param1 == 0)
        continue;
      if ((Int32)param1 < 0)
        licenseLangIndex = - ((Int32)param1 + 1);
      else
        noParseStringIndexes.AddToUniqueSorted(param1);
    }
  }

  unsigned paramsOffset;
  {
    unsigned numBhs = 8;
    // probably its for old NSIS?
    if (bhoSize == 8 && bhPages.Offset == 276)
      numBhs = 7;
    paramsOffset = 4 + bhoSize * numBhs;
  }

  const Byte *p2 = p1 + paramsOffset;

  {
    UInt32 rootKey = Get32(p2); // (rootKey = -1) in uninstaller by default (the bug in NSIS)
    UInt32 subKey = Get32(p2 + 4);
    UInt32 value = Get32(p2 + 8);
    if ((rootKey != 0 && rootKey != (UInt32)(Int32)-1) || subKey != 0 || value != 0)
    {
      Script += "InstallDirRegKey";
      AddRegRoot(rootKey);
      AddParam(subKey);
      AddParam(value);
      AddLF();
    }
  }


  {
    UInt32 bg_color1 = Get32(p2 + 12);
    UInt32 bg_color2 = Get32(p2 + 16);
    UInt32 bg_textcolor = Get32(p2 + 20);
    if (bg_color1 != (UInt32)(Int32)-1 || bg_color2 != (UInt32)(Int32)-1 || bg_textcolor != (UInt32)(Int32)-1)
    {
      Script += "BGGradient";
      if (bg_color1 != 0 || bg_color2 != 0xFF0000 || bg_textcolor != (UInt32)(Int32)-1)
      {
        Add_ColorParam(bg_color1);
        Add_ColorParam(bg_color2);
        if (bg_textcolor != (UInt32)(Int32)-1)
          Add_ColorParam(bg_textcolor);
      }
      AddLF();
    }
  }

  {
    UInt32 lb_bg = Get32(p2 + 24);
    UInt32 lb_fg = Get32(p2 + 28);
    if ((lb_bg != (UInt32)(Int32)-1 || lb_fg != (UInt32)(Int32)-1) &&
      (lb_bg != 0 || lb_fg != 0xFF00))
    {
      Script += "InstallColors";
      Add_ColorParam(lb_fg);
      Add_ColorParam(lb_bg);
      AddLF();
    }
  }

  UInt32 license_bg = Get32(p2 + 36);
  if (license_bg != (UInt32)(Int32)-1 &&
      license_bg != (UInt32)(Int32)-15) // COLOR_BTNFACE
  {
    Script += "LicenseBkColor";
    if ((Int32)license_bg == -5)  // COLOR_WINDOW
      Script += " /windows";
    /*
    else if ((Int32)license_bg == -15)
      Script += " /grey";
    */
    else
      Add_ColorParam(license_bg);
    AddLF();
  }

  if (bhLangTables.Num > 0)
  {
    const UInt32 langtable_size = Get32(p2 + 32);

    if (langtable_size == (UInt32)(Int32)-1)
      return E_NOTIMPL; // maybe it's old NSIS archive()

    if (langtable_size < 10)
      return S_FALSE;
    if (bhLangTables.Num > (_size - bhLangTables.Offset) / langtable_size)
      return S_FALSE;

    const UInt32 numStrings = (langtable_size - 10) / 4;
    _numLangStrings = numStrings;
    AddLF();
    Separator();
    PrintNumComment("LANG TABLES", bhLangTables.Num);
    PrintNumComment("LANG STRINGS", numStrings);
    AddLF();

    if (licenseLangIndex >= 0 && (unsigned)licenseLangIndex < numStrings)
    {
      for (UInt32 i = 0; i < bhLangTables.Num; i++)
      {
        const Byte * const p = _data + bhLangTables.Offset + langtable_size * i;
        const UInt16 langID = Get16(p);
        UInt32 val = Get32(p + 10 + (UInt32)licenseLangIndex * 4);
        if (val != 0)
        {
          Script += "LicenseLangString ";
          Add_LangStr_Simple((UInt32)licenseLangIndex);
          AddParam_UInt(langID);
          AddLicense(val, langID);
          noParseStringIndexes.AddToUniqueSorted(val);
          NewLine();
        }
      }
      AddLF();
    }
    
    UInt32 names[3] = { 0 };

    UInt32 i;
    for (i = 0; i < bhLangTables.Num; i++)
    {
      const Byte * const p = _data + bhLangTables.Offset + langtable_size * i;
      const UInt16 langID = Get16(p);
      if (i == 0 || langID == 1033)
        _mainLang = p + 10;
      for (unsigned k = 0; k < Z7_ARRAY_SIZE(names) && k < numStrings; k++)
      {
        UInt32 v = Get32(p + 10 + k * 4);
        if (v != 0 && (langID == 1033 || names[k] == 0))
          names[k] = v;
      }
    }

    const UInt32 name = names[2];
    if (name != 0)
    {
      Script += "Name";
      AddParam(name);
      NewLine();

      ReadString2(Name, name);
    }
    
    /*
    const UInt32 caption = names[1];
    if (caption != 0)
    {
      Script += "Caption";
      AddParam(caption);
      NewLine();
    }
    */
    
    const UInt32 brandingText = names[0];
    if (brandingText != 0)
    {
      Script += "BrandingText";
      AddParam(brandingText);
      NewLine();

      ReadString2(BrandingText, brandingText);
    }

    for (i = 0; i < bhLangTables.Num; i++)
    {
      const Byte * const p = _data + bhLangTables.Offset + langtable_size * i;
      const UInt16 langID = Get16(p);
      
      AddLF();
      AddCommentAndString("LANG:");
      AddParam_UInt(langID);
      /*
      Script += " (";
      LangId_To_String(Script, langID);
      Script += ')';
      */
      AddLF();
      // UInt32 dlg_offset = Get32(p + 2);
      // UInt32 g_exec_flags_rtl = Get32(p + 6);
      
      
      for (UInt32 j = 0; j < numStrings; j++)
      {
        UInt32 val = Get32(p + 10 + j * 4);
        if (val != 0)
        {
          if ((Int32)j != licenseLangIndex)
          {
            Script += "LangString ";
            Add_LangStr_Simple(j);
            AddParam_UInt(langID);
            AddParam(val);
            AddLF();
          }
        }
      }
      AddLF();
    }
    ClearLangComment();
  }

  {
    unsigned numInternalVars = GET_NUM_INTERNAL_VARS;
    UInt32 numUsedVars = GetNumUsedVars();
    if (numUsedVars > numInternalVars)
    {
      Separator();
      PrintNumComment("VARIABLES", numUsedVars - numInternalVars);
      AddLF();
      AString temp;
      for (UInt32 i = numInternalVars; i < numUsedVars; i++)
      {
        Script += "Var ";
        temp.Empty();
        GetVar2(temp, i);
        AddStringLF(temp);
      }
      AddLF();
    }
  }

  onFuncOffset = paramsOffset + 40;
  numOnFunc = Z7_ARRAY_SIZE(kOnFunc);
  if (bhPages.Offset == 276)
    numOnFunc--;
  p2 += 40 + numOnFunc * 4;
  
  #define NSIS_MAX_INST_TYPES 32

  AddLF();

  UInt32 i;
  for (i = 0; i < NSIS_MAX_INST_TYPES + 1; i++, p2 += 4)
  {
    UInt32 instType = Get32(p2);
    if (instType != 0)
    {
      Script += "InstType";
      AString s2;
      if (!IsInstaller)
        s2 += "un.";
      ReadString2(s2, instType);
      SpaceQuStr(s2);
      NewLine();
    }
  }
  
  {
    UInt32 installDir = Get32(p2);
    p2 += 4;
    if (installDir != 0)
    {
      Script += "InstallDir";
      AddParam(installDir);
      NewLine();
    }
  }

  if (bhPages.Offset >= 288)
    for (i = 0; i < 4; i++)
    {
      if (i != 0 && bhPages.Offset < 300)
        break;
      UInt32 param = Get32(p2 + 4 * i);
      if (param == 0 || param == (UInt32)(Int32)-1)
        continue;
      
      /*
      uninstaller:
      UInt32 uninstChild = Get32(p2 + 8); // "$TEMP\\$1u_.exe"
      UInt32 uninstCmd = Get32(p2 + 12); // "\"$TEMP\\$1u_.exe\" $0 _?=$INSTDIR\\"
      int str_wininit = Get32(p2 + 16); // "$WINDIR\\wininit.ini"
      */
      
      AddCommentAndString(k_PostStrings[i]);
      Script += " =";
      AddParam(param);
      NewLine();
    }

  AddLF();

  #endif

  RINOK(ReadEntries(bhEntries))

  #ifdef NSIS_SCRIPT

  Separator();
  AddCommentAndString("UNREFERENCED STRINGS:");
  AddLF();
  AddLF();
  CommentOpen();

  for (i = 0; i < NumStringChars;)
  {
    if (!strUsed[i] && i != 0)
    // Script += "!!! ";
    {
      Add_UInt(i);
      AddParam(i);
      NewLine();
    }
    if (IsUnicode)
      i += GetUi16Str_Len((const Byte *)_data + _stringsPos + i * 2);
    else
      i += (UInt32)strlen((const char *)(const Byte *)_data + _stringsPos + i);
    i++;
  }
  CommentClose();
  #endif

  return SortItems();
}

static bool IsLZMA(const Byte *p, UInt32 &dictionary)
{
  dictionary = Get32(p + 1);
  return (p[0] == 0x5D &&
      p[1] == 0x00 && p[2] == 0x00 &&
      p[5] == 0x00 && (p[6] & 0x80) == 0x00);
}

static bool IsLZMA(const Byte *p, UInt32 &dictionary, bool &thereIsFlag)
{
  if (IsLZMA(p, dictionary))
  {
    thereIsFlag = false;
    return true;
  }
  if (p[0] <= 1 && IsLZMA(p + 1, dictionary))
  {
    thereIsFlag = true;
    return true;
  }
  return false;
}

static bool IsBZip2(const Byte *p)
{
  return (p[0] == 0x31 && p[1] < 14);
}

static bool IsZstd(const Byte *p)
{
  return (p[0] == 0x28 && p[1] == 0xB5 && p[2] == 0x2F && p[3] == 0xFD);
}

HRESULT CInArchive::Open2(const Byte *sig, size_t size)
{
  const UInt32 kSigSize = 4 + 1 + 5 + 2; // size, flag, 5 - lzma props, 2 - lzma first bytes
  if (size < kSigSize)
    return S_FALSE;

  _headerIsCompressed = true;
  IsSolid = true;
  FilterFlag = false;
  UseFilter = false;
  DictionarySize = 1;

  #ifdef NSIS_SCRIPT
  AfterHeaderSize = 0;
  #endif

  UInt32 compressedHeaderSize = Get32(sig);
  

  /*
    XX XX XX XX             XX XX XX XX == FirstHeader.HeaderSize, nonsolid, uncompressed
    5D 00 00 dd dd 00       solid LZMA
    00 5D 00 00 dd dd 00    solid LZMA, empty filter (there are no such archives)
    01 5D 00 00 dd dd 00    solid LZMA, BCJ filter   (only 7-Zip installer used that format)
    
    SS SS SS 80 00 5D 00 00 dd dd 00     non-solid LZMA, empty filter
    SS SS SS 80 01 5D 00 00 dd dd 00     non-solid LZMA, BCJ filte
    SS SS SS 80 01 tt         non-solid BZip (tt < 14
    SS SS SS 80               non-solid  deflate

    01 tt         solid BZip (tt < 14
    other         solid Deflate
  */

  if (compressedHeaderSize == FirstHeader.HeaderSize)
  {
    _headerIsCompressed = false;
    IsSolid = false;
    Method = NMethodType::kCopy;
  }
  else if (IsLZMA(sig, DictionarySize, FilterFlag))
    Method = NMethodType::kLZMA;
  else if (sig[3] == 0x80)
  {
    IsSolid = false;
    if (IsLZMA(sig + 4, DictionarySize, FilterFlag) && sig[3] == 0x80)
      Method = NMethodType::kLZMA;
    else if (IsBZip2(sig + 4))
      Method = NMethodType::kBZip2;
    else if (IsZstd(sig + 4))
      Method = NMethodType::kZstd;
    else
      Method = NMethodType::kDeflate;
  }
  else if (IsBZip2(sig))
    Method = NMethodType::kBZip2;
  else if (IsZstd(sig))
    Method = NMethodType::kZstd;
  else
    Method = NMethodType::kDeflate;

  if (IsSolid)
  {
    RINOK(SeekTo_DataStreamOffset())
  }
  else
  {
    _headerIsCompressed = ((compressedHeaderSize & kMask_IsCompressed) != 0);
    compressedHeaderSize &= ~kMask_IsCompressed;
    _nonSolidStartOffset = compressedHeaderSize;
    RINOK(SeekTo(DataStreamOffset + 4))
  }

  if (FirstHeader.HeaderSize == 0)
    return S_FALSE;

  _data.Alloc(FirstHeader.HeaderSize);
  _size = (size_t)FirstHeader.HeaderSize;

  Decoder.Method = Method;
  Decoder.FilterFlag = FilterFlag;
  Decoder.Solid = IsSolid;
  
  Decoder.IsNsisDeflate = true; // we need some smart check that NSIS is not NSIS3 here.
  
  Decoder.InputStream = _stream;
  Decoder.Buffer.Alloc(kInputBufSize);
  Decoder.StreamPos = 0;

  if (_headerIsCompressed)
  {
    RINOK(Decoder.Init(_stream, UseFilter))
    if (IsSolid)
    {
      size_t processedSize = 4;
      Byte buf[4];
      RINOK(Decoder.Read(buf, &processedSize))
      if (processedSize != 4)
        return S_FALSE;
      if (Get32((const Byte *)buf) != FirstHeader.HeaderSize)
        return S_FALSE;
    }
    {
      size_t processedSize = FirstHeader.HeaderSize;
      RINOK(Decoder.Read(_data, &processedSize))
      if (processedSize != FirstHeader.HeaderSize)
        return S_FALSE;
    }
    
    #ifdef NSIS_SCRIPT
    if (IsSolid)
    {
      /* we need additional bytes for data for WriteRegBin */
      AfterHeaderSize = (1 << 12);
      _afterHeader.Alloc(AfterHeaderSize);
      size_t processedSize = AfterHeaderSize;
      RINOK(Decoder.Read(_afterHeader, &processedSize))
      AfterHeaderSize = (UInt32)processedSize;
    }
    #endif
  }
  else
  {
    size_t processedSize = FirstHeader.HeaderSize;
    RINOK(ReadStream(_stream, (Byte *)_data, &processedSize))
    if (processedSize < FirstHeader.HeaderSize)
      return S_FALSE;
  }

  #ifdef NUM_SPEED_TESTS
  for (unsigned i = 0; i < NUM_SPEED_TESTS; i++)
  {
    RINOK(Parse());
    Clear2();
  }
  #endif

  return Parse();
}

/*
NsisExe =
{
  ExeStub
  Archive  // must start from 512 * N
  #ifndef NSIS_CONFIG_CRC_ANAL
  {
    Some additional data
  }
}

Archive
{
  FirstHeader
  Data
  #ifdef NSIS_CONFIG_CRC_SUPPORT && FirstHeader.ThereIsCrc()
  {
    CRC
  }
}

FirstHeader
{
  UInt32 Flags;
  Byte Signature[16];
  // points to the header+sections+entries+stringtable in the datablock
  UInt32 HeaderSize;
  UInt32 ArcSize;
}
*/


// ---------- PE (EXE) parsing ----------

static const unsigned k_PE_StartSize = 0x40;
static const unsigned k_PE_HeaderSize = 4 + 20;
static const unsigned k_PE_OptHeader32_Size_MIN = 96;

static inline bool CheckPeOffset(UInt32 pe)
{
  return (pe >= 0x40 && pe <= 0x1000 && (pe & 7) == 0);
}


static bool IsArc_Pe(const Byte *p, size_t size)
{
  if (size < 2)
    return false;
  if (p[0] != 'M' || p[1] != 'Z')
    return false;
  if (size < k_PE_StartSize)
    return false; // k_IsArc_Res_NEED_MORE;
  UInt32 pe = Get32(p + 0x3C);
  if (!CheckPeOffset(pe))
    return false;
  if (pe + k_PE_HeaderSize > size)
    return false; // k_IsArc_Res_NEED_MORE;

  p += pe;
  if (Get32(p) != 0x00004550)
    return false;
  return Get16(p + 4 + 16) >= k_PE_OptHeader32_Size_MIN;
}

HRESULT CInArchive::Open(IInStream *inStream, const UInt64 *maxCheckStartPosition)
{
  Clear();
  
  RINOK(InStream_GetPos(inStream, StartOffset))
  
  const UInt32 kStartHeaderSize = 4 * 7;
  const unsigned kStep = 512; // nsis start is aligned for 512
  Byte buf[kStep];
  UInt64 pos = StartOffset;
  size_t bufSize = 0;
  UInt64 pePos = (UInt64)(Int64)-1;
  
  for (;;)
  {
    bufSize = kStep;
    RINOK(ReadStream(inStream, buf, &bufSize))
    if (bufSize < kStartHeaderSize)
      return S_FALSE;
    if (memcmp(buf + 4, kSignature, kSignatureSize) == 0)
      break;
    if (IsArc_Pe(buf, bufSize))
      pePos = pos;
    pos += kStep;
    UInt64 proc = pos - StartOffset;
    if (maxCheckStartPosition && proc > *maxCheckStartPosition)
    {
      if (pePos == 0)
      {
        if (proc > (1 << 20))
          return S_FALSE;
      }
      else
        return S_FALSE;
    }
  }
  
  if (pePos == (UInt64)(Int64)-1)
  {
    UInt64 posCur = StartOffset;
    for (;;)
    {
      if (posCur < kStep)
        break;
      posCur -= kStep;
      if (pos - posCur > (1 << 20))
        break;
      bufSize = kStep;
      RINOK(InStream_SeekSet(inStream, posCur))
      RINOK(ReadStream(inStream, buf, &bufSize))
      if (bufSize < kStep)
        break;
      if (IsArc_Pe(buf, bufSize))
      {
        pePos = posCur;
        break;
      }
    }

    // restore buf to nsis header
    bufSize = kStep;
    RINOK(InStream_SeekSet(inStream, pos))
    RINOK(ReadStream(inStream, buf, &bufSize))
    if (bufSize < kStartHeaderSize)
      return S_FALSE;
  }

  StartOffset = pos;
  UInt32 peSize = 0;
  
  if (pePos != (UInt64)(Int64)-1)
  {
    UInt64 peSize64 = (pos - pePos);
    if (peSize64 < (1 << 20))
    {
      peSize = (UInt32)peSize64;
      StartOffset = pePos;
    }
  }

  DataStreamOffset = pos + kStartHeaderSize;
  FirstHeader.Flags = Get32(buf);
  if ((FirstHeader.Flags & (~kFlagsMask)) != 0)
  {
    // return E_NOTIMPL;
    return S_FALSE;
  }
  IsInstaller = (FirstHeader.Flags & NFlags::kUninstall) == 0;

  FirstHeader.HeaderSize = Get32(buf + kSignatureSize + 4);
  FirstHeader.ArcSize = Get32(buf + kSignatureSize + 8);
  if (FirstHeader.ArcSize <= kStartHeaderSize)
    return S_FALSE;

  /*
  if ((FirstHeader.Flags & NFlags::k_BI_ExternalFileSupport) != 0)
  {
    UInt32 datablock_low = Get32(buf + kSignatureSize + 12);
    UInt32 datablock_high = Get32(buf + kSignatureSize + 16);
  }
  */
  
  RINOK(InStream_GetSize_SeekToEnd(inStream, _fileSize))

  IsArc = true;

  if (peSize != 0)
  {
    ExeStub.Alloc(peSize);
    RINOK(InStream_SeekSet(inStream, pePos))
    RINOK(ReadStream_FALSE(inStream, ExeStub, peSize))
  }

  HRESULT res = S_FALSE;
  try
  {
    CLimitedInStream *_limitedStreamSpec = new CLimitedInStream;
    _stream = _limitedStreamSpec;
    _limitedStreamSpec->SetStream(inStream);
    _limitedStreamSpec->InitAndSeek(pos, FirstHeader.ArcSize);
    DataStreamOffset -= pos;
    res = Open2(buf + kStartHeaderSize, bufSize - kStartHeaderSize);
  }
  catch(...)
  {
    _stream.Release();
    throw;
    // res = S_FALSE;
  }
  if (res != S_OK)
  {
    _stream.Release();
    // Clear();
  }
  return res;
}

UString CInArchive::ConvertToUnicode(const AString &s) const
{
  if (IsUnicode)
  {
    UString res;
    // if (
      ConvertUTF8ToUnicode(s, res);
      return res;
  }
  return MultiByteToUnicodeString(s);
}

void CInArchive::Clear2()
{
  IsUnicode = false;
  NsisType = k_NsisType_Nsis2;
  IsNsis225 = false;
  IsNsis200 = false;
  LogCmdIsEnabled = false;
  BadCmd = -1;
  Is64Bit = false;

  #ifdef NSIS_SCRIPT
  Name.Empty();
  BrandingText.Empty();
  Script.Empty();
  LicenseFiles.Clear();
  _numRootLicenses = 0;
  _numLangStrings = 0;
  langStrIDs.Clear();
  LangComment.Empty();
  noParseStringIndexes.Clear();
  #endif

  APrefixes.Clear();
  UPrefixes.Clear();
  Items.Clear();
  IsUnicode = false;
  ExeStub.Free();
}

void CInArchive::Clear()
{
  Clear2();
  IsArc = false;
  _stream.Release();
}

}}

/* ================ unit: CPP/7zip/Archive/Nsis/NsisRegister.cpp ================ */
// NsisRegister.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NNsis {

REGISTER_ARC_I(
  "Nsis", "nsis", NULL, 0x9,
  kSignature,
  4,
  NArcInfoFlags::kFindSignature |
  NArcInfoFlags::kUseGlobalOffset,
  NULL)

}}
