/* XArchive amalgamation of 7-Zip 26.01 -- Archive Zip/Nsis/Rar readers.
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

/* ---- CPP/7zip/Archive/Rar/StdAfx.h ---- */
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

/* ---- CPP/7zip/Common/MethodId.h ---- */
// MethodId.h

#ifndef ZIP7_INC_7Z_METHOD_ID_H
#define ZIP7_INC_7Z_METHOD_ID_H

// amalgamation: header emitted in prologue

typedef UInt64 CMethodId;

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

/* ---- CPP/7zip/Crypto/Rar5Aes.h ---- */
// Crypto/Rar5Aes.h

#ifndef ZIP7_INC_CRYPTO_RAR5_AES_H
#define ZIP7_INC_CRYPTO_RAR5_AES_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCrypto {
namespace NRar5 {

const unsigned kSaltSize = 16;
const unsigned kPswCheckSize32 = 2;
const unsigned kAesKeySize = 32;

namespace NCryptoFlags
{
  const unsigned kPswCheck = 1 << 0;
  const unsigned kUseMAC   = 1 << 1;
}

struct CKeyBase
{
protected:
  UInt32 _key32[kAesKeySize / 4];
  UInt32 _hashKey32[SHA256_NUM_DIGEST_WORDS];
  UInt32 _check_Calced32[kPswCheckSize32];

  void Wipe()
  {
    memset(this, 0, sizeof(*this));
  }
  
  void CopyCalcedKeysFrom(const CKeyBase &k)
  {
    *this = k;
  }
};

struct CKey: public CKeyBase
{
  CByteBuffer _password;
  bool _needCalc;
  unsigned _numIterationsLog;
  Byte _salt[kSaltSize];
  
  bool IsKeyEqualTo(const CKey &key)
  {
    return _numIterationsLog == key._numIterationsLog
        && memcmp(_salt, key._salt, sizeof(_salt)) == 0
        && _password == key._password;
  }

  CKey();
  ~CKey();
  
  void Wipe();

#ifdef Z7_CPP_IS_SUPPORTED_default
  // CKey(const CKey &) = default;
  CKey& operator =(const CKey &) = default;
#endif
};


class CDecoder Z7_final:
  public CAesCbcDecoder,
  public CKey
{
  UInt32 _check32[kPswCheckSize32];
  bool _canCheck;
  UInt64 Flags;

  bool IsThereCheck() const { return (Flags & NCryptoFlags::kPswCheck) != 0; }
public:
  Byte _iv[AES_BLOCK_SIZE];
  
  CDecoder();

  Z7_COM7F_IMP(Init())

  void SetPassword(const Byte *data, size_t size);
  HRESULT SetDecoderProps(const Byte *data, unsigned size, bool includeIV, bool isService);

  bool CalcKey_and_CheckPassword();

  bool UseMAC() const { return (Flags & NCryptoFlags::kUseMAC) != 0; }
  UInt32 Hmac_Convert_Crc32(UInt32 crc) const;
  void Hmac_Convert_32Bytes(Byte *data) const;
};

}}

#endif

/* ---- CPP/7zip/Archive/Common/FindSignature.h ---- */
// FindSignature.h

#ifndef ZIP7_INC_FIND_SIGNATURE_H
#define ZIP7_INC_FIND_SIGNATURE_H

// amalgamation: header emitted in prologue

HRESULT FindSignatureInStream(ISequentialInStream *stream,
    const Byte *signature, unsigned signatureSize,
    const UInt64 *limit, UInt64 &resPos);

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

/* ---- CPP/7zip/Archive/Rar/RarHeader.h ---- */
// Archive/RarHeader.h

#ifndef ZIP7_INC_ARCHIVE_RAR_HEADER_H
#define ZIP7_INC_ARCHIVE_RAR_HEADER_H

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NRar {
namespace NHeader {

const unsigned kMarkerSize = 7;
  
const unsigned kArchiveSolid = 0x1;

namespace NBlockType
{
  enum EBlockType
  {
    kMarker = 0x72,
    kArchiveHeader,
    kFileHeader,
    kCommentHeader,
    kOldAuthenticity,
    kOldSubBlock,
    kRecoveryRecord,
    kAuthenticity,
    kSubBlock,
    kEndOfArchive
  };
}

namespace NArchive
{
  const UInt16 kVolume  = 1;
  const UInt16 kComment = 2;
  const UInt16 kLock    = 4;
  const UInt16 kSolid   = 8;
  const UInt16 kNewVolName = 0x10; // ('volname.partN.rar')
  const UInt16 kAuthenticity  = 0x20;
  const UInt16 kRecovery = 0x40;
  const UInt16 kBlockEncryption  = 0x80;
  const UInt16 kFirstVolume = 0x100; // (set only by RAR 3.0 and later)

  // const UInt16 kEncryptVer = 0x200; // RAR 3.6 : that feature was discarded by origial RAR

  const UInt16 kEndOfArc_Flags_NextVol   = 1;
  const UInt16 kEndOfArc_Flags_DataCRC   = 2;
  const UInt16 kEndOfArc_Flags_RevSpace  = 4;
  const UInt16 kEndOfArc_Flags_VolNumber = 8;

  const unsigned kHeaderSizeMin = 7;
  
  const unsigned kArchiveHeaderSize = 13;

  const unsigned kBlockHeadersAreEncrypted = 0x80;
}

namespace NFile
{
  const unsigned kSplitBefore = 1 << 0;
  const unsigned kSplitAfter  = 1 << 1;
  const unsigned kEncrypted   = 1 << 2;
  const unsigned kComment     = 1 << 3;
  const unsigned kSolid       = 1 << 4;
  
  const unsigned kDictBitStart     = 5;
  const unsigned kNumDictBits  = 3;
  const unsigned kDictMask         = (1 << kNumDictBits) - 1;
  const unsigned kDictDirectoryValue  = 0x7;
  
  const unsigned kSize64Bits    = 1 << 8;
  const unsigned kUnicodeName   = 1 << 9;
  const unsigned kSalt          = 1 << 10;
  const unsigned kOldVersion    = 1 << 11;
  const unsigned kExtTime       = 1 << 12;
  // const unsigned kExtFlags      = 1 << 13;
  // const unsigned kSkipIfUnknown = 1 << 14;

  const unsigned kLongBlock    = 1 << 15;
  
  /*
  struct CBlock
  {
    // UInt16 HeadCRC;
    // Byte Type;
    // UInt16 Flags;
    // UInt16 HeadSize;
    UInt32 PackSize;
    UInt32 UnPackSize;
    Byte HostOS;
    UInt32 FileCRC;
    UInt32 Time;
    Byte UnPackVersion;
    Byte Method;
    UInt16 NameSize;
    UInt32 Attributes;
  };
  */

  /*
  struct CBlock32
  {
    UInt16 HeadCRC;
    Byte Type;
    UInt16 Flags;
    UInt16 HeadSize;
    UInt32 PackSize;
    UInt32 UnPackSize;
    Byte HostOS;
    UInt32 FileCRC;
    UInt32 Time;
    Byte UnPackVersion;
    Byte Method;
    UInt16 NameSize;
    UInt32 Attributes;
    UInt16 GetRealCRC(const void *aName, UInt32 aNameSize,
        bool anExtraDataDefined = false, Byte *anExtraData = 0) const;
  };
  struct CBlock64
  {
    UInt16 HeadCRC;
    Byte Type;
    UInt16 Flags;
    UInt16 HeadSize;
    UInt32 PackSizeLow;
    UInt32 UnPackSizeLow;
    Byte HostOS;
    UInt32 FileCRC;
    UInt32 Time;
    Byte UnPackVersion;
    Byte Method;
    UInt16 NameSize;
    UInt32 Attributes;
    UInt32 PackSizeHigh;
    UInt32 UnPackSizeHigh;
    UInt16 GetRealCRC(const void *aName, UInt32 aNameSize) const;
  };
  */
  
  const unsigned kLabelFileAttribute            = 0x08;
  const unsigned kWinFileDirectoryAttributeMask = 0x10;
  
  enum CHostOS
  {
    kHostMSDOS = 0,
    kHostOS2   = 1,
    kHostWin32 = 2,
    kHostUnix  = 3,
    kHostMacOS = 4,
    kHostBeOS  = 5
  };
}

namespace NBlock
{
  const UInt16 kLongBlock = 1 << 15;
  struct CBlock
  {
    UInt16 CRC;
    Byte Type;
    UInt16 Flags;
    UInt16 HeadSize;
    //  UInt32 DataSize;
  };
}

/*
struct CSubBlock
{
  UInt16 HeadCRC;
  Byte HeadType;
  UInt16 Flags;
  UInt16 HeadSize;
  UInt32 DataSize;
  UInt16 SubType;
  Byte Level; // Reserved : Must be 0
};

struct CCommentBlock
{
  UInt16 HeadCRC;
  Byte HeadType;
  UInt16 Flags;
  UInt16 HeadSize;
  UInt16 UnpSize;
  Byte UnpVer;
  Byte Method;
  UInt16 CommCRC;
};


struct CProtectHeader
{
  UInt16 HeadCRC;
  Byte HeadType;
  UInt16 Flags;
  UInt16 HeadSize;
  UInt32 DataSize;
  Byte Version;
  UInt16 RecSectors;
  UInt32 TotalBlocks;
  Byte Mark[8];
};
*/

}}}

#endif

/* ---- CPP/7zip/Archive/Rar/RarVol.h ---- */
// RarVol.h

#ifndef ZIP7_INC_ARCHIVE_RAR_VOL_H
#define ZIP7_INC_ARCHIVE_RAR_VOL_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NRar {

inline bool IsDigit(wchar_t c)
{
  return c >= L'0' && c <= L'9';
}

class CVolumeName
{
  bool _needChangeForNext;
  UString _before;
  UString _changed;
  UString _after;
public:
  CVolumeName(): _needChangeForNext(true) {}

  bool InitName(const UString &name, bool newStyle = true)
  {
    _needChangeForNext = true;
    _after.Empty();
    UString base (name);
    const int dotPos = name.ReverseFind_Dot();

    if (dotPos >= 0)
    {
      const UString ext (name.Ptr(dotPos + 1));
      if (ext.IsEqualTo_Ascii_NoCase("rar"))
      {
        _after = name.Ptr(dotPos);
        base.DeleteFrom(dotPos);
      }
      else if (ext.IsEqualTo_Ascii_NoCase("exe"))
      {
        _after = ".rar";
        base.DeleteFrom(dotPos);
      }
      else if (!newStyle)
      {
        if (ext.IsEqualTo_Ascii_NoCase("000") ||
            ext.IsEqualTo_Ascii_NoCase("001") ||
            ext.IsEqualTo_Ascii_NoCase("r00") ||
            ext.IsEqualTo_Ascii_NoCase("r01"))
        {
          _changed = ext;
          _before.SetFrom(name.Ptr(), (unsigned)dotPos + 1);
          return true;
        }
      }
    }

    if (newStyle)
    {
      unsigned k = base.Len();

      for (; k != 0; k--)
        if (IsDigit(base[k - 1]))
          break;

      unsigned i = k;

      for (; i != 0; i--)
        if (!IsDigit(base[i - 1]))
          break;

      if (i != k)
      {
        _before.SetFrom(base.Ptr(), i);
        _changed.SetFrom(base.Ptr(i), k - i);
        _after.Insert(0, base.Ptr(k));
        return true;
      }
    }
    
    _after.Empty();
    _before = base;
    _before.Add_Dot();
    _changed = "r00";
    _needChangeForNext = false;
    return true;
  }

  /*
  void MakeBeforeFirstName()
  {
    unsigned len = _changed.Len();
    _changed.Empty();
    for (unsigned i = 0; i < len; i++)
      _changed += L'0';
  }
  */

  UString GetNextName()
  {
    if (_needChangeForNext)
    {
      unsigned i = _changed.Len();
      if (i == 0)
        return UString();
      for (;;)
      {
        wchar_t c = _changed[--i];
        if (c == L'9')
        {
          c = L'0';
          _changed.ReplaceOneCharAtPos(i, c);
          if (i == 0)
          {
            _changed.InsertAtFront(L'1');
            break;
          }
          continue;
        }
        c++;
        _changed.ReplaceOneCharAtPos(i, c);
        break;
      }
    }
    
    _needChangeForNext = true;
    return _before + _changed + _after;
  }
};

}}

#endif

/* ---- C/Blake2.h ---- */
/* Blake2.h -- BLAKE2sp Hash
2024-01-17 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_BLAKE2_H
#define ZIP7_INC_BLAKE2_H

// amalgamation: header emitted in prologue

#if 0
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
#if defined(MY_CPU_X86_OR_AMD64)
#if defined(__SSE2__) \
    || defined(_MSC_VER) && _MSC_VER > 1200 \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 30300) \
    || defined(__clang__) \
    || defined(__INTEL_COMPILER)
#include <emmintrin.h> // SSE2
#endif

#if defined(__AVX2__) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100) \
    || defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
#include <immintrin.h>
#if defined(__clang__)
#include <avxintrin.h>
#include <avx2intrin.h>
#endif
#endif // avx2
#endif // MY_CPU_X86_OR_AMD64
#endif // 0

EXTERN_C_BEGIN

#define Z7_BLAKE2S_BLOCK_SIZE         64
#define Z7_BLAKE2S_DIGEST_SIZE        32
#define Z7_BLAKE2SP_PARALLEL_DEGREE   8
#define Z7_BLAKE2SP_NUM_STRUCT_WORDS  16

#if 1 || defined(Z7_BLAKE2SP_USE_FUNCTIONS)
typedef void (Z7_FASTCALL *Z7_BLAKE2SP_FUNC_COMPRESS)(UInt32 *states, const Byte *data, const Byte *end);
typedef void (Z7_FASTCALL *Z7_BLAKE2SP_FUNC_INIT)(UInt32 *states);
#endif

// it's required that CBlake2sp is aligned for 32-bytes,
// because the code can use unaligned access with sse and avx256.
// but 64-bytes alignment can be better.
MY_ALIGN(64)
typedef struct
{
  union
  {
#if 0
#if defined(MY_CPU_X86_OR_AMD64)
#if defined(__SSE2__) \
    || defined(_MSC_VER) && _MSC_VER > 1200 \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 30300) \
    || defined(__clang__) \
    || defined(__INTEL_COMPILER)
    __m128i _pad_align_128bit[4];
#endif // sse2
#if defined(__AVX2__) \
    || defined(Z7_GCC_VERSION) && (Z7_GCC_VERSION >= 40900) \
    || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 40600) \
    || defined(Z7_LLVM_CLANG_VERSION) && (Z7_LLVM_CLANG_VERSION >= 30100) \
    || defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL >= 1800) \
    || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1400)
    __m256i _pad_align_256bit[2];
#endif // avx2
#endif // x86
#endif // 0

    void * _pad_align_ptr[8];
    UInt32 _pad_align_32bit[16];
    struct
    {
      unsigned cycPos;
      unsigned _pad_unused;
#if 1 || defined(Z7_BLAKE2SP_USE_FUNCTIONS)
      Z7_BLAKE2SP_FUNC_COMPRESS func_Compress_Fast;
      Z7_BLAKE2SP_FUNC_COMPRESS func_Compress_Single;
      Z7_BLAKE2SP_FUNC_INIT func_Init;
      Z7_BLAKE2SP_FUNC_INIT func_Final;
#endif
    } header;
  } u;
  // MY_ALIGN(64)
  UInt32 states[Z7_BLAKE2SP_PARALLEL_DEGREE * Z7_BLAKE2SP_NUM_STRUCT_WORDS];
  // MY_ALIGN(64)
  UInt32  buf32[Z7_BLAKE2SP_PARALLEL_DEGREE * Z7_BLAKE2SP_NUM_STRUCT_WORDS * 2];
} CBlake2sp;

BoolInt Blake2sp_SetFunction(CBlake2sp *p, unsigned algo);
void Blake2sp_Init(CBlake2sp *p);
void Blake2sp_InitState(CBlake2sp *p);
void Blake2sp_Update(CBlake2sp *p, const Byte *data, size_t size);
void Blake2sp_Final(CBlake2sp *p, Byte *digest);
void z7_Black2sp_Prepare(void);

EXTERN_C_END

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

/* ---- CPP/7zip/Archive/Rar/Rar5Handler.h ---- */
// Rar5Handler.h

#ifndef ZIP7_INC_RAR5_HANDLER_H
#define ZIP7_INC_RAR5_HANDLER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NArchive {
namespace NRar5 {

namespace NHeaderFlags
{
  const unsigned kExtra   = 1 << 0;
  const unsigned kData    = 1 << 1;
  // const unsigned kUnknown = 1 << 2;
  const unsigned kPrevVol = 1 << 3;
  const unsigned kNextVol = 1 << 4;
  // const unsigned kIsChild = 1 << 5;
  // const unsigned kPreserveChild = 1 << 6;
}
  
namespace NHeaderType
{
  enum
  {
    kArc = 1,
    kFile,
    kService,
    kArcEncrypt,
    kEndOfArc
  };
}

namespace NArcFlags
{
  const unsigned kVol       = 1 << 0;
  const unsigned kVolNumber = 1 << 1;
  const unsigned kSolid     = 1 << 2;
  // const unsigned kRecovery  = 1 << 3;
  // const unsigned kLocked    = 1 << 4;
}

const unsigned kArcExtraRecordType_Locator  = 1;
const unsigned kArcExtraRecordType_Metadata = 2;

namespace NLocatorFlags
{
  const unsigned kQuickOpen  = 1 << 0;
  const unsigned kRecovery   = 1 << 1;
}

namespace NMetadataFlags
{
  const unsigned kArcName   = 1 << 0;
  const unsigned kCTime     = 1 << 1;
  const unsigned kUnixTime  = 1 << 2;
  const unsigned kNanoSec   = 1 << 3;
}

namespace NFileFlags
{
  const unsigned kIsDir    = 1 << 0;
  const unsigned kUnixTime = 1 << 1;
  const unsigned kCrc32    = 1 << 2;
  const unsigned kUnknownSize = 1 << 3;
}

namespace NMethodFlags
{
  // const unsigned kVersionMask = 0x3F;
  const unsigned kSolid = 1 << 6;
  const unsigned kRar5_Compat = 1u << 20;
}

namespace NArcEndFlags
{
  const unsigned kMoreVols = 1 << 0;
}

enum EHostOS
{
  kHost_Windows = 0,
  kHost_Unix
};



// ---------- Extra ----------

namespace NExtraID
{
  enum
  {
    kCrypto = 1,
    kHash,
    kTime,
    kVersion,
    kLink,
    kUnixOwner,
    kSubdata
  };
}

const unsigned kCryptoAlgo_AES = 0;

namespace NCryptoFlags
{
  const unsigned kPswCheck = 1 << 0;
  const unsigned kUseMAC   = 1 << 1;
}

struct CCryptoInfo
{
  UInt64 Algo;
  UInt64 Flags;
  Byte Cnt;

  bool UseMAC()       const { return (Flags & NCryptoFlags::kUseMAC) != 0; }
  bool IsThereCheck() const { return (Flags & NCryptoFlags::kPswCheck) != 0; }
  bool Parse(const Byte *p, size_t size);
};

const unsigned kHashID_Blake2sp = 0;

namespace NTimeRecord
{
  enum
  {
    k_Index_MTime = 0,
    k_Index_CTime,
    k_Index_ATime
  };
  
  namespace NFlags
  {
    const unsigned kUnixTime = 1 << 0;
    const unsigned kMTime    = 1 << 1;
    const unsigned kCTime    = 1 << 2;
    const unsigned kATime    = 1 << 3;
    const unsigned kUnixNs   = 1 << 4;
  }
}

namespace NLinkType
{
  enum
  {
    kUnixSymLink = 1,
    kWinSymLink,
    kWinJunction,
    kHardLink,
    kFileCopy
  };
}

namespace NLinkFlags
{
  const unsigned kTargetIsDir = 1 << 0;
}


struct CLinkInfo
{
  UInt64 Type;
  UInt64 Flags;
  unsigned NameOffset;
  unsigned NameLen;
  
  bool Parse(const Byte *p, unsigned size);
};


struct CItem
{
  UInt32 CommonFlags;
  UInt32 Flags;

  Byte RecordType;
  bool Version_Defined;

  int ACL;

  AString Name;

  unsigned VolIndex;
  int NextItem;        // in _items{}

  UInt32 UnixMTime;
  UInt32 CRC;
  UInt32 Attrib;
  UInt32 Method;

  CByteBuffer Extra;

  UInt64 Size;
  UInt64 PackSize;
  UInt64 HostOS;
  
  UInt64 DataPos;
  UInt64 Version;

  CItem() { Clear(); }

  void Clear()
  {
    // CommonFlags = 0;
    // Flags = 0;
    
    // UnixMTime = 0;
    // CRC = 0;

    VolIndex = 0;
    NextItem = -1;

    Version_Defined = false;
    Version = 0;

    Name.Empty();
    Extra.Free();
    ACL = -1;
  }

  bool IsSplitBefore()  const { return (CommonFlags & NHeaderFlags::kPrevVol) != 0; }
  bool IsSplitAfter()   const { return (CommonFlags & NHeaderFlags::kNextVol) != 0; }
  bool IsSplit()        const { return (CommonFlags & (NHeaderFlags::kPrevVol | NHeaderFlags::kNextVol)) != 0; }

  bool IsDir()          const { return (Flags & NFileFlags::kIsDir) != 0; }
  bool Has_UnixMTime()  const { return (Flags & NFileFlags::kUnixTime) != 0; }
  bool Has_CRC()        const { return (Flags & NFileFlags::kCrc32) != 0; }
  bool Is_UnknownSize() const { return (Flags & NFileFlags::kUnknownSize) != 0; }

  bool IsNextForItem(const CItem &prev) const
  {
    return !IsDir() && !prev.IsDir() && IsSplitBefore() && prev.IsSplitAfter() && (Name == prev.Name);
      // && false;
  }

  // rar docs: Solid flag can be set only for file headers and is never set for service headers.
  bool IsSolid()        const { return ((UInt32)Method & NMethodFlags::kSolid) != 0; }
  bool Is_Rar5_Compat() const { return ((UInt32)Method & NMethodFlags::kRar5_Compat) != 0; }
  unsigned Get_Rar5_CompatBit() const { return ((UInt32)Method >> 20) & 1; }

  unsigned Get_AlgoVersion_RawBits() const { return (unsigned)Method & 0x3F; }
  unsigned Get_AlgoVersion_HuffRev() const
  {
    unsigned w = (unsigned)Method & 0x3F;
    if (w == 1 && Is_Rar5_Compat())
      w = 0;
    return w;
  }
  unsigned Get_Method() const { return ((unsigned)Method >> 7) & 0x7; }

  unsigned Get_DictSize_Main() const
    { return ((UInt32)Method >> 10) & (Get_AlgoVersion_RawBits() == 0 ? 0xf : 0x1f); }
  unsigned Get_DictSize_Frac() const
  {
    // original-unrar ignores Frac, if (algo==0) (rar5):
    if (Get_AlgoVersion_RawBits() == 0)
      return 0;
    return ((UInt32)Method >> 15) & 0x1f;
  }
  UInt64 Get_DictSize64() const
  {
    // ver 6.* check
    // return (((UInt32)Method >> 10) & 0xF);
    UInt64 winSize = 0;
    const unsigned algo = Get_AlgoVersion_RawBits();
    if (algo <= 1)
    {
      UInt32 w = 32;
      if (algo == 1)
        w += Get_DictSize_Frac();
      winSize = (UInt64)w << (12 + Get_DictSize_Main());
    }
    return winSize;
  }


  bool IsService() const { return RecordType == NHeaderType::kService; }
  
  bool Is_STM() const { return IsService() && Name.IsEqualTo("STM"); }
  bool Is_CMT() const { return IsService() && Name.IsEqualTo("CMT"); }
  bool Is_ACL() const { return IsService() && Name.IsEqualTo("ACL"); }
  // bool Is_QO()  const { return IsService() && Name.IsEqualTo("QO"); }

  int FindExtra(unsigned extraID, unsigned &recordDataSize) const;
  void PrintInfo(AString &s) const;


  bool IsEncrypted() const
  {
    unsigned size;
    return FindExtra(NExtraID::kCrypto, size) >= 0;
  }

  int FindExtra_Blake() const
  {
    unsigned size = 0;
    const int offset = FindExtra(NExtraID::kHash, size);
    if (offset >= 0
        && size == Z7_BLAKE2S_DIGEST_SIZE + 1
        && Extra[(unsigned)offset] == kHashID_Blake2sp)
      return offset + 1;
    return -1;
  }

  bool FindExtra_Version(UInt64 &version) const;

  bool FindExtra_Link(CLinkInfo &link) const;
  void Link_to_Prop(unsigned linkType, NWindows::NCOM::CPropVariant &prop) const;
  bool Is_CopyLink() const;
  bool Is_HardLink() const;
  bool Is_CopyLink_or_HardLink() const;

  bool NeedUse_as_CopyLink() const { return PackSize == 0 && Is_CopyLink(); }
  bool NeedUse_as_HardLink() const { return PackSize == 0 && Is_HardLink(); }
  bool NeedUse_as_CopyLink_or_HardLink() const { return PackSize == 0 && Is_CopyLink_or_HardLink(); }

  bool GetAltStreamName(AString &name) const;

  UInt32 GetWinAttrib() const
  {
    UInt32 a;
    switch (HostOS)
    {
      case kHost_Windows:
          a = Attrib;
          break;
      case kHost_Unix:
          a = Attrib << 16;
          a |= 0x8000; // add posix mode marker
          break;
      default:
          a = 0;
    }
    if (IsDir()) a |= FILE_ATTRIBUTE_DIRECTORY;
    return a;
  }

  UInt64 GetDataPosition() const { return DataPos; }
};


struct CInArcInfo
{
  UInt64 Flags;
  UInt64 VolNumber;
  UInt64 StartPos;
  UInt64 EndPos;

  UInt64 EndFlags;
  bool EndOfArchive_was_Read;

  bool IsEncrypted;
  bool Locator_Defined;
  bool Locator_Error;
  bool Metadata_Defined;
  bool Metadata_Error;
  bool UnknownExtraRecord;
  bool Extra_Error;
  bool UnsupportedFeature;

  struct CLocator
  {
    UInt64 Flags;
    UInt64 QuickOpen;
    UInt64 Recovery;
    
    bool Is_QuickOpen() const { return (Flags & NLocatorFlags::kQuickOpen) != 0; }
    bool Is_Recovery() const { return (Flags & NLocatorFlags::kRecovery) != 0; }

    bool Parse(const Byte *p, size_t size);
    CLocator():
      Flags(0),
      QuickOpen(0),
      Recovery(0)
      {}
  };

  struct CMetadata
  {
    UInt64 Flags;
    UInt64 CTime;
    AString ArcName;

    bool Parse(const Byte *p, size_t size);
    CMetadata():
      Flags(0),
      CTime(0)
      {}
  };

  CLocator Locator;
  CMetadata Metadata;

  bool ParseExtra(const Byte *p, size_t size);

  CInArcInfo():
    Flags(0),
    VolNumber(0),
    StartPos(0),
    EndPos(0),
    EndFlags(0),
    EndOfArchive_was_Read(false),
    IsEncrypted(false),
    Locator_Defined(false),
    Locator_Error(false),
    Metadata_Defined(false),
    Metadata_Error(false),
    UnknownExtraRecord(false),
    Extra_Error(false),
    UnsupportedFeature(false)
      {}

  /*
  void Clear()
  {
    Flags = 0;
    VolNumber = 0;
    StartPos = 0;
    EndPos = 0;
    EndFlags = 0;
    EndOfArchive_was_Read = false;
  }
  */

  UInt64 GetPhySize() const { return EndPos - StartPos; }

  bool AreMoreVolumes()  const { return (EndFlags & NArcEndFlags::kMoreVols) != 0; }

  bool IsVolume()             const { return (Flags & NArcFlags::kVol) != 0; }
  bool IsSolid()              const { return (Flags & NArcFlags::kSolid) != 0; }
  bool Is_VolNumber_Defined() const { return (Flags & NArcFlags::kVolNumber) != 0; }

  UInt64 GetVolIndex() const { return Is_VolNumber_Defined() ? VolNumber : 0; }
};


struct CRefItem
{
  unsigned Item;   // First item in _items[]
  unsigned Last;   // Last  item in _items[]
  int Parent;      // in _refs[], if alternate stream
  int Link;        // in _refs[]
};


struct CArc
{
  CMyComPtr<IInStream> Stream;
  CInArcInfo Info;
};


class CHandler Z7_final:
  public IInArchive,
  public IArchiveGetRawProps,
  public ISetProperties,
  Z7_PUBLIC_ISetCompressCodecsInfo_IFEC
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(IInArchive)
  Z7_COM_QI_ENTRY(IArchiveGetRawProps)
  Z7_COM_QI_ENTRY(ISetProperties)
  Z7_COM_QI_ENTRY_ISetCompressCodecsInfo_IFEC
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE
  
  Z7_IFACE_COM7_IMP(IInArchive)
  Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
  Z7_IFACE_COM7_IMP(ISetProperties)
  DECL_ISetCompressCodecsInfo

  void InitDefaults();

  bool _isArc;
  bool _needChecksumCheck;
  bool _memUsage_WasSet;
  bool _comment_WasUsedInArc;
  bool _acl_Used;
  bool _error_in_ACL;
  bool _split_Error;
public:
  CRecordVector<CRefItem> _refs;
  CObjectVector<CItem> _items;

  CHandler();
private:
  CObjectVector<CArc> _arcs;
  CObjectVector<CByteBuffer> _acls;

  UInt32 _errorFlags;
  // UInt32 _warningFlags;

  UInt32 _numBlocks;
  unsigned _rar5comapt_mask;
  unsigned _methodMasks[2];
  UInt64 _algo_Mask;
  UInt64 _dictMaxSizes[2];

  CByteBuffer _comment;
  UString _missingVolName;

  UInt64 _memUsage_Decompress;

  DECL_EXTERNAL_CODECS_VARS

  UInt64 GetPackSize(unsigned refIndex) const;
  
  void FillLinks();
  
  HRESULT Open2(IInStream *stream,
      const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *openCallback);
};

}}

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

/* ---- C/7zVersion.h ---- */
#define MY_VER_MAJOR 26
#define MY_VER_MINOR 1
#define MY_VER_BUILD 0
#define MY_VERSION_NUMBERS "26.01"
#define MY_VERSION MY_VERSION_NUMBERS

#ifdef MY_CPU_NAME
  #define MY_VERSION_CPU MY_VERSION " (" MY_CPU_NAME ")"
#else
  #define MY_VERSION_CPU MY_VERSION
#endif

#define MY_DATE "2026-04-27"
#undef MY_COPYRIGHT
#undef MY_VERSION_COPYRIGHT_DATE
#define MY_AUTHOR_NAME "Igor Pavlov"
#define MY_COPYRIGHT_PD "Igor Pavlov : Public domain"
#define MY_COPYRIGHT_CR "Copyright (c) 1999-2026 Igor Pavlov"

#ifdef USE_COPYRIGHT_CR
  #define MY_COPYRIGHT MY_COPYRIGHT_CR
#else
  #define MY_COPYRIGHT MY_COPYRIGHT_PD
#endif

#define MY_COPYRIGHT_DATE MY_COPYRIGHT " : " MY_DATE
#define MY_VERSION_COPYRIGHT_DATE MY_VERSION_CPU " : " MY_COPYRIGHT " : " MY_DATE

/* ---- CPP/7zip/MyVersion.h ---- */
#define USE_COPYRIGHT_CR
// amalgamation: header emitted in prologue

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

/* ---- CPP/7zip/Compress/PpmdZip.h ---- */
// PpmdZip.h

#ifndef ZIP7_INC_COMPRESS_PPMD_ZIP_H
#define ZIP7_INC_COMPRESS_PPMD_ZIP_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NPpmdZip {

struct CBuf
{
  Byte *Buf;
  
  CBuf(): Buf(NULL) {}
  ~CBuf() { ::MidFree(Buf); }
  bool Alloc();
};


Z7_CLASS_IMP_NOQIB_3(
  CDecoder
  , ICompressCoder
  , ICompressSetFinishMode
  , ICompressGetInStreamProcessedSize
)
  bool _fullFileMode;
  CByteInBufWrap _inStream;
  CBuf _outStream;
  CPpmd8 _ppmd;
public:
  CDecoder(bool fullFileMode = true);
  ~CDecoder();
};


struct CEncProps
{
  UInt32 MemSizeMB;
  UInt32 ReduceSize;
  int Order;
  int Restor;
  
  CEncProps()
  {
    MemSizeMB = (UInt32)(Int32)-1;
    ReduceSize = (UInt32)(Int32)-1;
    Order = -1;
    Restor = -1;
  }
  void Normalize(int level);
};


Z7_CLASS_IMP_NOQIB_2(
  CEncoder
  , ICompressCoder
  , ICompressSetCoderProperties
)
  CByteOutBufWrap _outStream;
  CBuf _inStream;
  CPpmd8 _ppmd;
  CEncProps _props;
public:
  CEncoder();
  ~CEncoder();
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

/* ---- CPP/7zip/Compress/ImplodeDecoder.h ---- */
// ImplodeDecoder.h

#ifndef ZIP7_INC_COMPRESS_IMPLODE_DECODER_H
#define ZIP7_INC_COMPRESS_IMPLODE_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NImplode {
namespace NDecoder {

typedef NBitl::CDecoder<CInBuffer> CInBit;

const unsigned kNumHuffmanBits = 16;
const unsigned kMaxHuffTableSize = 1 << 8;

class CHuffmanDecoder
{
  UInt32 _limits[kNumHuffmanBits + 1];
  UInt32 _poses[kNumHuffmanBits + 1];
  Byte _symbols[kMaxHuffTableSize];
public:
  bool Build(const Byte *lens, unsigned numSymbols) throw();
  unsigned Decode(CInBit *inStream) const throw();
};


Z7_CLASS_IMP_NOQIB_4(
  CCoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
  , ICompressSetFinishMode
  , ICompressGetInStreamProcessedSize
)
  Byte _flags;
  bool _fullStreamMode;

  CLzOutWindow _outWindowStream;
  CInBit _inBitStream;
  
  CHuffmanDecoder _litDecoder;
  CHuffmanDecoder _lenDecoder;
  CHuffmanDecoder _distDecoder;

  bool BuildHuff(CHuffmanDecoder &table, unsigned numSymbols);
  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
public:
  CCoder();
};

}}}

#endif

/* ---- CPP/7zip/Compress/ShrinkDecoder.h ---- */
// ShrinkDecoder.h

#ifndef ZIP7_INC_COMPRESS_SHRINK_DECODER_H
#define ZIP7_INC_COMPRESS_SHRINK_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NShrink {

const unsigned kNumMaxBits = 13;
const unsigned kNumItems = 1 << kNumMaxBits;

Z7_CLASS_IMP_NOQIB_3(
  CDecoder
  , ICompressCoder
  , ICompressSetFinishMode
  , ICompressGetInStreamProcessedSize
)
  bool _fullStreamMode;
  UInt64 _inProcessed;

  UInt16 _parents[kNumItems];
  Byte _suffixes[kNumItems];
  Byte _stack[kNumItems];

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
};

}}

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

/* ---- CPP/7zip/Crypto/ZipStrong.h ---- */
// Crypto/ZipStrong.h

#ifndef ZIP7_INC_CRYPTO_ZIP_STRONG_H
#define ZIP7_INC_CRYPTO_ZIP_STRONG_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCrypto {
namespace NZipStrong {

/* ICompressFilter::Init() does nothing for this filter.
  Call to init:
    Decoder:
      [CryptoSetPassword();]
      ReadHeader();
      [CryptoSetPassword();] Init_and_CheckPassword();
      [CryptoSetPassword();] Init_and_CheckPassword();
*/

struct CKeyInfo
{
  Byte MasterKey[32];
  UInt32 KeySize;
  
  void SetPassword(const Byte *data, UInt32 size);

  void Wipe()
  {
    Z7_memset_0_ARRAY(MasterKey);
  }
};


const unsigned kAesPadAllign = AES_BLOCK_SIZE;

Z7_CLASS_IMP_COM_2(
  CDecoder
  , ICompressFilter
  , ICryptoSetPassword
)
  CAesCbcDecoder *_cbcDecoder;
  CMyComPtr<ICompressFilter> _aesFilter;
  CKeyInfo _key;
  CAlignedBuffer _bufAligned;

  UInt32 _ivSize;
  Byte _iv[16];
  UInt32 _remSize;
public:
  HRESULT ReadHeader(ISequentialInStream *inStream, UInt32 crc, UInt64 unpackSize);
  HRESULT Init_and_CheckPassword(bool &passwOK);
  UInt32 GetPadSize(UInt32 packSize32) const
  {
    // Padding is to align to blockSize of cipher.
    // Change it, if is not AES
    return kAesPadAllign - (packSize32 & (kAesPadAllign - 1));
  }
  CDecoder();
  ~CDecoder() { Wipe(); }
  void Wipe()
  {
    Z7_memset_0_ARRAY(_iv);
    _key.Wipe();
  }
};

}}

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

/* ---- CPP/7zip/Archive/Common/ParseProperties.h ---- */
// ParseProperties.h

#ifndef ZIP7_INC_PARSE_PROPERTIES_H
#define ZIP7_INC_PARSE_PROPERTIES_H

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

/* ================ unit: CPP/7zip/Archive/Rar/Rar5Handler.cpp ================ */
// Rar5Handler.cpp

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

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

using namespace NWindows;

#define Get32(p) GetUi32(p)

namespace NArchive {
namespace NRar5 {

static const unsigned kMarkerSize = 8;

static const Byte kMarker[kMarkerSize] =
  { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x01, 0 };

// Comment length is limited to 256 KB in rar-encoder.
// So we use same limitation
static const size_t kCommentSize_Max = (size_t)1 << 18;


static const char * const kHostOS[] =
{
    "Windows"
  , "Unix"
};


static const char * const k_ArcFlags[] =
{
    "Volume"
  , "VolumeField"
  , "Solid"
  , "Recovery"
  , "Lock" // 4
};


static const char * const k_FileFlags[] =
{
    "Dir"
  , "UnixTime"
  , "CRC"
  , "UnknownSize"
};


static const char * const g_ExtraTypes[] =
{
    "0"
  , "Crypto"
  , "Hash"
  , "Time"
  , "Version"
  , "Link"
  , "UnixOwner"
  , "Subdata"
};


static const char * const g_LinkTypes[] =
{
    "0"
  , "UnixSymLink"
  , "WinSymLink"
  , "WinJunction"
  , "HardLink"
  , "FileCopy"
};


static const char g_ExtraTimeFlags[] = { 'u', 'M', 'C', 'A', 'n' };


static
Z7_NO_INLINE
unsigned ReadVarInt(const Byte *p, size_t maxSize, UInt64 *val_ptr)
{
  if (maxSize > 10)
      maxSize = 10;
  UInt64 val = 0;
  unsigned i;
  for (i = 0; i < maxSize;)
  {
    const unsigned b = p[i];
    val |= (UInt64)(b & 0x7F) << (7 * i);
    i++;
    if ((b & 0x80) == 0)
    {
      *val_ptr = val;
      return i;
    }
  }
  *val_ptr = 0;
#if 1
  return 0; // 7zip-unrar : strict check of error
#else
  return i; // original-unrar : ignore error
#endif
}


#define PARSE_VAR_INT(p, size, dest) \
{ const unsigned num_ = ReadVarInt(p, size, &dest);  \
  if (num_ == 0) return false; \
  p += num_; \
  size -= num_; \
}


bool CLinkInfo::Parse(const Byte *p, unsigned size)
{
  const Byte *pStart = p;
  UInt64 len;
  PARSE_VAR_INT(p, size, Type)
  PARSE_VAR_INT(p, size, Flags)
  PARSE_VAR_INT(p, size, len)
  if (size != len)
    return false;
  NameLen = (unsigned)len;
  NameOffset = (unsigned)(size_t)(p - pStart);
  return true;
}


static void AddHex64(AString &s, UInt64 v)
{
  char sz[32];
  sz[0] = '0';
  sz[1] = 'x';
  ConvertUInt64ToHex(v, sz + 2);
  s += sz;
}


static void PrintType(AString &s, const char * const table[], unsigned num, UInt64 val)
{
  char sz[32];
  const char *p = NULL;
  if (val < num)
    p = table[(unsigned)val];
  if (!p)
  {
    ConvertUInt64ToString(val, sz);
    p = sz;
  }
  s += p;
}


int CItem::FindExtra(unsigned extraID, unsigned &recordDataSize) const
{
  recordDataSize = 0;
  size_t offset = 0;

  for (;;)
  {
    size_t rem = Extra.Size() - offset;
    if (rem == 0)
      return -1;
    
    {
      UInt64 size;
      const unsigned num = ReadVarInt(Extra + offset, rem, &size);
      if (num == 0)
        return -1;
      offset += num;
      rem -= num;
      if (size > rem)
        return -1;
      rem = (size_t)size;
    }
    {
      UInt64 id;
      const unsigned num = ReadVarInt(Extra + offset, rem, &id);
      if (num == 0)
        return -1;
      offset += num;
      rem -= num;

      // There was BUG in RAR 5.21- : it stored (size-1) instead of (size)
      // for Subdata record in Service header.
      // That record always was last in bad archives, so we can fix that case.
      if (id == NExtraID::kSubdata
          && RecordType == NHeaderType::kService
          && rem + 1 == Extra.Size() - offset)
        rem++;

      if (id == extraID)
      {
        recordDataSize = (unsigned)rem;
        return (int)offset;
      }

      offset += rem;
    }
  }
}


void CItem::PrintInfo(AString &s) const
{
  size_t offset = 0;

  for (;;)
  {
    size_t rem = Extra.Size() - offset;
    if (rem == 0)
      return;
    
    {
      UInt64 size;
      unsigned num = ReadVarInt(Extra + offset, rem, &size);
      if (num == 0)
        return;
      offset += num;
      rem -= num;
      if (size > rem)
        break;
      rem = (size_t)size;
    }
    {
      UInt64 id;
      {
        unsigned num = ReadVarInt(Extra + offset, rem, &id);
        if (num == 0)
          break;
        offset += num;
        rem -= num;
      }

      // There was BUG in RAR 5.21- : it stored (size-1) instead of (size)
      // for Subdata record in Service header.
      // That record always was last in bad archives, so we can fix that case.
      if (id == NExtraID::kSubdata
          && RecordType == NHeaderType::kService
          && rem + 1 == Extra.Size() - offset)
        rem++;

      s.Add_Space_if_NotEmpty();
      PrintType(s, g_ExtraTypes, Z7_ARRAY_SIZE(g_ExtraTypes), id);

      if (id == NExtraID::kTime)
      {
        const Byte *p = Extra + offset;
        UInt64 flags;
        const unsigned num = ReadVarInt(p, rem, &flags);
        if (num != 0)
        {
          s.Add_Colon();
          for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_ExtraTimeFlags); i++)
            if ((flags & ((UInt64)1 << i)) != 0)
              s.Add_Char(g_ExtraTimeFlags[i]);
          flags &= ~(((UInt64)1 << Z7_ARRAY_SIZE(g_ExtraTimeFlags)) - 1);
          if (flags != 0)
          {
            s.Add_Char('_');
            AddHex64(s, flags);
          }
        }
      }
      else if (id == NExtraID::kLink)
      {
        CLinkInfo linkInfo;
        if (linkInfo.Parse(Extra + offset, (unsigned)rem))
        {
          s.Add_Colon();
          PrintType(s, g_LinkTypes, Z7_ARRAY_SIZE(g_LinkTypes), linkInfo.Type);
          UInt64 flags = linkInfo.Flags;
          if (flags != 0)
          {
            s.Add_Colon();
            if (flags & NLinkFlags::kTargetIsDir)
            {
              s.Add_Char('D');
              flags &= ~((UInt64)NLinkFlags::kTargetIsDir);
            }
            if (flags != 0)
            {
              s.Add_Char('_');
              AddHex64(s, flags);
            }
          }
        }
      }

      offset += rem;
    }
  }

  s.Add_OptSpaced("ERROR");
}


bool CCryptoInfo::Parse(const Byte *p, size_t size)
{
  Algo = 0;
  Flags = 0;
  Cnt = 0;
  PARSE_VAR_INT(p, size, Algo)
  PARSE_VAR_INT(p, size, Flags)
  if (size > 0)
    Cnt = p[0];
  if (size != 1 + 16 + 16 + (unsigned)(IsThereCheck() ? 12 : 0))
    return false;
  return true;
}


bool CItem::FindExtra_Version(UInt64 &version) const
{
  unsigned size;
  const int offset = FindExtra(NExtraID::kVersion, size);
  if (offset < 0)
    return false;
  const Byte *p = Extra + (unsigned)offset;

  UInt64 flags;
  PARSE_VAR_INT(p, size, flags)
  PARSE_VAR_INT(p, size, version)
  return size == 0;
}

bool CItem::FindExtra_Link(CLinkInfo &link) const
{
  unsigned size;
  const int offset = FindExtra(NExtraID::kLink, size);
  if (offset < 0)
    return false;
  if (!link.Parse(Extra + (unsigned)offset, size))
    return false;
  link.NameOffset += (unsigned)offset;
  return true;
}

bool CItem::Is_CopyLink() const
{
  CLinkInfo link;
  return FindExtra_Link(link) && link.Type == NLinkType::kFileCopy;
}

bool CItem::Is_HardLink() const
{
  CLinkInfo link;
  return FindExtra_Link(link) && link.Type == NLinkType::kHardLink;
}

bool CItem::Is_CopyLink_or_HardLink() const
{
  CLinkInfo link;
  return FindExtra_Link(link) && (link.Type == NLinkType::kFileCopy || link.Type == NLinkType::kHardLink);
}

void CItem::Link_to_Prop(unsigned linkType, NWindows::NCOM::CPropVariant &prop) const
{
  CLinkInfo link;
  if (!FindExtra_Link(link))
    return;

  bool isWindows = (HostOS == kHost_Windows);
  if (link.Type != linkType)
  {
    if (linkType != NLinkType::kUnixSymLink)
      return;
    switch ((unsigned)link.Type)
    {
      case NLinkType::kUnixSymLink:
        isWindows = false;
        break;
      case NLinkType::kWinSymLink:
      case NLinkType::kWinJunction:
        isWindows = true;
        break;
      default: return;
    }
  }

  AString s;
  s.SetFrom_CalcLen((const char *)(Extra + link.NameOffset), link.NameLen);
  UString unicode;
  ConvertUTF8ToUnicode(s, unicode);
  // rar5.0  used '\\' separator for windows symlinks and \??\ prefix for abs paths.
  // rar5.1+ uses '/'  separator for windows symlinks and /??/ prefix for abs paths.
  // v25.00: we convert Windows slashes to Linux slashes:
  if (isWindows)
    unicode.Replace(L'\\', L'/');
  prop = unicode;
  // prop = NItemName::GetOsPath(unicode);
}

bool CItem::GetAltStreamName(AString &name) const
{
  name.Empty();
  unsigned size;
  const int offset = FindExtra(NExtraID::kSubdata, size);
  if (offset < 0)
    return false;
  name.SetFrom_CalcLen((const char *)(Extra + (unsigned)offset), size);
  return true;
}


class CHash
{
  bool _calcCRC;
  UInt32 _crc;
  int _blakeOffset;
  CAlignedBuffer1 _buf;
  // CBlake2sp _blake;
  CBlake2sp *BlakeObj() { return (CBlake2sp *)(void *)(Byte *)_buf; }
public:
  CHash():
    _buf(sizeof(CBlake2sp))
    {}

  void Init_NoCalc()
  {
    _calcCRC = false;
    _crc = CRC_INIT_VAL;
    _blakeOffset = -1;
  }

  void Init(const CItem &item);
  void Update(const void *data, size_t size);
  UInt32 GetCRC() const { return CRC_GET_DIGEST(_crc); }

  bool Check(const CItem &item, NCrypto::NRar5::CDecoder *cryptoDecoder);
};

void CHash::Init(const CItem &item)
{
  _crc = CRC_INIT_VAL;
  _calcCRC = item.Has_CRC();
  _blakeOffset = item.FindExtra_Blake();
  if (_blakeOffset >= 0)
    Blake2sp_Init(BlakeObj());
}

void CHash::Update(const void *data, size_t size)
{
  if (_calcCRC)
    _crc = CrcUpdate(_crc, data, size);
  if (_blakeOffset >= 0)
    Blake2sp_Update(BlakeObj(), (const Byte *)data, size);
}

bool CHash::Check(const CItem &item, NCrypto::NRar5::CDecoder *cryptoDecoder)
{
  if (_calcCRC)
  {
    UInt32 crc = GetCRC();
    if (cryptoDecoder)
      crc = cryptoDecoder->Hmac_Convert_Crc32(crc);
    if (crc != item.CRC)
      return false;
  }
  if (_blakeOffset >= 0)
  {
    UInt32 digest[Z7_BLAKE2S_DIGEST_SIZE / sizeof(UInt32)];
    Blake2sp_Final(BlakeObj(), (Byte *)(void *)digest);
    if (cryptoDecoder)
      cryptoDecoder->Hmac_Convert_32Bytes((Byte *)(void *)digest);
    if (memcmp(digest, item.Extra + (unsigned)_blakeOffset, Z7_BLAKE2S_DIGEST_SIZE) != 0)
      return false;
  }
  return true;
}


Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithHash
  , ISequentialOutStream
)
  bool _size_Defined;
  ISequentialOutStream *_stream;
  UInt64 _pos;
  UInt64 _size;
  Byte *_destBuf;
public:
  CHash _hash;

  COutStreamWithHash(): _destBuf(NULL) {}

  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void Init(const CItem &item, Byte *destBuf, bool needChecksumCheck)
  {
    _size_Defined = false;
    _size = 0;
    _destBuf = NULL;
    if (!item.Is_UnknownSize())
    {
      _size_Defined = true;
      _size = item.Size;
      _destBuf = destBuf;
    }
    _pos = 0;
    if (needChecksumCheck)
      _hash.Init(item);
    else
      _hash.Init_NoCalc();
  }
  UInt64 GetPos() const { return _pos; }
};


Z7_COM7F_IMF(COutStreamWithHash::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  HRESULT result = S_OK;
  if (_size_Defined)
  {
    const UInt64 rem = _size - _pos;
    if (size > rem)
      size = (UInt32)rem;
  }
  if (_stream)
    result = _stream->Write(data, size, &size);
  if (_destBuf)
    memcpy(_destBuf + (size_t)_pos, data, size);
  _hash.Update(data, size);
  _pos += size;
  if (processedSize)
    *processedSize = size;
  return result;
}





class CInArchive
{
  CAlignedBuffer _buf;
  size_t _bufSize;
  size_t _bufPos;
  ISequentialInStream *_stream;

  CMyComPtr2<ICompressFilter, NCrypto::NRar5::CDecoder> m_CryptoDecoder;

  Z7_CLASS_NO_COPY(CInArchive)

  HRESULT ReadStream_Check(void *data, size_t size);

public:
  bool m_CryptoMode;

  bool WrongPassword;
  bool IsArc;
  bool UnexpectedEnd;

  UInt64 StreamStartPosition;
  UInt64 Position;
    
  size_t Get_Buf_RemainSize() const { return _bufSize - _bufPos; }
  bool Is_Buf_Finished() const { return _bufPos == _bufSize; }
  const Byte *Get_Buf_Data() const { return _buf + _bufPos; }
  void Move_BufPos(size_t num) { _bufPos += num; }
  bool ReadVar(UInt64 &val);

  struct CHeader
  {
    UInt64 Type;
    UInt64 Flags;
    size_t ExtraSize;
    UInt64 DataSize;
  };

  CInArchive() {}

  HRESULT ReadBlockHeader(CHeader &h);
  bool ReadFileHeader(const CHeader &header, CItem &item);
  void AddToSeekValue(UInt64 addValue)
  {
    Position += addValue;
  }

  HRESULT Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit, ICryptoGetTextPassword *getTextPassword,
      CInArcInfo &info);
};
  

static HRESULT MySetPassword(ICryptoGetTextPassword *getTextPassword, NCrypto::NRar5::CDecoder *cryptoDecoder)
{
  CMyComBSTR_Wipe password;
  RINOK(getTextPassword->CryptoGetTextPassword(&password))
  AString_Wipe utf8;
  const unsigned kPasswordLen_MAX = 127;
  UString_Wipe unicode;
  unicode.SetFromBstr(password);
  if (unicode.Len() > kPasswordLen_MAX)
    unicode.DeleteFrom(kPasswordLen_MAX);
  ConvertUnicodeToUTF8(unicode, utf8);
  cryptoDecoder->SetPassword((const Byte *)(const char *)utf8, utf8.Len());
  return S_OK;
}


bool CInArchive::ReadVar(UInt64 &val)
{
  const unsigned offset = ReadVarInt(Get_Buf_Data(), Get_Buf_RemainSize(), &val);
  Move_BufPos(offset);
  return (offset != 0);
}


HRESULT CInArchive::ReadStream_Check(void *data, size_t size)
{
  size_t size2 = size;
  RINOK(ReadStream(_stream, data, &size2))
  if (size2 == size)
    return S_OK;
  UnexpectedEnd = true;
  return S_FALSE;
}


HRESULT CInArchive::ReadBlockHeader(CHeader &h)
{
  h.Type = 0;
  h.Flags = 0;
  h.ExtraSize = 0;
  h.DataSize = 0;

  Byte buf[AES_BLOCK_SIZE];
  unsigned filled;
  
  if (m_CryptoMode)
  {
    _buf.AllocAtLeast(1 << 12); // at least (AES_BLOCK_SIZE * 2)
    if (!(Byte *)_buf)
      return E_OUTOFMEMORY;
    RINOK(ReadStream_Check(_buf, AES_BLOCK_SIZE * 2))
    memcpy(m_CryptoDecoder->_iv, _buf, AES_BLOCK_SIZE);
    RINOK(m_CryptoDecoder->Init())
    // we call RAR5_AES_Filter with:
    //   data_ptr  == aligned_ptr + 16
    //   data_size == 16
    if (m_CryptoDecoder->Filter(_buf + AES_BLOCK_SIZE, AES_BLOCK_SIZE) != AES_BLOCK_SIZE)
      return E_FAIL;
    memcpy(buf, _buf + AES_BLOCK_SIZE, AES_BLOCK_SIZE);
    filled = AES_BLOCK_SIZE;
  }
  else
  {
    const unsigned kStartSize = 4 + 3;
    RINOK(ReadStream_Check(buf, kStartSize))
    filled = kStartSize;
  }
  
  {
    UInt64 val;
    unsigned offset = ReadVarInt(buf + 4, 3, &val);
    if (offset == 0)
      return S_FALSE;
    size_t size = (size_t)val;
    if (size < 2)
      return S_FALSE;
    offset += 4;
    _bufPos = offset;
    size += offset;
    _bufSize = size;
    if (m_CryptoMode)
      size = (size + AES_BLOCK_SIZE - 1) & ~(size_t)(AES_BLOCK_SIZE - 1);
    _buf.AllocAtLeast(size);
    if (!(Byte *)_buf)
      return E_OUTOFMEMORY;
    memcpy(_buf, buf, filled);
    const size_t rem = size - filled;
    // if (m_CryptoMode), we add AES_BLOCK_SIZE here, because _iv is not included to size.
    AddToSeekValue(size + (m_CryptoMode ? AES_BLOCK_SIZE : 0));
    RINOK(ReadStream_Check(_buf + filled, rem))
    if (m_CryptoMode)
    {
      // we call RAR5_AES_Filter with:
      //   data_ptr  == aligned_ptr + 16
      //   (rem) can be big
      if (m_CryptoDecoder->Filter(_buf + filled, (UInt32)rem) != rem)
        return E_FAIL;
#if 1
      // optional 7zip-unrar check : remainder must contain zeros.
      const size_t pad = size - _bufSize;
      const Byte *p = _buf + _bufSize;
      for (size_t i = 0; i < pad; i++)
        if (p[i])
          return S_FALSE;
#endif
    }
  }

  if (CrcCalc(_buf + 4, _bufSize - 4) != Get32(buf))
    return S_FALSE;

  if (!ReadVar(h.Type)) return S_FALSE;
  if (!ReadVar(h.Flags)) return S_FALSE;

  if (h.Flags & NHeaderFlags::kExtra)
  {
    UInt64 extraSize;
    if (!ReadVar(extraSize))
      return S_FALSE;
    if (extraSize >= (1u << 21))
      return S_FALSE;
    h.ExtraSize = (size_t)extraSize;
  }
  
  if (h.Flags & NHeaderFlags::kData)
  {
    if (!ReadVar(h.DataSize))
      return S_FALSE;
  }
  
  if (h.ExtraSize > Get_Buf_RemainSize())
    return S_FALSE;
  return S_OK;
}


bool CInArcInfo::CLocator::Parse(const Byte *p, size_t size)
{
  Flags = 0;
  QuickOpen = 0;
  Recovery = 0;

  PARSE_VAR_INT(p, size, Flags)

  if (Is_QuickOpen())
  {
    PARSE_VAR_INT(p, size, QuickOpen)
  }
  if (Is_Recovery())
  {
    PARSE_VAR_INT(p, size, Recovery)
  }
#if 0
  // another records are possible in future rar formats.
  if (size != 0)
    return false;
#endif
  return true;
}


bool CInArcInfo::CMetadata::Parse(const Byte *p, size_t size)
{
  PARSE_VAR_INT(p, size, Flags)
  if (Flags & NMetadataFlags::kArcName)
  {
    UInt64 nameLen;
    PARSE_VAR_INT(p, size, nameLen)
    if (nameLen > size)
      return false;
    ArcName.SetFrom_CalcLen((const char *)(const void *)p, (unsigned)nameLen);
    p += (size_t)nameLen;
    size -= (size_t)nameLen;
  }
  if (Flags & NMetadataFlags::kCTime)
  {
    if ((Flags & NMetadataFlags::kUnixTime) &&
        (Flags & NMetadataFlags::kNanoSec) == 0)
    {
      if (size < 4)
        return false;
      CTime = GetUi32(p);
      p += 4;
      size -= 4;
    }
    else
    {
      if (size < 8)
        return false;
      CTime = GetUi64(p);
      p += 8;
      size -= 8;
    }
  }
#if 0
  // another records are possible in future rar formats.
  if (size != 0)
    return false;
#endif
  return true;
}


bool CInArcInfo::ParseExtra(const Byte *p, size_t size)
{
  for (;;)
  {
    if (size == 0)
      return true;
    UInt64 recSize64, id;
    PARSE_VAR_INT(p, size, recSize64)
    if (recSize64 > size)
      return false;
    size_t recSize = (size_t)recSize64;
    size -= recSize;
    // READ_VAR_INT(p, recSize, recSize)
    {
      const unsigned num = ReadVarInt(p, recSize, &id);
      if (num == 0)
        return false;
      p += num;
      recSize -= num;
    }
    if (id == kArcExtraRecordType_Metadata)
    {
      Metadata_Defined = true;
      if (!Metadata.Parse(p, recSize))
        Metadata_Error = true;
    }
    else if (id == kArcExtraRecordType_Locator)
    {
      Locator_Defined = true;
      if (!Locator.Parse(p, recSize))
        Locator_Error = true;
    }
    else
      UnknownExtraRecord = true;
    p += recSize;
  }
}



HRESULT CInArchive::Open(IInStream *stream, const UInt64 *searchHeaderSizeLimit, ICryptoGetTextPassword *getTextPassword,
    CInArcInfo &info)
{
  m_CryptoMode = false;
  
  WrongPassword = false;
  IsArc = false;
  UnexpectedEnd = false;

  Position = StreamStartPosition;

  UInt64 arcStartPos = StreamStartPosition;
  {
    Byte marker[kMarkerSize];
    RINOK(ReadStream_FALSE(stream, marker, kMarkerSize))
    if (memcmp(marker, kMarker, kMarkerSize) == 0)
      Position += kMarkerSize;
    else
    {
      if (searchHeaderSizeLimit && *searchHeaderSizeLimit == 0)
        return S_FALSE;
      RINOK(InStream_SeekSet(stream, StreamStartPosition))
      RINOK(FindSignatureInStream(stream, kMarker, kMarkerSize,
          searchHeaderSizeLimit, arcStartPos))
      arcStartPos += StreamStartPosition;
      Position = arcStartPos + kMarkerSize;
      RINOK(InStream_SeekSet(stream, Position))
    }
  }

  info.StartPos = arcStartPos;
  _stream = stream;

  CHeader h;
  RINOK(ReadBlockHeader(h))
  info.IsEncrypted = false;
  
  if (h.Type == NHeaderType::kArcEncrypt)
  {
    info.IsEncrypted = true;
    IsArc = true;
    if (!getTextPassword)
      return E_NOTIMPL;
    m_CryptoMode = true;
    m_CryptoDecoder.Create_if_Empty();
    RINOK(m_CryptoDecoder->SetDecoderProps(
        Get_Buf_Data(), (unsigned)Get_Buf_RemainSize(), false, false))
    RINOK(MySetPassword(getTextPassword, m_CryptoDecoder.ClsPtr()))
    if (!m_CryptoDecoder->CalcKey_and_CheckPassword())
    {
      WrongPassword = True;
      return S_FALSE;
    }
    RINOK(ReadBlockHeader(h))
  }

  if (h.Type != NHeaderType::kArc)
    return S_FALSE;

  IsArc = true;
  info.VolNumber = 0;
  
  if (!ReadVar(info.Flags))
    return S_FALSE;
  
  if (info.Flags & NArcFlags::kVolNumber)
    if (!ReadVar(info.VolNumber))
      return S_FALSE;
  
  if (h.ExtraSize != Get_Buf_RemainSize())
    return S_FALSE;
  if (h.ExtraSize)
  {
    if (!info.ParseExtra(Get_Buf_Data(), h.ExtraSize))
      info.Extra_Error = true;
  }
  return S_OK;
}


bool CInArchive::ReadFileHeader(const CHeader &header, CItem &item)
{
  item.CommonFlags = (UInt32)header.Flags;
  item.PackSize = header.DataSize;
  item.UnixMTime = 0;
  item.CRC = 0;

  {
    UInt64 flags64;
    if (!ReadVar(flags64)) return false;
    item.Flags = (UInt32)flags64;
  }

  if (!ReadVar(item.Size)) return false;
  
  {
    UInt64 attrib;
    if (!ReadVar(attrib)) return false;
    item.Attrib = (UInt32)attrib;
  }
  if (item.Has_UnixMTime())
  {
    if (Get_Buf_RemainSize() < 4)
      return false;
    item.UnixMTime = Get32(Get_Buf_Data());
    Move_BufPos(4);
  }
  if (item.Has_CRC())
  {
    if (Get_Buf_RemainSize() < 4)
      return false;
    item.CRC = Get32(Get_Buf_Data());
    Move_BufPos(4);
  }
  {
    UInt64 method;
    if (!ReadVar(method)) return false;
    item.Method = (UInt32)method;
  }

  if (!ReadVar(item.HostOS)) return false;

  {
    UInt64 len;
    if (!ReadVar(len)) return false;
    if (len > Get_Buf_RemainSize())
      return false;
    item.Name.SetFrom_CalcLen((const char *)Get_Buf_Data(), (unsigned)len);
    Move_BufPos((size_t)len);
  }
  
  item.Extra.Free();
  const size_t extraSize = header.ExtraSize;
  if (extraSize != 0)
  {
    if (Get_Buf_RemainSize() < extraSize)
      return false;
    item.Extra.Alloc(extraSize);
    memcpy(item.Extra, Get_Buf_Data(), extraSize);
    Move_BufPos(extraSize);
  }
  
  return Is_Buf_Finished();
}



struct CLinkFile
{
  unsigned Index;
  unsigned NumLinks; // the number of links to Data
  CByteBuffer Data;
  HRESULT Res;
  bool crcOK;

  CLinkFile(): Index(0), NumLinks(0), Res(S_OK), crcOK(true) {}
};


struct CUnpacker
{
  CMyComPtr2<ICompressCoder, NCompress::CCopyCoder> copyCoder;
  CMyComPtr<ICompressCoder> LzCoders[2];
  bool SolidAllowed;
  bool NeedCrc;
  CFilterCoder *filterStreamSpec;
  CMyComPtr<ISequentialInStream> filterStream;
  CMyComPtr2<ICompressFilter, NCrypto::NRar5::CDecoder> cryptoDecoder;
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  CMyComPtr2<ISequentialOutStream, COutStreamWithHash> outStream;

  CByteBuffer _tempBuf;
  CLinkFile *linkFile;

  CUnpacker(): linkFile(NULL) { SolidAllowed = false; NeedCrc = true; }

  HRESULT Create(DECL_EXTERNAL_CODECS_LOC_VARS
      const CItem &item, bool isSolid, bool &wrongPassword);

  HRESULT Code(const CItem &item, const CItem &lastItem, UInt64 packSize,
      ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress,
      bool &isCrcOK);

  HRESULT DecodeToBuf(DECL_EXTERNAL_CODECS_LOC_VARS
      const CItem &item, UInt64 packSize, ISequentialInStream *inStream, CByteBuffer &buffer);
};


static const unsigned kLzMethodMax = 5;

HRESULT CUnpacker::Create(DECL_EXTERNAL_CODECS_LOC_VARS
    const CItem &item, bool isSolid, bool &wrongPassword)
{
  wrongPassword = false;

  if (item.Get_AlgoVersion_RawBits() > 1)
    return E_NOTIMPL;

  outStream.Create_if_Empty();

  const unsigned method = item.Get_Method();

  if (method == 0)
    copyCoder.Create_if_Empty();
  else
  {
    if (method > kLzMethodMax)
      return E_NOTIMPL;
    /*
    if (item.IsSplitBefore())
      return S_FALSE;
    */
    const unsigned lzIndex = item.IsService() ? 1 : 0;
    CMyComPtr<ICompressCoder> &lzCoder = LzCoders[lzIndex];
    if (!lzCoder)
    {
      const UInt32 methodID = 0x40305;
      RINOK(CreateCoder_Id(EXTERNAL_CODECS_LOC_VARS methodID, false, lzCoder))
      if (!lzCoder)
        return E_NOTIMPL;
    }

    CMyComPtr<ICompressSetDecoderProperties2> csdp;
    RINOK(lzCoder.QueryInterface(IID_ICompressSetDecoderProperties2, &csdp))
    if (!csdp)
      return E_NOTIMPL;
    const unsigned ver = item.Get_AlgoVersion_HuffRev();
    if (ver > 1)
      return E_NOTIMPL;
    const Byte props[2] =
    {
      (Byte)item.Get_DictSize_Main(),
      (Byte)((item.Get_DictSize_Frac() << 3) + (ver << 1) + (isSolid ? 1 : 0))
    };
    RINOK(csdp->SetDecoderProperties2(props, 2))
  }

  unsigned cryptoSize = 0;
  const int cryptoOffset = item.FindExtra(NExtraID::kCrypto, cryptoSize);

  if (cryptoOffset >= 0)
  {
    if (!filterStream)
    {
      filterStreamSpec = new CFilterCoder(false);
      filterStream = filterStreamSpec;
    }

    cryptoDecoder.Create_if_Empty();

    RINOK(cryptoDecoder->SetDecoderProps(item.Extra + (unsigned)cryptoOffset, cryptoSize, true, item.IsService()))

    if (!getTextPassword)
    {
      wrongPassword = True;
      return E_NOTIMPL;
    }

    RINOK(MySetPassword(getTextPassword, cryptoDecoder.ClsPtr()))
      
    if (!cryptoDecoder->CalcKey_and_CheckPassword())
      wrongPassword = True;
  }

  return S_OK;
}


HRESULT CUnpacker::Code(const CItem &item, const CItem &lastItem, UInt64 packSize,
    ISequentialInStream *volsInStream, ISequentialOutStream *realOutStream, ICompressProgressInfo *progress,
    bool &isCrcOK)
{
  isCrcOK = true;

  const unsigned method = item.Get_Method();
  if (method > kLzMethodMax)
    return E_NOTIMPL;

  const bool needBuf = (linkFile && linkFile->NumLinks != 0);

  if (needBuf && !lastItem.Is_UnknownSize())
  {
    const size_t dataSize = (size_t)lastItem.Size;
    if (dataSize != lastItem.Size)
      return E_NOTIMPL;
    linkFile->Data.Alloc(dataSize);
  }

  bool isCryptoMode = false;
  ISequentialInStream *inStream;

  if (item.IsEncrypted())
  {
    filterStreamSpec->Filter = cryptoDecoder;
    filterStreamSpec->SetInStream(volsInStream);
    filterStreamSpec->SetOutStreamSize(NULL);
    inStream = filterStream;
    isCryptoMode = true;
  }
  else
    inStream = volsInStream;

  ICompressCoder *commonCoder = (method == 0) ?
      copyCoder.Interface() :
      LzCoders[item.IsService() ? 1 : 0].Interface();

  outStream->SetStream(realOutStream);
  outStream->Init(lastItem, (needBuf ? (Byte *)linkFile->Data : NULL), NeedCrc);

  HRESULT res = S_OK;
  if (packSize != 0 || lastItem.Is_UnknownSize() || lastItem.Size != 0)
  {
    res = commonCoder->Code(inStream, outStream, &packSize,
      lastItem.Is_UnknownSize() ? NULL : &lastItem.Size, progress);
    if (!item.IsService())
      SolidAllowed = true;
  }
  else
  {
    // res = res;
  }

  if (isCryptoMode)
    filterStreamSpec->ReleaseInStream();

  const UInt64 processedSize = outStream->GetPos();
  if (res == S_OK && !lastItem.Is_UnknownSize() && processedSize != lastItem.Size)
  {
    // rar_v7.13-: linux archive contains symLink with (packSize == 0 && lastItem.Size != 0)
    // v25.02: we ignore such record in rar headers:
    if (packSize != 0
        || method != 0
        || lastItem.HostOS != kHost_Unix
        || !MY_LIN_S_ISLNK(lastItem.Attrib))
      res = S_FALSE;
  }

  // if (res == S_OK)
  {
    unsigned cryptoSize = 0;
    const int cryptoOffset = lastItem.FindExtra(NExtraID::kCrypto, cryptoSize);
    NCrypto::NRar5::CDecoder *crypto = NULL;
    if (cryptoOffset >= 0)
    {
      CCryptoInfo cryptoInfo;
      if (cryptoInfo.Parse(lastItem.Extra + (unsigned)cryptoOffset, cryptoSize))
        if (cryptoInfo.UseMAC())
          crypto = cryptoDecoder.ClsPtr();
    }
    if (NeedCrc)
      isCrcOK =  outStream->_hash.Check(lastItem, crypto);
  }

  if (linkFile)
  {
    linkFile->Res = res;
    linkFile->crcOK = isCrcOK;
    if (needBuf
        && !lastItem.Is_UnknownSize()
        && processedSize != lastItem.Size)
      linkFile->Data.ChangeSize_KeepData((size_t)processedSize, (size_t)processedSize);
  }

  return res;
}


HRESULT CUnpacker::DecodeToBuf(DECL_EXTERNAL_CODECS_LOC_VARS
    const CItem &item, UInt64 packSize,
    ISequentialInStream *inStream,
    CByteBuffer &buffer)
{
  CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> out;
  _tempBuf.AllocAtLeast((size_t)item.Size);
  out->Init(_tempBuf, (size_t)item.Size);

  bool wrongPassword;

  if (item.IsSolid())
    return E_NOTIMPL;

  HRESULT res = Create(EXTERNAL_CODECS_LOC_VARS item, item.IsSolid(), wrongPassword);
  
  if (res == S_OK)
  {
    if (wrongPassword)
      return S_FALSE;

    CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> limitedStream;
    limitedStream->SetStream(inStream);
    limitedStream->Init(packSize);

    bool crcOK = true;
    res = Code(item, item, packSize, limitedStream, out, NULL, crcOK);
    if (res == S_OK)
    {
      if (!crcOK || out->GetPos() != item.Size)
        res = S_FALSE;
      else
        buffer.CopyFrom(_tempBuf, (size_t)item.Size);
    }
  }
  
  return res;
}


struct CTempBuf
{
  CByteBuffer _buf;
  size_t _offset;
  bool _isOK;

  void Clear()
  {
    _offset = 0;
    _isOK = true;
  }

  CTempBuf() { Clear(); }

  HRESULT Decode(DECL_EXTERNAL_CODECS_LOC_VARS
      const CItem &item,
      ISequentialInStream *inStream,
      CUnpacker &unpacker,
      CByteBuffer &destBuf);
};


HRESULT CTempBuf::Decode(DECL_EXTERNAL_CODECS_LOC_VARS
    const CItem &item,
    ISequentialInStream *inStream,
    CUnpacker &unpacker,
    CByteBuffer &destBuf)
{
  const size_t kPackSize_Max = (1 << 24);
  if (item.Size > (1 << 24)
      || item.Size == 0
      || item.PackSize >= kPackSize_Max)
  {
    Clear();
    return S_OK;
  }

  if (item.IsSplit() /* && _isOK */)
  {
    size_t packSize = (size_t)item.PackSize;
    if (packSize > kPackSize_Max - _offset)
      return S_OK;
    size_t newSize = _offset + packSize;
    if (newSize > _buf.Size())
      _buf.ChangeSize_KeepData(newSize, _offset);
    
    Byte *data = (Byte *)_buf + _offset;
    RINOK(ReadStream_FALSE(inStream, data, packSize))
    
    _offset += packSize;
    
    if (item.IsSplitAfter())
    {
      CHash hash;
      hash.Init(item);
      hash.Update(data, packSize);
      _isOK = hash.Check(item, NULL); // RAR5 doesn't use HMAC for packed part
    }
  }
  
  if (_isOK)
  {
    if (!item.IsSplitAfter())
    {
      if (_offset == 0)
      {
        RINOK(unpacker.DecodeToBuf(EXTERNAL_CODECS_LOC_VARS
            item, item.PackSize, inStream, destBuf))
      }
      else
      {
        CMyComPtr2_Create<ISequentialInStream, CBufInStream> bufInStream;
        bufInStream->Init(_buf, _offset);
        RINOK(unpacker.DecodeToBuf(EXTERNAL_CODECS_LOC_VARS
            item, _offset, bufInStream, destBuf))
      }
    }
  }

  return S_OK;
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
  // kpidPosixAttrib, // for debug

  kpidIsAltStream,
  kpidEncrypted,
  kpidSolid,
  kpidSplitBefore,
  kpidSplitAfter,
  kpidCRC,
  kpidHostOS,
  kpidMethod,
  kpidCharacts,
  kpidSymLink,
  kpidHardLink,
  kpidCopyLink,

  kpidVolumeIndex
};


static const Byte kArcProps[] =
{
  kpidTotalPhySize,
  kpidCharacts,
  kpidEncrypted,
  kpidSolid,
  kpidNumBlocks,
  kpidMethod,
  kpidIsVolume,
  kpidVolumeIndex,
  kpidNumVolumes,
  kpidName,
  kpidCTime,
  kpidComment
};


IMP_IInArchive_Props
IMP_IInArchive_ArcProps


UInt64 CHandler::GetPackSize(unsigned refIndex) const
{
  UInt64 size = 0;
  unsigned index = _refs[refIndex].Item;
  for (;;)
  {
    const CItem &item = _items[index];
    size += item.PackSize;
    if (item.NextItem < 0)
      return size;
    index = (unsigned)item.NextItem;
  }
}

static char *PrintDictSize(char *s, UInt64 w)
{
  char                               c = 'K'; w >>= 10;
  if ((w & ((1 << 10) - 1)) == 0)  { c = 'M'; w >>= 10;
  if ((w & ((1 << 10) - 1)) == 0)  { c = 'G'; w >>= 10; }}
  s = ConvertUInt64ToString(w, s);
  *s++ = c;
  *s = 0;
  return s;
}


Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN

  NCOM::CPropVariant prop;

  const CInArcInfo *arcInfo = NULL;
  if (!_arcs.IsEmpty())
    arcInfo = &_arcs[0].Info;

  switch (propID)
  {
    case kpidVolumeIndex: if (arcInfo && arcInfo->IsVolume()) prop = arcInfo->GetVolIndex(); break;
    case kpidSolid: if (arcInfo) prop = arcInfo->IsSolid(); break;
    case kpidCharacts:
    {
      AString s;
      if (arcInfo)
      {
        s = FlagsToString(k_ArcFlags, Z7_ARRAY_SIZE(k_ArcFlags), (UInt32)arcInfo->Flags);
        if (arcInfo->Extra_Error)
          s.Add_OptSpaced("Extra-ERROR");
        if (arcInfo->UnsupportedFeature)
          s.Add_OptSpaced("unsupported-feature");
        if (arcInfo->Metadata_Defined)
        {
          s.Add_OptSpaced("Metadata");
          if (arcInfo->Metadata_Error)
            s += "-ERROR";
          else
          {
            if (arcInfo->Metadata.Flags & NMetadataFlags::kArcName)
              s.Add_OptSpaced("arc-name");
            if (arcInfo->Metadata.Flags & NMetadataFlags::kCTime)
            {
              s.Add_OptSpaced("ctime-");
              s +=
                (arcInfo->Metadata.Flags & NMetadataFlags::kUnixTime) ?
                (arcInfo->Metadata.Flags & NMetadataFlags::kNanoSec) ?
                    "1ns" : "1s" : "win";
            }
          }
        }
        if (arcInfo->Locator_Defined)
        {
          s.Add_OptSpaced("Locator");
          if (arcInfo->Locator_Error)
            s += "-ERROR";
          else
          {
            if (arcInfo->Locator.Is_QuickOpen())
            {
              s.Add_OptSpaced("QuickOpen:");
              s.Add_UInt64(arcInfo->Locator.QuickOpen);
            }
            if (arcInfo->Locator.Is_Recovery())
            {
              s.Add_OptSpaced("Recovery:");
              s.Add_UInt64(arcInfo->Locator.Recovery);
            }
          }
        }
        if (arcInfo->UnknownExtraRecord)
          s.Add_OptSpaced("Unknown-Extra-Record");

      }
      if (_comment_WasUsedInArc)
      {
        s.Add_OptSpaced("Comment");
        // s.Add_UInt32((UInt32)_comment.Size());
      }
      //
      if (_acls.Size() != 0)
      {
        s.Add_OptSpaced("ACL");
        // s.Add_UInt32(_acls.Size());
      }
      if (!s.IsEmpty())
        prop = s;
      break;
    }
    case kpidEncrypted: if (arcInfo) prop = arcInfo->IsEncrypted; break; // it's for encrypted names.
    case kpidIsVolume: if (arcInfo) prop = arcInfo->IsVolume(); break;
    case kpidNumVolumes: prop = (UInt32)_arcs.Size(); break;
    case kpidOffset: if (arcInfo && arcInfo->StartPos != 0) prop = arcInfo->StartPos; break;

    case kpidTotalPhySize:
    {
      if (_arcs.Size() > 1)
      {
        UInt64 sum = 0;
        FOR_VECTOR (v, _arcs)
          sum += _arcs[v].Info.GetPhySize();
        prop = sum;
      }
      break;
    }

    case kpidPhySize:
    {
      if (arcInfo)
        prop = arcInfo->GetPhySize();
      break;
    }

    case kpidName:
      if (arcInfo)
      if (!arcInfo->Metadata_Error
          && !arcInfo->Metadata.ArcName.IsEmpty())
      {
        UString s;
        if (ConvertUTF8ToUnicode(arcInfo->Metadata.ArcName, s))
          prop = s;
      }
      break;

    case kpidCTime:
      if (arcInfo)
      if (!arcInfo->Metadata_Error
          && (arcInfo->Metadata.Flags & NMetadataFlags::kCTime))
      {
        const UInt64 ct = arcInfo->Metadata.CTime;
        if (arcInfo->Metadata.Flags & NMetadataFlags::kUnixTime)
        {
          if (arcInfo->Metadata.Flags & NMetadataFlags::kNanoSec)
          {
            const UInt64 sec = ct / 1000000000;
            const UInt64 ns  = ct % 1000000000;
            UInt64 wt = NTime::UnixTime64_To_FileTime64((Int64)sec);
            wt += ns / 100;
            const unsigned ns100 = (unsigned)(ns % 100);
            FILETIME ft;
            ft.dwLowDateTime = (DWORD)(UInt32)wt;
            ft.dwHighDateTime = (DWORD)(UInt32)(wt >> 32);
            prop.SetAsTimeFrom_FT_Prec_Ns100(ft, k_PropVar_TimePrec_1ns, ns100);
          }
          else
          {
            const UInt64 wt = NTime::UnixTime64_To_FileTime64((Int64)ct);
            prop.SetAsTimeFrom_Ft64_Prec(wt, k_PropVar_TimePrec_Unix);
          }
        }
        else
          prop.SetAsTimeFrom_Ft64_Prec(ct, k_PropVar_TimePrec_100ns);
      }
      break;

    case kpidComment:
    {
      // if (!_arcs.IsEmpty())
      {
        // const CArc &arc = _arcs[0];
        const CByteBuffer &cmt = _comment;
        if (cmt.Size() != 0 /* && cmt.Size() < (1 << 16) */)
        {
          AString s;
          s.SetFrom_CalcLen((const char *)(const Byte *)cmt, (unsigned)cmt.Size());
          UString unicode;
          ConvertUTF8ToUnicode(s, unicode);
          prop = unicode;
        }
      }
      break;
    }

    case kpidNumBlocks:
    {
      prop = (UInt32)_numBlocks;
      break;
    }

    case kpidMethod:
    {
      AString s;

      UInt64 algo = _algo_Mask;
      for (unsigned v = 0; algo != 0; v++, algo >>= 1)
      {
        if ((algo & 1) == 0)
          continue;
        s.Add_OptSpaced("v");
        s.Add_UInt32(v + 6);
        if (v < Z7_ARRAY_SIZE(_methodMasks))
        {
          const UInt64 dict = _dictMaxSizes[v];
          if (dict)
          {
            char temp[24];
            temp[0] = ':';
            PrintDictSize(temp + 1, dict);
            s += temp;
          }
          unsigned method = _methodMasks[v];
          for (unsigned m = 0; method; m++, method >>= 1)
          {
            if ((method & 1) == 0)
              continue;
            s += ":m";
            s.Add_UInt32(m);
          }
        }
      }
      if (_rar5comapt_mask & 2)
      {
        s += ":c";
        if (_rar5comapt_mask & 1)
          s.Add_Char('n');
      }
      prop = s;
      break;
    }
    
    case kpidError:
    {
      if (/* &_missingVol || */ !_missingVolName.IsEmpty())
      {
        UString s ("Missing volume : ");
        s += _missingVolName;
        prop = s;
      }
      break;
    }

    case kpidErrorFlags:
    {
      UInt32 v = _errorFlags;
      if (!_isArc)
        v |= kpv_ErrorFlags_IsNotArc;
      if (_error_in_ACL)
        v |= kpv_ErrorFlags_HeadersError;
      if (_split_Error)
        v |= kpv_ErrorFlags_HeadersError;
      prop = v;
      break;
    }

    /*
    case kpidWarningFlags:
    {
      if (_warningFlags != 0)
        prop = _warningFlags;
      break;
    }
    */

    case kpidExtension:
      if (_arcs.Size() == 1)
      {
        if (arcInfo->IsVolume())
        {
          AString s ("part");
          UInt32 v = (UInt32)arcInfo->GetVolIndex() + 1;
          if (v < 10)
            s.Add_Char('0');
          s.Add_UInt32(v);
          s += ".rar";
          prop = s;
        }
      }
      break;

    case kpidIsAltStream: prop = true; break;
  }

  prop.Detach(value);
  return S_OK;
  
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = (UInt32)_refs.Size();
  return S_OK;
}


static const Byte kRawProps[] =
{
  kpidChecksum,
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

  if (index >= _refs.Size())
    return S_OK;

  const CRefItem &ref = _refs[index];
  const CItem &item = _items[ref.Item];

  if (item.Is_STM() && ref.Parent >= 0)
  {
    *parent = (UInt32)ref.Parent;
    *parentType = NParentType::kAltStream;
  }

  return S_OK;
}


Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType))
{
  *data = NULL;
  *dataSize = 0;
  *propType = 0;

  if (index >= _refs.Size())
    return E_INVALIDARG;

  const CItem &item = _items[_refs[index].Item];

  if (propID == kpidNtSecure)
  {
    if (item.ACL >= 0)
    {
      const CByteBuffer &buf = _acls[item.ACL];
      *dataSize = (UInt32)buf.Size();
      *propType = NPropDataType::kRaw;
      *data = (const Byte *)buf;
    }
    return S_OK;
  }
  
  if (propID == kpidChecksum)
  {
    const int hashRecOffset = item.FindExtra_Blake();
    if (hashRecOffset >= 0)
    {
      *dataSize = Z7_BLAKE2S_DIGEST_SIZE;
      *propType = NPropDataType::kRaw;
      *data = item.Extra + (unsigned)hashRecOffset;
    }
    /*
    else if (item.Has_CRC() && item.IsEncrypted())
    {
      *dataSize = 4;
      *propType = NPropDataType::kRaw;
      *data = &item->CRC; // we must show same value for big/little endian here
    }
    */
    return S_OK;
  }
  
  return S_OK;
}


static void TimeRecordToProp(const CItem &item, unsigned stampIndex, NCOM::CPropVariant &prop)
{
  unsigned size;
  const int offset = item.FindExtra(NExtraID::kTime, size);
  if (offset < 0)
    return;

  const Byte *p = item.Extra + (unsigned)offset;
  UInt64 flags;
  // PARSE_VAR_INT(p, size, flags)
  {
    const unsigned num = ReadVarInt(p, size, &flags);
    if (num == 0)
      return;
    p += num;
    size -= num;
  }

  if ((flags & (NTimeRecord::NFlags::kMTime << stampIndex)) == 0)
    return;
  
  unsigned numStamps = 0;
  unsigned curStamp = 0;

  for (unsigned i = 0; i < 3; i++)
    if ((flags & (NTimeRecord::NFlags::kMTime << i)) != 0)
    {
      if (i == stampIndex)
        curStamp = numStamps;
      numStamps++;
    }

  FILETIME ft;

  unsigned timePrec = 0;
  unsigned ns100 = 0;

  if ((flags & NTimeRecord::NFlags::kUnixTime) != 0)
  {
    curStamp *= 4;
    if (curStamp + 4 > size)
      return;
    p += curStamp;
    UInt64 val = NTime::UnixTime_To_FileTime64(Get32(p));
    numStamps *= 4;
    timePrec = k_PropVar_TimePrec_Unix;
    if ((flags & NTimeRecord::NFlags::kUnixNs) != 0 && numStamps * 2 <= size)
    {
      const UInt32 ns = Get32(p + numStamps) & 0x3FFFFFFF;
      if (ns < 1000000000)
      {
        val += ns / 100;
        ns100 = (unsigned)(ns % 100);
        timePrec = k_PropVar_TimePrec_1ns;
      }
    }
    ft.dwLowDateTime = (DWORD)val;
    ft.dwHighDateTime = (DWORD)(val >> 32);
  }
  else
  {
    curStamp *= 8;
    if (curStamp + 8 > size)
      return;
    p += curStamp;
    ft.dwLowDateTime = Get32(p);
    ft.dwHighDateTime = Get32(p + 4);
  }
  
  prop.SetAsTimeFrom_FT_Prec_Ns100(ft, timePrec, ns100);
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  
  NCOM::CPropVariant prop;
  const CRefItem &ref = _refs[index];
  const CItem &item = _items[ref.Item];
  const CItem &lastItem = _items[ref.Last];

  switch (propID)
  {
    case kpidPath:
    {
      UString unicodeName;
      
      if (item.Is_STM())
      {
        AString s;
        if (ref.Parent >= 0)
        {
          const CItem &mainItem = _items[_refs[ref.Parent].Item];
          s = mainItem.Name;
        }

        AString name;
        item.GetAltStreamName(name);
        if (name[0] != ':')
          s.Add_Colon();
        s += name;
        ConvertUTF8ToUnicode(s, unicodeName);
      }
      else
      {
        ConvertUTF8ToUnicode(item.Name, unicodeName);

        if (item.Version_Defined)
        {
          char temp[32];
          // temp[0] = ';';
          // ConvertUInt64ToString(item.Version, temp + 1);
          // unicodeName += temp;
          ConvertUInt64ToString(item.Version, temp);
          UString s2 ("[VER]" STRING_PATH_SEPARATOR);
          s2 += temp;
          s2.Add_PathSepar();
          unicodeName.Insert(0, s2);
        }
      }
      
      NItemName::ReplaceToOsSlashes_Remove_TailSlash(unicodeName);
      prop = unicodeName;

      break;
    }
    
    case kpidIsDir: prop = item.IsDir(); break;
    case kpidSize: if (!lastItem.Is_UnknownSize()) prop = lastItem.Size; break;
    case kpidPackSize: prop = GetPackSize((unsigned)index); break;
    
    case kpidMTime:
    {
      TimeRecordToProp(item, NTimeRecord::k_Index_MTime, prop);
      if (prop.vt == VT_EMPTY && item.Has_UnixMTime())
        PropVariant_SetFrom_UnixTime(prop, item.UnixMTime);
      if (prop.vt == VT_EMPTY && ref.Parent >= 0)
      {
        const CItem &baseItem = _items[_refs[ref.Parent].Item];
        TimeRecordToProp(baseItem, NTimeRecord::k_Index_MTime, prop);
        if (prop.vt == VT_EMPTY && baseItem.Has_UnixMTime())
          PropVariant_SetFrom_UnixTime(prop, baseItem.UnixMTime);
      }
      break;
    }
    case kpidCTime: TimeRecordToProp(item, NTimeRecord::k_Index_CTime, prop); break;
    case kpidATime: TimeRecordToProp(item, NTimeRecord::k_Index_ATime, prop); break;

    case kpidName:
    {
      if (item.Is_STM())
      {
        AString name;
        item.GetAltStreamName(name);
        if (name[0] == ':')
        {
          name.DeleteFrontal(1);
          UString unicodeName;
          ConvertUTF8ToUnicode(name, unicodeName);
          prop = unicodeName;
        }
      }
      break;
    }

    case kpidIsAltStream: prop = item.Is_STM(); break;

    case kpidSymLink: item.Link_to_Prop(NLinkType::kUnixSymLink, prop); break;
    case kpidHardLink: item.Link_to_Prop(NLinkType::kHardLink, prop); break;
    case kpidCopyLink: item.Link_to_Prop(NLinkType::kFileCopy, prop); break;

    case kpidAttrib: prop = item.GetWinAttrib(); break;
    case kpidPosixAttrib:
      if (item.HostOS == kHost_Unix)
        prop = (UInt32)item.Attrib;
      break;

    case kpidEncrypted: prop = item.IsEncrypted(); break;
    case kpidSolid: prop = item.IsSolid(); break;

    case kpidSplitBefore: prop = item.IsSplitBefore(); break;
    case kpidSplitAfter: prop = lastItem.IsSplitAfter(); break;

    case kpidVolumeIndex:
    {
      if (item.VolIndex < _arcs.Size())
      {
        const CInArcInfo &arcInfo = _arcs[item.VolIndex].Info;
        if (arcInfo.IsVolume())
          prop = (UInt64)arcInfo.GetVolIndex();
      }
      break;
    }

    case kpidCRC:
    {
      const CItem *item2 = (lastItem.IsSplitAfter() ? &item : &lastItem);
      // we don't want to show crc for encrypted file here,
      // because crc is also encrrypted.
      if (item2->Has_CRC() && !item2->IsEncrypted())
        prop = item2->CRC;
      break;
    }

    case kpidMethod:
    {
      char temp[128];
      const unsigned algo = item.Get_AlgoVersion_RawBits();
      char *s = temp;
      // if (algo != 0)
      {
        *s++ = 'v';
        s = ConvertUInt32ToString((UInt32)algo + 6, s);
        if (item.Is_Rar5_Compat())
          *s++ = 'c';
        *s++ = ':';
      }
      {
        const unsigned m = item.Get_Method();
        *s++ = 'm';
        *s++ = (char)(m + '0');
        if (!item.IsDir())
        {
          *s++ = ':';
          const unsigned dictMain = item.Get_DictSize_Main();
          const unsigned frac = item.Get_DictSize_Frac();
          /*
          if (frac == 0 && algo == 0)
            s = ConvertUInt32ToString(dictMain + 17, s);
          else
          */
          s = PrintDictSize(s, (UInt64)(32 + frac) << (12 + dictMain));
          if (item.Is_Rar5_Compat())
          {
            *s++ = ':';
            *s++ = 'c';
          }
        }
      }
      unsigned cryptoSize = 0;
      const int cryptoOffset = item.FindExtra(NExtraID::kCrypto, cryptoSize);
      if (cryptoOffset >= 0)
      {
        *s++ = ' ';
        CCryptoInfo cryptoInfo;
        const bool isOK = cryptoInfo.Parse(item.Extra + (unsigned)cryptoOffset, cryptoSize);
        if (cryptoInfo.Algo == 0)
          s = MyStpCpy(s, "AES");
        else
        {
          s = MyStpCpy(s, "Crypto_");
          s = ConvertUInt64ToString(cryptoInfo.Algo, s);
        }
        if (isOK)
        {
          *s++ = ':';
          s = ConvertUInt32ToString(cryptoInfo.Cnt, s);
          *s++ = ':';
          s = ConvertUInt64ToString(cryptoInfo.Flags, s);
        }
      }
      *s = 0;
      prop = temp;
      break;
    }
    
    case kpidCharacts:
    {
      AString s;

      if (item.ACL >= 0)
        s.Add_OptSpaced("ACL");

      const UInt32 flags = item.Flags;
      if (flags != 0)
      {
        const AString s2 = FlagsToString(k_FileFlags, Z7_ARRAY_SIZE(k_FileFlags), flags);
        if (!s2.IsEmpty())
          s.Add_OptSpaced(s2);
      }

      item.PrintInfo(s);
    
      if (!s.IsEmpty())
        prop = s;
      break;
    }


    case kpidHostOS:
      if (item.HostOS < Z7_ARRAY_SIZE(kHostOS))
        prop = kHostOS[(size_t)item.HostOS];
      else
        prop = (UInt64)item.HostOS;
      break;
  }
  
  prop.Detach(value);
  return S_OK;
  
  COM_TRY_END
}



// ---------- Copy Links ----------

static int CompareItemsPaths(const CHandler &handler, unsigned p1, unsigned p2, const AString *name1)
{
  const CItem &item1 = handler._items[handler._refs[p1].Item];
  const CItem &item2 = handler._items[handler._refs[p2].Item];
  
  if (item1.Version_Defined)
  {
    if (!item2.Version_Defined)
      return -1;
    const int res = MyCompare(item1.Version, item2.Version);
    if (res != 0)
      return res;
  }
  else if (item2.Version_Defined)
    return 1;

  if (!name1)
    name1 = &item1.Name;
  return strcmp(*name1, item2.Name);
}

static int CompareItemsPaths2(const CHandler &handler, unsigned p1, unsigned p2, const AString *name1)
{
  const int res = CompareItemsPaths(handler, p1, p2, name1);
  if (res != 0)
    return res;
  return MyCompare(p1, p2);
}

static int CompareItemsPaths_Sort(const unsigned *p1, const unsigned *p2, void *param)
{
  return CompareItemsPaths2(*(const CHandler *)param, *p1, *p2, NULL);
}

static int FindLink(const CHandler &handler, const CUIntVector &sorted,
    const AString &s, unsigned index)
{
  unsigned left = 0, right = sorted.Size();
  for (;;)
  {
    if (left == right)
    {
      if (left > 0)
      {
        const unsigned refIndex = sorted[left - 1];
        if (CompareItemsPaths(handler, index, refIndex, &s) == 0)
          return (int)refIndex;
      }
      if (right < sorted.Size())
      {
        const unsigned refIndex = sorted[right];
        if (CompareItemsPaths(handler, index, refIndex, &s) == 0)
          return (int)refIndex;
      }
      return -1;
    }

    const unsigned mid = (left + right) / 2;
    const unsigned refIndex = sorted[mid];
    const int compare = CompareItemsPaths2(handler, index, refIndex, &s);
    if (compare == 0)
      return (int)refIndex;
    if (compare < 0)
      right = mid;
    else
      left = mid + 1;
  }
}

void CHandler::FillLinks()
{
  unsigned i;

  bool need_FillLinks = false;

  for (i = 0; i < _refs.Size(); i++)
  {
    const CItem &item = _items[_refs[i].Item];
    if (!item.IsDir()
        && !item.IsService()
        && item.NeedUse_as_CopyLink())
      need_FillLinks = true;

    if (!item.IsSolid())
      _numBlocks++;

    const unsigned algo = item.Get_AlgoVersion_RawBits();
    _algo_Mask |= (UInt64)1 << algo;
    _rar5comapt_mask |= 1u << item.Get_Rar5_CompatBit();
    if (!item.IsDir() && algo < Z7_ARRAY_SIZE(_methodMasks))
    {
      _methodMasks[algo] |= 1u << (item.Get_Method());
      UInt64 d = 32 + item.Get_DictSize_Frac();
      d <<= (12 + item.Get_DictSize_Main());
      if (_dictMaxSizes[algo] < d)
          _dictMaxSizes[algo] = d;
    }
  }

  if (!need_FillLinks)
    return;
  
  CUIntVector sorted;
  for (i = 0; i < _refs.Size(); i++)
  {
    const CItem &item = _items[_refs[i].Item];
    if (!item.IsDir() && !item.IsService())
      sorted.Add(i);
  }
  
  if (sorted.IsEmpty())
    return;
  
  sorted.Sort(CompareItemsPaths_Sort, (void *)this);
  
  AString link;
  
  for (i = 0; i < _refs.Size(); i++)
  {
    CRefItem &ref = _refs[i];
    const CItem &item = _items[ref.Item];
    if (item.IsDir() || item.IsService() || item.PackSize != 0)
      continue;
    CLinkInfo linkInfo;
    if (!item.FindExtra_Link(linkInfo) || linkInfo.Type != NLinkType::kFileCopy)
      continue;
    link.SetFrom_CalcLen((const char *)(item.Extra + linkInfo.NameOffset), linkInfo.NameLen);
    const int linkIndex = FindLink(*this, sorted, link, i);
    if (linkIndex < 0)
      continue;
    if ((unsigned)linkIndex >= i)
      continue; // we don't support forward links that can lead to loops
    const CRefItem &linkRef = _refs[linkIndex];
    const CItem &linkItem = _items[linkRef.Item];
    if (linkItem.Size == item.Size)
    {
      if (linkRef.Link >= 0)
        ref.Link = linkRef.Link;
      else if (!linkItem.NeedUse_as_CopyLink())
        ref.Link = linkIndex;
    }
  }
}



HRESULT CHandler::Open2(IInStream *stream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openCallback)
{
  CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
  // CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  NRar::CVolumeName seqName;
  CTempBuf tempBuf;
  CUnpacker unpacker;
  
  if (openCallback)
  {
    openCallback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);
    openCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&unpacker.getTextPassword);
  }
  // unpacker.getTextPassword = getTextPassword;
  
  CInArchive arch;
  int prevSplitFile = -1;
  int prevMainFile = -1;
  UInt64 totalBytes = 0;
  UInt64 curBytes = 0;
  bool nextVol_is_Required = false;
  
  for (;;)
  {
    CMyComPtr<IInStream> inStream;
    
    if (_arcs.IsEmpty())
      inStream = stream;
    else
    {
      if (!openVolumeCallback)
        break;
      if (_arcs.Size() == 1)
      {
        UString baseName;
        {
          NCOM::CPropVariant prop;
          RINOK(openVolumeCallback->GetProperty(kpidName, &prop))
          if (prop.vt != VT_BSTR)
            break;
          baseName = prop.bstrVal;
        }
        if (!seqName.InitName(baseName))
          break;
      }
      const UString volName = seqName.GetNextName();
      const HRESULT result = openVolumeCallback->GetStream(volName, &inStream);
      if (result != S_OK && result != S_FALSE)
        return result;
      if (!inStream || result != S_OK)
      {
        if (nextVol_is_Required)
          _missingVolName = volName;
        break;
      }
    }
    
    UInt64 endPos = 0;
    RINOK(InStream_GetPos_GetSize(inStream, arch.StreamStartPosition, endPos))
    
    if (openCallback)
    {
      totalBytes += endPos;
      RINOK(openCallback->SetTotal(NULL, &totalBytes))
    }
    
    CInArcInfo arcInfo_Open;
    {
      const HRESULT res = arch.Open(inStream, maxCheckStartPosition, unpacker.getTextPassword, arcInfo_Open);
      if (arch.IsArc && arch.UnexpectedEnd)
        _errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
      if (_arcs.IsEmpty())
        _isArc = arch.IsArc;
      if (res != S_OK)
      {
        if (res != S_FALSE)
          return res;
        if (_arcs.IsEmpty())
          return res;
        break;
      }
    }
    
    CArc &arc = _arcs.AddNew();
    CInArcInfo &arcInfo = arc.Info;
    arcInfo = arcInfo_Open;
    arc.Stream = inStream;
    
    CItem item;
    
    for (;;)
    {
      item.Clear();
      
      arcInfo.EndPos = arch.Position;
      if (arch.Position > endPos)
      {
        _errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
        break;
      }
      RINOK(InStream_SeekSet(inStream, arch.Position))
      
      {
        CInArchive::CHeader h;
        const HRESULT res = arch.ReadBlockHeader(h);
        if (res != S_OK)
        {
          if (res != S_FALSE)
            return res;
          if (arch.UnexpectedEnd)
          {
            _errorFlags |= kpv_ErrorFlags_UnexpectedEnd;
            if (arcInfo.EndPos < arch.Position)
                arcInfo.EndPos = arch.Position;
            if (arcInfo.EndPos < endPos)
                arcInfo.EndPos = endPos;
          }
          else
            _errorFlags |= kpv_ErrorFlags_HeadersError;
          break;
        }
        
        if (h.Type == NHeaderType::kEndOfArc)
        {
          arcInfo.EndPos = arch.Position;
          arcInfo.EndOfArchive_was_Read = true;
          if (!arch.ReadVar(arcInfo.EndFlags))
            _errorFlags |= kpv_ErrorFlags_HeadersError;
          if (!arch.Is_Buf_Finished() || h.ExtraSize || h.DataSize)
            arcInfo.UnsupportedFeature = true;
          if (arcInfo.IsVolume())
          {
            // for multivolume archives RAR can add ZERO bytes at the end for alignment.
            // We must skip these bytes to prevent phySize warning.
            RINOK(InStream_SeekSet(inStream, arcInfo.EndPos))
            bool areThereNonZeros;
            UInt64 numZeros;
            const UInt64 maxSize = 1 << 12;
            RINOK(ReadZeroTail(inStream, areThereNonZeros, numZeros, maxSize))
            if (!areThereNonZeros && numZeros != 0 && numZeros <= maxSize)
              arcInfo.EndPos += numZeros;
          }
          break;
        }
        
        if (h.Type != NHeaderType::kFile &&
            h.Type != NHeaderType::kService)
        {
          _errorFlags |= kpv_ErrorFlags_UnsupportedFeature;
          break;
        }
        
        item.RecordType = (Byte)h.Type;
        if (!arch.ReadFileHeader(h, item))
        {
          _errorFlags |= kpv_ErrorFlags_HeadersError;
          break;
        }
        item.DataPos = arch.Position;
      }
      
      bool isOk_packSize = true;
      {
        arcInfo.EndPos = arch.Position;
        if (arch.Position + item.PackSize < arch.Position)
        {
          isOk_packSize = false;
          _errorFlags |= kpv_ErrorFlags_HeadersError;
          if (arcInfo.EndPos < endPos)
              arcInfo.EndPos = endPos;
        }
        else
        {
          arch.AddToSeekValue(item.PackSize); // Position points to next header;
          arcInfo.EndPos = arch.Position;
        }
      }

      bool needAdd = true;

      if (!_comment_WasUsedInArc
          && _comment.Size() == 0
          && item.Is_CMT())
      {
        _comment_WasUsedInArc = true;
        if (   item.PackSize <= kCommentSize_Max
            && item.PackSize == item.Size
            && item.PackSize != 0
            && item.Get_Method() == 0
            && !item.IsSplit())
        {
          RINOK(unpacker.DecodeToBuf(EXTERNAL_CODECS_VARS item, item.PackSize, inStream, _comment))
          needAdd = false;
          // item.RecordType = (Byte)NHeaderType::kFile; // for debug
        }
      }

      CRefItem ref;
      ref.Item = _items.Size();
      ref.Last = ref.Item;
      ref.Parent = -1;
      ref.Link = -1;
      
      if (needAdd)
      {
        if (item.IsService())
        {
          if (item.Is_STM())
          {
            if (prevMainFile >= 0)
              ref.Parent = prevMainFile;
          }
          else
          {
            needAdd = false;
            if (item.Is_ACL())
            {
              _acl_Used = true;
              if (item.IsEncrypted() && !arch.m_CryptoMode)
                _error_in_ACL = true;
              else if (item.IsSolid()
                  || prevMainFile < 0
                  || item.Size >= (1 << 24)
                  || item.Size == 0)
                _error_in_ACL = true;
              if (prevMainFile >= 0 && item.Size < (1 << 24) && item.Size != 0)
              {
                CItem &mainItem = _items[_refs[prevMainFile].Item];
                
                if (mainItem.ACL < 0)
                {
                  CByteBuffer acl;
                  const HRESULT res = tempBuf.Decode(EXTERNAL_CODECS_VARS item, inStream, unpacker, acl);
                  if (!item.IsSplitAfter())
                    tempBuf.Clear();
                  if (res != S_OK)
                  {
                    tempBuf.Clear();
                    if (res != S_FALSE && res != E_NOTIMPL)
                      return res;
                    _error_in_ACL = true;
                  }
                  else if (acl.Size() != 0)
                  {
                    if (_acls.IsEmpty() || acl != _acls.Back())
                      _acls.Add(acl);
                    mainItem.ACL = (int)_acls.Size() - 1;
                  }
                }
              }
            }
          }
        } // item.IsService()
        
        if (needAdd)
        {
          if (item.IsSplitBefore())
          {
            if (prevSplitFile >= 0)
            {
              CRefItem &ref2 = _refs[prevSplitFile];
              CItem &prevItem = _items[ref2.Last];
              if (item.IsNextForItem(prevItem))
              {
                ref2.Last = _items.Size();
                prevItem.NextItem = (int)ref2.Last;
                needAdd = false;
              }
            }
            else
              _split_Error = true;
          }
        }
        
        if (needAdd)
        {
          if (item.IsSplitAfter())
            prevSplitFile = (int)_refs.Size();
          if (!item.IsService())
            prevMainFile = (int)_refs.Size();
        }
      }
      
      {
        UInt64 version;
        if (item.FindExtra_Version(version))
        {
          item.Version_Defined = true;
          item.Version = version;
        }
      }
      
      item.VolIndex = _arcs.Size() - 1;
      _items.Add(item);
      if (needAdd)
        _refs.Add(ref);
      
      if (openCallback && (_items.Size() & 0xFF) == 0)
      {
        const UInt64 numFiles = _refs.Size(); // _items.Size()
        const UInt64 numBytes = curBytes + item.DataPos;
        RINOK(openCallback->SetCompleted(&numFiles, &numBytes))
      }

      if (!isOk_packSize)
        break;
    }
      
    curBytes += endPos;

    nextVol_is_Required = false;

    if (!arcInfo.IsVolume())
      break;

    if (arcInfo.EndOfArchive_was_Read)
    {
      if (!arcInfo.AreMoreVolumes())
        break;
      nextVol_is_Required = true;
    }
  }

  FillLinks();
  return S_OK;
}



Z7_COM7F_IMF(CHandler::Open(IInStream *stream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openCallback))
{
  COM_TRY_BEGIN
  Close();
  return Open2(stream, maxCheckStartPosition, openCallback);
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  COM_TRY_BEGIN
  _missingVolName.Empty();
  _errorFlags = 0;
  // _warningFlags = 0;
  _isArc = false;
  _comment_WasUsedInArc = false;
  _acl_Used = false;
  _error_in_ACL = false;
  _split_Error = false;
  _numBlocks = 0;
  _rar5comapt_mask = 0;
  _algo_Mask = 0; // (UInt64)0u - 1;
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(_methodMasks); i++)
  {
    _methodMasks[i] = 0;
    _dictMaxSizes[i] = 0;
  }

  _refs.Clear();
  _items.Clear();
  _arcs.Clear();
  _acls.Clear();
  _comment.Free();
  return S_OK;
  COM_TRY_END
}


Z7_CLASS_IMP_NOQIB_1(
  CVolsInStream
  , ISequentialInStream
)
  UInt64 _rem;
  ISequentialInStream *_stream;
  const CObjectVector<CArc> *_arcs;
  const CObjectVector<CItem> *_items;
  int _itemIndex;
public:
  bool CrcIsOK;
private:
  CHash _hash;
public:
  void Init(const CObjectVector<CArc> *arcs,
      const CObjectVector<CItem> *items,
      unsigned itemIndex)
  {
    _arcs = arcs;
    _items = items;
    _itemIndex = (int)itemIndex;
    _stream = NULL;
    CrcIsOK = true;
  }
};

Z7_COM7F_IMF(CVolsInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  UInt32 realProcessedSize = 0;

  while (size != 0)
  {
    if (!_stream)
    {
      if (_itemIndex < 0)
        break;
      const CItem &item = (*_items)[_itemIndex];
      IInStream *s = (*_arcs)[item.VolIndex].Stream;
      RINOK(InStream_SeekSet(s, item.GetDataPosition()))
      _stream = s;
      if (CrcIsOK && item.IsSplitAfter())
        _hash.Init(item);
      else
        _hash.Init_NoCalc();
      _rem = item.PackSize;
    }
    {
      UInt32 cur = size;
      if (cur > _rem)
        cur = (UInt32)_rem;
      const UInt32 num = cur;
      HRESULT res = _stream->Read(data, cur, &cur);
      _hash.Update(data, cur);
      realProcessedSize += cur;
      if (processedSize)
        *processedSize = realProcessedSize;
      data = (Byte *)data + cur;
      size -= cur;
      _rem -= cur;
      if (_rem == 0)
      {
        const CItem &item = (*_items)[_itemIndex];
        _itemIndex = item.NextItem;
        if (!_hash.Check(item, NULL)) // RAR doesn't use MAC here
          CrcIsOK = false;
        _stream = NULL;
      }
      if (res != S_OK)
        return res;
      if (realProcessedSize != 0)
        return S_OK;
      if (cur == 0 && num != 0)
        return S_OK;
    }
  }
  
  return S_OK;
}


static int FindLinkBuf(CObjectVector<CLinkFile> &linkFiles, unsigned index)
{
  unsigned left = 0, right = linkFiles.Size();
  for (;;)
  {
    if (left == right)
      return -1;
    const unsigned mid = (left + right) / 2;
    const unsigned linkIndex = linkFiles[mid].Index;
    if (index == linkIndex)
      return (int)mid;
    if (index < linkIndex)
      right = mid;
    else
      left = mid + 1;
  }
}


static inline int DecoderRes_to_OpRes(HRESULT res, bool crcOK)
{
  if (res == E_NOTIMPL)
    return NExtract::NOperationResult::kUnsupportedMethod;
  // if (res == S_FALSE)
  if (res != S_OK)
    return NExtract::NOperationResult::kDataError;
  return crcOK ?
    NExtract::NOperationResult::kOK :
    NExtract::NOperationResult::kCRCError;
}


static HRESULT CopyData_with_Progress(const Byte *data, size_t size,
    ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  UInt64 pos64 = 0;
  while (size)
  {
    const UInt32 kStepSize = (UInt32)1 << 24;
    UInt32 cur = kStepSize;
    if (cur > size)
      cur = (UInt32)size;
    RINOK(outStream->Write(data, cur, &cur))
    if (cur == 0)
      return E_FAIL;
    size -= cur;
    data += cur;
    pos64 += cur;
    if (progress)
    {
      RINOK(progress->SetRatioInfo(&pos64, &pos64))
    }
  }
  return S_OK;
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = (UInt32)_refs.Size();
  if (numItems == 0)
    return S_OK;
  
  CByteArr extractStatuses(_refs.Size());
  memset(extractStatuses, 0, _refs.Size());

  // we don't want to use temp buffer for big link files.
  const size_t k_CopyLinkFile_MaxSize = (size_t)1 << (28 + sizeof(size_t) / 2);

  const Byte kStatus_Extract = 1 << 0;
  const Byte kStatus_Skip = 1 << 1;
  const Byte kStatus_Link = 1 << 2;

  /*
    In original RAR:
    1) service streams are not allowed to be solid,
        and solid flag must be ignored for service streams.
    2) If RAR creates new solid block and first file in solid block is Link file,
         then it can clear solid flag for Link file and
         clear solid flag for first non-Link file after Link file.
  */

  CObjectVector<CLinkFile> linkFiles;

  {
    UInt64 total = 0;
    bool isThereUndefinedSize = false;
    bool thereAreLinks = false;
    {
      unsigned solidLimit = 0;
      for (UInt32 t = 0; t < numItems; t++)
      {
        const unsigned index = (unsigned)(allFilesMode ? t : indices[t]);
        const CRefItem &ref = _refs[index];
        const CItem &item = _items[ref.Item];
        const CItem &lastItem = _items[ref.Last];
        
        extractStatuses[index] |= kStatus_Extract;

        if (!lastItem.Is_UnknownSize())
          total += lastItem.Size;
        else
          isThereUndefinedSize = true;
        
        if (ref.Link >= 0)
        {
          // 18.06 fixed: we use links for Test mode too
          // if (!testMode)
          {
            if ((unsigned)ref.Link < index)
            {
              const CRefItem &linkRef = _refs[(unsigned)ref.Link];
              const CItem &linkItem = _items[linkRef.Item];
              if (linkItem.IsSolid())
              if (testMode || linkItem.Size <= k_CopyLinkFile_MaxSize)
              {
                if (extractStatuses[(unsigned)ref.Link] == 0)
                {
                  const CItem &lastLinkItem = _items[linkRef.Last];
                  if (!lastLinkItem.Is_UnknownSize())
                    total += lastLinkItem.Size;
                  else
                    isThereUndefinedSize = true;
                }
                extractStatuses[(unsigned)ref.Link] |= kStatus_Link;
                thereAreLinks = true;
              }
            }
          }
          continue;
        }
        
        if (item.IsService())
          continue;
        
        if (item.IsSolid())
        {
          unsigned j = index;
          
          while (j > solidLimit)
          {
            j--;
            const CRefItem &ref2 = _refs[j];
            const CItem &item2 = _items[ref2.Item];
            if (!item2.IsService())
            {
              if (extractStatuses[j] == 0)
              {
                const CItem &lastItem2 = _items[ref2.Last];
                if (!lastItem2.Is_UnknownSize())
                  total += lastItem2.Size;
                else
                  isThereUndefinedSize = true;
              }
              extractStatuses[j] |= kStatus_Skip;
              if (!item2.IsSolid())
                break;
            }
          }
        }
        
        solidLimit = index + 1;
      }
    }

    if (thereAreLinks)
    {
      unsigned solidLimit = 0;

      FOR_VECTOR (i, _refs)
      {
        if ((extractStatuses[i] & kStatus_Link) == 0)
          continue;

        // We use CLinkFile for testMode too.
        // So we can show errors for copy files.
        // if (!testMode)
        {
          CLinkFile &linkFile = linkFiles.AddNew();
          linkFile.Index = i;
        }

        const CItem &item = _items[_refs[i].Item];
        /*
        if (item.IsService())
          continue;
        */
        
        if (item.IsSolid())
        {
          unsigned j = i;
          
          while (j > solidLimit)
          {
            j--;
            const CRefItem &ref2 = _refs[j];
            const CItem &item2 = _items[ref2.Item];
            if (!item2.IsService())
            {
              if (extractStatuses[j] != 0)
                break;
              extractStatuses[j] = kStatus_Skip;
              {
                const CItem &lastItem2 = _items[ref2.Last];
                if (!lastItem2.Is_UnknownSize())
                  total += lastItem2.Size;
                else
                  isThereUndefinedSize = true;
              }
              if (!item2.IsSolid())
                break;
            }
          }
        }
        
        solidLimit = i + 1;
      }

      if (!testMode)
      for (UInt32 t = 0; t < numItems; t++)
      {
        const unsigned index = (unsigned)(allFilesMode ? t : indices[t]);
        const CRefItem &ref = _refs[index];
       
        const int linkIndex = ref.Link;
        if (linkIndex < 0 || (unsigned)linkIndex >= index)
          continue;
        const CItem &linkItem = _items[_refs[(unsigned)linkIndex].Item];
        if (!linkItem.IsSolid() || linkItem.Size > k_CopyLinkFile_MaxSize)
          continue;
        const int bufIndex = FindLinkBuf(linkFiles, (unsigned)linkIndex);
        if (bufIndex < 0)
          return E_FAIL;
        linkFiles[bufIndex].NumLinks++;
      }
    }
    
    if (total != 0 || !isThereUndefinedSize)
    {
      RINOK(extractCallback->SetTotal(total))
    }
  }


  
  // ---------- MEMORY REQUEST ----------
  {
    UInt64 dictMaxSize = 0;
    for (UInt32 i = 0; i < _refs.Size(); i++)
    {
      if (extractStatuses[i] == 0)
        continue;
      const CRefItem &ref = _refs[i];
      const CItem &item = _items[ref.Item];
/*
      if (!item.IsDir() && !item.IsService() && item.NeedUse_as_CopyLink())
      {
      }
*/
      const unsigned algo = item.Get_AlgoVersion_RawBits();
      if (!item.IsDir() && algo < Z7_ARRAY_SIZE(_methodMasks))
      {
        const UInt64 d = item.Get_DictSize64();
        if (dictMaxSize < d)
            dictMaxSize = d;
      }
    }
    // we use callback, if dict exceeds (1 GB), because
    // client code can set low limit (1 GB) for allowed memory usage.
    const UInt64 k_MemLimit_for_Callback = (UInt64)1 << 30;
    if (dictMaxSize > (_memUsage_WasSet ?
        _memUsage_Decompress : k_MemLimit_for_Callback))
    {
      {
        CMyComPtr<IArchiveRequestMemoryUseCallback> requestMem;
        extractCallback->QueryInterface(IID_IArchiveRequestMemoryUseCallback, (void **)&requestMem);
        if (!requestMem)
        {
          if (_memUsage_WasSet)
            return E_OUTOFMEMORY;
        }
        else
        {
          UInt64 allowedSize = _memUsage_WasSet ?
              _memUsage_Decompress :
              (UInt64)1 << 32; // 4 GB is default allowed limit for RAR7
          
          const UInt32 flags = (_memUsage_WasSet ?
                NRequestMemoryUseFlags::k_AllowedSize_WasForced |
                NRequestMemoryUseFlags::k_MLimit_Exceeded :
            (dictMaxSize > allowedSize) ?
                NRequestMemoryUseFlags::k_DefaultLimit_Exceeded:
                0)
             |  NRequestMemoryUseFlags::k_SkipArc_IsExpected
             // |  NRequestMemoryUseFlags::k_NoErrorMessage // for debug
             ;

          // we set "Allow" for default case, if requestMem doesn't process anything.
          UInt32 answerFlags =
              (_memUsage_WasSet && dictMaxSize > allowedSize) ?
                NRequestMemoryAnswerFlags::k_Limit_Exceeded
              | NRequestMemoryAnswerFlags::k_SkipArc
              : NRequestMemoryAnswerFlags::k_Allow;

          RINOK(requestMem->RequestMemoryUse(
              flags,
              NEventIndexType::kNoIndex,
              // NEventIndexType::kInArcIndex, // for debug
              0,    // index
              NULL, // path
              dictMaxSize, &allowedSize, &answerFlags))
          if ( (answerFlags & NRequestMemoryAnswerFlags::k_Allow) == 0
            || (answerFlags & NRequestMemoryAnswerFlags::k_Stop)
            || (answerFlags & NRequestMemoryAnswerFlags::k_SkipArc)
            )
          {
            return E_OUTOFMEMORY;
          }
/*
          if ((answerFlags & NRequestMemoryAnswerFlags::k_AskForBigFile) == 0 &&
              (answerFlags & NRequestMemoryAnswerFlags::k_ReportForBigFile) == 0)
          {
            // requestMem.Release();
          }
*/
        }
      }
    }
  }



  // ---------- UNPACK ----------

  UInt64 totalUnpacked = 0;
  UInt64 totalPacked = 0;
  UInt64 curUnpackSize;
  UInt64 curPackSize;

  CUnpacker unpacker;
  unpacker.NeedCrc = _needChecksumCheck;
  CMyComPtr2_Create<ISequentialInStream, CVolsInStream> volsInStream;
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);

/*
  bool prevSolidWasSkipped = false;
  UInt64 solidDictSize_Skip = 0;
*/

  for (unsigned i = 0;; i++,
      totalUnpacked += curUnpackSize,
      totalPacked += curPackSize)
  {
    lps->InSize = totalPacked;
    lps->OutSize = totalUnpacked;
    RINOK(lps->SetCur())
    {
      const unsigned num = _refs.Size();
      if (i >= num)
        break;
      for (;;)
      {
        if (extractStatuses[i] != 0)
          break;
        i++;
        if (i >= num)
          break;
      }
      if (i >= num)
        break;
    }
    curUnpackSize = 0;
    curPackSize = 0;
    
    // isExtract means that we don't skip that item. So we need read data.
    const bool isExtract = ((extractStatuses[i] & kStatus_Extract) != 0);
    Int32 askMode =
        isExtract ? (testMode ?
          NExtract::NAskMode::kTest :
          NExtract::NAskMode::kExtract) :
          NExtract::NAskMode::kSkip;

    unpacker.linkFile = NULL;

    // if (!testMode)
    if ((extractStatuses[i] & kStatus_Link) != 0)
    {
      const int bufIndex = FindLinkBuf(linkFiles, i);
      if (bufIndex < 0)
        return E_FAIL;
      unpacker.linkFile = &linkFiles[bufIndex];
    }

    const unsigned index = i;
    const CRefItem *ref = &_refs[index];
    const CItem *item = &_items[ref->Item];
    const CItem &lastItem = _items[ref->Last];

    curUnpackSize = 0;
    if (!lastItem.Is_UnknownSize())
      curUnpackSize = lastItem.Size;

    curPackSize = GetPackSize(index);

    bool isSolid = false;
    if (!item->IsService())
    {
      if (item->IsSolid())
        isSolid = unpacker.SolidAllowed;
      unpacker.SolidAllowed = isSolid;
    }


    // ----- request mem -----
/*
    // link files are complicated cases. (ref->Link >= 0)
    // link file can refer to non-solid file that can have big dictionary
    // link file can refer to solid files that requres buffer
    if (!item->IsDir() && requestMem && ref->Link < 0)
    {
      bool needSkip = false;
      if (isSolid)
        needSkip = prevSolidWasSkipped;
      else
      {
        // isSolid == false
        const unsigned algo = item->Get_AlgoVersion_RawBits();
        // const unsigned m = item.Get_Method();
        if (algo < Z7_ARRAY_SIZE(_methodMasks))
        {
          solidDictSize_Skip = item->Get_DictSize64();
          if (solidDictSize_Skip > allowedSize)
            needSkip = true;
        }
      }
      if (needSkip)
      {
        UInt32 answerFlags = 0;
        UInt64 allowedSize_File = allowedSize;
        RINOK(requestMem->RequestMemoryUse(
                  NRequestMemoryUseFlags::k_Limit_Exceeded |
                  NRequestMemoryUseFlags::k_IsReport,
              NEventIndexType::kInArcIndex,
              index,
              NULL, // path
              solidDictSize_Skip, &allowedSize_File, &answerFlags))
        if (!item->IsService())
          prevSolidWasSkipped = true;
        continue;
      }
    }
    if (!item->IsService() && item->IsDir())
      prevSolidWasSkipped = false;
*/
    
    CMyComPtr<ISequentialOutStream> realOutStream;
    RINOK(extractCallback->GetStream((UInt32)index, &realOutStream, askMode))

    if (item->IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode))
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      continue;
    }

    const int index2 = ref->Link;

    int bufIndex = -1;

    if (index2 >= 0)
    {
      const CRefItem &ref2 = _refs[index2];
      const CItem &item2 = _items[ref2.Item];
      const CItem &lastItem2 = _items[ref2.Last];
      if (!item2.IsSolid())
      {
        item = &item2;
        ref = &ref2;
        if (!lastItem2.Is_UnknownSize())
          curUnpackSize = lastItem2.Size;
        else
          curUnpackSize = 0;
        curPackSize = GetPackSize((unsigned)index2);
      }
      else
      {
        if ((unsigned)index2 < index)
          bufIndex = FindLinkBuf(linkFiles, (unsigned)index2);
      }
    }

    bool needCallback = true;

    if (!realOutStream)
    {
      if (testMode)
      {
        if (item->NeedUse_as_CopyLink_or_HardLink())
        {
          Int32 opRes = NExtract::NOperationResult::kOK;
          if (bufIndex >= 0)
          {
            const CLinkFile &linkFile = linkFiles[bufIndex];
            opRes = DecoderRes_to_OpRes(linkFile.Res, linkFile.crcOK);
          }

          RINOK(extractCallback->PrepareOperation(askMode))
          RINOK(extractCallback->SetOperationResult(opRes))
          continue;
        }
      }
      else
      {
        if (item->IsService())
          continue;

        needCallback = false;

        if (!item->NeedUse_as_HardLink())
        if (index2 < 0)

        for (unsigned n = i + 1; n < _refs.Size(); n++)
        {
          const CItem &nextItem = _items[_refs[n].Item];
          if (nextItem.IsService())
            continue;
          if (!nextItem.IsSolid())
            break;
          if (extractStatuses[i] != 0)
          {
            needCallback = true;
            break;
          }
        }
        
        askMode = NExtract::NAskMode::kSkip;
      }
    }

    if (needCallback)
    {
      RINOK(extractCallback->PrepareOperation(askMode))
    }

    if (bufIndex >= 0)
    {
      CLinkFile &linkFile = linkFiles[bufIndex];
     
      if (isExtract)
      {
        if (linkFile.NumLinks == 0)
          return E_FAIL;
       
        if (needCallback)
        if (realOutStream)
        {
          RINOK(CopyData_with_Progress(linkFile.Data, linkFile.Data.Size(), realOutStream, lps))
        }
      
        if (--linkFile.NumLinks == 0)
          linkFile.Data.Free();
      }
      
      if (needCallback)
      {
        RINOK(extractCallback->SetOperationResult(DecoderRes_to_OpRes(linkFile.Res, linkFile.crcOK)))
      }
      continue;
    }

    if (!needCallback)
      continue;
    
    if (item->NeedUse_as_CopyLink())
    {
      const int opRes = realOutStream ?
          NExtract::NOperationResult::kUnsupportedMethod:
          NExtract::NOperationResult::kOK;
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(opRes))
      continue;
    }

    volsInStream->Init(&_arcs, &_items, ref->Item);

    const UInt64 packSize = curPackSize;

    if (item->IsEncrypted())
      if (!unpacker.getTextPassword)
        extractCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&unpacker.getTextPassword);

    bool wrongPassword;
    HRESULT result = unpacker.Create(EXTERNAL_CODECS_VARS *item, isSolid, wrongPassword);

    if (wrongPassword)
    {
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kWrongPassword))
      continue;
    }
        
    bool crcOK = true;
    if (result == S_OK)
      result = unpacker.Code(*item, _items[ref->Last], packSize, volsInStream, realOutStream, lps, crcOK);
    realOutStream.Release();
    if (!volsInStream->CrcIsOK)
      crcOK = false;

    int opRes = crcOK ?
        NExtract::NOperationResult::kOK:
        NExtract::NOperationResult::kCRCError;

    if (result != S_OK)
    {
      if (result == S_FALSE)
        opRes = NExtract::NOperationResult::kDataError;
      else if (result == E_NOTIMPL)
        opRes = NExtract::NOperationResult::kUnsupportedMethod;
      else
        return result;
    }

    RINOK(extractCallback->SetOperationResult(opRes))
  }

  {
    FOR_VECTOR (k, linkFiles)
      if (linkFiles[k].NumLinks != 0)
        return E_FAIL;
  }

  return S_OK;
  COM_TRY_END
}


CHandler::CHandler()
{
  InitDefaults();
}

void CHandler::InitDefaults()
{
  _needChecksumCheck = true;
  _memUsage_WasSet = false;
  _memUsage_Decompress = (UInt64)1 << 32;
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

    if (name.IsPrefixedBy_Ascii_NoCase("mt"))
    {
    }
    else if (name.IsPrefixedBy_Ascii_NoCase("memx"))
    {
      size_t memAvail;
      if (!NWindows::NSystem::GetRamSize(memAvail))
        memAvail = (size_t)sizeof(size_t) << 28;
      UInt64 v;
      if (!ParseSizeString(name.Ptr(4), prop, memAvail, v))
        return E_INVALIDARG;
      _memUsage_Decompress = v;
      _memUsage_WasSet = true;
    }
    else if (name.IsPrefixedBy_Ascii_NoCase("crc"))
    {
      name.Delete(0, 3);
      UInt32 crcSize = 1;
      RINOK(ParsePropToUInt32(name, prop, crcSize))
      _needChecksumCheck = (crcSize != 0);
    }
    else
    {
      return E_INVALIDARG;
    }
  }
  return S_OK;
}


IMPL_ISetCompressCodecsInfo

REGISTER_ARC_I(
  "Rar5", "rar r00", NULL, 0xCC,
  kMarker,
  0,
  NArcInfoFlags::kFindSignature,
  NULL)

}}


Z7_CLASS_IMP_COM_2(
  CBlake2spHasher
  , IHasher
  , ICompressSetCoderProperties
)
  CAlignedBuffer1 _buf;
  // CBlake2sp _blake;
  #define Z7_BLACK2S_ALIGN_OBJECT_OFFSET 0
  CBlake2sp *Obj() { return (CBlake2sp *)(void *)((Byte *)_buf + Z7_BLACK2S_ALIGN_OBJECT_OFFSET); }
public:
  Byte _mtDummy[1 << 7];  // it's public to eliminate clang warning: unused private field
  CBlake2spHasher():
    _buf(sizeof(CBlake2sp) + Z7_BLACK2S_ALIGN_OBJECT_OFFSET)
  {
    Blake2sp_SetFunction(Obj(), 0);
    Blake2sp_InitState(Obj());
  }
};

Z7_COM7F_IMF2(void, CBlake2spHasher::Init())
{
  Blake2sp_InitState(Obj());
}

Z7_COM7F_IMF2(void, CBlake2spHasher::Update(const void *data, UInt32 size))
{
#if 1
  Blake2sp_Update(Obj(), (const Byte *)data, (size_t)size);
#else
  // for debug:
  for (;;)
  {
    if (size == 0)
      return;
    UInt32 size2 = (size * 0x85EBCA87) % size / 800;
    // UInt32 size2 = size / 2;
    if (size2 == 0)
      size2 = 1;
    Blake2sp_Update(Obj(), (const Byte *)data, size2);
    data = (const void *)((const Byte *)data + size2);
    size -= size2;
  }
#endif
}

Z7_COM7F_IMF2(void, CBlake2spHasher::Final(Byte *digest))
{
  Blake2sp_Final(Obj(), digest);
}

Z7_COM7F_IMF(CBlake2spHasher::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
{
  unsigned algo = 0;
  for (UInt32 i = 0; i < numProps; i++)
  {
    if (propIDs[i] == NCoderPropID::kDefaultProp)
    {
      const PROPVARIANT &prop = coderProps[i];
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      /*
      if (prop.ulVal > Z7_BLAKE2S_ALGO_MAX)
        return E_NOTIMPL;
      */
      algo = (unsigned)prop.ulVal;
    }
  }
  if (!Blake2sp_SetFunction(Obj(), algo))
    return E_NOTIMPL;
  return S_OK;
}

REGISTER_HASHER(CBlake2spHasher, 0x202, "BLAKE2sp", Z7_BLAKE2S_DIGEST_SIZE)

static struct CBlake2sp_Prepare { CBlake2sp_Prepare() { z7_Black2sp_Prepare(); } } g_Blake2sp_Prepare;

/* ================ unit: CPP/7zip/Archive/Zip/ZipAddCommon.cpp ================ */
// ZipAddCommon.cpp

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

namespace NArchive {
namespace NZip {

using namespace NFileHeader;


static const unsigned kLzmaPropsSize = 5;
static const unsigned kLzmaHeaderSize = 4 + kLzmaPropsSize;

Z7_CLASS_IMP_NOQIB_3(
  CLzmaEncoder
  , ICompressCoder
  , ICompressSetCoderProperties
  , ICompressSetCoderPropertiesOpt
)
public:
  CMyComPtr2<ICompressCoder, NCompress::NLzma::CEncoder> Encoder;
  Byte Header[kLzmaHeaderSize];
};

Z7_COM7F_IMF(CLzmaEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
{
  Encoder.Create_if_Empty();
  CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> outStream;
  outStream->Init(Header + 4, kLzmaPropsSize);
  RINOK(Encoder->SetCoderProperties(propIDs, props, numProps))
  RINOK(Encoder->WriteCoderProperties(outStream))
  if (outStream->GetPos() != kLzmaPropsSize)
    return E_FAIL;
  Header[0] = MY_VER_MAJOR;
  Header[1] = MY_VER_MINOR;
  Header[2] = kLzmaPropsSize;
  Header[3] = 0;
  return S_OK;
}

Z7_COM7F_IMF(CLzmaEncoder::SetCoderPropertiesOpt(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
{
  return Encoder->SetCoderPropertiesOpt(propIDs, props, numProps);
}

Z7_COM7F_IMF(CLzmaEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  RINOK(WriteStream(outStream, Header, kLzmaHeaderSize))
  return Encoder.Interface()->Code(inStream, outStream, inSize, outSize, progress);
}


CAddCommon::CAddCommon():
    _isLzmaEos(false),
    _buf(NULL)
    {}

void CAddCommon::SetOptions(const CCompressionMethodMode &options)
{
  _options = options;
}

CAddCommon::~CAddCommon()
{
  MidFree(_buf);
}

static const UInt32 kBufSize = ((UInt32)1 << 16);

HRESULT CAddCommon::CalcStreamCRC(ISequentialInStream *inStream, UInt32 &resultCRC)
{
  if (!_buf)
  {
    _buf = (Byte *)MidAlloc(kBufSize);
    if (!_buf)
      return E_OUTOFMEMORY;
  }

  UInt32 crc = CRC_INIT_VAL;
  for (;;)
  {
    UInt32 processed;
    RINOK(inStream->Read(_buf, kBufSize, &processed))
    if (processed == 0)
    {
      resultCRC = CRC_GET_DIGEST(crc);
      return S_OK;
    }
    crc = CrcUpdate(crc, _buf, (size_t)processed);
  }
}


HRESULT CAddCommon::Set_Pre_CompressionResult(bool inSeqMode, bool outSeqMode, UInt64 unpackSize,
    CCompressingResult &opRes) const
{
  // We use Zip64, if unPackSize size is larger than 0xF8000000 to support
  // cases when compressed size can be about 3% larger than uncompressed size

  const UInt32 kUnpackZip64Limit = 0xF8000000;
  
  opRes.UnpackSize = unpackSize;
  opRes.PackSize = (UInt64)1 << 60; // we use big value to force Zip64 mode.

  if (unpackSize < kUnpackZip64Limit)
    opRes.PackSize = (UInt32)0xFFFFFFFF - 1; // it will not use Zip64 for that size

  if (opRes.PackSize < unpackSize)
    opRes.PackSize = unpackSize;

  const Byte method = _options.MethodSequence[0];

  if (method == NCompressionMethod::kStore && !_options.Password_Defined)
    opRes.PackSize = unpackSize;

  opRes.CRC = 0;

  opRes.LzmaEos = false;

  opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;
  opRes.DescriptorMode = outSeqMode;
  
  if (_options.Password_Defined)
  {
    opRes.ExtractVersion = NCompressionMethod::kExtractVersion_ZipCrypto;
    if (_options.IsAesMode)
      opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Aes;
    else
    {
      if (inSeqMode)
        opRes.DescriptorMode = true;
    }
  }
  
  opRes.Method = method;
  Byte ver = 0;
  
  switch (method)
  {
    case NCompressionMethod::kStore: break;
    case NCompressionMethod::kDeflate: ver = NCompressionMethod::kExtractVersion_Deflate; break;
    case NCompressionMethod::kDeflate64: ver = NCompressionMethod::kExtractVersion_Deflate64; break;
    case NCompressionMethod::kXz   : ver = NCompressionMethod::kExtractVersion_Xz; break;
    case NCompressionMethod::kPPMd : ver = NCompressionMethod::kExtractVersion_PPMd; break;
    case NCompressionMethod::kBZip2: ver = NCompressionMethod::kExtractVersion_BZip2; break;
    case NCompressionMethod::kLZMA :
    {
      ver = NCompressionMethod::kExtractVersion_LZMA;
      const COneMethodInfo *oneMethodMain = &_options._methods[0];
      opRes.LzmaEos = oneMethodMain->Get_Lzma_Eos();
      break;
    }
    default: break;
  }
  if (opRes.ExtractVersion < ver)
      opRes.ExtractVersion = ver;

  return S_OK;
}


HRESULT CAddCommon::Compress(
    DECL_EXTERNAL_CODECS_LOC_VARS
    ISequentialInStream *inStream, IOutStream *outStream,
    bool inSeqMode, bool outSeqMode,
    UInt32 fileTime,
    UInt64 expectedDataSize, bool expectedDataSize_IsConfirmed,
    ICompressProgressInfo *progress, CCompressingResult &opRes)
{
  // opRes.LzmaEos = false;

  if (!inStream)
  {
    // We can create empty stream here. But it was already implemented in caller code in 9.33+
    return E_INVALIDARG;
  }

  CMyComPtr2_Create<ISequentialInStream, CSequentialInStreamWithCRC> inCrcStream;
  
  CMyComPtr<IInStream> inStream2;
  if (!inSeqMode)
  {
    inStream->QueryInterface(IID_IInStream, (void **)&inStream2);
    if (!inStream2)
    {
      // inSeqMode = true;
      // inSeqMode must be correct before
      return E_FAIL;
    }
  }

  inCrcStream->SetStream(inStream);
  inCrcStream->SetFullSize(expectedDataSize_IsConfirmed ? expectedDataSize : (UInt64)(Int64)-1);
  // inCrcStream->Init();

  unsigned numTestMethods = _options.MethodSequence.Size();
  // numTestMethods != 0

  bool descriptorMode = outSeqMode;
  
  // ZipCrypto without descriptor requires additional reading pass for
  // inStream to calculate CRC for password check field.
  // The descriptor allows to use ZipCrypto check field without CRC (InfoZip's modification).

  if (!outSeqMode)
    if (inSeqMode && _options.Password_Defined && !_options.IsAesMode)
      descriptorMode = true;
  opRes.DescriptorMode = descriptorMode;

  if (numTestMethods > 1)
    if (inSeqMode || outSeqMode || !inStream2)
      numTestMethods = 1;

  UInt32 crc = 0;
  bool crc_IsCalculated = false;
  
  CFilterCoder::C_OutStream_Releaser outStreamReleaser;
  // opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;
  
  for (unsigned i = 0; i < numTestMethods; i++)
  {
    inCrcStream->Init();

    if (i != 0)
    {
      // if (inStream2)
      {
        RINOK(InStream_SeekToBegin(inStream2))
      }
      RINOK(outStream->Seek(0, STREAM_SEEK_SET, NULL))
      RINOK(outStream->SetSize(0))
    }

    opRes.LzmaEos = false;
    opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Default;

    const Byte method = _options.MethodSequence[i];
    if (method == NCompressionMethod::kStore && descriptorMode)
    {
      // we still can create descriptor_mode archives with "Store" method, but they are not good for 100%
      return E_NOTIMPL;
    }
    
    bool needCode = true;
    
    if (_options.Password_Defined)
    {
      opRes.ExtractVersion = NCompressionMethod::kExtractVersion_ZipCrypto;

      if (!_cryptoStream.IsDefined())
        _cryptoStream.SetFromCls(new CFilterCoder(true));
      
      if (_options.IsAesMode)
      {
        opRes.ExtractVersion = NCompressionMethod::kExtractVersion_Aes;
        if (!_cryptoStream->Filter)
        {
          _cryptoStream->Filter = _filterAesSpec = new NCrypto::NWzAes::CEncoder;
          _filterAesSpec->SetKeyMode(_options.AesKeyMode);
          RINOK(_filterAesSpec->CryptoSetPassword((const Byte *)(const char *)_options.Password, _options.Password.Len()))
        }
        RINOK(_filterAesSpec->WriteHeader(outStream))
      }
      else
      {
        if (!_cryptoStream->Filter)
        {
          _cryptoStream->Filter = _filterSpec = new NCrypto::NZip::CEncoder;
          _filterSpec->CryptoSetPassword((const Byte *)(const char *)_options.Password, _options.Password.Len());
        }
        
        UInt32 check;
        
        if (descriptorMode)
        {
          // it's Info-ZIP modification for stream_mode descriptor_mode (bit 3 of the general purpose bit flag is set)
          check = (fileTime & 0xFFFF);
        }
        else
        {
          if (!crc_IsCalculated)
          {
            RINOK(CalcStreamCRC(inStream, crc))
            crc_IsCalculated = true;
            RINOK(InStream_SeekToBegin(inStream2))
            inCrcStream->Init();
          }
          check = (crc >> 16);
        }
        
        RINOK(_filterSpec->WriteHeader_Check16(outStream, (UInt16)check))
      }
      
      if (method == NCompressionMethod::kStore)
      {
        needCode = false;
        RINOK(_cryptoStream->Code(inCrcStream, outStream, NULL, NULL, progress))
      }
      else
      {
        RINOK(_cryptoStream->SetOutStream(outStream))
        RINOK(_cryptoStream->InitEncoder())
        outStreamReleaser.FilterCoder = _cryptoStream.ClsPtr();
      }
    }

    if (needCode)
    {
      switch (method)
      {
      case NCompressionMethod::kStore:
      {
        _copyCoder.Create_if_Empty();
        CMyComPtr<ISequentialOutStream> outStreamNew;
        if (_options.Password_Defined)
          outStreamNew = _cryptoStream;
        else
          outStreamNew = outStream;
        RINOK(_copyCoder.Interface()->Code(inCrcStream, outStreamNew, NULL, NULL, progress))
        break;
      }
      
      default:
      {
        if (!_compressEncoder)
        {
          CLzmaEncoder *_lzmaEncoder = NULL;
          if (method == NCompressionMethod::kLZMA)
          {
            _compressExtractVersion = NCompressionMethod::kExtractVersion_LZMA;
            _lzmaEncoder = new CLzmaEncoder();
            _compressEncoder = _lzmaEncoder;
          }
          else if (method == NCompressionMethod::kXz)
          {
            _compressExtractVersion = NCompressionMethod::kExtractVersion_Xz;
            NCompress::NXz::CEncoder *encoder = new NCompress::NXz::CEncoder();
            _compressEncoder = encoder;
          }
          else if (method == NCompressionMethod::kPPMd)
          {
            _compressExtractVersion = NCompressionMethod::kExtractVersion_PPMd;
            NCompress::NPpmdZip::CEncoder *encoder = new NCompress::NPpmdZip::CEncoder();
            _compressEncoder = encoder;
          }
          else
          {
          CMethodId methodId;
          switch (method)
          {
            case NCompressionMethod::kBZip2:
              methodId = kMethodId_BZip2;
              _compressExtractVersion = NCompressionMethod::kExtractVersion_BZip2;
              break;
            default:
              _compressExtractVersion = ((method == NCompressionMethod::kDeflate64) ?
                  NCompressionMethod::kExtractVersion_Deflate64 :
                  NCompressionMethod::kExtractVersion_Deflate);
              methodId = kMethodId_ZipBase + method;
              break;
          }
          RINOK(CreateCoder_Id(
              EXTERNAL_CODECS_LOC_VARS
              methodId, true, _compressEncoder))
          if (!_compressEncoder)
            return E_NOTIMPL;

          if (method == NCompressionMethod::kDeflate ||
              method == NCompressionMethod::kDeflate64)
          {
          }
          else if (method == NCompressionMethod::kBZip2)
          {
          }
          }
          {
            CMyComPtr<ICompressSetCoderProperties> setCoderProps;
            _compressEncoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProps);
            if (setCoderProps)
            {
              if (!_options._methods.IsEmpty())
              {
                COneMethodInfo *oneMethodMain = &_options._methods[0];

                RINOK(oneMethodMain->SetCoderProps(setCoderProps,
                    _options.DataSizeReduce_Defined ? &_options.DataSizeReduce : NULL))
              }
            }
          }
          if (method == NCompressionMethod::kLZMA)
            _isLzmaEos = _lzmaEncoder->Encoder->IsWriteEndMark();
        }

        if (method == NCompressionMethod::kLZMA)
          opRes.LzmaEos = _isLzmaEos;

        CMyComPtr<ISequentialOutStream> outStreamNew;
        if (_options.Password_Defined)
          outStreamNew = _cryptoStream;
        else
          outStreamNew = outStream;
        if (_compressExtractVersion > opRes.ExtractVersion)
          opRes.ExtractVersion = _compressExtractVersion;

        {
          CMyComPtr<ICompressSetCoderPropertiesOpt> optProps;
          _compressEncoder->QueryInterface(IID_ICompressSetCoderPropertiesOpt, (void **)&optProps);
          if (optProps)
          {
            const PROPID propID = NCoderPropID::kExpectedDataSize;
            NWindows::NCOM::CPropVariant prop = (UInt64)expectedDataSize;
            RINOK(optProps->SetCoderPropertiesOpt(&propID, &prop, 1))
          }
        }
        
        try {
        RINOK(_compressEncoder->Code(inCrcStream, outStreamNew, NULL, NULL, progress))
        } catch (...) { return E_FAIL; }
        break;
      }
      } // switch end

      if (_options.Password_Defined)
      {
        RINOK(_cryptoStream->OutStreamFinish())
      }
    }

    if (_options.Password_Defined)
    {
      if (_options.IsAesMode)
      {
        RINOK(_filterAesSpec->WriteFooter(outStream))
      }
    }
    
    RINOK(outStream->Seek(0, STREAM_SEEK_CUR, &opRes.PackSize))

    {
      opRes.CRC = inCrcStream->GetCRC();
      opRes.UnpackSize = inCrcStream->GetSize();
      opRes.Method = method;
    }

    if (!inCrcStream->WasFinished())
      return E_FAIL;

    if (_options.Password_Defined)
    {
      if (opRes.PackSize < opRes.UnpackSize +
          (_options.IsAesMode ? _filterAesSpec->GetAddPackSize() : NCrypto::NZip::kHeaderSize))
        break;
    }
    else if (opRes.PackSize < opRes.UnpackSize)
      break;
  }

  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Archive/Zip/ZipHandler.cpp ================ */
// ZipHandler.cpp

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
#ifndef Z7_ZIP_LZFSE_DISABLE
// amalgamation: header emitted in prologue
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

using namespace NWindows;

namespace NArchive {
namespace NZip {

static const char * const kHostOS[] =
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


const char * const kMethodNames1[kNumMethodNames1] =
{
    "Store"
  , "Shrink"
  , "Reduce1"
  , "Reduce2"
  , "Reduce3"
  , "Reduce4"
  , "Implode"
  , NULL // "Tokenize"
  , "Deflate"
  , "Deflate64"
  , "PKImploding"
  , NULL
  , "BZip2"
  , NULL
  , "LZMA"
  /*
  , NULL
  , NULL
  , NULL
  , NULL
  , NULL
  , "zstd-pk" // deprecated
  */
};


const char * const kMethodNames2[kNumMethodNames2] =
{
    "zstd"
  , "MP3"
  , "xz"
  , "Jpeg"
  , "WavPack"
  , "PPMd"
  , "LZFSE" // , "WzAES"
};

#define kMethod_AES "AES"
#define kMethod_ZipCrypto "ZipCrypto"
#define kMethod_StrongCrypto "StrongCrypto"

static const char * const kDeflateLevels[4] =
{
    "Normal"
  , "Maximum"
  , "Fast"
  , "Fastest"
};


static const CUInt32PCharPair g_HeaderCharacts[] =
{
  { 0, "Encrypt" },
  { 3, "Descriptor" },
  // { 4, "Enhanced" },
  // { 5, "Patched" },
  { 6, kMethod_StrongCrypto },
  { 11, "UTF8" },
  { 14, "Alt" }
};

struct CIdToNamePair
{
  unsigned Id;
  const char *Name;
};


static const CIdToNamePair k_StrongCryptoPairs[] =
{
  { NStrongCrypto_AlgId::kDES, "DES" },
  { NStrongCrypto_AlgId::kRC2old, "RC2a" },
  { NStrongCrypto_AlgId::k3DES168, "3DES-168" },
  { NStrongCrypto_AlgId::k3DES112, "3DES-112" },
  { NStrongCrypto_AlgId::kAES128, "pkAES-128" },
  { NStrongCrypto_AlgId::kAES192, "pkAES-192" },
  { NStrongCrypto_AlgId::kAES256, "pkAES-256" },
  { NStrongCrypto_AlgId::kRC2, "RC2" },
  { NStrongCrypto_AlgId::kBlowfish, "Blowfish" },
  { NStrongCrypto_AlgId::kTwofish, "Twofish" },
  { NStrongCrypto_AlgId::kRC4, "RC4" }
};

static const char *FindNameForId(const CIdToNamePair *pairs, unsigned num, unsigned id)
{
  for (unsigned i = 0; i < num; i++)
  {
    const CIdToNamePair &pair = pairs[i];
    if (id == pair.Id)
      return pair.Name;
  }
  return NULL;
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
  // kpidPosixAttrib,
  kpidEncrypted,
  kpidComment,
  kpidCRC,
  kpidMethod,
  kpidCharacts,
  kpidHostOS,
  kpidUnpackVer,
  kpidVolumeIndex,
  kpidOffset
  // kpidIsAltStream
  // , kpidChangeTime // for debug
  // , 255  // for debug
};

static const Byte kArcProps[] =
{
  kpidEmbeddedStubSize,
  kpidBit64,
  kpidComment,
  kpidCharacts,
  kpidTotalPhySize,
  kpidIsVolume,
  kpidVolumeIndex,
  kpidNumVolumes
};

CHandler::CHandler()
{
  InitMethodProps();
}

static AString BytesToString(const CByteBuffer &data)
{
  AString s;
  s.SetFrom_CalcLen((const char *)(const Byte *)data, (unsigned)data.Size());
  return s;
}

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidBit64:  if (m_Archive.IsZip64) prop = m_Archive.IsZip64; break;
    case kpidComment:  if (m_Archive.ArcInfo.Comment.Size() != 0) prop = MultiByteToUnicodeString(BytesToString(m_Archive.ArcInfo.Comment), CP_ACP); break;

    case kpidPhySize:  prop = m_Archive.GetPhySize(); break;
    case kpidOffset:  prop = m_Archive.GetOffset(); break;

    case kpidEmbeddedStubSize:
    {
      UInt64 stubSize = m_Archive.GetEmbeddedStubSize();
      if (stubSize != 0)
        prop = stubSize;
      break;
    }

    case kpidTotalPhySize: if (m_Archive.IsMultiVol) prop = m_Archive.Vols.TotalBytesSize; break;
    case kpidVolumeIndex: if (m_Archive.IsMultiVol) prop = (UInt32)m_Archive.Vols.StartVolIndex; break;
    case kpidIsVolume: if (m_Archive.IsMultiVol) prop = true; break;
    case kpidNumVolumes: if (m_Archive.IsMultiVol) prop = (UInt32)m_Archive.Vols.Streams.Size(); break;

    case kpidCharacts:
    {
      AString s;
      
      if (m_Archive.LocalsWereRead)
      {
        s.Add_OptSpaced("Local");

        if (m_Archive.LocalsCenterMerged)
          s.Add_OptSpaced("Central");
      }

      if (m_Archive.IsZip64)
        s.Add_OptSpaced("Zip64");

      if (m_Archive.IsCdUnsorted)
        s.Add_OptSpaced("Unsorted_CD");

      if (m_Archive.IsApk)
        s.Add_OptSpaced("apk");

      if (m_Archive.ExtraMinorError)
        s.Add_OptSpaced("Minor_Extra_ERROR");

      if (!s.IsEmpty())
        prop = s;
      break;
    }

    case kpidWarningFlags:
    {
      UInt32 v = 0;
      // if (m_Archive.ExtraMinorError) v |= kpv_ErrorFlags_HeadersError;
      if (m_Archive.HeadersWarning) v |= kpv_ErrorFlags_HeadersError;
      if (v != 0)
        prop = v;
      break;
    }

    case kpidWarning:
    {
      AString s;
      if (m_Archive.Overflow32bit)
        s.Add_OptSpaced("32-bit overflow in headers");
      if (m_Archive.Cd_NumEntries_Overflow_16bit)
        s.Add_OptSpaced("16-bit overflow for number of files in headers");
      if (!s.IsEmpty())
        prop = s;
      break;
    }

    case kpidError:
    {
      if (!m_Archive.Vols.MissingName.IsEmpty())
      {
        UString s("Missing volume : ");
        s += m_Archive.Vols.MissingName;
        prop = s;
      }
      break;
    }

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!m_Archive.IsArc) v |= kpv_ErrorFlags_IsNotArc;
      if (m_Archive.HeadersError) v |= kpv_ErrorFlags_HeadersError;
      if (m_Archive.UnexpectedEnd) v |= kpv_ErrorFlags_UnexpectedEnd;
      if (m_Archive.ArcInfo.Base < 0)
      {
        /* We try to support case when we have sfx-zip with embedded stub,
           but the stream has access only to zip part.
           In that case we ignore UnavailableStart error.
           maybe we must show warning in that case. */
        UInt64 stubSize = m_Archive.GetEmbeddedStubSize();
        if (stubSize < (UInt64)-m_Archive.ArcInfo.Base)
          v |= kpv_ErrorFlags_UnavailableStart;
      }
      if (m_Archive.NoCentralDir) v |= kpv_ErrorFlags_UnconfirmedStart;
      prop = v;
      break;
    }

    case kpidReadOnly:
    {
      if (m_Archive.IsOpen())
        if (!m_Archive.CanUpdate())
          prop = true;
      break;
    }

    // case kpidIsAltStream: prop = true; break;
    default: break;
  }
  return prop.Detach(value);
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = m_Items.Size();
  return S_OK;
}


static bool NtfsUnixTimeToProp(bool fromCentral,
    const CExtraBlock &extra,
    unsigned ntfsIndex, unsigned unixIndex, NWindows::NCOM::CPropVariant &prop)
{
  {
    FILETIME ft;
    if (extra.GetNtfsTime(ntfsIndex, ft))
    {
      PropVariant_SetFrom_NtfsTime(prop, ft);
      return true;
    }
  }
  {
    UInt32 unixTime = 0;
    if (!extra.GetUnixTime(fromCentral, unixIndex, unixTime))
      return false;
    /*
    // we allow unixTime == 0
    if (unixTime == 0)
      return false;
    */
    PropVariant_SetFrom_UnixTime(prop, unixTime);
    return true;
  }
}


Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value))
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItemEx &item = m_Items[index];
  const CExtraBlock &extra = item.GetMainExtra();
  
  switch (propID)
  {
    case kpidPath:
    {
      UString res;
      item.GetUnicodeString(res, item.Name, false, _forceCodePage, _specifiedCodePage);
      NItemName::ReplaceToOsSlashes_Remove_TailSlash(res,
          item.Is_MadeBy_Unix() // useBackslashReplacement
          );
      /*
      if (item.ParentOfAltStream >= 0)
      {
        const CItemEx &prevItem = m_Items[item.ParentOfAltStream];
        UString prevName;
        prevItem.GetUnicodeString(prevName, prevItem.Name, false, _forceCodePage, _specifiedCodePage);
        NItemName::ReplaceToOsSlashes_Remove_TailSlash(prevName);
        if (res.IsPrefixedBy(prevName))
          if (IsString1PrefixedByString2(res.Ptr(prevName.Len()), k_SpecName_NTFS_STREAM))
          {
            res.Delete(prevName.Len(), (unsigned)strlen(k_SpecName_NTFS_STREAM));
            res.Insert(prevName.Len(), L":");
          }
      }
      */
      prop = res;
      break;
    }
    
    case kpidIsDir:  prop = item.IsDir(); break;
    case kpidSize:
    {
      if (!item.IsBadDescriptor())
        prop = item.Size;
      break;
    }

    case kpidPackSize:  prop = item.PackSize; break;
    
    case kpidCTime:
      NtfsUnixTimeToProp(item.FromCentral, extra,
          NFileHeader::NNtfsExtra::kCTime,
          NFileHeader::NUnixTime::kCTime, prop);
      break;
    
    case kpidATime:
      NtfsUnixTimeToProp(item.FromCentral, extra,
          NFileHeader::NNtfsExtra::kATime,
          NFileHeader::NUnixTime::kATime, prop);
      break;
    
    case kpidMTime:
    {
      if (!NtfsUnixTimeToProp(item.FromCentral, extra,
          NFileHeader::NNtfsExtra::kMTime,
          NFileHeader::NUnixTime::kMTime, prop))
      {
        if (item.Time != 0)
          PropVariant_SetFrom_DosTime(prop, item.Time);
      }
      break;
    }

    case kpidTimeType:
    {
      FILETIME ft;
      UInt32 unixTime;
      UInt32 type;
      if (extra.GetNtfsTime(NFileHeader::NNtfsExtra::kMTime, ft))
        type = NFileTimeType::kWindows;
      else if (extra.GetUnixTime(item.FromCentral, NFileHeader::NUnixTime::kMTime, unixTime))
        type = NFileTimeType::kUnix;
      else
        type = NFileTimeType::kDOS;
      prop = type;
      break;
    }
    
    /*
    // for debug to get Dos time values:
    case kpidChangeTime: if (item.Time != 0) PropVariant_SetFrom_DosTime(prop, item.Time); break;
    // for debug
    // time difference (dos - utc)
    case 255:
    {
      if (NtfsUnixTimeToProp(item.FromCentral, extra,
          NFileHeader::NNtfsExtra::kMTime,
          NFileHeader::NUnixTime::kMTime, prop))
      {
        FILETIME localFileTime;
        if (item.Time != 0 && NTime::DosTime_To_FileTime(item.Time, localFileTime))
        {
          UInt64 t1 = FILETIME_To_UInt64(prop.filetime);
          UInt64 t2 = FILETIME_To_UInt64(localFileTime);
          prop.Set_Int64(t2 - t1);
        }
      }
      break;
    }
    */
    
    case kpidAttrib:  prop = item.GetWinAttrib(); break;
    
    case kpidPosixAttrib:
    {
      UInt32 attrib;
      if (item.GetPosixAttrib(attrib))
        prop = attrib;
      break;
    }
    
    case kpidEncrypted:  prop = item.IsEncrypted(); break;
    
    case kpidComment:
    {
      if (item.Comment.Size() != 0)
      {
        UString res;
        item.GetUnicodeString(res, BytesToString(item.Comment), true, _forceCodePage, _specifiedCodePage);
        prop = res;
      }
      break;
    }
    
    case kpidCRC:  if (item.IsThereCrc()) prop = item.Crc; break;
    
    case kpidMethod:
    {
      AString m;
      bool isWzAes = false;
      unsigned id = item.Method;

      if (id == NFileHeader::NCompressionMethod::kWzAES)
      {
        CWzAesExtra aesField;
        if (extra.GetWzAes(aesField))
        {
          m += kMethod_AES;
          m.Add_Minus();
          m.Add_UInt32(((unsigned)aesField.Strength + 1) * 64);
          id = aesField.Method;
          isWzAes = true;
        }
      }
      
      if (item.IsEncrypted())
      if (!isWzAes)
      {
        if (item.IsStrongEncrypted())
        {
          CStrongCryptoExtra f;
          f.AlgId = 0;
          if (extra.GetStrongCrypto(f))
          {
            const char *s = FindNameForId(k_StrongCryptoPairs, Z7_ARRAY_SIZE(k_StrongCryptoPairs), f.AlgId);
            if (s)
              m += s;
            else
            {
              m += kMethod_StrongCrypto;
              m.Add_Colon();
              m.Add_UInt32(f.AlgId);
            }
            if (f.CertificateIsUsed())
              m += "-Cert";
          }
          else
            m += kMethod_StrongCrypto;
        }
        else
          m += kMethod_ZipCrypto;
      }

      m.Add_Space_if_NotEmpty();
      
      {
        const char *s = NULL;
        if (id < kNumMethodNames1)
          s = kMethodNames1[id];
        else
        {
          const int id2 = (int)id - (int)kMethodNames2Start;
          if (id2 >= 0 && (unsigned)id2 < kNumMethodNames2)
            s = kMethodNames2[id2];
        }
        if (s)
          m += s;
        else
          m.Add_UInt32(id);
      }
      {
        unsigned level = item.GetDeflateLevel();
        if (level != 0)
        {
          if (id == NFileHeader::NCompressionMethod::kLZMA)
          {
            if (level & 1)
              m += ":eos";
            level &= ~(unsigned)1;
          }
          else if (id == NFileHeader::NCompressionMethod::kDeflate)
          {
            m.Add_Colon();
            m += kDeflateLevels[level];
            level = 0;
          }

          if (level != 0)
          {
            m += ":v";
            m.Add_UInt32(level);
          }
        }
      }
      
      prop = m;
      break;
    }

    case kpidCharacts:
    {
      AString s;
      
      if (item.FromLocal)
      {
        s.Add_OptSpaced("Local");

        item.LocalExtra.PrintInfo(s);

        if (item.FromCentral)
        {
          s.Add_OptSpaced(":");
          s.Add_OptSpaced("Central");
        }
      }

      if (item.FromCentral)
      {
        item.CentralExtra.PrintInfo(s);
      }

      UInt32 flags = item.Flags;
      flags &= ~(unsigned)6; // we don't need compression related bits here.

      if (flags != 0)
      {
        const AString s2 = FlagsToString(g_HeaderCharacts, Z7_ARRAY_SIZE(g_HeaderCharacts), flags);
        if (!s2.IsEmpty())
        {
          if (!s.IsEmpty())
            s.Add_OptSpaced(":");
          s.Add_OptSpaced(s2);
        }
      }

      if (item.IsBadDescriptor())
        s.Add_OptSpaced("Descriptor_ERROR");
    
      if (!s.IsEmpty())
        prop = s;
      break;
    }

    case kpidHostOS:
    {
      if (item.FromCentral)
      {
        // 18.06: now we use HostOS only from Central::MadeByVersion
        const Byte hostOS = item.MadeByVersion.HostOS;
        TYPE_TO_PROP(kHostOS, hostOS, prop);
      }
      break;
    }
    
    case kpidUnpackVer:
      prop = (UInt32)item.ExtractVersion.Version;
      break;

    case kpidVolumeIndex:
      prop = item.Disk;
      break;

    case kpidOffset:
      prop = item.LocalHeaderPos;
      break;

    /*
    case kpidIsAltStream:
      prop = (bool)(item.ParentOfAltStream >= 0); // item.IsAltStream();
      break;

    case kpidName:
      if (item.ParentOfAltStream >= 0)
      {
        // extract name of stream here
      }
      break;
    */
    default: break;
  }
  
  return prop.Detach(value);
  COM_TRY_END
}



/*
Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *numProps)
{
  *numProps = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *propID)
{
  UNUSED_VAR(index);
  *propID = 0;
  *name = 0;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *parentType)
{
  *parentType = NParentType::kDir;
  *parent = (UInt32)(Int32)-1;
  if (index >= m_Items.Size())
    return S_OK;
  const CItemEx &item = m_Items[index];

  if (item.ParentOfAltStream >= 0)
  {
    *parentType = NParentType::kAltStream;
    *parent = item.ParentOfAltStream;
  }
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetRawProp(UInt32 index, PROPID propID, const void **data, UInt32 *dataSize, UInt32 *propType)
{
  UNUSED_VAR(index);
  UNUSED_VAR(propID);
  *data = NULL;
  *dataSize = 0;
  *propType = 0;
  return S_OK;
}


void CHandler::MarkAltStreams(CObjectVector<CItemEx> &items)
{
  int prevIndex = -1;
  UString prevName;
  UString name;

  for (unsigned i = 0; i < items.Size(); i++)
  {
    CItemEx &item = m_Items[i];
    if (item.IsAltStream())
    {
      if (prevIndex == -1)
        continue;
      if (prevName.IsEmpty())
      {
        const CItemEx &prevItem = m_Items[prevIndex];
        prevItem.GetUnicodeString(prevName, prevItem.Name, false, _forceCodePage, _specifiedCodePage);
        NItemName::ReplaceToOsSlashes_Remove_TailSlash(prevName);
      }
      name.Empty();
      item.GetUnicodeString(name, item.Name, false, _forceCodePage, _specifiedCodePage);
      NItemName::ReplaceToOsSlashes_Remove_TailSlash(name);
      
      if (name.IsPrefixedBy(prevName))
        if (IsString1PrefixedByString2(name.Ptr(prevName.Len()), k_SpecName_NTFS_STREAM))
          item.ParentOfAltStream = prevIndex;
    }
    else
    {
      prevIndex = i;
      prevName.Empty();
    }
  }
}
*/

Z7_COM7F_IMF(CHandler::Open(IInStream *inStream,
    const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  try
  {
    Close();
    m_Archive.Force_ReadLocals_Mode = _force_OpenSeq;
    // m_Archive.Disable_VolsRead = _force_OpenSeq;
    // m_Archive.Disable_FindMarker = _force_OpenSeq;
    HRESULT res = m_Archive.Open(inStream, maxCheckStartPosition, callback, m_Items);
    if (res != S_OK)
    {
      m_Items.Clear();
      m_Archive.ClearRefs(); // we don't want to clear error flags
    }
    // MarkAltStreams(m_Items);
    return res;
  }
  catch(...) { Close(); throw; }
  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close())
{
  m_Items.Clear();
  m_Archive.Close();
  return S_OK;
}


Z7_CLASS_IMP_NOQIB_3(
  CLzmaDecoder
  , ICompressCoder
  , ICompressSetFinishMode
  , ICompressGetInStreamProcessedSize
)
public:
  CMyComPtr2_Create<ICompressCoder, NCompress::NLzma::CDecoder> Decoder;
};

static const unsigned kZipLzmaPropsSize = 4 + LZMA_PROPS_SIZE;

Z7_COM7F_IMF(CLzmaDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  Byte buf[kZipLzmaPropsSize];
  RINOK(ReadStream_FALSE(inStream, buf, kZipLzmaPropsSize))
  if (buf[2] != LZMA_PROPS_SIZE || buf[3] != 0)
    return E_NOTIMPL;
  RINOK(Decoder->SetDecoderProperties2(buf + 4, LZMA_PROPS_SIZE))
  UInt64 inSize2 = 0;
  if (inSize)
  {
    inSize2 = *inSize;
    if (inSize2 < kZipLzmaPropsSize)
      return S_FALSE;
    inSize2 -= kZipLzmaPropsSize;
  }
  return Decoder.Interface()->Code(inStream, outStream, inSize ? &inSize2 : NULL, outSize, progress);
}

Z7_COM7F_IMF(CLzmaDecoder::SetFinishMode(UInt32 finishMode))
{
  Decoder->FinishStream = (finishMode != 0);
  return S_OK;
}

Z7_COM7F_IMF(CLzmaDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = Decoder->GetInputProcessedSize() + kZipLzmaPropsSize;
  return S_OK;
}







struct CMethodItem
{
  unsigned ZipMethod;
  CMyComPtr<ICompressCoder> Coder;
};



class CZipDecoder
{
  CMyComPtr2<ICompressFilter, NCrypto::NZip::CDecoder> _zipCryptoDecoder;
  CMyComPtr2<ICompressFilter, NCrypto::NZipStrong::CDecoder> _pkAesDecoder;
  CMyComPtr2<ICompressFilter, NCrypto::NWzAes::CDecoder> _wzAesDecoder;

  CMyComPtr2<ISequentialInStream, CFilterCoder> filterStream;
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  CObjectVector<CMethodItem> methodItems;

  CLzmaDecoder *lzmaDecoderSpec;
public:
  CZipDecoder():
      lzmaDecoderSpec(NULL)
    {}

  HRESULT Decode(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CInArchive &archive, const CItemEx &item,
    ISequentialOutStream *realOutStream,
    IArchiveExtractCallback *extractCallback,
    ICompressProgressInfo *compressProgress,
    #ifndef Z7_ST
    UInt32 numThreads, UInt64 memUsage,
    #endif
    Int32 &res);
};


static HRESULT SkipStreamData(ISequentialInStream *stream,
    ICompressProgressInfo *progress, UInt64 packSize, UInt64 unpackSize,
    bool &thereAreData)
{
  thereAreData = false;
  const size_t kBufSize = 1 << 12;
  Byte buf[kBufSize];
  UInt64 prev = packSize;
  for (;;)
  {
    size_t size = kBufSize;
    RINOK(ReadStream(stream, buf, &size))
    if (size == 0)
      return S_OK;
    thereAreData = true;
    packSize += size;
    if ((packSize - prev) >= (1 << 22))
    {
      prev = packSize;
      RINOK(progress->SetRatioInfo(&packSize, &unpackSize))
    }
  }
}



Z7_CLASS_IMP_NOQIB_1(
  COutStreamWithPadPKCS7
  , ISequentialOutStream
)
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
  UInt64 _padPos;
  UInt32 _padSize;
  bool _padFailure;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }

  // padSize == 0 means (no_pad Mode)
  void Init(UInt64 padPos, UInt32 padSize)
  {
    _padPos = padPos;
    _padSize = padSize;
    _size = 0;
    _padFailure = false;
  }
  UInt64 GetSize() const { return _size; }
  bool WasPadFailure() const { return _padFailure; }
};


Z7_COM7F_IMF(COutStreamWithPadPKCS7::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 written = 0;
  HRESULT result = S_OK;
  if (_size < _padPos)
  {
    const UInt64 rem = _padPos - _size;
    UInt32 num = size;
    if (num > rem)
      num = (UInt32)rem;
    result = _stream->Write(data, num, &written);
    _size += written;
    if (processedSize)
      *processedSize = written;
    if (_size != _padPos || result != S_OK)
      return result;
    size -= written;
    data = ((const Byte *)data) + written;
  }
  _size += size;
  written += size;
  if (processedSize)
    *processedSize = written;
  if (_padSize != 0)
  for (; size != 0; size--)
  {
    if (*(const Byte *)data != _padSize)
      _padFailure = true;
    data = ((const Byte *)data) + 1;
  }
  return result;
}



HRESULT CZipDecoder::Decode(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CInArchive &archive, const CItemEx &item,
    ISequentialOutStream *realOutStream,
    IArchiveExtractCallback *extractCallback,
    ICompressProgressInfo *compressProgress,
    #ifndef Z7_ST
    UInt32 numThreads, UInt64 memUsage,
    #endif
    Int32 &res)
{
  res = NExtract::NOperationResult::kHeadersError;
  
  CFilterCoder::C_InStream_Releaser inStreamReleaser;
  CFilterCoder::C_Filter_Releaser filterReleaser;

  bool needCRC = true;
  bool wzAesMode = false;
  bool pkAesMode = false;

  bool badDescriptor = item.IsBadDescriptor();
  if (badDescriptor)
    needCRC = false;

  
  unsigned id = item.Method;

  CWzAesExtra aesField;
  // LZFSE and WinZip's AES use same id - kWzAES.

  if (id == NFileHeader::NCompressionMethod::kWzAES)
  {
    if (item.GetMainExtra().GetWzAes(aesField))
    {
      if (!item.IsEncrypted())
      {
        res = NExtract::NOperationResult::kUnsupportedMethod;
        return S_OK;
      }
      wzAesMode = true;
      needCRC = aesField.NeedCrc();
    }
  }

  if (!wzAesMode)
  if (item.IsEncrypted())
  {
    if (item.IsStrongEncrypted())
    {
      CStrongCryptoExtra f;
      if (!item.CentralExtra.GetStrongCrypto(f))
      {
        res = NExtract::NOperationResult::kUnsupportedMethod;
        return S_OK;
      }
      pkAesMode = true;
    }
  }

  CMyComPtr2_Create<ISequentialOutStream, COutStreamWithCRC> outStream;
  outStream->SetStream(realOutStream);
  outStream->Init(needCRC);
  
  CMyComPtr<ISequentialInStream> packStream;
  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> inStream;

  {
    UInt64 packSize = item.PackSize;
    if (wzAesMode)
    {
      if (packSize < NCrypto::NWzAes::kMacSize)
        return S_OK;
      packSize -= NCrypto::NWzAes::kMacSize;
    }
    RINOK(archive.GetItemStream(item, true, packStream))
    if (!packStream)
    {
      res = NExtract::NOperationResult::kUnavailable;
      return S_OK;
    }
    inStream->SetStream(packStream);
    inStream->Init(packSize);
  }

  
  res = NExtract::NOperationResult::kDataError;
  
  CMyComPtr<ICompressFilter> cryptoFilter;
  
  if (item.IsEncrypted())
  {
    if (wzAesMode)
    {
      id = aesField.Method;
      _wzAesDecoder.Create_if_Empty();
      cryptoFilter = _wzAesDecoder;
      if (!_wzAesDecoder->SetKeyMode(aesField.Strength))
      {
        res = NExtract::NOperationResult::kUnsupportedMethod;
        return S_OK;
      }
    }
    else if (pkAesMode)
    {
      _pkAesDecoder.Create_if_Empty();
      cryptoFilter = _pkAesDecoder;
    }
    else
    {
      _zipCryptoDecoder.Create_if_Empty();
      cryptoFilter = _zipCryptoDecoder;
    }
    
    CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
    RINOK(cryptoFilter.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword))
    if (!cryptoSetPassword)
      return E_FAIL;
    
    if (!getTextPassword)
      extractCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&getTextPassword);
    
    if (getTextPassword)
    {
      CMyComBSTR_Wipe password;
      RINOK(getTextPassword->CryptoGetTextPassword(&password))
      AString_Wipe charPassword;
      if (password)
      {
#if 0 && defined(_WIN32)
        // do we need UTF-8 passwords here ?
        if (item.GetHostOS() == NFileHeader::NHostOS::kUnix // 24.05
            // || item.IsUtf8() // 22.00
            )
        {
          // throw 1;
          ConvertUnicodeToUTF8((LPCOLESTR)password, charPassword);
        }
        else
#endif
        {
          UnicodeStringToMultiByte2(charPassword, (LPCOLESTR)password, CP_ACP);
        }
        /*
        if (wzAesMode || pkAesMode)
        {
        }
        else
        {
          // PASSWORD encoding for ZipCrypto:
          // pkzip25 / WinZip / Windows probably use ANSI
          // 7-Zip <  4.43 creates ZIP archives with OEM encoding in password
          // 7-Zip >= 4.43 creates ZIP archives only with ASCII characters in password
          // 7-Zip <  17.00 uses CP_OEMCP for password decoding
          // 7-Zip >= 17.00 uses CP_ACP   for password decoding
        }
        */
      }
      HRESULT result = cryptoSetPassword->CryptoSetPassword(
        (const Byte *)(const char *)charPassword, charPassword.Len());
      if (result != S_OK)
      {
        res = NExtract::NOperationResult::kWrongPassword;
        return S_OK;
      }
    }
    else
    {
      res = NExtract::NOperationResult::kWrongPassword;
      return S_OK;
      // RINOK(cryptoSetPassword->CryptoSetPassword(NULL, 0));
    }
  }
  
  unsigned m;
  for (m = 0; m < methodItems.Size(); m++)
    if (methodItems[m].ZipMethod == id)
      break;

  if (m == methodItems.Size())
  {
    CMethodItem mi;
    mi.ZipMethod = id;
    if (id == NFileHeader::NCompressionMethod::kStore)
      mi.Coder = new NCompress::CCopyCoder;
    else if (id == NFileHeader::NCompressionMethod::kShrink)
      mi.Coder = new NCompress::NShrink::CDecoder;
    else if (id == NFileHeader::NCompressionMethod::kImplode)
      mi.Coder = new NCompress::NImplode::NDecoder::CCoder;
    else if (id == NFileHeader::NCompressionMethod::kLZMA)
    {
      lzmaDecoderSpec = new CLzmaDecoder;
      mi.Coder = lzmaDecoderSpec;
    }
    else if (id == NFileHeader::NCompressionMethod::kXz)
      mi.Coder = new NCompress::NXz::CComDecoder;
    else if (id == NFileHeader::NCompressionMethod::kPPMd)
      mi.Coder = new NCompress::NPpmdZip::CDecoder(true);
    else if (id == NFileHeader::NCompressionMethod::kZstdWz)
      mi.Coder = new NCompress::NZstd::CDecoder();
#ifndef Z7_ZIP_LZFSE_DISABLE
    else if (id == NFileHeader::NCompressionMethod::kWzAES)
      mi.Coder = new NCompress::NLzfse::CDecoder;
#endif
    else
    {
      CMethodId szMethodID;
      if (id == NFileHeader::NCompressionMethod::kBZip2)
        szMethodID = kMethodId_BZip2;
      else
      {
        if (id > 0xFF)
        {
          res = NExtract::NOperationResult::kUnsupportedMethod;
          return S_OK;
        }
        szMethodID = kMethodId_ZipBase + (Byte)id;
      }

      RINOK(CreateCoder_Id(EXTERNAL_CODECS_LOC_VARS szMethodID, false, mi.Coder))

      if (!mi.Coder)
      {
        res = NExtract::NOperationResult::kUnsupportedMethod;
        return S_OK;
      }
    }
    m = methodItems.Add(mi);
  }

  const CMethodItem &mi = methodItems[m];
  ICompressCoder *coder = mi.Coder;

  
  #ifndef Z7_ST
  {
    CMyComPtr<ICompressSetCoderMt> setCoderMt;
    coder->QueryInterface(IID_ICompressSetCoderMt, (void **)&setCoderMt);
    if (setCoderMt)
    {
      RINOK(setCoderMt->SetNumberOfThreads(numThreads))
    }
  }
  // if (memUsage != 0)
  {
    CMyComPtr<ICompressSetMemLimit> setMemLimit;
    coder->QueryInterface(IID_ICompressSetMemLimit, (void **)&setMemLimit);
    if (setMemLimit)
    {
      RINOK(setMemLimit->SetMemLimit(memUsage))
    }
  }
  #endif

  {
    CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
    coder->QueryInterface(IID_ICompressSetDecoderProperties2, (void **)&setDecoderProperties);
    if (setDecoderProperties)
    {
      Byte properties = (Byte)item.Flags;
      RINOK(setDecoderProperties->SetDecoderProperties2(&properties, 1))
    }
  }
  
  
  bool isFullStreamExpected = (!item.HasDescriptor() || item.PackSize != 0);
  bool needReminderCheck = false;

  bool dataAfterEnd = false;
  bool truncatedError = false;
  bool lzmaEosError = false;
  bool headersError  = false;
  bool padError = false;
  bool readFromFilter = false;

  const bool useUnpackLimit = (id == NFileHeader::NCompressionMethod::kStore
      || !item.HasDescriptor()
      || item.Size >= ((UInt64)1 << 32)
      || item.LocalExtra.IsZip64
      || item.CentralExtra.IsZip64
      );

  {
    HRESULT result = S_OK;
    if (item.IsEncrypted())
    {
      if (!filterStream.IsDefined())
        filterStream.SetFromCls(new CFilterCoder(false));
     
      filterReleaser.FilterCoder = filterStream.ClsPtr();
      filterStream->Filter = cryptoFilter;
      
      if (wzAesMode)
      {
        result = _wzAesDecoder->ReadHeader(inStream);
        if (result == S_OK)
        {
          if (!_wzAesDecoder->Init_and_CheckPassword())
          {
            res = NExtract::NOperationResult::kWrongPassword;
            return S_OK;
          }
        }
      }
      else if (pkAesMode)
      {
        isFullStreamExpected = false;
        result = _pkAesDecoder->ReadHeader(inStream, item.Crc, item.Size);
        if (result == S_OK)
        {
          bool passwOK;
          result = _pkAesDecoder->Init_and_CheckPassword(passwOK);
          if (result == S_OK && !passwOK)
          {
            res = NExtract::NOperationResult::kWrongPassword;
            return S_OK;
          }
        }
      }
      else
      {
        result = _zipCryptoDecoder->ReadHeader(inStream);
        if (result == S_OK)
        {
          _zipCryptoDecoder->Init_BeforeDecode();
          
          /* Info-ZIP modification to ZipCrypto format:
               if bit 3 of the general purpose bit flag is set,
               it uses high byte of 16-bit File Time.
             Info-ZIP code probably writes 2 bytes of File Time.
             We check only 1 byte. */

          // UInt32 v1 = GetUi16(_zipCryptoDecoder->_header + NCrypto::NZip::kHeaderSize - 2);
          // UInt32 v2 = (item.HasDescriptor() ? (item.Time & 0xFFFF) : (item.Crc >> 16));

          Byte v1 = _zipCryptoDecoder->_header[NCrypto::NZip::kHeaderSize - 1];
          Byte v2 = (Byte)(item.HasDescriptor() ? (item.Time >> 8) : (item.Crc >> 24));

          if (v1 != v2)
          {
            res = NExtract::NOperationResult::kWrongPassword;
            return S_OK;
          }
        }
      }
    }

    if (result == S_OK)
    {
      CMyComPtr<ICompressSetFinishMode> setFinishMode;
      coder->QueryInterface(IID_ICompressSetFinishMode, (void **)&setFinishMode);
      if (setFinishMode)
      {
        RINOK(setFinishMode->SetFinishMode(BoolToUInt(true)))
      }
      
      const UInt64 coderPackSize = inStream->GetRem();

      if (id == NFileHeader::NCompressionMethod::kStore && item.IsEncrypted())
      {
        // for debug : we can disable this code (kStore + 50), if we want to test CopyCoder+Filter
        // here we use filter without CopyCoder
        readFromFilter = false;
        
        COutStreamWithPadPKCS7 *padStreamSpec = NULL;
        CMyComPtr<ISequentialOutStream> padStream;
        UInt32 padSize = 0;
        
        if (pkAesMode)
        {
          padStreamSpec = new COutStreamWithPadPKCS7;
          padStream = padStreamSpec;
          padSize = _pkAesDecoder->GetPadSize((UInt32)item.Size);
          padStreamSpec->SetStream(outStream);
          padStreamSpec->Init(item.Size, padSize);
        }

        // Here we decode minimal required size, including padding
        const UInt64 expectedSize = item.Size + padSize;
        UInt64 size = coderPackSize;
        if (item.Size > coderPackSize)
          headersError = true;
        else if (expectedSize != coderPackSize)
        {
          headersError = true;
          if (coderPackSize > expectedSize)
            size = expectedSize;
        }

        result = filterStream->Code(inStream, padStream ?
            padStream.Interface() :
            outStream.Interface(),
            NULL, &size, compressProgress);

        if (outStream->GetSize() != item.Size)
          truncatedError = true;

        if (pkAesMode)
        {
          if (padStreamSpec->GetSize() != size)
            truncatedError = true;
          if (padStreamSpec->WasPadFailure())
            padError = true;
        }
      }
      else
      {
        if (item.IsEncrypted())
        {
          readFromFilter = true;
          inStreamReleaser.FilterCoder = filterStream.ClsPtr();
          RINOK(filterStream->SetInStream(inStream))
          
          /* IFilter::Init() does nothing in all zip crypto filters.
          So we can call any Initialize function in CFilterCoder. */
          
          RINOK(filterStream->Init_NoSubFilterInit())
          // RINOK(filterStream->SetOutStreamSize(NULL));
        }

        try {
        result = coder->Code(readFromFilter ?
              filterStream.Interface() :
              inStream.Interface(),
            outStream,
            isFullStreamExpected ? &coderPackSize : NULL,
            // NULL,
            useUnpackLimit ? &item.Size : NULL,
            compressProgress);
        } catch (...) { return E_FAIL; }

        if (result == S_OK)
        {
        CMyComPtr<ICompressGetInStreamProcessedSize> getInStreamProcessedSize;
        coder->QueryInterface(IID_ICompressGetInStreamProcessedSize, (void **)&getInStreamProcessedSize);
        if (getInStreamProcessedSize && setFinishMode)
        {
          UInt64 processed;
          RINOK(getInStreamProcessedSize->GetInStreamProcessedSize(&processed))
          if (processed != (UInt64)(Int64)-1)
          {
            if (pkAesMode)
            {
              const UInt32 padSize = _pkAesDecoder->GetPadSize((UInt32)processed);
              if (processed + padSize > coderPackSize)
                truncatedError = true;
              else if (processed + padSize < coderPackSize)
                dataAfterEnd = true;
              else
              {
                {
                  // here we check PKCS7 padding data from reminder (it can be inside stream buffer in coder).
                  CMyComPtr<ICompressReadUnusedFromInBuf> readInStream;
                  coder->QueryInterface(IID_ICompressReadUnusedFromInBuf, (void **)&readInStream);
                  // CCopyCoder() for kStore doesn't read data outside of (item.Size)
                  if (readInStream || id == NFileHeader::NCompressionMethod::kStore)
                  {
                    // change pad size, if we support another block size in ZipStrong.
                    // here we request more data to detect error with data after end.
                    const UInt32 kBufSize = NCrypto::NZipStrong::kAesPadAllign + 16;
                    Byte buf[kBufSize];
                    UInt32 processedSize = 0;
                    if (readInStream)
                    {
                      RINOK(readInStream->ReadUnusedFromInBuf(buf, kBufSize, &processedSize))
                    }
                    if (processedSize > padSize)
                      dataAfterEnd = true;
                    else
                    {
                      size_t processedSize2 = kBufSize - processedSize;
                      result = ReadStream(filterStream, buf + processedSize, &processedSize2);
                      if (result == S_OK)
                      {
                        processedSize2 += processedSize;
                        if (processedSize2 > padSize)
                          dataAfterEnd = true;
                        else if (processedSize2 < padSize)
                          truncatedError = true;
                        else
                          for (unsigned i = 0; i < padSize; i++)
                            if (buf[i] != padSize)
                              padError = true;
                      }
                    }
                  }
                }
              }
            }
            else
            {
              if (processed < coderPackSize)
              {
                if (isFullStreamExpected)
                  dataAfterEnd = true;
              }
              else if (processed > coderPackSize)
              {
                // that case is additional check, that can show the bugs in code (coder)
                truncatedError = true;
              }
              needReminderCheck = isFullStreamExpected;
            }
          }
        }
        }
      }

      if (result == S_OK && id == NFileHeader::NCompressionMethod::kLZMA)
        if (!lzmaDecoderSpec->Decoder->CheckFinishStatus(item.IsLzmaEOS()))
          lzmaEosError = true;
    }
    
    if (result == S_FALSE)
      return S_OK;
    
    if (result == E_NOTIMPL)
    {
      res = NExtract::NOperationResult::kUnsupportedMethod;
      return S_OK;
    }

    RINOK(result)
  }

  bool crcOK = true;
  bool authOk = true;
  if (needCRC)
    crcOK = (outStream->GetCRC() == item.Crc);

  if (useUnpackLimit)
    if (outStream->GetSize() != item.Size)
      truncatedError = true;
  
  if (wzAesMode)
  {
    const UInt64 unpackSize = outStream->GetSize();
    const UInt64 packSize = inStream->GetSize();
    bool thereAreData = false;
    // read to the end from filter or from packed stream
    if (SkipStreamData(readFromFilter ?
          filterStream.Interface() :
          inStream.Interface(),
        compressProgress, packSize, unpackSize, thereAreData) != S_OK)
      authOk = false;
    if (needReminderCheck && thereAreData)
      dataAfterEnd = true;

    if (inStream->GetRem() != 0)
      truncatedError = true;
    else
    {
      inStream->Init(NCrypto::NWzAes::kMacSize);
      if (_wzAesDecoder->CheckMac(inStream, authOk) != S_OK)
        authOk = false;
    }
  }

  res = NExtract::NOperationResult::kCRCError;

  if (crcOK && authOk)
  {
    res = NExtract::NOperationResult::kOK;

    if (dataAfterEnd)
      res = NExtract::NOperationResult::kDataAfterEnd;
    else if (padError)
      res = NExtract::NOperationResult::kCRCError;
    else if (truncatedError)
      res = NExtract::NOperationResult::kUnexpectedEnd;
    else if (headersError)
      res = NExtract::NOperationResult::kHeadersError;
    else if (lzmaEosError)
      res = NExtract::NOperationResult::kHeadersError;
    else if (badDescriptor)
      res = NExtract::NOperationResult::kUnexpectedEnd;

    // CheckDescriptor() supports only data descriptor with signature and
    // it doesn't support "old" pkzip's data descriptor without signature.
    // So we disable that check.
    /*
    if (item.HasDescriptor() && archive.CheckDescriptor(item) != S_OK)
      res = NExtract::NOperationResult::kHeadersError;
    */
  }

  return S_OK;
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
  if (allFilesMode)
    numItems = m_Items.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 total = 0; // , totalPacked = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const CItemEx &item = m_Items[allFilesMode ? i : indices[i]];
    total += item.Size;
    // totalPacked += item.PackSize;
  }
  RINOK(extractCallback->SetTotal(total))

  CZipDecoder myDecoder;
  UInt64 cur_Unpacked, cur_Packed;
  
  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, false);

  for (i = 0;; i++,
      lps->OutSize += cur_Unpacked,
      lps->InSize += cur_Packed)
  {
    RINOK(lps->SetCur())
    if (i >= numItems)
      return S_OK;
    const UInt32 index = allFilesMode ? i : indices[i];
    CItemEx item = m_Items[index];
    cur_Unpacked = item.Size;
    cur_Packed = item.PackSize;

    const bool isLocalOffsetOK = m_Archive.IsLocalOffsetOK(item);
    const bool skip = !isLocalOffsetOK && !item.IsDir();
    const Int32 askMode = skip ?
        NExtract::NAskMode::kSkip : testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;

    Int32 opRes;
    {
    CMyComPtr<ISequentialOutStream> realOutStream;
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode))

    if (!isLocalOffsetOK)
    {
      RINOK(extractCallback->PrepareOperation(askMode))
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnavailable))
      continue;
    }

    bool headersError = false;
    
    if (!item.FromLocal)
    {
      bool isAvail = true;
      const HRESULT hres = m_Archive.Read_LocalItem_After_CdItem(item, isAvail, headersError);
      if (hres == S_FALSE)
      {
        if (item.IsDir() || realOutStream || testMode)
        {
          RINOK(extractCallback->PrepareOperation(askMode))
          realOutStream.Release();
          RINOK(extractCallback->SetOperationResult(
              isAvail ?
                NExtract::NOperationResult::kHeadersError :
                NExtract::NOperationResult::kUnavailable))
        }
        continue;
      }
      RINOK(hres)
    }

    if (item.IsDir())
    {
      // if (!testMode)
      {
        RINOK(extractCallback->PrepareOperation(askMode))
        realOutStream.Release();
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
      }
      continue;
    }

    if (!testMode && !realOutStream)
      continue;

    RINOK(extractCallback->PrepareOperation(askMode))

    const HRESULT hres = myDecoder.Decode(
        EXTERNAL_CODECS_VARS
        m_Archive, item, realOutStream, extractCallback,
        lps,
        #ifndef Z7_ST
        _props._numThreads, _props._memUsage_Decompress,
        #endif
        opRes);
    
    RINOK(hres)
    // realOutStream.Release();
    
    if (opRes == NExtract::NOperationResult::kOK && headersError)
      opRes = NExtract::NOperationResult::kHeadersError;
    }
    RINOK(extractCallback->SetOperationResult(opRes))
  }

  COM_TRY_END
}

IMPL_ISetCompressCodecsInfo

}}

/* ================ unit: CPP/7zip/Archive/Zip/ZipHandlerOut.cpp ================ */
// ZipHandlerOut.cpp

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
using namespace NCOM;
using namespace NTime;

namespace NArchive {
namespace NZip {

Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *timeType))
{
  *timeType = TimeOptions.Prec;
  return S_OK;
}

static bool IsSimpleAsciiString(const wchar_t *s)
{
  for (;;)
  {
    wchar_t c = *s++;
    if (c == 0)
      return true;
    if (c < 0x20 || c > 0x7F)
      return false;
  }
}


static int FindZipMethod(const char *s, const char * const *names, unsigned num)
{
  for (unsigned i = 0; i < num; i++)
  {
    const char *name = names[i];
    if (name && StringsAreEqualNoCase_Ascii(s, name))
      return (int)i;
  }
  return -1;
}

static int FindZipMethod(const char *s)
{
  int k = FindZipMethod(s, kMethodNames1, kNumMethodNames1);
  if (k >= 0)
    return k;
  k = FindZipMethod(s, kMethodNames2, kNumMethodNames2);
  if (k >= 0)
    return (int)kMethodNames2Start + k;
  return -1;
}


#define COM_TRY_BEGIN2 try {
#define COM_TRY_END2 } \
catch(const CSystemException &e) { return e.ErrorCode; } \
catch(...) { return E_OUTOFMEMORY; }

static HRESULT GetTime(IArchiveUpdateCallback *callback, unsigned index, PROPID propID, FILETIME &filetime)
{
  filetime.dwHighDateTime = filetime.dwLowDateTime = 0;
  NCOM::CPropVariant prop;
  RINOK(callback->GetProperty(index, propID, &prop))
  if (prop.vt == VT_FILETIME)
    filetime = prop.filetime;
  else if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  return S_OK;
}


Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *callback))
{
  COM_TRY_BEGIN2
  
  if (m_Archive.IsOpen())
  {
    if (!m_Archive.CanUpdate())
      return E_NOTIMPL;
  }

  CObjectVector<CUpdateItem> updateItems;
  updateItems.ClearAndReserve(numItems);

  bool thereAreAesUpdates = false;
  UInt64 largestSize = 0;
  bool largestSizeDefined = false;

  #ifdef _WIN32
  const UINT oemCP = GetOEMCP();
  #endif

  UString name;
  CUpdateItem ui;

  for (UInt32 i = 0; i < numItems; i++)
  {
    Int32 newData;
    Int32 newProps;
    UInt32 indexInArc;
    
    if (!callback)
      return E_FAIL;
    
    RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &indexInArc))
    
    name.Empty();
    ui.Clear();

    ui.NewProps = IntToBool(newProps);
    ui.NewData = IntToBool(newData);
    ui.IndexInArc = (int)indexInArc;
    ui.IndexInClient = i;
    
    bool existInArchive = (indexInArc != (UInt32)(Int32)-1);
    if (existInArchive)
    {
      const CItemEx &inputItem = m_Items[indexInArc];
      if (inputItem.IsAesEncrypted())
        thereAreAesUpdates = true;
      if (!IntToBool(newProps))
        ui.IsDir = inputItem.IsDir();
      // ui.IsAltStream = inputItem.IsAltStream();
    }

    if (IntToBool(newProps))
    {
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidAttrib, &prop))
        if (prop.vt == VT_EMPTY)
          ui.Attrib = 0;
        else if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        else
          ui.Attrib = prop.ulVal;
      }

      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidPath, &prop))
        if (prop.vt == VT_EMPTY)
        {
          // name.Empty();
        }
        else if (prop.vt != VT_BSTR)
          return E_INVALIDARG;
        else
          name = prop.bstrVal;
      }

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

      /*
      {
        bool isAltStream = false;
        {
          NCOM::CPropVariant prop;
          RINOK(callback->GetProperty(i, kpidIsAltStream, &prop));
          if (prop.vt == VT_BOOL)
            isAltStream = (prop.boolVal != VARIANT_FALSE);
          else if (prop.vt != VT_EMPTY)
            return E_INVALIDARG;
        }
      
        if (isAltStream)
        {
          if (ui.IsDir)
            return E_INVALIDARG;
          int delim = name.ReverseFind(L':');
          if (delim >= 0)
          {
            name.Delete(delim, 1);
            name.Insert(delim, UString(k_SpecName_NTFS_STREAM));
            ui.IsAltStream = true;
          }
        }
      }
      */

      // 22.00 : kpidTimeType is useless here : the code was disabled
      /*
      {
        CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidTimeType, &prop));
        if (prop.vt == VT_UI4)
          ui.NtfsTime_IsDefined = (prop.ulVal == NFileTimeType::kWindows);
        else
          ui.NtfsTime_IsDefined = _Write_NtfsTime;
      }
      */

      if (TimeOptions.Write_MTime.Val) RINOK (GetTime (callback, i, kpidMTime, ui.Ntfs_MTime))
      if (TimeOptions.Write_ATime.Val) RINOK (GetTime (callback, i, kpidATime, ui.Ntfs_ATime))
      if (TimeOptions.Write_CTime.Val) RINOK (GetTime (callback, i, kpidCTime, ui.Ntfs_CTime))

      if (TimeOptions.Prec != k_PropVar_TimePrec_DOS)
      {
        if (TimeOptions.Prec == k_PropVar_TimePrec_Unix ||
            TimeOptions.Prec == k_PropVar_TimePrec_Base)
          ui.Write_UnixTime = ! FILETIME_IsZero (ui.Ntfs_MTime);
        else
        {
          /*
          // if we want to store zero timestamps as zero timestamp, use the following:
            ui.Write_NtfsTime =
            _Write_MTime ||
            _Write_ATime ||
            _Write_CTime;
          */
          
          // We treat zero timestamp as no timestamp
          ui.Write_NtfsTime =
            ! FILETIME_IsZero (ui.Ntfs_MTime) ||
            ! FILETIME_IsZero (ui.Ntfs_ATime) ||
            ! FILETIME_IsZero (ui.Ntfs_CTime);
        }
      }

      /*
        how 0 in dos time works:
            win10 explorer extract : some random date 1601-04-25.
            winrar 6.10 : write time.
            7zip : MTime of archive is used
          how 0 in tar works:
            winrar 6.10 : 1970
        0 in dos field can show that there is no timestamp.
        we write correct 1970-01-01 in dos field, to support correct extraction in Win10.
      */

      UtcFileTime_To_LocalDosTime(ui.Ntfs_MTime, ui.Time);

      NItemName::ReplaceSlashes_OsToUnix(name);
      
      bool needSlash = ui.IsDir;
      const wchar_t kSlash = L'/';
      if (!name.IsEmpty())
      {
        if (name.Back() == kSlash)
        {
          if (!ui.IsDir)
            return E_INVALIDARG;
          needSlash = false;
        }
      }
      if (needSlash)
        name += kSlash;

      const UINT codePage = _forceCodePage ? _specifiedCodePage : CP_OEMCP;
      bool tryUtf8 = true;

      /*
        Windows 10 allows users to set UTF-8 in Region Settings via option:
        "Beta: Use Unicode UTF-8 for worldwide language support"
        In that case Windows uses CP_UTF8 when we use CP_OEMCP.
        21.02 fixed:
          we set UTF-8 mark for non-latin files for such UTF-8 mode in Windows.
          we write additional Info-Zip Utf-8 FileName Extra for non-latin names/
      */

      if ((codePage != CP_UTF8) &&
        #ifdef _WIN32
          (m_ForceLocal || !m_ForceUtf8) && (oemCP != CP_UTF8)
        #else
          (m_ForceLocal && !m_ForceUtf8)
        #endif
        )
      {
        bool defaultCharWasUsed;
        ui.Name = UnicodeStringToMultiByte(name, codePage, '_', defaultCharWasUsed);
        tryUtf8 = (!m_ForceLocal && (defaultCharWasUsed ||
          MultiByteToUnicodeString(ui.Name, codePage) != name));
      }

      const bool isNonLatin = !name.IsAscii();

      if (tryUtf8)
      {
        ui.IsUtf8 = isNonLatin;
        ConvertUnicodeToUTF8(name, ui.Name);

        #ifndef _WIN32
        if (ui.IsUtf8 && !CheckUTF8_AString(ui.Name))
        {
          // if it's non-Windows and there are non-UTF8 characters we clear UTF8-flag
          ui.IsUtf8 = false;
        }
        #endif
      }
      else if (isNonLatin)
        Convert_Unicode_To_UTF8_Buf(name, ui.Name_Utf);

      if (ui.Name.Len() >= (1 << 16)
          || ui.Name_Utf.Size() >= (1 << 16) - 128)
        return E_INVALIDARG;

      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidComment, &prop))
        if (prop.vt == VT_EMPTY)
        {
          // ui.Comment.Free();
        }
        else if (prop.vt != VT_BSTR)
          return E_INVALIDARG;
        else
        {
          UString s = prop.bstrVal;
          AString a;
          if (ui.IsUtf8)
            ConvertUnicodeToUTF8(s, a);
          else
          {
            bool defaultCharWasUsed;
            a = UnicodeStringToMultiByte(s, codePage, '_', defaultCharWasUsed);
          }
          if (a.Len() >= (1 << 16))
            return E_INVALIDARG;
          ui.Comment.CopyFrom((const Byte *)(const char *)a, a.Len());
        }
      }


      /*
      if (existInArchive)
      {
        const CItemEx &itemInfo = m_Items[indexInArc];
        // ui.Commented = itemInfo.IsCommented();
        ui.Commented = false;
        if (ui.Commented)
        {
          ui.CommentRange.Position = itemInfo.GetCommentPosition();
          ui.CommentRange.Size  = itemInfo.CommentSize;
        }
      }
      else
        ui.Commented = false;
      */
    }
    
    
    if (IntToBool(newData))
    {
      UInt64 size = 0;
      if (!ui.IsDir)
      {
        NCOM::CPropVariant prop;
        RINOK(callback->GetProperty(i, kpidSize, &prop))
        if (prop.vt != VT_UI8)
          return E_INVALIDARG;
        size = prop.uhVal.QuadPart;
        if (largestSize < size)
          largestSize = size;
        largestSizeDefined = true;
      }
      ui.Size = size;
    }

    updateItems.Add(ui);
  }


  CMyComPtr<ICryptoGetTextPassword2> getTextPassword;
  {
    CMyComPtr<IArchiveUpdateCallback> udateCallBack2(callback);
    udateCallBack2.QueryInterface(IID_ICryptoGetTextPassword2, &getTextPassword);
  }
  CCompressionMethodMode options;
  (CBaseProps &)options = _props;
  options.DataSizeReduce = largestSize;
  options.DataSizeReduce_Defined = largestSizeDefined;

  options.Password_Defined = false;
  options.Password.Wipe_and_Empty();
  if (getTextPassword)
  {
    CMyComBSTR_Wipe password;
    Int32 passwordIsDefined;
    RINOK(getTextPassword->CryptoGetTextPassword2(&passwordIsDefined, &password))
    options.Password_Defined = IntToBool(passwordIsDefined);
    if (options.Password_Defined)
    {
      if (!m_ForceAesMode)
        options.IsAesMode = thereAreAesUpdates;

      if (!IsSimpleAsciiString(password))
        return E_INVALIDARG;
      if (password)
        UnicodeStringToMultiByte2(options.Password, (LPCOLESTR)password, CP_OEMCP);
      if (options.IsAesMode)
      {
        if (options.Password.Len() > NCrypto::NWzAes::kPasswordSizeMax)
          return E_INVALIDARG;
      }
    }
  }

  
  int mainMethod = m_MainMethod;
  
  if (mainMethod < 0)
  {
    if (!_props._methods.IsEmpty())
    {
      const AString &methodName = _props._methods.Front().MethodName;
      if (!methodName.IsEmpty())
      {
        mainMethod = FindZipMethod(methodName);
        if (mainMethod < 0)
        {
          CMethodId methodId;
          UInt32 numStreams;
          bool isFilter;
          if (FindMethod_Index(EXTERNAL_CODECS_VARS methodName, true,
              methodId, numStreams, isFilter) < 0)
            return E_NOTIMPL;
          if (numStreams != 1)
            return E_NOTIMPL;
          if (methodId == kMethodId_BZip2)
            mainMethod = NFileHeader::NCompressionMethod::kBZip2;
          else
          {
            if (methodId < kMethodId_ZipBase)
              return E_NOTIMPL;
            methodId -= kMethodId_ZipBase;
            if (methodId > 0xFF)
              return E_NOTIMPL;
            mainMethod = (int)methodId;
          }
        }
      }
    }
  }

  if (mainMethod < 0)
    mainMethod = (Byte)(((_props.GetLevel() == 0) ?
        NFileHeader::NCompressionMethod::kStore :
        NFileHeader::NCompressionMethod::kDeflate));
  else
    mainMethod = (Byte)mainMethod;
  
  options.MethodSequence.Add((Byte)mainMethod);
  
  if (mainMethod != NFileHeader::NCompressionMethod::kStore)
    options.MethodSequence.Add(NFileHeader::NCompressionMethod::kStore);

  options.Force_SeqOutMode = _force_SeqOutMode;

  CUpdateOptions uo;
  uo.Write_MTime = TimeOptions.Write_MTime.Val;
  uo.Write_ATime = TimeOptions.Write_ATime.Val;
  uo.Write_CTime = TimeOptions.Write_CTime.Val;
  /*
  uo.Write_NtfsTime = _Write_NtfsTime &&
    (_Write_MTime || _Write_ATime  || _Write_CTime);
  uo.Write_UnixTime = _Write_UnixTime;
  */

  return Update(
      EXTERNAL_CODECS_VARS
      m_Items, updateItems, outStream,
      m_Archive.IsOpen() ? &m_Archive : NULL, _removeSfxBlock,
      uo, options, callback);
 
  COM_TRY_END2
}



Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  InitMethodProps();
  
  for (UInt32 i = 0; i < numProps; i++)
  {
    UString name = names[i];
    name.MakeLower_Ascii();
    if (name.IsEmpty())
      return E_INVALIDARG;

    const PROPVARIANT &prop = values[i];

    if (name.IsEqualTo_Ascii_NoCase("em"))
    {
      if (prop.vt != VT_BSTR)
        return E_INVALIDARG;
      {
        const wchar_t *m = prop.bstrVal;
        if (IsString1PrefixedByString2_NoCase_Ascii(m, "AES"))
        {
          m += 3;
          UInt32 v = 3;
          if (*m != 0)
          {
            if (*m == '-')
              m++;
            const wchar_t *end;
            v = ConvertStringToUInt32(m,  &end);
            if (*end != 0 || v % 64 != 0)
              return E_INVALIDARG;
            v /= 64;
            v -= 2;
            if (v >= 3)
              return E_INVALIDARG;
            v++;
          }
          _props.AesKeyMode = (Byte)v;
          _props.IsAesMode = true;
          m_ForceAesMode = true;
        }
        else if (StringsAreEqualNoCase_Ascii(m, "ZipCrypto"))
        {
          _props.IsAesMode = false;
          m_ForceAesMode = true;
        }
        else
          return E_INVALIDARG;
      }
    }
    

   
    else if (name.IsEqualTo("cl"))
    {
      RINOK(PROPVARIANT_to_bool(prop, m_ForceLocal))
      if (m_ForceLocal)
        m_ForceUtf8 = false;
    }
    else if (name.IsEqualTo("cu"))
    {
      RINOK(PROPVARIANT_to_bool(prop, m_ForceUtf8))
      if (m_ForceUtf8)
        m_ForceLocal = false;
    }
    else if (name.IsEqualTo("cp"))
    {
      UInt32 cp = CP_OEMCP;
      RINOK(ParsePropToUInt32(L"", prop, cp))
      _forceCodePage = true;
      _specifiedCodePage = cp;
    }
    else if (name.IsEqualTo("rsfx"))
    {
      RINOK(PROPVARIANT_to_bool(prop, _removeSfxBlock))
    }
    else if (name.IsEqualTo("rws"))
    {
      RINOK(PROPVARIANT_to_bool(prop, _force_SeqOutMode))
    }
    else if (name.IsEqualTo("ros"))
    {
      RINOK(PROPVARIANT_to_bool(prop, _force_OpenSeq))
    }
    else
    {
      if (name.IsEqualTo_Ascii_NoCase("m") && prop.vt == VT_UI4)
      {
        UInt32 id = prop.ulVal;
        if (id > 0xFF)
          return E_INVALIDARG;
        m_MainMethod = (int)id;
      }
      else
      {
        bool processed = false;
        RINOK(TimeOptions.Parse(name, prop, processed))
        if (!processed)
        {
          RINOK(_props.SetProperty(name, prop))
        }
      }
      // RINOK(_props.MethodInfo.ParseParamsFromPROPVARIANT(name, prop));
    }
  }

  _props._methods.DeleteFrontal(_props.GetNumEmptyMethods());
  if (_props._methods.Size() > 1)
    return E_INVALIDARG;
  if (_props._methods.Size() == 1)
  {
    const AString &methodName = _props._methods[0].MethodName;

    if (!methodName.IsEmpty())
    {
      const char *end;
      UInt32 id = ConvertStringToUInt32(methodName, &end);
      if (*end == 0 && id <= 0xFF)
        m_MainMethod = (int)id;
      else if (methodName.IsEqualTo_Ascii_NoCase("Copy")) // it's alias for "Store"
        m_MainMethod = 0;
    }
  }
  
  return S_OK;
}

}}
