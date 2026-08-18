/* XArchive amalgamation of 7-Zip 26.01 -- Common hasher registration.
 *
 * 1 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/Common/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

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

/* ---- C/XzCrc64.h ---- */
/* XzCrc64.h -- CRC64 calculation
2023-12-08 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_XZ_CRC64_H
#define ZIP7_INC_XZ_CRC64_H

#include <stddef.h>

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

// extern UInt64 g_Crc64Table[];

void Z7_FASTCALL Crc64GenerateTable(void);

#define CRC64_INIT_VAL UINT64_CONST(0xFFFFFFFFFFFFFFFF)
#define CRC64_GET_DIGEST(crc) ((crc) ^ CRC64_INIT_VAL)
// #define CRC64_UPDATE_BYTE(crc, b) (g_Crc64Table[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

UInt64 Z7_FASTCALL Crc64Update(UInt64 crc, const void *data, size_t size);
// UInt64 Z7_FASTCALL Crc64Calc(const void *data, size_t size);

EXTERN_C_END

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

/* ---- CPP/7zip/Common/MethodId.h ---- */
// MethodId.h

#ifndef ZIP7_INC_7Z_METHOD_ID_H
#define ZIP7_INC_7Z_METHOD_ID_H

// amalgamation: header emitted in prologue

typedef UInt64 CMethodId;

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

/* ================ unit bodies ================ */

/* ================ unit: CPP/Common/XzCrc64Reg.cpp ================ */
// XzCrc64Reg.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

Z7_CLASS_IMP_COM_1(
  CXzCrc64Hasher
  , IHasher
)
  UInt64 _crc;
public:
  Byte _mtDummy[1 << 7];  // it's public to eliminate clang warning: unused private field

  CXzCrc64Hasher(): _crc(CRC64_INIT_VAL) {}
};

Z7_COM7F_IMF2(void, CXzCrc64Hasher::Init())
{
  _crc = CRC64_INIT_VAL;
}

Z7_COM7F_IMF2(void, CXzCrc64Hasher::Update(const void *data, UInt32 size))
{
  _crc = Crc64Update(_crc, data, size);
}

Z7_COM7F_IMF2(void, CXzCrc64Hasher::Final(Byte *digest))
{
  const UInt64 val = CRC64_GET_DIGEST(_crc);
  SetUi64(digest, val)
}

REGISTER_HASHER(CXzCrc64Hasher, 0x4, "CRC64", 8)
