/* XArchive amalgamation of 7-Zip 26.01 -- CPP/Common non-registering + CPP/Windows.
 *
 * 28 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/Common/MyMap.h ---- */
// MyMap.h

#ifndef ZIP7_INC_COMMON_MY_MAP_H
#define ZIP7_INC_COMMON_MY_MAP_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

class CMap32
{
  struct CNode
  {
    UInt32 Key;
    UInt32 Keys[2];
    UInt32 Values[2];
    UInt16 Len;
    Byte IsLeaf[2];
  };
  CRecordVector<CNode> Nodes;

public:

  void Clear() { Nodes.Clear(); }
  bool Find(UInt32 key, UInt32 &valueRes) const throw();
  bool Set(UInt32 key, UInt32 value); // returns true, if there is such key already
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

/* ---- CPP/Windows/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

#endif

/* ---- CPP/Common/C_FileIO.h ---- */
// Common/C_FileIO.h

#ifndef ZIP7_INC_COMMON_C_FILEIO_H
#define ZIP7_INC_COMMON_C_FILEIO_H

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

/* ---- CPP/Windows/FileFind.h ---- */
// Windows/FileFind.h

#ifndef ZIP7_INC_WINDOWS_FILE_FIND_H
#define ZIP7_INC_WINDOWS_FILE_FIND_H

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NWindows {
namespace NFile {
namespace NFind {

// bool DoesFileExist(CFSTR name, bool followLink);
bool DoesFileExist_Raw(CFSTR name);
bool DoesFileExist_FollowLink(CFSTR name);
bool DoesDirExist(CFSTR name, bool followLink);

inline bool DoesDirExist(CFSTR name)
  { return DoesDirExist(name, false); }
inline bool DoesDirExist_FollowLink(CFSTR name)
  { return DoesDirExist(name, true); }

// it's always _Raw
bool DoesFileOrDirExist(CFSTR name);

DWORD GetFileAttrib(CFSTR path);

#ifdef _WIN32

namespace NAttributes
{
  inline bool IsReadOnly(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_READONLY) != 0; }
  inline bool IsHidden(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_HIDDEN) != 0; }
  inline bool IsSystem(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_SYSTEM) != 0; }
  inline bool IsDir(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0; }
  inline bool IsArchived(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_ARCHIVE) != 0; }
  inline bool IsCompressed(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_COMPRESSED) != 0; }
  inline bool IsEncrypted(DWORD attrib) { return (attrib & FILE_ATTRIBUTE_ENCRYPTED) != 0; }

  inline UInt32 Get_PosixMode_From_WinAttrib(DWORD attrib)
  {
    UInt32 v = IsDir(attrib) ? MY_LIN_S_IFDIR : MY_LIN_S_IFREG;
    /* 21.06: as WSL we allow write permissions (0222) for directories even for (FILE_ATTRIBUTE_READONLY).
    So extracting at Linux will be allowed to write files inside (0777) directories. */
    v |= ((IsReadOnly(attrib) && !IsDir(attrib)) ? 0555 : 0777);
    return v;
  }
}

#else

UInt32 Get_WinAttribPosix_From_PosixMode(UInt32 mode);

#endif

class CFileInfoBase
{
 #ifdef _WIN32
  bool MatchesMask(UINT32 mask) const { return ((Attrib & mask) != 0); }
 #endif
public:
  UInt64 Size;
  CFiTime CTime;
  CFiTime ATime;
  CFiTime MTime;
 #ifdef _WIN32
  DWORD Attrib;
  bool IsAltStream;
  bool IsDevice;

  /*
  #ifdef UNDER_CE
  DWORD ObjectID;
  #else
  UINT32 ReparseTag;
  #endif
  */
 #else
  dev_t dev;     /* ID of device containing file */
  ino_t ino;
  mode_t mode;
  nlink_t nlink;
  uid_t uid;     /* user ID of owner */
  gid_t gid;     /* group ID of owner */
  dev_t rdev;    /* device ID (defined, if S_ISCHR(mode) || S_ISBLK(mode)) */
  // bool Use_lstat;
 #endif

  CFileInfoBase() { ClearBase(); }
  void ClearBase() throw();
  bool SetAs_StdInFile();

 #ifdef _WIN32
 
  bool Fill_From_ByHandleFileInfo(CFSTR path);
  void SetAsDir()  { Attrib = FILE_ATTRIBUTE_DIRECTORY; } // |= (FILE_ATTRIBUTE_UNIX_EXTENSION + (S_IFDIR << 16));

  bool IsArchived() const { return MatchesMask(FILE_ATTRIBUTE_ARCHIVE); }
  bool IsCompressed() const { return MatchesMask(FILE_ATTRIBUTE_COMPRESSED); }
  bool IsDir() const { return MatchesMask(FILE_ATTRIBUTE_DIRECTORY); }
  bool IsEncrypted() const { return MatchesMask(FILE_ATTRIBUTE_ENCRYPTED); }
  bool IsHidden() const { return MatchesMask(FILE_ATTRIBUTE_HIDDEN); }
  bool IsNormal() const { return MatchesMask(FILE_ATTRIBUTE_NORMAL); }
  bool IsOffline() const { return MatchesMask(FILE_ATTRIBUTE_OFFLINE); }
  bool IsReadOnly() const { return MatchesMask(FILE_ATTRIBUTE_READONLY); }
  bool HasReparsePoint() const { return MatchesMask(FILE_ATTRIBUTE_REPARSE_POINT); }
  bool IsSparse() const { return MatchesMask(FILE_ATTRIBUTE_SPARSE_FILE); }
  bool IsSystem() const { return MatchesMask(FILE_ATTRIBUTE_SYSTEM); }
  bool IsTemporary() const { return MatchesMask(FILE_ATTRIBUTE_TEMPORARY); }

  UInt32 GetWinAttrib() const { return Attrib; }
  UInt32 GetPosixAttrib() const
  {
    return NAttributes::Get_PosixMode_From_WinAttrib(Attrib);
  }
  bool Has_Attrib_ReparsePoint() const { return (Attrib & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
 
 #else
  
  UInt32 GetPosixAttrib() const { return mode; }
  UInt32 GetWinAttrib() const { return Get_WinAttribPosix_From_PosixMode(mode); }

  bool IsDir() const { return S_ISDIR(mode); }
  void SetAsDir()  { mode = S_IFDIR | 0777; }
  void SetFrom_stat(const struct stat &st);

  bool IsReadOnly() const
  {
    // does linux support writing to ReadOnly files?
    if ((mode & 0222) == 0) // S_IWUSR in p7zip
      return true;
    return false;
  }
 
  bool IsPosixLink() const { return S_ISLNK(mode); }

 #endif

  bool IsOsSymLink() const
  {
    #ifdef _WIN32
      return HasReparsePoint();
    #else
      return IsPosixLink();
    #endif
  }
};

struct CFileInfo: public CFileInfoBase
{
  FString Name;
  #if defined(_WIN32) && !defined(UNDER_CE)
  // FString ShortName;
  #endif

  bool IsDots() const throw();
  bool Find(CFSTR path, bool followLink = false);
  bool Find_FollowLink(CFSTR path) { return Find(path, true); }

  #ifdef _WIN32
  // bool Fill_From_ByHandleFileInfo(CFSTR path);
  // bool FollowReparse(CFSTR path, bool isDir);
  #else
  bool Find_DontFill_Name(CFSTR path, bool followLink = false);
  #endif
};


#ifdef _WIN32

class CFindFileBase  MY_UNCOPYABLE
{
protected:
  HANDLE _handle;
public:
  bool IsHandleAllocated() const { return _handle != INVALID_HANDLE_VALUE; }
  CFindFileBase(): _handle(INVALID_HANDLE_VALUE) {}
  ~CFindFileBase() { Close(); }
  bool Close() throw();
};

class CFindFile: public CFindFileBase
{
public:
  bool FindFirst(CFSTR wildcard, CFileInfo &fileInfo);
  bool FindNext(CFileInfo &fileInfo);
};

#if defined(_WIN32) && !defined(UNDER_CE)

struct CStreamInfo
{
  UString Name;
  UInt64 Size;

  UString GetReducedName() const; // returns ":Name"
  // UString GetReducedName2() const; // returns "Name"
  bool IsMainStream() const throw();
};

class CFindStream: public CFindFileBase
{
public:
  bool FindFirst(CFSTR filePath, CStreamInfo &streamInfo);
  bool FindNext(CStreamInfo &streamInfo);
};

class CStreamEnumerator  MY_UNCOPYABLE
{
  CFindStream _find;
  FString _filePath;

  bool NextAny(CFileInfo &fileInfo, bool &found);
public:
  CStreamEnumerator(const FString &filePath): _filePath(filePath) {}
  bool Next(CStreamInfo &streamInfo, bool &found);
};

#endif // defined(_WIN32) && !defined(UNDER_CE)


class CEnumerator  MY_UNCOPYABLE
{
  CFindFile _findFile;
  FString _wildcard;

  bool NextAny(CFileInfo &fileInfo);
public:
  void SetDirPrefix(const FString &dirPrefix);
  bool Next(CFileInfo &fileInfo);
  bool Next(CFileInfo &fileInfo, bool &found);
};


class CFindChangeNotification  MY_UNCOPYABLE
{
  HANDLE _handle;
public:
  operator HANDLE () { return _handle; }
  bool IsHandleAllocated() const
  {
    /* at least on win2000/XP (undocumented):
       if pathName is "" or NULL,
       FindFirstChangeNotification() could return NULL.
       So we check for INVALID_HANDLE_VALUE and NULL.
    */
    return _handle != INVALID_HANDLE_VALUE && _handle != NULL;
  }
  CFindChangeNotification(): _handle(INVALID_HANDLE_VALUE) {}
  ~CFindChangeNotification() { Close(); }
  bool Close() throw();
  HANDLE FindFirst(CFSTR pathName, bool watchSubtree, DWORD notifyFilter);
  bool FindNext() { return BOOLToBool(::FindNextChangeNotification(_handle)); }
};

#ifndef UNDER_CE
bool MyGetLogicalDriveStrings(CObjectVector<FString> &driveStrings);
#endif

typedef CFileInfo CDirEntry;


#else // WIN32


struct CDirEntry
{
  ino_t iNode;
#if !defined(_AIX) && !defined(__sun) && !defined(__QNXNTO__)
  Byte Type;
#endif
  FString Name;

  /*
#if !defined(_AIX) && !defined(__sun) && !defined(__QNXNTO__)
  bool IsDir() const
  {
    // (Type == DT_UNKNOWN) on some systems
    return Type == DT_DIR;
  }
#endif
  */

  bool IsDots() const throw();
};

class CEnumerator  MY_UNCOPYABLE
{
  DIR *_dir;
  FString _wildcard;

  bool NextAny(CDirEntry &fileInfo, bool &found);
public:
  CEnumerator(): _dir(NULL) {}
  ~CEnumerator();
  void SetDirPrefix(const FString &dirPrefix);

  bool Next(CDirEntry &fileInfo, bool &found);
  bool Fill_FileInfo(const CDirEntry &de, CFileInfo &fileInfo, bool followLink) const;
  bool DirEntry_IsDir(const CDirEntry &de, bool followLink) const
  {
#if !defined(_AIX) && !defined(__sun) && !defined(__QNXNTO__)
    if (de.Type == DT_DIR)
      return true;
    if (de.Type != DT_UNKNOWN)
      return false;
#endif
    CFileInfo fileInfo;
    if (Fill_FileInfo(de, fileInfo, followLink))
    {
      return fileInfo.IsDir();
    }
    return false; // change it
  }
};

/*
inline UInt32 Get_WinAttrib_From_PosixMode(UInt32 mode)
{
  UInt32 attrib = S_ISDIR(mode) ?
    FILE_ATTRIBUTE_DIRECTORY :
    FILE_ATTRIBUTE_ARCHIVE;
  if ((st.st_mode & 0222) == 0) // check it !!!
    attrib |= FILE_ATTRIBUTE_READONLY;
  return attrib;
}
*/

// UInt32 Get_WinAttrib_From_stat(const struct stat &st);


#endif // WIN32

}}}

#endif

/* ---- CPP/Windows/FileName.h ---- */
// Windows/FileName.h

#ifndef ZIP7_INC_WINDOWS_FILE_NAME_H
#define ZIP7_INC_WINDOWS_FILE_NAME_H

// amalgamation: header emitted in prologue

namespace NWindows {
namespace NFile {
namespace NName {

int FindSepar(const wchar_t *s) throw();
#ifndef USE_UNICODE_FSTRING
int FindSepar(const FChar *s) throw();
#endif

void NormalizeDirPathPrefix(FString &dirPath); // ensures that it ended with '\\', if dirPath is not epmty
void NormalizeDirPathPrefix(UString &dirPath);

#ifdef _WIN32
void NormalizeDirSeparators(FString &s);
#endif

bool IsDrivePath(const wchar_t *s) throw();  // first 3 chars are drive chars like "a:\\"

bool IsAltPathPrefix(CFSTR s) throw(); /* name: */

extern const char * const kSuperPathPrefix; /* \\?\ */
const unsigned kDevicePathPrefixSize = 4;
const unsigned kSuperPathPrefixSize = 4;
const unsigned kSuperUncPathPrefixSize = kSuperPathPrefixSize + 4;

#if defined(_WIN32) && !defined(UNDER_CE)

bool IsDevicePath(CFSTR s) throw();   /* \\.\ */
bool IsSuperUncPath(CFSTR s) throw(); /* \\?\UNC\ */
bool IsNetworkPath(CFSTR s) throw();  /* \\?\UNC\ or \\SERVER */

/* GetNetworkServerPrefixSize() returns size of server prefix:
     \\?\UNC\SERVER\
     \\SERVER\
  in another cases it returns 0
*/

unsigned GetNetworkServerPrefixSize(CFSTR s) throw();

bool IsNetworkShareRootPath(CFSTR s) throw();  /* \\?\UNC\SERVER\share or \\SERVER\share or with slash */

bool IsDrivePath_SuperAllowed(CFSTR s) throw();  // first chars are drive chars like "a:\" or "\\?\a:\"
bool IsDriveRootPath_SuperAllowed(CFSTR s) throw();  // exact drive root path "a:\" or "\\?\a:\"

bool IsDrivePath2(const wchar_t *s) throw(); // first 2 chars are drive chars like "a:"
// bool IsDriveName2(const wchar_t *s) throw(); // is drive name like "a:"
bool IsSuperPath(const wchar_t *s) throw();
bool IsSuperOrDevicePath(const wchar_t *s) throw();

bool IsAltStreamPrefixWithColon(const UString &s) throw();
// returns true, if super prefix was removed
bool If_IsSuperPath_RemoveSuperPrefix(UString &s);

#ifndef USE_UNICODE_FSTRING
bool IsDrivePath2(CFSTR s) throw(); // first 2 chars are drive chars like "a:"
// bool IsDriveName2(CFSTR s) throw(); // is drive name like "a:"
bool IsDrivePath(CFSTR s) throw();
bool IsSuperPath(CFSTR s) throw();
bool IsSuperOrDevicePath(CFSTR s) throw();

/* GetRootPrefixSize() returns size of ROOT PREFIX for cases:
     \
     \\.\
     C:\
     \\?\C:\
     \\?\UNC\SERVER\Shared\
     \\SERVER\Shared\
  in another cases it returns 0
*/

unsigned GetRootPrefixSize(CFSTR s) throw();

#endif

int FindAltStreamColon(CFSTR path) throw();

#endif // _WIN32

bool IsAbsolutePath(const wchar_t *s) throw();
unsigned GetRootPrefixSize(const wchar_t *s) throw();

#ifndef _WIN32
/* GetRootPrefixSize_WINDOWS() is called in linux, but it parses path by windows rules.
   It supports only paths system (linux) slash separators (STRING_PATH_SEPARATOR),
   It doesn't parses paths with backslash (windows) separators.
   "c:/dir/file" is supported.
*/
unsigned GetRootPrefixSize_WINDOWS(const wchar_t *s) throw();
#endif

#ifdef Z7_LONG_PATH

const int kSuperPathType_UseOnlyMain = 0;
const int kSuperPathType_UseOnlySuper = 1;
const int kSuperPathType_UseMainAndSuper = 2;

int GetUseSuperPathType(CFSTR s) throw();
bool GetSuperPath(CFSTR path, UString &superPath, bool onlyIfNew);
bool GetSuperPaths(CFSTR s1, CFSTR s2, UString &d1, UString &d2, bool onlyIfNew);

#define USE_MAIN_PATH (_useSuperPathType != kSuperPathType_UseOnlySuper)
#define USE_MAIN_PATH_2 (_useSuperPathType1 != kSuperPathType_UseOnlySuper && _useSuperPathType2 != kSuperPathType_UseOnlySuper)

#define USE_SUPER_PATH (_useSuperPathType != kSuperPathType_UseOnlyMain)
#define USE_SUPER_PATH_2 (_useSuperPathType1 != kSuperPathType_UseOnlyMain || _useSuperPathType2 != kSuperPathType_UseOnlyMain)

#define IF_USE_MAIN_PATH int _useSuperPathType = GetUseSuperPathType(path); if (USE_MAIN_PATH)
#define IF_USE_MAIN_PATH_2(x1, x2) \
    int _useSuperPathType1 = GetUseSuperPathType(x1); \
    int _useSuperPathType2 = GetUseSuperPathType(x2); \
    if (USE_MAIN_PATH_2)

#else

#define IF_USE_MAIN_PATH
#define IF_USE_MAIN_PATH_2(x1, x2)

#endif // Z7_LONG_PATH

/*
  if (dirPrefix != NULL && (path) is relative)
  {
    (dirPrefix) will be used
    result (fullPath) will contain prefix part of (dirPrefix).
  }
  Current_Dir path can be used in 2 cases:
    1) if (path) is relative && dirPrefix == NULL
    2) for _WIN32: if (path) is absolute starting wuth "\"
*/
bool GetFullPath(CFSTR dirPrefix, CFSTR path, FString &fullPath);
bool GetFullPath(CFSTR path, FString &fullPath);

}}}

#endif

/* ---- CPP/Windows/PropVariantConv.h ---- */
// Windows/PropVariantConv.h

#ifndef ZIP7_INC_PROP_VARIANT_CONV_H
#define ZIP7_INC_PROP_VARIANT_CONV_H

// amalgamation: header emitted in prologue

// provide at least 32 bytes for buffer including zero-end

extern bool g_Timestamp_Show_UTC;

#define kTimestampPrintLevel_DAY -3
// #define kTimestampPrintLevel_HOUR -2
#define kTimestampPrintLevel_MIN -1
#define kTimestampPrintLevel_SEC  0
#define kTimestampPrintLevel_NTFS 7
#define kTimestampPrintLevel_NS   9


#define kTimestampPrintFlags_Force_UTC   (1 << 0)
#define kTimestampPrintFlags_Force_LOCAL (1 << 1)
#define kTimestampPrintFlags_DisableZ    (1 << 4)

bool ConvertUtcFileTimeToString(const FILETIME &ft, char *s, int level = kTimestampPrintLevel_SEC) throw();
bool ConvertUtcFileTimeToString(const FILETIME &ft, wchar_t *s, int level = kTimestampPrintLevel_SEC) throw();
bool ConvertUtcFileTimeToString2(const FILETIME &ft, unsigned ns100, char *s, int level = kTimestampPrintLevel_SEC, unsigned flags = 0) throw();
bool ConvertUtcFileTimeToString2(const FILETIME &ft, unsigned ns100, wchar_t *s, int level = kTimestampPrintLevel_SEC) throw();

// provide at least 32 bytes for buffer including zero-end
// don't send VT_BSTR to these functions
void ConvertPropVariantToShortString(const PROPVARIANT &prop, char *dest) throw();
void ConvertPropVariantToShortString(const PROPVARIANT &prop, wchar_t *dest) throw();

inline bool ConvertPropVariantToUInt64(const PROPVARIANT &prop, UInt64 &value)
{
  switch (prop.vt)
  {
    case VT_UI8: value = (UInt64)prop.uhVal.QuadPart; return true;
    case VT_UI4: value = prop.ulVal; return true;
    case VT_UI2: value = prop.uiVal; return true;
    case VT_UI1: value = prop.bVal; return true;
    case VT_EMPTY: return false;
    default: throw 151199;
  }
}

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

/* ================ unit bodies ================ */

/* ================ unit: CPP/Common/CRC.cpp ================ */
// Common/CRC.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static struct CCRCTableInit { CCRCTableInit() { CrcGenerateTable(); } } g_CRCTableInit;

/* ================ unit: CPP/Common/DynLimBuf.cpp ================ */
// Common/DynLimBuf.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

CDynLimBuf::CDynLimBuf(size_t limit) throw()
{
  _chars = NULL;
  _pos = 0;
  _size = 0;
  _sizeLimit = limit;
  _error = true;
  unsigned size = 1 << 4;
  if (size > limit)
    size = (unsigned)limit;
  _chars = (Byte *)MyAlloc(size);
  if (_chars)
  {
    _size = size;
    _error = false;
  }
}

CDynLimBuf & CDynLimBuf::operator+=(char c) throw()
{
  if (_error)
    return *this;
  if (_size == _pos)
  {
    size_t n = _sizeLimit - _size;
    if (n == 0)
    {
      _error = true;
      return *this;
    }
    if (n > _size)
      n = _size;

    n += _pos;

    Byte *newBuf = (Byte *)MyAlloc(n);
    if (!newBuf)
    {
      _error = true;
      return *this;
    }
    memcpy(newBuf, _chars, _pos);
    MyFree(_chars);
    _chars = newBuf;
    _size = n;
  }
  _chars[_pos++] = (Byte)c;
  return *this;
}

CDynLimBuf &CDynLimBuf::operator+=(const char *s) throw()
{
  if (_error)
    return *this;
  unsigned len = MyStringLen(s);
  size_t rem = _sizeLimit - _pos;
  if (rem < len)
  {
    len = (unsigned)rem;
    _error = true;
  }
  if (_size - _pos < len)
  {
    size_t n = _pos + len;
    if (n - _size < _size)
    {
      n = _sizeLimit;
      if (n - _size > _size)
        n = _size * 2;
    }

    Byte *newBuf = (Byte *)MyAlloc(n);
    if (!newBuf)
    {
      _error = true;
      return *this;
    }
    memcpy(newBuf, _chars, _pos);
    MyFree(_chars);
    _chars = newBuf;
    _size = n;
  }
  memcpy(_chars + _pos, s, len);
  _pos += len;
  return *this;
}

/* ================ unit: CPP/Common/IntToString.cpp ================ */
// Common/IntToString.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define CONVERT_INT_TO_STR(charType, tempSize) \
  if (val < 10) \
    *s++ = (charType)('0' + (unsigned)val); \
  else { \
    Byte temp[tempSize]; \
    size_t i = 0; \
    do { \
      temp[++i] = (Byte)('0' + (unsigned)(val % 10)); \
      val /= 10; } \
    while (val >= 10); \
    *s++ = (charType)('0' + (unsigned)val); \
    do { *s++ = (charType)temp[i]; } \
    while (--i); \
  } \
  *s = 0; \
  return s;

char * ConvertUInt32ToString(UInt32 val, char *s) throw()
{
  CONVERT_INT_TO_STR(char, 16)
}

char * ConvertUInt64ToString(UInt64 val, char *s) throw()
{
  if (val <= (UInt32)0xFFFFFFFF)
    return ConvertUInt32ToString((UInt32)val, s);
  CONVERT_INT_TO_STR(char, 24)
}

wchar_t * ConvertUInt32ToString(UInt32 val, wchar_t *s) throw()
{
  CONVERT_INT_TO_STR(wchar_t, 16)
}

wchar_t * ConvertUInt64ToString(UInt64 val, wchar_t *s) throw()
{
  if (val <= (UInt32)0xFFFFFFFF)
    return ConvertUInt32ToString((UInt32)val, s);
  CONVERT_INT_TO_STR(wchar_t, 24)
}

void ConvertInt64ToString(Int64 val, char *s) throw()
{
  if (val < 0)
  {
    *s++ = '-';
    val = -val;
  }
  ConvertUInt64ToString((UInt64)val, s);
}

void ConvertInt64ToString(Int64 val, wchar_t *s) throw()
{
  if (val < 0)
  {
    *s++ = L'-';
    val = -val;
  }
  ConvertUInt64ToString((UInt64)val, s);
}


void ConvertUInt64ToOct(UInt64 val, char *s) throw()
{
  {
    UInt64 v = val;
    do
      s++;
    while (v >>= 3);
  }
  *s = 0;
  do
  {
    const unsigned t = (unsigned)val & 7;
    *--s = (char)('0' + t);
  }
  while (val >>= 3);
}

MY_ALIGN(16) const char k_Hex_Upper[16] =
 { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
MY_ALIGN(16) const char k_Hex_Lower[16] =
 { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };

void ConvertUInt32ToHex(UInt32 val, char *s) throw()
{
  {
    UInt32 v = val;
    do
      s++;
    while (v >>= 4);
  }
  *s = 0;
  do
  {
    const unsigned t = (unsigned)val & 0xF;
    *--s = GET_HEX_CHAR_UPPER(t);
  }
  while (val >>= 4);
}

void ConvertUInt64ToHex(UInt64 val, char *s) throw()
{
  {
    UInt64 v = val;
    do
      s++;
    while (v >>= 4);
  }
  *s = 0;
  do
  {
    const unsigned t = (unsigned)val & 0xF;
    *--s = GET_HEX_CHAR_UPPER(t);
  }
  while (val >>= 4);
}

void ConvertUInt32ToHex8Digits(UInt32 val, char *s) throw()
{
  s[8] = 0;
  int i = 7;
  do
  {
    { const unsigned t = (unsigned)val & 0xF;       s[i--] = GET_HEX_CHAR_UPPER(t); }
    { const unsigned t = (Byte)val >> 4; val >>= 8; s[i--] = GET_HEX_CHAR_UPPER(t); }
  }
  while (i >= 0);
}

/*
void ConvertUInt32ToHex8Digits(UInt32 val, wchar_t *s)
{
  s[8] = 0;
  for (int i = 7; i >= 0; i--)
  {
    const unsigned t = (unsigned)val & 0xF;
    val >>= 4;
    s[i] = GET_HEX_CHAR(t);
  }
}
*/


MY_ALIGN(16) static const Byte k_Guid_Pos[] =
  { 6,4,2,0, 11,9, 16,14, 19,21, 24,26,28,30,32,34 };

char *RawLeGuidToString(const Byte *g, char *s) throw()
{
  s[ 8] = '-';
  s[13] = '-';
  s[18] = '-';
  s[23] = '-';
  s[36] = 0;
  for (unsigned i = 0; i < 16; i++)
  {
    char *s2 = s + k_Guid_Pos[i];
    const unsigned v = g[i];
    s2[0] = GET_HEX_CHAR_UPPER(v >> 4);
    s2[1] = GET_HEX_CHAR_UPPER(v & 0xF);
  }
  return s + 36;
}

char *RawLeGuidToString_Braced(const Byte *g, char *s) throw()
{
  *s++ = '{';
  s = RawLeGuidToString(g, s);
  *s++ = '}';
  *s = 0;
  return s;
}


void ConvertDataToHex_Lower(char *dest, const Byte *src, size_t size) throw()
{
  if (size)
  {
    const Byte *lim = src + size;
    do
    {
      const unsigned b = *src++;
      dest[0] = GET_HEX_CHAR_LOWER(b >> 4);
      dest[1] = GET_HEX_CHAR_LOWER(b & 0xF);
      dest += 2;
    }
    while (src != lim);
  }
  *dest = 0;
}

void ConvertDataToHex_Upper(char *dest, const Byte *src, size_t size) throw()
{
  if (size)
  {
    const Byte *lim = src + size;
    do
    {
      const unsigned b = *src++;
      dest[0] = GET_HEX_CHAR_UPPER(b >> 4);
      dest[1] = GET_HEX_CHAR_UPPER(b & 0xF);
      dest += 2;
    }
    while (src != lim);
  }
  *dest = 0;
}

/* ================ unit: CPP/Common/LzFindPrepare.cpp ================ */
// Sha256Prepare.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static struct CLzFindPrepare { CLzFindPrepare() { LzFindPrepare(); } } g_CLzFindPrepare;

/* ================ unit: CPP/Common/MyMap.cpp ================ */
// MyMap.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static const unsigned kNumBitsMax = sizeof(UInt32) * 8;

static UInt32 GetSubBits(UInt32 value, unsigned startPos, unsigned numBits) throw()
{
  if (startPos == sizeof(value) * 8)
    return 0;
  value >>= startPos;
  if (numBits == sizeof(value) * 8)
    return value;
  return value & (((UInt32)1 << numBits) - 1);
}

static inline unsigned GetSubBit(UInt32 v, unsigned n) { return (unsigned)(v >> n) & 1; }

bool CMap32::Find(UInt32 key, UInt32 &valueRes) const throw()
{
  valueRes = (UInt32)(Int32)-1;
  if (Nodes.Size() == 0)
    return false;
  if (Nodes.Size() == 1)
  {
    const CNode &n = Nodes[0];
    if (n.Len == kNumBitsMax)
    {
      valueRes = n.Values[0];
      return (key == n.Key);
    }
  }

  unsigned cur = 0;
  unsigned bitPos = kNumBitsMax;
  for (;;)
  {
    const CNode &n = Nodes[cur];
    bitPos -= n.Len;
    if (GetSubBits(key, bitPos, n.Len) != GetSubBits(n.Key, bitPos, n.Len))
      return false;
    unsigned bit = GetSubBit(key, --bitPos);
    if (n.IsLeaf[bit])
    {
      valueRes = n.Values[bit];
      return (key == n.Keys[bit]);
    }
    cur = (unsigned)n.Keys[bit];
  }
}

bool CMap32::Set(UInt32 key, UInt32 value)
{
  if (Nodes.Size() == 0)
  {
    CNode n;
    n.Key = n.Keys[0] = n.Keys[1] = key;
    n.Values[0] = n.Values[1] = value;
    n.IsLeaf[0] = n.IsLeaf[1] = 1;
    n.Len = kNumBitsMax;
    Nodes.Add(n);
    return false;
  }
  if (Nodes.Size() == 1)
  {
    CNode &n = Nodes[0];
    if (n.Len == kNumBitsMax)
    {
      if (key == n.Key)
      {
        n.Values[0] = n.Values[1] = value;
        return true;
      }
      unsigned i = kNumBitsMax - 1;
      for (; GetSubBit(key, i) == GetSubBit(n.Key, i); i--);
      n.Len = (UInt16)(kNumBitsMax - (1 + i));
      const unsigned newBit = GetSubBit(key, i);
      n.Values[newBit] = value;
      n.Keys[newBit] = key;
      return false;
    }
  }

  unsigned cur = 0;
  unsigned bitPos = kNumBitsMax;
  for (;;)
  {
    CNode &n = Nodes[cur];
    bitPos -= n.Len;
    if (GetSubBits(key, bitPos, n.Len) != GetSubBits(n.Key, bitPos, n.Len))
    {
      unsigned i = (unsigned)n.Len - 1;
      for (; GetSubBit(key, bitPos + i) == GetSubBit(n.Key, bitPos + i); i--);
      
      CNode e2(n);
      e2.Len = (UInt16)i;

      n.Len = (UInt16)(n.Len - (1 + i));
      unsigned newBit = GetSubBit(key, bitPos + i);
      n.Values[newBit] = value;
      n.IsLeaf[newBit] = 1;
      n.IsLeaf[1 - newBit] = 0;
      n.Keys[newBit] = key;
      n.Keys[1 - newBit] = (UInt32)Nodes.Size();
      Nodes.Add(e2);
      return false;
    }
    const unsigned bit = GetSubBit(key, --bitPos);

    if (n.IsLeaf[bit])
    {
      if (key == n.Keys[bit])
      {
        n.Values[bit] = value;
        return true;
      }
      unsigned i = bitPos - 1;
      for (; GetSubBit(key, i) == GetSubBit(n.Keys[bit], i); i--);
     
      CNode e2;
      
      const unsigned newBit = GetSubBit(key, i);
      e2.Values[newBit] = value;
      e2.Values[1 - newBit] = n.Values[bit];
      e2.IsLeaf[newBit] = e2.IsLeaf[1 - newBit] = 1;
      e2.Keys[newBit] = key;
      e2.Keys[1 - newBit] = e2.Key = n.Keys[bit];
      e2.Len = (UInt16)(bitPos - (1 + i));

      n.IsLeaf[bit] = 0;
      n.Keys[bit] = (UInt32)Nodes.Size();

      Nodes.Add(e2);
      return false;
    }
    cur = (unsigned)n.Keys[bit];
  }
}

/* ================ unit: CPP/Common/MyString.cpp ================ */
// Common/MyString.cpp

// amalgamation: header emitted in prologue

#ifdef _WIN32
#include <wchar.h>
#else
#include <ctype.h>
#endif

// amalgamation: header emitted in prologue

#if !defined(_UNICODE) || !defined(USE_UNICODE_FSTRING)
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue

#define MY_STRING_NEW(_T_, _size_) new _T_[_size_]
// #define MY_STRING_NEW(_T_, _size_) ((_T_ *)my_new((size_t)(_size_) * sizeof(_T_)))

/*
inline const char* MyStringGetNextCharPointer(const char *p) throw()
{
  #if defined(_WIN32) && !defined(UNDER_CE)
  return CharNextA(p);
  #else
  return p + 1;
  #endif
}
*/

#define MY_STRING_NEW_char(_size_) MY_STRING_NEW(char, (_size_))
#define MY_STRING_NEW_wchar_t(_size_) MY_STRING_NEW(wchar_t, (_size_))


int FindCharPosInString(const char *s, char c) throw()
{
  for (const char *p = s;; p++)
  {
    if (*p == c)
      return (int)(p - s);
    if (*p == 0)
      return -1;
    // MyStringGetNextCharPointer(p);
  }
}

int FindCharPosInString(const wchar_t *s, wchar_t c) throw()
{
  for (const wchar_t *p = s;; p++)
  {
    if (*p == c)
      return (int)(p - s);
    if (*p == 0)
      return -1;
  }
}

/*
void MyStringUpper_Ascii(char *s) throw()
{
  for (;;)
  {
    const char c = *s;
    if (c == 0)
      return;
    *s++ = MyCharUpper_Ascii(c);
  }
}

void MyStringUpper_Ascii(wchar_t *s) throw()
{
  for (;;)
  {
    const wchar_t c = *s;
    if (c == 0)
      return;
    *s++ = MyCharUpper_Ascii(c);
  }
}
*/

void MyStringLower_Ascii(char *s) throw()
{
  for (;;)
  {
    const char c = *s;
    if (c == 0)
      return;
    *s++ = MyCharLower_Ascii(c);
  }
}

void MyStringLower_Ascii(wchar_t *s) throw()
{
  for (;;)
  {
    const wchar_t c = *s;
    if (c == 0)
      return;
    *s++ = MyCharLower_Ascii(c);
  }
}

#ifdef _WIN32

#ifdef _UNICODE

// wchar_t * MyStringUpper(wchar_t *s) { return CharUpperW(s); }
// wchar_t * MyStringLower(wchar_t *s) { return CharLowerW(s); }
// for WinCE - FString - char
// const char *MyStringGetPrevCharPointer(const char * /* base */, const char *p) { return p - 1; }

#else

// const char * MyStringGetPrevCharPointer(const char *base, const char *p) throw() { return CharPrevA(base, p); }
// char * MyStringUpper(char *s) { return CharUpperA(s); }
// char * MyStringLower(char *s) { return CharLowerA(s); }

wchar_t MyCharUpper_WIN(wchar_t c) throw()
{
  wchar_t *res = CharUpperW((LPWSTR)(UINT_PTR)(unsigned)c);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return (wchar_t)(unsigned)(UINT_PTR)res;
  const int kBufSize = 4;
  char s[kBufSize + 1];
  int numChars = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, s, kBufSize, 0, 0);
  if (numChars == 0 || numChars > kBufSize)
    return c;
  s[numChars] = 0;
  ::CharUpperA(s);
  ::MultiByteToWideChar(CP_ACP, 0, s, numChars, &c, 1);
  return c;
}

/*
wchar_t MyCharLower_WIN(wchar_t c)
{
  wchar_t *res = CharLowerW((LPWSTR)(UINT_PTR)(unsigned)c);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return (wchar_t)(unsigned)(UINT_PTR)res;
  const int kBufSize = 4;
  char s[kBufSize + 1];
  int numChars = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, s, kBufSize, 0, 0);
  if (numChars == 0 || numChars > kBufSize)
    return c;
  s[numChars] = 0;
  ::CharLowerA(s);
  ::MultiByteToWideChar(CP_ACP, 0, s, numChars, &c, 1);
  return c;
}
*/

/*
wchar_t * MyStringUpper(wchar_t *s)
{
  if (s == 0)
    return 0;
  wchar_t *res = CharUpperW(s);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return res;
  AString a = UnicodeStringToMultiByte(s);
  a.MakeUpper();
  MyStringCopy(s, (const wchar_t *)MultiByteToUnicodeString(a));
  return s;
}
*/

/*
wchar_t * MyStringLower(wchar_t *s)
{
  if (s == 0)
    return 0;
  wchar_t *res = CharLowerW(s);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return res;
  AString a = UnicodeStringToMultiByte(s);
  a.MakeLower();
  MyStringCopy(s, (const wchar_t *)MultiByteToUnicodeString(a));
  return s;
}
*/

#endif

#endif

bool IsString1PrefixedByString2(const char *s1, const char *s2) throw()
{
  for (;;)
  {
    const char c2 = *s2++; if (c2 == 0) return true;
    const char c1 = *s1++; if (c1 != c2) return false;
  }
}

bool StringsAreEqualNoCase(const wchar_t *s1, const wchar_t *s2) throw()
{
  for (;;)
  {
    const wchar_t c1 = *s1++;
    const wchar_t c2 = *s2++;
    if (c1 != c2 && MyCharUpper(c1) != MyCharUpper(c2)) return false;
    if (c1 == 0) return true;
  }
}

// ---------- ASCII ----------

bool StringsAreEqual_Ascii(const char *u, const char *a) throw()
{
  for (;;)
  {
    const char c = *a;
    if (c != *u)
      return false;
    if (c == 0)
      return true;
    a++;
    u++;
  }
}

bool StringsAreEqual_Ascii(const wchar_t *u, const char *a) throw()
{
  for (;;)
  {
    const unsigned char c = (unsigned char)*a;
    if (c != *u)
      return false;
    if (c == 0)
      return true;
    a++;
    u++;
  }
}

bool StringsAreEqualNoCase_Ascii(const char *s1, const char *s2) throw()
{
  for (;;)
  {
    const char c1 = *s1++;
    const char c2 = *s2++;
    if (c1 != c2 && MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2))
      return false;
    if (c1 == 0)
      return true;
  }
}

bool StringsAreEqualNoCase_Ascii(const wchar_t *s1, const wchar_t *s2) throw()
{
  for (;;)
  {
    const wchar_t c1 = *s1++;
    const wchar_t c2 = *s2++;
    if (c1 != c2 && MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2))
      return false;
    if (c1 == 0)
      return true;
  }
}

bool StringsAreEqualNoCase_Ascii(const wchar_t *s1, const char *s2) throw()
{
  for (;;)
  {
    const wchar_t c1 = *s1++;
    const char c2 = *s2++;
    if (c1 != (unsigned char)c2 && (c1 > 0x7F || MyCharLower_Ascii(c1) != (unsigned char)MyCharLower_Ascii(c2)))
      return false;
    if (c1 == 0)
      return true;
  }
}

bool IsString1PrefixedByString2(const wchar_t *s1, const wchar_t *s2) throw()
{
  for (;;)
  {
    const wchar_t c2 = *s2++; if (c2 == 0) return true;
    const wchar_t c1 = *s1++; if (c1 != c2) return false;
  }
}

bool IsString1PrefixedByString2(const wchar_t *s1, const char *s2) throw()
{
  for (;;)
  {
    const unsigned char c2 = (unsigned char)(*s2++); if (c2 == 0) return true;
    const wchar_t c1 = *s1++; if (c1 != c2) return false;
  }
}

bool IsString1PrefixedByString2_NoCase_Ascii(const char *s1, const char *s2) throw()
{
  for (;;)
  {
    const char c2 = *s2++; if (c2 == 0) return true;
    const char c1 = *s1++;
    if (c1 != c2 && MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2))
      return false;
  }
}

bool IsString1PrefixedByString2_NoCase_Ascii(const wchar_t *s1, const char *s2) throw()
{
  for (;;)
  {
    const char c2 = *s2++; if (c2 == 0) return true;
    const wchar_t c1 = *s1++;
    if (c1 != (unsigned char)c2 && MyCharLower_Ascii(c1) != (unsigned char)MyCharLower_Ascii(c2))
      return false;
  }
}

bool IsString1PrefixedByString2_NoCase(const wchar_t *s1, const wchar_t *s2) throw()
{
  for (;;)
  {
    const wchar_t c2 = *s2++; if (c2 == 0) return true;
    const wchar_t c1 = *s1++;
    if (c1 != c2 && MyCharUpper(c1) != MyCharUpper(c2))
      return false;
  }
}

// NTFS order: uses upper case
int MyStringCompareNoCase(const wchar_t *s1, const wchar_t *s2) throw()
{
  for (;;)
  {
    const wchar_t c1 = *s1++;
    const wchar_t c2 = *s2++;
    if (c1 != c2)
    {
      const wchar_t u1 = MyCharUpper(c1);
      const wchar_t u2 = MyCharUpper(c2);
      if (u1 < u2) return -1;
      if (u1 > u2) return 1;
    }
    if (c1 == 0) return 0;
  }
}

/*
int MyStringCompareNoCase_N(const wchar_t *s1, const wchar_t *s2, unsigned num)
{
  for (; num != 0; num--)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2)
    {
      wchar_t u1 = MyCharUpper(c1);
      wchar_t u2 = MyCharUpper(c2);
      if (u1 < u2) return -1;
      if (u1 > u2) return 1;
    }
    if (c1 == 0) return 0;
  }
  return 0;
}
*/

// ---------- AString ----------

void AString::InsertSpace(unsigned &index, unsigned size)
{
  Grow(size);
  MoveItems(index + size, index);
}

#define k_Alloc_Len_Limit (0x40000000 - 2)
// #define k_Alloc_Len_Limit (((unsigned)1 << (sizeof(unsigned) * 8 - 2)) - 2)

void AString::ReAlloc(unsigned newLimit)
{
  // MY_STRING_REALLOC(_chars, char, (size_t)newLimit + 1, (size_t)_len + 1);
  char *newBuf = MY_STRING_NEW_char((size_t)newLimit + 1);
  memcpy(newBuf, _chars, (size_t)_len + 1);
  MY_STRING_DELETE(_chars)
  _chars = newBuf;
  _limit = newLimit;
}

#define THROW_STRING_ALLOC_EXCEPTION  { throw 20130220; }

#define CHECK_STRING_ALLOC_LEN(len) \
  { if ((len) > k_Alloc_Len_Limit) THROW_STRING_ALLOC_EXCEPTION }

void AString::ReAlloc2(unsigned newLimit)
{
  CHECK_STRING_ALLOC_LEN(newLimit)
  // MY_STRING_REALLOC(_chars, char, (size_t)newLimit + 1, 0);
  char *newBuf = MY_STRING_NEW_char((size_t)newLimit + 1);
  newBuf[0] = 0;
  MY_STRING_DELETE(_chars)
  _chars = newBuf;
  _limit = newLimit;
  _len = 0;
}

void AString::SetStartLen(unsigned len)
{
  _chars = NULL;
  _chars = MY_STRING_NEW_char((size_t)len + 1);
  _len = len;
  _limit = len;
}

Z7_NO_INLINE
void AString::Grow_1()
{
  unsigned next = _len;
  next += next / 2;
  next += 16;
  next &= ~(unsigned)15;
  next--;
  if (next < _len || next > k_Alloc_Len_Limit)
    next = k_Alloc_Len_Limit;
  if (next <= _len)
    THROW_STRING_ALLOC_EXCEPTION
  ReAlloc(next);
  // Grow(1);
}

void AString::Grow(unsigned n)
{
  const unsigned freeSize = _limit - _len;
  if (n <= freeSize)
    return;
  unsigned next = _len + n;
  next += next / 2;
  next += 16;
  next &= ~(unsigned)15;
  next--;
  if (next < _len || next > k_Alloc_Len_Limit)
    next = k_Alloc_Len_Limit;
  if (next <= _len || next - _len < n)
    THROW_STRING_ALLOC_EXCEPTION
  ReAlloc(next);
}

AString::AString(unsigned num, const char *s)
{
  unsigned len = MyStringLen(s);
  if (num > len)
    num = len;
  SetStartLen(num);
  memcpy(_chars, s, num);
  _chars[num] = 0;
}

AString::AString(unsigned num, const AString &s)
{
  if (num > s._len)
    num = s._len;
  SetStartLen(num);
  memcpy(_chars, s._chars, num);
  _chars[num] = 0;
}

AString::AString(const AString &s, char c)
{
  SetStartLen(s.Len() + 1);
  char *chars = _chars;
  unsigned len = s.Len();
  memcpy(chars, s, len);
  chars[len] = c;
  chars[(size_t)len + 1] = 0;
}

AString::AString(const char *s1, unsigned num1, const char *s2, unsigned num2)
{
  SetStartLen(num1 + num2);
  char *chars = _chars;
  memcpy(chars, s1, num1);
  memcpy(chars + num1, s2, num2 + 1);
}

AString operator+(const AString &s1, const AString &s2) { return AString(s1, s1.Len(), s2, s2.Len()); }
AString operator+(const AString &s1, const char    *s2) { return AString(s1, s1.Len(), s2, MyStringLen(s2)); }
AString operator+(const char    *s1, const AString &s2) { return AString(s1, MyStringLen(s1), s2, s2.Len()); }

static const unsigned kStartStringCapacity = 4;
 
AString::AString()
{
  _chars = NULL;
  _chars = MY_STRING_NEW_char(kStartStringCapacity);
  _len = 0;
  _limit = kStartStringCapacity - 1;
  _chars[0] = 0;
}

AString::AString(char c)
{
  SetStartLen(1);
  char *chars = _chars;
  chars[0] = c;
  chars[1] = 0;
}

AString::AString(const char *s)
{
  SetStartLen(MyStringLen(s));
  MyStringCopy(_chars, s);
}

AString::AString(const AString &s)
{
  SetStartLen(s._len);
  MyStringCopy(_chars, s._chars);
}

AString &AString::operator=(char c)
{
  if (1 > _limit)
  {
    char *newBuf = MY_STRING_NEW_char(1 + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = 1;
  }
  _len = 1;
  char *chars = _chars;
  chars[0] = c;
  chars[1] = 0;
  return *this;
}

AString &AString::operator=(const char *s)
{
  unsigned len = MyStringLen(s);
  if (len > _limit)
  {
    char *newBuf = MY_STRING_NEW_char((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  MyStringCopy(_chars, s);
  return *this;
}

AString &AString::operator=(const AString &s)
{
  if (&s == this)
    return *this;
  unsigned len = s._len;
  if (len > _limit)
  {
    char *newBuf = MY_STRING_NEW_char((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  MyStringCopy(_chars, s._chars);
  return *this;
}

void AString::SetFromWStr_if_Ascii(const wchar_t *s)
{
  unsigned len = 0;
  {
    for (;; len++)
    {
      wchar_t c = s[len];
      if (c == 0)
        break;
      if (c >= 0x80)
        return;
    }
  }
  if (len > _limit)
  {
    char *newBuf = MY_STRING_NEW_char((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  char *dest = _chars;
  unsigned i;
  for (i = 0; i < len; i++)
    dest[i] = (char)s[i];
  dest[i] = 0;
}

/*
void AString::SetFromBstr_if_Ascii(BSTR s)
{
  unsigned len = ::SysStringLen(s);
  {
    for (unsigned i = 0; i < len; i++)
      if (s[i] <= 0 || s[i] >= 0x80)
        return;
  }
  if (len > _limit)
  {
    char *newBuf = MY_STRING_NEW_char((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  char *dest = _chars;
  unsigned i;
  for (i = 0; i < len; i++)
    dest[i] = (char)s[i];
  dest[i] = 0;
}
*/

void AString::Add_Char(char c) { operator+=(c); }
void AString::Add_Space() { operator+=(' '); }
void AString::Add_Space_if_NotEmpty() { if (!IsEmpty()) Add_Space(); }
void AString::Add_LF() { operator+=('\n'); }
void AString::Add_Slash() { operator+=('/'); }
void AString::Add_Dot() { operator+=('.'); }
void AString::Add_Minus() { operator+=('-'); }
void AString::Add_Colon() { operator+=(':'); }

AString &AString::operator+=(const char *s)
{
  unsigned len = MyStringLen(s);
  Grow(len);
  MyStringCopy(_chars + _len, s);
  _len += len;
  return *this;
}

void AString::Add_OptSpaced(const char *s)
{
  Add_Space_if_NotEmpty();
  (*this) += s;
}

AString &AString::operator+=(const AString &s)
{
  Grow(s._len);
  MyStringCopy(_chars + _len, s._chars);
  _len += s._len;
  return *this;
}

void AString::Add_UInt32(UInt32 v)
{
  Grow(10);
  _len = (unsigned)(ConvertUInt32ToString(v, _chars + _len) - _chars);
}

void UString::Add_UInt64(UInt64 v)
{
  Grow(20);
  _len = (unsigned)(ConvertUInt64ToString(v, _chars + _len) - _chars);
}

void AString::AddFrom(const char *s, unsigned len) // no check
{
  if (len != 0)
  {
    Grow(len);
    memcpy(_chars + _len, s, len);
    len += _len;
    _chars[len] = 0;
    _len = len;
  }
}

void AString::SetFrom(const char *s, unsigned len) // no check
{
  if (len > _limit)
  {
    CHECK_STRING_ALLOC_LEN(len)
    char *newBuf = MY_STRING_NEW_char((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  if (len != 0)
    memcpy(_chars, s, len);
  _chars[len] = 0;
  _len = len;
}

void AString::SetFrom_Chars_SizeT(const char *s, size_t len)
{
  CHECK_STRING_ALLOC_LEN(len)
  SetFrom(s, (unsigned)len);
}

void AString::SetFrom_CalcLen(const char *s, unsigned len) // no check
{
  unsigned i;
  for (i = 0; i < len; i++)
    if (s[i] == 0)
      break;
  SetFrom(s, i);
}

int AString::Find(const char *s, unsigned startIndex) const throw()
{
  const char *fs = strstr(_chars + startIndex, s);
  if (!fs)
    return -1;
  return (int)(fs - _chars);

  /*
  if (s[0] == 0)
    return startIndex;
  unsigned len = MyStringLen(s);
  const char *p = _chars + startIndex;
  for (;; p++)
  {
    const char c = *p;
    if (c != s[0])
    {
      if (c == 0)
        return -1;
      continue;
    }
    unsigned i;
    for (i = 1; i < len; i++)
      if (p[i] != s[i])
        break;
    if (i == len)
      return (int)(p - _chars);
  }
  */
}

int AString::ReverseFind(char c) const throw()
{
  if (_len == 0)
    return -1;
  const char *p = _chars + _len - 1;
  for (;;)
  {
    if (*p == c)
      return (int)(p - _chars);
    if (p == _chars)
      return -1;
    p--; // p = GetPrevCharPointer(_chars, p);
  }
}

int AString::ReverseFind_PathSepar() const throw()
{
  if (_len == 0)
    return -1;
  const char *p = _chars + _len - 1;
  for (;;)
  {
    const char c = *p;
    if (IS_PATH_SEPAR(c))
      return (int)(p - _chars);
    if (p == _chars)
      return -1;
    p--;
  }
}

void AString::TrimLeft() throw()
{
  const char *p = _chars;
  for (;; p++)
  {
    char c = *p;
    if (c != ' ' && c != '\n' && c != '\t')
      break;
  }
  unsigned pos = (unsigned)(p - _chars);
  if (pos != 0)
  {
    MoveItems(0, pos);
    _len -= pos;
  }
}

void AString::TrimRight() throw()
{
  const char *p = _chars;
  unsigned i;
  for (i = _len; i != 0; i--)
  {
    char c = p[(size_t)i - 1];
    if (c != ' ' && c != '\n' && c != '\t')
      break;
  }
  if (i != _len)
  {
    _chars[i] = 0;
    _len = i;
  }
}

void AString::InsertAtFront(char c)
{
  if (_limit == _len)
    Grow_1();
  MoveItems(1, 0);
  _chars[0] = c;
  _len++;
}

/*
void AString::Insert(unsigned index, char c)
{
  InsertSpace(index, 1);
  _chars[index] = c;
  _len++;
}
*/

void AString::Insert(unsigned index, const char *s)
{
  unsigned num = MyStringLen(s);
  if (num != 0)
  {
    InsertSpace(index, num);
    memcpy(_chars + index, s, num);
    _len += num;
  }
}

void AString::Insert(unsigned index, const AString &s)
{
  unsigned num = s.Len();
  if (num != 0)
  {
    InsertSpace(index, num);
    memcpy(_chars + index, s, num);
    _len += num;
  }
}

void AString::RemoveChar(char ch) throw()
{
  char *src = _chars;
  
  for (;;)
  {
    char c = *src++;
    if (c == 0)
      return;
    if (c == ch)
      break;
  }

  char *dest = src - 1;
  
  for (;;)
  {
    char c = *src++;
    if (c == 0)
      break;
    if (c != ch)
      *dest++ = c;
  }
  
  *dest = 0;
  _len = (unsigned)(dest - _chars);
}

// !!!!!!!!!!!!!!! test it if newChar = '\0'
void AString::Replace(char oldChar, char newChar) throw()
{
  if (oldChar == newChar)
    return; // 0;
  // unsigned number = 0;
  int pos = 0;
  char *chars = _chars;
  while ((unsigned)pos < _len)
  {
    pos = Find(oldChar, (unsigned)pos);
    if (pos < 0)
      break;
    chars[(unsigned)pos] = newChar;
    pos++;
    // number++;
  }
  return; //  number;
}

void AString::Replace(const AString &oldString, const AString &newString)
{
  if (oldString.IsEmpty())
    return; // 0;
  if (oldString == newString)
    return; // 0;
  const unsigned oldLen = oldString.Len();
  const unsigned newLen = newString.Len();
  // unsigned number = 0;
  int pos = 0;
  while ((unsigned)pos < _len)
  {
    pos = Find(oldString, (unsigned)pos);
    if (pos < 0)
      break;
    Delete((unsigned)pos, oldLen);
    Insert((unsigned)pos, newString);
    pos += newLen;
    // number++;
  }
  // return number;
}

void AString::Delete(unsigned index) throw()
{
  MoveItems(index, index + 1);
  _len--;
}

void AString::Delete(unsigned index, unsigned count) throw()
{
  if (index + count > _len)
    count = _len - index;
  if (count > 0)
  {
    MoveItems(index, index + count);
    _len -= count;
  }
}

void AString::DeleteFrontal(unsigned num) throw()
{
  if (num != 0)
  {
    MoveItems(0, num);
    _len -= num;
  }
}

/*
AString operator+(const AString &s1, const AString &s2)
{
  AString result(s1);
  result += s2;
  return result;
}

AString operator+(const AString &s, const char *chars)
{
  AString result(s);
  result += chars;
  return result;
}

AString operator+(const char *chars, const AString &s)
{
  AString result(chars);
  result += s;
  return result;
}

AString operator+(const AString &s, char c)
{
  AString result(s);
  result += c;
  return result;
}
*/

/*
AString operator+(char c, const AString &s)
{
  AString result(c);
  result += s;
  return result;
}
*/




// ---------- UString ----------

void UString::InsertSpace(unsigned index, unsigned size)
{
  Grow(size);
  MoveItems(index + size, index);
}

void UString::ReAlloc(unsigned newLimit)
{
  // MY_STRING_REALLOC(_chars, wchar_t, (size_t)newLimit + 1, (size_t)_len + 1);
  wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)newLimit + 1);
  wmemcpy(newBuf, _chars, _len + 1);
  MY_STRING_DELETE(_chars)
  _chars = newBuf;
  _limit = newLimit;
}

void UString::ReAlloc2(unsigned newLimit)
{
  CHECK_STRING_ALLOC_LEN(newLimit)
  // MY_STRING_REALLOC(_chars, wchar_t, newLimit + 1, 0);
  wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)newLimit + 1);
  newBuf[0] = 0;
  MY_STRING_DELETE(_chars)
  _chars = newBuf;
  _limit = newLimit;
  _len = 0;
}

void UString::SetStartLen(unsigned len)
{
  _chars = NULL;
  _chars = MY_STRING_NEW_wchar_t((size_t)len + 1);
  _len = len;
  _limit = len;
}

Z7_NO_INLINE
void UString::Grow_1()
{
  unsigned next = _len;
  next += next / 2;
  next += 16;
  next &= ~(unsigned)15;
  next--;
  if (next < _len || next > k_Alloc_Len_Limit)
    next = k_Alloc_Len_Limit;
  if (next <= _len)
    THROW_STRING_ALLOC_EXCEPTION
  ReAlloc(next);
}

void UString::Grow(unsigned n)
{
  const unsigned freeSize = _limit - _len;
  if (n <= freeSize)
    return;
  unsigned next = _len + n;
  next += next / 2;
  next += 16;
  next &= ~(unsigned)15;
  next--;
  if (next < _len || next > k_Alloc_Len_Limit)
    next = k_Alloc_Len_Limit;
  if (next <= _len || next - _len < n)
    THROW_STRING_ALLOC_EXCEPTION
  ReAlloc(next - 1);
}


UString::UString(unsigned num, const wchar_t *s)
{
  unsigned len = MyStringLen(s);
  if (num > len)
    num = len;
  SetStartLen(num);
  wmemcpy(_chars, s, num);
  _chars[num] = 0;
}


UString::UString(unsigned num, const UString &s)
{
  if (num > s._len)
    num = s._len;
  SetStartLen(num);
  wmemcpy(_chars, s._chars, num);
  _chars[num] = 0;
}

UString::UString(const UString &s, wchar_t c)
{
  SetStartLen(s.Len() + 1);
  wchar_t *chars = _chars;
  unsigned len = s.Len();
  wmemcpy(chars, s, len);
  chars[len] = c;
  chars[(size_t)len + 1] = 0;
}

UString::UString(const wchar_t *s1, unsigned num1, const wchar_t *s2, unsigned num2)
{
  SetStartLen(num1 + num2);
  wchar_t *chars = _chars;
  wmemcpy(chars, s1, num1);
  wmemcpy(chars + num1, s2, num2 + 1);
}

UString operator+(const UString &s1, const UString &s2) { return UString(s1, s1.Len(), s2, s2.Len()); }
UString operator+(const UString &s1, const wchar_t *s2) { return UString(s1, s1.Len(), s2, MyStringLen(s2)); }
UString operator+(const wchar_t *s1, const UString &s2) { return UString(s1, MyStringLen(s1), s2, s2.Len()); }

UString::UString()
{
  _chars = NULL;
  _chars = MY_STRING_NEW_wchar_t(kStartStringCapacity);
  _len = 0;
  _limit = kStartStringCapacity - 1;
  _chars[0] = 0;
}

UString::UString(wchar_t c)
{
  SetStartLen(1);
  wchar_t *chars = _chars;
  chars[0] = c;
  chars[1] = 0;
}

UString::UString(char c)
{
  SetStartLen(1);
  wchar_t *chars = _chars;
  chars[0] = (unsigned char)c;
  chars[1] = 0;
}

UString::UString(const wchar_t *s)
{
  const unsigned len = MyStringLen(s);
  SetStartLen(len);
  wmemcpy(_chars, s, len + 1);
}

UString::UString(const char *s)
{
  const unsigned len = MyStringLen(s);
  SetStartLen(len);
  wchar_t *chars = _chars;
  for (unsigned i = 0; i < len; i++)
    chars[i] = (unsigned char)s[i];
  chars[len] = 0;
}

UString::UString(const AString &s)
{
  const unsigned len = s.Len();
  SetStartLen(len);
  wchar_t *chars = _chars;
  const char *s2 = s.Ptr();
  for (unsigned i = 0; i < len; i++)
    chars[i] = (unsigned char)s2[i];
  chars[len] = 0;
}

UString::UString(const UString &s)
{
  SetStartLen(s._len);
  wmemcpy(_chars, s._chars, s._len + 1);
}

UString &UString::operator=(wchar_t c)
{
  if (1 > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t(1 + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = 1;
  }
  _len = 1;
  wchar_t *chars = _chars;
  chars[0] = c;
  chars[1] = 0;
  return *this;
}

UString &UString::operator=(const wchar_t *s)
{
  unsigned len = MyStringLen(s);
  if (len > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  wmemcpy(_chars, s, len + 1);
  return *this;
}

UString &UString::operator=(const UString &s)
{
  if (&s == this)
    return *this;
  unsigned len = s._len;
  if (len > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  wmemcpy(_chars, s._chars, len + 1);
  return *this;
}

void UString::SetFrom(const wchar_t *s, unsigned len) // no check
{
  if (len > _limit)
  {
    CHECK_STRING_ALLOC_LEN(len)
    wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  if (len != 0)
    wmemcpy(_chars, s, len);
  _chars[len] = 0;
  _len = len;
}

void UString::SetFromBstr(LPCOLESTR s)
{
  unsigned len = ::SysStringLen((BSTR)(void *)(s));

  /*
  #if WCHAR_MAX > 0xffff
  size_t num_wchars = 0;
  for (size_t i = 0; i < len;)
  {
    wchar_t c = s[i++];
    if (c >= 0xd800 && c < 0xdc00 && i + 1 != len)
    {
      wchar_t c2 = s[i];
      if (c2 >= 0xdc00 && c2 < 0xe000)
      {
        c = 0x10000 + ((c & 0x3ff) << 10) + (c2 & 0x3ff);
        i++;
      }
    }
    num_wchars++;
  }
  len = num_wchars;
  #endif
  */

  if (len > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  _len = len;

  /*
  #if WCHAR_MAX > 0xffff

  wchar_t *chars = _chars;
  for (size_t i = 0; i <= len; i++)
  {
    wchar_t c = *s++;
    if (c >= 0xd800 && c < 0xdc00 && i + 1 != len)
    {
      wchar_t c2 = *s;
      if (c2 >= 0xdc00 && c2 < 0xe000)
      {
        s++;
        c = 0x10000 + ((c & 0x3ff) << 10) + (c2 & 0x3ff);
      }
    }
    chars[i] = c;
  }

  #else
  */

  // if (s)
    wmemcpy(_chars, s, len + 1);
  
  // #endif
}

UString &UString::operator=(const char *s)
{
  unsigned len = MyStringLen(s);
  if (len > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)len + 1);
    MY_STRING_DELETE(_chars)
    _chars = newBuf;
    _limit = len;
  }
  wchar_t *chars = _chars;
  for (unsigned i = 0; i < len; i++)
    chars[i] = (unsigned char)s[i];
  chars[len] = 0;
  _len = len;
  return *this;
}

void UString::Add_Char(char c) { operator+=((wchar_t)(unsigned char)c); }
// void UString::Add_WChar(wchar_t c) { operator+=(c); }
void UString::Add_Dot() { operator+=(L'.'); }
void UString::Add_Space() { operator+=(L' '); }
void UString::Add_Minus() { operator+=(L'-'); }
void UString::Add_Colon() { operator+=(L':'); }
void UString::Add_LF() { operator+=(L'\n'); }
void UString::Add_Space_if_NotEmpty() { if (!IsEmpty()) Add_Space(); }

UString &UString::operator+=(const wchar_t *s)
{
  unsigned len = MyStringLen(s);
  Grow(len);
  wmemcpy(_chars + _len, s, len + 1);
  _len += len;
  return *this;
}

UString &UString::operator+=(const UString &s)
{
  Grow(s._len);
  wmemcpy(_chars + _len, s._chars, s._len + 1);
  _len += s._len;
  return *this;
}

UString &UString::operator+=(const char *s)
{
  unsigned len = MyStringLen(s);
  Grow(len);
  wchar_t *chars = _chars + _len;
  for (unsigned i = 0; i < len; i++)
    chars[i] = (unsigned char)s[i];
  chars[len] = 0;
  _len += len;
  return *this;
}


void UString::Add_UInt32(UInt32 v)
{
  Grow(10);
  _len = (unsigned)(ConvertUInt32ToString(v, _chars + _len) - _chars);
}

void AString::Add_UInt64(UInt64 v)
{
  Grow(20);
  _len = (unsigned)(ConvertUInt64ToString(v, _chars + _len) - _chars);
}


int UString::Find(const wchar_t *s, unsigned startIndex) const throw()
{
  const wchar_t *fs = wcsstr(_chars + startIndex, s);
  if (!fs)
    return -1;
  return (int)(fs - _chars);

  /*
  if (s[0] == 0)
    return startIndex;
  unsigned len = MyStringLen(s);
  const wchar_t *p = _chars + startIndex;
  for (;; p++)
  {
    const wchar_t c = *p;
    if (c != s[0])
    {
      if (c == 0)
        return -1;
      continue;
    }
    unsigned i;
    for (i = 1; i < len; i++)
      if (p[i] != s[i])
        break;
    if (i == len)
      return (int)(p - _chars);
  }
  */
}

int UString::ReverseFind(wchar_t c) const throw()
{
  if (_len == 0)
    return -1;
  const wchar_t *p = _chars + _len;
  do
  {
    if (*(--p) == c)
      return (int)(p - _chars);
  }
  while (p != _chars);
  return -1;
}

int UString::ReverseFind_PathSepar() const throw()
{
  const wchar_t *p = _chars + _len;
  while (p != _chars)
  {
    const wchar_t c = *(--p);
    if (IS_PATH_SEPAR(c))
      return (int)(p - _chars);
  }
  return -1;
}

void UString::TrimLeft() throw()
{
  const wchar_t *p = _chars;
  for (;; p++)
  {
    wchar_t c = *p;
    if (c != ' ' && c != '\n' && c != '\t')
      break;
  }
  unsigned pos = (unsigned)(p - _chars);
  if (pos != 0)
  {
    MoveItems(0, pos);
    _len -= pos;
  }
}

void UString::TrimRight() throw()
{
  const wchar_t *p = _chars;
  unsigned i;
  for (i = _len; i != 0; i--)
  {
    wchar_t c = p[(size_t)i - 1];
    if (c != ' ' && c != '\n' && c != '\t')
      break;
  }
  if (i != _len)
  {
    _chars[i] = 0;
    _len = i;
  }
}

void UString::InsertAtFront(wchar_t c)
{
  if (_limit == _len)
    Grow_1();
  MoveItems(1, 0);
  _chars[0] = c;
  _len++;
}

/*
void UString::Insert_wchar_t(unsigned index, wchar_t c)
{
  InsertSpace(index, 1);
  _chars[index] = c;
  _len++;
}
*/

void UString::Insert(unsigned index, const wchar_t *s)
{
  unsigned num = MyStringLen(s);
  if (num != 0)
  {
    InsertSpace(index, num);
    wmemcpy(_chars + index, s, num);
    _len += num;
  }
}

void UString::Insert(unsigned index, const UString &s)
{
  unsigned num = s.Len();
  if (num != 0)
  {
    InsertSpace(index, num);
    wmemcpy(_chars + index, s, num);
    _len += num;
  }
}

void UString::RemoveChar(wchar_t ch) throw()
{
  wchar_t *src = _chars;
  
  for (;;)
  {
    wchar_t c = *src++;
    if (c == 0)
      return;
    if (c == ch)
      break;
  }

  wchar_t *dest = src - 1;
  
  for (;;)
  {
    wchar_t c = *src++;
    if (c == 0)
      break;
    if (c != ch)
      *dest++ = c;
  }
  
  *dest = 0;
  _len = (unsigned)(dest - _chars);
}

// !!!!!!!!!!!!!!! test it if newChar = '\0'
void UString::Replace(wchar_t oldChar, wchar_t newChar) throw()
{
  if (oldChar == newChar)
    return; // 0;
  // unsigned number = 0;
  int pos = 0;
  wchar_t *chars = _chars;
  while ((unsigned)pos < _len)
  {
    pos = Find(oldChar, (unsigned)pos);
    if (pos < 0)
      break;
    chars[(unsigned)pos] = newChar;
    pos++;
    // number++;
  }
  return; //  number;
}

void UString::Replace(const UString &oldString, const UString &newString)
{
  if (oldString.IsEmpty())
    return; // 0;
  if (oldString == newString)
    return; // 0;
  unsigned oldLen = oldString.Len();
  unsigned newLen = newString.Len();
  // unsigned number = 0;
  int pos = 0;
  while ((unsigned)pos < _len)
  {
    pos = Find(oldString, (unsigned)pos);
    if (pos < 0)
      break;
    Delete((unsigned)pos, oldLen);
    Insert((unsigned)pos, newString);
    pos += newLen;
    // number++;
  }
  // return number;
}

void UString::Delete(unsigned index) throw()
{
  MoveItems(index, index + 1);
  _len--;
}

void UString::Delete(unsigned index, unsigned count) throw()
{
  if (index + count > _len)
    count = _len - index;
  if (count > 0)
  {
    MoveItems(index, index + count);
    _len -= count;
  }
}

void UString::DeleteFrontal(unsigned num) throw()
{
  if (num != 0)
  {
    MoveItems(0, num);
    _len -= num;
  }
}


// ---------- UString2 ----------

void UString2::ReAlloc2(unsigned newLimit)
{
  // wrong (_len) is allowed after this function
  CHECK_STRING_ALLOC_LEN(newLimit)
  // MY_STRING_REALLOC(_chars, wchar_t, newLimit + 1, 0);
  if (_chars)
  {
    MY_STRING_DELETE(_chars)
    _chars = NULL;
    // _len = 0;
  }
  _chars = MY_STRING_NEW_wchar_t((size_t)newLimit + 1);
  _chars[0] = 0;
  // _len = newLimit;
}

void UString2::SetStartLen(unsigned len)
{
  _chars = NULL;
  _chars = MY_STRING_NEW_wchar_t((size_t)len + 1);
  _len = len;
}


/*
UString2::UString2(wchar_t c)
{
  SetStartLen(1);
  wchar_t *chars = _chars;
  chars[0] = c;
  chars[1] = 0;
}
*/

UString2::UString2(const wchar_t *s)
{
  const unsigned len = MyStringLen(s);
  SetStartLen(len);
  wmemcpy(_chars, s, len + 1);
}

UString2::UString2(const UString2 &s): _chars(NULL), _len(0)
{
  if (s._chars)
  {
    SetStartLen(s._len);
    wmemcpy(_chars, s._chars, s._len + 1);
  }
}

/*
UString2 &UString2::operator=(wchar_t c)
{
  if (1 > _len)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t(1 + 1);
    if (_chars)
      MY_STRING_DELETE(_chars)
    _chars = newBuf;
  }
  _len = 1;
  wchar_t *chars = _chars;
  chars[0] = c;
  chars[1] = 0;
  return *this;
}
*/

UString2 &UString2::operator=(const wchar_t *s)
{
  unsigned len = MyStringLen(s);
  if (len > _len)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)len + 1);
    if (_chars)
      MY_STRING_DELETE(_chars)
    _chars = newBuf;
  }
  _len = len;
  MyStringCopy(_chars, s);
  return *this;
}

void UString2::SetFromAscii(const char *s)
{
  unsigned len = MyStringLen(s);
  if (len > _len)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)len + 1);
    if (_chars)
      MY_STRING_DELETE(_chars)
    _chars = newBuf;
  }
  wchar_t *chars = _chars;
  for (unsigned i = 0; i < len; i++)
    chars[i] = (unsigned char)s[i];
  chars[len] = 0;
  _len = len;
}

UString2 &UString2::operator=(const UString2 &s)
{
  if (&s == this)
    return *this;
  unsigned len = s._len;
  if (len > _len)
  {
    wchar_t *newBuf = MY_STRING_NEW_wchar_t((size_t)len + 1);
    if (_chars)
      MY_STRING_DELETE(_chars)
    _chars = newBuf;
  }
  _len = len;
  MyStringCopy(_chars, s._chars);
  return *this;
}

bool operator==(const UString2 &s1, const UString2 &s2)
{
  return s1.Len() == s2.Len() && (s1.IsEmpty() || wcscmp(s1.GetRawPtr(), s2.GetRawPtr()) == 0);
}

bool operator==(const UString2 &s1, const wchar_t *s2)
{
  if (s1.IsEmpty())
    return (*s2 == 0);
  return wcscmp(s1.GetRawPtr(), s2) == 0;
}

bool operator==(const wchar_t *s1, const UString2 &s2)
{
  if (s2.IsEmpty())
    return (*s1 == 0);
  return wcscmp(s1, s2.GetRawPtr()) == 0;
}



// ----------------------------------------

/*
int MyStringCompareNoCase(const char *s1, const char *s2)
{
  return MyStringCompareNoCase(MultiByteToUnicodeString(s1), MultiByteToUnicodeString(s2));
}
*/

#if !defined(USE_UNICODE_FSTRING) || !defined(_UNICODE)

static inline UINT GetCurrentCodePage()
{
  #if defined(UNDER_CE) || !defined(_WIN32)
  return CP_ACP;
  #else
  return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  #endif
}

#endif

#ifdef USE_UNICODE_FSTRING

#ifndef _UNICODE

AString fs2fas(CFSTR s)
{
  return UnicodeStringToMultiByte(s, GetCurrentCodePage());
}

FString fas2fs(const char *s)
{
  return MultiByteToUnicodeString(s, GetCurrentCodePage());
}

FString fas2fs(const AString &s)
{
  return MultiByteToUnicodeString(s, GetCurrentCodePage());
}

#endif //  _UNICODE

#else // USE_UNICODE_FSTRING

UString fs2us(const FChar *s)
{
  return MultiByteToUnicodeString(s, GetCurrentCodePage());
}

UString fs2us(const FString &s)
{
  return MultiByteToUnicodeString(s, GetCurrentCodePage());
}

FString us2fs(const wchar_t *s)
{
  return UnicodeStringToMultiByte(s, GetCurrentCodePage());
}

#endif // USE_UNICODE_FSTRING


bool CStringFinder::FindWord_In_LowCaseAsciiList_NoCase(const char *p, const wchar_t *str)
{
  _temp.Empty();
  for (;;)
  {
    const wchar_t c = *str++;
    if (c == 0)
      break;
    if (c <= 0x20 || c > 0x7f)
      return false;
    _temp.Add_Char((char)MyCharLower_Ascii((char)c));
  }

  while (*p != 0)
  {
    const char *s2 = _temp.Ptr();
    char c, c2;
    do
    {
      c = *p++;
      c2 = *s2++;
    }
    while (c == c2);
    
    if (c == ' ')
    {
      if (c2 == 0)
        return true;
      continue;
    }
    
    while (*p++ != ' ');
  }
  
  return false;
}


void SplitString(const UString &srcString, UStringVector &destStrings)
{
  destStrings.Clear();
  unsigned len = srcString.Len();
  if (len == 0)
    return;
  UString s;
  for (unsigned i = 0; i < len; i++)
  {
    const wchar_t c = srcString[i];
    if (c == ' ')
    {
      if (!s.IsEmpty())
      {
        destStrings.Add(s);
        s.Empty();
      }
    }
    else
      s += c;
  }
  if (!s.IsEmpty())
    destStrings.Add(s);
}

/* ================ unit: CPP/Common/MyVector.cpp ================ */
// Common/MyVector.cpp

// amalgamation: header emitted in prologue

/* ================ unit: CPP/Common/MyXml.cpp ================ */
// MyXml.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

static bool IsValidChar(char c)
{
  return
    (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') ||
    (c >= '0' && c <= '9') ||
    c == '-';
}

static bool IsSpaceChar(char c)
{
  return (c == ' ' || c == '\t' || c == 0x0D || c == 0x0A);
}

#define SKIP_SPACES(s) while (IsSpaceChar(*s)) s++;

int CXmlItem::FindProp(const char *propName) const throw()
{
  FOR_VECTOR (i, Props)
    if (Props[i].Name.IsEqualTo(propName))
      return (int)i;
  return -1;
}

AString CXmlItem::GetPropVal(const char *propName) const
{
  const int index = FindProp(propName);
  if (index >= 0)
    return Props[(unsigned)index].Value;
  return AString();
}

bool CXmlItem::IsTagged(const char *tag) const throw()
{
  return (IsTag && Name.IsEqualTo(tag));
}

int CXmlItem::FindSubTag(const char *tag) const throw()
{
  FOR_VECTOR (i, SubItems)
    if (SubItems[i].IsTagged(tag))
      return (int)i;
  return -1;
}

const CXmlItem *CXmlItem::FindSubTag_GetPtr(const char *tag) const throw()
{
  FOR_VECTOR (i, SubItems)
  {
    const CXmlItem *p = &SubItems[i];
    if (p->IsTagged(tag))
      return p;
  }
  return NULL;
}

AString CXmlItem::GetSubString() const
{
  if (SubItems.Size() == 1)
  {
    const CXmlItem &item = SubItems[0];
    if (!item.IsTag)
      return item.Name;
  }
  return AString();
}

const AString * CXmlItem::GetSubStringPtr() const throw()
{
  if (SubItems.Size() == 1)
  {
    const CXmlItem &item = SubItems[0];
    if (!item.IsTag)
      return &item.Name;
  }
  return NULL;
}

AString CXmlItem::GetSubStringForTag(const char *tag) const
{
  const CXmlItem *item = FindSubTag_GetPtr(tag);
  if (item)
    return item->GetSubString();
  return AString();
}

const char * CXmlItem::ParseItem(const char *s, int numAllowedLevels)
{
  SKIP_SPACES(s)

  const char *beg = s;
  for (;;)
  {
    char c;
    c = *s; if (c == 0 || c == '<') break; s++;
    c = *s; if (c == 0 || c == '<') break; s++;
  }
  if (*s == 0)
    return NULL;
  {
    const size_t num = (size_t)(s - beg);
    if (num)
    {
      IsTag = false;
      Name.SetFrom_Chars_SizeT(beg, num);
      return s;
    }
  }
  
  IsTag = true;

  s++;
  SKIP_SPACES(s)

  beg = s;
  for (;; s++)
    if (!IsValidChar(*s))
      break;
  if (s == beg || *s == 0)
    return NULL;
  Name.SetFrom_Chars_SizeT(beg, (size_t)(s - beg));

  for (;;)
  {
    beg = s;
    SKIP_SPACES(s)
    if (*s == '/')
    {
      s++;
      // SKIP_SPACES(s)
      if (*s != '>')
        return NULL;
      return s + 1;
    }
    if (*s == '>')
    {
      s++;
      if (numAllowedLevels == 0)
        return NULL;
      SubItems.Clear();
      for (;;)
      {
        SKIP_SPACES(s)
        if (s[0] == '<' && s[1] == '/')
          break;
        CXmlItem &item = SubItems.AddNew();
        s = item.ParseItem(s, numAllowedLevels - 1);
        if (!s)
          return NULL;
      }

      s += 2;
      const unsigned len = Name.Len();
      const char *name = Name.Ptr();
      for (unsigned i = 0; i < len; i++)
        if (*s++ != *name++)
          return NULL;
      // s += len;
      if (s[0] != '>')
        return NULL;
      return s + 1;
    }
    if (beg == s)
      return NULL;

    // ReadProperty
    CXmlProp &prop = Props.AddNew();

    beg = s;
    for (;; s++)
    {
      char c = *s;
      if (!IsValidChar(c))
        break;
    }
    if (s == beg)
      return NULL;
    prop.Name.SetFrom_Chars_SizeT(beg, (size_t)(s - beg));
    
    SKIP_SPACES(s)
    if (*s != '=')
      return NULL;
    s++;
    SKIP_SPACES(s)
    if (*s != '\"')
      return NULL;
    s++;
    
    beg = s;
    for (;;)
    {
      char c = *s;
      if (c == 0)
        return NULL;
      if (c == '\"')
        break;
      s++;
    }
    prop.Value.SetFrom_Chars_SizeT(beg, (size_t)(s - beg));
    s++;
  }
}

static const char * SkipHeader(const char *s, const char *startString, const char *endString)
{
  SKIP_SPACES(s)
  if (IsString1PrefixedByString2(s, startString))
  {
    s = strstr(s, endString);
    if (!s)
      return NULL;
    s += strlen(endString);
  }
  return s;
}

void CXmlItem::AppendTo(AString &s) const
{
  if (IsTag)
    s += '<';
  s += Name;
  if (IsTag)
  {
    FOR_VECTOR (i, Props)
    {
      const CXmlProp &prop = Props[i];
      s.Add_Space();
      s += prop.Name;
      s += '=';
      s += '\"';
      s += prop.Value;
      s += '\"';
    }
    s += '>';
  }
  FOR_VECTOR (i, SubItems)
  {
    const CXmlItem &item = SubItems[i];
    if (i != 0 && !SubItems[i - 1].IsTag)
      s.Add_Space();
    item.AppendTo(s);
  }
  if (IsTag)
  {
    s += '<';
    s += '/';
    s += Name;
    s += '>';
  }
}

bool CXml::Parse(const char *s)
{
  s = SkipHeader(s, "<?xml",    "?>"); if (!s) return false;
  s = SkipHeader(s, "<!DOCTYPE", ">"); if (!s) return false;

  s = Root.ParseItem(s, 1000);
  if (!s || !Root.IsTag)
    return false;
  SKIP_SPACES(s)
  return *s == 0;
}

/*
void CXml::AppendTo(AString &s) const
{
  Root.AppendTo(s);
}
*/


void z7_xml_DecodeString(AString &temp)
{
  char * const beg = temp.GetBuf();
  char *dest = beg;
  const char *p = beg;
  for (;;)
  {
    char c = *p++;
    if (c == 0)
      break;
    if (c == '&')
    {
      if (p[0] == '#')
      {
        const char *end;
        const UInt32 number = ConvertStringToUInt32(p + 1, &end);
        if (*end == ';' && number != 0 && number <= 127)
        {
          p = end + 1;
          c = (char)number;
        }
      }
      else if (
          p[0] == 'a' &&
          p[1] == 'm' &&
          p[2] == 'p' &&
          p[3] == ';')
      {
        p += 4;
      }
      else if (
          p[0] == 'l' &&
          p[1] == 't' &&
          p[2] == ';')
      {
        p += 3;
        c = '<';
      }
      else if (
          p[0] == 'g' &&
          p[1] == 't' &&
          p[2] == ';')
      {
        p += 3;
        c = '>';
      }
      else if (
          p[0] == 'a' &&
          p[1] == 'p' &&
          p[2] == 'o' &&
          p[3] == 's' &&
          p[4] == ';')
      {
        p += 5;
        c = '\'';
      }
      else if (
          p[0] == 'q' &&
          p[1] == 'u' &&
          p[2] == 'o' &&
          p[3] == 't' &&
          p[4] == ';')
      {
        p += 5;
        c = '\"';
      }
    }
    *dest++ = c;
  }
  temp.ReleaseBuf_SetEnd((unsigned)(dest - beg));
}

/* ================ unit: CPP/Common/NewHandler.cpp ================ */
// NewHandler.cpp
 
// amalgamation: header emitted in prologue

#include <stdlib.h>

// amalgamation: header emitted in prologue

// #define DEBUG_MEMORY_LEAK

#ifndef DEBUG_MEMORY_LEAK

#ifdef Z7_REDEFINE_OPERATOR_NEW

/*
void * my_new(size_t size)
{
  // void *p = ::HeapAlloc(::GetProcessHeap(), 0, size);
  if (size == 0)
    size = 1;
  void *p = ::malloc(size);
  if (!p)
    throw CNewException();
  return p;
}

void my_delete(void *p) throw()
{
  // if (!p) return; ::HeapFree(::GetProcessHeap(), 0, p);
  ::free(p);
}

void * my_Realloc(void *p, size_t newSize, size_t oldSize)
{
  void *newBuf = my_new(newSize);
  if (oldSize != 0)
    memcpy(newBuf, p, oldSize);
  my_delete(p);
  return newBuf;
}
*/

void *
#ifdef _MSC_VER
__cdecl
#endif
operator new(size_t size)
{
  /* by C++ specification:
       if (size == 0), operator new(size) returns non_NULL pointer.
     If (operator new(0) returns NULL), it's out of specification.
       but some calling code can work correctly even in this case too. */
  // if (size == 0) return NULL; // for debug only. don't use it

  /* malloc(0) returns non_NULL in main compilers, as we need here.
     But specification also allows malloc(0) to return NULL.
     So we change (size=0) to (size=1) here to get real non_NULL pointer */
  if (size == 0)
    size = 1;
  // void *p = ::HeapAlloc(::GetProcessHeap(), 0, size);
  // void *p = ::MyAlloc(size);  // note: MyAlloc(0) returns NULL
  void *p = ::malloc(size);
  if (!p)
    throw CNewException();
  return p;
}


#if defined(_MSC_VER) && _MSC_VER == 1600
// vs2010 has no throw() by default ?
#pragma warning(push)
#pragma warning(disable : 4986) // 'operator delete': exception specification does not match previous declaration
#endif

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p) throw()
{
  // if (!p) return; ::HeapFree(::GetProcessHeap(), 0, p);
  // MyFree(p);
  ::free(p);
}

/* we define operator delete(void *p, size_t n) because
   vs2022 compiler uses delete(void *p, size_t n), and
   we want to mix files from different compilers:
     - old vc6 linker
     - old vc6 complier
     - new vs2022 complier
*/
void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p, size_t n) throw()
{
  UNUSED_VAR(n)
  ::free(p);
}

#if defined(_MSC_VER) && _MSC_VER == 1600
#pragma warning(pop)
#endif

/*
void *
#ifdef _MSC_VER
__cdecl
#endif
operator new[](size_t size)
{
  // void *p = ::HeapAlloc(::GetProcessHeap(), 0, size);
  if (size == 0)
    size = 1;
  void *p = ::malloc(size);
  if (!p)
    throw CNewException();
  return p;
}

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete[](void *p) throw()
{
  // if (!p) return; ::HeapFree(::GetProcessHeap(), 0, p);
  ::free(p);
}
*/

#endif

#else

#include <stdio.h>

// #pragma init_seg(lib)
/*
const int kDebugSize = 1000000;
static void *a[kDebugSize];
static int g_index = 0;

class CC
{
public:
  CC()
  {
    for (int i = 0; i < kDebugSize; i++)
      a[i] = 0;
  }
  ~CC()
  {
    printf("\nDestructor: %d\n", numAllocs);
    for (int i = 0; i < kDebugSize; i++)
      if (a[i] != 0)
        return;
  }
} g_CC;
*/

#ifdef _WIN32
static bool wasInit = false;
static CRITICAL_SECTION cs;
#endif

static int numAllocs = 0;

void *
#ifdef _MSC_VER
__cdecl
#endif
operator new(size_t size)
{
 #ifdef _WIN32
  if (!wasInit)
  {
    InitializeCriticalSection(&cs);
    wasInit = true;
  }
  EnterCriticalSection(&cs);

  numAllocs++;
  int loc = numAllocs;
  void *p = HeapAlloc(GetProcessHeap(), 0, size);
  /*
  if (g_index < kDebugSize)
  {
    a[g_index] = p;
    g_index++;
  }
  */
  printf("Alloc %6d, size = %8u\n", loc, (unsigned)size);
  LeaveCriticalSection(&cs);
  if (!p)
    throw CNewException();
  return p;
 #else
  numAllocs++;
  int loc = numAllocs;
  if (size == 0)
    size = 1;
  void *p = malloc(size);
  /*
  if (g_index < kDebugSize)
  {
    a[g_index] = p;
    g_index++;
  }
  */
  printf("Alloc %6d, size = %8u\n", loc, (unsigned)size);
  if (!p)
    throw CNewException();
  return p;
 #endif
}

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p) throw()
{
  if (!p)
    return;
 #ifdef _WIN32
  EnterCriticalSection(&cs);
  /*
  for (int i = 0; i < g_index; i++)
    if (a[i] == p)
      a[i] = 0;
  */
  HeapFree(GetProcessHeap(), 0, p);
  // if (numAllocs == 0) numAllocs = numAllocs; // ERROR
  numAllocs--;
  // if (numAllocs == 0) numAllocs = numAllocs; // OK: all objects were deleted
  printf("Free %d\n", numAllocs);
  LeaveCriticalSection(&cs);
 #else
  free(p);
  numAllocs--;
  printf("Free %d\n", numAllocs);
 #endif
}

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p, size_t n) throw();
void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p, size_t n) throw()
{
  UNUSED_VAR(n)
  printf("delete_WITH_SIZE=%u, ptr = %p\n", (unsigned)n, p);
  operator delete(p);
}

/*
void *
#ifdef _MSC_VER
__cdecl
#endif
operator new[](size_t size)
{
  printf("operator_new[] : ");
  return operator new(size);
}

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p, size_t sz) throw();

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete(void *p, size_t sz) throw()
{
  if (!p)
    return;
  printf("operator_delete_size : size=%d  : ", (unsigned)sz);
  operator delete(p);
}

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete[](void *p) throw()
{
  if (!p)
    return;
  printf("operator_delete[] : ");
  operator delete(p);
}

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete[](void *p, size_t sz) throw();

void
#ifdef _MSC_VER
__cdecl
#endif
operator delete[](void *p, size_t sz) throw()
{
  if (!p)
    return;
  printf("operator_delete_size[] : size=%d  : ", (unsigned)sz);
  operator delete(p);
}
*/

#endif

/*
int MemErrorVC(size_t)
{
  throw CNewException();
  // return 1;
}
CNewHandlerSetter::CNewHandlerSetter()
{
  // MemErrorOldVCFunction = _set_new_handler(MemErrorVC);
}
CNewHandlerSetter::~CNewHandlerSetter()
{
  // _set_new_handler(MemErrorOldVCFunction);
}
*/

/* ================ unit: CPP/Common/Sha1Prepare.cpp ================ */
// Sha1Prepare.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static struct CSha1Prepare { CSha1Prepare() { Sha1Prepare(); } } g_Sha1Prepare;

/* ================ unit: CPP/Common/Sha256Prepare.cpp ================ */
// Sha256Prepare.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static struct CSha256Prepare { CSha256Prepare() { Sha256Prepare(); } } g_Sha256Prepare;

/* ================ unit: CPP/Common/Sha512Prepare.cpp ================ */
// Sha512Prepare.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static struct CSha512Prepare { CSha512Prepare() { Sha512Prepare(); } } g_Sha512Prepare;

/* ================ unit: CPP/Common/StringConvert.cpp ================ */
// Common/StringConvert.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#ifndef _WIN32
// #include <stdio.h>
#include <stdlib.h>
#endif

#if !defined(_WIN32) || defined(ENV_HAVE_LOCALE)
// amalgamation: header emitted in prologue
#endif

#ifdef ENV_HAVE_LOCALE
#include <locale.h>
#endif

static const char k_DefultChar = '_';

#ifdef _WIN32

/*
MultiByteToWideChar(CodePage, DWORD dwFlags,
    LPCSTR lpMultiByteStr, int cbMultiByte,
    LPWSTR lpWideCharStr, int cchWideChar)

  if (cbMultiByte == 0)
    return: 0. ERR: ERROR_INVALID_PARAMETER

  if (cchWideChar == 0)
    return: the required buffer size in characters.

  if (supplied buffer size was not large enough)
    return: 0. ERR: ERROR_INSUFFICIENT_BUFFER
    The number of filled characters in lpWideCharStr can be smaller than cchWideChar (if last character is complex)

  If there are illegal characters:
    if MB_ERR_INVALID_CHARS is set in dwFlags:
      - the function stops conversion on illegal character.
      - Return: 0. ERR: ERROR_NO_UNICODE_TRANSLATION.
    
    if MB_ERR_INVALID_CHARS is NOT set in dwFlags:
      before Vista: illegal character is dropped (skipped). WinXP-64: GetLastError() returns 0.
      in Vista+:    illegal character is not dropped (MSDN). Undocumented: illegal
                    character is converted to U+FFFD, which is REPLACEMENT CHARACTER.
*/


void MultiByteToUnicodeString2(UString &dest, const AString &src, UINT codePage)
{
  dest.Empty();
  if (src.IsEmpty())
    return;
  {
    /*
    wchar_t *d = dest.GetBuf(src.Len());
    const char *s = (const char *)src;
    unsigned i;
    
    for (i = 0;;)
    {
      Byte c = (Byte)s[i];
      if (c >= 0x80 || c == 0)
        break;
      d[i++] = (wchar_t)c;
    }

    if (i != src.Len())
    {
      unsigned len = MultiByteToWideChar(codePage, 0, s + i,
          src.Len() - i, d + i,
          src.Len() + 1 - i);
      if (len == 0)
        throw 282228;
      i += len;
    }

    d[i] = 0;
    dest.ReleaseBuf_SetLen(i);
    */
    unsigned len = (unsigned)MultiByteToWideChar(codePage, 0, src, (int)src.Len(), NULL, 0);
    if (len == 0)
    {
      if (GetLastError() != 0)
        throw 282228;
    }
    else
    {
      len = (unsigned)MultiByteToWideChar(codePage, 0, src, (int)src.Len(), dest.GetBuf(len), (int)len);
      if (len == 0)
        throw 282228;
      dest.ReleaseBuf_SetEnd(len);
    }
  }
}

/*
  int WideCharToMultiByte(
      UINT CodePage, DWORD dwFlags,
      LPCWSTR lpWideCharStr, int cchWideChar,
      LPSTR lpMultiByteStr, int cbMultiByte,
      LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);

if (lpDefaultChar == NULL),
  - it uses system default value.

if (CodePage == CP_UTF7 || CodePage == CP_UTF8)
  if (lpDefaultChar != NULL || lpUsedDefaultChar != NULL)
    return: 0. ERR: ERROR_INVALID_PARAMETER.

The function operates most efficiently, if (lpDefaultChar == NULL && lpUsedDefaultChar == NULL)

*/

static void UnicodeStringToMultiByte2(AString &dest, const UString &src, UINT codePage, char defaultChar, bool &defaultCharWasUsed)
{
  dest.Empty();
  defaultCharWasUsed = false;
  if (src.IsEmpty())
    return;
  {
    /*
    unsigned numRequiredBytes = src.Len() * 2;
    char *d = dest.GetBuf(numRequiredBytes);
    const wchar_t *s = (const wchar_t *)src;
    unsigned i;
    
    for (i = 0;;)
    {
      wchar_t c = s[i];
      if (c >= 0x80 || c == 0)
        break;
      d[i++] = (char)c;
    }
    
    if (i != src.Len())
    {
      BOOL defUsed = FALSE;
      defaultChar = defaultChar;

      bool isUtf = (codePage == CP_UTF8 || codePage == CP_UTF7);
      unsigned len = WideCharToMultiByte(codePage, 0, s + i, src.Len() - i,
          d + i, numRequiredBytes + 1 - i,
          (isUtf ? NULL : &defaultChar),
          (isUtf ? NULL : &defUsed));
      defaultCharWasUsed = (defUsed != FALSE);
      if (len == 0)
        throw 282229;
      i += len;
    }

    d[i] = 0;
    dest.ReleaseBuf_SetLen(i);
    */

    /*
    if (codePage != CP_UTF7)
    {
      const wchar_t *s = (const wchar_t *)src;
      unsigned i;
      for (i = 0;; i++)
      {
        wchar_t c = s[i];
        if (c >= 0x80 || c == 0)
          break;
      }
      
      if (s[i] == 0)
      {
        char *d = dest.GetBuf(src.Len());
        for (i = 0;;)
        {
          wchar_t c = s[i];
          if (c == 0)
            break;
          d[i++] = (char)c;
        }
        d[i] = 0;
        dest.ReleaseBuf_SetLen(i);
        return;
      }
    }
    */

    unsigned len = (unsigned)WideCharToMultiByte(codePage, 0, src, (int)src.Len(), NULL, 0, NULL, NULL);
    if (len == 0)
    {
      if (GetLastError() != 0)
        throw 282228;
    }
    else
    {
      BOOL defUsed = FALSE;
      bool isUtf = (codePage == CP_UTF8 || codePage == CP_UTF7);
      // defaultChar = defaultChar;
      len = (unsigned)WideCharToMultiByte(codePage, 0, src, (int)src.Len(),
          dest.GetBuf(len), (int)len,
          (isUtf ? NULL : &defaultChar),
          (isUtf ? NULL : &defUsed)
          );
      if (!isUtf)
        defaultCharWasUsed = (defUsed != FALSE);
      if (len == 0)
        throw 282228;
      dest.ReleaseBuf_SetEnd(len);
    }
  }
}

/*
#ifndef UNDER_CE
AString SystemStringToOemString(const CSysString &src)
{
  AString dest;
  const unsigned len = src.Len() * 2;
  CharToOem(src, dest.GetBuf(len));
  dest.ReleaseBuf_CalcLen(len);
  return dest;
}
#endif
*/

#else // _WIN32

// #include <stdio.h>
/*
  if (wchar_t is 32-bit (#if WCHAR_MAX > 0xffff),
      and utf-8 string contains big unicode character > 0xffff),
  then we still use 16-bit surrogate pair in UString.
  It simplifies another code where utf-16 encoding is used.
  So we use surrogate-conversion code only in is file.
*/

/*
   mbstowcs() returns error if there is error in utf-8 stream,
   mbstowcs() returns error if there is single surrogates point (d800-dfff) in utf-8 stream
*/

/*
static void MultiByteToUnicodeString2_Native(UString &dest, const AString &src)
{
  dest.Empty();
  if (src.IsEmpty())
    return;

  const size_t limit = ((size_t)src.Len() + 1) * 2;
  wchar_t *d = dest.GetBuf((unsigned)limit);
  const size_t len = mbstowcs(d, src, limit);
  if (len != (size_t)-1)
  {
    dest.ReleaseBuf_SetEnd((unsigned)len);
    return;
  }
  dest.ReleaseBuf_SetEnd(0);
}
*/

bool g_ForceToUTF8 = true; // false;

void MultiByteToUnicodeString2(UString &dest, const AString &src, UINT codePage)
{
  dest.Empty();
  if (src.IsEmpty())
    return;

  if (codePage == CP_UTF8 || g_ForceToUTF8)
  {
#if 1
    ConvertUTF8ToUnicode(src, dest);
    return;
#endif
  }

  const size_t limit = ((size_t)src.Len() + 1) * 2;
  wchar_t *d = dest.GetBuf((unsigned)limit);
  const size_t len = mbstowcs(d, src, limit);
  if (len != (size_t)-1)
  {
    dest.ReleaseBuf_SetEnd((unsigned)len);

#if WCHAR_MAX > 0xffff
    d = dest.GetBuf();
    for (size_t i = 0;; i++)
    {
      wchar_t c = d[i];
      // printf("\ni=%2d c = %4x\n", (unsigned)i, (unsigned)c);
      if (c == 0)
        break;
      if (c >= 0x10000 && c < 0x110000)
      {
        UString tempString = d + i;
        const wchar_t *t = tempString.Ptr();

        for (;;)
        {
          wchar_t w = *t++;
          // printf("\nchar=%x\n", w);
          if (w == 0)
            break;
          if (i == limit)
            break; // unexpected error
          if (w >= 0x10000 && w < 0x110000)
          {
#if 1
            if (i + 1 == limit)
              break; // unexpected error
            w -= 0x10000;
            d[i++] = (unsigned)0xd800 + (((unsigned)w >> 10) & 0x3ff);
            w = 0xdc00 + (w & 0x3ff);
#else
            // w = '_'; // for debug
#endif
          }
          d[i++] = w;
        }
        dest.ReleaseBuf_SetEnd((unsigned)i);
        break;
      }
    }

#endif
 
    /*
    printf("\nMultiByteToUnicodeString2 (%d) %s\n", (int)src.Len(),  src.Ptr());
    printf("char:    ");
    for (unsigned i = 0; i < src.Len(); i++)
      printf (" %02x", (int)(Byte)src[i]);
    printf("\n");
    printf("\n-> (%d) %ls\n", (int)dest.Len(), dest.Ptr());
    printf("wchar_t: ");
    for (unsigned i = 0; i < dest.Len(); i++)
    {
      printf (" %02x", (int)dest[i]);
    }
    printf("\n");
    */

    return;
  }

  /* if there is mbstowcs() error, we have two ways:
     
     1) change 0x80+ characters to some character: '_'
        in that case we lose data, but we have correct UString()
        and that scheme can show errors to user in early stages,
        when file converted back to mbs() cannot be found

     2) transfer bad characters in some UTF-16 range.
        it can be non-original Unicode character.
        but later we still can restore original character.
  */

  
  // printf("\nmbstowcs  ERROR !!!!!! s=%s\n", src.Ptr());
  {
    unsigned i;
    const char *s = (const char *)src;
    for (i = 0;;)
    {
      Byte c = (Byte)s[i];
      if (c == 0)
        break;
      // we can use ascii compatibilty character '_'
      // if (c > 0x7F) c = '_'; // we replace "bad: character
      d[i++] = (wchar_t)c;
    }
    d[i] = 0;
    dest.ReleaseBuf_SetLen(i);
  }
}

static void UnicodeStringToMultiByte2_Native(AString &dest, const UString &src)
{
  dest.Empty();
  if (src.IsEmpty())
    return;

  const size_t limit = ((size_t)src.Len() + 1) * 6;
  char *d = dest.GetBuf((unsigned)limit);

  const size_t len = wcstombs(d, src, limit);

  if (len != (size_t)-1)
  {
    dest.ReleaseBuf_SetEnd((unsigned)len);
    return;
  }
  dest.ReleaseBuf_SetEnd(0);
}


static void UnicodeStringToMultiByte2(AString &dest, const UString &src2, UINT codePage, char defaultChar, bool &defaultCharWasUsed)
{
  // if (codePage == 1234567) // for debug purposes
  if (codePage == CP_UTF8 || g_ForceToUTF8)
  {
#if 1
    defaultCharWasUsed = false;
    ConvertUnicodeToUTF8(src2, dest);
    return;
#endif
  }

  UString src = src2;
#if WCHAR_MAX > 0xffff
  {
    src.Empty();
    for (unsigned i = 0; i < src2.Len();)
    {
      wchar_t c = src2[i++];
      if (c >= 0xd800 && c < 0xdc00 && i != src2.Len())
      {
        const wchar_t c2 = src2[i];
        if (c2 >= 0xdc00 && c2 < 0xe000)
        {
#if 1
          // printf("\nSurragate [%d]: %4x %4x -> ", i, (int)c, (int)c2);
          c = 0x10000 + ((c & 0x3ff) << 10) + (c2 & 0x3ff);
          // printf("%4x\n", (int)c);
          i++;
#else
          // c = '_'; // for debug
#endif
        }
      }
      src += c;
    }
  }
#endif

  dest.Empty();
  defaultCharWasUsed = false;
  if (src.IsEmpty())
    return;

  const size_t len = wcstombs(NULL, src, 0);

  if (len != (size_t)-1)
  {
    const unsigned limit = ((unsigned)len);
    if (limit == len)
    {
      char *d = dest.GetBuf(limit);

      /*
      {
        printf("\nwcstombs; len = %d %ls \n", (int)src.Len(), src.Ptr());
        for (unsigned i = 0; i < src.Len(); i++)
          printf (" %02x", (int)src[i]);
        printf("\n");
        printf("\ndest Limit = %d \n", limit);
      }
      */

      const size_t len2 = wcstombs(d, src, len + 1);
      
      if (len2 != (size_t)-1 && len2 <= limit)
      {
        /*
        printf("\nOK : destLen = %d : %s\n", (int)len, dest.Ptr());
        for (unsigned i = 0; i < len2; i++)
          printf(" %02x", (int)(Byte)dest[i]);
        printf("\n");
        */
        dest.ReleaseBuf_SetEnd((unsigned)len2);
        return;
      }
    }
  }

  {
    const wchar_t *s = (const wchar_t *)src;
    char *d = dest.GetBuf(src.Len());

    unsigned i;
    for (i = 0;;)
    {
      wchar_t c = s[i];
      if (c == 0)
        break;
      if (c >=
            0x100
            // 0x80
          )
      {
        c = defaultChar;
        defaultCharWasUsed = true;
      }

      d[i++] = (char)c;
    }
    d[i] = 0;
    dest.ReleaseBuf_SetLen(i);
    /*
    printf("\nUnicodeStringToMultiByte2; len = %d \n", (int)src.Len());
    printf("ERROR: %s\n", dest.Ptr());
    */
  }
}

#endif // _WIN32


UString MultiByteToUnicodeString(const AString &src, UINT codePage)
{
  UString dest;
  MultiByteToUnicodeString2(dest, src, codePage);
  return dest;
}

UString MultiByteToUnicodeString(const char *src, UINT codePage)
{
  return MultiByteToUnicodeString(AString(src), codePage);
}


void UnicodeStringToMultiByte2(AString &dest, const UString &src, UINT codePage)
{
  bool defaultCharWasUsed;
  UnicodeStringToMultiByte2(dest, src, codePage, k_DefultChar, defaultCharWasUsed);
}

AString UnicodeStringToMultiByte(const UString &src, UINT codePage, char defaultChar, bool &defaultCharWasUsed)
{
  AString dest;
  UnicodeStringToMultiByte2(dest, src, codePage, defaultChar, defaultCharWasUsed);
  return dest;
}

AString UnicodeStringToMultiByte(const UString &src, UINT codePage)
{
  AString dest;
  bool defaultCharWasUsed;
  UnicodeStringToMultiByte2(dest, src, codePage, k_DefultChar, defaultCharWasUsed);
  return dest;
}




#if !defined(_WIN32) || defined(ENV_HAVE_LOCALE)

#ifdef _WIN32
#define U_to_A(a, b, c)  UnicodeStringToMultiByte2
// #define A_to_U(a, b, c)  MultiByteToUnicodeString2
#else
// void MultiByteToUnicodeString2_Native(UString &dest, const AString &src);
#define U_to_A(a, b, c)  UnicodeStringToMultiByte2_Native(a, b)
// #define A_to_U(a, b, c)  MultiByteToUnicodeString2_Native(a, b)
#endif

bool IsNativeUTF8()
{
  UString u;
  AString a, a2;
  // for (unsigned c = 0x80; c < (UInt32)0x10000; c += (c >> 9) + 1)
  for (unsigned c = 0x80; c < (UInt32)0xD000; c += (c >> 2) + 1)
  {
    u.Empty();
    u += (wchar_t)c;
    /*
    if (Unicode_Is_There_Utf16SurrogateError(u))
      continue;
    #ifndef _WIN32
    if (Unicode_Is_There_BmpEscape(u))
      continue;
    #endif
    */
    ConvertUnicodeToUTF8(u, a);
    U_to_A(a2, u, CP_OEMCP);
    if (a != a2)
      return false;
  }
  return true;
}

#endif


#ifdef ENV_HAVE_LOCALE

const char *GetLocale(void)
{
  #ifdef ENV_HAVE_LOCALE
    // printf("\n\nsetlocale(LC_CTYPE, NULL) : return : ");
    const char *s = setlocale(LC_CTYPE, NULL);
    if (!s)
    {
      // printf("[NULL]\n");
      s = "C";
    }
    else
    {
      // ubuntu returns "C" after program start
      // printf("\"%s\"\n", s);
    }
    return s;
  #elif defined(LOCALE_IS_UTF8)
    return "utf8";
  #else
    return "C";
  #endif
}

#ifdef _WIN32
  static void Set_ForceToUTF8(bool) {}
#else
  static void Set_ForceToUTF8(bool val) { g_ForceToUTF8 = val; }
#endif

static bool Is_Default_Basic_Locale(const char *locale)
{
  const AString a (locale);
  if (a.IsEqualTo_Ascii_NoCase("")
      || a.IsEqualTo_Ascii_NoCase("C")
      || a.IsEqualTo_Ascii_NoCase("POSIX"))
      return true;
  return false;
}

static bool Is_Default_Basic_Locale()
{
  return Is_Default_Basic_Locale(GetLocale());
}


void MY_SetLocale()
{
  #ifdef ENV_HAVE_LOCALE
  /*
  {
    const char *s = GetLocale();
    printf("\nGetLocale() : returned : \"%s\"\n", s);
  }
  */
  
  unsigned start = 0;
  // unsigned lim = 0;
  unsigned lim = 3;

  /*
  #define MY_SET_LOCALE_FLAGS__FROM_ENV 1
  #define MY_SET_LOCALE_FLAGS__TRY_UTF8 2

  unsigned flags =
      MY_SET_LOCALE_FLAGS__FROM_ENV |
      MY_SET_LOCALE_FLAGS__TRY_UTF8

  if (flags != 0)
  {
    if (flags & MY_SET_LOCALE_FLAGS__FROM_ENV)
      lim = (flags & MY_SET_LOCALE_FLAGS__TRY_UTF8) ? 3 : 1;
    else
    {
      start = 1;
      lim = 2;
    }
  }
  */

  for (unsigned i = start; i < lim; i++)
  {
    /*
    man7: "If locale is an empty string, "", each part of the locale that
    should be modified is set according to the environment variables.
    for glibc: glibc, first from the user's environment variables:
      1) the environment variable LC_ALL,
      2) environment variable with the same name as the category (see the
      3) the environment variable LANG
    The locale "C" or "POSIX" is a portable locale; it exists on all conforming systems.
    
    for WIN32 : MSDN :
      Sets the locale to the default, which is the user-default
      ANSI code page obtained from the operating system.
      The locale name is set to the value returned by GetUserDefaultLocaleName.
      The code page is set to the value returned by GetACP
  */
    const char *newLocale = "";
    
    #ifdef __APPLE__
    
    /* look also CFLocale
       there is no C.UTF-8 in macos
       macos has UTF-8 locale only with some language like en_US.UTF-8
       what is best way to set UTF-8 locale in macos? */
    if (i == 1)
      newLocale = "en_US.UTF-8";
   
    /* file open with non-utf8 sequencies return
      #define EILSEQ    92    // "Illegal byte sequence"
    */
#else
    // newLocale = "C";
    if (i == 1)
    {
      newLocale = "C.UTF-8";    // main UTF-8 locale in ubuntu
      // newLocale = ".utf8";    // supported in new Windows 10 build 17134 (April 2018 Update), the Universal C Runtime
      // newLocale = "en_US.utf8"; // supported by ubuntu ?
      // newLocale = "en_US.UTF-8";
      /* setlocale() in ubuntu allows locales with minor chracter changes in strings
        "en_US.UTF-8" /  "en_US.utf8" */
    }
    
#endif
    
    // printf("\nsetlocale(LC_ALL, \"%s\") : returned: ", newLocale);
    
    // const char *s =
    setlocale(LC_ALL, newLocale);
    
    /*
    if (!s)
      printf("NULL: can't set locale");
    else
      printf("\"%s\"\n", s);
    */
    
    // request curent locale of program
    const char *locale = GetLocale();
    if (locale)
    {
      AString a (locale);
      a.MakeLower_Ascii();
      // if (a.Find("utf") >= 0)
      {
        if (IsNativeUTF8())
        {
          Set_ForceToUTF8(true);
          return;
        }
      }
      if (!Is_Default_Basic_Locale(locale))
      {
        // if there is some non-default and non-utf locale, we want to use it
        break; // comment it for debug
      }
    }
  }

  if (IsNativeUTF8())
  {
    Set_ForceToUTF8(true);
    return;
  }

  if (Is_Default_Basic_Locale())
  {
    Set_ForceToUTF8(true);
    return;
  }

  Set_ForceToUTF8(false);

  #elif defined(LOCALE_IS_UTF8)
    // assume LC_CTYPE="utf8"
  #else
    // assume LC_CTYPE="C"
  #endif
}
#endif

/* ================ unit: CPP/Common/StringToInt.cpp ================ */
// Common/StringToInt.cpp

// amalgamation: header emitted in prologue

#include <limits.h>
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#include <stdint.h> // for WCHAR_MAX in vs2022
#endif

// amalgamation: header emitted in prologue

static const UInt32 k_UInt32_max = 0xFFFFFFFF;
static const UInt64 k_UInt64_max = UINT64_CONST(0xFFFFFFFFFFFFFFFF);
// static const UInt64 k_UInt64_max = (UInt64)(Int64)-1;

#define DIGIT_TO_VALUE(charTypeUnsigned, digit)   ((unsigned)(charTypeUnsigned)digit - '0')
// #define DIGIT_TO_VALUE(charTypeUnsigned, digit)   ((unsigned)digit - '0')
// #define DIGIT_TO_VALUE(charTypeUnsigned, digit)   ((unsigned)(digit - '0'))

#define CONVERT_STRING_TO_UINT_FUNC(uintType, charType, charTypeUnsigned) \
uintType ConvertStringTo ## uintType(const charType *s, const charType **end) throw() { \
    if (end) *end = s; \
    uintType res = 0; \
    for (;; s++) { \
      const unsigned v = DIGIT_TO_VALUE(charTypeUnsigned, *s); \
      if (v > 9) { if (end) *end = s; return res; } \
      if (res > (k_ ## uintType ## _max) / 10) return 0; \
      res *= 10; \
      if (res > (k_ ## uintType ## _max) - v) return 0; \
      res += v; }}

// arm-linux-gnueabi GCC compilers give (WCHAR_MAX > UINT_MAX) by some unknown reason
// so we don't use this branch
#if 0 && WCHAR_MAX > UINT_MAX
/*
   if (sizeof(wchar_t) > sizeof(unsigned)
      we must use CONVERT_STRING_TO_UINT_FUNC_SLOW
   But we just stop compiling instead.
   We need some real cases to test this code.
*/
#error Stop_Compiling_WCHAR_MAX_IS_LARGER_THAN_UINT_MAX
#define CONVERT_STRING_TO_UINT_FUNC_SLOW(uintType, charType, charTypeUnsigned) \
uintType ConvertStringTo ## uintType(const charType *s, const charType **end) throw() { \
    if (end) *end = s; \
    uintType res = 0; \
    for (;; s++) { \
      const charTypeUnsigned c = (charTypeUnsigned)*s; \
      if (c < '0' || c > '9') { if (end) *end = s; return res; } \
      if (res > (k_ ## uintType ## _max) / 10) return 0; \
      res *= 10; \
      const unsigned v = (unsigned)(c - '0'); \
      if (res > (k_ ## uintType ## _max) - v) return 0; \
      res += v; }}
#endif


CONVERT_STRING_TO_UINT_FUNC(UInt32, char, Byte)
CONVERT_STRING_TO_UINT_FUNC(UInt32, wchar_t, wchar_t)
CONVERT_STRING_TO_UINT_FUNC(UInt64, char, Byte)
CONVERT_STRING_TO_UINT_FUNC(UInt64, wchar_t, wchar_t)

Int32 ConvertStringToInt32(const wchar_t *s, const wchar_t **end) throw()
{
  if (end)
    *end = s;
  const wchar_t *s2 = s;
  if (*s == '-')
    s2++;
  const wchar_t *end2;
  UInt32 res = ConvertStringToUInt32(s2, &end2);
  if (s2 == end2)
    return 0;
  if (s != s2)
  {
    if (res > (UInt32)1 << (32 - 1))
      return 0;
    res = 0 - res;
  }
  else
  {
    if (res & (UInt32)1 << (32 - 1))
      return 0;
  }
  if (end)
    *end = end2;
  return (Int32)res;
}


#define CONVERT_OCT_STRING_TO_UINT_FUNC(uintType) \
uintType ConvertOctStringTo ## uintType(const char *s, const char **end) throw() \
{ \
  if (end) *end = s; \
  uintType res = 0; \
  for (;; s++) { \
    const unsigned c = (unsigned)(Byte)*s - '0'; \
    if (c > 7) { \
      if (end) \
        *end = s; \
      return res; \
    } \
    if (res & (uintType)7 << (sizeof(uintType) * 8 - 3)) \
      return 0; \
    res <<= 3; \
    res |= c; \
  } \
}

CONVERT_OCT_STRING_TO_UINT_FUNC(UInt32)
CONVERT_OCT_STRING_TO_UINT_FUNC(UInt64)


#define CONVERT_HEX_STRING_TO_UINT_FUNC(uintType) \
uintType ConvertHexStringTo ## uintType(const char *s, const char **end) throw() \
{ \
  if (end) *end = s; \
  uintType res = 0; \
  for (;; s++) { \
    unsigned c = (unsigned)(Byte)*s; \
    Z7_PARSE_HEX_DIGIT(c, { if (end) *end = s;  return res; }) \
    if (res & (uintType)0xF << (sizeof(uintType) * 8 - 4)) \
      return 0; \
    res <<= 4; \
    res |= c; \
  } \
}

CONVERT_HEX_STRING_TO_UINT_FUNC(UInt32)
CONVERT_HEX_STRING_TO_UINT_FUNC(UInt64)

const char *FindNonHexChar(const char *s) throw()
{
  for (;;)
  {
    unsigned c = (Byte)*s++; // pointer can go 1 byte after end
    c -= '0';
    if (c <= 9)
      continue;
    c -= 'A' - '0';
    c &= ~0x20u;
    if (c > 5)
      return s - 1;
  }
}

Byte *ParseHexString(const char *s, Byte *dest) throw()
{
  for (;;)
  {
    unsigned v0 = (Byte)s[0];         Z7_PARSE_HEX_DIGIT(v0, return dest;)
    unsigned v1 = (Byte)s[1]; s += 2; Z7_PARSE_HEX_DIGIT(v1, return dest;)
    *dest++ = (Byte)(v1 | (v0 << 4));
  }
}

/* ================ unit: CPP/Common/UTFConvert.cpp ================ */
// UTFConvert.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue


#ifndef Z7_WCHART_IS_16BIT
#ifndef __APPLE__
  // we define it if the system supports files with non-utf8 symbols:
  #define MY_UTF8_RAW_NON_UTF8_SUPPORTED
#endif
#endif

/*
  MY_UTF8_START(n) - is a base value for start byte (head), if there are (n) additional bytes after start byte
  
  n : MY_UTF8_START(n) : Bits of code point

  0 : 0x80 :    : unused
  1 : 0xC0 : 11 :
  2 : 0xE0 : 16 : Basic Multilingual Plane
  3 : 0xF0 : 21 : Unicode space
  4 : 0xF8 : 26 :
  5 : 0xFC : 31 : UCS-4 : wcstombs() in ubuntu is limited to that value
  6 : 0xFE : 36 : We can use it, if we want to encode any 32-bit value
  7 : 0xFF :
*/

#define MY_UTF8_START(n) (0x100 - (1 << (7 - (n))))

#define MY_UTF8_HEAD_PARSE2(n) \
    if (c < MY_UTF8_START((n) + 1)) \
    { numBytes = (n); val -= MY_UTF8_START(n); }

#ifndef Z7_WCHART_IS_16BIT

/*
   if (wchar_t is 32-bit), we can support large points in long UTF-8 sequence,
   when we convert wchar_t strings to UTF-8:
     (_UTF8_NUM_TAIL_BYTES_MAX == 3) : (21-bits points) - Unicode
     (_UTF8_NUM_TAIL_BYTES_MAX == 5) : (31-bits points) - UCS-4
     (_UTF8_NUM_TAIL_BYTES_MAX == 6) : (36-bit hack)
*/

#define MY_UTF8_NUM_TAIL_BYTES_MAX 5
#endif

/*
#define MY_UTF8_HEAD_PARSE \
    UInt32 val = c; \
         MY_UTF8_HEAD_PARSE2(1) \
    else MY_UTF8_HEAD_PARSE2(2) \
    else MY_UTF8_HEAD_PARSE2(3) \
    else MY_UTF8_HEAD_PARSE2(4) \
    else MY_UTF8_HEAD_PARSE2(5) \
  #if MY_UTF8_NUM_TAIL_BYTES_MAX >= 6
    else MY_UTF8_HEAD_PARSE2(6)
  #endif
*/

#define MY_UTF8_HEAD_PARSE_MAX_3_BYTES \
    UInt32 val = c; \
         MY_UTF8_HEAD_PARSE2(1) \
    else MY_UTF8_HEAD_PARSE2(2) \
    else { numBytes = 3; val -= MY_UTF8_START(3); }


#define MY_UTF8_RANGE(n) (((UInt32)1) << ((n) * 5 + 6))


#define START_POINT_FOR_SURROGATE 0x10000


/* we use 128 bytes block in 16-bit BMP-PLANE to encode non-UTF-8 Escapes
   Also we can use additional HIGH-PLANE (we use 21-bit points above 0x1f0000)
   to simplify internal intermediate conversion in Linux:
   RAW-UTF-8 <-> internal wchar_t utf-16 strings <-> RAW-UTF-UTF-8
*/
 

#if defined(Z7_WCHART_IS_16BIT)

#define UTF_ESCAPE_PLANE 0

#else

/*
we can place 128 ESCAPE chars to
   ef 80 -    ee be 80 (3-bytes utf-8) : similar to WSL
   ef ff -    ee bf bf

1f ef 80 - f7 be be 80 (4-bytes utf-8) : last  4-bytes utf-8 plane (out of Unicode)
1f ef ff - f7 be bf bf (4-bytes utf-8) : last  4-bytes utf-8 plane (out of Unicode)
*/

// #define UTF_ESCAPE_PLANE_HIGH  (0x1f << 16)
// #define UTF_ESCAPE_PLANE        UTF_ESCAPE_PLANE_HIGH
#define UTF_ESCAPE_PLANE 0

/*
  if (Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE is set)
  {
    if (UTF_ESCAPE_PLANE is UTF_ESCAPE_PLANE_HIGH)
    {
      we can restore any 8-bit Escape from ESCAPE-PLANE-21 plane.
      But ESCAPE-PLANE-21 point cannot be stored to utf-16 (7z archive)
      So we still need a way to extract 8-bit Escapes and BMP-Escapes-8
      from same BMP-Escapes-16 stored in 7z.
      And if we want to restore any 8-bit from 7z archive,
      we still must use Z7_UTF_FLAG_FROM_UTF8_BMP_ESCAPE_CONVERT for (utf-8 -> utf-16)
      Also we need additional Conversions to tranform from utf-16 to utf-16-With-Escapes-21
    }
    else (UTF_ESCAPE_PLANE == 0)
    {
      we must convert original 3-bytes utf-8 BMP-Escape point to sequence
      of 3 BMP-Escape-16 points with Z7_UTF_FLAG_FROM_UTF8_BMP_ESCAPE_CONVERT
      so we can extract original RAW-UTF-8 from UTFD-16 later.
    }
  }
*/

#endif



#define UTF_ESCAPE_BASE 0xef00


#ifdef UTF_ESCAPE_BASE
#define IS_ESCAPE_POINT(v, plane) (((v) & (UInt32)0xffffff80) == (plane) + UTF_ESCAPE_BASE + 0x80)
#endif

#define IS_SURROGATE_POINT(v)     (((v) & (UInt32)0xfffff800) == 0xd800)
#define IS_LOW_SURROGATE_POINT(v) (((v) & (UInt32)0xfffffc00) == 0xdc00)


#define UTF_ERROR_UTF8_CHECK \
  { NonUtf = true; continue; }

void CUtf8Check::Check_Buf(const char *src, size_t size) throw()
{
  Clear();
  // Byte maxByte = 0;

  for (;;)
  {
    if (size == 0)
      break;

    const Byte c = (Byte)(*src++);
    size--;

    if (c == 0)
    {
      ZeroChar = true;
      continue;
    }

    /*
    if (c > maxByte)
      maxByte = c;
    */

    if (c < 0x80)
      continue;
    
    if (c < 0xc0 + 2)
      UTF_ERROR_UTF8_CHECK

    unsigned numBytes;
    UInt32 val = c;
         MY_UTF8_HEAD_PARSE2(1)
    else MY_UTF8_HEAD_PARSE2(2)
    else MY_UTF8_HEAD_PARSE2(3)
    else MY_UTF8_HEAD_PARSE2(4)
    else MY_UTF8_HEAD_PARSE2(5)
    else
    {
      UTF_ERROR_UTF8_CHECK
    }

    unsigned pos = 0;
    do
    {
      if (pos == size)
        break;
      unsigned c2 = (Byte)src[pos];
      c2 -= 0x80;
      if (c2 >= 0x40)
        break;
      val <<= 6;
      val |= c2;
      if (pos == 0)
        if (val < (((unsigned)1 << 7) >> numBytes))
          break;
      pos++;
    }
    while (--numBytes);

    if (numBytes != 0)
    {
      if (pos == size)
        Truncated = true;
      else
        UTF_ERROR_UTF8_CHECK
    }

    #ifdef UTF_ESCAPE_BASE
      if (IS_ESCAPE_POINT(val, 0))
        Escape = true;
    #endif

    if (MaxHighPoint < val)
      MaxHighPoint = val;

    if (IS_SURROGATE_POINT(val))
      SingleSurrogate = true;
    
    src += pos;
    size -= pos;
  }

  // MaxByte = maxByte;
}

bool Check_UTF8_Buf(const char *src, size_t size, bool allowReduced) throw()
{
  CUtf8Check check;
  check.Check_Buf(src, size);
  return check.IsOK(allowReduced);
}

/*
bool CheckUTF8_chars(const char *src, bool allowReduced) throw()
{
  CUtf8Check check;
  check.CheckBuf(src, strlen(src));
  return check.IsOK(allowReduced);
}
*/

bool CheckUTF8_AString(const AString &s) throw()
{
  CUtf8Check check;
  check.Check_AString(s);
  return check.IsOK();
}


/*
bool CheckUTF8(const char *src, bool allowReduced) throw()
{
  // return Check_UTF8_Buf(src, strlen(src), allowReduced);

  for (;;)
  {
    const Byte c = (Byte)(*src++);
    if (c == 0)
      return true;

    if (c < 0x80)
      continue;
    if (c < 0xC0 + 2 || c >= 0xf5)
      return false;
    
    unsigned numBytes;
    MY_UTF8_HEAD_PARSE
    else
      return false;

    unsigned pos = 0;
    
    do
    {
      Byte c2 = (Byte)(*src++);
      if (c2 < 0x80 || c2 >= 0xC0)
        return allowReduced && c2 == 0;
      val <<= 6;
      val |= (c2 - 0x80);
      pos++;
    }
    while (--numBytes);

    if (val < MY_UTF8_RANGE(pos - 1))
      return false;

    if (val >= 0x110000)
      return false;
  }
}
*/

// in case of UTF-8 error we have two ways:
// 21.01- : old : 0xfffd: REPLACEMENT CHARACTER : old version
// 21.02+ : new : 0xef00 + (c) : similar to WSL scheme for low symbols

#define UTF_REPLACEMENT_CHAR  0xfffd



#define UTF_ESCAPE(c) \
   ((flags & Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE) ? \
    UTF_ESCAPE_PLANE + UTF_ESCAPE_BASE + (c) : UTF_REPLACEMENT_CHAR)

/*
#define UTF_HARD_ERROR_UTF8
  { if (dest) dest[destPos] = (wchar_t)UTF_ESCAPE(c); \
    destPos++; ok = false; continue; }
*/

// we ignore utf errors, and don't change (ok) variable!

#define UTF_ERROR_UTF8 \
  { if (dest) dest[destPos] = (wchar_t)UTF_ESCAPE(c); \
    destPos++; continue; }

// we store UTF-16 in wchar_t strings. So we use surrogates for big unicode points:

// for debug puposes only we can store UTF-32 in wchar_t:
// #define START_POINT_FOR_SURROGATE ((UInt32)0 - 1)


/*
  WIN32 MultiByteToWideChar(CP_UTF8) emits 0xfffd point, if utf-8 error was found.
  Ant it can emit single 0xfffd from 2 src bytes.
  It doesn't emit single 0xfffd from 3-4 src bytes.
  We can
    1) emit Escape point for each incorrect byte. So we can data recover later
    2) emit 0xfffd for each incorrect byte.
       That scheme is similar to Escape scheme, but we emit 0xfffd
       instead of each Escape point.
    3) emit single 0xfffd from 1-2 incorrect bytes, as WIN32 MultiByteToWideChar scheme
*/

static bool Utf8_To_Utf16(wchar_t *dest, size_t *destLen, const char *src, const char *srcLim, unsigned flags) throw()
{
  size_t destPos = 0;
  bool ok = true;

  for (;;)
  {
    if (src == srcLim)
    {
      *destLen = destPos;
      return ok;
    }
    
    const Byte c = (Byte)(*src++);

    if (c < 0x80)
    {
      if (dest)
        dest[destPos] = (wchar_t)c;
      destPos++;
      continue;
    }
    
    if (c < 0xc0 + 2
      || c >= 0xf5) // it's limit for 0x140000 unicode codes : win32 compatibility
    {
      UTF_ERROR_UTF8
    }

    unsigned numBytes;

    MY_UTF8_HEAD_PARSE_MAX_3_BYTES

    unsigned pos = 0;
    do
    {
      if (src + pos == srcLim)
        break;
      unsigned c2 = (Byte)src[pos];
      c2 -= 0x80;
      if (c2 >= 0x40)
        break;
      val <<= 6;
      val |= c2;
      pos++;
      if (pos == 1)
      {
        if (val < (((unsigned)1 << 7) >> numBytes))
          break;
        if (numBytes == 2)
        {
          if (flags & Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR)
            if ((val & (0xF800 >> 6)) == (0xd800 >> 6))
              break;
        }
        else if (numBytes == 3 && val >= (0x110000 >> 12))
          break;
      }
    }
    while (--numBytes);

    if (numBytes != 0)
    {
      if ((flags & Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE) == 0)
      {
        // the following code to emit the 0xfffd chars as win32 Utf8 function.
        // disable the folling line, if you need 0xfffd for each incorrect byte as in Escape mode
        src += pos;
      }
      UTF_ERROR_UTF8
    }

    /*
    if (val < MY_UTF8_RANGE(pos - 1))
      UTF_ERROR_UTF8
    */

    #ifdef UTF_ESCAPE_BASE
    
      if ((flags & Z7_UTF_FLAG_FROM_UTF8_BMP_ESCAPE_CONVERT)
          && IS_ESCAPE_POINT(val, 0))
      {
        // We will emit 3 utf16-Escape-16-21 points from one Escape-16 point (3 bytes)
        UTF_ERROR_UTF8
      }
    
    #endif

    /*
       We don't expect virtual Escape-21 points in UTF-8 stream.
       And we don't check for Escape-21.
       So utf8-Escape-21 will be converted to another 3 utf16-Escape-21 points.
       Maybe we could convert virtual utf8-Escape-21 to one utf16-Escape-21 point in some cases?
    */
    
    if (val < START_POINT_FOR_SURROGATE)
    {
      /*
      if ((flags & Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR)
          && IS_SURROGATE_POINT(val))
      {
        // We will emit 3 utf16-Escape-16-21 points from one Surrogate-16 point (3 bytes)
        UTF_ERROR_UTF8
      }
      */
      if (dest)
        dest[destPos] = (wchar_t)val;
      destPos++;
    }
    else
    {
      /*
      if (val >= 0x110000)
      {
        // We will emit utf16-Escape-16-21 point from each source byte
        UTF_ERROR_UTF8
      }
      */
      if (dest)
      {
        dest[destPos + 0] = (wchar_t)(0xd800 - (0x10000 >> 10) + (val >> 10));
        dest[destPos + 1] = (wchar_t)(0xdc00 + (val & 0x3ff));
      }
      destPos += 2;
    }
    src += pos;
  }
}



#define MY_UTF8_HEAD(n, val) ((char)(MY_UTF8_START(n) + (val >> (6 * (n)))))
#define MY_UTF8_CHAR(n, val) ((char)(0x80 + (((val) >> (6 * (n))) & 0x3F)))

static size_t Utf16_To_Utf8_Calc(const wchar_t *src, const wchar_t *srcLim, unsigned flags)
{
  size_t size = (size_t)(srcLim - src);
  for (;;)
  {
    if (src == srcLim)
      return size;
    
    UInt32 val = (UInt32)(*src++);
   
    if (val < 0x80)
      continue;

    if (val < MY_UTF8_RANGE(1))
    {
      size++;
      continue;
    }

    #ifdef UTF_ESCAPE_BASE
    
    #if UTF_ESCAPE_PLANE != 0
    if (flags & Z7_UTF_FLAG_TO_UTF8_PARSE_HIGH_ESCAPE)
      if (IS_ESCAPE_POINT(val, UTF_ESCAPE_PLANE))
        continue;
    #endif
    
    if (flags & Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE)
      if (IS_ESCAPE_POINT(val, 0))
        continue;
    
    #endif

    if (IS_SURROGATE_POINT(val))
    {
      // it's hack to UTF-8 encoding

      if (val < 0xdc00 && src != srcLim)
      {
        const UInt32 c2 = (UInt32)*src;
        if (c2 >= 0xdc00 && c2 < 0xe000)
          src++;
      }
      size += 2;
      continue;
    }

    #ifdef Z7_WCHART_IS_16BIT
    
    size += 2;
    
    #else

         if (val < MY_UTF8_RANGE(2)) size += 2;
    else if (val < MY_UTF8_RANGE(3)) size += 3;
    else if (val < MY_UTF8_RANGE(4)) size += 4;
    else if (val < MY_UTF8_RANGE(5)) size += 5;
    else
    #if MY_UTF8_NUM_TAIL_BYTES_MAX >= 6
      size += 6;
    #else
      size += 3;
    #endif
    
    #endif
  }
}


static char *Utf16_To_Utf8(char *dest, const wchar_t *src, const wchar_t *srcLim, unsigned flags)
{
  for (;;)
  {
    if (src == srcLim)
      return dest;
    
    UInt32 val = (UInt32)*src++;
    
    if (val < 0x80)
    {
      *dest++ = (char)val;
      continue;
    }

    if (val < MY_UTF8_RANGE(1))
    {
      dest[0] = MY_UTF8_HEAD(1, val);
      dest[1] = MY_UTF8_CHAR(0, val);
      dest += 2;
      continue;
    }

    #ifdef UTF_ESCAPE_BASE
    
    #if UTF_ESCAPE_PLANE != 0
    /*
       if (wchar_t is 32-bit)
            && (Z7_UTF_FLAG_TO_UTF8_PARSE_HIGH_ESCAPE is set)
            && (point is virtual escape plane)
          we extract 8-bit byte from virtual HIGH-ESCAPE PLANE.
    */
    if (flags & Z7_UTF_FLAG_TO_UTF8_PARSE_HIGH_ESCAPE)
      if (IS_ESCAPE_POINT(val, UTF_ESCAPE_PLANE))
      {
        *dest++ = (char)(val);
        continue;
      }
    #endif // UTF_ESCAPE_PLANE != 0

    /* if (Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE is defined)
          we extract 8-bit byte from BMP-ESCAPE PLANE. */

    if (flags & Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE)
      if (IS_ESCAPE_POINT(val, 0))
      {
        *dest++ = (char)(val);
        continue;
      }
    
    #endif // UTF_ESCAPE_BASE

    if (IS_SURROGATE_POINT(val))
    {
      // it's hack to UTF-8 encoding
      if (val < 0xdc00 && src != srcLim)
      {
        const UInt32 c2 = (UInt32)*src;
        if (IS_LOW_SURROGATE_POINT(c2))
        {
          src++;
          val = (((val - 0xd800) << 10) | (c2 - 0xdc00)) + 0x10000;
          dest[0] = MY_UTF8_HEAD(3, val);
          dest[1] = MY_UTF8_CHAR(2, val);
          dest[2] = MY_UTF8_CHAR(1, val);
          dest[3] = MY_UTF8_CHAR(0, val);
          dest += 4;
          continue;
        }
      }
      if (flags & Z7_UTF_FLAG_TO_UTF8_SURROGATE_ERROR)
        val = UTF_REPLACEMENT_CHAR; // WIN32 function does it
    }

    #ifndef Z7_WCHART_IS_16BIT
    if (val < MY_UTF8_RANGE(2))
    #endif
    {
      dest[0] = MY_UTF8_HEAD(2, val);
      dest[1] = MY_UTF8_CHAR(1, val);
      dest[2] = MY_UTF8_CHAR(0, val);
      dest += 3;
      continue;
    }
    
    #ifndef Z7_WCHART_IS_16BIT

    // we don't expect this case. so we can throw exception
    // throw 20210407;
   
    char b;
    unsigned numBits;
         if (val < MY_UTF8_RANGE(3)) { numBits = 6 * 3; b = MY_UTF8_HEAD(3, val); }
    else if (val < MY_UTF8_RANGE(4)) { numBits = 6 * 4; b = MY_UTF8_HEAD(4, val); }
    else if (val < MY_UTF8_RANGE(5)) { numBits = 6 * 5; b = MY_UTF8_HEAD(5, val); }
    #if MY_UTF8_NUM_TAIL_BYTES_MAX >= 6
    else                           { numBits = 6 * 6; b = (char)MY_UTF8_START(6); }
    #else
    else
    {
      val = UTF_REPLACEMENT_CHAR;
                                   { numBits = 6 * 3; b = MY_UTF8_HEAD(3, val); }
    }
    #endif

    *dest++ = b;
    
    do
    {
      numBits -= 6;
      *dest++ = (char)(0x80 + ((val >> numBits) & 0x3F));
    }
    while (numBits != 0);

    #endif
  }
}

bool Convert_UTF8_Buf_To_Unicode(const char *src, size_t srcSize, UString &dest, unsigned flags)
{
  dest.Empty();
  size_t destLen = 0;
  Utf8_To_Utf16(NULL, &destLen, src, src + srcSize, flags);
  bool res = Utf8_To_Utf16(dest.GetBuf((unsigned)destLen), &destLen, src, src + srcSize, flags);
  dest.ReleaseBuf_SetEnd((unsigned)destLen);
  return res;
}

bool ConvertUTF8ToUnicode_Flags(const AString &src, UString &dest, unsigned flags)
{
  return Convert_UTF8_Buf_To_Unicode(src, src.Len(), dest,  flags);
}


static
unsigned g_UTF8_To_Unicode_Flags =
    Z7_UTF_FLAG_FROM_UTF8_USE_ESCAPE
  #ifndef Z7_WCHART_IS_16BIT
    | Z7_UTF_FLAG_FROM_UTF8_SURROGATE_ERROR
  #ifdef MY_UTF8_RAW_NON_UTF8_SUPPORTED
    | Z7_UTF_FLAG_FROM_UTF8_BMP_ESCAPE_CONVERT
  #endif
  #endif
    ;
    

/*
bool ConvertUTF8ToUnicode_boolRes(const AString &src, UString &dest)
{
  return ConvertUTF8ToUnicode_Flags(src, dest, g_UTF8_To_Unicode_Flags);
}
*/

bool ConvertUTF8ToUnicode(const AString &src, UString &dest)
{
  return ConvertUTF8ToUnicode_Flags(src, dest, g_UTF8_To_Unicode_Flags);
}

void Print_UString(const UString &a);

void ConvertUnicodeToUTF8_Flags(const UString &src, AString &dest, unsigned flags)
{
  /*
  if (src.Len()== 24)
    throw "202104";
  */
  dest.Empty();
  const size_t destLen = Utf16_To_Utf8_Calc(src, src.Ptr(src.Len()), flags);
  char *destStart = dest.GetBuf((unsigned)destLen);
  const char *destEnd = Utf16_To_Utf8(destStart, src, src.Ptr(src.Len()), flags);
  dest.ReleaseBuf_SetEnd((unsigned)destLen);
  // printf("\nlen = %d\n", src.Len());
  if (destLen != (size_t)(destEnd - destStart))
  {
    /*
    // dest.ReleaseBuf_SetEnd((unsigned)(destEnd - destStart));
    printf("\nlen = %d\n", (unsigned)destLen);
    printf("\n(destEnd - destStart) = %d\n", (unsigned)(destEnd - destStart));
    printf("\n");
    // Print_UString(src);
    printf("\n");
    // printf("\nlen = %d\n", destLen);
    */
    throw 20210406;
  }
}



unsigned g_Unicode_To_UTF8_Flags =
      // Z7_UTF_FLAG_TO_UTF8_PARSE_HIGH_ESCAPE
      0
  #ifndef _WIN32
    #ifdef MY_UTF8_RAW_NON_UTF8_SUPPORTED
      | Z7_UTF_FLAG_TO_UTF8_EXTRACT_BMP_ESCAPE
    #else
      | Z7_UTF_FLAG_TO_UTF8_SURROGATE_ERROR
    #endif
  #endif
    ;

void ConvertUnicodeToUTF8(const UString &src, AString &dest)
{
  ConvertUnicodeToUTF8_Flags(src, dest, g_Unicode_To_UTF8_Flags);
}

void Convert_Unicode_To_UTF8_Buf(const UString &src, CByteBuffer &dest)
{
  const unsigned flags = g_Unicode_To_UTF8_Flags;
  dest.Free();
  const size_t destLen = Utf16_To_Utf8_Calc(src, src.Ptr(src.Len()), flags);
  dest.Alloc(destLen);
  const char *destEnd = Utf16_To_Utf8((char *)(void *)(Byte *)dest, src, src.Ptr(src.Len()), flags);
  if (destLen != (size_t)(destEnd - (char *)(void *)(Byte *)dest))
    throw 202104;
}

/*

#ifndef _WIN32
void Convert_UTF16_To_UTF32(const UString &src, UString &dest)
{
  dest.Empty();
  for (size_t i = 0; i < src.Len();)
  {
    wchar_t c = src[i++];
    if (c >= 0xd800 && c < 0xdc00 && i < src.Len())
    {
      const wchar_t c2 = src[i];
      if (c2 >= 0xdc00 && c2 < 0xe000)
      {
        // printf("\nSurragate [%d]: %4x %4x -> ", i, (int)c, (int)c2);
        c = 0x10000 + ((c & 0x3ff) << 10) + (c2 & 0x3ff);
        // printf("%4x\n", (int)c);
        i++;
      }
    }
    dest += c;
  }
}

void Convert_UTF32_To_UTF16(const UString &src, UString &dest)
{
  dest.Empty();
  for (size_t i = 0; i < src.Len();)
  {
    wchar_t w = src[i++];
    if (w >= 0x10000 && w < 0x110000)
    {
      w -= 0x10000;
      dest += (wchar_t)((unsigned)0xd800 + (((unsigned)w >> 10) & 0x3ff));
      w = 0xdc00 + (w & 0x3ff);
    }
    dest += w;
  }
}

bool UTF32_IsThere_BigPoint(const UString &src)
{
  for (size_t i = 0; i < src.Len();)
  {
    const UInt32 c = (UInt32)src[i++];
    if (c >= 0x110000)
      return true;
  }
  return false;
}

bool Unicode_IsThere_BmpEscape(const UString &src)
{
  for (size_t i = 0; i < src.Len();)
  {
    const UInt32 c = (UInt32)src[i++];
    if (IS_ESCAPE_POINT(c, 0))
      return true;
  }
  return false;
}


#endif

bool Unicode_IsThere_Utf16SurrogateError(const UString &src)
{
  for (size_t i = 0; i < src.Len();)
  {
    const UInt32 val = (UInt32)src[i++];
    if (IS_SURROGATE_POINT(val))
    {
      // it's hack to UTF-8 encoding
      if (val >= 0xdc00 || i == src.Len())
        return true;
      const UInt32 c2 = (UInt32)*src;
      if (!IS_LOW_SURROGATE_POINT(c2))
        return true;
    }
  }
  return false;
}
*/

#ifndef Z7_WCHART_IS_16BIT

void Convert_UnicodeEsc16_To_UnicodeEscHigh
#if UTF_ESCAPE_PLANE == 0
    (UString &) {}
#else
    (UString &s)
{
  const unsigned len = s.Len();
  for (unsigned i = 0; i < len; i++)
  {
    wchar_t c = s[i];
    if (IS_ESCAPE_POINT(c, 0))
    {
      c += UTF_ESCAPE_PLANE;
      s.ReplaceOneCharAtPos(i, c);
    }
  }
}
#endif
#endif

/* ================ unit: CPP/Common/Wildcard.cpp ================ */
// Common/Wildcard.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

extern
bool g_CaseSensitive;
bool g_CaseSensitive =
  #ifdef _WIN32
    false;
  #elif defined (__APPLE__)
    #ifdef TARGET_OS_IPHONE
      true;
    #else
      false;
    #endif
  #else
    true;
  #endif


bool IsPath1PrefixedByPath2(const wchar_t *s1, const wchar_t *s2)
{
  if (g_CaseSensitive)
    return IsString1PrefixedByString2(s1, s2);
  return IsString1PrefixedByString2_NoCase(s1, s2);
}

// #include <stdio.h>

/*
static int MyStringCompare_PathLinux(const wchar_t *s1, const wchar_t *s2) throw()
{
  for (;;)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2)
    {
      if (c1 == 0) return -1;
      if (c2 == 0) return 1;
      if (c1 == '/') c1 = 0;
      if (c2 == '/') c2 = 0;
      if (c1 < c2) return -1;
      if (c1 > c2) return 1;
      continue;
    }
    if (c1 == 0) return 0;
  }
}
*/

static int MyStringCompare_Path(const wchar_t *s1, const wchar_t *s2) throw()
{
  for (;;)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2)
    {
      if (c1 == 0) return -1;
      if (c2 == 0) return 1;
      if (IS_PATH_SEPAR(c1)) c1 = 0;
      if (IS_PATH_SEPAR(c2)) c2 = 0;
      if (c1 < c2) return -1;
      if (c1 > c2) return 1;
      continue;
    }
    if (c1 == 0) return 0;
  }
}

static int MyStringCompareNoCase_Path(const wchar_t *s1, const wchar_t *s2) throw()
{
  for (;;)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2)
    {
      if (c1 == 0) return -1;
      if (c2 == 0) return 1;
      if (IS_PATH_SEPAR(c1)) c1 = 0;
      if (IS_PATH_SEPAR(c2)) c2 = 0;
      c1 = MyCharUpper(c1);
      c2 = MyCharUpper(c2);
      if (c1 < c2) return -1;
      if (c1 > c2) return 1;
      continue;
    }
    if (c1 == 0) return 0;
  }
}

int CompareFileNames(const wchar_t *s1, const wchar_t *s2) STRING_UNICODE_THROW
{
  /*
  printf("\nCompareFileNames");
  printf("\n S1: %ls", s1);
  printf("\n S2: %ls", s2);
  printf("\n");
  */
  // 21.07 : we parse PATH_SEPARATOR so: 0 < PATH_SEPARATOR < 1
  if (g_CaseSensitive)
    return MyStringCompare_Path(s1, s2);
  return MyStringCompareNoCase_Path(s1, s2);
}

#ifndef USE_UNICODE_FSTRING
int CompareFileNames(const char *s1, const char *s2)
{
  const UString u1 = fs2us(s1);
  const UString u2 = fs2us(s2);
  return CompareFileNames(u1, u2);
}
#endif

// -----------------------------------------
// this function compares name with mask
// ? - any char
// * - any char or empty

static bool EnhancedMaskTest(const wchar_t *mask, const wchar_t *name)
{
  for (;;)
  {
    const wchar_t m = *mask;
    const wchar_t c = *name;
    if (m == 0)
      return (c == 0);
    if (m == '*')
    {
      if (EnhancedMaskTest(mask + 1, name))
        return true;
      if (c == 0)
        return false;
    }
    else
    {
      if (m == '?')
      {
        if (c == 0)
          return false;
      }
      else if (m != c)
        if (g_CaseSensitive || MyCharUpper(m) != MyCharUpper(c))
          return false;
      mask++;
    }
    name++;
  }
}

// --------------------------------------------------
// Splits path to strings

void SplitPathToParts(const UString &path, UStringVector &pathParts)
{
  pathParts.Clear();
  unsigned len = path.Len();
  if (len == 0)
    return;
  UString name;
  unsigned prev = 0;
  for (unsigned i = 0; i < len; i++)
    if (IsPathSepar(path[i]))
    {
      name.SetFrom(path.Ptr(prev), i - prev);
      pathParts.Add(name);
      prev = i + 1;
    }
  name.SetFrom(path.Ptr(prev), len - prev);
  pathParts.Add(name);
}

void SplitPathToParts_2(const UString &path, UString &dirPrefix, UString &name)
{
  const wchar_t *start = path;
  const wchar_t *p = start + path.Len();
  for (; p != start; p--)
    if (IsPathSepar(*(p - 1)))
      break;
  dirPrefix.SetFrom(path, (unsigned)(p - start));
  name = p;
}

void SplitPathToParts_Smart(const UString &path, UString &dirPrefix, UString &name)
{
  const wchar_t *start = path;
  const wchar_t *p = start + path.Len();
  if (p != start)
  {
    if (IsPathSepar(*(p - 1)))
      p--;
    for (; p != start; p--)
      if (IsPathSepar(*(p - 1)))
        break;
  }
  dirPrefix.SetFrom(path, (unsigned)(p - start));
  name = p;
}

/*
UString ExtractDirPrefixFromPath(const UString &path)
{
  return path.Left(path.ReverseFind_PathSepar() + 1));
}
*/

UString ExtractFileNameFromPath(const UString &path)
{
  return UString(path.Ptr((unsigned)(path.ReverseFind_PathSepar() + 1)));
}


bool DoesWildcardMatchName(const UString &mask, const UString &name)
{
  return EnhancedMaskTest(mask, name);
}

bool DoesNameContainWildcard(const UString &path)
{
  for (unsigned i = 0; i < path.Len(); i++)
  {
    wchar_t c = path[i];
    if (c == '*' || c == '?')
      return true;
  }
  return false;
}


// ----------------------------------------------------------'
// NWildcard

namespace NWildcard {

/*

M = MaskParts.Size();
N = TestNameParts.Size();

                           File                          Dir
ForFile     rec   M<=N  [N-M, N)                          -
!ForDir  nonrec   M=N   [0, M)                            -
 
ForDir      rec   M<N   [0, M) ... [N-M-1, N-1)  same as ForBoth-File
!ForFile nonrec         [0, M)                   same as ForBoth-File

ForFile     rec   m<=N  [0, M) ... [N-M, N)      same as ForBoth-File
ForDir   nonrec         [0, M)                   same as ForBoth-File

*/

bool CItem::AreAllAllowed() const
{
  return ForFile && ForDir && WildcardMatching
      && PathParts.Size() == 1 && PathParts.Front().IsEqualTo("*");
}

bool CItem::CheckPath(const UStringVector &pathParts, bool isFile) const
{
  if (!isFile && !ForDir)
    return false;

  /*
  if (PathParts.IsEmpty())
  {
    // PathParts.IsEmpty() means all items (universal wildcard)
    if (!isFile)
      return true;
    if (pathParts.Size() <= 1)
      return ForFile;
    return (ForDir || Recursive && ForFile);
  }
  */

  int delta = (int)pathParts.Size() - (int)PathParts.Size();
  if (delta < 0)
    return false;
  int start = 0;
  int finish = 0;
  
  if (isFile)
  {
    if (!ForDir)
    {
      if (Recursive)
        start = delta;
      else if (delta !=0)
        return false;
    }
    if (!ForFile && delta == 0)
      return false;
  }
  
  if (Recursive)
  {
    finish = delta;
    if (isFile && !ForFile)
      finish = delta - 1;
  }
  
  for (int d = start; d <= finish; d++)
  {
    unsigned i;
    for (i = 0; i < PathParts.Size(); i++)
    {
      if (WildcardMatching)
      {
        if (!DoesWildcardMatchName(PathParts[i], pathParts[i + (unsigned)d]))
          break;
      }
      else
      {
        if (CompareFileNames(PathParts[i], pathParts[i + (unsigned)d]) != 0)
          break;
      }
    }
    if (i == PathParts.Size())
      return true;
  }
  return false;
}

bool CCensorNode::AreAllAllowed() const
{
  if (!Name.IsEmpty() ||
      !SubNodes.IsEmpty() ||
      !ExcludeItems.IsEmpty() ||
      IncludeItems.Size() != 1)
    return false;
  return IncludeItems.Front().AreAllAllowed();
}

int CCensorNode::FindSubNode(const UString &name) const
{
  FOR_VECTOR (i, SubNodes)
    if (CompareFileNames(SubNodes[i].Name, name) == 0)
      return (int)i;
  return -1;
}

void CCensorNode::AddItemSimple(bool include, CItem &item)
{
  CObjectVector<CItem> &items = include ? IncludeItems : ExcludeItems;
  items.Add(item);
}

void CCensorNode::AddItem(bool include, CItem &item, int ignoreWildcardIndex)
{
  if (item.PathParts.Size() <= 1)
  {
    if (item.PathParts.Size() != 0 && item.WildcardMatching)
    {
      if (!DoesNameContainWildcard(item.PathParts.Front()))
        item.WildcardMatching = false;
    }
    AddItemSimple(include, item);
    return;
  }

  const UString &front = item.PathParts.Front();
  
  // WIN32 doesn't support wildcards in file names
  if (item.WildcardMatching
      && ignoreWildcardIndex != 0
      && DoesNameContainWildcard(front))
  {
    AddItemSimple(include, item);
    return;
  }
  CCensorNode &subNode = Find_SubNode_Or_Add_New(front);
  item.PathParts.Delete(0);
  subNode.AddItem(include, item, ignoreWildcardIndex - 1);
}

/*
void CCensorNode::AddItem(bool include, const UString &path, const CCensorPathProps &props)
{
  CItem item;
  SplitPathToParts(path, item.PathParts);
  item.Recursive = props.Recursive;
  item.ForFile = props.ForFile;
  item.ForDir = props.ForDir;
  item.WildcardMatching = props.WildcardMatching;
  AddItem(include, item);
}
*/

bool CCensorNode::NeedCheckSubDirs() const
{
  FOR_VECTOR (i, IncludeItems)
  {
    const CItem &item = IncludeItems[i];
    if (item.Recursive || item.PathParts.Size() > 1)
      return true;
  }
  return false;
}

bool CCensorNode::AreThereIncludeItems() const
{
  if (IncludeItems.Size() > 0)
    return true;
  FOR_VECTOR (i, SubNodes)
    if (SubNodes[i].AreThereIncludeItems())
      return true;
  return false;
}

bool CCensorNode::CheckPathCurrent(bool include, const UStringVector &pathParts, bool isFile) const
{
  const CObjectVector<CItem> &items = include ? IncludeItems : ExcludeItems;
  FOR_VECTOR (i, items)
    if (items[i].CheckPath(pathParts, isFile))
      return true;
  return false;
}

bool CCensorNode::CheckPathVect(const UStringVector &pathParts, bool isFile, bool &include) const
{
  if (CheckPathCurrent(false, pathParts, isFile))
  {
    include = false;
    return true;
  }
  if (pathParts.Size() > 1)
  {
    int index = FindSubNode(pathParts.Front());
    if (index >= 0)
    {
      UStringVector pathParts2 = pathParts;
      pathParts2.Delete(0);
      if (SubNodes[(unsigned)index].CheckPathVect(pathParts2, isFile, include))
        return true;
    }
  }
  bool finded = CheckPathCurrent(true, pathParts, isFile);
  include = finded; // if (!finded), then (true) is allowed also
  return finded;
}

/*
bool CCensorNode::CheckPath2(bool isAltStream, const UString &path, bool isFile, bool &include) const
{
  UStringVector pathParts;
  SplitPathToParts(path, pathParts);
  if (CheckPathVect(pathParts, isFile, include))
  {
    if (!include || !isAltStream)
      return true;
  }
  if (isAltStream && !pathParts.IsEmpty())
  {
    UString &back = pathParts.Back();
    int pos = back.Find(L':');
    if (pos > 0)
    {
      back.DeleteFrom(pos);
      return CheckPathVect(pathParts, isFile, include);
    }
  }
  return false;
}

bool CCensorNode::CheckPath(bool isAltStream, const UString &path, bool isFile) const
{
  bool include;
  if (CheckPath2(isAltStream, path, isFile, include))
    return include;
  return false;
}
*/

bool CCensorNode::CheckPathToRoot_Change(bool include, UStringVector &pathParts, bool isFile) const
{
  if (CheckPathCurrent(include, pathParts, isFile))
    return true;
  if (!Parent)
    return false;
  pathParts.Insert(0, Name);
  return Parent->CheckPathToRoot_Change(include, pathParts, isFile);
}

bool CCensorNode::CheckPathToRoot(bool include, const UStringVector &pathParts, bool isFile) const
{
  if (CheckPathCurrent(include, pathParts, isFile))
    return true;
  if (!Parent)
    return false;
  UStringVector pathParts2;
  pathParts2.Add(Name);
  pathParts2 += pathParts;
  return Parent->CheckPathToRoot_Change(include, pathParts2, isFile);
}

/*
bool CCensorNode::CheckPathToRoot(bool include, const UString &path, bool isFile) const
{
  UStringVector pathParts;
  SplitPathToParts(path, pathParts);
  return CheckPathToRoot(include, pathParts, isFile);
}
*/

void CCensorNode::ExtendExclude(const CCensorNode &fromNodes)
{
  ExcludeItems += fromNodes.ExcludeItems;
  FOR_VECTOR (i, fromNodes.SubNodes)
  {
    const CCensorNode &node = fromNodes.SubNodes[i];
    Find_SubNode_Or_Add_New(node.Name).ExtendExclude(node);
  }
}

int CCensor::FindPairForPrefix(const UString &prefix) const
{
  FOR_VECTOR (i, Pairs)
    if (CompareFileNames(Pairs[i].Prefix, prefix) == 0)
      return (int)i;
  return -1;
}

#ifdef _WIN32

bool IsDriveColonName(const wchar_t *s)
{
  unsigned c = s[0];
  c |= 0x20;
  c -= 'a';
  return c <= (unsigned)('z' - 'a') && s[1] == ':' && s[2] == 0;
}

unsigned GetNumPrefixParts_if_DrivePath(UStringVector &pathParts)
{
  if (pathParts.IsEmpty())
    return 0;
  
  unsigned testIndex = 0;
  if (pathParts[0].IsEmpty())
  {
    if (pathParts.Size() < 4
        || !pathParts[1].IsEmpty()
        || !pathParts[2].IsEqualTo("?"))
      return 0;
    testIndex = 3;
  }
  if (NWildcard::IsDriveColonName(pathParts[testIndex]))
    return testIndex + 1;
  return 0;
}

#endif

static unsigned GetNumPrefixParts(const UStringVector &pathParts)
{
  if (pathParts.IsEmpty())
    return 0;

  /* empty last part could be removed already from (pathParts),
     if there was tail path separator (slash) in original full path string. */
  
  #ifdef _WIN32

  if (IsDriveColonName(pathParts[0]))
    return 1;
  if (!pathParts[0].IsEmpty())
    return 0;

  if (pathParts.Size() == 1)
    return 1;
  if (!pathParts[1].IsEmpty())
    return 1;
  if (pathParts.Size() == 2)
    return 2;
  if (pathParts[2].IsEqualTo("."))
    return 3;

  unsigned networkParts = 2;
  if (pathParts[2].IsEqualTo("?"))
  {
    if (pathParts.Size() == 3)
      return 3;
    if (IsDriveColonName(pathParts[3]))
      return 4;
    if (!pathParts[3].IsEqualTo_Ascii_NoCase("UNC"))
      return 3;
    networkParts = 4;
  }

  networkParts +=
      // 2; // server/share
      1; // server
  if (pathParts.Size() <= networkParts)
    return pathParts.Size();
  return networkParts;

  #else
  
  return pathParts[0].IsEmpty() ? 1 : 0;
 
  #endif
}

void CCensor::AddItem(ECensorPathMode pathMode, bool include, const UString &path,
    const CCensorPathProps &props)
{
  if (path.IsEmpty())
    throw "Empty file path";

  UStringVector pathParts;
  SplitPathToParts(path, pathParts);

  CCensorPathProps props2 = props;

  bool forFile = true;
  bool forDir = true;
  const UString &back = pathParts.Back();
  if (back.IsEmpty())
  {
    // we have tail path separator. So it's directory.
    // we delete tail path separator here even for "\" and "c:\"
    forFile = false;
    pathParts.DeleteBack();
  }
  else
  {
    if (props.MarkMode == kMark_StrictFile
        || (props.MarkMode == kMark_StrictFile_IfWildcard
            && DoesNameContainWildcard(back)))
      forDir = false;
  }

  
  UString prefix;
  
  int ignoreWildcardIndex = -1;

  // #ifdef _WIN32
  // we ignore "?" wildcard in "\\?\" prefix.
  if (pathParts.Size() >= 3
      && pathParts[0].IsEmpty()
      && pathParts[1].IsEmpty()
      && pathParts[2].IsEqualTo("?"))
    ignoreWildcardIndex = 2;
  // #endif

  if (pathMode != k_AbsPath)
  {
    // detection of the number of Skip Parts for prefix
    ignoreWildcardIndex = -1;

    const unsigned numPrefixParts = GetNumPrefixParts(pathParts);
    unsigned numSkipParts = numPrefixParts;

    if (pathMode != k_FullPath)
    {
      // if absolute path, then all parts before last part will be in prefix
      if (numPrefixParts != 0 && pathParts.Size() > numPrefixParts)
        numSkipParts = pathParts.Size() - 1;
    }
    {
      int dotsIndex = -1;
      for (unsigned i = numPrefixParts; i < pathParts.Size(); i++)
      {
        const UString &part = pathParts[i];
        if (part.IsEqualTo("..") || part.IsEqualTo("."))
          dotsIndex = (int)i;
      }

      if (dotsIndex >= 0)
      {
        if (dotsIndex == (int)pathParts.Size() - 1)
          numSkipParts = pathParts.Size();
        else
          numSkipParts = pathParts.Size() - 1;
      }
    }

    // we split (pathParts) to (prefix) and (pathParts).
    for (unsigned i = 0; i < numSkipParts; i++)
    {
      {
        const UString &front = pathParts.Front();
        // WIN32 doesn't support wildcards in file names
        if (props.WildcardMatching)
          if (i >= numPrefixParts && DoesNameContainWildcard(front))
            break;
        prefix += front;
        prefix.Add_PathSepar();
      }
      pathParts.Delete(0);
    }
  }

  int index = FindPairForPrefix(prefix);
  if (index < 0)
  {
    index = (int)Pairs.Size();
    Pairs.AddNew().Prefix = prefix;
  }

  if (pathMode != k_AbsPath)
  {
    if (pathParts.IsEmpty() || (pathParts.Size() == 1 && pathParts[0].IsEmpty()))
    {
      // we create universal item, if we skip all parts as prefix (like \ or L:\ )
      pathParts.Clear();
      pathParts.Add(UString("*"));
      forFile = true;
      forDir = true;
      props2.WildcardMatching = true;
      props2.Recursive = false;
    }
  }

  /*
  // not possible now
  if (!forDir && !forFile)
  {
    UString s ("file path was blocked for files and directories: ");
    s += path;
    throw s;
    // return; // for debug : ignore item (don't create Item)
  }
  */

  CItem item;
  item.PathParts = pathParts;
  item.ForDir = forDir;
  item.ForFile = forFile;
  item.Recursive = props2.Recursive;
  item.WildcardMatching = props2.WildcardMatching;
  Pairs[(unsigned)index].Head.AddItem(include, item, ignoreWildcardIndex);
}

/*
bool CCensor::CheckPath(bool isAltStream, const UString &path, bool isFile) const
{
  bool finded = false;
  FOR_VECTOR (i, Pairs)
  {
    bool include;
    if (Pairs[i].Head.CheckPath2(isAltStream, path, isFile, include))
    {
      if (!include)
        return false;
      finded = true;
    }
  }
  return finded;
}
*/

void CCensor::ExtendExclude()
{
  unsigned i;
  for (i = 0; i < Pairs.Size(); i++)
    if (Pairs[i].Prefix.IsEmpty())
      break;
  if (i == Pairs.Size())
    return;
  unsigned index = i;
  for (i = 0; i < Pairs.Size(); i++)
    if (index != i)
      Pairs[i].Head.ExtendExclude(Pairs[index].Head);
}

void CCensor::AddPathsToCensor(ECensorPathMode censorPathMode)
{
  FOR_VECTOR(i, CensorPaths)
  {
    const CCensorPath &cp = CensorPaths[i];
    AddItem(censorPathMode, cp.Include, cp.Path, cp.Props);
  }
  CensorPaths.Clear();
}

void CCensor::AddPreItem(bool include, const UString &path, const CCensorPathProps &props)
{
  CCensorPath &cp = CensorPaths.AddNew();
  cp.Path = path;
  cp.Include = include;
  cp.Props = props;
}

}

/* ================ unit: CPP/Common/XzCrc64Init.cpp ================ */
// XzCrc64Init.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

static struct CCrc64Gen { CCrc64Gen() { Crc64GenerateTable(); } } g_Crc64TableInit;

/* ================ unit: CPP/Windows/FileDir.cpp ================ */
// Windows/FileDir.cpp

// amalgamation: header emitted in prologue


#ifndef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef _UNICODE
extern bool g_IsNT;
#endif

using namespace NWindows;
using namespace NFile;
using namespace NName;

#ifndef _WIN32

static bool FiTime_To_timespec(const CFiTime *ft, timespec &ts)
{
  if (ft)
  {
#if defined(_AIX)
    ts.tv_sec  = ft->tv_sec;
    ts.tv_nsec = ft->tv_nsec;
#else
    ts = *ft;
#endif
    return true;
  }
  // else
  {
    ts.tv_sec = 0;
    ts.tv_nsec =
    #ifdef UTIME_OMIT
      UTIME_OMIT; // -2 keep old timesptamp
    #else
      // UTIME_NOW; -1 // set to the current time
      0;
    #endif
    return false;
  }
}
#endif

namespace NWindows {
namespace NFile {
namespace NDir {

#ifdef _WIN32

#ifndef UNDER_CE

bool GetWindowsDir(FString &path)
{
  const unsigned kBufSize = MAX_PATH + 16;
  UINT len;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    TCHAR s[kBufSize + 1];
    s[0] = 0;
    len = ::GetWindowsDirectory(s, kBufSize);
    path = fas2fs(s);
  }
  else
  #endif
  {
    WCHAR s[kBufSize + 1];
    s[0] = 0;
    len = ::GetWindowsDirectoryW(s, kBufSize);
    path = us2fs(s);
  }
  return (len != 0 && len < kBufSize);
}


/*
new DOCs for GetSystemDirectory:
  returned path does not end with a backslash unless the
  system directory is the root directory.
*/

bool GetSystemDir(FString &path)
{
  const unsigned kBufSize = MAX_PATH + 16;
  UINT len;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    TCHAR s[kBufSize + 1];
    s[0] = 0;
    len = ::GetSystemDirectory(s, kBufSize);
    path = fas2fs(s);
  }
  else
  #endif
  {
    WCHAR s[kBufSize + 1];
    s[0] = 0;
    len = ::GetSystemDirectoryW(s, kBufSize);
    path = us2fs(s);
  }
  return (len != 0 && len < kBufSize);
}
#endif // UNDER_CE


static bool SetFileTime_Base(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime, DWORD dwFlagsAndAttributes)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
  }
  #endif
  
  HANDLE hDir = INVALID_HANDLE_VALUE;
  IF_USE_MAIN_PATH
    hDir = ::CreateFileW(fs2us(path), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, dwFlagsAndAttributes, NULL);
  #ifdef Z7_LONG_PATH
  if (hDir == INVALID_HANDLE_VALUE && USE_SUPER_PATH)
  {
    UString superPath;
    if (GetSuperPath(path, superPath, USE_MAIN_PATH))
      hDir = ::CreateFileW(superPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
          NULL, OPEN_EXISTING, dwFlagsAndAttributes, NULL);
  }
  #endif

  bool res = false;
  if (hDir != INVALID_HANDLE_VALUE)
  {
    res = BOOLToBool(::SetFileTime(hDir, cTime, aTime, mTime));
    ::CloseHandle(hDir);
  }
  return res;
}

bool SetDirTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime)
{
  return SetFileTime_Base(path, cTime, aTime, mTime, FILE_FLAG_BACKUP_SEMANTICS);
}

bool SetLinkFileTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime)
{
  return SetFileTime_Base(path, cTime, aTime, mTime, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT);
}


bool SetFileAttrib(CFSTR path, DWORD attrib)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::SetFileAttributes(fs2fas(path), attrib))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      if (::SetFileAttributesW(fs2us(path), attrib))
        return true;
    #ifdef Z7_LONG_PATH
    if (USE_SUPER_PATH)
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        return BOOLToBool(::SetFileAttributesW(superPath, attrib));
    }
    #endif
  }
  return false;
}


bool SetFileAttrib_PosixHighDetect(CFSTR path, DWORD attrib)
{
  #ifdef _WIN32
  if ((attrib & 0xF0000000) != 0)
    attrib &= 0x3FFF;
  #endif
  return SetFileAttrib(path, attrib);
}


bool RemoveDir(CFSTR path)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::RemoveDirectory(fs2fas(path)))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      if (::RemoveDirectoryW(fs2us(path)))
        return true;
    #ifdef Z7_LONG_PATH
    if (USE_SUPER_PATH)
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        return BOOLToBool(::RemoveDirectoryW(superPath));
    }
    #endif
  }
  return false;
}


// When moving a directory, oldFile and newFile must be on the same drive.

bool MyMoveFile(CFSTR oldFile, CFSTR newFile)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::MoveFile(fs2fas(oldFile), fs2fas(newFile)))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH_2(oldFile, newFile)
    {
      if (::MoveFileW(fs2us(oldFile), fs2us(newFile)))
        return true;
    }
    #ifdef Z7_LONG_PATH
    if (USE_SUPER_PATH_2)
    {
      UString d1, d2;
      if (GetSuperPaths(oldFile, newFile, d1, d2, USE_MAIN_PATH_2))
        return BOOLToBool(::MoveFileW(d1, d2));
    }
    #endif
  }
  return false;
}

#if defined(Z7_WIN32_WINNT_MIN) && Z7_WIN32_WINNT_MIN >= 0x0500
static DWORD WINAPI CopyProgressRoutine_to_ICopyFileProgress(
  LARGE_INTEGER TotalFileSize,          // file size
  LARGE_INTEGER TotalBytesTransferred,  // bytes transferred
  LARGE_INTEGER /* StreamSize */,             // bytes in stream
  LARGE_INTEGER /* StreamBytesTransferred */, // bytes transferred for stream
  DWORD /* dwStreamNumber */,                 // current stream
  DWORD /* dwCallbackReason */,               // callback reason
  HANDLE /* hSourceFile */,                   // handle to source file
  HANDLE /* hDestinationFile */,              // handle to destination file
  LPVOID lpData                         // from CopyFileEx
)
{
  return ((ICopyFileProgress *)lpData)->CopyFileProgress(
      (UInt64)TotalFileSize.QuadPart,
      (UInt64)TotalBytesTransferred.QuadPart);
}
#endif

bool MyMoveFile_with_Progress(CFSTR oldFile, CFSTR newFile,
    ICopyFileProgress *progress)
{
#if defined(Z7_WIN32_WINNT_MIN) && Z7_WIN32_WINNT_MIN >= 0x0500
#ifndef _UNICODE
  if (g_IsNT)
#endif
  if (progress)
  {
    IF_USE_MAIN_PATH_2(oldFile, newFile)
    {
      if (::MoveFileWithProgressW(fs2us(oldFile), fs2us(newFile),
          CopyProgressRoutine_to_ICopyFileProgress, progress, MOVEFILE_COPY_ALLOWED))
        return true;
      if (::GetLastError() == ERROR_REQUEST_ABORTED)
        return false;
    }
    #ifdef Z7_LONG_PATH
    if (USE_SUPER_PATH_2)
    {
      UString d1, d2;
      if (GetSuperPaths(oldFile, newFile, d1, d2, USE_MAIN_PATH_2))
        return BOOLToBool(::MoveFileWithProgressW(d1, d2,
            CopyProgressRoutine_to_ICopyFileProgress, progress, MOVEFILE_COPY_ALLOWED));
    }
    #endif
    return false;
  }
#else
  UNUSED_VAR(progress)
#endif
  return MyMoveFile(oldFile, newFile);
}

#ifndef UNDER_CE
#if !defined(Z7_WIN32_WINNT_MIN) || Z7_WIN32_WINNT_MIN < 0x0500  // Win2000
#define Z7_USE_DYN_CreateHardLink
#endif

#ifdef Z7_USE_DYN_CreateHardLink
EXTERN_C_BEGIN
typedef BOOL (WINAPI *Func_CreateHardLinkW)(
    LPCWSTR lpFileName,
    LPCWSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
EXTERN_C_END
#endif
#endif // UNDER_CE

bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
    /*
    if (::CreateHardLink(fs2fas(newFileName), fs2fas(existFileName), NULL))
      return true;
    */
  }
  else
  #endif
  {
#ifdef Z7_USE_DYN_CreateHardLink
    const
    Func_CreateHardLinkW
      my_CreateHardLinkW = Z7_GET_PROC_ADDRESS(
    Func_CreateHardLinkW, ::GetModuleHandleW(L"kernel32.dll"),
        "CreateHardLinkW");
    if (!my_CreateHardLinkW)
      return false;
    #define MY_CreateHardLinkW  my_CreateHardLinkW
#else
    #define MY_CreateHardLinkW  CreateHardLinkW
#endif
    IF_USE_MAIN_PATH_2(newFileName, existFileName)
    {
      if (MY_CreateHardLinkW(fs2us(newFileName), fs2us(existFileName), NULL))
        return true;
    }
    #ifdef Z7_LONG_PATH
    if (USE_SUPER_PATH_2)
    {
      UString d1, d2;
      if (GetSuperPaths(newFileName, existFileName, d1, d2, USE_MAIN_PATH_2))
        return BOOLToBool(MY_CreateHardLinkW(d1, d2, NULL));
    }
    #endif
  }
  return false;
}


/*
WinXP-64 CreateDir():
  ""                  - ERROR_PATH_NOT_FOUND
  \                   - ERROR_ACCESS_DENIED
  C:\                 - ERROR_ACCESS_DENIED, if there is such drive,
  
  D:\folder             - ERROR_PATH_NOT_FOUND, if there is no such drive,
  C:\nonExistent\folder - ERROR_PATH_NOT_FOUND
  
  C:\existFolder      - ERROR_ALREADY_EXISTS
  C:\existFolder\     - ERROR_ALREADY_EXISTS

  C:\folder   - OK
  C:\folder\  - OK

  \\Server\nonExistent    - ERROR_BAD_NETPATH
  \\Server\Share_Readonly - ERROR_ACCESS_DENIED
  \\Server\Share          - ERROR_ALREADY_EXISTS

  \\Server\Share_NTFS_drive - ERROR_ACCESS_DENIED
  \\Server\Share_FAT_drive  - ERROR_ALREADY_EXISTS
*/

bool CreateDir(CFSTR path)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::CreateDirectory(fs2fas(path), NULL))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      if (::CreateDirectoryW(fs2us(path), NULL))
        return true;
    #ifdef Z7_LONG_PATH
    if ((!USE_MAIN_PATH || ::GetLastError() != ERROR_ALREADY_EXISTS) && USE_SUPER_PATH)
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        return BOOLToBool(::CreateDirectoryW(superPath, NULL));
    }
    #endif
  }
  return false;
}

/*
  CreateDir2 returns true, if directory can contain files after the call (two cases):
    1) the directory already exists
    2) the directory was created
  path must be WITHOUT trailing path separator.

  We need CreateDir2, since fileInfo.Find() for reserved names like "com8"
   returns FILE instead of DIRECTORY. And we need to use SuperPath */
 
static bool CreateDir2(CFSTR path)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::CreateDirectory(fs2fas(path), NULL))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      if (::CreateDirectoryW(fs2us(path), NULL))
        return true;
    #ifdef Z7_LONG_PATH
    if ((!USE_MAIN_PATH || ::GetLastError() != ERROR_ALREADY_EXISTS) && USE_SUPER_PATH)
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
      {
        if (::CreateDirectoryW(superPath, NULL))
          return true;
        if (::GetLastError() != ERROR_ALREADY_EXISTS)
          return false;
        NFind::CFileInfo fi;
        if (!fi.Find(us2fs(superPath)))
          return false;
        return fi.IsDir();
      }
    }
    #endif
  }
  if (::GetLastError() != ERROR_ALREADY_EXISTS)
    return false;
  NFind::CFileInfo fi;
  if (!fi.Find(path))
    return false;
  return fi.IsDir();
}

#endif // _WIN32

static bool CreateDir2(CFSTR path);

bool CreateComplexDir(CFSTR _path)
{
  #ifdef _WIN32
  
  {
    const DWORD attrib = NFind::GetFileAttrib(_path);
    if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0)
      return true;
  }

  #ifndef UNDER_CE
  
  if (IsDriveRootPath_SuperAllowed(_path))
    return false;
  
  const unsigned prefixSize = GetRootPrefixSize(_path);
  
  #endif // UNDER_CE

  #else // _WIN32

  // Posix
  NFind::CFileInfo fi;
  if (fi.Find(_path))
  {
    if (fi.IsDir())
      return true;
  }
  
  #endif // _WIN32

  FString path (_path);

  int pos = path.ReverseFind_PathSepar();
  if (pos >= 0 && (unsigned)pos == path.Len() - 1)
  {
    if (path.Len() == 1)
      return true;
    path.DeleteBack();
  }

  const FString path2 (path);
  pos = (int)path.Len();
  
  for (;;)
  {
    if (CreateDir2(path))
      break;
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
      return false;
    pos = path.ReverseFind_PathSepar();
    if (pos < 0 || pos == 0)
      return false;
    
    #if defined(_WIN32) && !defined(UNDER_CE)
    if (pos == 1 && IS_PATH_SEPAR(path[0]))
      return false;
    if (prefixSize >= (unsigned)pos + 1)
      return false;
    #endif
    
    path.DeleteFrom((unsigned)pos);
  }
  
  while (pos < (int)path2.Len())
  {
    int pos2 = NName::FindSepar(path2.Ptr((unsigned)pos + 1));
    if (pos2 < 0)
      pos = (int)path2.Len();
    else
      pos += 1 + pos2;
    path.SetFrom(path2, (unsigned)pos);
    if (!CreateDir(path))
      return false;
  }
  
  return true;
}


#ifdef _WIN32

bool DeleteFileAlways(CFSTR path)
{
  /* If alt stream, we also need to clear READ-ONLY attribute of main file before delete.
     SetFileAttrib("name:stream", ) changes attributes of main file. */
  {
    DWORD attrib = NFind::GetFileAttrib(path);
    if (attrib != INVALID_FILE_ATTRIBUTES
        && (attrib & FILE_ATTRIBUTE_DIRECTORY) == 0
        && (attrib & FILE_ATTRIBUTE_READONLY) != 0)
    {
      if (!SetFileAttrib(path, attrib & ~(DWORD)FILE_ATTRIBUTE_READONLY))
        return false;
    }
  }

  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::DeleteFile(fs2fas(path)))
      return true;
  }
  else
  #endif
  {
    /* DeleteFile("name::$DATA") deletes all alt streams (same as delete DeleteFile("name")).
       Maybe it's better to open "name::$DATA" and clear data for unnamed stream? */
    IF_USE_MAIN_PATH
      if (::DeleteFileW(fs2us(path)))
        return true;
    #ifdef Z7_LONG_PATH
    if (USE_SUPER_PATH)
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        return BOOLToBool(::DeleteFileW(superPath));
    }
    #endif
  }
  return false;
}



bool RemoveDirWithSubItems(const FString &path)
{
  bool needRemoveSubItems = true;
  {
    NFind::CFileInfo fi;
    if (!fi.Find(path))
      return false;
    if (!fi.IsDir())
    {
      ::SetLastError(ERROR_DIRECTORY);
      return false;
    }
    if (fi.HasReparsePoint())
      needRemoveSubItems = false;
  }

  if (needRemoveSubItems)
  {
    FString s (path);
    s.Add_PathSepar();
    const unsigned prefixSize = s.Len();
    NFind::CEnumerator enumerator;
    enumerator.SetDirPrefix(s);
    NFind::CDirEntry fi;
    bool isError = false;
    DWORD lastError = 0;
    while (enumerator.Next(fi))
    {
      s.DeleteFrom(prefixSize);
      s += fi.Name;
      if (fi.IsDir())
      {
        if (!RemoveDirWithSubItems(s))
        {
          lastError = GetLastError();
          isError = true;
        }
      }
      else if (!DeleteFileAlways(s))
      {
        lastError = GetLastError();
        isError = false;
      }
    }
    if (isError)
    {
      SetLastError(lastError);
      return false;
    }
  }
  
  // we clear read-only attrib to remove read-only dir
  if (!SetFileAttrib(path, 0))
    return false;
  return RemoveDir(path);
}

bool RemoveDirAlways_if_Empty(const FString &path)
{
  const DWORD attrib = NFind::GetFileAttrib(path);
  if (attrib != INVALID_FILE_ATTRIBUTES
      && (attrib & FILE_ATTRIBUTE_READONLY))
  {
    bool need_ClearAttrib = true;
    if ((attrib & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
    {
      FString s (path);
      s.Add_PathSepar();
      NFind::CEnumerator enumerator;
      enumerator.SetDirPrefix(s);
      NFind::CDirEntry fi;
      if (enumerator.Next(fi))
      {
        // we don't want to change attributes, if there are files
        // in directory, because RemoveDir(path) will fail.
        need_ClearAttrib = false;
        // SetLastError(ERROR_DIR_NOT_EMPTY);
        // return false;
      }
    }
    if (need_ClearAttrib)
      SetFileAttrib(path, 0); // we clear read-only attrib to remove read-only dir
  }
  return RemoveDir(path);
}

#endif // _WIN32

#ifdef UNDER_CE

bool MyGetFullPathName(CFSTR path, FString &resFullPath)
{
  resFullPath = path;
  return true;
}

#else

bool MyGetFullPathName(CFSTR path, FString &resFullPath)
{
  return GetFullPath(path, resFullPath);
}

#ifdef _WIN32

/* Win10: SetCurrentDirectory() doesn't support long paths and
    doesn't support super prefix "\\?\", if long path behavior is not
    enabled in registry (LongPathsEnabled) and in manifest (longPathAware). */

bool SetCurrentDir(CFSTR path)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    return BOOLToBool(::SetCurrentDirectory(fs2fas(path)));
  }
  else
  #endif
  {
    return BOOLToBool(::SetCurrentDirectoryW(fs2us(path)));
  }
}


/*
we use system function GetCurrentDirectory()
new GetCurrentDirectory() DOCs:
  - If the function fails, the return value is zero.
  - If the function succeeds, the return value specifies
      the number of characters that are written to the buffer,
      not including the terminating null character.
  - If the buffer is not large enough, the return value specifies
      the required size of the buffer, in characters,
      including the null-terminating character.
  
GetCurrentDir() calls GetCurrentDirectory().
GetCurrentDirectory() in win10 in tests:
  the returned (path) does not end with a backslash, if
  current directory is not root directory of drive.
  But that behavior is not guarantied in specification docs.
*/

bool GetCurrentDir(FString &path)
{
  const unsigned kBufSize = MAX_PATH + 16;
  path.Empty();

  #ifndef _UNICODE
  if (!g_IsNT)
  {
    TCHAR s[kBufSize + 1];
    s[0] = 0;
    const DWORD len = ::GetCurrentDirectory(kBufSize, s);
    if (len == 0 || len >= kBufSize)
      return false;
    s[kBufSize] = 0;  // optional guard
    path = fas2fs(s);
    return true;
  }
  else
  #endif
  {
    DWORD len;
    {
      WCHAR s[kBufSize + 1];
      s[0] = 0;
      len = ::GetCurrentDirectoryW(kBufSize, s);
      if (len == 0)
        return false;
      if (len < kBufSize)
      {
        s[kBufSize] = 0;  // optional guard
        path = us2fs(s);
        return true;
      }
    }
    UString temp;
    const DWORD len2 = ::GetCurrentDirectoryW(len, temp.GetBuf(len));
    if (len2 == 0)
      return false;
    temp.ReleaseBuf_CalcLen(len);
    if (temp.Len() != len2 || len - 1 != len2)
    {
      /* it's unexpected case, if current dir of process
         was changed between two function calls,
         or some unexpected function implementation */
      // SetLastError((DWORD)E_FAIL);  // we can set some error code
      return false;
    }
    path = us2fs(temp);
    return true;
  }
}

#endif // _WIN32
#endif // UNDER_CE


bool GetFullPathAndSplit(CFSTR path, FString &resDirPrefix, FString &resFileName)
{
  bool res = MyGetFullPathName(path, resDirPrefix);
  if (!res)
    resDirPrefix = path;
  int pos = resDirPrefix.ReverseFind_PathSepar();
  pos++;
  resFileName = resDirPrefix.Ptr((unsigned)pos);
  resDirPrefix.DeleteFrom((unsigned)pos);
  return res;
}

bool GetOnlyDirPrefix(CFSTR path, FString &resDirPrefix)
{
  FString resFileName;
  return GetFullPathAndSplit(path, resDirPrefix, resFileName);
}



bool MyGetTempPath(FString &path)
{
  #ifdef _WIN32

  /*
  new DOCs for GetTempPathW():
    - The returned string ends with a backslash.
    - The maximum possible return value is MAX_PATH+1 (261).
  */

  const unsigned kBufSize = MAX_PATH + 16;
  DWORD len;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    TCHAR s[kBufSize + 1];
    s[0] = 0;
    len = ::GetTempPath(kBufSize, s);
    path = fas2fs(s);
  }
  else
  #endif
  {
    WCHAR s[kBufSize + 1];
    s[0] = 0;
    len = ::GetTempPathW(kBufSize, s);
    path = us2fs(s);
  }
  /* win10: GetTempPathW() doesn't set backslash at the end of path,
       if (buffer_size == len_of(path_with_backslash)).
     So we normalize path here: */
  NormalizeDirPathPrefix(path);
  return (len != 0 && len < kBufSize);

  #else  // !_WIN32
  
  // FIXME: improve that code
  path = STRING_PATH_SEPARATOR "tmp";
  const char *s;
  if (NFind::DoesDirExist_FollowLink(path))
    s = STRING_PATH_SEPARATOR "tmp" STRING_PATH_SEPARATOR;
  else
    s = "." STRING_PATH_SEPARATOR;
  path = s;
  return true;
  
  #endif
}


bool CreateTempFile2(CFSTR prefix, bool addRandom, AString &postfix, NIO::COutFile *outFile)
{
  UInt32 d =
    #ifdef _WIN32
      (GetTickCount() << 12) ^ (GetCurrentThreadId() << 14) ^ GetCurrentProcessId();
    #else
      (UInt32)(time(NULL) << 12) ^  ((UInt32)getppid() << 14) ^ (UInt32)(getpid());
    #endif

  for (unsigned i = 0; i < 100; i++)
  {
    postfix.Empty();
    if (addRandom)
    {
      char s[16];
      UInt32 val = d;
      unsigned k;
      for (k = 0; k < 8; k++)
      {
        const unsigned t = (unsigned)val & 0xF;
        val >>= 4;
        s[k] = (char)((t < 10) ? ('0' + t) : ('A' + (t - 10)));
      }
      s[k] = '\0';
      if (outFile)
        postfix.Add_Dot();
      postfix += s;
      UInt32 step = GetTickCount() + 2;
      if (step == 0)
        step = 1;
      d += step;
    }
    addRandom = true;
    if (outFile)
      postfix += ".tmp";
    FString path (prefix);
    path += postfix;
    if (NFind::DoesFileOrDirExist(path))
    {
      SetLastError(ERROR_ALREADY_EXISTS);
      continue;
    }
    if (outFile)
    {
      if (outFile->Create_NEW(path))
        return true;
    }
    else
    {
      if (CreateDir(path))
        return true;
    }
    const DWORD error = GetLastError();
    if (error != ERROR_FILE_EXISTS &&
        error != ERROR_ALREADY_EXISTS)
      break;
  }
  postfix.Empty();
  return false;
}

bool CTempFile::Create(CFSTR prefix, NIO::COutFile *outFile)
{
  if (!Remove())
    return false;
  _path.Empty();
  AString postfix;
  if (!CreateTempFile2(prefix, false, postfix, outFile))
    return false;
  _path = prefix;
  _path += postfix;
  _mustBeDeleted = true;
  return true;
}

bool CTempFile::CreateRandomInTempFolder(CFSTR namePrefix, NIO::COutFile *outFile)
{
  if (!Remove())
    return false;
  _path.Empty();
  FString tempPath;
  if (!MyGetTempPath(tempPath))
    return false;
  AString postfix;
  tempPath += namePrefix;
  if (!CreateTempFile2(tempPath, true, postfix, outFile))
    return false;
  _path = tempPath;
  _path += postfix;
  _mustBeDeleted = true;
  return true;
}

bool CTempFile::Remove()
{
  if (!_mustBeDeleted)
    return true;
  _mustBeDeleted = !DeleteFileAlways(_path);
  return !_mustBeDeleted;
}

bool CTempFile::MoveTo(CFSTR name, bool deleteDestBefore,
    ICopyFileProgress *progress)
{
  if (deleteDestBefore)
  {
    if (NFind::DoesFileExist_Raw(name))
    {
      // attrib = NFind::GetFileAttrib(name);
      if (!DeleteFileAlways(name))
        return false;
    }
  }
  DisableDeleting();
  // if (!progress) return MyMoveFile(_path, name);
  return MyMoveFile_with_Progress(_path, name, progress);
  /*
  if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_READONLY))
  {
    DWORD attrib2 = NFind::GetFileAttrib(name);
    if (attrib2 != INVALID_FILE_ATTRIBUTES)
      SetFileAttrib(name, attrib2 | FILE_ATTRIBUTE_READONLY);
  }
  */
}

#ifdef _WIN32
bool CTempDir::Create(CFSTR prefix)
{
  if (!Remove())
    return false;
  _path.Empty();
  FString tempPath;
  if (!MyGetTempPath(tempPath))
    return false;
  tempPath += prefix;
  AString postfix;
  if (!CreateTempFile2(tempPath, true, postfix, NULL))
    return false;
  _path = tempPath;
  _path += postfix;
  _mustBeDeleted = true;
  return true;
}

bool CTempDir::Remove()
{
  if (!_mustBeDeleted)
    return true;
  _mustBeDeleted = !RemoveDirWithSubItems(_path);
  return !_mustBeDeleted;
}
#endif



#ifndef _WIN32

bool RemoveDir(CFSTR path)
{
  return (rmdir(path) == 0);
}


static BOOL My_CopyFile(CFSTR oldFile, CFSTR newFile, ICopyFileProgress *progress)
{
  {
    NIO::COutFile outFile;
    if (!outFile.Create_NEW(newFile))
      return FALSE;
    NIO::CInFile inFile;
    if (!inFile.Open(oldFile))
      return FALSE;
    
    const size_t k_BufSize = 1 << 16;
    CAlignedBuffer1 buf(k_BufSize);
    
    UInt64 length = 0;
    if (progress && !inFile.GetLength(length))
      length = 0;
    UInt64 prev = 0;
    UInt64 cur = 0;
    for (;;)
    {
      const ssize_t num = inFile.read_part(buf, k_BufSize);
      if (num == 0)
        return TRUE;
      if (num < 0)
        break;
      size_t processed;
      const ssize_t num2 = outFile.write_full(buf, (size_t)num, processed);
      if (num2 != num || processed != (size_t)num)
        break;
      cur += (size_t)num2;
      if (progress && cur - prev >= (1u << 20))
      {
        prev = cur;
        if (progress->CopyFileProgress(length, cur) != PROGRESS_CONTINUE)
        {
          errno = EINTR; // instead of WIN32::ERROR_REQUEST_ABORTED
          break;
        }
      }
    }
  }
  // There is file IO error or process was interrupted by user.
  // We close output file and delete it.
  // DeleteFileAlways doesn't change errno (if successed), but we restore errno.
  const int errno_save = errno;
  DeleteFileAlways(newFile);
  errno = errno_save;
  return FALSE;
}


bool MyMoveFile_with_Progress(CFSTR oldFile, CFSTR newFile,
    ICopyFileProgress *progress)
{
  int res = rename(oldFile, newFile);
  if (res == 0)
    return true;
  if (errno != EXDEV) // (oldFile and newFile are not on the same mounted filesystem)
    return false;

  if (My_CopyFile(oldFile, newFile, progress) == FALSE)
    return false;
    
  struct stat info_file;
  res = stat(oldFile, &info_file);
  if (res != 0)
    return false;

  /*
  ret = chmod(dst,info_file.st_mode & g_umask.mask);
  */
  return (unlink(oldFile) == 0);
}

bool MyMoveFile(CFSTR oldFile, CFSTR newFile)
{
  return MyMoveFile_with_Progress(oldFile, newFile, NULL);
}


bool CreateDir(CFSTR path)
{
  return (mkdir(path, 0777) == 0); // change it
}

static bool CreateDir2(CFSTR path)
{
  return (mkdir(path, 0777) == 0); // change it
}


bool DeleteFileAlways(CFSTR path)
{
  return (remove(path) == 0);
}

bool SetCurrentDir(CFSTR path)
{
  return (chdir(path) == 0);
}


bool GetCurrentDir(FString &path)
{
  path.Empty();

  #define MY_PATH_MAX  PATH_MAX
  // #define MY_PATH_MAX  1024

  char s[MY_PATH_MAX + 1];
  char *res = getcwd(s, MY_PATH_MAX);
  if (res)
  {
    path = fas2fs(s);
    return true;
  }
  {
    // if (errno != ERANGE) return false;
    #if defined(__GLIBC__) || defined(__APPLE__)
    /* As an extension to the POSIX.1-2001 standard, glibc's getcwd()
       allocates the buffer dynamically using malloc(3) if buf is NULL. */
    res = getcwd(NULL, 0);
    if (res)
    {
      path = fas2fs(res);
      ::free(res);
      return true;
    }
    #endif
    return false;
  }
}



// #undef UTIME_OMIT // to debug

#ifndef UTIME_OMIT
  /* we can define UTIME_OMIT for debian and another systems.
     Is it OK to define UTIME_OMIT to -2 here, if UTIME_OMIT is not defined? */
  // #define UTIME_OMIT -2
#endif





static bool SetFileTime_Base(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime, const int flags)
{
  // need testing
  /*
  struct utimbuf buf;
  struct stat st;
  UNUSED_VAR(cTime)
  printf("\nstat = %s\n", path);
  int ret = stat(path, &st);
  if (ret == 0)
  {
    buf.actime  = st.st_atime;
    buf.modtime = st.st_mtime;
  }
  else
  {
    time_t cur_time = time(0);
    buf.actime  = cur_time;
    buf.modtime = cur_time;
  }
  if (aTime)
  {
    UInt32 ut;
    if (NTime::FileTimeToUnixTime(*aTime, ut))
      buf.actime = ut;
  }
  if (mTime)
  {
    UInt32 ut;
    if (NTime::FileTimeToUnixTime(*mTime, ut))
      buf.modtime = ut;
  }
  return utime(path, &buf) == 0;
  */

  // if (!aTime && !mTime) return true;
  struct timespec times[2];
  UNUSED_VAR(cTime)
  bool needChange;
  needChange  = FiTime_To_timespec(aTime, times[0]);
  needChange |= FiTime_To_timespec(mTime, times[1]);
  // if (mTime) { printf("\n time = %ld.%9ld\n", mTime->tv_sec, mTime->tv_nsec);  }
  if (!needChange)
    return true;
  return utimensat(AT_FDCWD, path, times, flags) == 0;
}

bool SetDirTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime)
{
  return SetFileTime_Base(path, cTime, aTime, mTime, 0); // (flags = 0) means follow_link
}

bool SetLinkFileTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime)
{
  return SetFileTime_Base(path, cTime, aTime, mTime, AT_SYMLINK_NOFOLLOW);
}


struct C_umask
{
  mode_t mask;

  C_umask()
  {
    /* by security reasons we restrict attributes according
       with process's file mode creation mask (umask) */
    const mode_t um = umask(0); // octal :0022 is expected
    mask = 0777 & (~um);        // octal: 0755 is expected
    umask(um);  // restore the umask
    // printf("\n umask = 0%03o mask = 0%03o\n", um, mask);
    
    // mask = 0777; // debug we can disable the restriction:
  }
};

static C_umask g_umask;

// #define PRF(x) x;
#define PRF(x)

#define TRACE_SetFileAttrib(msg) \
  PRF(printf("\nSetFileAttrib(%s, %x) : %s\n", (const char *)path, attrib, msg);)

#define TRACE_chmod(s, mode) \
  PRF(printf("\n chmod(%s, %o)\n", (const char *)path, (unsigned)(mode));)

int my_chown(CFSTR path, uid_t owner, gid_t group)
{
  return chown(path, owner, group);
}

bool SetFileAttrib_PosixHighDetect(CFSTR path, DWORD attrib)
{
  TRACE_SetFileAttrib("")

  struct stat st;

  bool use_lstat = true;
  if (use_lstat)
  {
    if (lstat(path, &st) != 0)
    {
      TRACE_SetFileAttrib("bad lstat()")
      return false;
    }
    // TRACE_chmod("lstat", st.st_mode);
  }
  else
  {
    if (stat(path, &st) != 0)
    {
      TRACE_SetFileAttrib("bad stat()")
      return false;
    }
  }
  
  if (attrib & FILE_ATTRIBUTE_UNIX_EXTENSION)
  {
    TRACE_SetFileAttrib("attrib & FILE_ATTRIBUTE_UNIX_EXTENSION")
    st.st_mode = attrib >> 16;
    if (S_ISDIR(st.st_mode))
    {
      // user/7z must be able to create files in this directory
      st.st_mode |= (S_IRUSR | S_IWUSR | S_IXUSR);
    }
    else if (!S_ISREG(st.st_mode))
      return true;
  }
  else if (S_ISLNK(st.st_mode))
  {
    /* for most systems: permissions for symlinks are fixed to rwxrwxrwx.
       so we don't need chmod() for symlinks. */
    return true;
    // SetLastError(ENOSYS);
    // return false;
  }
  else
  {
    TRACE_SetFileAttrib("Only Windows Attributes")
    // Only Windows Attributes
    if (S_ISDIR(st.st_mode)
        || (attrib & FILE_ATTRIBUTE_READONLY) == 0)
      return true;
    st.st_mode &= ~(mode_t)(S_IWUSR | S_IWGRP | S_IWOTH); // octal: ~0222; // disable write permissions
  }

  int res;
  /*
  if (S_ISLNK(st.st_mode))
  {
    printf("\nfchmodat()\n");
    TRACE_chmod(path, (st.st_mode) & g_umask.mask)
    // AT_SYMLINK_NOFOLLOW is not implemted still in Linux.
    res = fchmodat(AT_FDCWD, path, (st.st_mode) & g_umask.mask,
        S_ISLNK(st.st_mode) ? AT_SYMLINK_NOFOLLOW : 0);
  }
  else
  */
  {
    TRACE_chmod(path, (st.st_mode) & g_umask.mask)
    res = chmod(path, (st.st_mode) & g_umask.mask);
  }
  // TRACE_SetFileAttrib("End")
  return (res == 0);
}


bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName)
{
  PRF(printf("\nhard link() %s -> %s\n", newFileName, existFileName);)
  return (link(existFileName, newFileName) == 0);
}

#endif // !_WIN32

// #endif

}}}

/* ================ unit: CPP/Windows/FileFind.cpp ================ */
// Windows/FileFind.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

#ifndef _WIN32
#include <fcntl.h>           /* Definition of AT_* constants */
// amalgamation: header emitted in prologue
// for major
// #include <sys/sysmacros.h>
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef _UNICODE
extern bool g_IsNT;
#endif

using namespace NWindows;
using namespace NFile;
using namespace NName;

#if defined(_WIN32) && !defined(UNDER_CE)

#if !defined(Z7_WIN32_WINNT_MIN) || Z7_WIN32_WINNT_MIN < 0x0502  // Win2003
#define Z7_USE_DYN_FindFirstStream
#endif

#ifdef Z7_USE_DYN_FindFirstStream

Z7_DIAGNOSTIC_IGNORE_CAST_FUNCTION

EXTERN_C_BEGIN

typedef enum
{
  My_FindStreamInfoStandard,
  My_FindStreamInfoMaxInfoLevel
} MY_STREAM_INFO_LEVELS;

typedef struct
{
  LARGE_INTEGER StreamSize;
  WCHAR cStreamName[MAX_PATH + 36];
} MY_WIN32_FIND_STREAM_DATA, *MY_PWIN32_FIND_STREAM_DATA;

typedef HANDLE (WINAPI *Func_FindFirstStreamW)(LPCWSTR fileName, MY_STREAM_INFO_LEVELS infoLevel,
    LPVOID findStreamData, DWORD flags);

typedef BOOL (APIENTRY *Func_FindNextStreamW)(HANDLE findStream, LPVOID findStreamData);

EXTERN_C_END

#else

#define MY_WIN32_FIND_STREAM_DATA  WIN32_FIND_STREAM_DATA
#define My_FindStreamInfoStandard  FindStreamInfoStandard

#endif
#endif // defined(_WIN32) && !defined(UNDER_CE)


namespace NWindows {
namespace NFile {


#ifdef _WIN32
#ifdef Z7_DEVICE_FILE
namespace NSystem
{
bool MyGetDiskFreeSpace(CFSTR rootPath, UInt64 &clusterSize, UInt64 &totalSize, UInt64 &freeSize);
}
#endif
#endif

namespace NFind {

/*
#ifdef _WIN32
#define MY_CLEAR_FILETIME(ft) ft.dwLowDateTime = ft.dwHighDateTime = 0;
#else
#define MY_CLEAR_FILETIME(ft) ft.tv_sec = 0;  ft.tv_nsec = 0;
#endif
*/

void CFileInfoBase::ClearBase() throw()
{
  Size = 0;
  FiTime_Clear(CTime);
  FiTime_Clear(ATime);
  FiTime_Clear(MTime);
 
 #ifdef _WIN32
  Attrib = 0;
  // ReparseTag = 0;
  IsAltStream = false;
  IsDevice = false;
 #else
  dev = 0;
  ino = 0;
  mode = 0;
  nlink = 0;
  uid = 0;
  gid = 0;
  rdev = 0;
 #endif
}


bool CFileInfoBase::SetAs_StdInFile()
{
  ClearBase();
  Size = (UInt64)(Int64)-1;
  NTime::GetCurUtc_FiTime(MTime);
  CTime = ATime = MTime;

#ifdef _WIN32

  /* in GUI mode : GetStdHandle(STD_INPUT_HANDLE) returns NULL,
     and it doesn't set LastError.  */
#if 1
  SetLastError(0);
  const HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  if (!h || h == INVALID_HANDLE_VALUE)
  {
    if (GetLastError() == 0)
      SetLastError(ERROR_INVALID_HANDLE);
    return false;
  }
  BY_HANDLE_FILE_INFORMATION info;
  if (GetFileInformationByHandle(h, &info)
      && info.dwVolumeSerialNumber)
  {
    Size = (((UInt64)info.nFileSizeHigh) << 32) + info.nFileSizeLow;
    // FileID_Low = (((UInt64)info.nFileIndexHigh) << 32) + info.nFileIndexLow;
    // NumLinks = SupportHardLinks ? info.nNumberOfLinks : 1;
    Attrib = info.dwFileAttributes;
    CTime = info.ftCreationTime;
    ATime = info.ftLastAccessTime;
    MTime = info.ftLastWriteTime;
  }
#if 0
  printf(
    "\ndwFileAttributes = %8x"
    "\nftCreationTime   = %8x"
    "\nftLastAccessTime = %8x"
    "\nftLastWriteTime  = %8x"
    "\ndwVolumeSerialNumber  = %8x"
    "\nnFileSizeHigh  = %8x"
    "\nnFileSizeLow   = %8x"
    "\nnNumberOfLinks  = %8x"
    "\nnFileIndexHigh  = %8x"
    "\nnFileIndexLow   = %8x \n",
      (unsigned)info.dwFileAttributes,
      (unsigned)info.ftCreationTime.dwHighDateTime,
      (unsigned)info.ftLastAccessTime.dwHighDateTime,
      (unsigned)info.ftLastWriteTime.dwHighDateTime,
      (unsigned)info.dwVolumeSerialNumber,
      (unsigned)info.nFileSizeHigh,
      (unsigned)info.nFileSizeLow,
      (unsigned)info.nNumberOfLinks,
      (unsigned)info.nFileIndexHigh,
      (unsigned)info.nFileIndexLow);
#endif
#endif

#else // non-Wiondow

  mode = S_IFIFO | 0777; // 0755 : 0775 : 0664 : 0644 :
#if 1
  struct stat st;
  if (fstat(0, &st) == 0)
  {
    SetFrom_stat(st);
    if (!S_ISREG(st.st_mode)
        // S_ISFIFO(st->st_mode)
        || st.st_size == 0)
    {
      Size = (UInt64)(Int64)-1;
      // mode = S_IFIFO | 0777;
    }
  }
#endif
#endif

  return true;
}

bool CFileInfo::IsDots() const throw()
{
  if (!IsDir() || Name.IsEmpty())
    return false;
  if (Name[0] != '.')
    return false;
  return Name.Len() == 1 || (Name.Len() == 2 && Name[1] == '.');
}


#ifdef _WIN32


#define WIN_FD_TO_MY_FI(fi, fd) \
  fi.Attrib = fd.dwFileAttributes; \
  fi.CTime = fd.ftCreationTime; \
  fi.ATime = fd.ftLastAccessTime; \
  fi.MTime = fd.ftLastWriteTime; \
  fi.Size = (((UInt64)fd.nFileSizeHigh) << 32) + fd.nFileSizeLow; \
  /* fi.ReparseTag = fd.dwReserved0; */ \
  fi.IsAltStream = false; \
  fi.IsDevice = false;

  /*
  #ifdef UNDER_CE
  fi.ObjectID = fd.dwOID;
  #else
  fi.ReparseTag = fd.dwReserved0;
  #endif
  */

static void Convert_WIN32_FIND_DATA_to_FileInfo(const WIN32_FIND_DATAW &fd, CFileInfo &fi)
{
  WIN_FD_TO_MY_FI(fi, fd)
  fi.Name = us2fs(fd.cFileName);
  #if defined(_WIN32) && !defined(UNDER_CE)
  // fi.ShortName = us2fs(fd.cAlternateFileName);
  #endif
}

#ifndef _UNICODE
static void Convert_WIN32_FIND_DATA_to_FileInfo(const WIN32_FIND_DATA &fd, CFileInfo &fi)
{
  WIN_FD_TO_MY_FI(fi, fd)
  fi.Name = fas2fs(fd.cFileName);
  #if defined(_WIN32) && !defined(UNDER_CE)
  // fi.ShortName = fas2fs(fd.cAlternateFileName);
  #endif
}
#endif
  
////////////////////////////////
// CFindFile

bool CFindFileBase::Close() throw()
{
  if (_handle == INVALID_HANDLE_VALUE)
    return true;
  if (!::FindClose(_handle))
    return false;
  _handle = INVALID_HANDLE_VALUE;
  return true;
}

/*
WinXP-64 FindFirstFile():
  ""      -  ERROR_PATH_NOT_FOUND
  folder\ -  ERROR_FILE_NOT_FOUND
  \       -  ERROR_FILE_NOT_FOUND
  c:\     -  ERROR_FILE_NOT_FOUND
  c:      -  ERROR_FILE_NOT_FOUND, if current dir is ROOT     ( c:\ )
  c:      -  OK,                   if current dir is NOT ROOT ( c:\folder )
  folder  -  OK

  \\               - ERROR_INVALID_NAME
  \\Server         - ERROR_INVALID_NAME
  \\Server\        - ERROR_INVALID_NAME
      
  \\Server\Share            - ERROR_BAD_NETPATH
  \\Server\Share            - ERROR_BAD_NET_NAME (Win7).
             !!! There is problem : Win7 makes some requests for "\\Server\Shar" (look in Procmon),
                 when we call it for "\\Server\Share"
                      
  \\Server\Share\           - ERROR_FILE_NOT_FOUND
  
  \\?\UNC\Server\Share      - ERROR_INVALID_NAME
  \\?\UNC\Server\Share      - ERROR_BAD_PATHNAME (Win7)
  \\?\UNC\Server\Share\     - ERROR_FILE_NOT_FOUND
  
  \\Server\Share_RootDrive  - ERROR_INVALID_NAME
  \\Server\Share_RootDrive\ - ERROR_INVALID_NAME
  
  e:\* - ERROR_FILE_NOT_FOUND, if there are no items in that root folder
  w:\* - ERROR_PATH_NOT_FOUND, if there is no such drive w:
*/

bool CFindFile::FindFirst(CFSTR path, CFileInfo &fi)
{
  if (!Close())
    return false;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    WIN32_FIND_DATAA fd;
    _handle = ::FindFirstFileA(fs2fas(path), &fd);
    if (_handle == INVALID_HANDLE_VALUE)
      return false;
    Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
  }
  else
  #endif
  {
    WIN32_FIND_DATAW fd;

    IF_USE_MAIN_PATH
      _handle = ::FindFirstFileW(fs2us(path), &fd);
    #ifdef Z7_LONG_PATH
    if (_handle == INVALID_HANDLE_VALUE && USE_SUPER_PATH)
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        _handle = ::FindFirstFileW(superPath, &fd);
    }
    #endif
    if (_handle == INVALID_HANDLE_VALUE)
      return false;
    Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
  }
  return true;
}

bool CFindFile::FindNext(CFileInfo &fi)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    WIN32_FIND_DATAA fd;
    if (!::FindNextFileA(_handle, &fd))
      return false;
    Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
  }
  else
  #endif
  {
    WIN32_FIND_DATAW fd;
    if (!::FindNextFileW(_handle, &fd))
      return false;
    Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
  }
  return true;
}

#if defined(_WIN32) && !defined(UNDER_CE)

////////////////////////////////
// AltStreams

#ifdef Z7_USE_DYN_FindFirstStream
static Func_FindFirstStreamW g_FindFirstStreamW;
static Func_FindNextStreamW  g_FindNextStreamW;
#define MY_FindFirstStreamW  g_FindFirstStreamW
#define MY_FindNextStreamW   g_FindNextStreamW
static struct CFindStreamLoader
{
  CFindStreamLoader()
  {
    const HMODULE hm = ::GetModuleHandleA("kernel32.dll");
       g_FindFirstStreamW = Z7_GET_PROC_ADDRESS(
    Func_FindFirstStreamW, hm,
        "FindFirstStreamW");
       g_FindNextStreamW = Z7_GET_PROC_ADDRESS(
    Func_FindNextStreamW, hm,
        "FindNextStreamW");
  }
} g_FindStreamLoader;
#else
#define MY_FindFirstStreamW  FindFirstStreamW
#define MY_FindNextStreamW   FindNextStreamW
#endif


bool CStreamInfo::IsMainStream() const throw()
{
  return StringsAreEqualNoCase_Ascii(Name, "::$DATA");
}

UString CStreamInfo::GetReducedName() const
{
  // remove ":$DATA" postfix, but keep postfix, if Name is "::$DATA"
  UString s (Name);
  if (s.Len() > 6 + 1 && StringsAreEqualNoCase_Ascii(s.RightPtr(6), ":$DATA"))
    s.DeleteFrom(s.Len() - 6);
  return s;
}

/*
UString CStreamInfo::GetReducedName2() const
{
  UString s = GetReducedName();
  if (!s.IsEmpty() && s[0] == ':')
    s.Delete(0);
  return s;
}
*/

static void Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(const MY_WIN32_FIND_STREAM_DATA &sd, CStreamInfo &si)
{
  si.Size = (UInt64)sd.StreamSize.QuadPart;
  si.Name = sd.cStreamName;
}

/*
  WinXP-64 FindFirstStream():
  ""      -  ERROR_PATH_NOT_FOUND
  folder\ -  OK
  folder  -  OK
  \       -  OK
  c:\     -  OK
  c:      -  OK, if current dir is ROOT     ( c:\ )
  c:      -  OK, if current dir is NOT ROOT ( c:\folder )
  \\Server\Share   - OK
  \\Server\Share\  - OK

  \\               - ERROR_INVALID_NAME
  \\Server         - ERROR_INVALID_NAME
  \\Server\        - ERROR_INVALID_NAME
*/

bool CFindStream::FindFirst(CFSTR path, CStreamInfo &si)
{
  if (!Close())
    return false;
#ifdef Z7_USE_DYN_FindFirstStream
  if (!g_FindFirstStreamW)
  {
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
  }
#endif
  {
    MY_WIN32_FIND_STREAM_DATA sd;
    SetLastError(0);
    IF_USE_MAIN_PATH
      _handle = MY_FindFirstStreamW(fs2us(path), My_FindStreamInfoStandard, &sd, 0);
    if (_handle == INVALID_HANDLE_VALUE)
    {
      if (::GetLastError() == ERROR_HANDLE_EOF)
        return false;
      // long name can be tricky for path like ".\dirName".
      #ifdef Z7_LONG_PATH
      if (USE_SUPER_PATH)
      {
        UString superPath;
        if (GetSuperPath(path, superPath, USE_MAIN_PATH))
          _handle = MY_FindFirstStreamW(superPath, My_FindStreamInfoStandard, &sd, 0);
      }
      #endif
    }
    if (_handle == INVALID_HANDLE_VALUE)
      return false;
    Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(sd, si);
  }
  return true;
}

bool CFindStream::FindNext(CStreamInfo &si)
{
#ifdef Z7_USE_DYN_FindFirstStream
  if (!g_FindNextStreamW)
  {
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
  }
#endif
  {
    MY_WIN32_FIND_STREAM_DATA sd;
    if (!MY_FindNextStreamW(_handle, &sd))
      return false;
    Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(sd, si);
  }
  return true;
}

bool CStreamEnumerator::Next(CStreamInfo &si, bool &found)
{
  bool res;
  if (_find.IsHandleAllocated())
    res = _find.FindNext(si);
  else
    res = _find.FindFirst(_filePath, si);
  if (res)
  {
    found = true;
    return true;
  }
  found = false;
  return (::GetLastError() == ERROR_HANDLE_EOF);
}

#endif


/*
WinXP-64 GetFileAttributes():
  If the function fails, it returns INVALID_FILE_ATTRIBUTES and use GetLastError() to get error code

  \    - OK
  C:\  - OK, if there is such drive,
  D:\  - ERROR_PATH_NOT_FOUND, if there is no such drive,

  C:\folder     - OK
  C:\folder\    - OK
  C:\folderBad  - ERROR_FILE_NOT_FOUND

  \\Server\BadShare  - ERROR_BAD_NETPATH
  \\Server\Share     - WORKS OK, but MSDN says:
                          GetFileAttributes for a network share, the function fails, and GetLastError
                          returns ERROR_BAD_NETPATH. You must specify a path to a subfolder on that share.
*/

DWORD GetFileAttrib(CFSTR path)
{
  #ifndef _UNICODE
  if (!g_IsNT)
    return ::GetFileAttributes(fs2fas(path));
  else
  #endif
  {
    IF_USE_MAIN_PATH
    {
      DWORD dw = ::GetFileAttributesW(fs2us(path));
      if (dw != INVALID_FILE_ATTRIBUTES)
        return dw;
    }
    #ifdef Z7_LONG_PATH
    if (USE_SUPER_PATH)
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        return ::GetFileAttributesW(superPath);
    }
    #endif
    return INVALID_FILE_ATTRIBUTES;
  }
}

/* if path is "c:" or "c::" then CFileInfo::Find() returns name of current folder for that disk
   so instead of absolute path we have relative path in Name. That is not good in some calls */

/* In CFileInfo::Find() we want to support same names for alt streams as in CreateFile(). */

/* CFileInfo::Find()
We alow the following paths (as FindFirstFile):
  C:\folder
  c:                      - if current dir is NOT ROOT ( c:\folder )

also we support paths that are not supported by FindFirstFile:
  \
  \\.\c:
  c:\                     - Name will be without tail slash ( c: )
  \\?\c:\                 - Name will be without tail slash ( c: )
  \\Server\Share
  \\?\UNC\Server\Share

  c:\folder:stream  - Name = folder:stream
  c:\:stream        - Name = :stream
  c::stream         - Name = c::stream
*/

bool CFileInfo::Find(CFSTR path, bool followLink)
{
  #ifdef Z7_DEVICE_FILE
  
  if (IS_PATH_SEPAR(path[0]) &&
      IS_PATH_SEPAR(path[1]) &&
      path[2] == '.' &&
      path[3] == 0)
  {
    // 22.00 : it's virtual directory for devices
    // IsDevice = true;
    ClearBase();
    Name = path + 2;
    Attrib = FILE_ATTRIBUTE_DIRECTORY;
    return true;
  }
  
  if (IsDevicePath(path))
  {
    ClearBase();
    Name = path + 4;
    IsDevice = true;
    
    if (NName::IsDrivePath2(path + 4) && path[6] == 0)
    {
      FChar drive[4] = { path[4], ':', '\\', 0 };
      UInt64 clusterSize, totalSize, freeSize;
      if (NSystem::MyGetDiskFreeSpace(drive, clusterSize, totalSize, freeSize))
      {
        Size = totalSize;
        return true;
      }
    }

    NIO::CInFile inFile;
    // ::OutputDebugStringW(path);
    if (!inFile.Open(path))
      return false;
    // ::OutputDebugStringW(L"---");
    if (inFile.SizeDefined)
      Size = inFile.Size;
    return true;
  }
  #endif

  #if defined(_WIN32) && !defined(UNDER_CE)

  const int colonPos = FindAltStreamColon(path);
  if (colonPos >= 0 && path[(unsigned)colonPos + 1] != 0)
  {
    UString streamName = fs2us(path + (unsigned)colonPos);
    FString filePath (path);
    filePath.DeleteFrom((unsigned)colonPos);
    /* we allow both cases:
      name:stream
      name:stream:$DATA
    */
    const unsigned kPostfixSize = 6;
    if (streamName.Len() <= kPostfixSize
        || !StringsAreEqualNoCase_Ascii(streamName.RightPtr(kPostfixSize), ":$DATA"))
      streamName += ":$DATA";

    bool isOk = true;
    
    if (IsDrivePath2(filePath) &&
        (colonPos == 2 || (colonPos == 3 && filePath[2] == '\\')))
    {
      // FindFirstFile doesn't work for "c:\" and for "c:" (if current dir is ROOT)
      ClearBase();
      Name.Empty();
      if (colonPos == 2)
        Name = filePath;
    }
    else
      isOk = Find(filePath, followLink); // check it (followLink)

    if (isOk)
    {
      Attrib &= ~(DWORD)(FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT);
      Size = 0;
      CStreamEnumerator enumerator(filePath);
      for (;;)
      {
        CStreamInfo si;
        bool found;
        if (!enumerator.Next(si, found))
          return false;
        if (!found)
        {
          ::SetLastError(ERROR_FILE_NOT_FOUND);
          return false;
        }
        if (si.Name.IsEqualTo_NoCase(streamName))
        {
          // we delete postfix, if alt stream name is not "::$DATA"
          if (si.Name.Len() > kPostfixSize + 1)
            si.Name.DeleteFrom(si.Name.Len() - kPostfixSize);
          Name += us2fs(si.Name);
          Size = si.Size;
          IsAltStream = true;
          return true;
        }
      }
    }
  }
  
  #endif

  CFindFile finder;

  #if defined(_WIN32) && !defined(UNDER_CE)
  {
    /*
    DWORD lastError = GetLastError();
    if (lastError == ERROR_FILE_NOT_FOUND
        || lastError == ERROR_BAD_NETPATH  // XP64: "\\Server\Share"
        || lastError == ERROR_BAD_NET_NAME // Win7: "\\Server\Share"
        || lastError == ERROR_INVALID_NAME // XP64: "\\?\UNC\Server\Share"
        || lastError == ERROR_BAD_PATHNAME // Win7: "\\?\UNC\Server\Share"
        )
    */
    
    unsigned rootSize = 0;
    if (IsSuperPath(path))
      rootSize = kSuperPathPrefixSize;
    
    if (NName::IsDrivePath(path + rootSize) && path[rootSize + 3] == 0)
    {
      DWORD attrib = GetFileAttrib(path);
      if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0)
      {
        ClearBase();
        Attrib = attrib;
        Name = path + rootSize;
        Name.DeleteFrom(2);
        if (!Fill_From_ByHandleFileInfo(path))
        {
        }
        return true;
      }
    }
    else if (IS_PATH_SEPAR(path[0]))
    {
      if (path[1] == 0)
      {
        DWORD attrib = GetFileAttrib(path);
        if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
          ClearBase();
          Name.Empty();
          Attrib = attrib;
          return true;
        }
      }
      else
      {
        const unsigned prefixSize = GetNetworkServerPrefixSize(path);
        if (prefixSize > 0 && path[prefixSize] != 0)
        {
          if (NName::FindSepar(path + prefixSize) < 0)
          {
            if (Fill_From_ByHandleFileInfo(path))
            {
              Name = path + prefixSize;
              return true;
            }

            FString s (path);
            s.Add_PathSepar();
            s.Add_Char('*'); // CHAR_ANY_MASK
            bool isOK = false;
            if (finder.FindFirst(s, *this))
            {
              if (Name.IsEqualTo("."))
              {
                Name = path + prefixSize;
                return true;
              }
              isOK = true;
              /* if "\\server\share" maps to root folder "d:\", there is no "." item.
                 But it's possible that there are another items */
            }
            {
              const DWORD attrib = GetFileAttrib(path);
              if (isOK || (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0))
              {
                ClearBase();
                if (attrib != INVALID_FILE_ATTRIBUTES)
                  Attrib = attrib;
                else
                  SetAsDir();
                Name = path + prefixSize;
                return true;
              }
            }
            // ::SetLastError(lastError);
          }
        }
      }
    }
  }
  #endif

  bool res = finder.FindFirst(path, *this);
  if (!followLink
      || !res
      || !HasReparsePoint())
    return res;

  // return FollowReparse(path, IsDir());
  return Fill_From_ByHandleFileInfo(path);
/*
  // Fill_From_ByHandleFileInfo returns false (with Access Denied error),
  // if there is reparse link file (not directory reparse item).
  if (Fill_From_ByHandleFileInfo(path))
    return true;
  return HasReparsePoint();
*/
}

bool CFileInfoBase::Fill_From_ByHandleFileInfo(CFSTR path)
{
  BY_HANDLE_FILE_INFORMATION info;
  if (!NIO::CFileBase::GetFileInformation(path, &info))
    return false;
  {
    Size = (((UInt64)info.nFileSizeHigh) << 32) + info.nFileSizeLow;
    CTime = info.ftCreationTime;
    ATime = info.ftLastAccessTime;
    MTime = info.ftLastWriteTime;
    Attrib = info.dwFileAttributes;
    return true;
  }
}

/*
bool CFileInfo::FollowReparse(CFSTR path, bool isDir)
{
  if (isDir)
  {
    FString prefix = path;
    prefix.Add_PathSepar();

    // "folder/." refers to folder itself. So we can't use that path
    // we must use enumerator and search "." item
    CEnumerator enumerator;
    enumerator.SetDirPrefix(prefix);
    for (;;)
    {
      CFileInfo fi;
      if (!enumerator.NextAny(fi))
        break;
      if (fi.Name.IsEqualTo_Ascii_NoCase("."))
      {
        // we can copy preperies;
        CTime = fi.CTime;
        ATime = fi.ATime;
        MTime = fi.MTime;
        Attrib = fi.Attrib;
        Size = fi.Size;
        return true;
      }
      break;
    }
    // LastError(lastError);
    return false;
  }

  {
    NIO::CInFile inFile;
    if (inFile.Open(path))
    {
      BY_HANDLE_FILE_INFORMATION info;
      if (inFile.GetFileInformation(&info))
      {
        ClearBase();
        Size = (((UInt64)info.nFileSizeHigh) << 32) + info.nFileSizeLow;
        CTime = info.ftCreationTime;
        ATime = info.ftLastAccessTime;
        MTime = info.ftLastWriteTime;
        Attrib = info.dwFileAttributes;
        return true;
      }
    }
    return false;
  }
}
*/

bool DoesFileExist_Raw(CFSTR name)
{
  CFileInfo fi;
  return fi.Find(name) && !fi.IsDir();
}

bool DoesFileExist_FollowLink(CFSTR name)
{
  CFileInfo fi;
  return fi.Find_FollowLink(name) && !fi.IsDir();
}

bool DoesDirExist(CFSTR name, bool followLink)
{
  CFileInfo fi;
  return fi.Find(name, followLink) && fi.IsDir();
}

bool DoesFileOrDirExist(CFSTR name)
{
  CFileInfo fi;
  return fi.Find(name);
}


void CEnumerator::SetDirPrefix(const FString &dirPrefix)
{
  _wildcard = dirPrefix;
  _wildcard.Add_Char('*');
}

bool CEnumerator::NextAny(CFileInfo &fi)
{
  if (_findFile.IsHandleAllocated())
    return _findFile.FindNext(fi);
  else
    return _findFile.FindFirst(_wildcard, fi);
}

bool CEnumerator::Next(CFileInfo &fi)
{
  for (;;)
  {
    if (!NextAny(fi))
      return false;
    if (!fi.IsDots())
      return true;
  }
}

bool CEnumerator::Next(CFileInfo &fi, bool &found)
{
  /*
  for (;;)
  {
    if (!NextAny(fi))
      break;
    if (!fi.IsDots())
    {
      found = true;
      return true;
    }
  }
  */

  if (Next(fi))
  {
    found = true;
    return true;
  }

  found = false;
  DWORD lastError = ::GetLastError();
  if (_findFile.IsHandleAllocated())
    return (lastError == ERROR_NO_MORE_FILES);
  // we support the case for empty root folder: FindFirstFile("c:\\*") returns ERROR_FILE_NOT_FOUND
  if (lastError == ERROR_FILE_NOT_FOUND)
    return true;
  if (lastError == ERROR_ACCESS_DENIED)
  {
    // here we show inaccessible root system folder as empty folder to eliminate redundant user warnings
    const char *s = "System Volume Information" STRING_PATH_SEPARATOR "*";
    const int len = (int)strlen(s);
    const int delta = (int)_wildcard.Len() - len;
    if (delta == 0 || (delta > 0 && IS_PATH_SEPAR(_wildcard[(unsigned)delta - 1])))
      if (StringsAreEqual_Ascii(_wildcard.Ptr((unsigned)delta), s))
        return true;
  }
  return false;
}


////////////////////////////////
// CFindChangeNotification
// FindFirstChangeNotification can return 0. MSDN doesn't tell about it.

bool CFindChangeNotification::Close() throw()
{
  if (!IsHandleAllocated())
    return true;
  if (!::FindCloseChangeNotification(_handle))
    return false;
  _handle = INVALID_HANDLE_VALUE;
  return true;
}
           
HANDLE CFindChangeNotification::FindFirst(CFSTR path, bool watchSubtree, DWORD notifyFilter)
{
  #ifndef _UNICODE
  if (!g_IsNT)
    _handle = ::FindFirstChangeNotification(fs2fas(path), BoolToBOOL(watchSubtree), notifyFilter);
  else
  #endif
  {
    IF_USE_MAIN_PATH
    _handle = ::FindFirstChangeNotificationW(fs2us(path), BoolToBOOL(watchSubtree), notifyFilter);
    #ifdef Z7_LONG_PATH
    if (!IsHandleAllocated())
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        _handle = ::FindFirstChangeNotificationW(superPath, BoolToBOOL(watchSubtree), notifyFilter);
    }
    #endif
  }
  return _handle;
}

#ifndef UNDER_CE

bool MyGetLogicalDriveStrings(CObjectVector<FString> &driveStrings)
{
  driveStrings.Clear();
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    driveStrings.Clear();
    UINT32 size = GetLogicalDriveStrings(0, NULL);
    if (size == 0)
      return false;
    CObjArray<char> buf(size);
    UINT32 newSize = GetLogicalDriveStrings(size, buf);
    if (newSize == 0 || newSize > size)
      return false;
    AString s;
    UINT32 prev = 0;
    for (UINT32 i = 0; i < newSize; i++)
    {
      if (buf[i] == 0)
      {
        s = buf + prev;
        prev = i + 1;
        driveStrings.Add(fas2fs(s));
      }
    }
    return prev == newSize;
  }
  else
  #endif
  {
    UINT32 size = GetLogicalDriveStringsW(0, NULL);
    if (size == 0)
      return false;
    CObjArray<wchar_t> buf(size);
    UINT32 newSize = GetLogicalDriveStringsW(size, buf);
    if (newSize == 0 || newSize > size)
      return false;
    UString s;
    UINT32 prev = 0;
    for (UINT32 i = 0; i < newSize; i++)
    {
      if (buf[i] == 0)
      {
        s = buf + prev;
        prev = i + 1;
        driveStrings.Add(us2fs(s));
      }
    }
    return prev == newSize;
  }
}

#endif // UNDER_CE



#else // _WIN32

// ---------- POSIX ----------

static int MY_lstat(CFSTR path, struct stat *st, bool followLink)
{
  memset(st, 0, sizeof(*st));
  int res;
  // #ifdef ENV_HAVE_LSTAT
  if (/* global_use_lstat && */ !followLink)
  {
    // printf("\nlstat\n");
    res = lstat(path, st);
  }
  else
  // #endif
  {
    // printf("\nstat\n");
    res = stat(path, st);
  }
#if 0
#if defined(__clang__) && __clang_major__ >= 14
  #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
#endif

  printf("\n st_dev = %lld", (long long)(st->st_dev));
  printf("\n st_ino = %lld", (long long)(st->st_ino));
  printf("\n st_mode = %llx", (long long)(st->st_mode));
  printf("\n st_nlink = %lld", (long long)(st->st_nlink));
  printf("\n st_uid = %lld", (long long)(st->st_uid));
  printf("\n st_gid = %lld", (long long)(st->st_gid));
  printf("\n st_size = %lld", (long long)(st->st_size));
  printf("\n st_blksize = %lld", (long long)(st->st_blksize));
  printf("\n st_blocks = %lld", (long long)(st->st_blocks));
  printf("\n st_ctim = %lld", (long long)(ST_CTIME((*st)).tv_sec));
  printf("\n st_mtim = %lld", (long long)(ST_MTIME((*st)).tv_sec));
  printf("\n st_atim = %lld", (long long)(ST_ATIME((*st)).tv_sec));
     printf(S_ISFIFO(st->st_mode) ? "\n FIFO" : "\n NO FIFO");
  printf("\n");
#endif

  return res;
}


static const char *Get_Name_from_Path(CFSTR path) throw()
{
  size_t len = strlen(path);
  if (len == 0)
    return path;
  const char *p = path + len - 1;
  {
    if (p == path)
      return path;
    p--;
  }
  for (;;)
  {
    char c = *p;
    if (IS_PATH_SEPAR(c))
      return p + 1;
    if (p == path)
      return path;
    p--;
  }
}


UInt32 Get_WinAttribPosix_From_PosixMode(UInt32 mode)
{
  UInt32 attrib = S_ISDIR(mode) ?
      FILE_ATTRIBUTE_DIRECTORY :
      FILE_ATTRIBUTE_ARCHIVE;
  if ((mode & 0222) == 0) // S_IWUSR in p7zip
    attrib |= FILE_ATTRIBUTE_READONLY;
  return attrib | FILE_ATTRIBUTE_UNIX_EXTENSION | ((mode & 0xFFFF) << 16);
}

/*
UInt32 Get_WinAttrib_From_stat(const struct stat &st)
{
  UInt32 attrib = S_ISDIR(st.st_mode) ?
    FILE_ATTRIBUTE_DIRECTORY :
    FILE_ATTRIBUTE_ARCHIVE;

  if ((st.st_mode & 0222) == 0) // check it !!!
    attrib |= FILE_ATTRIBUTE_READONLY;

  attrib |= FILE_ATTRIBUTE_UNIX_EXTENSION + ((st.st_mode & 0xFFFF) << 16);
  return attrib;
}
*/

void CFileInfoBase::SetFrom_stat(const struct stat &st)
{
  // IsDevice = false;

  if (S_ISDIR(st.st_mode))
  {
    Size = 0;
  }
  else
  {
    Size = (UInt64)st.st_size; // for a symbolic link, size = size of filename
  }

  // Attrib = Get_WinAttribPosix_From_PosixMode(st.st_mode);

  // NTime::UnixTimeToFileTime(st.st_ctime, CTime);
  // NTime::UnixTimeToFileTime(st.st_mtime, MTime);
  // NTime::UnixTimeToFileTime(st.st_atime, ATime);
  #ifdef __APPLE__
  // #ifdef _DARWIN_FEATURE_64_BIT_INODE
  /*
    here we can use birthtime instead of st_ctimespec.
    but we use st_ctimespec for compatibility with previous versions and p7zip.
    st_birthtimespec in OSX
    st_birthtim : at FreeBSD, NetBSD
  */
  // timespec_To_FILETIME(st.st_birthtimespec, CTime);
  // #else
  // timespec_To_FILETIME(st.st_ctimespec, CTime);
  // #endif
  // timespec_To_FILETIME(st.st_mtimespec, MTime);
  // timespec_To_FILETIME(st.st_atimespec, ATime);
  CTime = st.st_ctimespec;
  MTime = st.st_mtimespec;
  ATime = st.st_atimespec;

  #elif defined(__QNXNTO__) && defined(__ARM__) && !defined(__aarch64__)
  
  // CTime = ST_CTIME(st);
  // MTime = ST_MTIME(st);
  // ATime = ST_ATIME(st);
  CTime.tv_sec = st.st_ctime;  CTime.tv_nsec = 0;
  MTime.tv_sec = st.st_mtime;  MTime.tv_nsec = 0;
  ATime.tv_sec = st.st_atime;  ATime.tv_nsec = 0;

  #else
  // timespec_To_FILETIME(st.st_ctim, CTime, &CTime_ns100);
  // timespec_To_FILETIME(st.st_mtim, MTime, &MTime_ns100);
  // timespec_To_FILETIME(st.st_atim, ATime, &ATime_ns100);
  CTime = st.st_ctim;
  MTime = st.st_mtim;
  ATime = st.st_atim;

  #endif

  dev = st.st_dev;
  ino = st.st_ino;
  mode = st.st_mode;
  nlink = st.st_nlink;
  uid = st.st_uid;
  gid = st.st_gid;
  rdev = st.st_rdev;

  /*
  printf("\n sizeof timespec = %d", (int)sizeof(timespec));
  printf("\n sizeof st_rdev = %d", (int)sizeof(rdev));
  printf("\n sizeof st_ino = %d", (int)sizeof(ino));
  printf("\n sizeof mode_t = %d", (int)sizeof(mode_t));
  printf("\n sizeof nlink_t = %d", (int)sizeof(nlink_t));
  printf("\n sizeof uid_t = %d", (int)sizeof(uid_t));
  printf("\n");
  */
  /*
  printf("\n st_rdev = %llx", (long long)rdev);
  printf("\n st_dev  = %llx", (long long)dev);
  printf("\n dev  : major = %5x minor = %5x", (unsigned)major(dev), (unsigned)minor(dev));
  printf("\n st_ino = %lld", (long long)(ino));
  printf("\n rdev : major = %5x minor = %5x", (unsigned)major(rdev), (unsigned)minor(rdev));
  printf("\n size = %lld \n", (long long)(Size));
  printf("\n");
  */
}

/*
int Uid_To_Uname(uid_t uid, AString &name)
{
  name.Empty();
  struct passwd *passwd;

  if (uid != 0 && uid == cached_no_such_uid)
    {
      *uname = xstrdup ("");
      return;
    }

  if (!cached_uname || uid != cached_uid)
    {
      passwd = getpwuid (uid);
      if (passwd)
  {
    cached_uid = uid;
    assign_string (&cached_uname, passwd->pw_name);
  }
      else
  {
    cached_no_such_uid = uid;
    *uname = xstrdup ("");
    return;
  }
    }
  *uname = xstrdup (cached_uname);
}
*/

bool CFileInfo::Find_DontFill_Name(CFSTR path, bool followLink)
{
  struct stat st;
  if (MY_lstat(path, &st, followLink) != 0)
    return false;
  // printf("\nFind_DontFill_Name : name=%s\n", path);
  SetFrom_stat(st);
  return true;
}


bool CFileInfo::Find(CFSTR path, bool followLink)
{
  // printf("\nCEnumerator::Find() name = %s\n", path);
  if (!Find_DontFill_Name(path, followLink))
    return false;

  // printf("\nOK\n");

  Name = Get_Name_from_Path(path);
  if (!Name.IsEmpty())
  {
    char c = Name.Back();
    if (IS_PATH_SEPAR(c))
      Name.DeleteBack();
  }
  return true;
}


bool DoesFileExist_Raw(CFSTR name)
{
  // FIXME for symbolic links.
  struct stat st;
  if (MY_lstat(name, &st, false) != 0)
    return false;
  return !S_ISDIR(st.st_mode);
}

bool DoesFileExist_FollowLink(CFSTR name)
{
  // FIXME for symbolic links.
  struct stat st;
  if (MY_lstat(name, &st, true) != 0)
    return false;
  return !S_ISDIR(st.st_mode);
}

bool DoesDirExist(CFSTR name, bool followLink)
{
  struct stat st;
  if (MY_lstat(name, &st, followLink) != 0)
    return false;
  return S_ISDIR(st.st_mode);
}

bool DoesFileOrDirExist(CFSTR name)
{
  struct stat st;
  if (MY_lstat(name, &st, false) != 0)
    return false;
  return true;
}


CEnumerator::~CEnumerator()
{
  if (_dir)
    closedir(_dir);
}

void CEnumerator::SetDirPrefix(const FString &dirPrefix)
{
  _wildcard = dirPrefix;
}

bool CDirEntry::IsDots() const throw()
{
  /* some systems (like CentOS 7.x on XFS) have (Type == DT_UNKNOWN)
     we can call fstatat() for that case, but we use only (Name) check here */

#if !defined(_AIX) && !defined(__sun) && !defined(__QNXNTO__)
  if (Type != DT_DIR && Type != DT_UNKNOWN)
    return false;
#endif

  return Name.Len() != 0
      && Name.Len() <= 2
      && Name[0] == '.'
      && (Name.Len() == 1 || Name[1] == '.');
}


bool CEnumerator::NextAny(CDirEntry &fi, bool &found)
{
  found = false;

  if (!_dir)
  {
    const char *w = "./";
    if (!_wildcard.IsEmpty())
      w = _wildcard.Ptr();
    _dir = ::opendir((const char *)w);
    if (_dir == NULL)
      return false;
  }

  // To distinguish end of stream from an error, we must set errno to zero before readdir()
  errno = 0;

  struct dirent *de = readdir(_dir);
  if (!de)
  {
    if (errno == 0)
      return true; // it's end of stream, and we report it with (found = false)
    // it's real error
    return false;
  }

  fi.iNode = de->d_ino;
  
#if !defined(_AIX) && !defined(__sun) && !defined(__QNXNTO__)
  fi.Type = de->d_type;
  /* some systems (like CentOS 7.x on XFS) have (Type == DT_UNKNOWN)
     we can set (Type) from fstatat() in that case.
     But (Type) is not too important. So we don't set it here with slow fstatat() */
  /*
  // fi.Type = DT_UNKNOWN; // for debug
  if (fi.Type == DT_UNKNOWN)
  {
    struct stat st;
    if (fstatat(dirfd(_dir), de->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0)
      if (S_ISDIR(st.st_mode))
        fi.Type = DT_DIR;
  }
  */
#endif
  
  /*
  if (de->d_type == DT_DIR)
    fi.Attrib = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_UNIX_EXTENSION | ((UInt32)(S_IFDIR) << 16);
  else if (de->d_type < 16)
    fi.Attrib = FILE_ATTRIBUTE_UNIX_EXTENSION | ((UInt32)(de->d_type) << (16 + 12));
  */
  fi.Name = de->d_name;

  /*
  printf("\nCEnumerator::NextAny; len = %d %s \n", (int)fi.Name.Len(), fi.Name.Ptr());
  for (unsigned i = 0; i < fi.Name.Len(); i++)
    printf (" %02x", (unsigned)(Byte)de->d_name[i]);
  printf("\n");
  */

  found = true;
  return true;
}


bool CEnumerator::Next(CDirEntry &fi, bool &found)
{
  // printf("\nCEnumerator::Next()\n");
  // PrintName("Next", "");
  for (;;)
  {
    if (!NextAny(fi, found))
      return false;
    if (!found)
      return true;
    if (!fi.IsDots())
    {
      /*
      if (!NeedFullStat)
        return true;
      // we silently skip error file here - it can be wrong link item
      if (fi.Find_DontFill_Name(path))
        return true;
      */
      return true;
    }
  }
}

/*
bool CEnumerator::Next(CDirEntry &fileInfo, bool &found)
{
  bool found;
  if (!Next(fi, found))
    return false;
  return found;
}
*/

bool CEnumerator::Fill_FileInfo(const CDirEntry &de, CFileInfo &fileInfo, bool followLink) const
{
  // printf("\nCEnumerator::Fill_FileInfo()\n");
  struct stat st;
  // probably it's OK to use fstatat() even if it changes file position dirfd(_dir)
  int res = fstatat(dirfd(_dir), de.Name, &st, followLink ? 0 : AT_SYMLINK_NOFOLLOW);
  // if fstatat() is not supported, we can use stat() / lstat()
  
  /*
  const FString path = _wildcard + s;
  int res = MY_lstat(path, &st, followLink);
  */
  
  if (res != 0)
    return false;
  // printf("\nname=%s\n", de.Name.Ptr());
  fileInfo.SetFrom_stat(st);
  fileInfo.Name = de.Name;
  return true;
}

#endif // _WIN32

}}}

/* ================ unit: CPP/Windows/FileIO.cpp ================ */
// Windows/FileIO.cpp

// amalgamation: header emitted in prologue

#ifdef Z7_DEVICE_FILE
// amalgamation: header emitted in prologue
#endif

// #include <stdio.h>

/*
#ifndef _WIN32
// for ioctl BLKGETSIZE64
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif
*/

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

HRESULT GetLastError_noZero_HRESULT()
{
  const DWORD res = ::GetLastError();
  if (res == 0)
    return E_FAIL;
  return HRESULT_FROM_WIN32(res);
}

#ifdef _WIN32

#ifndef _UNICODE
extern bool g_IsNT;
#endif

using namespace NWindows;
using namespace NFile;
using namespace NName;

namespace NWindows {
namespace NFile {

#ifdef Z7_DEVICE_FILE

namespace NSystem
{
bool MyGetDiskFreeSpace(CFSTR rootPath, UInt64 &clusterSize, UInt64 &totalSize, UInt64 &freeSize);
}
#endif

namespace NIO {

/*
WinXP-64 CreateFile():
  ""             -  ERROR_PATH_NOT_FOUND
  :stream        -  OK
  .:stream       -  ERROR_PATH_NOT_FOUND
  .\:stream      -  OK
  
  folder\:stream -  ERROR_INVALID_NAME
  folder:stream  -  OK

  c:\:stream     -  OK

  c::stream      -  ERROR_INVALID_NAME, if current dir is NOT ROOT ( c:\dir1 )
  c::stream      -  OK,                 if current dir is ROOT     ( c:\ )
*/

bool CFileBase::Create(CFSTR path, DWORD desiredAccess,
    DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
{
  if (!Close())
    return false;

  #ifdef Z7_DEVICE_FILE
  IsDeviceFile = false;
  #endif

  #ifndef _UNICODE
  if (!g_IsNT)
  {
    _handle = ::CreateFile(fs2fas(path), desiredAccess, shareMode,
        (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, flagsAndAttributes, (HANDLE)NULL);
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      _handle = ::CreateFileW(fs2us(path), desiredAccess, shareMode,
        (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, flagsAndAttributes, (HANDLE)NULL);
    #ifdef Z7_LONG_PATH
    if (_handle == INVALID_HANDLE_VALUE && USE_SUPER_PATH)
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        _handle = ::CreateFileW(superPath, desiredAccess, shareMode,
            (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, flagsAndAttributes, (HANDLE)NULL);
    }
    #endif
  }
  
  /*
  #ifndef UNDER_CE
  #ifndef Z7_SFX
  if (_handle == INVALID_HANDLE_VALUE)
  {
    // it's debug hack to open symbolic links in Windows XP and WSL links in Windows 10
    DWORD lastError = GetLastError();
    if (lastError == ERROR_CANT_ACCESS_FILE)
    {
      CByteBuffer buf;
      if (NIO::GetReparseData(path, buf, NULL))
      {
        CReparseAttr attr;
        if (attr.Parse(buf, buf.Size()))
        {
          FString dirPrefix, fileName;
          if (NDir::GetFullPathAndSplit(path, dirPrefix, fileName))
          {
            FString fullPath;
            if (GetFullPath(dirPrefix, us2fs(attr.GetPath()), fullPath))
            {
              // FIX IT: recursion levels must be restricted
              return Create(fullPath, desiredAccess,
                shareMode, creationDisposition, flagsAndAttributes);
            }
          }
        }
      }
      SetLastError(lastError);
    }
  }
  #endif
  #endif
  */

  return (_handle != INVALID_HANDLE_VALUE);
}

bool CFileBase::Close() throw()
{
  if (_handle == INVALID_HANDLE_VALUE)
    return true;
#if 0
  if (!IsStdStream)
#endif
  {
    if (!::CloseHandle(_handle))
      return false;
  }
#if 0
  IsStdStream = false;
  IsStdPipeStream = false;
#endif
  _handle = INVALID_HANDLE_VALUE;
  return true;
}

bool CFileBase::GetLength(UInt64 &length) const throw()
{
  #ifdef Z7_DEVICE_FILE
  if (IsDeviceFile && SizeDefined)
  {
    length = Size;
    return true;
  }
  #endif

  DWORD high = 0;
  const DWORD low = ::GetFileSize(_handle, &high);
  if (low == INVALID_FILE_SIZE)
    if (::GetLastError() != NO_ERROR)
      return false;
  length = (((UInt64)high) << 32) + low;
  return true;

  /*
  LARGE_INTEGER fileSize;
  // GetFileSizeEx() is unsupported in 98/ME/NT, and supported in Win2000+
  if (!GetFileSizeEx(_handle, &fileSize))
    return false;
  length = (UInt64)fileSize.QuadPart;
  return true;
  */
}


/* Specification for SetFilePointer():
   
   If a new file pointer is a negative value,
   {
     the function fails,
     the file pointer is not moved,
     the code returned by GetLastError() is ERROR_NEGATIVE_SEEK.
   }

   If the hFile handle is opened with the FILE_FLAG_NO_BUFFERING flag set
   {
     an application can move the file pointer only to sector-aligned positions.
     A sector-aligned position is a position that is a whole number multiple of
     the volume sector size.
     An application can obtain a volume sector size by calling the GetDiskFreeSpace.
   }

   It is not an error to set a file pointer to a position beyond the end of the file.
   The size of the file does not increase until you call the SetEndOfFile, WriteFile, or WriteFileEx function.

   If the return value is INVALID_SET_FILE_POINTER and if lpDistanceToMoveHigh is non-NULL,
   an application must call GetLastError to determine whether or not the function has succeeded or failed.
*/

bool CFileBase::GetPosition(UInt64 &position) const throw()
{
  LONG high = 0;
  const DWORD low = ::SetFilePointer(_handle, 0, &high, FILE_CURRENT);
  if (low == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
  {
    // for error case we can set (position)  to (-1) or (0) or leave (position) unchanged.
    // position = (UInt64)(Int64)-1; // for debug
    position = 0;
    return false;
  }
  position = (((UInt64)(UInt32)high) << 32) + low;
  return true;
  // we don't want recursed GetPosition()
  // return Seek(0, FILE_CURRENT, position);
}

bool CFileBase::Seek(Int64 distanceToMove, DWORD moveMethod, UInt64 &newPosition) const throw()
{
  #ifdef Z7_DEVICE_FILE
  if (IsDeviceFile && SizeDefined && moveMethod == FILE_END)
  {
    distanceToMove += Size;
    moveMethod = FILE_BEGIN;
  }
  #endif

  LONG high = (LONG)(distanceToMove >> 32);
  const DWORD low = ::SetFilePointer(_handle, (LONG)(distanceToMove & 0xFFFFFFFF), &high, moveMethod);
  if (low == INVALID_SET_FILE_POINTER)
  {
    const DWORD lastError = ::GetLastError();
    if (lastError != NO_ERROR)
    {
      // 21.07: we set (newPosition) to real position even after error.
      GetPosition(newPosition);
      SetLastError(lastError); // restore LastError
      return false;
    }
  }
  newPosition = (((UInt64)(UInt32)high) << 32) + low;
  return true;
}

bool CFileBase::Seek(UInt64 position, UInt64 &newPosition) const throw()
{
  return Seek((Int64)position, FILE_BEGIN, newPosition);
}

bool CFileBase::SeekToBegin() const throw()
{
  UInt64 newPosition = 0;
  return Seek(0, newPosition) && (newPosition == 0);
}

bool CFileBase::SeekToEnd(UInt64 &newPosition) const throw()
{
  return Seek(0, FILE_END, newPosition);
}

// ---------- CInFile ---------

#ifdef Z7_DEVICE_FILE

void CInFile::CorrectDeviceSize()
{
  // maybe we must decrease kClusterSize to 1 << 12, if we want correct size at tail
  const UInt32 kClusterSize = 1 << 14;
  UInt64 pos = Size & ~(UInt64)(kClusterSize - 1);
  UInt64 realNewPosition;
  if (!Seek(pos, realNewPosition))
    return;
  Byte *buf = (Byte *)MidAlloc(kClusterSize);

  bool needbackward = true;

  for (;;)
  {
    UInt32 processed = 0;
    // up test is slow for "PhysicalDrive".
    // processed size for latest block for "PhysicalDrive0" is 0.
    if (!Read1(buf, kClusterSize, processed))
      break;
    if (processed == 0)
      break;
    needbackward = false;
    Size = pos + processed;
    if (processed != kClusterSize)
      break;
    pos += kClusterSize;
  }

  if (needbackward && pos != 0)
  {
    pos -= kClusterSize;
    for (;;)
    {
      // break;
      if (!Seek(pos, realNewPosition))
        break;
      if (!buf)
      {
        buf = (Byte *)MidAlloc(kClusterSize);
        if (!buf)
          break;
      }
      UInt32 processed = 0;
      // that code doesn't work for "PhysicalDrive0"
      if (!Read1(buf, kClusterSize, processed))
        break;
      if (processed != 0)
      {
        Size = pos + processed;
        break;
      }
      if (pos == 0)
        break;
      pos -= kClusterSize;
    }
  }
  MidFree(buf);
}


void CInFile::CalcDeviceSize(CFSTR s)
{
  SizeDefined = false;
  Size = 0;
  if (_handle == INVALID_HANDLE_VALUE || !IsDeviceFile)
    return;
  #ifdef UNDER_CE

  SizeDefined = true;
  Size = 128 << 20;
  
  #else
  
  PARTITION_INFORMATION partInfo;
  bool needCorrectSize = true;

  /*
    WinXP 64-bit:

    HDD \\.\PhysicalDrive0 (MBR):
      GetPartitionInfo == GeometryEx :  corrrect size? (includes tail)
      Geometry   :  smaller than GeometryEx (no tail, maybe correct too?)
      MyGetDiskFreeSpace : FAIL
      Size correction is slow and block size (kClusterSize) must be small?

    HDD partition \\.\N: (NTFS):
      MyGetDiskFreeSpace   :  Size of NTFS clusters. Same size can be calculated after correction
      GetPartitionInfo     :  size of partition data: NTFS clusters + TAIL; TAIL contains extra empty sectors and copy of first sector of NTFS
      Geometry / CdRomGeometry / GeometryEx :  size of HDD (not that partition)

    CD-ROM drive (ISO):
      MyGetDiskFreeSpace   :  correct size. Same size can be calculated after correction
      Geometry == CdRomGeometry  :  smaller than corrrect size
      GetPartitionInfo == GeometryEx :  larger than corrrect size

    Floppy \\.\a: (FAT):
      Geometry :  correct size.
      CdRomGeometry / GeometryEx / GetPartitionInfo / MyGetDiskFreeSpace - FAIL
      correction works OK for FAT.
      correction works OK for non-FAT, if kClusterSize = 512.
  */

  if (GetPartitionInfo(&partInfo))
  {
    Size = (UInt64)partInfo.PartitionLength.QuadPart;
    SizeDefined = true;
    needCorrectSize = false;
    if ((s)[0] == '\\' && (s)[1] == '\\' && (s)[2] == '.' && (s)[3] == '\\' && (s)[5] == ':' && (s)[6] == 0)
    {
      FChar path[4] = { s[4], ':', '\\', 0 };
      UInt64 clusterSize, totalSize, freeSize;
      if (NSystem::MyGetDiskFreeSpace(path, clusterSize, totalSize, freeSize))
        Size = totalSize;
      else
        needCorrectSize = true;
    }
  }
  
  if (!SizeDefined)
  {
    my_DISK_GEOMETRY_EX geomEx;
    SizeDefined = GetGeometryEx(&geomEx);
    if (SizeDefined)
      Size = (UInt64)geomEx.DiskSize.QuadPart;
    else
    {
      DISK_GEOMETRY geom;
      SizeDefined = GetGeometry(&geom);
      if (!SizeDefined)
        SizeDefined = GetCdRomGeometry(&geom);
      if (SizeDefined)
        Size = (UInt64)geom.Cylinders.QuadPart * geom.TracksPerCylinder * geom.SectorsPerTrack * geom.BytesPerSector;
    }
  }
  
  if (needCorrectSize && SizeDefined && Size != 0)
  {
    CorrectDeviceSize();
    SeekToBegin();
  }

  // SeekToBegin();
  #endif
}

// ((desiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA | GENERIC_WRITE)) == 0 &&

#define MY_DEVICE_EXTRA_CODE \
  IsDeviceFile = IsDevicePath(fileName); \
  CalcDeviceSize(fileName);
#else
#define MY_DEVICE_EXTRA_CODE
#endif

bool CInFile::Open(CFSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
{
  DWORD desiredAccess = GENERIC_READ;
  
  #ifdef _WIN32
  if (PreserveATime)
    desiredAccess |= FILE_WRITE_ATTRIBUTES;
  #endif
  
  bool res = Create(fileName, desiredAccess, shareMode, creationDisposition, flagsAndAttributes);

  #ifdef _WIN32
  if (res && PreserveATime)
  {
    FILETIME ft;
    ft.dwHighDateTime = ft.dwLowDateTime = 0xFFFFFFFF;
    ::SetFileTime(_handle, NULL, &ft, NULL);
  }
  #endif

  MY_DEVICE_EXTRA_CODE
  return res;
}

bool CInFile::OpenShared(CFSTR fileName, bool shareForWrite)
{ return Open(fileName, FILE_SHARE_READ | (shareForWrite ? FILE_SHARE_WRITE : 0), OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL); }

bool CInFile::Open(CFSTR fileName)
  { return OpenShared(fileName, false); }

// ReadFile and WriteFile functions in Windows have BUG:
// If you Read or Write 64MB or more (probably min_failure_size = 64MB - 32KB + 1)
// from/to Network file, it returns ERROR_NO_SYSTEM_RESOURCES
// (Insufficient system resources exist to complete the requested service).

// Probably in some version of Windows there are problems with other sizes:
// for 32 MB (maybe also for 16 MB).
// And message can be "Network connection was lost"

static const UInt32 kChunkSizeMax = 1 << 22;

bool CInFile::Read1(void *data, UInt32 size, UInt32 &processedSize) throw()
{
  DWORD processedLoc = 0;
  const bool res = BOOLToBool(::ReadFile(_handle, data, size, &processedLoc, NULL));
  processedSize = (UInt32)processedLoc;
  return res;
}

bool CInFile::ReadPart(void *data, UInt32 size, UInt32 &processedSize) throw()
{
#if 0
  const UInt32 chunkSizeMax = (0 || IsStdStream) ? (1 << 20) : kChunkSizeMax;
  if (size > chunkSizeMax)
      size = chunkSizeMax;
#else
  if (size > kChunkSizeMax)
      size = kChunkSizeMax;
#endif
  return Read1(data, size, processedSize);
}

bool CInFile::Read(void *data, UInt32 size, UInt32 &processedSize) throw()
{
  processedSize = 0;
  do
  {
    UInt32 processedLoc = 0;
    const bool res = ReadPart(data, size, processedLoc);
    processedSize += processedLoc;
    if (!res)
      return false;
    if (processedLoc == 0)
      return true;
    data = (void *)((Byte *)data + processedLoc);
    size -= processedLoc;
  }
  while (size);
  return true;
}

bool CInFile::ReadFull(void *data, size_t size, size_t &processedSize) throw()
{
  processedSize = 0;
  do
  {
    UInt32 processedLoc = 0;
    const UInt32 sizeLoc = (size > kChunkSizeMax ? (UInt32)kChunkSizeMax : (UInt32)size);
    const bool res = Read1(data, sizeLoc, processedLoc);
    processedSize += processedLoc;
    if (!res)
      return false;
    if (processedLoc == 0)
      return true;
    data = (void *)((Byte *)data + processedLoc);
    size -= processedLoc;
  }
  while (size);
  return true;
}

// ---------- COutFile ---------

bool COutFile::Open(CFSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
  { return CFileBase::Create(fileName, GENERIC_WRITE, shareMode, creationDisposition, flagsAndAttributes); }

bool COutFile::Open_Disposition(CFSTR fileName, DWORD creationDisposition)
  { return Open(fileName, FILE_SHARE_READ, creationDisposition, FILE_ATTRIBUTE_NORMAL); }

bool COutFile::Create_ALWAYS_with_Attribs(CFSTR fileName, DWORD flagsAndAttributes)
  { return Open(fileName, FILE_SHARE_READ, CREATE_ALWAYS, flagsAndAttributes); }

bool COutFile::SetTime(const FILETIME *cTime, const FILETIME *aTime, const FILETIME *mTime) throw()
  { return BOOLToBool(::SetFileTime(_handle, cTime, aTime, mTime)); }

bool COutFile::SetMTime(const FILETIME *mTime) throw() {  return SetTime(NULL, NULL, mTime); }

bool COutFile::WritePart(const void *data, UInt32 size, UInt32 &processedSize) throw()
{
  if (size > kChunkSizeMax)
    size = kChunkSizeMax;
  DWORD processedLoc = 0;
  bool res = BOOLToBool(::WriteFile(_handle, data, size, &processedLoc, NULL));
  processedSize = (UInt32)processedLoc;
  return res;
}

bool COutFile::Write(const void *data, UInt32 size, UInt32 &processedSize) throw()
{
  processedSize = 0;
  do
  {
    UInt32 processedLoc = 0;
    const bool res = WritePart(data, size, processedLoc);
    processedSize += processedLoc;
    if (!res)
      return false;
    if (processedLoc == 0)
      return true;
    data = (const void *)((const Byte *)data + processedLoc);
    size -= processedLoc;
  }
  while (size);
  return true;
}

bool COutFile::WriteFull(const void *data, size_t size) throw()
{
  do
  {
    UInt32 processedLoc = 0;
    const UInt32 sizeCur = (size > kChunkSizeMax ? kChunkSizeMax : (UInt32)size);
    if (!WritePart(data, sizeCur, processedLoc))
      return false;
    if (processedLoc == 0)
      return (size == 0);
    data = (const void *)((const Byte *)data + processedLoc);
    size -= processedLoc;
  }
  while (size);
  return true;
}

bool COutFile::SetEndOfFile() throw() { return BOOLToBool(::SetEndOfFile(_handle)); }

bool COutFile::SetLength(UInt64 length) throw()
{
  UInt64 newPosition;
  if (!Seek(length, newPosition))
    return false;
  if (newPosition != length)
    return false;
  return SetEndOfFile();
}

bool COutFile::SetLength_KeepPosition(UInt64 length) throw()
{
  UInt64 currentPos = 0;
  if (!GetPosition(currentPos))
    return false;
  DWORD lastError = 0;
  const bool result = SetLength(length);
  if (!result)
    lastError = GetLastError();
  UInt64 currentPos2;
  const bool result2 = Seek(currentPos, currentPos2);
  if (lastError != 0)
    SetLastError(lastError);
  return (result && result2);
}

}}}

#else // _WIN32


// POSIX

#include <fcntl.h>
#include <unistd.h>

namespace NWindows {
namespace NFile {

namespace NDir {
bool SetDirTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime);
}

namespace NIO {

bool CFileBase::OpenBinary(const char *name, int flags, mode_t mode)
{
  #ifdef O_BINARY
  flags |= O_BINARY;
  #endif

  Close();
  _handle = ::open(name, flags, mode);
  return _handle != -1;

  /*
  if (_handle == -1)
    return false;
  if (IsString1PrefixedByString2(name, "/dev/"))
  {
    // /dev/sda
    // IsDeviceFile = true; // for debug
    // SizeDefined = false;
    // SizeDefined = (GetDeviceSize_InBytes(Size) == 0);
  }
  return true;
  */
}

bool CFileBase::Close()
{
  if (_handle == -1)
    return true;
  if (close(_handle) != 0)
    return false;
  _handle = -1;
  /*
  IsDeviceFile = false;
  SizeDefined = false;
  */
  return true;
}

bool CFileBase::GetLength(UInt64 &length) const
{
  length = 0;
  // length = (UInt64)(Int64)-1; // for debug
  const off_t curPos = seekToCur();
  if (curPos == -1)
    return false;
  const off_t lengthTemp = seek(0, SEEK_END);
  seek(curPos, SEEK_SET);
  length = (UInt64)lengthTemp;

  /*
  // 22.00:
  if (lengthTemp == 1)
  if (IsDeviceFile && SizeDefined)
  {
    length = Size;
    return true;
  }
  */

  return (lengthTemp != -1);
}

off_t CFileBase::seek(off_t distanceToMove, int moveMethod) const
{
  /*
  if (IsDeviceFile && SizeDefined && moveMethod == SEEK_END)
  {
    printf("\n seek : IsDeviceFile moveMethod = %d distanceToMove = %ld\n", moveMethod, distanceToMove);
    distanceToMove += Size;
    moveMethod = SEEK_SET;
  }
  */

  // printf("\nCFileBase::seek() moveMethod = %d, distanceToMove = %lld", moveMethod, (long long)distanceToMove);
  // off_t res = ::lseek(_handle, distanceToMove, moveMethod);
  // printf("\n lseek : moveMethod = %d distanceToMove = %ld\n", moveMethod, distanceToMove);
  return ::lseek(_handle, distanceToMove, moveMethod);
  // return res;
}

off_t CFileBase::seekToBegin() const throw()
{
  return seek(0, SEEK_SET);
}

off_t CFileBase::seekToCur() const throw()
{
  return seek(0, SEEK_CUR);
}

/*
bool CFileBase::SeekToBegin() const throw()
{
  return (::seek(0, SEEK_SET) != -1);
}
*/


/////////////////////////
// CInFile

bool CInFile::Open(const char *name)
{
  return CFileBase::OpenBinary(name, O_RDONLY);
}

bool CInFile::OpenShared(const char *name, bool)
{
  return Open(name);
}


/*
int CFileBase::my_ioctl_BLKGETSIZE64(unsigned long long *numBlocks)
{
  // we can read "/sys/block/sda/size" "/sys/block/sda/sda1/size" - partition
  // #include <linux/fs.h>
  return ioctl(_handle, BLKGETSIZE64, numBlocks);
  // in block size
}

int CFileBase::GetDeviceSize_InBytes(UInt64 &size)
{
  size = 0;
  unsigned long long numBlocks;
  int res = my_ioctl_BLKGETSIZE64(&numBlocks);
  if (res == 0)
    size = numBlocks; // another blockSize s possible?
  printf("\nGetDeviceSize_InBytes res = %d, size = %lld\n", res, (long long)size);
  return res;
}
*/

/*
On Linux (32-bit and 64-bit):
read(), write() (and similar system calls) will transfer at most
0x7ffff000 = (2GiB - 4 KiB) bytes, returning the number of bytes actually transferred.
*/

static const size_t kChunkSizeMax = ((size_t)1 << 22);

ssize_t CInFile::read_part(void *data, size_t size) throw()
{
  if (size > kChunkSizeMax)
    size = kChunkSizeMax;
  return ::read(_handle, data, size);
}

bool CInFile::ReadFull(void *data, size_t size, size_t &processed) throw()
{
  processed = 0;
  do
  {
    const ssize_t res = read_part(data, size);
    if (res < 0)
      return false;
    if (res == 0)
      break;
    data = (void *)((Byte *)data + (size_t)res);
    processed += (size_t)res;
    size -= (size_t)res;
  }
  while (size);
  return true;
}


/////////////////////////
// COutFile

bool COutFile::OpenBinary_forWrite_oflag(const char *name, int oflag)
{
  Path = name; // change it : set it only if open is success.
  return OpenBinary(name, oflag, mode_for_Create);
}


/*
  windows           exist  non-exist  posix
  CREATE_NEW        Fail   Create     O_CREAT | O_EXCL
  CREATE_ALWAYS     Trunc  Create     O_CREAT | O_TRUNC
  OPEN_ALWAYS       Open   Create     O_CREAT
  OPEN_EXISTING     Open   Fail       0
  TRUNCATE_EXISTING Trunc  Fail       O_TRUNC ???

  // O_CREAT = If the file exists, this flag has no effect except as noted under O_EXCL below.
  // If O_CREAT and O_EXCL are set, open() shall fail if the file exists.
  // O_TRUNC : If the file exists and the file is successfully opened, its length shall be truncated to 0.
*/
bool COutFile::Open_EXISTING(const char *name)
  { return OpenBinary_forWrite_oflag(name, O_WRONLY); }
bool COutFile::Create_ALWAYS(const char *name)
  { return OpenBinary_forWrite_oflag(name, O_WRONLY | O_CREAT | O_TRUNC); }
bool COutFile::Create_NEW(const char *name)
  { return OpenBinary_forWrite_oflag(name, O_WRONLY | O_CREAT | O_EXCL);  }
bool COutFile::Create_ALWAYS_or_Open_ALWAYS(const char *name, bool createAlways)
{
  return OpenBinary_forWrite_oflag(name,
      createAlways ?
        O_WRONLY | O_CREAT | O_TRUNC :
        O_WRONLY | O_CREAT);
}
/*
bool COutFile::Create_ALWAYS_or_NEW(const char *name, bool createAlways)
{
  return OpenBinary_forWrite_oflag(name,
      createAlways ?
        O_WRONLY | O_CREAT | O_TRUNC :
        O_WRONLY | O_CREAT | O_EXCL);
}
bool COutFile::Open_Disposition(const char *name, DWORD creationDisposition)
{
  int flag;
  switch (creationDisposition)
  {
    case CREATE_NEW:        flag = O_WRONLY | O_CREAT | O_EXCL;  break;
    case CREATE_ALWAYS:     flag = O_WRONLY | O_CREAT | O_TRUNC;  break;
    case OPEN_ALWAYS:       flag = O_WRONLY | O_CREAT;  break;
    case OPEN_EXISTING:     flag = O_WRONLY;  break;
    case TRUNCATE_EXISTING: flag = O_WRONLY | O_TRUNC; break;
    default:
      SetLastError(EINVAL);
      return false;
  }
  return OpenBinary_forWrite_oflag(name, flag);
}
*/

ssize_t COutFile::write_part(const void *data, size_t size) throw()
{
  if (size > kChunkSizeMax)
    size = kChunkSizeMax;
  return ::write(_handle, data, size);
}

ssize_t COutFile::write_full(const void *data, size_t size, size_t &processed) throw()
{
  processed = 0;
  do
  {
    const ssize_t res = write_part(data, size);
    if (res < 0)
      return res;
    if (res == 0)
      break;
    data = (const void *)((const Byte *)data + (size_t)res);
    processed += (size_t)res;
    size -= (size_t)res;
  }
  while (size);
  return (ssize_t)processed;
}

bool COutFile::SetLength(UInt64 length) throw()
{
  const off_t len2 = (off_t)length;
  if ((Int64)length != len2)
  {
    SetLastError(EFBIG);
    return false;
  }
  // The value of the seek pointer shall not be modified by a call to ftruncate().
  const int iret = ftruncate(_handle, len2);
  return (iret == 0);
}

bool COutFile::Close()
{
  const bool res = CFileBase::Close();
  if (!res)
    return res;
  if (CTime_defined || ATime_defined || MTime_defined)
  {
    /* bool res2 = */ NWindows::NFile::NDir::SetDirTime(Path,
        CTime_defined ? &CTime : NULL,
        ATime_defined ? &ATime : NULL,
        MTime_defined ? &MTime : NULL);
  }
  return res;
}

bool COutFile::SetTime(const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime) throw()
{
  // On some OS (cygwin, MacOSX ...), you must close the file before updating times
  // return true;

  if (cTime) { CTime = *cTime; CTime_defined = true; } else CTime_defined = false;
  if (aTime) { ATime = *aTime; ATime_defined = true; } else ATime_defined = false;
  if (mTime) { MTime = *mTime; MTime_defined = true; } else MTime_defined = false;
  return true;

  /*
  struct timespec times[2];
  UNUSED_VAR(cTime)
  if (!aTime && !mTime)
    return true;
  bool needChange;
  needChange  = FiTime_To_timespec(aTime, times[0]);
  needChange |= FiTime_To_timespec(mTime, times[1]);
  if (!needChange)
    return true;
  return futimens(_handle, times) == 0;
  */
}

bool COutFile::SetMTime(const CFiTime *mTime) throw()
{
  if (mTime) { MTime = *mTime; MTime_defined = true; } else MTime_defined = false;
  return true;
}

}}}


#endif

/* ================ unit: CPP/Windows/FileName.cpp ================ */
// Windows/FileName.cpp

// amalgamation: header emitted in prologue

#ifndef _WIN32
#include <limits.h>
#include <unistd.h>
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows {
namespace NFile {
namespace NName {

#define IS_SEPAR(c) IS_PATH_SEPAR(c)

int FindSepar(const wchar_t *s) throw()
{
  for (const wchar_t *p = s;; p++)
  {
    const wchar_t c = *p;
    if (c == 0)
      return -1;
    if (IS_SEPAR(c))
      return (int)(p - s);
  }
}

#ifndef USE_UNICODE_FSTRING
int FindSepar(const FChar *s) throw()
{
  for (const FChar *p = s;; p++)
  {
    const FChar c = *p;
    if (c == 0)
      return -1;
    if (IS_SEPAR(c))
      return (int)(p - s);
  }
}
#endif

#ifndef USE_UNICODE_FSTRING
void NormalizeDirPathPrefix(FString &dirPath)
{
  if (dirPath.IsEmpty())
    return;
  if (!IsPathSepar(dirPath.Back()))
    dirPath.Add_PathSepar();
}
#endif

void NormalizeDirPathPrefix(UString &dirPath)
{
  if (dirPath.IsEmpty())
    return;
  if (!IsPathSepar(dirPath.Back()))
    dirPath.Add_PathSepar();
}


#define IS_LETTER_CHAR(c) ((((unsigned)(int)(c) | 0x20) - (unsigned)'a' <= (unsigned)('z' - 'a')))
bool IsDrivePath (const wchar_t *s) throw() { return IS_LETTER_CHAR(s[0]) && s[1] == ':' && IS_SEPAR(s[2]); }
// bool IsDriveName2(const wchar_t *s) throw() { return IS_LETTER_CHAR(s[0]) && s[1] == ':' && s[2] == 0; }

#ifdef _WIN32

bool IsDrivePath2(const wchar_t *s) throw() { return IS_LETTER_CHAR(s[0]) && s[1] == ':'; }

#ifndef USE_UNICODE_FSTRING
#ifdef Z7_LONG_PATH
static void NormalizeDirSeparators(UString &s)
{
  const unsigned len = s.Len();
  for (unsigned i = 0; i < len; i++)
    if (s[i] == '/')
      s.ReplaceOneCharAtPos(i, WCHAR_PATH_SEPARATOR);
}
#endif
#endif

void NormalizeDirSeparators(FString &s)
{
  const unsigned len = s.Len();
  for (unsigned i = 0; i < len; i++)
    if (s[i] == '/')
      s.ReplaceOneCharAtPos(i, FCHAR_PATH_SEPARATOR);
}

bool IsAltPathPrefix(CFSTR s) throw()
{
  unsigned len = MyStringLen(s);
  if (len == 0)
    return false;
  if (s[len - 1] != ':')
    return false;

  #if defined(_WIN32) && !defined(UNDER_CE)
  if (IsDevicePath(s))
    return false;
  if (IsSuperPath(s))
  {
    s += kSuperPathPrefixSize;
    len -= kSuperPathPrefixSize;
  }
  if (len == 2 && IsDrivePath2(s))
    return false;
  #endif

  return true;
}

#endif // _WIN32


const char * const kSuperPathPrefix =
    STRING_PATH_SEPARATOR
    STRING_PATH_SEPARATOR "?"
    STRING_PATH_SEPARATOR;
#ifdef Z7_LONG_PATH
static const char * const kSuperUncPrefix =
    STRING_PATH_SEPARATOR
    STRING_PATH_SEPARATOR "?"
    STRING_PATH_SEPARATOR "UNC"
    STRING_PATH_SEPARATOR;
#endif

#define IS_DEVICE_PATH(s)          (IS_SEPAR((s)[0]) && IS_SEPAR((s)[1]) && (s)[2] == '.' && IS_SEPAR((s)[3]))
#define IS_SUPER_PREFIX(s)         (IS_SEPAR((s)[0]) && IS_SEPAR((s)[1]) && (s)[2] == '?' && IS_SEPAR((s)[3]))

#define IS_UNC_WITH_SLASH(s) ( \
     ((s)[0] == 'U' || (s)[0] == 'u') \
  && ((s)[1] == 'N' || (s)[1] == 'n') \
  && ((s)[2] == 'C' || (s)[2] == 'c') \
  && IS_SEPAR((s)[3]))

static const unsigned kDrivePrefixSize = 3; /* c:\ */

bool IsSuperPath(const wchar_t *s) throw();
bool IsSuperPath(const wchar_t *s) throw() { return IS_SUPER_PREFIX(s); }
// bool IsSuperUncPath(const wchar_t *s) throw() { return (IS_SUPER_PREFIX(s) && IS_UNC_WITH_SLASH(s + kSuperPathPrefixSize)); }

#if defined(_WIN32) && !defined(UNDER_CE)

#define IS_SUPER_OR_DEVICE_PATH(s) (IS_SEPAR((s)[0]) && IS_SEPAR((s)[1]) && ((s)[2] == '?' || (s)[2] == '.') && IS_SEPAR((s)[3]))
bool IsSuperOrDevicePath(const wchar_t *s) throw() { return IS_SUPER_OR_DEVICE_PATH(s); }
bool IsDevicePath(CFSTR s) throw()
{
  #ifdef UNDER_CE

  s = s;
  return false;
  /*
  // actually we don't know the way to open device file in WinCE.
  unsigned len = MyStringLen(s);
  if (len < 5 || len > 5 || !IsString1PrefixedByString2(s, "DSK"))
    return false;
  if (s[4] != ':')
    return false;
  // for reading use SG_REQ sg; if (DeviceIoControl(dsk, IOCTL_DISK_READ));
  */
  
  #else
  
  if (!IS_DEVICE_PATH(s))
    return false;
  const unsigned len = MyStringLen(s);
  if (len == 6 && s[5] == ':')
    return true;
  if (len < 18 || len > 22 || !IsString1PrefixedByString2(s + kDevicePathPrefixSize, "PhysicalDrive"))
    return false;
  for (unsigned i = 17; i < len; i++)
    if (s[i] < '0' || s[i] > '9')
      return false;
  return true;
  
  #endif
}

bool IsSuperUncPath(CFSTR s) throw() { return (IS_SUPER_PREFIX(s) && IS_UNC_WITH_SLASH(s + kSuperPathPrefixSize)); }
bool IsNetworkPath(CFSTR s) throw()
{
  if (!IS_SEPAR(s[0]) || !IS_SEPAR(s[1]))
    return false;
  if (IsSuperUncPath(s))
    return true;
  const FChar c = s[2];
  return (c != '.' && c != '?');
}

unsigned GetNetworkServerPrefixSize(CFSTR s) throw()
{
  if (!IS_SEPAR(s[0]) || !IS_SEPAR(s[1]))
    return 0;
  unsigned prefixSize = 2;
  if (IsSuperUncPath(s))
    prefixSize = kSuperUncPathPrefixSize;
  else
  {
    const FChar c = s[2];
    if (c == '.' || c == '?')
      return 0;
  }
  const int pos = FindSepar(s + prefixSize);
  if (pos < 0)
    return 0;
  return prefixSize + (unsigned)(pos + 1);
}

bool IsNetworkShareRootPath(CFSTR s) throw()
{
  const unsigned prefixSize = GetNetworkServerPrefixSize(s);
  if (prefixSize == 0)
    return false;
  s += prefixSize;
  const int pos = FindSepar(s);
  if (pos < 0)
    return true;
  return s[(unsigned)pos + 1] == 0;
}

bool IsAltStreamPrefixWithColon(const UString &s) throw()
{
  if (s.IsEmpty())
    return false;
  if (s.Back() != ':')
    return false;
  unsigned pos = 0;
  if (IsSuperPath(s))
    pos = kSuperPathPrefixSize;
  if (s.Len() - pos == 2 && IsDrivePath2(s.Ptr(pos)))
    return false;
  return true;
}

bool If_IsSuperPath_RemoveSuperPrefix(UString &s)
{
  if (!IsSuperPath(s))
    return false;
  unsigned start = 0;
  unsigned count = kSuperPathPrefixSize;
  const wchar_t *s2 = s.Ptr(kSuperPathPrefixSize);
  if (IS_UNC_WITH_SLASH(s2))
  {
    start = 2;
    count = kSuperUncPathPrefixSize - 2;
  }
  s.Delete(start, count);
  return true;
}


#ifndef USE_UNICODE_FSTRING
bool IsDrivePath2(CFSTR s) throw() { return IS_LETTER_CHAR(s[0]) && s[1] == ':'; }
// bool IsDriveName2(CFSTR s) throw() { return IS_LETTER_CHAR(s[0]) && s[1] == ':' && s[2] == 0; }
bool IsDrivePath(CFSTR s) throw() { return IS_LETTER_CHAR(s[0]) && s[1] == ':' && IS_SEPAR(s[2]); }
bool IsSuperPath(CFSTR s) throw() { return IS_SUPER_PREFIX(s); }
bool IsSuperOrDevicePath(CFSTR s) throw() { return IS_SUPER_OR_DEVICE_PATH(s); }
#endif // USE_UNICODE_FSTRING

bool IsDrivePath_SuperAllowed(CFSTR s) throw()
{
  if (IsSuperPath(s))
    s += kSuperPathPrefixSize;
  return IsDrivePath(s);
}

bool IsDriveRootPath_SuperAllowed(CFSTR s) throw()
{
  if (IsSuperPath(s))
    s += kSuperPathPrefixSize;
  return IsDrivePath(s) && s[kDrivePrefixSize] == 0;
}

bool IsAbsolutePath(const wchar_t *s) throw()
{
  return IS_SEPAR(s[0]) || IsDrivePath2(s);
}

int FindAltStreamColon(CFSTR path) throw()
{
  unsigned i = 0;
  if (IsSuperPath(path))
    i = kSuperPathPrefixSize;
  if (IsDrivePath2(path + i))
    i += 2;
  int colonPos = -1;
  for (;; i++)
  {
    const FChar c = path[i];
    if (c == 0)
      return colonPos;
    if (c == ':')
    {
      if (colonPos < 0)
        colonPos = (int)i;
      continue;
    }
    if (IS_SEPAR(c))
      colonPos = -1;
  }
}

#ifndef USE_UNICODE_FSTRING

static unsigned GetRootPrefixSize_Of_NetworkPath(CFSTR s)
{
  // Network path: we look "server\path\" as root prefix
  const int pos = FindSepar(s);
  if (pos < 0)
    return 0;
  const int pos2 = FindSepar(s + (unsigned)pos + 1);
  if (pos2 < 0)
    return 0;
  return (unsigned)pos + (unsigned)pos2 + 2;
}

static unsigned GetRootPrefixSize_Of_SimplePath(CFSTR s)
{
  if (IsDrivePath(s))
    return kDrivePrefixSize;
  if (!IS_SEPAR(s[0]))
    return 0;
  if (s[1] == 0 || !IS_SEPAR(s[1]))
    return 1;
  const unsigned size = GetRootPrefixSize_Of_NetworkPath(s + 2);
  return (size == 0) ? 0 : 2 + size;
}

static unsigned GetRootPrefixSize_Of_SuperPath(CFSTR s)
{
  if (IS_UNC_WITH_SLASH(s + kSuperPathPrefixSize))
  {
    const unsigned size = GetRootPrefixSize_Of_NetworkPath(s + kSuperUncPathPrefixSize);
    return (size == 0) ? 0 : kSuperUncPathPrefixSize + size;
  }
  // we support \\?\c:\ paths and volume GUID paths \\?\Volume{GUID}\"
  const int pos = FindSepar(s + kSuperPathPrefixSize);
  if (pos < 0)
    return 0;
  return kSuperPathPrefixSize + (unsigned)pos + 1;
}

unsigned GetRootPrefixSize(CFSTR s) throw()
{
  if (IS_DEVICE_PATH(s))
    return kDevicePathPrefixSize;
  if (IsSuperPath(s))
    return GetRootPrefixSize_Of_SuperPath(s);
  return GetRootPrefixSize_Of_SimplePath(s);
}

#endif // USE_UNICODE_FSTRING
#endif // _WIN32


static unsigned GetRootPrefixSize_Of_NetworkPath(const wchar_t *s) throw()
{
  // Network path: we look "server\path\" as root prefix
  const int pos = FindSepar(s);
  if (pos < 0)
    return 0;
  const int pos2 = FindSepar(s + (unsigned)pos + 1);
  if (pos2 < 0)
    return 0;
  return (unsigned)(pos + pos2 + 2);
}

static unsigned GetRootPrefixSize_Of_SimplePath(const wchar_t *s) throw()
{
  if (IsDrivePath(s))
    return kDrivePrefixSize;
  if (!IS_SEPAR(s[0]))
    return 0;
  if (s[1] == 0 || !IS_SEPAR(s[1]))
    return 1;
  const unsigned size = GetRootPrefixSize_Of_NetworkPath(s + 2);
  return (size == 0) ? 0 : 2 + size;
}

static unsigned GetRootPrefixSize_Of_SuperPath(const wchar_t *s) throw()
{
  if (IS_UNC_WITH_SLASH(s + kSuperPathPrefixSize))
  {
    const unsigned size = GetRootPrefixSize_Of_NetworkPath(s + kSuperUncPathPrefixSize);
    return (size == 0) ? 0 : kSuperUncPathPrefixSize + size;
  }
  // we support \\?\c:\ paths and volume GUID paths \\?\Volume{GUID}\"
  const int pos = FindSepar(s + kSuperPathPrefixSize);
  if (pos < 0)
    return 0;
  return kSuperPathPrefixSize + (unsigned)(pos + 1);
}

#ifdef _WIN32
unsigned GetRootPrefixSize(const wchar_t *s) throw()
#else
unsigned GetRootPrefixSize_WINDOWS(const wchar_t *s) throw()
#endif
{
  if (IS_DEVICE_PATH(s))
    return kDevicePathPrefixSize;
  if (IsSuperPath(s))
    return GetRootPrefixSize_Of_SuperPath(s);
  return GetRootPrefixSize_Of_SimplePath(s);
}

#ifndef _WIN32

bool IsAbsolutePath(const wchar_t *s) throw() { return IS_SEPAR(s[0]); }

#ifndef USE_UNICODE_FSTRING
unsigned GetRootPrefixSize(CFSTR s) throw();
unsigned GetRootPrefixSize(CFSTR s) throw() { return IS_SEPAR(s[0]) ? 1 : 0; }
#endif
unsigned GetRootPrefixSize(const wchar_t *s) throw() { return IS_SEPAR(s[0]) ? 1 : 0; }

#endif // _WIN32


#ifndef UNDER_CE


#ifdef USE_UNICODE_FSTRING

#define GetCurDir NDir::GetCurrentDir

#else

static bool GetCurDir(UString &path)
{
  path.Empty();
  FString s;
  if (!NDir::GetCurrentDir(s))
    return false;
  path = fs2us(s);
  return true;
}

#endif


static bool ResolveDotsFolders(UString &s)
{
  #ifdef _WIN32
  // s.Replace(L'/', WCHAR_PATH_SEPARATOR);
  #endif
  
  for (unsigned i = 0;;)
  {
    const wchar_t c = s[i];
    if (c == 0)
      return true;
    if (c == '.' && (i == 0 || IS_SEPAR(s[i - 1])))
    {
      const wchar_t c1 = s[i + 1];
      if (c1 == '.')
      {
        const wchar_t c2 = s[i + 2];
        if (IS_SEPAR(c2) || c2 == 0)
        {
          if (i == 0)
            return false;
          int k = (int)i - 2;
          i += 2;
          
          for (;; k--)
          {
            if (k < 0)
              return false;
            if (!IS_SEPAR(s[(unsigned)k]))
              break;
          }

          do
            k--;
          while (k >= 0 && !IS_SEPAR(s[(unsigned)k]));
          
          unsigned num;
          
          if (k >= 0)
          {
            num = i - (unsigned)k;
            i = (unsigned)k;
          }
          else
          {
            num = (c2 == 0 ? i : (i + 1));
            i = 0;
          }
          
          s.Delete(i, num);
          continue;
        }
      }
      else if (IS_SEPAR(c1) || c1 == 0)
      {
        unsigned num = 2;
        if (i != 0)
          i--;
        else if (c1 == 0)
          num = 1;
        s.Delete(i, num);
        continue;
      }
    }

    i++;
  }
}

#endif // UNDER_CE

#define LONG_PATH_DOTS_FOLDERS_PARSING


/*
Windows (at least 64-bit XP) can't resolve "." or ".." in paths that start with SuperPrefix \\?\
To solve that problem we check such path:
   - super path contains        "." or ".." - we use kSuperPathType_UseOnlySuper
   - super path doesn't contain "." or ".." - we use kSuperPathType_UseOnlyMain
*/
#ifdef LONG_PATH_DOTS_FOLDERS_PARSING
#ifndef UNDER_CE
static bool AreThereDotsFolders(CFSTR s)
{
  for (unsigned i = 0;; i++)
  {
    FChar c = s[i];
    if (c == 0)
      return false;
    if (c == '.' && (i == 0 || IS_SEPAR(s[i - 1])))
    {
      FChar c1 = s[i + 1];
      if (c1 == 0 || IS_SEPAR(c1) ||
          (c1 == '.' && (s[i + 2] == 0 || IS_SEPAR(s[i + 2]))))
        return true;
    }
  }
}
#endif
#endif // LONG_PATH_DOTS_FOLDERS_PARSING

#ifdef Z7_LONG_PATH

/*
Most of Windows versions have problems, if some file or dir name
contains '.' or ' ' at the end of name (Bad Path).
To solve that problem, we always use Super Path ("\\?\" prefix and full path)
in such cases. Note that "." and ".." are not bad names.

There are 3 cases:
  1) If the path is already Super Path, we use that path
  2) If the path is not Super Path :
     2.1) Bad Path;  we use only Super Path.
     2.2) Good Path; we use Main Path. If it fails, we use Super Path.

 NeedToUseOriginalPath returns:
    kSuperPathType_UseOnlyMain    : Super already
    kSuperPathType_UseOnlySuper    : not Super, Bad Path
    kSuperPathType_UseMainAndSuper : not Super, Good Path
*/

int GetUseSuperPathType(CFSTR s) throw()
{
  if (IsSuperOrDevicePath(s))
  {
    #ifdef LONG_PATH_DOTS_FOLDERS_PARSING
    if ((s)[2] != '.')
      if (AreThereDotsFolders(s + kSuperPathPrefixSize))
        return kSuperPathType_UseOnlySuper;
    #endif
    return kSuperPathType_UseOnlyMain;
  }

  for (unsigned i = 0;; i++)
  {
    FChar c = s[i];
    if (c == 0)
      return kSuperPathType_UseMainAndSuper;
    if (c == '.' || c == ' ')
    {
      FChar c2 = s[i + 1];
      if (c2 == 0 || IS_SEPAR(c2))
      {
        // if it's "." or "..", it's not bad name.
        if (c == '.')
        {
          if (i == 0 || IS_SEPAR(s[i - 1]))
            continue;
          if (s[i - 1] == '.')
          {
            if (i - 1 == 0 || IS_SEPAR(s[i - 2]))
              continue;
          }
        }
        return kSuperPathType_UseOnlySuper;
      }
    }
  }
}



/*
   returns false in two cases:
     - if GetCurDir was used, and GetCurDir returned error.
     - if we can't resolve ".." name.
   if path is ".", "..", res is empty.
   if it's Super Path already, res is empty.
   for \**** , and if GetCurDir is not drive (c:\), res is empty
   for absolute paths, returns true, res is Super path.
*/

static bool GetSuperPathBase(CFSTR s, UString &res)
{
  res.Empty();
  
  FChar c = s[0];
  if (c == 0)
    return true;
  if (c == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)))
    return true;
  
  if (IsSuperOrDevicePath(s))
  {
    #ifdef LONG_PATH_DOTS_FOLDERS_PARSING
    
    if ((s)[2] == '.')
      return true;

    // we will return true here, so we will try to use these problem paths.

    if (!AreThereDotsFolders(s + kSuperPathPrefixSize))
      return true;
    
    UString temp = fs2us(s);
    const unsigned fixedSize = GetRootPrefixSize_Of_SuperPath(temp);
    if (fixedSize == 0)
      return true;

    UString rem = temp.Ptr(fixedSize);
    if (!ResolveDotsFolders(rem))
      return true;

    temp.DeleteFrom(fixedSize);
    res += temp;
    res += rem;
    
    #endif

    return true;
  }

  if (IS_SEPAR(c))
  {
    if (IS_SEPAR(s[1]))
    {
      UString temp = fs2us(s + 2);
      const unsigned fixedSize = GetRootPrefixSize_Of_NetworkPath(temp);
      // we ignore that error to allow short network paths server\share?
      /*
      if (fixedSize == 0)
        return false;
      */
      UString rem = temp.Ptr(fixedSize);
      if (!ResolveDotsFolders(rem))
        return false;
      res += kSuperUncPrefix;
      temp.DeleteFrom(fixedSize);
      res += temp;
      res += rem;
      return true;
    }
  }
  else
  {
    if (IsDrivePath2(s))
    {
      UString temp = fs2us(s);
      unsigned prefixSize = 2;
      if (IsDrivePath(s))
        prefixSize = kDrivePrefixSize;
      UString rem = temp.Ptr(prefixSize);
      if (!ResolveDotsFolders(rem))
        return true;
      res += kSuperPathPrefix;
      temp.DeleteFrom(prefixSize);
      res += temp;
      res += rem;
      return true;
    }
  }

  UString curDir;
  if (!GetCurDir(curDir))
    return false;
  NormalizeDirPathPrefix(curDir);

  unsigned fixedSizeStart = 0;
  unsigned fixedSize = 0;
  const char *superMarker = NULL;
  if (IsSuperPath(curDir))
  {
    fixedSize = GetRootPrefixSize_Of_SuperPath(curDir);
    if (fixedSize == 0)
      return false;
  }
  else
  {
    if (IsDrivePath(curDir))
    {
      superMarker = kSuperPathPrefix;
      fixedSize = kDrivePrefixSize;
    }
    else
    {
      if (!IsPathSepar(curDir[0]) || !IsPathSepar(curDir[1]))
        return false;
      fixedSizeStart = 2;
      fixedSize = GetRootPrefixSize_Of_NetworkPath(curDir.Ptr(2));
      if (fixedSize == 0)
        return false;
      superMarker = kSuperUncPrefix;
    }
  }
  
  UString temp;
  if (IS_SEPAR(c))
  {
    temp = fs2us(s + 1);
  }
  else
  {
    temp += &curDir[fixedSizeStart + fixedSize];
    temp += fs2us(s);
  }
  if (!ResolveDotsFolders(temp))
    return false;
  if (superMarker)
    res += superMarker;
  res += curDir.Mid(fixedSizeStart, fixedSize);
  res += temp;
  return true;
}


/*
  In that case if GetSuperPathBase doesn't return new path, we don't need
  to use same path that was used as main path
                        
  GetSuperPathBase  superPath.IsEmpty() onlyIfNew
     false            *                *          GetCurDir Error
     true            false             *          use Super path
     true            true             true        don't use any path, we already used mainPath
     true            true             false       use main path as Super Path, we don't try mainMath
                                                  That case is possible now if GetCurDir returns unknown
                                                  type of path (not drive and not network)

  We can change that code if we want to try mainPath, if GetSuperPathBase returns error,
  and we didn't try mainPath still.
  If we want to work that way, we don't need to use GetSuperPathBase return code.
*/

bool GetSuperPath(CFSTR path, UString &superPath, bool onlyIfNew)
{
  if (GetSuperPathBase(path, superPath))
  {
    if (superPath.IsEmpty())
    {
      // actually the only possible when onlyIfNew == true and superPath is empty
      // is case when

      if (onlyIfNew)
        return false;
      superPath = fs2us(path);
    }
    
    NormalizeDirSeparators(superPath);
    return true;
  }
  return false;
}

bool GetSuperPaths(CFSTR s1, CFSTR s2, UString &d1, UString &d2, bool onlyIfNew)
{
  if (!GetSuperPathBase(s1, d1) ||
      !GetSuperPathBase(s2, d2))
    return false;
 
  NormalizeDirSeparators(d1);
  NormalizeDirSeparators(d2);

  if (d1.IsEmpty() && d2.IsEmpty() && onlyIfNew)
    return false;
  if (d1.IsEmpty()) d1 = fs2us(s1);
  if (d2.IsEmpty()) d2 = fs2us(s2);
  return true;
}


/*
// returns true, if we need additional use with New Super path.
bool GetSuperPath(CFSTR path, UString &superPath)
{
  if (GetSuperPathBase(path, superPath))
    return !superPath.IsEmpty();
  return false;
}
*/
#endif // Z7_LONG_PATH

bool GetFullPath(CFSTR dirPrefix, CFSTR s, FString &res)
{
  res = s;

  #ifdef UNDER_CE

  if (!IS_SEPAR(s[0]))
  {
    if (!dirPrefix)
      return false;
    res = dirPrefix;
    res += s;
  }

  #else

  const unsigned prefixSize = GetRootPrefixSize(s);
  if (prefixSize != 0)
#ifdef _WIN32
  if (prefixSize != 1)
#endif
  {
    if (!AreThereDotsFolders(s + prefixSize))
      return true;
    
    UString rem = fs2us(s + prefixSize);
    if (!ResolveDotsFolders(rem))
      return true; // maybe false;
    res.DeleteFrom(prefixSize);
    res += us2fs(rem);
    return true;
  }

  UString curDir;
  if (dirPrefix && prefixSize == 0)
    curDir = fs2us(dirPrefix);  // we use (dirPrefix), only if (s) path is relative
  else
  {
    if (!GetCurDir(curDir))
      return false;
  }
  NormalizeDirPathPrefix(curDir);

  unsigned fixedSize = GetRootPrefixSize(curDir);

  UString temp;
#ifdef _WIN32
  if (prefixSize != 0)
  {
    /* (s) is absolute path, but only (prefixSize == 1) is possible here.
       So for full resolving we need root of current folder and
       relative part of (s). */
    s += prefixSize;
    // (s) is relative part now
    if (fixedSize == 0)
    {
      // (curDir) is not absolute.
      // That case is unexpected, but we support it too.
      curDir.Empty();
      curDir.Add_PathSepar();
      fixedSize = 1;
      // (curDir) now is just Separ character.
      // So final (res) path later also will have Separ prefix.
    }
  }
  else
#endif // _WIN32
  {
    // (s) is relative path
    temp = curDir.Ptr(fixedSize);
    // (temp) is relative_part_of(curDir)
  }
  temp += fs2us(s);
  if (!ResolveDotsFolders(temp))
    return false;
  curDir.DeleteFrom(fixedSize);
  // (curDir) now contains only absolute prefix part
  res = us2fs(curDir);
  res += us2fs(temp);
  
  #endif // UNDER_CE

  return true;
}


bool GetFullPath(CFSTR path, FString &fullPath)
{
  return GetFullPath(NULL, path, fullPath);
}

}}}

/* ================ unit: CPP/Windows/PropVariant.cpp ================ */
// Windows/PropVariant.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NWindows {
namespace NCOM {

BSTR AllocBstrFromAscii(const char *s) throw()
{
  if (!s)
    return NULL;
  UINT len = (UINT)strlen(s);
  BSTR p = ::SysAllocStringLen(NULL, len);
  if (p)
  {
    for (UINT i = 0; i <= len; i++)
      p[i] = (Byte)s[i];
  }
  return p;
}

HRESULT PropVarEm_Alloc_Bstr(PROPVARIANT *p, unsigned numChars) throw()
{
  p->bstrVal = ::SysAllocStringLen(NULL, numChars);
  if (!p->bstrVal)
  {
    p->vt = VT_ERROR;
    p->scode = E_OUTOFMEMORY;
    return E_OUTOFMEMORY;
  }
  p->vt = VT_BSTR;
  return S_OK;
}

HRESULT PropVarEm_Set_Str(PROPVARIANT *p, const char *s) throw()
{
  p->bstrVal = AllocBstrFromAscii(s);
  if (p->bstrVal)
  {
    p->vt = VT_BSTR;
    return S_OK;
  }
  p->vt = VT_ERROR;
  p->scode = E_OUTOFMEMORY;
  return E_OUTOFMEMORY;
}

CPropVariant::CPropVariant(const PROPVARIANT &varSrc)
{
  vt = VT_EMPTY;
  InternalCopy(&varSrc);
}

CPropVariant::CPropVariant(const CPropVariant &varSrc)
{
  vt = VT_EMPTY;
  InternalCopy(&varSrc);
}

CPropVariant::CPropVariant(BSTR bstrSrc)
{
  vt = VT_EMPTY;
  *this = bstrSrc;
}

CPropVariant::CPropVariant(LPCOLESTR lpszSrc)
{
  vt = VT_EMPTY;
  *this = lpszSrc;
}

CPropVariant& CPropVariant::operator=(const CPropVariant &varSrc)
{
  InternalCopy(&varSrc);
  return *this;
}

CPropVariant& CPropVariant::operator=(const PROPVARIANT &varSrc)
{
  InternalCopy(&varSrc);
  return *this;
}

CPropVariant& CPropVariant::operator=(BSTR bstrSrc)
{
  *this = (LPCOLESTR)bstrSrc;
  return *this;
}

static const char * const kMemException = "out of memory";

CPropVariant& CPropVariant::operator=(LPCOLESTR lpszSrc)
{
  InternalClear();
  vt = VT_BSTR;
  wReserved1 = 0;
  bstrVal = ::SysAllocString(lpszSrc);
  if (!bstrVal && lpszSrc)
  {
    throw kMemException;
    // vt = VT_ERROR;
    // scode = E_OUTOFMEMORY;
  }
  return *this;
}

CPropVariant& CPropVariant::operator=(const UString &s)
{
  InternalClear();
  vt = VT_BSTR;
  wReserved1 = 0;
  bstrVal = ::SysAllocStringLen(s, s.Len());
  if (!bstrVal)
    throw kMemException;
  return *this;
}

CPropVariant& CPropVariant::operator=(const UString2 &s)
{
  /*
  if (s.IsEmpty())
    *this = L"";
  else
  */
  {
    InternalClear();
    vt = VT_BSTR;
    wReserved1 = 0;
    bstrVal = ::SysAllocStringLen(s.GetRawPtr(), s.Len());
    if (!bstrVal)
      throw kMemException;
    /* SysAllocStringLen probably appends a null-terminating character for NULL string.
       But it doesn't specified in MSDN.
       But we suppose that it works

    if (!s.GetRawPtr())
    {
      *bstrVal = 0;
    }
    */

    /* MSDN: Windows CE: SysAllocStringLen() : Passing invalid (and under some circumstances NULL)
                         pointers to this function causes  an unexpected termination of the application.
       Is it safe? Maybe we must chamnge the code for that case ? */
  }
  return *this;
}

CPropVariant& CPropVariant::operator=(const char *s)
{
  InternalClear();
  vt = VT_BSTR;
  wReserved1 = 0;
  bstrVal = AllocBstrFromAscii(s);
  if (!bstrVal)
  {
    throw kMemException;
    // vt = VT_ERROR;
    // scode = E_OUTOFMEMORY;
  }
  return *this;
}

CPropVariant& CPropVariant::operator=(bool bSrc) throw()
{
  if (vt != VT_BOOL)
  {
    InternalClear();
    vt = VT_BOOL;
  }
  boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
  return *this;
}

BSTR CPropVariant::AllocBstr(unsigned numChars)
{
  if (vt != VT_EMPTY)
    InternalClear();
  vt = VT_BSTR;
  wReserved1 = 0;
  bstrVal = ::SysAllocStringLen(NULL, numChars);
  if (!bstrVal)
  {
    throw kMemException;
    // vt = VT_ERROR;
    // scode = E_OUTOFMEMORY;
  }
  return bstrVal;
}

#define SET_PROP_id_dest(id, dest) \
  if (vt != id) { InternalClear(); vt = id; } dest = value; wReserved1 = 0;

void CPropVariant::Set_Int32(Int32 value) throw()
{
  SET_PROP_id_dest(VT_I4, lVal)
}

void CPropVariant::Set_Int64(Int64 value) throw()
{
  SET_PROP_id_dest(VT_I8, hVal.QuadPart)
}

#define SET_PROP_FUNC(type, id, dest) \
  CPropVariant& CPropVariant::operator=(type value) throw() \
  { SET_PROP_id_dest(id, dest)  return *this; }

SET_PROP_FUNC(Byte, VT_UI1, bVal)
// SET_PROP_FUNC(Int16, VT_I2, iVal)
// SET_PROP_FUNC(Int32, VT_I4, lVal)
SET_PROP_FUNC(UInt32, VT_UI4, ulVal)
SET_PROP_FUNC(UInt64, VT_UI8, uhVal.QuadPart)
// SET_PROP_FUNC(Int64, VT_I8, hVal.QuadPart)
SET_PROP_FUNC(const FILETIME &, VT_FILETIME, filetime)

#define CASE_SIMPLE_VT_VALUES \
    case VT_EMPTY:    \
    case VT_BOOL:     \
    case VT_FILETIME: \
    case VT_UI8:      \
    case VT_UI4:      \
    case VT_UI2:      \
    case VT_UI1:      \
    case VT_I8:       \
    case VT_I4:       \
    case VT_I2:       \
    case VT_I1:       \
    case VT_UINT:     \
    case VT_INT:      \
    case VT_NULL:     \
    case VT_ERROR:    \
    case VT_R4:       \
    case VT_R8:       \
    case VT_CY:       \
    case VT_DATE:     \


/*
  ::VariantClear() and ::VariantCopy() don't work, if (vt == VT_FILETIME)
  So we handle VT_FILETIME and another simple types directly
  we call system functions for VT_BSTR and for unknown typed
*/
 
CPropVariant::~CPropVariant() throw()
{
  switch ((unsigned)vt)
  {
    CASE_SIMPLE_VT_VALUES
      // vt = VT_EMPTY; // it's optional
      return;
    default: break;
  }
  ::VariantClear((tagVARIANT *)this);
}

HRESULT PropVariant_Clear(PROPVARIANT *prop) throw()
{
  switch ((unsigned)prop->vt)
  {
    CASE_SIMPLE_VT_VALUES
      prop->vt = VT_EMPTY;
      break;
    default:
    {
      const HRESULT res = ::VariantClear((VARIANTARG *)prop);
      if (res != S_OK || prop->vt != VT_EMPTY)
        return res;
      break;
    }
  }
  prop->wReserved1 = 0;
  prop->wReserved2 = 0;
  prop->wReserved3 = 0;
  prop->uhVal.QuadPart = 0;
  return S_OK;
}

HRESULT CPropVariant::Clear() throw()
{
  if (vt == VT_EMPTY)
  {
    wReserved1 = 0;
    return S_OK;
  }
  return PropVariant_Clear(this);
}

HRESULT CPropVariant::Copy(const PROPVARIANT* pSrc) throw()
{
  Clear();
  switch ((unsigned)pSrc->vt)
  {
    CASE_SIMPLE_VT_VALUES
      memmove((PROPVARIANT*)this, pSrc, sizeof(PROPVARIANT));
      return S_OK;
    default: break;
  }
  return ::VariantCopy((tagVARIANT *)this, (tagVARIANT *)const_cast<PROPVARIANT *>(pSrc));
}


HRESULT CPropVariant::Attach(PROPVARIANT *pSrc) throw()
{
  const HRESULT hr = Clear();
  if (FAILED(hr))
    return hr;
  // memcpy((PROPVARIANT *)this, pSrc, sizeof(PROPVARIANT));
  *(PROPVARIANT *)this = *pSrc;
  pSrc->vt = VT_EMPTY;
  pSrc->wReserved1 = 0;
  return S_OK;
}

HRESULT CPropVariant::Detach(PROPVARIANT *pDest) throw()
{
  if (pDest->vt != VT_EMPTY)
  {
    const HRESULT hr = PropVariant_Clear(pDest);
    if (FAILED(hr))
      return hr;
  }
  // memcpy(pDest, this, sizeof(PROPVARIANT));
  *pDest = *(PROPVARIANT *)this;
  vt = VT_EMPTY;
  wReserved1 = 0;
  return S_OK;
}

HRESULT CPropVariant::InternalClear() throw()
{
  if (vt == VT_EMPTY)
  {
    wReserved1 = 0;
    return S_OK;
  }
  const HRESULT hr = Clear();
  if (FAILED(hr))
  {
    vt = VT_ERROR;
    scode = hr;
  }
  return hr;
}

void CPropVariant::InternalCopy(const PROPVARIANT *pSrc)
{
  const HRESULT hr = Copy(pSrc);
  if (FAILED(hr))
  {
    if (hr == E_OUTOFMEMORY)
      throw kMemException;
    vt = VT_ERROR;
    scode = hr;
  }
}


int CPropVariant::Compare(const CPropVariant &a) throw()
{
  if (vt != a.vt)
    return MyCompare(vt, a.vt);
  switch ((unsigned)vt)
  {
    case VT_EMPTY: return 0;
    // case VT_I1: return MyCompare(cVal, a.cVal);
    case VT_UI1: return MyCompare(bVal, a.bVal);
    case VT_I2: return MyCompare(iVal, a.iVal);
    case VT_UI2: return MyCompare(uiVal, a.uiVal);
    case VT_I4: return MyCompare(lVal, a.lVal);
    case VT_UI4: return MyCompare(ulVal, a.ulVal);
    // case VT_UINT: return MyCompare(uintVal, a.uintVal);
    case VT_I8: return MyCompare(hVal.QuadPart, a.hVal.QuadPart);
    case VT_UI8: return MyCompare(uhVal.QuadPart, a.uhVal.QuadPart);
    case VT_BOOL: return -MyCompare(boolVal, a.boolVal);
    case VT_FILETIME:
    {
      const int res = CompareFileTime(&filetime, &a.filetime);
      if (res != 0)
        return res;
      const unsigned v1 = Get_Ns100();
      const unsigned v2 = a.Get_Ns100();
      return MyCompare(v1, v2);
    }
    case VT_BSTR: return 0; // Not implemented
    default: return 0;
  }
}

}}

/* ================ unit: CPP/Windows/PropVariantConv.cpp ================ */
// PropVariantConv.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#define UINT_TO_STR_2(c, val) { s[0] = (c); s[1] = (char)('0' + (val) / 10); s[2] = (char)('0' + (val) % 10); s += 3; }

static const unsigned k_TimeStringBufferSize = 64;

bool g_Timestamp_Show_UTC;
#if 0
bool g_Timestamp_Show_DisableZ;
bool g_Timestamp_Show_TDelimeter;
bool g_Timestamp_Show_ZoneOffset;
#endif

Z7_NO_INLINE
bool ConvertUtcFileTimeToString2(const FILETIME &utc, unsigned ns100, char *s, int level, unsigned flags) throw()
{
  *s = 0;
  FILETIME ft;

#if 0
  Int64 bias64 = 0;
#endif

  const bool show_utc =
      (flags & kTimestampPrintFlags_Force_UTC) ? true :
      (flags & kTimestampPrintFlags_Force_LOCAL) ? false :
      g_Timestamp_Show_UTC;

  if (show_utc)
    ft = utc;
  else
  {
    if (!FileTimeToLocalFileTime(&utc, &ft))
      return false;
#if 0
    if (g_Timestamp_Show_ZoneOffset)
    {
      const UInt64 utc64 = (((UInt64)utc.dwHighDateTime) << 32) + utc.dwLowDateTime;
      const UInt64 loc64 = (((UInt64) ft.dwHighDateTime) << 32) +  ft.dwLowDateTime;
      bias64 = (Int64)utc64 - (Int64)loc64;
    }
#endif
  }

  SYSTEMTIME st;
  if (!BOOLToBool(FileTimeToSystemTime(&ft, &st)))
  {
    // win10 : that function doesn't work, if bit 63 of 64-bit FILETIME is set.
    return false;
  }

  {
    unsigned val = st.wYear;
    if (val >= 10000)
    {
      *s++ = (char)('0' + val / 10000);
      val %= 10000;
    }
    s[3] = (char)('0' + val % 10); val /= 10;
    s[2] = (char)('0' + val % 10); val /= 10;
    s[1] = (char)('0' + val % 10);
    s[0] = (char)('0' + val / 10);
    s += 4;
  }
  UINT_TO_STR_2('-', st.wMonth)
  UINT_TO_STR_2('-', st.wDay)
  
  if (level > kTimestampPrintLevel_DAY)
  {
    const char setChar =
#if 0
      g_Timestamp_Show_TDelimeter ? 'T' : // ISO 8601
#endif
      ' ';
    UINT_TO_STR_2(setChar, st.wHour)
    UINT_TO_STR_2(':', st.wMinute)
    
    if (level >= kTimestampPrintLevel_SEC)
    {
      UINT_TO_STR_2(':', st.wSecond)

      if (level > kTimestampPrintLevel_SEC)
      {
        *s++ = '.';
        /*
        {
          unsigned val = st.wMilliseconds;
          s[2] = (char)('0' + val % 10); val /= 10;
          s[1] = (char)('0' + val % 10);
          s[0] = (char)('0' + val / 10);
          s += 3;
        }
        *s++ = ' ';
        */
        
        {
          unsigned numDigits = 7;
          UInt32 val = (UInt32)((((UInt64)ft.dwHighDateTime << 32) + ft.dwLowDateTime) % 10000000);
          for (unsigned i = numDigits; i != 0;)
          {
            i--;
            s[i] = (char)('0' + val % 10); val /= 10;
          }
          if (numDigits > (unsigned)level)
            numDigits = (unsigned)level;
          s += numDigits;
        }
        if (level >= kTimestampPrintLevel_NTFS + 1)
        {
          *s++ = (char)('0' + (ns100 / 10));
          if (level >= kTimestampPrintLevel_NTFS + 2)
            *s++ = (char)('0' + (ns100 % 10));
        }
      }
    }
  }
  
  if (show_utc)
  {
    if ((flags & kTimestampPrintFlags_DisableZ) == 0
#if 0
      && !g_Timestamp_Show_DisableZ
#endif
      )
      *s++ = 'Z';
  }
#if 0
  else if (g_Timestamp_Show_ZoneOffset)
  {
#if 1
    {
      char c;
      if (bias64 < 0)
      {
        bias64 = -bias64;
        c = '+';
      }
      else
        c = '-';
      UInt32 bias = (UInt32)((UInt64)bias64 / (10000000 * 60));
#else
    TIME_ZONE_INFORMATION zi;
    const DWORD dw = GetTimeZoneInformation(&zi);
    if (dw <= TIME_ZONE_ID_DAYLIGHT) // == 2
    {
      // UTC = LOCAL + Bias
      Int32 bias = zi.Bias;
      char c;
      if (bias < 0)
      {
        bias = -bias;
        c = '+';
      }
      else
        c = '-';
#endif
      const UInt32 hours = (UInt32)bias / 60;
      const UInt32 mins = (UInt32)bias % 60;
      UINT_TO_STR_2(c, hours)
      UINT_TO_STR_2(':', mins)
    }
  }
#endif
  *s = 0;
  return true;
}


bool ConvertUtcFileTimeToString(const FILETIME &utc, char *s, int level) throw()
{
  return ConvertUtcFileTimeToString2(utc, 0, s, level);
}

bool ConvertUtcFileTimeToString2(const FILETIME &ft, unsigned ns100, wchar_t *dest, int level) throw()
{
  char s[k_TimeStringBufferSize];
  const bool res = ConvertUtcFileTimeToString2(ft, ns100, s, level);
  for (unsigned i = 0;; i++)
  {
    const Byte c = (Byte)s[i];
    dest[i] = c;
    if (c == 0)
      break;
  }
  return res;
}

bool ConvertUtcFileTimeToString(const FILETIME &ft, wchar_t *dest, int level) throw()
{
  char s[k_TimeStringBufferSize];
  const bool res = ConvertUtcFileTimeToString(ft, s, level);
  for (unsigned i = 0;; i++)
  {
    const Byte c = (Byte)s[i];
    dest[i] = c;
    if (c == 0)
      break;
  }
  return res;
}


void ConvertPropVariantToShortString(const PROPVARIANT &prop, char *dest) throw()
{
  *dest = 0;
  switch (prop.vt)
  {
    case VT_EMPTY: return;
    case VT_BSTR: dest[0] = '?'; dest[1] = 0; return;
    case VT_UI1: ConvertUInt32ToString(prop.bVal, dest); return;
    case VT_UI2: ConvertUInt32ToString(prop.uiVal, dest); return;
    case VT_UI4: ConvertUInt32ToString(prop.ulVal, dest); return;
    case VT_UI8: ConvertUInt64ToString(prop.uhVal.QuadPart, dest); return;
    case VT_FILETIME:
    {
      // const unsigned prec = prop.wReserved1;
      int level = 0;
      /*
      if (prec == 0)
        level = 7;
      else if (prec > 16 && prec <= 16 + 9)
        level = prec - 16;
      */
      ConvertUtcFileTimeToString(prop.filetime, dest, level);
      return;
    }
    // case VT_I1: return ConvertInt64ToString(prop.cVal, dest); return;
    case VT_I2: ConvertInt64ToString(prop.iVal, dest); return;
    case VT_I4: ConvertInt64ToString(prop.lVal, dest); return;
    case VT_I8: ConvertInt64ToString(prop.hVal.QuadPart, dest); return;
    case VT_BOOL: dest[0] = VARIANT_BOOLToBool(prop.boolVal) ? '+' : '-'; dest[1] = 0; return;
    default: dest[0] = '?'; dest[1] = ':'; ConvertUInt64ToString(prop.vt, dest + 2);
  }
}

void ConvertPropVariantToShortString(const PROPVARIANT &prop, wchar_t *dest) throw()
{
  *dest = 0;
  switch (prop.vt)
  {
    case VT_EMPTY: return;
    case VT_BSTR: dest[0] = '?'; dest[1] = 0; return;
    case VT_UI1: ConvertUInt32ToString(prop.bVal, dest); return;
    case VT_UI2: ConvertUInt32ToString(prop.uiVal, dest); return;
    case VT_UI4: ConvertUInt32ToString(prop.ulVal, dest); return;
    case VT_UI8: ConvertUInt64ToString(prop.uhVal.QuadPart, dest); return;
    case VT_FILETIME:
    {
      // const unsigned prec = prop.wReserved1;
      int level = 0;
      /*
      if (prec == 0)
        level = 7;
      else if (prec > 16 && prec <= 16 + 9)
        level = prec - 16;
      */
      ConvertUtcFileTimeToString(prop.filetime, dest, level);
      return;
    }
    // case VT_I1: return ConvertInt64ToString(prop.cVal, dest); return;
    case VT_I2: ConvertInt64ToString(prop.iVal, dest); return;
    case VT_I4: ConvertInt64ToString(prop.lVal, dest); return;
    case VT_I8: ConvertInt64ToString(prop.hVal.QuadPart, dest); return;
    case VT_BOOL: dest[0] = VARIANT_BOOLToBool(prop.boolVal) ? (wchar_t)'+' : (wchar_t)'-'; dest[1] = 0; return;
    default: dest[0] = '?'; dest[1] = ':'; ConvertUInt32ToString(prop.vt, dest + 2);
  }
}

/* ================ unit: CPP/Windows/PropVariantUtils.cpp ================ */
// PropVariantUtils.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

using namespace NWindows;

static void AddHex(AString &s, UInt32 v)
{
  char sz[16];
  sz[0] = '0';
  sz[1] = 'x';
  ConvertUInt32ToHex(v, sz + 2);
  s += sz;
}


AString TypePairToString(const CUInt32PCharPair *pairs, unsigned num, UInt32 value)
{
  char sz[16];
  const char *p = NULL;
  for (unsigned i = 0; i < num; i++)
  {
    const CUInt32PCharPair &pair = pairs[i];
    if (pair.Value == value)
      p = pair.Name;
  }
  if (!p)
  {
    ConvertUInt32ToString(value, sz);
    p = sz;
  }
  return (AString)p;
}

void PairToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 value, NCOM::CPropVariant &prop)
{
  prop = TypePairToString(pairs, num, value);
}


AString TypeToString(const char * const table[], unsigned num, UInt32 value)
{
  char sz[16];
  const char *p = NULL;
  if (value < num)
    p = table[value];
  if (!p)
  {
    ConvertUInt32ToString(value, sz);
    p = sz;
  }
  return (AString)p;
}

void TypeToProp(const char * const table[], unsigned num, UInt32 value, NWindows::NCOM::CPropVariant &prop)
{
  char sz[16];
  const char *p = NULL;
  if (value < num)
    p = table[value];
  if (!p)
  {
    ConvertUInt32ToString(value, sz);
    p = sz;
  }
  prop = p;
}


AString FlagsToString(const char * const *names, unsigned num, UInt32 flags)
{
  AString s;
  for (unsigned i = 0; i < num; i++)
  {
    UInt32 flag = (UInt32)1 << i;
    if ((flags & flag) != 0)
    {
      const char *name = names[i];
      if (name && name[0] != 0)
      {
        s.Add_OptSpaced(name);
        flags &= ~flag;
      }
    }
  }
  if (flags != 0)
  {
    s.Add_Space_if_NotEmpty();
    AddHex(s, flags);
  }
  return s;
}

AString FlagsToString(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags)
{
  AString s;
  for (unsigned i = 0; i < num; i++)
  {
    const CUInt32PCharPair &p = pairs[i];
    UInt32 flag = (UInt32)1 << (unsigned)p.Value;
    if ((flags & flag) != 0)
    {
      if (p.Name[0] != 0)
        s.Add_OptSpaced(p.Name);
    }
    flags &= ~flag;
  }
  if (flags != 0)
  {
    s.Add_Space_if_NotEmpty();
    AddHex(s, flags);
  }
  return s;
}

void FlagsToProp(const char * const *names, unsigned num, UInt32 flags, NCOM::CPropVariant &prop)
{
  prop = FlagsToString(names, num, flags);
}

void FlagsToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags, NCOM::CPropVariant &prop)
{
  prop = FlagsToString(pairs, num, flags);
}


static AString Flags64ToString(const CUInt32PCharPair *pairs, unsigned num, UInt64 flags)
{
  AString s;
  for (unsigned i = 0; i < num; i++)
  {
    const CUInt32PCharPair &p = pairs[i];
    UInt64 flag = (UInt64)1 << (unsigned)p.Value;
    if ((flags & flag) != 0)
    {
      if (p.Name[0] != 0)
        s.Add_OptSpaced(p.Name);
    }
    flags &= ~flag;
  }
  if (flags != 0)
  {
    {
      char sz[32];
      sz[0] = '0';
      sz[1] = 'x';
      ConvertUInt64ToHex(flags, sz + 2);
      s.Add_OptSpaced(sz);
    }
  }
  return s;
}

void Flags64ToProp(const CUInt32PCharPair *pairs, unsigned num, UInt64 flags, NCOM::CPropVariant &prop)
{
  prop = Flags64ToString(pairs, num, flags);
}

/* ================ unit: CPP/Windows/Synchronization.cpp ================ */
// Windows/Synchronization.cpp

// amalgamation: header emitted in prologue

#ifndef _WIN32

// amalgamation: header emitted in prologue

namespace NWindows {
namespace NSynchronization {

/*
#define INFINITE  0xFFFFFFFF
#define MAXIMUM_WAIT_OBJECTS 64
#define STATUS_ABANDONED_WAIT_0 ((NTSTATUS)0x00000080L)
#define WAIT_ABANDONED   ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0 ((STATUS_ABANDONED_WAIT_0 ) + 0 )
// WINAPI
DWORD WaitForMultipleObjects(DWORD count, const HANDLE *handles, BOOL wait_all, DWORD timeout);
*/

/* clang: we need to place some virtual functions in cpp file to rid off the warning:
   'CBaseHandle_WFMO' has no out-of-line virtual method definitions;
   its vtable will be emitted in every translation unit */
CBaseHandle_WFMO::~CBaseHandle_WFMO()
{
}

bool CBaseEvent_WFMO::IsSignaledAndUpdate()
{
  if (this->_state == false)
    return false;
  if (this->_manual_reset == false)
    this->_state = false;
  return true;
}

bool CSemaphore_WFMO::IsSignaledAndUpdate()
{
  if (this->_count == 0)
    return false;
  this->_count--;
  return true;
}

DWORD WINAPI WaitForMultiObj_Any_Infinite(DWORD count, const CHandle_WFMO *handles)
{
  if (count < 1)
  {
    // abort();
    SetLastError(EINVAL);
    return WAIT_FAILED;
  }

  CSynchro *synchro = handles[0]->_sync;
  synchro->Enter();
  
  // #ifdef DEBUG_SYNCHRO
  for (DWORD i = 1; i < count; i++)
  {
    if (synchro != handles[i]->_sync)
    {
      // abort();
      synchro->Leave();
      SetLastError(EINVAL);
      return WAIT_FAILED;
    }
  }
  // #endif

  for (;;)
  {
    for (DWORD i = 0; i < count; i++)
    {
      if (handles[i]->IsSignaledAndUpdate())
      {
        synchro->Leave();
        return WAIT_OBJECT_0 + i;
      }
    }
    synchro->WaitCond();
  }
}

}}

#endif

/* ================ unit: CPP/Windows/System.cpp ================ */
// Windows/System.cpp

// amalgamation: header emitted in prologue

#ifndef _WIN32
#include <unistd.h>
#include <limits.h>
#if defined(__APPLE__) || defined(__DragonFly__) \
    || defined(BSD) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(__QNXNTO__)
#include <sys/sysctl.h>
#else
#include <sys/sysinfo.h>
#endif
#endif

// amalgamation: header emitted in prologue
// #include "../Common/MyWindows.h"

// #include "../../C/CpuArch.h"

// amalgamation: header emitted in prologue

namespace NWindows {
namespace NSystem {

#ifdef _WIN32

/*
note: returned value in 32-bit version can be limited by value 32.
      while 64-bit version returns full value.
GetMaximumProcessorCount(groupNumber) can return higher value than
GetActiveProcessorCount(groupNumber) in some cases, because CPUs can be added.
*/
// typedef DWORD (WINAPI *Func_GetMaximumProcessorCount)(WORD GroupNumber);
typedef DWORD (WINAPI *Func_GetActiveProcessorCount)(WORD GroupNumber);
typedef WORD (WINAPI *Func_GetActiveProcessorGroupCount)(VOID);
/*
#if 0 && defined(ALL_PROCESSOR_GROUPS)
#define MY_ALL_PROCESSOR_GROUPS   ALL_PROCESSOR_GROUPS
#else
#define MY_ALL_PROCESSOR_GROUPS   0xffff
#endif
*/

Z7_DIAGNOSTIC_IGNORE_CAST_FUNCTION

bool CCpuGroups::Load()
{
  NumThreadsTotal = 0;
  GroupSizes.Clear();
  const HMODULE hmodule = ::GetModuleHandleA("kernel32.dll");
  // Is_Win11_Groups = GetProcAddress(hmodule, "SetThreadSelectedCpuSetMasks") != NULL;
  const
      Func_GetActiveProcessorGroupCount
        fn_GetActiveProcessorGroupCount = Z7_GET_PROC_ADDRESS(
      Func_GetActiveProcessorGroupCount, hmodule,
          "GetActiveProcessorGroupCount");
  const
      Func_GetActiveProcessorCount
        fn_GetActiveProcessorCount = Z7_GET_PROC_ADDRESS(
      Func_GetActiveProcessorCount, hmodule,
          "GetActiveProcessorCount");
  if (!fn_GetActiveProcessorGroupCount ||
      !fn_GetActiveProcessorCount)
    return false;

  const unsigned numGroups = fn_GetActiveProcessorGroupCount();
  if (numGroups == 0)
    return false;
  UInt32 sum = 0;
  for (unsigned i = 0; i < numGroups; i++)
  {
    const UInt32 num = fn_GetActiveProcessorCount((WORD)i);
    /*
    if (num == 0)
    {
      // it means error
      // but is it possible that some group is empty by some reason?
      // GroupSizes.Clear();
      // return false;
    }
    */
    sum += num;
    GroupSizes.Add(num);
  }
  NumThreadsTotal = sum;
  // NumThreadsTotal = fn_GetActiveProcessorCount(MY_ALL_PROCESSOR_GROUPS);
  return true;
}

UInt32 CountAffinity(DWORD_PTR mask)
{
  UInt32 num = 0;
  for (unsigned i = 0; i < sizeof(mask) * 8; i++)
  {
    num += (UInt32)(mask & 1);
    mask >>= 1;
  }
  return num;
}

BOOL CProcessAffinity::Get()
{
  IsGroupMode = false;
  Groups.Load();
  // SetThreadAffinityMask(GetCurrentThread(), 1);
  // SetProcessAffinityMask(GetCurrentProcess(), 1);
  BOOL res = GetProcessAffinityMask(GetCurrentProcess(),
      &processAffinityMask, &systemAffinityMask);
  /* DOCs: On a system with more than 64 processors, if the threads
     of the calling process  are in a single processor group, the
     function sets the variables pointed to by lpProcessAffinityMask
     and lpSystemAffinityMask to the process affinity mask and the
     processor mask of active logical processors for that group.
     If the calling process contains threads in multiple groups,
     the function returns zero for both affinity masks

     note: tested in Win10: GetProcessAffinityMask() doesn't return 0
           in (processAffinityMask) and (systemAffinityMask) masks.
     We need to test it in Win11: how to get mask==0 from GetProcessAffinityMask()?
  */
  if (!res)
  {
    processAffinityMask = 0;
    systemAffinityMask = 0;
  }
  if (Groups.GroupSizes.Size() > 1 && Groups.NumThreadsTotal)
    if (// !res ||
        processAffinityMask == 0 || // to support case described in DOCs and for (!res) case
        processAffinityMask == systemAffinityMask) // for default nonchanged affinity
    {
      // we set IsGroupMode only if processAffinity is default (not changed).
      res = TRUE;
      IsGroupMode = true;
    }
  return res;
}


UInt32 CProcessAffinity::Load_and_GetNumberOfThreads()
{
  if (Get())
  {
    const UInt32 numProcessors = GetNumProcessThreads();
    if (numProcessors)
      return numProcessors;
  }
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  // the number of logical processors in the current group
  return systemInfo.dwNumberOfProcessors;
}

UInt32 GetNumberOfProcessors()
{
  // We need to know how many threads we can use.
  // By default the process is assigned to one group.
  CProcessAffinity pa;
  return pa.Load_and_GetNumberOfThreads();
}

#else


BOOL CProcessAffinity::Get()
{
  numSysThreads = GetNumberOfProcessors();

  /*
  numSysThreads = 8;
  for (unsigned i = 0; i < numSysThreads; i++)
    CpuSet_Set(&cpu_set, i);
  return TRUE;
  */
  
  #ifdef Z7_AFFINITY_SUPPORTED
  
  // numSysThreads = sysconf(_SC_NPROCESSORS_ONLN); // The number of processors currently online
  if (sched_getaffinity(0, sizeof(cpu_set), &cpu_set) != 0)
    return FALSE;
  return TRUE;
  
  #else
  
  // cpu_set = ((CCpuSet)1 << (numSysThreads)) - 1;
  return TRUE;
  // errno = ENOSYS;
  // return FALSE;
  
  #endif
}

UInt32 GetNumberOfProcessors()
{
  #ifndef Z7_ST
  long n = sysconf(_SC_NPROCESSORS_CONF);  // The number of processors configured
  if (n < 1)
    n = 1;
  return (UInt32)n;
  #else
  return 1;
  #endif
}

#endif


#ifdef _WIN32

#ifndef UNDER_CE

#if !defined(_WIN64) && \
  (defined(__MINGW32_VERSION) || defined(Z7_OLD_WIN_SDK))

typedef struct {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  DWORDLONG ullTotalPhys;
  DWORDLONG ullAvailPhys;
  DWORDLONG ullTotalPageFile;
  DWORDLONG ullAvailPageFile;
  DWORDLONG ullTotalVirtual;
  DWORDLONG ullAvailVirtual;
  DWORDLONG ullAvailExtendedVirtual;
} MY_MEMORYSTATUSEX, *MY_LPMEMORYSTATUSEX;

#else

#define MY_MEMORYSTATUSEX MEMORYSTATUSEX
#define MY_LPMEMORYSTATUSEX LPMEMORYSTATUSEX

#endif

typedef BOOL (WINAPI *Func_GlobalMemoryStatusEx)(MY_LPMEMORYSTATUSEX lpBuffer);

#endif // !UNDER_CE

  
bool GetRamSize(size_t &size)
{
  size = (size_t)sizeof(size_t) << 29;

  #ifndef UNDER_CE
    MY_MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
  #endif
  
  #ifdef _WIN64
    
    if (!::GlobalMemoryStatusEx(&stat))
      return false;
    size = MyMin(stat.ullTotalVirtual, stat.ullTotalPhys);
    return true;

  #else
    
    #ifndef UNDER_CE
      const
      Func_GlobalMemoryStatusEx fn = Z7_GET_PROC_ADDRESS(
      Func_GlobalMemoryStatusEx, ::GetModuleHandleA("kernel32.dll"),
          "GlobalMemoryStatusEx");
      if (fn && fn(&stat))
      {
        // (MY_MEMORYSTATUSEX::ullTotalVirtual) < 4 GiB in 32-bit mode
        size_t size2 = (size_t)0 - 1;
        if (size2 > stat.ullTotalPhys)
            size2 = (size_t)stat.ullTotalPhys;
        if (size2 > stat.ullTotalVirtual)
            size2 = (size_t)stat.ullTotalVirtual;
        size = size2;
        return true;
      }
    #endif
  
    // On computers with more than 4 GB of memory:
    //   new docs  : GlobalMemoryStatus can report (-1) value to indicate an overflow.
    //   some old docs : GlobalMemoryStatus can report (modulo 4 GiB) value.
    //                   (for example, if 5 GB total memory, it could report 1 GB).
    // We don't want to get (modulo 4 GiB) value.
    // So we use GlobalMemoryStatusEx() instead.
    {
      MEMORYSTATUS stat2;
      stat2.dwLength = sizeof(stat2);
      ::GlobalMemoryStatus(&stat2);
      size = MyMin(stat2.dwTotalVirtual, stat2.dwTotalPhys);
      return true;
    }
  #endif
}
  
#else

// POSIX
// #include <stdio.h>

bool GetRamSize(size_t &size)
{
  UInt64 size64;
  size = (size_t)sizeof(size_t) << 29;
  size64 = size;

#if defined(__APPLE__) || defined(__DragonFly__) \
    || defined(BSD) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(__QNXNTO__)

    uint64_t val = 0;
    int mib[2];
    mib[0] = CTL_HW;

    #ifdef HW_MEMSIZE
      mib[1] = HW_MEMSIZE;
      // printf("\n sysctl HW_MEMSIZE");
    #elif defined(HW_PHYSMEM64)
      mib[1] = HW_PHYSMEM64;
      // printf("\n sysctl HW_PHYSMEM64");
    #else
      mib[1] = HW_PHYSMEM;
      // printf("\n sysctl HW_PHYSMEM");
    #endif

    size_t size_sys = sizeof(val);
    int res = sysctl(mib, 2, &val, &size_sys, NULL, 0);
    // printf("\n sysctl res=%d val=%llx size_sys = %d, %d\n", res, (long long int)val, (int)size_sys, errno);
    // we use strict check (size_sys == sizeof(val)) for returned value
    // because big-endian encoding is possible:
    if (res == 0 && size_sys == sizeof(val) && val)
      size64 = val;
    else
    {
      uint32_t val32 = 0;
      size_sys = sizeof(val32);
      res = sysctl(mib, 2, &val32, &size_sys, NULL, 0);
      // printf("\n sysctl res=%d val=%llx size_sys = %d, %d\n", res, (long long int)val32, (int)size_sys, errno);
      if (res == 0 && size_sys == sizeof(val32) && val32)
        size64 = val32;
    }

  #elif defined(_AIX)
    #if defined(_SC_AIX_REALMEM) // AIX
      size64 = (UInt64)sysconf(_SC_AIX_REALMEM) * 1024;
    #endif
  #elif 0 || defined(__sun)
    #if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    // FreeBSD, Linux, OpenBSD, and Solaris.
    {
      const long phys_pages = sysconf(_SC_PHYS_PAGES);
      const long page_size = sysconf(_SC_PAGESIZE);
      // #pragma message("GetRamSize : sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE)")
      // printf("\n_SC_PHYS_PAGES (hex) = %lx", (unsigned long)phys_pages);
      // printf("\n_SC_PAGESIZE = %lu\n", (unsigned long)page_size);
      if (phys_pages != -1 && page_size != -1)
        size64 = (UInt64)(Int64)phys_pages * (UInt64)(Int64)page_size;
    }
    #endif
  #elif defined(__gnu_hurd__)
  // fixme
  #elif defined(__FreeBSD_kernel__) && defined(__GLIBC__)
  // GNU/kFreeBSD Debian
  // fixme
  #else

  struct sysinfo info;
  if (::sysinfo(&info) != 0)
    return false;
  size64 = (UInt64)info.mem_unit * info.totalram;
  /*
  printf("\n mem_unit  = %lld", (UInt64)info.mem_unit);
  printf("\n totalram  = %lld", (UInt64)info.totalram);
  printf("\n freeram   = %lld", (UInt64)info.freeram);
  */

  #endif

  size = (size_t)1 << (sizeof(size_t) * 8 - 1);
  if (size > size64)
      size = (size_t)size64;
  return true;
}

#endif


unsigned long Get_File_OPEN_MAX()
{
 #ifdef _WIN32
  return (1 << 24) - (1 << 16); // ~16M handles
 #else
  // some linux versions have default open file limit for user process of 1024 files.
  long n = sysconf(_SC_OPEN_MAX);
  // n = -1; // for debug
  // n = 9; // for debug
  if (n < 1)
  {
    // n = OPEN_MAX;  // ???
    // n = FOPEN_MAX; // = 16 : <stdio.h>
   #ifdef _POSIX_OPEN_MAX
    n = _POSIX_OPEN_MAX; // = 20 : <limits.h>
   #else
    n = 30; // our limit
   #endif
  }
  return (unsigned long)n;
 #endif
}

unsigned Get_File_OPEN_MAX_Reduced_for_3_tasks()
{
  unsigned long numFiles_OPEN_MAX = NSystem::Get_File_OPEN_MAX();
  const unsigned delta = 10; // the reserve for another internal needs of process
  if (numFiles_OPEN_MAX > delta)
    numFiles_OPEN_MAX -= delta;
  else
    numFiles_OPEN_MAX = 1;
  numFiles_OPEN_MAX /= 3; // we suppose that we have up to 3 tasks in total for multiple file processing
  numFiles_OPEN_MAX = MyMax(numFiles_OPEN_MAX, (unsigned long)1);
  unsigned n = (unsigned)(int)-1;
  if (n > numFiles_OPEN_MAX)
    n = (unsigned)numFiles_OPEN_MAX;
  return n;
}

}}

/* ================ unit: CPP/Windows/TimeUtils.cpp ================ */
// Windows/TimeUtils.cpp

// amalgamation: header emitted in prologue

#ifndef _WIN32
#include <sys/time.h>
#include <time.h>
#endif

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NWindows {
namespace NTime {

static const UInt32 kNumTimeQuantumsInSecond = 10000000;
static const unsigned kFileTimeStartYear = 1601;
#if !defined(_WIN32) || defined(UNDER_CE)
static const unsigned kDosTimeStartYear = 1980;
#endif
static const unsigned kUnixTimeStartYear = 1970;
static const UInt64 kUnixTimeOffset =
    (UInt64)60 * 60 * 24 * (89 + 365 * (UInt32)(kUnixTimeStartYear - kFileTimeStartYear));
static const UInt64 kNumSecondsInFileTime = (UInt64)(Int64)-1 / kNumTimeQuantumsInSecond;

bool DosTime_To_FileTime(UInt32 dosTime, FILETIME &ft) throw()
{
  #if defined(_WIN32) && !defined(UNDER_CE)
  return BOOLToBool(::DosDateTimeToFileTime((UInt16)(dosTime >> 16), (UInt16)(dosTime & 0xFFFF), &ft));
  #else
  ft.dwLowDateTime = 0;
  ft.dwHighDateTime = 0;
  UInt64 res;
  if (!GetSecondsSince1601(
      kDosTimeStartYear + (unsigned)(dosTime >> 25),
      (unsigned)((dosTime >> 21) & 0xF),
      (unsigned)((dosTime >> 16) & 0x1F),
      (unsigned)((dosTime >> 11) & 0x1F),
      (unsigned)((dosTime >>  5) & 0x3F),
      (unsigned)((dosTime & 0x1F)) * 2,
      res))
    return false;
  res *= kNumTimeQuantumsInSecond;
  ft.dwLowDateTime = (UInt32)res;
  ft.dwHighDateTime = (UInt32)(res >> 32);
  return true;
  #endif
}

static const UInt32 kHighDosTime = 0xFF9FBF7D;
static const UInt32 kLowDosTime = 0x210000;

bool FileTime_To_DosTime(const FILETIME &ft, UInt32 &dosTime) throw()
{
  #if defined(_WIN32) && !defined(UNDER_CE)

  WORD datePart, timePart;
  if (!::FileTimeToDosDateTime(&ft, &datePart, &timePart))
  {
    dosTime = (ft.dwHighDateTime >= 0x01C00000) ? kHighDosTime : kLowDosTime;
    return false;
  }
  dosTime = (((UInt32)datePart) << 16) + timePart;

  #else

#define PERIOD_4 (4 * 365 + 1)
#define PERIOD_100 (PERIOD_4 * 25 - 1)
#define PERIOD_400 (PERIOD_100 * 4 + 1)

  unsigned year, mon, day, hour, min, sec;
  UInt64 v64 = ft.dwLowDateTime | ((UInt64)ft.dwHighDateTime << 32);
  Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  unsigned temp;
  UInt32 v;
  v64 += (kNumTimeQuantumsInSecond * 2 - 1);
  v64 /= kNumTimeQuantumsInSecond;
  sec = (unsigned)(v64 % 60);
  v64 /= 60;
  min = (unsigned)(v64 % 60);
  v64 /= 60;
  hour = (unsigned)(v64 % 24);
  v64 /= 24;

  v = (UInt32)v64;

  year = kFileTimeStartYear + (unsigned)(v / PERIOD_400 * 400);
  v %= PERIOD_400;

  temp = (unsigned)(v / PERIOD_100);
  if (temp == 4)
    temp = 3;
  year += temp * 100;
  v -= temp * PERIOD_100;

  temp = v / PERIOD_4;
  if (temp == 25)
    temp = 24;
  year += temp * 4;
  v -= temp * PERIOD_4;

  temp = v / 365;
  if (temp == 4)
    temp = 3;
  year += temp;
  v -= temp * 365;

  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
  for (mon = 1; mon <= 12; mon++)
  {
    unsigned s = ms[mon - 1];
    if (v < s)
      break;
    v -= s;
  }
  day = (unsigned)v + 1;

  dosTime = kLowDosTime;
  if (year < kDosTimeStartYear)
    return false;
  year -= kDosTimeStartYear;
  dosTime = kHighDosTime;
  if (year >= 128)
    return false;
  dosTime =
      ((UInt32)year << 25)
    | ((UInt32)mon  << 21)
    | ((UInt32)day  << 16)
    | ((UInt32)hour << 11)
    | ((UInt32)min  << 5)
    | ((UInt32)sec  >> 1);
  #endif
  return true;
}


bool UtcFileTime_To_LocalDosTime(const FILETIME &utc, UInt32 &dosTime) throw()
{
  FILETIME loc = { 0, 0 };
  const UInt64 u1 = FILETIME_To_UInt64(utc);
  const UInt64 kDelta = ((UInt64)1 << 41); // it's larger than quantums in 1 sec.
  if (u1 >= kDelta)
  {
    if (!FileTimeToLocalFileTime(&utc, &loc))
      loc = utc;
    else
    {
      const UInt64 u2 = FILETIME_To_UInt64(loc);
      const UInt64 delta = u1 < u2 ? (u2 - u1) : (u1 - u2);
      if (delta > kDelta) // if FileTimeToLocalFileTime() overflow, we use UTC time
        loc = utc;
    }
  }
  return FileTime_To_DosTime(loc, dosTime);
}

UInt64 UnixTime_To_FileTime64(UInt32 unixTime) throw()
{
  return (kUnixTimeOffset + (UInt64)unixTime) * kNumTimeQuantumsInSecond;
}

void UnixTime_To_FileTime(UInt32 unixTime, FILETIME &ft) throw()
{
  const UInt64 v = UnixTime_To_FileTime64(unixTime);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}

UInt64 UnixTime64_To_FileTime64(Int64 unixTime) throw()
{
  return (UInt64)((Int64)kUnixTimeOffset + unixTime) * kNumTimeQuantumsInSecond;
}


bool UnixTime64_To_FileTime64(Int64 unixTime, UInt64 &fileTime) throw()
{
  if (unixTime > (Int64)(kNumSecondsInFileTime - kUnixTimeOffset))
  {
    fileTime = (UInt64)(Int64)-1;
    return false;
  }
  if (unixTime < -(Int64)kUnixTimeOffset)
  {
    fileTime = 0;
    return false;
  }
  fileTime = UnixTime64_To_FileTime64(unixTime);
  return true;
}


bool UnixTime64_To_FileTime(Int64 unixTime, FILETIME &ft) throw()
{
  UInt64 v;
  const bool res = UnixTime64_To_FileTime64(unixTime, v);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
  return res;
}


Int64 FileTime_To_UnixTime64(const FILETIME &ft) throw()
{
  const UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
  return (Int64)(winTime / kNumTimeQuantumsInSecond) - (Int64)kUnixTimeOffset;
}

Int64 FileTime_To_UnixTime64_and_Quantums(const FILETIME &ft, UInt32 &quantums) throw()
{
  const UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
  quantums = (UInt32)(winTime % kNumTimeQuantumsInSecond);
  return (Int64)(winTime / kNumTimeQuantumsInSecond) - (Int64)kUnixTimeOffset;
}

bool FileTime_To_UnixTime(const FILETIME &ft, UInt32 &unixTime) throw()
{
  UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
  winTime /= kNumTimeQuantumsInSecond;
  if (winTime < kUnixTimeOffset)
  {
    unixTime = 0;
    return false;
  }
  winTime -= kUnixTimeOffset;
  if (winTime > (UInt32)0xFFFFFFFF)
  {
    unixTime = (UInt32)0xFFFFFFFF;
    return false;
  }
  unixTime = (UInt32)winTime;
  return true;
}

bool GetSecondsSince1601(unsigned year, unsigned month, unsigned day,
  unsigned hour, unsigned min, unsigned sec, UInt64 &resSeconds) throw()
{
  resSeconds = 0;
  if (year < kFileTimeStartYear || year >= 10000 || month < 1 || month > 12 ||
      day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
    return false;
  const unsigned numYears = year - kFileTimeStartYear;
  UInt32 numDays = (UInt32)((UInt32)numYears * 365 + numYears / 4 - numYears / 100 + numYears / 400);
  Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
  month--;
  for (unsigned i = 0; i < month; i++)
    numDays += ms[i];
  numDays += (UInt32)(day - 1);
  resSeconds = ((UInt64)(numDays * 24 + hour) * 60 + min) * 60 + sec;
  return true;
}


/* docs: TIME_UTC is not defined on many platforms:
      glibc 2.15, macOS 10.13
      FreeBSD 11.0, NetBSD 7.1, OpenBSD 6.0,
      Minix 3.1.8, AIX 7.1, HP-UX 11.31, IRIX 6.5, Solaris 11.3,
      Cygwin 2.9, mingw, MSVC 14, Android 9.0.
  Android NDK defines TIME_UTC but doesn't have the timespec_get().
*/
#if defined(TIME_UTC) && !defined(__ANDROID__)
#define ZIP7_USE_timespec_get
// #pragma message("ZIP7_USE_timespec_get")
#elif defined(CLOCK_REALTIME)
#define ZIP7_USE_clock_gettime
// #pragma message("ZIP7_USE_clock_gettime")
#endif

void GetCurUtc_FiTime(CFiTime &ft) throw()
{
 #ifdef _WIN32

  // Both variants provide same low resolution on WinXP: about 15 ms.
  // But GetSystemTimeAsFileTime is much faster.
  #ifdef UNDER_CE
  SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &ft);
  #else
  GetSystemTimeAsFileTime(&ft);
  #endif

 #else
  
  FiTime_Clear(ft);
#ifdef ZIP7_USE_timespec_get
  timespec_get(&ft, TIME_UTC);
#elif defined ZIP7_USE_clock_gettime

#if defined(_AIX)
  {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ft.tv_sec = ts.tv_sec;
    ft.tv_nsec = ts.tv_nsec;
  }
#else
  clock_gettime(CLOCK_REALTIME, &ft);
#endif

#else
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0)
  {
    ft.tv_sec = now.tv_sec;
    // timeval::tv_usec   can be 64-bit signed in some cases
    // timespec::tv_nsec  can be 32-bit signed in some cases
    ft.tv_nsec =
      (Int32) // to eliminate compiler conversion error
      (now.tv_usec * 1000);
  }
#endif

 #endif
}

#ifndef _WIN32
void GetCurUtcFileTime(FILETIME &ft) throw()
{
  UInt64 v = 0;
#if defined(ZIP7_USE_timespec_get) || \
    defined(ZIP7_USE_clock_gettime)
  timespec ts;
#if defined(ZIP7_USE_timespec_get)
  if (timespec_get(&ts, TIME_UTC))
#else
  if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
#endif
  {
    v = ((UInt64)ts.tv_sec + kUnixTimeOffset) *
      kNumTimeQuantumsInSecond + (UInt64)ts.tv_nsec / 100;
  }
#else
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0)
  {
    v = ((UInt64)now.tv_sec + kUnixTimeOffset) *
      kNumTimeQuantumsInSecond + (UInt64)now.tv_usec * 10;
  }
#endif
  ft.dwLowDateTime  = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}
#endif


}}


#ifdef _WIN32

/*
void FiTime_Normalize_With_Prec(CFiTime &ft, unsigned prec)
{
  if (prec == k_PropVar_TimePrec_0
      || prec == k_PropVar_TimePrec_HighPrec
      || prec >= k_PropVar_TimePrec_100ns)
    return;
  UInt64 v = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;

  int numDigits = (int)prec - (int)k_PropVar_TimePrec_Base;
  UInt32 d;
  if (prec == k_PropVar_TimePrec_DOS)
  {
    // we round up as windows DosDateTimeToFileTime()
    v += NWindows::NTime::kNumTimeQuantumsInSecond * 2 - 1;
    d = NWindows::NTime::kNumTimeQuantumsInSecond * 2;
  }
  else
  {
    if (prec == k_PropVar_TimePrec_Unix)
      numDigits = 0;
    else if (numDigits < 0)
      return;
    d = 1;
    for (unsigned k = numDigits; k < 7; k++)
      d *= 10;
  }
  v /= d;
  v *= d;
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}
*/

#else

/*
void FiTime_Normalize_With_Prec(CFiTime &ft, unsigned prec)
{
  if (prec >= k_PropVar_TimePrec_1ns
      || prec == k_PropVar_TimePrec_HighPrec)
    return;

  int numDigits = (int)prec - (int)k_PropVar_TimePrec_Base;
  UInt32 d;
  if (prec == k_PropVar_TimePrec_Unix ||
      prec == (int)k_PropVar_TimePrec_Base)
  {
    ft.tv_nsec = 0;
    return;
  }
  if (prec == k_PropVar_TimePrec_DOS)
  {
    // we round up as windows DosDateTimeToFileTime()
    const unsigned sec1 = (ft.tv_sec & 1);
    if (ft.tv_nsec == 0 && sec1 == 0)
      return;
    ft.tv_nsec = 0;
    ft.tv_sec += 2 - sec1;
    return;
  }
  {
    if (prec == k_PropVar_TimePrec_0
        || numDigits < 0)
      numDigits = 7;
    d = 1;
    for (unsigned k = numDigits; k < 9; k++)
      d *= 10;
    ft.tv_nsec /= d;
    ft.tv_nsec *= d;
  }
}
*/

int Compare_FiTime(const CFiTime *a1, const CFiTime *a2)
{
  if (a1->tv_sec < a2->tv_sec) return -1;
  if (a1->tv_sec > a2->tv_sec) return 1;
  if (a1->tv_nsec < a2->tv_nsec) return -1;
  if (a1->tv_nsec > a2->tv_nsec) return 1;
  return 0;
}

bool FILETIME_To_timespec(const FILETIME &ft, CFiTime &ts)
{
  UInt32 quantums;
  const Int64 sec = NWindows::NTime::FileTime_To_UnixTime64_and_Quantums(ft, quantums);
  // time_t is long
  const time_t sec2 = (time_t)sec;
  if (sec2 == sec)
  {
    ts.tv_sec = sec2;
    ts.tv_nsec = (Int32)(quantums * 100);
    return true;
  }
  return false;
}

void FiTime_To_FILETIME_ns100(const CFiTime &ts, FILETIME &ft, unsigned &ns100)
{
  const UInt64 v = NWindows::NTime::UnixTime64_To_FileTime64(ts.tv_sec) + ((UInt64)ts.tv_nsec / 100);
  ns100 = (unsigned)((UInt64)ts.tv_nsec % 100);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}

void FiTime_To_FILETIME(const CFiTime &ts, FILETIME &ft)
{
  const UInt64 v = NWindows::NTime::UnixTime64_To_FileTime64(ts.tv_sec) + ((UInt64)ts.tv_nsec / 100);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}

#endif

/* ================ unit: CPP/Common/MyWindows.cpp ================ */
// MyWindows.cpp

// amalgamation: header emitted in prologue

#ifndef _WIN32

#include <stdlib.h>
#include <time.h>
#ifdef __GNUC__
#include <sys/time.h>
#endif

// amalgamation: header emitted in prologue

static inline void *AllocateForBSTR(size_t cb) { return ::malloc(cb); }
static inline void FreeForBSTR(void *pv) { ::free(pv);}

/* Win32 uses DWORD (32-bit) type to store size of string before (OLECHAR *) string.
  We must select CBstrSizeType for another systems (not Win32):

    if (CBstrSizeType is UINT32),
          then we support only strings smaller than 4 GB.
          Win32 version always has that limitation.
  
    if (CBstrSizeType is UINT),
          (UINT can be 16/32/64-bit)
          We can support strings larger than 4 GB (if UINT is 64-bit),
          but sizeof(UINT) can be different in parts compiled by
          different compilers/settings,
          and we can't send such BSTR strings between such parts.
*/

typedef UINT32 CBstrSizeType;
// typedef UINT CBstrSizeType;

#define k_BstrSize_Max 0xFFFFFFFF
// #define k_BstrSize_Max UINT_MAX
// #define k_BstrSize_Max ((UINT)(INT)-1)

BSTR SysAllocStringByteLen(LPCSTR s, UINT len)
{
  /* Original SysAllocStringByteLen in Win32 maybe fills only unaligned null OLECHAR at the end.
     We provide also aligned null OLECHAR at the end. */

  if (len >= (k_BstrSize_Max - (UINT)sizeof(OLECHAR) - (UINT)sizeof(OLECHAR) - (UINT)sizeof(CBstrSizeType)))
    return NULL;

  UINT size = (len + (UINT)sizeof(OLECHAR) + (UINT)sizeof(OLECHAR) - 1) & ~((UINT)sizeof(OLECHAR) - 1);
  void *p = AllocateForBSTR(size + (UINT)sizeof(CBstrSizeType));
  if (!p)
    return NULL;
  *(CBstrSizeType *)p = (CBstrSizeType)len;
  BSTR bstr = (BSTR)((CBstrSizeType *)p + 1);
  if (s)
    memcpy(bstr, s, len);
  for (; len < size; len++)
    ((Byte *)bstr)[len] = 0;
  return bstr;
}

BSTR SysAllocStringLen(const OLECHAR *s, UINT len)
{
  if (len >= (k_BstrSize_Max - (UINT)sizeof(OLECHAR) - (UINT)sizeof(CBstrSizeType)) / (UINT)sizeof(OLECHAR))
    return NULL;

  UINT size = len * (UINT)sizeof(OLECHAR);
  void *p = AllocateForBSTR(size + (UINT)sizeof(CBstrSizeType) + (UINT)sizeof(OLECHAR));
  if (!p)
    return NULL;
  *(CBstrSizeType *)p = (CBstrSizeType)size;
  BSTR bstr = (BSTR)((CBstrSizeType *)p + 1);
  if (s)
    memcpy(bstr, s, size);
  bstr[len] = 0;
  return bstr;
}

BSTR SysAllocString(const OLECHAR *s)
{
  if (!s)
    return NULL;
  const OLECHAR *s2 = s;
  while (*s2 != 0)
    s2++;
  return SysAllocStringLen(s, (UINT)(s2 - s));
}

void SysFreeString(BSTR bstr)
{
  if (bstr)
    FreeForBSTR((CBstrSizeType *)(void *)bstr - 1);
}

UINT SysStringByteLen(BSTR bstr)
{
  if (!bstr)
    return 0;
  return *((CBstrSizeType *)(void *)bstr - 1);
}

UINT SysStringLen(BSTR bstr)
{
  if (!bstr)
    return 0;
  return *((CBstrSizeType *)(void *)bstr - 1) / (UINT)sizeof(OLECHAR);
}


HRESULT VariantClear(VARIANTARG *prop)
{
  if (prop->vt == VT_BSTR)
    SysFreeString(prop->bstrVal);
  prop->vt = VT_EMPTY;
  return S_OK;
}

HRESULT VariantCopy(VARIANTARG *dest, const VARIANTARG *src)
{
  HRESULT res = ::VariantClear(dest);
  if (res != S_OK)
    return res;
  if (src->vt == VT_BSTR)
  {
    dest->bstrVal = SysAllocStringByteLen((LPCSTR)src->bstrVal,
        SysStringByteLen(src->bstrVal));
    if (!dest->bstrVal)
      return E_OUTOFMEMORY;
    dest->vt = VT_BSTR;
  }
  else
    *dest = *src;
  return S_OK;
}

LONG CompareFileTime(const FILETIME* ft1, const FILETIME* ft2)
{
  if (ft1->dwHighDateTime < ft2->dwHighDateTime) return -1;
  if (ft1->dwHighDateTime > ft2->dwHighDateTime) return 1;
  if (ft1->dwLowDateTime < ft2->dwLowDateTime) return -1;
  if (ft1->dwLowDateTime > ft2->dwLowDateTime) return 1;
  return 0;
}

DWORD GetLastError()
{
  return (DWORD)errno;
}

void SetLastError(DWORD dw)
{
  errno = (int)dw;
}


static LONG TIME_GetBias()
{
  time_t utc = time(NULL);
  struct tm *ptm = localtime(&utc);
  int localdaylight = ptm->tm_isdst; /* daylight for local timezone */
  ptm = gmtime(&utc);
  ptm->tm_isdst = localdaylight; /* use local daylight, not that of Greenwich */
  LONG bias = (int)(mktime(ptm)-utc);
  return bias;
}

#define TICKS_PER_SEC 10000000
/*
#define SECS_PER_DAY (24 * 60 * 60)
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (UInt64)SECS_PER_DAY)
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKS_PER_SEC)
*/

#define GET_TIME_64(pft) ((pft)->dwLowDateTime | ((UInt64)(pft)->dwHighDateTime << 32))

#define SET_FILETIME(ft, v64) \
   (ft)->dwLowDateTime = (DWORD)v64; \
   (ft)->dwHighDateTime = (DWORD)(v64 >> 32);


BOOL WINAPI FileTimeToLocalFileTime(const FILETIME *fileTime, FILETIME *localFileTime)
{
  UInt64 v = GET_TIME_64(fileTime);
  v = (UInt64)((Int64)v - (Int64)TIME_GetBias() * TICKS_PER_SEC);
  SET_FILETIME(localFileTime, v)
  return TRUE;
}

BOOL WINAPI LocalFileTimeToFileTime(const FILETIME *localFileTime, FILETIME *fileTime)
{
  UInt64 v = GET_TIME_64(localFileTime);
  v = (UInt64)((Int64)v + (Int64)TIME_GetBias() * TICKS_PER_SEC);
  SET_FILETIME(fileTime, v)
  return TRUE;
}

/*
VOID WINAPI GetSystemTimeAsFileTime(FILETIME *ft)
{
  UInt64 t = 0;
  timeval tv;
  if (gettimeofday(&tv, NULL) == 0)
  {
    t = tv.tv_sec * (UInt64)TICKS_PER_SEC + TICKS_1601_TO_1970;
    t += tv.tv_usec * 10;
  }
  SET_FILETIME(ft, t)
}
*/

DWORD WINAPI GetTickCount(VOID)
{
  #ifndef _WIN32
  // gettimeofday() doesn't work in some MINGWs by unknown reason
  timeval tv;
  if (gettimeofday(&tv, NULL) == 0)
  {
    // tv_sec and tv_usec are (long)
    return (DWORD)((UInt64)(Int64)tv.tv_sec * (UInt64)1000 + (UInt64)(Int64)tv.tv_usec / 1000);
  }
  #endif
  return (DWORD)time(NULL) * 1000;
}


// amalgamation: CPP/Windows/TimeUtils.cpp above defines these three with the
// same token sequence; undef so a future upstream divergence is a hard error
// rather than a silent last-one-wins.
#undef PERIOD_4
#undef PERIOD_100
#undef PERIOD_400
#define PERIOD_4 (4 * 365 + 1)
#define PERIOD_100 (PERIOD_4 * 25 - 1)
#define PERIOD_400 (PERIOD_100 * 4 + 1)

BOOL WINAPI FileTimeToSystemTime(const FILETIME *ft, SYSTEMTIME *st)
{
  UInt32 v;
  UInt64 v64 = GET_TIME_64(ft);
  v64 /= 10000;
  st->wMilliseconds = (WORD)(v64 % 1000); v64 /= 1000;
  st->wSecond       = (WORD)(v64 %   60); v64 /= 60;
  st->wMinute       = (WORD)(v64 %   60); v64 /= 60;
  v = (UInt32)v64;
  st->wHour         = (WORD)(v %   24); v /= 24;

  // 1601-01-01 was Monday
  st->wDayOfWeek = (WORD)((v + 1) % 7);

  UInt32 leaps, year, day, mon;
  leaps = (3 * ((4 * v + (365 - 31 - 28) * 4 + 3) / PERIOD_400) + 3) / 4;
  v += 28188 + leaps;
  // leaps - the number of exceptions from PERIOD_4 rules starting from 1600-03-01
  // (1959 / 64) - converts day from 03-01 to month
  year = (20 * v - 2442) / (5 * PERIOD_4);
  day = v - (year * PERIOD_4) / 4;
  mon = (64 * day) / 1959;
  st->wDay = (WORD)(day - (1959 * mon) / 64);
  mon -= 1;
  year += 1524;
  if (mon > 12)
  {
    mon -= 12;
    year++;
  }
  st->wMonth = (WORD)mon;
  st->wYear = (WORD)year;

  /*
  unsigned year, mon;
  unsigned char ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  unsigned t;

  year = (WORD)(1601 + v / PERIOD_400 * 400);
  v %= PERIOD_400;

  t = v / PERIOD_100; if (t ==  4) t =  3; year += t * 100; v -= t * PERIOD_100;
  t = v / PERIOD_4;   if (t == 25) t = 24; year += t * 4;   v -= t * PERIOD_4;
  t = v / 365;        if (t ==  4) t =  3; year += t;       v -= t * 365;

  st->wYear = (WORD)year;

  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
  for (mon = 0;; mon++)
  {
    unsigned d = ms[mon];
    if (v < d)
      break;
    v -= d;
  }
  st->wDay = (WORD)(v + 1);
  st->wMonth = (WORD)(mon + 1);
  */

  return TRUE;
}

#endif
