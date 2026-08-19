/* XArchive amalgamation of 7-Zip 26.01 -- Archive Zip/Nsis/Rar readers.
 *
 * 5 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/7zip/Archive/Zip/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

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

/* ---- CPP/7zip/Archive/Zip/ZipHeader.h ---- */
// ZipHeader.h

#ifndef ZIP7_INC_ARCHIVE_ZIP_HEADER_H
#define ZIP7_INC_ARCHIVE_ZIP_HEADER_H

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

const unsigned kMarkerSize = 4;

namespace NSignature
{
  const UInt32 kLocalFileHeader   = 0x04034B50;
  const UInt32 kDataDescriptor    = 0x08074B50;
  const UInt32 kCentralFileHeader = 0x02014B50;
  const UInt32 kEcd               = 0x06054B50;
  const UInt32 kEcd64             = 0x06064B50;
  const UInt32 kEcd64Locator      = 0x07064B50;
  const UInt32 kSpan              = 0x08074B50;
  const UInt32 kNoSpan            = 0x30304B50; // PK00, replaces kSpan, if there is only 1 segment
}

const unsigned kLocalHeaderSize = 4 + 26; // including signature
const unsigned kDataDescriptorSize32 = 4 + 4 + 4 * 2;  // including signature
const unsigned kDataDescriptorSize64 = 4 + 4 + 8 * 2;  // including signature
const unsigned kCentralHeaderSize = 4 + 42; // including signature

const unsigned kEcdSize = 22; // including signature
const unsigned kEcd64_MainSize = 44;
const unsigned kEcd64_FullSize = 12 + kEcd64_MainSize;
const unsigned kEcd64Locator_Size = 20;

namespace NFileHeader
{
  namespace NCompressionMethod
  {
    enum EType
    {
      kStore = 0,
      kShrink = 1,
      kReduce1 = 2,
      kReduce2 = 3,
      kReduce3 = 4,
      kReduce4 = 5,
      kImplode = 6,
      kTokenize = 7,
      kDeflate = 8,
      kDeflate64 = 9,
      kPKImploding = 10,
      
      kBZip2 = 12,
      
      kLZMA = 14,
      
      kTerse = 18,
      kLz77 = 19,
      kZstdPk = 20,
      
      kZstdWz = 93,
      kMP3 = 94,
      kXz = 95,
      kJpeg = 96,
      kWavPack = 97,
      kPPMd = 98,
      kWzAES = 99
    };

    const Byte kMadeByProgramVersion = 63;
    
    const Byte kExtractVersion_Default = 10;
    const Byte kExtractVersion_Dir = 20;
    const Byte kExtractVersion_ZipCrypto = 20;
    const Byte kExtractVersion_Deflate = 20;
    const Byte kExtractVersion_Deflate64 = 21;
    const Byte kExtractVersion_Zip64 = 45;
    const Byte kExtractVersion_BZip2 = 46;
    const Byte kExtractVersion_Aes = 51;
    const Byte kExtractVersion_LZMA = 63;
    const Byte kExtractVersion_PPMd = 63;
    const Byte kExtractVersion_Xz = 20; // test it
  }

  namespace NExtraID
  {
    enum
    {
      kZip64 = 0x01,
      kNTFS = 0x0A,
      kUnix0 = 0x0D,                // Info-ZIP : (UNIX) PK
      kStrongEncrypt = 0x17,
      kIzNtSecurityDescriptor = 0x4453,
      kUnixTime = 0x5455,           // "UT" (time) Info-ZIP
      kUnix1 = 0x5855,              // Info-ZIP
      kIzUnicodeComment = 0x6375,
      kIzUnicodeName = 0x7075,
      kUnix2 = 0x7855,              // Info-ZIP
      kUnixN = 0x7875,              // Info-ZIP
      kWzAES = 0x9901,
      kApkAlign = 0xD935
    };
  }

  namespace NNtfsExtra
  {
    const UInt16 kTagTime = 1;
    enum
    {
      kMTime = 0,
      kATime,
      kCTime
    };
  }

  namespace NUnixTime
  {
    enum
    {
      kMTime = 0,
      kATime,
      kCTime
    };
  }

  namespace NUnixExtra
  {
    enum
    {
      kATime = 0,
      kMTime
    };
  }

  namespace NFlags
  {
    const unsigned kEncrypted = 1 << 0;
    const unsigned kLzmaEOS = 1 << 1;
    const unsigned kDescriptorUsedMask = 1 << 3;
    const unsigned kStrongEncrypted = 1 << 6;
    const unsigned kUtf8 = 1 << 11;
    const unsigned kAltStream = 1 << 14;

    const unsigned kImplodeDictionarySizeMask = 1 << 1;
    const unsigned kImplodeLiteralsOnMask     = 1 << 2;
    
    /*
    const unsigned kDeflateTypeBitStart = 1;
    const unsigned kNumDeflateTypeBits = 2;
    const unsigned kNumDeflateTypes = (1 << kNumDeflateTypeBits);
    const unsigned kDeflateTypeMask = (1 << kNumDeflateTypeBits) - 1;
    */
  }
  
  namespace NHostOS
  {
    enum EEnum
    {
      kFAT      =  0,
      kAMIGA    =  1,
      kVMS      =  2,  // VAX/VMS
      kUnix     =  3,
      kVM_CMS   =  4,
      kAtari    =  5,  // what if it's a minix filesystem? [cjh]
      kHPFS     =  6,  // filesystem used by OS/2 (and NT 3.x)
      kMac      =  7,
      kZ_System =  8,
      kCPM      =  9,
      kTOPS20   = 10,  // pkzip 2.50 NTFS
      kNTFS     = 11,  // filesystem used by Windows NT
      kQDOS     = 12,  // SMS/QDOS
      kAcorn    = 13,  // Archimedes Acorn RISC OS
      kVFAT     = 14,  // filesystem used by Windows 95, NT
      kMVS      = 15,
      kBeOS     = 16,  // hybrid POSIX/database filesystem
      kTandem   = 17,
      kOS400    = 18,
      kOSX      = 19
    };
  }


  namespace NAmigaAttrib
  {
    const UInt32 kIFMT     = 06000;    // Amiga file type mask
    const UInt32 kIFDIR    = 04000;    // Amiga directory
    const UInt32 kIFREG    = 02000;    // Amiga regular file
    const UInt32 kIHIDDEN  = 00200;    // to be supported in AmigaDOS 3.x
    const UInt32 kISCRIPT  = 00100;    // executable script (text command file)
    const UInt32 kIPURE    = 00040;    // allow loading into resident memory
    const UInt32 kIARCHIVE = 00020;    // not modified since bit was last set
    const UInt32 kIREAD    = 00010;    // can be opened for reading
    const UInt32 kIWRITE   = 00004;    // can be opened for writing
    const UInt32 kIEXECUTE = 00002;    // executable image, a loadable runfile
    const UInt32 kIDELETE  = 00001;    // can be deleted
  }
}

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

/* ---- CPP/7zip/Archive/Zip/ZipItem.h ---- */
// Archive/ZipItem.h

#ifndef ZIP7_INC_ARCHIVE_ZIP_ITEM_H
#define ZIP7_INC_ARCHIVE_ZIP_ITEM_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

/*
extern const char *k_SpecName_NTFS_STREAM;
extern const char *k_SpecName_MAC_RESOURCE_FORK;
*/

struct CVersion
{
  Byte Version;
  Byte HostOS;
};

struct CExtraSubBlock
{
  UInt32 ID;
  CByteBuffer Data;

  bool ExtractNtfsTime(unsigned index, FILETIME &ft) const;
  bool Extract_UnixTime(bool isCentral, unsigned index, UInt32 &res) const;
  bool Extract_Unix01_Time(unsigned index, UInt32 &res) const;
  // bool Extract_Unix_Time(unsigned index, UInt32 &res) const;

  bool CheckIzUnicode(const AString &s) const;

  void PrintInfo(AString &s) const;
};

const unsigned k_WzAesExtra_Size = 7;

struct CWzAesExtra
{
  UInt16 VendorVersion; // 1: AE-1, 2: AE-2,
  // UInt16 VendorId; // 'A' 'E'
  Byte Strength; // 1: 128-bit, 2: 192-bit, 3: 256-bit
  UInt16 Method;

  CWzAesExtra(): VendorVersion(2), Strength(3), Method(0) {}

  bool NeedCrc() const { return (VendorVersion == 1); }

  bool ParseFromSubBlock(const CExtraSubBlock &sb)
  {
    if (sb.ID != NFileHeader::NExtraID::kWzAES)
      return false;
    if (sb.Data.Size() < k_WzAesExtra_Size)
      return false;
    const Byte *p = (const Byte *)sb.Data;
    VendorVersion = GetUi16(p);
    if (p[2] != 'A' || p[3] != 'E')
      return false;
    Strength = p[4];
    // 9.31: The BUG was fixed:
    Method = GetUi16(p + 5);
    return true;
  }
  
  void SetSubBlock(CExtraSubBlock &sb) const
  {
    sb.Data.Alloc(k_WzAesExtra_Size);
    sb.ID = NFileHeader::NExtraID::kWzAES;
    Byte *p = (Byte *)sb.Data;
    p[0] = (Byte)VendorVersion;
    p[1] = (Byte)(VendorVersion >> 8);
    p[2] = 'A';
    p[3] = 'E';
    p[4] = Strength;
    p[5] = (Byte)Method;
    p[6] = (Byte)(Method >> 8);
  }
};

namespace NStrongCrypto_AlgId
{
  const UInt16 kDES = 0x6601;
  const UInt16 kRC2old = 0x6602;
  const UInt16 k3DES168 = 0x6603;
  const UInt16 k3DES112 = 0x6609;
  const UInt16 kAES128 = 0x660E;
  const UInt16 kAES192 = 0x660F;
  const UInt16 kAES256 = 0x6610;
  const UInt16 kRC2 = 0x6702;
  const UInt16 kBlowfish = 0x6720;
  const UInt16 kTwofish = 0x6721;
  const UInt16 kRC4 = 0x6801;
}

struct CStrongCryptoExtra
{
  UInt16 Format;
  UInt16 AlgId;
  UInt16 BitLen;
  UInt16 Flags;

  bool ParseFromSubBlock(const CExtraSubBlock &sb)
  {
    if (sb.ID != NFileHeader::NExtraID::kStrongEncrypt)
      return false;
    const Byte *p = (const Byte *)sb.Data;
    if (sb.Data.Size() < 8)
      return false;
    Format = GetUi16(p + 0);
    AlgId  = GetUi16(p + 2);
    BitLen = GetUi16(p + 4);
    Flags  = GetUi16(p + 6);
    return (Format == 2);
  }

  bool CertificateIsUsed() const { return (Flags > 0x0001); }
};


struct CExtraBlock
{
  CObjectVector<CExtraSubBlock> SubBlocks;
  bool Error;
  bool MinorError;
  bool IsZip64;
  bool IsZip64_Error;
  
  CExtraBlock(): Error(false), MinorError(false), IsZip64(false), IsZip64_Error(false) {}

  void Clear()
  {
    SubBlocks.Clear();
    IsZip64 = false;
  }
  
  size_t GetSize() const
  {
    size_t res = 0;
    FOR_VECTOR (i, SubBlocks)
      res += SubBlocks[i].Data.Size() + 2 + 2;
    return res;
  }
  
  bool GetWzAes(CWzAesExtra &e) const
  {
    FOR_VECTOR (i, SubBlocks)
      if (e.ParseFromSubBlock(SubBlocks[i]))
        return true;
    return false;
  }

  bool HasWzAes() const
  {
    CWzAesExtra e;
    return GetWzAes(e);
  }

  bool GetStrongCrypto(CStrongCryptoExtra &e) const
  {
    FOR_VECTOR (i, SubBlocks)
      if (e.ParseFromSubBlock(SubBlocks[i]))
        return true;
    return false;
  }

  /*
  bool HasStrongCrypto() const
  {
    CStrongCryptoExtra e;
    return GetStrongCrypto(e);
  }
  */

  bool GetNtfsTime(unsigned index, FILETIME &ft) const;
  bool GetUnixTime(bool isCentral, unsigned index, UInt32 &res) const;

  void PrintInfo(AString &s) const;

  void RemoveUnknownSubBlocks()
  {
    for (unsigned i = SubBlocks.Size(); i != 0;)
    {
      i--;
      switch (SubBlocks[i].ID)
      {
        case NFileHeader::NExtraID::kStrongEncrypt:
        case NFileHeader::NExtraID::kWzAES:
          break;
        default:
          SubBlocks.Delete(i);
      }
    }
  }
};


class CLocalItem
{
public:
  UInt16 Flags;
  UInt16 Method;
  
  /*
    Zip specification doesn't mention that ExtractVersion field uses HostOS subfield.
    18.06: 7-Zip now doesn't use ExtractVersion::HostOS to detect codePage
  */

  CVersion ExtractVersion;

  UInt64 Size;
  UInt64 PackSize;
  UInt32 Time;
  UInt32 Crc;

  UInt32 Disk;
  
  AString Name;

  CExtraBlock LocalExtra;

  unsigned GetDescriptorSize() const { return LocalExtra.IsZip64 ? kDataDescriptorSize64 : kDataDescriptorSize32; }

  UInt64 GetPackSizeWithDescriptor() const
    { return PackSize + (HasDescriptor() ? GetDescriptorSize() : 0); }

  bool IsUtf8() const { return (Flags & NFileHeader::NFlags::kUtf8) != 0; }
  bool IsEncrypted() const { return (Flags & NFileHeader::NFlags::kEncrypted) != 0; }
  bool IsStrongEncrypted() const { return IsEncrypted() && (Flags & NFileHeader::NFlags::kStrongEncrypted) != 0; }
  bool IsAesEncrypted() const { return IsEncrypted() && (IsStrongEncrypted() || Method == NFileHeader::NCompressionMethod::kWzAES); }
  bool IsLzmaEOS() const { return (Flags & NFileHeader::NFlags::kLzmaEOS) != 0; }
  bool HasDescriptor() const { return (Flags & NFileHeader::NFlags::kDescriptorUsedMask) != 0; }
  // bool IsAltStream() const { return (Flags & NFileHeader::NFlags::kAltStream) != 0; }

  unsigned GetDeflateLevel() const { return (Flags >> 1) & 3; }
  
  bool IsDir() const;

  /*
  void GetUnicodeString(const AString &s, UString &res) const
  {
    bool isUtf8 = IsUtf8();
    if (isUtf8)
      if (ConvertUTF8ToUnicode(s, res))
        return;
    MultiByteToUnicodeString2(res, s, GetCodePage());
  }
  */

private:

  void SetFlag(unsigned bitMask, bool enable)
  {
    if (enable)
      Flags = (UInt16)(Flags | bitMask);
    else
      Flags = (UInt16)(Flags & ~bitMask);
  }

public:

  void ClearFlags() { Flags = 0; }
  void SetEncrypted(bool encrypted) { SetFlag(NFileHeader::NFlags::kEncrypted, encrypted); }
  void SetUtf8(bool isUtf8) { SetFlag(NFileHeader::NFlags::kUtf8, isUtf8); }
  // void SetFlag_AltStream(bool isAltStream) { SetFlag(NFileHeader::NFlags::kAltStream, isAltStream); }
  void SetDescriptorMode(bool useDescriptor) { SetFlag(NFileHeader::NFlags::kDescriptorUsedMask, useDescriptor); }

  UINT GetCodePage() const
  {
    if (IsUtf8())
      return CP_UTF8;
    return CP_OEMCP;
  }
};


class CItem: public CLocalItem
{
public:
  CVersion MadeByVersion;
  UInt16 InternalAttrib;
  UInt32 ExternalAttrib;
  
  UInt64 LocalHeaderPos;
  
  CExtraBlock CentralExtra;
  CByteBuffer Comment;

  bool FromLocal;
  bool FromCentral;
  
  // CItem can be used as CLocalItem. So we must clear unused fields
  CItem():
      InternalAttrib(0),
      ExternalAttrib(0),
      FromLocal(false),
      FromCentral(false)
  {
    MadeByVersion.Version = 0;
    MadeByVersion.HostOS = 0;
  }

  const CExtraBlock &GetMainExtra() const { return *(FromCentral ? &CentralExtra : &LocalExtra); }

  bool IsDir() const;
  UInt32 GetWinAttrib() const;
  bool GetPosixAttrib(UInt32 &attrib) const;

  // 18.06: 0 instead of ExtractVersion.HostOS for local item
  Byte GetHostOS() const { return FromCentral ? MadeByVersion.HostOS : (Byte)0; }

  void GetUnicodeString(UString &res, const AString &s, bool isComment, bool useSpecifiedCodePage, UINT codePage) const;

  bool IsThereCrc() const
  {
    if (Method == NFileHeader::NCompressionMethod::kWzAES)
    {
      CWzAesExtra aesField;
      if (GetMainExtra().GetWzAes(aesField))
        return aesField.NeedCrc();
    }
    return (Crc != 0 || !IsDir());
  }

  bool Is_MadeBy_Unix() const
  {
    if (!FromCentral)
      return false;
    return (MadeByVersion.HostOS == NFileHeader::NHostOS::kUnix);
  }
  
  UINT GetCodePage() const
  {
    // 18.06: now we use HostOS only from Central::MadeByVersion
    if (IsUtf8())
      return CP_UTF8;
    if (!FromCentral)
      return CP_OEMCP;
    Byte hostOS = MadeByVersion.HostOS;
    return (UINT)((
           hostOS == NFileHeader::NHostOS::kFAT
        || hostOS == NFileHeader::NHostOS::kNTFS
        || hostOS == NFileHeader::NHostOS::kUnix // do we need it?
        ) ? CP_OEMCP : CP_ACP);
  }
};

}}

#endif

/* ---- CPP/7zip/Archive/Zip/ZipIn.h ---- */
// Archive/ZipIn.h

#ifndef ZIP7_INC_ZIP_IN_H
#define ZIP7_INC_ZIP_IN_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

API_FUNC_IsArc IsArc_Zip(const Byte *p, size_t size);

namespace NArchive {
namespace NZip {
  
class CItemEx: public CItem
{
public:
  UInt32 LocalFullHeaderSize; // including Name and Extra
  // int ParentOfAltStream; // -1, if not AltStream
  
  bool DescriptorWasRead;

  CItemEx():
    // ParentOfAltStream(-1),
    DescriptorWasRead(false) {}

  UInt64 GetLocalFullSize() const
    { return LocalFullHeaderSize + GetPackSizeWithDescriptor(); }
  UInt64 GetDataPosition() const
    { return LocalHeaderPos + LocalFullHeaderSize; }

  bool IsBadDescriptor() const
  {
    return !FromCentral && FromLocal && HasDescriptor() && !DescriptorWasRead;
  }
};


struct CInArchiveInfo
{
  Int64 Base; /* Base offset of start of archive in stream.
                 Offsets in headers must be calculated from that Base.
                 Base is equal to MarkerPos for normal ZIPs.
                 Base can point to PE stub for some ZIP SFXs.
                 if CentralDir was read,
                   Base can be negative, if start of data is not available,
                 if CentralDirs was not read,
                   Base = ArcInfo.MarkerPos; */

  /* The following *Pos variables contain absolute offsets in Stream */

  UInt64 MarkerPos;  /* Pos of first signature, it can point to kSpan/kNoSpan signature
                        = MarkerPos2      in most archives
                        = MarkerPos2 - 4  if there is kSpan/kNoSpan signature */
  UInt64 MarkerPos2; // Pos of first local item signature in stream
  UInt64 FinishPos;  // Finish pos of archive data in starting volume
  UInt64 FileEndPos; // Finish pos of stream

  UInt64 FirstItemRelatOffset; /* Relative offset of first local (read from cd) (relative to Base).
                                  = 0 in most archives
                                  = size of stub for some SFXs */


  int MarkerVolIndex;

  bool CdWasRead;
  bool IsSpanMode;
  bool ThereIsTail;

  // UInt32 BaseVolIndex;

  CByteBuffer Comment;


  CInArchiveInfo():
      Base(0),
      MarkerPos(0),
      MarkerPos2(0),
      FinishPos(0),
      FileEndPos(0),
      FirstItemRelatOffset(0),
      MarkerVolIndex(-1),
      CdWasRead(false),
      IsSpanMode(false),
      ThereIsTail(false)
      // BaseVolIndex(0)
      {}
  
  void Clear()
  {
    // BaseVolIndex = 0;
    Base = 0;
    MarkerPos = 0;
    MarkerPos2 = 0;
    FinishPos = 0;
    FileEndPos = 0;
    MarkerVolIndex = -1;
    ThereIsTail = false;

    FirstItemRelatOffset = 0;

    CdWasRead = false;
    IsSpanMode = false;

    Comment.Free();
  }
};


struct CCdInfo
{
  bool IsFromEcd64;
  
  UInt16 CommentSize;
  
  // 64
  UInt16 VersionMade;
  UInt16 VersionNeedExtract;

  // old zip
  UInt32 ThisDisk;
  UInt32 CdDisk;
  UInt64 NumEntries_in_ThisDisk;
  UInt64 NumEntries;
  UInt64 Size;
  UInt64 Offset;

  CCdInfo() { memset(this, 0, sizeof(*this)); IsFromEcd64 = false; }

  void ParseEcd32(const Byte *p);   // (p) includes signature
  void ParseEcd64e(const Byte *p);  // (p) exclude signature

  bool IsEmptyArc() const
  {
    return ThisDisk == 0
        && CdDisk == 0
        && NumEntries_in_ThisDisk == 0
        && NumEntries == 0
        && Size == 0
        && Offset == 0 // test it
    ;
  }
};


struct CVols
{
  struct CSubStreamInfo
  {
    CMyComPtr<IInStream> Stream;
    UInt64 Size;

    HRESULT SeekToStart() const { return InStream_SeekToBegin(Stream); }

    CSubStreamInfo(): Size(0) {}
  };
  
  CObjectVector<CSubStreamInfo> Streams;
  
  int StreamIndex;   // -1 for StartStream
                     // -2 for ZipStream at multivol detection code
                     // >=0 volume index in multivol
  
  bool NeedSeek;

  bool DisableVolsSearch;
  bool StartIsExe;  // is .exe
  bool StartIsZ;    // is .zip or .zNN
  bool StartIsZip;  // is .zip
  bool IsUpperCase;
  bool MissingZip;

  bool ecd_wasRead;

  Int32 StartVolIndex; // -1, if unknown vol index
                       // = (NN - 1), if StartStream is .zNN
                       // = 0, if start vol is exe

  Int32 StartParsingVol; // if we need local parsing, we must use that stream
  unsigned NumVols;

  int EndVolIndex; // index of last volume (ecd volume),
                   // -1, if is not multivol

  UString BaseName; // name of archive including '.'
  UString MissingName;

  CMyComPtr<IInStream> ZipStream;

  CCdInfo ecd;

  UInt64 TotalBytesSize; // for MultiVol only

  void ClearRefs()
  {
    Streams.Clear();
    ZipStream.Release();
    TotalBytesSize = 0;
  }

  void Clear()
  {
    StreamIndex = -1;
    NeedSeek = false;

    DisableVolsSearch = false;
    StartIsExe = false;
    StartIsZ = false;
    StartIsZip = false;
    IsUpperCase = false;

    StartVolIndex = -1;
    StartParsingVol = 0;
    NumVols = 0;
    EndVolIndex = -1;

    BaseName.Empty();
    MissingName.Empty();

    MissingZip = false;
    ecd_wasRead = false;

    ClearRefs();
  }

  HRESULT ParseArcName(IArchiveOpenVolumeCallback *volCallback);

  HRESULT Read(void *data, UInt32 size, UInt32 *processedSize);
};


Z7_CLASS_IMP_COM_1(
  CVolStream
  , ISequentialInStream
)
public:
  CVols *Vols;
};


class CInArchive
{
  CMidBuffer Buffer;
  size_t _bufPos;
  size_t _bufCached;

  UInt64 _streamPos;
  UInt64 _cnt;

  // UInt32 _startLocalFromCd_Disk;
  // UInt64 _startLocalFromCd_Offset;

  size_t GetAvail() const { return _bufCached - _bufPos; }

  void InitBuf() { _bufPos = 0; _bufCached = 0; }
  void DisableBufMode() { InitBuf(); _inBufMode = false; }

  void SkipLookahed(size_t skip)
  {
    _bufPos += skip;
    _cnt += skip;
  }

  HRESULT AllocateBuffer(size_t size);

  UInt64 GetVirtStreamPos() { return _streamPos - _bufCached + _bufPos; }

  bool _inBufMode;

  bool IsArcOpen;
  bool CanStartNewVol;

  UInt32 _signature;

  CMyComPtr<IInStream> StreamRef;
  IInStream *Stream;
  IInStream *StartStream;
  IArchiveOpenCallback *Callback;

  HRESULT Seek_SavePos(UInt64 offset);
  HRESULT SeekToVol(int volIndex, UInt64 offset);

  HRESULT ReadFromCache(Byte *data, unsigned size, unsigned &processed);
  HRESULT ReadFromCache_FALSE(Byte *data, unsigned size);

  HRESULT ReadVols2(IArchiveOpenVolumeCallback *volCallback,
      unsigned start, int lastDisk, int zipDisk, unsigned numMissingVolsMax, unsigned &numMissingVols);
  HRESULT ReadVols();

  HRESULT FindMarker(const UInt64 *searchLimit);
  HRESULT IncreaseRealPosition(UInt64 addValue, bool &isFinished);

  HRESULT LookAhead(size_t minRequiredInBuffer);
  void SafeRead(Byte *data, unsigned size);
  void ReadBuffer(CByteBuffer &buffer, unsigned size);
  // Byte ReadByte();
  // UInt16 ReadUInt16();
  UInt32 ReadUInt32();
  UInt64 ReadUInt64();

  void ReadSignature();

  void Skip(size_t num);
  HRESULT Skip64(UInt64 num, unsigned numFiles);

  bool ReadFileName(unsigned nameSize, AString &dest);

  bool ReadExtra(const CLocalItem &item, unsigned extraSize, CExtraBlock &extra,
      UInt64 &unpackSize, UInt64 &packSize, CItem *cdItem);
  bool ReadLocalItem(CItemEx &item);
  HRESULT FindDescriptor(CItemEx &item, unsigned numFiles);
  HRESULT ReadCdItem(CItemEx &item);
  HRESULT TryEcd64(UInt64 offset, CCdInfo &cdInfo);
  HRESULT FindCd(bool checkOffsetMode);
  HRESULT TryReadCd(CObjectVector<CItemEx> &items, const CCdInfo &cdInfo, UInt64 cdOffset, UInt64 cdSize);
  HRESULT ReadCd(CObjectVector<CItemEx> &items, UInt32 &cdDisk, UInt64 &cdOffset, UInt64 &cdSize);
  HRESULT ReadLocals(CObjectVector<CItemEx> &localItems);

  HRESULT ReadHeaders(CObjectVector<CItemEx> &items);

  HRESULT GetVolStream(unsigned vol, UInt64 pos, CMyComPtr<ISequentialInStream> &stream);

public:
  CInArchiveInfo ArcInfo;
  
  bool IsArc;
  bool IsZip64;

  bool IsApk;
  bool IsCdUnsorted;
  
  bool HeadersError;
  bool HeadersWarning;
  bool ExtraMinorError;
  bool UnexpectedEnd;
  bool LocalsWereRead;
  bool LocalsCenterMerged;
  bool NoCentralDir;
  bool Overflow32bit; // = true, if zip without Zip64 extension support and it has some fields values truncated to 32-bits.
  bool Cd_NumEntries_Overflow_16bit; // = true, if no Zip64 and 16-bit ecd:NumEntries was overflowed.

  bool MarkerIsFound;
  bool MarkerIsSafe;

  bool IsMultiVol;
  bool UseDisk_in_SingleVol;
  UInt32 EcdVolIndex;

  CVols Vols;

  bool Force_ReadLocals_Mode;
  bool Disable_VolsRead;
  bool Disable_FindMarker;
 
  CInArchive():
      IsArcOpen(false),
      Stream(NULL),
      StartStream(NULL),
      Callback(NULL),
      Force_ReadLocals_Mode(false),
      Disable_VolsRead(false),
      Disable_FindMarker(false)
      {}

  UInt64 GetPhySize() const
  {
    if (IsMultiVol)
      return ArcInfo.FinishPos;
    else
      return (UInt64)((Int64)ArcInfo.FinishPos - ArcInfo.Base);
  }

  UInt64 GetOffset() const
  {
    if (IsMultiVol)
      return 0;
    else
      return (UInt64)ArcInfo.Base;
  }

  
  void ClearRefs();
  void Close();
  HRESULT Open(IInStream *stream, const UInt64 *searchLimit, IArchiveOpenCallback *callback, CObjectVector<CItemEx> &items);

  bool IsOpen() const { return IsArcOpen; }
  
  bool AreThereErrors() const
  {
    return HeadersError
        || UnexpectedEnd
        || !Vols.MissingName.IsEmpty();
  }

  bool IsLocalOffsetOK(const CItemEx &item) const
  {
    if (item.FromLocal)
      return true;
    return (Int64)GetOffset() + (Int64)item.LocalHeaderPos >= 0;
  }

  UInt64 GetEmbeddedStubSize() const
  {
    // it's possible that first item in CD doesn refers to first local item
    // so FirstItemRelatOffset is not first local item

    if (ArcInfo.CdWasRead)
      return ArcInfo.FirstItemRelatOffset;
    if (IsMultiVol)
      return 0;
    return (UInt64)((Int64)ArcInfo.MarkerPos2 - ArcInfo.Base);
  }


  HRESULT CheckDescriptor(const CItemEx &item);
  HRESULT Read_LocalItem_After_CdItem(CItemEx &item, bool &isAvail, bool &headersError);
  HRESULT Read_LocalItem_After_CdItem_Full(CItemEx &item);

  HRESULT GetItemStream(const CItemEx &item, bool seekPackData, CMyComPtr<ISequentialInStream> &stream);

  IInStream *GetBaseStream() { return StreamRef; }

  bool CanUpdate() const
  {
    if (AreThereErrors()
       || IsMultiVol
       || ArcInfo.Base < 0
       || (Int64)ArcInfo.MarkerPos2 < ArcInfo.Base
       || ArcInfo.ThereIsTail
       || GetEmbeddedStubSize() != 0
       || IsApk
       || IsCdUnsorted)
      return false;
   
    // 7-zip probably can update archives with embedded stubs.
    // we just disable that feature for more safety.

    return true;
  }
};
  
}}
  
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

/* ---- CPP/7zip/Archive/Zip/ZipOut.h ---- */
// ZipOut.h

#ifndef ZIP7_INC_ZIP_OUT_H
#define ZIP7_INC_ZIP_OUT_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

class CItemOut: public CItem
{
public:
  FILETIME Ntfs_MTime;
  FILETIME Ntfs_ATime;
  FILETIME Ntfs_CTime;
  bool Write_NtfsTime;
  bool Write_UnixTime;

  // It's possible that NtfsTime is not defined, but there is NtfsTime in Extra.
  
  CByteBuffer Name_Utf; // for Info-Zip (kIzUnicodeName) Extra

  size_t Get_UtfName_ExtraSize() const
  {
    const size_t size = Name_Utf.Size();
    if (size == 0)
      return 0;
    return 4 + 5 + size;
  }

  CItemOut():
      Write_NtfsTime(false),
      Write_UnixTime(false)
      {}
};


// COutArchive can throw CSystemException and COutBufferException

class COutArchive
{
  COutBuffer m_OutBuffer;
  CMyComPtr<IOutStream> m_Stream;

  UInt64 m_Base; // Base of archive (offset in output Stream)
  UInt64 m_CurPos; // Curent position in archive (relative from m_Base)
  UInt64 m_LocalHeaderPos; // LocalHeaderPos (relative from m_Base) for last WriteLocalHeader() call

  UInt32 m_LocalFileHeaderSize;
  UInt32 m_ExtraSize;
  bool m_IsZip64;

  void WriteBytes(const void *data, size_t size);
  void Write8(Byte b);
  void Write16(UInt16 val);
  void Write32(UInt32 val);
  void Write64(UInt64 val);
  void WriteNtfsTime(const FILETIME &ft)
  {
    Write32(ft.dwLowDateTime);
    Write32(ft.dwHighDateTime);
  }

  void WriteTimeExtra(const CItemOut &item, bool writeNtfs);
  void WriteUtfName(const CItemOut &item);
  void WriteExtra(const CExtraBlock &extra);
  void WriteCommonItemInfo(const CLocalItem &item, bool isZip64);
  void WriteCentralHeader(const CItemOut &item);

  void SeekToCurPos();
public:
  CMyComPtr<IStreamSetRestriction> SetRestriction;

  HRESULT ClearRestriction();
  HRESULT SetRestrictionFromCurrent();
  HRESULT Create(IOutStream *outStream);
  
  UInt64 GetCurPos() const { return m_CurPos; }

  void MoveCurPos(UInt64 distanceToMove)
  {
    m_CurPos += distanceToMove;
  }

  void WriteLocalHeader(CItemOut &item, bool needCheck = false);
  void WriteLocalHeader_Replace(CItemOut &item);

  void WriteDescriptor(const CItemOut &item);

  HRESULT WriteCentralDir(const CObjectVector<CItemOut> &items, const CByteBuffer *comment);

  void CreateStreamForCompressing(CMyComPtr<IOutStream> &outStream);
  void CreateStreamForCopying(CMyComPtr<ISequentialOutStream> &outStream);
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

/* ---- CPP/7zip/Archive/Zip/ZipCompressionMode.h ---- */
// CompressionMode.h

#ifndef ZIP7_INC_ZIP_COMPRESSION_MODE_H
#define ZIP7_INC_ZIP_COMPRESSION_MODE_H

// amalgamation: header emitted in prologue

#ifndef Z7_ST
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

const CMethodId kMethodId_ZipBase = 0x040100;
const CMethodId kMethodId_BZip2   = 0x040202;

struct CBaseProps: public CMultiMethodProps
{
  bool IsAesMode;
  Byte AesKeyMode;

  void Init()
  {
    CMultiMethodProps::Init();
    
    IsAesMode = false;
    AesKeyMode = 3;
  }
};

struct CCompressionMethodMode: public CBaseProps
{
  CRecordVector<Byte> MethodSequence;
  AString Password; // _Wipe
  bool Password_Defined;
  bool Force_SeqOutMode;
  bool DataSizeReduce_Defined;
  UInt64 DataSizeReduce;

  bool IsRealAesMode() const { return Password_Defined && IsAesMode; }

  CCompressionMethodMode()
  {
    Password_Defined = false;
    Force_SeqOutMode = false;
    DataSizeReduce_Defined = false;
    DataSizeReduce = 0;
  }

#ifdef Z7_CPP_IS_SUPPORTED_default
  CCompressionMethodMode(const CCompressionMethodMode &) = default;
  CCompressionMethodMode& operator =(const CCompressionMethodMode &) = default;
#endif
  ~CCompressionMethodMode() { Password.Wipe_and_Empty(); }
};

}}

#endif

/* ---- CPP/7zip/Archive/Zip/ZipHandler.h ---- */
// Zip/Handler.h

#ifndef ZIP7_INC_ZIP_HANDLER_H
#define ZIP7_INC_ZIP_HANDLER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

const unsigned kNumMethodNames1 = NFileHeader::NCompressionMethod::kZstdPk + 1;
const unsigned kMethodNames2Start = NFileHeader::NCompressionMethod::kZstdWz;
const unsigned kNumMethodNames2 = NFileHeader::NCompressionMethod::kWzAES + 1 - kMethodNames2Start;

extern const char * const kMethodNames1[kNumMethodNames1];
extern const char * const kMethodNames2[kNumMethodNames2];


class CHandler Z7_final:
  public IInArchive,
  // public IArchiveGetRawProps,
  public IOutArchive,
  public ISetProperties,
  Z7_PUBLIC_ISetCompressCodecsInfo_IFEC
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(IInArchive)
  // Z7_COM_QI_ENTRY(IArchiveGetRawProps)
  Z7_COM_QI_ENTRY(IOutArchive)
  Z7_COM_QI_ENTRY(ISetProperties)
  Z7_COM_QI_ENTRY_ISetCompressCodecsInfo_IFEC
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(IInArchive)
  // Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
  Z7_IFACE_COM7_IMP(IOutArchive)
  Z7_IFACE_COM7_IMP(ISetProperties)
  DECL_ISetCompressCodecsInfo

private:
  CObjectVector<CItemEx> m_Items;
  CInArchive m_Archive;

  CBaseProps _props;
  CHandlerTimeOptions TimeOptions;

  int m_MainMethod;
  bool m_ForceAesMode;

  bool _removeSfxBlock;
  bool m_ForceLocal;
  bool m_ForceUtf8;
  bool _force_SeqOutMode; // for creation
  bool _force_OpenSeq;
  bool _forceCodePage;
  UInt32 _specifiedCodePage;

  DECL_EXTERNAL_CODECS_VARS

  void InitMethodProps()
  {
    _props.Init();
    TimeOptions.Init();
    TimeOptions.Prec = k_PropVar_TimePrec_0;
    m_MainMethod = -1;
    m_ForceAesMode = false;
    _removeSfxBlock = false;
    m_ForceLocal = false;
    m_ForceUtf8 = false;
    _force_SeqOutMode = false;
    _force_OpenSeq = false;
    _forceCodePage = false;
    _specifiedCodePage = CP_OEMCP;
  }

  // void MarkAltStreams(CObjectVector<CItemEx> &items);

  HRESULT GetOutProperty(IArchiveUpdateCallback *callback, UInt32 callbackIndex, Int32 arcIndex, PROPID propID, PROPVARIANT *value);

public:
  CHandler();
};

}}

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

/* ---- CPP/7zip/Crypto/ZipCrypto.h ---- */
// Crypto/ZipCrypto.h

#ifndef ZIP7_INC_CRYPTO_ZIP_CRYPTO_H
#define ZIP7_INC_CRYPTO_ZIP_CRYPTO_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCrypto {
namespace NZip {

const unsigned kHeaderSize = 12;

/* ICompressFilter::Init() does nothing for this filter.
  Call to init:
    Encoder:
      CryptoSetPassword();
      WriteHeader();
    Decoder:
      [CryptoSetPassword();]
      ReadHeader();
      [CryptoSetPassword();] Init_and_GetCrcByte();
      [CryptoSetPassword();] Init_and_GetCrcByte();
*/

class CCipher:
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_1(ICryptoSetPassword)
  Z7_COM7F_IMP(Init())
public:
  Z7_IFACE_COM7_IMP(ICryptoSetPassword)
protected:
  UInt32 Key0;
  UInt32 Key1;
  UInt32 Key2;
  
  UInt32 KeyMem0;
  UInt32 KeyMem1;
  UInt32 KeyMem2;

  void RestoreKeys()
  {
    Key0 = KeyMem0;
    Key1 = KeyMem1;
    Key2 = KeyMem2;
  }

public:
  virtual ~CCipher()
  {
    Key0 = KeyMem0 =
    Key1 = KeyMem1 =
    Key2 = KeyMem2 = 0;
  }
};

class CEncoder Z7_final: public CCipher
{
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  HRESULT WriteHeader_Check16(ISequentialOutStream *outStream, UInt16 crc);
};

class CDecoder Z7_final: public CCipher
{
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  Byte _header[kHeaderSize];
  HRESULT ReadHeader(ISequentialInStream *inStream);
  void Init_BeforeDecode();
};

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

/* ---- CPP/7zip/Crypto/HmacSha1.h ---- */
// HmacSha1.h
// Implements HMAC-SHA-1 (RFC2104, FIPS-198)

#ifndef ZIP7_INC_CRYPTO_HMAC_SHA1_H
#define ZIP7_INC_CRYPTO_HMAC_SHA1_H

// amalgamation: header emitted in prologue

namespace NCrypto {
namespace NSha1 {

// Use:  SetKey(key, keySize); for () Update(data, size); FinalFull(mac);

class CHmac
{
  CContext _sha;
  CContext _sha2;
public:
  void SetKey(const Byte *key, size_t keySize);
  void Update(const Byte *data, size_t dataSize) { _sha.Update(data, dataSize); }
  
  // Final() : mac is recommended to be aligned for 4 bytes
  // GetLoopXorDigest1() : mac is required    to be aligned for 4 bytes
  // The caller can use: UInt32 mac[NSha1::kNumDigestWords] and typecast to (Byte *) and (void *);
  void Final(Byte *mac);
  void GetLoopXorDigest1(void *mac, UInt32 numIteration);
};

}}

#endif

/* ---- C/Aes.h ---- */
/* Aes.h -- AES encryption / decryption
2023-04-02 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_AES_H
#define ZIP7_INC_AES_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define AES_BLOCK_SIZE 16

/* Call AesGenTables one time before other AES functions */
void AesGenTables(void);

/* UInt32 pointers must be 16-byte aligned */

/* 16-byte (4 * 32-bit words) blocks: 1 (IV) + 1 (keyMode) + 15 (AES-256 roundKeys) */
#define AES_NUM_IVMRK_WORDS ((1 + 1 + 15) * 4)

/* aes - 16-byte aligned pointer to keyMode+roundKeys sequence */
/* keySize = 16 or 24 or 32 (bytes) */
typedef void (Z7_FASTCALL *AES_SET_KEY_FUNC)(UInt32 *aes, const Byte *key, unsigned keySize);
void Z7_FASTCALL Aes_SetKey_Enc(UInt32 *aes, const Byte *key, unsigned keySize);
void Z7_FASTCALL Aes_SetKey_Dec(UInt32 *aes, const Byte *key, unsigned keySize);

/* ivAes - 16-byte aligned pointer to iv+keyMode+roundKeys sequence: UInt32[AES_NUM_IVMRK_WORDS] */
void AesCbc_Init(UInt32 *ivAes, const Byte *iv); /* iv size is AES_BLOCK_SIZE */

/* data - 16-byte aligned pointer to data */
/* numBlocks - the number of 16-byte blocks in data array */
typedef void (Z7_FASTCALL *AES_CODE_FUNC)(UInt32 *ivAes, Byte *data, size_t numBlocks);

extern AES_CODE_FUNC g_AesCbc_Decode;
#ifndef Z7_SFX
extern AES_CODE_FUNC g_AesCbc_Encode;
extern AES_CODE_FUNC g_AesCtr_Code;
#define k_Aes_SupportedFunctions_HW     (1 << 2)
#define k_Aes_SupportedFunctions_HW_256 (1 << 3)
extern UInt32 g_Aes_SupportedFunctions_Flags;
#endif


#define Z7_DECLARE_AES_CODE_FUNC(funcName) \
    void Z7_FASTCALL funcName(UInt32 *ivAes, Byte *data, size_t numBlocks);

Z7_DECLARE_AES_CODE_FUNC (AesCbc_Encode)
Z7_DECLARE_AES_CODE_FUNC (AesCbc_Decode)
Z7_DECLARE_AES_CODE_FUNC (AesCtr_Code)

Z7_DECLARE_AES_CODE_FUNC (AesCbc_Encode_HW)
Z7_DECLARE_AES_CODE_FUNC (AesCbc_Decode_HW)
Z7_DECLARE_AES_CODE_FUNC (AesCtr_Code_HW)

Z7_DECLARE_AES_CODE_FUNC (AesCbc_Decode_HW_256)
Z7_DECLARE_AES_CODE_FUNC (AesCtr_Code_HW_256)

EXTERN_C_END

#endif

/* ---- CPP/7zip/Crypto/MyAes.h ---- */
// Crypto/MyAes.h

#ifndef ZIP7_INC_CRYPTO_MY_AES_H
#define ZIP7_INC_CRYPTO_MY_AES_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCrypto {

#ifdef Z7_EXTRACT_ONLY
#define Z7_IFACEN_IAesCoderSetFunctions(x)
#else
#define Z7_IFACEN_IAesCoderSetFunctions(x) \
  virtual bool SetFunctions(UInt32 algo) x
#endif


class CAesCoder:
  public ICompressFilter,
  public ICryptoProperties,
 #ifndef Z7_EXTRACT_ONLY
  public ICompressSetCoderProperties,
 #endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressFilter)
  Z7_COM_QI_ENTRY(ICryptoProperties)
 #ifndef Z7_EXTRACT_ONLY
  Z7_COM_QI_ENTRY(ICompressSetCoderProperties)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE
  
public:
  Z7_IFACE_COM7_IMP_NONFINAL(ICompressFilter)
  Z7_IFACE_COM7_IMP(ICryptoProperties)
private:
 #ifndef Z7_EXTRACT_ONLY
  Z7_IFACE_COM7_IMP(ICompressSetCoderProperties)
 #endif

protected:
  bool _keyIsSet;
  // bool _encodeMode;
  // bool _ctrMode;
  // unsigned _offset;
  unsigned _keySize;
  unsigned _ctrPos; // we need _ctrPos here for Init() / SetInitVector()
  AES_CODE_FUNC _codeFunc;
  AES_SET_KEY_FUNC _setKeyFunc;
private:
  // UInt32 _aes[AES_NUM_IVMRK_WORDS + 3];
  CAlignedBuffer1 _aes;

  Byte _iv[AES_BLOCK_SIZE];

  // UInt32 *Aes() { return _aes + _offset; }
protected:
  UInt32 *Aes() { return (UInt32 *)(void *)(Byte *)_aes; }

 Z7_IFACE_PURE(IAesCoderSetFunctions)

public:
  CAesCoder(
      // bool encodeMode,
      unsigned keySize
      // , bool ctrMode
      );
  virtual ~CAesCoder() {}   // we need virtual destructor for derived classes
  void SetKeySize(unsigned size) { _keySize = size; }
};


#ifndef Z7_EXTRACT_ONLY
struct CAesCbcEncoder: public CAesCoder
{
  CAesCbcEncoder(unsigned keySize = 0): CAesCoder(keySize)
  {
    _setKeyFunc = Aes_SetKey_Enc;
    _codeFunc = g_AesCbc_Encode;
  }
  Z7_IFACE_IMP(IAesCoderSetFunctions)
};
#endif

struct CAesCbcDecoder: public CAesCoder
{
  CAesCbcDecoder(unsigned keySize = 0): CAesCoder(keySize)
  {
    _setKeyFunc = Aes_SetKey_Dec;
    _codeFunc = g_AesCbc_Decode;
  }
  Z7_IFACE_IMP(IAesCoderSetFunctions)
};

#ifndef Z7_SFX
struct CAesCtrCoder: public CAesCoder
{
private:
  // unsigned _ctrPos;
  // Z7_IFACE_COM7_IMP(ICompressFilter)
  // Z7_COM7F_IMP(Init())
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  CAesCtrCoder(unsigned keySize = 0): CAesCoder(keySize)
  {
    _ctrPos = 0;
    _setKeyFunc = Aes_SetKey_Enc;
    _codeFunc = g_AesCtr_Code;
  }
  Z7_IFACE_IMP(IAesCoderSetFunctions)
};
#endif

}

#endif

/* ---- CPP/7zip/Crypto/WzAes.h ---- */
// Crypto/WzAes.h
/*
This code implements Brian Gladman's scheme
specified in "A Password Based File Encryption Utility":
  - AES encryption (128,192,256-bit) in Counter (CTR) mode.
  - HMAC-SHA1 authentication for encrypted data (10 bytes)
  - Keys are derived by PPKDF2(RFC2898)-HMAC-SHA1 from ASCII password and
    Salt (saltSize = aesKeySize / 2).
  - 2 bytes contain Password Verifier's Code
*/

#ifndef ZIP7_INC_CRYPTO_WZ_AES_H
#define ZIP7_INC_CRYPTO_WZ_AES_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCrypto {
namespace NWzAes {

/* ICompressFilter::Init() does nothing for this filter.

  Call to init:
    Encoder:
      CryptoSetPassword();
      WriteHeader();
    Decoder:
      [CryptoSetPassword();]
      ReadHeader();
      [CryptoSetPassword();] Init_and_CheckPassword();
      [CryptoSetPassword();] Init_and_CheckPassword();
*/

const UInt32 kPasswordSizeMax = 99; // 128;

const unsigned kSaltSizeMax = 16;
const unsigned kPwdVerifSize = 2;
const unsigned kMacSize = 10;

enum EKeySizeMode
{
  kKeySizeMode_AES128 = 1,
  kKeySizeMode_AES192 = 2,
  kKeySizeMode_AES256 = 3
};

struct CKeyInfo
{
  EKeySizeMode KeySizeMode;
  Byte Salt[kSaltSizeMax];
  Byte PwdVerifComputed[kPwdVerifSize];

  CByteBuffer Password;

  unsigned GetKeySize()  const { return (8 * KeySizeMode + 8); }
  unsigned GetSaltSize() const { return (4 * KeySizeMode + 4); }
  unsigned GetNumSaltWords() const { return (KeySizeMode + 1); }

  CKeyInfo(): KeySizeMode(kKeySizeMode_AES256) {}

  void Wipe()
  {
    Password.Wipe();
    Z7_memset_0_ARRAY(Salt);
    Z7_memset_0_ARRAY(PwdVerifComputed);
  }

  ~CKeyInfo() { Wipe(); }
};

/*
struct CAesCtr2
{
  unsigned pos;
  CAlignedBuffer aes;
  UInt32 *Aes() { return (UInt32 *)(Byte *)aes; }

  // unsigned offset;
  // UInt32 aes[4 + AES_NUM_IVMRK_WORDS + 3];
  // UInt32 *Aes() { return aes + offset; }
  CAesCtr2();
};

void AesCtr2_Init(CAesCtr2 *p);
void AesCtr2_Code(CAesCtr2 *p, Byte *data, SizeT size);
*/

class CBaseCoder:
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_1(ICryptoSetPassword)
  Z7_COM7F_IMP(Init())
public:
  Z7_IFACE_COM7_IMP(ICryptoSetPassword)
protected:
  CKeyInfo _key;

  // NSha1::CHmac _hmac;
  // NSha1::CHmac *Hmac() { return &_hmac; }
  CAlignedBuffer1 _hmacBuf;
  UInt32 _hmacOverCalc;

  NSha1::CHmac *Hmac() { return (NSha1::CHmac *)(void *)(Byte *)_hmacBuf; }

  // CAesCtr2 _aes;
  CAesCoder *_aesCoderSpec;
  CMyComPtr<ICompressFilter> _aesCoder;
  CBaseCoder():
    _hmacBuf(sizeof(NSha1::CHmac))
  {
    _aesCoderSpec = new CAesCtrCoder(32);
    _aesCoder = _aesCoderSpec;
  }

  void Init2();
public:
  unsigned GetHeaderSize() const { return _key.GetSaltSize() + kPwdVerifSize; }
  unsigned GetAddPackSize() const { return GetHeaderSize() + kMacSize; }

  bool SetKeyMode(unsigned mode)
  {
    if (mode < kKeySizeMode_AES128 || mode > kKeySizeMode_AES256)
      return false;
    _key.KeySizeMode = (EKeySizeMode)mode;
    return true;
  }

  virtual ~CBaseCoder() {}
};

class CEncoder Z7_final:
  public CBaseCoder
{
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  HRESULT WriteHeader(ISequentialOutStream *outStream);
  HRESULT WriteFooter(ISequentialOutStream *outStream);
};

class CDecoder Z7_final:
  public CBaseCoder
  // public ICompressSetDecoderProperties2
{
  Byte _pwdVerifFromArchive[kPwdVerifSize];
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  // Z7_IFACE_COM7_IMP(ICompressSetDecoderProperties2)
  HRESULT ReadHeader(ISequentialInStream *inStream);
  bool Init_and_CheckPassword();
  HRESULT CheckMac(ISequentialInStream *inStream, bool &isOK);
};

}}

#endif

/* ---- CPP/7zip/Archive/Zip/ZipAddCommon.h ---- */
// ZipAddCommon.h

#ifndef ZIP7_INC_ZIP_ADD_COMMON_H
#define ZIP7_INC_ZIP_ADD_COMMON_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

struct CCompressingResult
{
  UInt64 UnpackSize;
  UInt64 PackSize;
  UInt32 CRC;
  UInt16 Method;
  Byte ExtractVersion;
  bool DescriptorMode;
  bool LzmaEos;

  CCompressingResult()
  {
    // for GCC:
    UnpackSize = 0;
  }
};

class CAddCommon  MY_UNCOPYABLE
{
  CCompressionMethodMode _options;
  CMyComPtr2<ICompressCoder, NCompress::CCopyCoder> _copyCoder;

  CMyComPtr<ICompressCoder> _compressEncoder;
  Byte _compressExtractVersion;
  bool _isLzmaEos;

  CMyComPtr2<ISequentialOutStream, CFilterCoder> _cryptoStream;

  NCrypto::NZip::CEncoder *_filterSpec;
  NCrypto::NWzAes::CEncoder *_filterAesSpec;

  Byte *_buf;
  
  HRESULT CalcStreamCRC(ISequentialInStream *inStream, UInt32 &resultCRC);
public:
  // CAddCommon(const CCompressionMethodMode &options);
  CAddCommon();
  void SetOptions(const CCompressionMethodMode &options);
  ~CAddCommon();

  HRESULT Set_Pre_CompressionResult(bool inSeqMode, bool outSeqMode, UInt64 unpackSize,
      CCompressingResult &opRes) const;
  
  HRESULT Compress(
      DECL_EXTERNAL_CODECS_LOC_VARS
      ISequentialInStream *inStream, IOutStream *outStream,
      bool inSeqMode, bool outSeqMode,
      UInt32 fileTime,
      UInt64 expectedDataSize, bool expectedDataSize_IsConfirmed,
      ICompressProgressInfo *progress, CCompressingResult &opRes);
};

}}

#endif

/* ---- CPP/7zip/Archive/Zip/ZipUpdate.h ---- */
// ZipUpdate.h

#ifndef ZIP7_INC_ZIP_UPDATE_H
#define ZIP7_INC_ZIP_UPDATE_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

/*
struct CUpdateRange
{
  UInt64 Position;
  UInt64 Size;
  
  // CUpdateRange() {}
  CUpdateRange(UInt64 position, UInt64 size): Position(position), Size(size) {}
};
*/

struct CUpdateItem
{
  bool NewData;
  bool NewProps;
  bool IsDir;
  bool Write_NtfsTime;
  bool Write_UnixTime;
  // bool Write_UnixTime_ATime;
  bool IsUtf8;
  bool Size_WasSetFromStream;
  // bool IsAltStream;
  int IndexInArc;
  unsigned IndexInClient;
  UInt32 Attrib;
  UInt32 Time;
  UInt64 Size;
  AString Name;
  CByteBuffer Name_Utf;    // for Info-Zip (kIzUnicodeName) Extra
  CByteBuffer Comment;
  // bool Commented;
  // CUpdateRange CommentRange;
  FILETIME Ntfs_MTime;
  FILETIME Ntfs_ATime;
  FILETIME Ntfs_CTime;

  void Clear()
  {
    IsDir = false;
    
    Write_NtfsTime = false;
    Write_UnixTime = false;

    IsUtf8 = false;
    Size_WasSetFromStream = false;
    // IsAltStream = false;
    Time = 0;
    Size = 0;
    Name.Empty();
    Name_Utf.Free();
    Comment.Free();

    FILETIME_Clear(Ntfs_MTime);
    FILETIME_Clear(Ntfs_ATime);
    FILETIME_Clear(Ntfs_CTime);
  }

  CUpdateItem():
    IsDir(false),
    Write_NtfsTime(false),
    Write_UnixTime(false),
    IsUtf8(false),
    Size_WasSetFromStream(false),
    // IsAltStream(false),
    Time(0),
    Size(0)
    {}
};


struct CUpdateOptions
{
  bool Write_MTime;
  bool Write_ATime;
  bool Write_CTime;
};


HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    ISequentialOutStream *seqOutStream,
    CInArchive *inArchive, bool removeSfx,
    const CUpdateOptions &updateOptions,
    const CCompressionMethodMode &compressionMethodMode,
    IArchiveUpdateCallback *updateCallback);

}}

#endif

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Archive/Zip/ZipIn.cpp ================ */
// Archive/ZipIn.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

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
#define G64(offs, v) v = Get64(p + (offs))

namespace NArchive {
namespace NZip {

/* we try to use same size of Buffer (1 << 17) for all tasks.
   it allow to avoid reallocations and cache clearing. */

static const size_t kSeqBufferSize = (size_t)1 << 17;

/*
Open()
{
  _inBufMode = false;
  ReadVols()
    FindCd();
      TryEcd64()
  SeekToVol()
  FindMarker()
    _inBufMode = true;
  ReadHeaders()
    _inBufMode = false;
    ReadCd()
      FindCd()
        TryEcd64()
      TryReadCd()
      {
        SeekToVol();
        _inBufMode = true;
      }
    _inBufMode = true;
    ReadLocals()
    ReadCdItem()
    ....
}
FindCd() writes to Buffer without touching (_inBufMode)
*/

/*
  if (not defined ZIP_SELF_CHECK) : it reads CD and if error in first pass CD reading, it reads LOCALS-CD-MODE
  if (    defined ZIP_SELF_CHECK) : it always reads CD and LOCALS-CD-MODE
  use ZIP_SELF_CHECK to check LOCALS-CD-MODE for any zip archive
*/

// #define ZIP_SELF_CHECK


struct CEcd
{
  UInt16 ThisDisk;
  UInt16 CdDisk;
  UInt16 NumEntries_in_ThisDisk;
  UInt16 NumEntries;
  UInt32 Size;
  UInt32 Offset;
  UInt16 CommentSize;
  
  bool IsEmptyArc() const
  {
    return ThisDisk == 0
        && CdDisk == 0
        && NumEntries_in_ThisDisk == 0
        && NumEntries == 0
        && Size == 0
        && Offset == 0 // test it
    ;
  }

  void Parse(const Byte *p); // (p) doesn't include signature
};

void CEcd::Parse(const Byte *p)
{
  // (p) doesn't include signature
  G16(0, ThisDisk);
  G16(2, CdDisk);
  G16(4, NumEntries_in_ThisDisk);
  G16(6, NumEntries);
  G32(8, Size);
  G32(12, Offset);
  G16(16, CommentSize);
}


void CCdInfo::ParseEcd32(const Byte *p)
{
  IsFromEcd64 = false;
  // (p) includes signature
  p += 4;
  G16(0, ThisDisk);
  G16(2, CdDisk);
  G16(4, NumEntries_in_ThisDisk);
  G16(6, NumEntries);
  G32(8, Size);
  G32(12, Offset);
  G16(16, CommentSize);
}

void CCdInfo::ParseEcd64e(const Byte *p)
{
  IsFromEcd64 = true;
  // (p) exclude signature
  G16(0, VersionMade);
  G16(2, VersionNeedExtract);
  G32(4, ThisDisk);
  G32(8, CdDisk);

  G64(12, NumEntries_in_ThisDisk);
  G64(20, NumEntries);
  G64(28, Size);
  G64(36, Offset);
}


struct CLocator
{
  UInt32 Ecd64Disk;
  UInt32 NumDisks;
  UInt64 Ecd64Offset;
  
  CLocator(): Ecd64Disk(0), NumDisks(0), Ecd64Offset(0) {}

  void Parse(const Byte *p)
  {
    G32(0, Ecd64Disk);
    G64(4, Ecd64Offset);
    G32(12, NumDisks);
  }

  bool IsEmptyArc() const
  {
    return Ecd64Disk == 0 && NumDisks == 0 && Ecd64Offset == 0;
  }
};

  


void CInArchive::ClearRefs()
{
  StreamRef.Release();
  Stream = NULL;
  StartStream = NULL;
  Callback = NULL;

  Vols.Clear();
}

void CInArchive::Close()
{
  _cnt = 0;
  DisableBufMode();

  IsArcOpen = false;

  IsArc = false;
  IsZip64 = false;
  
  IsApk = false;
  IsCdUnsorted = false;

  HeadersError = false;
  HeadersWarning = false;
  ExtraMinorError = false;
  
  UnexpectedEnd = false;
  LocalsWereRead = false;
  LocalsCenterMerged = false;
  NoCentralDir = false;
  Overflow32bit = false;
  Cd_NumEntries_Overflow_16bit = false;
  
  MarkerIsFound = false;
  MarkerIsSafe = false;

  IsMultiVol = false;
  UseDisk_in_SingleVol = false;
  EcdVolIndex = 0;
 
  ArcInfo.Clear();

  ClearRefs();
}



HRESULT CInArchive::Seek_SavePos(UInt64 offset)
{
  // InitBuf();
  // if (!Stream) return S_FALSE;
  return Stream->Seek((Int64)offset, STREAM_SEEK_SET, &_streamPos);
}


/* SeekToVol() will keep the cached mode, if new volIndex is
   same Vols.StreamIndex volume, and offset doesn't go out of cached region */

HRESULT CInArchive::SeekToVol(int volIndex, UInt64 offset)
{
  if (volIndex != Vols.StreamIndex)
  {
    if (IsMultiVol && volIndex >= 0)
    {
      if ((unsigned)volIndex >= Vols.Streams.Size())
        return S_FALSE;
      if (!Vols.Streams[(unsigned)volIndex].Stream)
        return S_FALSE;
      Stream = Vols.Streams[(unsigned)volIndex].Stream;
    }
    else if (volIndex == -2)
    {
      if (!Vols.ZipStream)
        return S_FALSE;
      Stream = Vols.ZipStream;
    }
    else
      Stream = StartStream;
    Vols.StreamIndex = volIndex;
  }
  else
  {
    if (offset <= _streamPos)
    {
      const UInt64 back = _streamPos - offset;
      if (back <= _bufCached)
      {
        _bufPos = _bufCached - (size_t)back;
        return S_OK;
      }
    }
  }
  InitBuf();
  return Seek_SavePos(offset);
}


HRESULT CInArchive::AllocateBuffer(size_t size)
{
  if (size <= Buffer.Size())
    return S_OK;
  /* in cached mode virtual_pos is not equal to phy_pos (_streamPos)
     so we change _streamPos and do Seek() to virtual_pos before cache clearing */
  if (_bufPos != _bufCached)
  {
    RINOK(Seek_SavePos(GetVirtStreamPos()))
  }
  InitBuf();
  Buffer.AllocAtLeast(size);
  if (!Buffer.IsAllocated())
    return E_OUTOFMEMORY;
  return S_OK;
}

// ---------- ReadFromCache ----------
// reads from cache and from Stream
// move to next volume can be allowed if (CanStartNewVol) and only before first byte reading

HRESULT CInArchive::ReadFromCache(Byte *data, unsigned size, unsigned &processed)
{
  HRESULT result = S_OK;
  processed = 0;

  for (;;)
  {
    if (size == 0)
      return S_OK;
    
    const size_t avail = GetAvail();

    if (avail != 0)
    {
      unsigned cur = size;
      if (cur > avail)
        cur = (unsigned)avail;
      memcpy(data, (const Byte *)Buffer + _bufPos, cur);

      data += cur;
      size -= cur;
      processed += cur;

      _bufPos += cur;
      _cnt += cur;

      CanStartNewVol = false;
      
      continue;
    }

    InitBuf();

    if (_inBufMode)
    {
      UInt32 cur = 0;
      result = Stream->Read(Buffer, (UInt32)Buffer.Size(), &cur);
      _bufPos = 0;
      _bufCached = cur;
      _streamPos += cur;
      if (cur != 0)
        CanStartNewVol = false;
      if (result != S_OK)
        break;
      if (cur != 0)
        continue;
    }
    else
    {
      size_t cur = size;
      result = ReadStream(Stream, data, &cur);
      data += cur;
      size -= (unsigned)cur;
      processed += (unsigned)cur;
      _streamPos += cur;
      _cnt += cur;
      if (cur != 0)
      {
        CanStartNewVol = false;
        break;
      }
      if (result != S_OK)
        break;
    }

    if (   !IsMultiVol
        || !CanStartNewVol
        || Vols.StreamIndex < 0
        || (unsigned)Vols.StreamIndex + 1 >= Vols.Streams.Size())
      break;

    const CVols::CSubStreamInfo &s = Vols.Streams[(unsigned)Vols.StreamIndex + 1];
    if (!s.Stream)
      break;
    result = s.SeekToStart();
    if (result != S_OK)
      break;
    Vols.StreamIndex++;
    _streamPos = 0;
    // Vols.NeedSeek = false;

    Stream = s.Stream;
  }

  return result;
}


HRESULT CInArchive::ReadFromCache_FALSE(Byte *data, unsigned size)
{
  unsigned processed;
  HRESULT res = ReadFromCache(data, size, processed);
  if (res == S_OK && size != processed)
    return S_FALSE;
  return res;
}


static bool CheckDosTime(UInt32 dosTime)
{
  if (dosTime == 0)
    return true;
  unsigned month = (dosTime >> 21) & 0xF;
  unsigned day = (dosTime >> 16) & 0x1F;
  unsigned hour = (dosTime >> 11) & 0x1F;
  unsigned min = (dosTime >> 5) & 0x3F;
  unsigned sec = (dosTime & 0x1F) * 2;
  if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
    return false;
  return true;
}

API_FUNC_IsArc IsArc_Zip(const Byte *p, size_t size)
{
  if (size < 8)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != 'P')
    return k_IsArc_Res_NO;

  UInt32 sig = Get32(p);

  if (sig == NSignature::kNoSpan || sig == NSignature::kSpan)
  {
    p += 4;
    size -= 4;
  }

  sig = Get32(p);

  if (sig == NSignature::kEcd64)
  {
    if (size < kEcd64_FullSize)
      return k_IsArc_Res_NEED_MORE;

    const UInt64 recordSize = Get64(p + 4);
    if (   recordSize < kEcd64_MainSize
        || recordSize > kEcd64_MainSize + (1 << 20))
      return k_IsArc_Res_NO;
    CCdInfo cdInfo;
    cdInfo.ParseEcd64e(p + 12);
    if (!cdInfo.IsEmptyArc())
      return k_IsArc_Res_NO;
    return k_IsArc_Res_YES; // k_IsArc_Res_YES_2;
  }

  if (sig == NSignature::kEcd)
  {
    if (size < kEcdSize)
      return k_IsArc_Res_NEED_MORE;
    CEcd ecd;
    ecd.Parse(p + 4);
    // if (ecd.cdSize != 0)
    if (!ecd.IsEmptyArc())
      return k_IsArc_Res_NO;
    return k_IsArc_Res_YES; // k_IsArc_Res_YES_2;
  }
 
  if (sig != NSignature::kLocalFileHeader)
    return k_IsArc_Res_NO;

  if (size < kLocalHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  
  p += 4;

  {
    const unsigned kPureHeaderSize = kLocalHeaderSize - 4;
    unsigned i;
    for (i = 0; i < kPureHeaderSize && p[i] == 0; i++);
    if (i == kPureHeaderSize)
      return k_IsArc_Res_NEED_MORE;
  }

  /*
  if (p[0] >= 128) // ExtractVersion.Version;
    return k_IsArc_Res_NO;
  */

  // ExtractVersion.Version = p[0];
  // ExtractVersion.HostOS = p[1];
  // Flags = Get16(p + 2);
  // Method = Get16(p + 4);
  /*
  // 9.33: some zip archives contain incorrect value in timestamp. So we don't check it now
  UInt32 dosTime = Get32(p + 6);
  if (!CheckDosTime(dosTime))
    return k_IsArc_Res_NO;
  */
  // Crc = Get32(p + 10);
  // PackSize = Get32(p + 14);
  // Size = Get32(p + 18);
  const unsigned nameSize = Get16(p + 22);
  unsigned extraSize = Get16(p + 24);
  const UInt32 extraOffset = kLocalHeaderSize + (UInt32)nameSize;
  
  /*
  // 21.02: fixed. we don't use the following check
  if (extraOffset + extraSize > (1 << 16))
    return k_IsArc_Res_NO;
  */

  p -= 4;

  {
    size_t rem = size - kLocalHeaderSize;
    if (rem > nameSize)
      rem = nameSize;
    const Byte *p2 = p + kLocalHeaderSize;
    for (size_t i = 0; i < rem; i++)
      if (p2[i] == 0)
      {
        // we support some "bad" zip archives that contain zeros after name
        for (size_t k = i + 1; k < rem; k++)
          if (p2[k] != 0)
            return k_IsArc_Res_NO;
        break;
        /*
        if (i != nameSize - 1)
          return k_IsArc_Res_NO;
        */
      }
  }

  if (size < extraOffset)
    return k_IsArc_Res_NEED_MORE;

  if (extraSize > 0)
  {
    p += extraOffset;
    size -= extraOffset;
    while (extraSize != 0)
    {
      if (extraSize < 4)
      {
        // 7-Zip before 9.31 created incorrect WzAES Extra in folder's local headers.
        // so we return k_IsArc_Res_YES to support such archives.
        // return k_IsArc_Res_NO; // do we need to support such extra ?
        return k_IsArc_Res_YES;
      }
      if (size < 4)
        return k_IsArc_Res_NEED_MORE;
      unsigned dataSize = Get16(p + 2);
      size -= 4;
      extraSize -= 4;
      p += 4;
      if (dataSize > extraSize)
      {
        // It can be error on header.
        // We want to support such rare case bad archives.
        // We use additional checks to reduce false-positive probability.
        if (nameSize == 0
            || nameSize > (1 << 9)
            || extraSize > (1 << 9))
          return k_IsArc_Res_NO;
        return k_IsArc_Res_YES;
      }
      if (dataSize > size)
        return k_IsArc_Res_NEED_MORE;
      size -= dataSize;
      extraSize -= dataSize;
      p += dataSize;
    }
  }
  
  return k_IsArc_Res_YES;
}

static UInt32 IsArc_Zip_2(const Byte *p, size_t size, bool isFinal)
{
  UInt32 res = IsArc_Zip(p, size);
  if (res == k_IsArc_Res_NEED_MORE && isFinal)
    return k_IsArc_Res_NO;
  return res;
}


  
/* FindPK_4() is allowed to access data up to and including &limit[3].
   limit[4] access is not allowed.
  return:
    (return_ptr <  limit) : "PK" was found at (return_ptr)
    (return_ptr >= limit) : limit was reached or crossed. So no "PK" found before limit
*/
Z7_NO_INLINE
static const Byte *FindPK_4(const Byte *p, const Byte *limit)
{
  for (;;)
  {
    for (;;)
    {
      if (p >= limit)
        return limit;
      Byte b = p[1];
      if (b == 0x4B) { if (p[0] == 0x50) { return p;     } p += 1; break; }
      if (b == 0x50) { if (p[2] == 0x4B) { return p + 1; } p += 2; break; }
      b = p[3];
      p += 4;
      if (b == 0x4B) { if (p[-2]== 0x50) { return p - 2; } p -= 1; break; }
      if (b == 0x50) { if (p[0] == 0x4B) { return p - 1; }         break; }
    }
  }
  /*
  for (;;)
  {
    for (;;)
    {
      if (p >= limit)
        return limit;
      if (*p++ == 0x50) break;
      if (*p++ == 0x50) break;
      if (*p++ == 0x50) break;
      if (*p++ == 0x50) break;
    }
    if (*p == 0x4B)
      return p - 1;
  }
  */
}


/*
---------- FindMarker ----------
returns:
  S_OK:
    ArcInfo.MarkerVolIndex : volume of marker
    ArcInfo.MarkerPos   : Pos of first signature
    ArcInfo.MarkerPos2  : Pos of main signature (local item signature in most cases)
    _streamPos          : stream pos
    _cnt                : The number of virtal Bytes after start of search to offset after signature
    _signature          : main signature
 
  S_FALSE: can't find marker, or there is some non-zip data after marker

  Error code: stream reading error.
*/

HRESULT CInArchive::FindMarker(const UInt64 *searchLimit)
{
  ArcInfo.MarkerPos = GetVirtStreamPos();
  ArcInfo.MarkerPos2 = ArcInfo.MarkerPos;
  ArcInfo.MarkerVolIndex = Vols.StreamIndex;

  _cnt = 0;

  CanStartNewVol = false;

  if (searchLimit && *searchLimit == 0)
  {
    Byte startBuf[kMarkerSize];
    RINOK(ReadFromCache_FALSE(startBuf, kMarkerSize))

    UInt32 marker = Get32(startBuf);
    _signature = marker;

    if (   marker == NSignature::kNoSpan
        || marker == NSignature::kSpan)
    {
      RINOK(ReadFromCache_FALSE(startBuf, kMarkerSize))
      _signature = Get32(startBuf);
    }
      
    if (   _signature != NSignature::kEcd
        && _signature != NSignature::kEcd64
        && _signature != NSignature::kLocalFileHeader)
      return S_FALSE;

    ArcInfo.MarkerPos2 = GetVirtStreamPos() - 4;
    ArcInfo.IsSpanMode = (marker == NSignature::kSpan);

    // we use weak test in case of (*searchLimit == 0)
    // since error will be detected later in Open function
    return S_OK;
  }

  // zip specification: (_zip_header_size < (1 << 16))
  // so we need such size to check header
  const size_t kCheckSize = (size_t)1 << 16;
  const size_t kBufSize   = (size_t)1 << 17; // (kBufSize must be > kCheckSize)

  RINOK(AllocateBuffer(kBufSize))

  _inBufMode = true;

  UInt64 progressPrev = 0;

  for (;;)
  {
    RINOK(LookAhead(kBufSize))
    
    const size_t avail = GetAvail();
    
    size_t limitPos;
    // (avail > kBufSize) is possible, if (Buffer.Size() > kBufSize)
    const bool isFinished = (avail < kBufSize);
    if (isFinished)
    {
      const unsigned kMinAllowed = 4;
      if (avail <= kMinAllowed)
      {
        if (   !IsMultiVol
            || Vols.StreamIndex < 0
            || (unsigned)Vols.StreamIndex + 1 >= Vols.Streams.Size())
          break;

        SkipLookahed(avail);

        const CVols::CSubStreamInfo &s = Vols.Streams[(unsigned)Vols.StreamIndex + 1];
        if (!s.Stream)
          break;
        
        RINOK(s.SeekToStart())
        
        InitBuf();
        Vols.StreamIndex++;
        _streamPos = 0;
        Stream = s.Stream;
        continue;
      }
      limitPos = avail - kMinAllowed;
    }
    else
      limitPos = (avail - kCheckSize);

    // we don't check at (limitPos) for good fast aligned operations

    if (searchLimit)
    {
      if (_cnt > *searchLimit)
        break;
      UInt64 rem = *searchLimit - _cnt;
      if (limitPos > rem)
        limitPos = (size_t)rem + 1;
    }

    if (limitPos == 0)
      break;

    const Byte * const pStart = Buffer + _bufPos;
    const Byte * p = pStart;
    const Byte * const limit = pStart + limitPos;
   
    for (;; p++)
    {
      p = FindPK_4(p, limit);
      if (p >= limit)
        break;
      size_t rem = (size_t)(pStart + avail - p);
      /* 22.02 : we limit check size with kCheckSize to be consistent for
         any different combination of _bufPos in Buffer and size of Buffer. */
      if (rem > kCheckSize)
        rem = kCheckSize;
      const UInt32 res = IsArc_Zip_2(p, rem, isFinished);
      if (res != k_IsArc_Res_NO)
      {
        if (rem < kMarkerSize)
          return S_FALSE;
        _signature = Get32(p);
        SkipLookahed((size_t)(p - pStart));
        ArcInfo.MarkerVolIndex = Vols.StreamIndex;
        ArcInfo.MarkerPos = GetVirtStreamPos();
        ArcInfo.MarkerPos2 = ArcInfo.MarkerPos;
        SkipLookahed(4);
        if (   _signature == NSignature::kNoSpan
            || _signature == NSignature::kSpan)
        {
          if (rem < kMarkerSize * 2)
            return S_FALSE;
          ArcInfo.IsSpanMode = (_signature == NSignature::kSpan);
          _signature = Get32(p + 4);
          ArcInfo.MarkerPos2 += 4;
          SkipLookahed(4);
        }
        return S_OK;
      }
    }

    if (!IsMultiVol && isFinished)
      break;

    SkipLookahed((size_t)(p - pStart));

    if (Callback && (_cnt - progressPrev) >= ((UInt32)1 << 23))
    {
      progressPrev = _cnt;
      // const UInt64 numFiles64 = 0;
      RINOK(Callback->SetCompleted(NULL, &_cnt))
    }
  }
  
  return S_FALSE;
}


/*
---------- IncreaseRealPosition ----------
moves virtual offset in virtual stream.
changing to new volumes is allowed
*/

HRESULT CInArchive::IncreaseRealPosition(UInt64 offset, bool &isFinished)
{
  isFinished = false;

  for (;;)
  {
    const size_t avail = GetAvail();
    
    if (offset <= avail)
    {
      _bufPos += (size_t)offset;
      _cnt += offset;
      return S_OK;
    }
    
    _cnt += avail;
    offset -= avail;
    
    _bufCached = 0;
    _bufPos = 0;
    
    if (!_inBufMode)
      break;
  
    CanStartNewVol = true;
    LookAhead(1);

    if (GetAvail() == 0)
      return S_OK;
  }

  // cache is empty

  if (!IsMultiVol)
  {
    _cnt += offset;
    return Stream->Seek((Int64)offset, STREAM_SEEK_CUR, &_streamPos);
  }

  for (;;)
  {
    if (offset == 0)
      return S_OK;
    
    if (Vols.StreamIndex < 0)
      return S_FALSE;
    if ((unsigned)Vols.StreamIndex >= Vols.Streams.Size())
    {
      isFinished = true;
      return S_OK;
    }
    {
      const CVols::CSubStreamInfo &s = Vols.Streams[(unsigned)Vols.StreamIndex];
      if (!s.Stream)
      {
        isFinished = true;
        return S_OK;
      }
      if (_streamPos > s.Size)
        return S_FALSE;
      const UInt64 rem = s.Size - _streamPos;
      if ((UInt64)offset <= rem)
      {
        _cnt += offset;
        return Stream->Seek((Int64)offset, STREAM_SEEK_CUR, &_streamPos);
      }
      RINOK(Seek_SavePos(s.Size))
      offset -= rem;
      _cnt += rem;
    }
    
    Stream = NULL;
    _streamPos = 0;
    Vols.StreamIndex++;
    if ((unsigned)Vols.StreamIndex >= Vols.Streams.Size())
    {
      isFinished = true;
      return S_OK;
    }
    const CVols::CSubStreamInfo &s2 = Vols.Streams[(unsigned)Vols.StreamIndex];
    if (!s2.Stream)
    {
      isFinished = true;
      return S_OK;
    }
    Stream = s2.Stream;
    RINOK(Seek_SavePos(0))
  }
}



/*
---------- LookAhead ----------
Reads data to buffer, if required.

It can read from volumes as long as Buffer.Size().
But it moves to new volume, only if it's required to provide minRequired bytes in buffer.

in:
  (minRequired <= Buffer.Size())

return:
  S_OK : if (GetAvail() < minRequired) after function return, it's end of stream(s) data, or no new volume stream.
  Error codes: IInStream::Read() error or IInStream::Seek() error for multivol
*/

HRESULT CInArchive::LookAhead(size_t minRequired)
{
  for (;;)
  {
    const size_t avail = GetAvail();

    if (minRequired <= avail)
      return S_OK;
    
    if (_bufPos != 0)
    {
      if (avail != 0)
        memmove(Buffer, Buffer + _bufPos, avail);
      _bufPos = 0;
      _bufCached = avail;
    }

    const size_t pos = _bufCached;
    UInt32 processed = 0;
    HRESULT res = Stream->Read(Buffer + pos, (UInt32)(Buffer.Size() - pos), &processed);
    _streamPos += processed;
    _bufCached += processed;

    if (res != S_OK)
      return res;

    if (processed != 0)
      continue;

    if (   !IsMultiVol
        || !CanStartNewVol
        || Vols.StreamIndex < 0
        || (unsigned)Vols.StreamIndex + 1 >= Vols.Streams.Size())
      return S_OK;

    const CVols::CSubStreamInfo &s = Vols.Streams[(unsigned)Vols.StreamIndex + 1];
    if (!s.Stream)
      return S_OK;
    
    RINOK(s.SeekToStart())

    Vols.StreamIndex++;
    _streamPos = 0;
    Stream = s.Stream;
    // Vols.NeedSeek = false;
  }
}


class CUnexpectEnd {};


/*
---------- SafeRead ----------

reads data of exact size from stream(s)

in:
  _inBufMode
  if (CanStartNewVol) it can go to next volume before first byte reading, if there is end of volume data.

in, out:
  _streamPos  :  position in Stream
  Stream
  Vols  :  if (IsMultiVol)
  _cnt

out:
  (CanStartNewVol == false), if some data was read

return:
  S_OK : success reading of requested data

exceptions:
  CSystemException() - stream reading error
  CUnexpectEnd()  :  could not read data of requested size
*/

void CInArchive::SafeRead(Byte *data, unsigned size)
{
  unsigned processed;
  HRESULT result = ReadFromCache(data, size, processed);
  if (result != S_OK)
    throw CSystemException(result);
  if (size != processed)
    throw CUnexpectEnd();
}

void CInArchive::ReadBuffer(CByteBuffer &buffer, unsigned size)
{
  buffer.Alloc(size);
  if (size != 0)
    SafeRead(buffer, size);
}

// Byte CInArchive::ReadByte  () { Byte b;      SafeRead(&b, 1); return b; }
// UInt16 CInArchive::ReadUInt16() { Byte buf[2]; SafeRead(buf, 2); return Get16(buf); }
UInt32 CInArchive::ReadUInt32() { Byte buf[4]; SafeRead(buf, 4); return Get32(buf); }
UInt64 CInArchive::ReadUInt64() { Byte buf[8]; SafeRead(buf, 8); return Get64(buf); }

void CInArchive::ReadSignature()
{
  CanStartNewVol = true;
  _signature = ReadUInt32();
  // CanStartNewVol = false; // it's already changed in SafeRead
}


// we Skip() inside headers only, so no need for stream change in multivol.

void CInArchive::Skip(size_t num)
{
  while (num != 0)
  {
    const unsigned kBufSize = (size_t)1 << 10;
    Byte buf[kBufSize];
    unsigned step = kBufSize;
    if (step > num)
      step = (unsigned)num;
    SafeRead(buf, step);
    num -= step;
  }
}

/*
HRESULT CInArchive::Callback_Completed(unsigned numFiles)
{
  const UInt64 numFiles64 = numFiles;
  return Callback->SetCompleted(&numFiles64, &_cnt);
}
*/

HRESULT CInArchive::Skip64(UInt64 num, unsigned numFiles)
{
  if (num == 0)
    return S_OK;

  for (;;)
  {
    size_t step = (size_t)1 << 24;
    if (step > num)
      step = (size_t)num;
    Skip(step);
    num -= step;
    if (num == 0)
      return S_OK;
    if (Callback)
    {
      const UInt64 numFiles64 = numFiles;
      RINOK(Callback->SetCompleted(&numFiles64, &_cnt))
    }
  }
}


bool CInArchive::ReadFileName(unsigned size, AString &s)
{
  if (size == 0)
  {
    s.Empty();
    return true;
  }
  char *p = s.GetBuf(size);
  SafeRead((Byte *)p, size);
  unsigned i = size;
  do
  {
    if (p[i - 1] != 0)
      break;
  }
  while (--i);
  s.ReleaseBuf_CalcLen(size);
  return s.Len() == i;
}


#define ZIP64_IS_32_MAX(n) ((n) == 0xFFFFFFFF)
#define ZIP64_IS_16_MAX(n) ((n) == 0xFFFF)


bool CInArchive::ReadExtra(const CLocalItem &item, unsigned extraSize, CExtraBlock &extra,
    UInt64 &unpackSize, UInt64 &packSize,
    CItem *cdItem)
{
  extra.Clear();
  
  while (extraSize >= 4)
  {
    CExtraSubBlock subBlock;
    const UInt32 pair = ReadUInt32();
    subBlock.ID = (pair & 0xFFFF);
    unsigned size = (unsigned)(pair >> 16);
    // const unsigned origSize = size;
    
    extraSize -= 4;
    
    if (size > extraSize)
    {
      // it's error in extra
      HeadersWarning = true;
      extra.Error = true;
      Skip(extraSize);
      return false;
    }
 
    extraSize -= size;
    
    if (subBlock.ID == NFileHeader::NExtraID::kZip64)
    {
      extra.IsZip64 = true;
      bool isOK = true;

      if (!cdItem
          && size == 16
          && !ZIP64_IS_32_MAX(unpackSize)
          && !ZIP64_IS_32_MAX(packSize))
      {
        /* Win10 Explorer's "Send to Zip" for big (3500 MiB) files
           creates Zip64 Extra in local file header.
           But if both uncompressed and compressed sizes are smaller than 4 GiB,
           Win10 doesn't store 0xFFFFFFFF in 32-bit fields as expected by zip specification.
           21.04: we ignore these minor errors in Win10 zip archives. */
        if (ReadUInt64() != unpackSize)
          isOK = false;
        if (ReadUInt64() != packSize)
          isOK = false;
        size = 0;
      }
      else
      {
        if (ZIP64_IS_32_MAX(unpackSize))
          { if (size < 8) isOK = false; else { size -= 8; unpackSize = ReadUInt64(); }}
      
        if (isOK && ZIP64_IS_32_MAX(packSize))
          { if (size < 8) isOK = false; else { size -= 8; packSize = ReadUInt64(); }}
      
        if (cdItem)
        {
          if (isOK)
          {
            if (ZIP64_IS_32_MAX(cdItem->LocalHeaderPos))
              { if (size < 8) isOK = false; else { size -= 8; cdItem->LocalHeaderPos = ReadUInt64(); }}
            /*
            else if (size == 8)
            {
              size -= 8;
              const UInt64 v = ReadUInt64();
              // soong_zip, an AOSP tool (written in the Go) writes incorrect value.
              // we can ignore that minor error here
              if (v != cdItem->LocalHeaderPos)
                isOK = false; // ignore error
              // isOK = false; // force error
            }
            */
          }
         
          if (isOK && ZIP64_IS_16_MAX(cdItem->Disk))
            { if (size < 4) isOK = false; else { size -= 4; cdItem->Disk = ReadUInt32(); }}
        }
      }
    
      // we can ignore errors, when some zip archiver still write all fields to zip64 extra in local header
      // if (&& (cdItem || !isOK || origSize != 8 * 3 + 4 || size != 8 * 1 + 4))
      if (!isOK || size != 0)
      {
        HeadersWarning = true;
        extra.Error = true;
        extra.IsZip64_Error = true;
      }
      Skip(size);
    }
    else
    {
      ReadBuffer(subBlock.Data, size);
      extra.SubBlocks.Add(subBlock);
      if (subBlock.ID == NFileHeader::NExtraID::kIzUnicodeName)
      {
        if (!subBlock.CheckIzUnicode(item.Name))
          extra.Error = true;
      }
    }
  }

  if (extraSize != 0)
  {
    ExtraMinorError = true;
    extra.MinorError = true;
    // 7-Zip before 9.31 created incorrect WzAES Extra in folder's local headers.
    // so we don't return false, but just set warning flag
    // return false;
    Skip(extraSize);
  }

  return true;
}


bool CInArchive::ReadLocalItem(CItemEx &item)
{
  item.Disk = 0;
  if (IsMultiVol && Vols.StreamIndex >= 0)
    item.Disk = (UInt32)Vols.StreamIndex;
  const unsigned kPureHeaderSize = kLocalHeaderSize - 4;
  Byte p[kPureHeaderSize];
  SafeRead(p, kPureHeaderSize);
  {
    unsigned i;
    for (i = 0; i < kPureHeaderSize && p[i] == 0; i++);
    if (i == kPureHeaderSize)
      return false;
  }

  item.ExtractVersion.Version = p[0];
  item.ExtractVersion.HostOS = p[1];
  G16(2, item.Flags);
  G16(4, item.Method);
  G32(6, item.Time);
  G32(10, item.Crc);
  G32(14, item.PackSize);
  G32(18, item.Size);
  const unsigned nameSize = Get16(p + 22);
  const unsigned extraSize = Get16(p + 24);
  bool isOkName = ReadFileName(nameSize, item.Name);
  item.LocalFullHeaderSize = kLocalHeaderSize + (UInt32)nameSize + extraSize;
  item.DescriptorWasRead = false;

  /*
  if (item.IsDir())
    item.Size = 0; // check It
  */

  if (extraSize > 0)
  {
    if (!ReadExtra(item, extraSize, item.LocalExtra, item.Size, item.PackSize, NULL))
    {
      /* Most of archives are OK for Extra. But there are some rare cases
         that have error. And if error in first item, it can't open archive.
         So we ignore that error */
      // return false;
    }
  }
  
  if (!CheckDosTime(item.Time))
  {
    HeadersWarning = true;
    // return false;
  }
  
  if (item.Name.Len() != nameSize)
  {
    // we support some "bad" zip archives that contain zeros after name
    if (!isOkName)
      return false;
    HeadersWarning = true;
  }
  
  // return item.LocalFullHeaderSize <= ((UInt32)1 << 16);
  return true;
}


static bool FlagsAreSame(const CItem &i1, const CItem &i2_cd)
{
  if (i1.Method != i2_cd.Method)
    return false;
  
  UInt32 mask = i1.Flags ^ i2_cd.Flags;
  if (mask == 0)
    return true;
  switch (i1.Method)
  {
    case NFileHeader::NCompressionMethod::kDeflate:
      mask &= 0x7FF9;
      break;
    default:
      if (i1.Method <= NFileHeader::NCompressionMethod::kImplode)
        mask &= 0x7FFF;
  }

  // we can ignore utf8 flag, if name is ascii, or if only cdItem has utf8 flag
  if (mask & NFileHeader::NFlags::kUtf8)
    if ((i1.Name.IsAscii() && i2_cd.Name.IsAscii())
        || (i2_cd.Flags & NFileHeader::NFlags::kUtf8))
      mask &= ~NFileHeader::NFlags::kUtf8;

  // some bad archive in rare case can use descriptor without descriptor flag in Central Dir
  // if (i1.HasDescriptor())
  mask &= ~NFileHeader::NFlags::kDescriptorUsedMask;
  
  return (mask == 0);
}


// #ifdef _WIN32
static bool AreEqualPaths_IgnoreSlashes(const char *s1, const char *s2)
{
  for (;;)
  {
    char c1 = *s1++;
    char c2 = *s2++;
    if (c1 == c2)
    {
      if (c1 == 0)
        return true;
    }
    else
    {
      if (c1 == '\\') c1 = '/';
      if (c2 == '\\') c2 = '/';
      if (c1 != c2)
        return false;
    }
  }
}
// #endif


static bool AreItemsEqual(const CItemEx &localItem, const CItemEx &cdItem)
{
  if (!FlagsAreSame(localItem, cdItem))
    return false;
  if (!localItem.HasDescriptor())
  {
    if (cdItem.PackSize != localItem.PackSize
        || cdItem.Size != localItem.Size
        || (cdItem.Crc != localItem.Crc && cdItem.Crc != 0)) // some program writes 0 to crc field in central directory
      return false;
  }
  /* pkzip 2.50 creates incorrect archives. It uses
       - WIN encoding for name in local header
       - OEM encoding for name in central header
     We don't support these strange items. */

  /* if (cdItem.Name.Len() != localItem.Name.Len())
    return false;
  */
  if (cdItem.Name != localItem.Name)
  {
    // #ifdef _WIN32
    // some xap files use backslash in central dir items.
    // we can ignore such errors in windows, where all slashes are converted to backslashes
    unsigned hostOs = cdItem.GetHostOS();
    
    if (hostOs == NFileHeader::NHostOS::kFAT ||
        hostOs == NFileHeader::NHostOS::kNTFS)
    {
      if (!AreEqualPaths_IgnoreSlashes(cdItem.Name, localItem.Name))
      {
        // pkzip 2.50 uses DOS encoding in central dir and WIN encoding in local header.
        // so we ignore that error
        if (hostOs != NFileHeader::NHostOS::kFAT
            || cdItem.MadeByVersion.Version < 25
            || cdItem.MadeByVersion.Version > 40)
          return false;
      }
    }
    /*
    else
    #endif
      return false;
    */
  }
  return true;
}


HRESULT CInArchive::Read_LocalItem_After_CdItem(CItemEx &item, bool &isAvail, bool &headersError)
{
  isAvail = true;
  headersError = false;
  if (item.FromLocal)
    return S_OK;
  try
  {
    UInt64 offset = item.LocalHeaderPos;

    if (IsMultiVol)
    {
      if (item.Disk >= Vols.Streams.Size())
      {
        isAvail = false;
        return S_FALSE;
      }
      Stream = Vols.Streams[item.Disk].Stream;
      Vols.StreamIndex = (int)item.Disk;
      if (!Stream)
      {
        isAvail = false;
        return S_FALSE;
      }
    }
    else
    {
      if (UseDisk_in_SingleVol && item.Disk != EcdVolIndex)
      {
        isAvail = false;
        return S_FALSE;
      }
      Stream = StreamRef;

      offset = (UInt64)((Int64)offset + ArcInfo.Base);
      if (ArcInfo.Base < 0 && (Int64)offset < 0)
      {
        isAvail = false;
        return S_FALSE;
      }
    }

    _inBufMode = false;
    RINOK(Seek_SavePos(offset))
    InitBuf();
    /*
    // we can use buf mode with small buffer to reduce
    // the number of Read() calls in ReadLocalItem()
    _inBufMode = true;
    Buffer.Alloc(1 << 10);
    if (!Buffer.IsAllocated())
      return E_OUTOFMEMORY;
    */

    CItemEx localItem;
    if (ReadUInt32() != NSignature::kLocalFileHeader)
      return S_FALSE;
    ReadLocalItem(localItem);
    if (!AreItemsEqual(localItem, item))
      return S_FALSE;
    item.LocalFullHeaderSize = localItem.LocalFullHeaderSize;
    item.LocalExtra = localItem.LocalExtra;
    if (item.Crc != localItem.Crc && !localItem.HasDescriptor())
    {
      item.Crc = localItem.Crc;
      headersError = true;
    }
    if ((item.Flags ^ localItem.Flags) & NFileHeader::NFlags::kDescriptorUsedMask)
    {
      item.Flags = (UInt16)(item.Flags ^ NFileHeader::NFlags::kDescriptorUsedMask);
      headersError = true;
    }
    item.FromLocal = true;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
}


/*
---------- FindDescriptor ----------

in:
  _streamPos : position in Stream
  Stream :
  Vols : if (IsMultiVol)

action:
  searches descriptor in input stream(s).
  sets
    item.DescriptorWasRead = true;
    item.Size
    item.PackSize
    item.Crc
  if descriptor was found

out:
  S_OK:
      if ( item.DescriptorWasRead) : if descriptor was found
      if (!item.DescriptorWasRead) : if descriptor was not found : unexpected end of stream(s)

  S_FALSE: if no items or there is just one item with strange properies that doesn't look like real archive.

  another error code: Callback error.

exceptions :
  CSystemException() : stream reading error
*/

HRESULT CInArchive::FindDescriptor(CItemEx &item, unsigned numFiles)
{
  // const size_t kBufSize = (size_t)1 << 5; // don't increase it too much. It reads data look ahead.

  // Buffer.Alloc(kBufSize);
  // Byte *buf = Buffer;
  
  UInt64 packedSize = 0;
  
  UInt64 progressPrev = _cnt;

  for (;;)
  {
    /* appnote specification claims that we must use 64-bit descriptor, if there is zip64 extra.
       But some old third-party xps archives used 64-bit descriptor without zip64 extra. */
    // unsigned descriptorSize = kDataDescriptorSize64 + kNextSignatureSize;
    
    // const unsigned kNextSignatureSize = 0;  // we can disable check for next signatuire
    const unsigned kNextSignatureSize = 4;  // we check also for signature for next File headear

    const unsigned descriptorSize4 = item.GetDescriptorSize() + kNextSignatureSize;

    if (descriptorSize4 > Buffer.Size()) return E_FAIL;

    // size_t processedSize;
    CanStartNewVol = true;
    RINOK(LookAhead(descriptorSize4))
    const size_t avail = GetAvail();
    
    if (avail < descriptorSize4)
    {
      // we write to packSize all these available bytes.
      // later it's simpler to work with such value than with 0
      // if (item.PackSize == 0)
        item.PackSize = packedSize + avail;
      if (item.Method == 0)
        item.Size = item.PackSize;
      SkipLookahed(avail);
      return S_OK;
    }

    const Byte * const pStart = Buffer + _bufPos;
    const Byte * p = pStart;
    const Byte * const limit = pStart + (avail - descriptorSize4);
    
    for (; p <= limit; p++)
    {
      // descriptor signature field is Info-ZIP's extension to pkware Zip specification.
      // New ZIP specification also allows descriptorSignature.
      
      p = FindPK_4(p, limit + 1);
      if (p > limit)
        break;

      /*
      if (*p != 0x50)
        continue;
      */

      if (Get32(p) != NSignature::kDataDescriptor)
        continue;

      // we check next signatuire after descriptor
      // maybe we need check only 2 bytes "PK" instead of 4 bytes, if some another type of header is possible after descriptor
      const UInt32 sig = Get32(p + descriptorSize4 - kNextSignatureSize);
      if (   sig != NSignature::kLocalFileHeader
          && sig != NSignature::kCentralFileHeader)
        continue;

      const UInt64 packSizeCur = packedSize + (size_t)(p - pStart);
      if (descriptorSize4 == kDataDescriptorSize64 + kNextSignatureSize) // if (item.LocalExtra.IsZip64)
      {
        const UInt64 descriptorPackSize = Get64(p + 8);
        if (descriptorPackSize != packSizeCur)
          continue;
        item.Size = Get64(p + 16);
      }
      else
      {
        const UInt32 descriptorPackSize = Get32(p + 8);
        if (descriptorPackSize != (UInt32)packSizeCur)
          continue;
        item.Size = Get32(p + 12);
        // that item.Size can be truncated to 32-bit value here
      }
      // We write calculated 64-bit packSize, even if descriptor64 was not used
      item.PackSize = packSizeCur;
      
      item.DescriptorWasRead = true;
      item.Crc = Get32(p + 4);

      const size_t skip = (size_t)(p - pStart) + descriptorSize4 - kNextSignatureSize;

      SkipLookahed(skip);

      return S_OK;
    }
    
    const size_t skip = (size_t)(p - pStart);
    SkipLookahed(skip);

    packedSize += skip;

    if (Callback)
    if (_cnt - progressPrev >= ((UInt32)1 << 22))
    {
      progressPrev = _cnt;
      const UInt64 numFiles64 = numFiles;
      RINOK(Callback->SetCompleted(&numFiles64, &_cnt))
    }
  }
}


HRESULT CInArchive::CheckDescriptor(const CItemEx &item)
{
  if (!item.HasDescriptor())
    return S_OK;
  
  // pkzip's version without descriptor signature is not supported
  
  bool isFinished = false;
  RINOK(IncreaseRealPosition(item.PackSize, isFinished))
  if (isFinished)
    return S_FALSE;

  /*
  if (!IsMultiVol)
  {
    RINOK(Seek_SavePos(ArcInfo.Base + item.GetDataPosition() + item.PackSize));
  }
  */
  
  Byte buf[kDataDescriptorSize64];
  try
  {
    CanStartNewVol = true;
    SafeRead(buf, item.GetDescriptorSize());
  }
  catch (const CSystemException &e) { return e.ErrorCode; }
  // catch (const CUnexpectEnd &)
  catch(...)
  {
    return S_FALSE;
  }
  // RINOK(ReadStream_FALSE(Stream, buf, item.GetDescriptorSize()));

  if (Get32(buf) != NSignature::kDataDescriptor)
    return S_FALSE;
  UInt32 crc = Get32(buf + 4);
  UInt64 packSize, unpackSize;
  
  if (item.LocalExtra.IsZip64)
  {
    packSize = Get64(buf + 8);
    unpackSize = Get64(buf + 16);
  }
  else
  {
    packSize = Get32(buf + 8);
    unpackSize = Get32(buf + 12);
  }
  
  if (crc != item.Crc || item.PackSize != packSize || item.Size != unpackSize)
    return S_FALSE;
  return S_OK;
}


HRESULT CInArchive::Read_LocalItem_After_CdItem_Full(CItemEx &item)
{
  if (item.FromLocal)
    return S_OK;
  try
  {
    bool isAvail = true;
    bool headersError = false;
    RINOK(Read_LocalItem_After_CdItem(item, isAvail, headersError))
    if (headersError)
      return S_FALSE;
    if (item.HasDescriptor())
      return CheckDescriptor(item);
  }
  catch(...) { return S_FALSE; }
  return S_OK;
}
  

HRESULT CInArchive::ReadCdItem(CItemEx &item)
{
  item.FromCentral = true;
  Byte p[kCentralHeaderSize - 4];
  SafeRead(p, kCentralHeaderSize - 4);

  item.MadeByVersion.Version = p[0];
  item.MadeByVersion.HostOS = p[1];
  item.ExtractVersion.Version = p[2];
  item.ExtractVersion.HostOS = p[3];
  G16(4, item.Flags);
  G16(6, item.Method);
  G32(8, item.Time);
  G32(12, item.Crc);
  G32(16, item.PackSize);
  G32(20, item.Size);
  const unsigned nameSize = Get16(p + 24);
  const unsigned extraSize = Get16(p + 26);
  const unsigned commentSize = Get16(p + 28);
  G16(30, item.Disk);
  G16(32, item.InternalAttrib);
  G32(34, item.ExternalAttrib);
  G32(38, item.LocalHeaderPos);
  ReadFileName(nameSize, item.Name);
  
  if (extraSize > 0)
    ReadExtra(item, extraSize, item.CentralExtra, item.Size, item.PackSize, &item);

  // May be these strings must be deleted
  /*
  if (item.IsDir())
    item.Size = 0;
  */
  
  ReadBuffer(item.Comment, commentSize);
  return S_OK;
}


/*
TryEcd64()
  (_inBufMode == false) is expected here
  so TryEcd64() can't change the Buffer.
  if (Ecd64 is not covered by cached region),
    TryEcd64() can change cached region ranges (_bufCached, _bufPos) and _streamPos.
*/

HRESULT CInArchive::TryEcd64(UInt64 offset, CCdInfo &cdInfo)
{
  if (offset >= ((UInt64)1 << 63))
    return S_FALSE;
  Byte buf[kEcd64_FullSize];

  RINOK(SeekToVol(Vols.StreamIndex, offset))
  RINOK(ReadFromCache_FALSE(buf, kEcd64_FullSize))

  if (Get32(buf) != NSignature::kEcd64)
    return S_FALSE;
  UInt64 mainSize = Get64(buf + 4);
  if (mainSize < kEcd64_MainSize || mainSize > ((UInt64)1 << 40))
    return S_FALSE;
  cdInfo.ParseEcd64e(buf + 12);
  return S_OK;
}


/* FindCd() doesn't use previous cached region,
   but it uses Buffer. So it sets new cached region */

HRESULT CInArchive::FindCd(bool checkOffsetMode)
{
  // There are no useful data in cache in most cases here.
  // So here we don't use cache data from previous operations.
  InitBuf();
  UInt64 endPos;
  RINOK(InStream_GetSize_SeekToEnd(Stream, endPos))
  _streamPos = endPos;
  const size_t kBufSizeMax = (size_t)1 << 17; // must be larger than
      // (1 << 16) + kEcdSize + kEcd64Locator_Size + kEcd64_FullSize
  const size_t bufSize = (endPos < kBufSizeMax) ? (size_t)endPos : kBufSizeMax;
  if (bufSize < kEcdSize)
    return S_FALSE;
  RINOK(AllocateBuffer(kBufSizeMax))
  {
    RINOK(Seek_SavePos(endPos - bufSize))
    size_t processed = bufSize;
    const HRESULT res = ReadStream(Stream, Buffer, &processed);
    _streamPos += processed;
    _bufCached = processed;
    _bufPos = 0;
    _cnt += processed;
    if (res != S_OK)
      return res;
    if (processed != bufSize)
      return S_FALSE;
  }

  CCdInfo &cdInfo = Vols.ecd;
  
  for (size_t i = bufSize - kEcdSize + 1;;)
  {
    const Byte *buf = Buffer;
    {
      const Byte *p = buf + i;
      do
        if (p == buf)
          return S_FALSE;
      while (*(--p) != 0x50);

      i = (size_t)(p - buf);
      if (Get32(p) != NSignature::kEcd)
        continue;
      cdInfo.ParseEcd32(p);
    }
    
    if (i >= kEcd64Locator_Size)
    {
      const size_t locatorIndex = i - kEcd64Locator_Size;
      if (Get32(buf + locatorIndex) == NSignature::kEcd64Locator)
      {
        CLocator locator;
        locator.Parse(buf + locatorIndex + 4);
        UInt32 numDisks = locator.NumDisks;
        // we ignore the error, where some zip creators use (NumDisks == 0)
        if (numDisks == 0)
          numDisks = 1;
        if ((cdInfo.ThisDisk == numDisks - 1 || ZIP64_IS_16_MAX(cdInfo.ThisDisk))
            && locator.Ecd64Disk < numDisks)
        {
          if (locator.Ecd64Disk != cdInfo.ThisDisk && !ZIP64_IS_16_MAX(cdInfo.ThisDisk))
            return E_NOTIMPL;
          
          // Most of the zip64 use fixed size Zip64 ECD
          // we try relative backward reading.
          const UInt64 absEcd64 = endPos - bufSize + i - (kEcd64Locator_Size + kEcd64_FullSize);
          
          if (locatorIndex >= kEcd64_FullSize)
          if (checkOffsetMode || absEcd64 == locator.Ecd64Offset)
          {
            const Byte *ecd64 = buf + locatorIndex - kEcd64_FullSize;
            if (Get32(ecd64) == NSignature::kEcd64 &&
                Get64(ecd64 + 4) == kEcd64_MainSize)
            {
              cdInfo.ParseEcd64e(ecd64 + 12);
              ArcInfo.Base = (Int64)(absEcd64 - locator.Ecd64Offset);
              // ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
              return S_OK;
            }
          }
          
          // some zip64 use variable size Zip64 ECD.
          // we try to use absolute offset from locator.
          if (absEcd64 != locator.Ecd64Offset)
          {
            if (TryEcd64(locator.Ecd64Offset, cdInfo) == S_OK)
            {
              ArcInfo.Base = 0;
              // ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
              return S_OK;
            }
          }
          
          // for variable Zip64 ECD with for archives with offset != 0.

          if (checkOffsetMode
              && ArcInfo.MarkerPos != 0
              && ArcInfo.MarkerPos + locator.Ecd64Offset != absEcd64)
          {
            if (TryEcd64(ArcInfo.MarkerPos + locator.Ecd64Offset, cdInfo) == S_OK)
            {
              ArcInfo.Base = (Int64)ArcInfo.MarkerPos;
              // ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
              return S_OK;
            }
          }
        }
      }
    }
    
    // bool isVolMode = (Vols.EndVolIndex != -1);
    // UInt32 searchDisk = (isVolMode ? Vols.EndVolIndex : 0);
    
    if (/* searchDisk == thisDisk && */ cdInfo.CdDisk <= cdInfo.ThisDisk)
    {
      // if (isVolMode)
      {
        if (cdInfo.CdDisk != cdInfo.ThisDisk)
          return S_OK;
      }
      
      UInt64 absEcdPos = endPos - bufSize + i;
      UInt64 cdEnd = cdInfo.Size + cdInfo.Offset;
      ArcInfo.Base = 0;
      // ArcInfo.BaseVolIndex = cdInfo.ThisDisk;
      if (absEcdPos != cdEnd)
      {
        /*
        if (cdInfo.Offset <= 16 && cdInfo.Size != 0)
        {
          // here we support some rare ZIP files with Central directory at the start
          ArcInfo.Base = 0;
        }
        else
        */
        ArcInfo.Base = (Int64)(absEcdPos - cdEnd);
      }
      return S_OK;
    }
  }
}


HRESULT CInArchive::TryReadCd(CObjectVector<CItemEx> &items, const CCdInfo &cdInfo, UInt64 cdOffset, UInt64 cdSize)
{
  items.Clear();
  IsCdUnsorted = false;
  
  if ((Int64)cdOffset < 0)
    return S_FALSE;

  // _startLocalFromCd_Disk = (UInt32)(Int32)-1;
  // _startLocalFromCd_Offset = (UInt64)(Int64)-1;

  RINOK(SeekToVol(IsMultiVol ? (int)cdInfo.CdDisk : -1, cdOffset))

  _inBufMode = true;
  _cnt = 0;

  if (Callback)
  {
    RINOK(Callback->SetTotal(&cdInfo.NumEntries, IsMultiVol ? &Vols.TotalBytesSize : NULL))
  }
  UInt64 numFileExpected = cdInfo.NumEntries;
  const UInt64 *totalFilesPtr = &numFileExpected;
  bool isCorrect_NumEntries = (cdInfo.IsFromEcd64 || numFileExpected >= ((UInt32)1 << 16));

  while (_cnt < cdSize)
  {
    CanStartNewVol = true;
    if (ReadUInt32() != NSignature::kCentralFileHeader)
      return S_FALSE;
    CanStartNewVol = false;
    {
      CItemEx cdItem;
      RINOK(ReadCdItem(cdItem))
      
      /*
      if (cdItem.Disk < _startLocalFromCd_Disk ||
          cdItem.Disk == _startLocalFromCd_Disk &&
          cdItem.LocalHeaderPos < _startLocalFromCd_Offset)
      {
        _startLocalFromCd_Disk = cdItem.Disk;
        _startLocalFromCd_Offset = cdItem.LocalHeaderPos;
      }
      */

      if (items.Size() > 0 && !IsCdUnsorted)
      {
        const CItemEx &prev = items.Back();
        if (cdItem.Disk < prev.Disk
            || (cdItem.Disk == prev.Disk &&
            cdItem.LocalHeaderPos < prev.LocalHeaderPos))
          IsCdUnsorted = true;
      }

      items.Add(cdItem);
    }
    if (Callback && (items.Size() & 0xFFF) == 0)
    {
      const UInt64 numFiles = items.Size();

      if (numFiles > numFileExpected && totalFilesPtr)
      {
        if (isCorrect_NumEntries)
          totalFilesPtr = NULL;
        else
          while (numFiles > numFileExpected)
            numFileExpected += (UInt32)1 << 16;
        RINOK(Callback->SetTotal(totalFilesPtr, NULL))
      }

      RINOK(Callback->SetCompleted(&numFiles, &_cnt))
    }
  }

  CanStartNewVol = true;

  return (_cnt == cdSize) ? S_OK : S_FALSE;
}


/*
static int CompareCdItems(void *const *elem1, void *const *elem2, void *)
{
  const CItemEx *i1 = *(const CItemEx **)elem1;
  const CItemEx *i2 = *(const CItemEx **)elem2;

  if (i1->Disk < i2->Disk) return -1;
  if (i1->Disk > i2->Disk) return 1;
  if (i1->LocalHeaderPos < i2->LocalHeaderPos) return -1;
  if (i1->LocalHeaderPos > i2->LocalHeaderPos) return 1;
  if (i1 < i2) return -1;
  if (i1 > i2) return 1;
  return 0;
}
*/

HRESULT CInArchive::ReadCd(CObjectVector<CItemEx> &items, UInt32 &cdDisk, UInt64 &cdOffset, UInt64 &cdSize)
{
  bool checkOffsetMode = true;
  
  if (IsMultiVol)
  {
    if (Vols.EndVolIndex == -1)
      return S_FALSE;
    Stream = Vols.Streams[(unsigned)Vols.EndVolIndex].Stream;
    if (!Vols.StartIsZip)
      checkOffsetMode = false;
  }
  else
    Stream = StartStream;

  if (!Vols.ecd_wasRead)
  {
    RINOK(FindCd(checkOffsetMode))
  }

  CCdInfo &cdInfo = Vols.ecd;
  
  HRESULT res = S_FALSE;
  
  cdSize = cdInfo.Size;
  cdOffset = cdInfo.Offset;
  cdDisk = cdInfo.CdDisk;

  if (!IsMultiVol)
  {
    if (cdInfo.ThisDisk != cdInfo.CdDisk)
      return S_FALSE;
  }

  const UInt64 base = (IsMultiVol ? 0 : (UInt64)ArcInfo.Base);
  res = TryReadCd(items, cdInfo, base + cdOffset, cdSize);
  
  if (res == S_FALSE && !IsMultiVol && base != ArcInfo.MarkerPos)
  {
    // do we need that additional attempt to read cd?
    res = TryReadCd(items, cdInfo, ArcInfo.MarkerPos + cdOffset, cdSize);
    if (res == S_OK)
      ArcInfo.Base = (Int64)ArcInfo.MarkerPos;
  }
  
  // Some rare case files are unsorted
  // items.Sort(CompareCdItems, NULL);
  return res;
}


static int FindItem(const CObjectVector<CItemEx> &items, const CItemEx &item)
{
  unsigned left = 0, right = items.Size();
  for (;;)
  {
    if (left >= right)
      return -1;
    const unsigned index = (unsigned)(((size_t)left + (size_t)right) / 2);
    const CItemEx &item2 = items[index];
    if (item.Disk < item2.Disk)
      right = index;
    else if (item.Disk > item2.Disk)
      left = index + 1;
    else if (item.LocalHeaderPos == item2.LocalHeaderPos)
      return (int)index;
    else if (item.LocalHeaderPos < item2.LocalHeaderPos)
      right = index;
    else
      left = index + 1;
  }
}

static bool IsStrangeItem(const CItem &item)
{
  return item.Name.Len() > (1 << 14) || item.Method > (1 << 8);
}



/*
  ---------- ReadLocals ----------

in:
  (_signature == NSignature::kLocalFileHeader)
  VirtStreamPos : after _signature : position in Stream
  Stream :
  Vols : if (IsMultiVol)
  (_inBufMode == false)

action:
  it parses local items.

  if ( IsMultiVol) it writes absolute offsets to CItemEx::LocalHeaderPos
  if (!IsMultiVol) it writes relative (from ArcInfo.Base) offsets to CItemEx::LocalHeaderPos
               later we can correct CItemEx::LocalHeaderPos values, if
               some new value for ArcInfo.Base will be detected
out:
  S_OK:
    (_signature != NSignature::kLocalFileHeade)
    _streamPos : after _signature

  S_FALSE: if no items or there is just one item with strange properies that doesn't look like real archive.

  another error code: stream reading error or Callback error.

  CUnexpectEnd() exception : it's not fatal exception here.
      It means that reading was interrupted by unexpected end of input stream,
      but some CItemEx items were parsed OK.
      We can stop further archive parsing.
      But we can use all filled CItemEx items.
*/

HRESULT CInArchive::ReadLocals(CObjectVector<CItemEx> &items)
{
  items.Clear();

  UInt64 progressPrev = _cnt;
  
  if (Callback)
  {
    RINOK(Callback->SetTotal(NULL, IsMultiVol ? &Vols.TotalBytesSize : NULL))
  }

  while (_signature == NSignature::kLocalFileHeader)
  {
    CItemEx item;

    item.LocalHeaderPos = GetVirtStreamPos() - 4;
    if (!IsMultiVol)
      item.LocalHeaderPos = (UInt64)((Int64)item.LocalHeaderPos - ArcInfo.Base);
    
    try
    {
      ReadLocalItem(item);
      item.FromLocal = true;
      bool isFinished = false;

      if (item.HasDescriptor())
      {
        RINOK(FindDescriptor(item, items.Size()))
        isFinished = !item.DescriptorWasRead;
      }
      else
      {
        if (item.PackSize >= ((UInt64)1 << 62))
          throw CUnexpectEnd();
        RINOK(IncreaseRealPosition(item.PackSize, isFinished))
      }
   
      items.Add(item);
      
      if (isFinished)
        throw CUnexpectEnd();

      ReadSignature();
    }
    catch (CUnexpectEnd &)
    {
      if (items.IsEmpty() || (items.Size() == 1 && IsStrangeItem(items[0])))
        return S_FALSE;
      throw;
    }


    if (Callback)
    if ((items.Size() & 0xFF) == 0
        || _cnt - progressPrev >= ((UInt32)1 << 22))
    {
      progressPrev = _cnt;
      const UInt64 numFiles = items.Size();
      RINOK(Callback->SetCompleted(&numFiles, &_cnt))
    }
  }

  if (items.Size() == 1 && _signature != NSignature::kCentralFileHeader)
    if (IsStrangeItem(items[0]))
      return S_FALSE;
  
  return S_OK;
}



HRESULT CVols::ParseArcName(IArchiveOpenVolumeCallback *volCallback)
{
  UString name;
  {
    NWindows::NCOM::CPropVariant prop;
    RINOK(volCallback->GetProperty(kpidName, &prop))
    if (prop.vt != VT_BSTR)
      return S_OK;
    name = prop.bstrVal;
  }

  const int dotPos = name.ReverseFind_Dot();
  if (dotPos < 0)
    return S_OK;
  const UString ext = name.Ptr((unsigned)(dotPos + 1));
  name.DeleteFrom((unsigned)(dotPos + 1));

  StartVolIndex = (Int32)(-1);

  if (ext.IsEmpty())
    return S_OK;
  {
    wchar_t c = ext[0];
    IsUpperCase = (c >= 'A' && c <= 'Z');
    if (ext.IsEqualTo_Ascii_NoCase("zip"))
    {
      BaseName = name;
      StartIsZ = true;
      StartIsZip = true;
      return S_OK;
    }
    else if (ext.IsEqualTo_Ascii_NoCase("exe"))
    {
      /* possible cases:
         - exe with zip inside
         - sfx: a.exe, a.z02, a.z03,... , a.zip
                a.exe is start volume.
         - zip renamed to exe
      */

      StartIsExe = true;
      BaseName = name;
      StartVolIndex = 0;
      /* sfx-zip can use both arc.exe and arc.zip
         We can open arc.zip, if it was requesed to open arc.exe.
         But it's possible that arc.exe and arc.zip are not parts of same archive.
         So we can disable such operation */

      // 18.04: we still want to open zip renamed to exe.
      /*
      {
        UString volName = name;
        volName += IsUpperCase ? "Z01" : "z01";
        {
          CMyComPtr<IInStream> stream;
          HRESULT res2 = volCallback->GetStream(volName, &stream);
          if (res2 == S_OK)
            DisableVolsSearch = true;
        }
      }
      */
      DisableVolsSearch = true;
      return S_OK;
    }
    else if (ext[0] == 'z' || ext[0] == 'Z')
    {
      if (ext.Len() < 3)
        return S_OK;
      const wchar_t *end = NULL;
      UInt32 volNum = ConvertStringToUInt32(ext.Ptr(1), &end);
      if (*end != 0 || volNum < 1 || volNum > ((UInt32)1 << 30))
        return S_OK;
      StartVolIndex = (Int32)(volNum - 1);
      BaseName = name;
      StartIsZ = true;
    }
    else
      return S_OK;
  }

  UString volName = BaseName;
  volName += (IsUpperCase ? "ZIP" : "zip");
  
  HRESULT res = volCallback->GetStream(volName, &ZipStream);
  
  if (res == S_FALSE || !ZipStream)
  {
    if (MissingName.IsEmpty())
    {
      MissingZip = true;
      MissingName = volName;
    }
    return S_OK;
  }

  return res;
}


HRESULT CInArchive::ReadVols2(IArchiveOpenVolumeCallback *volCallback,
    unsigned start, int lastDisk, int zipDisk, unsigned numMissingVolsMax, unsigned &numMissingVols)
{
  if (Vols.DisableVolsSearch)
    return S_OK;

  numMissingVols = 0;

  for (unsigned i = start;; i++)
  {
    if (lastDisk >= 0 && i >= (unsigned)lastDisk)
      break;
    
    if (i < Vols.Streams.Size())
      if (Vols.Streams[i].Stream)
        continue;

    CMyComPtr<IInStream> stream;

    if ((int)i == zipDisk)
    {
      stream = Vols.ZipStream;
    }
    else if ((int)i == Vols.StartVolIndex)
    {
      stream = StartStream;
    }
    else
    {
      UString volName = Vols.BaseName;
      {
        volName.Add_Char(Vols.IsUpperCase ? 'Z' : 'z');
        const unsigned v = i + 1;
        if (v < 10)
          volName.Add_Char('0');
        volName.Add_UInt32(v);
      }
        
      HRESULT res = volCallback->GetStream(volName, &stream);
      if (res != S_OK && res != S_FALSE)
        return res;
      if (res == S_FALSE || !stream)
      {
        if (i == 0)
        {
          UString volName_exe = Vols.BaseName;
          volName_exe += (Vols.IsUpperCase ? "EXE" : "exe");
          
          HRESULT res2 = volCallback->GetStream(volName_exe, &stream);
          if (res2 != S_OK && res2 != S_FALSE)
            return res2;
          res = res2;
        }
      }
      if (res == S_FALSE || !stream)
      {
        if (i == 1 && Vols.StartIsExe)
          return S_OK;
        if (Vols.MissingName.IsEmpty())
          Vols.MissingName = volName;
        numMissingVols++;
        if (numMissingVols > numMissingVolsMax)
          return S_OK;
        if (lastDisk == -1 && numMissingVols != 0)
          return S_OK;
        continue;
      }
    }

    UInt64 pos, size;
    RINOK(InStream_GetPos_GetSize(stream, pos, size))

    while (i >= Vols.Streams.Size())
      Vols.Streams.AddNew();
    
    CVols::CSubStreamInfo &ss = Vols.Streams[i];
    Vols.NumVols++;
    Vols.TotalBytesSize += size;

    ss.Stream = stream;
    ss.Size = size;

    if ((int)i == zipDisk)
    {
      Vols.EndVolIndex = (int)(Vols.Streams.Size() - 1);
      break;
    }
  }

  return S_OK;
}


HRESULT CInArchive::ReadVols()
{
  CMyComPtr<IArchiveOpenVolumeCallback> volCallback;

  Callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&volCallback);
  if (!volCallback)
    return S_OK;

  RINOK(Vols.ParseArcName(volCallback))

  // const int startZIndex = Vols.StartVolIndex;

  if (!Vols.StartIsZ)
  {
    if (!Vols.StartIsExe)
      return S_OK;
  }

  int zipDisk = -1;
  int cdDisk = -1;

  if (Vols.StartIsZip)
    Vols.ZipStream = StartStream;

  if (Vols.ZipStream)
  {
    Stream = Vols.ZipStream;
    
    if (Vols.StartIsZip)
      Vols.StreamIndex = -1;
    else
    {
      Vols.StreamIndex = -2;
      InitBuf();
    }

    HRESULT res = FindCd(true);

    CCdInfo &ecd = Vols.ecd;
    if (res == S_OK)
    {
      zipDisk = (int)ecd.ThisDisk;
      Vols.ecd_wasRead = true;

      // if is not multivol or bad multivol, we return to main single stream code
      if (ecd.ThisDisk == 0
          || ecd.ThisDisk >= ((UInt32)1 << 30)
          || ecd.ThisDisk < ecd.CdDisk)
        return S_OK;
      
      cdDisk = (int)ecd.CdDisk;
      if (Vols.StartVolIndex < 0)
        Vols.StartVolIndex = (Int32)ecd.ThisDisk;
      else if ((UInt32)Vols.StartVolIndex >= ecd.ThisDisk)
        return S_OK;

      // Vols.StartVolIndex = ecd.ThisDisk;
      // Vols.EndVolIndex = ecd.ThisDisk;
      unsigned numMissingVols;
      if (cdDisk != zipDisk)
      {
        // get volumes required for cd.
        RINOK(ReadVols2(volCallback, (unsigned)cdDisk, zipDisk, zipDisk, 0, numMissingVols))
        if (numMissingVols != 0)
        {
          // cdOK = false;
        }
      }
    }
    else if (res != S_FALSE)
      return res;
  }

  if (Vols.StartVolIndex < 0)
  {
    // is not mutivol;
    return S_OK;
  }

  /*
  if (!Vols.Streams.IsEmpty())
    IsMultiVol = true;
  */
  
  unsigned numMissingVols;

  if (cdDisk != 0)
  {
    // get volumes that were no requested still
    const unsigned kNumMissingVolsMax = 1 << 12;
    RINOK(ReadVols2(volCallback, 0, cdDisk < 0 ? -1 : cdDisk, zipDisk, kNumMissingVolsMax, numMissingVols))
  }

  // if (Vols.StartVolIndex >= 0)
  {
    if (Vols.Streams.IsEmpty())
      if (Vols.StartVolIndex > (1 << 20))
        return S_OK;
    if ((unsigned)Vols.StartVolIndex >= Vols.Streams.Size()
        || !Vols.Streams[(unsigned)Vols.StartVolIndex].Stream)
    {
      // we get volumes starting from StartVolIndex, if they we not requested before know the volume index (if FindCd() was ok)
      RINOK(ReadVols2(volCallback, (unsigned)Vols.StartVolIndex, zipDisk, zipDisk, 0, numMissingVols))
    }
  }

  if (Vols.ZipStream)
  {
    // if there is no another volumes and volumeIndex is too big, we don't use multivol mode
    if (Vols.Streams.IsEmpty())
      if (zipDisk > (1 << 10))
        return S_OK;
    if (zipDisk >= 0)
    {
      // we create item in Streams for ZipStream, if we know the volume index (if FindCd() was ok)
      RINOK(ReadVols2(volCallback, (unsigned)zipDisk, zipDisk + 1, zipDisk, 0, numMissingVols))
    }
  }

  if (!Vols.Streams.IsEmpty())
  {
    IsMultiVol = true;
    /*
    if (cdDisk)
      IsMultiVol = true;
    */
    const int startZIndex = Vols.StartVolIndex;
    if (startZIndex >= 0)
    {
      // if all volumes before start volume are OK, we can start parsing from 0
      // if there are missing volumes before startZIndex, we start parsing in current startZIndex
      if ((unsigned)startZIndex < Vols.Streams.Size())
      {
        for (unsigned i = 0; i <= (unsigned)startZIndex; i++)
          if (!Vols.Streams[i].Stream)
          {
            Vols.StartParsingVol = startZIndex;
            break;
          }
      }
    }
  }

  return S_OK;
}



HRESULT CVols::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;

  for (;;)
  {
    if (StreamIndex < 0)
      return S_OK;
    if ((unsigned)StreamIndex >= Streams.Size())
      return S_OK;
    const CVols::CSubStreamInfo &s = Streams[(unsigned)StreamIndex];
    if (!s.Stream)
      return S_FALSE;
    if (NeedSeek)
    {
      RINOK(s.SeekToStart())
      NeedSeek = false;
    }
    UInt32 realProcessedSize = 0;
    HRESULT res = s.Stream->Read(data, size, &realProcessedSize);
    if (processedSize)
      *processedSize = realProcessedSize;
    if (res != S_OK)
      return res;
    if (realProcessedSize != 0)
      return res;
    StreamIndex++;
    NeedSeek = true;
  }
}

Z7_COM7F_IMF(CVolStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  return Vols->Read(data, size, processedSize);
}




#define COPY_ECD_ITEM_16(n) if (!isZip64 || !ZIP64_IS_16_MAX(ecd. n))     cdInfo. n = ecd. n;
#define COPY_ECD_ITEM_32(n) if (!isZip64 || !ZIP64_IS_32_MAX(ecd. n)) cdInfo. n = ecd. n;


HRESULT CInArchive::ReadHeaders(CObjectVector<CItemEx> &items)
{
  // buffer that can be used for cd reading
  RINOK(AllocateBuffer(kSeqBufferSize))

  // here we can read small records. So we switch off _inBufMode.
  _inBufMode = false;

  HRESULT res = S_OK;

  bool localsWereRead = false;

  /* we try to open archive with the following modes:
     1) CD-MODE        : fast mode : we read backward ECD and CD, compare CD items with first Local item.
     2) LOCALS-CD-MODE : slow mode, if CD-MODE fails : we sequentially read all Locals and then CD.
     Then we read sequentially ECD64, Locator, ECD again at the end.

     - in LOCALS-CD-MODE we use use the following
         variables (with real cd properties) to set Base archive offset
         and check real cd properties with values from ECD/ECD64.
  */

  UInt64 cdSize = 0;
  UInt64 cdRelatOffset = 0;
  UInt32 cdDisk = 0;

  UInt64 cdAbsOffset = 0;   // absolute cd offset, for LOCALS-CD-MODE only.

if (Force_ReadLocals_Mode)
{
  IsArc = true;
  res = S_FALSE; // we will use LOCALS-CD-MODE mode
}
else
{
  if (!MarkerIsFound || !MarkerIsSafe)
  {
    IsArc = true;
    res = ReadCd(items, cdDisk, cdRelatOffset, cdSize);
    if (res == S_OK)
      ReadSignature();
    else if (res != S_FALSE)
      return res;
  }
  else  // (MarkerIsFound && MarkerIsSafe)
  {
 
  // _signature must be kLocalFileHeader or kEcd or kEcd64

  SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2 + 4);

  CanStartNewVol = false;

  if (_signature == NSignature::kEcd64)
  {
    // UInt64 ecd64Offset = GetVirtStreamPos() - 4;
    IsZip64 = true;

    {
      const UInt64 recordSize = ReadUInt64();
      if (recordSize < kEcd64_MainSize)
        return S_FALSE;
      if (recordSize >= ((UInt64)1 << 62))
        return S_FALSE;
      
      {
        const unsigned kBufSize = kEcd64_MainSize;
        Byte buf[kBufSize];
        SafeRead(buf, kBufSize);
        CCdInfo cdInfo;
        cdInfo.ParseEcd64e(buf);
        if (!cdInfo.IsEmptyArc())
          return S_FALSE;
      }
      
      RINOK(Skip64(recordSize - kEcd64_MainSize, 0))
    }

    ReadSignature();
    if (_signature != NSignature::kEcd64Locator)
      return S_FALSE;

    {
      const unsigned kBufSize = 16;
      Byte buf[kBufSize];
      SafeRead(buf, kBufSize);
      CLocator locator;
      locator.Parse(buf);
      if (!locator.IsEmptyArc())
        return S_FALSE;
    }

    ReadSignature();
    if (_signature != NSignature::kEcd)
      return S_FALSE;
  }
  
  if (_signature == NSignature::kEcd)
  {
    // It must be empty archive or backware archive
    // we don't support backware archive still
    
    const unsigned kBufSize = kEcdSize - 4;
    Byte buf[kBufSize];
    SafeRead(buf, kBufSize);
    CEcd ecd;
    ecd.Parse(buf);
    // if (ecd.cdSize != 0)
    // Do we need also to support the case where empty zip archive with PK00 uses cdOffset = 4 ??
    if (!ecd.IsEmptyArc())
      return S_FALSE;

    ArcInfo.Base = (Int64)ArcInfo.MarkerPos;
    IsArc = true; // check it: we need more tests?

    RINOK(SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2))
    ReadSignature();
  }
  else
  {
    CItemEx firstItem;
    try
    {
      try
      {
        if (!ReadLocalItem(firstItem))
          return S_FALSE;
      }
      catch(CUnexpectEnd &)
      {
        return S_FALSE;
      }

      IsArc = true;
      res = ReadCd(items, cdDisk, cdRelatOffset, cdSize);
      if (res == S_OK)
        ReadSignature();
    }
    catch(CUnexpectEnd &) { res = S_FALSE; }
    
    if (res != S_FALSE && res != S_OK)
      return res;

    if (res == S_OK && items.Size() == 0)
      res = S_FALSE;

    if (res == S_OK)
    {
      // we can't read local items here to keep _inBufMode state
      if ((Int64)ArcInfo.MarkerPos2 < ArcInfo.Base)
        res = S_FALSE;
      else
      {
        firstItem.LocalHeaderPos = (UInt64)((Int64)ArcInfo.MarkerPos2 - ArcInfo.Base);
        int index = -1;

        UInt32 min_Disk = (UInt32)(Int32)-1;
        UInt64 min_LocalHeaderPos = (UInt64)(Int64)-1;

        if (!IsCdUnsorted)
          index = FindItem(items, firstItem);
        else
        {
          FOR_VECTOR (i, items)
          {
            const CItemEx &cdItem = items[i];
            if (cdItem.Disk == firstItem.Disk
                && (cdItem.LocalHeaderPos == firstItem.LocalHeaderPos))
              index = (int)i;
            
            if (i == 0
                || cdItem.Disk < min_Disk
                || (cdItem.Disk == min_Disk && cdItem.LocalHeaderPos < min_LocalHeaderPos))
            {
              min_Disk = cdItem.Disk;
              min_LocalHeaderPos = cdItem.LocalHeaderPos;
            }
          }
        }

        if (index == -1)
          res = S_FALSE;
        else if (!AreItemsEqual(firstItem, items[(unsigned)index]))
          res = S_FALSE;
        else
        {
          ArcInfo.CdWasRead = true;
          if (IsCdUnsorted)
            ArcInfo.FirstItemRelatOffset = min_LocalHeaderPos;
          else
            ArcInfo.FirstItemRelatOffset = items[0].LocalHeaderPos;

          // ArcInfo.FirstItemRelatOffset = _startLocalFromCd_Offset;
        }
      }
    }
  }
  } // (MarkerIsFound && MarkerIsSafe)

} // (!onlyLocalsMode)


  CObjectVector<CItemEx> cdItems;

  bool needSetBase = false; // we set needSetBase only for LOCALS_CD_MODE
  unsigned numCdItems = items.Size();
  
  #ifdef ZIP_SELF_CHECK
  res = S_FALSE; // if uncommented, it uses additional LOCALS-CD-MODE mode to check the code
  #endif

  if (res != S_OK)
  {
    // ---------- LOCALS-CD-MODE ----------
    // CD doesn't match firstItem,
    // so we clear items and read Locals and CD.

    items.Clear();
    localsWereRead = true;
    
    HeadersError = false;
    HeadersWarning = false;
    ExtraMinorError = false;

    /* we can use any mode: with buffer and without buffer
         without buffer : skips packed data : fast for big files : slow for small files
         with    buffer : reads packed data : slow for big files : fast for small files
       Buffer mode is more effective. */
    // _inBufMode = false;
    _inBufMode = true;
    // we could change the buffer size here, if we want smaller Buffer.
    // RINOK(ReAllocateBuffer(kSeqBufferSize));
    // InitBuf()
    
    ArcInfo.Base = 0;

   if (!Disable_FindMarker)
   {
    if (!MarkerIsFound)
    {
      if (!IsMultiVol)
        return S_FALSE;
      if (Vols.StartParsingVol != 0)
        return S_FALSE;
      // if (StartParsingVol == 0) and we didn't find marker, we use default zero marker.
      // so we suppose that there is no sfx stub
      RINOK(SeekToVol(0, ArcInfo.MarkerPos2))
    }
    else
    {
      if (ArcInfo.MarkerPos != 0)
      {
        /*
        If multi-vol or there is (No)Span-marker at start of stream, we set (Base) as 0.
        In another caes:
          (No)Span-marker is supposed as false positive. So we set (Base) as main marker (MarkerPos2).
          The (Base) can be corrected later after ECD reading.
          But sfx volume with stub and (No)Span-marker in (!IsMultiVol) mode will have incorrect (Base) here.
        */
        ArcInfo.Base = (Int64)ArcInfo.MarkerPos2;
      }
      RINOK(SeekToVol(ArcInfo.MarkerVolIndex, ArcInfo.MarkerPos2))
    }
   }
    _cnt = 0;

    ReadSignature();
    
    LocalsWereRead = true;

    RINOK(ReadLocals(items))

    if (_signature != NSignature::kCentralFileHeader)
    {
      // GetVirtStreamPos() - 4
      if (items.IsEmpty())
        return S_FALSE;

      bool isError = true;

      const UInt32 apkSize = _signature;
      const unsigned kApkFooterSize = 16 + 8;
      if (apkSize >= kApkFooterSize && apkSize <= (1 << 20))
      {
        if (ReadUInt32() == 0)
        {
          CByteBuffer apk;
          apk.Alloc(apkSize);
          SafeRead(apk, apkSize);
          ReadSignature();
          const Byte *footer = apk + apkSize - kApkFooterSize;
          if (_signature == NSignature::kCentralFileHeader)
          if (GetUi64(footer) == apkSize)
          if (memcmp(footer + 8, "APK Sig Block 42", 16) == 0)
          {
            isError = false;
            IsApk = true;
          }
        }
      }
      
      if (isError)
      {
        NoCentralDir = true;
        HeadersError = true;
        return S_OK;
      }
    }
    
    _inBufMode = true;

    cdAbsOffset = GetVirtStreamPos() - 4;
    cdDisk = (UInt32)Vols.StreamIndex;

    #ifdef ZIP_SELF_CHECK
    if (!IsMultiVol && _cnt != GetVirtStreamPos() - ArcInfo.MarkerPos2)
      return E_FAIL;
    #endif

    const UInt64 processedCnt_start = _cnt;

    for (;;)
    {
      CItemEx cdItem;
      
      RINOK(ReadCdItem(cdItem))
      
      cdItems.Add(cdItem);
      if (Callback && (cdItems.Size() & 0xFFF) == 0)
      {
        const UInt64 numFiles = items.Size();
        const UInt64 numBytes = _cnt;
        RINOK(Callback->SetCompleted(&numFiles, &numBytes))
      }
      ReadSignature();
      if (_signature != NSignature::kCentralFileHeader)
        break;
    }
    
    cdSize = _cnt - processedCnt_start;

    #ifdef ZIP_SELF_CHECK
    if (!IsMultiVol)
    {
      if (_cnt != GetVirtStreamPos() - ArcInfo.MarkerPos2)
        return E_FAIL;
      if (cdSize != (GetVirtStreamPos() - 4) - cdAbsOffset)
        return E_FAIL;
    }
    #endif

    needSetBase = true;
    numCdItems = cdItems.Size();
    cdRelatOffset = (UInt64)((Int64)cdAbsOffset - ArcInfo.Base);

    if (!cdItems.IsEmpty())
    {
      ArcInfo.CdWasRead = true;
      ArcInfo.FirstItemRelatOffset = cdItems[0].LocalHeaderPos;
    }
  }

  
  
  CCdInfo cdInfo;
  CLocator locator;
  bool isZip64 = false;
  const UInt64 ecd64AbsOffset = GetVirtStreamPos() - 4;
  int ecd64Disk = -1;
  
  if (_signature == NSignature::kEcd64)
  {
    ecd64Disk = Vols.StreamIndex;

    IsZip64 = isZip64 = true;

    {
      const UInt64 recordSize = ReadUInt64();
      if (recordSize < kEcd64_MainSize
          || recordSize >= ((UInt64)1 << 62))
      {
        HeadersError = true;
        return S_OK;
      }

      {
        const unsigned kBufSize = kEcd64_MainSize;
        Byte buf[kBufSize];
        SafeRead(buf, kBufSize);
        cdInfo.ParseEcd64e(buf);
      }
      
      RINOK(Skip64(recordSize - kEcd64_MainSize, items.Size()))
    }


    ReadSignature();

    if (_signature != NSignature::kEcd64Locator)
    {
      HeadersError = true;
      return S_OK;
    }
  
    {
      const unsigned kBufSize = 16;
      Byte buf[kBufSize];
      SafeRead(buf, kBufSize);
      locator.Parse(buf);
      // we ignore the error, where some zip creators use (NumDisks == 0)
      // if (locator.NumDisks == 0) HeadersWarning = true;
    }

    ReadSignature();
  }
  
  
  if (_signature != NSignature::kEcd)
  {
    HeadersError = true;
    return S_OK;
  }

  
  CanStartNewVol = false;

  // ---------- ECD ----------

  CEcd ecd;
  {
    const unsigned kBufSize = kEcdSize - 4;
    Byte buf[kBufSize];
    SafeRead(buf, kBufSize);
    ecd.Parse(buf);
  }

  COPY_ECD_ITEM_16(ThisDisk)
  COPY_ECD_ITEM_16(CdDisk)
  COPY_ECD_ITEM_16(NumEntries_in_ThisDisk)
  COPY_ECD_ITEM_16(NumEntries)
  COPY_ECD_ITEM_32(Size)
  COPY_ECD_ITEM_32(Offset)

  bool cdOK = true;

  if ((UInt32)cdInfo.Size != (UInt32)cdSize)
  {
    // return S_FALSE;
    cdOK = false;
  }

  if (isZip64)
  {
    if (cdInfo.NumEntries != numCdItems
        || cdInfo.Size != cdSize)
    {
      cdOK = false;
    }
  }


  if (IsMultiVol)
  {
    if (cdDisk != cdInfo.CdDisk)
      HeadersError = true;
  }
  else if (needSetBase && cdOK)
  {
    const UInt64 oldBase = (UInt64)ArcInfo.Base;
    // localsWereRead == true
    // ArcInfo.Base == ArcInfo.MarkerPos2
    // cdRelatOffset == (cdAbsOffset - ArcInfo.Base)

    if (isZip64)
    {
      if (ecd64Disk == Vols.StartVolIndex)
      {
        const Int64 newBase = (Int64)ecd64AbsOffset - (Int64)locator.Ecd64Offset;
        if (newBase <= (Int64)ecd64AbsOffset)
        {
          if (!localsWereRead || newBase <= (Int64)ArcInfo.MarkerPos2)
          {
            ArcInfo.Base = newBase;
            cdRelatOffset = (UInt64)((Int64)cdAbsOffset - newBase);
          }
          else
            cdOK = false;
        }
      }
    }
    else if (numCdItems != 0) // we can't use ecd.Offset in empty archive?
    {
      if ((int)cdDisk == Vols.StartVolIndex)
      {
        const Int64 newBase = (Int64)cdAbsOffset - (Int64)cdInfo.Offset;
        if (newBase <= (Int64)cdAbsOffset)
        {
          if (!localsWereRead || newBase <= (Int64)ArcInfo.MarkerPos2)
          {
            // cd can be more accurate, when it points before Locals
            // so we change Base and cdRelatOffset
            ArcInfo.Base = newBase;
            cdRelatOffset = cdInfo.Offset;
          }
          else
          {
            // const UInt64 delta = ((UInt64)cdRelatOffset - cdInfo.Offset);
            const UInt64 delta = ((UInt64)(newBase - ArcInfo.Base));
            if ((UInt32)delta == 0)
            {
              // we set Overflow32bit mode, only if there is (x<<32) offset
              // between real_CD_offset_from_MarkerPos and CD_Offset_in_ECD.
              // Base and cdRelatOffset unchanged
              Overflow32bit = true;
            }
            else
              cdOK = false;
          }
        }
        else
          cdOK = false;
      }
    }
    // cdRelatOffset = cdAbsOffset - ArcInfo.Base;

    if (localsWereRead)
    {
      const UInt64 delta = (UInt64)((Int64)oldBase - ArcInfo.Base);
      if (delta != 0)
      {
        FOR_VECTOR (i, items)
          items[i].LocalHeaderPos += delta;
      }
    }
  }

  if (!cdOK)
    HeadersError = true;

  EcdVolIndex = cdInfo.ThisDisk;

  if (!IsMultiVol)
  {
    if (EcdVolIndex == 0 && Vols.MissingZip && Vols.StartIsExe)
    {
      Vols.MissingName.Empty();
      Vols.MissingZip = false;
    }

    if (localsWereRead)
    {
      if (EcdVolIndex != 0)
      {
        FOR_VECTOR (i, items)
          items[i].Disk = EcdVolIndex;
      }
    }

    UseDisk_in_SingleVol = true;
  }

  if (isZip64)
  {
    if ((cdInfo.ThisDisk == 0 && ecd64AbsOffset != (UInt64)(ArcInfo.Base + (Int64)locator.Ecd64Offset))
        // || cdInfo.NumEntries_in_ThisDisk != numCdItems
        || cdInfo.NumEntries != numCdItems
        || cdInfo.Size != cdSize
        || (cdInfo.Offset != cdRelatOffset && !items.IsEmpty()))
    {
      HeadersError = true;
      return S_OK;
    }
  }

  if (cdOK && !cdItems.IsEmpty())
  {
    // ---------- merge Central Directory Items ----------
  
    CRecordVector<unsigned> items2;

    int nextLocalIndex = 0;

    LocalsCenterMerged = true;

    FOR_VECTOR (i, cdItems)
    {
      if (Callback)
      if ((i & 0x3FFF) == 0)
      {
        const UInt64 numFiles64 = items.Size() + items2.Size();
        RINOK(Callback->SetCompleted(&numFiles64, &_cnt))
      }

      const CItemEx &cdItem = cdItems[i];
      
      int index = -1;
      
      if (nextLocalIndex != -1)
      {
        if ((unsigned)nextLocalIndex < items.Size())
        {
          CItemEx &item = items[(unsigned)nextLocalIndex];
          if (item.Disk == cdItem.Disk &&
              (item.LocalHeaderPos == cdItem.LocalHeaderPos
              || (Overflow32bit && (UInt32)item.LocalHeaderPos == cdItem.LocalHeaderPos)))
            index = nextLocalIndex++;
          else
            nextLocalIndex = -1;
        }
      }

      if (index == -1)
        index = FindItem(items, cdItem);

      // index = -1;

      if (index == -1)
      {
        items2.Add(i);
        HeadersError = true;
        continue;
      }

      CItemEx &item = items[(unsigned)index];
      if (item.Name != cdItem.Name
          // || item.Name.Len() != cdItem.Name.Len()
          || item.PackSize != cdItem.PackSize
          || item.Size != cdItem.Size
          // item.ExtractVersion != cdItem.ExtractVersion
          || !FlagsAreSame(item, cdItem)
          || item.Crc != cdItem.Crc)
      {
        HeadersError = true;
        continue;
      }

      // item.Name = cdItem.Name;
      item.MadeByVersion = cdItem.MadeByVersion;
      item.CentralExtra = cdItem.CentralExtra;
      item.InternalAttrib = cdItem.InternalAttrib;
      item.ExternalAttrib = cdItem.ExternalAttrib;
      item.Comment = cdItem.Comment;
      item.FromCentral = cdItem.FromCentral;
      // 22.02: we force utf8 flag, if central header has utf8 flag
      if (cdItem.Flags & NFileHeader::NFlags::kUtf8)
        item.Flags |= NFileHeader::NFlags::kUtf8;
    }

    FOR_VECTOR (k, items2)
      items.Add(cdItems[items2[k]]);
  }

  if (ecd.NumEntries < ecd.NumEntries_in_ThisDisk)
    HeadersError = true;

  if (ecd.ThisDisk == 0)
  {
    // if (isZip64)
    {
      if (ecd.NumEntries != ecd.NumEntries_in_ThisDisk)
        HeadersError = true;
    }
  }

  if (isZip64)
  {
    if (cdInfo.NumEntries != items.Size()
        || (ecd.NumEntries != items.Size() && ecd.NumEntries != 0xFFFF))
      HeadersError = true;
  }
  else
  {
    // old 7-zip could store 32-bit number of CD items to 16-bit field.
    // if (ecd.NumEntries != items.Size())
    if (ecd.NumEntries > items.Size())
      HeadersError = true;

    if (cdInfo.NumEntries != numCdItems)
    {
      if ((UInt16)cdInfo.NumEntries != (UInt16)numCdItems)
        HeadersError = true;
      else
        Cd_NumEntries_Overflow_16bit = true;
    }
  }

  ReadBuffer(ArcInfo.Comment, ecd.CommentSize);

  _inBufMode = false;

  // DisableBufMode();
  // Buffer.Free();
  /* we can't clear buf varibles. we need them to calculate PhySize of archive */

  if ((UInt16)cdInfo.NumEntries != (UInt16)numCdItems
      || (UInt32)cdInfo.Size != (UInt32)cdSize
      || ((UInt32)cdInfo.Offset != (UInt32)cdRelatOffset && !items.IsEmpty()))
  {
    // return S_FALSE;
    HeadersError = true;
  }

  #ifdef ZIP_SELF_CHECK
  if (localsWereRead)
  {
    const UInt64 endPos = ArcInfo.MarkerPos2 + _cnt;
    if (endPos != (IsMultiVol ? Vols.TotalBytesSize : ArcInfo.FileEndPos))
    {
      // there are some data after the end of archive or error in code;
      return E_FAIL;
    }
  }
  #endif
       
  // printf("\nOpen OK");
  return S_OK;
}



HRESULT CInArchive::Open(IInStream *stream, const UInt64 *searchLimit,
    IArchiveOpenCallback *callback, CObjectVector<CItemEx> &items)
{
  items.Clear();
  
  Close();

  UInt64 startPos;
  RINOK(InStream_GetPos(stream, startPos))
  RINOK(InStream_GetSize_SeekToEnd(stream, ArcInfo.FileEndPos))
  _streamPos = ArcInfo.FileEndPos;

  StartStream = stream;
  Stream = stream;
  Callback = callback;

  DisableBufMode();
  
  bool volWasRequested = false;

  if (!Disable_VolsRead)
  if (callback
      && (startPos == 0 || !searchLimit || *searchLimit != 0))
  {
    // we try to read volumes only if it's first call (offset == 0) or scan is allowed.
    volWasRequested = true;
    RINOK(ReadVols())
  }

  if (Disable_FindMarker)
  {
    RINOK(SeekToVol(-1, startPos))
    StreamRef = stream;
    Stream = stream;
    MarkerIsFound = true;
    MarkerIsSafe = true;
    ArcInfo.MarkerPos = startPos;
    ArcInfo.MarkerPos2 = startPos;
  }
  else
  if (IsMultiVol && Vols.StartParsingVol == 0 && (unsigned)Vols.StartParsingVol < Vols.Streams.Size())
  {
    // only StartParsingVol = 0 is safe search.
    RINOK(SeekToVol(0, 0))
    // if (Stream)
    {
      // UInt64 limit = 1 << 22; // for sfx
      UInt64 limit = 0; // without sfx
    
      HRESULT res = FindMarker(&limit);
      
      if (res == S_OK)
      {
        MarkerIsFound = true;
        MarkerIsSafe = true;
      }
      else if (res != S_FALSE)
        return res;
    }
  }
  else
  {
    // printf("\nOpen offset = %u\n", (unsigned)startPos);
    if (IsMultiVol
        && (unsigned)Vols.StartParsingVol < Vols.Streams.Size()
        && Vols.Streams[(unsigned)Vols.StartParsingVol].Stream)
    {
      RINOK(SeekToVol(Vols.StartParsingVol, Vols.StreamIndex == Vols.StartVolIndex ? startPos : 0))
    }
    else
    {
      RINOK(SeekToVol(-1, startPos))
    }
    
    // UInt64 limit = 1 << 22;
    // HRESULT res = FindMarker(&limit);

    HRESULT res = FindMarker(searchLimit);
    
    // const UInt64 curPos = GetVirtStreamPos();
    const UInt64 curPos = ArcInfo.MarkerPos2 + 4;

    if (res == S_OK)
      MarkerIsFound = true;
    else if (!IsMultiVol)
    {
      /*
      // if (startPos != 0), probably CD could be already tested with another call with (startPos == 0).
      // so we don't want to try to open CD again in that case.
      if (startPos != 0)
        return res;
      // we can try to open CD, if there is no Marker and (startPos == 0).
      // is it OK to open such files as ZIP, or big number of false positive, when CD can be find in end of file ?
      */
      return res;
    }
    
    if (ArcInfo.IsSpanMode && !volWasRequested)
    {
      RINOK(ReadVols())
      if (IsMultiVol && MarkerIsFound && ArcInfo.MarkerVolIndex < 0)
        ArcInfo.MarkerVolIndex = Vols.StartVolIndex;
    }

    MarkerIsSafe = !IsMultiVol
        || (ArcInfo.MarkerVolIndex == 0 && ArcInfo.MarkerPos == 0)
        ;
    

    if (IsMultiVol)
    {
      if ((unsigned)Vols.StartVolIndex < Vols.Streams.Size())
      {
        Stream = Vols.Streams[(unsigned)Vols.StartVolIndex].Stream;
        if (Stream)
        {
          RINOK(Seek_SavePos(curPos))
        }
        else
          IsMultiVol = false;
      }
      else
        IsMultiVol = false;
    }

    if (!IsMultiVol)
    {
      if (Vols.StreamIndex != -1)
      {
        Stream = StartStream;
        Vols.StreamIndex = -1;
        InitBuf();
        RINOK(Seek_SavePos(curPos))
      }

      ArcInfo.MarkerVolIndex = -1;
      StreamRef = stream;
      Stream = stream;
    }
  }


  if (!IsMultiVol)
    Vols.ClearRefs();

  {
    HRESULT res;
    try
    {
      res = ReadHeaders(items);
    }
    catch (const CSystemException &e) { res = e.ErrorCode; }
    catch (const CUnexpectEnd &)
    {
      if (items.IsEmpty())
        return S_FALSE;
      UnexpectedEnd = true;
      res = S_OK;
    }
    catch (...)
    {
      DisableBufMode();
      throw;
    }
    
    if (IsMultiVol)
    {
      ArcInfo.FinishPos = ArcInfo.FileEndPos;
      if ((unsigned)Vols.StreamIndex < Vols.Streams.Size())
        if (GetVirtStreamPos() < Vols.Streams[(unsigned)Vols.StreamIndex].Size)
          ArcInfo.ThereIsTail = true;
    }
    else
    {
      ArcInfo.FinishPos = GetVirtStreamPos();
      ArcInfo.ThereIsTail = (ArcInfo.FileEndPos > ArcInfo.FinishPos);
    }

    DisableBufMode();

    IsArcOpen = true;
    if (!IsMultiVol)
      Vols.Streams.Clear();
    return res;
  }
}


HRESULT CInArchive::GetItemStream(const CItemEx &item, bool seekPackData, CMyComPtr<ISequentialInStream> &stream)
{
  stream.Release();

  UInt64 pos = item.LocalHeaderPos;
  if (seekPackData)
    pos += item.LocalFullHeaderSize;

  if (!IsMultiVol)
  {
    if (UseDisk_in_SingleVol && item.Disk != EcdVolIndex)
      return S_OK;
    pos = (UInt64)((Int64)pos + ArcInfo.Base);
    RINOK(InStream_SeekSet(StreamRef, pos))
    stream = StreamRef;
    return S_OK;
  }

  if (item.Disk >= Vols.Streams.Size())
    return S_OK;
    
  IInStream *str2 = Vols.Streams[item.Disk].Stream;
  if (!str2)
    return S_OK;
  RINOK(InStream_SeekSet(str2, pos))
    
  Vols.NeedSeek = false;
  Vols.StreamIndex = (int)item.Disk;
    
  CVolStream *volsStreamSpec = new CVolStream;
  volsStreamSpec->Vols = &Vols;
  stream = volsStreamSpec;
  
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Archive/Zip/ZipItem.cpp ================ */
// Archive/ZipItem.cpp

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
namespace NZip {

using namespace NFileHeader;


/*
const char *k_SpecName_NTFS_STREAM = "@@NTFS@STREAM@";
const char *k_SpecName_MAC_RESOURCE_FORK = "@@MAC@RESOURCE-FORK@";
*/

static const CUInt32PCharPair g_ExtraTypes[] =
{
  { NExtraID::kZip64, "Zip64" },
  { NExtraID::kNTFS, "NTFS" },
  { NExtraID::kUnix0, "UNIX" },
  { NExtraID::kStrongEncrypt, "StrongCrypto" },
  { NExtraID::kUnixTime, "UT" },
  { NExtraID::kUnix1, "UX" },
  { NExtraID::kUnix2, "Ux" },
  { NExtraID::kUnixN, "ux" },
  { NExtraID::kIzUnicodeComment, "uc" },
  { NExtraID::kIzUnicodeName, "up" },
  { NExtraID::kIzNtSecurityDescriptor, "SD" },
  { NExtraID::kWzAES, "WzAES" },
  { NExtraID::kApkAlign, "ApkAlign" }
};

void CExtraSubBlock::PrintInfo(AString &s) const
{
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_ExtraTypes); i++)
  {
    const CUInt32PCharPair &pair = g_ExtraTypes[i];
    if (pair.Value == ID)
    {
      s += pair.Name;
      if (ID == NExtraID::kUnixTime)
      {
        if (Data.Size() >= 1)
        {
          s.Add_Colon();
          const Byte flags = Data[0];
          if (flags & 1) s.Add_Char('M');
          if (flags & 2) s.Add_Char('A');
          if (flags & 4) s.Add_Char('C');
          const UInt32 size = (UInt32)(Data.Size()) - 1;
          if (size % 4 == 0)
          {
            s.Add_Colon();
            s.Add_UInt32(size / 4);
          }
        }
      }
      /*
      if (ID == NExtraID::kApkAlign && Data.Size() >= 2)
      {
        char sz[32];
        sz[0] = ':';
        ConvertUInt32ToHex(GetUi16(Data), sz + 1);
        s += sz;
        for (unsigned j = 2; j < Data.Size(); j++)
        {
          char sz[32];
          sz[0] = '-';
          ConvertUInt32ToHex(Data[j], sz + 1);
          s += sz;
        }
      }
      */
      return;
    }
  }
  {
    char sz[16];
    sz[0] = '0';
    sz[1] = 'x';
    ConvertUInt32ToHex(ID, sz + 2);
    s += sz;
  }
}


void CExtraBlock::PrintInfo(AString &s) const
{
  if (Error)
    s.Add_OptSpaced("Extra_ERROR");

  if (MinorError)
    s.Add_OptSpaced("Minor_Extra_ERROR");

  if (IsZip64 || IsZip64_Error)
  {
    s.Add_OptSpaced("Zip64");
    if (IsZip64_Error)
      s += "_ERROR";
  }

  FOR_VECTOR (i, SubBlocks)
  {
    s.Add_Space_if_NotEmpty();
    SubBlocks[i].PrintInfo(s);
  }
}


bool CExtraSubBlock::ExtractNtfsTime(unsigned index, FILETIME &ft) const
{
  ft.dwHighDateTime = ft.dwLowDateTime = 0;
  UInt32 size = (UInt32)Data.Size();
  if (ID != NExtraID::kNTFS || size < 32)
    return false;
  const Byte *p = (const Byte *)Data;
  p += 4; // for reserved
  size -= 4;
  while (size > 4)
  {
    UInt16 tag = GetUi16(p);
    unsigned attrSize = GetUi16(p + 2);
    p += 4;
    size -= 4;
    if (attrSize > size)
      attrSize = size;
    
    if (tag == NNtfsExtra::kTagTime && attrSize >= 24)
    {
      p += 8 * index;
      ft.dwLowDateTime = GetUi32(p);
      ft.dwHighDateTime = GetUi32(p + 4);
      return true;
    }
    p += attrSize;
    size -= attrSize;
  }
  return false;
}

bool CExtraSubBlock::Extract_UnixTime(bool isCentral, unsigned index, UInt32 &res) const
{
  /* Info-Zip :
     The central-header extra field contains the modification
     time only, or no timestamp at all.
     Size of Data is used to flag its presence or absence
     If "Flags" indicates that Modtime is present in the local header
     field, it MUST be present in the central header field, too
  */

  res = 0;
  UInt32 size = (UInt32)Data.Size();
  if (ID != NExtraID::kUnixTime || size < 5)
    return false;
  const Byte *p = (const Byte *)Data;
  const Byte flags = *p++;
  size--;
  if (isCentral)
  {
    if (index != NUnixTime::kMTime ||
        (flags & (1 << NUnixTime::kMTime)) == 0 ||
        size < 4)
      return false;
    res = GetUi32(p);
    return true;
  }
  for (unsigned i = 0; i < 3; i++)
    if ((flags & (1 << i)) != 0)
    {
      if (size < 4)
        return false;
      if (index == i)
      {
        res = GetUi32(p);
        return true;
      }
      p += 4;
      size -= 4;
    }
  return false;
}


// Info-ZIP's abandoned "Unix1 timestamps & owner ID info"

bool CExtraSubBlock::Extract_Unix01_Time(unsigned index, UInt32 &res) const
{
  res = 0;
  const unsigned offset = index * 4;
  if (Data.Size() < offset + 4)
    return false;
  if (ID != NExtraID::kUnix0 &&
      ID != NExtraID::kUnix1)
    return false;
  const Byte *p = (const Byte *)Data + offset;
  res = GetUi32(p);
  return true;
}

/*
// PKWARE's Unix "extra" is similar to Info-ZIP's abandoned "Unix1 timestamps"
bool CExtraSubBlock::Extract_Unix_Time(unsigned index, UInt32 &res) const
{
  res = 0;
  const unsigned offset = index * 4;
  if (ID != NExtraID::kUnix0 || Data.Size() < offset)
    return false;
  const Byte *p = (const Byte *)Data + offset;
  res = GetUi32(p);
  return true;
}
*/

bool CExtraBlock::GetNtfsTime(unsigned index, FILETIME &ft) const
{
  FOR_VECTOR (i, SubBlocks)
  {
    const CExtraSubBlock &sb = SubBlocks[i];
    if (sb.ID == NFileHeader::NExtraID::kNTFS)
      return sb.ExtractNtfsTime(index, ft);
  }
  return false;
}

bool CExtraBlock::GetUnixTime(bool isCentral, unsigned index, UInt32 &res) const
{
  {
    FOR_VECTOR (i, SubBlocks)
    {
      const CExtraSubBlock &sb = SubBlocks[i];
      if (sb.ID == NFileHeader::NExtraID::kUnixTime)
        return sb.Extract_UnixTime(isCentral, index, res);
    }
  }
  
  switch (index)
  {
    case NUnixTime::kMTime: index = NUnixExtra::kMTime; break;
    case NUnixTime::kATime: index = NUnixExtra::kATime; break;
    default: return false;
  }
  
  {
    FOR_VECTOR (i, SubBlocks)
    {
      const CExtraSubBlock &sb = SubBlocks[i];
      if (sb.ID == NFileHeader::NExtraID::kUnix0 ||
          sb.ID == NFileHeader::NExtraID::kUnix1)
        return sb.Extract_Unix01_Time(index, res);
    }
  }
  return false;
}


bool CLocalItem::IsDir() const
{
  return NItemName::HasTailSlash(Name, GetCodePage());
}

bool CItem::IsDir() const
{
  // FIXME: we can check InfoZip UTF-8 name at first.
  if (NItemName::HasTailSlash(Name, GetCodePage()))
    return true;
  
  Byte hostOS = GetHostOS();

  if (Size == 0 && PackSize == 0 && !Name.IsEmpty() && Name.Back() == '\\')
  {
    // do we need to use CharPrevExA?
    // .NET Framework 4.5 : System.IO.Compression::CreateFromDirectory() probably writes backslashes to headers?
    // so we support that case
    switch (hostOS)
    {
      case NHostOS::kFAT:
      case NHostOS::kNTFS:
      case NHostOS::kHPFS:
      case NHostOS::kVFAT:
        return true;
      default: break;
    }
  }

  if (!FromCentral)
    return false;
  
  UInt16 highAttrib = (UInt16)((ExternalAttrib >> 16 ) & 0xFFFF);

  switch (hostOS)
  {
    case NHostOS::kAMIGA:
      switch (highAttrib & NAmigaAttrib::kIFMT)
      {
        case NAmigaAttrib::kIFDIR: return true;
        case NAmigaAttrib::kIFREG: return false;
        default: return false; // change it throw kUnknownAttributes;
      }
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
    case NHostOS::kHPFS:
    case NHostOS::kVFAT:
      return ((ExternalAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
    case NHostOS::kAtari:
    case NHostOS::kMac:
    case NHostOS::kVMS:
    case NHostOS::kVM_CMS:
    case NHostOS::kAcorn:
    case NHostOS::kMVS:
      return false; // change it throw kUnknownAttributes;
    case NHostOS::kUnix:
      return MY_LIN_S_ISDIR(highAttrib);
    default:
      return false;
  }
}

UInt32 CItem::GetWinAttrib() const
{
  UInt32 winAttrib = 0;
  switch (GetHostOS())
  {
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
      if (FromCentral)
        winAttrib = ExternalAttrib;
      break;
    case NHostOS::kUnix:
      // do we need to clear 16 low bits in this case?
      if (FromCentral)
      {
        /*
          Some programs write posix attributes in high 16 bits of ExternalAttrib
          Also some programs can write additional marker flag:
            0x8000 - p7zip
            0x4000 - Zip in MacOS
            no marker - Info-Zip

          Client code has two options to detect posix field:
            1) check 0x8000 marker. In that case we must add 0x8000 marker here.
            2) check that high 4 bits (file type bits in posix field) of attributes are not zero.
        */
        
        winAttrib = ExternalAttrib & 0xFFFF0000;
        
        // #ifndef _WIN32
        winAttrib |= 0x8000; // add posix mode marker
        // #endif
      }
      break;
    default: break;
  }
  if (IsDir()) // test it;
    winAttrib |= FILE_ATTRIBUTE_DIRECTORY;
  return winAttrib;
}

bool CItem::GetPosixAttrib(UInt32 &attrib) const
{
  // some archivers can store PosixAttrib in high 16 bits even with HostOS=FAT.
  if (FromCentral && GetHostOS() == NHostOS::kUnix)
  {
    attrib = ExternalAttrib >> 16;
    return (attrib != 0);
  }
  attrib = 0;
  if (IsDir())
    attrib = MY_LIN_S_IFDIR;
  return false;
}


bool CExtraSubBlock::CheckIzUnicode(const AString &s) const
{
  size_t size = Data.Size();
  if (size < 1 + 4)
    return false;
  const Byte *p = (const Byte *)Data;
  if (p[0] > 1)
    return false;
  if (CrcCalc(s, s.Len()) != GetUi32(p + 1))
    return false;
  size -= 5;
  p += 5;
  for (size_t i = 0; i < size; i++)
    if (p[i] == 0)
      return false;
  return Check_UTF8_Buf((const char *)(const void *)p, size, false);
}
  

void CItem::GetUnicodeString(UString &res, const AString &s, bool isComment, bool useSpecifiedCodePage, UINT codePage) const
{
  bool isUtf8 = IsUtf8();
  // bool ignore_Utf8_Errors = true;
  
  if (!isUtf8)
  {
    {
      const unsigned id = isComment ?
          NFileHeader::NExtraID::kIzUnicodeComment:
          NFileHeader::NExtraID::kIzUnicodeName;
      const CObjectVector<CExtraSubBlock> &subBlocks = GetMainExtra().SubBlocks;
      
      FOR_VECTOR (i, subBlocks)
      {
        const CExtraSubBlock &sb = subBlocks[i];
        if (sb.ID == id)
        {
          if (sb.CheckIzUnicode(s))
          {
            // const unsigned kIzUnicodeHeaderSize = 5;
            if (Convert_UTF8_Buf_To_Unicode(
                (const char *)(const void *)(const Byte *)sb.Data + 5,
                sb.Data.Size() - 5, res))
              return;
          }
          break;
        }
      }
    }
    
    if (useSpecifiedCodePage)
      isUtf8 = (codePage == CP_UTF8);
    #ifdef _WIN32
    else if (GetHostOS() == NFileHeader::NHostOS::kUnix)
    {
      /* Some ZIP archives in Unix use UTF-8 encoding without Utf8 flag in header.
         We try to get name as UTF-8.
         Do we need to do it in POSIX version also? */
      isUtf8 = true;

      /* 21.02: we want to ignore UTF-8 errors to support file paths that are mixed
          of UTF-8 and non-UTF-8 characters. */
      // ignore_Utf8_Errors = false;
      // ignore_Utf8_Errors = true;
    }
    #endif
  }
  
  
  if (isUtf8)
  {
    ConvertUTF8ToUnicode(s, res);
    return;
  }
  
  MultiByteToUnicodeString2(res, s, useSpecifiedCodePage ? codePage : GetCodePage());
}

}}

/* ================ unit: CPP/7zip/Archive/Zip/ZipOut.cpp ================ */
// ZipOut.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

HRESULT COutArchive::ClearRestriction()
{
  if (SetRestriction)
    return SetRestriction->SetRestriction(0, 0);
  return S_OK;
}

HRESULT COutArchive::SetRestrictionFromCurrent()
{
  if (SetRestriction)
    return SetRestriction->SetRestriction(m_Base + m_CurPos, (UInt64)(Int64)-1);
  return S_OK;
}

HRESULT COutArchive::Create(IOutStream *outStream)
{
  m_CurPos = 0;
  if (!m_OutBuffer.Create(1 << 16))
    return E_OUTOFMEMORY;
  m_Stream = outStream;
  m_OutBuffer.SetStream(outStream);
  m_OutBuffer.Init();

  return m_Stream->Seek(0, STREAM_SEEK_CUR, &m_Base);
}

void COutArchive::SeekToCurPos()
{
  HRESULT res = m_Stream->Seek((Int64)(m_Base + m_CurPos), STREAM_SEEK_SET, NULL);
  if (res != S_OK)
    throw CSystemException(res);
}

#define DOES_NEED_ZIP64(v) (v >= (UInt32)0xFFFFFFFF)
// #define DOES_NEED_ZIP64(v) (v >= 0)


Z7_NO_INLINE
void COutArchive::WriteBytes(const void *data, size_t size)
{
  m_OutBuffer.WriteBytes(data, size);
  m_CurPos += size;
}

Z7_NO_INLINE
void COutArchive::Write8(Byte b)
{
  m_OutBuffer.WriteByte(b);
  m_CurPos++;
}

Z7_NO_INLINE
void COutArchive::Write16(UInt16 val)
{
  Write8((Byte)val);
  Write8((Byte)(val >> 8));
}

Z7_NO_INLINE
void COutArchive::Write32(UInt32 val)
{
  for (int i = 0; i < 4; i++)
  {
    // Write8((Byte)val);
    m_OutBuffer.WriteByte((Byte)val);
    val >>= 8;
  }
  m_CurPos += 4;
}

#define WRITE_CONST_PAIR_16_16(a, b)  { Write32((a) | ((UInt32)(b) << 16)); }

Z7_NO_INLINE
void COutArchive::Write64(UInt64 val)
{
  for (int i = 0; i < 8; i++)
  {
    // Write8((Byte)val);
    m_OutBuffer.WriteByte((Byte)val);
    val >>= 8;
  }
  m_CurPos += 8;
}

Z7_NO_INLINE
void COutArchive::WriteExtra(const CExtraBlock &extra)
{
  FOR_VECTOR (i, extra.SubBlocks)
  {
    const CExtraSubBlock &subBlock = extra.SubBlocks[i];
    Write16((UInt16)subBlock.ID);
    Write16((UInt16)subBlock.Data.Size());
    WriteBytes(subBlock.Data, (UInt16)subBlock.Data.Size());
  }
}

void COutArchive::WriteCommonItemInfo(const CLocalItem &item, bool isZip64)
{
  {
    Byte ver = item.ExtractVersion.Version;
    if (isZip64 && ver < NFileHeader::NCompressionMethod::kExtractVersion_Zip64)
      ver = NFileHeader::NCompressionMethod::kExtractVersion_Zip64;
    Write8(ver);
  }
  Write8(item.ExtractVersion.HostOS);
  Write16(item.Flags);
  Write16(item.Method);
  Write32(item.Time);
}


#define WRITE_32_VAL_SPEC(_v_, _isZip64_) Write32((_isZip64_) ? 0xFFFFFFFF : (UInt32)(_v_));


void COutArchive::WriteUtfName(const CItemOut &item)
{
  if (item.Name_Utf.Size() == 0)
    return;
  Write16(NFileHeader::NExtraID::kIzUnicodeName);
  Write16((UInt16)(5 + item.Name_Utf.Size()));
  Write8(1); // (1 = version) of that extra field
  Write32(CrcCalc(item.Name.Ptr(), item.Name.Len()));
  WriteBytes(item.Name_Utf, (UInt16)item.Name_Utf.Size());
}


static const unsigned k_Ntfs_ExtraSize = 4 + 2 + 2 + (3 * 8);
static const unsigned k_UnixTime_ExtraSize = 1 + (1 * 4);

void COutArchive::WriteTimeExtra(const CItemOut &item, bool writeNtfs)
{
  if (writeNtfs)
  {
    // windows explorer ignores that extra
    WRITE_CONST_PAIR_16_16(NFileHeader::NExtraID::kNTFS, k_Ntfs_ExtraSize)
    Write32(0); // reserved
    WRITE_CONST_PAIR_16_16(NFileHeader::NNtfsExtra::kTagTime, 8 * 3)
    WriteNtfsTime(item.Ntfs_MTime);
    WriteNtfsTime(item.Ntfs_ATime);
    WriteNtfsTime(item.Ntfs_CTime);
  }

  if (item.Write_UnixTime)
  {
    // windows explorer ignores that extra
    // by specification : should we write to local header also?
    WRITE_CONST_PAIR_16_16(NFileHeader::NExtraID::kUnixTime, k_UnixTime_ExtraSize)
    const Byte flags = (Byte)((unsigned)1 << NFileHeader::NUnixTime::kMTime);
    Write8(flags);
    UInt32 unixTime;
    NWindows::NTime::FileTime_To_UnixTime(item.Ntfs_MTime, unixTime);
    Write32(unixTime);
  }
}


void COutArchive::WriteLocalHeader(CItemOut &item, bool needCheck)
{
  m_LocalHeaderPos = m_CurPos;
  item.LocalHeaderPos = m_CurPos;
  
  bool isZip64 =
      DOES_NEED_ZIP64(item.PackSize) ||
      DOES_NEED_ZIP64(item.Size);

  if (needCheck && m_IsZip64)
    isZip64 = true;

  // Why don't we write NTFS timestamps to local header?
  // Probably we want to reduce size of archive?
  const bool writeNtfs = false; // do not write NTFS timestamp to local header
  // const bool writeNtfs = item.Write_NtfsTime; // write NTFS time to local header
  const UInt32 localExtraSize = (UInt32)(
      (isZip64 ? (4 + 8 + 8): 0)
      + (writeNtfs ? 4 + k_Ntfs_ExtraSize : 0)
      + (item.Write_UnixTime ? 4 + k_UnixTime_ExtraSize : 0)
      + item.Get_UtfName_ExtraSize()
      + item.LocalExtra.GetSize());
  if ((UInt16)localExtraSize != localExtraSize)
    throw CSystemException(E_FAIL);
  if (needCheck && m_ExtraSize != localExtraSize)
    throw CSystemException(E_FAIL);

  m_IsZip64 = isZip64;
  m_ExtraSize = localExtraSize;

  item.LocalExtra.IsZip64 = isZip64;

  Write32(NSignature::kLocalFileHeader);
  
  WriteCommonItemInfo(item, isZip64);
  
  Write32(item.HasDescriptor() ? 0 : item.Crc);

  UInt64 packSize = item.PackSize;
  UInt64 size = item.Size;
  
  if (item.HasDescriptor())
  {
    packSize = 0;
    size = 0;
  }
  
  WRITE_32_VAL_SPEC(packSize, isZip64)
  WRITE_32_VAL_SPEC(size, isZip64)

  Write16((UInt16)item.Name.Len());

  Write16((UInt16)localExtraSize);
  
  WriteBytes((const char *)item.Name, (UInt16)item.Name.Len());

  if (isZip64)
  {
    WRITE_CONST_PAIR_16_16(NFileHeader::NExtraID::kZip64, 8 + 8)
    Write64(size);
    Write64(packSize);
  }

  WriteTimeExtra(item, writeNtfs);

  WriteUtfName(item);

  WriteExtra(item.LocalExtra);

  const UInt32 localFileHeaderSize = (UInt32)(m_CurPos - m_LocalHeaderPos);
  if (needCheck && m_LocalFileHeaderSize != localFileHeaderSize)
    throw CSystemException(E_FAIL);
  m_LocalFileHeaderSize = localFileHeaderSize;

  m_OutBuffer.FlushWithCheck();
}


void COutArchive::WriteLocalHeader_Replace(CItemOut &item)
{
  m_CurPos = m_LocalHeaderPos + m_LocalFileHeaderSize + item.PackSize;

  if (item.HasDescriptor())
  {
    WriteDescriptor(item);
    m_OutBuffer.FlushWithCheck();
    return;
    // we don't replace local header, if we write Descriptor.
    // so local header with Descriptor flag must be written to local header before.
  }

  const UInt64 nextPos = m_CurPos;
  m_CurPos = m_LocalHeaderPos;
  SeekToCurPos();
  WriteLocalHeader(item, true);
  m_CurPos = nextPos;
  SeekToCurPos();
}


void COutArchive::WriteDescriptor(const CItemOut &item)
{
  Byte buf[kDataDescriptorSize64];
  SetUi32(buf, NSignature::kDataDescriptor)
  SetUi32(buf + 4, item.Crc)
  unsigned descriptorSize;
  if (m_IsZip64)
  {
    SetUi64(buf + 8, item.PackSize)
    SetUi64(buf + 16, item.Size)
    descriptorSize = kDataDescriptorSize64;
  }
  else
  {
    SetUi32(buf + 8, (UInt32)item.PackSize)
    SetUi32(buf + 12, (UInt32)item.Size)
    descriptorSize = kDataDescriptorSize32;
  }
  WriteBytes(buf, descriptorSize);
}



void COutArchive::WriteCentralHeader(const CItemOut &item)
{
  const bool isUnPack64 = DOES_NEED_ZIP64(item.Size);
  const bool isPack64 = DOES_NEED_ZIP64(item.PackSize);
  const bool isPosition64 = DOES_NEED_ZIP64(item.LocalHeaderPos);
  const bool isZip64 = isPack64 || isUnPack64 || isPosition64;
  
  Write32(NSignature::kCentralFileHeader);
  Write8(item.MadeByVersion.Version);
  Write8(item.MadeByVersion.HostOS);
  
  WriteCommonItemInfo(item, isZip64);
  Write32(item.Crc);

  WRITE_32_VAL_SPEC(item.PackSize, isPack64)
  WRITE_32_VAL_SPEC(item.Size, isUnPack64)

  Write16((UInt16)item.Name.Len());
  
  const UInt16 zip64ExtraSize = (UInt16)((isUnPack64 ? 8: 0) + (isPack64 ? 8: 0) + (isPosition64 ? 8: 0));
  const bool writeNtfs = item.Write_NtfsTime;
  const size_t centralExtraSize =
      (isZip64 ? 4 + zip64ExtraSize : 0)
      + (writeNtfs ? 4 + k_Ntfs_ExtraSize : 0)
      + (item.Write_UnixTime ? 4 + k_UnixTime_ExtraSize : 0)
      + item.Get_UtfName_ExtraSize()
      + item.CentralExtra.GetSize();

  const UInt16 centralExtraSize16 = (UInt16)centralExtraSize;
  if (centralExtraSize16 != centralExtraSize)
    throw CSystemException(E_FAIL);

  Write16(centralExtraSize16);

  const UInt16 commentSize = (UInt16)item.Comment.Size();
  
  Write16(commentSize);
  Write16(0); // DiskNumberStart
  Write16(item.InternalAttrib);
  Write32(item.ExternalAttrib);
  WRITE_32_VAL_SPEC(item.LocalHeaderPos, isPosition64)
  WriteBytes((const char *)item.Name, item.Name.Len());
  
  if (isZip64)
  {
    Write16(NFileHeader::NExtraID::kZip64);
    Write16(zip64ExtraSize);
    if (isUnPack64)
      Write64(item.Size);
    if (isPack64)
      Write64(item.PackSize);
    if (isPosition64)
      Write64(item.LocalHeaderPos);
  }
  
  WriteTimeExtra(item, writeNtfs);
  WriteUtfName(item);
  
  WriteExtra(item.CentralExtra);
  if (commentSize != 0)
    WriteBytes(item.Comment, commentSize);
}

HRESULT COutArchive::WriteCentralDir(const CObjectVector<CItemOut> &items, const CByteBuffer *comment)
{
  RINOK(ClearRestriction())
  
  const UInt64 cdOffset = GetCurPos();
  FOR_VECTOR (i, items)
    WriteCentralHeader(items[i]);
  const UInt64 cd64EndOffset = GetCurPos();
  const UInt64 cdSize = cd64EndOffset - cdOffset;
  const bool cdOffset64 = DOES_NEED_ZIP64(cdOffset);
  const bool cdSize64 = DOES_NEED_ZIP64(cdSize);
  const bool need_Items_64 = items.Size() >= 0xFFFF;
  const unsigned items16 = (UInt16)(need_Items_64 ? 0xFFFF: items.Size());
  const bool isZip64 = (cdOffset64 || cdSize64 || need_Items_64);
  
  // isZip64 = true; // to test Zip64

  if (isZip64)
  {
    Write32(NSignature::kEcd64);
    Write64(kEcd64_MainSize);
    
    // to test extra block:
    // const UInt32 extraSize = 1 << 26;
    // Write64(kEcd64_MainSize + extraSize);

    WRITE_CONST_PAIR_16_16(45, // made by version
        45) // extract version
    Write32(0); // ThisDiskNumber
    Write32(0); // StartCentralDirectoryDiskNumber
    Write64((UInt64)items.Size());
    Write64((UInt64)items.Size());
    Write64((UInt64)cdSize);
    Write64((UInt64)cdOffset);

    // for (UInt32 iii = 0; iii < extraSize; iii++) Write8(1);

    Write32(NSignature::kEcd64Locator);
    Write32(0); // number of the disk with the start of the zip64 end of central directory
    Write64(cd64EndOffset);
    Write32(1); // total number of disks
  }
  
  Write32(NSignature::kEcd);
  WRITE_CONST_PAIR_16_16(0, 0)  // ThisDiskNumber, StartCentralDirectoryDiskNumber
  Write16((UInt16)items16);
  Write16((UInt16)items16);
  
  WRITE_32_VAL_SPEC(cdSize, cdSize64)
  WRITE_32_VAL_SPEC(cdOffset, cdOffset64)

  const UInt16 commentSize = (UInt16)(comment ? comment->Size() : 0);
  Write16((UInt16)commentSize);
  if (commentSize != 0)
    WriteBytes((const Byte *)*comment, commentSize);
  m_OutBuffer.FlushWithCheck();
  return S_OK;
}

void COutArchive::CreateStreamForCompressing(CMyComPtr<IOutStream> &outStream)
{
  COffsetOutStream *streamSpec = new COffsetOutStream;
  outStream = streamSpec;
  streamSpec->Init(m_Stream, m_Base + m_CurPos);
}

void COutArchive::CreateStreamForCopying(CMyComPtr<ISequentialOutStream> &outStream)
{
  outStream = m_Stream;
}

}}

/* ================ unit: CPP/7zip/Archive/Zip/ZipRegister.cpp ================ */
// ZipRegister.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NZip {

static const Byte k_Signature[] = {
    4, 0x50, 0x4B, 0x03, 0x04,               // Local
    4, 0x50, 0x4B, 0x05, 0x06,               // Ecd
    4, 0x50, 0x4B, 0x06, 0x06,               // Ecd64
    6, 0x50, 0x4B, 0x07, 0x08, 0x50, 0x4B,   // Span / Descriptor
    6, 0x50, 0x4B, 0x30, 0x30, 0x50, 0x4B }; // NoSpan

REGISTER_ARC_IO(
  "zip", "zip z01 zipx jar xpi odt ods docx xlsx epub ipa apk appx", NULL, 1,
  k_Signature,
  0,
    NArcInfoFlags::kFindSignature
  | NArcInfoFlags::kMultiSignature
  | NArcInfoFlags::kUseGlobalOffset
  | NArcInfoFlags::kCTime
  // | NArcInfoFlags::kCTime_Default
  | NArcInfoFlags::kATime
  // | NArcInfoFlags::kATime_Default
  | NArcInfoFlags::kMTime
  | NArcInfoFlags::kMTime_Default
  , TIME_PREC_TO_ARC_FLAGS_MASK (NFileTimeType::kWindows)
  | TIME_PREC_TO_ARC_FLAGS_MASK (NFileTimeType::kUnix)
  | TIME_PREC_TO_ARC_FLAGS_MASK (NFileTimeType::kDOS)
  | TIME_PREC_TO_ARC_FLAGS_TIME_DEFAULT (NFileTimeType::kWindows)
  , IsArc_Zip)
 
}}

/* ================ unit: CPP/7zip/Archive/Zip/ZipUpdate.cpp ================ */
// ZipUpdate.cpp

// amalgamation: header emitted in prologue

// #define DEBUG_CACHE

#ifdef DEBUG_CACHE
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
#ifndef Z7_ST
// amalgamation: header emitted in prologue
#endif
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// #include "../../Compress/ZstdEncoderProps.h"

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

using namespace NWindows;
using namespace NSynchronization;

namespace NArchive {
namespace NZip {

static const Byte kHostOS =
  #ifdef _WIN32
  NFileHeader::NHostOS::kFAT;
  #else
  NFileHeader::NHostOS::kUnix;
  #endif

static const Byte kMadeByHostOS = kHostOS;

// 18.06: now we always write zero to high byte of ExtractVersion field.
// Previous versions of p7zip wrote (NFileHeader::NHostOS::kUnix) there, that is not correct
static const Byte kExtractHostOS = 0;

static const Byte kMethodForDirectory = NFileHeader::NCompressionMethod::kStore;


static void AddAesExtra(CItem &item, Byte aesKeyMode, UInt16 method)
{
  CWzAesExtra wzAesField;
  wzAesField.Strength = aesKeyMode;
  wzAesField.Method = method;
  item.Method = NFileHeader::NCompressionMethod::kWzAES;
  item.Crc = 0;
  CExtraSubBlock sb;
  wzAesField.SetSubBlock(sb);
  item.LocalExtra.SubBlocks.Add(sb);
  item.CentralExtra.SubBlocks.Add(sb);
}


static void Copy_From_UpdateItem_To_ItemOut(const CUpdateItem &ui, CItemOut &item)
{
  item.Name = ui.Name;
  item.Name_Utf = ui.Name_Utf;
  item.Comment = ui.Comment;
  item.SetUtf8(ui.IsUtf8);
  // item.SetFlag_AltStream(ui.IsAltStream);
  // item.ExternalAttrib = ui.Attrib;
  item.Time = ui.Time;
  item.Ntfs_MTime = ui.Ntfs_MTime;
  item.Ntfs_ATime = ui.Ntfs_ATime;
  item.Ntfs_CTime = ui.Ntfs_CTime;

  item.Write_UnixTime = ui.Write_UnixTime;
  item.Write_NtfsTime = ui.Write_NtfsTime;
}

static void SetFileHeader(
    const CCompressionMethodMode &options,
    const CUpdateItem &ui,
    bool useDescriptor,
    CItemOut &item)
{
  item.Size = ui.Size;
  const bool isDir = ui.IsDir;

  item.ClearFlags();

  if (ui.NewProps)
  {
    Copy_From_UpdateItem_To_ItemOut(ui, item);
    // item.SetFlag_AltStream(ui.IsAltStream);
    item.ExternalAttrib = ui.Attrib;
  }
  /*
  else
    isDir = item.IsDir();
  */

  item.MadeByVersion.HostOS = kMadeByHostOS;
  item.MadeByVersion.Version = NFileHeader::NCompressionMethod::kMadeByProgramVersion;
  
  item.ExtractVersion.HostOS = kExtractHostOS;

  item.InternalAttrib = 0; // test it
  item.SetEncrypted(!isDir && options.Password_Defined);
  item.SetDescriptorMode(useDescriptor);

  if (isDir)
  {
    item.ExtractVersion.Version = NFileHeader::NCompressionMethod::kExtractVersion_Dir;
    item.Method = kMethodForDirectory;
    item.PackSize = 0;
    item.Size = 0;
    item.Crc = 0;
  }

  item.LocalExtra.Clear();
  item.CentralExtra.Clear();

  if (isDir)
  {
    item.ExtractVersion.Version = NFileHeader::NCompressionMethod::kExtractVersion_Dir;
    item.Method = kMethodForDirectory;
    item.PackSize = 0;
    item.Size = 0;
    item.Crc = 0;
  }
  else if (options.IsRealAesMode())
    AddAesExtra(item, options.AesKeyMode, (Byte)(options.MethodSequence.IsEmpty() ? 8 : options.MethodSequence[0]));
}


// we call SetItemInfoFromCompressingResult() after SetFileHeader()

static void SetItemInfoFromCompressingResult(const CCompressingResult &compressingResult,
    bool isAesMode, Byte aesKeyMode, CItem &item)
{
  item.ExtractVersion.Version = compressingResult.ExtractVersion;
  item.Method = compressingResult.Method;
  if (compressingResult.Method == NFileHeader::NCompressionMethod::kLZMA && compressingResult.LzmaEos)
    item.Flags |= NFileHeader::NFlags::kLzmaEOS;
  item.Crc = compressingResult.CRC;
  item.Size = compressingResult.UnpackSize;
  item.PackSize = compressingResult.PackSize;

  item.LocalExtra.Clear();
  item.CentralExtra.Clear();

  if (isAesMode)
    AddAesExtra(item, aesKeyMode, compressingResult.Method);
}


#ifndef Z7_ST

struct CMtSem
{
  NWindows::NSynchronization::CSemaphore Semaphore;
  NWindows::NSynchronization::CCriticalSection CS;
  CIntVector Indexes;
  int Head;

  void ReleaseItem(unsigned index)
  {
    {
      CCriticalSectionLock lock(CS);
      Indexes[index] = Head;
      Head = (int)index;
    }
    Semaphore.Release();
  }

  int GetFreeItem()
  {
    int i;
    {
      CCriticalSectionLock lock(CS);
      i = Head;
      Head = Indexes[(unsigned)i];
    }
    return i;
  }
};

static THREAD_FUNC_DECL CoderThread(void *threadCoderInfo);

struct CThreadInfo
{
  DECL_EXTERNAL_CODECS_LOC_VARS_DECL

  NWindows::CThread Thread;
  NWindows::NSynchronization::CAutoResetEvent CompressEvent;
  CMtSem *MtSem;
  unsigned ThreadIndex;

  bool ExitThread;

  CMtCompressProgress *ProgressSpec;
  CMyComPtr<ICompressProgressInfo> Progress;

  COutMemStream *OutStreamSpec;
  CMyComPtr<IOutStream> OutStream;
  CMyComPtr<ISequentialInStream> InStream;

  CAddCommon Coder;
  HRESULT Result;
  CCompressingResult CompressingResult;

  bool IsFree;
  bool InSeqMode;
  bool OutSeqMode;
  bool ExpectedDataSize_IsConfirmed;

  UInt32 UpdateIndex;
  UInt32 FileTime;
  UInt64 ExpectedDataSize;

  CThreadInfo():
      MtSem(NULL),
      ExitThread(false),
      ProgressSpec(NULL),
      OutStreamSpec(NULL),
      IsFree(true),
      InSeqMode(false),
      OutSeqMode(false),
      ExpectedDataSize_IsConfirmed(false),
      FileTime(0),
      ExpectedDataSize((UInt64)(Int64)-1)
  {}

  void SetOptions(const CCompressionMethodMode &options)
  {
    Coder.SetOptions(options);
  }
  
  HRESULT CreateEvents()
  {
    const WRes wres = CompressEvent.CreateIfNotCreated_Reset();
    return HRESULT_FROM_WIN32(wres);
  }
  
  // (group < 0) means no_group.
  HRESULT CreateThread_with_group(
#ifdef _WIN32
      int group
#endif
      )
  {
    // tested in win10: If thread is created by another thread,
    // child thread probably uses same group as parent thread.
    // So we don't need to send (group) to encoder in created thread.
    const WRes wres =
#ifdef _WIN32
      group >= 0 ?
        Thread.Create_With_Group(CoderThread, this, (unsigned)group) :
#endif
        Thread.Create(CoderThread, this);
    return HRESULT_FROM_WIN32(wres);
  }

  void WaitAndCode();

  void StopWait_Close()
  {
    ExitThread = true;
    if (OutStreamSpec)
      OutStreamSpec->StopWriting(E_ABORT);
    if (CompressEvent.IsCreated())
      CompressEvent.Set();
    Thread.Wait_Close();
  }
};

void CThreadInfo::WaitAndCode()
{
  for (;;)
  {
    CompressEvent.Lock();
    if (ExitThread)
      return;

    Result = Coder.Compress(
        EXTERNAL_CODECS_LOC_VARS
        InStream, OutStream,
        InSeqMode, OutSeqMode, FileTime, ExpectedDataSize,
        ExpectedDataSize_IsConfirmed,
        Progress, CompressingResult);
    
    if (Result == S_OK && Progress)
      Result = Progress->SetRatioInfo(&CompressingResult.UnpackSize, &CompressingResult.PackSize);
    
    MtSem->ReleaseItem(ThreadIndex);
  }
}

static THREAD_FUNC_DECL CoderThread(void *threadCoderInfo)
{
  ((CThreadInfo *)threadCoderInfo)->WaitAndCode();
  return 0;
}

class CThreads
{
public:
  CObjectVector<CThreadInfo> Threads;
  ~CThreads()
  {
    FOR_VECTOR (i, Threads)
      Threads[i].StopWait_Close();
  }
};

struct CMemBlocks2: public CMemLockBlocks
{
  bool Skip;
  bool InSeqMode;
  bool PreDescriptorMode;
  bool Finished;
  CCompressingResult CompressingResult;
  
  CMemBlocks2(): Skip(false), InSeqMode(false), PreDescriptorMode(false), Finished(false),
    CompressingResult() {}
};

class CMemRefs
{
public:
  CMemBlockManagerMt *Manager;
  CObjectVector<CMemBlocks2> Refs;
  CMemRefs(CMemBlockManagerMt *manager): Manager(manager) {}
  ~CMemRefs()
  {
    FOR_VECTOR (i, Refs)
      Refs[i].FreeOpt(Manager);
  }
};


Z7_CLASS_IMP_NOQIB_1(
  CMtProgressMixer2
  , ICompressProgressInfo
)
  UInt64 ProgressOffset;
  UInt64 InSizes[2];
  UInt64 OutSizes[2];
  CMyComPtr<IProgress> Progress;
  CMyComPtr<ICompressProgressInfo> RatioProgress;
  bool _inSizeIsMain;
public:
  NWindows::NSynchronization::CCriticalSection CriticalSection;
  void Create(IProgress *progress, bool inSizeIsMain);
  void SetProgressOffset(UInt64 progressOffset);
  void SetProgressOffset_NoLock(UInt64 progressOffset);
  HRESULT SetRatioInfo(unsigned index, const UInt64 *inSize, const UInt64 *outSize);
};

void CMtProgressMixer2::Create(IProgress *progress, bool inSizeIsMain)
{
  Progress = progress;
  Progress.QueryInterface(IID_ICompressProgressInfo, &RatioProgress);
  _inSizeIsMain = inSizeIsMain;
  ProgressOffset = InSizes[0] = InSizes[1] = OutSizes[0] = OutSizes[1] = 0;
}

void CMtProgressMixer2::SetProgressOffset_NoLock(UInt64 progressOffset)
{
  InSizes[1] = OutSizes[1] = 0;
  ProgressOffset = progressOffset;
}

void CMtProgressMixer2::SetProgressOffset(UInt64 progressOffset)
{
  CriticalSection.Enter();
  SetProgressOffset_NoLock(progressOffset);
  CriticalSection.Leave();
}

HRESULT CMtProgressMixer2::SetRatioInfo(unsigned index, const UInt64 *inSize, const UInt64 *outSize)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(CriticalSection);
  if (index == 0 && RatioProgress)
  {
    RINOK(RatioProgress->SetRatioInfo(inSize, outSize))
  }
  if (inSize)
    InSizes[index] = *inSize;
  if (outSize)
    OutSizes[index] = *outSize;
  UInt64 v = ProgressOffset + (_inSizeIsMain  ?
      (InSizes[0] + InSizes[1]) :
      (OutSizes[0] + OutSizes[1]));
  return Progress->SetCompleted(&v);
}

Z7_COM7F_IMF(CMtProgressMixer2::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  return SetRatioInfo(0, inSize, outSize);
}


Z7_CLASS_IMP_NOQIB_1(
  CMtProgressMixer
  , ICompressProgressInfo
)
public:
  CMtProgressMixer2 *Mixer2;
  CMyComPtr<ICompressProgressInfo> RatioProgress;
  void Create(IProgress *progress, bool inSizeIsMain);
};

void CMtProgressMixer::Create(IProgress *progress, bool inSizeIsMain)
{
  Mixer2 = new CMtProgressMixer2;
  RatioProgress = Mixer2;
  Mixer2->Create(progress, inSizeIsMain);
}

Z7_COM7F_IMF(CMtProgressMixer::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  return Mixer2->SetRatioInfo(1, inSize, outSize);
}


#endif

static HRESULT UpdateItemOldData(
    COutArchive &archive,
    CInArchive *inArchive,
    const CItemEx &itemEx,
    const CUpdateItem &ui,
    CItemOut &item,
    /* bool izZip64, */
    ICompressProgressInfo *progress,
    IArchiveUpdateCallbackFile *opCallback,
    UInt64 &complexity)
{
  if (opCallback)
  {
    RINOK(opCallback->ReportOperation(
        NEventIndexType::kInArcIndex, (UInt32)ui.IndexInArc,
        NUpdateNotifyOp::kReplicate))
  }

  UInt64 rangeSize;

  RINOK(archive.ClearRestriction())

  if (ui.NewProps)
  {
    if (item.HasDescriptor())
    {
      // we know compressed / uncompressed sizes and crc.
      // so we remove descriptor here
      item.Flags = (UInt16)(item.Flags & ~NFileHeader::NFlags::kDescriptorUsedMask);
      // return E_NOTIMPL;
    }
    // we keep ExternalAttrib and some another properties from old archive
    // item.ExternalAttrib = ui.Attrib;
    // if we don't change Comment, we keep Comment from OldProperties
    Copy_From_UpdateItem_To_ItemOut(ui, item);
    // item.SetFlag_AltStream(ui.IsAltStream);

    item.CentralExtra.RemoveUnknownSubBlocks();
    item.LocalExtra.RemoveUnknownSubBlocks();

    archive.WriteLocalHeader(item);
    rangeSize = item.GetPackSizeWithDescriptor();
  }
  else
  {
    item.LocalHeaderPos = archive.GetCurPos();
    rangeSize = itemEx.GetLocalFullSize();
  }

  CMyComPtr<ISequentialInStream> packStream;

  RINOK(inArchive->GetItemStream(itemEx, ui.NewProps, packStream))
  if (!packStream)
    return E_NOTIMPL;

  complexity += rangeSize;

  CMyComPtr<ISequentialOutStream> outStream;
  archive.CreateStreamForCopying(outStream);
  HRESULT res = NCompress::CopyStream_ExactSize(packStream, outStream, rangeSize, progress);
  archive.MoveCurPos(rangeSize);
  return res;
}


static HRESULT WriteDirHeader(COutArchive &archive, const CCompressionMethodMode *options,
    const CUpdateItem &ui, CItemOut &item)
{
  SetFileHeader(*options, ui, false, item);
  RINOK(archive.ClearRestriction())
  archive.WriteLocalHeader(item);
  return S_OK;
}


static void UpdatePropsFromStream(
    const CUpdateOptions &options,
    CUpdateItem &item, ISequentialInStream *fileInStream,
    IArchiveUpdateCallback *updateCallback, UInt64 &totalComplexity)
{
  CMyComPtr<IStreamGetProps> getProps;
  fileInStream->QueryInterface(IID_IStreamGetProps, (void **)&getProps);
  UInt64 size = (UInt64)(Int64)-1;
  bool size_WasSet = false;
  
  if (getProps)
  {
    FILETIME cTime, aTime, mTime;
    UInt32 attrib;
    if (getProps->GetProps(&size, &cTime, &aTime, &mTime, &attrib) == S_OK)
    {
      if (options.Write_MTime)
        if (!FILETIME_IsZero(mTime))
        {
          item.Ntfs_MTime = mTime;
          NTime::UtcFileTime_To_LocalDosTime(mTime, item.Time);
        }
        
      if (options.Write_CTime) if (!FILETIME_IsZero(cTime)) item.Ntfs_CTime = cTime;
      if (options.Write_ATime) if (!FILETIME_IsZero(aTime)) item.Ntfs_ATime = aTime;
        
      item.Attrib = attrib;
      size_WasSet = true;
    }
  }

  if (!size_WasSet)
  {
    CMyComPtr<IStreamGetSize> streamGetSize;
    fileInStream->QueryInterface(IID_IStreamGetSize, (void **)&streamGetSize);
    if (streamGetSize)
    {
      if (streamGetSize->GetSize(&size) == S_OK)
        size_WasSet = true;
    }
  }

  if (size_WasSet && size != (UInt64)(Int64)-1)
  {
    item.Size_WasSetFromStream = true;
    if (size != item.Size)
    {
      const Int64 newComplexity = (Int64)totalComplexity + ((Int64)size - (Int64)item.Size);
      if (newComplexity > 0)
      {
        totalComplexity = (UInt64)newComplexity;
        updateCallback->SetTotal(totalComplexity);
      }
      item.Size = size;
    }
  }
}


/*
static HRESULT ReportProps(
    IArchiveUpdateCallbackArcProp *reportArcProp,
    UInt32 index,
    const CItemOut &item,
    bool isAesMode)
{
  PROPVARIANT prop;
  prop.vt = VT_EMPTY;
  prop.wReserved1 = 0;
  
  NCOM::PropVarEm_Set_UInt64(&prop, item.Size);
  RINOK(reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, index, kpidSize, &prop));
  
  NCOM::PropVarEm_Set_UInt64(&prop, item.PackSize);
  RINOK(reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, index, kpidPackSize, &prop));

  if (!isAesMode)
  {
    NCOM::PropVarEm_Set_UInt32(&prop, item.Crc);
    RINOK(reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, index, kpidCRC, &prop));
  }

  RINOK(reportArcProp->ReportFinished(NEventIndexType::kOutArcIndex, index, NUpdate::NOperationResult::kOK));

  // if (opCallback) RINOK(opCallback->ReportOperation(NEventIndexType::kOutArcIndex, index, NUpdateNotifyOp::kOpFinished))

  return S_OK;
}
*/

/*
struct CTotalStats
{
  UInt64 Size;
  UInt64 PackSize;

  void UpdateWithItem(const CItemOut &item)
  {
    Size += item.Size;
    PackSize += item.PackSize;
  }
};

static HRESULT ReportArcProps(IArchiveUpdateCallbackArcProp *reportArcProp,
    CTotalStats &st)
{
  PROPVARIANT prop;
  prop.vt = VT_EMPTY;
  prop.wReserved1 = 0;
  {
    NWindows::NCOM::PropVarEm_Set_UInt64(&prop, st.Size);
    RINOK(reportArcProp->ReportProp(
      NEventIndexType::kArcProp, 0, kpidSize, &prop));
  }
  {
    NWindows::NCOM::PropVarEm_Set_UInt64(&prop, st.PackSize);
    RINOK(reportArcProp->ReportProp(
      NEventIndexType::kArcProp, 0, kpidPackSize, &prop));
  }
  return S_OK;
}
*/


static HRESULT Update2St(
    DECL_EXTERNAL_CODECS_LOC_VARS
    COutArchive &archive,
    CInArchive *inArchive,
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    const CUpdateOptions &updateOptions,
    const CCompressionMethodMode *options, bool outSeqMode,
    const CByteBuffer *comment,
    IArchiveUpdateCallback *updateCallback,
    UInt64 &totalComplexity,
    IArchiveUpdateCallbackFile *opCallback
    // , IArchiveUpdateCallbackArcProp *reportArcProp
    )
{
  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);

  CAddCommon compressor;
  compressor.SetOptions(*options);
  
  CObjectVector<CItemOut> items;
  UInt64 unpackSizeTotal = 0, packSizeTotal = 0;

  FOR_VECTOR (itemIndex, updateItems)
  {
    lps->InSize = unpackSizeTotal;
    lps->OutSize = packSizeTotal;
    RINOK(lps->SetCur())
    CUpdateItem &ui = updateItems[itemIndex];
    CItemEx itemEx;
    CItemOut item;

    if (!ui.NewProps || !ui.NewData)
    {
      // Note: for (ui.NewProps && !ui.NewData) it copies Props from old archive,
      // But we will rewrite all important properties later. But we can keep some properties like Comment
      itemEx = inputItems[(unsigned)ui.IndexInArc];
      if (inArchive->Read_LocalItem_After_CdItem_Full(itemEx) != S_OK)
        return E_NOTIMPL;
      (CItem &)item = itemEx;
    }

    if (ui.NewData)
    {
      // bool isDir = ((ui.NewProps) ? ui.IsDir : item.IsDir());
      bool isDir = ui.IsDir;
      if (isDir)
      {
        RINOK(WriteDirHeader(archive, options, ui, item))
      }
      else
      {
       CMyComPtr<ISequentialInStream> fileInStream;
       {
        HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
        if (res == S_FALSE)
        {
          lps->ProgressOffset += ui.Size;
          RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
          continue;
        }
        RINOK(res)
        if (!fileInStream)
          return E_INVALIDARG;

        bool inSeqMode = false;
        if (!inSeqMode)
        {
          CMyComPtr<IInStream> inStream2;
          fileInStream->QueryInterface(IID_IInStream, (void **)&inStream2);
          inSeqMode = (inStream2 == NULL);
        }
        // seqMode = true; // to test seqMode

        UpdatePropsFromStream(updateOptions, ui, fileInStream, updateCallback, totalComplexity);

        CCompressingResult compressingResult;
        
        RINOK(compressor.Set_Pre_CompressionResult(
            inSeqMode, outSeqMode,
            ui.Size,
            compressingResult))

        SetFileHeader(*options, ui, compressingResult.DescriptorMode, item);

        // file Size can be 64-bit !!!

        SetItemInfoFromCompressingResult(compressingResult, options->IsRealAesMode(), options->AesKeyMode, item);

        RINOK(archive.SetRestrictionFromCurrent())
        archive.WriteLocalHeader(item);

        CMyComPtr<IOutStream> outStream;
        archive.CreateStreamForCompressing(outStream);
        
        RINOK(compressor.Compress(
            EXTERNAL_CODECS_LOC_VARS
            fileInStream, outStream,
            inSeqMode, outSeqMode,
            ui.Time,
            ui.Size, ui.Size_WasSetFromStream,
            progress, compressingResult))
        
        if (item.HasDescriptor() != compressingResult.DescriptorMode)
          return E_FAIL;

        SetItemInfoFromCompressingResult(compressingResult, options->IsRealAesMode(), options->AesKeyMode, item);

        archive.WriteLocalHeader_Replace(item);
       }
       // if (reportArcProp) RINOK(ReportProps(reportArcProp, ui.IndexInClient, item, options->IsRealAesMode()))
       RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
       unpackSizeTotal += item.Size;
       packSizeTotal += item.PackSize;
      }
    }
    else
    {
      UInt64 complexity = 0;
      lps->SendRatio = false;

      RINOK(UpdateItemOldData(archive, inArchive, itemEx, ui, item, progress, opCallback, complexity))

      lps->SendRatio = true;
      lps->ProgressOffset += complexity;
    }
  
    items.Add(item);
    lps->ProgressOffset += kLocalHeaderSize;
  }

  lps->InSize = unpackSizeTotal;
  lps->OutSize = packSizeTotal;
  RINOK(lps->SetCur())

  RINOK(archive.WriteCentralDir(items, comment))

  /*
  CTotalStats stat;
  stat.Size = unpackSizeTotal;
  stat.PackSize = packSizeTotal;
  if (reportArcProp)
    RINOK(ReportArcProps(reportArcProp, stat))
  */

  lps->ProgressOffset += kCentralHeaderSize * updateItems.Size() + 1;
  return lps->SetCur();
}

#ifndef Z7_ST


static const size_t kBlockSize = 1 << 16;
// kMemPerThread must be >= kBlockSize
//
static const size_t kMemPerThread = (size_t)sizeof(size_t) << 23;
// static const size_t kMemPerThread = (size_t)sizeof(size_t) << 16; // for debug
// static const size_t kMemPerThread = (size_t)1 << 16; // for debug

/*
in:
   nt_Zip >= 1:  the starting maximum number of ZIP threads for search
out:
   nt_Zip:  calculated number of ZIP threads
   returns: calculated number of ZSTD threads
*/
/*
static UInt32 CalcThreads_for_ZipZstd(CZstdEncProps *zstdProps,
    UInt64 memLimit, UInt32 totalThreads,
    UInt32 &nt_Zip)
{
  for (; nt_Zip > 1; nt_Zip--)
  {
    UInt64 mem1 = memLimit / nt_Zip;
    if (mem1 <= kMemPerThread)
      continue;
    mem1 -= kMemPerThread;
    UInt32 n_ZSTD = ZstdEncProps_GetNumThreads_for_MemUsageLimit(
        zstdProps, mem1, totalThreads / nt_Zip);
    // we don't allow (nbWorkers == 1) here
    if (n_ZSTD <= 1)
      n_ZSTD = 0;
    zstdProps->nbWorkers = n_ZSTD;
    mem1 = ZstdEncProps_GetMemUsage(zstdProps);
    if ((mem1 + kMemPerThread) * nt_Zip <= memLimit)
      return n_ZSTD;
  }
  return ZstdEncProps_GetNumThreads_for_MemUsageLimit(
      zstdProps, memLimit, totalThreads);
}


static UInt32 SetZstdThreads(
    const CCompressionMethodMode &options,
    COneMethodInfo *oneMethodMain,
    UInt32 numThreads,
    UInt32 numZipThreads_limit,
    UInt64 numFilesToCompress,
    UInt64 numBytesToCompress)
{
  NCompress::NZstd::CEncoderProps encoderProps;
  RINOK(encoderProps.SetFromMethodProps(*oneMethodMain));
  CZstdEncProps &zstdProps = encoderProps.EncProps;
  ZstdEncProps_NormalizeFull(&zstdProps);
  if (oneMethodMain->FindProp(NCoderPropID::kNumThreads) >= 0)
  {
    // threads for ZSTD are fixed
    if (zstdProps.nbWorkers > 1)
      numThreads /= zstdProps.nbWorkers;
    if (numThreads > numZipThreads_limit)
      numThreads = numZipThreads_limit;
    if (options._memUsage_WasSet
        && !options._numThreads_WasForced)
    {
      const UInt64 mem1 = ZstdEncProps_GetMemUsage(&zstdProps);
      const UInt64 numZipThreads = options._memUsage_Compress / (mem1 + kMemPerThread);
      if (numThreads > numZipThreads)
        numThreads = (UInt32)numZipThreads;
    }
    return numThreads;
  }
  {
    // threads for ZSTD are not fixed

    // calculate estimated required number of ZST threads per file size statistics
    UInt32 t = MY_ZSTDMT_NBWORKERS_MAX;
    {
      UInt64 averageNumberOfBlocks = 0;
      const UInt64 averageSize = numBytesToCompress / numFilesToCompress;
      const UInt64 jobSize = zstdProps.jobSize;
      if (jobSize != 0)
        averageNumberOfBlocks = averageSize / jobSize + 0;
      if (t > averageNumberOfBlocks)
        t = (UInt32)averageNumberOfBlocks;
    }
    if (t > numThreads)
      t = numThreads;

    // calculate the nuber of zip threads
    UInt32 numZipThreads = numThreads;
    if (t > 1)
      numZipThreads = numThreads / t;
    if (numZipThreads > numZipThreads_limit)
      numZipThreads = numZipThreads_limit;
    if (numZipThreads < 1)
      numZipThreads = 1;
    {
      // recalculate the number of ZSTD threads via the number of ZIP threads
      const UInt32 t2 = numThreads / numZipThreads;
      if (t < t2)
        t = t2;
    }
    
    if (options._memUsage_WasSet
        && !options._numThreads_WasForced)
    {
      t = CalcThreads_for_ZipZstd(&zstdProps,
          options._memUsage_Compress, numThreads, numZipThreads);
      numThreads = numZipThreads;
    }
    // we don't use (nbWorkers = 1) here
    if (t <= 1)
      t = 0;
    oneMethodMain->AddProp_NumThreads(t);
    return numThreads;
  }
}
*/

#endif




static HRESULT Update2(
    DECL_EXTERNAL_CODECS_LOC_VARS
    COutArchive &archive,
    CInArchive *inArchive,
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    const CUpdateOptions &updateOptions,
    const CCompressionMethodMode &options, bool outSeqMode,
    const CByteBuffer *comment,
    IArchiveUpdateCallback *updateCallback)
{
  CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
  updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void **)&opCallback);

  /*
  CMyComPtr<IArchiveUpdateCallbackArcProp> reportArcProp;
  updateCallback->QueryInterface(IID_IArchiveUpdateCallbackArcProp, (void **)&reportArcProp);
  */

  bool unknownComplexity = false;
  UInt64 complexity = 0;
 #ifndef Z7_ST
  UInt64 numFilesToCompress = 0;
  UInt64 numBytesToCompress = 0;
 #endif
 
  unsigned i;
  
  for (i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &ui = updateItems[i];
    if (ui.NewData)
    {
      if (ui.Size == (UInt64)(Int64)-1)
        unknownComplexity = true;
      else
        complexity += ui.Size;
     #ifndef Z7_ST
      numBytesToCompress += ui.Size;
      numFilesToCompress++;
     #endif
      /*
      if (ui.Commented)
        complexity += ui.CommentRange.Size;
      */
    }
    else
    {
      CItemEx inputItem = inputItems[(unsigned)ui.IndexInArc];
      if (inArchive->Read_LocalItem_After_CdItem_Full(inputItem) != S_OK)
        return E_NOTIMPL;
      complexity += inputItem.GetLocalFullSize();
      // complexity += inputItem.GetCentralExtraPlusCommentSize();
    }
    complexity += kLocalHeaderSize;
    complexity += kCentralHeaderSize;
  }

  if (comment)
    complexity += comment->Size();
  complexity++; // end of central
  
  if (!unknownComplexity)
    updateCallback->SetTotal(complexity);

  UInt64 totalComplexity = complexity;

  CCompressionMethodMode options2 = options;

  if (options2._methods.IsEmpty())
  {
    // we need method item, if default method was used
    options2._methods.AddNew();
  }

  CAddCommon compressor;
  compressor.SetOptions(options2);
  
  complexity = 0;
  
  const Byte method = options.MethodSequence.FrontItem();

  COneMethodInfo *oneMethodMain = NULL;
  if (!options2._methods.IsEmpty())
    oneMethodMain = &options2._methods[0];

  {
    FOR_VECTOR (mi, options2._methods)
    {
      options2.SetGlobalLevelTo(options2._methods[mi]);
    }
  }

  if (oneMethodMain)
  {
    // appnote recommends to use EOS marker for LZMA.
    if (method == NFileHeader::NCompressionMethod::kLZMA)
      oneMethodMain->AddProp_EndMarker_if_NotFound(true);
  }


  #ifndef Z7_ST

  UInt32 numThreads = options._numThreads;
#ifdef _WIN32
  const UInt32 numThreadGroups = options._numThreadGroups;
#endif

  UInt32 numZipThreads_limit = numThreads;
  if (numZipThreads_limit > numFilesToCompress)
    numZipThreads_limit = (UInt32)numFilesToCompress;

  if (numZipThreads_limit > 1)
  {
    const unsigned numFiles_OPEN_MAX = NSystem::Get_File_OPEN_MAX_Reduced_for_3_tasks();
    // printf("\nzip:numFiles_OPEN_MAX =%d\n", (unsigned)numFiles_OPEN_MAX);
    if (numZipThreads_limit > numFiles_OPEN_MAX)
      numZipThreads_limit = (UInt32)numFiles_OPEN_MAX;
  }

  {
    // we reduce number of threads for 32-bit to reduce memory usege to 256 MB
    const UInt32 kNumMaxThreads =
        // _WIN32 (64-bit) supports only 64 threads in one group.
        8 << (sizeof(size_t) / 2); // 32 threads for 32-bit : 128 threads for 64-bit
    if (numThreads > kNumMaxThreads)
      numThreads = kNumMaxThreads;
  }
  /*
  if (numThreads > MAXIMUM_WAIT_OBJECTS) // is 64 in Windows
    numThreads = MAXIMUM_WAIT_OBJECTS;
  */


  /*
  // zstd supports (numThreads == 0);
  if (numThreads < 1)
    numThreads = 1;
  */

  bool mtMode = (numThreads > 1);

  if (numFilesToCompress <= 1)
    mtMode = false;

  // mtMode = true; // debug: to test mtMode

  if (!mtMode)
  {
    // if (oneMethodMain) {
    /*
    if (method == NFileHeader::NCompressionMethod::kZstdWz)
    {
      if (oneMethodMain->FindProp(NCoderPropID::kNumThreads) < 0)
      {
        // numZstdThreads was not forced in oneMethodMain
        if (numThreads >= 1
            && options._memUsage_WasSet
            && !options._numThreads_WasForced)
        {
          NCompress::NZstd::CEncoderProps encoderProps;
          RINOK(encoderProps.SetFromMethodProps(*oneMethodMain))
          CZstdEncProps &zstdProps = encoderProps.EncProps;
          ZstdEncProps_NormalizeFull(&zstdProps);
          numThreads = ZstdEncProps_GetNumThreads_for_MemUsageLimit(
              &zstdProps, options._memUsage_Compress, numThreads);
          // we allow (nbWorkers = 1) here.
        }
        oneMethodMain->AddProp_NumThreads(numThreads);
      }
    } // kZstdWz
    */
    // } // oneMethodMain

    FOR_VECTOR (mi, options2._methods)
    {
      COneMethodInfo &onem = options2._methods[mi];

      if (onem.FindProp(NCoderPropID::kNumThreads) < 0)
      {
        // fixme: we should check the number of threads for xz method also
        // fixed for 9.31. bzip2 default is just one thread.
        onem.AddProp_NumThreads(numThreads);
      }
    }
  }
  else // mtMode
  {
    if (method == NFileHeader::NCompressionMethod::kStore && !options.Password_Defined)
      numThreads = 1;
    
   if (oneMethodMain)
   {

    if (method == NFileHeader::NCompressionMethod::kBZip2)
    {
      bool fixedNumber;
      UInt32 numBZip2Threads = oneMethodMain->Get_BZip2_NumThreads(fixedNumber);
      if (!fixedNumber)
      {
        const UInt64 averageSize = numBytesToCompress / numFilesToCompress;
        const UInt32 blockSize = oneMethodMain->Get_BZip2_BlockSize();
        const UInt64 averageNumberOfBlocks = averageSize / blockSize + 1;
        numBZip2Threads = 64;
        if (numBZip2Threads > averageNumberOfBlocks)
          numBZip2Threads = (UInt32)averageNumberOfBlocks;
        if (numBZip2Threads > numThreads)
          numBZip2Threads = numThreads;
        oneMethodMain->AddProp_NumThreads(numBZip2Threads);
      }
      numThreads /= numBZip2Threads;
    }
    else if (method == NFileHeader::NCompressionMethod::kXz)
    {
      UInt32 numLzmaThreads = 1;
      int numXzThreads = oneMethodMain->Get_Xz_NumThreads(numLzmaThreads);
      if (numXzThreads < 0)
      {
        // numXzThreads is unknown
        const UInt64 averageSize = numBytesToCompress / numFilesToCompress;
        const UInt64 blockSize = oneMethodMain->Get_Xz_BlockSize();
        UInt64 averageNumberOfBlocks = 1;
        if (blockSize != (UInt64)(Int64)-1)
          averageNumberOfBlocks = averageSize / blockSize + 1;
        UInt32 t = 256;
        if (t > averageNumberOfBlocks)
          t = (UInt32)averageNumberOfBlocks;
        t *= numLzmaThreads;
        if (t > numThreads)
          t = numThreads;
        oneMethodMain->AddProp_NumThreads(t);
        numXzThreads = (int)t;
      }
      numThreads /= (unsigned)numXzThreads;
    }
    /*
    else if (method == NFileHeader::NCompressionMethod::kZstdWz)
    {
      numThreads = SetZstdThreads(options,
          oneMethodMain, numThreads,
          numZipThreads_limit,
          numFilesToCompress, numBytesToCompress);
    }
    */
    else if (
           method == NFileHeader::NCompressionMethod::kDeflate
        || method == NFileHeader::NCompressionMethod::kDeflate64
        || method == NFileHeader::NCompressionMethod::kPPMd)
    {
      if (numThreads > 1
          && options._memUsage_WasSet
          && !options._numThreads_WasForced)
      {
        UInt64 methodMemUsage;
        if (method == NFileHeader::NCompressionMethod::kPPMd)
          methodMemUsage = oneMethodMain->Get_Ppmd_MemSize();
        else
          methodMemUsage = (4 << 20); // for deflate
        const UInt64 threadMemUsage = kMemPerThread + methodMemUsage;
        const UInt64 numThreads64 = options._memUsage_Compress / threadMemUsage;
        if (numThreads64 < numThreads)
          numThreads = (UInt32)numThreads64;
      }
    }
    else if (method == NFileHeader::NCompressionMethod::kLZMA)
    {
      // we suppose that default LZMA is 2 thread. So we don't change it
      const UInt32 numLZMAThreads = oneMethodMain->Get_Lzma_NumThreads();
      numThreads /= numLZMAThreads;

      if (numThreads > 1
          && options._memUsage_WasSet
          && !options._numThreads_WasForced)
      {
        const UInt64 methodMemUsage = oneMethodMain->Get_Lzma_MemUsage(true);
        const UInt64 threadMemUsage = kMemPerThread + methodMemUsage;
        const UInt64 numThreads64 = options._memUsage_Compress / threadMemUsage;
        if (numThreads64 < numThreads)
          numThreads = (UInt32)numThreads64;
      }
    }
   } // (oneMethodMain)

    if (numThreads > numZipThreads_limit)
      numThreads = numZipThreads_limit;
    if (numThreads <= 1)
    {
      mtMode = false;
      numThreads = 1;
    }
  }

  // mtMode = true; // to test mtMode for seqMode

  if (!mtMode)
  #endif
    return Update2St(
        EXTERNAL_CODECS_LOC_VARS
        archive, inArchive,
        inputItems, updateItems,
        updateOptions,
        &options2, outSeqMode,
        comment, updateCallback, totalComplexity,
        opCallback
        // , reportArcProp
        );


  #ifndef Z7_ST

  /*
  CTotalStats stat;
  stat.Size = 0;
  stat.PackSize = 0;
  */
  if (numThreads < 1)
    numThreads = 1;

  CObjectVector<CItemOut> items;

  CMtProgressMixer *mtProgressMixerSpec = new CMtProgressMixer;
  CMyComPtr<ICompressProgressInfo> progress = mtProgressMixerSpec;
  mtProgressMixerSpec->Create(updateCallback, true);

  CMtCompressProgressMixer mtCompressProgressMixer;
  mtCompressProgressMixer.Init(numThreads, mtProgressMixerSpec->RatioProgress);

  CMemBlockManagerMt memManager(kBlockSize);
  CMemRefs refs(&memManager);

  CMtSem mtSem;
  CThreads threads;
  mtSem.Head = -1;
  mtSem.Indexes.ClearAndSetSize(numThreads);
  {
    WRes wres = mtSem.Semaphore.Create(0, numThreads);
    if (wres != 0)
      return HRESULT_FROM_WIN32(wres);
  }

  CUIntVector threadIndices;  // list threads in order of updateItems

  {
    RINOK(memManager.AllocateSpaceAlways((size_t)numThreads * (kMemPerThread / kBlockSize)))
    for (i = 0; i < updateItems.Size(); i++)
      refs.Refs.Add(CMemBlocks2());

    for (i = 0; i < numThreads; i++)
    {
      threads.Threads.AddNew();
      // mtSem.Indexes[i] = -1; // actually we don't use these values
    }

    for (i = 0; i < numThreads; i++)
    {
      CThreadInfo &threadInfo = threads.Threads[i];
      threadInfo.ThreadIndex = i;
      threadInfo.SetOptions(options2);
      #ifdef Z7_EXTERNAL_CODECS
      threadInfo._externalCodecs = _externalCodecs;
      #endif
      RINOK(threadInfo.CreateEvents())
      threadInfo.OutStreamSpec = new COutMemStream(&memManager);
      RINOK(threadInfo.OutStreamSpec->CreateEvents(SYNC_WFMO(&memManager.Synchro)))
      threadInfo.OutStream = threadInfo.OutStreamSpec;
      threadInfo.ProgressSpec = new CMtCompressProgress();
      threadInfo.Progress = threadInfo.ProgressSpec;
      threadInfo.ProgressSpec->Init(&mtCompressProgressMixer, i);
      threadInfo.MtSem = &mtSem;
      const HRESULT hres =
        threadInfo.CreateThread_with_group(
#ifdef _WIN32
          (numThreadGroups > 1 && numThreads > 1) ?
            (int)(i % numThreadGroups) : -1
#endif
        );
      RINOK(hres)
    }
  }

  unsigned mtItemIndex = 0;
  unsigned itemIndex = 0;
  int lastRealStreamItemIndex = -1;

  
  while (itemIndex < updateItems.Size())
  {
    if (threadIndices.Size() < numThreads && mtItemIndex < updateItems.Size())
    {
      // we start ahead the threads for compressing
      // also we set refs.Refs[itemIndex].SeqMode that is used later
      // don't move that code block
      
      CUpdateItem &ui = updateItems[mtItemIndex++];
      if (!ui.NewData)
        continue;
      CItemEx itemEx;
      CItemOut item;
      
      if (ui.NewProps)
      {
        if (ui.IsDir)
          continue;
      }
      else
      {
        itemEx = inputItems[(unsigned)ui.IndexInArc];
        if (inArchive->Read_LocalItem_After_CdItem_Full(itemEx) != S_OK)
          return E_NOTIMPL;
        (CItem &)item = itemEx;
        if (item.IsDir() != ui.IsDir)
          return E_NOTIMPL;
        if (ui.IsDir)
          continue;
      }
      
      CMyComPtr<ISequentialInStream> fileInStream;
      
      CMemBlocks2 &memRef2 = refs.Refs[mtItemIndex - 1];

      {
        NWindows::NSynchronization::CCriticalSectionLock lock(mtProgressMixerSpec->Mixer2->CriticalSection);
        const HRESULT res = updateCallback->GetStream(ui.IndexInClient, &fileInStream);
        if (res == S_FALSE)
        {
          complexity += ui.Size;
          complexity += kLocalHeaderSize;
          mtProgressMixerSpec->Mixer2->SetProgressOffset_NoLock(complexity);
          RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
          memRef2.Skip = true;
          continue;
        }
        RINOK(res)
        if (!fileInStream)
          return E_INVALIDARG;
        UpdatePropsFromStream(updateOptions, ui, fileInStream, updateCallback, totalComplexity);
        RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK))
      }

      UInt32 k;
      for (k = 0; k < numThreads; k++)
        if (threads.Threads[k].IsFree)
          break;

      if (k == numThreads)
        return E_FAIL;
      {
        {
          CThreadInfo &threadInfo = threads.Threads[k];
          threadInfo.IsFree = false;
          threadInfo.InStream = fileInStream;

          bool inSeqMode = false;

          if (!inSeqMode)
          {
            CMyComPtr<IInStream> inStream2;
            fileInStream->QueryInterface(IID_IInStream, (void **)&inStream2);
            inSeqMode = (inStream2 == NULL);
          }
          memRef2.InSeqMode = inSeqMode;

          // !!!!! we must release ref before sending event
          // BUG was here in v4.43 and v4.44. It could change ref counter in two threads in same time
          fileInStream.Release();

          threadInfo.OutStreamSpec->Init();
          threadInfo.ProgressSpec->Reinit();
          
          threadInfo.UpdateIndex = mtItemIndex - 1;
          threadInfo.InSeqMode = inSeqMode;
          threadInfo.OutSeqMode = outSeqMode;
          threadInfo.FileTime = ui.Time; // FileTime is used for ZipCrypto only in seqMode
          threadInfo.ExpectedDataSize = ui.Size;
          threadInfo.ExpectedDataSize_IsConfirmed = ui.Size_WasSetFromStream;

          threadInfo.CompressEvent.Set();
          
          threadIndices.Add(k);
        }
      }
 
      continue;
    }
    
    if (refs.Refs[itemIndex].Skip)
    {
      itemIndex++;
      continue;
    }

    const CUpdateItem &ui = updateItems[itemIndex];

    CItemEx itemEx;
    CItemOut item;
    
    if (!ui.NewProps || !ui.NewData)
    {
      itemEx = inputItems[(unsigned)ui.IndexInArc];
      if (inArchive->Read_LocalItem_After_CdItem_Full(itemEx) != S_OK)
        return E_NOTIMPL;
      (CItem &)item = itemEx;
    }

    if (ui.NewData)
    {
      // bool isDir = ((ui.NewProps) ? ui.IsDir : item.IsDir());
      const bool isDir = ui.IsDir;
      
      if (isDir)
      {
        RINOK(WriteDirHeader(archive, &options, ui, item))
      }
      else
      {
        CMemBlocks2 &memRef = refs.Refs[itemIndex];
        
        if (memRef.Finished)
        {
          if (lastRealStreamItemIndex < (int)itemIndex)
            lastRealStreamItemIndex = (int)itemIndex;

          SetFileHeader(options, ui, memRef.CompressingResult.DescriptorMode, item);

          // the BUG was fixed in 9.26:
          // SetItemInfoFromCompressingResult must be after SetFileHeader
          // to write correct Size.

          SetItemInfoFromCompressingResult(memRef.CompressingResult,
              options.IsRealAesMode(), options.AesKeyMode, item);
          RINOK(archive.ClearRestriction())
          archive.WriteLocalHeader(item);
          // RINOK(updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
          CMyComPtr<ISequentialOutStream> outStream;
          archive.CreateStreamForCopying(outStream);
          memRef.WriteToStream(memManager.GetBlockSize(), outStream);
          // v23: we fixed the bug: we need to write descriptor also
          if (item.HasDescriptor())
          {
            /* that function doesn't rewrite local header, if item.HasDescriptor().
               it just writes descriptor */
            archive.WriteLocalHeader_Replace(item);
          }
          else
            archive.MoveCurPos(item.PackSize);
          memRef.FreeOpt(&memManager);
          /*
          if (reportArcProp)
          {
            stat.UpdateWithItem(item);
            RINOK(ReportProps(reportArcProp, ui.IndexInClient, item, options.IsRealAesMode()));
          }
          */
        }
        else
        {
          // current file was not finished

          if (lastRealStreamItemIndex < (int)itemIndex)
          {
            // LocalHeader was not written for current itemIndex still

            lastRealStreamItemIndex = (int)itemIndex;

            // thread was started before for that item already, and memRef.SeqMode was set

            CCompressingResult compressingResult;
            RINOK(compressor.Set_Pre_CompressionResult(
                memRef.InSeqMode, outSeqMode,
                ui.Size,
                compressingResult))

            memRef.PreDescriptorMode = compressingResult.DescriptorMode;
            SetFileHeader(options, ui, compressingResult.DescriptorMode, item);

            SetItemInfoFromCompressingResult(compressingResult, options.IsRealAesMode(), options.AesKeyMode, item);

            // file Size can be 64-bit !!!
            RINOK(archive.SetRestrictionFromCurrent())
            archive.WriteLocalHeader(item);
          }

          {
            CThreadInfo &thread = threads.Threads[threadIndices.FrontItem()];
            if (!thread.OutStreamSpec->WasUnlockEventSent())
            {
              CMyComPtr<IOutStream> outStream;
              archive.CreateStreamForCompressing(outStream);
              thread.OutStreamSpec->SetOutStream(outStream);
              thread.OutStreamSpec->SetRealStreamMode();
            }
          }

          const WRes wres = mtSem.Semaphore.Lock();
          if (wres != 0)
            return HRESULT_FROM_WIN32(wres);

          const int ti = mtSem.GetFreeItem();
          if (ti < 0)
            return E_FAIL;

          CThreadInfo &threadInfo = threads.Threads[(unsigned)ti];
          threadInfo.InStream.Release();
          threadInfo.IsFree = true;
          RINOK(threadInfo.Result)

          unsigned t = 0;

          for (;;)
          {
            if (t == threadIndices.Size())
              return E_FAIL;
            if (threadIndices[t] == (unsigned)ti)
              break;
            t++;
          }
          threadIndices.Delete(t);
          
          if (t == 0)
          {
            // if thread for current file was finished.
            if (threadInfo.UpdateIndex != itemIndex)
              return E_FAIL;

            if (memRef.PreDescriptorMode != threadInfo.CompressingResult.DescriptorMode)
              return E_FAIL;

            RINOK(threadInfo.OutStreamSpec->WriteToRealStream())
            threadInfo.OutStreamSpec->ReleaseOutStream();
            SetFileHeader(options, ui, threadInfo.CompressingResult.DescriptorMode, item);
            SetItemInfoFromCompressingResult(threadInfo.CompressingResult,
                options.IsRealAesMode(), options.AesKeyMode, item);

            archive.WriteLocalHeader_Replace(item);

            /*
            if (reportArcProp)
            {
              stat.UpdateWithItem(item);
              RINOK(ReportProps(reportArcProp, ui.IndexInClient, item, options.IsRealAesMode()));
            }
            */
          }
          else
          {
            // it's not current file. So we must store information in array
            CMemBlocks2 &memRef2 = refs.Refs[threadInfo.UpdateIndex];
            threadInfo.OutStreamSpec->DetachData(memRef2);
            memRef2.CompressingResult = threadInfo.CompressingResult;
            // memRef2.SeqMode = threadInfo.SeqMode; // it was set before
            memRef2.Finished = true;
            continue;
          }
        }
      }
    }
    else
    {
      RINOK(UpdateItemOldData(archive, inArchive, itemEx, ui, item, progress, opCallback, complexity))
    }
 
    items.Add(item);
    complexity += kLocalHeaderSize;
    mtProgressMixerSpec->Mixer2->SetProgressOffset(complexity);
    itemIndex++;
  }
  
  RINOK(mtCompressProgressMixer.SetRatioInfo(0, NULL, NULL))

  RINOK(archive.WriteCentralDir(items, comment))

  /*
  if (reportArcProp)
  {
    RINOK(ReportArcProps(reportArcProp, stat));
  }
  */

  complexity += kCentralHeaderSize * updateItems.Size() + 1;
  mtProgressMixerSpec->Mixer2->SetProgressOffset(complexity);
  return mtCompressProgressMixer.SetRatioInfo(0, NULL, NULL);

  #endif
}

/*
// we need CSeekOutStream, if we need Seek(0, STREAM_SEEK_CUR) for seqential stream
Z7_CLASS_IMP_COM_1(
  CSeekOutStream
  , IOutStream
)
  Z7_IFACE_COM7_IMP(ISequentialOutStream)

  CMyComPtr<ISequentialOutStream> _seqStream;
  UInt64 _size;
public:
  void Init(ISequentialOutStream *seqStream)
  {
    _size = 0;
    _seqStream = seqStream;
  }
};

Z7_COM7F_IMF(CSeekOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 realProcessedSize;
  const HRESULT result = _seqStream->Write(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize)
    *processedSize = realProcessedSize;
  return result;
}

Z7_COM7F_IMF(CSeekOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  if (seekOrigin != STREAM_SEEK_CUR || offset != 0)
    return E_NOTIMPL;
  if (newPosition)
    *newPosition = (UInt64)_size;
  return S_OK;
}

Z7_COM7F_IMF(CSeekOutStream::SetSize(UInt64 newSize))
{
  UNUSED_VAR(newSize)
  return E_NOTIMPL;
}
*/

static const size_t kCacheBlockSize = 1 << 20;
static const size_t kCacheSize = kCacheBlockSize << 2;
static const size_t kCacheMask = kCacheSize - 1;

Z7_CLASS_IMP_NOQIB_2(
  CCacheOutStream
  , IOutStream
  , IStreamSetRestriction
)
  Z7_IFACE_COM7_IMP(ISequentialOutStream)

  HRESULT _hres;
  CMyComPtr<ISequentialOutStream> _seqStream;
  CMyComPtr<IOutStream> _stream;
  CMyComPtr<IStreamSetRestriction> _setRestriction;
  Byte *_cache;
  size_t _cachedSize;
  UInt64 _cachedPos;
  UInt64 _virtPos;
  UInt64 _virtSize;
  UInt64 _phyPos;
  UInt64 _phySize;
  UInt64 _restrict_begin;
  UInt64 _restrict_end;

  HRESULT FlushFromCache(size_t size);
  HRESULT FlushNonRestrictedBlocks();
  HRESULT FlushCache();
  HRESULT SetRestriction_ForWrite(size_t writeSize) const;

  HRESULT SeekPhy(UInt64 pos)
  {
    if (pos == _phyPos)
      return S_OK;
    if (!_stream)
      return E_NOTIMPL;
    _hres = _stream->Seek((Int64)pos, STREAM_SEEK_SET, &_phyPos);
    if (_hres == S_OK && _phyPos != pos)
      _hres = E_FAIL;
    return _hres;
  }

public:
  CCacheOutStream(): _cache(NULL) {}
  ~CCacheOutStream();
  bool Allocate()
  {
    if (!_cache)
      _cache = (Byte *)::MidAlloc(kCacheSize);
    return _cache != NULL;
  }
  HRESULT Init(ISequentialOutStream *seqStream, IOutStream *stream, IStreamSetRestriction *setRestriction);
  HRESULT FinalFlush();
};

CCacheOutStream::~CCacheOutStream()
{
  ::MidFree(_cache);
}


HRESULT CCacheOutStream::Init(ISequentialOutStream *seqStream, IOutStream *stream, IStreamSetRestriction *setRestriction)
{
  _hres = S_OK;
  _cachedSize = 0;
  _cachedPos = 0;
  _virtPos = 0;
  _virtSize = 0;
  // by default we have no restriction
  _restrict_begin = 0;
  _restrict_end = 0;
  _seqStream = seqStream;
  _stream = stream;
  _setRestriction = setRestriction;
  if (_stream)
  {
    RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &_virtPos))
    RINOK(_stream->Seek(0, STREAM_SEEK_END, &_virtSize))
    RINOK(_stream->Seek((Int64)_virtPos, STREAM_SEEK_SET, &_virtPos))
  }
  _phyPos = _virtPos;
  _phySize = _virtSize;
  return S_OK;
}


/* we call SetRestriction_ForWrite() just before Write() from cache.
   (_phyPos == _cachedPos)
   (writeSize != 0)
*/
HRESULT CCacheOutStream::SetRestriction_ForWrite(size_t writeSize) const
{
  if (!_setRestriction)
    return S_OK;
  PRF(printf("\n-- CCacheOutStream::SetRestriction_ForWrite _cachedPos = 0x%x, writeSize = %d\n", (unsigned)_cachedPos, (unsigned)writeSize));
  UInt64 begin = _restrict_begin;
  UInt64 end = _restrict_end;
  const UInt64 phyPos = _phyPos;
  if (phyPos != _cachedPos) return E_FAIL;
  if (phyPos == _phySize)
  {
    // The writing will be to the end of phy stream.
    // So we will try to use non-restricted write, if possible.
    if (begin == end)
      begin = _virtPos; // _virtSize; // it's supposed that (_virtSize == _virtPos)
    if (phyPos + writeSize <= begin)
    {
      // the write is not restricted
      PRF(printf("\n+++ write is not restricted \n"));
      begin = 0;
      end = 0;
    }
    else
    {
      if (begin > phyPos)
        begin = phyPos;
      end = (UInt64)(Int64)-1;
    }
  }
  else
  {
    // (phyPos != _phySize)
    if (begin == end || begin > phyPos)
      begin = phyPos;
    end = (UInt64)(Int64)-1;
  }
  return _setRestriction->SetRestriction(begin, end);
}


/* it writes up to (size) bytes from cache.
   (size > _cachedSize) is allowed
*/
HRESULT CCacheOutStream::FlushFromCache(size_t size)
{
  PRF(printf("\n-- CCacheOutStream::FlushFromCache %u\n", (unsigned)size));
  if (_hres != S_OK)
    return _hres;
  if (size > _cachedSize)
      size = _cachedSize;
  // (size <= _cachedSize)
  if (size == 0)
    return S_OK;
  RINOK(SeekPhy(_cachedPos))
  for (;;)
  {
    // (_phyPos == _cachedPos)
    const size_t pos = (size_t)_cachedPos & kCacheMask;
    const size_t cur = MyMin(kCacheSize - pos, size);
    _hres = SetRestriction_ForWrite(cur);
    RINOK(_hres)
    PRF(printf("\n-- CCacheOutStream::WriteFromCache _phyPos = 0x%x, size = %d\n", (unsigned)_phyPos, (unsigned)cur));
    _hres = WriteStream(_seqStream, _cache + pos, cur);
    RINOK(_hres)
    _phyPos += cur;
    if (_phySize < _phyPos)
      _phySize = _phyPos;
    _cachedPos += cur;
    _cachedSize -= cur;
    size -= cur;
    if (size == 0)
      return S_OK;
  }
}


HRESULT CCacheOutStream::FlushNonRestrictedBlocks()
{
  for (;;)
  {
    const size_t size = kCacheBlockSize - ((size_t)_cachedPos & (kCacheBlockSize - 1));
    if (_cachedSize < size)
      break;
    UInt64 begin = _restrict_begin;
    if (begin == _restrict_end)
      begin = _virtPos;
    // we don't flush the data to restricted area
    if (_cachedPos + size > begin)
      break;
    RINOK(FlushFromCache(size))
  }
  return S_OK;
}


HRESULT CCacheOutStream::FlushCache()
{
  return FlushFromCache(_cachedSize);
}

HRESULT CCacheOutStream::FinalFlush()
{
  _restrict_begin = 0;
  _restrict_end = 0;
  RINOK(FlushCache())
  if (_stream && _hres == S_OK)
  {
    if (_virtSize != _phySize)
    {
      // it's unexpected
      RINOK(_stream->SetSize(_virtSize))
      _phySize = _virtSize;
    }
    _hres = SeekPhy(_virtPos);
  }
  return _hres;
}


Z7_COM7F_IMF(CCacheOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  PRF(printf("\n==== CCacheOutStream::Write virtPos=0x%x, %u\n", (unsigned)_virtPos, (unsigned)size));

  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  if (_hres != S_OK)
    return _hres;

  if (_cachedSize != 0)
  if (_virtPos < _cachedPos ||
      _virtPos > _cachedPos + _cachedSize)
  {
    RINOK(FlushCache())
  }

  if (_cachedSize == 0)
    _cachedPos = _virtPos;

  const size_t pos = (size_t)_virtPos & kCacheMask;
  {
    const size_t blockRem = kCacheBlockSize - ((size_t)_virtPos & (kCacheBlockSize - 1));
    if (size > blockRem)
      size = (UInt32)blockRem;
  }
  // _cachedPos <= _virtPos <= _cachedPos + _cachedSize
  const UInt64 cachedRem = _cachedPos + _cachedSize - _virtPos;
  if (cachedRem)
  {
    // _virtPos < _cachedPos + _cachedSize
    // we rewrite only existing data in cache. So _cachedSize will be not changed
    if (size > cachedRem)
      size = (UInt32)cachedRem;
  }
  else
  {
    // _virtPos == _cachedPos + _cachedSize
    // so we need to add new data to the end of cache
    if (_cachedSize == kCacheSize)
    {
      // cache is full. So we need to flush some part of cache.
      // we flush only one block, but we are allowed to flush any size here
      RINOK(FlushFromCache(kCacheBlockSize - ((size_t)_cachedPos & (kCacheBlockSize - 1))))
    }
    // _cachedSize != kCacheSize
    // so we have some space for new data in cache
    if (_cachedSize == 0)
    {
      /* this code is optional (for optimization):
         we write data directly without cache,
         if there is no restriction and we have full block. */
      if (_restrict_begin == _restrict_end
          && size == kCacheBlockSize)
      {
        RINOK(SeekPhy(_virtPos))
        if (_setRestriction)
        {
          _hres = _setRestriction->SetRestriction(_restrict_begin, _restrict_end);
          RINOK(_hres)
        }
        PRF(printf("\n-- CCacheOutStream::WriteDirectly _phyPos = 0x%x, size = %d\n", (unsigned)_phyPos, (unsigned)size));
        _hres = WriteStream(_seqStream, data, size);
        RINOK(_hres)
        if (processedSize)
          *processedSize = size;
        _virtPos += size;
        if (_virtSize < _virtPos)
          _virtSize = _virtPos;
        _phyPos += size;
        if (_phySize < _phyPos)
          _phySize = _phyPos;
        return S_OK;
      }
    }
    else // (_cachedSize != 0)
    {
      const size_t startPos = (size_t)_cachedPos & kCacheMask;
      // we don't allow new data to overwrite old start data in cache.
      // (startPos == pos) here means that cache is empty.
      // (startPos == pos) is not possible here.
      if (startPos > pos)
        size = (UInt32)MyMin((size_t)size, (size_t)(startPos - pos));
    }
    // _virtPos == (_cachedPos + _cachedSize) still
    _cachedSize += size;
  }
  
  memcpy(_cache + pos, data, size);
  if (processedSize)
    *processedSize = size;
  _virtPos += size;
  if (_virtSize < _virtPos)
    _virtSize = _virtPos;
  return FlushNonRestrictedBlocks();
}


Z7_COM7F_IMF(CCacheOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  PRF(printf("\n==== CCacheOutStream::Seek seekOrigin=%d Seek =%u\n", seekOrigin, (unsigned)offset));
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
    *newPosition = (UInt64)offset;
  return S_OK;
}


Z7_COM7F_IMF(CCacheOutStream::SetSize(UInt64 newSize))
{
  if (_hres != S_OK)
    return _hres;

  if (newSize <= _cachedPos || _cachedSize == 0)
  {
    _cachedSize = 0;
    _cachedPos = newSize;
  }
  else
  {
    // _cachedSize != 0
    // newSize > _cachedPos
    const UInt64 offset = newSize - _cachedPos;
    if (offset <= _cachedSize)
    {
      // newSize is inside cached block or is touching cached block.
      // so we reduce cache
      _cachedSize = (size_t)offset;
      if (_phySize <= newSize)
      {
        // _phySize will be restored later after cache flush
        _virtSize = newSize;
        return S_OK;
      }
      // (_phySize > newSize)
      // so we must reduce phyStream size to (newSize) or to (_cachedPos)
      // newPhySize = _cachedPos; // optional reduce to _cachedPos
    }
    else
    {
      // newSize > _cachedPos + _cachedSize
      /* It's possible that we need to write zeros,
         if new size is larger than old size.
         We don't optimize for possible cases here.
         So we just flush the cache. */
      _hres = FlushCache();
    }
  }

  _virtSize = newSize;

  if (_hres != S_OK)
    return _hres;

  if (newSize != _phySize)
  {
    if (!_stream)
      return E_NOTIMPL;
    // if (_phyPos > newSize)
    RINOK(SeekPhy(newSize))
    if (_setRestriction)
    {
      UInt64 begin = _restrict_begin;
      UInt64 end = _restrict_end;
      if (_cachedSize != 0)
      {
        if (begin > _cachedPos)
          begin = _cachedPos;
        end = (UInt64)(Int64)-1;
      }
      _hres = _setRestriction->SetRestriction(begin, end);
      RINOK(_hres)
    }
    _hres = _stream->SetSize(newSize);
    RINOK(_hres)
    _phySize = newSize;
  }
  return S_OK;
}


Z7_COM7F_IMF(CCacheOutStream::SetRestriction(UInt64 begin, UInt64 end))
{
  PRF(printf("\n============ CCacheOutStream::SetRestriction 0x%x, %u\n", (unsigned)begin, (unsigned)end));
  _restrict_begin = begin;
  _restrict_end = end;
  return FlushNonRestrictedBlocks();
}



HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CItemEx> &inputItems,
    CObjectVector<CUpdateItem> &updateItems,
    ISequentialOutStream *seqOutStream,
    CInArchive *inArchive, bool removeSfx,
    const CUpdateOptions &updateOptions,
    const CCompressionMethodMode &compressionMethodMode,
    IArchiveUpdateCallback *updateCallback)
{
  /*
  // it was tested before
  if (inArchive)
  {
    if (!inArchive->CanUpdate())
      return E_NOTIMPL;
  }
  */

  CMyComPtr<IStreamSetRestriction> setRestriction;
  seqOutStream->QueryInterface(IID_IStreamSetRestriction, (void **)&setRestriction);
  if (setRestriction)
  {
    RINOK(setRestriction->SetRestriction(0, 0))
  }

  CMyComPtr<IOutStream> outStream;
  CCacheOutStream *cacheStream;
  bool outSeqMode;

  {
    CMyComPtr<IOutStream> outStreamReal;

    if (!compressionMethodMode.Force_SeqOutMode)
    {
      seqOutStream->QueryInterface(IID_IOutStream, (void **)&outStreamReal);
      /*
      if (!outStreamReal)
        return E_NOTIMPL;
      */
    }

    if (inArchive)
    {
      if (!inArchive->IsMultiVol && inArchive->ArcInfo.Base > 0 && !removeSfx)
      {
        IInStream *baseStream = inArchive->GetBaseStream();
        RINOK(InStream_SeekToBegin(baseStream))
        RINOK(NCompress::CopyStream_ExactSize(baseStream, seqOutStream, (UInt64)inArchive->ArcInfo.Base, NULL))
      }
    }

    outSeqMode = (outStreamReal == NULL);
    if (outSeqMode)
      setRestriction.Release();
    /* CCacheOutStream works as non-restricted by default.
       So we use (setRestriction == NULL) for outSeqMode */
    // bool use_cacheStream = true;
    // if (use_cacheStream)
    {
      cacheStream = new CCacheOutStream();
      outStream = cacheStream;
      if (!cacheStream->Allocate())
        return E_OUTOFMEMORY;
      RINOK(cacheStream->Init(seqOutStream, outStreamReal, setRestriction))
      setRestriction.Release();
      if (!outSeqMode)
        setRestriction = cacheStream;
    }
    /*
    else if (!outStreamReal)
    {
      CSeekOutStream *seekOutStream = new CSeekOutStream();
      outStream = seekOutStream;
      seekOutStream->Init(seqOutStream);
    }
    else
      outStream = outStreamReal;
    */
  }

  COutArchive outArchive;
  outArchive.SetRestriction = setRestriction;

  RINOK(outArchive.Create(outStream))

  if (inArchive)
  {
    if (!inArchive->IsMultiVol && (Int64)inArchive->ArcInfo.MarkerPos2 > inArchive->ArcInfo.Base)
    {
      IInStream *baseStream = inArchive->GetBaseStream();
      RINOK(InStream_SeekSet(baseStream, (UInt64)inArchive->ArcInfo.Base))
      const UInt64 embStubSize = (UInt64)((Int64)inArchive->ArcInfo.MarkerPos2 - inArchive->ArcInfo.Base);
      RINOK(NCompress::CopyStream_ExactSize(baseStream, outStream, embStubSize, NULL))
      outArchive.MoveCurPos(embStubSize);
    }
  }

  RINOK (Update2(
      EXTERNAL_CODECS_LOC_VARS
      outArchive, inArchive,
      inputItems, updateItems,
      updateOptions,
      compressionMethodMode, outSeqMode,
      inArchive ? &inArchive->ArcInfo.Comment : NULL,
      updateCallback))

  return cacheStream->FinalFlush();
}

}}
