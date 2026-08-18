/* XArchive amalgamation of 7-Zip 26.01 -- CPP/7zip/Compress codecs (non-registering).
 *
 * 37 upstream translation units folded into one. Code is verbatim;
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

/* ---- CPP/7zip/Compress/StdAfx.h ---- */
// StdAfx.h

#ifndef ZIP7_INC_STDAFX_H
#define ZIP7_INC_STDAFX_H

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif
// amalgamation: header emitted in prologue

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

/* ---- C/BwtSort.h ---- */
/* BwtSort.h -- BWT block sorting
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_BWT_SORT_H
#define ZIP7_INC_BWT_SORT_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

/* use BLOCK_SORT_EXTERNAL_FLAGS if blockSize can be > 1M */
/* #define BLOCK_SORT_EXTERNAL_FLAGS */
// #define BLOCK_SORT_EXTERNAL_FLAGS

#ifdef BLOCK_SORT_EXTERNAL_FLAGS
#define BLOCK_SORT_EXTERNAL_SIZE(blockSize) (((blockSize) + 31) >> 5)
#else
#define BLOCK_SORT_EXTERNAL_SIZE(blockSize) 0
#endif

#define BLOCK_SORT_BUF_SIZE(blockSize) ((blockSize) * 2 + BLOCK_SORT_EXTERNAL_SIZE(blockSize) + (1 << 16))

UInt32 BlockSort(UInt32 *indices, const Byte *data, size_t blockSize);

EXTERN_C_END

#endif

/* ---- C/HuffEnc.h ---- */
/* HuffEnc.h -- Huffman encoding
Igor Pavlov : Public domain */

#ifndef ZIP7_INC_HUFF_ENC_H
#define ZIP7_INC_HUFF_ENC_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define Z7_HUFFMAN_LEN_MAX 16
#define Z7_HUFFMAN_FREQS_SUM_MAX ((1 << 22) - 1)
/*
Conditions:
  2 <= num <= 1024 = 2 ^ NUM_BITS
  Sum(freqs) <= Z7_HUFFMAN_FREQS_SUM_MAX = 4M - 1 = 2 ^ (32 - NUM_BITS) - 1
  1 <= maxLen <= 16 = Z7_HUFFMAN_LEN_MAX
*/
void Huffman_Generate(const UInt32 *freqs, UInt32 *p, Byte *lens, unsigned num, unsigned maxLen);

EXTERN_C_END

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

/* ---- CPP/7zip/Compress/BitmEncoder.h ---- */
// BitmEncoder.h -- the Most Significant Bit of byte is First

#ifndef ZIP7_INC_BITM_ENCODER_H
#define ZIP7_INC_BITM_ENCODER_H

// amalgamation: header emitted in prologue

template<class TOutByte>
class CBitmEncoder
{
  unsigned _bitPos;  // 0 < _bitPos <= 8 : number of non-filled low bits in _curByte
  unsigned _curByte; // low (_bitPos) bits are zeros
                     // high (8 - _bitPos) bits are filled
  TOutByte _stream;
public:
  bool Create(UInt32 bufferSize) { return _stream.Create(bufferSize); }
  void SetStream(ISequentialOutStream *outStream) { _stream.SetStream(outStream);}
  UInt64 GetProcessedSize() const { return _stream.GetProcessedSize() + ((8 - _bitPos + 7) >> 3); }
  void Init()
  {
    _stream.Init();
    _bitPos = 8;
    _curByte = 0;
  }
  HRESULT Flush()
  {
    if (_bitPos < 8)
    {
      _stream.WriteByte((Byte)_curByte);
      _bitPos = 8;
      _curByte = 0;
    }
    return _stream.Flush();
  }

  // required condition: (value >> numBits) == 0
  // numBits == 0 is allowed
  void WriteBits(UInt32 value, unsigned numBits)
  {
    do
    {
      unsigned bp = _bitPos;
      unsigned curByte = _curByte;
      if (numBits < bp)
      {
        bp -= numBits;
        _curByte = curByte | (value << bp);
        _bitPos = bp;
        return;
      }
      numBits -= bp;
      const UInt32 hi = (value >> numBits);
      value -= (hi << numBits);
      _stream.WriteByte((Byte)(curByte | hi));
      _bitPos = 8;
      _curByte = 0;
    }
    while (numBits);
  }
  void WriteByte(unsigned b)
  {
    const unsigned bp = _bitPos;
    const unsigned a = _curByte | (b >> (8 - bp));
    _curByte = b << bp;
   _stream.WriteByte((Byte)a);
  }

  void WriteBytes(const Byte *data, size_t num)
  {
    const unsigned bp = _bitPos;
#if 1 // 1 for optional speed-optimized code branch
    if (bp == 8)
    {
      _stream.WriteBytes(data, num);
      return;
    }
#endif
    unsigned c = _curByte;
    const unsigned bp_rev = 8 - bp;
    for (size_t i = 0; i < num; i++)
    {
      const unsigned b = data[i];
      _stream.WriteByte((Byte)(c | (b >> bp_rev)));
      c = b << bp;
    }
    _curByte = c;
  }
};

#endif

/* ---- CPP/7zip/Compress/BZip2Encoder.h ---- */
// BZip2Encoder.h

#ifndef ZIP7_INC_COMPRESS_BZIP2_ENCODER_H
#define ZIP7_INC_COMPRESS_BZIP2_ENCODER_H

// amalgamation: header emitted in prologue

#ifndef Z7_ST
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
#endif

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBZip2 {

const unsigned kNumPassesMax = 10;

struct CMsbfEncoderTemp
{
  unsigned _bitPos;  // 0 < _bitPos <= 8 : number of non-filled low bits in _curByte
  unsigned _curByte; // low (_bitPos) bits are zeros
                     // high (8 - _bitPos) bits are filled
  Byte *_buf;
  Byte *_buf_base;
  void SetStream(Byte *buf) { _buf_base = _buf = buf;  }
  Byte *GetStream() const { return _buf_base; }

  void Init()
  {
    _bitPos = 8;
    _curByte = 0;
    _buf = _buf_base;
  }

  // required condition: (value >> numBits) == 0
  // numBits == 0 is allowed
  void WriteBits(UInt32 value, unsigned numBits)
  {
    do
    {
      unsigned bp = _bitPos;
      unsigned curByte = _curByte;
      if (numBits < bp)
      {
        bp -= numBits;
        _curByte = curByte | (value << bp);
        _bitPos = bp;
        return;
      }
      numBits -= bp;
      const UInt32 hi = value >> numBits;
      value -= (hi << numBits);
      Byte *buf = _buf;
      _bitPos = 8;
      _curByte = 0;
      *buf++ = (Byte)(curByte | hi);
      _buf = buf;
    }
    while (numBits);
  }

  void WriteBit(unsigned value)
  {
    const unsigned bp = _bitPos - 1;
    const unsigned curByte = _curByte | (value << bp);
    _curByte = curByte;
    _bitPos = bp;
    if (bp == 0)
    {
      *_buf++ = (Byte)curByte;
      _curByte = 0;
      _bitPos = 8;
    }
  }

  void WriteByte(unsigned b)
  {
    const unsigned bp = _bitPos;
    const unsigned a = _curByte | (b >> (8 - bp));
    _curByte = b << bp;
    Byte *buf = _buf;
    *buf++ = (Byte)a;
    _buf = buf;
  }

  UInt32 GetBytePos() const { return (UInt32)(size_t)(_buf - _buf_base); }
  UInt32 GetPos() const { return GetBytePos() * 8 + 8 - _bitPos; }
  unsigned GetCurByte() const { return _curByte; }
  unsigned GetNonFlushedByteBits() const { return _curByte >> _bitPos; }
  void SetPos(UInt32 bitPos)
  {
    _buf = _buf_base + (bitPos >> 3);
    _bitPos = 8 - ((unsigned)bitPos & 7);
  }
  void SetCurState(unsigned bitPos, unsigned curByte)
  {
    _bitPos = 8 - bitPos;
    _curByte = curByte;
  }
};


class CEncoder;

class CThreadInfo
{
private:
  CMsbfEncoderTemp m_OutStreamCurrent;
public:
  CEncoder *Encoder;
  Byte *m_Block;
private:
  Byte *m_MtfArray;
  Byte *m_TempArray;
  UInt32 *m_BlockSorterIndex;

public:
  bool m_OptimizeNumTables;
  UInt32 m_NumCrcs;
  UInt32 m_BlockIndex;
  UInt64 m_UnpackSize;

  Byte *m_Block_Base;

  Byte Lens[kNumTablesMax][kMaxAlphaSize];
  UInt32 Freqs[kNumTablesMax][kMaxAlphaSize];
  UInt32 Codes[kNumTablesMax][kMaxAlphaSize];

  Byte m_Selectors[kNumSelectorsMax];

  UInt32 m_CRCs[1 << kNumPassesMax];

  void WriteBits2(UInt32 value, unsigned numBits);
  void WriteByte2(unsigned b) { WriteBits2(b, 8); }
  void WriteBit2(unsigned v)  { m_OutStreamCurrent.WriteBit(v); }

  void EncodeBlock(const Byte *block, UInt32 blockSize);
  UInt32 EncodeBlockWithHeaders(const Byte *block, UInt32 blockSize);
  void EncodeBlock2(const Byte *block, UInt32 blockSize, UInt32 numPasses);
public:
#ifndef Z7_ST
  NWindows::CThread Thread;

  NWindows::NSynchronization::CAutoResetEvent StreamWasFinishedEvent;
  NWindows::NSynchronization::CAutoResetEvent WaitingWasStartedEvent;

  // it's not member of this thread. We just need one event per thread
  NWindows::NSynchronization::CAutoResetEvent CanWriteEvent;

public:
  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
  HRESULT Create();
  void FinishStream(bool needLeave);
  THREAD_FUNC_RET_TYPE ThreadFunc();
#endif

  CThreadInfo(): m_BlockSorterIndex(NULL), m_Block_Base(NULL) {}
  ~CThreadInfo() { Free(); }
  bool Alloc();
  void Free();

  HRESULT EncodeBlock3(UInt32 blockSize);
};


struct CEncProps
{
  UInt32 BlockSizeMult;
  UInt32 NumPasses;
  UInt32 NumThreadGroups;
  UInt64 Affinity;
  
  CEncProps()
  {
    BlockSizeMult = (UInt32)(Int32)-1;
    NumPasses = (UInt32)(Int32)-1;
    NumThreadGroups = 0;
    Affinity = 0;
  }
  void Normalize(int level);
  bool DoOptimizeNumTables() const { return NumPasses > 1; }
};

class CEncoder Z7_final:
  public ICompressCoder,
  public ICompressSetCoderProperties,
 #ifndef Z7_ST
  public ICompressSetCoderMt,
 #endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetCoderProperties)
 #ifndef Z7_ST
  Z7_COM_QI_ENTRY(ICompressSetCoderMt)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetCoderProperties)
 #ifndef Z7_ST
  Z7_IFACE_COM7_IMP(ICompressSetCoderMt)
 #endif

 #ifndef Z7_ST
  UInt32 m_NumThreadsPrev;
 #endif
public:
  CInBuffer m_InStream;
 #ifndef Z7_ST
  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
 #endif
  CBitmEncoder<COutBuffer> m_OutStream;
  CEncProps _props;
  CBZip2CombinedCrc CombinedCrc;

 #ifndef Z7_ST
  CThreadInfo *ThreadsInfo;
  NWindows::NSynchronization::CManualResetEvent CanProcessEvent;
  NWindows::NSynchronization::CCriticalSection CS;
  UInt32 NumThreads;
  bool MtMode;
  UInt32 NextBlockIndex;

  bool CloseThreads;
  bool StreamWasFinished;
  NWindows::NSynchronization::CManualResetEvent CanStartWaitingEvent;
  CThreadNextGroup ThreadNextGroup;

  HRESULT Result;
  ICompressProgressInfo *Progress;
 #else
  CThreadInfo ThreadsInfo;
 #endif

  UInt64 NumBlocks;

  UInt64 GetInProcessedSize() const { return m_InStream.GetProcessedSize(); }

  UInt32 ReadRleBlock(Byte *buf);
  void WriteBytes(const Byte *data, UInt32 sizeInBits, unsigned lastByteBits);
  void WriteByte(Byte b);

 #ifndef Z7_ST
  HRESULT Create();
  void Free();
 #endif

public:
  CEncoder();
 #ifndef Z7_ST
  ~CEncoder();
 #endif

  HRESULT Flush() { return m_OutStream.Flush(); }
  
  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
};

}}

#endif

/* ---- C/Bcj2.h ---- */
/* Bcj2.h -- BCJ2 converter for x86 code (Branch CALL/JUMP variant2)
2023-03-02 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_BCJ2_H
#define ZIP7_INC_BCJ2_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

#define BCJ2_NUM_STREAMS 4

enum
{
  BCJ2_STREAM_MAIN,
  BCJ2_STREAM_CALL,
  BCJ2_STREAM_JUMP,
  BCJ2_STREAM_RC
};

enum
{
  BCJ2_DEC_STATE_ORIG_0 = BCJ2_NUM_STREAMS,
  BCJ2_DEC_STATE_ORIG_1,
  BCJ2_DEC_STATE_ORIG_2,
  BCJ2_DEC_STATE_ORIG_3,
  
  BCJ2_DEC_STATE_ORIG,
  BCJ2_DEC_STATE_ERROR     /* after detected data error */
};

enum
{
  BCJ2_ENC_STATE_ORIG = BCJ2_NUM_STREAMS,
  BCJ2_ENC_STATE_FINISHED  /* it's state after fully encoded stream */
};


/* #define BCJ2_IS_32BIT_STREAM(s) ((s) == BCJ2_STREAM_CALL || (s) == BCJ2_STREAM_JUMP) */
#define BCJ2_IS_32BIT_STREAM(s) ((unsigned)((unsigned)(s) - (unsigned)BCJ2_STREAM_CALL) < 2)

/*
CBcj2Dec / CBcj2Enc
bufs sizes:
  BUF_SIZE(n) = lims[n] - bufs[n]
bufs sizes for BCJ2_STREAM_CALL and BCJ2_STREAM_JUMP must be multiply of 4:
    (BUF_SIZE(BCJ2_STREAM_CALL) & 3) == 0
    (BUF_SIZE(BCJ2_STREAM_JUMP) & 3) == 0
*/

// typedef UInt32 CBcj2Prob;
typedef UInt16 CBcj2Prob;

/*
BCJ2 encoder / decoder internal requirements:
  - If last bytes of stream contain marker (e8/e8/0f8x), then
    there is also encoded symbol (0 : no conversion) in RC stream.
  - One case of overlapped instructions is supported,
    if last byte of converted instruction is (0f) and next byte is (8x):
      marker [xx xx xx 0f] 8x
    then the pair (0f 8x) is treated as marker.
*/

/* ---------- BCJ2 Decoder ---------- */

/*
CBcj2Dec:
(dest) is allowed to overlap with bufs[BCJ2_STREAM_MAIN], with the following conditions:
  bufs[BCJ2_STREAM_MAIN] >= dest &&
  bufs[BCJ2_STREAM_MAIN] - dest >=
        BUF_SIZE(BCJ2_STREAM_CALL) +
        BUF_SIZE(BCJ2_STREAM_JUMP)
  reserve = bufs[BCJ2_STREAM_MAIN] - dest -
      ( BUF_SIZE(BCJ2_STREAM_CALL) +
        BUF_SIZE(BCJ2_STREAM_JUMP) )
  and additional conditions:
  if (it's first call of Bcj2Dec_Decode() after Bcj2Dec_Init())
  {
    (reserve != 1) : if (ver <  v23.00)
  }
  else // if there are more than one calls of Bcj2Dec_Decode() after Bcj2Dec_Init())
  {
    (reserve >= 6) : if (ver <  v23.00)
    (reserve >= 4) : if (ver >= v23.00)
    We need that (reserve) because after first call of Bcj2Dec_Decode(),
    CBcj2Dec::temp can contain up to 4 bytes for writing to (dest).
  }
  (reserve == 0) is allowed, if we decode full stream via single call of Bcj2Dec_Decode().
  (reserve == 0) also is allowed in case of multi-call, if we use fixed buffers,
     and (reserve) is calculated from full (final) sizes of all streams before first call.
*/

typedef struct
{
  const Byte *bufs[BCJ2_NUM_STREAMS];
  const Byte *lims[BCJ2_NUM_STREAMS];
  Byte *dest;
  const Byte *destLim;

  unsigned state; /* BCJ2_STREAM_MAIN has more priority than BCJ2_STATE_ORIG */

  UInt32 ip;      /* property of starting base for decoding */
  UInt32 temp;    /* Byte temp[4]; */
  UInt32 range;
  UInt32 code;
  CBcj2Prob probs[2 + 256];
} CBcj2Dec;


/* Note:
   Bcj2Dec_Init() sets (CBcj2Dec::ip = 0)
   if (ip != 0) property is required, the caller must set CBcj2Dec::ip after Bcj2Dec_Init()
*/
void Bcj2Dec_Init(CBcj2Dec *p);


/* Bcj2Dec_Decode():
   returns:
     SZ_OK
     SZ_ERROR_DATA : if data in 5 starting bytes of BCJ2_STREAM_RC stream are not correct
*/
SRes Bcj2Dec_Decode(CBcj2Dec *p);

/* To check that decoding was finished you can compare
   sizes of processed streams with sizes known from another sources.
   You must do at least one mandatory check from the two following options:
      - the check for size of processed output (ORIG) stream.
      - the check for size of processed input  (MAIN) stream.
   additional optional checks:
      - the checks for processed sizes of all input streams (MAIN, CALL, JUMP, RC)
      - the checks Bcj2Dec_IsMaybeFinished*()
   also before actual decoding you can check that the
   following condition is met for stream sizes:
     ( size(ORIG) == size(MAIN) + size(CALL) + size(JUMP) )
*/

/* (state == BCJ2_STREAM_MAIN) means that decoder is ready for
      additional input data in BCJ2_STREAM_MAIN stream.
   Note that (state == BCJ2_STREAM_MAIN) is allowed for non-finished decoding.
*/
#define Bcj2Dec_IsMaybeFinished_state_MAIN(_p_) ((_p_)->state == BCJ2_STREAM_MAIN)

/* if the stream decoding was finished correctly, then range decoder
   part of CBcj2Dec also was finished, and then (CBcj2Dec::code == 0).
   Note that (CBcj2Dec::code == 0) is allowed for non-finished decoding.
*/
#define Bcj2Dec_IsMaybeFinished_code(_p_) ((_p_)->code == 0)

/* use Bcj2Dec_IsMaybeFinished() only as additional check
    after at least one mandatory check from the two following options:
      - the check for size of processed output (ORIG) stream.
      - the check for size of processed input  (MAIN) stream.
*/
#define Bcj2Dec_IsMaybeFinished(_p_) ( \
        Bcj2Dec_IsMaybeFinished_state_MAIN(_p_) && \
        Bcj2Dec_IsMaybeFinished_code(_p_))



/* ---------- BCJ2 Encoder ---------- */

typedef enum
{
  BCJ2_ENC_FINISH_MODE_CONTINUE,
  BCJ2_ENC_FINISH_MODE_END_BLOCK,
  BCJ2_ENC_FINISH_MODE_END_STREAM
} EBcj2Enc_FinishMode;

/*
  BCJ2_ENC_FINISH_MODE_CONTINUE:
     process non finished encoding.
     It notifies the encoder that additional further calls
     can provide more input data (src) than provided by current call.
     In  that case the CBcj2Enc encoder still can move (src) pointer
     up to (srcLim), but CBcj2Enc encoder can store some of the last
     processed bytes (up to 4 bytes) from src to internal CBcj2Enc::temp[] buffer.
   at return:
       (CBcj2Enc::src will point to position that includes
       processed data and data copied to (temp[]) buffer)
       That data from (temp[]) buffer will be used in further calls.
  
  BCJ2_ENC_FINISH_MODE_END_BLOCK:
     finish encoding of current block (ended at srcLim) without RC flushing.
   at return: if (CBcj2Enc::state == BCJ2_ENC_STATE_ORIG) &&
                  CBcj2Enc::src == CBcj2Enc::srcLim)
        :  it shows that block encoding was finished. And the encoder is
           ready for new (src) data or for stream finish operation.
     finished block means
     {
       CBcj2Enc has completed block encoding up to (srcLim).
       (1 + 4 bytes) or (2 + 4 bytes) CALL/JUMP cortages will
       not cross block boundary at (srcLim).
       temporary CBcj2Enc buffer for (ORIG) src data is empty.
       3 output uncompressed streams (MAIN, CALL, JUMP) were flushed.
       RC stream was not flushed. And RC stream will cross block boundary.
     }
     Note: some possible implementation of BCJ2 encoder could
     write branch marker (e8/e8/0f8x) in one call of Bcj2Enc_Encode(),
     and it could calculate symbol for RC in another call of Bcj2Enc_Encode().
     BCJ2 encoder uses ip/fileIp/fileSize/relatLimit values to calculate RC symbol.
     And these CBcj2Enc variables can have different values in different Bcj2Enc_Encode() calls.
     So caller must finish each block with BCJ2_ENC_FINISH_MODE_END_BLOCK
     to ensure that RC symbol is calculated and written in proper block.
    
  BCJ2_ENC_FINISH_MODE_END_STREAM
     finish encoding of stream (ended at srcLim) fully including RC flushing.
   at return: if (CBcj2Enc::state == BCJ2_ENC_STATE_FINISHED)
        : it shows that stream encoding was finished fully,
          and all output streams were flushed fully.
     also Bcj2Enc_IsFinished() can be called.
*/


/*
  32-bit relative offset in JUMP/CALL commands is
    - (mod 4 GiB)  for 32-bit x86 code
    - signed Int32 for 64-bit x86-64 code
  BCJ2 encoder also does internal relative to absolute address conversions.
  And there are 2 possible ways to do it:
    before v23: we used 32-bit variables and (mod 4 GiB) conversion
    since  v23: we use  64-bit variables and (signed Int32 offset) conversion.
  The absolute address condition for conversion in v23:
    ((UInt64)((Int64)ip64 - (Int64)fileIp64 + 5 + (Int32)offset) < (UInt64)fileSize64)
  note that if (fileSize64 > 2 GiB). there is difference between
  old (mod 4 GiB) way (v22) and new (signed Int32 offset) way (v23).
  And new (v23) way is more suitable to encode 64-bit x86-64 code for (fileSize64 > 2 GiB) cases.
*/

/*
// for old (v22) way for conversion:
typedef UInt32 CBcj2Enc_ip_unsigned;
typedef  Int32 CBcj2Enc_ip_signed;
#define BCJ2_ENC_FileSize_MAX ((UInt32)1 << 31)
*/
typedef UInt64 CBcj2Enc_ip_unsigned;
typedef  Int64 CBcj2Enc_ip_signed;

/* maximum size of file that can be used for conversion condition */
#define BCJ2_ENC_FileSize_MAX             ((CBcj2Enc_ip_unsigned)0 - 2)

/* default value of fileSize64_minus1 variable that means
   that absolute address limitation will not be used */
#define BCJ2_ENC_FileSizeField_UNLIMITED  ((CBcj2Enc_ip_unsigned)0 - 1)

/* calculate value that later can be set to CBcj2Enc::fileSize64_minus1 */
#define BCJ2_ENC_GET_FileSizeField_VAL_FROM_FileSize(fileSize) \
    ((CBcj2Enc_ip_unsigned)(fileSize) - 1)

/* set CBcj2Enc::fileSize64_minus1 variable from size of file */
#define Bcj2Enc_SET_FileSize(p, fileSize) \
    (p)->fileSize64_minus1 = BCJ2_ENC_GET_FileSizeField_VAL_FROM_FileSize(fileSize);


typedef struct
{
  Byte *bufs[BCJ2_NUM_STREAMS];
  const Byte *lims[BCJ2_NUM_STREAMS];
  const Byte *src;
  const Byte *srcLim;

  unsigned state;
  EBcj2Enc_FinishMode finishMode;

  Byte context;
  Byte flushRem;
  Byte isFlushState;

  Byte cache;
  UInt32 range;
  UInt64 low;
  UInt64 cacheSize;
  
  // UInt32 context;  // for marker version, it can include marker flag.

  /* (ip64) and (fileIp64) correspond to virtual source stream position
     that doesn't include data in temp[] */
  CBcj2Enc_ip_unsigned ip64;         /* current (ip) position */
  CBcj2Enc_ip_unsigned fileIp64;     /* start (ip) position of current file */
  CBcj2Enc_ip_unsigned fileSize64_minus1;   /* size of current file (for conversion limitation) */
  UInt32 relatLimit;  /* (relatLimit <= ((UInt32)1 << 31)) : 0 means disable_conversion */
  // UInt32 relatExcludeBits;

  UInt32 tempTarget;
  unsigned tempPos; /* the number of bytes that were copied to temp[] buffer
                       (tempPos <= 4) outside of Bcj2Enc_Encode() */
  // Byte temp[4]; // for marker version
  Byte temp[8];
  CBcj2Prob probs[2 + 256];
} CBcj2Enc;

void Bcj2Enc_Init(CBcj2Enc *p);


/*
Bcj2Enc_Encode(): at exit:
  p->State <  BCJ2_NUM_STREAMS    : we need more buffer space for output stream
                                    (bufs[p->State] == lims[p->State])
  p->State == BCJ2_ENC_STATE_ORIG : we need more data in input src stream
                                    (src == srcLim)
  p->State == BCJ2_ENC_STATE_FINISHED : after fully encoded stream
*/
void Bcj2Enc_Encode(CBcj2Enc *p);

/* Bcj2Enc encoder can look ahead for up 4 bytes of source stream.
   CBcj2Enc::tempPos : is the number of bytes that were copied from input stream to temp[] buffer.
   (CBcj2Enc::src) after Bcj2Enc_Encode() is starting position after
   fully processed data and after data copied to temp buffer.
   So if the caller needs to get real number of fully processed input
   bytes (without look ahead data in temp buffer),
   the caller must subtruct (CBcj2Enc::tempPos) value from processed size
   value that is calculated based on current (CBcj2Enc::src):
     cur_processed_pos = Calc_Big_Processed_Pos(enc.src)) -
        Bcj2Enc_Get_AvailInputSize_in_Temp(&enc);
*/
/* get the size of input data that was stored in temp[] buffer: */
#define Bcj2Enc_Get_AvailInputSize_in_Temp(p) ((p)->tempPos)

#define Bcj2Enc_IsFinished(p) ((p)->flushRem == 0)

/* Note : the decoder supports overlapping of marker (0f 80).
   But we can eliminate such overlapping cases by setting
   the limit for relative offset conversion as
     CBcj2Enc::relatLimit <= (0x0f << 24) == (240 MiB)
*/
/* default value for CBcj2Enc::relatLimit */
#define BCJ2_ENC_RELAT_LIMIT_DEFAULT  ((UInt32)0x0f << 24)
#define BCJ2_ENC_RELAT_LIMIT_MAX      ((UInt32)1 << 31)
// #define BCJ2_RELAT_EXCLUDE_NUM_BITS 5

EXTERN_C_END

#endif

/* ---- CPP/7zip/Compress/Bcj2Coder.h ---- */
// Bcj2Coder.h

#ifndef ZIP7_INC_COMPRESS_BCJ2_CODER_H
#define ZIP7_INC_COMPRESS_BCJ2_CODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBcj2 {

class CBaseCoder
{
protected:
  Byte *_bufs[BCJ2_NUM_STREAMS + 1];
  UInt32 _bufsSizes[BCJ2_NUM_STREAMS + 1];
  UInt32 _bufsSizes_New[BCJ2_NUM_STREAMS + 1];

  HRESULT Alloc(bool allocForOrig = true);
public:
  CBaseCoder();
  ~CBaseCoder();
};


#ifndef Z7_EXTRACT_ONLY

class CEncoder Z7_final:
  public ICompressCoder2,
  public ICompressSetCoderProperties,
  public ICompressSetBufSize,
  public CMyUnknownImp,
  public CBaseCoder
{
  Z7_IFACES_IMP_UNK_3(
      ICompressCoder2,
      ICompressSetCoderProperties,
      ICompressSetBufSize)

  UInt32 _relatLim;
  // UInt32 _excludeRangeBits;

  HRESULT CodeReal(
      ISequentialInStream * const *inStreams, const UInt64 * const *inSizes, UInt32 numInStreams,
      ISequentialOutStream * const *outStreams, const UInt64 * const *outSizes, UInt32 numOutStreams,
      ICompressProgressInfo *progress);
public:
  CEncoder();
  ~CEncoder();
};

#endif



class CBaseDecoder: public CBaseCoder
{
protected:
  HRESULT _readRes[BCJ2_NUM_STREAMS];
  unsigned _extraSizes[BCJ2_NUM_STREAMS];
  UInt64 _readSizes[BCJ2_NUM_STREAMS];

  CBcj2Dec dec;

  UInt64 GetProcessedSize_ForInStream(unsigned i) const
  {
    return _readSizes[i] - ((size_t)(dec.lims[i] - dec.bufs[i]) + _extraSizes[i]);
  }
  void InitCommon();
  void ReadInStream(ISequentialInStream *inStream);
};


class CDecoder Z7_final:
  public ICompressCoder2,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize2,
  public ICompressSetBufSize,
#ifndef Z7_NO_READ_FROM_CODER
  public ICompressSetInStream2,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
#endif
  public CMyUnknownImp,
  public CBaseDecoder
{
  Z7_COM_QI_BEGIN2(ICompressCoder2)
    Z7_COM_QI_ENTRY(ICompressSetFinishMode)
    Z7_COM_QI_ENTRY(ICompressGetInStreamProcessedSize2)
    Z7_COM_QI_ENTRY(ICompressSetBufSize)
  #ifndef Z7_NO_READ_FROM_CODER
    Z7_COM_QI_ENTRY(ICompressSetInStream2)
    Z7_COM_QI_ENTRY(ICompressSetOutStreamSize)
    Z7_COM_QI_ENTRY(ISequentialInStream)
  #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE
  
  Z7_IFACE_COM7_IMP(ICompressCoder2)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize2)
  Z7_IFACE_COM7_IMP(ICompressSetBufSize)
#ifndef Z7_NO_READ_FROM_CODER
  Z7_IFACE_COM7_IMP(ICompressSetInStream2)
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
  Z7_IFACE_COM7_IMP(ISequentialInStream)
#endif

  bool _finishMode;

#ifndef Z7_NO_READ_FROM_CODER
  bool _outSizeDefined;
  UInt64 _outSize;
  UInt64 _outSize_Processed;
  CMyComPtr<ISequentialInStream> _inStreams[BCJ2_NUM_STREAMS];
#endif
 
public:
  CDecoder();
};

}}

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

/* ---- CPP/7zip/Compress/BranchMisc.h ---- */
// BranchMisc.h

#ifndef ZIP7_INC_COMPRESS_BRANCH_MISC_H
#define ZIP7_INC_COMPRESS_BRANCH_MISC_H
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBranch {

Z7_CLASS_IMP_COM_1(
  CCoder
  , ICompressFilter
)
  UInt32 _pc;
  z7_Func_BranchConv BraFunc;
public:
  CCoder(z7_Func_BranchConv bra): _pc(0), BraFunc(bra) {}
};

#ifndef Z7_EXTRACT_ONLY

Z7_CLASS_IMP_COM_3(
  CEncoder
  , ICompressFilter
  , ICompressSetCoderProperties
  , ICompressWriteCoderProperties
)
  UInt32 _pc;
  UInt32 _pc_Init;
  UInt32 _alignment;
  z7_Func_BranchConv BraFunc;
public:
  CEncoder(z7_Func_BranchConv bra, UInt32 alignment):
      _pc(0), _pc_Init(0), _alignment(alignment), BraFunc(bra) {}
};

#endif

Z7_CLASS_IMP_COM_2(
  CDecoder
  , ICompressFilter
  , ICompressSetDecoderProperties2
)
  UInt32 _pc;
  UInt32 _pc_Init;
  UInt32 _alignment;
  z7_Func_BranchConv BraFunc;
public:
  CDecoder(z7_Func_BranchConv bra, UInt32 alignment):
      _pc(0), _pc_Init(0), _alignment(alignment), BraFunc(bra) {}
};

}}

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

/* ---- C/Lzma2DecMt.h ---- */
/* Lzma2DecMt.h -- LZMA2 Decoder Multi-thread
2023-04-13 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_LZMA2_DEC_MT_H
#define ZIP7_INC_LZMA2_DEC_MT_H

// amalgamation: header emitted in prologue

EXTERN_C_BEGIN

typedef struct
{
  size_t inBufSize_ST;
  size_t outStep_ST;
  
  #ifndef Z7_ST
  unsigned numThreads;
  size_t inBufSize_MT;
  size_t outBlockMax;
  size_t inBlockMax;
  #endif
} CLzma2DecMtProps;

/* init to single-thread mode */
void Lzma2DecMtProps_Init(CLzma2DecMtProps *p);


/* ---------- CLzma2DecMtHandle Interface ---------- */

/* Lzma2DecMt_ * functions can return the following exit codes:
SRes:
  SZ_OK           - OK
  SZ_ERROR_MEM    - Memory allocation error
  SZ_ERROR_PARAM  - Incorrect paramater in props
  SZ_ERROR_WRITE  - ISeqOutStream write callback error
  // SZ_ERROR_OUTPUT_EOF - output buffer overflow - version with (Byte *) output
  SZ_ERROR_PROGRESS - some break from progress callback
  SZ_ERROR_THREAD - error in multithreading functions (only for Mt version)
*/

typedef struct CLzma2DecMt CLzma2DecMt;
typedef CLzma2DecMt * CLzma2DecMtHandle;
// Z7_DECLARE_HANDLE(CLzma2DecMtHandle)

CLzma2DecMtHandle Lzma2DecMt_Create(ISzAllocPtr alloc, ISzAllocPtr allocMid);
void Lzma2DecMt_Destroy(CLzma2DecMtHandle p);

SRes Lzma2DecMt_Decode(CLzma2DecMtHandle p,
    Byte prop,
    const CLzma2DecMtProps *props,
    ISeqOutStreamPtr outStream,
    const UInt64 *outDataSize, // NULL means undefined
    int finishMode,            // 0 - partial unpacking is allowed, 1 - if lzma2 stream must be finished
    // Byte *outBuf, size_t *outBufSize,
    ISeqInStreamPtr inStream,
    // const Byte *inData, size_t inDataSize,
    
    // out variables:
    UInt64 *inProcessed,
    int *isMT,  /* out: (*isMT == 0), if single thread decoding was used */

    // UInt64 *outProcessed,
    ICompressProgressPtr progress);


/* ---------- Read from CLzma2DecMtHandle Interface ---------- */

SRes Lzma2DecMt_Init(CLzma2DecMtHandle pp,
    Byte prop,
    const CLzma2DecMtProps *props,
    const UInt64 *outDataSize, int finishMode,
    ISeqInStreamPtr inStream);

SRes Lzma2DecMt_Read(CLzma2DecMtHandle pp,
    Byte *data, size_t *outSize,
    UInt64 *inStreamProcessed);


EXTERN_C_END

#endif

/* ---- CPP/7zip/Compress/Lzma2Decoder.h ---- */
// Lzma2Decoder.h

#ifndef ZIP7_INC_LZMA2_DECODER_H
#define ZIP7_INC_LZMA2_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzma2 {

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
 #ifndef Z7_ST
  public ICompressSetCoderMt,
  public ICompressSetMemLimit,
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
 #ifndef Z7_ST
  Z7_COM_QI_ENTRY(ICompressSetCoderMt)
  Z7_COM_QI_ENTRY(ICompressSetMemLimit)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetDecoderProperties2)
  Z7_IFACE_COM7_IMP(ICompressSetFinishMode)
  Z7_IFACE_COM7_IMP(ICompressGetInStreamProcessedSize)
  Z7_IFACE_COM7_IMP(ICompressSetBufSize)
 #ifndef Z7_NO_READ_FROM_CODER
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)
 #endif
 #ifndef Z7_ST
  Z7_IFACE_COM7_IMP(ICompressSetCoderMt)
  Z7_IFACE_COM7_IMP(ICompressSetMemLimit)
 #endif

  CLzma2DecMtHandle _dec;
  UInt64 _inProcessed;
  Byte _prop;
  int _finishMode;
  UInt32 _inBufSize;
  UInt32 _outStep;

 #ifndef Z7_ST
  int _tryMt;
  UInt32 _numThreads;
  UInt64 _memUsage;
 #endif

 #ifndef Z7_NO_READ_FROM_CODER
  CMyComPtr<ISequentialInStream> _inStream;
  CSeqInStreamWrap _inWrap;
 #endif

public:
  CDecoder();
  ~CDecoder();
};

}}

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

/* ---- CPP/7zip/Compress/Lzma2Encoder.h ---- */
// Lzma2Encoder.h

#ifndef ZIP7_INC_LZMA2_ENCODER_H
#define ZIP7_INC_LZMA2_ENCODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzma2 {

Z7_CLASS_IMP_COM_4(
  CEncoder
  , ICompressCoder
  , ICompressSetCoderProperties
  , ICompressWriteCoderProperties
  , ICompressSetCoderPropertiesOpt
)
  CLzma2EncHandle _encoder;
public:
  CEncoder();
  ~CEncoder();
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

/* ---- CPP/7zip/Compress/LzmsDecoder.h ---- */
// LzmsDecoder.h
// The code is based on LZMS description from wimlib code

#ifndef ZIP7_INC_LZMS_DECODER_H
#define ZIP7_INC_LZMS_DECODER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzms {

const unsigned k_NumLitSyms = 256;
const unsigned k_NumLenSyms = 54;
const unsigned k_NumPosSyms = 799;
const unsigned k_NumPowerSyms = 8;

const unsigned k_NumProbBits = 6;
const unsigned k_ProbLimit = 1 << k_NumProbBits;
const unsigned k_InitialProb = 48;
const UInt32 k_InitialHist = 0x55555555;

const unsigned k_NumReps = 3;

const unsigned k_NumMainProbs  = 16;
const unsigned k_NumMatchProbs = 32;
const unsigned k_NumRepProbs   = 64;

const unsigned k_NumHuffmanBits = 15;

template <UInt32 m_NumSyms, UInt32 m_RebuildFreq, unsigned numTableBits>
class CHuffDecoder: public NCompress::NHuffman::CDecoder<k_NumHuffmanBits, m_NumSyms, numTableBits>
{
public:
  UInt32 RebuildRem;
  UInt32 NumSyms;
  UInt32 Freqs[m_NumSyms];

  void Generate() throw()
  {
    UInt32 vals[m_NumSyms];
    Byte levels[m_NumSyms];

    // We need to check that our algorithm is OK, when optimal Huffman tree uses more than 15 levels !!!
    Huffman_Generate(Freqs, vals, levels, NumSyms, k_NumHuffmanBits);

    for (UInt32 i = NumSyms; i < m_NumSyms; i++)
      levels[i] = 0;

    this->Build(levels, /* NumSyms, */ NHuffman::k_BuildMode_Full);
  }
  
  void Rebuild() throw()
  {
    Generate();
    RebuildRem = m_RebuildFreq;
    const UInt32 num = NumSyms;
    for (UInt32 i = 0; i < num; i++)
      Freqs[i] = (Freqs[i] >> 1) + 1;
  }

public:
  void Init(UInt32 numSyms = m_NumSyms) throw()
  {
    RebuildRem = m_RebuildFreq;
    NumSyms = numSyms;
    for (UInt32 i = 0; i < numSyms; i++)
      Freqs[i] = 1;
    // for (; i < m_NumSyms; i++) Freqs[i] = 0;
    Generate();
  }
};


struct CProbEntry
{
  UInt32 Prob;
  UInt64 Hist;

  void Init()
  {
    Prob = k_InitialProb;
    Hist = k_InitialHist;
  }

  UInt32 GetProb() const throw()
  {
    UInt32 prob = Prob;
    if (prob == 0)
      prob = 1;
    else if (prob == k_ProbLimit)
      prob = k_ProbLimit - 1;
    return prob;
  }

  void Update(unsigned bit) throw()
  {
    Prob += (UInt32)((Int32)(Hist >> (k_ProbLimit - 1)) - (Int32)bit);
    Hist = (Hist << 1) | bit;
  }
};


struct CRangeDecoder
{
  UInt32 range;
  UInt32 code;
  const Byte *cur;
  // const Byte *end;

  void Init(const Byte *data, size_t /* size */) throw()
  {
    range = 0xFFFFFFFF;
    code = (((UInt32)GetUi16(data)) << 16) | GetUi16(data + 2);
    cur = data + 4;
    // end = data + size;
  }

  void Normalize()
  {
    if (range <= 0xFFFF)
    {
      range <<= 16;
      code <<= 16;
      // if (cur >= end) throw 1;
      code |= GetUi16(cur);
      cur += 2;
    }
  }

  unsigned Decode(UInt32 *state, UInt32 numStates, struct CProbEntry *probs)
  {
    UInt32 st = *state;
    CProbEntry *entry = &probs[st];
    st = (st << 1) & (numStates - 1);

    const UInt32 prob = entry->GetProb();

    if (range <= 0xFFFF)
    {
      range <<= 16;
      code <<= 16;
      // if (cur >= end) throw 1;
      code |= GetUi16(cur);
      cur += 2;
    }

    const UInt32 bound = (range >> k_NumProbBits) * prob;
    
    if (code < bound)
    {
      range = bound;
      *state = st;
      entry->Update(0);
      return 0;
    }
    else
    {
      range -= bound;
      code -= bound;
      *state = st | 1;
      entry->Update(1);
      return 1;
    }
  }
};


class CDecoder
{
  // CRangeDecoder _rc;
  size_t _pos;

  UInt32 _reps[k_NumReps + 1];
  UInt64 _deltaReps[k_NumReps + 1];

  UInt32 mainState;
  UInt32 matchState;
  UInt32 lzRepStates[k_NumReps];
  UInt32 deltaRepStates[k_NumReps];

  struct CProbEntry mainProbs[k_NumMainProbs];
  struct CProbEntry matchProbs[k_NumMatchProbs];
  
  struct CProbEntry lzRepProbs[k_NumReps][k_NumRepProbs];
  struct CProbEntry deltaRepProbs[k_NumReps][k_NumRepProbs];

  CHuffDecoder<k_NumLitSyms, 1024, 9> m_LitDecoder;
  CHuffDecoder<k_NumPosSyms, 1024, 9> m_PosDecoder;
  CHuffDecoder<k_NumLenSyms, 512, 8> m_LenDecoder;
  CHuffDecoder<k_NumPowerSyms, 512, 6> m_PowerDecoder;
  CHuffDecoder<k_NumPosSyms, 1024, 9> m_DeltaDecoder;

  Int32 *_x86_history;

  HRESULT CodeReal(const Byte *in, size_t inSize, Byte *out, size_t outSize);
public:
  CDecoder();
  ~CDecoder();

  HRESULT Code(const Byte *in, size_t inSize, Byte *out, size_t outSize);
  size_t GetUnpackSize() const { return _pos; }
};

}}

#endif

/* ---- C/RotateDefs.h ---- */
/* RotateDefs.h -- Rotate functions
2023-06-18 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_ROTATE_DEFS_H
#define ZIP7_INC_ROTATE_DEFS_H

#ifdef _MSC_VER

#include <stdlib.h>

/* don't use _rotl with old MINGW. It can insert slow call to function. */
 
/* #if (_MSC_VER >= 1200) */
#pragma intrinsic(_rotl)
#pragma intrinsic(_rotr)
/* #endif */

#define rotlFixed(x, n) _rotl((x), (n))
#define rotrFixed(x, n) _rotr((x), (n))

#if (_MSC_VER >= 1300)
#define Z7_ROTL64(x, n) _rotl64((x), (n))
#define Z7_ROTR64(x, n) _rotr64((x), (n))
#else
#define Z7_ROTL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define Z7_ROTR64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))
#endif

#else

/* new compilers can translate these macros to fast commands. */

#if defined(__clang__) && (__clang_major__ >= 4) \
  || defined(__GNUC__) && (__GNUC__ >= 5)
/* GCC 4.9.0 and clang 3.5 can recognize more correct version: */
#define rotlFixed(x, n) (((x) << (n)) | ((x) >> (-(n) & 31)))
#define rotrFixed(x, n) (((x) >> (n)) | ((x) << (-(n) & 31)))
#define Z7_ROTL64(x, n) (((x) << (n)) | ((x) >> (-(n) & 63)))
#define Z7_ROTR64(x, n) (((x) >> (n)) | ((x) << (-(n) & 63)))
#else
/* for old GCC / clang: */
#define rotlFixed(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define rotrFixed(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define Z7_ROTL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define Z7_ROTR64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))
#endif

#endif

#endif

/* ---- CPP/7zip/Compress/Lzx.h ---- */
// Lzx.h

#ifndef ZIP7_INC_COMPRESS_LZX_H
#define ZIP7_INC_COMPRESS_LZX_H

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzx {

const unsigned kBlockType_NumBits = 3;
const unsigned kBlockType_Verbatim = 1;
const unsigned kBlockType_Aligned = 2;
const unsigned kBlockType_Uncompressed = 3;

const unsigned kNumHuffmanBits = 16;
const unsigned kNumReps = 3;

const unsigned kNumLenSlots = 8;
const unsigned kMatchMinLen = 2;
const unsigned kNumLenSymbols = 249;
const unsigned kMatchMaxLen = kMatchMinLen + (kNumLenSlots - 1) + kNumLenSymbols - 1;

const unsigned kNumAlignLevelBits = 3;
const unsigned kNumAlignBits = 3;
const unsigned kAlignTableSize = 1 << kNumAlignBits;

const unsigned kNumPosSlots = 50;
const unsigned kNumPosLenSlots = kNumPosSlots * kNumLenSlots;

const unsigned kMainTableSize = 256 + kNumPosLenSlots;
const unsigned kLevelTableSize = 20;
const unsigned kMaxTableSize = kMainTableSize;

const unsigned kNumLevelBits = 4;

const unsigned kLevelSym_Zero1 = 17;
const unsigned kLevelSym_Zero2 = 18;
const unsigned kLevelSym_Same = 19;

const unsigned kLevelSym_Zero1_Start = 4;
const unsigned kLevelSym_Zero1_NumBits = 4;

const unsigned kLevelSym_Zero2_Start = kLevelSym_Zero1_Start + (1 << kLevelSym_Zero1_NumBits);
const unsigned kLevelSym_Zero2_NumBits = 5;

const unsigned kLevelSym_Same_NumBits = 1;
const unsigned kLevelSym_Same_Start = 4;
 
const unsigned kNumDictBits_Min = 15;
const unsigned kNumDictBits_Max = 21;
const UInt32 kDictSize_Max = (UInt32)1 << kNumDictBits_Max;

const unsigned kNumLinearPosSlotBits = 17;
// const unsigned kNumPowerPosSlots = 38;
// const unsigned kNumPowerPosSlots = (kNumLinearPosSlotBits + 1) * 2; // non-including two first linear slots.
const unsigned kNumPowerPosSlots = (kNumLinearPosSlotBits + 2) * 2; // including two first linear slots.

}}

#endif

/* ---- CPP/7zip/Compress/LzxDecoder.h ---- */
// LzxDecoder.h

#ifndef ZIP7_INC_LZX_DECODER_H
#define ZIP7_INC_LZX_DECODER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzx {

const unsigned kAdditionalOutputBufSize = 32 * 2;

const unsigned kNumTableBits_Main = 11;
const unsigned kNumTableBits_Len = 8;

// if (kNumLenSymols_Big <= 256) we can  use NHuffman::CDecoder256
// if (kNumLenSymols_Big >  256) we must use NHuffman::CDecoder
// const unsigned kNumLenSymols_Big_Start = kNumLenSlots - 1 + kMatchMinLen;  // 8 - 1 + 2
const unsigned kNumLenSymols_Big_Start = 0;
// const unsigned kNumLenSymols_Big_Start = 0;
const unsigned kNumLenSymols_Big = kNumLenSymols_Big_Start + kNumLenSymbols;

#if 1
  // for smallest structure size:
  const unsigned kPosSlotOffset = 0;
#else
  // use virtual entries for mispredicted branches:
  const unsigned kPosSlotOffset = 256 / kNumLenSlots;
#endif

class CBitByteDecoder;

class CDecoder
{
public:
  UInt32 _pos;
  UInt32 _winSize;
  Byte *_win;

  bool _overDict;
  bool _isUncompressedBlock;
  bool _skipByte;
  bool _keepHistory;
  bool _keepHistoryForNext;
  bool _needAlloc;
  bool _wimMode;
  Byte _numDictBits;

  // unsigned _numAlignBits_PosSlots;
  // unsigned _numAlignBits;
  UInt32 _numAlignBits_Dist;
private:
  unsigned _numPosLenSlots;
  UInt32 _unpackBlockSize;

  UInt32 _writePos;

  UInt32 _x86_translationSize;
  UInt32 _x86_processedSize;
  Byte *_x86_buf;

  Byte *_unpackedData;
public:
  Byte  _extra[kPosSlotOffset + kNumPosSlots];
  UInt32 _reps[kPosSlotOffset + kNumPosSlots];

  NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize, kNumTableBits_Main> _mainDecoder;
  NHuffman::CDecoder256<kNumHuffmanBits, kNumLenSymols_Big, kNumTableBits_Len> _lenDecoder;
  NHuffman::CDecoder7b<kAlignTableSize> _alignDecoder;
private:
  Byte _mainLevels[kMainTableSize];
  Byte _lenLevels[kNumLenSymols_Big];

  HRESULT Flush() throw();
  bool ReadTables(CBitByteDecoder &_bitStream) throw();

  HRESULT CodeSpec(const Byte *inData, size_t inSize, UInt32 outSize) throw();
  HRESULT SetParams2(unsigned numDictBits) throw();
public:
  CDecoder() throw();
  ~CDecoder() throw();
  
  void Set_WimMode(bool wimMode) { _wimMode = wimMode; }
  void Set_KeepHistory(bool keepHistory) { _keepHistory = keepHistory; }
  void Set_KeepHistoryForNext(bool keepHistoryForNext) { _keepHistoryForNext = keepHistoryForNext; }

  HRESULT Set_ExternalWindow_DictBits(Byte *win, unsigned numDictBits)
  {
    _needAlloc = false;
    _win = win;
    _winSize = (UInt32)1 << numDictBits;
    return SetParams2(numDictBits);
  }
  HRESULT Set_DictBits_and_Alloc(unsigned numDictBits) throw();

  HRESULT Code_WithExceedReadWrite(const Byte *inData, size_t inSize, UInt32 outSize) throw();
  
  bool WasBlockFinished()     const { return _unpackBlockSize == 0; }
  const Byte *GetUnpackData() const { return _unpackedData; }
  UInt32 GetUnpackSize()      const { return _pos - _writePos; }
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

/* ---- CPP/7zip/Compress/PpmdDecoder.h ---- */
// PpmdDecoder.h

#ifndef ZIP7_INC_COMPRESS_PPMD_DECODER_H
#define ZIP7_INC_COMPRESS_PPMD_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NPpmd {

class CDecoder Z7_final:
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public ICompressSetFinishMode,
  public ICompressGetInStreamProcessedSize,
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
 #ifndef Z7_NO_READ_FROM_CODER
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
 #ifndef Z7_NO_READ_FROM_CODER
  Z7_IFACE_COM7_IMP(ICompressSetOutStreamSize)
  Z7_IFACE_COM7_IMP(ICompressSetInStream)
  Z7_IFACE_COM7_IMP(ISequentialInStream)
 #else
  Z7_COM7F_IMF(SetOutStreamSize(const UInt64 *outSize));
 #endif

  Byte *_outBuf;
  CByteInBufWrap _inStream;
  CPpmd7 _ppmd;

  Byte _order;
  bool  FinishStream;
  bool _outSizeDefined;
  HRESULT _res;
  int _status;
  UInt64 _outSize;
  UInt64 _processedSize;

  HRESULT CodeSpec(Byte *memStream, UInt32 size);

public:

 #ifndef Z7_NO_READ_FROM_CODER
  CMyComPtr<ISequentialInStream> InSeqStream;
 #endif

  CDecoder():
      _outBuf(NULL),
      FinishStream(false),
      _outSizeDefined(false)
  {
    Ppmd7_Construct(&_ppmd);
    _ppmd.rc.dec.Stream = &_inStream.vt;
  }

  ~CDecoder();
};

}}

#endif

/* ---- CPP/7zip/Compress/PpmdEncoder.h ---- */
// PpmdEncoder.h

#ifndef ZIP7_INC_COMPRESS_PPMD_ENCODER_H
#define ZIP7_INC_COMPRESS_PPMD_ENCODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NPpmd {

struct CEncProps
{
  UInt32 MemSize;
  UInt32 ReduceSize;
  int Order;
  
  CEncProps()
  {
    MemSize = (UInt32)(Int32)-1;
    ReduceSize = (UInt32)(Int32)-1;
    Order = -1;
  }
  void Normalize(int level);
};

Z7_CLASS_IMP_COM_3(
  CEncoder
  , ICompressCoder
  , ICompressSetCoderProperties
  , ICompressWriteCoderProperties
)
  Byte *_inBuf;
  CByteOutBufWrap _outStream;
  CPpmd7 _ppmd;
  CEncProps _props;
public:
  CEncoder();
  ~CEncoder();
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

/* ---- CPP/7zip/Compress/QuantumDecoder.h ---- */
// QuantumDecoder.h

#ifndef ZIP7_INC_COMPRESS_QUANTUM_DECODER_H
#define ZIP7_INC_COMPRESS_QUANTUM_DECODER_H

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NQuantum {

const unsigned kNumLitSelectorBits = 2;
const unsigned kNumLitSelectors = 1 << kNumLitSelectorBits;
const unsigned kNumLitSymbols = 1 << (8 - kNumLitSelectorBits);
const unsigned kNumMatchSelectors = 3;
const unsigned kNumSelectors = kNumLitSelectors + kNumMatchSelectors;
const unsigned kNumSymbolsMax = kNumLitSymbols; // 64

class CRangeDecoder;

class CModelDecoder
{
  unsigned NumItems;
  unsigned ReorderCount;
  Byte Vals[kNumSymbolsMax];
  UInt16 Freqs[kNumSymbolsMax + 1];
public:
  Byte _pad[64 - 10]; // for structure size alignment

  void Init(unsigned numItems, unsigned startVal);
  unsigned Decode(CRangeDecoder *rc);
};


class CDecoder
{
  UInt32 _winSize;
  UInt32 _winPos;
  UInt32 _winSize_allocated;
  bool _overWin;
  Byte *_win;
  unsigned _numDictBits;

  CModelDecoder m_Selector;
  CModelDecoder m_Literals[kNumLitSelectors];
  CModelDecoder m_PosSlot[kNumMatchSelectors];
  CModelDecoder m_LenSlot;

  void Init();
  HRESULT CodeSpec(const Byte *inData, size_t inSize, UInt32 outSize);
public:
  HRESULT Code(const Byte *inData, size_t inSize, UInt32 outSize, bool keepHistory);
  HRESULT SetParams(unsigned numDictBits);

  CDecoder(): _win(NULL), _numDictBits(0) {}
  const Byte * GetDataPtr() const { return _win + _winPos; }
};

}}

#endif

/* ---- CPP/7zip/Compress/Rar1Decoder.h ---- */
// Rar1Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

#ifndef ZIP7_INC_COMPRESS_RAR1_DECODER_H
#define ZIP7_INC_COMPRESS_RAR1_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NRar1 {

const unsigned kNumRepDists = 4;

Z7_CLASS_IMP_COM_2(
  CDecoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
)
  bool _isSolid;
  bool _solidAllowed;
  bool StMode;

  CLzOutWindow m_OutWindowStream;
  NBitm::CDecoder<CInBuffer> m_InBitStream;

  UInt64 m_UnpackSize;

  UInt32 LastDist;
  UInt32 LastLength;

  UInt32 m_RepDistPtr;
  UInt32 m_RepDists[kNumRepDists];

  int FlagsCnt;
  UInt32 FlagBuf, AvrPlc, AvrPlcB, AvrLn1, AvrLn2, AvrLn3;
  unsigned Buf60, NumHuf, LCount;
  UInt32 Nhfb, Nlzb, MaxDist3;

  UInt32 ChSet[256], ChSetA[256], ChSetB[256], ChSetC[256];
  UInt32 Place[256], PlaceA[256], PlaceB[256], PlaceC[256];
  UInt32 NToPl[256], NToPlB[256], NToPlC[256];

  UInt32 ReadBits(unsigned numBits);
  HRESULT CopyBlock(UInt32 distance, UInt32 len);
  UInt32 DecodeNum(const Byte *numTab);
  HRESULT ShortLZ();
  HRESULT LongLZ();
  HRESULT HuffDecode();
  void GetFlagsBuf();
  void CorrHuff(UInt32 *CharSet, UInt32 *NumToPlace);
  void OldUnpWriteBuf();
  
  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

public:
  CDecoder();
};

}}

#endif

/* ---- CPP/7zip/Compress/Rar2Decoder.h ---- */
// Rar2Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

#ifndef ZIP7_INC_COMPRESS_RAR2_DECODER_H
#define ZIP7_INC_COMPRESS_RAR2_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NRar2 {

const unsigned kNumReps = 4;
const unsigned kDistTableSize = 48;
const unsigned kNumLen2Symbols = 8;
const unsigned kLenTableSize = 28;
const unsigned kMainTableSize = 256 + 2 + kNumReps + kNumLen2Symbols + kLenTableSize;
const unsigned kHeapTablesSizesSum = kMainTableSize + kDistTableSize + kLenTableSize;
const unsigned k_MM_TableSize = 256 + 1;
const unsigned k_MM_NumChanelsMax = 4;
const unsigned k_MM_TablesSizesSum = k_MM_TableSize * k_MM_NumChanelsMax;
const unsigned kMaxTableSize = k_MM_TablesSizesSum;

namespace NMultimedia {

struct CFilter
{
  int K1,K2,K3,K4,K5;
  int D1,D2,D3,D4;
  int LastDelta;
  UInt32 Dif[11];
  UInt32 ByteCount;
  int LastChar;

  void Init() { memset(this, 0, sizeof(*this)); }
  Byte Decode(int &channelDelta, Byte delta);
};

struct CFilter2
{
  CFilter  m_Filters[k_MM_NumChanelsMax];
  int m_ChannelDelta;
  unsigned CurrentChannel;

  void Init() { memset(this, 0, sizeof(*this)); }
  Byte Decode(Byte delta)
  {
    return m_Filters[CurrentChannel].Decode(m_ChannelDelta, delta);
  }
};

}

typedef NBitm::CDecoder<CInBuffer> CBitDecoder;

const unsigned kNumHufBits = 15;

Z7_CLASS_IMP_NOQIB_2(
  CDecoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
)
  bool _isSolid;
  bool _solidAllowed;
  bool m_TablesOK;
  bool m_AudioMode;

  CLzOutWindow m_OutWindowStream;
  CBitDecoder m_InBitStream;

  UInt32 m_RepDistPtr;
  UInt32 m_RepDists[kNumReps];
  UInt32 m_LastLength;
  unsigned m_NumChannels;

  NHuffman::CDecoder<kNumHufBits, kMainTableSize, 9> m_MainDecoder;
  NHuffman::CDecoder256<kNumHufBits, kDistTableSize, 7> m_DistDecoder;
  NHuffman::CDecoder256<kNumHufBits, kLenTableSize, 7> m_LenDecoder;
  NHuffman::CDecoder<kNumHufBits, k_MM_TableSize, 9> m_MMDecoders[k_MM_NumChanelsMax];

  UInt64 m_PackSize;
  
  NMultimedia::CFilter2 m_MmFilter;
  Byte m_LastLevels[kMaxTableSize];

  void InitStructures();
  UInt32 ReadBits(unsigned numBits);
  bool ReadTables();
  bool ReadLastTables();

  bool DecodeMm(UInt32 pos);
  bool DecodeLz(Int32 pos);

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

public:
  CDecoder();
};

}}

#endif

/* ---- CPP/7zip/Compress/Rar3Vm.h ---- */
// Rar3Vm.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

#ifndef ZIP7_INC_COMPRESS_RAR3_VM_H
#define ZIP7_INC_COMPRESS_RAR3_VM_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#define Z7_RARVM_STANDARD_FILTERS
// #define Z7_RARVM_VM_ENABLE

namespace NCompress {
namespace NRar3 {

class CMemBitDecoder
{
  const Byte *_data;
  UInt32 _bitSize;
  UInt32 _bitPos;
public:
  void Init(const Byte *data, UInt32 byteSize)
  {
    _data = data;
    _bitSize = (byteSize << 3);
    _bitPos = 0;
  }
  UInt32 ReadBits(unsigned numBits);
  UInt32 ReadBit();
  bool Avail() const { return (_bitPos < _bitSize); }

  UInt32 ReadEncodedUInt32();
};

namespace NVm {

inline UInt32 GetValue32(const void *addr) { return GetUi32(addr); }
inline void SetValue32(void *addr, UInt32 value) { SetUi32(addr, value) }

const unsigned kNumRegBits = 3;
const UInt32 kNumRegs = 1 << kNumRegBits;
const UInt32 kNumGpRegs = kNumRegs - 1;

const UInt32 kSpaceSize = 0x40000;
const UInt32 kSpaceMask = kSpaceSize - 1;
const UInt32 kGlobalOffset = 0x3C000;
const UInt32 kGlobalSize = 0x2000;
const UInt32 kFixedGlobalSize = 64;

namespace NGlobalOffset
{
  const UInt32 kBlockSize = 0x1C;
  const UInt32 kBlockPos  = 0x20;
  const UInt32 kExecCount = 0x2C;
  const UInt32 kGlobalMemOutSize = 0x30;
}


#ifdef Z7_RARVM_VM_ENABLE

enum ECommand
{
  CMD_MOV,  CMD_CMP,  CMD_ADD,  CMD_SUB,  CMD_JZ,   CMD_JNZ,  CMD_INC,  CMD_DEC,
  CMD_JMP,  CMD_XOR,  CMD_AND,  CMD_OR,   CMD_TEST, CMD_JS,   CMD_JNS,  CMD_JB,
  CMD_JBE,  CMD_JA,   CMD_JAE,  CMD_PUSH, CMD_POP,  CMD_CALL, CMD_RET,  CMD_NOT,
  CMD_SHL,  CMD_SHR,  CMD_SAR,  CMD_NEG,  CMD_PUSHA,CMD_POPA, CMD_PUSHF,CMD_POPF,
  CMD_MOVZX,CMD_MOVSX,CMD_XCHG, CMD_MUL,  CMD_DIV,  CMD_ADC,  CMD_SBB,  CMD_PRINT,

  CMD_MOVB, CMD_CMPB, CMD_ADDB, CMD_SUBB, CMD_INCB, CMD_DECB,
  CMD_XORB, CMD_ANDB, CMD_ORB,  CMD_TESTB,CMD_NEGB,
  CMD_SHLB, CMD_SHRB, CMD_SARB, CMD_MULB
};

enum EOpType {OP_TYPE_REG, OP_TYPE_INT, OP_TYPE_REGMEM, OP_TYPE_NONE};

// Addr in COperand object can link (point) to CVm object!!!

struct COperand
{
  EOpType Type;
  UInt32 Data;
  UInt32 Base;
  COperand(): Type(OP_TYPE_NONE), Data(0), Base(0) {}
};

struct CCommand
{
  ECommand OpCode;
  bool ByteMode;
  COperand Op1, Op2;
};

#endif


struct CBlockRef
{
  UInt32 Offset;
  UInt32 Size;
};


class CProgram
{
  #ifdef Z7_RARVM_VM_ENABLE
  void ReadProgram(const Byte *code, UInt32 codeSize);
public:
  CRecordVector<CCommand> Commands;
  #endif
  
public:
  #ifdef Z7_RARVM_STANDARD_FILTERS
  int StandardFilterIndex;
  #endif
  
  bool IsSupported;
  CRecordVector<Byte> StaticData;

  bool PrepareProgram(const Byte *code, UInt32 codeSize);
};


struct CProgramInitState
{
  UInt32 InitR[kNumGpRegs];
  CRecordVector<Byte> GlobalData;

  void AllocateEmptyFixedGlobal()
  {
    GlobalData.ClearAndSetSize(NVm::kFixedGlobalSize);
    memset(&GlobalData[0], 0, NVm::kFixedGlobalSize);
  }
};


class CVm
{
  static UInt32 GetValue(bool byteMode, const void *addr)
  {
    if (byteMode)
      return(*(const Byte *)addr);
    else
      return GetUi32(addr);
  }

  static void SetValue(bool byteMode, void *addr, UInt32 value)
  {
    if (byteMode)
      *(Byte *)addr = (Byte)value;
    else
      SetUi32(addr, value)
  }

  UInt32 GetFixedGlobalValue32(UInt32 globalOffset) { return GetValue(false, &Mem[kGlobalOffset + globalOffset]); }

  void SetBlockSize(UInt32 v) { SetValue(&Mem[kGlobalOffset + NGlobalOffset::kBlockSize], v); }
  void SetBlockPos(UInt32 v) { SetValue(&Mem[kGlobalOffset + NGlobalOffset::kBlockPos], v); }
public:
  static void SetValue(void *addr, UInt32 value) { SetValue(false, addr, value); }

private:

  #ifdef Z7_RARVM_VM_ENABLE
  UInt32 GetOperand32(const COperand *op) const;
  void SetOperand32(const COperand *op, UInt32 val);
  Byte GetOperand8(const COperand *op) const;
  void SetOperand8(const COperand *op, Byte val);
  UInt32 GetOperand(bool byteMode, const COperand *op) const;
  void SetOperand(bool byteMode, const COperand *op, UInt32 val);
  bool ExecuteCode(const CProgram *prg);
  #endif
  
  #ifdef Z7_RARVM_STANDARD_FILTERS
  bool ExecuteStandardFilter(unsigned filterIndex);
  #endif
  
  Byte *Mem;
  UInt32 R[kNumRegs + 1]; // R[kNumRegs] = 0 always (speed optimization)
  UInt32 Flags;

public:
  CVm();
  ~CVm();
  bool Create();
  void SetMemory(UInt32 pos, const Byte *data, UInt32 dataSize);
  bool Execute(CProgram *prg, const CProgramInitState *initState,
      CBlockRef &outBlockRef, CRecordVector<Byte> &outGlobalData);
  const Byte *GetDataPointer(UInt32 offset) const { return Mem + offset; }
};

#endif

}}}

/* ---- CPP/7zip/Compress/Rar3Decoder.h ---- */
// Rar3Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

/* This code uses Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

#ifndef ZIP7_INC_COMPRESS_RAR3_DECODER_H
#define ZIP7_INC_COMPRESS_RAR3_DECODER_H

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NRar3 {

const unsigned kNumHuffmanBits = 15;

const UInt32 kWindowSize = 1 << 22;
const UInt32 kWindowMask = kWindowSize - 1;

const unsigned kNumReps = 4;
const unsigned kNumLen2Symbols = 8;
const unsigned kLenTableSize = 28;
const unsigned kMainTableSize = 256 + 3 + kNumReps + kNumLen2Symbols + kLenTableSize;
const unsigned kDistTableSize = 60;

const unsigned kNumAlignBits = 4;
const unsigned kAlignTableSize = (1 << kNumAlignBits) + 1;

const unsigned kTablesSizesSum = kMainTableSize + kDistTableSize + kAlignTableSize + kLenTableSize;

class CBitDecoder
{
  UInt32 _value;
  unsigned _bitPos;
public:
  CInBuffer Stream;

  bool Create(UInt32 bufSize) { return Stream.Create(bufSize); }
  void SetStream(ISequentialInStream *inStream) { Stream.SetStream(inStream);}

  void Init()
  {
    Stream.Init();
    _bitPos = 0;
    _value = 0;
  }
  
  bool ExtraBitsWereRead() const
  {
    return (Stream.NumExtraBytes > 4 || _bitPos < (Stream.NumExtraBytes << 3));
  }

  UInt64 GetProcessedSize() const { return Stream.GetProcessedSize() - (_bitPos >> 3); }

  void AlignToByte()
  {
    _bitPos &= ~(unsigned)7;
    _value = _value & ((1 << _bitPos) - 1);
  }
  
  Z7_FORCE_INLINE
  UInt32 GetValue(unsigned numBits)
  {
    if (_bitPos < numBits)
    {
      _bitPos += 8;
      _value = (_value << 8) | Stream.ReadByte();
      if (_bitPos < numBits)
      {
        _bitPos += 8;
        _value = (_value << 8) | Stream.ReadByte();
      }
    }
    return _value >> (_bitPos - numBits);
  }

  Z7_FORCE_INLINE
  UInt32 GetValue_InHigh32bits()
  {
    return GetValue(kNumHuffmanBits) << (32 - kNumHuffmanBits);
  }

  
  Z7_FORCE_INLINE
  void MovePos(unsigned numBits)
  {
    _bitPos -= numBits;
    _value = _value & ((1 << _bitPos) - 1);
  }
  
  UInt32 ReadBits(unsigned numBits)
  {
    const UInt32 res = GetValue(numBits);
    MovePos(numBits);
    return res;
  }

  UInt32 ReadBits_upto8(unsigned numBits)
  {
    if (_bitPos < numBits)
    {
      _bitPos += 8;
      _value = (_value << 8) | Stream.ReadByte();
    }
    _bitPos -= numBits;
    const UInt32 res = _value >> _bitPos;
    _value = _value & ((1 << _bitPos) - 1);
    return res;
  }

  Byte ReadByteFromAligned()
  {
    if (_bitPos == 0)
      return Stream.ReadByte();
    const unsigned bitsPos = _bitPos - 8;
    const Byte b = (Byte)(_value >> bitsPos);
    _value = _value & ((1 << bitsPos) - 1);
    _bitPos = bitsPos;
    return b;
  }
};


struct CByteIn
{
  IByteIn IByteIn_obj;
  CBitDecoder BitDecoder;
};


struct CFilter: public NVm::CProgram
{
  CRecordVector<Byte> GlobalData;
  UInt32 BlockStart;
  UInt32 BlockSize;
  UInt32 ExecCount;
  
  CFilter(): BlockStart(0), BlockSize(0), ExecCount(0) {}
};

struct CTempFilter: public NVm::CProgramInitState
{
  UInt32 BlockStart;
  UInt32 BlockSize;
  bool NextWindow;
  
  UInt32 FilterIndex;

  CTempFilter()
  {
    // all filters must contain at least FixedGlobal block
    AllocateEmptyFixedGlobal();
  }
};


Z7_CLASS_IMP_NOQIB_2(
  CDecoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
)
  bool _isSolid;
  bool _solidAllowed;
  // bool _errorMode;

  bool _lzMode;
  bool _unsupportedFilter;

  CByteIn m_InBitStream;
  Byte *_window;
  UInt32 _winPos;
  UInt32 _wrPtr;
  UInt64 _lzSize;
  UInt64 _unpackSize;
  UInt64 _writtenFileSize; // if it's > _unpackSize, then _unpackSize only written
  ISequentialOutStream *_outStream;

  NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize, 9> m_MainDecoder;
  UInt32 kDistStart[kDistTableSize];
  NHuffman::CDecoder256<kNumHuffmanBits, kDistTableSize, 7> m_DistDecoder;
  NHuffman::CDecoder256<kNumHuffmanBits, kAlignTableSize, 6> m_AlignDecoder;
  NHuffman::CDecoder256<kNumHuffmanBits, kLenTableSize, 7> m_LenDecoder;

  UInt32 _reps[kNumReps];
  UInt32 _lastLength;
  
  Byte m_LastLevels[kTablesSizesSum];

  Byte *_vmData;
  Byte *_vmCode;
  NVm::CVm _vm;
  CRecordVector<CFilter *> _filters;
  CRecordVector<CTempFilter *>  _tempFilters;
  unsigned _numEmptyTempFilters;
  UInt32 _lastFilter;

  UInt32 PrevAlignBits;
  UInt32 PrevAlignCount;

  bool TablesRead;
  bool TablesOK;
  bool PpmError;

  int PpmEscChar;
  CPpmd7 _ppmd;
  
  HRESULT WriteDataToStream(const Byte *data, UInt32 size);
  HRESULT WriteData(const Byte *data, UInt32 size);
  HRESULT WriteArea(UInt32 startPtr, UInt32 endPtr);
  void ExecuteFilter(unsigned tempFilterIndex, NVm::CBlockRef &outBlockRef);
  HRESULT WriteBuf();

  void InitFilters();
  bool AddVmCode(UInt32 firstByte, UInt32 codeSize);
  bool ReadVmCodeLZ();
  bool ReadVmCodePPM();
  
  UInt32 ReadBits(unsigned numBits);

  HRESULT InitPPM();
  // int DecodePpmSymbol();
  HRESULT DecodePPM(Int32 num, bool &keepDecompressing);

  HRESULT ReadTables(bool &keepDecompressing);
  HRESULT ReadEndOfBlock(bool &keepDecompressing);
  HRESULT DecodeLZ(bool &keepDecompressing);
  HRESULT CodeReal(ICompressProgressInfo *progress);
  
  bool InputEofError() const { return m_InBitStream.BitDecoder.ExtraBitsWereRead(); }
  bool InputEofError_Fast() const { return (m_InBitStream.BitDecoder.Stream.NumExtraBytes > 2); }

  void CopyBlock(UInt32 dist, UInt32 len)
  {
    _lzSize += len;
    UInt32 pos = (_winPos - dist - 1) & kWindowMask;
    Byte *window = _window;
    UInt32 winPos = _winPos;
    if (kWindowSize - winPos > len && kWindowSize - pos > len)
    {
      const Byte *src = window + pos;
      Byte *dest = window + winPos;
      _winPos += len;
      do
        *dest++ = *src++;
      while (--len != 0);
      return;
    }
    do
    {
      window[winPos] = window[pos];
      winPos = (winPos + 1) & kWindowMask;
      pos = (pos + 1) & kWindowMask;
    }
    while (--len != 0);
    _winPos = winPos;
  }
  
  void PutByte(Byte b)
  {
    const UInt32 wp = _winPos;
    _window[wp] = b;
    _winPos = (wp + 1) & kWindowMask;
    _lzSize++;
  }

public:
  CDecoder();
  ~CDecoder();
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

/* ---- CPP/7zip/Compress/Rar5Decoder.h ---- */
// Rar5Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

#ifndef ZIP7_INC_COMPRESS_RAR5_DECODER_H
#define ZIP7_INC_COMPRESS_RAR5_DECODER_H

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NRar5 {

class CBitDecoder;

struct CFilter
{
  Byte Type;
  Byte Channels;
  UInt32 Size;
  UInt64 Start;
};


const unsigned kNumReps = 4;
const unsigned kLenTableSize = 11 * 4;
const unsigned kMainTableSize = 256 + 1 + 1 + kNumReps + kLenTableSize;
const unsigned kExtraDistSymbols_v7 = 16;
const unsigned kDistTableSize_v6 = 64;
const unsigned kDistTableSize_MAX = 64 + kExtraDistSymbols_v7;
const unsigned kNumAlignBits = 4;
const unsigned kAlignTableSize = 1 << kNumAlignBits;

const unsigned kNumHufBits = 15;

const unsigned k_NumHufTableBits_Main = 10;
const unsigned k_NumHufTableBits_Dist = 7;
const unsigned k_NumHufTableBits_Len = 7;
const unsigned k_NumHufTableBits_Align = 6;

const unsigned DICT_SIZE_BITS_MAX = 40;

Z7_CLASS_IMP_NOQIB_2(
  CDecoder
  , ICompressCoder
  , ICompressSetDecoderProperties2
)
  bool _useAlignBits;
  bool _isLastBlock;
  bool _unpackSize_Defined;
  // bool _packSize_Defined;
  
  bool _unsupportedFilter;
  Byte _lzError;
  bool _writeError;

  bool _isSolid;
  // bool _solidAllowed;
  bool _is_v7;
  bool _tableWasFilled;
  bool _wasInit;

  Byte _exitType;

  // Byte _dictSizeLog;
  size_t _dictSize;
  Byte *_window;
  size_t _winPos;
  size_t _winSize;
  size_t _dictSize_forCheck;
  size_t _limit;
  const Byte *_buf_Res;
  UInt64 _lzSize;
  size_t _reps[kNumReps];
  unsigned _bitPos_Res;
  UInt32 _lastLen;

  // unsigned _numCorrectDistSymbols;
  unsigned _numUnusedFilters;
  unsigned _numFilters;

  UInt64 _lzWritten;
  UInt64 _lzFileStart;
  UInt64 _unpackSize;
  // UInt64 _packSize;
  UInt64 _lzEnd;
  UInt64 _writtenFileSize;
  UInt64 _filterEnd;
  UInt64 _progress_Pack;
  UInt64 _progress_Unpack;
  CAlignedBuffer _filterSrc;
  CAlignedBuffer _filterDst;

  CFilter *_filters;
  size_t _winSize_Allocated;
  ISequentialInStream *_inStream;
  ISequentialOutStream *_outStream;
  ICompressProgressInfo *_progress;
  Byte *_inputBuf;

  NHuffman::CDecoder<kNumHufBits, kMainTableSize,  k_NumHufTableBits_Main>  m_MainDecoder;
  NHuffman::CDecoder256<kNumHufBits, kDistTableSize_MAX,  k_NumHufTableBits_Dist>  m_DistDecoder;
  NHuffman::CDecoder256<kNumHufBits, kAlignTableSize,     k_NumHufTableBits_Align> m_AlignDecoder;
  NHuffman::CDecoder256<kNumHufBits, kLenTableSize,       k_NumHufTableBits_Len>   m_LenDecoder;
  Byte m_LenPlusTable[DICT_SIZE_BITS_MAX];

  void InitFilters()
  {
    _numUnusedFilters = 0;
    _numFilters = 0;
  }
  void DeleteUnusedFilters();
  HRESULT WriteData(const Byte *data, size_t size);
  HRESULT ExecuteFilter(const CFilter &f);
  HRESULT WriteBuf();
  HRESULT AddFilter(CBitDecoder &_bitStream);
  HRESULT ReadTables(CBitDecoder &_bitStream);
  HRESULT DecodeLZ2(const CBitDecoder &_bitStream) throw();
  HRESULT DecodeLZ();
  HRESULT CodeReal();
public:
  CDecoder();
  ~CDecoder();
};

}}

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

/* ---- CPP/7zip/Compress/XpressDecoder.h ---- */
// XpressDecoder.h

#ifndef ZIP7_INC_XPRESS_DECODER_H
#define ZIP7_INC_XPRESS_DECODER_H

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NXpress {

// (out) buffer size must be larger than (outSize) for the following value:
const unsigned kAdditionalOutputBufSize = 32;

HRESULT Decode_WithExceedWrite(const Byte *in, size_t inSize, Byte *out, size_t outSize);

}}

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

/* ================ unit bodies ================ */

/* ================ unit: CPP/7zip/Compress/BZip2Crc.cpp ================ */
// BZip2Crc.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

MY_ALIGN(64)
UInt32 CBZip2Crc::Table[256];

static const UInt32 kBZip2CrcPoly = 0x04c11db7;  /* AUTODIN II, Ethernet, & FDDI */

void CBZip2Crc::InitTable()
{
  for (UInt32 i = 0; i < 256; i++)
  {
    UInt32 r = i << 24;
    for (unsigned j = 0; j < 8; j++)
      r = (r << 1) ^ (kBZip2CrcPoly & ((UInt32)0 - (r >> 31)));
    Table[i] = r;
  }
}

static
class CBZip2CrcTableInit
{
public:
  CBZip2CrcTableInit() { CBZip2Crc::InitTable(); }
} g_BZip2CrcTableInit;

/* ================ unit: CPP/7zip/Compress/BZip2Decoder.cpp ================ */
// BZip2Decoder.cpp

// amalgamation: header emitted in prologue

// #include "CopyCoder.h"

/*
#include <stdio.h>
// amalgamation: header emitted in prologue
*/
#define TICKS_START
#define TICKS_UPDATE(n)


/*
#define PRIN(s) printf(s "\n"); fflush(stdout);
#define PRIN_VAL(s, val) printf(s " = %u \n", val); fflush(stdout);
*/

#define PRIN(s)
#define PRIN_VAL(s, val)


// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue


namespace NCompress {
namespace NBZip2 {

// #undef NO_INLINE
#define NO_INLINE Z7_NO_INLINE

#define BZIP2_BYTE_MODE


static const UInt32 kInBufSize = (UInt32)1 << 17;
static const size_t kOutBufSize = (size_t)1 << 20;

static const UInt32 kProgressStep = (UInt32)1 << 16;

MY_ALIGN(64)
static const UInt16 kRandNums[512] = {
   619, 720, 127, 481, 931, 816, 813, 233, 566, 247,
   985, 724, 205, 454, 863, 491, 741, 242, 949, 214,
   733, 859, 335, 708, 621, 574, 73, 654, 730, 472,
   419, 436, 278, 496, 867, 210, 399, 680, 480, 51,
   878, 465, 811, 169, 869, 675, 611, 697, 867, 561,
   862, 687, 507, 283, 482, 129, 807, 591, 733, 623,
   150, 238, 59, 379, 684, 877, 625, 169, 643, 105,
   170, 607, 520, 932, 727, 476, 693, 425, 174, 647,
   73, 122, 335, 530, 442, 853, 695, 249, 445, 515,
   909, 545, 703, 919, 874, 474, 882, 500, 594, 612,
   641, 801, 220, 162, 819, 984, 589, 513, 495, 799,
   161, 604, 958, 533, 221, 400, 386, 867, 600, 782,
   382, 596, 414, 171, 516, 375, 682, 485, 911, 276,
   98, 553, 163, 354, 666, 933, 424, 341, 533, 870,
   227, 730, 475, 186, 263, 647, 537, 686, 600, 224,
   469, 68, 770, 919, 190, 373, 294, 822, 808, 206,
   184, 943, 795, 384, 383, 461, 404, 758, 839, 887,
   715, 67, 618, 276, 204, 918, 873, 777, 604, 560,
   951, 160, 578, 722, 79, 804, 96, 409, 713, 940,
   652, 934, 970, 447, 318, 353, 859, 672, 112, 785,
   645, 863, 803, 350, 139, 93, 354, 99, 820, 908,
   609, 772, 154, 274, 580, 184, 79, 626, 630, 742,
   653, 282, 762, 623, 680, 81, 927, 626, 789, 125,
   411, 521, 938, 300, 821, 78, 343, 175, 128, 250,
   170, 774, 972, 275, 999, 639, 495, 78, 352, 126,
   857, 956, 358, 619, 580, 124, 737, 594, 701, 612,
   669, 112, 134, 694, 363, 992, 809, 743, 168, 974,
   944, 375, 748, 52, 600, 747, 642, 182, 862, 81,
   344, 805, 988, 739, 511, 655, 814, 334, 249, 515,
   897, 955, 664, 981, 649, 113, 974, 459, 893, 228,
   433, 837, 553, 268, 926, 240, 102, 654, 459, 51,
   686, 754, 806, 760, 493, 403, 415, 394, 687, 700,
   946, 670, 656, 610, 738, 392, 760, 799, 887, 653,
   978, 321, 576, 617, 626, 502, 894, 679, 243, 440,
   680, 879, 194, 572, 640, 724, 926, 56, 204, 700,
   707, 151, 457, 449, 797, 195, 791, 558, 945, 679,
   297, 59, 87, 824, 713, 663, 412, 693, 342, 606,
   134, 108, 571, 364, 631, 212, 174, 643, 304, 329,
   343, 97, 430, 751, 497, 314, 983, 374, 822, 928,
   140, 206, 73, 263, 980, 736, 876, 478, 430, 305,
   170, 514, 364, 692, 829, 82, 855, 953, 676, 246,
   369, 970, 294, 750, 807, 827, 150, 790, 288, 923,
   804, 378, 215, 828, 592, 281, 565, 555, 710, 82,
   896, 831, 547, 261, 524, 462, 293, 465, 502, 56,
   661, 821, 976, 991, 658, 869, 905, 758, 745, 193,
   768, 550, 608, 933, 378, 286, 215, 979, 792, 961,
   61, 688, 793, 644, 986, 403, 106, 366, 905, 644,
   372, 567, 466, 434, 645, 210, 389, 550, 919, 135,
   780, 773, 635, 389, 707, 100, 626, 958, 165, 504,
   920, 176, 193, 713, 857, 265, 203, 50, 668, 108,
   645, 990, 626, 197, 510, 357, 358, 850, 858, 364,
   936, 638
};



enum EState
{
  STATE_STREAM_SIGNATURE,
  STATE_BLOCK_SIGNATURE,

  STATE_BLOCK_START,
  STATE_ORIG_BITS,
  STATE_IN_USE,
  STATE_IN_USE2,
  STATE_NUM_TABLES,
  STATE_NUM_SELECTORS,
  STATE_SELECTORS,
  STATE_LEVELS,
  
  STATE_BLOCK_SYMBOLS,

  STATE_STREAM_FINISHED
};


#define UPDATE_VAL_2(val, num_bits) { \
  val |= (UInt32)(*_buf) << (24 - num_bits); \
  num_bits += 8; \
 _buf++; \
}

#define UPDATE_VAL  UPDATE_VAL_2(VAL, NUM_BITS)

#define READ_BITS(res, num) { \
  while (_numBits < num) { \
    if (_buf == _lim) return SZ_OK; \
    UPDATE_VAL_2(_value, _numBits) } \
  res = _value >> (32 - num); \
  _value <<= num; \
  _numBits -= num; \
}

#define READ_BITS_8(res, num) { \
  if (_numBits < num) { \
    if (_buf == _lim) return SZ_OK; \
    UPDATE_VAL_2(_value, _numBits) } \
  res = _value >> (32 - num); \
  _value <<= num; \
  _numBits -= num; \
}

#define READ_BIT(res) READ_BITS_8(res, 1)



#define VAL _value2
// #define NUM_BITS _numBits2
#define NUM_BITS _numBits
#define BLOCK_SIZE blockSize2
#define RUN_COUNTER runCounter2

#define LOAD_LOCAL \
    UInt32 VAL = this->_value; \
    /* unsigned NUM_BITS = this->_numBits; */ \
    UInt32 BLOCK_SIZE = this->blockSize; \
    UInt32 RUN_COUNTER = this->runCounter; \

#define SAVE_LOCAL \
    this->_value = VAL; \
    /* this->_numBits = NUM_BITS; */ \
    this->blockSize = BLOCK_SIZE; \
    this->runCounter = RUN_COUNTER; \



SRes CBitDecoder::ReadByte(int &b)
{
  b = -1;
  READ_BITS_8(b, 8)
  return SZ_OK;
}


NO_INLINE
SRes CBase::ReadStreamSignature2()
{
  for (;;)
  {
    unsigned b;
    READ_BITS_8(b, 8)

    if (   (state2 == 0 && b != kArSig0)
        || (state2 == 1 && b != kArSig1)
        || (state2 == 2 && b != kArSig2)
        || (state2 == 3 && (b <= kArSig3 || b > kArSig3 + kBlockSizeMultMax)))
      return SZ_ERROR_DATA;
    state2++;

    if (state2 == 4)
    {
      blockSizeMax = (UInt32)(b - kArSig3) * kBlockSizeStep;
      CombinedCrc.Init();
      state = STATE_BLOCK_SIGNATURE;
      state2 = 0;
      return SZ_OK;
    }
  }
}


bool IsEndSig(const Byte *p) throw()
{
  return
    p[0] == kFinSig0 &&
    p[1] == kFinSig1 &&
    p[2] == kFinSig2 &&
    p[3] == kFinSig3 &&
    p[4] == kFinSig4 &&
    p[5] == kFinSig5;
}

bool IsBlockSig(const Byte *p) throw()
{
  return
    p[0] == kBlockSig0 &&
    p[1] == kBlockSig1 &&
    p[2] == kBlockSig2 &&
    p[3] == kBlockSig3 &&
    p[4] == kBlockSig4 &&
    p[5] == kBlockSig5;
}


NO_INLINE
SRes CBase::ReadBlockSignature2()
{
  while (state2 < 10)
  {
    unsigned b;
    READ_BITS_8(b, 8)
    temp[state2] = (Byte)b;
    state2++;
  }

  crc = 0;
  for (unsigned i = 0; i < 4; i++)
  {
    crc <<= 8;
    crc |= temp[6 + i];
  }

  if (IsBlockSig(temp))
  {
    if (!IsBz)
      NumStreams++;
    NumBlocks++;
    IsBz = true;
    CombinedCrc.Update(crc);
    state = STATE_BLOCK_START;
    return SZ_OK;
  }
  
  if (!IsEndSig(temp))
    return SZ_ERROR_DATA;

  if (!IsBz)
    NumStreams++;
  IsBz = true;

  if (_value != 0)
    MinorError = true;

  AlignToByte();

  state = STATE_STREAM_FINISHED;
  if (crc != CombinedCrc.GetDigest())
  {
    StreamCrcError = true;
    return SZ_ERROR_DATA;
  }
  return SZ_OK;
}


NO_INLINE
SRes CBase::ReadBlock2()
{
  if (state != STATE_BLOCK_SYMBOLS) {
  PRIN("ReadBlock2")

  if (state == STATE_BLOCK_START)
  {
    if (Props.randMode)
    {
      READ_BIT(Props.randMode)
    }
    state = STATE_ORIG_BITS;
    // g_Tick = GetCpuTicks();
  }

  if (state == STATE_ORIG_BITS)
  {
    READ_BITS(Props.origPtr, kNumOrigBits)
    if (Props.origPtr >= blockSizeMax)
      return SZ_ERROR_DATA;
    state = STATE_IN_USE;
  }
  
  // why original code compares origPtr to (UInt32)(10 + blockSizeMax)) ?

  if (state == STATE_IN_USE)
  {
    READ_BITS(state2, 16)
    state = STATE_IN_USE2;
    state3 = 0;
    numInUse = 0;
    mtf.StartInit();
  }

  if (state == STATE_IN_USE2)
  {
    for (; state3 < 256; state3++)
      if (state2 & ((UInt32)0x8000 >> (state3 >> 4)))
      {
        unsigned b;
        READ_BIT(b)
        if (b)
          mtf.Add(numInUse++, (Byte)state3);
      }
    if (numInUse == 0)
      return SZ_ERROR_DATA;
    state = STATE_NUM_TABLES;
  }

  
  if (state == STATE_NUM_TABLES)
  {
    READ_BITS_8(numTables, kNumTablesBits)
    state = STATE_NUM_SELECTORS;
    if (numTables < kNumTablesMin || numTables > kNumTablesMax)
      return SZ_ERROR_DATA;
  }
  
  if (state == STATE_NUM_SELECTORS)
  {
    READ_BITS(numSelectors, kNumSelectorsBits)
    state = STATE_SELECTORS;
    state2 = 0x543210;
    state3 = 0;
    state4 = 0;
    // lbzip2 can write small number of additional selectors,
    // 20.01: we allow big number of selectors here like bzip2-1.0.8
    if (numSelectors == 0
      // || numSelectors > kNumSelectorsMax_Decoder
      )
      return SZ_ERROR_DATA;
  }

  if (state == STATE_SELECTORS)
  {
    const unsigned kMtfBits = 4;
    const UInt32 kMtfMask = (1 << kMtfBits) - 1;
    do
    {
      for (;;)
      {
        unsigned b;
        READ_BIT(b)
        if (!b)
          break;
        if (++state4 >= numTables)
          return SZ_ERROR_DATA;
      }
      const UInt32 tmp = (state2 >> (kMtfBits * state4)) & kMtfMask;
      const UInt32 mask = ((UInt32)1 << ((state4 + 1) * kMtfBits)) - 1;
      state4 = 0;
      state2 = ((state2 << kMtfBits) & mask) | (state2 & ~mask) | tmp;
      // 20.01: here we keep compatibility with bzip2-1.0.8 decoder:
      if (state3 < kNumSelectorsMax)
        selectors[state3] = (Byte)tmp;
    }
    while (++state3 < numSelectors);

    // we allowed additional dummy selector records filled above to support lbzip2's archives.
    // but we still don't allow to use these additional dummy selectors in the code bellow
    // bzip2 1.0.8 decoder also has similar restriction.

    if (numSelectors > kNumSelectorsMax)
      numSelectors = kNumSelectorsMax;

    state = STATE_LEVELS;
    state2 = 0;
    state3 = 0;
  }

  if (state == STATE_LEVELS)
  {
    do
    {
      if (state3 == 0)
      {
        READ_BITS_8(state3, kNumLevelsBits)
        state4 = 0;
        state5 = 0;
      }
      const unsigned alphaSize = numInUse + 2;
      for (; state4 < alphaSize; state4++)
      {
        for (;;)
        {
          if (state3 < 1 || state3 > kMaxHuffmanLen)
            return SZ_ERROR_DATA;
          
          if (state5 == 0)
          {
            unsigned b;
            READ_BIT(b)
            if (!b)
              break;
          }

          state5 = 1;
          unsigned b;
          READ_BIT(b)

          state5 = 0;
          state3++;
          state3 -= (b << 1);
        }
        lens[state4] = (Byte)state3;
        state5 = 0;
      }
      
      // 19.03: we use non-full Build() to support lbzip2 archives.
      // lbzip2 2.5 can produce dummy tree, where lens[i] = kMaxHuffmanLen
      for (unsigned i = state4; i < kMaxAlphaSize; i++)
        lens[i] = 0;
      if (!huffs[state2].Build(lens)) // k_BuildMode_Partial
        return SZ_ERROR_DATA;
      state3 = 0;
    }
    while (++state2 < numTables);

    {
      UInt32 *counters = this->Counters;
      for (unsigned i = 0; i < 256; i++)
        counters[i] = 0;
    }

    state = STATE_BLOCK_SYMBOLS;

    groupIndex = 0;
    groupSize = kGroupSize;
    runPower = 0;
    runCounter = 0;
    blockSize = 0;
  }
  
  if (state != STATE_BLOCK_SYMBOLS)
    return SZ_ERROR_DATA;

  // g_Ticks[3] += GetCpuTicks() - g_Tick;

  }

  {
    LOAD_LOCAL
    const CHuffmanDecoder *huf = &huffs[selectors[groupIndex]];

    for (;;)
    {
      if (groupSize == 0)
      {
        if (++groupIndex >= numSelectors)
          return SZ_ERROR_DATA;
        huf = &huffs[selectors[groupIndex]];
        groupSize = kGroupSize;
      }

      if (NUM_BITS < kMaxHuffmanLen && _buf != _lim) { UPDATE_VAL
      if (NUM_BITS < kMaxHuffmanLen && _buf != _lim) { UPDATE_VAL
      if (NUM_BITS < kMaxHuffmanLen && _buf != _lim) { UPDATE_VAL }}}

      unsigned sym;

      #define MOV_POS(bs, len) \
      { \
        if (NUM_BITS < len) \
        { \
          SAVE_LOCAL \
          return SZ_OK; \
        } \
        VAL <<= len; \
        NUM_BITS -= (unsigned)len; \
      }

      Z7_HUFF_DECODE_VAL_IN_HIGH32(sym, huf, kMaxHuffmanLen, kNumTableBits,
          VAL,
          Z7_HUFF_DECODE_ERROR_SYM_CHECK_YES,
          { return SZ_ERROR_DATA; },
          MOV_POS, {}, bs)

      groupSize--;

      if (sym < 2)
      {
        RUN_COUNTER += (UInt32)(sym + 1) << runPower;
        runPower++;
        if (blockSizeMax - BLOCK_SIZE < RUN_COUNTER)
          return SZ_ERROR_DATA;
        continue;
      }

      UInt32 *counters = this->Counters;
      if (RUN_COUNTER != 0)
      {
        UInt32 b = (UInt32)(mtf.Buf[0] & 0xFF);
        counters[b] += RUN_COUNTER;
        runPower = 0;
        #ifdef BZIP2_BYTE_MODE
          Byte *dest = (Byte *)(&counters[256 + kBlockSizeMax]) + BLOCK_SIZE;
          const Byte *limit = dest + RUN_COUNTER;
          BLOCK_SIZE += RUN_COUNTER;
          RUN_COUNTER = 0;
          do
          {
            dest[0] = (Byte)b;
            dest[1] = (Byte)b;
            dest[2] = (Byte)b;
            dest[3] = (Byte)b;
            dest += 4;
          }
          while (dest < limit);
        #else
          UInt32 *dest = &counters[256 + BLOCK_SIZE];
          const UInt32 *limit = dest + RUN_COUNTER;
          BLOCK_SIZE += RUN_COUNTER;
          RUN_COUNTER = 0;
          do
          {
            dest[0] = b;
            dest[1] = b;
            dest[2] = b;
            dest[3] = b;
            dest += 4;
          }
          while (dest < limit);
        #endif
      }
      
      sym -= 1;
      if (sym < numInUse)
      {
        if (BLOCK_SIZE >= blockSizeMax)
          return SZ_ERROR_DATA;

        // UInt32 b = (UInt32)mtf.GetAndMove((unsigned)sym);

        const unsigned lim = sym >> Z7_MTF_MOVS;
        const unsigned pos = (sym & Z7_MTF_MASK) << 3;
        CMtfVar next = mtf.Buf[lim];
        CMtfVar prev = (next >> pos) & 0xFF;
        
        #ifdef BZIP2_BYTE_MODE
          ((Byte *)(counters + 256 + kBlockSizeMax))[BLOCK_SIZE++] = (Byte)prev;
        #else
          (counters + 256)[BLOCK_SIZE++] = (UInt32)prev;
        #endif
        counters[prev]++;

        CMtfVar *m = mtf.Buf;
        CMtfVar *mLim = m + lim;
        if (lim != 0)
        {
          do
          {
            CMtfVar n0 = *m;
            *m = (n0 << 8) | prev;
            prev = (n0 >> (Z7_MTF_MASK << 3));
          }
          while (++m != mLim);
        }

        CMtfVar mask = (((CMtfVar)0x100 << pos) - 1);
        *mLim = (next & ~mask) | (((next << 8) | prev) & mask);
        continue;
      }
      
      if (sym != numInUse)
        return SZ_ERROR_DATA;
      break;
    }

    // we write additional item that will be read in DecodeBlock1 for prefetching
    #ifdef BZIP2_BYTE_MODE
      ((Byte *)(Counters + 256 + kBlockSizeMax))[BLOCK_SIZE] = 0;
    #else
      (counters + 256)[BLOCK_SIZE] = 0;
    #endif

    SAVE_LOCAL
    Props.blockSize = blockSize;
    state = STATE_BLOCK_SIGNATURE;
    state2 = 0;

    PRIN_VAL("origPtr", Props.origPtr);
    PRIN_VAL("blockSize", Props.blockSize);

    return (Props.origPtr < Props.blockSize) ? SZ_OK : SZ_ERROR_DATA;
  }
}


NO_INLINE
static void DecodeBlock1(UInt32 *counters, UInt32 blockSize)
{
  {
    UInt32 sum = 0;
    for (UInt32 i = 0; i < 256; i++)
    {
      const UInt32 v = counters[i];
      counters[i] = sum;
      sum += v;
    }
  }
  
  UInt32 *tt = counters + 256;
  // Compute the T^(-1) vector

  // blockSize--;

  #ifdef BZIP2_BYTE_MODE

  unsigned c = ((const Byte *)(tt + kBlockSizeMax))[0];
  
  for (UInt32 i = 0; i < blockSize; i++)
  {
    unsigned c1 = c;
    const UInt32 pos = counters[c];
    c = ((const Byte *)(tt + kBlockSizeMax))[(size_t)i + 1];
    counters[c1] = pos + 1;
    tt[pos] = (i << 8) | ((const Byte *)(tt + kBlockSizeMax))[pos];
  }

  /*
  // last iteration without next character prefetching
  {
    const UInt32 pos = counters[c];
    counters[c] = pos + 1;
    tt[pos] = (blockSize << 8) | ((const Byte *)(tt + kBlockSizeMax))[pos];
  }
  */

  #else

  unsigned c = (unsigned)(tt[0] & 0xFF);
  
  for (UInt32 i = 0; i < blockSize; i++)
  {
    unsigned c1 = c;
    const UInt32 pos = counters[c];
    c = (unsigned)(tt[(size_t)i + 1] & 0xFF);
    counters[c1] = pos + 1;
    tt[pos] |= (i << 8);
  }

  /*
  {
    const UInt32 pos = counters[c];
    counters[c] = pos + 1;
    tt[pos] |= (blockSize << 8);
  }
  */

  #endif


  /*
  for (UInt32 i = 0; i < blockSize; i++)
  {
    #ifdef BZIP2_BYTE_MODE
      const unsigned c = ((const Byte *)(tt + kBlockSizeMax))[i];
      const UInt32 pos = counters[c]++;
      tt[pos] = (i << 8) | ((const Byte *)(tt + kBlockSizeMax))[pos];
    #else
      const unsigned c = (unsigned)(tt[i] & 0xFF);
      const UInt32 pos = counters[c]++;
      tt[pos] |= (i << 8);
    #endif
  }
  */
}


void CSpecState::Init(UInt32 origPtr, unsigned randMode) throw()
{
  _tPos = _tt[_tt[origPtr] >> 8];
   _prevByte = (unsigned)(_tPos & 0xFF);
  _reps = 0;
  _randIndex = 0;
  _randToGo = -1;
  if (randMode)
  {
    _randIndex = 1;
    _randToGo = kRandNums[0] - 2;
  }
  _crc.Init();
}



NO_INLINE
Byte * CSpecState::Decode(Byte *data, size_t size) throw()
{
  if (size == 0)
    return data;

  unsigned prevByte = _prevByte;
  int reps = _reps;
  CBZip2Crc crc = _crc;
  const Byte *lim = data + size;

  while (reps > 0)
  {
    reps--;
    *data++ = (Byte)prevByte;
    crc.UpdateByte(prevByte);
    if (data == lim)
      break;
  }

  UInt32 tPos = _tPos;
  UInt32 blockSize = _blockSize;
  const UInt32 *tt = _tt;

  if (data != lim && blockSize)

  for (;;)
  {
    unsigned b = (unsigned)(tPos & 0xFF);
    tPos = tt[tPos >> 8];
    blockSize--;

    if (_randToGo >= 0)
    {
      if (_randToGo == 0)
      {
        b ^= 1;
        _randToGo = kRandNums[_randIndex];
        _randIndex++;
        _randIndex &= 0x1FF;
      }
      _randToGo--;
    }

    if (reps != -(int)kRleModeRepSize)
    {
      if (b != prevByte)
        reps = 0;
      reps--;
      prevByte = b;
      *data++ = (Byte)b;
      crc.UpdateByte(b);
      if (data == lim || blockSize == 0)
        break;
      continue;
    }

    reps = (int)b;
    while (reps)
    {
      reps--;
      *data++ = (Byte)prevByte;
      crc.UpdateByte(prevByte);
      if (data == lim)
        break;
    }
    if (data == lim)
      break;
    if (blockSize == 0)
      break;
  }

  if (blockSize == 1 && reps == -(int)kRleModeRepSize)
  {
    unsigned b = (unsigned)(tPos & 0xFF);
    tPos = tt[tPos >> 8];
    blockSize--;

    if (_randToGo >= 0)
    {
      if (_randToGo == 0)
      {
        b ^= 1;
        _randToGo = kRandNums[_randIndex];
        _randIndex++;
        _randIndex &= 0x1FF;
      }
      _randToGo--;
    }

    reps = (int)b;
  }

  _tPos = tPos;
  _prevByte = prevByte;
  _reps = reps;
  _crc = crc;
  _blockSize = blockSize;
  
  return data;
}


HRESULT CDecoder::Flush()
{
  if (_writeRes == S_OK)
  {
    _writeRes = WriteStream(_outStream, _outBuf, _outPos);
    _outWritten += _outPos;
    _outPos = 0;
  }
  return _writeRes;
}


NO_INLINE
HRESULT CDecoder::DecodeBlock(const CBlockProps &props)
{
  _calcedBlockCrc = 0;
  _blockFinished = false;

  CSpecState block;

  block._blockSize = props.blockSize;
  block._tt = _counters + 256;

  block.Init(props.origPtr, props.randMode);

  for (;;)
  {
    Byte *data = _outBuf + _outPos;
    size_t size = kOutBufSize - _outPos;
    
    if (_outSizeDefined)
    {
      const UInt64 rem = _outSize - _outPosTotal;
      if (size >= rem)
      {
        size = (size_t)rem;
        if (size == 0)
          return FinishMode ? S_FALSE : S_OK;
      }
    }

    TICKS_START
    const size_t processed = (size_t)(block.Decode(data, size) - data);
    TICKS_UPDATE(2)

    _outPosTotal += processed;
    _outPos += processed;
    
    if (processed >= size)
    {
      RINOK(Flush())
    }
    
    if (block.Finished())
    {
      _blockFinished = true;
      _calcedBlockCrc = block._crc.GetDigest();
      return S_OK;
    }
  }
}


CDecoder::CDecoder():
    _outBuf(NULL),
    FinishMode(false),
    _outSizeDefined(false),
    _counters(NULL),
    _inBuf(NULL),
    _inProcessed(0)
{
  #ifndef Z7_ST
  MtMode = false;
  NeedWaitScout = false;
  // ScoutRes = S_OK;
  #endif
}


CDecoder::~CDecoder()
{
  PRIN("\n~CDecoder()");

  #ifndef Z7_ST
  
  if (Thread.IsCreated())
  {
    WaitScout();

    _block.StopScout = true;

    PRIN("\nScoutEvent.Set()");
    ScoutEvent.Set();

    PRIN("\nThread.Wait()()");
    Thread.Wait_Close();
    PRIN("\n after Thread.Wait()()");

    // if (ScoutRes != S_OK) throw ScoutRes;
  }
  
  #endif

  BigFree(_counters);
  MidFree(_outBuf);
  MidFree(_inBuf);
}


HRESULT CDecoder::ReadInput()
{
  if (Base._buf != Base._lim || _inputFinished || _inputRes != S_OK)
    return _inputRes;

  _inProcessed += (size_t)(Base._buf - _inBuf);
  Base._buf = _inBuf;
  Base._lim = _inBuf;
  UInt32 size = 0;
  _inputRes = Base.InStream->Read(_inBuf, kInBufSize, &size);
  _inputFinished = (size == 0);
  Base._lim = _inBuf + size;
  return _inputRes;
}


void CDecoder::StartNewStream()
{
  Base.state = STATE_STREAM_SIGNATURE;
  Base.state2 = 0;
  Base.IsBz = false;
}


HRESULT CDecoder::ReadStreamSignature()
{
  for (;;)
  {
    RINOK(ReadInput())
    SRes res = Base.ReadStreamSignature2();
    if (res != SZ_OK)
      return S_FALSE;
    if (Base.state == STATE_BLOCK_SIGNATURE)
      return S_OK;
    if (_inputFinished)
    {
      Base.NeedMoreInput = true;
      return S_FALSE;
    }
  }
}


HRESULT CDecoder::StartRead()
{
  StartNewStream();
  return ReadStreamSignature();
}


HRESULT CDecoder::ReadBlockSignature()
{
  for (;;)
  {
    RINOK(ReadInput())
    
    SRes res = Base.ReadBlockSignature2();
    
    if (Base.state == STATE_STREAM_FINISHED)
      Base.FinishedPackSize = GetInputProcessedSize();
    if (res != SZ_OK)
      return S_FALSE;
    if (Base.state != STATE_BLOCK_SIGNATURE)
      return S_OK;
    if (_inputFinished)
    {
      Base.NeedMoreInput = true;
      return S_FALSE;
    }
  }
}


HRESULT CDecoder::ReadBlock()
{
  for (;;)
  {
    RINOK(ReadInput())

    SRes res = Base.ReadBlock2();

    if (res != SZ_OK)
      return S_FALSE;
    if (Base.state == STATE_BLOCK_SIGNATURE)
      return S_OK;
    if (_inputFinished)
    {
      Base.NeedMoreInput = true;
      return S_FALSE;
    }
  }
}



HRESULT CDecoder::DecodeStreams(ICompressProgressInfo *progress)
{
  {
    #ifndef Z7_ST
    _block.StopScout = false;
    #endif
  }

  RINOK(StartRead())

  UInt64 inPrev = 0;
  UInt64 outPrev = 0;

  {
    #ifndef Z7_ST
    CWaitScout_Releaser waitScout_Releaser(this);

    bool useMt = false;
    #endif

    bool wasFinished = false;

    UInt32 crc = 0;
    UInt32 nextCrc = 0;
    HRESULT nextRes = S_OK;

    UInt64 packPos = 0;

    CBlockProps props;

    props.blockSize = 0;

    for (;;)
    {
      if (progress)
      {
        const UInt64 outCur = GetOutProcessedSize();
        if (packPos - inPrev >= kProgressStep || outCur - outPrev >= kProgressStep)
        {
          RINOK(progress->SetRatioInfo(&packPos, &outCur))
          inPrev = packPos;
          outPrev = outCur;
        }
      }

      if (props.blockSize == 0)
        if (wasFinished || nextRes != S_OK)
          return nextRes;

      if (
          #ifndef Z7_ST
          !useMt &&
          #endif
          !wasFinished && Base.state == STATE_BLOCK_SIGNATURE)
      {
        nextRes = ReadBlockSignature();
        nextCrc = Base.crc;
        packPos = GetInputProcessedSize();

        wasFinished = true;

        if (nextRes != S_OK)
          continue;

        if (Base.state == STATE_STREAM_FINISHED)
        {
          if (!Base.DecodeAllStreams)
          {
            wasFinished = true;
            continue;
          }
          
          nextRes = StartRead();
         
          if (Base.NeedMoreInput)
          {
            if (Base.state2 == 0)
              Base.NeedMoreInput = false;
            wasFinished = true;
            nextRes = S_OK;
            continue;
          }
          
          if (nextRes != S_OK)
            continue;

          wasFinished = false;
          continue;
        }

        wasFinished = false;

        #ifndef Z7_ST
        if (MtMode)
        if (props.blockSize != 0)
        {
          // we start multithreading, if next block is big enough.
          const UInt32 k_Mt_BlockSize_Threshold = (1 << 12);  // (1 << 13)
          if (props.blockSize > k_Mt_BlockSize_Threshold)
          {
            if (!Thread.IsCreated())
            {
              PRIN("=== MT_MODE");
              RINOK(CreateThread())
            }
            useMt = true;
          }
        }
        #endif
      }

      if (props.blockSize == 0)
      {
        crc = nextCrc;
        
        #ifndef Z7_ST
        if (useMt)
        {
          PRIN("DecoderEvent.Lock()");
          {
            WRes wres = DecoderEvent.Lock();
            if (wres != 0)
              return HRESULT_FROM_WIN32(wres);
          }
          NeedWaitScout = false;
          PRIN("-- DecoderEvent.Lock()");
          props = _block.Props;
          nextCrc = _block.NextCrc;
          if (_block.Crc_Defined)
            crc = _block.Crc;
          packPos = _block.PackPos;
          wasFinished = _block.WasFinished;
          RINOK(_block.Res)
        }
        else
        #endif
        {
          if (Base.state != STATE_BLOCK_START)
            return E_FAIL;

          TICKS_START
          Base.Props.randMode = 1;
          RINOK(ReadBlock())
          TICKS_UPDATE(0)
          
          props = Base.Props;
          continue;
        }
      }

      if (props.blockSize != 0)
      {
        TICKS_START
        DecodeBlock1(_counters, props.blockSize);
        TICKS_UPDATE(1)
      }
      
      #ifndef Z7_ST
      if (useMt && !wasFinished)
      {
        /*
        if (props.blockSize == 0)
        {
          // this codes switches back to single-threadMode
          useMt = false;
          PRIN("=== ST_MODE");
          continue;
          }
        */
        
        PRIN("ScoutEvent.Set()");
        {
          WRes wres = ScoutEvent.Set();
          if (wres != 0)
            return HRESULT_FROM_WIN32(wres);
        }
        NeedWaitScout = true;
      }
      #endif
        
      if (props.blockSize == 0)
        continue;

      RINOK(DecodeBlock(props))

      if (!_blockFinished)
        return nextRes;

      props.blockSize = 0;
      if (_calcedBlockCrc != crc)
      {
        BlockCrcError = true;
        return S_FALSE;
      }
    }
  }
}




bool CDecoder::CreateInputBufer()
{
  if (!_inBuf)
  {
    _inBuf = (Byte *)MidAlloc(kInBufSize);
    if (!_inBuf)
      return false;
    Base._buf = _inBuf;
    Base._lim = _inBuf;
  }
  if (!_counters)
  {
    const size_t size = (256 + kBlockSizeMax) * sizeof(UInt32)
      #ifdef BZIP2_BYTE_MODE
        + kBlockSizeMax
      #endif
        + 256;
    _counters = (UInt32 *)::BigAlloc(size);
    if (!_counters)
      return false;
    Base.Counters = _counters;
  }
  return true;
}


void CDecoder::InitOutSize(const UInt64 *outSize)
{
  _outPosTotal = 0;
  
  _outSizeDefined = false;
  _outSize = 0;
  if (outSize)
  {
    _outSize = *outSize;
    _outSizeDefined = true;
  }
  
  BlockCrcError = false;
  
  Base.InitNumStreams2();
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  /*
  {
    RINOK(SetInStream(inStream));
    RINOK(SetOutStreamSize(outSize));

    RINOK(CopyStream(this, outStream, progress));
    return ReleaseInStream();
  }
  */

  _inputFinished = false;
  _inputRes = S_OK;
  _writeRes = S_OK;

  try {

  InitOutSize(outSize);
  
  // we can request data from InputBuffer after Code().
  // so we init InputBuffer before any function return.

  InitInputBuffer();

  if (!CreateInputBufer())
    return E_OUTOFMEMORY;

  if (!_outBuf)
  {
    _outBuf = (Byte *)MidAlloc(kOutBufSize);
    if (!_outBuf)
      return E_OUTOFMEMORY;
  }

  Base.InStream = inStream;
  
  // InitInputBuffer();
  
  _outStream = outStream;
  _outWritten = 0;
  _outPos = 0;

  HRESULT res = DecodeStreams(progress);

  Flush();

  Base.InStream = NULL;
  _outStream = NULL;

  /*
  if (res == S_OK)
    if (FinishMode && inSize && *inSize != GetInputProcessedSize())
      res = S_FALSE;
  */

  if (res != S_OK)
    return res;

  } catch(...) { return E_FAIL; }

  return _writeRes;
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  FinishMode = (finishMode != 0);
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = GetInStreamSize();
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::ReadUnusedFromInBuf(void *data, UInt32 size, UInt32 *processedSize))
{
  Base.AlignToByte();
  UInt32 i;
  for (i = 0; i < size; i++)
  {
    int b;
    Base.ReadByte(b);
    if (b < 0)
      break;
    ((Byte *)data)[i] = (Byte)b;
  }
  if (processedSize)
    *processedSize = i;
  return S_OK;
}


#ifndef Z7_ST

#define PRIN_MT(s) PRIN("    " s)

// #define RINOK_THREAD(x) { WRes __result_ = (x); if (__result_ != 0) return __result_; }

static THREAD_FUNC_DECL RunScout2(void *p) { ((CDecoder *)p)->RunScout(); return 0; }

HRESULT CDecoder::CreateThread()
{
  WRes             wres = DecoderEvent.CreateIfNotCreated_Reset();
  if (wres == 0) { wres = ScoutEvent.CreateIfNotCreated_Reset();
  if (wres == 0) { wres = Thread.Create(RunScout2, this); }}
  return HRESULT_FROM_WIN32(wres);
}

void CDecoder::RunScout()
{
  for (;;)
  {
    {
      PRIN_MT("ScoutEvent.Lock()")
      WRes wres = ScoutEvent.Lock();
      PRIN_MT("-- ScoutEvent.Lock()")
      if (wres != 0)
      {
        // ScoutRes = wres;
        return;
      }
    }

    CBlock &block = _block;

    if (block.StopScout)
    {
      // ScoutRes = S_OK;
      return;
    }

    block.Res = S_OK;
    block.WasFinished = false;

    HRESULT res = S_OK;

    try
    {
      UInt64 packPos = GetInputProcessedSize();

      block.Props.blockSize = 0;
      block.Crc_Defined = false;
      // block.NextCrc_Defined = false;
      block.NextCrc = 0;
      
      for (;;)
      {
        if (Base.state == STATE_BLOCK_SIGNATURE)
        {
          res = ReadBlockSignature();

          if (res != S_OK)
            break;
          
          if (block.Props.blockSize == 0)
          {
            block.Crc = Base.crc;
            block.Crc_Defined = true;
          }
          else
          {
            block.NextCrc = Base.crc;
            // block.NextCrc_Defined = true;
          }

          continue;
        }

        if (Base.state == STATE_BLOCK_START)
        {
          if (block.Props.blockSize != 0)
            break;

          Base.Props.randMode = 1;

          res = ReadBlock();
          
          PRIN_MT("-- Base.ReadBlock")
          if (res != S_OK)
            break;
          block.Props = Base.Props;
          continue;
        }

        if (Base.state == STATE_STREAM_FINISHED)
        {
          if (!Base.DecodeAllStreams)
          {
            block.WasFinished = true;
            break;
          }
          
          res = StartRead();
          
          if (Base.NeedMoreInput)
          {
            if (Base.state2 == 0)
              Base.NeedMoreInput = false;
            block.WasFinished = true;
            res = S_OK;
            break;
          }
          
          if (res != S_OK)
            break;
          
          if (GetInputProcessedSize() - packPos > 0) // kProgressStep
            break;
          continue;
        }
        
        // throw 1;
        res = E_FAIL;
        break;
      }
    }
      
    catch (...) { res = E_FAIL; }
      
    if (res != S_OK)
    {
      PRIN_MT("error")
      block.Res = res;
      block.WasFinished = true;
    }

    block.PackPos = GetInputProcessedSize();
    PRIN_MT("DecoderEvent.Set()")
    WRes wres = DecoderEvent.Set();
    if (wres != 0)
    {
      // ScoutRes = wres;
      return;
    }
  }
}


Z7_COM7F_IMF(CDecoder::SetNumberOfThreads(UInt32 numThreads))
{
  MtMode = (numThreads > 1);

  #ifndef BZIP2_BYTE_MODE
  MtMode = false;
  #endif

  // MtMode = false;
  return S_OK;
}

#endif



#ifndef Z7_NO_READ_FROM_CODER


Z7_COM7F_IMF(CDecoder::SetInStream(ISequentialInStream *inStream))
{
  Base.InStreamRef = inStream;
  Base.InStream = inStream;
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::ReleaseInStream())
{
  Base.InStreamRef.Release();
  Base.InStream = NULL;
  return S_OK;
}



Z7_COM7F_IMF(CDecoder::SetOutStreamSize(const UInt64 *outSize))
{
  InitOutSize(outSize);

  InitInputBuffer();
  
  if (!CreateInputBufer())
    return E_OUTOFMEMORY;

  // InitInputBuffer();

  StartNewStream();

  _blockFinished = true;

  ErrorResult = S_OK;

  _inputFinished = false;
  _inputRes = S_OK;

  return S_OK;
}



Z7_COM7F_IMF(CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  *processedSize = 0;

  try {

  if (ErrorResult != S_OK)
    return ErrorResult;

  for (;;)
  {
    if (Base.state == STATE_STREAM_FINISHED)
    {
      if (!Base.DecodeAllStreams)
        return ErrorResult;
      StartNewStream();
      continue;
    }

    if (Base.state == STATE_STREAM_SIGNATURE)
    {
      ErrorResult = ReadStreamSignature();

      if (Base.NeedMoreInput)
        if (Base.state2 == 0 && Base.NumStreams != 0)
        {
          Base.NeedMoreInput = false;
          ErrorResult = S_OK;
          return S_OK;
        }
      if (ErrorResult != S_OK)
        return ErrorResult;
      continue;
    }
    
    if (_blockFinished && Base.state == STATE_BLOCK_SIGNATURE)
    {
      ErrorResult = ReadBlockSignature();
      
      if (ErrorResult != S_OK)
        return ErrorResult;
      
      continue;
    }

    if (_outSizeDefined)
    {
      const UInt64 rem = _outSize - _outPosTotal;
      if (size >= rem)
        size = (UInt32)rem;
    }
    if (size == 0)
      return S_OK;
    
    if (_blockFinished)
    {
      if (Base.state != STATE_BLOCK_START)
      {
        ErrorResult = E_FAIL;
        return ErrorResult;
      }
      
      Base.Props.randMode = 1;
      ErrorResult = ReadBlock();
      
      if (ErrorResult != S_OK)
        return ErrorResult;
      
      DecodeBlock1(_counters, Base.Props.blockSize);
      
      _spec._blockSize = Base.Props.blockSize;
      _spec._tt = _counters + 256;
      _spec.Init(Base.Props.origPtr, Base.Props.randMode);

      _blockFinished = false;
    }

    {
      Byte *ptr = _spec.Decode((Byte *)data, size);
      
      const UInt32 processed = (UInt32)(ptr - (Byte *)data);
      data = ptr;
      size -= processed;
      (*processedSize) += processed;
      _outPosTotal += processed;
      
      if (_spec.Finished())
      {
        _blockFinished = true;
        if (Base.crc != _spec._crc.GetDigest())
        {
          BlockCrcError = true;
          ErrorResult = S_FALSE;
          return ErrorResult;
        }
      }
    }
  }

  } catch(...) { ErrorResult = S_FALSE; return S_FALSE; }
}



// ---------- NSIS ----------

Z7_COM7F_IMF(CNsisDecoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  *processedSize = 0;

  try {

  if (ErrorResult != S_OK)
    return ErrorResult;
    
  if (Base.state == STATE_STREAM_FINISHED)
    return S_OK;

  if (Base.state == STATE_STREAM_SIGNATURE)
  {
    Base.blockSizeMax = 9 * kBlockSizeStep;
    Base.state = STATE_BLOCK_SIGNATURE;
    // Base.state2 = 0;
  }

  for (;;)
  {
    if (_blockFinished && Base.state == STATE_BLOCK_SIGNATURE)
    {
      ErrorResult = ReadInput();
      if (ErrorResult != S_OK)
        return ErrorResult;
      
      int b;
      Base.ReadByte(b);
      if (b < 0)
      {
        ErrorResult = S_FALSE;
        return ErrorResult;
      }
      
      if (b == kFinSig0)
      {
        /*
        if (!Base.AreRemainByteBitsEmpty())
          ErrorResult = S_FALSE;
        */
        Base.state = STATE_STREAM_FINISHED;
        return ErrorResult;
      }
      
      if (b != kBlockSig0)
      {
        ErrorResult = S_FALSE;
        return ErrorResult;
      }
      
      Base.state = STATE_BLOCK_START;
    }

    if (_outSizeDefined)
    {
      const UInt64 rem = _outSize - _outPosTotal;
      if (size >= rem)
        size = (UInt32)rem;
    }
    if (size == 0)
      return S_OK;
    
    if (_blockFinished)
    {
      if (Base.state != STATE_BLOCK_START)
      {
        ErrorResult = E_FAIL;
        return ErrorResult;
      }

      Base.Props.randMode = 0;
      ErrorResult = ReadBlock();
      
      if (ErrorResult != S_OK)
        return ErrorResult;
      
      DecodeBlock1(_counters, Base.Props.blockSize);
      
      _spec._blockSize = Base.Props.blockSize;
      _spec._tt = _counters + 256;
      _spec.Init(Base.Props.origPtr, Base.Props.randMode);
      
      _blockFinished = false;
    }
    
    {
      Byte *ptr = _spec.Decode((Byte *)data, size);
      
      const UInt32 processed = (UInt32)(ptr - (Byte *)data);
      data = ptr;
      size -= processed;
      (*processedSize) += processed;
      _outPosTotal += processed;
      
      if (_spec.Finished())
        _blockFinished = true;
    }
  }

  } catch(...) { ErrorResult = S_FALSE; return S_FALSE; }
}

#endif

}}

/* ================ unit: CPP/7zip/Compress/BZip2Encoder.cpp ================ */
// BZip2Encoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBZip2 {

#define HUFFMAN_LEN 16
#if HUFFMAN_LEN > Z7_HUFFMAN_LEN_MAX
  #error Stop_Compiling_Bad_HUFFMAN_LEN_BZip2Encoder
#endif
  
static const size_t kBufferSize = 1 << 17;
static const unsigned kNumHuffPasses = 4;


bool CThreadInfo::Alloc()
{
  if (!m_BlockSorterIndex)
  {
    m_BlockSorterIndex = (UInt32 *)::BigAlloc(BLOCK_SORT_BUF_SIZE(kBlockSizeMax) * sizeof(UInt32));
    if (!m_BlockSorterIndex)
      return false;
  }

  if (!m_Block_Base)
  {
    const unsigned kPadSize = 1 << 7; // we need at least 1 byte backward padding, becuase we use (m_Block - 1) pointer;
    m_Block_Base = (Byte *)::MidAlloc(kBlockSizeMax * 5
        + kBlockSizeMax / 10 + (20 << 10)
        + kPadSize);
    if (!m_Block_Base)
      return false;
    m_Block = m_Block_Base + kPadSize;
    m_MtfArray = m_Block + kBlockSizeMax;
    m_TempArray = m_MtfArray + kBlockSizeMax * 2 + 2;
  }
  return true;
}

void CThreadInfo::Free()
{
  ::BigFree(m_BlockSorterIndex);
  m_BlockSorterIndex = NULL;
  ::MidFree(m_Block_Base);
  m_Block_Base = NULL;
}

#ifndef Z7_ST

static THREAD_FUNC_DECL MFThread(void *threadCoderInfo)
{
  return ((CThreadInfo *)threadCoderInfo)->ThreadFunc();
}

HRESULT CThreadInfo::Create()
{
  WRes             wres = StreamWasFinishedEvent.Create();
  if (wres == 0) { wres = WaitingWasStartedEvent.Create();
  if (wres == 0) { wres = CanWriteEvent.Create();
  if (wres == 0)
  {
    wres =
#ifdef _WIN32
      Encoder->_props.NumThreadGroups > 1 ?
        Thread.Create_With_Group(MFThread, this, ThreadNextGroup_GetNext(&Encoder->ThreadNextGroup), 0) : // affinity
#endif
      Encoder->_props.Affinity != 0 ?
        Thread.Create_With_Affinity(MFThread, this, (CAffinityMask)Encoder->_props.Affinity) :
        Thread.Create(MFThread, this);
  }}}
  return HRESULT_FROM_WIN32(wres);
}

void CThreadInfo::FinishStream(bool needLeave)
{
  Encoder->StreamWasFinished = true;
  StreamWasFinishedEvent.Set();
  if (needLeave)
    Encoder->CS.Leave();
  Encoder->CanStartWaitingEvent.Lock();
  WaitingWasStartedEvent.Set();
}

THREAD_FUNC_RET_TYPE CThreadInfo::ThreadFunc()
{
  for (;;)
  {
    Encoder->CanProcessEvent.Lock();
    Encoder->CS.Enter();
    if (Encoder->CloseThreads)
    {
      Encoder->CS.Leave();
      return 0;
    }
    if (Encoder->StreamWasFinished)
    {
      FinishStream(true);
      continue;
    }
    HRESULT res = S_OK;
    bool needLeave = true;
    try
    {
      const UInt32 blockSize = Encoder->ReadRleBlock(m_Block);
      m_UnpackSize = Encoder->m_InStream.GetProcessedSize();
      m_BlockIndex = Encoder->NextBlockIndex;
      if (++Encoder->NextBlockIndex == Encoder->NumThreads)
        Encoder->NextBlockIndex = 0;
      if (blockSize == 0)
      {
        FinishStream(true);
        continue;
      }
      Encoder->CS.Leave();
      needLeave = false;
      res = EncodeBlock3(blockSize);
    }
    catch(const CInBufferException &e)  { res = e.ErrorCode; }
    catch(const COutBufferException &e) { res = e.ErrorCode; }
    catch(...) { res = E_FAIL; }
    if (res != S_OK)
    {
      Encoder->Result = res;
      FinishStream(needLeave);
      continue;
    }
  }
}

#endif

void CEncProps::Normalize(int level)
{
  if (level < 0) level = 5;
  if (level > 9) level = 9;
  
  if (NumPasses == (UInt32)(Int32)-1)
    NumPasses = (level >= 9 ? 7 : (level >= 7 ? 2 : 1));
  if (NumPasses < 1) NumPasses = 1;
  if (NumPasses > kNumPassesMax) NumPasses = kNumPassesMax;
  
  if (BlockSizeMult == (UInt32)(Int32)-1)
    BlockSizeMult = (level >= 5 ? 9 : (level >= 1 ? (unsigned)level * 2 - 1: 1));
  if (BlockSizeMult < kBlockSizeMultMin) BlockSizeMult = kBlockSizeMultMin;
  if (BlockSizeMult > kBlockSizeMultMax) BlockSizeMult = kBlockSizeMultMax;
}

CEncoder::CEncoder()
{
  _props.Normalize(-1);

  #ifndef Z7_ST
  ThreadsInfo = NULL;
  m_NumThreadsPrev = 0;
  NumThreads = 1;
  #endif
}

#ifndef Z7_ST
CEncoder::~CEncoder()
{
  Free();
}

HRESULT CEncoder::Create()
{
  {
    WRes             wres = CanProcessEvent.CreateIfNotCreated_Reset();
    if (wres == 0) { wres = CanStartWaitingEvent.CreateIfNotCreated_Reset(); }
    if (wres != 0)
      return HRESULT_FROM_WIN32(wres);
  }
  
  if (ThreadsInfo && m_NumThreadsPrev == NumThreads)
    return S_OK;
  try
  {
    Free();
    MtMode = (NumThreads > 1);
    m_NumThreadsPrev = NumThreads;
    ThreadsInfo = new CThreadInfo[NumThreads];
    if (!ThreadsInfo)
      return E_OUTOFMEMORY;
  }
  catch(...) { return E_OUTOFMEMORY; }
  for (UInt32 t = 0; t < NumThreads; t++)
  {
    CThreadInfo &ti = ThreadsInfo[t];
    ti.Encoder = this;
    if (MtMode)
    {
      HRESULT res = ti.Create();
      if (res != S_OK)
      {
        NumThreads = t;
        Free();
        return res;
      }
    }
  }
  return S_OK;
}

void CEncoder::Free()
{
  if (!ThreadsInfo)
    return;
  CloseThreads = true;
  CanProcessEvent.Set();
  for (UInt32 t = 0; t < NumThreads; t++)
  {
    CThreadInfo &ti = ThreadsInfo[t];
    if (MtMode)
      ti.Thread.Wait_Close();
    ti.Free();
  }
  delete []ThreadsInfo;
  ThreadsInfo = NULL;
}
#endif

struct CRleEncoder
{
  const Byte *_src;
  const Byte *_srcLim;
  Byte *_dest;
  const Byte *_destLim;
  Byte _prevByte;
  unsigned _numReps;

  void Encode();
};

Z7_NO_INLINE
void CRleEncoder::Encode()
{
  const Byte *src = _src;
  const Byte * const srcLim = _srcLim;
  Byte *dest = _dest;
  const Byte * const destLim = _destLim;
  Byte prev = _prevByte;
  unsigned numReps = _numReps;
  // (dest < destLim)
  // src = srcLim; // for debug
  while (dest < destLim)
  {
    if (src == srcLim)
      break;
    const Byte b = *src++;
    if (b != prev)
    {
      if (numReps >= kRleModeRepSize)
        *dest++ = (Byte)(numReps - kRleModeRepSize);
      *dest++ = b;
      numReps = 1;
      prev = b;
      /*
      { // speed optimization code:
        if (dest >= destLim || src == srcLim)
          break;
        const Byte b2 = *src++;
        *dest++ = b2;
        numReps += (prev == b2);
        prev = b2;
      }
      */
      continue;
    }
    numReps++;
    if (numReps <= kRleModeRepSize)
      *dest++ = b;
    else if (numReps == kRleModeRepSize + 255)
    {
      *dest++ = (Byte)(numReps - kRleModeRepSize);
      numReps = 0;
    }
  }
  _src = src;
  _dest = dest;
  _prevByte = prev;
  _numReps = numReps;
  // (dest <= destLim + 1)
}


// out: return value is blockSize: size of data filled in buffer[]:
// (returned_blockSize <= _props.BlockSizeMult * kBlockSizeStep)
UInt32 CEncoder::ReadRleBlock(Byte *buffer)
{
  CRleEncoder rle;
  UInt32 i = 0;
  if (m_InStream.ReadByte(rle._prevByte))
  {
    NumBlocks++;
    const UInt32 blockSize = _props.BlockSizeMult * kBlockSizeStep - 1; // -1 for RLE
    rle._destLim = buffer + blockSize;
    rle._numReps = 1;
    buffer[i++] = rle._prevByte;
    while (i < blockSize)
    {
      rle._dest = buffer + i;
      size_t rem;
      const Byte * const ptr = m_InStream.Lookahead(rem);
      if (rem == 0)
        break;
      rle._src = ptr;
      rle._srcLim = ptr + rem;
      rle.Encode();
      m_InStream.Skip((size_t)(rle._src - ptr));
      i = (UInt32)(size_t)(rle._dest - buffer);
      // (i <= blockSize + 1)
    }
    const int n = (int)rle._numReps - (int)kRleModeRepSize;
    if (n >= 0)
      buffer[i++] = (Byte)n;
  }
  return i;
}



Z7_NO_INLINE
void CThreadInfo::WriteBits2(UInt32 value, unsigned numBits)
  { m_OutStreamCurrent.WriteBits(value, numBits); }
/*
Z7_NO_INLINE
void CThreadInfo::WriteByte2(unsigned b)
  { m_OutStreamCurrent.WriteByte(b); }
*/
// void CEncoder::WriteBits(UInt32 value, unsigned numBits) { m_OutStream.WriteBits(value, numBits); }
Z7_NO_INLINE
void CEncoder::WriteByte(Byte b) { m_OutStream.WriteByte(b); }


#define WRITE_BITS_UPDATE(value, numBits) \
{ \
  numBits -= _bitPos; \
  const UInt32 hi = value >> numBits; \
  *_buf++ = (Byte)(_curByte | hi); \
  value -= hi << numBits; \
  _bitPos = 8; \
  _curByte = 0; \
}

#if HUFFMAN_LEN > 16

#define WRITE_BITS_HUFF(value2, numBits2) \
{ \
  UInt32 value = value2; \
  unsigned numBits = numBits2; \
  while (numBits >= _bitPos) { \
    WRITE_BITS_UPDATE(value, numBits) \
  } \
  _bitPos -= numBits; \
  _curByte |= (value << _bitPos); \
}

#else // HUFFMAN_LEN <= 16

// numBits2 <= 16 is supported
#define WRITE_BITS_HUFF(value2, numBits2) \
{ \
  UInt32 value = value2; \
  unsigned numBits = numBits2; \
  if (numBits >= _bitPos) \
  { \
    WRITE_BITS_UPDATE(value, numBits) \
    if (numBits >= _bitPos) \
    { \
      numBits -= _bitPos; \
      const UInt32 hi = value >> numBits; \
      *_buf++ = (Byte)hi; \
      value -= hi << numBits; \
    } \
  } \
  _bitPos -= numBits; \
  _curByte |= (value << _bitPos); \
}

#endif

#define WRITE_BITS_8(value2, numBits2) \
{ \
  UInt32 value = value2; \
  unsigned numBits = numBits2; \
  if (numBits >= _bitPos) \
  { \
    WRITE_BITS_UPDATE(value, numBits) \
  } \
  _bitPos -= numBits; \
  _curByte |= (value << _bitPos); \
}

#define WRITE_BIT_PRE \
  { _bitPos--; }

#define WRITE_BIT_POST \
{ \
  if (_bitPos == 0) \
  { \
    *_buf++ = (Byte)_curByte; \
    _curByte = 0; \
    _bitPos = 8; \
  } \
}

#define WRITE_BIT_0 \
{ \
  WRITE_BIT_PRE \
  WRITE_BIT_POST \
}

#define WRITE_BIT_1 \
{ \
  WRITE_BIT_PRE \
  _curByte |= 1u << _bitPos; \
  WRITE_BIT_POST \
}


// blockSize > 0
void CThreadInfo::EncodeBlock(const Byte *block, UInt32 blockSize)
{
  // WriteBit2(0); // Randomised = false
  {
    const UInt32 origPtr = BlockSort(m_BlockSorterIndex, block, blockSize);
    // if (m_BlockSorterIndex[origPtr] != 0) throw 1;
    m_BlockSorterIndex[origPtr] = blockSize;
    WriteBits2(origPtr, kNumOrigBits + 1); // + 1 for additional high bit flag (Randomised = false)
  }
  Byte mtfBuf[256];
  // memset(mtfBuf, 0, sizeof(mtfBuf)); // to disable MSVC warning
  unsigned numInUse;
  {
    Byte inUse[256];
    Byte inUse16[16];
    unsigned i;
    for (i = 0; i < 256; i++)
      inUse[i] = 0;
    for (i = 0; i < 16; i++)
      inUse16[i] = 0;
    {
      const Byte *       cur = block;
      block = block + (size_t)blockSize - 1;
      if (cur != block)
      {
        do
        {
          const unsigned b0 = cur[0];
          const unsigned b1 = cur[1];
          cur += 2;
          inUse[b0] = 1;
          inUse[b1] = 1;
        }
        while (cur < block);
      }
      if (cur == block)
        inUse[cur[0]] = 1;
      block -= blockSize; // block pointer is (original_block - 1)
    }
    numInUse = 0;
    for (i = 0; i < 256; i++)
      if (inUse[i])
      {
        inUse16[i >> 4] = 1;
        mtfBuf[numInUse++] = (Byte)i;
      }
    for (i = 0; i < 16; i++)
      WriteBit2(inUse16[i]);
    for (i = 0; i < 256; i++)
      if (inUse16[i >> 4])
        WriteBit2(inUse[i]);
  }
  const unsigned alphaSize = numInUse + 2;

  UInt32 symbolCounts[kMaxAlphaSize];
  {
    for (unsigned i = 0; i < kMaxAlphaSize; i++)
      symbolCounts[i] = 0;
    symbolCounts[(size_t)alphaSize - 1] = 1;
  }

  Byte *mtfs = m_MtfArray;
  {
    const UInt32 *bsIndex = m_BlockSorterIndex;
    const UInt32 *bsIndex_rle = bsIndex;
    const UInt32 * const bsIndex_end = bsIndex + blockSize;
    // block--; // backward fix
    // block pointer is (original_block - 1)
    do
    {
      const Byte v = block[*bsIndex++];
      Byte a = mtfBuf[0];
      if (v != a)
      {
        mtfBuf[0] = v;
        {
          UInt32 rleSize = (UInt32)(size_t)(bsIndex - bsIndex_rle) - 1;
          bsIndex_rle = bsIndex;
          while (rleSize)
          {
            const unsigned sym = (unsigned)(--rleSize & 1);
            *mtfs++ = (Byte)sym;
            symbolCounts[sym]++;
            rleSize >>= 1;
          }
        }
        unsigned pos1 = 2; // = real_pos + 1
        Byte b;
               b = mtfBuf[1];  mtfBuf[1] = a;  if (v != b)
             { a = mtfBuf[2];  mtfBuf[2] = b;  if (v == a) pos1 = 3;
        else { b = mtfBuf[3];  mtfBuf[3] = a;  if (v == b) pos1 = 4;
        else
        {
          Byte *m = mtfBuf + 7;
          for (;;)
          {
            a = m[-3];  m[-3] = b;           if (v == a) { pos1 = (unsigned)(size_t)(m - (mtfBuf + 2)); break; }
            b = m[-2];  m[-2] = a;           if (v == b) { pos1 = (unsigned)(size_t)(m - (mtfBuf + 1)); break; }
            a = m[-1];  m[-1] = b;           if (v == a) { pos1 = (unsigned)(size_t)(m - (mtfBuf    )); break; }
            b = m[ 0];  m[ 0] = a;  m += 4;  if (v == b) { pos1 = (unsigned)(size_t)(m - (mtfBuf + 3)); break; }
          }
        }}}
        symbolCounts[pos1]++;
        if (pos1 >= 0xff)
        {
          *mtfs++ = 0xff;
          // pos1 -= 0xff;
          pos1++; // we need only low byte
        }
        *mtfs++ = (Byte)pos1;
      }
    }
    while (bsIndex < bsIndex_end);

    UInt32 rleSize = (UInt32)(size_t)(bsIndex - bsIndex_rle);
    while (rleSize)
    {
      const unsigned sym = (unsigned)(--rleSize & 1);
      *mtfs++ = (Byte)sym;
      symbolCounts[sym]++;
      rleSize >>= 1;
    }
    
    unsigned d = alphaSize - 1;
    if (alphaSize >= 256)
    {
      *mtfs++ = 0xff;
      d = alphaSize; // (-256)
    }
    *mtfs++ = (Byte)d;
  }

  const Byte * const mtf_lim = mtfs;

  UInt32 numSymbols = 0;
  {
    for (unsigned i = 0; i < kMaxAlphaSize; i++)
      numSymbols += symbolCounts[i];
  }

  unsigned bestNumTables = kNumTablesMin;
  UInt32 bestPrice = 0xFFFFFFFF;
  const UInt32 startPos = m_OutStreamCurrent.GetPos();
  const unsigned startCurByte = m_OutStreamCurrent.GetCurByte();
  for (unsigned nt = kNumTablesMin; nt <= kNumTablesMax + 1; nt++)
  {
    unsigned numTables;

    if (m_OptimizeNumTables)
    {
      m_OutStreamCurrent.SetPos(startPos);
      m_OutStreamCurrent.SetCurState(startPos & 7, startCurByte);
      numTables = (nt <= kNumTablesMax ? nt : bestNumTables);
    }
    else
    {
           if (numSymbols <  200) numTables = 2;
      else if (numSymbols <  600) numTables = 3;
      else if (numSymbols < 1200) numTables = 4;
      else if (numSymbols < 2400) numTables = 5;
      else                        numTables = 6;
    }

    WriteBits2(numTables, kNumTablesBits);
    const unsigned numSelectors = (numSymbols + kGroupSize - 1) / kGroupSize;
    WriteBits2((UInt32)numSelectors, kNumSelectorsBits);
    
    {
      UInt32 remFreq = numSymbols;
      unsigned gs = 0;
      unsigned t = numTables;
      do
      {
        UInt32 tFreq = remFreq / t;
        unsigned ge = gs;
        UInt32 aFreq = 0;
        while (aFreq < tFreq) //  && ge < alphaSize)
          aFreq += symbolCounts[ge++];
        
        if (ge > gs + 1 && t != numTables && t != 1 && (((numTables - t) & 1) == 1))
          aFreq -= symbolCounts[--ge];
        
        Byte *lens = Lens[(size_t)t - 1];
        unsigned i = 0;
        do
          lens[i] = (Byte)((i >= gs && i < ge) ? 0 : 1);
        while (++i < alphaSize);
        gs = ge;
        remFreq -= aFreq;
      }
      while (--t != 0);
    }
    
    
    for (unsigned pass = 0; pass < kNumHuffPasses; pass++)
    {
      memset(Freqs, 0, sizeof(Freqs[0]) * numTables);
      // memset(Freqs, 0, sizeof(Freqs));
      {
        mtfs = m_MtfArray;
        UInt32 g = 0;
        do
        {
          unsigned symbols[kGroupSize];
          unsigned i = 0;
          do
          {
            UInt32 symbol = *mtfs++;
            if (symbol >= 0xFF)
              symbol += *mtfs++;
            symbols[i] = symbol;
          }
          while (++i < kGroupSize && mtfs < mtf_lim);
          
          UInt32 bestPrice2 = 0xFFFFFFFF;
          unsigned t = 0;
          do
          {
            const Byte *lens = Lens[t];
            UInt32 price = 0;
            unsigned j = 0;
            do
              price += lens[symbols[j]];
            while (++j < i);
            if (price < bestPrice2)
            {
              m_Selectors[g] = (Byte)t;
              bestPrice2 = price;
            }
          }
          while (++t < numTables);
          UInt32 *freqs = Freqs[m_Selectors[g++]];
          unsigned j = 0;
          do
            freqs[symbols[j]]++;
          while (++j < i);
        }
        while (mtfs < mtf_lim);
      }
      
      unsigned t = 0;
      do
      {
        UInt32 *freqs = Freqs[t];
        unsigned i = 0;
        do
          if (freqs[i] == 0)
            freqs[i] = 1;
        while (++i < alphaSize);
        Huffman_Generate(freqs, Codes[t], Lens[t], kMaxAlphaSize, HUFFMAN_LEN);
      }
      while (++t < numTables);
    }
    
    unsigned _bitPos;  // 0 < _bitPos <= 8 : number of non-filled low bits in _curByte
    unsigned _curByte; // low (_bitPos) bits are zeros
                       // high (8 - _bitPos) bits are filled
    Byte *_buf;
    {
      Byte mtfSel[kNumTablesMax];
      {
        unsigned t = 0;
        do
          mtfSel[t] = (Byte)t;
        while (++t < numTables);
      }

      _bitPos = m_OutStreamCurrent._bitPos;
      _curByte = m_OutStreamCurrent._curByte;
      _buf = m_OutStreamCurrent._buf;
      // stream.Init_from_Global(m_OutStreamCurrent);
      
      const Byte *selectors = m_Selectors;
      const Byte * const selectors_lim = selectors + numSelectors;
      Byte prev = 0; // mtfSel[0];
      do
      {
        const Byte sel = *selectors++;
        if (prev != sel)
        {
          Byte *mtfSel_cur = &mtfSel[1];
          for (;;)
          {
            WRITE_BIT_1
            const Byte next = *mtfSel_cur;
            *mtfSel_cur++ = prev;
            prev = next;
            if (next == sel)
              break;
          }
          // mtfSel[0] = sel;
        }
        WRITE_BIT_0
      }
      while (selectors != selectors_lim);
    }
    {
      unsigned t = 0;
      do
      {
        const Byte *lens = Lens[t];
        unsigned len = lens[0];
        WRITE_BITS_8(len, kNumLevelsBits)
        unsigned i = 0;
        do
        {
          const unsigned level = lens[i];
          while (len != level)
          {
            WRITE_BIT_1
            if (len < level)
            {
              len++;
              WRITE_BIT_0
            }
            else
            {
              len--;
              WRITE_BIT_1
            }
          }
          WRITE_BIT_0
        }
        while (++i < alphaSize);
      }
      while (++t < numTables);
    }
    {
      UInt32 groupSize = 1;
      const Byte *selectors = m_Selectors;
      const Byte *lens = NULL;
      const UInt32 *codes = NULL;
      mtfs = m_MtfArray;
      do
      {
        unsigned symbol = *mtfs++;
        if (symbol >= 0xFF)
          symbol += *mtfs++;
        if (--groupSize == 0)
        {
          groupSize = kGroupSize;
          const unsigned t = *selectors++;
          lens = Lens[t];
          codes = Codes[t];
        }
        WRITE_BITS_HUFF(codes[symbol], lens[symbol])
      }
      while (mtfs < mtf_lim);
    }
    // Restore_from_Local:
    m_OutStreamCurrent._bitPos = _bitPos;
    m_OutStreamCurrent._curByte = _curByte;
    m_OutStreamCurrent._buf = _buf;

    if (!m_OptimizeNumTables)
      break;
    const UInt32 price = m_OutStreamCurrent.GetPos() - startPos;
    if (price <= bestPrice)
    {
      if (nt == kNumTablesMax)
        break;
      bestPrice = price;
      bestNumTables = nt;
    }
  }
}


// blockSize > 0
UInt32 CThreadInfo::EncodeBlockWithHeaders(const Byte *block, UInt32 blockSize)
{
  WriteByte2(kBlockSig0);
  WriteByte2(kBlockSig1);
  WriteByte2(kBlockSig2);
  WriteByte2(kBlockSig3);
  WriteByte2(kBlockSig4);
  WriteByte2(kBlockSig5);

  CBZip2Crc crc;
  const Byte * const lim = block + blockSize;
  unsigned b = *block++;
  crc.UpdateByte(b);
  for (;;)
  {
    const unsigned prev = b;
    if (block >= lim) { break; } b = *block++;  crc.UpdateByte(b);  if (prev != b) continue;
    if (block >= lim) { break; } b = *block++;  crc.UpdateByte(b);  if (prev != b) continue;
    if (block >= lim) { break; } b = *block++;  crc.UpdateByte(b);  if (prev != b) continue;
    if (block >= lim) { break; } b = *block++;  if (b) do crc.UpdateByte(prev); while (--b);
    if (block >= lim) { break; } b = *block++;  crc.UpdateByte(b);
  }
  const UInt32 crcRes = crc.GetDigest();
  for (int i = 24; i >= 0; i -= 8)
    WriteByte2((Byte)(crcRes >> i));
  EncodeBlock(lim - blockSize, blockSize);
  return crcRes;
}


void CThreadInfo::EncodeBlock2(const Byte *block, UInt32 blockSize, UInt32 numPasses)
{
  const UInt32 numCrcs = m_NumCrcs;

  const UInt32 startBytePos = m_OutStreamCurrent.GetBytePos();
  const UInt32 startPos = m_OutStreamCurrent.GetPos();
  const unsigned startCurByte = m_OutStreamCurrent.GetCurByte();
  unsigned endCurByte = 0;
  UInt32 endPos = 0; // 0 means no no additional passes
  if (numPasses > 1 && blockSize >= (1 << 10))
  {
    UInt32 bs0 = blockSize / 2;
    for (; bs0 < blockSize &&
           (block[        bs0    ] ==
            block[(size_t)bs0 - 1] ||
            block[(size_t)bs0 - 1] ==
            block[(size_t)bs0 - 2]);
      bs0++)
    {}
    
    if (bs0 < blockSize)
    {
      EncodeBlock2(block, bs0, numPasses - 1);
      EncodeBlock2(block + bs0, blockSize - bs0, numPasses - 1);
      endPos = m_OutStreamCurrent.GetPos();
      endCurByte = m_OutStreamCurrent.GetCurByte();
      // we prepare next byte as identical byte to starting byte for main encoding attempt:
      if (endPos & 7)
        WriteBits2(0, 8 - (endPos & 7));
      m_OutStreamCurrent.SetCurState((startPos & 7), startCurByte);
    }
  }

  const UInt32 startBytePos2 = m_OutStreamCurrent.GetBytePos();
  const UInt32 startPos2 = m_OutStreamCurrent.GetPos();
  const UInt32 crcVal = EncodeBlockWithHeaders(block, blockSize);

  if (endPos)
  {
    const UInt32 size2 = m_OutStreamCurrent.GetPos() - startPos2;
    if (size2 >= endPos - startPos)
    {
      m_OutStreamCurrent.SetPos(endPos);
      m_OutStreamCurrent.SetCurState((endPos & 7), endCurByte);
      return;
    }
    const UInt32 numBytes = m_OutStreamCurrent.GetBytePos() - startBytePos2;
    Byte * const buffer = m_OutStreamCurrent.GetStream();
    memmove(buffer + startBytePos, buffer + startBytePos2, numBytes);
    m_OutStreamCurrent.SetPos(startPos + size2);
    // we don't call m_OutStreamCurrent.SetCurState() here because
    // m_OutStreamCurrent._curByte is correct already
  }
  m_CRCs[numCrcs] = crcVal;
  m_NumCrcs = numCrcs + 1;
}


HRESULT CThreadInfo::EncodeBlock3(UInt32 blockSize)
{
  CMsbfEncoderTemp &outStreamTemp = m_OutStreamCurrent;
  outStreamTemp.SetStream(m_TempArray);
  outStreamTemp.Init();
  m_NumCrcs = 0;

  EncodeBlock2(m_Block, blockSize, Encoder->_props.NumPasses);

#ifndef Z7_ST
  if (Encoder->MtMode)
    Encoder->ThreadsInfo[m_BlockIndex].CanWriteEvent.Lock();
#endif

  for (UInt32 i = 0; i < m_NumCrcs; i++)
    Encoder->CombinedCrc.Update(m_CRCs[i]);
  Encoder->WriteBytes(m_TempArray, outStreamTemp.GetPos(), outStreamTemp.GetNonFlushedByteBits());
  HRESULT res = S_OK;

#ifndef Z7_ST
  if (Encoder->MtMode)
  {
    UInt32 blockIndex = m_BlockIndex + 1;
    if (blockIndex == Encoder->NumThreads)
      blockIndex = 0;
    if (Encoder->Progress)
    {
      const UInt64 packSize = Encoder->m_OutStream.GetProcessedSize();
      res = Encoder->Progress->SetRatioInfo(&m_UnpackSize, &packSize);
    }
    Encoder->ThreadsInfo[blockIndex].CanWriteEvent.Set();
  }
#endif
  return res;
}

void CEncoder::WriteBytes(const Byte *data, UInt32 sizeInBits, unsigned lastByteBits)
{
  m_OutStream.WriteBytes(data, sizeInBits >> 3);
  sizeInBits &= 7;
  if (sizeInBits)
    m_OutStream.WriteBits(lastByteBits, sizeInBits);
}


HRESULT CEncoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress)
{
  NumBlocks = 0;
#ifndef Z7_ST
  Progress = progress;
  ThreadNextGroup_Init(&ThreadNextGroup, _props.NumThreadGroups, 0); // startGroup
  RINOK(Create())
  for (UInt32 t = 0; t < NumThreads; t++)
#endif
  {
    #ifndef Z7_ST
    CThreadInfo &ti = ThreadsInfo[t];
    if (MtMode)
    {
      WRes             wres = ti.StreamWasFinishedEvent.Reset();
      if (wres == 0) { wres = ti.WaitingWasStartedEvent.Reset();
      if (wres == 0) { wres = ti.CanWriteEvent.Reset(); }}
      if (wres != 0)
        return HRESULT_FROM_WIN32(wres);
    }
    #else
    CThreadInfo &ti = ThreadsInfo;
    ti.Encoder = this;
    #endif

    ti.m_OptimizeNumTables = _props.DoOptimizeNumTables();

    if (!ti.Alloc())
      return E_OUTOFMEMORY;
  }


  if (!m_InStream.Create(kBufferSize))
    return E_OUTOFMEMORY;
  if (!m_OutStream.Create(kBufferSize))
    return E_OUTOFMEMORY;


  m_InStream.SetStream(inStream);
  m_InStream.Init();

  m_OutStream.SetStream(outStream);
  m_OutStream.Init();

  CombinedCrc.Init();
  #ifndef Z7_ST
  NextBlockIndex = 0;
  StreamWasFinished = false;
  CloseThreads = false;
  CanStartWaitingEvent.Reset();
  #endif

  WriteByte(kArSig0);
  WriteByte(kArSig1);
  WriteByte(kArSig2);
  WriteByte((Byte)(kArSig3 + _props.BlockSizeMult));

  #ifndef Z7_ST

  if (MtMode)
  {
    ThreadsInfo[0].CanWriteEvent.Set();
    Result = S_OK;
    CanProcessEvent.Set();
    UInt32 t;
    for (t = 0; t < NumThreads; t++)
      ThreadsInfo[t].StreamWasFinishedEvent.Lock();
    CanProcessEvent.Reset();
    CanStartWaitingEvent.Set();
    for (t = 0; t < NumThreads; t++)
      ThreadsInfo[t].WaitingWasStartedEvent.Lock();
    CanStartWaitingEvent.Reset();
    RINOK(Result)
  }
  else
  #endif
  {
    for (;;)
    {
      CThreadInfo &ti =
      #ifndef Z7_ST
          ThreadsInfo[0];
      #else
          ThreadsInfo;
      #endif
      const UInt32 blockSize = ReadRleBlock(ti.m_Block);
      if (blockSize == 0)
        break;
      RINOK(ti.EncodeBlock3(blockSize))
      if (progress)
      {
        const UInt64 unpackSize = m_InStream.GetProcessedSize();
        const UInt64 packSize = m_OutStream.GetProcessedSize();
        RINOK(progress->SetRatioInfo(&unpackSize, &packSize))
      }
    }
  }
  WriteByte(kFinSig0);
  WriteByte(kFinSig1);
  WriteByte(kFinSig2);
  WriteByte(kFinSig3);
  WriteByte(kFinSig4);
  WriteByte(kFinSig5);
  {
    const UInt32 v = CombinedCrc.GetDigest();
    for (int i = 24; i >= 0; i -= 8)
      WriteByte((Byte)(v >> i));
  }
  RINOK(Flush())
  if (!m_InStream.WasFinished())
    return E_FAIL;
  return S_OK;
}

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
{
  int level = -1;
  CEncProps props;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];

    if (propID == NCoderPropID::kAffinity)
    {
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      props.Affinity = prop.uhVal.QuadPart;
      continue;
    }

    if (propID == NCoderPropID::kNumThreadGroups)
    {
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      props.NumThreadGroups = (UInt32)prop.ulVal;
      continue;
    }

    if (propID >= NCoderPropID::kReduceSize)
      continue;
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    const UInt32 v = (UInt32)prop.ulVal;
    switch (propID)
    {
      case NCoderPropID::kNumPasses: props.NumPasses = v; break;
      case NCoderPropID::kDictionarySize: props.BlockSizeMult = v / kBlockSizeStep; break;
      case NCoderPropID::kLevel: level = (int)v; break;
      case NCoderPropID::kNumThreads:
      {
        #ifndef Z7_ST
        SetNumberOfThreads(v);
        #endif
        break;
      }
      default: return E_INVALIDARG;
    }
  }
  props.Normalize(level);
  _props = props;
  return S_OK;
}

#ifndef Z7_ST
Z7_COM7F_IMF(CEncoder::SetNumberOfThreads(UInt32 numThreads))
{
  const UInt32 kNumThreadsMax = 64;
  if (numThreads < 1) numThreads = 1;
  if (numThreads > kNumThreadsMax) numThreads = kNumThreadsMax;
  NumThreads = numThreads;
  return S_OK;
}
#endif

}}

/* ================ unit: CPP/7zip/Compress/Bcj2Coder.cpp ================ */
// Bcj2Coder.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBcj2 {

CBaseCoder::CBaseCoder()
{
  for (unsigned i = 0; i < BCJ2_NUM_STREAMS + 1; i++)
  {
    _bufs[i] = NULL;
    _bufsSizes[i] = 0;
    _bufsSizes_New[i] = (1 << 18);
  }
}

CBaseCoder::~CBaseCoder()
{
  for (unsigned i = 0; i < BCJ2_NUM_STREAMS + 1; i++)
    ::MidFree(_bufs[i]);
}

HRESULT CBaseCoder::Alloc(bool allocForOrig)
{
  const unsigned num = allocForOrig ? BCJ2_NUM_STREAMS + 1 : BCJ2_NUM_STREAMS;
  for (unsigned i = 0; i < num; i++)
  {
    UInt32 size = _bufsSizes_New[i];
    /* buffer sizes for BCJ2_STREAM_CALL and BCJ2_STREAM_JUMP streams
       must be aligned for 4 */
    size &= ~(UInt32)3;
    const UInt32 kMinBufSize = 4;
    if (size < kMinBufSize)
      size = kMinBufSize;
    // size = 4 * 100; // for debug
    // if (BCJ2_IS_32BIT_STREAM(i) == 1) size = 4 * 1; // for debug
    if (!_bufs[i] || size != _bufsSizes[i])
    {
      if (_bufs[i])
      {
        ::MidFree(_bufs[i]);
        _bufs[i] = NULL;
      }
      _bufsSizes[i] = 0;
      Byte *buf = (Byte *)::MidAlloc(size);
      if (!buf)
        return E_OUTOFMEMORY;
      _bufs[i] = buf;
      _bufsSizes[i] = size;
    }
  }
  return S_OK;
}



#ifndef Z7_EXTRACT_ONLY

CEncoder::CEncoder():
    _relatLim(BCJ2_ENC_RELAT_LIMIT_DEFAULT)
    // , _excludeRangeBits(BCJ2_RELAT_EXCLUDE_NUM_BITS)
    {}
CEncoder::~CEncoder() {}

Z7_COM7F_IMF(CEncoder::SetInBufSize(UInt32, UInt32 size))
  { _bufsSizes_New[BCJ2_NUM_STREAMS] = size; return S_OK; }
Z7_COM7F_IMF(CEncoder::SetOutBufSize(UInt32 streamIndex, UInt32 size))
  { _bufsSizes_New[streamIndex] = size; return S_OK; }

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
{
  UInt32 relatLim = BCJ2_ENC_RELAT_LIMIT_DEFAULT;
  // UInt32 excludeRangeBits = BCJ2_RELAT_EXCLUDE_NUM_BITS;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = props[i];
    const PROPID propID = propIDs[i];
    if (propID >= NCoderPropID::kReduceSize
        // && propID != NCoderPropID::kHashBits
        )
      continue;
    switch (propID)
    {
      /*
      case NCoderPropID::kDefaultProp:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 v = prop.ulVal;
        if (v > 31)
          return E_INVALIDARG;
        relatLim = (UInt32)1 << v;
        break;
      }
      case NCoderPropID::kHashBits:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 v = prop.ulVal;
        if (v > 31)
          return E_INVALIDARG;
        excludeRangeBits = v;
        break;
      }
      */
      case NCoderPropID::kDictionarySize:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        relatLim = prop.ulVal;
        if (relatLim > BCJ2_ENC_RELAT_LIMIT_MAX)
          return E_INVALIDARG;
        break;
      }
      case NCoderPropID::kNumThreads:
      case NCoderPropID::kLevel:
        continue;
      default: return E_INVALIDARG;
    }
  }
  _relatLim = relatLim;
  // _excludeRangeBits = excludeRangeBits;
  return S_OK;
}


HRESULT CEncoder::CodeReal(
    ISequentialInStream * const *inStreams, const UInt64 * const *inSizes, UInt32 numInStreams,
    ISequentialOutStream * const *outStreams, const UInt64 * const * /* outSizes */, UInt32 numOutStreams,
    ICompressProgressInfo *progress)
{
  if (numInStreams != 1 || numOutStreams != BCJ2_NUM_STREAMS)
    return E_INVALIDARG;

  RINOK(Alloc())

  CBcj2Enc_ip_unsigned fileSize_minus1 = BCJ2_ENC_FileSizeField_UNLIMITED;
  if (inSizes && inSizes[0])
  {
    const UInt64 inSize = *inSizes[0];
   #ifdef BCJ2_ENC_FileSize_MAX
    if (inSize <= BCJ2_ENC_FileSize_MAX)
   #endif
      fileSize_minus1 = BCJ2_ENC_GET_FileSizeField_VAL_FROM_FileSize(inSize);
  }

  Z7_DECL_CMyComPtr_QI_FROM(ICompressGetSubStreamSize, getSubStreamSize, inStreams[0])

  CBcj2Enc enc;
  enc.src = _bufs[BCJ2_NUM_STREAMS];
  enc.srcLim = enc.src;
  {
    for (unsigned i = 0; i < BCJ2_NUM_STREAMS; i++)
    {
      enc.bufs[i] = _bufs[i];
      enc.lims[i] = _bufs[i] + _bufsSizes[i];
    }
  }
  Bcj2Enc_Init(&enc);
  enc.fileIp64 = 0;
  enc.fileSize64_minus1 = fileSize_minus1;
  enc.relatLimit = _relatLim;
  // enc.relatExcludeBits = _excludeRangeBits;
  enc.finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;

  // Varibales that correspond processed data in input stream:
  UInt64 inPos_without_Temp = 0;  // it doesn't include data in enc.temp[]
  UInt64 inPos_with_Temp = 0;     // it        includes data in enc.temp[]

  UInt64 prevProgress = 0;
  UInt64 totalRead = 0;  // size read from input stream
  UInt64 outSizeRc = 0;
  UInt64 subStream_Index = 0;
  UInt64 subStream_StartPos = 0; // global start offset of subStreams[subStream_Index]
  UInt64 subStream_Size = 0;
  const Byte *srcLim_Read = _bufs[BCJ2_NUM_STREAMS];
  bool readWasFinished = false;
  bool isAccurate = false;
  bool wasUnknownSize = false;

  for (;;)
  {
    if (readWasFinished && enc.srcLim == srcLim_Read)
      enc.finishMode = BCJ2_ENC_FINISH_MODE_END_STREAM;

    // for debug:
    // for (int y=0;y<100;y++) { CBcj2Enc enc2 = enc; Bcj2Enc_Encode(&enc2); }
    
    Bcj2Enc_Encode(&enc);

    inPos_with_Temp = totalRead - (size_t)(srcLim_Read - enc.src);
    inPos_without_Temp = inPos_with_Temp - Bcj2Enc_Get_AvailInputSize_in_Temp(&enc);
    
    // if (inPos_without_Temp != enc.ip64) return E_FAIL;

    if (Bcj2Enc_IsFinished(&enc))
      break;

    if (enc.state < BCJ2_NUM_STREAMS)
    {
      if (enc.bufs[enc.state] != enc.lims[enc.state])
        return E_FAIL;
      const size_t curSize = (size_t)(enc.bufs[enc.state] - _bufs[enc.state]);
      // printf("Write stream = %2d %6d\n", enc.state, curSize);
      RINOK(WriteStream(outStreams[enc.state], _bufs[enc.state], curSize))
      if (enc.state == BCJ2_STREAM_RC)
        outSizeRc += curSize;
      enc.bufs[enc.state] = _bufs[enc.state];
      enc.lims[enc.state] = _bufs[enc.state] + _bufsSizes[enc.state];
    }
    else
    {
      if (enc.state != BCJ2_ENC_STATE_ORIG)
        return E_FAIL;
      // (enc.state == BCJ2_ENC_STATE_ORIG)
      if (enc.src != enc.srcLim)
        return E_FAIL;
      if (enc.finishMode != BCJ2_ENC_FINISH_MODE_CONTINUE
          && Bcj2Enc_Get_AvailInputSize_in_Temp(&enc) != 0)
        return E_FAIL;

      if (enc.src == srcLim_Read)
      {
        if (readWasFinished)
          return E_FAIL;
        UInt32 curSize = _bufsSizes[BCJ2_NUM_STREAMS];
        RINOK(inStreams[0]->Read(_bufs[BCJ2_NUM_STREAMS], curSize, &curSize))
        // printf("Read %6u bytes\n", curSize);
        if (curSize == 0)
          readWasFinished = true;
        totalRead += curSize;
        enc.src     = _bufs[BCJ2_NUM_STREAMS];
        srcLim_Read = _bufs[BCJ2_NUM_STREAMS] + curSize;
      }
      enc.srcLim = srcLim_Read;

      if (getSubStreamSize)
      {
        /* we set base default conversions options that will be used,
           if subStream related options will be not OK */
        enc.fileIp64 = 0;
        enc.fileSize64_minus1 = fileSize_minus1;
        for (;;)
        {
          UInt64 nextPos;
          if (isAccurate)
            nextPos = subStream_StartPos + subStream_Size;
          else
          {
            const HRESULT hres = getSubStreamSize->GetSubStreamSize(subStream_Index, &subStream_Size);
            if (hres != S_OK)
            {
              enc.finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
              /* if sub-stream size is unknown, we use default settings.
                 We still can recover to normal mode for next sub-stream,
                 if GetSubStreamSize() will return S_OK, when current
                 sub-stream will be finished.
              */
              if (hres == S_FALSE)
              {
                wasUnknownSize = true;
                break;
              }
              if (hres == E_NOTIMPL)
              {
                getSubStreamSize.Release();
                break;
              }
              return hres;
            }
            // printf("GetSubStreamSize %6u : %6u \n", (unsigned)subStream_Index, (unsigned)subStream_Size);
            nextPos = subStream_StartPos + subStream_Size;
            if ((Int64)subStream_Size == -1)
            {
              /* it's not expected, but (-1) can mean unknown size. */
              enc.finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
              wasUnknownSize = true;
              break;
            }
            if (nextPos < subStream_StartPos)
              return E_FAIL;
            isAccurate =
                 (nextPos <  totalRead
              || (nextPos <= totalRead && readWasFinished));
          }
          
          /* (nextPos) is estimated end position of current sub_stream.
             But only (totalRead) and (readWasFinished) values
             can confirm that this estimated end position is accurate.
             That end position is accurate, if it can't be changed in
             further calls of GetSubStreamSize() */

          /* (nextPos < inPos_with_Temp) is unexpected case here, that we
               can get if from some incorrect ICompressGetSubStreamSize object,
               where new GetSubStreamSize() call returns smaller size than
               confirmed by Read() size from previous GetSubStreamSize() call.
          */
          if (nextPos < inPos_with_Temp)
          {
            if (wasUnknownSize)
            {
              /* that case can be complicated for recovering.
                 so we disable sub-streams requesting. */
              enc.finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
              getSubStreamSize.Release();
              break;
            }
            return E_FAIL; // to stop after failure
          }

          if (nextPos <= inPos_with_Temp)
          {
            // (nextPos == inPos_with_Temp)
            /* CBcj2Enc encoder requires to finish each [non-empty] block (sub-stream)
                  with BCJ2_ENC_FINISH_MODE_END_BLOCK
               or with BCJ2_ENC_FINISH_MODE_END_STREAM for last block:
               And we send data of new block to CBcj2Enc, only if previous block was finished.
               So we switch to next sub-stream if after Bcj2Enc_Encode() call we have
                 && (enc.finishMode != BCJ2_ENC_FINISH_MODE_CONTINUE)
                 && (nextPos == inPos_with_Temp)
                 && (enc.state == BCJ2_ENC_STATE_ORIG)
            */
            if (enc.finishMode != BCJ2_ENC_FINISH_MODE_CONTINUE)
            {
              /* subStream_StartPos is increased only here.
                   (subStream_StartPos == inPos_with_Temp) : at start
                   (subStream_StartPos <= inPos_with_Temp) : will be later
              */
              subStream_StartPos = nextPos;
              subStream_Size = 0;
              wasUnknownSize = false;
              subStream_Index++;
              isAccurate = false;
              // we don't change finishMode here
              continue;
            }
          }
          
          enc.finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
          /* for (!isAccurate) case:
             (totalRead <= real_end_of_subStream)
             so we can use BCJ2_ENC_FINISH_MODE_CONTINUE up to (totalRead)
             // we don't change settings at the end of substream, if settings were unknown,
          */
         
          /* if (wasUnknownSize) then we can't trust size of that sub-stream.
             so we use default settings instead */
          if (!wasUnknownSize)
         #ifdef BCJ2_ENC_FileSize_MAX
          if (subStream_Size <= BCJ2_ENC_FileSize_MAX)
         #endif
          {
            enc.fileIp64 =
                (CBcj2Enc_ip_unsigned)(
                (CBcj2Enc_ip_signed)enc.ip64 +
                (CBcj2Enc_ip_signed)(subStream_StartPos - inPos_without_Temp));
            Bcj2Enc_SET_FileSize(&enc, subStream_Size)
          }

          if (isAccurate)
          {
            /* (real_end_of_subStream == nextPos <= totalRead)
               So we can use BCJ2_ENC_FINISH_MODE_END_BLOCK up to (nextPos). */
            const size_t rem = (size_t)(totalRead - nextPos);
            if ((size_t)(enc.srcLim - enc.src) < rem)
              return E_FAIL;
            enc.srcLim -= rem;
            enc.finishMode = BCJ2_ENC_FINISH_MODE_END_BLOCK;
          }

          break;
        } // for() loop
      } // getSubStreamSize
    }

    if (progress && inPos_without_Temp - prevProgress >= (1 << 22))
    {
      prevProgress = inPos_without_Temp;
      const UInt64 outSize2 = inPos_without_Temp + outSizeRc +
          (size_t)(enc.bufs[BCJ2_STREAM_RC] - _bufs[BCJ2_STREAM_RC]);
      // printf("progress %8u, %8u\n", (unsigned)inSize2, (unsigned)outSize2);
      RINOK(progress->SetRatioInfo(&inPos_without_Temp, &outSize2))
    }
  }

  for (unsigned i = 0; i < BCJ2_NUM_STREAMS; i++)
  {
    RINOK(WriteStream(outStreams[i], _bufs[i], (size_t)(enc.bufs[i] - _bufs[i])))
  }
  // if (inPos_without_Temp != subStream_StartPos + subStream_Size) return E_FAIL;
  return S_OK;
}


Z7_COM7F_IMF(CEncoder::Code(
    ISequentialInStream * const *inStreams, const UInt64 * const *inSizes, UInt32 numInStreams,
    ISequentialOutStream * const *outStreams, const UInt64 * const *outSizes, UInt32 numOutStreams,
    ICompressProgressInfo *progress))
{
  try
  {
    return CodeReal(inStreams, inSizes, numInStreams, outStreams, outSizes,numOutStreams, progress);
  }
  catch(...) { return E_FAIL; }
}

#endif






CDecoder::CDecoder():
    _finishMode(false)
#ifndef Z7_NO_READ_FROM_CODER
    , _outSizeDefined(false)
    , _outSize(0)
    , _outSize_Processed(0)
#endif
{}

Z7_COM7F_IMF(CDecoder::SetInBufSize(UInt32 streamIndex, UInt32 size))
  { _bufsSizes_New[streamIndex] = size; return S_OK; }
Z7_COM7F_IMF(CDecoder::SetOutBufSize(UInt32, UInt32 size))
  { _bufsSizes_New[BCJ2_NUM_STREAMS] = size; return S_OK; }

Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  _finishMode = (finishMode != 0);
  return S_OK;
}

void CBaseDecoder::InitCommon()
{
  for (unsigned i = 0; i < BCJ2_NUM_STREAMS; i++)
  {
    dec.lims[i] = dec.bufs[i] = _bufs[i];
    _readRes[i] = S_OK;
    _extraSizes[i] = 0;
    _readSizes[i] = 0;
  }
  Bcj2Dec_Init(&dec);
}


/* call ReadInStream() only after Bcj2Dec_Decode().
   input requirement:
      (dec.state < BCJ2_NUM_STREAMS)
*/
void CBaseDecoder::ReadInStream(ISequentialInStream *inStream)
{
  const unsigned state = dec.state;
  UInt32 total;
  {
    Byte *buf = _bufs[state];
    const Byte *cur = dec.bufs[state];
    // if (cur != dec.lims[state]) throw 1; // unexpected case
    dec.lims[state] =
    dec.bufs[state] = buf;
    total = (UInt32)_extraSizes[state];
    for (UInt32 i = 0; i < total; i++)
      buf[i] = cur[i];
  }
  
  if (_readRes[state] != S_OK)
    return;
  
  do
  {
    UInt32 curSize = _bufsSizes[state] - total;
    // if (state == 0) curSize = 0; // for debug
    // curSize = 7; // for debug
    /* even if we have reached provided inSizes[state] limit,
       we call Read() with (curSize != 0), because
       we want the called handler of stream->Read() could
       execute required Init/Flushing code even for empty stream.
       In another way we could call Read() with (curSize == 0) for
       finished streams, but some Read() handlers can ignore Read(size=0) calls.
    */
    const HRESULT hres = inStream->Read(_bufs[state] + total, curSize, &curSize);
    _readRes[state] = hres;
    if (curSize == 0)
      break;
    _readSizes[state] += curSize;
    total += curSize;
    if (hres != S_OK)
      break;
  }
  while (total < 4 && BCJ2_IS_32BIT_STREAM(state));
  
  /* we exit from decoding loop here, if we can't
     provide new data for input stream.
     Usually it's normal exit after full stream decoding. */
  if (total == 0)
    return;
  
  if (BCJ2_IS_32BIT_STREAM(state))
  {
    const unsigned extra = (unsigned)total & 3;
    _extraSizes[state] = extra;
    if (total < 4)
    {
      if (_readRes[state] == S_OK)
        _readRes[state] = S_FALSE; // actually it's stream error. So maybe we need another error code.
      return;
    }
    total -= (UInt32)extra;
  }
  
  dec.lims[state] += total; // = _bufs[state] + total;
}


Z7_COM7F_IMF(CDecoder::Code(
    ISequentialInStream * const *inStreams, const UInt64 * const *inSizes, UInt32 numInStreams,
    ISequentialOutStream * const *outStreams, const UInt64 * const *outSizes, UInt32 numOutStreams,
    ICompressProgressInfo *progress))
{
  if (numInStreams != BCJ2_NUM_STREAMS || numOutStreams != 1)
    return E_INVALIDARG;

  RINOK(Alloc())
  InitCommon();

  dec.destLim = dec.dest = _bufs[BCJ2_NUM_STREAMS];
  
  UInt64 outSizeWritten = 0;
  UInt64 prevProgress = 0;

  HRESULT hres_Crit = S_OK;  // critical hres status (mostly from input stream reading)
  HRESULT hres_Weak = S_OK;  // first non-critical error code from input stream reading

  for (;;)
  {
    if (Bcj2Dec_Decode(&dec) != SZ_OK)
    {
      /* it's possible only at start (first 5 bytes in RC stream) */
      hres_Crit = S_FALSE;
      break;
    }
    if (dec.state < BCJ2_NUM_STREAMS)
    {
      ReadInStream(inStreams[dec.state]);
      const unsigned state = dec.state;
      const HRESULT hres = _readRes[state];
      if (dec.lims[state] == _bufs[state])
      {
        // we break decoding, if there are no new data in input stream
        hres_Crit = hres;
        break;
      }
      if (hres != S_OK && hres_Weak == S_OK)
        hres_Weak = hres;
    }
    else  // (BCJ2_DEC_STATE_ORIG_0 <= state <= BCJ2_STATE_ORIG)
    {
      {
        const size_t curSize = (size_t)(dec.dest - _bufs[BCJ2_NUM_STREAMS]);
        if (curSize != 0)
        {
          outSizeWritten += curSize;
          RINOK(WriteStream(outStreams[0], _bufs[BCJ2_NUM_STREAMS], curSize))
        }
      }
      {
        UInt32 rem = _bufsSizes[BCJ2_NUM_STREAMS];
        if (outSizes && outSizes[0])
        {
          const UInt64 outSize = *outSizes[0] - outSizeWritten;
          if (rem > outSize)
            rem = (UInt32)outSize;
        }
        dec.dest = _bufs[BCJ2_NUM_STREAMS];
        dec.destLim = dec.dest + rem;
        /* we exit from decoding loop here,
           if (outSizes[0]) limit for output stream was reached */
        if (rem == 0)
          break;
      }
    }

    if (progress)
    {
      // here we don't count additional data in dec.temp (up to 4 bytes for output stream)
      const UInt64 processed = outSizeWritten + (size_t)(dec.dest - _bufs[BCJ2_NUM_STREAMS]);
      if (processed - prevProgress >= (1 << 24))
      {
        prevProgress = processed;
        const UInt64 inSize = processed +
            _readSizes[BCJ2_STREAM_RC] - (size_t)(
              dec.lims[BCJ2_STREAM_RC] -
              dec.bufs[BCJ2_STREAM_RC]);
        RINOK(progress->SetRatioInfo(&inSize, &prevProgress))
      }
    }
  }

  {
    const size_t curSize = (size_t)(dec.dest - _bufs[BCJ2_NUM_STREAMS]);
    if (curSize != 0)
    {
      outSizeWritten += curSize;
      RINOK(WriteStream(outStreams[0], _bufs[BCJ2_NUM_STREAMS], curSize))
    }
  }

  if (hres_Crit == S_OK) hres_Crit = hres_Weak;
  if (hres_Crit != S_OK) return hres_Crit;

  if (_finishMode)
  {
    if (!Bcj2Dec_IsMaybeFinished_code(&dec))
      return S_FALSE;

    /* here we support two correct ways to finish full stream decoding
       with one of the following conditions:
          - the end of input  stream MAIN was reached
          - the end of output stream ORIG was reached
       Currently 7-Zip/7z code ends with (state == BCJ2_STREAM_MAIN),
       because the sizes of MAIN and ORIG streams are known and these
       sizes are stored in 7z archive headers.
       And Bcj2Dec_Decode() exits with (state == BCJ2_STREAM_MAIN),
       if both MAIN and ORIG streams have reached buffers limits.
       But if the size of MAIN stream is not known or if the
       size of MAIN stream includes some padding after payload data,
       then we still can correctly finish decoding with
       (state == BCJ2_DEC_STATE_ORIG), if we know the exact size
       of output ORIG stream.
    */
    if (dec.state != BCJ2_STREAM_MAIN)
    if (dec.state != BCJ2_DEC_STATE_ORIG)
      return S_FALSE;

    /* the caller also will know written size.
       So the following check is optional: */
    if (outSizes && outSizes[0] && *outSizes[0] != outSizeWritten)
      return S_FALSE;

    if (inSizes)
    {
      for (unsigned i = 0; i < BCJ2_NUM_STREAMS; i++)
      {
        /* if (inSizes[i]) is defined, we do full check for processed stream size. */
        if (inSizes[i] && *inSizes[i] != GetProcessedSize_ForInStream(i))
          return S_FALSE;
      }
    }

    /* v23.02: we call Read(0) for BCJ2_STREAM_CALL and BCJ2_STREAM_JUMP streams,
       if there were no Read() calls for such stream.
       So the handlers of these input streams objects can do
       Init/Flushing even for case when stream is empty:
    */
    for (unsigned i = BCJ2_STREAM_CALL; i < BCJ2_STREAM_CALL + 2; i++)
    {
      if (_readSizes[i])
        continue;
      Byte b;
      UInt32 processed;
      RINOK(inStreams[i]->Read(&b, 0, &processed))
    }
  }

  return S_OK;
}


Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize2(UInt32 streamIndex, UInt64 *value))
{
  *value = GetProcessedSize_ForInStream(streamIndex);
  return S_OK;
}


#ifndef Z7_NO_READ_FROM_CODER

Z7_COM7F_IMF(CDecoder::SetInStream2(UInt32 streamIndex, ISequentialInStream *inStream))
{
  _inStreams[streamIndex] = inStream;
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::ReleaseInStream2(UInt32 streamIndex))
{
  _inStreams[streamIndex].Release();
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::SetOutStreamSize(const UInt64 *outSize))
{
  _outSizeDefined = (outSize != NULL);
  _outSize = 0;
  if (_outSizeDefined)
    _outSize = *outSize;
  _outSize_Processed = 0;

  const HRESULT res = Alloc(false); // allocForOrig
  InitCommon();
  dec.destLim = dec.dest = NULL;
  return res;
}


Z7_COM7F_IMF(CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;

  /* Note the case:
     The output (ORIG) stream can be empty.
     But BCJ2_STREAM_RC stream always is not empty.
     And we want to support full data processing for all streams.
     We disable check (size == 0) here.
     So if the caller calls this CDecoder::Read() with (size == 0),
     we execute required Init/Flushing code in this CDecoder object.
     Also this CDecoder::Read() function will call Read() for input streams.
     So the handlers of input streams objects also can do Init/Flushing.
  */
  // if (size == 0) return S_OK;  // disabled to allow (size == 0) processing

  UInt32 totalProcessed = 0;
 
  if (_outSizeDefined)
  {
    const UInt64 rem = _outSize - _outSize_Processed;
    if (size > rem)
      size = (UInt32)rem;
  }
  dec.dest = (Byte *)data;
  dec.destLim = (const Byte *)data + size;

  HRESULT res = S_OK;

  for (;;)
  {
    if (Bcj2Dec_Decode(&dec) != SZ_OK)
      return S_FALSE;  // this error can be only at start of stream
    {
      const UInt32 curSize = (UInt32)(size_t)(dec.dest - (Byte *)data);
      if (curSize != 0)
      {
        data = (void *)((Byte *)data + curSize);
        size -= curSize;
        _outSize_Processed += curSize;
        totalProcessed += curSize;
        if (processedSize)
          *processedSize = totalProcessed;
      }
    }
    if (dec.state >= BCJ2_NUM_STREAMS)
      break;
    ReadInStream(_inStreams[dec.state]);
    if (dec.lims[dec.state] == _bufs[dec.state])
    {
      /* we break decoding, if there are no new data in input stream.
         and we ignore error code, if some data were written to output buffer. */
      if (totalProcessed == 0)
        res = _readRes[dec.state];
      break;
    }
  }

  if (res == S_OK)
  if (_finishMode && _outSizeDefined && _outSize == _outSize_Processed)
  {
    if (!Bcj2Dec_IsMaybeFinished_code(&dec))
      return S_FALSE;
    if (dec.state != BCJ2_STREAM_MAIN)
    if (dec.state != BCJ2_DEC_STATE_ORIG)
      return S_FALSE;
  }

  return res;
}

#endif

}}


/*
extern "C"
{
extern UInt32 bcj2_stats[256 + 2][2];
}

static class CBcj2Stat
{
public:
  ~CBcj2Stat()
  {
    printf("\nBCJ2 stat:");
    unsigned sums[2] = { 0, 0 };
    int i;
    for (i = 2; i < 256 + 2; i++)
    {
      sums[0] += bcj2_stats[i][0];
      sums[1] += bcj2_stats[i][1];
    }
    const unsigned sums2 = sums[0] + sums[1];
    for (int vi = 0; vi < 256 + 3; vi++)
    {
      printf("\n");
      UInt32 n0, n1;
      if (vi < 4)
        printf("\n");
      
      if (vi < 2)
        i = vi;
      else if (vi == 2)
        i = -1;
      else
        i = vi - 1;
  
      if (i < 0)
      {
        n0 = sums[0];
        n1 = sums[1];
        printf("calls   :");
      }
      else
      {
        if (i == 0)
          printf("jcc     :");
        else if (i == 1)
          printf("jump    :");
        else
          printf("call %02x :", i - 2);
        n0 = bcj2_stats[i][0];
        n1 = bcj2_stats[i][1];
      }
      
      const UInt32 sum = n0 + n1;
      printf(" %10u", sum);

    #define PRINT_PERC(val, sum) \
        { UInt32 _sum  = sum; if (_sum == 0) _sum = 1; \
        printf(" %7.3f %%", (double)((double)val * (double)100 / (double)_sum )); }

      if (i >= 2 || i < 0)
      {
        PRINT_PERC(sum, sums2);
      }
      else
        printf("%10s", "");

      printf(" :%10u", n0);
      PRINT_PERC(n0, sum);

      printf(" :%10u", n1);
      PRINT_PERC(n1, sum);
    }
    printf("\n\n");
    fflush(stdout);
  }
} g_CBcjStat;
*/

/* ================ unit: CPP/7zip/Compress/BcjCoder.cpp ================ */
// BcjCoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBcj {

Z7_COM7F_IMF(CCoder2::Init())
{
  _pc = 0;
  _state = Z7_BRANCH_CONV_ST_X86_STATE_INIT_VAL;
  return S_OK;
}

Z7_COM7F_IMF2(UInt32, CCoder2::Filter(Byte *data, UInt32 size))
{
  const UInt32 processed = (UInt32)(size_t)(_convFunc(data, size, _pc, &_state) - data);
  _pc += processed;
  return processed;
}

}}

/* ================ unit: CPP/7zip/Compress/BitlDecoder.cpp ================ */
// BitlDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NBitl {

#if defined(Z7_BITL_USE_REVERSE_BITS_TABLE)

MY_ALIGN(64)
Byte kReverseTable[256];

static
struct CReverseerTableInitializer
{
  CReverseerTableInitializer()
  {
    for (unsigned i = 0; i < 256; i++)
    {
      unsigned
      x = ((i & 0x55) << 1) | ((i >> 1) & 0x55);
      x = ((x & 0x33) << 2) | ((x >> 2) & 0x33);
      kReverseTable[i] = (Byte)((x << 4) | (x >> 4));
    }
  }
} g_ReverseerTableInitializer;

#elif 0
unsigned ReverseBits8test(unsigned i) { return ReverseBits8(i); }
#endif
}

/* ================ unit: CPP/7zip/Compress/BranchMisc.cpp ================ */
// BranchMisc.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NBranch {

Z7_COM7F_IMF(CCoder::Init())
{
  _pc = 0;
  return S_OK;
}


Z7_COM7F_IMF2(UInt32, CCoder::Filter(Byte *data, UInt32 size))
{
  const UInt32 processed = (UInt32)(size_t)(BraFunc(data, size, _pc) - data);
  _pc += processed;
  return processed;
}


#ifndef Z7_EXTRACT_ONLY

Z7_COM7F_IMF(CEncoder::Init())
{
  _pc = _pc_Init;
  return S_OK;
}

Z7_COM7F_IMF2(UInt32, CEncoder::Filter(Byte *data, UInt32 size))
{
  const UInt32 processed = (UInt32)(size_t)(BraFunc(data, size, _pc) - data);
  _pc += processed;
  return processed;
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
{
  UInt32 pc = 0;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPID propID = propIDs[i];
    if (propID == NCoderPropID::kDefaultProp ||
        propID == NCoderPropID::kBranchOffset)
    {
      const PROPVARIANT &prop = props[i];
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      pc = prop.ulVal;
      if (pc & _alignment)
        return E_INVALIDARG;
    }
  }
  _pc_Init = pc;
  return S_OK;
}


Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  if (_pc_Init == 0)
    return S_OK;
  UInt32 buf32[1];
  SetUi32(buf32, _pc_Init)
  return WriteStream(outStream, buf32, 4);
}

#endif


Z7_COM7F_IMF(CDecoder::Init())
{
  _pc = _pc_Init;
  return S_OK;
}

Z7_COM7F_IMF2(UInt32, CDecoder::Filter(Byte *data, UInt32 size))
{
  const UInt32 processed = (UInt32)(size_t)(BraFunc(data, size, _pc) - data);
  _pc += processed;
  return processed;
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *props, UInt32 size))
{
  UInt32 val = 0;
  if (size != 0)
  {
    if (size != 4)
      return E_NOTIMPL;
    val = GetUi32(props);
    if (val & _alignment)
      return E_NOTIMPL;
  }
  _pc_Init = val;
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Compress/CopyCoder.cpp ================ */
// Compress/CopyCoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {

static const UInt32 kBufSize = 1 << 17;

CCopyCoder::~CCopyCoder()
{
  ::MidFree(_buf);
}

Z7_COM7F_IMF(CCopyCoder::SetFinishMode(UInt32 /* finishMode */))
{
  return S_OK;
}

Z7_COM7F_IMF(CCopyCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize,
    ICompressProgressInfo *progress))
{
  if (!_buf)
  {
    _buf = (Byte *)::MidAlloc(kBufSize);
    if (!_buf)
      return E_OUTOFMEMORY;
  }

  TotalSize = 0;
  
  for (;;)
  {
    UInt32 size = kBufSize;
    if (outSize)
    {
      const UInt64 rem = *outSize - TotalSize;
      if (size > rem)
      {
        size = (UInt32)rem;
        if (size == 0)
        {
          /* if we enable the following check,
             we will make one call of Read(_buf, 0) for empty stream */
          // if (TotalSize != 0)
          return S_OK;
        }
      }
    }
    
    HRESULT readRes;
    {
      UInt32 pos = 0;
      do
      {
        const UInt32 curSize = size - pos;
        UInt32 processed = 0;
        readRes = inStream->Read(_buf + pos, curSize, &processed);
        if (processed > curSize)
          return E_FAIL; // internal code failure
        pos += processed;
        if (readRes != S_OK || processed == 0)
          break;
      }
      while (pos < kBufSize);
      size = pos;
    }

    if (size == 0)
      return readRes;

    if (outStream)
    {
      UInt32 pos = 0;
      do
      {
        const UInt32 curSize = size - pos;
        UInt32 processed = 0;
        const HRESULT res = outStream->Write(_buf + pos, curSize, &processed);
        if (processed > curSize)
          return E_FAIL; // internal code failure
        pos += processed;
        TotalSize += processed;
        RINOK(res)
        if (processed == 0)
          return E_FAIL;
      }
      while (pos < size);
    }
    else
      TotalSize += size;

    RINOK(readRes)

    if (size != kBufSize)
      return S_OK;

    if (progress && (TotalSize & (((UInt32)1 << 22) - 1)) == 0)
    {
      RINOK(progress->SetRatioInfo(&TotalSize, &TotalSize))
    }
  }
}

Z7_COM7F_IMF(CCopyCoder::SetInStream(ISequentialInStream *inStream))
{
  _inStream = inStream;
  TotalSize = 0;
  return S_OK;
}

Z7_COM7F_IMF(CCopyCoder::ReleaseInStream())
{
  _inStream.Release();
  return S_OK;
}

Z7_COM7F_IMF(CCopyCoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  UInt32 realProcessedSize = 0;
  HRESULT res = _inStream->Read(data, size, &realProcessedSize);
  TotalSize += realProcessedSize;
  if (processedSize)
    *processedSize = realProcessedSize;
  return res;
}

Z7_COM7F_IMF(CCopyCoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = TotalSize;
  return S_OK;
}

HRESULT CopyStream(ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  CMyComPtr<ICompressCoder> copyCoder = new CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, progress);
}

HRESULT CopyStream_ExactSize(ISequentialInStream *inStream, ISequentialOutStream *outStream, UInt64 size, ICompressProgressInfo *progress)
{
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  RINOK(copyCoder->Code(inStream, outStream, NULL, &size, progress))
  return copyCoderSpec->TotalSize == size ? S_OK : E_FAIL;
}

}

/* ================ unit: CPP/7zip/Compress/DeflateDecoder.cpp ================ */
// DeflateDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

CCoder::CCoder(bool deflate64Mode):
    _deflateNSIS(false),
    _deflate64Mode(deflate64Mode),
    _keepHistory(false),
    _needFinishInput(false),
    _needInitInStream(true),
    _outSizeDefined(false),
    _outStartPos(0)
    {}

UInt32 CCoder::ReadBits(unsigned numBits)
{
  return m_InBitStream.ReadBits(numBits);
}

Byte CCoder::ReadAlignedByte()
{
  return m_InBitStream.ReadAlignedByte();
}

bool CCoder::DecodeLevels(Byte *levels, unsigned numSymbols)
{
  unsigned i = 0;
  
  do
  {
    unsigned sym = m_LevelDecoder.Decode(&m_InBitStream);
    if (sym < kTableDirectLevels)
      levels[i++] = (Byte)sym;
    else
    {
      if (sym >= kLevelTableSize)
        return false;
      
      unsigned num;
      unsigned numBits;
      Byte symbol;
      
      if (sym == kTableLevelRepNumber)
      {
        if (i == 0)
          return false;
        numBits = 2;
        num = 0;
        symbol = levels[(size_t)i - 1];
      }
      else
      {
        sym -= kTableLevel0Number;
        sym <<= 2;
        numBits = 3 + (unsigned)sym;
        num = ((unsigned)sym << 1);
        symbol = 0;
      }
      
      num += i + 3 + ReadBits(numBits);
      if (num > numSymbols)
        return false;
      do
        levels[i++] = symbol;
      while (i < num);
    }
  }
  while (i < numSymbols);
  
  return true;
}

#define RIF(x) { if (!(x)) return false; }

bool CCoder::ReadTables(void)
{
  m_FinalBlock = (ReadBits(kFinalBlockFieldSize) == NFinalBlockField::kFinalBlock);
  if (m_InBitStream.ExtraBitsWereRead())
    return false;
  const UInt32 blockType = ReadBits(kBlockTypeFieldSize);
  if (blockType > NBlockType::kDynamicHuffman)
    return false;
  if (m_InBitStream.ExtraBitsWereRead())
    return false;

  if (blockType == NBlockType::kStored)
  {
    m_StoredMode = true;
    m_InBitStream.AlignToByte();
    m_StoredBlockSize = ReadAligned_UInt16(); // ReadBits(kStoredBlockLengthFieldSize)
    if (_deflateNSIS)
      return true;
    return (m_StoredBlockSize == (UInt16)~ReadAligned_UInt16());
  }

  m_StoredMode = false;

  CLevels levels;
  if (blockType == NBlockType::kFixedHuffman)
  {
    levels.SetFixedLevels();
    _numDistLevels = _deflate64Mode ? kDistTableSize64 : kDistTableSize32;
  }
  else
  {
    const unsigned numLitLenLevels = ReadBits(kNumLenCodesFieldSize) + kNumLitLenCodesMin;
    _numDistLevels = (unsigned)ReadBits(kNumDistCodesFieldSize) + kNumDistCodesMin;
    const unsigned numLevelCodes = ReadBits(kNumLevelCodesFieldSize) + kNumLevelCodesMin;

    if (!_deflate64Mode)
      if (_numDistLevels > kDistTableSize32)
        return false;
    
    const unsigned kLevelTableSize_aligned4 = kLevelTableSize + 1;
    Byte levelLevels[kLevelTableSize_aligned4];
    memset (levelLevels, 0, sizeof(levelLevels));
    unsigned i = 0;
    do
      levelLevels[kCodeLengthAlphabetOrder[i++]] = (Byte)ReadBits(kLevelFieldSize);
    while (i != numLevelCodes);
    
    if (m_InBitStream.ExtraBitsWereRead())
      return false;

    RIF(m_LevelDecoder.Build(levelLevels, false)) // full
    
    Byte tmpLevels[kFixedMainTableSize + kFixedDistTableSize];
    if (!DecodeLevels(tmpLevels, numLitLenLevels + _numDistLevels))
      return false;
    
    if (m_InBitStream.ExtraBitsWereRead())
      return false;

    levels.SubClear();
    memcpy(levels.litLenLevels, tmpLevels, numLitLenLevels);
    memcpy(levels.distLevels, tmpLevels + numLitLenLevels, _numDistLevels);
  }
  RIF(m_MainDecoder.Build(levels.litLenLevels))
  return m_DistDecoder.Build(levels.distLevels);
}


HRESULT CCoder::InitInStream(bool needInit)
{
  if (needInit)
  {
    // for HDD-Windows:
    // (1 << 15) - best for reading only prefetch
    // (1 << 22) - best for real reading / writing
    if (!m_InBitStream.Create(1 << 20))
      return E_OUTOFMEMORY;
    m_InBitStream.Init();
    _needInitInStream = false;
  }
  return S_OK;
}


HRESULT CCoder::CodeSpec(UInt32 curSize, bool finishInputStream, UInt32 inputProgressLimit)
{
  if (_remainLen == kLenIdFinished)
    return S_OK;
  
  if (_remainLen == kLenIdNeedInit)
  {
    if (!_keepHistory)
      if (!m_OutWindowStream.Create(_deflate64Mode ? kHistorySize64: kHistorySize32))
        return E_OUTOFMEMORY;
    RINOK(InitInStream(_needInitInStream))
    m_OutWindowStream.Init(_keepHistory);
  
    m_FinalBlock = false;
    _remainLen = 0;
    _needReadTable = true;
  }

  // _remainLen >= 0
  while (_remainLen && curSize)
  {
    _remainLen--;
    const Byte b = m_OutWindowStream.GetByte(_rep0);
    m_OutWindowStream.PutByte(b);
    curSize--;
  }

  UInt64 inputStart = 0;
  if (inputProgressLimit != 0)
    inputStart = m_InBitStream.GetProcessedSize();

  while (curSize || finishInputStream)
  {
    if (m_InBitStream.ExtraBitsWereRead())
      return S_FALSE;

    if (_needReadTable)
    {
      if (m_FinalBlock)
      {
        _remainLen = kLenIdFinished;
        break;
      }
 
      if (inputProgressLimit != 0)
        if (m_InBitStream.GetProcessedSize() - inputStart >= inputProgressLimit)
          return S_OK;
      
      if (!ReadTables())
        return S_FALSE;
      if (m_InBitStream.ExtraBitsWereRead())
        return S_FALSE;
      _needReadTable = false;
    }

    if (m_StoredMode)
    {
      if (finishInputStream && curSize == 0 && m_StoredBlockSize != 0)
        return S_FALSE;
      /* NSIS version contains some bits in bitl bits buffer.
         So we must read some first bytes via ReadAlignedByte */
      UInt32 num = m_StoredBlockSize;
      if (num > curSize)
          num = curSize;
      m_StoredBlockSize -= num;
      curSize -= num;
      for (; num && m_InBitStream.ThereAreDataInBitsBuffer(); num--)
        m_OutWindowStream.PutByte(ReadAlignedByte());
      if (num)
      {
#if 1
        // fast code
        do
        {
          size_t a;
          Byte *buf = m_OutWindowStream.GetOutBuffer(a);
          // a != 0
          if (a > num)
              a = num;
          // a != 0
          a = m_InBitStream.ReadDirectBytesPart(buf, a);
          if (a == 0)
            return S_FALSE;
          m_OutWindowStream.SkipWrittenBytes(a);
          num -= (UInt32)a;
        }
        while (num);
#else
        // slow code:
        do
          m_OutWindowStream.PutByte(m_InBitStream.ReadDirectByte());
        while (--num);
#endif
      }
      _needReadTable = (m_StoredBlockSize == 0);
      continue;
    }
    
    while (curSize)
    {
      if (m_InBitStream.ExtraBitsWereRead_Fast())
        return S_FALSE;
      unsigned sym;
#if 0
      sym = m_MainDecoder.Decode(&m_InBitStream);
#else
      Z7_HUFF_DECODE_CHECK(sym, &m_MainDecoder, kNumHuffmanBits, kNumTableBits_Main, &m_InBitStream, { return S_FALSE; })
#endif

      if (sym < 0x100)
      {
        m_OutWindowStream.PutByte((Byte)sym);
        curSize--;
        continue;
      }
      if (sym == kSymbolEndOfBlock)
      {
        _needReadTable = true;
        break;
      }
#if 0
      if (sym >= kMainTableSize)
        return S_FALSE;
#endif
      {
        sym -= kSymbolMatch;
        UInt32 len;
        {
          unsigned numBits;
          if (_deflate64Mode)
          {
            len = kLenStart64[sym];
            numBits = kLenDirectBits64[sym];
          }
          else
          {
            len = kLenStart32[sym];
            numBits = kLenDirectBits32[sym];
          }
          len += kMatchMinLen + m_InBitStream.ReadBits(numBits);
        }
       
#if 0
        sym = m_DistDecoder.Decode(&m_InBitStream);
        if (sym >= _numDistLevels)
          return S_FALSE;
#else
        Z7_HUFF_DECODE_CHECK(sym, &m_DistDecoder, kNumHuffmanBits, kNumTableBits_Dist, &m_InBitStream, { return S_FALSE; })
#endif

#if 1
        sym = kDistStart[sym] + m_InBitStream.ReadBits(kDistDirectBits[sym]);
#else
        if (sym >= 4)
        {
          // sym &= 31;
          const unsigned numDirectBits = (sym - 2) >> 1;
          sym = (2u | (sym & 1)) << numDirectBits;
          sym += m_InBitStream.ReadBits(numDirectBits);
        }
#endif
        UInt32 locLen = len;
        if (locLen > curSize)
          locLen = (UInt32)curSize;
        if (!m_OutWindowStream.CopyBlock(sym, locLen))
          return S_FALSE;
        curSize -= locLen;
        len -= locLen;
        if (len != 0)
        {
          _remainLen = (Int32)len;
          _rep0 = sym;
          break;
        }
      }
    }
    
    if (finishInputStream && curSize == 0)
    {
      if (m_MainDecoder.Decode(&m_InBitStream) != kSymbolEndOfBlock)
        return S_FALSE;
      _needReadTable = true;
    }
  }

  if (m_InBitStream.ExtraBitsWereRead())
    return S_FALSE;

  return S_OK;
}


#ifdef Z7_NO_EXCEPTIONS

#define DEFLATE_TRY_BEGIN
#define DEFLATE_TRY_END(res)

#else

#define DEFLATE_TRY_BEGIN try {
#define DEFLATE_TRY_END(res) } \
  catch(const CSystemException &e) { res = e.ErrorCode; } \
  catch(...) { res = S_FALSE; }

  // catch(const CInBufferException &e)  { res = e.ErrorCode; }
  // catch(const CLzOutWindowException &e)  { res = e.ErrorCode; }

#endif


HRESULT CCoder::CodeReal(ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  HRESULT res;
  
  DEFLATE_TRY_BEGIN
  
  m_OutWindowStream.SetStream(outStream);
  CCoderReleaser flusher(this);

  const UInt64 inStart = _needInitInStream ? 0 : m_InBitStream.GetProcessedSize();

  for (;;)
  {
    const UInt32 kInputProgressLimit = 1 << 21;
    UInt32 curSize = 1 << 20;
    bool finishInputStream = false;
    if (_outSizeDefined)
    {
      const UInt64 rem = _outSize - GetOutProcessedCur();
      if (curSize >= rem)
      {
        curSize = (UInt32)rem;
        if (_needFinishInput)
          finishInputStream = true;
        else if (curSize == 0)
          break;
      }
    }
    
    RINOK(CodeSpec(curSize, finishInputStream, progress ? kInputProgressLimit : 0))
    
    if (_remainLen == kLenIdFinished)
      break;

    if (progress)
    {
      const UInt64 inSize = m_InBitStream.GetProcessedSize() - inStart;
      const UInt64 nowPos64 = GetOutProcessedCur();
      RINOK(progress->SetRatioInfo(&inSize, &nowPos64))
    }
  }
  
  flusher.NeedFlush = false;
  res = Flush();
  if (res == S_OK && _remainLen != kLenIdNeedInit && InputEofError())
    return S_FALSE;
  
  DEFLATE_TRY_END(res)
  
  return res;
}


Z7_COM7F_IMF(CCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  SetInStream(inStream);
  SetOutStreamSize(outSize);
  const HRESULT res = CodeReal(outStream, progress);
  ReleaseInStream();
  /*
  if (res == S_OK)
    if (_needFinishInput && inSize && *inSize != m_InBitStream.GetProcessedSize())
      res = S_FALSE;
  */
  return res;
}


Z7_COM7F_IMF(CCoder::SetFinishMode(UInt32 finishMode))
{
  Set_NeedFinishInput(finishMode != 0);
  return S_OK;
}


Z7_COM7F_IMF(CCoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = m_InBitStream.GetStreamSize();
  return S_OK;
}


Z7_COM7F_IMF(CCoder::ReadUnusedFromInBuf(void *data, UInt32 size, UInt32 *processedSize))
{
  AlignToByte();
  UInt32 i = 0;
  {
    for (i = 0; i < size; i++)
    {
      if (!m_InBitStream.ReadAlignedByte_FromBuf(((Byte *)data)[i]))
        break;
    }
  }
  if (processedSize)
    *processedSize = i;
  return S_OK;
}


Z7_COM7F_IMF(CCoder::SetInStream(ISequentialInStream *inStream))
{
  m_InStreamRef = inStream;
  m_InBitStream.SetStream(inStream);
  return S_OK;
}


Z7_COM7F_IMF(CCoder::ReleaseInStream())
{
  m_InStreamRef.Release();
  m_InBitStream.ClearStreamPtr();
  return S_OK;
}


void CCoder::SetOutStreamSizeResume(const UInt64 *outSize)
{
  _outSizeDefined = (outSize != NULL);
  _outSize = 0;
  if (_outSizeDefined)
    _outSize = *outSize;
  m_OutWindowStream.Init(_keepHistory);
  _outStartPos = m_OutWindowStream.GetProcessedSize();
  _remainLen = kLenIdNeedInit;
}


Z7_COM7F_IMF(CCoder::SetOutStreamSize(const UInt64 *outSize))
{
  /*
    18.06:
    We want to support GetInputProcessedSize() before CCoder::Read()
    So we call m_InBitStream.Init() even before buffer allocations
    m_InBitStream.Init() just sets variables to default values
    But later we will call m_InBitStream.Init() again with real buffer pointers
  */
  m_InBitStream.Init();
  _needInitInStream = true;
  SetOutStreamSizeResume(outSize);
  return S_OK;
}


#ifndef Z7_NO_READ_FROM_CODER

Z7_COM7F_IMF(CCoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  const UInt64 outPos = GetOutProcessedCur();

  bool finishInputStream = false;
  if (_outSizeDefined)
  {
    const UInt64 rem = _outSize - outPos;
    if (size >= rem)
    {
      size = (UInt32)rem;
      if (_needFinishInput)
        finishInputStream = true;
    }
  }
  if (!finishInputStream && size == 0)
    return S_OK;

  HRESULT res;
  DEFLATE_TRY_BEGIN
  m_OutWindowStream.SetMemStream((Byte *)data);
  res = CodeSpec(size, finishInputStream);
  DEFLATE_TRY_END(res)
  {
    const HRESULT res2 = Flush();
    if (res2 != S_OK)
      res = res2;
  }
  if (processedSize)
    *processedSize = (UInt32)(GetOutProcessedCur() - outPos);
  m_OutWindowStream.SetMemStream(NULL);
  return res;
}

#endif


HRESULT CCoder::CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  SetOutStreamSizeResume(outSize);
  return CodeReal(outStream, progress);
}

}}}

/* ================ unit: CPP/7zip/Compress/DeflateEncoder.cpp ================ */
// DeflateEncoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

#undef NO_INLINE

#ifdef _MSC_VER
#define NO_INLINE Z7_NO_INLINE
#else
#define NO_INLINE
#endif

#define MAX_HUF_LEN_12  12

namespace NCompress {
namespace NDeflate {
namespace NEncoder {

static const unsigned k_CodeValue_Len_Is_Literal_Flag = 1u << 15;

static const unsigned kNumDivPassesMax = 10; // [0, 16); ratio/speed/ram tradeoff; use big value for better compression ratio.
static const unsigned kNumTables = 1u << kNumDivPassesMax;

static const UInt32 kFixedHuffmanCodeBlockSizeMax = (1 << 8); // [0, (1 << 32)); ratio/speed tradeoff; use big value for better compression ratio.
static const UInt32 kDivideCodeBlockSizeMin = (1 << 7); // [1, (1 << 32)); ratio/speed tradeoff; use small value for better compression ratio.
static const UInt32 kDivideBlockSizeMin = (1 << 6); // [1, (1 << 32)); ratio/speed tradeoff; use small value for better compression ratio.

static const UInt32 kMaxUncompressedBlockSize = ((1 << 16) - 1) * 1; // [1, (1 << 32))
static const UInt32 kMatchArraySize = kMaxUncompressedBlockSize * 10; // [kMatchMaxLen * 2, (1 << 32))
static const UInt32 kMatchArrayLimit = kMatchArraySize - kMatchMaxLen * 4 * sizeof(UInt16);
static const UInt32 kBlockUncompressedSizeThreshold = kMaxUncompressedBlockSize -
    kMatchMaxLen - kNumOpts;

// static const unsigned kMaxCodeBitLength = 11;
static const unsigned kMaxLevelBitLength = 7;

static const Byte kNoLiteralStatPrice = 11;
static const Byte kNoLenStatPrice = 11;
static const Byte kNoPosStatPrice = 6;

MY_ALIGN(64)
static Byte g_LenSlots[kNumLenSymbolsMax];

#define kNumLogBits 9    // do not change it
MY_ALIGN(64)
static Byte g_FastPos[1 << kNumLogBits];

class CFastPosInit
{
public:
  CFastPosInit()
  {
    unsigned i;
    for (i = 0; i < kNumLenSlots; i++)
    {
      unsigned c = kLenStart32[i];
      const unsigned j = 1u << kLenDirectBits32[i];
      for (unsigned k = 0; k < j; k++, c++)
        g_LenSlots[c] = (Byte)i;
    }
    
    const unsigned kFastSlots = kNumLogBits * 2;
    unsigned c = 0;
    for (Byte slotFast = 0; slotFast < kFastSlots; slotFast++)
    {
      const unsigned k = 1u << kDistDirectBits[slotFast];
      for (unsigned j = 0; j < k; j++, c++)
        g_FastPos[c] = slotFast;
    }
  }
};

static CFastPosInit g_FastPosInit;

inline unsigned GetPosSlot(UInt32 pos)
{
  /*
  if (pos < 0x200)
    return g_FastPos[pos];
  return g_FastPos[pos >> 8] + 16;
  */
  // const unsigned zz = (pos < ((UInt32)1 << (kNumLogBits))) ? 0 : 8;
  /*
  const unsigned zz = (kNumLogBits - 1) &
      ((UInt32)0 - (((((UInt32)1 << kNumLogBits) - 1) - pos) >> 31));
  */
  const unsigned zz = (kNumLogBits - 1) &
      (((((UInt32)1 << kNumLogBits) - 1) - pos) >> (31 - 3));
  return g_FastPos[pos >> zz] + (zz * 2);
}


void CEncProps::Normalize()
{
  int level = Level;
  if (level < 0) level = 5;
  Level = level;
  if (algo < 0) algo = (level < 5 ? 0 : 1);
  if (fb < 0) fb = (level < 7 ? 32 : (level < 9 ? 64 : 128));
  if (btMode < 0) btMode = (algo == 0 ? 0 : 1);
  if (mc == 0) mc = (16 + ((unsigned)fb >> 1));
  if (numPasses == (UInt32)(Int32)-1) numPasses = (level < 7 ? 1 : (level < 9 ? 3 : 10));
}

void CCoder::SetProps(const CEncProps *props2)
{
  CEncProps props = *props2;
  props.Normalize();

  m_MatchFinderCycles = props.mc;
  {
    unsigned fb = (unsigned)props.fb;
    if (fb < kMatchMinLen)
      fb = kMatchMinLen;
    if (fb > m_MatchMaxLen)
      fb = m_MatchMaxLen;
    m_NumFastBytes = fb;
  }
  _fastMode = (props.algo == 0);
  _btMode = (props.btMode != 0);

  m_NumDivPasses = props.numPasses;
  if (m_NumDivPasses == 0)
    m_NumDivPasses = 1;
  if (m_NumDivPasses == 1)
    m_NumPasses = 1;
  else if (m_NumDivPasses <= kNumDivPassesMax)
    m_NumPasses = 2;
  else
  {
    m_NumPasses = 2 + (m_NumDivPasses - kNumDivPassesMax);
    m_NumDivPasses = kNumDivPassesMax;
  }
}

CCoder::CCoder(bool deflate64Mode):
  m_Values(NULL),
  m_OnePosMatchesMemory(NULL),
  m_DistanceMemory(NULL),
  m_Created(false),
  m_Deflate64Mode(deflate64Mode),
  m_Tables(NULL)
{
  m_MatchMaxLen = deflate64Mode ? kMatchMaxLen64 : kMatchMaxLen32;
  m_NumLenCombinations = deflate64Mode ? kNumLenSymbols64 : kNumLenSymbols32;
  m_LenStart = deflate64Mode ? kLenStart64 : kLenStart32;
  m_LenDirectBits = deflate64Mode ? kLenDirectBits64 : kLenDirectBits32;
  {
    CEncProps props;
    SetProps(&props);
  }
  MatchFinder_Construct(&_lzInWindow);
}

HRESULT CCoder::Create()
{
  // COM_TRY_BEGIN
  if (!m_Values)
  {
    m_Values = (CCodeValue *)MyAlloc(kMaxUncompressedBlockSize * sizeof(CCodeValue));
    if (!m_Values)
      return E_OUTOFMEMORY;
  }
  if (!m_Tables)
  {
    m_Tables = (CTables *)MyAlloc(kNumTables * sizeof(CTables));
    if (!m_Tables)
      return E_OUTOFMEMORY;
  }

  if (m_IsMultiPass)
  {
    if (!m_OnePosMatchesMemory)
    {
      m_OnePosMatchesMemory = (UInt16 *)::MidAlloc(kMatchArraySize * sizeof(UInt16));
      if (!m_OnePosMatchesMemory)
        return E_OUTOFMEMORY;
    }
  }
  else
  {
    if (!m_DistanceMemory)
    {
      m_DistanceMemory = (UInt16 *)MyAlloc((kMatchMaxLen + 2) * 2 * sizeof(UInt16));
      if (!m_DistanceMemory)
        return E_OUTOFMEMORY;
      m_MatchDistances = m_DistanceMemory;
    }
  }

  if (!m_Created)
  {
    _lzInWindow.btMode = (Byte)(_btMode ? 1 : 0);
    _lzInWindow.numHashBytes = 3;
    _lzInWindow.numHashBytes_Min = 3;
    if (!MatchFinder_Create(&_lzInWindow,
        m_Deflate64Mode ? kHistorySize64 : kHistorySize32,
        kNumOpts + kMaxUncompressedBlockSize,
        m_NumFastBytes, m_MatchMaxLen - m_NumFastBytes, &g_AlignedAlloc))
      return E_OUTOFMEMORY;
    if (!m_OutStream.Create(1 << 20))
      return E_OUTOFMEMORY;
  }
  if (m_MatchFinderCycles != 0)
    _lzInWindow.cutValue = m_MatchFinderCycles;
  m_Created = true;
  return S_OK;
  // COM_TRY_END
}

HRESULT CCoder::BaseSetEncoderProperties2(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps)
{
  CEncProps props;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    PROPID propID = propIDs[i];
    if (propID >= NCoderPropID::kReduceSize)
      continue;
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    UInt32 v = (UInt32)prop.ulVal;
    switch (propID)
    {
      case NCoderPropID::kNumPasses: props.numPasses = v; break;
      case NCoderPropID::kNumFastBytes: props.fb = (int)v; break;
      case NCoderPropID::kMatchFinderCycles: props.mc = v; break;
      case NCoderPropID::kAlgorithm: props.algo = (int)v; break;
      case NCoderPropID::kLevel: props.Level = (int)v; break;
      case NCoderPropID::kNumThreads: break;
      default: return E_INVALIDARG;
    }
  }
  SetProps(&props);
  return S_OK;
}
  
void CCoder::Free()
{
  ::MidFree(m_OnePosMatchesMemory); m_OnePosMatchesMemory = NULL;
  ::MyFree(m_DistanceMemory); m_DistanceMemory = NULL;
  ::MyFree(m_Values); m_Values = NULL;
  ::MyFree(m_Tables); m_Tables = NULL;
}

CCoder::~CCoder()
{
  Free();
  MatchFinder_Free(&_lzInWindow, &g_AlignedAlloc);
}

NO_INLINE void CCoder::GetMatches()
{
  if (m_IsMultiPass)
  {
    m_MatchDistances = m_OnePosMatchesMemory + m_Pos;
    if (m_SecondPass)
    {
      m_Pos += *m_MatchDistances + 1;
      return;
    }
  }

  UInt32 distanceTmp[kMatchMaxLen * 2 + 3];
  
  const size_t numPairs = (size_t)((_btMode ?
      Bt3Zip_MatchFinder_GetMatches(&_lzInWindow, distanceTmp):
      Hc3Zip_MatchFinder_GetMatches(&_lzInWindow, distanceTmp)) - distanceTmp);

  UInt16 *matchDistances = m_MatchDistances;
  *matchDistances++ = (UInt16)numPairs;
   
  if (numPairs != 0)
  {
    size_t i;
    for (i = 0; i < numPairs; i += 2)
    {
      matchDistances[0] = (UInt16)distanceTmp[i];
      matchDistances[1] = (UInt16)distanceTmp[(size_t)i + 1];
      matchDistances += 2;
    }
    UInt32 len = distanceTmp[(size_t)numPairs - 2];
    if (len == m_NumFastBytes && m_NumFastBytes != m_MatchMaxLen)
    {
      UInt32 numAvail = Inline_MatchFinder_GetNumAvailableBytes(&_lzInWindow) + 1;
      const Byte *pby = Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow) - 1;
      const Byte *pby2 = pby - (distanceTmp[(size_t)numPairs - 1] + 1);
      if (numAvail > m_MatchMaxLen)
        numAvail = m_MatchMaxLen;
      for (; len < numAvail && pby[len] == pby2[len]; len++);
      matchDistances[-2] = (UInt16)len;
    }
  }
  if (m_IsMultiPass)
    m_Pos += (UInt32)numPairs + 1;
  if (!m_SecondPass)
    m_AdditionalOffset++;
}

void CCoder::MovePos(UInt32 num)
{
  if (!m_SecondPass && num > 0)
  {
    if (_btMode)
      Bt3Zip_MatchFinder_Skip(&_lzInWindow, num);
    else
      Hc3Zip_MatchFinder_Skip(&_lzInWindow, num);
    m_AdditionalOffset += num;
  }
}

static const UInt32 kIfinityPrice = 0xFFFFFFF;

NO_INLINE UInt32 CCoder::Backward(UInt32 &backRes, UInt32 cur)
{
  m_OptimumEndIndex = cur;
  UInt32 posMem = m_Optimum[cur].PosPrev;
  UInt16 backMem = m_Optimum[cur].BackPrev;
  do
  {
    UInt32 posPrev = posMem;
    UInt16 backCur = backMem;
    backMem = m_Optimum[posPrev].BackPrev;
    posMem = m_Optimum[posPrev].PosPrev;
    m_Optimum[posPrev].BackPrev = backCur;
    m_Optimum[posPrev].PosPrev = (UInt16)cur;
    cur = posPrev;
  }
  while (cur > 0);
  backRes = m_Optimum[0].BackPrev;
  m_OptimumCurrentIndex = m_Optimum[0].PosPrev;
  return m_OptimumCurrentIndex;
}

NO_INLINE UInt32 CCoder::GetOptimal(UInt32 &backRes)
{
  if (m_OptimumEndIndex != m_OptimumCurrentIndex)
  {
    UInt32 len = m_Optimum[m_OptimumCurrentIndex].PosPrev - m_OptimumCurrentIndex;
    backRes = m_Optimum[m_OptimumCurrentIndex].BackPrev;
    m_OptimumCurrentIndex = m_Optimum[m_OptimumCurrentIndex].PosPrev;
    return len;
  }
  m_OptimumCurrentIndex = m_OptimumEndIndex = 0;
  
  GetMatches();

  UInt32 lenEnd;
  {
    const UInt32 numDistancePairs = m_MatchDistances[0];
    if (numDistancePairs == 0)
      return 1;
    const UInt16 *matchDistances = m_MatchDistances + 1;
    lenEnd = matchDistances[(size_t)numDistancePairs - 2];
    
    if (lenEnd > m_NumFastBytes)
    {
      backRes = matchDistances[(size_t)numDistancePairs - 1];
      MovePos(lenEnd - 1);
      return lenEnd;
    }

    m_Optimum[1].Price = m_LiteralPrices[*(Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow) - m_AdditionalOffset)];
    m_Optimum[1].PosPrev = 0;
    
    m_Optimum[2].Price = kIfinityPrice;
    m_Optimum[2].PosPrev = 1;
    
    UInt32 offs = 0;
  
    for (UInt32 i = kMatchMinLen; i <= lenEnd; i++)
    {
      UInt32 distance = matchDistances[(size_t)offs + 1];
      m_Optimum[i].PosPrev = 0;
      m_Optimum[i].BackPrev = (UInt16)distance;
      m_Optimum[i].Price = m_LenPrices[(size_t)i - kMatchMinLen] + m_PosPrices[GetPosSlot(distance)];
      if (i == matchDistances[offs])
        offs += 2;
    }
  }

  UInt32 cur = 0;

  for (;;)
  {
    ++cur;
    if (cur == lenEnd || cur == kNumOptsBase || m_Pos >= kMatchArrayLimit)
      return Backward(backRes, cur);
    GetMatches();
    const UInt16 *matchDistances = m_MatchDistances + 1;
    const UInt32 numDistancePairs = m_MatchDistances[0];
    UInt32 newLen = 0;
    if (numDistancePairs != 0)
    {
      newLen = matchDistances[(size_t)numDistancePairs - 2];
      if (newLen > m_NumFastBytes)
      {
        UInt32 len = Backward(backRes, cur);
        m_Optimum[cur].BackPrev = matchDistances[(size_t)numDistancePairs - 1];
        m_OptimumEndIndex = cur + newLen;
        m_Optimum[cur].PosPrev = (UInt16)m_OptimumEndIndex;
        MovePos(newLen - 1);
        return len;
      }
    }
    UInt32 curPrice = m_Optimum[cur].Price;
    {
      const UInt32 curAnd1Price = curPrice + m_LiteralPrices[*(Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow) + cur - m_AdditionalOffset)];
      COptimal &optimum = m_Optimum[(size_t)cur + 1];
      if (curAnd1Price < optimum.Price)
      {
        optimum.Price = curAnd1Price;
        optimum.PosPrev = (UInt16)cur;
      }
    }
    if (numDistancePairs == 0)
      continue;
    while (lenEnd < cur + newLen)
      m_Optimum[++lenEnd].Price = kIfinityPrice;
    UInt32 offs = 0;
    UInt32 distance = matchDistances[(size_t)offs + 1];
    curPrice += m_PosPrices[GetPosSlot(distance)];
    for (UInt32 lenTest = kMatchMinLen; ; lenTest++)
    {
      UInt32 curAndLenPrice = curPrice + m_LenPrices[(size_t)lenTest - kMatchMinLen];
      COptimal &optimum = m_Optimum[cur + lenTest];
      if (curAndLenPrice < optimum.Price)
      {
        optimum.Price = curAndLenPrice;
        optimum.PosPrev = (UInt16)cur;
        optimum.BackPrev = (UInt16)distance;
      }
      if (lenTest == matchDistances[offs])
      {
        offs += 2;
        if (offs == numDistancePairs)
          break;
        curPrice -= m_PosPrices[GetPosSlot(distance)];
        distance = matchDistances[(size_t)offs + 1];
        curPrice += m_PosPrices[GetPosSlot(distance)];
      }
    }
  }
}

UInt32 CCoder::GetOptimalFast(UInt32 &backRes)
{
  GetMatches();
  UInt32 numDistancePairs = m_MatchDistances[0];
  if (numDistancePairs == 0)
    return 1;
  UInt32 lenMain = m_MatchDistances[(size_t)numDistancePairs - 1];
  backRes = m_MatchDistances[numDistancePairs];
  MovePos(lenMain - 1);
  return lenMain;
}

void CTables::InitStructures()
{
  UInt32 i;
  for (i = 0; i < 256; i++)
    litLenLevels[i] = 8;
  litLenLevels[i++] = 13;
  for (;i < kFixedMainTableSize; i++)
    litLenLevels[i] = 5;
  for (i = 0; i < kFixedDistTableSize; i++)
    distLevels[i] = 5;
}

NO_INLINE void CCoder::LevelTableDummy(const Byte *levels, unsigned numLevels, UInt32 *freqs)
{
  unsigned prevLen = 0xFF;
  unsigned nextLen = levels[0];
  unsigned count = 0;
  unsigned maxCount = 7;
  unsigned minCount = 4;
  
  if (nextLen == 0)
  {
    maxCount = 138;
    minCount = 3;
  }
  
  for (unsigned n = 0; n < numLevels; n++)
  {
    unsigned curLen = nextLen;
    nextLen = (n < numLevels - 1) ? levels[(size_t)n + 1] : 0xFF;
    count++;
    if (count < maxCount && curLen == nextLen)
      continue;
    
    if (count < minCount)
      freqs[curLen] += (UInt32)count;
    else if (curLen != 0)
    {
      if (curLen != prevLen)
      {
        freqs[curLen]++;
        count--;
      }
      freqs[kTableLevelRepNumber]++;
    }
    else if (count <= 10)
      freqs[kTableLevel0Number]++;
    else
      freqs[kTableLevel0Number2]++;

    count = 0;
    prevLen = curLen;
    
    if (nextLen == 0)
    {
      maxCount = 138;
      minCount = 3;
    }
    else if (curLen == nextLen)
    {
      maxCount = 6;
      minCount = 3;
    }
    else
    {
      maxCount = 7;
      minCount = 4;
    }
  }
}

NO_INLINE void CCoder::WriteBits(UInt32 value, unsigned numBits)
{
  m_OutStream.WriteBits(value, numBits);
}

#define WRITE_HF2(codes, lens, i) m_OutStream.WriteBits(codes[i], lens[i])
#define WRITE_HF2_NO_INLINE(codes, lens, i)   WriteBits(codes[i], lens[i])
#define WRITE_HF(i) WriteBits(codes[i], lens[i])

NO_INLINE void CCoder::LevelTableCode(const Byte *levels, unsigned numLevels, const Byte *lens, const UInt32 *codes)
{
  unsigned prevLen = 0xFF;
  unsigned nextLen = levels[0];
  unsigned count = 0;
  unsigned maxCount = 7;
  unsigned minCount = 4;
  
  if (nextLen == 0)
  {
    maxCount = 138;
    minCount = 3;
  }
  
  for (unsigned n = 0; n < numLevels; n++)
  {
    unsigned curLen = nextLen;
    nextLen = (n < numLevels - 1) ? levels[(size_t)n + 1] : 0xFF;
    count++;
    if (count < maxCount && curLen == nextLen)
      continue;
    
    if (count < minCount)
      for (unsigned i = 0; i < count; i++)
        WRITE_HF(curLen);
    else if (curLen != 0)
    {
      if (curLen != prevLen)
      {
        WRITE_HF(curLen);
        count--;
      }
      WRITE_HF(kTableLevelRepNumber);
      WriteBits(count - 3, 2);
    }
    else if (count <= 10)
    {
      WRITE_HF(kTableLevel0Number);
      WriteBits(count - 3, 3);
    }
    else
    {
      WRITE_HF(kTableLevel0Number2);
      WriteBits(count - 11, 7);
    }

    count = 0;
    prevLen = curLen;
    
    if (nextLen == 0)
    {
      maxCount = 138;
      minCount = 3;
    }
    else if (curLen == nextLen)
    {
      maxCount = 6;
      minCount = 3;
    }
    else
    {
      maxCount = 7;
      minCount = 4;
    }
  }
}

NO_INLINE void CCoder::MakeTables(unsigned maxHuffLen)
{
  Huffman_Generate(mainFreqs, mainCodes, m_NewLevels.litLenLevels, kFixedMainTableSize, maxHuffLen);
  Huffman_Generate(distFreqs, distCodes, m_NewLevels.distLevels, kDistTableSize64, maxHuffLen);
}

static NO_INLINE UInt32 Huffman_GetPrice(const UInt32 *freqs, const Byte *lens, UInt32 num)
{
  UInt32 price = 0;
  UInt32 i;
  for (i = 0; i < num; i++)
    price += lens[i] * freqs[i];
  return price;
}

static NO_INLINE UInt32 Huffman_GetPrice_Spec(
    const UInt32 *freqs, const Byte *lens, UInt32 num,
    const Byte *extraBits, UInt32 extraBase)
{
  return
    Huffman_GetPrice(freqs, lens, num) +
    Huffman_GetPrice(freqs + extraBase, extraBits, num - extraBase);
}

NO_INLINE UInt32 CCoder::GetLzBlockPrice() const
{
  return
    Huffman_GetPrice_Spec(mainFreqs, m_NewLevels.litLenLevels,
        kFixedMainTableSize, m_LenDirectBits, kSymbolMatch) +
    Huffman_GetPrice_Spec(distFreqs, m_NewLevels.distLevels,
        kDistTableSize64, kDistDirectBits, 0);
}

NO_INLINE void CCoder::TryBlock()
{
  memset(mainFreqs, 0, sizeof(mainFreqs));
  memset(distFreqs, 0, sizeof(distFreqs));

  m_ValueIndex = 0;
  UInt32 blockSize = BlockSizeRes;
  BlockSizeRes = 0;
  for (;;)
  {
    if (m_OptimumCurrentIndex == m_OptimumEndIndex)
    {
      if (m_Pos >= kMatchArrayLimit
          || BlockSizeRes >= blockSize
          || (!m_SecondPass && ((Inline_MatchFinder_GetNumAvailableBytes(&_lzInWindow) == 0) || m_ValueIndex >= m_ValueBlockSize)))
        break;
    }
    UInt32 pos;
    UInt32 len;
    if (_fastMode)
      len = GetOptimalFast(pos);
    else
      len = GetOptimal(pos);
    CCodeValue &codeValue = m_Values[m_ValueIndex++];
    if (len >= kMatchMinLen)
    {
      const UInt32 newLen = len - kMatchMinLen;
      codeValue.Len = (UInt16)newLen;
      mainFreqs[kSymbolMatch + (size_t)g_LenSlots[newLen]]++;
      codeValue.Pos = (UInt16)pos;
      distFreqs[GetPosSlot(pos)]++;
    }
    else
    {
      const unsigned b = *(Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow) - m_AdditionalOffset);
      mainFreqs[b]++;
      codeValue.Len = k_CodeValue_Len_Is_Literal_Flag;
      codeValue.Pos = (UInt16)b;
    }
    m_AdditionalOffset -= len;
    BlockSizeRes += len;
  }
  mainFreqs[kSymbolEndOfBlock]++;
  m_AdditionalOffset += BlockSizeRes;
  m_SecondPass = true;
}

NO_INLINE void CCoder::SetPrices(const CLevels &levels)
{
  if (_fastMode)
    return;
  UInt32 i;
  for (i = 0; i < 256; i++)
  {
    Byte price = levels.litLenLevels[i];
    m_LiteralPrices[i] = ((price != 0) ? price : kNoLiteralStatPrice);
  }
  
  for (i = 0; i < m_NumLenCombinations; i++)
  {
    UInt32 slot = g_LenSlots[i];
    Byte price = levels.litLenLevels[kSymbolMatch + (size_t)slot];
    m_LenPrices[i] = (Byte)(((price != 0) ? price : kNoLenStatPrice) + m_LenDirectBits[slot]);
  }
  
  for (i = 0; i < kDistTableSize64; i++)
  {
    Byte price = levels.distLevels[i];
    m_PosPrices[i] = (Byte)(((price != 0) ? price: kNoPosStatPrice) + kDistDirectBits[i]);
  }
}

#if MAX_HUF_LEN_12 > 12
// Huffman_ReverseBits() now supports 12-bits values only.
#error Stop_Compiling_Bad_MAX_HUF_LEN_12
#endif
static NO_INLINE void Huffman_ReverseBits(UInt32 *codes, const Byte *lens, UInt32 num)
{
  const Byte * const lens_lim = lens + num;
  do
  {
    // we should change constants, if lens[*] can be larger than 12.
    UInt32 x = *codes;
    x = ((x & (0x555     )) << 2) + (x & (0xAAA     ));
    x = ((x & (0x333 << 1)) << 4) | (x & (0xCCC << 1));
    x = ((x & (0xF0F << 3)) << 8) | (x & (0x0F0 << 3));
    // we can use (x) instead of (x & (0xFF << 7)), if we support garabage data after (*lens) bits.
    *codes++ = (((x & (0xFF << 7)) << 16) | x) >> (*lens ^ 31);
  }
  while (++lens != lens_lim);
}

NO_INLINE void CCoder::WriteBlock()
{
  Huffman_ReverseBits(mainCodes, m_NewLevels.litLenLevels, kFixedMainTableSize);
  Huffman_ReverseBits(distCodes, m_NewLevels.distLevels, kDistTableSize64);

  CCodeValue *values = m_Values;
  const CCodeValue * const values_lim = values + m_ValueIndex;

  if (values != values_lim)
  do
  {
    const UInt32 len = values->Len;
    const UInt32 dist = values->Pos;
    if (len == k_CodeValue_Len_Is_Literal_Flag)
      WRITE_HF2(mainCodes, m_NewLevels.litLenLevels, dist);
    else
    {
      const unsigned lenSlot = g_LenSlots[len];
      WRITE_HF2(mainCodes, m_NewLevels.litLenLevels, kSymbolMatch + lenSlot);
      m_OutStream.WriteBits(len - m_LenStart[lenSlot], m_LenDirectBits[lenSlot]);
      const unsigned posSlot = GetPosSlot(dist);
      WRITE_HF2(distCodes, m_NewLevels.distLevels, posSlot);
      m_OutStream.WriteBits(dist - kDistStart[posSlot], kDistDirectBits[posSlot]);
    }
  }
  while (++values != values_lim);
  WRITE_HF2_NO_INLINE(mainCodes, m_NewLevels.litLenLevels, kSymbolEndOfBlock);
}

static UInt32 GetStorePrice(UInt32 blockSize, unsigned bitPosition)
{
  UInt32 price = 0;
  do
  {
    UInt32 nextBitPosition = (bitPosition + kFinalBlockFieldSize + kBlockTypeFieldSize) & 7;
    unsigned numBitsForAlign = nextBitPosition > 0 ? (8 - nextBitPosition): 0;
    UInt32 curBlockSize = (blockSize < (1 << 16)) ? blockSize : (1 << 16) - 1;
    price += kFinalBlockFieldSize + kBlockTypeFieldSize + numBitsForAlign + (2 + 2) * 8 + curBlockSize * 8;
    bitPosition = 0;
    blockSize -= curBlockSize;
  }
  while (blockSize != 0);
  return price;
}

void CCoder::WriteStoreBlock(UInt32 blockSize, UInt32 additionalOffset, bool finalBlock)
{
  do
  {
    UInt32 curBlockSize = (blockSize < (1 << 16)) ? blockSize : (1 << 16) - 1;
    blockSize -= curBlockSize;
    WriteBits((finalBlock && (blockSize == 0) ? NFinalBlockField::kFinalBlock: NFinalBlockField::kNotFinalBlock), kFinalBlockFieldSize);
    WriteBits(NBlockType::kStored, kBlockTypeFieldSize);
    m_OutStream.FlushByte();
    WriteBits((UInt16)curBlockSize, kStoredBlockLengthFieldSize);
    WriteBits((UInt16)~curBlockSize, kStoredBlockLengthFieldSize);
    const Byte *data = Inline_MatchFinder_GetPointerToCurrentPos(&_lzInWindow)- additionalOffset;
    for (UInt32 i = 0; i < curBlockSize; i++)
      m_OutStream.WriteByte(data[i]);
    additionalOffset -= curBlockSize;
  }
  while (blockSize != 0);
}

NO_INLINE UInt32 CCoder::TryDynBlock(unsigned tableIndex, UInt32 numPasses)
{
  CTables &t = m_Tables[tableIndex];
  BlockSizeRes = t.BlockSizeRes;
  UInt32 posTemp = t.m_Pos;
  SetPrices(t);

  for (UInt32 p = 0; p < numPasses; p++)
  {
    m_Pos = posTemp;
    TryBlock();
    const unsigned numHuffBits =
        m_ValueIndex > 18000 ? MAX_HUF_LEN_12 :
        m_ValueIndex >  7000 ? 11 :
        m_ValueIndex >  2000 ? 10 : 9;
    MakeTables(numHuffBits);
    SetPrices(m_NewLevels);
  }

  (CLevels &)t = m_NewLevels;

  m_NumLitLenLevels = kMainTableSize;
  while (m_NumLitLenLevels > kNumLitLenCodesMin && m_NewLevels.litLenLevels[(size_t)m_NumLitLenLevels - 1] == 0)
    m_NumLitLenLevels--;
  
  m_NumDistLevels = kDistTableSize64;
  while (m_NumDistLevels > kNumDistCodesMin && m_NewLevels.distLevels[(size_t)m_NumDistLevels - 1] == 0)
    m_NumDistLevels--;
  
  UInt32 levelFreqs[kLevelTableSize];
  memset(levelFreqs, 0, sizeof(levelFreqs));

  LevelTableDummy(m_NewLevels.litLenLevels, m_NumLitLenLevels, levelFreqs);
  LevelTableDummy(m_NewLevels.distLevels, m_NumDistLevels, levelFreqs);
  
  Huffman_Generate(levelFreqs, levelCodes, levelLens, kLevelTableSize, kMaxLevelBitLength);
  
  m_NumLevelCodes = kNumLevelCodesMin;
  for (UInt32 i = 0; i < kLevelTableSize; i++)
  {
    Byte level = levelLens[kCodeLengthAlphabetOrder[i]];
    if (level > 0 && i >= m_NumLevelCodes)
      m_NumLevelCodes = i + 1;
    m_LevelLevels[i] = level;
  }
  
  return GetLzBlockPrice() +
      Huffman_GetPrice_Spec(levelFreqs, levelLens, kLevelTableSize, kLevelDirectBits, kTableDirectLevels) +
      kNumLenCodesFieldSize + kNumDistCodesFieldSize + kNumLevelCodesFieldSize +
      m_NumLevelCodes * kLevelFieldSize + kFinalBlockFieldSize + kBlockTypeFieldSize;
}

NO_INLINE UInt32 CCoder::TryFixedBlock(unsigned tableIndex)
{
  CTables &t = m_Tables[tableIndex];
  BlockSizeRes = t.BlockSizeRes;
  m_Pos = t.m_Pos;
  m_NewLevels.SetFixedLevels();
  SetPrices(m_NewLevels);
  TryBlock();
  return kFinalBlockFieldSize + kBlockTypeFieldSize + GetLzBlockPrice();
}

NO_INLINE UInt32 CCoder::GetBlockPrice(unsigned tableIndex, unsigned numDivPasses)
{
  CTables &t = m_Tables[tableIndex];
  t.StaticMode = false;
  UInt32 price = TryDynBlock(tableIndex, m_NumPasses);
  t.BlockSizeRes = BlockSizeRes;
  UInt32 numValues = m_ValueIndex;
  UInt32 posTemp = m_Pos;
  UInt32 additionalOffsetEnd = m_AdditionalOffset;
  
  if (m_CheckStatic && m_ValueIndex <= kFixedHuffmanCodeBlockSizeMax)
  {
    const UInt32 fixedPrice = TryFixedBlock(tableIndex);
    t.StaticMode = (fixedPrice < price);
    if (t.StaticMode)
      price = fixedPrice;
  }

  const UInt32 storePrice = GetStorePrice(BlockSizeRes, 0); // bitPosition
  t.StoreMode = (storePrice <= price);
  if (t.StoreMode)
    price = storePrice;

  t.UseSubBlocks = false;

  if (numDivPasses > 1 && numValues >= kDivideCodeBlockSizeMin)
  {
    CTables &t0 = m_Tables[(tableIndex << 1)];
    (CLevels &)t0 = t;
    t0.BlockSizeRes = t.BlockSizeRes >> 1;
    t0.m_Pos = t.m_Pos;
    UInt32 subPrice = GetBlockPrice((tableIndex << 1), numDivPasses - 1);

    UInt32 blockSize2 = t.BlockSizeRes - t0.BlockSizeRes;
    if (t0.BlockSizeRes >= kDivideBlockSizeMin && blockSize2 >= kDivideBlockSizeMin)
    {
      CTables &t1 = m_Tables[(tableIndex << 1) + 1];
      (CLevels &)t1 = t;
      t1.BlockSizeRes = blockSize2;
      t1.m_Pos = m_Pos;
      m_AdditionalOffset -= t0.BlockSizeRes;
      subPrice += GetBlockPrice((tableIndex << 1) + 1, numDivPasses - 1);
      t.UseSubBlocks = (subPrice < price);
      if (t.UseSubBlocks)
        price = subPrice;
    }
  }
  
  m_AdditionalOffset = additionalOffsetEnd;
  m_Pos = posTemp;
  return price;
}

void CCoder::CodeBlock(unsigned tableIndex, bool finalBlock)
{
  CTables &t = m_Tables[tableIndex];
  if (t.UseSubBlocks)
  {
    CodeBlock((tableIndex << 1), false);
    CodeBlock((tableIndex << 1) + 1, finalBlock);
  }
  else
  {
    if (t.StoreMode)
      WriteStoreBlock(t.BlockSizeRes, m_AdditionalOffset, finalBlock);
    else
    {
      WriteBits((finalBlock ? NFinalBlockField::kFinalBlock: NFinalBlockField::kNotFinalBlock), kFinalBlockFieldSize);
      if (t.StaticMode)
      {
        WriteBits(NBlockType::kFixedHuffman, kBlockTypeFieldSize);
        TryFixedBlock(tableIndex);
        unsigned i;
        const unsigned kMaxStaticHuffLen = 9;
        for (i = 0; i < kFixedMainTableSize; i++)
          mainFreqs[i] = (UInt32)1 << (kMaxStaticHuffLen - m_NewLevels.litLenLevels[i]);
        for (i = 0; i < kFixedDistTableSize; i++)
          distFreqs[i] = (UInt32)1 << (kMaxStaticHuffLen - m_NewLevels.distLevels[i]);
        MakeTables(kMaxStaticHuffLen);
      }
      else
      {
        if (m_NumDivPasses > 1 || m_CheckStatic)
          TryDynBlock(tableIndex, 1);
        WriteBits(NBlockType::kDynamicHuffman, kBlockTypeFieldSize);
        WriteBits(m_NumLitLenLevels - kNumLitLenCodesMin, kNumLenCodesFieldSize);
        WriteBits(m_NumDistLevels - kNumDistCodesMin, kNumDistCodesFieldSize);
        WriteBits(m_NumLevelCodes - kNumLevelCodesMin, kNumLevelCodesFieldSize);
        
        for (UInt32 i = 0; i < m_NumLevelCodes; i++)
          WriteBits(m_LevelLevels[i], kLevelFieldSize);
        
        Huffman_ReverseBits(levelCodes, levelLens, kLevelTableSize);
        LevelTableCode(m_NewLevels.litLenLevels, m_NumLitLenLevels, levelLens, levelCodes);
        LevelTableCode(m_NewLevels.distLevels, m_NumDistLevels, levelLens, levelCodes);
      }
      WriteBlock();
    }
    m_AdditionalOffset -= t.BlockSizeRes;
  }
}


HRESULT CCoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */ , const UInt64 * /* outSize */ , ICompressProgressInfo *progress)
{
  m_CheckStatic = (m_NumPasses != 1 || m_NumDivPasses != 1);
  m_IsMultiPass = (m_CheckStatic || (m_NumPasses != 1 || m_NumDivPasses != 1));

  /* we can set stream mode before MatchFinder_Create
    if default MatchFinder mode was not STREAM_MODE) */
  // MatchFinder_SET_STREAM_MODE(&_lzInWindow);

  CSeqInStreamWrap _seqInStream;
  _seqInStream.Init(inStream);
  MatchFinder_SET_STREAM(&_lzInWindow, &_seqInStream.vt)

  RINOK(Create())

  m_ValueBlockSize = (7 << 10) + (1 << 12) * m_NumDivPasses;

  UInt64 nowPos = 0;

  MatchFinder_Init(&_lzInWindow);
  m_OutStream.SetStream(outStream);
  m_OutStream.Init();

  m_OptimumEndIndex = m_OptimumCurrentIndex = 0;

  CTables &t = m_Tables[1];
  t.m_Pos = 0;
  t.InitStructures();

  m_AdditionalOffset = 0;
  do
  {
    t.BlockSizeRes = kBlockUncompressedSizeThreshold;
    m_SecondPass = false;
    GetBlockPrice(1, m_NumDivPasses);
    CodeBlock(1, Inline_MatchFinder_GetNumAvailableBytes(&_lzInWindow) == 0);
    nowPos += m_Tables[1].BlockSizeRes;
    if (progress != NULL)
    {
      UInt64 packSize = m_OutStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&nowPos, &packSize))
    }
  }
  while (Inline_MatchFinder_GetNumAvailableBytes(&_lzInWindow) != 0);
  
  if (_seqInStream.Res != S_OK)
    return _seqInStream.Res;

  if (_lzInWindow.result != SZ_OK)
    return SResToHRESULT(_lzInWindow.result);
  return m_OutStream.Flush();
}

HRESULT CCoder::BaseCode(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return E_FAIL; }
}

Z7_COM7F_IMF(CCOMCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
  { return BaseCode(inStream, outStream, inSize, outSize, progress); }

Z7_COM7F_IMF(CCOMCoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
  { return BaseSetEncoderProperties2(propIDs, props, numProps); }

Z7_COM7F_IMF(CCOMCoder64::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
  { return BaseCode(inStream, outStream, inSize, outSize, progress); }

Z7_COM7F_IMF(CCOMCoder64::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps))
  { return BaseSetEncoderProperties2(propIDs, props, numProps); }

}}}

/* ================ unit: CPP/7zip/Compress/ImplodeDecoder.cpp ================ */
// ImplodeDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NImplode {
namespace NDecoder {

bool CHuffmanDecoder::Build(const Byte *lens, unsigned numSymbols) throw()
{
  unsigned counts[kNumHuffmanBits + 1];
  unsigned i;
  for (i = 0; i <= kNumHuffmanBits; i++)
    counts[i] = 0;
  for (i = 0; i < numSymbols; i++)
    counts[lens[i]]++;

  const UInt32 kMaxValue = (UInt32)1 << kNumHuffmanBits;
  // _limits[0] = kMaxValue;
  UInt32 startPos = kMaxValue;
  unsigned sum = 0;

  for (i = 1; i <= kNumHuffmanBits; i++)
  {
    const unsigned cnt = counts[i];
    const UInt32 range = (UInt32)cnt << (kNumHuffmanBits - i);
    if (startPos < range)
      return false;
    startPos -= range;
    _limits[i] = startPos;
    _poses[i] = sum;
    sum += cnt;
    counts[i] = sum;
  }
  // counts[0] += sum;
  if (startPos != 0)
    return false;
  for (i = 0; i < numSymbols; i++)
  {
    const unsigned len = lens[i];
    if (len != 0)
      _symbols[--counts[len]] = (Byte)i;
  }
  return true;
}


unsigned CHuffmanDecoder::Decode(CInBit *inStream) const throw()
{
  const UInt32 val = inStream->GetValue(kNumHuffmanBits);
  size_t numBits;
  for (numBits = 1; val < _limits[numBits]; numBits++);
  const unsigned sym = _symbols[_poses[numBits]
      + (unsigned)((val - _limits[numBits]) >> (kNumHuffmanBits - numBits))];
  inStream->MovePos(numBits);
  return sym;
}



static const unsigned kNumLenDirectBits = 8;

static const unsigned kNumDistDirectBitsSmall = 6;
static const unsigned kNumDistDirectBitsBig = 7;

static const unsigned kLitTableSize = (1 << 8);
static const unsigned kDistTableSize = 64;
static const unsigned kLenTableSize = 64;

static const UInt32 kHistorySize = (1 << kNumDistDirectBitsBig) * kDistTableSize; // 8 KB


CCoder::CCoder():
  _flags(0),
  _fullStreamMode(false)
{}


bool CCoder::BuildHuff(CHuffmanDecoder &decoder, unsigned numSymbols)
{
  Byte levels[kMaxHuffTableSize];
  unsigned numRecords = (unsigned)_inBitStream.ReadAlignedByte() + 1;
  unsigned index = 0;
  do
  {
    const unsigned b = (unsigned)_inBitStream.ReadAlignedByte();
    const unsigned level = (b & 0xF) + 1;
    const unsigned rep = ((unsigned)b >> 4) + 1;
    if (index + rep > numSymbols)
      return false;
    for (unsigned j = 0; j < rep; j++)
      levels[index++] = (Byte)level;
  }
  while (--numRecords);

  if (index != numSymbols)
    return false;
  return decoder.Build(levels, numSymbols);
}


HRESULT CCoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  if (!_inBitStream.Create(1 << 18))
    return E_OUTOFMEMORY;
  if (!_outWindowStream.Create(kHistorySize << 1)) // 16 KB
    return E_OUTOFMEMORY;
  if (!outSize)
    return E_INVALIDARG;

  _outWindowStream.SetStream(outStream);
  _outWindowStream.Init(false);
  _inBitStream.SetStream(inStream);
  _inBitStream.Init();

  const unsigned numDistDirectBits = (_flags & 2) ?
      kNumDistDirectBitsBig:
      kNumDistDirectBitsSmall;
  const bool literalsOn = ((_flags & 4) != 0);
  const UInt32 minMatchLen = (literalsOn ? 3 : 2);

  if (literalsOn)
    if (!BuildHuff(_litDecoder, kLitTableSize))
      return S_FALSE;
  if (!BuildHuff(_lenDecoder, kLenTableSize))
    return S_FALSE;
  if (!BuildHuff(_distDecoder, kDistTableSize))
    return S_FALSE;
 
  UInt64 prevProgress = 0;
  bool moreOut = false;
  UInt64 pos = 0, unPackSize = *outSize;

  while (pos < unPackSize)
  {
    if (pos - prevProgress >= (1u << 18) && progress)
    {
      prevProgress = pos;
      const UInt64 packSize = _inBitStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &pos))
    }

    if (_inBitStream.ReadBits(1) != 0)
    {
      Byte b;
      if (literalsOn)
      {
        const unsigned sym = _litDecoder.Decode(&_inBitStream);
        // if (sym >= kLitTableSize) break;
        b = (Byte)sym;
      }
      else
        b = (Byte)_inBitStream.ReadBits(8);
      _outWindowStream.PutByte(b);
      pos++;
    }
    else
    {
      const UInt32 lowDistBits = _inBitStream.ReadBits(numDistDirectBits);
      UInt32 dist = (UInt32)_distDecoder.Decode(&_inBitStream);
      // if (dist >= kDistTableSize) break;
      dist = (dist << numDistDirectBits) + lowDistBits;
      unsigned len = _lenDecoder.Decode(&_inBitStream);
      // if (len >= kLenTableSize) break;
      if (len == kLenTableSize - 1)
        len += _inBitStream.ReadBits(kNumLenDirectBits);
      len += minMatchLen;
      {
        const UInt64 limit = unPackSize - pos;
        // limit != 0
        if (len > limit)
        {
          moreOut = true;
          len = (UInt32)limit;
        }
      }
      do
      {
        // len != 0
        if (dist < pos)
        {
          _outWindowStream.CopyBlock(dist, len);
          pos += len;
          break;
        }
        _outWindowStream.PutByte(0);
        pos++;
      }
      while (--len);
    }
  }

  HRESULT res = _outWindowStream.Flush();

  if (res == S_OK)
  {
    if (_fullStreamMode)
    {
      if (moreOut)
        res = S_FALSE;
      if (inSize && *inSize != _inBitStream.GetProcessedSize())
        res = S_FALSE;
    }
    if (pos != unPackSize)
      res = S_FALSE;
  }

  return res;
}


Z7_COM7F_IMF(CCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress);  }
  // catch(const CInBufferException &e)  { return e.ErrorCode; }
  // catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(const CSystemException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}


Z7_COM7F_IMF(CCoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  if (size == 0)
    return E_NOTIMPL;
  _flags = data[0];
  return S_OK;
}


Z7_COM7F_IMF(CCoder::SetFinishMode(UInt32 finishMode))
{
  _fullStreamMode = (finishMode != 0);
  return S_OK;
}


Z7_COM7F_IMF(CCoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inBitStream.GetProcessedSize();
  return S_OK;
}

}}}

/* ================ unit: CPP/7zip/Compress/LzOutWindow.cpp ================ */
// LzOutWindow.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

void CLzOutWindow::Init(bool solid) throw()
{
  if (!solid)
    COutBuffer::Init();
  #ifdef Z7_NO_EXCEPTIONS
  ErrorCode = S_OK;
  #endif
}

/* ================ unit: CPP/7zip/Compress/LzfseDecoder.cpp ================ */
// LzfseDecoder.cpp

/*
This code implements LZFSE data decompressing.
The code from "LZFSE compression library" was used.

2018      : Igor Pavlov : BSD 3-clause License : the code in this file
2015-2017 : Apple Inc   : BSD 3-clause License : original "LZFSE compression library" code

The code in the "LZFSE compression library" is licensed under the "BSD 3-clause License":
----
Copyright (c) 2015-2016, Apple Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1.  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.

3.  Neither the name of the copyright holder(s) nor the names of any contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
----
*/

// amalgamation: header emitted in prologue

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzfse {

static const Byte kSignature_LZFSE_V1 = 0x31; // '1'
static const Byte kSignature_LZFSE_V2 = 0x32; // '2'


HRESULT CDecoder::GetUInt32(UInt32 &val)
{
  Byte b[4];
  for (unsigned i = 0; i < 4; i++)
    if (!m_InStream.ReadByte(b[i]))
      return S_FALSE;
  val = GetUi32(b);
  return S_OK;
}



HRESULT CDecoder::DecodeUncompressed(UInt32 unpackSize)
{
  PRF(printf("\nUncompressed %7u\n", unpackSize));

  const unsigned kBufSize = 1 << 8;
  Byte buf[kBufSize];
  for (;;)
  {
    if (unpackSize == 0)
      return S_OK;
    UInt32 cur = unpackSize;
    if (cur > kBufSize)
      cur = kBufSize;
    const UInt32 cur2 = (UInt32)m_InStream.ReadBytes(buf, cur);
    m_OutWindowStream.PutBytes(buf, cur2);
    if (cur != cur2)
      return S_FALSE;
  }
}



HRESULT CDecoder::DecodeLzvn(UInt32 unpackSize, UInt32 packSize)
{
  PRF(printf("\nLZVN 0x%07x 0x%07x\n", unpackSize, packSize));
  
  UInt32 D = 0;

  for (;;)
  {
    if (packSize == 0)
      return S_FALSE;
    Byte b;
    if (!m_InStream.ReadByte(b))
      return S_FALSE;
    packSize--;

    UInt32 M;
    UInt32 L;

    if (b >= 0xE0)
    {
      /*
      large L   - 11100000 LLLLLLLL <LITERALS>
      small L   - 1110LLLL <LITERALS>
      
      large Rep - 11110000 MMMMMMMM
      small Rep - 1111MMMM
      */
        
      M = b & 0xF;
      if (M == 0)
      {
        if (packSize == 0)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        M = (UInt32)b1 + 16;
      }
      L = 0;
      if ((b & 0x10) == 0)
      {
        // Literals only
        L = M;
        M = 0;
      }
    }
      
    // ERROR codes
    else if ((b & 0xF0) == 0x70) // 0111xxxx
      return S_FALSE;
    else if ((b & 0xF0) == 0xD0) // 1101xxxx
      return S_FALSE;
    
    else
    {
      if ((b & 0xE0) == 0xA0)
      {
        // medium  - 101LLMMM DDDDDDMM DDDDDDDD <LITERALS>
        if (packSize < 2)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        
        Byte b2;
        if (!m_InStream.ReadByte(b2))
          return S_FALSE;
        packSize--;
        L = (((UInt32)b >> 3) & 3);
        M = (((UInt32)b & 7) << 2) + (b1 & 3);
        D = ((UInt32)b1 >> 2) + ((UInt32)b2 << 6);
      }
      else
      {
        L = (UInt32)b >> 6;
        M = ((UInt32)b >> 3) & 7;
        if ((b & 0x7) == 6)
        {
          // REP - LLMMM110 <LITERALS>
          if (L == 0)
          {
            // spec
            if (M == 0)
              break; // EOS
            if (M <= 2)
              continue; // NOP
            return S_FALSE; // UNDEFINED
          }
        }
        else
        {
          if (packSize == 0)
            return S_FALSE;
          Byte b1;
          if (!m_InStream.ReadByte(b1))
            return S_FALSE;
          packSize--;
          
          // large - LLMMM111 DDDDDDDD DDDDDDDD <LITERALS>
          // small - LLMMMDDD DDDDDDDD <LITERALS>
          D  = ((UInt32)b & 7);
          if (D == 7)
          {
            if (packSize == 0)
              return S_FALSE;
            Byte b2;
            if (!m_InStream.ReadByte(b2))
              return S_FALSE;
            packSize--;
            D = b2;
          }
          D = (D << 8) + b1;
        }
      }

      M += 3;
    }
    {
      for (unsigned i = 0; i < L; i++)
      {
        if (packSize == 0 || unpackSize == 0)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        m_OutWindowStream.PutByte(b1);
        unpackSize--;
      }
    }
    
    if (M != 0)
    {
      if (unpackSize == 0 || D == 0)
        return S_FALSE;
      unsigned cur = M;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      if (!m_OutWindowStream.CopyBlock(D - 1, cur))
        return S_FALSE;
      unpackSize -= cur;
      if (cur != M)
        return S_FALSE;
    }
  }

  if (unpackSize != 0)
    return S_FALSE;

  // LZVN encoder writes 7 additional zero bytes
  if (packSize < 7)
    return S_FALSE;
  for (unsigned i = 0; i < 7; i++)
  {
    Byte b;
    if (!m_InStream.ReadByte(b))
      return S_FALSE;
    if (b != 0)
      return S_FALSE;
  }
  packSize -= 7;
  if (packSize)
  {
    PRF(printf("packSize after unused = %u\n", packSize));
    // if (packSize <= 0x100) { Byte buf[0x100]; m_InStream.ReadBytes(buf, packSize); }
    /* Lzvn block that is used in HFS can contain junk data
       (at least 256 bytes) after payload data. Why?
       We ignore that junk data, if it's HFS (LzvnMode) mode. */
    if (!LzvnMode)
      return S_FALSE;
  }
  return S_OK;
}



// ---------- LZFSE ----------

#define MATCHES_PER_BLOCK 10000
#define LITERALS_PER_BLOCK (4 * MATCHES_PER_BLOCK)

#define NUM_L_SYMBOLS 20
#define NUM_M_SYMBOLS 20
#define NUM_D_SYMBOLS 64
#define NUM_LIT_SYMBOLS 256

#define NUM_SYMBOLS ( \
    NUM_L_SYMBOLS + \
    NUM_M_SYMBOLS + \
    NUM_D_SYMBOLS + \
    NUM_LIT_SYMBOLS)

#define NUM_L_STATES (1 << 6)
#define NUM_M_STATES (1 << 6)
#define NUM_D_STATES (1 << 8)
#define NUM_LIT_STATES (1 << 10)


typedef UInt32 CFseState;


static UInt32 SumFreqs(const UInt16 *freqs, unsigned num)
{
  UInt32 sum = 0;
  for (unsigned i = 0; i < num; i++)
    sum += (UInt32)freqs[i];
  return sum;
}


static Z7_FORCE_INLINE unsigned CountZeroBits(UInt32 val, UInt32 mask)
{
  for (unsigned i = 0;;)
  {
    if (val & mask)
      return i;
    i++;
    mask >>= 1;
  }
}


static Z7_FORCE_INLINE void InitLitTable(const UInt16 *freqs, UInt32 *table)
{
  for (unsigned i = 0; i < NUM_LIT_SYMBOLS; i++)
  {
    unsigned f = freqs[i];
    if (f == 0)
      continue;

    //         0 <   f     <= numStates
    //         0 <=  k     <= numStatesLog
    // numStates <= (f<<k) <  numStates * 2
    //         0 <  j0     <= f
    // (f + j0) = next_power_of_2 for f
    unsigned k = CountZeroBits(f, NUM_LIT_STATES);
    unsigned j0 = (((unsigned)NUM_LIT_STATES * 2) >> k) - f;

    /*
    CEntry
    {
      Byte k;
      Byte symbol;
      UInt16 delta;
    };
    */

    UInt32 e = ((UInt32)i << 8) + k;
    k += 16;
    UInt32 d = e + ((UInt32)f << k) - ((UInt32)NUM_LIT_STATES << 16);
    UInt32 step = (UInt32)1 << k;

    unsigned j = 0;
    do
    {
      *table++ = d;
      d += step;
    }
    while (++j < j0);

    e--;
    step >>= 1;
    
    for (j = j0; j < f; j++)
    {
      *table++ = e;
      e += step;
    }
  }
}


typedef struct
{
  Byte totalBits;
  Byte extraBits;
  UInt16 delta;
  UInt32 vbase;
} CExtraEntry;


static void InitExtraDecoderTable(unsigned numStates,
    unsigned numSymbols,
    const UInt16 *freqs,
    const Byte *vbits,
    CExtraEntry *table)
{
  UInt32 vbase = 0;

  for (unsigned i = 0; i < numSymbols; i++)
  {
    unsigned f = freqs[i];
    unsigned extraBits = vbits[i];

    if (f != 0)
    {
      unsigned k = CountZeroBits(f, numStates);
      unsigned j0 = ((2 * numStates) >> k) - f;
      
      unsigned j = 0;
      do
      {
        CExtraEntry *e = table++;
        e->totalBits = (Byte)(k + extraBits);
        e->extraBits = (Byte)extraBits;
        e->delta = (UInt16)(((f + j) << k) - numStates);
        e->vbase = vbase;
      }
      while (++j < j0);
      
      f -= j0;
      k--;
      
      for (j = 0; j < f; j++)
      {
        CExtraEntry *e = table++;
        e->totalBits = (Byte)(k + extraBits);
        e->extraBits = (Byte)extraBits;
        e->delta = (UInt16)(j << k);
        e->vbase = vbase;
      }
    }

    vbase += ((UInt32)1 << extraBits);
  }
}


static const Byte k_L_extra[NUM_L_SYMBOLS] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 5, 8
};

static const Byte k_M_extra[NUM_M_SYMBOLS] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 5, 8, 11
};

static const Byte k_D_extra[NUM_D_SYMBOLS] =
{
   0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,
   4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,
   8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11, 11, 11, 11,
  12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15
};



// ---------- CBitStream ----------

typedef struct
{
  UInt32 accum;
  unsigned numBits; // [0, 31] - Number of valid bits in (accum), other bits are 0
} CBitStream;


static Z7_FORCE_INLINE int FseInStream_Init(CBitStream *s,
    int n, // [-7, 0], (-n == number_of_unused_bits) in last byte
    const Byte **pbuf)
{
  *pbuf -= 4;
  s->accum = GetUi32(*pbuf);
  if (n)
  {
    s->numBits = (unsigned)(n + 32);
    if ((s->accum >> s->numBits) != 0)
      return -1; // ERROR, encoder should have zeroed the upper bits
  }
  else
  {
    *pbuf += 1;
    s->accum >>= 8;
    s->numBits = 24;
  }
  return 0; // OK
}


// 0 <= numBits < 32
#define mask31(x, numBits) ((x) & (((UInt32)1 << (numBits)) - 1))

#define FseInStream_FLUSH \
  { const unsigned nbits = (31 - in.numBits) & (unsigned)-8; \
  if (nbits) { \
    buf -= (nbits >> 3); \
    if (buf < buf_check) return S_FALSE; \
    UInt32 v = GetUi32(buf); \
    in.accum = (in.accum << nbits) | mask31(v, nbits); \
    in.numBits += nbits; }}



static Z7_FORCE_INLINE UInt32 BitStream_Pull(CBitStream *s, unsigned numBits)
{
  s->numBits -= numBits;
  const UInt32 v = s->accum >> s->numBits;
  s->accum = mask31(s->accum, s->numBits);
  return v;
}


#define DECODE_LIT(dest, pstate) { \
  UInt32 e = lit_decoder[pstate]; \
  pstate = (CFseState)((e >> 16) + BitStream_Pull(&in, e & 0xff)); \
  dest = (Byte)(e >> 8); }


static Z7_FORCE_INLINE UInt32 FseDecodeExtra(CFseState *pstate,
    const CExtraEntry *table,
    CBitStream *s)
{
  const CExtraEntry *e = &table[*pstate];
  UInt32 v = BitStream_Pull(s, e->totalBits);
  unsigned extraBits = e->extraBits;
  *pstate = (CFseState)(e->delta + (v >> extraBits));
  return e->vbase + mask31(v, extraBits);
}


#define freqs_L (freqs)
#define freqs_M (freqs_L + NUM_L_SYMBOLS)
#define freqs_D (freqs_M + NUM_M_SYMBOLS)
#define freqs_LIT (freqs_D + NUM_D_SYMBOLS)

#define GET_BITS_64(v, offset, num, dest) dest = (UInt32)   ((v >> (offset)) & ((1 << (num)) - 1));
#define GET_BITS_64_Int32(v, offset, num, dest) dest = (Int32)((v >> (offset)) & ((1 << (num)) - 1));
#define GET_BITS_32(v, offset, num, dest) dest = (CFseState)((v >> (offset)) & ((1 << (num)) - 1));


HRESULT CDecoder::DecodeLzfse(UInt32 unpackSize, Byte version)
{
  PRF(printf("\nLZFSE-%d %7u", version - '0', unpackSize));

  UInt32 numLiterals;
  UInt32 litPayloadSize;
  Int32 literal_bits;

  UInt32 lit_state_0;
  UInt32 lit_state_1;
  UInt32 lit_state_2;
  UInt32 lit_state_3;

  UInt32 numMatches;
  UInt32 lmdPayloadSize;
  Int32 lmd_bits;

  CFseState l_state;
  CFseState m_state;
  CFseState d_state;

  UInt16 freqs[NUM_SYMBOLS];

  if (version == kSignature_LZFSE_V1)
  {
    return E_NOTIMPL;
    // we need examples to test LZFSE-V1 code
    /*
    const unsigned k_v1_SubHeaderSize = 7 * 4 + 7 * 2;
    const unsigned k_v1_HeaderSize = k_v1_SubHeaderSize + NUM_SYMBOLS * 2;
    _buffer.AllocAtLeast(k_v1_HeaderSize);
    if (m_InStream.ReadBytes(_buffer, k_v1_HeaderSize) != k_v1_HeaderSize)
      return S_FALSE;

    const Byte *buf = _buffer;
    #define GET_32(offs, dest) dest = GetUi32(buf + offs)
    #define GET_16(offs, dest) dest = GetUi16(buf + offs)

    UInt32 payload_bytes;
    GET_32(0, payload_bytes);
    GET_32(4, numLiterals);
    GET_32(8, numMatches);
    GET_32(12, litPayloadSize);
    GET_32(16, lmdPayloadSize);
    if (litPayloadSize > (1 << 20) || lmdPayloadSize > (1 << 20))
      return S_FALSE;
    GET_32(20, literal_bits);
    if (literal_bits < -7 || literal_bits > 0)
      return S_FALSE;

    GET_16(24, lit_state_0);
    GET_16(26, lit_state_1);
    GET_16(28, lit_state_2);
    GET_16(30, lit_state_3);

    GET_32(32, lmd_bits);
    if (lmd_bits < -7 || lmd_bits > 0)
      return S_FALSE;

    GET_16(36, l_state);
    GET_16(38, m_state);
    GET_16(40, d_state);

    for (unsigned i = 0; i < NUM_SYMBOLS; i++)
      freqs[i] = GetUi16(buf + k_v1_SubHeaderSize + i * 2);
    */
  }
  else
  {
    UInt32 headerSize;
    {
      const unsigned kPreHeaderSize = 4 * 2; // signature and upackSize
      const unsigned kHeaderSize = 8 * 3;
      Byte temp[kHeaderSize];
      if (m_InStream.ReadBytes(temp, kHeaderSize) != kHeaderSize)
        return S_FALSE;
      
      UInt64 v;
      
      v = GetUi64(temp);
      GET_BITS_64(v,  0, 20, numLiterals)
      GET_BITS_64(v, 20, 20, litPayloadSize)
      GET_BITS_64(v, 40, 20, numMatches)
      GET_BITS_64_Int32(v, 60, 3 + 1, literal_bits) // (NumberOfUsedBits - 1)
      literal_bits -= 7; // (-NumberOfUnusedBits)
      if (literal_bits > 0)
        return S_FALSE;
      // GET_BITS_64(v, 63, 1, unused);
      
      v = GetUi64(temp + 8);
      GET_BITS_64(v,  0, 10, lit_state_0)
      GET_BITS_64(v, 10, 10, lit_state_1)
      GET_BITS_64(v, 20, 10, lit_state_2)
      GET_BITS_64(v, 30, 10, lit_state_3)
      GET_BITS_64(v, 40, 20, lmdPayloadSize)
      GET_BITS_64_Int32(v, 60, 3 + 1, lmd_bits)
      lmd_bits -= 7;
      if (lmd_bits > 0)
        return S_FALSE;
      // GET_BITS_64(v, 63, 1, unused)
      
      UInt32 v32 = GetUi32(temp + 20);
      // (total header size in bytes; this does not
      // correspond to a field in the uncompressed header version,
      // but is required; we wouldn't know the size of the
      // compresssed header otherwise.
      GET_BITS_32(v32, 0, 10, l_state)
      GET_BITS_32(v32, 10, 10, m_state)
      GET_BITS_32(v32, 20, 10 + 2, d_state)
      // GET_BITS_64(v, 62, 2, unused)
      
      headerSize = GetUi32(temp + 16);
      if (headerSize <= kPreHeaderSize + kHeaderSize)
        return S_FALSE;
      headerSize -= kPreHeaderSize + kHeaderSize;
    }

    // no freqs case is not allowed ?
    // memset(freqs, 0, sizeof(freqs));
    // if (headerSize != 0)
    {
      static const Byte numBitsTable[32] =
      {
        2, 3, 2, 5, 2, 3, 2, 8, 2, 3, 2, 5, 2, 3, 2, 14,
        2, 3, 2, 5, 2, 3, 2, 8, 2, 3, 2, 5, 2, 3, 2, 14
      };
  
      static const Byte valueTable[32] =
      {
        0, 2, 1, 4, 0, 3, 1, 8, 0, 2, 1, 5, 0, 3, 1, 24,
        0, 2, 1, 6, 0, 3, 1, 8, 0, 2, 1, 7, 0, 3, 1, 24
      };

      UInt32 accum = 0;
      unsigned numBits = 0;

      for (unsigned i = 0; i < NUM_SYMBOLS; i++)
      {
        while (numBits <= 14 && headerSize != 0)
        {
          Byte b;
          if (!m_InStream.ReadByte(b))
            return S_FALSE;
          accum |= (UInt32)b << numBits;
          numBits += 8;
          headerSize--;
        }
        
        unsigned b = (unsigned)accum & 31;
        unsigned n = numBitsTable[b];
        if (numBits < n)
          return S_FALSE;
        numBits -= n;
        UInt32 f = valueTable[b];
        if (n >= 8)
          f += ((accum >> 4) & (0x3ff >> (14 - n)));
        accum >>= n;
        freqs[i] = (UInt16)f;
      }
      
      if (numBits >= 8 || headerSize != 0)
        return S_FALSE;
    }
  }

  PRF(printf(" Literals=%6u Matches=%6u", numLiterals, numMatches));

  if (numLiterals > LITERALS_PER_BLOCK
      || (numLiterals & 3) != 0
      || numMatches > MATCHES_PER_BLOCK
      || lit_state_0 >= NUM_LIT_STATES
      || lit_state_1 >= NUM_LIT_STATES
      || lit_state_2 >= NUM_LIT_STATES
      || lit_state_3 >= NUM_LIT_STATES
      || l_state >= NUM_L_STATES
      || m_state >= NUM_M_STATES
      || d_state >= NUM_D_STATES)
    return S_FALSE;

  // only full table is allowed ?
  if (   SumFreqs(freqs_L, NUM_L_SYMBOLS) != NUM_L_STATES
      || SumFreqs(freqs_M, NUM_M_SYMBOLS) != NUM_M_STATES
      || SumFreqs(freqs_D, NUM_D_SYMBOLS) != NUM_D_STATES
      || SumFreqs(freqs_LIT, NUM_LIT_SYMBOLS) != NUM_LIT_STATES)
    return S_FALSE;


  const unsigned kPad = 16;

  // ---------- Decode literals ----------

  {
    _literals.AllocAtLeast(LITERALS_PER_BLOCK + 16);
    _buffer.AllocAtLeast(kPad + litPayloadSize);
    memset(_buffer, 0, kPad);
   
    if (m_InStream.ReadBytes(_buffer + kPad, litPayloadSize) != litPayloadSize)
      return S_FALSE;

    UInt32 lit_decoder[NUM_LIT_STATES];
    InitLitTable(freqs_LIT, lit_decoder);
    
    const Byte *buf_start = _buffer + kPad;
    const Byte *buf_check = buf_start - 4;
    const Byte *buf = buf_start + litPayloadSize;
    CBitStream in;
    if (FseInStream_Init(&in, literal_bits, &buf) != 0)
      return S_FALSE;
    
    Byte *lit = _literals;
    const Byte *lit_limit = lit + numLiterals;
    for (; lit < lit_limit; lit += 4)
    {
      FseInStream_FLUSH
      DECODE_LIT (lit[0], lit_state_0)
      DECODE_LIT (lit[1], lit_state_1)
      FseInStream_FLUSH
      DECODE_LIT (lit[2], lit_state_2)
      DECODE_LIT (lit[3], lit_state_3)
    }
    
    if ((buf_start - buf) * 8 != (int)in.numBits)
      return S_FALSE;
  }

  
  // ---------- Decode LMD ----------

  _buffer.AllocAtLeast(kPad + lmdPayloadSize);
  memset(_buffer, 0, kPad);
  if (m_InStream.ReadBytes(_buffer + kPad, lmdPayloadSize) != lmdPayloadSize)
    return S_FALSE;

  CExtraEntry l_decoder[NUM_L_STATES];
  CExtraEntry m_decoder[NUM_M_STATES];
  CExtraEntry d_decoder[NUM_D_STATES];

  InitExtraDecoderTable(NUM_L_STATES, NUM_L_SYMBOLS, freqs_L, k_L_extra, l_decoder);
  InitExtraDecoderTable(NUM_M_STATES, NUM_M_SYMBOLS, freqs_M, k_M_extra, m_decoder);
  InitExtraDecoderTable(NUM_D_STATES, NUM_D_SYMBOLS, freqs_D, k_D_extra, d_decoder);

  const Byte *buf_start = _buffer + kPad;
  const Byte *buf_check = buf_start - 4;
  const Byte *buf = buf_start + lmdPayloadSize;
  CBitStream in;
  if (FseInStream_Init(&in, lmd_bits, &buf))
    return S_FALSE;
    
  const Byte *lit = _literals;
  const Byte *lit_limit = lit + numLiterals;
  
  UInt32 D = 0;

  for (;;)
  {
    if (numMatches == 0)
      break;
    numMatches--;

    FseInStream_FLUSH

    unsigned L = (unsigned)FseDecodeExtra(&l_state, l_decoder, &in);

    FseInStream_FLUSH
    
    unsigned M = (unsigned)FseDecodeExtra(&m_state, m_decoder, &in);

    FseInStream_FLUSH

    {
      UInt32 new_D = FseDecodeExtra(&d_state, d_decoder, &in);
      if (new_D)
        D = new_D;
    }

    if (L != 0)
    {
      if (L > (size_t)(lit_limit - lit))
        return S_FALSE;
      unsigned cur = L;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      m_OutWindowStream.PutBytes(lit, cur);
      unpackSize -= cur;
      lit += cur;
      if (cur != L)
        return S_FALSE;
    }

    if (M != 0)
    {
      if (unpackSize == 0 || D == 0)
        return S_FALSE;
      unsigned cur = M;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      if (!m_OutWindowStream.CopyBlock(D - 1, cur))
        return S_FALSE;
      unpackSize -= cur;
      if (cur != M)
        return S_FALSE;
    }
  }

  if (unpackSize != 0)
    return S_FALSE;

  // LZFSE encoder writes 8 additional zero bytes before LMD payload
  // We test it:
  if ((size_t)(buf - buf_start) * 8 + in.numBits != 64)
    return S_FALSE;
  if (GetUi64(buf_start) != 0)
    return S_FALSE;

  return S_OK;
}


HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  PRF(printf("\n\nLzfseDecoder %7u %7u\n", (unsigned)*outSize, (unsigned)*inSize));

  const UInt32 kLzfseDictSize = 1 << 18;
  if (!m_OutWindowStream.Create(kLzfseDictSize))
    return E_OUTOFMEMORY;
  if (!m_InStream.Create(1 << 18))
    return E_OUTOFMEMORY;

  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(false);
  m_InStream.SetStream(inStream);
  m_InStream.Init();
  
  CCoderReleaser coderReleaser(this);

  UInt64 prevOut = 0;
  UInt64 prevIn = 0;

  if (LzvnMode)
  {
    if (!outSize || !inSize)
      return E_NOTIMPL;
    const UInt64 unpackSize = *outSize;
    const UInt64 packSize = *inSize;
    if (unpackSize > (UInt32)(Int32)-1
        || packSize > (UInt32)(Int32)-1)
      return S_FALSE;
    RINOK(DecodeLzvn((UInt32)unpackSize, (UInt32)packSize))
  }
  else
  for (;;)
  {
    const UInt64 pos = m_OutWindowStream.GetProcessedSize();
    const UInt64 packPos = m_InStream.GetProcessedSize();

    if (progress && ((pos - prevOut) >= (1 << 22) || (packPos - prevIn) >= (1 << 22)))
    {
      RINOK(progress->SetRatioInfo(&packPos, &pos))
      prevIn = packPos;
      prevOut = pos;
    }

    UInt32 v;
    RINOK(GetUInt32(v))
    if ((v & 0xFFFFFF) != 0x787662) // bvx
      return S_FALSE;
    v >>= 24;
    
    if (v == 0x24) // '$', end of stream
      break;
    
    UInt32 unpackSize;
    RINOK(GetUInt32(unpackSize))

    UInt32 cur = unpackSize;
    if (outSize)
    {
      const UInt64 rem = *outSize - pos;
      if (cur > rem)
        cur = (UInt32)rem;
    }
    unpackSize -= cur;
    
    HRESULT res;
    if (v == kSignature_LZFSE_V1 || v == kSignature_LZFSE_V2)
      res = DecodeLzfse(cur, (Byte)v);
    else if (v == 0x6E) // 'n'
    {
      UInt32 packSize;
      res = GetUInt32(packSize);
      if (res == S_OK)
        res = DecodeLzvn(cur, packSize);
    }
    else if (v == 0x2D) // '-'
      res = DecodeUncompressed(cur);
    else
      return E_NOTIMPL;
    
    if (res != S_OK)
      return res;
    
    if (unpackSize != 0)
      return S_FALSE;
  }
  
  coderReleaser.NeedFlush = false;
  HRESULT res = m_OutWindowStream.Flush();
  if (res == S_OK)
    if ((!LzvnMode && inSize && *inSize != m_InStream.GetProcessedSize())
        || (outSize && *outSize != m_OutWindowStream.GetProcessedSize()))
      res = S_FALSE;
  return res;
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return E_OUTOFMEMORY; }
  // catch(...) { return S_FALSE; }
}

}}

/* ================ unit: CPP/7zip/Compress/LzhDecoder.cpp ================ */
// LzhDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress{
namespace NLzh {
namespace NDecoder {

static const UInt32 kWindowSizeMin = 1 << 16;

bool CCoder::ReadTP(unsigned num, unsigned numBits, int spec)
{
  _symbolT = -1;

  const unsigned n = (unsigned)_inBitStream.ReadBits(numBits);
  if (n == 0)
  {
    const unsigned s = (unsigned)_inBitStream.ReadBits(numBits);
    _symbolT = (int)s;
    return (s < num);
  }
  if (n > num)
    return false;

  {
    Byte lens[NPT];
    unsigned i;
    for (i = 0; i < NPT; i++)
      lens[i] = 0;
    i = 0;
    do
    {
      unsigned val = (unsigned)_inBitStream.GetValue(16);
      unsigned c = val >> 13;
      unsigned mov = 3;
      if (c == 7)
      {
        while (val & (1 << 12))
        {
          val += val;
          c++;
        }
        if (c > 16)
          return false;
        mov = c - 3;
      }
      lens[i++] = (Byte)c;
      _inBitStream.MovePos(mov);
      if ((int)i == spec)
        i += _inBitStream.ReadBits(2);
    }
    while (i < n);
    
    return _decoderT.Build(lens, NHuffman::k_BuildMode_Full);
  }
}

static const unsigned NUM_C_BITS = 9;

bool CCoder::ReadC()
{
  _symbolC = -1;

  const unsigned n = (unsigned)_inBitStream.ReadBits(NUM_C_BITS);
  if (n == 0)
  {
    const unsigned s = (unsigned)_inBitStream.ReadBits(NUM_C_BITS);
    _symbolC = (int)s;
    return (s < NC);
  }
  if (n > NC)
    return false;

  {
    Byte lens[NC];
    unsigned i = 0;
    do
    {
      unsigned c = (unsigned)_symbolT;
      if (_symbolT < 0)
        c = _decoderT.DecodeFull(&_inBitStream);
      
      if (c <= 2)
      {
        if (c == 0)
          c = 1;
        else if (c == 1)
          c = _inBitStream.ReadBits(4) + 3;
        else
          c = _inBitStream.ReadBits(NUM_C_BITS) + 20;
    
        if (i + c > n)
          return false;
        
        do
          lens[i++] = 0;
        while (--c);
      }
      else
        lens[i++] = (Byte)(c - 2);
    }
    while (i < n);
    
    while (i < NC) lens[i++] = 0;
    return _decoderC.Build(lens, /* n, */ NHuffman::k_BuildMode_Full);
  }
}

HRESULT CCoder::CodeReal(UInt32 rem, ICompressProgressInfo *progress)
{
  UInt32 blockSize = 0;

  while (rem != 0)
  {
    if (blockSize == 0)
    {
      if (_inBitStream.ExtraBitsWereRead())
        return S_FALSE;
      if (progress)
      {
        const UInt64 packSize = _inBitStream.GetProcessedSize();
        const UInt64 pos = _outWindow.GetProcessedSize();
        RINOK(progress->SetRatioInfo(&packSize, &pos))
      }
      
      blockSize = _inBitStream.ReadBits(16);
      if (blockSize == 0)
        return S_FALSE;
      
      if (!ReadTP(NT, 5, 3))
        return S_FALSE;
      if (!ReadC())
        return S_FALSE;
      const unsigned pbit = (DictSize <= (1 << 14) ? 4 : 5);
      if (!ReadTP(NP, pbit, -1))
        return S_FALSE;
    }
  
    blockSize--;

    unsigned number = (unsigned)_symbolC;
    if (_symbolC < 0)
      number = _decoderC.DecodeFull(&_inBitStream);

    if (number < 256)
    {
      _outWindow.PutByte((Byte)number);
      rem--;
    }
    else
    {
      const unsigned len = number - 256 + kMatchMinLen;

      UInt32 dist = (UInt32)(unsigned)_symbolT;
      if (_symbolT < 0)
        dist = (UInt32)_decoderT.DecodeFull(&_inBitStream);
      
      if (dist > 1)
      {
        dist--;
        dist = ((UInt32)1 << dist) + _inBitStream.ReadBits((unsigned)dist);
      }
      
      if (dist >= DictSize)
        return S_FALSE;

      if (len > rem)
      {
        // if (FinishMode)
        return S_FALSE;
        // len = (unsigned)rem;
      }

      if (!_outWindow.CopyBlock(dist, len))
        return S_FALSE;
      rem -= len;
    }
  }

  // if (FinishMode)
  {
    if (blockSize != 0)
      return S_FALSE;
    if (_inBitStream.ReadAlignBits() != 0)
      return S_FALSE;
  }
  if (_inBitStream.ExtraBitsWereRead())
    return S_FALSE;
  return S_OK;
}


HRESULT CCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt32 outSize, ICompressProgressInfo *progress)
{
  try
  {
    if (!_outWindow.Create(DictSize > kWindowSizeMin ? DictSize : kWindowSizeMin))
      return E_OUTOFMEMORY;
    if (!_inBitStream.Create(1 << 17))
      return E_OUTOFMEMORY;
    _outWindow.SetStream(outStream);
    _outWindow.Init(false);
    _inBitStream.SetStream(inStream);
    _inBitStream.Init();
    {
      CCoderReleaser coderReleaser(this);
      RINOK(CodeReal(outSize, progress))
      coderReleaser.Disable();
    }
    return _outWindow.Flush();
  }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

}}}

/* ================ unit: CPP/7zip/Compress/Lzma2Decoder.cpp ================ */
// Lzma2Decoder.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue
// #include "../../../C/CpuTicks.h"

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzma2 {

CDecoder::CDecoder():
      _dec(NULL)
    , _inProcessed(0)
    , _prop(0xFF)
    , _finishMode(false)
    , _inBufSize(1 << 20)
    , _outStep(1 << 20)
    #ifndef Z7_ST
    , _tryMt(1)
    , _numThreads(1)
    , _memUsage((UInt64)(sizeof(size_t)) << 28)
    #endif
{}

CDecoder::~CDecoder()
{
  if (_dec)
    Lzma2DecMt_Destroy(_dec);
}

Z7_COM7F_IMF(CDecoder::SetInBufSize(UInt32 , UInt32 size)) { _inBufSize = size; return S_OK; }
Z7_COM7F_IMF(CDecoder::SetOutBufSize(UInt32 , UInt32 size)) { _outStep = size; return S_OK; }

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *prop, UInt32 size))
{
  if (size != 1)
    return E_NOTIMPL;
  if (prop[0] > 40)
    return E_NOTIMPL;
  _prop = prop[0];
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  _finishMode = (finishMode != 0);
  return S_OK;
}



#ifndef Z7_ST

static UInt64 Get_ExpectedBlockSize_From_Dict(UInt32 dictSize)
{
  const UInt32 kMinSize = (UInt32)1 << 20;
  const UInt32 kMaxSize = (UInt32)1 << 28;
  UInt64 blockSize = (UInt64)dictSize << 2;
  if (blockSize < kMinSize) blockSize = kMinSize;
  if (blockSize > kMaxSize) blockSize = kMaxSize;
  if (blockSize < dictSize) blockSize = dictSize;
  blockSize += (kMinSize - 1);
  blockSize &= ~(UInt64)(kMinSize - 1);
  return blockSize;
}

#define LZMA2_DIC_SIZE_FROM_PROP_FULL(p) ((p) == 40 ? 0xFFFFFFFF : (((UInt32)2 | ((p) & 1)) << ((p) / 2 + 11)))

#endif

#define RET_IF_WRAP_ERROR_CONFIRMED(wrapRes, sRes, sResErrorCode) \
  if (wrapRes != S_OK && sRes == sResErrorCode) return wrapRes;

#define RET_IF_WRAP_ERROR(wrapRes, sRes, sResErrorCode) \
  if (wrapRes != S_OK /* && (sRes == SZ_OK || sRes == sResErrorCode) */) return wrapRes;

Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  _inProcessed = 0;

  if (!_dec)
  {
    _dec = Lzma2DecMt_Create(
      // &g_AlignedAlloc,
      &g_Alloc,
      &g_MidAlloc);
    if (!_dec)
      return E_OUTOFMEMORY;
  }

  CLzma2DecMtProps props;
  Lzma2DecMtProps_Init(&props);

  props.inBufSize_ST = _inBufSize;
  props.outStep_ST = _outStep;

  #ifndef Z7_ST
  {
    props.numThreads = 1;
    UInt32 numThreads = _numThreads;

    if (_tryMt && numThreads >= 1)
    {
      const UInt64 useLimit = _memUsage;
      const UInt32 dictSize = LZMA2_DIC_SIZE_FROM_PROP_FULL(_prop);
      const UInt64 expectedBlockSize64 = Get_ExpectedBlockSize_From_Dict(dictSize);
      const size_t expectedBlockSize = (size_t)expectedBlockSize64;
      const size_t inBlockMax = expectedBlockSize + expectedBlockSize / 16;
      if (expectedBlockSize == expectedBlockSize64 && inBlockMax >= expectedBlockSize)
      {
        props.outBlockMax = expectedBlockSize;
        props.inBlockMax = inBlockMax;
        const size_t kOverheadSize = props.inBufSize_MT + (1 << 16);
        const UInt64 okThreads = useLimit / (props.outBlockMax + props.inBlockMax + kOverheadSize);
        if (numThreads > okThreads)
          numThreads = (UInt32)okThreads;
        if (numThreads == 0)
          numThreads = 1;
        props.numThreads = numThreads;
      }
    }
  }
  #endif

  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;

  inWrap.Init(inStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);

  SRes res;

  UInt64 inProcessed = 0;
  int isMT = False;

  #ifndef Z7_ST
  isMT = _tryMt;
  #endif

  // UInt64 cpuTicks = GetCpuTicks();

  res = Lzma2DecMt_Decode(_dec, _prop, &props,
      &outWrap.vt, outSize, _finishMode,
      &inWrap.vt,
      &inProcessed,
      &isMT,
      progress ? &progressWrap.vt : NULL);

  /*
  cpuTicks = GetCpuTicks() - cpuTicks;
  printf("\n             ticks = %10I64u\n", cpuTicks / 1000000);
  */


  #ifndef Z7_ST
  /* we reset _tryMt, only if p->props.numThreads was changed */
  if (props.numThreads > 1)
    _tryMt = isMT;
  #endif

  _inProcessed = inProcessed;

  RET_IF_WRAP_ERROR(progressWrap.Res, res, SZ_ERROR_PROGRESS)
  RET_IF_WRAP_ERROR(outWrap.Res, res, SZ_ERROR_WRITE)
  RET_IF_WRAP_ERROR_CONFIRMED(inWrap.Res, res, SZ_ERROR_READ)

  if (res == SZ_OK && _finishMode)
  {
    if (inSize && *inSize != inProcessed)
      res = SZ_ERROR_DATA;
    if (outSize && *outSize != outWrap.Processed)
      res = SZ_ERROR_DATA;
  }

  return SResToHRESULT(res);
}


Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inProcessed;
  return S_OK;
}


#ifndef Z7_ST

Z7_COM7F_IMF(CDecoder::SetNumberOfThreads(UInt32 numThreads))
{
  _numThreads = numThreads;
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::SetMemLimit(UInt64 memUsage))
{
  _memUsage = memUsage;
  return S_OK;
}

#endif


#ifndef Z7_NO_READ_FROM_CODER

Z7_COM7F_IMF(CDecoder::SetOutStreamSize(const UInt64 *outSize))
{
  CLzma2DecMtProps props;
  Lzma2DecMtProps_Init(&props);
  props.inBufSize_ST = _inBufSize;
  props.outStep_ST = _outStep;

  _inProcessed = 0;

  if (!_dec)
  {
    _dec = Lzma2DecMt_Create(&g_AlignedAlloc, &g_MidAlloc);
    if (!_dec)
      return E_OUTOFMEMORY;
  }

  _inWrap.Init(_inStream);

  const SRes res = Lzma2DecMt_Init(_dec, _prop, &props, outSize, _finishMode, &_inWrap.vt);

  if (res != SZ_OK)
    return SResToHRESULT(res);
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::SetInStream(ISequentialInStream *inStream))
  { _inStream = inStream; return S_OK; }
Z7_COM7F_IMF(CDecoder::ReleaseInStream())
  { _inStream.Release(); return S_OK; }
  

Z7_COM7F_IMF(CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;

  size_t size2 = size;
  UInt64 inProcessed = 0;

  const SRes res = Lzma2DecMt_Read(_dec, (Byte *)data, &size2, &inProcessed);

  _inProcessed += inProcessed;
  if (processedSize)
    *processedSize = (UInt32)size2;
  if (res != SZ_OK)
    return SResToHRESULT(res);
  return S_OK;
}

#endif

}}

/* ================ unit: CPP/7zip/Compress/Lzma2Encoder.cpp ================ */
// Lzma2Encoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {

namespace NLzma {

HRESULT SetLzmaProp(PROPID propID, const PROPVARIANT &prop, CLzmaEncProps &ep);

}

namespace NLzma2 {

CEncoder::CEncoder()
{
  _encoder = NULL;
  _encoder = Lzma2Enc_Create(&g_AlignedAlloc, &g_BigAlloc);
  if (!_encoder)
    throw 1;
}

CEncoder::~CEncoder()
{
  if (_encoder)
    Lzma2Enc_Destroy(_encoder);
}


HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props);
HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props)
{
  switch (propID)
  {
    case NCoderPropID::kBlockSize:
    {
      if (prop.vt == VT_UI4)
        lzma2Props.blockSize = prop.ulVal;
      else if (prop.vt == VT_UI8)
        lzma2Props.blockSize = prop.uhVal.QuadPart;
      else
        return E_INVALIDARG;
      break;
    }
    case NCoderPropID::kNumThreads:
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      lzma2Props.numTotalThreads = (int)prop.ulVal;
      break;
    case NCoderPropID::kNumThreadGroups:
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      // 16-bit value supported by Windows
      if (prop.ulVal >= (1u << 16))
        return E_INVALIDARG;
      lzma2Props.numThreadGroups = (unsigned)prop.ulVal;
      break;
    default:
      RINOK(NLzma::SetLzmaProp(propID, prop, lzma2Props.lzmaProps))
  }
  return S_OK;
}


Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  CLzma2EncProps lzma2Props;
  Lzma2EncProps_Init(&lzma2Props);

  for (UInt32 i = 0; i < numProps; i++)
  {
    RINOK(SetLzma2Prop(propIDs[i], coderProps[i], lzma2Props))
  }
  return SResToHRESULT(Lzma2Enc_SetProps(_encoder, &lzma2Props));
}


Z7_COM7F_IMF(CEncoder::SetCoderPropertiesOpt(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];
    if (propID == NCoderPropID::kExpectedDataSize)
      if (prop.vt == VT_UI8)
        Lzma2Enc_SetDataSize(_encoder, prop.uhVal.QuadPart);
  }
  return S_OK;
}


Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  const Byte prop = Lzma2Enc_WriteProperties(_encoder);
  return WriteStream(outStream, &prop, 1);
}


#define RET_IF_WRAP_ERROR(wrapRes, sRes, sResErrorCode) \
  if (wrapRes != S_OK /* && (sRes == SZ_OK || sRes == sResErrorCode) */) return wrapRes;

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;

  inWrap.Init(inStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);

  SRes res = Lzma2Enc_Encode2(_encoder,
      &outWrap.vt, NULL, NULL,
      &inWrap.vt, NULL, 0,
      progress ? &progressWrap.vt : NULL);

  RET_IF_WRAP_ERROR(inWrap.Res, res, SZ_ERROR_READ)
  RET_IF_WRAP_ERROR(outWrap.Res, res, SZ_ERROR_WRITE)
  RET_IF_WRAP_ERROR(progressWrap.Res, res, SZ_ERROR_PROGRESS)

  return SResToHRESULT(res);
}
  
}}

/* ================ unit: CPP/7zip/Compress/LzmaDecoder.cpp ================ */
// LzmaDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: renamed from SResToHRESULT. Upstream LzmaDecoder.cpp defines
// this file-local and deliberately does not include CWrappers.h, but the
// prologue hoists that header, so the name collided with the extern declaration
// (declared extern then static -- accepted by MSVC with C4211, hard error on
// GCC/Clang). The two mappings are NOT equivalent: CWrappers' sends
// SZ_ERROR_CRC/INPUT_EOF/ARCHIVE/NO_ARCHIVE to S_FALSE and SZ_ERROR_PROGRESS to
// E_ABORT, so merging them would change error reporting. Only the one caller
// that is also from LzmaDecoder.cpp uses this; every other call site in this
// amalgamation belongs to a different upstream unit and keeps CWrappers'.
// Same treatment as SResToHRESULT_Code below.
static HRESULT SResToHRESULT_LzmaDec(SRes res)
{
  switch (res)
  {
    case SZ_OK: return S_OK;
    case SZ_ERROR_MEM: return E_OUTOFMEMORY;
    case SZ_ERROR_PARAM: return E_INVALIDARG;
    case SZ_ERROR_UNSUPPORTED: return E_NOTIMPL;
    case SZ_ERROR_DATA: return S_FALSE;
    default: break;
  }
  return E_FAIL;
}

namespace NCompress {
namespace NLzma {

CDecoder::CDecoder():
    FinishStream(false),
    _propsWereSet(false),
    _outSizeDefined(false),
    _outStep(1 << 20),
    _inBufSize(0),
    _inBufSizeNew(1 << 20),
    _lzmaStatus(LZMA_STATUS_NOT_SPECIFIED),
    _inBuf(NULL)
{
  _inProcessed = 0;
  _inPos = _inLim = 0;

  /*
  AlignOffsetAlloc_CreateVTable(&_alloc);
  _alloc.numAlignBits = 7;
  _alloc.offset = 0;
  */
  LzmaDec_CONSTRUCT(&_state)
}

CDecoder::~CDecoder()
{
  LzmaDec_Free(&_state, &g_AlignedAlloc); // &_alloc.vt
  MyFree(_inBuf);
}

Z7_COM7F_IMF(CDecoder::SetInBufSize(UInt32 , UInt32 size))
  { _inBufSizeNew = size; return S_OK; }
Z7_COM7F_IMF(CDecoder::SetOutBufSize(UInt32 , UInt32 size))
  { _outStep = size; return S_OK; }

HRESULT CDecoder::CreateInputBuffer()
{
  if (!_inBuf || _inBufSizeNew != _inBufSize)
  {
    MyFree(_inBuf);
    _inBufSize = 0;
    _inBuf = (Byte *)MyAlloc(_inBufSizeNew);
    if (!_inBuf)
      return E_OUTOFMEMORY;
    _inBufSize = _inBufSizeNew;
  }
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *prop, UInt32 size))
{
  RINOK(SResToHRESULT_LzmaDec(LzmaDec_Allocate(&_state, prop, size, &g_AlignedAlloc))) // &_alloc.vt
  _propsWereSet = true;
  return CreateInputBuffer();
}


void CDecoder::SetOutStreamSizeResume(const UInt64 *outSize)
{
  _outSizeDefined = (outSize != NULL);
  _outSize = 0;
  if (_outSizeDefined)
    _outSize = *outSize;
  _outProcessed = 0;
  _lzmaStatus = LZMA_STATUS_NOT_SPECIFIED;

  LzmaDec_Init(&_state);
}


Z7_COM7F_IMF(CDecoder::SetOutStreamSize(const UInt64 *outSize))
{
  _inProcessed = 0;
  _inPos = _inLim = 0;
  SetOutStreamSizeResume(outSize);
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  FinishStream = (finishMode != 0);
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inProcessed;
  return S_OK;
}


HRESULT CDecoder::CodeSpec(ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  if (!_inBuf || !_propsWereSet)
    return S_FALSE;
  
  const UInt64 startInProgress = _inProcessed;
  SizeT wrPos = _state.dicPos;
  HRESULT readRes = S_OK;

  for (;;)
  {
    if (_inPos == _inLim && readRes == S_OK)
    {
      _inPos = _inLim = 0;
      readRes = inStream->Read(_inBuf, _inBufSize, &_inLim);
    }

    const SizeT dicPos = _state.dicPos;
    SizeT size;
    {
      SizeT next = _state.dicBufSize;
      if (next - wrPos > _outStep)
        next = wrPos + _outStep;
      size = next - dicPos;
    }

    ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
    if (_outSizeDefined)
    {
      const UInt64 rem = _outSize - _outProcessed;
      if (size >= rem)
      {
        size = (SizeT)rem;
        if (FinishStream)
          finishMode = LZMA_FINISH_END;
      }
    }

    SizeT inProcessed = _inLim - _inPos;
    ELzmaStatus status;

    const SRes res = LzmaDec_DecodeToDic(&_state, dicPos + size, _inBuf + _inPos, &inProcessed, finishMode, &status);

    _lzmaStatus = status;
    _inPos += (UInt32)inProcessed;
    _inProcessed += inProcessed;
    const SizeT outProcessed = _state.dicPos - dicPos;
    _outProcessed += outProcessed;

    // we check for LZMA_STATUS_NEEDS_MORE_INPUT to allow RangeCoder initialization, if (_outSizeDefined && _outSize == 0)
    const bool outFinished = (_outSizeDefined && _outProcessed >= _outSize);

    const bool needStop = (res != 0
        || (inProcessed == 0 && outProcessed == 0)
        || status == LZMA_STATUS_FINISHED_WITH_MARK
        || (outFinished && status != LZMA_STATUS_NEEDS_MORE_INPUT));

    if (needStop || outProcessed >= size)
    {
      const HRESULT res2 = WriteStream(outStream, _state.dic + wrPos, _state.dicPos - wrPos);

      if (_state.dicPos == _state.dicBufSize)
        _state.dicPos = 0;
      wrPos = _state.dicPos;
      
      RINOK(res2)

      if (needStop)
      {
        if (res != 0)
        {
          // return SResToHRESULT(res);
          return S_FALSE;
        }

        if (status == LZMA_STATUS_FINISHED_WITH_MARK)
        {
          if (FinishStream)
            if (_outSizeDefined && _outSize != _outProcessed)
              return S_FALSE;
          return readRes;
        }
        
        if (outFinished && status != LZMA_STATUS_NEEDS_MORE_INPUT)
          if (!FinishStream || status == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK)
            return readRes;
          
        return S_FALSE;
      }
    }
    
    if (progress)
    {
      const UInt64 inSize = _inProcessed - startInProgress;
      RINOK(progress->SetRatioInfo(&inSize, &_outProcessed))
    }
  }
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  if (!_inBuf)
    return E_INVALIDARG;
  SetOutStreamSize(outSize);
  HRESULT res = CodeSpec(inStream, outStream, progress);
  if (res == S_OK)
    if (FinishStream && inSize && *inSize != _inProcessed)
      res = S_FALSE;
  return res;
}


#ifndef Z7_NO_READ_FROM_CODER

Z7_COM7F_IMF(CDecoder::SetInStream(ISequentialInStream *inStream))
  { _inStream = inStream; return S_OK; }
Z7_COM7F_IMF(CDecoder::ReleaseInStream())
  { _inStream.Release(); return S_OK; }

Z7_COM7F_IMF(CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;

  ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
  if (_outSizeDefined)
  {
    const UInt64 rem = _outSize - _outProcessed;
    if (size >= rem)
    {
      size = (UInt32)rem;
      if (FinishStream)
        finishMode = LZMA_FINISH_END;
    }
  }

  HRESULT readRes = S_OK;
  
  for (;;)
  {
    if (_inPos == _inLim && readRes == S_OK)
    {
      _inPos = _inLim = 0;
      readRes = _inStream->Read(_inBuf, _inBufSize, &_inLim);
    }

    SizeT inProcessed = _inLim - _inPos;
    SizeT outProcessed = size;
    ELzmaStatus status;
    
    const SRes res = LzmaDec_DecodeToBuf(&_state, (Byte *)data, &outProcessed,
        _inBuf + _inPos, &inProcessed, finishMode, &status);
    
    _lzmaStatus = status;
    _inPos += (UInt32)inProcessed;
    _inProcessed += inProcessed;
    _outProcessed += outProcessed;
    size -= (UInt32)outProcessed;
    data = (Byte *)data + outProcessed;
    if (processedSize)
      *processedSize += (UInt32)outProcessed;

    if (res != 0)
      return S_FALSE;
    
    /*
    if (status == LZMA_STATUS_FINISHED_WITH_MARK)
      return readRes;

    if (size == 0 && status != LZMA_STATUS_NEEDS_MORE_INPUT)
    {
      if (FinishStream
          && _outSizeDefined && _outProcessed >= _outSize
          && status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK)
        return S_FALSE;
      return readRes;
    }
    */

    if (inProcessed == 0 && outProcessed == 0)
      return readRes;
  }
}


HRESULT CDecoder::CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  SetOutStreamSizeResume(outSize);
  return CodeSpec(_inStream, outStream, progress);
}


HRESULT CDecoder::ReadFromInputStream(void *data, UInt32 size, UInt32 *processedSize)
{
  RINOK(CreateInputBuffer())
  
  if (processedSize)
    *processedSize = 0;

  HRESULT readRes = S_OK;

  while (size != 0)
  {
    if (_inPos == _inLim)
    {
      _inPos = _inLim = 0;
      if (readRes == S_OK)
        readRes = _inStream->Read(_inBuf, _inBufSize, &_inLim);
      if (_inLim == 0)
        break;
    }

    UInt32 cur = _inLim - _inPos;
    if (cur > size)
      cur = size;
    memcpy(data, _inBuf + _inPos, cur);
    _inPos += cur;
    _inProcessed += cur;
    size -= cur;
    data = (Byte *)data + cur;
    if (processedSize)
      *processedSize += cur;
  }
  
  return readRes;
}

#endif

}}

/* ================ unit: CPP/7zip/Compress/LzmaEncoder.cpp ================ */
// LzmaEncoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// #define LOG_LZMA_THREADS

#ifdef LOG_LZMA_THREADS

#include <stdio.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

EXTERN_C_BEGIN
void LzmaEnc_GetLzThreads(CLzmaEncHandle pp, HANDLE lz_threads[2]);
EXTERN_C_END

#endif

namespace NCompress {
namespace NLzma {

CEncoder::CEncoder()
{
  _encoder = NULL;
  _encoder = LzmaEnc_Create(&g_AlignedAlloc);
  if (!_encoder)
    throw 1;
}

CEncoder::~CEncoder()
{
  if (_encoder)
    LzmaEnc_Destroy(_encoder, &g_AlignedAlloc, &g_BigAlloc);
}

static inline wchar_t GetLowCharFast(wchar_t c)
{
  return c |= 0x20;
}

static int ParseMatchFinder(const wchar_t *s, int *btMode, int *numHashBytes)
{
  const wchar_t c = GetLowCharFast(*s++);
  if (c == 'h')
  {
    if (GetLowCharFast(*s++) != 'c')
      return 0;
    const int num = (int)(*s++ - L'0');
    if (num < 4 || num > 5)
      return 0;
    if (*s != 0)
      return 0;
    *btMode = 0;
    *numHashBytes = num;
    return 1;
  }

  if (c != 'b')
    return 0;
  {
    if (GetLowCharFast(*s++) != 't')
      return 0;
    const int num = (int)(*s++ - L'0');
    if (num < 2 || num > 5)
      return 0;
    if (*s != 0)
      return 0;
    *btMode = 1;
    *numHashBytes = num;
    return 1;
  }
}

#define SET_PROP_32(_id_, _dest_) case NCoderPropID::_id_: ep._dest_ = (int)v; break;
#define SET_PROP_32U(_id_, _dest_) case NCoderPropID::_id_: ep._dest_ = v; break;

HRESULT SetLzmaProp(PROPID propID, const PROPVARIANT &prop, CLzmaEncProps &ep);
HRESULT SetLzmaProp(PROPID propID, const PROPVARIANT &prop, CLzmaEncProps &ep)
{
  if (propID == NCoderPropID::kMatchFinder)
  {
    if (prop.vt != VT_BSTR)
      return E_INVALIDARG;
    return ParseMatchFinder(prop.bstrVal, &ep.btMode, &ep.numHashBytes) ? S_OK : E_INVALIDARG;
  }

  if (propID == NCoderPropID::kAffinity)
  {
    if (prop.vt == VT_UI8)
      ep.affinity = prop.uhVal.QuadPart;
    else
      return E_INVALIDARG;
    return S_OK;
  }

  if (propID == NCoderPropID::kAffinityInGroup)
  {
    if (prop.vt == VT_UI8)
      ep.affinityInGroup = prop.uhVal.QuadPart;
    else
      return E_INVALIDARG;
    return S_OK;
  }

  if (propID == NCoderPropID::kThreadGroup)
  {
    if (prop.vt == VT_UI4)
      ep.affinityGroup = (Int32)(UInt32)prop.ulVal;
    else
      return E_INVALIDARG;
    return S_OK;
  }

  if (propID == NCoderPropID::kHashBits)
  {
    if (prop.vt == VT_UI4)
      ep.numHashOutBits = prop.ulVal;
    else
      return E_INVALIDARG;
    return S_OK;
  }

  if (propID > NCoderPropID::kReduceSize)
    return S_OK;
  
  if (propID == NCoderPropID::kReduceSize)
  {
    if (prop.vt == VT_UI8)
      ep.reduceSize = prop.uhVal.QuadPart;
    else
      return E_INVALIDARG;
    return S_OK;
  }

  if (propID == NCoderPropID::kDictionarySize)
  {
    if (prop.vt == VT_UI8)
    {
      // 21.03 : we support 64-bit VT_UI8 for dictionary and (dict == 4 GiB)
      const UInt64 v = prop.uhVal.QuadPart;
      if (v > ((UInt64)1 << 32))
        return E_INVALIDARG;
      UInt32 dict;
      if (v == ((UInt64)1 << 32))
        dict = (UInt32)(Int32)-1;
      else
        dict = (UInt32)v;
      ep.dictSize = dict;
      return S_OK;
    }
  }

  if (prop.vt != VT_UI4)
    return E_INVALIDARG;
  const UInt32 v = prop.ulVal;
  switch (propID)
  {
    case NCoderPropID::kDefaultProp:
      if (v > 32)
        return E_INVALIDARG;
      ep.dictSize = (v == 32) ? (UInt32)(Int32)-1 : (UInt32)1 << (unsigned)v;
      break;
    SET_PROP_32(kLevel, level)
    SET_PROP_32(kNumFastBytes, fb)
    SET_PROP_32U(kMatchFinderCycles, mc)
    SET_PROP_32(kAlgorithm, algo)
    SET_PROP_32U(kDictionarySize, dictSize)
    SET_PROP_32(kPosStateBits, pb)
    SET_PROP_32(kLitPosBits, lp)
    SET_PROP_32(kLitContextBits, lc)
    SET_PROP_32(kNumThreads, numThreads)
    default: return E_INVALIDARG;
  }
  return S_OK;
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  CLzmaEncProps props;
  LzmaEncProps_Init(&props);

  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];
    switch (propID)
    {
      case NCoderPropID::kEndMarker:
        if (prop.vt != VT_BOOL)
          return E_INVALIDARG;
        props.writeEndMark = (prop.boolVal != VARIANT_FALSE);
        break;
      default:
        RINOK(SetLzmaProp(propID, prop, props))
    }
  }
  return SResToHRESULT(LzmaEnc_SetProps(_encoder, &props));
}


Z7_COM7F_IMF(CEncoder::SetCoderPropertiesOpt(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];
    if (propID == NCoderPropID::kExpectedDataSize)
      if (prop.vt == VT_UI8)
        LzmaEnc_SetDataSize(_encoder, prop.uhVal.QuadPart);
  }
  return S_OK;
}


Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  Byte props[LZMA_PROPS_SIZE];
  SizeT size = LZMA_PROPS_SIZE;
  RINOK(LzmaEnc_WriteProperties(_encoder, props, &size))
  return WriteStream(outStream, props, size);
}


#define RET_IF_WRAP_ERROR(wrapRes, sRes, sResErrorCode) \
  if (wrapRes != S_OK /* && (sRes == SZ_OK || sRes == sResErrorCode) */) return wrapRes;



#ifdef LOG_LZMA_THREADS

static inline UInt64 GetTime64(const FILETIME &t) { return ((UInt64)t.dwHighDateTime << 32) | t.dwLowDateTime; }

static void PrintNum(UInt64 val, unsigned numDigits, char c = ' ')
{
  char temp[64];
  char *p = temp + 32;
  ConvertUInt64ToString(val, p);
  unsigned len = (unsigned)strlen(p);
  for (; len < numDigits; len++)
    *--p = c;
  printf("%s", p);
}

static void PrintTime(const char *s, UInt64 val, UInt64 total)
{
  printf("  %s :", s);
  const UInt32 kFreq = 10000000;
  UInt64 sec = val / kFreq;
  PrintNum(sec, 6);
  printf(" .");
  UInt32 ms = (UInt32)(val - (sec * kFreq)) / (kFreq / 1000);
  PrintNum(ms, 3, '0');
  
  while (val > ((UInt64)1 << 56))
  {
    val >>= 1;
    total >>= 1;
  }

  UInt64 percent = 0;
  if (total != 0)
    percent = val * 100 / total;
  printf("  =");
  PrintNum(percent, 4);
  printf("%%");
}


struct CBaseStat
{
  UInt64 kernelTime, userTime;
  
  BOOL Get(HANDLE thread, const CBaseStat *prevStat)
  {
    FILETIME creationTimeFT, exitTimeFT, kernelTimeFT, userTimeFT;
    BOOL res = GetThreadTimes(thread
      , &creationTimeFT, &exitTimeFT, &kernelTimeFT, &userTimeFT);
    if (res)
    {
      kernelTime = GetTime64(kernelTimeFT);
      userTime = GetTime64(userTimeFT);
      if (prevStat)
      {
        kernelTime -= prevStat->kernelTime;
        userTime -= prevStat->userTime;
      }
    }
    return res;
  }
};


static void PrintStat(HANDLE thread, UInt64 totalTime, const CBaseStat *prevStat)
{
  CBaseStat newStat;
  if (!newStat.Get(thread, prevStat))
    return;

  PrintTime("K", newStat.kernelTime, totalTime);

  const UInt64 processTime = newStat.kernelTime + newStat.userTime;
  
  PrintTime("U", newStat.userTime, totalTime);
  PrintTime("S", processTime, totalTime);
  printf("\n");
  // PrintTime("G ", totalTime, totalTime);
}

#endif



Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;

  inWrap.Init(inStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);

  #ifdef LOG_LZMA_THREADS

  FILETIME startTimeFT;
  NWindows::NTime::GetCurUtcFileTime(startTimeFT);
  UInt64 totalTime = GetTime64(startTimeFT);
  CBaseStat oldStat;
  if (!oldStat.Get(GetCurrentThread(), NULL))
    return E_FAIL;
  
  #endif
  
  
  SRes res = LzmaEnc_Encode(_encoder, &outWrap.vt, &inWrap.vt,
      progress ? &progressWrap.vt : NULL, &g_AlignedAlloc, &g_BigAlloc);

  _inputProcessed = inWrap.Processed;

  RET_IF_WRAP_ERROR(inWrap.Res, res, SZ_ERROR_READ)
  RET_IF_WRAP_ERROR(outWrap.Res, res, SZ_ERROR_WRITE)
  RET_IF_WRAP_ERROR(progressWrap.Res, res, SZ_ERROR_PROGRESS)

  
  #ifdef LOG_LZMA_THREADS
  
  NWindows::NTime::GetCurUtcFileTime(startTimeFT);
  totalTime = GetTime64(startTimeFT) - totalTime;
  HANDLE lz_threads[2];
  LzmaEnc_GetLzThreads(_encoder, lz_threads);
  printf("\n");
  printf("Main: ");  PrintStat(GetCurrentThread(), totalTime, &oldStat);
  printf("Hash: ");  PrintStat(lz_threads[0], totalTime, NULL);
  printf("BinT: ");  PrintStat(lz_threads[1], totalTime, NULL);
  // PrintTime("Total: ", totalTime, totalTime);
  printf("\n");

  #endif

  return SResToHRESULT(res);
}

}}

/* ================ unit: CPP/7zip/Compress/LzmsDecoder.cpp ================ */
// LzmsDecoder.cpp
// The code is based on LZMS description from wimlib code

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NLzms {

class CBitDecoder
{
public:
  const Byte *_buf;
  unsigned _bitPos;

  void Init(const Byte *buf, size_t size) throw()
  {
    _buf = buf + size;
    _bitPos = 0;
  }

  Z7_FORCE_INLINE
  UInt32 GetValue(unsigned numBits) const
  {
    UInt32 v =
        ((UInt32)_buf[-1] << 16) |
        ((UInt32)_buf[-2] << 8) |
         (UInt32)_buf[-3];
    v >>= 24 - numBits - _bitPos;
    return v & ((1u << numBits) - 1);
  }

  Z7_FORCE_INLINE
  UInt32 GetValue_InHigh32bits()
  {
    return GetUi32(_buf - 4) << _bitPos;
  }
  
  void MovePos(unsigned numBits)
  {
    _bitPos += numBits;
    _buf -= (_bitPos >> 3);
    _bitPos &= 7;
  }

  UInt32 ReadBits32(unsigned numBits)
  {
    UInt32 mask = (((UInt32)1 << numBits) - 1);
    numBits += _bitPos;
    const Byte *buf = _buf;
    UInt32 v = GetUi32(buf - 4);
    if (numBits > 32)
    {
      v <<= (numBits - 32);
      v |= (UInt32)buf[-5] >> (40 - numBits);
    }
    else
      v >>= (32 - numBits);
    _buf = buf - (numBits >> 3);
    _bitPos = numBits & 7;
    return v & mask;
  }
};

static UInt32 g_PosBases[k_NumPosSyms /* + 1 */];

static Byte g_PosDirectBits[k_NumPosSyms];

static const Byte k_PosRuns[31] =
{
  8, 0, 9, 7, 10, 15, 15, 20, 20, 30, 33, 40, 42, 45, 60, 73,
  80, 85, 95, 105, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

static UInt32 g_LenBases[k_NumLenSyms];

static const Byte k_LenDirectBits[k_NumLenSyms] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2,
  2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 6,
  7, 8, 9, 10, 16, 30,
};

static struct CInit
{
  CInit()
  {
    {
      unsigned sum = 0;
      for (unsigned i = 0; i < sizeof(k_PosRuns); i++)
      {
        unsigned t = k_PosRuns[i];
        for (unsigned y = 0; y < t; y++)
          g_PosDirectBits[sum + y] = (Byte)i;
        sum += t;
      }
    }
    {
      UInt32 sum = 1;
      for (unsigned i = 0; i < k_NumPosSyms; i++)
      {
        g_PosBases[i] = sum;
        sum += (UInt32)1 << g_PosDirectBits[i];
      }
      // g_PosBases[k_NumPosSyms] = sum;
    }
    {
      UInt32 sum = 1;
      for (unsigned i = 0; i < k_NumLenSyms; i++)
      {
        g_LenBases[i] = sum;
        sum += (UInt32)1 << k_LenDirectBits[i];
      }
    }
  }
} g_Init;

static unsigned GetNumPosSlots(size_t size)
{
  if (size < 2)
    return 0;
  
  size--;

  if (size >= g_PosBases[k_NumPosSyms - 1])
    return k_NumPosSyms;
  unsigned left = 0;
  unsigned right = k_NumPosSyms;
  for (;;)
  {
    const unsigned m = (left + right) / 2;
    if (left == m)
      return m + 1;
    if (size >= g_PosBases[m])
      left = m;
    else
      right = m;
  }
}


static const Int32 k_x86_WindowSize = 65535;
static const Int32 k_x86_TransOffset = 1023;

static const size_t k_x86_HistorySize = 1 << 16;

static void x86_Filter(Byte *data, UInt32 size, Int32 *history)
{
  if (size <= 17)
    return;

  Byte isCode[256];
  memset(isCode, 0, 256);
  isCode[0x48] = 1;
  isCode[0x4C] = 1;
  isCode[0xE8] = 1;
  isCode[0xE9] = 1;
  isCode[0xF0] = 1;
  isCode[0xFF] = 1;

  {
    for (size_t i = 0; i < k_x86_HistorySize; i++)
      history[i] = -(Int32)k_x86_WindowSize - 1;
  }

  size -= 16;
  const unsigned kSave = 6;
  const Byte savedByte = data[(size_t)size + kSave];
  data[(size_t)size + kSave] = 0xE8;
  Int32 last_x86_pos = -k_x86_TransOffset - 1;

  // first byte is ignored
  Int32 i = 0;
  
  for (;;)
  {
    Byte *p = data + (UInt32)i;

    for (;;)
    {
      if (isCode[*(++p)]) break;
      if (isCode[*(++p)]) break;
    }
    
    i = (Int32)(p - data);
    if ((UInt32)i >= size)
      break;

    UInt32 codeLen;

    Int32 maxTransOffset = k_x86_TransOffset;
    
    const Byte b = p[0];
    
    if ((b & 0x80) == 0) // REX (0x48 or 0x4c)
    {
      const unsigned b2 = p[2] - 0x5; // [RIP + disp32]
      if (b2 & 0x7)
        continue;
      if (p[1] != 0x8d) // LEA
      {
        if (p[1] != 0x8b || b != 0x48 || (b2 & 0xf7))
          continue;
        // MOV RAX / RCX, [RIP + disp32]
      }
      codeLen = 3;
    }
    else if (b == 0xE8)
    {
      // CALL
      codeLen = 1;
      maxTransOffset /= 2;
    }
    else if (b == 0xE9)
    {
      // JUMP
      i += 4;
      continue;
    }
    else if (b == 0xF0)
    {
      if (p[1] != 0x83 || p[2] != 0x05)
        continue;
      // LOCK ADD [RIP + disp32], imm8
      // LOCK ADD [disp32], imm8
      codeLen = 3;
    }
    else
    // if (b == 0xFF)
    {
      if (p[1] != 0x15)
        continue;
      // CALL [RIP + disp32];
      // CALL [disp32];
      codeLen = 2;
    }

    Int32 *target;
    {
      Byte *p2 = p + codeLen;
      UInt32 n = GetUi32(p2);
      if (i - last_x86_pos <= maxTransOffset)
      {
        n = (UInt32)((Int32)n - i);
        SetUi32(p2, n)
      }
      target = history + (((UInt32)i + n) & 0xFFFF);
    }

    i += (Int32)(codeLen + sizeof(UInt32) - 1);

    if (i - *target <= k_x86_WindowSize)
      last_x86_pos = i;
    *target = i;
  }

  data[(size_t)size + kSave] = savedByte;
}



// static const int kLenIdNeedInit = -2;

CDecoder::CDecoder():
  _x86_history(NULL)
{
}

CDecoder::~CDecoder()
{
  ::MidFree(_x86_history);
}

// #define RIF(x) { if (!(x)) return false; }

#define LIMIT_CHECK if (_bs._buf < _rc.cur) return S_FALSE;
// #define LIMIT_CHECK

#define READ_BITS_CHECK(numDirectBits) \
  if (_bs._buf < _rc.cur) return S_FALSE; \
  if ((size_t)(_bs._buf - _rc.cur) < (numDirectBits >> 3)) return S_FALSE;


#define HUFF_DEC(sym, pp) \
    sym = pp.DecodeFull(&_bs); \
    pp.Freqs[sym]++; \
    if (--pp.RebuildRem == 0) pp.Rebuild();


HRESULT CDecoder::CodeReal(const Byte *in, size_t inSize, Byte *_win, size_t outSize)
{
  // size_t inSizeT = (size_t)(inSize);
  // Byte *_win;
  // size_t _pos;
  _pos = 0;

  CBitDecoder _bs;
  CRangeDecoder _rc;
 
  if (inSize < 8 || (inSize & 1) != 0)
    return S_FALSE;
  _rc.Init(in, inSize);
  if (_rc.code >= _rc.range)
    return S_FALSE;
  _bs.Init(in, inSize);

  {
    {
      {
        for (unsigned i = 0 ; i < k_NumReps + 1; i++)
          _reps[i] = i + 1;
      }

      {
        for (unsigned i = 0 ; i < k_NumReps + 1; i++)
          _deltaReps[i] = i + 1;
      }

      mainState = 0;
      matchState = 0;

      { for (size_t i = 0; i < k_NumMainProbs; i++) mainProbs[i].Init(); }
      { for (size_t i = 0; i < k_NumMatchProbs; i++) matchProbs[i].Init(); }

      {
        for (size_t k = 0; k < k_NumReps; k++)
        {
          lzRepStates[k] = 0;
          for (size_t i = 0; i < k_NumRepProbs; i++)
            lzRepProbs[k][i].Init();
        }
      }
      {
        for (size_t k = 0; k < k_NumReps; k++)
        {
          deltaRepStates[k] = 0;
          for (size_t i = 0; i < k_NumRepProbs; i++)
            deltaRepProbs[k][i].Init();
        }
      }

      m_LitDecoder.Init();
      m_LenDecoder.Init();
      m_PowerDecoder.Init();
      unsigned numPosSyms = GetNumPosSlots(outSize);
      if (numPosSyms < 2)
        numPosSyms = 2;
      m_PosDecoder.Init(numPosSyms);
      m_DeltaDecoder.Init(numPosSyms);
    }
  }

  {
    unsigned prevType = 0;
    
    while (_pos < outSize)
    {
      if (_rc.Decode(&mainState, k_NumMainProbs, mainProbs) == 0)
      {
        unsigned number;
        HUFF_DEC(number, m_LitDecoder)
        LIMIT_CHECK
        _win[_pos++] = (Byte)number;
        prevType = 0;
      }
      else if (_rc.Decode(&matchState, k_NumMatchProbs, matchProbs) == 0)
      {
        UInt32 distance;
        
        if (_rc.Decode(&lzRepStates[0], k_NumRepProbs, lzRepProbs[0]) == 0)
        {
          unsigned number;
          HUFF_DEC(number, m_PosDecoder)
          LIMIT_CHECK

          const unsigned numDirectBits = g_PosDirectBits[number];
          distance = g_PosBases[number];
          READ_BITS_CHECK(numDirectBits)
          distance += _bs.ReadBits32(numDirectBits);
          // LIMIT_CHECK
          _reps[3] = _reps[2];
          _reps[2] = _reps[1];
          _reps[1] = _reps[0];
          _reps[0] = distance;
        }
        else
        {
          if (_rc.Decode(&lzRepStates[1], k_NumRepProbs, lzRepProbs[1]) == 0)
          {
            if (prevType != 1)
              distance = _reps[0];
            else
            {
              distance = _reps[1];
              _reps[1] = _reps[0];
              _reps[0] = distance;
            }
          }
          else if (_rc.Decode(&lzRepStates[2], k_NumRepProbs, lzRepProbs[2]) == 0)
          {
            if (prevType != 1)
            {
              distance = _reps[1];
              _reps[1] = _reps[0];
              _reps[0] = distance;
            }
            else
            {
              distance = _reps[2];
              _reps[2] = _reps[1];
              _reps[1] = _reps[0];
              _reps[0] = distance;
            }
          }
          else
          {
            if (prevType != 1)
            {
              distance = _reps[2];
              _reps[2] = _reps[1];
              _reps[1] = _reps[0];
              _reps[0] = distance;
            }
            else
            {
              distance = _reps[3];
              _reps[3] = _reps[2];
              _reps[2] = _reps[1];
              _reps[1] = _reps[0];
              _reps[0] = distance;
            }
          }
        }

        unsigned lenSlot;
        HUFF_DEC(lenSlot, m_LenDecoder)
        LIMIT_CHECK

        UInt32 len = g_LenBases[lenSlot];
        {
          const unsigned numDirectBits = k_LenDirectBits[lenSlot];
          READ_BITS_CHECK(numDirectBits)
          len += _bs.ReadBits32(numDirectBits);
        }
        // LIMIT_CHECK

        if (len > outSize - _pos)
          return S_FALSE;

        if (distance > _pos)
          return S_FALSE;

        Byte *dest = _win + _pos;
        const Byte *src = dest - distance;
        _pos += len;
        do
          *dest++ = *src++;
        while (--len);

        prevType = 1;
      }
      else
      {
        UInt64 distance;

        unsigned power;
        UInt32 distance32;
        
        if (_rc.Decode(&deltaRepStates[0], k_NumRepProbs, deltaRepProbs[0]) == 0)
        {
          HUFF_DEC(power, m_PowerDecoder)
          LIMIT_CHECK

          unsigned number;
          HUFF_DEC(number, m_DeltaDecoder)
          LIMIT_CHECK

          const unsigned numDirectBits = g_PosDirectBits[number];
          distance32 = g_PosBases[number];
          READ_BITS_CHECK(numDirectBits)
          distance32 += _bs.ReadBits32(numDirectBits);
          // LIMIT_CHECK

          distance = ((UInt64)power << 32) | distance32;

          _deltaReps[3] = _deltaReps[2];
          _deltaReps[2] = _deltaReps[1];
          _deltaReps[1] = _deltaReps[0];
          _deltaReps[0] = distance;
        }
        else
        {
          if (_rc.Decode(&deltaRepStates[1], k_NumRepProbs, deltaRepProbs[1]) == 0)
          {
            if (prevType != 2)
              distance = _deltaReps[0];
            else
            {
              distance = _deltaReps[1];
              _deltaReps[1] = _deltaReps[0];
              _deltaReps[0] = distance;
            }
          }
          else if (_rc.Decode(&deltaRepStates[2], k_NumRepProbs, deltaRepProbs[2]) == 0)
          {
            if (prevType != 2)
            {
              distance = _deltaReps[1];
              _deltaReps[1] = _deltaReps[0];
              _deltaReps[0] = distance;
            }
            else
            {
              distance = _deltaReps[2];
              _deltaReps[2] = _deltaReps[1];
              _deltaReps[1] = _deltaReps[0];
              _deltaReps[0] = distance;
            }
          }
          else
          {
            if (prevType != 2)
            {
              distance = _deltaReps[2];
              _deltaReps[2] = _deltaReps[1];
              _deltaReps[1] = _deltaReps[0];
              _deltaReps[0] = distance;
            }
            else
            {
              distance = _deltaReps[3];
              _deltaReps[3] = _deltaReps[2];
              _deltaReps[2] = _deltaReps[1];
              _deltaReps[1] = _deltaReps[0];
              _deltaReps[0] = distance;
            }
          }
          distance32 = (UInt32)_deltaReps[0] & 0xFFFFFFFF;
          power = (UInt32)(_deltaReps[0] >> 32);
        }

        const UInt32 dist = (distance32 << power);
        
        unsigned lenSlot;
        HUFF_DEC(lenSlot, m_LenDecoder)
        LIMIT_CHECK

        UInt32 len = g_LenBases[lenSlot];
        {
          const unsigned numDirectBits = k_LenDirectBits[lenSlot];
          READ_BITS_CHECK(numDirectBits)
          len += _bs.ReadBits32(numDirectBits);
        }
        // LIMIT_CHECK

        if (len > outSize - _pos)
          return S_FALSE;

        size_t span = (size_t)1 << power;
        if ((UInt64)dist + span > _pos)
          return S_FALSE;
        Byte *dest = _win + _pos - span;
        const Byte *src = dest - dist;
        _pos += len;
        do
        {
          *(dest + span) = (Byte)(*(dest) + *(src + span) - *(src));
          src++;
          dest++;
        }
        while (--len);

        prevType = 2;
      }
    }
  }

  _rc.Normalize();
  if (_rc.code != 0)
    return S_FALSE;
  if (_rc.cur > _bs._buf
      || (_rc.cur == _bs._buf && _bs._bitPos != 0))
    return S_FALSE;

  /*
  int delta = (int)(_bs._buf - _rc.cur);
  if (_bs._bitPos != 0)
    delta--;
  if ((delta & 1))
    delta--;
  printf("%d ", delta);
  */

  return S_OK;
}

HRESULT CDecoder::Code(const Byte *in, size_t inSize, Byte *out, size_t outSize)
{
  if (!_x86_history)
  {
    _x86_history = (Int32 *)::MidAlloc(sizeof(Int32) * k_x86_HistorySize);
    if (!_x86_history)
      return E_OUTOFMEMORY;
  }
  HRESULT res;
  // try
  {
    res = CodeReal(in, inSize, out, outSize);
  }
  // catch (...) { res = S_FALSE; }
  x86_Filter(out, (UInt32)_pos, _x86_history);
  return res;
}

}}

/* ================ unit: CPP/7zip/Compress/LzxDecoder.cpp ================ */
// LzxDecoder.cpp

// amalgamation: header emitted in prologue

#include <string.h>
// #include <stdio.h>

// #define SHOW_DEBUG_INFO

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


#ifdef MY_CPU_X86_OR_AMD64
#if defined(MY_CPU_AMD64)  \
    || defined(__SSE2__) \
    || defined(_M_IX86_FP) && (_M_IX86_FP >= 2) \
    || 0 && defined(_MSC_VER) && (_MSC_VER >= 1400) // set (1 &&) for debug
  
#if defined(__clang__) && (__clang_major__ >= 2) \
    || defined(__GNUC__) && (__GNUC__ >= 4) \
    || defined(_MSC_VER) && (_MSC_VER >= 1400)
#define Z7_LZX_X86_FILTER_USE_SSE2
#endif
#endif
#endif


#ifdef Z7_LZX_X86_FILTER_USE_SSE2
// #ifdef MY_CPU_X86_OR_AMD64
#include <emmintrin.h> // SSE2
// #endif
  #if defined(__clang__) || defined(__GNUC__)
    typedef int ctz_type;
    #define MY_CTZ(dest, mask) dest = __builtin_ctz((UInt32)(mask))
  #else  // #if defined(_MSC_VER)
    #if (_MSC_VER >= 1600)
      // #include <intrin.h>
    #endif
    typedef unsigned long ctz_type;
    #define MY_CTZ(dest, mask)  _BitScanForward(&dest, (mask));
  #endif // _MSC_VER
#endif

// when window buffer is filled, we must wrap position to zero,
// and we want to wrap at same points where original-lzx must wrap.
// But the wrapping is possible in point where chunk is finished.
// Usually (chunk_size == 32KB), but (chunk_size != 32KB) also is allowed.
// So we don't use additional buffer space over required (winSize).
// And we can't use large overwrite after (len) in CopyLzMatch().
// But we are allowed to write 3 bytes after (len), because
// (delta <= _winSize - 3).

// #define k_Lz_OverwriteSize  0  // for debug : to disable overwrite
#define k_Lz_OverwriteSize  3 // = kNumReps
#if k_Lz_OverwriteSize > 0
// (k_Lz_OutBufSize_Add >= k_Lz_OverwriteSize) is required
// we use value 4 to simplify memset() code.
#define k_Lz_OutBufSize_Add  (k_Lz_OverwriteSize + 1) // == 4
#else
#define k_Lz_OutBufSize_Add  0
#endif

// (len != 0)
// (0 < delta <= _winSize - 3)
Z7_FORCE_INLINE
void CopyLzMatch(Byte *dest, const Byte *src, UInt32 len, UInt32 delta)
{
  if (delta >= 4)
  {
#if k_Lz_OverwriteSize >= 3
    // optimized code with overwrite to reduce the number of branches
  #ifdef MY_CPU_LE_UNALIGN
    *(UInt32 *)(void *)(dest) = *(const UInt32 *)(const void *)(src);
  #else
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
  #endif
    len--;
    src++;
    dest++;
    {
#else
    // no overwrite in out buffer
    dest[0] = src[0];
    {
      const unsigned m = (unsigned)len & 1;
      src += m;
      dest += m;
    }
    if (len &= ~(unsigned)1)
    {
      dest[0] = src[0];
      dest[1] = src[1];
#endif
      // len == 0 is allowed here
      {
        const unsigned m = (unsigned)len & 3;
        src += m;
        dest += m;
      }
      if (len &= ~(unsigned)3)
      {
#ifdef MY_CPU_LE_UNALIGN
      #if 1
        *(UInt32 *)(void *)(dest) = *(const UInt32 *)(const void *)(src);
        {
          const unsigned m = (unsigned)len & 7;
          dest += m;
          src += m;
        }
        if (len &= ~(unsigned)7)
          do
          {
            *(UInt32 *)(void *)(dest    ) = *(const UInt32 *)(const void *)(src);
            *(UInt32 *)(void *)(dest + 4) = *(const UInt32 *)(const void *)(src + 4);
            src += 8;
            dest += 8;
          }
          while (len -= 8);
      #else
        // gcc-11 -O3 for x64 generates incorrect code here
        do
        {
          *(UInt32 *)(void *)(dest) = *(const UInt32 *)(const void *)(src);
          src += 4;
          dest += 4;
        }
        while (len -= 4);
      #endif
#else
        do
        {
          const Byte b0 = src[0];
          const Byte b1 = src[1];
          dest[0] = b0;
          dest[1] = b1;
          const Byte b2 = src[2];
          const Byte b3 = src[3];
          dest[2] = b2;
          dest[3] = b3;
          src += 4;
          dest += 4;
        }
        while (len -= 4);
#endif
      }
    }
  }
  else // (delta < 4)
  {
    const unsigned b0 = *src;
    *dest = (Byte)b0;
    if (len >= 2)
    {
      if (delta < 2)
      {
        dest += (unsigned)len & 1;
        dest[0] = (Byte)b0;
        dest[1] = (Byte)b0;
        dest += (unsigned)len & 2;
        if (len &= ~(unsigned)3)
        {
#ifdef MY_CPU_LE_UNALIGN
          #ifdef MY_CPU_64BIT
          const UInt64 a = (UInt64)b0 * 0x101010101010101;
          *(UInt32 *)(void *)dest = (UInt32)a;
          dest += (unsigned)len & 7;
          if (len &= ~(unsigned)7)
          {
            // *(UInt64 *)(void *)dest = a;
            // dest += 8;
            // len -= 8;
            // if (len)
            {
              // const ptrdiff_t delta = (ptrdiff_t)dest & 7;
              // dest -= delta;
              do
              {
                *(UInt64 *)(void *)dest = a;
                dest += 8;
              }
              while (len -= 8);
              // dest += delta - 8;
              // *(UInt64 *)(void *)dest = a;
            }
          }
          #else
          const UInt32 a = (UInt32)b0 * 0x1010101;
          do
          {
            *(UInt32 *)(void *)dest = a;
            dest += 4;
          }
          while (len -= 4);
          #endif
#else
          do
          {
            dest[0] = (Byte)b0;
            dest[1] = (Byte)b0;
            dest[2] = (Byte)b0;
            dest[3] = (Byte)b0;
            dest += 4;
          }
          while (len -= 4);
#endif
        }
      }
      else if (delta == 2)
      {
        const unsigned m = (unsigned)len & 1;
        len &= ~(unsigned)1;
        src += m;
        dest += m;
        {
          const Byte a0 = src[0];
          const Byte a1 = src[1];
          do
          {
            dest[0] = a0;
            dest[1] = a1;
            dest += 2;
          }
          while (len -= 2);
        }
      }
      else /* if (delta == 3) */
      {
        const unsigned b1 = src[1];
        dest[1] = (Byte)b1;
        if (len -= 2)
        {
          const unsigned b2 = src[2];
          dest += 2;
          do
          {
            dest[0] = (Byte)b2;  if (--len == 0) break;
            dest[1] = (Byte)b0;  if (--len == 0) break;
            dest[2] = (Byte)b1;
            dest += 3;
          }
          while (--len);
        }
      }
    }
  }
}

// #define Z7_LZX_SHOW_STAT
#ifdef Z7_LZX_SHOW_STAT
#include <stdio.h>
#endif

namespace NCompress {
namespace NLzx {

// #define Z7_LZX_SHOW_STAT
#ifdef Z7_LZX_SHOW_STAT
static UInt32 g_stats_Num_x86[3];
static UInt32 g_stats_NumTables;
static UInt32 g_stats_NumLits;
static UInt32 g_stats_NumAlign;
static UInt32 g_stats_main[kMainTableSize];
static UInt32 g_stats_len[kNumLenSymbols];
static UInt32 g_stats_main_levels[kNumHuffmanBits + 1];
static UInt32 g_stats_len_levels[kNumHuffmanBits + 1];
#define UPDATE_STAT(a) a
static void PrintVal(UInt32 v)
{
  printf("\n    : %9u", v);
}
static void PrintStat(const char *name, const UInt32 *a, size_t num)
{
  printf("\n\n==== %s:", name);
  UInt32 sum = 0;
  size_t i;
  for (i = 0; i < num; i++)
    sum += a[i];
  PrintVal(sum);
  if (sum != 0)
  {
    for (i = 0; i < num; i++)
    {
      if (i % 8 == 0)
        printf("\n");
      printf("\n%3x : %9u : %5.2f", (unsigned)i, (unsigned)a[i], (double)a[i] * 100 / sum);
    }
  }
  printf("\n");
}

static struct CStat
{
  ~CStat()
  {
    PrintStat("x86_filter", g_stats_Num_x86, Z7_ARRAY_SIZE(g_stats_Num_x86));
    printf("\nTables:"); PrintVal(g_stats_NumTables);
    printf("\nLits:");   PrintVal(g_stats_NumLits);
    printf("\nAlign:");  PrintVal(g_stats_NumAlign);
    PrintStat("Main", g_stats_main, Z7_ARRAY_SIZE(g_stats_main));
    PrintStat("Len", g_stats_len, Z7_ARRAY_SIZE(g_stats_len));
    PrintStat("Main Levels", g_stats_main_levels, Z7_ARRAY_SIZE(g_stats_main_levels));
    PrintStat("Len Levels", g_stats_len_levels, Z7_ARRAY_SIZE(g_stats_len_levels));
  }
} g_stat;
#else
#define UPDATE_STAT(a)
#endif



/*
3 p015  : ivb-   : or r32,r32 / add r32,r32
4 p0156 : hsw+
5 p0156b: adl+
2 p0_5  : ivb-   : shl r32,i8
2 p0__6 : hsw+
1 p5    : ivb-   : jb
2 p0__6 : hsw+
2 p0_5  : wsm-    : SSE2  : pcmpeqb  : _mm_cmpeq_epi8
2 p_15  : snb-bdw
2 p01   : skl+
1 p0              : SSE2  : pmovmskb : _mm_movemask_epi8
*/
/*
  v24.00: the code was fixed for more compatibility with original-ms-cab-decoder.
  for ((Int32)translationSize >= 0) : LZX specification shows the code with signed Int32.
  for ((Int32)translationSize <  0) : no specification for that case, but we support that case.
  We suppose our code now is compatible with original-ms-cab-decoder.

  Starting byte of data stream (real_pos == 0) is special corner case,
  where we don't need any conversion (as in original-ms-cab-decoder).
  Our optimization: we use unsigned (UInt32 pos) (pos = -1 - real_pos).
  So (pos) is always negative: ((Int32)pos < 0).
  It allows us to use simple comparison (v > pos) instead of more complex comparisons.
*/
// (p) will point 5 bytes after 0xe8 byte:
// pos == -1 - (p - 5 - data_start) == 4 + data_start - p
// (FILTER_PROCESSED_SIZE_DELTA == 4) is optimized value for better speed in some compilers:
#define FILTER_PROCESSED_SIZE_DELTA  4

#if defined(MY_CPU_X86_OR_AMD64) || defined(MY_CPU_ARM_OR_ARM64)
  // optimized branch:
  // size_t must be at least 32-bit for this branch.
  #if 1 // use 1 for simpler code
    // use integer (low 32 bits of pointer) instead of pointer
    #define X86_FILTER_PREPARE  processedSize4 = (UInt32)(size_t)(ptrdiff_t)data + \
        (UInt32)(4 - FILTER_PROCESSED_SIZE_DELTA) - processedSize4;
    #define X86_FILTER_CALC_pos(p)  const UInt32 pos = processedSize4 - (UInt32)(size_t)(ptrdiff_t)p;
  #else
    // note: (dataStart) pointer can point out of array ranges:
    #define X86_FILTER_PREPARE  const Byte *dataStart = data + \
                (4 - FILTER_PROCESSED_SIZE_DELTA) - processedSize4;
    #define X86_FILTER_CALC_pos(p)  const UInt32 pos = (UInt32)(size_t)(dataStart - p);
  #endif
#else
  // non-optimized branch for unusual platforms (16-bit size_t or unusual size_t):
    #define X86_FILTER_PREPARE  processedSize4 = \
        (UInt32)(4 - FILTER_PROCESSED_SIZE_DELTA) - processedSize4;
    #define X86_FILTER_CALC_pos(p)  const UInt32 pos = processedSize4 - (UInt32)(size_t)(p - data);
#endif

#define X86_TRANSLATE_PRE(p) \
    UInt32 v = GetUi32((p) - 4);

#define X86_TRANSLATE_POST(p) \
  { \
    X86_FILTER_CALC_pos(p) \
    if (v < translationSize) { \
      UPDATE_STAT(g_stats_Num_x86[0]++;) \
      v += pos + 1; \
      SetUi32((p) - 4, v) \
    } \
    else if (v > pos) { \
      UPDATE_STAT(g_stats_Num_x86[1]++;) \
      v += translationSize; \
      SetUi32((p) - 4, v) \
    } else { UPDATE_STAT(g_stats_Num_x86[2]++;) } \
  }


/*
  if (   defined(Z7_LZX_X86_FILTER_USE_SSE2)
      && defined(Z7_LZX_X86_FILTER_USE_SSE2_ALIGNED))
    the function can read up to aligned_for_32_up_from(size) bytes in (data).
*/
// processedSize < (1 << 30)
Z7_NO_INLINE
static void x86_Filter4(Byte *data, size_t size, UInt32 processedSize4, UInt32 translationSize)
{
  const size_t kResidue = 10;
  if (size <= kResidue)
    return;
  Byte * const lim = data + size - kResidue + 4;
  const Byte save = lim[0];
  lim[0] = 0xe8;
  X86_FILTER_PREPARE
  Byte *p = data;

#define FILTER_RETURN_IF_LIM(_p_)  if (_p_ > lim) { lim[0] = save; return; }

#ifdef Z7_LZX_X86_FILTER_USE_SSE2

// sse2-aligned/sse2-unaligned provide same speed on real data.
// but the code is smaller for sse2-unaligned version.
// for debug : define it to get alternative version with aligned 128-bit reads:
// #define Z7_LZX_X86_FILTER_USE_SSE2_ALIGNED

#define FILTER_MASK_INT  UInt32
#define FILTER_NUM_VECTORS_IN_CHUNK   2
#define FILTER_CHUNK_BYTES_OFFSET     (16 * FILTER_NUM_VECTORS_IN_CHUNK - 5)

#ifdef Z7_LZX_X86_FILTER_USE_SSE2_ALIGNED
  // aligned version doesn't uses additional space if buf size is aligned for 32
  #define k_Filter_OutBufSize_Add  0
  #define k_Filter_OutBufSize_AlignMask  (16 * FILTER_NUM_VECTORS_IN_CHUNK - 1)
  #define FILTER_LOAD_128(p)  _mm_load_si128 ((const __m128i *)(const void *)(p))
#else
  #define k_Filter_OutBufSize_Add  (16 * FILTER_NUM_VECTORS_IN_CHUNK)
  #define k_Filter_OutBufSize_AlignMask 0
  #define FILTER_LOAD_128(p)  _mm_loadu_si128((const __m128i *)(const void *)(p))
#endif

#define GET_E8_MASK(dest, dest1, p) \
{ \
  __m128i v0 = FILTER_LOAD_128(p); \
  __m128i v1 = FILTER_LOAD_128(p + 16); \
  p += 16 * FILTER_NUM_VECTORS_IN_CHUNK; \
  v0 = _mm_cmpeq_epi8(v0, k_e8_Vector); \
  v1 = _mm_cmpeq_epi8(v1, k_e8_Vector); \
  dest  = (unsigned)_mm_movemask_epi8(v0); \
  dest1 = (unsigned)_mm_movemask_epi8(v1); \
}

  const __m128i k_e8_Vector = _mm_set1_epi32((Int32)(UInt32)0xe8e8e8e8);
  for (;;)
  {
      // for debug: define it for smaller code:
      // #define Z7_LZX_X86_FILTER_CALC_IN_LOOP
      // without Z7_LZX_X86_FILTER_CALC_IN_LOOP, we can get faster and simpler loop
    FILTER_MASK_INT mask;
    {
      FILTER_MASK_INT mask1;
      do
      {
        GET_E8_MASK(mask, mask1, p)
        #ifndef Z7_LZX_X86_FILTER_CALC_IN_LOOP
          mask += mask1;
        #else
          mask |= mask1 << 16;
        #endif
      }
      while (!mask);
     
      #ifndef Z7_LZX_X86_FILTER_CALC_IN_LOOP
        mask -= mask1;
        mask |= mask1 << 16;
      #endif
    }
      
#ifdef Z7_LZX_X86_FILTER_USE_SSE2_ALIGNED
    for (;;)
    {
      ctz_type index;
      typedef
      #ifdef MY_CPU_64BIT
        UInt64
      #else
        UInt32
      #endif
        SUPER_MASK_INT;
      SUPER_MASK_INT superMask;
      {
        MY_CTZ(index, mask);
        Byte *p2 = p - FILTER_CHUNK_BYTES_OFFSET + (unsigned)index;
        X86_TRANSLATE_PRE(p2)
        superMask = ~(SUPER_MASK_INT)0x1f << index;
        FILTER_RETURN_IF_LIM(p2)
        X86_TRANSLATE_POST(p2)
        mask &= (UInt32)superMask;
      }
      if (mask)
        continue;
      if (index <= FILTER_CHUNK_BYTES_OFFSET)
        break;
      {
        FILTER_MASK_INT mask1;
        GET_E8_MASK(mask, mask1, p)
        mask &=
            #ifdef MY_CPU_64BIT
              (UInt32)(superMask >> 32);
            #else
              ((FILTER_MASK_INT)0 - 1) << ((int)index - FILTER_CHUNK_BYTES_OFFSET);
            #endif
        mask |= mask1 << 16;
      }
      if (!mask)
        break;
    }
#else // ! Z7_LZX_X86_FILTER_USE_SSE2_ALIGNED
    {
      // we use simplest version without loop:
      // for (;;)
      {
        ctz_type index;
        MY_CTZ(index, mask);
        /*
        printf("\np=%p, mask=%8x, index = %2d, p + index = %x\n",
            (p - 16 * FILTER_NUM_VECTORS_IN_CHUNK), (unsigned)mask,
            (unsigned)index, (unsigned)((unsigned)(ptrdiff_t)(p - 16 * FILTER_NUM_VECTORS_IN_CHUNK) + index));
        */
        p += (size_t)(unsigned)index - FILTER_CHUNK_BYTES_OFFSET;
        FILTER_RETURN_IF_LIM(p)
        // mask &= ~(FILTER_MASK_INT)0x1f << index;  mask >>= index;
        X86_TRANSLATE_PRE(p)
        X86_TRANSLATE_POST(p)
        // if (!mask) break; // p += 16 * FILTER_NUM_VECTORS_IN_CHUNK;
      }
    }
#endif // ! Z7_LZX_X86_FILTER_USE_SSE2_ALIGNED
  }

#else // ! Z7_LZX_X86_FILTER_USE_SSE2

#define k_Filter_OutBufSize_Add  0
#define k_Filter_OutBufSize_AlignMask 0


  for (;;)
  {
    for (;;)
    {
      if (p[0] == 0xe8) { p += 5; break; }
      if (p[1] == 0xe8) { p += 6; break; }
      if (p[2] == 0xe8) { p += 7; break; }
      p += 4;
      if (p[-1] == 0xe8) { p += 4; break; }
    }
    FILTER_RETURN_IF_LIM(p)
    X86_TRANSLATE_PRE(p)
    X86_TRANSLATE_POST(p)
  }

#endif // ! Z7_LZX_X86_FILTER_USE_SSE2
}


CDecoder::CDecoder() throw():
    _win(NULL),
    _isUncompressedBlock(false),
    _skipByte(false),
    _keepHistory(false),
    _keepHistoryForNext(true),
    _needAlloc(true),
    _wimMode(false),
    _numDictBits(15),
    _unpackBlockSize(0),
    _x86_translationSize(0),
    _x86_buf(NULL),
    _unpackedData(NULL)
{
  {
    // it's better to get empty virtual entries, if mispredicted value can be used:
    memset(_reps, 0, kPosSlotOffset * sizeof(_reps[0]));
    memset(_extra, 0, kPosSlotOffset);
#define SET_NUM_BITS(i) i // #define NUM_BITS_DELTA 31
    _extra[kPosSlotOffset + 0] = SET_NUM_BITS(0);
    _extra[kPosSlotOffset + 1] = SET_NUM_BITS(0);
    // reps[0] = 0 - (kNumReps - 1);
    // reps[1] = 1 - (kNumReps - 1);
    UInt32 a = 2 - (kNumReps - 1);
    UInt32 delta = 1;
    unsigned i;
    for (i = 0; i < kNumLinearPosSlotBits; i++)
    {
      _extra[(size_t)i * 2 + 2 + kPosSlotOffset] = (Byte)(SET_NUM_BITS(i));
      _extra[(size_t)i * 2 + 3 + kPosSlotOffset] = (Byte)(SET_NUM_BITS(i));
      _reps [(size_t)i * 2 + 2 + kPosSlotOffset] = a;  a += delta;
      _reps [(size_t)i * 2 + 3 + kPosSlotOffset] = a;  a += delta;
      delta += delta;
    }
    for (i = kNumLinearPosSlotBits * 2 + 2; i < kNumPosSlots; i++)
    {
      _extra[(size_t)i + kPosSlotOffset] = SET_NUM_BITS(kNumLinearPosSlotBits);
      _reps [(size_t)i + kPosSlotOffset] = a;
      a += (UInt32)1 << kNumLinearPosSlotBits;
    }
  }
}

CDecoder::~CDecoder() throw()
{
  if (_needAlloc)
    // BigFree
    z7_AlignedFree
      (_win);
  z7_AlignedFree(_x86_buf);
}

HRESULT CDecoder::Flush() throw()
{
  // UInt32 t = _x86_processedSize; for (int y = 0; y < 50; y++) { _x86_processedSize = t; // benchmark: (branch predicted)
  if (_x86_translationSize != 0)
  {
    Byte *destData = _win + _writePos;
    const UInt32 curSize = _pos - _writePos;
    if (_keepHistoryForNext)
    {
      const size_t kChunkSize = (size_t)1 << 15;
      if (curSize > kChunkSize)
        return E_NOTIMPL;
      if (!_x86_buf)
      {
        // (kChunkSize % 32 == 0) is required in some cases, because
        // the filter can read data by 32-bytes chunks in some cases.
        // if (chunk_size > (1 << 15)) is possible, then we must the code:
        const size_t kAllocSize = kChunkSize + k_Filter_OutBufSize_Add;
        _x86_buf = (Byte *)z7_AlignedAlloc(kAllocSize);
        if (!_x86_buf)
          return E_OUTOFMEMORY;
        #if 0 != k_Filter_OutBufSize_Add || \
            0 != k_Filter_OutBufSize_AlignMask
          // x86_Filter4() can read after curSize.
          // So we set all data to zero to prevent reading of uninitialized data:
          memset(_x86_buf, 0, kAllocSize); // optional
        #endif
      }
      // for (int yy = 0; yy < 1; yy++) // for debug
      memcpy(_x86_buf, destData, curSize);
      _unpackedData = _x86_buf;
      destData = _x86_buf;
    }
    else
    {
      // x86_Filter4() can overread after (curSize),
      // so we can do memset() after (curSize):
      // k_Filter_OutBufSize_AlignMask also can be used
      // if (!_overDict) memset(destData + curSize, 0, k_Filter_OutBufSize_Add);
    }
    x86_Filter4(destData, curSize, _x86_processedSize - FILTER_PROCESSED_SIZE_DELTA, _x86_translationSize);
    _x86_processedSize += (UInt32)curSize;
    if (_x86_processedSize >= ((UInt32)1 << 30))
      _x86_translationSize = 0;
  }
  // }
  return S_OK;
}



// (NUM_DELTA_BYTES == 2) reduces the code in main loop.
#if 1
  #define NUM_DELTA_BYTES  2
#else
  #define NUM_DELTA_BYTES  0
#endif

#define NUM_DELTA_BIT_OFFSET_BITS  (NUM_DELTA_BYTES * 8)

#if NUM_DELTA_BIT_OFFSET_BITS > 0
  #define DECODE_ERROR_CODE  0
  #define IS_OVERFLOW_bitOffset(bo)  ((bo) >= 0)
  // ( >= 0) comparison after bitOffset change gives simpler commands than ( > 0) comparison
#else
  #define DECODE_ERROR_CODE  1
  #define IS_OVERFLOW_bitOffset(bo)  ((bo) >  0)
#endif

// (numBits != 0)
#define GET_VAL_BASE(numBits)  (_value >> (32 - (numBits)))

#define Z7_LZX_HUFF_DECODE( sym, huff, kNumTableBits, move_pos_op, check_op, error_op) \
    Z7_HUFF_DECODE_VAL_IN_HIGH32(sym, huff, kNumHuffmanBits, kNumTableBits,  \
        _value, check_op, error_op, move_pos_op, NORMALIZE, bs)

#define Z7_LZX_HUFF_DECODE_CHECK_YES(sym, huff, kNumTableBits, move_pos_op) \
        Z7_LZX_HUFF_DECODE(          sym, huff, kNumTableBits, move_pos_op, \
            Z7_HUFF_DECODE_ERROR_SYM_CHECK_YES, { return DECODE_ERROR_CODE; })

#define Z7_LZX_HUFF_DECODE_CHECK_NO( sym, huff, kNumTableBits, move_pos_op) \
        Z7_LZX_HUFF_DECODE(          sym, huff, kNumTableBits, move_pos_op, \
            Z7_HUFF_DECODE_ERROR_SYM_CHECK_NO, {})

#define NORMALIZE \
{ \
  const Byte *ptr = _buf + (_bitOffset >> 4) * 2; \
  /* _value = (((UInt32)GetUi16(ptr) << 16) | GetUi16(ptr + 2)) << (_bitOffset & 15); */ \
  const UInt32 v = GetUi32(ptr); \
  _value = rotlFixed (v, ((int)_bitOffset & 15) + 16); \
}

#define MOVE_POS(bs, numBits) \
{ \
  _bitOffset += numBits; \
}

#define MOVE_POS_STAT(bs, numBits) \
{ \
  UPDATE_STAT(g_stats_len_levels[numBits]++;) \
  MOVE_POS(bs, numBits); \
}

#define MOVE_POS_CHECK(bs, numBits) \
{ \
  if (IS_OVERFLOW_bitOffset(_bitOffset += numBits)) return DECODE_ERROR_CODE; \
}

#define MOVE_POS_CHECK_STAT(bs, numBits) \
{ \
  UPDATE_STAT(g_stats_main_levels[numBits]++;) \
  MOVE_POS_CHECK(bs, numBits) \
}


// (numBits == 0) is supported

#ifdef Z7_HUFF_USE_64BIT_LIMIT

#define MACRO_ReadBitsBig_pre(numBits) \
{ \
  _bitOffset += (numBits); \
  _value >>= 32 - (numBits); \
}

#else

#define MACRO_ReadBitsBig_pre(numBits) \
{ \
  _bitOffset += (numBits); \
  _value = (UInt32)((UInt32)_value >> 1 >> (31 ^ (numBits))); \
}

#endif


#define MACRO_ReadBitsBig_add(dest) \
  { dest += (UInt32)_value; }

#define MACRO_ReadBitsBig_add3(dest) \
  { dest += (UInt32)(_value) << 3; }


// (numBits != 0)
#define MACRO_ReadBits_NonZero(val, numBits) \
{ \
  val = (UInt32)(_value >> (32 - (numBits))); \
  MOVE_POS(bs, numBits); \
  NORMALIZE \
}


struct CBitDecoder
{
  ptrdiff_t _bitOffset;
  const Byte *_buf;

  Z7_FORCE_INLINE
  UInt32 GetVal() const
  {
    const Byte *ptr = _buf + (_bitOffset >> 4) * 2;
    const UInt32 v = GetUi32(ptr);
    return rotlFixed (v, ((int)_bitOffset & 15) + 16);
  }

  Z7_FORCE_INLINE
  bool IsOverRead() const
  {
    return _bitOffset > (int)(0 - NUM_DELTA_BIT_OFFSET_BITS);
  }


  Z7_FORCE_INLINE
  bool WasBitStreamFinishedOK() const
  {
    // we check that all 0-15 unused bits are zeros:
    if (_bitOffset == 0 - NUM_DELTA_BIT_OFFSET_BITS)
      return true;
    if ((_bitOffset + NUM_DELTA_BIT_OFFSET_BITS + 15) & ~(ptrdiff_t)15)
      return false;
    const Byte *ptr = _buf - NUM_DELTA_BYTES - 2;
    if ((UInt16)(GetUi16(ptr) << (_bitOffset & 15)))
      return false;
    return true;
  }

  // (numBits != 0)
  Z7_FORCE_INLINE
  UInt32 ReadBits_NonZero(unsigned numBits) throw()
  {
    const UInt32 val = GetVal() >> (32 - numBits);
    _bitOffset += numBits;
    return val;
  }
};


class CBitByteDecoder: public CBitDecoder
{
  size_t _size;
public:

  Z7_FORCE_INLINE
  void Init_ByteMode(const Byte *data, size_t size)
  {
    _buf = data;
    _size = size;
  }

  Z7_FORCE_INLINE
  void Init_BitMode(const Byte *data, size_t size)
  {
    _size = size & 1;
    size &= ~(size_t)1;
    _buf = data + size + NUM_DELTA_BYTES;
    _bitOffset = 0 - (ptrdiff_t)(size * 8) - NUM_DELTA_BIT_OFFSET_BITS;
  }

  Z7_FORCE_INLINE
  void Switch_To_BitMode()
  {
    Init_BitMode(_buf, _size);
  }

  Z7_FORCE_INLINE
  bool Switch_To_ByteMode()
  {
    /* here we check that unused bits in high 16-bits word are zeros.
       If high word is full (all 16-bits are unused),
       we check that all 16-bits are zeros.
       So we check and skip (1-16 bits) unused bits */
    if ((GetVal() >> (16 + (_bitOffset & 15))) != 0)
      return false;
    _bitOffset += 16;
    _bitOffset &= ~(ptrdiff_t)15;
    if (_bitOffset > 0 - NUM_DELTA_BIT_OFFSET_BITS)
      return false;
    const ptrdiff_t delta = _bitOffset >> 3;
    _size = (size_t)((ptrdiff_t)(_size) - delta - NUM_DELTA_BYTES);
    _buf += delta;
    // _bitOffset = 0; // optional
    return true;
  }

  Z7_FORCE_INLINE
  size_t GetRem() const { return _size; }

  Z7_FORCE_INLINE
  UInt32 ReadUInt32()
  {
    const Byte *ptr = _buf;
    const UInt32 v = GetUi32(ptr);
    _buf += 4;
    _size -= 4;
    return v;
  }

  Z7_FORCE_INLINE
  void CopyTo(Byte *dest, size_t size)
  {
    memcpy(dest, _buf, size);
    _buf += size;
    _size -= size;
  }

  Z7_FORCE_INLINE
  bool IsOneDirectByteLeft() const
  {
    return GetRem() == 1;
  }

  Z7_FORCE_INLINE
  Byte DirectReadByte()
  {
    _size--;
    return *_buf++;
  }
};


// numBits != 0
// Z7_FORCE_INLINE
Z7_NO_INLINE
static
UInt32 ReadBits(CBitDecoder &_bitStream, unsigned numBits)
{
  return _bitStream.ReadBits_NonZero(numBits);
}

#define RIF(x) { if (!(x)) return false; }


/*
MSVC compiler adds extra move operation,
  if we access array with 32-bit index
    array[calc_index_32_bit(32-bit_var)]
    where calc_index_32_bit operations are: ((unsigned)a>>cnt), &, ^, |
  clang is also affected for ((unsigned)a>>cnt) in byte array.
*/

// it can overread input buffer for 7-17 bytes.
// (levels != levelsEnd)
Z7_NO_INLINE
static ptrdiff_t ReadTable(ptrdiff_t _bitOffset, const Byte *_buf, Byte *levels, const Byte *levelsEnd)
{
  const unsigned kNumTableBits_Level = 7;
  NHuffman::CDecoder256<kNumHuffmanBits, kLevelTableSize, kNumTableBits_Level> _levelDecoder;
  NHuffman::CValueInt _value;
  // optional check to reduce size of overread zone:
  if (_bitOffset > (int)0 - (int)NUM_DELTA_BIT_OFFSET_BITS - (int)(kLevelTableSize * kNumLevelBits))
    return DECODE_ERROR_CODE;
  NORMALIZE
  {
    Byte levels2[kLevelTableSize / 4 * 4];
    for (size_t i = 0; i < kLevelTableSize / 4 * 4; i += 4)
    {
      UInt32 val;
      MACRO_ReadBits_NonZero(val, kNumLevelBits * 4)
      levels2[i + 0] = (Byte)((val >> (3 * kNumLevelBits)));
      levels2[i + 1] = (Byte)((val >> (2 * kNumLevelBits)) & ((1u << kNumLevelBits) - 1));
      levels2[i + 2] = (Byte)((Byte)val >> (1 * kNumLevelBits));
      levels2[i + 3] = (Byte)((val) & ((1u << kNumLevelBits) - 1));
    }
    RIF(_levelDecoder.Build(levels2, NHuffman::k_BuildMode_Full))
  }
  
  do
  {
    unsigned sym;
    Z7_LZX_HUFF_DECODE_CHECK_NO(sym, &_levelDecoder, kNumTableBits_Level, MOVE_POS_CHECK)
    // Z7_HUFF_DECODE_CHECK(sym, &_levelDecoder, kNumHuffmanBits, kNumTableBits_Level, &bitStream, return false)
    // sym = _levelDecoder.Decode(&bitStream);
    // if (!_levelDecoder.Decode_SymCheck_MovePosCheck(&bitStream, sym)) return false;

    if (sym <= kNumHuffmanBits)
    {
      int delta = (int)*levels - (int)sym;
      delta += delta < 0 ? kNumHuffmanBits + 1 : 0;
      *levels++ = (Byte)delta;
      continue;
    }
    
    unsigned num;
    int symbol;

    if (sym < kLevelSym_Same)
    {
      // sym -= kLevelSym_Zero1;
      MACRO_ReadBits_NonZero(num, kLevelSym_Zero1_NumBits + (sym - kLevelSym_Zero1))
      num += (sym << kLevelSym_Zero1_NumBits) - (kLevelSym_Zero1 << kLevelSym_Zero1_NumBits) + kLevelSym_Zero1_Start;
      symbol = 0;
    }
    // else if (sym != kLevelSym_Same) return DECODE_ERROR_CODE;
    else // (sym == kLevelSym_Same)
    {
      MACRO_ReadBits_NonZero(num, kLevelSym_Same_NumBits)
      num += kLevelSym_Same_Start;
      // + (unsigned)bitStream.ReadBitsSmall(kLevelSym_Same_NumBits);
      // Z7_HUFF_DECODE_CHECK(sym, &_levelDecoder, kNumHuffmanBits, kNumTableBits_Level, &bitStream, return DECODE_ERROR_CODE)
      // if (!_levelDecoder.Decode2(&bitStream, sym)) return DECODE_ERROR_CODE;
      // sym = _levelDecoder.Decode(&bitStream);

      Z7_LZX_HUFF_DECODE_CHECK_NO(sym, &_levelDecoder, kNumTableBits_Level, MOVE_POS)

      if (sym > kNumHuffmanBits) return DECODE_ERROR_CODE;
      symbol = *levels - (int)sym;
      symbol += symbol < 0 ? kNumHuffmanBits + 1 : 0;
    }

    if (num > (size_t)(levelsEnd - levels))
      return false;
    const Byte *limit = levels + num;
    do
      *levels++ = (Byte)symbol;
    while (levels != limit);
  }
  while (levels != levelsEnd);

  return _bitOffset;
}


static const unsigned kPosSlotDelta = 256 / kNumLenSlots - kPosSlotOffset;


#define READ_TABLE(_bitStream, levels, levelsEnd) \
{ \
  _bitStream._bitOffset = ReadTable(_bitStream._bitOffset, _bitStream._buf, levels, levelsEnd); \
  if (_bitStream.IsOverRead()) return false; \
}

// can over-read input buffer for less than 32 bytes
bool CDecoder::ReadTables(CBitByteDecoder &_bitStream) throw()
{
  UPDATE_STAT(g_stats_NumTables++;)
  {
    const unsigned blockType = (unsigned)ReadBits(_bitStream, kBlockType_NumBits);
    // if (blockType > kBlockType_Uncompressed || blockType == 0)
    if ((unsigned)(blockType - 1) > kBlockType_Uncompressed - 1)
      return false;
    _unpackBlockSize = 1u << 15;
    if (!_wimMode || ReadBits(_bitStream, 1) == 0)
    {
      _unpackBlockSize = ReadBits(_bitStream, 16);
      // wimlib supports chunks larger than 32KB (unsupported my MS wim).
      if (!_wimMode || _numDictBits >= 16)
      {
        _unpackBlockSize <<= 8;
        _unpackBlockSize |= ReadBits(_bitStream, 8);
      }
    }

    PRF(printf("\nBlockSize = %6d   %s  ", _unpackBlockSize, (_pos & 1) ? "@@@" : "   "));

    _isUncompressedBlock = (blockType == kBlockType_Uncompressed);
    _skipByte = false;

    if (_isUncompressedBlock)
    {
      _skipByte = ((_unpackBlockSize & 1) != 0);
      // printf("\n UncompressedBlock %d", _unpackBlockSize);
      PRF(printf(" UncompressedBlock ");)
      // if (_unpackBlockSize & 1) { PRF(printf(" ######### ")); }
      if (!_bitStream.Switch_To_ByteMode())
        return false;
      if (_bitStream.GetRem() < kNumReps * 4)
        return false;
      for (unsigned i = 0; i < kNumReps; i++)
      {
        const UInt32 rep = _bitStream.ReadUInt32();
        // here we allow only such values for (rep) that can be set also by LZ code:
        if (rep == 0 || rep > _winSize - kNumReps)
          return false;
        _reps[(size_t)i + kPosSlotOffset] = rep;
      }
      // printf("\n");
      return true;
    }
    
    // _numAlignBits = 64;
    // const UInt32 k_numAlignBits_PosSlots_MAX = 64 + kPosSlotDelta;
    // _numAlignBits_PosSlots = k_numAlignBits_PosSlots_MAX;
    const UInt32 k_numAlignBits_Dist_MAX = (UInt32)(Int32)-1;
    _numAlignBits_Dist = k_numAlignBits_Dist_MAX;
    if (blockType == kBlockType_Aligned)
    {
      Byte levels[kAlignTableSize];
      // unsigned not0 = 0;
      unsigned not3 = 0;
      for (unsigned i = 0; i < kAlignTableSize; i++)
      {
        const unsigned val = ReadBits(_bitStream, kNumAlignLevelBits);
        levels[i] = (Byte)val;
        // not0 |= val;
        not3 |= (val ^ 3);
      }
      // static unsigned number = 0, all = 0; all++;
      // if (!not0) return false; // Build(true) will test this case
      if (not3)
      {
        // _numAlignBits_PosSlots = (kNumAlignBits + 1) * 2 + kPosSlotDelta;
        // _numAlignBits = kNumAlignBits;
        _numAlignBits_Dist = (1u << (kNumAlignBits + 1)) - (kNumReps - 1);
        RIF(_alignDecoder.Build(levels, true)) // full
      }
      // else { number++; if (number % 4 == 0) printf("\nnumber= %u : %u%%", number, number * 100 / all); }
    }
    // if (_numAlignBits_PosSlots == k_numAlignBits_PosSlots_MAX)
    if (_numAlignBits_Dist == k_numAlignBits_Dist_MAX)
    {
      size_t i;
      for (i = 3; i < kNumLinearPosSlotBits; i++)
      {
        _extra[i * 2 + 2 + kPosSlotOffset] = (Byte)(SET_NUM_BITS(i));
        _extra[i * 2 + 3 + kPosSlotOffset] = (Byte)(SET_NUM_BITS(i));
      }
      for (i = kNumLinearPosSlotBits * 2 + 2; i < kNumPosSlots; i++)
        _extra[i + kPosSlotOffset] = (Byte)SET_NUM_BITS(kNumLinearPosSlotBits);
    }
    else
    {
      size_t i;
      for (i = 3; i < kNumLinearPosSlotBits; i++)
      {
        _extra[i * 2 + 2 + kPosSlotOffset] = (Byte)(SET_NUM_BITS(i) - 3);
        _extra[i * 2 + 3 + kPosSlotOffset] = (Byte)(SET_NUM_BITS(i) - 3);
      }
      for (i = kNumLinearPosSlotBits * 2 + 2; i < kNumPosSlots; i++)
        _extra[i + kPosSlotOffset] = (Byte)(SET_NUM_BITS(kNumLinearPosSlotBits) - 3);
    }
  }

  READ_TABLE(_bitStream, _mainLevels, _mainLevels + 256)
  READ_TABLE(_bitStream, _mainLevels + 256, _mainLevels + 256 + _numPosLenSlots)
  const unsigned end = 256 + _numPosLenSlots;
  memset(_mainLevels + end, 0, kMainTableSize - end);
  // #define NUM_CYC 1
  // unsigned j; for (j = 0; j < NUM_CYC; j++)
  RIF(_mainDecoder.Build(_mainLevels, NHuffman::k_BuildMode_Full))
  // if (kNumLenSymols_Big_Start)
  memset(_lenLevels, 0, kNumLenSymols_Big_Start);
  READ_TABLE(_bitStream,
      _lenLevels + kNumLenSymols_Big_Start,
      _lenLevels + kNumLenSymols_Big_Start + kNumLenSymbols)
  // for (j = 0; j < NUM_CYC; j++)
  RIF(_lenDecoder.Build(_lenLevels, NHuffman::k_BuildMode_Full_or_Empty))
  return true;
}



static ptrdiff_t CodeLz(CDecoder *dec, size_t next, ptrdiff_t _bitOffset, const Byte *_buf) throw()
{
  {
    Byte *const win = dec->_win;
    const UInt32 winSize = dec->_winSize;
    Byte *pos = win + dec->_pos;
    const Byte * const posEnd = pos + next;
    NHuffman::CValueInt _value;

    NORMALIZE

#if 1
  #define HUFF_DEC_PREFIX  dec->
#else
    const NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize, kNumTableBits_Main> _mainDecoder = dec->_mainDecoder;
    const NHuffman::CDecoder256<kNumHuffmanBits, kNumLenSymbols, kNumTableBits_Len> _lenDecoder = dec->_lenDecoder;
    const NHuffman::CDecoder7b<kAlignTableSize> _alignDecoder = dec->_alignDecoder;
  #define HUFF_DEC_PREFIX
#endif

    do
    {
      unsigned sym;
      // printf("\npos = %6u", pos - win);
      {
        const NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize, kNumTableBits_Main>
            *mainDecoder = & HUFF_DEC_PREFIX _mainDecoder;
        Z7_LZX_HUFF_DECODE_CHECK_NO(sym, mainDecoder, kNumTableBits_Main, MOVE_POS_CHECK_STAT)
      }
      // if (!_mainDecoder.Decode_SymCheck_MovePosCheck(&bitStream, sym)) return DECODE_ERROR_CODE;
      // sym = _mainDecoder.Decode(&bitStream);
      // if (bitStream.WasExtraReadError_Fast()) return DECODE_ERROR_CODE;

      // printf(" sym = %3x", sym);
      UPDATE_STAT(g_stats_main[sym]++;)
      
      if (sym < 256)
      {
        UPDATE_STAT(g_stats_NumLits++;)
        *pos++ = (Byte)sym;
      }
      else
      {
        // sym -= 256;
        // if (sym >= _numPosLenSlots) return DECODE_ERROR_CODE;
        const unsigned posSlot = sym / kNumLenSlots;
        unsigned len = sym % kNumLenSlots + kMatchMinLen;
        if (len == kNumLenSlots - 1 + kMatchMinLen)
        {
          const NHuffman::CDecoder256<kNumHuffmanBits, kNumLenSymbols, kNumTableBits_Len>
              *lenDecoder = & HUFF_DEC_PREFIX _lenDecoder;
          Z7_LZX_HUFF_DECODE_CHECK_YES(len, lenDecoder, kNumTableBits_Len, MOVE_POS_STAT)
          // if (!_lenDecoder.Decode2(&bitStream, len)) return DECODE_ERROR_CODE;
          // len = _lenDecoder.Decode(&bitStream);
          // if (len >= kNumLenSymbols) return DECODE_ERROR_CODE;
          UPDATE_STAT(g_stats_len[len - kNumLenSymols_Big_Start]++;)
          len += kNumLenSlots - 1 + kMatchMinLen - kNumLenSymols_Big_Start;
        }
        /*
        if ((next -= len) < 0)
          return DECODE_ERROR_CODE;
        */
        UInt32 dist;
        
        dist = dec->_reps[(size_t)posSlot - kPosSlotDelta];
        if (posSlot < kNumReps + 256 / kNumLenSlots)
        {
          // if (posSlot != kNumReps + kPosSlotDelta)
          // if (posSlot - (kNumReps + kPosSlotDelta + 1) < 2)
          dec->_reps[(size_t)posSlot - kPosSlotDelta] = dec->_reps[kPosSlotOffset];
          /*
          if (posSlot != kPosSlotDelta)
          {
            UInt32 temp = dist;
            if (posSlot == kPosSlotDelta + 1)
            {
              dist = reps[1];
              reps[1] = temp;
            }
            else
            {
              dist = reps[2];
              reps[2] = temp;
            }
            // dist = reps[(size_t)(posSlot) - kPosSlotDelta];
            // reps[(size_t)(posSlot) - kPosSlotDelta] = reps[0];
            // reps[(size_t)(posSlot) - kPosSlotDelta] = temp;
          }
          */
        }
        else // if (posSlot != kNumReps + kPosSlotDelta)
        {
          unsigned numDirectBits;
#if 0
          if (posSlot < kNumPowerPosSlots + kPosSlotDelta)
          {
            numDirectBits = (posSlot - 2 - kPosSlotDelta) >> 1;
            dist = (UInt32)(2 | (posSlot & 1)) << numDirectBits;
          }
          else
          {
            numDirectBits = kNumLinearPosSlotBits;
            dist = (UInt32)(posSlot - 0x22 - kPosSlotDelta) << kNumLinearPosSlotBits;
          }
          dist -= kNumReps - 1;
#else
          numDirectBits = dec->_extra[(size_t)posSlot - kPosSlotDelta];
          // dist = reps[(size_t)(posSlot) - kPosSlotDelta];
#endif
          dec->_reps[kPosSlotOffset + 2] =
          dec->_reps[kPosSlotOffset + 1];
          dec->_reps[kPosSlotOffset + 1] =
          dec->_reps[kPosSlotOffset + 0];

          // dist += val; dist += bitStream.ReadBitsBig(numDirectBits);
          // if (posSlot >= _numAlignBits_PosSlots)
          // if (numDirectBits >= _numAlignBits)
          // if (val >= _numAlignBits_Dist)
          // UInt32 val; MACRO_ReadBitsBig(val , numDirectBits)
          // dist += val;
          // dist += (UInt32)((UInt32)_value >> 1 >> (/* 31 ^ */ (numDirectBits)));
          // MOVE_POS((numDirectBits ^ 31))
          MACRO_ReadBitsBig_pre(numDirectBits)
          // dist += (UInt32)_value;
          if (dist >= dec->_numAlignBits_Dist)
          {
            // if (numDirectBits != _numAlignBits)
            {
              // UInt32 val;
              // dist -= (UInt32)_value;
              MACRO_ReadBitsBig_add3(dist)
              NORMALIZE
              // dist += (val << kNumAlignBits);
              // dist += bitStream.ReadBitsSmall(numDirectBits - kNumAlignBits) << kNumAlignBits;
            }
            {
              // const unsigned alignTemp = _alignDecoder.Decode(&bitStream);
              const NHuffman::CDecoder7b<kAlignTableSize> *alignDecoder = & HUFF_DEC_PREFIX _alignDecoder;
              unsigned alignTemp;
              UPDATE_STAT(g_stats_NumAlign++;)
              Z7_HUFF_DECODER_7B_DECODE(alignTemp, alignDecoder, GET_VAL_BASE, MOVE_POS, bs)
              // NORMALIZE
              // if (alignTemp >= kAlignTableSize) return DECODE_ERROR_CODE;
              dist += alignTemp;
            }
          }
          else
          {
            {
              MACRO_ReadBitsBig_add(dist)
              // dist += bitStream.ReadBitsSmall(numDirectBits - kNumAlignBits) << kNumAlignBits;
            }
          }
          NORMALIZE
          /*
          else
          {
            UInt32 val;
            MACRO_ReadBitsBig(val, numDirectBits)
            dist += val;
            // dist += bitStream.ReadBitsBig(numDirectBits);
          }
          */
        }
        dec->_reps[kPosSlotOffset + 0] = dist;

        Byte *dest = pos;
        if (len > (size_t)(posEnd - pos))
          return DECODE_ERROR_CODE;
        Int32 srcPos = (Int32)(pos - win);
        pos += len;
        srcPos -= (Int32)dist;
        if (srcPos < 0) // fast version
        {
          if (!dec->_overDict)
            return DECODE_ERROR_CODE;
          srcPos &= winSize - 1;
          UInt32 rem = winSize - (UInt32)srcPos;
          if (len > rem)
          {
            len -= rem;
            const Byte *src = win + (UInt32)srcPos;
            do
              *dest++ = *src++;
            while (--rem);
            srcPos = 0;
          }
        }
        CopyLzMatch(dest, win + (UInt32)srcPos, len, dist);
      }
    }
    while (pos != posEnd);
    
    return _bitOffset;
  }
}




// inSize != 0
// outSize != 0 ???
HRESULT CDecoder::CodeSpec(const Byte *inData, size_t inSize, UInt32 outSize) throw()
{
  // ((inSize & 1) != 0) case is possible, if current call will be finished with Uncompressed Block.
  CBitByteDecoder _bitStream;
  if (_keepHistory && _isUncompressedBlock)
    _bitStream.Init_ByteMode(inData, inSize);
  else
    _bitStream.Init_BitMode(inData, inSize);
 
  if (!_keepHistory)
  {
    _isUncompressedBlock = false;
    _skipByte = false;
    _unpackBlockSize = 0;
    memset(_mainLevels, 0, sizeof(_mainLevels));
    memset(_lenLevels, 0, sizeof(_lenLevels));
    {
      _x86_translationSize = 12000000;
      if (!_wimMode)
      {
        _x86_translationSize = 0;
        if (ReadBits(_bitStream, 1) != 0)
        {
          UInt32 v = ReadBits(_bitStream, 16) << 16;
          v       |= ReadBits(_bitStream, 16);
          _x86_translationSize = v;
        }
      }
      _x86_processedSize = 0;
    }
    _reps[0 + kPosSlotOffset] = 1;
    _reps[1 + kPosSlotOffset] = 1;
    _reps[2 + kPosSlotOffset] = 1;
  }

  while (outSize)
  {
    /*
    // check it for bit mode only:
    if (_bitStream.WasExtraReadError_Fast())
      return S_FALSE;
    */
    if (_unpackBlockSize == 0)
    {
      if (_skipByte)
      {
        if (_bitStream.GetRem() < 1)
          return S_FALSE;
        if (_bitStream.DirectReadByte() != 0)
          return S_FALSE;
      }
      if (_isUncompressedBlock)
        _bitStream.Switch_To_BitMode();
      if (!ReadTables(_bitStream))
        return S_FALSE;
      continue;
    }

    // _unpackBlockSize != 0
    UInt32 next = _unpackBlockSize;
    if (next > outSize)
        next = outSize;
    // next != 0

    // PRF(printf("\nnext = %d", (unsigned)next);)
    
    if (_isUncompressedBlock)
    {
      if (_bitStream.GetRem() < next)
        return S_FALSE;
      _bitStream.CopyTo(_win + _pos, next);
      _pos += next;
      _unpackBlockSize -= next;
    }
    else
    {
      _unpackBlockSize -= next;
      _bitStream._bitOffset = CodeLz(this, next, _bitStream._bitOffset, _bitStream._buf);
      if (_bitStream.IsOverRead())
        return S_FALSE;
      _pos += next;
    }
    outSize -= next;
  }

  // outSize == 0

  if (_isUncompressedBlock)
  {
    /* we don't know where skipByte can be placed, if it's end of chunk:
        1) in current chunk - there are such cab archives, if chunk is last
        2) in next chunk - are there such archives ? */
    if (_unpackBlockSize == 0
        && _skipByte
        // && outSize == 0
        && _bitStream.IsOneDirectByteLeft())
    {
      _skipByte = false;
      if (_bitStream.DirectReadByte() != 0)
        return S_FALSE;
    }
  }

  if (_bitStream.GetRem() != 0)
    return S_FALSE;
  if (!_isUncompressedBlock)
    if (!_bitStream.WasBitStreamFinishedOK())
      return S_FALSE;
  return S_OK;
}


#if k_Filter_OutBufSize_Add > k_Lz_OutBufSize_Add
  #define k_OutBufSize_Add  k_Filter_OutBufSize_Add
#else
  #define k_OutBufSize_Add  k_Lz_OutBufSize_Add
#endif

HRESULT CDecoder::Code_WithExceedReadWrite(const Byte *inData, size_t inSize, UInt32 outSize) throw()
{
  if (!_keepHistory)
  {
    _pos = 0;
    _overDict = false;
  }
  else if (_pos == _winSize)
  {
    _pos = 0;
    _overDict = true;
#if k_OutBufSize_Add > 0
    // data after (_winSize) can be used, because we can use overwrite.
    // memset(_win + _winSize, 0, k_OutBufSize_Add);
#endif
  }
  _writePos = _pos;
  _unpackedData = _win + _pos;
 
  if (outSize > _winSize - _pos)
    return S_FALSE;
  
  PRF(printf("\ninSize = %d", (unsigned)inSize);)
  PRF(if ((inSize & 1) != 0) printf("---------");)

  if (inSize == 0)
    return S_FALSE;
  const HRESULT res = CodeSpec(inData, inSize, outSize);
  const HRESULT res2 = Flush();
  return (res == S_OK ? res2 : res);
}


HRESULT CDecoder::SetParams2(unsigned numDictBits) throw()
{
  if (numDictBits < kNumDictBits_Min ||
      numDictBits > kNumDictBits_Max)
    return E_INVALIDARG;
  _numDictBits = (Byte)numDictBits;
  const unsigned numPosSlots2 = (numDictBits < 20) ?
      numDictBits : 17 + (1u << (numDictBits - 18));
  _numPosLenSlots = numPosSlots2 * (kNumLenSlots * 2);
  return S_OK;
}
  

HRESULT CDecoder::Set_DictBits_and_Alloc(unsigned numDictBits) throw()
{
  RINOK(SetParams2(numDictBits))
  const UInt32 newWinSize = (UInt32)1 << numDictBits;
  if (_needAlloc)
  {
    if (!_win || newWinSize != _winSize)
    {
      // BigFree
      z7_AlignedFree
        (_win);
      _winSize = 0;
      const size_t alloc_size = newWinSize + k_OutBufSize_Add;
      _win = (Byte *)
          // BigAlloc
          z7_AlignedAlloc
          (alloc_size);
      if (!_win)
        return E_OUTOFMEMORY;
      // optional:
      memset(_win, 0, alloc_size);
    }
  }
  _winSize = newWinSize;
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Compress/PpmdDecoder.cpp ================ */
// PpmdDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NPpmd {

static const UInt32 kBufSize = (1 << 16);

enum
{
  kStatus_NeedInit,
  kStatus_Normal,
  kStatus_Finished_With_Mark,
  kStatus_Error
};

CDecoder::~CDecoder()
{
  ::MidFree(_outBuf);
  Ppmd7_Free(&_ppmd, &g_BigAlloc);
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *props, UInt32 size))
{
  if (size < 5)
    return E_INVALIDARG;
  _order = props[0];
  const UInt32 memSize = GetUi32(props + 1);
  if (_order < PPMD7_MIN_ORDER ||
      _order > PPMD7_MAX_ORDER ||
      memSize < PPMD7_MIN_MEM_SIZE ||
      memSize > PPMD7_MAX_MEM_SIZE)
    return E_NOTIMPL;
  if (!_inStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;
  if (!Ppmd7_Alloc(&_ppmd, memSize, &g_BigAlloc))
    return E_OUTOFMEMORY;
  return S_OK;
}

#define MY_rangeDec  _ppmd.rc.dec

#define CHECK_EXTRA_ERROR \
    if (_inStream.Extra) { \
      _status = kStatus_Error; \
      return (_res = (_inStream.Res != SZ_OK ? _inStream.Res: S_FALSE)); }


HRESULT CDecoder::CodeSpec(Byte *memStream, UInt32 size)
{
  if (_res != S_OK)
    return _res;
  
  switch (_status)
  {
    case kStatus_Finished_With_Mark: return S_OK;
    case kStatus_Error: return S_FALSE;
    case kStatus_NeedInit:
      _inStream.Init();
      if (!Ppmd7z_RangeDec_Init(&MY_rangeDec))
      {
        _status = kStatus_Error;
        return (_res = S_FALSE);
      }
      CHECK_EXTRA_ERROR
      _status = kStatus_Normal;
      Ppmd7_Init(&_ppmd, _order);
      break;
    default: break;
  }
  
  if (_outSizeDefined)
  {
    const UInt64 rem = _outSize - _processedSize;
    if (size > rem)
      size = (UInt32)rem;
  }

  int sym = 0;
  {
    Byte *buf = memStream;
    const Byte *lim = buf + size;
    for (; buf != lim; buf++)
    {
      sym = Ppmd7z_DecodeSymbol(&_ppmd);
      if (_inStream.Extra || sym < 0)
        break;
      *buf = (Byte)sym;
    }
    /*
    buf = Ppmd7z_DecodeSymbols(&_ppmd, buf, lim);
    sym = _ppmd.LastSymbol;
    */
    _processedSize += (size_t)(buf - memStream);
  }

  CHECK_EXTRA_ERROR
  
  if (sym >= 0)
  {
    if (!FinishStream
        || !_outSizeDefined
        || _outSize != _processedSize
        || MY_rangeDec.Code == 0)
      return S_OK;
    /*
    // We can decode additional End Marker here:
    sym = Ppmd7z_DecodeSymbol(&_ppmd);
    CHECK_EXTRA_ERROR
    */
  }

  if (sym != PPMD7_SYM_END || MY_rangeDec.Code != 0)
  {
    _status = kStatus_Error;
    return (_res = S_FALSE);
  }
  
  _status = kStatus_Finished_With_Mark;
  return S_OK;
}



Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  if (!_outBuf)
  {
    _outBuf = (Byte *)::MidAlloc(kBufSize);
    if (!_outBuf)
      return E_OUTOFMEMORY;
  }
  
  _inStream.Stream = inStream;
  SetOutStreamSize(outSize);

  do
  {
    const UInt64 startPos = _processedSize;
    const HRESULT res = CodeSpec(_outBuf, kBufSize);
    const size_t processed = (size_t)(_processedSize - startPos);
    RINOK(WriteStream(outStream, _outBuf, processed))
    RINOK(res)
    if (_status == kStatus_Finished_With_Mark)
      break;
    if (progress)
    {
      const UInt64 inProcessed = _inStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&inProcessed, &_processedSize))
    }
  }
  while (!_outSizeDefined || _processedSize < _outSize);

  if (FinishStream && inSize && *inSize != _inStream.GetProcessed())
    return S_FALSE;

  return S_OK;
}


Z7_COM7F_IMF(CDecoder::SetOutStreamSize(const UInt64 *outSize))
{
  _outSizeDefined = (outSize != NULL);
  if (_outSizeDefined)
    _outSize = *outSize;
  _processedSize = 0;
  _status = kStatus_NeedInit;
  _res = SZ_OK;
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  FinishStream = (finishMode != 0);
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inStream.GetProcessed();
  return S_OK;
}

#ifndef Z7_NO_READ_FROM_CODER

Z7_COM7F_IMF(CDecoder::SetInStream(ISequentialInStream *inStream))
{
  InSeqStream = inStream;
  _inStream.Stream = inStream;
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::ReleaseInStream())
{
  InSeqStream.Release();
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  const UInt64 startPos = _processedSize;
  const HRESULT res = CodeSpec((Byte *)data, size);
  if (processedSize)
    *processedSize = (UInt32)(_processedSize - startPos);
  return res;
}

#endif

}}

/* ================ unit: CPP/7zip/Compress/PpmdEncoder.cpp ================ */
// PpmdEncoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NPpmd {

static const UInt32 kPpmdEnc_kBufSize = (1 << 20);

static const Byte kOrders[10] = { 3, 4, 4, 5, 5, 6, 8, 16, 24, 32 };

void CEncProps::Normalize(int level)
{
  if (level < 0) level = 5;
  if (level > 9) level = 9;
  if (MemSize == (UInt32)(Int32)-1)
    MemSize = (UInt32)1 << (level + 19);
  const unsigned kMult = 16;
  if (MemSize / kMult > ReduceSize)
  {
    for (unsigned i = 16; i < 32; i++)
    {
      UInt32 m = (UInt32)1 << i;
      if (ReduceSize <= m / kMult)
      {
        if (MemSize > m)
          MemSize = m;
        break;
      }
    }
  }
  if (Order == -1) Order = kOrders[(unsigned)level];
}

CEncoder::CEncoder():
  _inBuf(NULL)
{
  _props.Normalize(-1);
  Ppmd7_Construct(&_ppmd);
  _ppmd.rc.enc.Stream = &_outStream.vt;
}

CEncoder::~CEncoder()
{
  ::MidFree(_inBuf);
  Ppmd7_Free(&_ppmd, &g_BigAlloc);
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
{
  int level = -1;
  CEncProps props;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];
    if (propID > NCoderPropID::kReduceSize)
      continue;
    if (propID == NCoderPropID::kReduceSize)
    {
      if (prop.vt == VT_UI8 && prop.uhVal.QuadPart < (UInt32)(Int32)-1)
        props.ReduceSize = (UInt32)prop.uhVal.QuadPart;
      continue;
    }

    if (propID == NCoderPropID::kUsedMemorySize)
    {
      // here we have selected (4 GiB - 1 KiB) as replacement for (4 GiB) MEM_SIZE.
      const UInt32 kPpmd_Default_4g = (UInt32)0 - ((UInt32)1 << 10);
      UInt32 v;
      if (prop.vt == VT_UI8)
      {
        // 21.03 : we support 64-bit values (for 4 GiB value)
        const UInt64 v64 = prop.uhVal.QuadPart;
        if (v64 > ((UInt64)1 << 32))
          return E_INVALIDARG;
        if (v64 == ((UInt64)1 << 32))
          v = kPpmd_Default_4g;
        else
          v = (UInt32)v64;
      }
      else if (prop.vt == VT_UI4)
        v = (UInt32)prop.ulVal;
      else
        return E_INVALIDARG;
      if (v > PPMD7_MAX_MEM_SIZE)
        v = kPpmd_Default_4g;

      /* here we restrict MEM_SIZE for Encoder.
         It's for better performance of encoding and decoding.
         The Decoder still supports more MEM_SIZE values. */
      if (v < ((UInt32)1 << 16) || (v & 3) != 0)
        return E_INVALIDARG;
      // if (v < PPMD7_MIN_MEM_SIZE) return E_INVALIDARG; // (1 << 11)
      /*
        Supported MEM_SIZE range :
        [ (1 << 11) , 0xFFFFFFFF - 12 * 3 ] - current 7-Zip's Ppmd7 constants
        [ 1824      , 0xFFFFFFFF          ] - real limits of Ppmd7 code
      */
      props.MemSize = v;
      continue;
    }

    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    const UInt32 v = (UInt32)prop.ulVal;
    switch (propID)
    {
      case NCoderPropID::kOrder:
        if (v < 2 || v > 32)
          return E_INVALIDARG;
        props.Order = (Byte)v;
        break;
      case NCoderPropID::kNumThreads: break;
      case NCoderPropID::kLevel: level = (int)v; break;
      default: return E_INVALIDARG;
    }
  }
  props.Normalize(level);
  _props = props;
  return S_OK;
}

Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  const UInt32 kPropSize = 5;
  Byte props[kPropSize];
  props[0] = (Byte)_props.Order;
  SetUi32(props + 1, _props.MemSize)
  return WriteStream(outStream, props, kPropSize);
}

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  if (!_inBuf)
  {
    _inBuf = (Byte *)::MidAlloc(kPpmdEnc_kBufSize);
    if (!_inBuf)
      return E_OUTOFMEMORY;
  }
  if (!_outStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;
  if (!Ppmd7_Alloc(&_ppmd, _props.MemSize, &g_BigAlloc))
    return E_OUTOFMEMORY;

  _outStream.Stream = outStream;
  _outStream.Init();

  Ppmd7z_Init_RangeEnc(&_ppmd);
  Ppmd7_Init(&_ppmd, (unsigned)_props.Order);

  UInt64 processed = 0;
  for (;;)
  {
    UInt32 size;
    RINOK(inStream->Read(_inBuf, kPpmdEnc_kBufSize, &size))
    if (size == 0)
    {
      // We don't write EndMark in PPMD-7z.
      // Ppmd7z_EncodeSymbol(&_ppmd, -1);
      Ppmd7z_Flush_RangeEnc(&_ppmd);
      return _outStream.Flush();
    }
    const Byte *buf = _inBuf;
    const Byte *lim = buf + size;
    /*
    for (; buf < lim; buf++)
    {
      Ppmd7z_EncodeSymbol(&_ppmd, *buf);
      RINOK(_outStream.Res);
    }
    */

    Ppmd7z_EncodeSymbols(&_ppmd, buf, lim);
    RINOK(_outStream.Res)

    processed += size;
    if (progress)
    {
      const UInt64 outSize = _outStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&processed, &outSize))
    }
  }
}

}}

/* ================ unit: CPP/7zip/Compress/PpmdZip.cpp ================ */
// PpmdZip.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NPpmdZip {

static const UInt32 kBufSize = 1 << 20;

bool CBuf::Alloc()
{
  if (!Buf)
    Buf = (Byte *)::MidAlloc(kBufSize);
  return (Buf != NULL);
}

CDecoder::CDecoder(bool fullFileMode):
  _fullFileMode(fullFileMode)
{
  Ppmd8_Construct(&_ppmd);
  _ppmd.Stream.In = &_inStream.vt;
}

CDecoder::~CDecoder()
{
  Ppmd8_Free(&_ppmd, &g_BigAlloc);
}

Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  // try {

  if (!_outStream.Alloc())
    return E_OUTOFMEMORY;
  if (!_inStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;

  _inStream.Stream = inStream;
  _inStream.Init();

  {
    Byte buf[2];
    for (int i = 0; i < 2; i++)
      buf[i] = _inStream.ReadByte();
    if (_inStream.Extra)
      return S_FALSE;
    
    const UInt32 val = GetUi16(buf);
    const unsigned order = (val & 0xF) + 1;
    const UInt32 mem = ((val >> 4) & 0xFF) + 1;
    const unsigned restor = (val >> 12);
    if (order < 2 || restor > 2)
      return S_FALSE;
    
    #ifndef PPMD8_FREEZE_SUPPORT
    if (restor == 2)
      return E_NOTIMPL;
    #endif
    
    if (!Ppmd8_Alloc(&_ppmd, mem << 20, &g_BigAlloc))
      return E_OUTOFMEMORY;
    
    if (!Ppmd8_Init_RangeDec(&_ppmd))
      return S_FALSE;
    Ppmd8_Init(&_ppmd, order, restor);
  }

  bool wasFinished = false;
  UInt64 processedSize = 0;

  for (;;)
  {
    size_t size = kBufSize;
    if (outSize)
    {
      const UInt64 rem = *outSize - processedSize;
      if (size > rem)
      {
        size = (size_t)rem;
        if (size == 0)
          break;
      }
    }

    int sym;
    Byte *buf = _outStream.Buf;
    const Byte *lim = buf + size;
    
    do
    {
      sym = Ppmd8_DecodeSymbol(&_ppmd);
      if (_inStream.Extra || sym < 0)
        break;
      *buf++ = (Byte)sym;
    }
    while (buf != lim);
    
    const size_t cur = (size_t)(buf - _outStream.Buf);
    processedSize += cur;

    RINOK(WriteStream(outStream, _outStream.Buf, cur))

    RINOK(_inStream.Res)
    if (_inStream.Extra)
      return S_FALSE;

    if (sym < 0)
    {
      if (sym != -1)
        return S_FALSE;
      wasFinished = true;
      break;
    }
    
    if (progress)
    {
      const UInt64 inProccessed = _inStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&inProccessed, &processedSize))
    }
  }
  
  RINOK(_inStream.Res)
  
  if (_fullFileMode)
  {
    if (!wasFinished)
    {
      const int res = Ppmd8_DecodeSymbol(&_ppmd);
      RINOK(_inStream.Res)
      if (_inStream.Extra || res != -1)
        return S_FALSE;
    }
    if (!Ppmd8_RangeDec_IsFinishedOK(&_ppmd))
      return S_FALSE;

    if (inSize && *inSize != _inStream.GetProcessed())
      return S_FALSE;
  }
  
  return S_OK;

  // } catch (...) { return E_FAIL; }
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  _fullFileMode = (finishMode != 0);
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inStream.GetProcessed();
  return S_OK;
}



// ---------- Encoder ----------

void CEncProps::Normalize(int level)
{
  if (level < 0) level = 5;
  if (level == 0) level = 1;
  if (level > 9) level = 9;
  if (MemSizeMB == (UInt32)(Int32)-1)
    MemSizeMB = 1 << (level - 1);
  const unsigned kMult = 16;
  for (UInt32 m = 1; m < MemSizeMB; m <<= 1)
    if (ReduceSize <= (m << 20) / kMult)
    {
      MemSizeMB = m;
      break;
    }
  if (Order == -1) Order = 3 + level;
  if (Restor == -1)
    Restor = level < 7 ?
      PPMD8_RESTORE_METHOD_RESTART :
      PPMD8_RESTORE_METHOD_CUT_OFF;
}

CEncoder::~CEncoder()
{
  Ppmd8_Free(&_ppmd, &g_BigAlloc);
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
{
  int level = -1;
  CEncProps props;
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];
    if (propID > NCoderPropID::kReduceSize)
      continue;
    if (propID == NCoderPropID::kReduceSize)
    {
      props.ReduceSize = (UInt32)(Int32)-1;
      if (prop.vt == VT_UI8 && prop.uhVal.QuadPart < (UInt32)(Int32)-1)
        props.ReduceSize = (UInt32)prop.uhVal.QuadPart;
      continue;
    }
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    const UInt32 v = (UInt32)prop.ulVal;
    switch (propID)
    {
      case NCoderPropID::kUsedMemorySize:
        if (v < (1 << 20) || v > (1 << 28))
          return E_INVALIDARG;
        props.MemSizeMB = v >> 20;
        break;
      case NCoderPropID::kOrder:
        if (v < PPMD8_MIN_ORDER || v > PPMD8_MAX_ORDER)
          return E_INVALIDARG;
        props.Order = (Byte)v;
        break;
      case NCoderPropID::kNumThreads: break;
      case NCoderPropID::kLevel: level = (int)v; break;
      case NCoderPropID::kAlgorithm:
        if (v >= PPMD8_RESTORE_METHOD_UNSUPPPORTED)
          return E_INVALIDARG;
        props.Restor = (int)v;
        break;
      default: return E_INVALIDARG;
    }
  }
  props.Normalize(level);
  _props = props;
  return S_OK;
}

CEncoder::CEncoder()
{
  _props.Normalize(-1);
  _ppmd.Stream.Out = &_outStream.vt;
  Ppmd8_Construct(&_ppmd);
}

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  if (!_inStream.Alloc())
    return E_OUTOFMEMORY;
  if (!_outStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;
  if (!Ppmd8_Alloc(&_ppmd, _props.MemSizeMB << 20, &g_BigAlloc))
    return E_OUTOFMEMORY;

  _outStream.Stream = outStream;
  _outStream.Init();

  Ppmd8_Init_RangeEnc(&_ppmd)
  Ppmd8_Init(&_ppmd, (unsigned)_props.Order, (unsigned)_props.Restor);

  {
    const unsigned val =
           ((unsigned)_props.Order - 1)
         + (((unsigned)_props.MemSizeMB - 1) << 4)
         + ((unsigned)_props.Restor << 12);
    _outStream.WriteByte((Byte)(val & 0xFF));
    _outStream.WriteByte((Byte)(val >> 8));
  }
  RINOK(_outStream.Res)

  UInt64 processed = 0;
  for (;;)
  {
    UInt32 size;
    RINOK(inStream->Read(_inStream.Buf, kBufSize, &size))
    if (size == 0)
    {
      Ppmd8_EncodeSymbol(&_ppmd, -1);
      Ppmd8_Flush_RangeEnc(&_ppmd);
      return _outStream.Flush();
    }

    processed += size;
    const Byte *buf = _inStream.Buf;
    const Byte *lim = buf + size;
    do
    {
      Ppmd8_EncodeSymbol(&_ppmd, *buf);
      if (_outStream.Res != S_OK)
        break;
    }
    while (++buf != lim);

    RINOK(_outStream.Res)

    if (progress)
    {
      const UInt64 outProccessed = _outStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&processed, &outProccessed))
    }
  }
}

}}

/* ================ unit: CPP/7zip/Compress/QuantumDecoder.cpp ================ */
// QuantumDecoder.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NQuantum {

static const unsigned kNumLenSymbols = 27;
static const unsigned kMatchMinLen = 3;
static const unsigned kNumSimpleLenSlots = 6;

static const unsigned kUpdateStep = 8;
static const unsigned kFreqSumMax = 3800;
static const unsigned kReorderCount_Start = 4;
static const unsigned kReorderCount = 50;


class CRangeDecoder
{
  UInt32 Low;
  UInt32 Range;
  UInt32 Code;

  unsigned _bitOffset;
  const Byte *_buf;
  const Byte *_bufLim;

public:

  Z7_FORCE_INLINE
  void Init(const Byte *inData, size_t inSize)
  {
    Code = ((UInt32)*inData << 8) | inData[1];
    _buf = inData + 2;
    _bufLim = inData + inSize;
    _bitOffset = 0;
    Low = 0;
    Range = 0x10000;
  }

  Z7_FORCE_INLINE
  bool WasExtraRead() const
  {
    return _buf > _bufLim;
  }

  Z7_FORCE_INLINE
  UInt32 ReadBits(unsigned numBits) // numBits > 0
  {
    unsigned bitOffset = _bitOffset;
    const Byte *buf = _buf;
    const UInt32 res = GetBe32(buf) << bitOffset;
    bitOffset += numBits;
    _buf = buf + (bitOffset >> 3);
    _bitOffset = bitOffset & 7;
    return res >> (32 - numBits);
  }

  // ---------- Range Decoder functions ----------

  Z7_FORCE_INLINE
  bool Finish()
  {
    const unsigned numBits = 2 + ((16 - 2 - _bitOffset) & 7);
    if (ReadBits(numBits) != 0)
      return false;
    return _buf == _bufLim;
  }

  Z7_FORCE_INLINE
  UInt32 GetThreshold(UInt32 total) const
  {
    return ((Code + 1) * total - 1) / Range; // & 0xFFFF is not required;
  }
  
  Z7_FORCE_INLINE
  void Decode(UInt32 start, UInt32 end, UInt32 total)
  {
    // UInt32 hi = ~(Low + end * Range / total - 1);
    UInt32 hi = 0 - (Low + end * Range / total);
    const UInt32 offset = start * Range / total;
    UInt32 lo = Low + offset;
    Code -= offset;
    UInt32 numBits = 0;
    lo ^= hi;
    while (lo & (1u << 15))
    {
      lo <<= 1;
      hi <<= 1;
      numBits++;
    }
    lo ^= hi;
    UInt32 an = lo & hi;
    while (an & (1u << 14))
    {
      an <<= 1;
      lo <<= 1;
      hi <<= 1;
      numBits++;
    }
    Low = lo;
    Range = ((~hi - lo) & 0xffff) + 1;
    if (numBits)
      Code = (Code << numBits) + ReadBits(numBits);
  }
};


// Z7_FORCE_INLINE
Z7_NO_INLINE
unsigned CModelDecoder::Decode(CRangeDecoder *rc)
// Z7_NO_INLINE void CModelDecoder::Normalize()
{
  if (Freqs[0] > kFreqSumMax)
  {
    if (--ReorderCount == 0)
    {
      ReorderCount = kReorderCount;
      {
        unsigned i = NumItems;
        unsigned next = 0;
        UInt16 *freqs = &Freqs[i];
        do
        {
          const unsigned freq = *--freqs;
          *freqs = (UInt16)((freq - next + 1) >> 1);
          next = freq;
        }
        while (--i);
      }
      {
        for (unsigned i = 0; i < NumItems - 1; i++)
        {
          UInt16 freq = Freqs[i];
          for (unsigned k = i + 1; k < NumItems; k++)
            if (freq < Freqs[k])
            {
              const UInt16 freq2 = Freqs[k];
              Freqs[k] = freq;
              Freqs[i] = freq2;
              freq = freq2;
              const Byte val = Vals[i];
              Vals[i] = Vals[k];
              Vals[k] = val;
            }
        }
      }
      unsigned i = NumItems;
      unsigned freq = 0;
      UInt16 *freqs = &Freqs[i];
      do
      {
        freq += *--freqs;
        *freqs = (UInt16)freq;
      }
      while (--i);
    }
    else
    {
      unsigned i = NumItems;
      unsigned next = 1;
      UInt16 *freqs = &Freqs[i];
      do
      {
        unsigned freq = *--freqs >> 1;
        if (freq < next)
          freq = next;
        *freqs = (UInt16)freq;
        next = freq + 1;
      }
      while (--i);
    }
  }
  unsigned res;
  {
    const unsigned freq0 = Freqs[0];
    Freqs[0] = (UInt16)(freq0 + kUpdateStep);
    const unsigned threshold = rc->GetThreshold(freq0);
    UInt16 *freqs = &Freqs[1];
    unsigned freq = *freqs;
    while (freq > threshold)
    {
      *freqs++ = (UInt16)(freq + kUpdateStep);
      freq = *freqs;
    }
    res = Vals[freqs - Freqs - 1];
    rc->Decode(freq, freqs[-1] - kUpdateStep, freq0);
  }
  return res;
}


Z7_NO_INLINE
void CModelDecoder::Init(unsigned numItems, unsigned startVal)
{
  NumItems = numItems;
  ReorderCount = kReorderCount_Start;
  UInt16 *freqs = Freqs;
  freqs[numItems] = 0;
  Byte *vals = Vals;
  do
  {
    *freqs++ = (UInt16)numItems;
    *vals++ = (Byte)startVal;
    startVal++;
  }
  while (--numItems);
}


HRESULT CDecoder::Code(const Byte *inData, size_t inSize, UInt32 outSize, bool keepHistory)
{
  if (inSize < 2)
    return S_FALSE;
  if (!keepHistory)
  {
    _winPos = 0;
    m_Selector.Init(kNumSelectors, 0);
    unsigned i;
    for (i = 0; i < kNumLitSelectors; i++)
      m_Literals[i].Init(kNumLitSymbols, i * kNumLitSymbols);
    const unsigned numItems = (_numDictBits == 0 ? 1 : (_numDictBits << 1));
    // const unsigned kNumPosSymbolsMax[kNumMatchSelectors] = { 24, 36, 42 };
    for (i = 0; i < kNumMatchSelectors; i++)
    {
      const unsigned num = 24 + i * 6 + ((i + 1) & 2) * 3;
      m_PosSlot[i].Init(MyMin(numItems, num), 0);
    }
    m_LenSlot.Init(kNumLenSymbols, kMatchMinLen + kNumMatchSelectors - 1);
  }

  CRangeDecoder rc;
  rc.Init(inData, inSize);
  const UInt32 winSize = _winSize;
  Byte *pos;
  {
    UInt32 winPos = _winPos;
    if (winPos == winSize)
    {
      winPos = 0;
      _winPos = winPos;
      _overWin = true;
    }
    if (outSize > winSize - winPos)
      return S_FALSE;
    pos = _win + winPos;
  }

  while (outSize != 0)
  {
    if (rc.WasExtraRead())
      return S_FALSE;

    const unsigned selector = m_Selector.Decode(&rc);
    
    if (selector < kNumLitSelectors)
    {
      const unsigned b = m_Literals[selector].Decode(&rc);
      *pos++ = (Byte)b;
      --outSize;
      // if (--outSize == 0) break;
    }
    else
    {
      unsigned len = selector - kNumLitSelectors + kMatchMinLen;
    
      if (selector == kNumLitSelectors + kNumMatchSelectors - 1)
      {
        len = m_LenSlot.Decode(&rc);
        if (len >= kNumSimpleLenSlots + kMatchMinLen + kNumMatchSelectors - 1)
        {
          len -= kNumSimpleLenSlots - 4 + kMatchMinLen + kNumMatchSelectors - 1;
          const unsigned numDirectBits = (unsigned)(len >> 2);
          len = ((4 | (len & 3)) << numDirectBits) - (4 << 1)
              + kNumSimpleLenSlots
              + kMatchMinLen + kNumMatchSelectors - 1;
          if (numDirectBits < 6)
            len += rc.ReadBits(numDirectBits);
        }
      }
      
      UInt32 dist = m_PosSlot[(size_t)selector - kNumLitSelectors].Decode(&rc);
      
      if (dist >= 4)
      {
        const unsigned numDirectBits = (unsigned)((dist >> 1) - 1);
        dist = ((2 | (dist & 1)) << numDirectBits) + rc.ReadBits(numDirectBits);
      }
      
      if ((Int32)(outSize -= len) < 0)
        return S_FALSE;

      ptrdiff_t srcPos = (ptrdiff_t)(Int32)((pos - _win) - (ptrdiff_t)dist - 1);
      if (srcPos < 0)
      {
        if (!_overWin)
          return S_FALSE;
        UInt32 rem = (UInt32)-srcPos;
        srcPos += winSize;
        if (rem < len)
        {
          const Byte *src = _win + srcPos;
          len -= rem;
          do
            *pos++ = *src++;
          while (--rem);
          srcPos = 0;
        }
      }
      const Byte *src = _win + srcPos;
      do
        *pos++ = *src++;
      while (--len);
      // if (outSize == 0) break;
    }
  }

  _winPos = (UInt32)(size_t)(pos - _win);
  return rc.Finish() ? S_OK : S_FALSE;
}


HRESULT CDecoder::SetParams(unsigned numDictBits)
{
  if (numDictBits > 21)
    return E_INVALIDARG;
  _numDictBits = numDictBits;
  _winPos = 0;
  _overWin = false;

  if (numDictBits < 15)
      numDictBits = 15;
  _winSize = (UInt32)1 << numDictBits;
  if (!_win || _winSize > _winSize_allocated)
  {
    MidFree(_win);
    _win = NULL;
    _win = (Byte *)MidAlloc(_winSize);
    if (!_win)
      return E_OUTOFMEMORY;
    _winSize_allocated = _winSize;
  }
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Compress/Rar1Decoder.cpp ================ */
// Rar1Decoder.cpp
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives
 
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NRar1 {

static const unsigned kNumBits = 12;

static const Byte kShortLen1[16 * 3] =
{
  0,0xa0,0xd0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff,0xc0,0x80,0x90,0x98,0x9c,0xb0,0,
  1,3,4,4,5,6,7,8,8,4,4,5,6,6,0,0,
  1,4,4,4,5,6,7,8,8,4,4,5,6,6,4,0
};

static const Byte kShortLen2[16 * 3] =
{
  0,0x40,0x60,0xa0,0xd0,0xe0,0xf0,0xf8,0xfc,0xc0,0x80,0x90,0x98,0x9c,0xb0,0,
  2,3,3,3,4,4,5,6,6,4,4,5,6,6,0,0,
  2,3,3,4,4,4,5,6,6,4,4,5,6,6,4,0
};

static const Byte PosL1[kNumBits + 1]  = { 0,0,2,1,2,2,4,5,4,4,8,0,224 };
static const Byte PosL2[kNumBits + 1]  = { 0,0,0,5,2,2,4,5,4,4,8,2,220 };

static const Byte PosHf0[kNumBits + 1] = { 0,0,0,0,8,8,8,9,0,0,0,0,224 };
static const Byte PosHf1[kNumBits + 1] = { 0,0,0,0,0,4,40,16,16,4,0,47,130 };
static const Byte PosHf2[kNumBits + 1] = { 0,0,0,0,0,2,5,46,64,116,24,0,0 };
static const Byte PosHf3[kNumBits + 1] = { 0,0,0,0,0,0,2,14,202,33,6,0,0 };
static const Byte PosHf4[kNumBits + 1] = { 0,0,0,0,0,0,0,0,255,2,0,0,0 };

static const UInt32 kHistorySize = (1 << 16);

CDecoder::CDecoder():
   _isSolid(false),
   _solidAllowed(false)
   {}

UInt32 CDecoder::ReadBits(unsigned numBits) { return m_InBitStream.ReadBits(numBits); }

HRESULT CDecoder::CopyBlock(UInt32 distance, UInt32 len)
{
  if (len == 0)
    return S_FALSE;
  if (m_UnpackSize < len)
    return S_FALSE;
  m_UnpackSize -= len;
  return m_OutWindowStream.CopyBlock(distance, len) ? S_OK : S_FALSE;
}


UInt32 CDecoder::DecodeNum(const Byte *numTab)
{
  /*
  {
    // we can check that tables are correct
    UInt32 sum = 0;
    for (unsigned i = 0; i <= kNumBits; i++)
      sum += ((UInt32)numTab[i] << (kNumBits - i));
    if (sum != (1 << kNumBits))
      throw 111;
  }
  */

  UInt32 val = m_InBitStream.GetValue(kNumBits);
  UInt32 sum = 0;
  unsigned i = 2;

  for (;;)
  {
    const UInt32 num = numTab[i];
    const UInt32 cur = num << (kNumBits - i);
    if (val < cur)
      break;
    i++;
    val -= cur;
    sum += num;
  }
  m_InBitStream.MovePos(i);
  return ((val >> (kNumBits - i)) + sum);
}


HRESULT CDecoder::ShortLZ()
{
  NumHuf = 0;

  if (LCount == 2)
  {
    if (ReadBits(1))
      return CopyBlock(LastDist, LastLength);
    LCount = 0;
  }

  UInt32 bitField = m_InBitStream.GetValue(8);

  UInt32 len, dist;
  {
    const Byte *xors = (AvrLn1 < 37) ? kShortLen1 : kShortLen2;
    const Byte *lens = xors + 16 + Buf60;
    for (len = 0; ((bitField ^ xors[len]) >> (8 - lens[len])) != 0; len++);
    m_InBitStream.MovePos(lens[len]);
  }
  
  if (len >= 9)
  {
    if (len == 9)
    {
      LCount++;
      return CopyBlock(LastDist, LastLength);
    }

    LCount = 0;

    if (len == 14)
    {
      len = DecodeNum(PosL2) + 5;
      dist = 0x8000 + ReadBits(15) - 1;
      LastLength = len;
      LastDist = dist;
      return CopyBlock(dist, len);
    }

    const UInt32 saveLen = len;
    dist = m_RepDists[(m_RepDistPtr - (len - 9)) & 3];
    
    len = DecodeNum(PosL1);
    
    if (len == 0xff && saveLen == 10)
    {
      Buf60 ^= 16;
      return S_OK;
    }
    if (dist >= 256)
    {
      len++;
      if (dist >= MaxDist3 - 1)
        len++;
    }
  }
  else
  {
    LCount = 0;
    AvrLn1 += len;
    AvrLn1 -= AvrLn1 >> 4;
    
    unsigned distancePlace = DecodeNum(PosHf2) & 0xff;
    
    dist = ChSetA[distancePlace];
    
    if (distancePlace != 0)
    {
      PlaceA[dist]--;
      UInt32 lastDistance = ChSetA[(size_t)distancePlace - 1];
      PlaceA[lastDistance]++;
      ChSetA[distancePlace] = lastDistance;
      ChSetA[(size_t)distancePlace - 1] = dist;
    }
  }

  m_RepDists[m_RepDistPtr++] = dist;
  m_RepDistPtr &= 3;
  len += 2;
  LastLength = len;
  LastDist = dist;
  return CopyBlock(dist, len);
}


HRESULT CDecoder::LongLZ()
{
  UInt32 len;
  UInt32 dist;
  UInt32 distancePlace, newDistancePlace;
  UInt32 oldAvr2, oldAvr3;

  NumHuf = 0;
  Nlzb += 16;
  if (Nlzb > 0xff)
  {
    Nlzb = 0x90;
    Nhfb >>= 1;
  }
  oldAvr2 = AvrLn2;

  if (AvrLn2 >= 64)
    len = DecodeNum(AvrLn2 < 122 ? PosL1 : PosL2);
  else
  {
    UInt32 bitField = m_InBitStream.GetValue(16);
    if (bitField < 0x100)
    {
      len = bitField;
      m_InBitStream.MovePos(16);
    }
    else
    {
      for (len = 0; ((bitField << len) & 0x8000) == 0; len++);
      
      m_InBitStream.MovePos(len + 1);
    }
  }

  AvrLn2 += len;
  AvrLn2 -= AvrLn2 >> 5;

  {
    const Byte *tab;
         if (AvrPlcB >= 0x2900) tab = PosHf2;
    else if (AvrPlcB >= 0x0700) tab = PosHf1;
    else                        tab = PosHf0;
    distancePlace = DecodeNum(tab); // [0, 256]
  }

  AvrPlcB += distancePlace;
  AvrPlcB -= AvrPlcB >> 8;

  distancePlace &= 0xff;
  
  for (;;)
  {
    dist = ChSetB[distancePlace];
    newDistancePlace = NToPlB[dist++ & 0xff]++;
    if (dist & 0xff)
      break;
    CorrHuff(ChSetB,NToPlB);
  }

  ChSetB[distancePlace] = ChSetB[newDistancePlace];
  ChSetB[newDistancePlace] = dist;

  dist = ((dist & 0xff00) >> 1) | ReadBits(7);

  oldAvr3 = AvrLn3;
  
  if (len != 1 && len != 4)
  {
    if (len == 0 && dist <= MaxDist3)
    {
      AvrLn3++;
      AvrLn3 -= AvrLn3 >> 8;
    }
    else if (AvrLn3 > 0)
      AvrLn3--;
  }
  
  len += 3;
  
  if (dist >= MaxDist3)
    len++;
  if (dist <= 256)
    len += 8;
  
  if (oldAvr3 > 0xb0 || (AvrPlc >= 0x2a00 && oldAvr2 < 0x40))
    MaxDist3 = 0x7f00;
  else
    MaxDist3 = 0x2001;
  
  m_RepDists[m_RepDistPtr++] = --dist;
  m_RepDistPtr &= 3;
  LastLength = len;
  LastDist = dist;
  
  return CopyBlock(dist, len);
}


HRESULT CDecoder::HuffDecode()
{
  UInt32 curByte, newBytePlace;
  UInt32 len;
  UInt32 dist;
  unsigned bytePlace;
  {
    const Byte *tab;
    
    if      (AvrPlc >= 0x7600)  tab = PosHf4;
    else if (AvrPlc >= 0x5e00)  tab = PosHf3;
    else if (AvrPlc >= 0x3600)  tab = PosHf2;
    else if (AvrPlc >= 0x0e00)  tab = PosHf1;
    else                        tab = PosHf0;
    
    bytePlace = DecodeNum(tab); // [0, 256]
  }
  
  if (StMode)
  {
    if (bytePlace == 0)
    {
      if (ReadBits(1))
      {
        NumHuf = 0;
        StMode = false;
        return S_OK;
      }
      len = ReadBits(1) + 3;
      dist = DecodeNum(PosHf2);
      dist = (dist << 5) | ReadBits(5);
      if (dist == 0)
        return S_FALSE;
      return CopyBlock(dist - 1, len);
    }
    bytePlace--; // bytePlace is [0, 255]
  }
  else if (NumHuf++ >= 16 && FlagsCnt == 0)
    StMode = true;
  
  bytePlace &= 0xff;
  AvrPlc += bytePlace;
  AvrPlc -= AvrPlc >> 8;
  Nhfb += 16;
  
  if (Nhfb > 0xff)
  {
    Nhfb = 0x90;
    Nlzb >>= 1;
  }

  m_UnpackSize--;
  m_OutWindowStream.PutByte((Byte)(ChSet[bytePlace] >> 8));

  for (;;)
  {
    curByte = ChSet[bytePlace];
    newBytePlace = NToPl[curByte++ & 0xff]++;
    if ((curByte & 0xff) <= 0xa1)
      break;
    CorrHuff(ChSet, NToPl);
  }

  ChSet[bytePlace] = ChSet[newBytePlace];
  ChSet[newBytePlace] = curByte;
  return S_OK;
}


void CDecoder::GetFlagsBuf()
{
  UInt32 flags, newFlagsPlace;
  const UInt32 flagsPlace = DecodeNum(PosHf2); // [0, 256]

  if (flagsPlace >= Z7_ARRAY_SIZE(ChSetC))
    return;

  for (;;)
  {
    flags = ChSetC[flagsPlace];
    FlagBuf = flags >> 8;
    newFlagsPlace = NToPlC[flags++ & 0xff]++;
    if ((flags & 0xff) != 0)
      break;
    CorrHuff(ChSetC, NToPlC);
  }

  ChSetC[flagsPlace] = ChSetC[newFlagsPlace];
  ChSetC[newFlagsPlace] = flags;
}


void CDecoder::CorrHuff(UInt32 *CharSet, UInt32 *NumToPlace)
{
  int i;
  for (i = 7; i >= 0; i--)
    for (unsigned j = 0; j < 32; j++, CharSet++)
      *CharSet = (*CharSet & ~(UInt32)0xff) | (unsigned)i;
  memset(NumToPlace, 0, sizeof(NToPl));
  for (i = 6; i >= 0; i--)
    NumToPlace[i] = (7 - (unsigned)i) * 32;
}



HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo * /* progress */)
{
  if (!inSize || !outSize)
    return E_INVALIDARG;

  if (_isSolid && !_solidAllowed)
    return S_FALSE;

  _solidAllowed = false;

  if (!m_OutWindowStream.Create(kHistorySize))
    return E_OUTOFMEMORY;
  if (!m_InBitStream.Create(1 << 20))
    return E_OUTOFMEMORY;

  m_UnpackSize = *outSize;

  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(_isSolid);
  m_InBitStream.SetStream(inStream);
  m_InBitStream.Init();

  // InitData

  FlagsCnt = 0;
  FlagBuf = 0;
  StMode = false;
  LCount = 0;

  if (!_isSolid)
  {
    AvrPlcB = AvrLn1 = AvrLn2 = AvrLn3 = NumHuf = Buf60 = 0;
    AvrPlc = 0x3500;
    MaxDist3 = 0x2001;
    Nhfb = Nlzb = 0x80;

    {
      // InitStructures
      for (unsigned i = 0; i < kNumRepDists; i++)
        m_RepDists[i] = 0;
      m_RepDistPtr = 0;
      LastLength = 0;
      LastDist = 0;
    }
    
    // InitHuff
    
    for (UInt32 i = 0; i < 256; i++)
    {
      Place[i] = PlaceA[i] = PlaceB[i] = i;
      UInt32 c = (~i + 1) & 0xff;
      PlaceC[i] = c;
      ChSet[i] = ChSetB[i] = i << 8;
      ChSetA[i] = i;
      ChSetC[i] = c << 8;
    }
    memset(NToPl, 0, sizeof(NToPl));
    memset(NToPlB, 0, sizeof(NToPlB));
    memset(NToPlC, 0, sizeof(NToPlC));
    CorrHuff(ChSetB, NToPlB);
  }
   
  if (m_UnpackSize > 0)
  {
    GetFlagsBuf();
    FlagsCnt = 8;
  }

  while (m_UnpackSize != 0)
  {
    if (!StMode)
    {
      if (--FlagsCnt < 0)
      {
        GetFlagsBuf();
        FlagsCnt = 7;
      }
      
      if (FlagBuf & 0x80)
      {
        FlagBuf <<= 1;
        if (Nlzb > Nhfb)
        {
          RINOK(LongLZ())
          continue;
        }
      }
      else
      {
        FlagBuf <<= 1;
        
        if (--FlagsCnt < 0)
        {
          GetFlagsBuf();
          FlagsCnt = 7;
        }

        if ((FlagBuf & 0x80) == 0)
        {
          FlagBuf <<= 1;
          RINOK(ShortLZ())
          continue;
        }
        
        FlagBuf <<= 1;
        
        if (Nlzb <= Nhfb)
        {
          RINOK(LongLZ())
          continue;
        }
      }
    }

    RINOK(HuffDecode())
  }
  
  _solidAllowed = true;
  return m_OutWindowStream.Flush();
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  if (size < 1)
    return E_INVALIDARG;
  _isSolid = ((data[0] & 1) != 0);
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Compress/Rar2Decoder.cpp ================ */
// Rar2Decoder.cpp
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives
 
// amalgamation: header emitted in prologue

#include <stdlib.h>

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NRar2 {

namespace NMultimedia {

#define my_abs(x) (unsigned)abs(x)

Byte CFilter::Decode(int &channelDelta, Byte deltaByte)
{
  D4 = D3;
  D3 = D2;
  D2 = LastDelta - D1;
  D1 = LastDelta;
  const int predictedValue = ((8 * LastChar + K1 * D1 + K2 * D2 + K3 * D3 + K4 * D4 + K5 * channelDelta) >> 3);

  const Byte realValue = (Byte)(predictedValue - deltaByte);
  
  {
    const int i = ((int)(signed char)deltaByte) << 3;

    Dif[0] += my_abs(i);
    Dif[1] += my_abs(i - D1);
    Dif[2] += my_abs(i + D1);
    Dif[3] += my_abs(i - D2);
    Dif[4] += my_abs(i + D2);
    Dif[5] += my_abs(i - D3);
    Dif[6] += my_abs(i + D3);
    Dif[7] += my_abs(i - D4);
    Dif[8] += my_abs(i + D4);
    Dif[9] += my_abs(i - channelDelta);
    Dif[10] += my_abs(i + channelDelta);
  }

  channelDelta = LastDelta = (signed char)(realValue - LastChar);
  LastChar = realValue;

  if (((++ByteCount) & 0x1F) == 0)
  {
    UInt32 minDif = Dif[0];
    UInt32 numMinDif = 0;
    Dif[0] = 0;
    
    for (unsigned i = 1; i < Z7_ARRAY_SIZE(Dif); i++)
    {
      if (Dif[i] < minDif)
      {
        minDif = Dif[i];
        numMinDif = i;
      }
      Dif[i] = 0;
    }
    
    switch (numMinDif)
    {
      case 1: if (K1 >= -16) K1--; break;
      case 2: if (K1 <   16) K1++; break;
      case 3: if (K2 >= -16) K2--; break;
      case 4: if (K2 <   16) K2++; break;
      case 5: if (K3 >= -16) K3--; break;
      case 6: if (K3 <   16) K3++; break;
      case 7: if (K4 >= -16) K4--; break;
      case 8: if (K4 <   16) K4++; break;
      case 9: if (K5 >= -16) K5--; break;
      case 10:if (K5 <   16) K5++; break;
    }
  }
  
  return realValue;
}
}

static const UInt32 kHistorySize = 1 << 20;

// static const UInt32 kWindowReservSize = (1 << 22) + 256;

CDecoder::CDecoder():
  _isSolid(false),
  _solidAllowed(false),
  m_TablesOK(false)
{
}

void CDecoder::InitStructures()
{
  m_MmFilter.Init();
  for (unsigned i = 0; i < kNumReps; i++)
    m_RepDists[i] = 0;
  m_RepDistPtr = 0;
  m_LastLength = 0;
  memset(m_LastLevels, 0, kMaxTableSize);
}

UInt32 CDecoder::ReadBits(unsigned numBits) { return m_InBitStream.ReadBits(numBits); }

#define RIF(x) { if (!(x)) return false; }

static const unsigned kRepBothNumber = 256;
static const unsigned kRepNumber = kRepBothNumber + 1;
static const unsigned kLen2Number = kRepNumber + kNumReps;
static const unsigned kReadTableNumber = kLen2Number + kNumLen2Symbols;
static const unsigned kMatchNumber = kReadTableNumber + 1;

// static const unsigned kDistTableStart = kMainTableSize;
// static const unsigned kLenTableStart = kDistTableStart + kDistTableSize;

static const UInt32 kDistStart   [kDistTableSize] = {0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576,32768U,49152U,65536,98304,131072,196608,262144,327680,393216,458752,524288,589824,655360,720896,786432,851968,917504,983040};
static const Byte kDistDirectBits[kDistTableSize] = {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13,    14,    14,   15,   15,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16,    16};

static const Byte kLen2DistStarts    [kNumLen2Symbols]={0,4,8,16,32,64,128,192};
static const Byte kLen2DistDirectBits[kNumLen2Symbols]={2,2,3, 4, 5, 6,  6,  6};

static const UInt32 kDistLimit2 = 0x101 - 1;
static const UInt32 kDistLimit3 = 0x2000 - 1;
static const UInt32 kDistLimit4 = 0x40000 - 1;

// static const UInt32 kMatchMaxLen = 255 + 2;
// static const UInt32 kMatchMaxLenMax = 255 + 5;


bool CDecoder::ReadTables(void)
{
  m_TablesOK = false;

  const unsigned kLevelTableSize = 19;
  Byte levelLevels[kLevelTableSize];
  Byte lens[kMaxTableSize];
  
  m_AudioMode = (ReadBits(1) == 1);

  if (ReadBits(1) == 0)
    memset(m_LastLevels, 0, kMaxTableSize);
  
  unsigned numLevels;
  
  if (m_AudioMode)
  {
    m_NumChannels = ReadBits(2) + 1;
    if (m_MmFilter.CurrentChannel >= m_NumChannels)
      m_MmFilter.CurrentChannel = 0;
    numLevels = m_NumChannels * k_MM_TableSize;
  }
  else
    numLevels = kHeapTablesSizesSum;
 
  unsigned i;
  for (i = 0; i < kLevelTableSize; i++)
    levelLevels[i] = (Byte)ReadBits(4);
  NHuffman::CDecoder256<kNumHufBits, kLevelTableSize, 6> m_LevelDecoder;
  RIF(m_LevelDecoder.Build(levelLevels, NHuffman::k_BuildMode_Full))
  
  i = 0;
  do
  {
    const unsigned sym = m_LevelDecoder.DecodeFull(&m_InBitStream);
    if (sym < 16)
    {
      lens[i] = (Byte)((sym + m_LastLevels[i]) & 15);
      i++;
    }
#if 0
    else if (sym >= kLevelTableSize)
      return false;
#endif
    else
    {
      unsigned num;
      Byte v;
      if (sym == 16)
      {
        if (i == 0)
          return false;
        num = ReadBits(2) + 3;
        v = lens[(size_t)i - 1];
      }
      else
      {
        num = (sym - 17) * 4;
        num += num + 3 + ReadBits(3 + num);
        v = 0;
      }
      num += i;
      if (num > numLevels)
      {
        // return false;
        num = numLevels; // original unRAR
      }
      do
        lens[i++] = v;
      while (i < num);
    }
  }
  while (i < numLevels);

  if (m_InBitStream.ExtraBitsWereRead())
    return false;

  if (m_AudioMode)
    for (i = 0; i < m_NumChannels; i++)
    {
      RIF(m_MMDecoders[i].Build(&lens[(size_t)i * k_MM_TableSize]))
    }
  else
  {
    RIF(m_MainDecoder.Build(&lens[0]))
    RIF(m_DistDecoder.Build(&lens[kMainTableSize]))
    RIF(m_LenDecoder.Build(&lens[kMainTableSize + kDistTableSize]))
  }
  
  memcpy(m_LastLevels, lens, kMaxTableSize);

  m_TablesOK = true;
  return true;
}


bool CDecoder::ReadLastTables()
{
  // it differs a little from pure RAR sources;
  // UInt64 ttt = m_InBitStream.GetProcessedSize() + 2;
  // + 2 works for: return 0xFF; in CInBuffer::ReadByte.
  if (m_InBitStream.GetProcessedSize() + 7 <= m_PackSize) // test it: probably incorrect;
  // if (m_InBitStream.GetProcessedSize() + 2 <= m_PackSize) // test it: probably incorrect;
  {
    if (m_AudioMode)
    {
      const unsigned symbol = m_MMDecoders[m_MmFilter.CurrentChannel].Decode(&m_InBitStream);
      if (symbol == 256)
        return ReadTables();
      if (symbol >= k_MM_TableSize)
        return false;
    }
    else
    {
      const unsigned sym = m_MainDecoder.Decode(&m_InBitStream);
      if (sym == kReadTableNumber)
        return ReadTables();
      if (sym >= kMainTableSize)
        return false;
    }
  }
  return true;
}


bool CDecoder::DecodeMm(UInt32 pos)
{
  while (pos-- != 0)
  {
    const unsigned symbol = m_MMDecoders[m_MmFilter.CurrentChannel].Decode(&m_InBitStream);
    if (m_InBitStream.ExtraBitsWereRead())
      return false;
    if (symbol >= 256)
      return symbol == 256;
    /*
    Byte byPredict = m_Predictor.Predict();
    Byte byReal = (Byte)(byPredict - (Byte)symbol);
    m_Predictor.Update(byReal, byPredict);
    */
    const Byte byReal = m_MmFilter.Decode((Byte)symbol);
    m_OutWindowStream.PutByte(byReal);
    if (++m_MmFilter.CurrentChannel == m_NumChannels)
      m_MmFilter.CurrentChannel = 0;
  }
  return true;
}


typedef unsigned CLenType;

static inline CLenType SlotToLen(CBitDecoder &_bitStream, CLenType slot)
{
  const unsigned numBits = ((unsigned)slot >> 2) - 1;
  return ((4 | (slot & 3)) << numBits) + (CLenType)_bitStream.ReadBits(numBits);
}

bool CDecoder::DecodeLz(Int32 pos)
{
  while (pos > 0)
  {
    unsigned sym = m_MainDecoder.Decode(&m_InBitStream);
    if (m_InBitStream.ExtraBitsWereRead())
      return false;
    UInt32 len, distance;
    if (sym < 256)
    {
      m_OutWindowStream.PutByte(Byte(sym));
      pos--;
      continue;
    }
    else if (sym >= kMatchNumber)
    {
      if (sym >= kMainTableSize)
        return false;
      len = sym - kMatchNumber;
      if (len >= 8)
        len = SlotToLen(m_InBitStream, len);
      len += 3;

      sym = m_DistDecoder.Decode(&m_InBitStream);
      if (sym >= kDistTableSize)
        return false;
      distance = kDistStart[sym] + m_InBitStream.ReadBits(kDistDirectBits[sym]);
      if (distance >= kDistLimit3)
      {
        len += 2 - ((distance - kDistLimit4) >> 31);
        // len++;
        // if (distance >= kDistLimit4)
        //  len++;
      }
    }
    else if (sym == kRepBothNumber)
    {
      len = m_LastLength;
      if (len == 0)
        return false;
      distance = m_RepDists[(m_RepDistPtr + 4 - 1) & 3];
    }
    else if (sym < kLen2Number)
    {
      distance = m_RepDists[(m_RepDistPtr - (sym - kRepNumber + 1)) & 3];
      len = m_LenDecoder.Decode(&m_InBitStream);
      if (len >= kLenTableSize)
        return false;
      if (len >= 8)
        len = SlotToLen(m_InBitStream, len);
      len += 2;

      if (distance >= kDistLimit2)
      {
        len++;
        if (distance >= kDistLimit3)
        {
          len += 2 - ((distance - kDistLimit4) >> 31);
          // len++;
          // if (distance >= kDistLimit4)
          //   len++;
        }
      }
    }
    else if (sym < kReadTableNumber)
    {
      sym -= kLen2Number;
      distance = kLen2DistStarts[sym] +
        m_InBitStream.ReadBits(kLen2DistDirectBits[sym]);
      len = 2;
    }
    else // (sym == kReadTableNumber)
      return true;

    m_RepDists[m_RepDistPtr++ & 3] = distance;
    m_LastLength = len;
    if (!m_OutWindowStream.CopyBlock(distance, len))
      return false;
    pos -= len;
  }
  return true;
}

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  if (!inSize || !outSize)
    return E_INVALIDARG;

  if (_isSolid && !_solidAllowed)
    return S_FALSE;
  _solidAllowed = false;

  if (!m_OutWindowStream.Create(kHistorySize))
    return E_OUTOFMEMORY;
  if (!m_InBitStream.Create(1 << 20))
    return E_OUTOFMEMORY;

  m_PackSize = *inSize;

  UInt64 pos = 0, unPackSize = *outSize;
  
  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(_isSolid);
  m_InBitStream.SetStream(inStream);
  m_InBitStream.Init();

  // CCoderReleaser coderReleaser(this);
  if (!_isSolid)
  {
    InitStructures();
    if (unPackSize == 0)
    {
      if (m_InBitStream.GetProcessedSize() + 2 <= m_PackSize) // test it: probably incorrect;
        if (!ReadTables())
          return S_FALSE;
      _solidAllowed = true;
      return S_OK;
    }
    ReadTables();
  }

  if (!m_TablesOK)
    return S_FALSE;

  const UInt64 startPos = m_OutWindowStream.GetProcessedSize();
  while (pos < unPackSize)
  {
    UInt32 blockSize = 1 << 20;
    if (blockSize > unPackSize - pos)
      blockSize = (UInt32)(unPackSize - pos);
    UInt64 blockStartPos = m_OutWindowStream.GetProcessedSize();
    if (m_AudioMode)
    {
      if (!DecodeMm(blockSize))
        return S_FALSE;
    }
    else
    {
      if (!DecodeLz((Int32)blockSize))
        return S_FALSE;
    }

    if (m_InBitStream.ExtraBitsWereRead())
      return S_FALSE;

    const UInt64 globalPos = m_OutWindowStream.GetProcessedSize();
    pos = globalPos - blockStartPos;
    if (pos < blockSize)
      if (!ReadTables())
        return S_FALSE;
    pos = globalPos - startPos;
    if (progress)
    {
      const UInt64 packSize = m_InBitStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &pos))
    }
  }
  if (pos > unPackSize)
    return S_FALSE;

  if (!ReadLastTables())
    return S_FALSE;

  _solidAllowed = true;

  return m_OutWindowStream.Flush();
}

Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  if (size < 1)
    return E_INVALIDARG;
  _isSolid = ((data[0] & 1) != 0);
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Compress/Rar3Decoder.cpp ================ */
// Rar3Decoder.cpp
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

/* This code uses Carryless rangecoder (1999): Dmitry Subbotin : Public domain */
 
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NRar3 {

static const UInt32 kNumAlignReps = 15;

static const unsigned kSymbolReadTable = 256;
static const unsigned kSymbolRep = 259;

static const Byte kDistDirectBits[kDistTableSize] =
  {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  18,18,18,18,18,18,18,18,18,18,18,18};

static const Byte kLen2DistStarts[kNumLen2Symbols] = {0,4,8,16,32,64,128,192};
static const Byte kLen2DistDirectBits[kNumLen2Symbols] = {2,2,3, 4, 5, 6,  6,  6};

static const UInt32 kDistLimit3 = 0x2000 - 2;
static const UInt32 kDistLimit4 = 0x40000 - 2;

static const UInt32 kNormalMatchMinLen = 3;

static const UInt32 kVmDataSizeMax = 1 << 16;
static const UInt32 kVmCodeSizeMax = 1 << 16;

extern "C" {

static Byte Wrap_ReadByte(IByteInPtr pp) throw()
{
  CByteIn *p = Z7_CONTAINER_FROM_VTBL_CLS(pp, CByteIn, IByteIn_obj);
  return p->BitDecoder.Stream.ReadByte();
}

static Byte Wrap_ReadBits8(IByteInPtr pp) throw()
{
  CByteIn *p = Z7_CONTAINER_FROM_VTBL_CLS(pp, CByteIn, IByteIn_obj);
  return (Byte)p->BitDecoder.ReadByteFromAligned();
}

}


CDecoder::CDecoder():
  _isSolid(false),
  _solidAllowed(false),
  _window(NULL),
  _winPos(0),
  _wrPtr(0),
  _lzSize(0),
  _writtenFileSize(0),
  _vmData(NULL),
  _vmCode(NULL)
{
  Ppmd7_Construct(&_ppmd);
  
  UInt32 start = 0;
  for (UInt32 i = 0; i < kDistTableSize; i++)
  {
    kDistStart[i] = start;
    start += ((UInt32)1 << kDistDirectBits[i]);
  }
}

CDecoder::~CDecoder()
{
  InitFilters();
  ::MidFree(_vmData);
  ::MidFree(_window);
  Ppmd7_Free(&_ppmd, &g_BigAlloc);
}

HRESULT CDecoder::WriteDataToStream(const Byte *data, UInt32 size)
{
  return WriteStream(_outStream, data, size);
}

HRESULT CDecoder::WriteData(const Byte *data, UInt32 size)
{
  HRESULT res = S_OK;
  if (_writtenFileSize < _unpackSize)
  {
    UInt32 curSize = size;
    UInt64 remain = _unpackSize - _writtenFileSize;
    if (remain < curSize)
      curSize = (UInt32)remain;
    res = WriteDataToStream(data, curSize);
  }
  _writtenFileSize += size;
  return res;
}

HRESULT CDecoder::WriteArea(UInt32 startPtr, UInt32 endPtr)
{
  if (startPtr <= endPtr)
    return WriteData(_window + startPtr, endPtr - startPtr);
  RINOK(WriteData(_window + startPtr, kWindowSize - startPtr))
  return WriteData(_window, endPtr);
}

void CDecoder::ExecuteFilter(unsigned tempFilterIndex, NVm::CBlockRef &outBlockRef)
{
  CTempFilter *tempFilter = _tempFilters[tempFilterIndex];
  tempFilter->InitR[6] = (UInt32)_writtenFileSize;
  NVm::SetValue32(&tempFilter->GlobalData[0x24], (UInt32)_writtenFileSize);
  NVm::SetValue32(&tempFilter->GlobalData[0x28], (UInt32)(_writtenFileSize >> 32));
  CFilter *filter = _filters[tempFilter->FilterIndex];
  if (!filter->IsSupported)
    _unsupportedFilter = true;
  if (!_vm.Execute(filter, tempFilter, outBlockRef, filter->GlobalData))
    _unsupportedFilter = true;
  delete tempFilter;
  _tempFilters[tempFilterIndex] = NULL;
  _numEmptyTempFilters++;
}

HRESULT CDecoder::WriteBuf()
{
  UInt32 writtenBorder = _wrPtr;
  UInt32 writeSize = (_winPos - writtenBorder) & kWindowMask;
  FOR_VECTOR (i, _tempFilters)
  {
    CTempFilter *filter = _tempFilters[i];
    if (!filter)
      continue;
    if (filter->NextWindow)
    {
      filter->NextWindow = false;
      continue;
    }
    UInt32 blockStart = filter->BlockStart;
    UInt32 blockSize = filter->BlockSize;
    if (((blockStart - writtenBorder) & kWindowMask) < writeSize)
    {
      if (writtenBorder != blockStart)
      {
        RINOK(WriteArea(writtenBorder, blockStart))
        writtenBorder = blockStart;
        writeSize = (_winPos - writtenBorder) & kWindowMask;
      }
      if (blockSize <= writeSize)
      {
        UInt32 blockEnd = (blockStart + blockSize) & kWindowMask;
        if (blockStart < blockEnd || blockEnd == 0)
          _vm.SetMemory(0, _window + blockStart, blockSize);
        else
        {
          UInt32 tailSize = kWindowSize - blockStart;
          _vm.SetMemory(0, _window + blockStart, tailSize);
          _vm.SetMemory(tailSize, _window, blockEnd);
        }
        NVm::CBlockRef outBlockRef;
        ExecuteFilter(i, outBlockRef);
        while (i + 1 < _tempFilters.Size())
        {
          CTempFilter *nextFilter = _tempFilters[i + 1];
          if (!nextFilter
              || nextFilter->BlockStart != blockStart
              || nextFilter->BlockSize != outBlockRef.Size
              || nextFilter->NextWindow)
            break;
          _vm.SetMemory(0, _vm.GetDataPointer(outBlockRef.Offset), outBlockRef.Size);
          ExecuteFilter(++i, outBlockRef);
        }
        WriteDataToStream(_vm.GetDataPointer(outBlockRef.Offset), outBlockRef.Size);
        _writtenFileSize += outBlockRef.Size;
        writtenBorder = blockEnd;
        writeSize = (_winPos - writtenBorder) & kWindowMask;
      }
      else
      {
        for (unsigned j = i; j < _tempFilters.Size(); j++)
        {
          CTempFilter *filter2 = _tempFilters[j];
          if (filter2 && filter2->NextWindow)
            filter2->NextWindow = false;
        }
        _wrPtr = writtenBorder;
        return S_OK; // check it
      }
    }
  }
      
  _wrPtr = _winPos;
  return WriteArea(writtenBorder, _winPos);
}

void CDecoder::InitFilters()
{
  _lastFilter = 0;
  _numEmptyTempFilters = 0;
  unsigned i;
  for (i = 0; i < _tempFilters.Size(); i++)
    delete _tempFilters[i];
  _tempFilters.Clear();
  for (i = 0; i < _filters.Size(); i++)
    delete _filters[i];
  _filters.Clear();
}

static const unsigned MAX_UNPACK_FILTERS = 8192;

bool CDecoder::AddVmCode(UInt32 firstByte, UInt32 codeSize)
{
  CMemBitDecoder inp;
  inp.Init(_vmData, codeSize);

  UInt32 filterIndex;
  
  if (firstByte & 0x80)
  {
    filterIndex = inp.ReadEncodedUInt32();
    if (filterIndex == 0)
      InitFilters();
    else
      filterIndex--;
  }
  else
    filterIndex = _lastFilter;
  
  if (filterIndex > (UInt32)_filters.Size())
    return false;
  _lastFilter = filterIndex;
  bool newFilter = (filterIndex == (UInt32)_filters.Size());

  CFilter *filter;
  if (newFilter)
  {
    // check if too many filters
    if (filterIndex > MAX_UNPACK_FILTERS)
      return false;
    filter = new CFilter;
    _filters.Add(filter);
  }
  else
  {
    filter = _filters[filterIndex];
    filter->ExecCount++;
  }

  if (_numEmptyTempFilters != 0)
  {
    const unsigned num = _tempFilters.Size();
    CTempFilter **tempFilters = _tempFilters.NonConstData();
    
    unsigned w = 0;
    for (unsigned i = 0; i < num; i++)
    {
      CTempFilter *tf = tempFilters[i];
      if (tf)
        tempFilters[w++] = tf;
    }

    _tempFilters.DeleteFrom(w);
    _numEmptyTempFilters = 0;
  }
  
  if (_tempFilters.Size() > MAX_UNPACK_FILTERS)
    return false;
  CTempFilter *tempFilter = new CTempFilter;
  _tempFilters.Add(tempFilter);
  tempFilter->FilterIndex = filterIndex;
 
  UInt32 blockStart = inp.ReadEncodedUInt32();
  if (firstByte & 0x40)
    blockStart += 258;
  tempFilter->BlockStart = (blockStart + _winPos) & kWindowMask;
  if (firstByte & 0x20)
    filter->BlockSize = inp.ReadEncodedUInt32();
  tempFilter->BlockSize = filter->BlockSize;
  tempFilter->NextWindow = _wrPtr != _winPos && ((_wrPtr - _winPos) & kWindowMask) <= blockStart;

  memset(tempFilter->InitR, 0, sizeof(tempFilter->InitR));
  tempFilter->InitR[3] = NVm::kGlobalOffset;
  tempFilter->InitR[4] = tempFilter->BlockSize;
  tempFilter->InitR[5] = filter->ExecCount;
  if (firstByte & 0x10)
  {
    UInt32 initMask = inp.ReadBits(NVm::kNumGpRegs);
    for (unsigned i = 0; i < NVm::kNumGpRegs; i++)
      if (initMask & (1 << i))
        tempFilter->InitR[i] = inp.ReadEncodedUInt32();
  }

  bool isOK = true;
  if (newFilter)
  {
    UInt32 vmCodeSize = inp.ReadEncodedUInt32();
    if (vmCodeSize >= kVmCodeSizeMax || vmCodeSize == 0)
      return false;
    for (UInt32 i = 0; i < vmCodeSize; i++)
      _vmCode[i] = (Byte)inp.ReadBits(8);
    isOK = filter->PrepareProgram(_vmCode, vmCodeSize);
  }

  {
    Byte *globalData = &tempFilter->GlobalData[0];
    for (unsigned i = 0; i < NVm::kNumGpRegs; i++)
      NVm::SetValue32(&globalData[i * 4], tempFilter->InitR[i]);
    NVm::SetValue32(&globalData[NVm::NGlobalOffset::kBlockSize], tempFilter->BlockSize);
    NVm::SetValue32(&globalData[NVm::NGlobalOffset::kBlockPos], 0); // It was commented. why?
    NVm::SetValue32(&globalData[NVm::NGlobalOffset::kExecCount], filter->ExecCount);
  }

  if (firstByte & 8)
  {
    UInt32 dataSize = inp.ReadEncodedUInt32();
    if (dataSize > NVm::kGlobalSize - NVm::kFixedGlobalSize)
      return false;
    CRecordVector<Byte> &globalData = tempFilter->GlobalData;
    unsigned requiredSize = (unsigned)(dataSize + NVm::kFixedGlobalSize);
    if (globalData.Size() < requiredSize)
      globalData.ChangeSize_KeepData(requiredSize);
    Byte *dest = &globalData[NVm::kFixedGlobalSize];
    for (UInt32 i = 0; i < dataSize; i++)
      dest[i] = (Byte)inp.ReadBits(8);
  }
  
  return isOK;
}

bool CDecoder::ReadVmCodeLZ()
{
  UInt32 firstByte = ReadBits(8);
  UInt32 len = (firstByte & 7) + 1;
  if (len == 7)
    len = ReadBits(8) + 7;
  else if (len == 8)
    len = ReadBits(16);
  if (len > kVmDataSizeMax)
    return false;
  for (UInt32 i = 0; i < len; i++)
    _vmData[i] = (Byte)ReadBits(8);
  return AddVmCode(firstByte, len);
}


// int CDecoder::DecodePpmSymbol() { return Ppmd7a_DecodeSymbol(&_ppmd); }
#define DecodePpmSymbol() Ppmd7a_DecodeSymbol(&_ppmd)


bool CDecoder::ReadVmCodePPM()
{
  const int firstByte = DecodePpmSymbol();
  if (firstByte < 0)
    return false;
  UInt32 len = (firstByte & 7) + 1;
  if (len == 7)
  {
    const int b1 = DecodePpmSymbol();
    if (b1 < 0)
      return false;
    len = (unsigned)b1 + 7;
  }
  else if (len == 8)
  {
    const int b1 = DecodePpmSymbol();
    if (b1 < 0)
      return false;
    const int b2 = DecodePpmSymbol();
    if (b2 < 0)
      return false;
    len = (unsigned)b1 * 256 + (unsigned)b2;
  }
  if (len > kVmDataSizeMax)
    return false;
  if (InputEofError_Fast())
    return false;
  for (UInt32 i = 0; i < len; i++)
  {
    const int b = DecodePpmSymbol();
    if (b < 0)
      return false;
    _vmData[i] = (Byte)b;
  }
  return AddVmCode((unsigned)firstByte, len);
}

#define RIF(x) { if (!(x)) return S_FALSE; }

UInt32 CDecoder::ReadBits(unsigned numBits) { return m_InBitStream.BitDecoder.ReadBits(numBits); }

// ---------- PPM ----------

HRESULT CDecoder::InitPPM()
{
  unsigned maxOrder = (unsigned)ReadBits(7);

  const bool reset = ((maxOrder & 0x20) != 0);
  UInt32 maxMB = 0;
  if (reset)
    maxMB = (Byte)Wrap_ReadBits8(&m_InBitStream.IByteIn_obj);
  else
  {
    if (PpmError || !Ppmd7_WasAllocated(&_ppmd))
      return S_FALSE;
  }
  if (maxOrder & 0x40)
    PpmEscChar = (Byte)Wrap_ReadBits8(&m_InBitStream.IByteIn_obj);

  _ppmd.rc.dec.Stream = &m_InBitStream.IByteIn_obj;
  m_InBitStream.IByteIn_obj.Read = Wrap_ReadBits8;

  Ppmd7a_RangeDec_Init(&_ppmd.rc.dec);

  m_InBitStream.IByteIn_obj.Read = Wrap_ReadByte;

  if (reset)
  {
    PpmError = true;
    maxOrder = (maxOrder & 0x1F) + 1;
    if (maxOrder > 16)
      maxOrder = 16 + (maxOrder - 16) * 3;
    if (maxOrder == 1)
    {
      Ppmd7_Free(&_ppmd, &g_BigAlloc);
      return S_FALSE;
    }
    if (!Ppmd7_Alloc(&_ppmd, (maxMB + 1) << 20, &g_BigAlloc))
      return E_OUTOFMEMORY;
    Ppmd7_Init(&_ppmd, maxOrder);
    PpmError = false;
  }
  return S_OK;
}


HRESULT CDecoder::DecodePPM(Int32 num, bool &keepDecompressing)
{
  keepDecompressing = false;
  if (PpmError)
    return S_FALSE;
  do
  {
    if (((_wrPtr - _winPos) & kWindowMask) < 260 && _wrPtr != _winPos)
    {
      RINOK(WriteBuf())
      if (_writtenFileSize > _unpackSize)
      {
        keepDecompressing = false;
        return S_OK;
      }
    }
    if (InputEofError_Fast())
      return false;
    const int c = DecodePpmSymbol();
    if (c < 0)
    {
      PpmError = true;
      return S_FALSE;
    }
    if (c == PpmEscChar)
    {
      const int nextCh = DecodePpmSymbol();
      if (nextCh < 0)
      {
        PpmError = true;
        return S_FALSE;
      }
      if (nextCh == 0)
        return ReadTables(keepDecompressing);
      if (nextCh == 2 || nextCh == -1)
        return S_OK;
      if (nextCh == 3)
      {
        if (!ReadVmCodePPM())
        {
          PpmError = true;
          return S_FALSE;
        }
        continue;
      }
      if (nextCh == 4 || nextCh == 5)
      {
        UInt32 dist = 0;
        UInt32 len = 4;
        if (nextCh == 4)
        {
          for (int i = 0; i < 3; i++)
          {
            const int c2 = DecodePpmSymbol();
            if (c2 < 0)
            {
              PpmError = true;
              return S_FALSE;
            }
            dist = (dist << 8) + (Byte)c2;
          }
          dist++;
          len += 28;
        }
        const int c2 = DecodePpmSymbol();
        if (c2 < 0)
        {
          PpmError = true;
          return S_FALSE;
        }
        len += (unsigned)c2;
        if (dist >= _lzSize)
          return S_FALSE;
        CopyBlock(dist, len);
        num -= (Int32)len;
        continue;
      }
    }
    PutByte((Byte)c);
    num--;
  }
  while (num >= 0);
  keepDecompressing = true;
  return S_OK;
}

// ---------- LZ ----------

HRESULT CDecoder::ReadTables(bool &keepDecompressing)
{
  keepDecompressing = true;
  m_InBitStream.BitDecoder.AlignToByte();
  if (ReadBits(1) != 0)
  {
    _lzMode = false;
    return InitPPM();
  }

  TablesRead = false;
  TablesOK = false;

  _lzMode = true;
  PrevAlignBits = 0;
  PrevAlignCount = 0;

  const unsigned kLevelTableSize = 20;
  Byte levelLevels[kLevelTableSize];
  Byte lens[kTablesSizesSum];

  if (ReadBits(1) == 0)
    memset(m_LastLevels, 0, kTablesSizesSum);

  unsigned i;

  for (i = 0; i < kLevelTableSize; i++)
  {
    const UInt32 len = ReadBits(4);
    if (len == 15)
    {
      UInt32 zeroCount = ReadBits(4);
      if (zeroCount != 0)
      {
        zeroCount += 2;
        while (zeroCount-- > 0 && i < kLevelTableSize)
          levelLevels[i++]=0;
        i--;
        continue;
      }
    }
    levelLevels[i] = (Byte)len;
  }
  
  NHuffman::CDecoder256<kNumHuffmanBits, kLevelTableSize, 6> m_LevelDecoder;
  RIF(m_LevelDecoder.Build(levelLevels, NHuffman::k_BuildMode_Full))
  
  i = 0;
  
  do
  {
    const unsigned sym = m_LevelDecoder.DecodeFull(&m_InBitStream.BitDecoder);
    if (sym < 16)
    {
      lens[i] = Byte((sym + m_LastLevels[i]) & 15);
      i++;
    }
#if 0
    else if (sym > kLevelTableSize)
      return S_FALSE;
#endif
    else
    {
      unsigned num = ((sym /* - 16 */) & 1) * 4;
      num += num + 3 + (unsigned)ReadBits(num + 3);
      num += i;
      if (num > kTablesSizesSum)
        num = kTablesSizesSum;
      Byte v = 0;
      if (sym < 16 + 2)
      {
        if (i == 0)
          return S_FALSE;
        v = lens[(size_t)i - 1];
      }
      do
        lens[i++] = v;
      while (i < num);
    }
  }
  while (i < kTablesSizesSum);

  if (InputEofError())
    return S_FALSE;

  TablesRead = true;

  // original code has check here:
  /*
  if (InAddr > ReadTop)
  {
    keepDecompressing = false;
    return true;
  }
  */

  RIF(m_MainDecoder.Build(&lens[0]))
  RIF(m_DistDecoder.Build(&lens[kMainTableSize]))
  RIF(m_AlignDecoder.Build(&lens[kMainTableSize + kDistTableSize]))
  RIF(m_LenDecoder.Build(&lens[kMainTableSize + kDistTableSize + kAlignTableSize]))

  memcpy(m_LastLevels, lens, kTablesSizesSum);

  TablesOK = true;

  return S_OK;
}

/*
class CCoderReleaser
{
  CDecoder *m_Coder;
public:
  CCoderReleaser(CDecoder *coder): m_Coder(coder) {}
  ~CCoderReleaser()
  {
    m_Coder->ReleaseStreams();
  }
};
*/

HRESULT CDecoder::ReadEndOfBlock(bool &keepDecompressing)
{
  if (ReadBits(1) == 0)
  {
    // new file
    keepDecompressing = false;
    TablesRead = (ReadBits(1) == 0);
    return S_OK;
  }
  TablesRead = false;
  return ReadTables(keepDecompressing);
}


HRESULT CDecoder::DecodeLZ(bool &keepDecompressing)
{
  UInt32 rep0 = _reps[0];
  UInt32 rep1 = _reps[1];
  UInt32 rep2 = _reps[2];
  UInt32 rep3 = _reps[3];
  UInt32 len = _lastLength;
  for (;;)
  {
    if (((_wrPtr - _winPos) & kWindowMask) < 260 && _wrPtr != _winPos)
    {
      RINOK(WriteBuf())
      if (_writtenFileSize > _unpackSize)
      {
        keepDecompressing = false;
        return S_OK;
      }
    }
    
    if (InputEofError_Fast())
      return S_FALSE;

    unsigned sym = m_MainDecoder.Decode(&m_InBitStream.BitDecoder);
    if (sym < 256)
    {
      PutByte((Byte)sym);
      continue;
    }
    else if (sym == kSymbolReadTable)
    {
      RINOK(ReadEndOfBlock(keepDecompressing))
      break;
    }
    else if (sym == 257)
    {
      if (!ReadVmCodeLZ())
        return S_FALSE;
      continue;
    }
    else if (sym == 258)
    {
      if (len == 0)
        return S_FALSE;
    }
    else if (sym < kSymbolRep + 4)
    {
      if (sym != kSymbolRep)
      {
        UInt32 dist;
        if (sym == kSymbolRep + 1)
          dist = rep1;
        else
        {
          if (sym == kSymbolRep + 2)
            dist = rep2;
          else
          {
            dist = rep3;
            rep3 = rep2;
          }
          rep2 = rep1;
        }
        rep1 = rep0;
        rep0 = dist;
      }

      const unsigned sym2 = m_LenDecoder.Decode(&m_InBitStream.BitDecoder);
      if (sym2 >= kLenTableSize)
        return S_FALSE;
      len = 2 + sym2;
      if (sym2 >= 8)
      {
        const unsigned num = (sym2 >> 2) - 1;
        len = 2 + (UInt32)((4 + (sym2 & 3)) << num) + m_InBitStream.BitDecoder.ReadBits_upto8(num);
      }
    }
    else
    {
      rep3 = rep2;
      rep2 = rep1;
      rep1 = rep0;
      if (sym < 271)
      {
        sym -= 263;
        rep0 = kLen2DistStarts[sym] + m_InBitStream.BitDecoder.ReadBits_upto8(kLen2DistDirectBits[sym]);
        len = 2;
      }
      else if (sym < 299)
      {
        sym -= 271;
        len = kNormalMatchMinLen + sym;
        if (sym >= 8)
        {
          const unsigned num = (sym >> 2) - 1;
          len = kNormalMatchMinLen + (UInt32)((4 + (sym & 3)) << num) + m_InBitStream.BitDecoder.ReadBits_upto8(num);
        }
        const unsigned sym2 = m_DistDecoder.Decode(&m_InBitStream.BitDecoder);
        if (sym2 >= kDistTableSize)
          return S_FALSE;
        rep0 = kDistStart[sym2];
        unsigned numBits = kDistDirectBits[sym2];
        if (sym2 >= (kNumAlignBits * 2) + 2)
        {
          if (numBits > kNumAlignBits)
            rep0 += (m_InBitStream.BitDecoder.ReadBits(numBits - kNumAlignBits) << kNumAlignBits);
          if (PrevAlignCount > 0)
          {
            PrevAlignCount--;
            rep0 += PrevAlignBits;
          }
          else
          {
            const unsigned sym3 = m_AlignDecoder.Decode(&m_InBitStream.BitDecoder);
            if (sym3 < (1 << kNumAlignBits))
            {
              rep0 += sym3;
              PrevAlignBits = sym3;
            }
            else if (sym3 == (1 << kNumAlignBits))
            {
              PrevAlignCount = kNumAlignReps;
              rep0 += PrevAlignBits;
            }
            else
              return S_FALSE;
          }
        }
        else
          rep0 += m_InBitStream.BitDecoder.ReadBits_upto8(numBits);
        len += ((UInt32)(kDistLimit4 - rep0) >> 31) + ((UInt32)(kDistLimit3 - rep0) >> 31);
      }
      else
        return S_FALSE;
    }
    if (rep0 >= _lzSize)
      return S_FALSE;
    CopyBlock(rep0, len);
  }
  _reps[0] = rep0;
  _reps[1] = rep1;
  _reps[2] = rep2;
  _reps[3] = rep3;
  _lastLength = len;

  return S_OK;
}


HRESULT CDecoder::CodeReal(ICompressProgressInfo *progress)
{
  _writtenFileSize = 0;
  _unsupportedFilter = false;
  
  if (!_isSolid)
  {
    _lzSize = 0;
    _winPos = 0;
    _wrPtr = 0;
    for (unsigned i = 0; i < kNumReps; i++)
      _reps[i] = 0;
    _lastLength = 0;
    memset(m_LastLevels, 0, kTablesSizesSum);
    TablesRead = false;
    PpmEscChar = 2;
    PpmError = true;
    InitFilters();
    // _errorMode = false;
  }

  /*
  if (_errorMode)
    return S_FALSE;
  */

  if (!_isSolid || !TablesRead)
  {
    bool keepDecompressing;
    RINOK(ReadTables(keepDecompressing))
    if (!keepDecompressing)
    {
      _solidAllowed = true;
      return S_OK;
    }
  }

  for (;;)
  {
    bool keepDecompressing;
    if (_lzMode)
    {
      if (!TablesOK)
        return S_FALSE;
      RINOK(DecodeLZ(keepDecompressing))
    }
    else
    {
      RINOK(DecodePPM(1 << 18, keepDecompressing))
    }

    if (InputEofError())
      return S_FALSE;

    const UInt64 packSize = m_InBitStream.BitDecoder.GetProcessedSize();
    RINOK(progress->SetRatioInfo(&packSize, &_writtenFileSize))
    if (!keepDecompressing)
      break;
  }

  _solidAllowed = true;

  RINOK(WriteBuf())
  const UInt64 packSize = m_InBitStream.BitDecoder.GetProcessedSize();
  RINOK(progress->SetRatioInfo(&packSize, &_writtenFileSize))
  if (_writtenFileSize < _unpackSize)
    return S_FALSE;

  if (_unsupportedFilter)
    return E_NOTIMPL;

  return S_OK;
}

Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try
  {
    if (!inSize)
      return E_INVALIDARG;

    if (_isSolid && !_solidAllowed)
      return S_FALSE;
    _solidAllowed = false;

    if (!_vmData)
    {
      _vmData = (Byte *)::MidAlloc(kVmDataSizeMax + kVmCodeSizeMax);
      if (!_vmData)
        return E_OUTOFMEMORY;
      _vmCode = _vmData + kVmDataSizeMax;
    }
    
    if (!_window)
    {
      _window = (Byte *)::MidAlloc(kWindowSize);
      if (!_window)
        return E_OUTOFMEMORY;
    }
    if (!m_InBitStream.BitDecoder.Create(1 << 20))
      return E_OUTOFMEMORY;
    if (!_vm.Create())
      return E_OUTOFMEMORY;

    
    m_InBitStream.BitDecoder.SetStream(inStream);
    m_InBitStream.BitDecoder.Init();
    _outStream = outStream;
   
    // CCoderReleaser coderReleaser(this);
    _unpackSize = outSize ? *outSize : (UInt64)(Int64)-1;
    return CodeReal(progress);
  }
  catch(const CInBufferException &e)  { /* _errorMode = true; */ return e.ErrorCode; }
  catch(...) { /* _errorMode = true; */ return S_FALSE; }
  // CNewException is possible here. But probably CNewException is caused
  // by error in data stream.
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  if (size < 1)
    return E_INVALIDARG;
  _isSolid = ((data[0] & 1) != 0);
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Compress/Rar3Vm.cpp ================ */
// Rar3Vm.cpp
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

/*
Note:
  Due to performance considerations Rar VM may set Flags C incorrectly
  for some operands (SHL x, 0, ... ).
  Check implementation of concrete VM command
  to see if it sets flags right.
*/

// amalgamation: header emitted in prologue

#include <stdlib.h>

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NRar3 {

UInt32 CMemBitDecoder::ReadBits(unsigned numBits)
{
  UInt32 res = 0;
  for (;;)
  {
    const unsigned b = _bitPos < _bitSize ? (unsigned)_data[_bitPos >> 3] : 0;
    const unsigned avail = (unsigned)(8 - (_bitPos & 7));
    if (numBits <= avail)
    {
      _bitPos += numBits;
      return res | ((b >> (avail - numBits)) & ((1 << numBits) - 1));
    }
    numBits -= avail;
    res |= (UInt32)(b & ((1 << avail) - 1)) << numBits;
    _bitPos += avail;
  }
}

UInt32 CMemBitDecoder::ReadBit() { return ReadBits(1); }

UInt32 CMemBitDecoder::ReadEncodedUInt32()
{
  const unsigned v = (unsigned)ReadBits(2);
  UInt32 res = ReadBits(4u << v);
  if (v == 1 && res < 16)
    res = 0xFFFFFF00 | (res << 4) | ReadBits(4);
  return res;
}

namespace NVm {

static const UInt32 kStackRegIndex = kNumRegs - 1;

#ifdef Z7_RARVM_VM_ENABLE

#if   defined(Z7_GCC_VERSION)   && (Z7_GCC_VERSION   >= 40400) \
   || defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30000)
// enumeration values not explicitly handled in switch
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif

static const UInt32 FLAG_C = 1;
static const UInt32 FLAG_Z = 2;
static const UInt32 FLAG_S = 0x80000000;

static const Byte CF_OP0 = 0;
static const Byte CF_OP1 = 1;
static const Byte CF_OP2 = 2;
static const Byte CF_OPMASK = 3;
static const Byte CF_BYTEMODE = 4;
static const Byte CF_JUMP = 8;
static const Byte CF_PROC = 16;
static const Byte CF_USEFLAGS = 32;
static const Byte CF_CHFLAGS = 64;

static const Byte kCmdFlags[]=
{
  /* CMD_MOV   */ CF_OP2 | CF_BYTEMODE,
  /* CMD_CMP   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_ADD   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_SUB   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_JZ    */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
  /* CMD_JNZ   */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
  /* CMD_INC   */ CF_OP1 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_DEC   */ CF_OP1 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_JMP   */ CF_OP1 | CF_JUMP,
  /* CMD_XOR   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_AND   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_OR    */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_TEST  */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_JS    */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
  /* CMD_JNS   */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
  /* CMD_JB    */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
  /* CMD_JBE   */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
  /* CMD_JA    */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
  /* CMD_JAE   */ CF_OP1 | CF_JUMP | CF_USEFLAGS,
  /* CMD_PUSH  */ CF_OP1,
  /* CMD_POP   */ CF_OP1,
  /* CMD_CALL  */ CF_OP1 | CF_PROC,
  /* CMD_RET   */ CF_OP0 | CF_PROC,
  /* CMD_NOT   */ CF_OP1 | CF_BYTEMODE,
  /* CMD_SHL   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_SHR   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_SAR   */ CF_OP2 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_NEG   */ CF_OP1 | CF_BYTEMODE | CF_CHFLAGS,
  /* CMD_PUSHA */ CF_OP0,
  /* CMD_POPA  */ CF_OP0,
  /* CMD_PUSHF */ CF_OP0 | CF_USEFLAGS,
  /* CMD_POPF  */ CF_OP0 | CF_CHFLAGS,
  /* CMD_MOVZX */ CF_OP2,
  /* CMD_MOVSX */ CF_OP2,
  /* CMD_XCHG  */ CF_OP2 | CF_BYTEMODE,
  /* CMD_MUL   */ CF_OP2 | CF_BYTEMODE,
  /* CMD_DIV   */ CF_OP2 | CF_BYTEMODE,
  /* CMD_ADC   */ CF_OP2 | CF_BYTEMODE | CF_USEFLAGS | CF_CHFLAGS ,
  /* CMD_SBB   */ CF_OP2 | CF_BYTEMODE | CF_USEFLAGS | CF_CHFLAGS ,
  /* CMD_PRINT */ CF_OP0
};

#endif


CVm::CVm(): Mem(NULL) {}

bool CVm::Create()
{
  if (!Mem)
    Mem = (Byte *)::MyAlloc(kSpaceSize + 4);
  return (Mem != NULL);
}

CVm::~CVm()
{
  ::MyFree(Mem);
}

// CVm::Execute can change CProgram object: it clears progarm if VM returns error.

bool CVm::Execute(CProgram *prg, const CProgramInitState *initState,
    CBlockRef &outBlockRef, CRecordVector<Byte> &outGlobalData)
{
  memcpy(R, initState->InitR, sizeof(initState->InitR));
  R[kStackRegIndex] = kSpaceSize;
  R[kNumRegs] = 0;
  Flags = 0;

  const UInt32 globalSize = MyMin((UInt32)initState->GlobalData.Size(), kGlobalSize);
  if (globalSize != 0)
    memcpy(Mem + kGlobalOffset, &initState->GlobalData[0], globalSize);
  UInt32 staticSize = MyMin((UInt32)prg->StaticData.Size(), kGlobalSize - globalSize);
  if (staticSize != 0)
    memcpy(Mem + kGlobalOffset + globalSize, &prg->StaticData[0], staticSize);

  bool res = true;
  
  #ifdef Z7_RARVM_STANDARD_FILTERS
  if (prg->StandardFilterIndex >= 0)
    res = ExecuteStandardFilter((unsigned)prg->StandardFilterIndex);
  else
  #endif
  {
    #ifdef Z7_RARVM_VM_ENABLE
    res = ExecuteCode(prg);
    if (!res)
    {
      prg->Commands.Clear();
      prg->Commands.Add(CCommand());
      prg->Commands.Back().OpCode = CMD_RET;
    }
    #else
    res = false;
    #endif
  }
  
  UInt32 newBlockPos = GetFixedGlobalValue32(NGlobalOffset::kBlockPos) & kSpaceMask;
  UInt32 newBlockSize = GetFixedGlobalValue32(NGlobalOffset::kBlockSize) & kSpaceMask;
  if (newBlockPos + newBlockSize >= kSpaceSize)
    newBlockPos = newBlockSize = 0;
  outBlockRef.Offset = newBlockPos;
  outBlockRef.Size = newBlockSize;

  outGlobalData.Clear();
  UInt32 dataSize = GetFixedGlobalValue32(NGlobalOffset::kGlobalMemOutSize);
  dataSize = MyMin(dataSize, kGlobalSize - kFixedGlobalSize);
  if (dataSize != 0)
  {
    dataSize += kFixedGlobalSize;
    outGlobalData.ClearAndSetSize(dataSize);
    memcpy(&outGlobalData[0], Mem + kGlobalOffset, dataSize);
  }

  return res;
}

#ifdef Z7_RARVM_VM_ENABLE

#define SET_IP(IP) \
  if ((IP) >= numCommands) return true; \
  if (--maxOpCount <= 0) return false; \
  cmd = commands + (IP);

#define GET_FLAG_S_B(res) (((res) & 0x80) ? FLAG_S : 0)
#define SET_IP_OP1 { const UInt32 val = GetOperand32(&cmd->Op1); SET_IP(val) }
#define FLAGS_UPDATE_SZ Flags = res == 0 ? FLAG_Z : res & FLAG_S
#define FLAGS_UPDATE_SZ_B Flags = (res & 0xFF) == 0 ? FLAG_Z : GET_FLAG_S_B(res)

UInt32 CVm::GetOperand32(const COperand *op) const
{
  switch (op->Type)
  {
    case OP_TYPE_REG: return R[op->Data];
    case OP_TYPE_REGMEM: return GetValue32(&Mem[(op->Base + R[op->Data]) & kSpaceMask]);
    default: return op->Data;
  }
}

void CVm::SetOperand32(const COperand *op, UInt32 val)
{
  switch (op->Type)
  {
    case OP_TYPE_REG: R[op->Data] = val; return;
    case OP_TYPE_REGMEM: SetValue32(&Mem[(op->Base + R[op->Data]) & kSpaceMask], val); return;
    default: break;
  }
}

Byte CVm::GetOperand8(const COperand *op) const
{
  switch (op->Type)
  {
    case OP_TYPE_REG: return (Byte)R[op->Data];
    case OP_TYPE_REGMEM: return Mem[(op->Base + R[op->Data]) & kSpaceMask];
    default: return (Byte)op->Data;
  }
}

void CVm::SetOperand8(const COperand *op, Byte val)
{
  switch (op->Type)
  {
    case OP_TYPE_REG: R[op->Data] = (R[op->Data] & 0xFFFFFF00) | val; return;
    case OP_TYPE_REGMEM: Mem[(op->Base + R[op->Data]) & kSpaceMask] = val; return;
    default: break;
  }
}

UInt32 CVm::GetOperand(bool byteMode, const COperand *op) const
{
  if (byteMode)
    return GetOperand8(op);
  return GetOperand32(op);
}

void CVm::SetOperand(bool byteMode, const COperand *op, UInt32 val)
{
  if (byteMode)
    SetOperand8(op, (Byte)(val & 0xFF));
  else
    SetOperand32(op, val);
}

bool CVm::ExecuteCode(const CProgram *prg)
{
  Int32 maxOpCount = 25000000;
  const CCommand *commands = &prg->Commands[0];
  const CCommand *cmd = commands;
  const UInt32 numCommands = prg->Commands.Size();
  if (numCommands == 0)
    return false;

  for (;;)
  {
    switch (cmd->OpCode)
    {
      case CMD_MOV:
        SetOperand32(&cmd->Op1, GetOperand32(&cmd->Op2));
        break;
      case CMD_MOVB:
        SetOperand8(&cmd->Op1, GetOperand8(&cmd->Op2));
        break;
      case CMD_CMP:
        {
          const UInt32 v1 = GetOperand32(&cmd->Op1);
          const UInt32 res = v1 - GetOperand32(&cmd->Op2);
          Flags = res == 0 ? FLAG_Z : (res > v1) | (res & FLAG_S);
        }
        break;
      case CMD_CMPB:
        {
          const Byte v1 = GetOperand8(&cmd->Op1);
          const Byte res = (Byte)((v1 - GetOperand8(&cmd->Op2)) & 0xFF);
          Flags = res == 0 ? FLAG_Z : (res > v1) | GET_FLAG_S_B(res);
        }
        break;
      case CMD_ADD:
        {
          const UInt32 v1 = GetOperand32(&cmd->Op1);
          const UInt32 res = v1 + GetOperand32(&cmd->Op2);
          SetOperand32(&cmd->Op1, res);
          Flags = (res < v1) | (res == 0 ? FLAG_Z : (res & FLAG_S));
        }
        break;
      case CMD_ADDB:
        {
          const Byte v1 = GetOperand8(&cmd->Op1);
          const Byte res = (Byte)((v1 + GetOperand8(&cmd->Op2)) & 0xFF);
          SetOperand8(&cmd->Op1, (Byte)res);
          Flags = (res < v1) | (res == 0 ? FLAG_Z : GET_FLAG_S_B(res));
        }
        break;
      case CMD_ADC:
        {
          const UInt32 v1 = GetOperand(cmd->ByteMode, &cmd->Op1);
          const UInt32 FC = (Flags & FLAG_C);
          UInt32 res = v1 + GetOperand(cmd->ByteMode, &cmd->Op2) + FC;
          if (cmd->ByteMode)
            res &= 0xFF;
          SetOperand(cmd->ByteMode, &cmd->Op1, res);
          Flags = (res < v1 || (res == v1 && FC)) | (res == 0 ? FLAG_Z : (res & FLAG_S));
        }
        break;
      case CMD_SUB:
        {
          const UInt32 v1 = GetOperand32(&cmd->Op1);
          const UInt32 res = v1 - GetOperand32(&cmd->Op2);
          SetOperand32(&cmd->Op1, res);
          Flags = res == 0 ? FLAG_Z : (res > v1) | (res & FLAG_S);
        }
        break;
      case CMD_SUBB:
        {
          const UInt32 v1 = GetOperand8(&cmd->Op1);
          const UInt32 res = v1 - GetOperand8(&cmd->Op2);
          SetOperand8(&cmd->Op1, (Byte)res);
          Flags = res == 0 ? FLAG_Z : (res > v1) | (res & FLAG_S);
        }
        break;
      case CMD_SBB:
        {
          const UInt32 v1 = GetOperand(cmd->ByteMode, &cmd->Op1);
          const UInt32 FC = (Flags & FLAG_C);
          UInt32 res = v1 - GetOperand(cmd->ByteMode, &cmd->Op2) - FC;
          // Flags = res == 0 ? FLAG_Z : (res > v1 || res == v1 && FC) | (res & FLAG_S);
          if (cmd->ByteMode)
            res &= 0xFF;
          SetOperand(cmd->ByteMode, &cmd->Op1, res);
          Flags = (res > v1 || (res == v1 && FC)) | (res == 0 ? FLAG_Z : (res & FLAG_S));
        }
        break;
      case CMD_INC:
        {
          const UInt32 res = GetOperand32(&cmd->Op1) + 1;
          SetOperand32(&cmd->Op1, res);
          FLAGS_UPDATE_SZ;
        }
        break;
      case CMD_INCB:
        {
          const Byte res = (Byte)(GetOperand8(&cmd->Op1) + 1);
          SetOperand8(&cmd->Op1, res);
          FLAGS_UPDATE_SZ_B;
        }
        break;
      case CMD_DEC:
        {
          const UInt32 res = GetOperand32(&cmd->Op1) - 1;
          SetOperand32(&cmd->Op1, res);
          FLAGS_UPDATE_SZ;
        }
        break;
      case CMD_DECB:
        {
          const Byte res = (Byte)(GetOperand8(&cmd->Op1) - 1);
          SetOperand8(&cmd->Op1, res);
          FLAGS_UPDATE_SZ_B;
        }
        break;
      case CMD_XOR:
        {
          const UInt32 res = GetOperand32(&cmd->Op1) ^ GetOperand32(&cmd->Op2);
          SetOperand32(&cmd->Op1, res);
          FLAGS_UPDATE_SZ;
        }
        break;
      case CMD_XORB:
        {
          const Byte res = (Byte)(GetOperand8(&cmd->Op1) ^ GetOperand8(&cmd->Op2));
          SetOperand8(&cmd->Op1, res);
          FLAGS_UPDATE_SZ_B;
        }
        break;
      case CMD_AND:
        {
          const UInt32 res = GetOperand32(&cmd->Op1) & GetOperand32(&cmd->Op2);
          SetOperand32(&cmd->Op1, res);
          FLAGS_UPDATE_SZ;
        }
        break;
      case CMD_ANDB:
        {
          const Byte res = (Byte)(GetOperand8(&cmd->Op1) & GetOperand8(&cmd->Op2));
          SetOperand8(&cmd->Op1, res);
          FLAGS_UPDATE_SZ_B;
        }
        break;
      case CMD_OR:
        {
          const UInt32 res = GetOperand32(&cmd->Op1) | GetOperand32(&cmd->Op2);
          SetOperand32(&cmd->Op1, res);
          FLAGS_UPDATE_SZ;
        }
        break;
      case CMD_ORB:
        {
          const Byte res = (Byte)(GetOperand8(&cmd->Op1) | GetOperand8(&cmd->Op2));
          SetOperand8(&cmd->Op1, res);
          FLAGS_UPDATE_SZ_B;
        }
        break;
      case CMD_TEST:
        {
          const UInt32 res = GetOperand32(&cmd->Op1) & GetOperand32(&cmd->Op2);
          FLAGS_UPDATE_SZ;
        }
        break;
      case CMD_TESTB:
        {
          const Byte res = (Byte)(GetOperand8(&cmd->Op1) & GetOperand8(&cmd->Op2));
          FLAGS_UPDATE_SZ_B;
        }
        break;
      case CMD_NOT:
        SetOperand(cmd->ByteMode, &cmd->Op1, ~GetOperand(cmd->ByteMode, &cmd->Op1));
        break;
      case CMD_NEG:
        {
          const UInt32 res = 0 - GetOperand32(&cmd->Op1);
          SetOperand32(&cmd->Op1, res);
          Flags = res == 0 ? FLAG_Z : FLAG_C | (res & FLAG_S);
        }
        break;
      case CMD_NEGB:
        {
          const Byte res = (Byte)(0 - GetOperand8(&cmd->Op1));
          SetOperand8(&cmd->Op1, res);
          Flags = res == 0 ? FLAG_Z : FLAG_C | GET_FLAG_S_B(res);
        }
        break;

      case CMD_SHL:
        {
          const UInt32 v1 = GetOperand32(&cmd->Op1);
          const int v2 = (int)GetOperand32(&cmd->Op2);
          const UInt32 res = v1 << v2;
          SetOperand32(&cmd->Op1, res);
          Flags = (res == 0 ? FLAG_Z : (res & FLAG_S)) | ((v1 << (v2 - 1)) & 0x80000000 ? FLAG_C : 0);
        }
        break;
      case CMD_SHLB:
        {
          const Byte v1 = GetOperand8(&cmd->Op1);
          const int v2 = (int)GetOperand8(&cmd->Op2);
          const Byte res = (Byte)(v1 << v2);
          SetOperand8(&cmd->Op1, res);
          Flags = (res == 0 ? FLAG_Z : GET_FLAG_S_B(res)) | ((v1 << (v2 - 1)) & 0x80 ? FLAG_C : 0);
        }
        break;
      case CMD_SHR:
        {
          const UInt32 v1 = GetOperand32(&cmd->Op1);
          const int v2 = (int)GetOperand32(&cmd->Op2);
          const UInt32 res = v1 >> v2;
          SetOperand32(&cmd->Op1, res);
          Flags = (res == 0 ? FLAG_Z : (res & FLAG_S)) | ((v1 >> (v2 - 1)) & FLAG_C);
        }
        break;
      case CMD_SHRB:
        {
          const Byte v1 = GetOperand8(&cmd->Op1);
          const int v2 = (int)GetOperand8(&cmd->Op2);
          const Byte res = (Byte)(v1 >> v2);
          SetOperand8(&cmd->Op1, res);
          Flags = (res == 0 ? FLAG_Z : GET_FLAG_S_B(res)) | ((v1 >> (v2 - 1)) & FLAG_C);
        }
        break;
      case CMD_SAR:
        {
          const UInt32 v1 = GetOperand32(&cmd->Op1);
          const int v2 = (int)GetOperand32(&cmd->Op2);
          const UInt32 res = UInt32(((Int32)v1) >> v2);
          SetOperand32(&cmd->Op1, res);
          Flags= (res == 0 ? FLAG_Z : (res & FLAG_S)) | ((v1 >> (v2 - 1)) & FLAG_C);
        }
        break;
      case CMD_SARB:
        {
          const Byte v1 = GetOperand8(&cmd->Op1);
          const int v2 = (int)GetOperand8(&cmd->Op2);
          const Byte res = (Byte)(((signed char)v1) >> v2);
          SetOperand8(&cmd->Op1, res);
          Flags= (res == 0 ? FLAG_Z : GET_FLAG_S_B(res)) | ((v1 >> (v2 - 1)) & FLAG_C);
        }
        break;

      case CMD_JMP:
        SET_IP_OP1
        continue;
      case CMD_JZ:
        if ((Flags & FLAG_Z) != 0)
        {
          SET_IP_OP1
          continue;
        }
        break;
      case CMD_JNZ:
        if ((Flags & FLAG_Z) == 0)
        {
          SET_IP_OP1
          continue;
        }
        break;
      case CMD_JS:
        if ((Flags & FLAG_S) != 0)
        {
          SET_IP_OP1
          continue;
        }
        break;
      case CMD_JNS:
        if ((Flags & FLAG_S) == 0)
        {
          SET_IP_OP1
          continue;
        }
        break;
      case CMD_JB:
        if ((Flags & FLAG_C) != 0)
        {
          SET_IP_OP1
          continue;
        }
        break;
      case CMD_JBE:
        if ((Flags & (FLAG_C | FLAG_Z)) != 0)
        {
          SET_IP_OP1
          continue;
        }
        break;
      case CMD_JA:
        if ((Flags & (FLAG_C | FLAG_Z)) == 0)
        {
          SET_IP_OP1
          continue;
        }
        break;
      case CMD_JAE:
        if ((Flags & FLAG_C) == 0)
        {
          SET_IP_OP1
          continue;
        }
        break;
      
      case CMD_PUSH:
        R[kStackRegIndex] -= 4;
        SetValue32(&Mem[R[kStackRegIndex] & kSpaceMask], GetOperand32(&cmd->Op1));
        break;
      case CMD_POP:
        SetOperand32(&cmd->Op1, GetValue32(&Mem[R[kStackRegIndex] & kSpaceMask]));
        R[kStackRegIndex] += 4;
        break;
      case CMD_CALL:
        R[kStackRegIndex] -= 4;
        SetValue32(&Mem[R[kStackRegIndex] & kSpaceMask], (UInt32)(cmd - commands + 1));
        SET_IP_OP1
        continue;

      case CMD_PUSHA:
        {
          for (UInt32 i = 0, SP = R[kStackRegIndex] - 4; i < kNumRegs; i++, SP -= 4)
            SetValue32(&Mem[SP & kSpaceMask], R[i]);
          R[kStackRegIndex] -= kNumRegs * 4;
        }
        break;
      case CMD_POPA:
        {
          for (UInt32 i = 0, SP = R[kStackRegIndex]; i < kNumRegs; i++, SP += 4)
            R[kStackRegIndex - i] = GetValue32(&Mem[SP & kSpaceMask]);
        }
        break;
      case CMD_PUSHF:
        R[kStackRegIndex] -= 4;
        SetValue32(&Mem[R[kStackRegIndex]&kSpaceMask], Flags);
        break;
      case CMD_POPF:
        Flags = GetValue32(&Mem[R[kStackRegIndex] & kSpaceMask]);
        R[kStackRegIndex] += 4;
        break;
      
      case CMD_MOVZX:
        SetOperand32(&cmd->Op1, GetOperand8(&cmd->Op2));
        break;
      case CMD_MOVSX:
        SetOperand32(&cmd->Op1, (UInt32)(Int32)(signed char)GetOperand8(&cmd->Op2));
        break;
      case CMD_XCHG:
        {
          const UInt32 v1 = GetOperand(cmd->ByteMode, &cmd->Op1);
          SetOperand(cmd->ByteMode, &cmd->Op1, GetOperand(cmd->ByteMode, &cmd->Op2));
          SetOperand(cmd->ByteMode, &cmd->Op2, v1);
        }
        break;
      case CMD_MUL:
        {
          const UInt32 res = GetOperand32(&cmd->Op1) * GetOperand32(&cmd->Op2);
          SetOperand32(&cmd->Op1, res);
        }
        break;
      case CMD_MULB:
        {
          const Byte res = (Byte)(GetOperand8(&cmd->Op1) * GetOperand8(&cmd->Op2));
          SetOperand8(&cmd->Op1, res);
        }
        break;
      case CMD_DIV:
        {
          const UInt32 divider = GetOperand(cmd->ByteMode, &cmd->Op2);
          if (divider != 0)
          {
            const UInt32 res = GetOperand(cmd->ByteMode, &cmd->Op1) / divider;
            SetOperand(cmd->ByteMode, &cmd->Op1, res);
          }
        }
        break;
      
      case CMD_RET:
        {
          if (R[kStackRegIndex] >= kSpaceSize)
            return true;
          const UInt32 ip = GetValue32(&Mem[R[kStackRegIndex] & kSpaceMask]);
          SET_IP(ip)
          R[kStackRegIndex] += 4;
          continue;
        }
      case CMD_PRINT:
        break;
    }
    cmd++;
    --maxOpCount;
  }
}

//////////////////////////////////////////////////////
// Read program

static void DecodeArg(CMemBitDecoder &inp, COperand &op, bool byteMode)
{
  if (inp.ReadBit())
  {
    op.Type = OP_TYPE_REG;
    op.Data = inp.ReadBits(kNumRegBits);
  }
  else if (inp.ReadBit() == 0)
  {
    op.Type = OP_TYPE_INT;
    if (byteMode)
      op.Data = inp.ReadBits(8);
    else
      op.Data = inp.ReadEncodedUInt32();
  }
  else
  {
    op.Type = OP_TYPE_REGMEM;
    if (inp.ReadBit() == 0)
    {
      op.Data = inp.ReadBits(kNumRegBits);
      op.Base = 0;
    }
    else
    {
      if (inp.ReadBit() == 0)
        op.Data = inp.ReadBits(kNumRegBits);
      else
        op.Data = kNumRegs;
      op.Base = inp.ReadEncodedUInt32();
    }
  }
}

void CProgram::ReadProgram(const Byte *code, UInt32 codeSize)
{
  CMemBitDecoder inp;
  inp.Init(code, codeSize);

  StaticData.Clear();
  
  if (inp.ReadBit())
  {
    const UInt32 dataSize = inp.ReadEncodedUInt32() + 1;
    for (UInt32 i = 0; inp.Avail() && i < dataSize; i++)
      StaticData.Add((Byte)inp.ReadBits(8));
  }
  
  while (inp.Avail())
  {
    Commands.Add(CCommand());
    CCommand *cmd = &Commands.Back();
    
    unsigned opCode;
    if (inp.ReadBit() == 0)
      opCode = inp.ReadBits(3);
    else
      opCode = 8 + inp.ReadBits(5);
    cmd->OpCode = (ECommand)opCode;
    const unsigned cmdFlags = kCmdFlags[opCode];
    if (cmdFlags & CF_BYTEMODE)
      cmd->ByteMode = (inp.ReadBit()) ? true : false;
    else
      cmd->ByteMode = 0;
    
    const unsigned opNum = (cmdFlags & CF_OPMASK);
    
    if (opNum)
    {
      DecodeArg(inp, cmd->Op1, cmd->ByteMode);
      if (opNum == 2)
        DecodeArg(inp, cmd->Op2, cmd->ByteMode);
      else
      {
        if (cmd->Op1.Type == OP_TYPE_INT && (cmdFlags & (CF_JUMP | CF_PROC)))
        {
          Int32 dist = (Int32)cmd->Op1.Data;
          if (dist >= 256)
            dist -= 256;
          else
          {
            if (dist >= 136)
              dist -= 264;
            else if (dist >= 16)
              dist -= 8;
            else if (dist >= 8)
              dist -= 16;
            dist += Commands.Size() - 1;
          }
          cmd->Op1.Data = (UInt32)dist;
        }
      }
    }

    if (cmd->ByteMode)
    {
      switch (cmd->OpCode)
      {
        case CMD_MOV: cmd->OpCode = CMD_MOVB; break;
        case CMD_CMP: cmd->OpCode = CMD_CMPB; break;
        case CMD_ADD: cmd->OpCode = CMD_ADDB; break;
        case CMD_SUB: cmd->OpCode = CMD_SUBB; break;
        case CMD_INC: cmd->OpCode = CMD_INCB; break;
        case CMD_DEC: cmd->OpCode = CMD_DECB; break;
        case CMD_XOR: cmd->OpCode = CMD_XORB; break;
        case CMD_AND: cmd->OpCode = CMD_ANDB; break;
        case CMD_OR: cmd->OpCode = CMD_ORB; break;
        case CMD_TEST: cmd->OpCode = CMD_TESTB; break;
        case CMD_NEG: cmd->OpCode = CMD_NEGB; break;
        case CMD_SHL: cmd->OpCode = CMD_SHLB; break;
        case CMD_SHR: cmd->OpCode = CMD_SHRB; break;
        case CMD_SAR: cmd->OpCode = CMD_SARB; break;
        case CMD_MUL: cmd->OpCode = CMD_MULB; break;
        default: break;
      }
    }
  }
}

#endif


#ifdef Z7_RARVM_STANDARD_FILTERS

enum EStandardFilter
{
  SF_E8,
  SF_E8E9,
  SF_ITANIUM,
  SF_RGB,
  SF_AUDIO,
  SF_DELTA
  // SF_UPCASE
};

static const struct CStandardFilterSignature
{
  UInt32 Length;
  UInt32 CRC;
  EStandardFilter Type;
}
kStdFilters[]=
{
  {  53, 0xad576887, SF_E8 },
  {  57, 0x3cd7e57e, SF_E8E9 },
  { 120, 0x3769893f, SF_ITANIUM },
  {  29, 0x0e06077d, SF_DELTA },
  { 149, 0x1c2c5dc8, SF_RGB },
  { 216, 0xbc85e701, SF_AUDIO }
  // {  40, 0x46b9c560, SF_UPCASE }
};

static int FindStandardFilter(const Byte *code, UInt32 codeSize)
{
  // return -1; // for debug VM execution
  const UInt32 crc = CrcCalc(code, codeSize);
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(kStdFilters); i++)
  {
    const CStandardFilterSignature &sfs = kStdFilters[i];
    if (sfs.CRC == crc && sfs.Length == codeSize)
      return (int)i;
  }
  return -1;
}

#endif


bool CProgram::PrepareProgram(const Byte *code, UInt32 codeSize)
{
  IsSupported = false;

  #ifdef Z7_RARVM_VM_ENABLE
  Commands.Clear();
  #endif
  
  #ifdef Z7_RARVM_STANDARD_FILTERS
  StandardFilterIndex = -1;
  #endif

  bool isOK = false;

  Byte xorSum = 0;
  for (UInt32 i = 0; i < codeSize; i++)
    xorSum ^= code[i];

  if (xorSum == 0 && codeSize != 0)
  {
    IsSupported = true;
    isOK = true;
    #ifdef Z7_RARVM_STANDARD_FILTERS
    StandardFilterIndex = FindStandardFilter(code, codeSize);
    if (StandardFilterIndex >= 0)
      return true;
    #endif
  
    #ifdef Z7_RARVM_VM_ENABLE
    ReadProgram(code + 1, codeSize - 1);
    #else
    IsSupported = false;
    #endif
  }
  
  #ifdef Z7_RARVM_VM_ENABLE
  Commands.Add(CCommand());
  Commands.Back().OpCode = CMD_RET;
  #endif
  
  return isOK;
}

void CVm::SetMemory(UInt32 pos, const Byte *data, UInt32 dataSize)
{
  if (pos < kSpaceSize && data != Mem + pos)
    memmove(Mem + pos, data, MyMin(dataSize, kSpaceSize - pos));
}

#ifdef Z7_RARVM_STANDARD_FILTERS

static void E8E9Decode(Byte *data, UInt32 dataSize, UInt32 fileOffset, bool e9)
{
  if (dataSize <= 4)
    return;
  dataSize -= 4;
  const UInt32 kFileSize = 0x1000000;
  const Byte cmpMask = (Byte)(e9 ? 0xFE : 0xFF);
  for (UInt32 curPos = 0; curPos < dataSize;)
  {
    curPos++;
    if (((*data++) & cmpMask) == 0xE8)
    {
      UInt32 offset = curPos + fileOffset;
      UInt32 addr = GetValue32(data);
      if (addr < kFileSize)
        SetValue32(data, addr - offset);
      else if ((addr & 0x80000000) != 0 && ((addr + offset) & 0x80000000) == 0)
        SetValue32(data, addr + kFileSize);
      data += 4;
      curPos += 4;
    }
  }
}


static void ItaniumDecode(Byte *data, UInt32 dataSize, UInt32 fileOffset)
{
  if (dataSize <= 21)
    return;
  fileOffset >>= 4;
  dataSize -= 21;
  dataSize += 15;
  dataSize >>= 4;
  dataSize += fileOffset;
  do
  {
    unsigned m = ((UInt32)0x334B0000 >> (data[0] & 0x1E)) & 3;
    if (m)
    {
      m++;
      do
      {
        Byte *p = data + ((size_t)m * 5 - 8);
        if (((p[3] >> m) & 15) == 5)
        {
          const UInt32 kMask = 0xFFFFF;
          // UInt32 raw = ((UInt32)p[0]) | ((UInt32)p[1] << 8) | ((UInt32)p[2] << 16);
          UInt32 raw = GetUi32(p);
          UInt32 v = raw >> m;
          v -= fileOffset;
          v &= kMask;
          raw &= ~(kMask << m);
          raw |= (v << m);
          // p[0] = (Byte)raw; p[1] = (Byte)(raw >> 8); p[2] = (Byte)(raw >> 16);
          SetUi32(p, raw)
        }
      }
      while (++m <= 4);
    }
    data += 16;
  }
  while (++fileOffset != dataSize);
}


static void DeltaDecode(Byte *data, UInt32 dataSize, UInt32 numChannels)
{
  UInt32 srcPos = 0;
  const UInt32 border = dataSize * 2;
  for (UInt32 curChannel = 0; curChannel < numChannels; curChannel++)
  {
    Byte prevByte = 0;
    for (UInt32 destPos = dataSize + curChannel; destPos < border; destPos += numChannels)
      data[destPos] = (prevByte = (Byte)(prevByte - data[srcPos++]));
  }
}

static void RgbDecode(Byte *srcData, UInt32 dataSize, UInt32 width, UInt32 posR)
{
  Byte *destData = srcData + dataSize;
  const UInt32 kNumChannels = 3;
  
  for (UInt32 curChannel = 0; curChannel < kNumChannels; curChannel++)
  {
    Byte prevByte = 0;
    
    for (UInt32 i = curChannel; i < dataSize; i += kNumChannels)
    {
      unsigned predicted;
      if (i < width)
        predicted = prevByte;
      else
      {
        const unsigned upperLeftByte = destData[i - width];
        const unsigned upperByte = destData[i - width + 3];
        predicted = prevByte + upperByte - upperLeftByte;
        const int pa = abs((int)(predicted - prevByte));
        const int pb = abs((int)(predicted - upperByte));
        const int pc = abs((int)(predicted - upperLeftByte));
        if (pa <= pb && pa <= pc)
          predicted = prevByte;
        else
          if (pb <= pc)
            predicted = upperByte;
          else
            predicted = upperLeftByte;
      }
      destData[i] = prevByte = (Byte)(predicted - *(srcData++));
    }
  }
  if (dataSize < 3)
    return;
  const UInt32 border = dataSize - 2;
  for (UInt32 i = posR; i < border; i += 3)
  {
    const Byte g = destData[i + 1];
    destData[i    ] = (Byte)(destData[i    ] + g);
    destData[i + 2] = (Byte)(destData[i + 2] + g);
  }
}

#define my_abs(x) (unsigned)abs(x)

static void AudioDecode(Byte *srcData, UInt32 dataSize, UInt32 numChannels)
{
  Byte *destData = srcData + dataSize;
  for (UInt32 curChannel = 0; curChannel < numChannels; curChannel++)
  {
    UInt32 prevByte = 0, prevDelta = 0, dif[7];
    Int32 D1 = 0, D2 = 0, D3;
    Int32 K1 = 0, K2 = 0, K3 = 0;
    memset(dif, 0, sizeof(dif));
    
    for (UInt32 i = curChannel, byteCount = 0; i < dataSize; i += numChannels, byteCount++)
    {
      D3 = D2;
      D2 = (Int32)prevDelta - D1;
      D1 = (Int32)prevDelta;
      
      UInt32 predicted = (UInt32)((Int32)(8 * prevByte) + K1 * D1 + K2 * D2 + K3 * D3);
      predicted = (predicted >> 3) & 0xFF;
      
      const UInt32 curByte = *(srcData++);
      
      predicted -= curByte;
      destData[i] = (Byte)predicted;
      prevDelta = (UInt32)(Int32)(signed char)(predicted - prevByte);
      prevByte = predicted;
      
      const Int32 D = ((Int32)(signed char)curByte) << 3;
      
      dif[0] += my_abs(D);
      dif[1] += my_abs(D - D1);
      dif[2] += my_abs(D + D1);
      dif[3] += my_abs(D - D2);
      dif[4] += my_abs(D + D2);
      dif[5] += my_abs(D - D3);
      dif[6] += my_abs(D + D3);
      
      if ((byteCount & 0x1F) == 0)
      {
        UInt32 minDif = dif[0], numMinDif = 0;
        dif[0] = 0;
        for (unsigned j = 1; j < Z7_ARRAY_SIZE(dif); j++)
        {
          if (dif[j] < minDif)
          {
            minDif = dif[j];
            numMinDif = j;
          }
          dif[j] = 0;
        }
        switch (numMinDif)
        {
          case 1: if (K1 >= -16) K1--; break;
          case 2: if (K1 <   16) K1++; break;
          case 3: if (K2 >= -16) K2--; break;
          case 4: if (K2 <   16) K2++; break;
          case 5: if (K3 >= -16) K3--; break;
          case 6: if (K3 <   16) K3++; break;
        }
      }
    }
  }
}

/*
static UInt32 UpCaseDecode(Byte *data, UInt32 dataSize)
{
  UInt32 srcPos = 0, destPos = dataSize;
  while (srcPos < dataSize)
  {
    Byte curByte = data[srcPos++];
    if (curByte == 2 && (curByte = data[srcPos++]) != 2)
      curByte -= 32;
    data[destPos++] = curByte;
  }
  return destPos - dataSize;
}
*/

bool CVm::ExecuteStandardFilter(unsigned filterIndex)
{
  const UInt32 dataSize = R[4];
  if (dataSize >= kGlobalOffset)
    return false;
  EStandardFilter filterType = kStdFilters[filterIndex].Type;

  switch (filterType)
  {
    case SF_E8:
    case SF_E8E9:
      E8E9Decode(Mem, dataSize, R[6], (filterType == SF_E8E9));
      break;
    
    case SF_ITANIUM:
      ItaniumDecode(Mem, dataSize, R[6]);
      break;
    
    case SF_DELTA:
    {
      if (dataSize >= kGlobalOffset / 2)
        return false;
      const UInt32 numChannels = R[0];
      if (numChannels == 0 || numChannels > 1024) // unrar 5.5.5
        return false;
      SetBlockPos(dataSize);
      DeltaDecode(Mem, dataSize, numChannels);
      break;
    }
    
    case SF_RGB:
    {
      if (dataSize >= kGlobalOffset / 2 || dataSize < 3) // unrar 5.5.5
        return false;
      const UInt32 width = R[0];
      const UInt32 posR = R[1];
      if (width < 3 || width - 3 > dataSize || posR > 2) // unrar 5.5.5
        return false;
      SetBlockPos(dataSize);
      RgbDecode(Mem, dataSize, width, posR);
      break;
    }
    
    case SF_AUDIO:
    {
      if (dataSize >= kGlobalOffset / 2)
        return false;
      const UInt32 numChannels = R[0];
      if (numChannels == 0 || numChannels > 128) // unrar 5.5.5
        return false;
      SetBlockPos(dataSize);
      AudioDecode(Mem, dataSize, numChannels);
      break;
    }
    
    /*
    case SF_UPCASE:
      if (dataSize >= kGlobalOffset / 2)
        return false;
      UInt32 destSize = UpCaseDecode(Mem, dataSize);
      SetBlockSize(destSize);
      SetBlockPos(dataSize);
      break;
    */
  }
  return true;
}

#endif

}}}

/* ================ unit: CPP/7zip/Compress/Rar5Decoder.cpp ================ */
// Rar5Decoder.cpp
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

// amalgamation: header emitted in prologue

#define DICT_SIZE_MAX ((UInt64)1 << DICT_SIZE_BITS_MAX)

// #include <emmintrin.h> // SSE2
// #endif

// amalgamation: header emitted in prologue
#if 0
// amalgamation: header emitted in prologue
#endif

#if defined(MY_CPU_ARM64)
#include <arm_neon.h>
#endif

// #define Z7_RAR5_SHOW_STAT
// #include <stdio.h>
#ifdef Z7_RAR5_SHOW_STAT
#include <stdio.h>
#endif

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

/*
Note: original-unrar claims that encoder has limitation for Distance:
  (Distance <= MaxWinSize - MAX_INC_LZ_MATCH)
  MAX_INC_LZ_MATCH = 0x1001 + 3;
*/

#define LZ_ERROR_TYPE_NO      0
#define LZ_ERROR_TYPE_HEADER  1
// #define LZ_ERROR_TYPE_SYM     1
#define LZ_ERROR_TYPE_DIST    2

static
void My_ZeroMemory(void *p, size_t size)
{
  #if defined(MY_CPU_AMD64) && !defined(_M_ARM64EC) \
    && defined(Z7_MSC_VER_ORIGINAL) && (Z7_MSC_VER_ORIGINAL <= 1400)
      // __stosq((UInt64 *)(void *)win, 0, size / 8);
      /*
      printf("\n__stosb \n");
      #define STEP_BIG (1 << 28)
      for (size_t i = 0; i < ((UInt64)1 << 50); i += STEP_BIG)
      {
        printf("\n__stosb end %p\n", (void *)i);
        __stosb((Byte *)p + i, 0, STEP_BIG);
      }
      */
      // __stosb((Byte *)p, 0, 0);
      __stosb((Byte *)p, 0, size);
  #else
    // SecureZeroMemory (win, STEP);
    // ZeroMemory(win, STEP);
    // memset(win, 0, STEP);
    memset(p, 0, size);
  #endif
}



#ifdef MY_CPU_LE_UNALIGN
  #define Z7_RAR5_DEC_USE_UNALIGNED_COPY
#endif

#ifdef Z7_RAR5_DEC_USE_UNALIGNED_COPY

  #define COPY_CHUNK_SIZE 16

    #define COPY_CHUNK_4_2(dest, src) \
    { \
      ((UInt32 *)(void *)dest)[0] = ((const UInt32 *)(const void *)src)[0]; \
      ((UInt32 *)(void *)dest)[1] = ((const UInt32 *)(const void *)src)[1]; \
      src  += 4 * 2; \
      dest += 4 * 2; \
    }

  /* sse2 doesn't help here in GCC and CLANG.
     so we disabled sse2 here */
#if 0
  #if defined(MY_CPU_AMD64)
    #define Z7_RAR5_DEC_USE_SSE2
  #elif defined(MY_CPU_X86)
    #if defined(_MSC_VER) && _MSC_VER >= 1300 && defined(_M_IX86_FP) && (_M_IX86_FP >= 2) \
      || defined(__SSE2__) \
      // || 1 == 1  // for debug only
      #define Z7_RAR5_DEC_USE_SSE2
    #endif
  #endif
#endif

  #if defined(MY_CPU_ARM64)

    #define COPY_OFFSET_MIN  16
    #define COPY_CHUNK1(dest, src) \
    { \
      vst1q_u8((uint8_t *)(void *)dest, \
      vld1q_u8((const uint8_t *)(const void *)src)); \
      src += 16; \
      dest += 16; \
    }
    
    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK1(dest, src) \
      if (dest >= lim) break; \
      COPY_CHUNK1(dest, src) \
    }

  #elif defined(Z7_RAR5_DEC_USE_SSE2)
    #include <emmintrin.h> // sse2
    #define COPY_OFFSET_MIN  16

    #define COPY_CHUNK1(dest, src) \
    { \
      _mm_storeu_si128((__m128i *)(void *)dest, \
      _mm_loadu_si128((const __m128i *)(const void *)src)); \
      src += 16; \
      dest += 16; \
    }

    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK1(dest, src) \
      if (dest >= lim) break; \
      COPY_CHUNK1(dest, src) \
    }

  #elif defined(MY_CPU_64BIT)
    #define COPY_OFFSET_MIN  8

    #define COPY_CHUNK(dest, src) \
    { \
      ((UInt64 *)(void *)dest)[0] = ((const UInt64 *)(const void *)src)[0]; \
      src  += 8 * 1; dest += 8 * 1; \
      ((UInt64 *)(void *)dest)[0] = ((const UInt64 *)(const void *)src)[0]; \
      src  += 8 * 1; dest += 8 * 1; \
    }

  #else
    #define COPY_OFFSET_MIN  4

    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK_4_2(dest, src); \
      COPY_CHUNK_4_2(dest, src); \
    }

  #endif
#endif


#ifndef COPY_CHUNK_SIZE
    #define COPY_OFFSET_MIN  4
    #define COPY_CHUNK_SIZE  8
    #define COPY_CHUNK_2(dest, src) \
    { \
      const Byte a0 = src[0]; \
      const Byte a1 = src[1]; \
      dest[0] = a0; \
      dest[1] = a1; \
      src += 2; \
      dest += 2; \
    }
    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
    }
#endif


#define COPY_CHUNKS \
{ \
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE \
  do { COPY_CHUNK(dest, src) } \
  while (dest < lim); \
}

namespace NCompress {
namespace NRar5 {

typedef
#if 1
  unsigned
#else
  size_t
#endif
  CLenType;

// (len != 0)
static
Z7_FORCE_INLINE
// Z7_ATTRIB_NO_VECTOR
void CopyMatch(size_t offset, Byte *dest, const Byte *src, const Byte *lim)
{
  {
    // (COPY_OFFSET_MIN >= 4)
    if (offset >= COPY_OFFSET_MIN)
    {
      COPY_CHUNKS
      // return;
    }
    else
  #if (COPY_OFFSET_MIN > 4)
    #if COPY_CHUNK_SIZE < 8
      #error Stop_Compiling_Bad_COPY_CHUNK_SIZE
    #endif
    if (offset >= 4)
    {
      Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
      do
      {
        COPY_CHUNK_4_2(dest, src)
        #if COPY_CHUNK_SIZE < 16
          if (dest >= lim) break;
        #endif
        COPY_CHUNK_4_2(dest, src)
      }
      while (dest < lim);
      // return;
    }
    else
  #endif
    {
      // (offset < 4)
      const unsigned b0 = src[0];
      if (offset < 2)
      {
      #if defined(Z7_RAR5_DEC_USE_UNALIGNED_COPY) && (COPY_CHUNK_SIZE == 16)
        #if defined(MY_CPU_64BIT)
        {
          const UInt64 v64 = (UInt64)b0 * 0x0101010101010101;
          Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
          do
          {
            ((UInt64 *)(void *)dest)[0] = v64;
            ((UInt64 *)(void *)dest)[1] = v64;
            dest += 16;
          }
          while (dest < lim);
        }
        #else
        {
          UInt32 v = b0;
          v |= v << 8;
          v |= v << 16;
          do
          {
            ((UInt32 *)(void *)dest)[0] = v;
            ((UInt32 *)(void *)dest)[1] = v;
            dest += 8;
            ((UInt32 *)(void *)dest)[0] = v;
            ((UInt32 *)(void *)dest)[1] = v;
            dest += 8;
          }
          while (dest < lim);
        }
        #endif
      #else
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = (Byte)b0;
          dest += 2;
          dest[0] = (Byte)b0;
          dest[1] = (Byte)b0;
          dest += 2;
        }
        while (dest < lim);
      #endif
      }
      else if (offset == 2)
      {
        const Byte b1 = src[1];
        {
          do
          {
            dest[0] = (Byte)b0;
            dest[1] = b1;
            dest += 2;
          }
          while (dest < lim);
        }
      }
      else // (offset == 3)
      {
        const Byte b1 = src[1];
        const Byte b2 = src[2];
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = b1;
          dest[2] = b2;
          dest += 3;
        }
        while (dest < lim);
      }
    }
  }
}

static const size_t kInputBufSize = 1 << 20;
static const UInt32   k_Filter_BlockSize_MAX = 1 << 22;
static const unsigned k_Filter_AfterPad_Size = 64;

#ifdef Z7_RAR5_SHOW_STAT
static const unsigned kNumStats1 = 10;
static const unsigned kNumStats2 = (1 << 12) + 16;
static UInt32 g_stats1[kNumStats1];
static UInt32 g_stats2[kNumStats1][kNumStats2];
#endif

#if 1
MY_ALIGN(32)
// DICT_SIZE_BITS_MAX-1 are required
static const Byte k_LenPlusTable[DICT_SIZE_BITS_MAX] =
  { 0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
#endif



class CBitDecoder
{
public:
  const Byte *_buf;
  const Byte *_bufCheck_Block;  // min(ptr for _blockEnd, _bufCheck)
  unsigned _bitPos;             // = [0 ... 7]
  bool _wasFinished;
  bool _minorError;
  unsigned _blockEndBits7;      // = [0 ... 7] : the number of additional bits in (_blockEnd) poisition.
  HRESULT _hres;
  const Byte *_bufCheck;        // relaxed limit (16 bytes before real end of input data in buffer)
  Byte *_bufLim;                // end if input data
  Byte *_bufBase;
  ISequentialInStream *_stream;

  UInt64 _processedSize;
  UInt64 _blockEnd;     // absolute end of current block
      // but it doesn't include additional _blockEndBits7 [0 ... 7] bits

  Z7_FORCE_INLINE
  void CopyFrom(const CBitDecoder &a)
  {
    _buf = a._buf;
    _bufCheck_Block = a._bufCheck_Block;
    _bitPos = a._bitPos;
    _wasFinished = a._wasFinished;
    _blockEndBits7 = a._blockEndBits7;
    _bufCheck = a._bufCheck;
    _bufLim = a._bufLim;
    _bufBase = a._bufBase;
    
    _processedSize = a._processedSize;
    _blockEnd = a._blockEnd;
  }

  Z7_FORCE_INLINE
  void RestoreFrom2(const CBitDecoder &a)
  {
    _buf = a._buf;
    _bitPos = a._bitPos;
  }

  Z7_FORCE_INLINE
  void SetCheck_forBlock()
  {
    _bufCheck_Block = _bufCheck;
    if (_bufCheck > _buf)
    {
      const UInt64 processed = GetProcessedSize_Round();
      if (_blockEnd < processed)
        _bufCheck_Block = _buf;
      else
      {
        const UInt64 delta = _blockEnd - processed;
        if ((size_t)(_bufCheck - _buf) > delta)
          _bufCheck_Block = _buf + (size_t)delta;
      }
    }
  }

  Z7_FORCE_INLINE
  bool IsBlockOverRead() const
  {
    const UInt64 v = GetProcessedSize_Round();
    if (v < _blockEnd) return false;
    if (v > _blockEnd) return true;
    return _bitPos > _blockEndBits7;
  }

  /*
  CBitDecoder() throw():
      _buf(0),
      _bufLim(0),
      _bufBase(0),
      _stream(0),
      _processedSize(0),
      _wasFinished(false)
      {}
  */

  Z7_FORCE_INLINE
  void Init() throw()
  {
    _blockEnd = 0;
    _blockEndBits7 = 0;

    _bitPos = 0;
    _processedSize = 0;
    _buf = _bufBase;
    _bufLim = _bufBase;
    _bufCheck = _buf;
    _bufCheck_Block = _buf;
    _wasFinished = false;
    _minorError = false;
  }

  void Prepare2() throw();

  Z7_FORCE_INLINE
  void Prepare() throw()
  {
    if (_buf >= _bufCheck)
      Prepare2();
  }

  Z7_FORCE_INLINE
  bool ExtraBitsWereRead() const
  {
    return _buf >= _bufLim && (_buf > _bufLim || _bitPos != 0);
  }

  Z7_FORCE_INLINE bool InputEofError() const { return ExtraBitsWereRead(); }

  Z7_FORCE_INLINE unsigned GetProcessedBits7() const { return _bitPos; }
  Z7_FORCE_INLINE UInt64 GetProcessedSize_Round() const { return _processedSize + (size_t)(_buf - _bufBase); }
  Z7_FORCE_INLINE UInt64 GetProcessedSize() const { return _processedSize + (size_t)(_buf - _bufBase) + ((_bitPos + 7) >> 3); }

  Z7_FORCE_INLINE
  void AlignToByte()
  {
    if (_bitPos != 0)
    {
#if 1
      // optional check of unused bits for strict checking:
      // original-unrar doesn't check it:
      const unsigned b = (unsigned)*_buf << _bitPos;
      if (b & 0xff)
        _minorError = true;
#endif
      _buf++;
      _bitPos = 0;
    }
    // _buf += (_bitPos + 7) >> 3;
    // _bitPos = 0;
  }

  Z7_FORCE_INLINE
  Byte ReadByte_InAligned()
  {
    return *_buf++;
  }

  Z7_FORCE_INLINE
  UInt32 GetValue(unsigned numBits) const
  {
    // 0 < numBits <= 17 : supported values
#if defined(Z7_CPU_FAST_BSWAP_SUPPORTED) && defined(MY_CPU_LE_UNALIGN)
    UInt32 v = GetBe32(_buf);
#if 1
    return (v >> (32 - numBits - _bitPos)) & ((1u << numBits) - 1);
#else
    return (v << _bitPos) >> (32 - numBits);
#endif
#else
    UInt32 v = ((UInt32)_buf[0] << 16) | ((UInt32)_buf[1] << 8) | (UInt32)_buf[2];
    v >>= 24 - numBits - _bitPos;
    return v & ((1 << numBits) - 1);
#endif
  }

  Z7_FORCE_INLINE
  UInt32 GetValue_InHigh32bits() const
  {
    // 0 < numBits <= 17 : supported vales
#if defined(Z7_CPU_FAST_BSWAP_SUPPORTED) && defined(MY_CPU_LE_UNALIGN)
    return GetBe32(_buf) << _bitPos;
#else
    const UInt32 v = ((UInt32)_buf[0] << 16) | ((UInt32)_buf[1] << 8) | (UInt32)_buf[2];
    return v << (_bitPos + 8);
#endif
  }
  

  Z7_FORCE_INLINE
  void MovePos(unsigned numBits)
  {
    numBits += _bitPos;
    _buf += numBits >> 3;
    _bitPos = numBits & 7;
  }
    

  Z7_FORCE_INLINE
  UInt32 ReadBits9(unsigned numBits)
  {
    const Byte *buf = _buf;
    UInt32 v = ((UInt32)buf[0] << 8) | (UInt32)buf[1];
    v &= (UInt32)0xFFFF >> _bitPos;
    numBits += _bitPos;
    v >>= 16 - numBits;
    _buf = buf + (numBits >> 3);
    _bitPos = numBits & 7;
    return v;
  }

  Z7_FORCE_INLINE
  UInt32 ReadBits_9fix(unsigned numBits)
  {
    const Byte *buf = _buf;
    UInt32 v = ((UInt32)buf[0] << 8) | (UInt32)buf[1];
    const UInt32 mask = (1u << numBits) - 1;
    numBits += _bitPos;
    v >>= 16 - numBits;
    _buf = buf + (numBits >> 3);
    _bitPos = numBits & 7;
    return v & mask;
  }

#if 1 && defined(MY_CPU_SIZEOF_POINTER) && (MY_CPU_SIZEOF_POINTER == 8)
#define Z7_RAR5_USE_64BIT
#endif

#ifdef Z7_RAR5_USE_64BIT
#define MAX_DICT_LOG (sizeof(size_t) / 8 * 5 + 31)
#else
#define MAX_DICT_LOG 31
#endif

#ifdef Z7_RAR5_USE_64BIT

  Z7_FORCE_INLINE
  size_t ReadBits_Big(unsigned numBits, UInt64 v)
  {
    const UInt64 mask = ((UInt64)1 << numBits) - 1;
    numBits += _bitPos;
    const Byte *buf = _buf;
    // UInt64 v = GetBe64(buf);
    v >>= 64 - numBits;
    _buf = buf + (numBits >> 3);
    _bitPos = numBits & 7;
    return (size_t)(v & mask);
  }
  #define ReadBits_Big25 ReadBits_Big

#else

  // (numBits <= 25) for 32-bit mode
  Z7_FORCE_INLINE
  size_t ReadBits_Big25(unsigned numBits, UInt32 v)
  {
    const UInt32 mask = ((UInt32)1 << numBits) - 1;
    numBits += _bitPos;
    v >>= 32 - numBits;
    _buf += numBits >> 3;
    _bitPos = numBits & 7;
    return v & mask;
  }

  // numBits != 0
  Z7_FORCE_INLINE
  size_t ReadBits_Big(unsigned numBits, UInt32 v)
  {
    const Byte *buf = _buf;
    // UInt32 v = GetBe32(buf);
#if 0
    const UInt32 mask = ((UInt32)1 << numBits) - 1;
    numBits += _bitPos;
    if (numBits > 32)
    {
      v <<= numBits - 32;
      v |= (UInt32)buf[4] >> (40 - numBits);
    }
    else
      v >>= 32 - numBits;
    _buf = buf + (numBits >> 3);
    _bitPos = numBits & 7;
    return v & mask;
#else
    v <<= _bitPos;
    v |= (UInt32)buf[4] >> (8 - _bitPos);
    v >>= 32 - numBits;
    numBits += _bitPos;
    _buf = buf + (numBits >> 3);
    _bitPos = numBits & 7;
    return v;
#endif
  }
#endif
};


static const unsigned kLookaheadSize = 16;
static const unsigned kInputBufferPadZone = kLookaheadSize;

Z7_NO_INLINE
void CBitDecoder::Prepare2() throw()
{
  if (_buf > _bufLim)
    return;

  size_t rem = (size_t)(_bufLim - _buf);
  if (rem != 0)
    memmove(_bufBase, _buf, rem);

  _bufLim = _bufBase + rem;
  _processedSize += (size_t)(_buf - _bufBase);
  _buf = _bufBase;

  // we do not look ahead more than 16 bytes before limit checks.

  if (!_wasFinished)
  {
    while (rem <= kLookaheadSize)
    {
      UInt32 processed = (UInt32)(kInputBufSize - rem);
      // processed = 33; // for debug
      _hres = _stream->Read(_bufLim, processed, &processed);
      _bufLim += processed;
      rem += processed;
      if (processed == 0 || _hres != S_OK)
      {
        _wasFinished = true;
        // if (_hres != S_OK) throw CInBufferException(result);
        break;
      }
    }
  }

  // we always fill pad zone here.
  // so we don't need to call Prepare2() if (_wasFinished == true)
  memset(_bufLim, 0xFF, kLookaheadSize);

  if (rem < kLookaheadSize)
  {
    _bufCheck = _buf;
    // memset(_bufLim, 0xFF, kLookaheadSize - rem);
  }
  else
    _bufCheck = _bufLim - kLookaheadSize;

  SetCheck_forBlock();
}


enum FilterType
{
  FILTER_DELTA = 0,
  FILTER_E8,
  FILTER_E8E9,
  FILTER_ARM
};

static const size_t kWriteStep = (size_t)1 << 18;
      // (size_t)1 << 22; // original-unrar

// Original unRAR claims that maximum possible filter block size is (1 << 16) now,
// and (1 << 17) is minimum win size required to support filter.
// Original unRAR uses (1u << 18) for "extra safety and possible filter area size expansion"
// We can use any win size, but we use same (1u << 18) for compatibility
// with WinRar

// static const unsigned kWinSize_Log_Min = 17;
static const size_t kWinSize_Min = 1u << 18;

CDecoder::CDecoder():
    _isSolid(false),
    _is_v7(false),
    _wasInit(false),
    // _dictSizeLog(0),
    _dictSize(kWinSize_Min),
    _window(NULL),
    _winPos(0),
    _winSize(0),
    _dictSize_forCheck(0),
    _lzSize(0),
    _lzEnd(0),
    _writtenFileSize(0),
    _filters(NULL),
    _winSize_Allocated(0),
    _inputBuf(NULL)
{
#if 1
  memcpy(m_LenPlusTable, k_LenPlusTable, sizeof(k_LenPlusTable));
#endif
  // printf("\nsizeof(CDecoder) == %d\n", sizeof(CDecoder));
}

CDecoder::~CDecoder()
{
#ifdef Z7_RAR5_SHOW_STAT
  printf("\n%4d :", 0);
  for (unsigned k = 0; k < kNumStats1; k++)
    printf(" %8u", (unsigned)g_stats1[k]);
  printf("\n");
  for (unsigned i = 0; i < kNumStats2; i++)
  {
    printf("\n%4d :", i);
    for (unsigned k = 0; k < kNumStats1; k++)
      printf(" %8u", (unsigned)g_stats2[k][i]);
  }
  printf("\n");
#endif

#define Z7_RAR_FREE_WINDOW ::BigFree(_window);
  
  Z7_RAR_FREE_WINDOW
  z7_AlignedFree(_inputBuf);
  z7_AlignedFree(_filters);
}

Z7_NO_INLINE
void CDecoder::DeleteUnusedFilters()
{
  if (_numUnusedFilters != 0)
  {
    // printf("\nDeleteUnusedFilters _numFilters = %6u\n", _numFilters);
    const unsigned n = _numFilters - _numUnusedFilters;
    _numFilters = n;
    memmove(_filters, _filters + _numUnusedFilters, n * sizeof(CFilter));
    _numUnusedFilters = 0;
  }
}


Z7_NO_INLINE
HRESULT CDecoder::WriteData(const Byte *data, size_t size)
{
  HRESULT res = S_OK;
  if (!_unpackSize_Defined || _writtenFileSize < _unpackSize)
  {
    size_t cur = size;
    if (_unpackSize_Defined)
    {
      const UInt64 rem = _unpackSize - _writtenFileSize;
      if (cur > rem)
        cur = (size_t)rem;
    }
    res = WriteStream(_outStream, data, cur);
    if (res != S_OK)
      _writeError = true;
  }
  _writtenFileSize += size;
  return res;
}


#if defined(MY_CPU_SIZEOF_POINTER) \
    && ( MY_CPU_SIZEOF_POINTER == 4 \
      || MY_CPU_SIZEOF_POINTER == 8)
  #define BR_CONV_USE_OPT_PC_PTR
#endif

#ifdef BR_CONV_USE_OPT_PC_PTR
#define BR_PC_INIT(lim_back)  pc -= (UInt32)(SizeT)data;
#define BR_PC_GET        (pc + (UInt32)(SizeT)data)
#else
#define BR_PC_INIT(lim_back)  pc += (UInt32)dataSize - (lim_back);
#define BR_PC_GET        (pc - (UInt32)(SizeT)(data_lim - data))
#endif

#ifdef MY_CPU_LE_UNALIGN
#define Z7_RAR5_FILTER_USE_LE_UNALIGN
#endif

#ifdef Z7_RAR5_FILTER_USE_LE_UNALIGN
#define RAR_E8_FILT(mask) \
{ \
  for (;;) \
  { UInt32 v; \
    do { \
      v = GetUi32(data) ^ (UInt32)0xe8e8e8e8; \
      data += 4; \
      if ((v & ((UInt32)(mask) << (8 * 0))) == 0) { data -= 3; break; } \
      if ((v & ((UInt32)(mask) << (8 * 1))) == 0) { data -= 2; break; } \
      if ((v & ((UInt32)(mask) << (8 * 2))) == 0) { data -= 1; break; } } \
    while((v & ((UInt32)(mask) << (8 * 3)))); \
    if (data > data_lim) break; \
    const UInt32 offset = BR_PC_GET & (kFileSize - 1); \
    const UInt32 addr = GetUi32(data); \
    data += 4; \
    if (addr < kFileSize) \
      SetUi32(data - 4, addr - offset) \
    else if (addr > ~offset) /* if (addr > ((UInt32)0xFFFFFFFF - offset)) */ \
      SetUi32(data - 4, addr + kFileSize) \
  } \
}
#else
#define RAR_E8_FILT(get_byte) \
{ \
  for (;;) \
  { \
    if ((get_byte) != 0xe8) \
    if ((get_byte) != 0xe8) \
    if ((get_byte) != 0xe8) \
    if ((get_byte) != 0xe8) \
      continue; \
    { if (data > data_lim) break; \
    const UInt32 offset = BR_PC_GET & (kFileSize - 1); \
    const UInt32 addr = GetUi32(data); \
    data += 4; \
    if (addr < kFileSize) \
      SetUi32(data - 4, addr - offset) \
      else if (addr > ~offset) /* if (addr > ((UInt32)0xFFFFFFFF - offset)) */ \
      SetUi32(data - 4, addr + kFileSize) \
    } \
  } \
}
#endif

HRESULT CDecoder::ExecuteFilter(const CFilter &f)
{
  Byte *data = _filterSrc;
  UInt32 dataSize = f.Size;
  // printf("\nType = %d offset = %9d  size = %5d", f.Type, (unsigned)(f.Start - _lzFileStart), dataSize);

  if (f.Type == FILTER_DELTA)
  {
    // static unsigned g1 = 0, g2 = 0; g1 += dataSize;
    // if (g2++ % 100 == 0) printf("DELTA  num %8u, size %8u MiB, channels = %2u curSize=%8u\n", g2, (g1 >> 20), f.Channels, dataSize);
    _filterDst.AllocAtLeast_max((size_t)dataSize, k_Filter_BlockSize_MAX);
    if (!_filterDst.IsAllocated())
      return E_OUTOFMEMORY;
    
    Byte *dest = _filterDst;
    const unsigned numChannels = f.Channels;
    unsigned curChannel = 0;
    do
    {
      Byte prevByte = 0;
      Byte *dest2 = dest + curChannel;
      const Byte *dest_lim = dest + dataSize;
      for (; dest2 < dest_lim; dest2 += numChannels)
        *dest2 = (prevByte = (Byte)(prevByte - *data++));
    }
    while (++curChannel != numChannels);
    // return WriteData(dest, dataSize);
    data = dest;
  }
  else if (f.Type < FILTER_ARM)
  {
    // FILTER_E8 or FILTER_E8E9
    if (dataSize > 4)
    {
      UInt32 pc = (UInt32)(f.Start - _lzFileStart);
      const UInt32 kFileSize = (UInt32)1 << 24;
      const Byte *data_lim = data + dataSize - 4;
      BR_PC_INIT(4) // because (data_lim) was moved back for 4 bytes
      data[dataSize] = 0xe8;
      if (f.Type == FILTER_E8)
      {
        // static unsigned g1 = 0; g1 += dataSize; printf("\n  FILTER_E8   %u", (g1 >> 20));
#ifdef Z7_RAR5_FILTER_USE_LE_UNALIGN
        RAR_E8_FILT (0xff)
#else
        RAR_E8_FILT (*data++)
#endif
      }
      else
      {
        // static unsigned g1 = 0; g1 += dataSize; printf("\n  FILTER_E8_E9 %u", (g1 >> 20));
#ifdef Z7_RAR5_FILTER_USE_LE_UNALIGN
        RAR_E8_FILT (0xfe)
#else
        RAR_E8_FILT (*data++ & 0xfe)
#endif
      }
    }
    data = _filterSrc;
  }
  else if (f.Type == FILTER_ARM)
  {
    UInt32 pc = (UInt32)(f.Start - _lzFileStart);
#if 0
    // z7_BranchConv_ARM_Dec expects that (fileOffset & 3) == 0;
    // but even if (fileOffset & 3) then current code
    // in z7_BranchConv_ARM_Dec works same way as unrar's code still.
    z7_BranchConv_ARM_Dec(data, dataSize, pc - 8);
#else
    dataSize &= ~(UInt32)3;
    if (dataSize)
    {
      Byte *data_lim = data + dataSize;
      data_lim[3] = 0xeb;
      BR_PC_INIT(0)
      pc -= 4;  // because (data) will point to next instruction
      for (;;) // do
      {
        data += 4;
        if (data[-1] != 0xeb)
          continue;
        if (data > data_lim)
          break;
        {
          UInt32 v = GetUi32a(data - 4) - (BR_PC_GET >> 2);
          v &= 0x00ffffff;
          v |= 0xeb000000;
          SetUi32a(data - 4, v)
        }
      }
    }
#endif
    data = _filterSrc;
  }
  else
  {
    _unsupportedFilter = true;
    My_ZeroMemory(data, dataSize);
    // return S_OK;  // unrar
  }
  // return WriteData(_filterSrc, (size_t)f.Size);
  return WriteData(data, (size_t)f.Size);
}


HRESULT CDecoder::WriteBuf()
{
  DeleteUnusedFilters();
  const UInt64 lzSize = _lzSize + _winPos;

  for (unsigned i = 0; i < _numFilters;)
  {
    const size_t lzAvail = (size_t)(lzSize - _lzWritten);
    if (lzAvail == 0)
      break;
    // (lzAvail != 0)
    const CFilter &f = _filters[i];
    const UInt64 blockStart = f.Start;
    if (blockStart > _lzWritten)
    {
      const UInt64 rem = blockStart - _lzWritten;
      // (rem != 0)
      size_t size = lzAvail;
      if (size > rem)
        size = (size_t)rem;
      // (size != 0)
      RINOK(WriteData(_window + _winPos - lzAvail, size))
      _lzWritten += size;
      continue;
    }

    // (blockStart <= _lzWritten)
    const UInt32 blockSize = f.Size;
    size_t offset = (size_t)(_lzWritten - blockStart);
    if (offset == 0)
    {
      _filterSrc.AllocAtLeast_max(
          (size_t)blockSize      + k_Filter_AfterPad_Size,
          k_Filter_BlockSize_MAX + k_Filter_AfterPad_Size);
      if (!_filterSrc.IsAllocated())
        return E_OUTOFMEMORY;
    }
    
    const size_t blockRem = (size_t)blockSize - offset;
    size_t size = lzAvail;
    if (size > blockRem)
        size = blockRem;
    memcpy(_filterSrc + offset, _window + _winPos - lzAvail, size);
    _lzWritten += size;
    offset += size;
    if (offset != blockSize)
      return S_OK;

    _numUnusedFilters = ++i;
    RINOK(ExecuteFilter(f))
  }
      
  DeleteUnusedFilters();
  if (_numFilters)
    return S_OK;
  const size_t lzAvail = (size_t)(lzSize - _lzWritten);
  RINOK(WriteData(_window + _winPos - lzAvail, lzAvail))
  _lzWritten += lzAvail;
  return S_OK;
}


Z7_NO_INLINE
static UInt32 ReadUInt32(CBitDecoder &bi)
{
  const unsigned numBits = (unsigned)bi.ReadBits_9fix(2) * 8 + 8;
  UInt32 v = 0;
  unsigned i = 0;
  do
  {
    v += (UInt32)bi.ReadBits_9fix(8) << i;
    i += 8;
  }
  while (i != numBits);
  return v;
}


static const unsigned MAX_UNPACK_FILTERS = 8192;

HRESULT CDecoder::AddFilter(CBitDecoder &_bitStream)
{
  DeleteUnusedFilters();

  if (_numFilters >= MAX_UNPACK_FILTERS)
  {
    RINOK(WriteBuf())
    DeleteUnusedFilters();
    if (_numFilters >= MAX_UNPACK_FILTERS)
    {
      _unsupportedFilter = true;
      InitFilters();
    }
  }

  _bitStream.Prepare();

  CFilter f;
  const UInt32 blockStart = ReadUInt32(_bitStream);
  f.Size = ReadUInt32(_bitStream);

  if (f.Size > k_Filter_BlockSize_MAX)
  {
    _unsupportedFilter = true;
    f.Size = 0;  // unrar 5.5.5
  }

  f.Type = (Byte)_bitStream.ReadBits_9fix(3);
  f.Channels = 0;
  if (f.Type == FILTER_DELTA)
    f.Channels = (Byte)(_bitStream.ReadBits_9fix(5) + 1);
  f.Start = _lzSize + _winPos + blockStart;

#if 0
  static unsigned z_cnt = 0; if (z_cnt++ % 100 == 0)
    printf ("\nFilter %7u : %4u : %8p, st=%8x, size=%8x, type=%u ch=%2u",
      z_cnt, (unsigned)_filters.Size(), (void *)(size_t)(_lzSize + _winPos),
      (unsigned)blockStart, (unsigned)f.Size, (unsigned)f.Type, (unsigned)f.Channels);
#endif

  if (f.Start < _filterEnd)
    _unsupportedFilter = true;
  else
  {
    _filterEnd = f.Start + f.Size;
    if (f.Size != 0)
    {
      if (!_filters)
      {
        _filters = (CFilter *)z7_AlignedAlloc(MAX_UNPACK_FILTERS * sizeof(CFilter));
        if (!_filters)
          return E_OUTOFMEMORY;
      }
      // printf("\n_numFilters = %6u\n", _numFilters);
      const unsigned i = _numFilters++;
      _filters[i] = f;
    }
  }

  return S_OK;
}


#define RIF(x) { if (!(x)) return S_FALSE; }

#if 1
#define PRINT_CNT(name, skip)
#else
#define PRINT_CNT(name, skip) \
  { static unsigned g_cnt = 0; if (g_cnt++ % skip == 0) printf("\n%16s:  %8u", name, g_cnt); }
#endif

HRESULT CDecoder::ReadTables(CBitDecoder &_bitStream)
{
  if (_progress)
  {
    const UInt64 packSize = _bitStream.GetProcessedSize();
    if (packSize - _progress_Pack >= (1u << 24)
        || _writtenFileSize - _progress_Unpack >= (1u << 26))
    {
      _progress_Pack = packSize;
      _progress_Unpack = _writtenFileSize;
      RINOK(_progress->SetRatioInfo(&_progress_Pack, &_writtenFileSize))
    }
    // printf("\ntable read pos=%p packSize=%p _writtenFileSize = %p\n", (size_t)_winPos, (size_t)packSize, (size_t)_writtenFileSize);
  }

  // _bitStream is aligned already
  _bitStream.Prepare();
  {
    const unsigned flags = _bitStream.ReadByte_InAligned();
    /* ((flags & 20) == 0) in all rar archives now,
       but (flags & 20) flag can be used as some decoding hint in future versions of original rar.
       So we ignore that bit here. */
    unsigned checkSum = _bitStream.ReadByte_InAligned();
    checkSum ^= flags;
    const unsigned num = (flags >> 3) & 3;
    if (num >= 3)
      return S_FALSE;
    UInt32 blockSize = _bitStream.ReadByte_InAligned();
    checkSum ^= blockSize;
    if (num != 0)
    {
      {
        const unsigned b = _bitStream.ReadByte_InAligned();
        checkSum ^= b;
        blockSize += (UInt32)b << 8;
      }
      if (num > 1)
      {
        const unsigned b = _bitStream.ReadByte_InAligned();
        checkSum ^= b;
        blockSize += (UInt32)b << 16;
      }
    }
    if (checkSum != 0x5A)
      return S_FALSE;
    unsigned blockSizeBits7 = (flags & 7) + 1;
    blockSize += (UInt32)(blockSizeBits7 >> 3);
    if (blockSize == 0)
    {
      // it's error in data stream
      // but original-unrar ignores that error
      _bitStream._minorError = true;
#if 1
      // we ignore that error as original-unrar:
      blockSizeBits7 = 0;
      blockSize = 1;
#else
      // we can stop decoding:
      return S_FALSE;
#endif
    }
    blockSize--;
    blockSizeBits7 &= 7;
    PRINT_CNT("Blocks", 100)
    /*
    {
      static unsigned g_prev = 0;
      static unsigned g_cnt = 0;
      unsigned proc = unsigned(_winPos);
      if (g_cnt++ % 100 == 0) printf("  c_size = %8u  ", blockSize);
      if (g_cnt++ % 100 == 1) printf("  unp_size = %8u", proc - g_prev);
      g_prev = proc;
    }
    */
    _bitStream._blockEndBits7 = blockSizeBits7;
    _bitStream._blockEnd = _bitStream.GetProcessedSize_Round() + blockSize;
    _bitStream.SetCheck_forBlock();
    _isLastBlock = ((flags & 0x40) != 0);
    if ((flags & 0x80) == 0)
    {
      if (!_tableWasFilled)
        // if (blockSize != 0 || blockSizeBits7 != 0)
        if (blockSize + blockSizeBits7 != 0)
          return S_FALSE;
      return S_OK;
    }
    _tableWasFilled = false;
  }

  PRINT_CNT("Tables", 100);

  const unsigned kLevelTableSize = 20;
  const unsigned k_NumHufTableBits_Level = 6;
  NHuffman::CDecoder256<kNumHufBits, kLevelTableSize, k_NumHufTableBits_Level> m_LevelDecoder;
  const unsigned kTablesSizesSum_MAX = kMainTableSize + kDistTableSize_MAX + kAlignTableSize + kLenTableSize;
  Byte lens[kTablesSizesSum_MAX];
  {
    // (kLevelTableSize + 16 < kTablesSizesSum). So we use lens[] array for (Level) table
    // Byte lens2[kLevelTableSize + 16];
    unsigned i = 0;
    do
    {
      if (_bitStream._buf >= _bitStream._bufCheck_Block)
      {
        _bitStream.Prepare();
        if (_bitStream.IsBlockOverRead())
          return S_FALSE;
      }
      const unsigned len = (unsigned)_bitStream.ReadBits_9fix(4);
      if (len == 15)
      {
        unsigned num = (unsigned)_bitStream.ReadBits_9fix(4);
        if (num != 0)
        {
          num += 2;
          num += i;
          // we are allowed to overwrite to lens[] for extra 16 bytes after kLevelTableSize
#if 0
          if (num > kLevelTableSize)
          {
            // we ignore this error as original-unrar
            num = kLevelTableSize;
            // return S_FALSE;
          }
#endif
          do
            lens[i++] = 0;
          while (i < num);
          continue;
        }
      }
      lens[i++] = (Byte)len;
    }
    while (i < kLevelTableSize);
    if (_bitStream.IsBlockOverRead())
      return S_FALSE;
    RIF(m_LevelDecoder.Build(lens, NHuffman::k_BuildMode_Full))
  }

  unsigned i = 0;
  const unsigned tableSize = _is_v7 ?
      kTablesSizesSum_MAX :
      kTablesSizesSum_MAX - kExtraDistSymbols_v7;
  do
  {
    if (_bitStream._buf >= _bitStream._bufCheck_Block)
    {
      // if (_bitStream._buf >= _bitStream._bufCheck)
      _bitStream.Prepare();
      if (_bitStream.IsBlockOverRead())
        return S_FALSE;
    }
    const unsigned sym = m_LevelDecoder.DecodeFull(&_bitStream);
    if (sym < 16)
      lens[i++] = (Byte)sym;
#if 0
    else if (sym > kLevelTableSize)
      return S_FALSE;
#endif
    else
    {
      unsigned num = ((sym /* - 16 */) & 1) * 4;
      num += num + 3 + (unsigned)_bitStream.ReadBits9(num + 3);
      num += i;
      if (num > tableSize)
      {
        // we ignore this error as original-unrar
        num = tableSize;
        // return S_FALSE;
      }
      unsigned v = 0;
      if (sym < 16 + 2)
      {
        if (i == 0)
          return S_FALSE;
        v = lens[(size_t)i - 1];
      }
      do
        lens[i++] = (Byte)v;
      while (i < num);
    }
  }
  while (i < tableSize);

  if (_bitStream.IsBlockOverRead())
    return S_FALSE;
  if (_bitStream.InputEofError())
    return S_FALSE;

  /* We suppose that original-rar encoder can create only two cases for Huffman:
      1) Empty Huffman tree (if num_used_symbols == 0)
      2) Full  Huffman tree (if num_used_symbols != 0)
     Usually the block contains at least one symbol for m_MainDecoder.
     So original-rar-encoder creates full Huffman tree for m_MainDecoder.
     But we suppose that (num_used_symbols == 0) is possible for m_MainDecoder,
     because file must be finished with (_isLastBlock) flag,
     even if there are no symbols in m_MainDecoder.
     So we use k_BuildMode_Full_or_Empty for m_MainDecoder.
  */
  const NHuffman::enum_BuildMode buildMode = NHuffman::
      k_BuildMode_Full_or_Empty; // strict check
      // k_BuildMode_Partial;    // non-strict check (ignore errors)

  RIF(m_MainDecoder.Build(&lens[0], buildMode))
  if (!_is_v7)
  {
#if 1
    /* we use this manual loop to avoid compiler BUG.
       GCC 4.9.2 compiler has BUG with overlapping memmove() to right in local array. */
    Byte *dest = lens + kMainTableSize + kDistTableSize_v6 +
                   kAlignTableSize + kLenTableSize - 1;
    unsigned num = kAlignTableSize + kLenTableSize;
    do
    {
      dest[kExtraDistSymbols_v7] = dest[0];
      dest--;
    }
    while (--num);
#else
    memmove(lens + kMainTableSize + kDistTableSize_v6 + kExtraDistSymbols_v7,
            lens + kMainTableSize + kDistTableSize_v6,
            kAlignTableSize + kLenTableSize);
#endif
    memset(lens + kMainTableSize + kDistTableSize_v6, 0, kExtraDistSymbols_v7);
  }

  RIF(m_DistDecoder.Build(&lens[kMainTableSize], buildMode))
  RIF( m_LenDecoder.Build(&lens[kMainTableSize
        + kDistTableSize_MAX + kAlignTableSize], buildMode))

  _useAlignBits = false;
  for (i = 0; i < kAlignTableSize; i++)
    if (lens[kMainTableSize + kDistTableSize_MAX + (size_t)i] != kNumAlignBits)
    {
      RIF(m_AlignDecoder.Build(&lens[kMainTableSize + kDistTableSize_MAX], buildMode))
      _useAlignBits = true;
      break;
    }

  _tableWasFilled = true;
  return S_OK;
}

static inline CLenType SlotToLen(CBitDecoder &_bitStream, CLenType slot)
{
  const unsigned numBits = ((unsigned)slot >> 2) - 1;
  return ((4 | (slot & 3)) << numBits) + (CLenType)_bitStream.ReadBits9(numBits);
}


static const unsigned kSymbolRep = 258;
static const unsigned kMaxMatchLen = 0x1001 + 3;

enum enum_exit_type
{
  Z7_RAR_EXIT_TYPE_NONE,
  Z7_RAR_EXIT_TYPE_ADD_FILTER
};


#define LZ_RESTORE \
{ \
  _reps[0] = rep0; \
  _winPos = (size_t)(winPos - _window); \
  _buf_Res = _bitStream._buf; \
  _bitPos_Res = _bitStream._bitPos; \
}

#define LZ_LOOP_BREAK_OK { break; }
// #define LZ_LOOP_BREAK_ERROR { _lzError = LZ_ERROR_TYPE_SYM; break; }
// #define LZ_LOOP_BREAK_ERROR { LZ_RESTORE; return S_FALSE; }
#define LZ_LOOP_BREAK_ERROR { goto decode_error; }
// goto decode_error; }
// #define LZ_LOOP_BREAK_ERROR { break; }

#define Z7_RAR_HUFF_DECODE_CHECK_break(sym, huf, kNumTableBits, bitStream) \
  Z7_HUFF_DECODE_CHECK(sym, huf, kNumHufBits, kNumTableBits, bitStream, { LZ_LOOP_BREAK_ERROR })


/*
  DecodeLZ2() will stop decoding if it reaches limit when (_winPos >= _limit)
  at return:
    (_winPos < _limit + kMaxMatchLen)
    also it can write up to (COPY_CHUNK_SIZE - 1) additional junk bytes after (_winPos).
*/
HRESULT CDecoder::DecodeLZ2(const CBitDecoder &bitStream) throw()
{
#if 0
  Byte k_LenPlusTable_LOC[DICT_SIZE_BITS_MAX];
  memcpy(k_LenPlusTable_LOC, k_LenPlusTable, sizeof(k_LenPlusTable));
#endif

  PRINT_CNT("DecodeLZ2", 2000);

  CBitDecoder _bitStream;
  _bitStream.CopyFrom(bitStream);
  // _bitStream._stream = _inStream;
  // _bitStream._bufBase = _inputBuf;
  // _bitStream.Init();

  // _reps[*] can be larger than _winSize, if _winSize was reduced in solid stream.
  size_t rep0 = _reps[0];
  // size_t rep1 = _reps[1];
  // Byte *win = _window;
  Byte *winPos = _window + _winPos;
  const Byte *limit = _window + _limit;
  _exitType = Z7_RAR_EXIT_TYPE_NONE;

  for (;;)
  {
    if (winPos >= limit)
      LZ_LOOP_BREAK_OK
    // (winPos < limit)
    if (_bitStream._buf >= _bitStream._bufCheck_Block)
    {
      if (_bitStream.InputEofError())
        LZ_LOOP_BREAK_OK
      if (_bitStream._buf >= _bitStream._bufCheck)
      {
        if (!_bitStream._wasFinished)
          LZ_LOOP_BREAK_OK
        // _bitStream._wasFinished == true
        // we don't need Prepare() here, because all data was read
        // and PadZone (16 bytes) after data was filled.
      }
      const UInt64 processed = _bitStream.GetProcessedSize_Round();
      // some cases are error, but the caller will process such error cases.
      if (processed >= _bitStream._blockEnd &&
          (processed > _bitStream._blockEnd
            || _bitStream.GetProcessedBits7() >= _bitStream._blockEndBits7))
          LZ_LOOP_BREAK_OK
      // that check is not required, but it can help, if there is BUG in another code
      if (!_tableWasFilled)
        LZ_LOOP_BREAK_ERROR
    }
    
#if 0
    const unsigned sym = m_MainDecoder.Decode(&_bitStream);
#else
    unsigned sym;
    Z7_RAR_HUFF_DECODE_CHECK_break(sym, &m_MainDecoder, k_NumHufTableBits_Main, &_bitStream)
#endif
    
    if (sym < 256)
    {
      *winPos++ = (Byte)sym;
      // _lzSize++;
      continue;
    }
   
    CLenType len;

    if (sym < kSymbolRep + kNumReps)
    {
      if (sym >= kSymbolRep)
      {
        if (sym != kSymbolRep)
        {
          size_t dist = _reps[1];
          _reps[1] = rep0;
          rep0 = dist;
          if (sym >= kSymbolRep + 2)
          {
            #if 1
              rep0 = _reps[(size_t)sym - kSymbolRep];
              _reps[(size_t)sym - kSymbolRep] = _reps[2];
              _reps[2] = dist;
            #else
              if (sym != kSymbolRep + 2)
              {
                rep0 = _reps[3];
                _reps[3] = _reps[2];
                _reps[2] = dist;
              }
              else
              {
                rep0 = _reps[2];
                _reps[2] = dist;
              }
            #endif
          }
        }
#if 0
        len = m_LenDecoder.Decode(&_bitStream);
        if (len >= kLenTableSize)
          LZ_LOOP_BREAK_ERROR
#else
        Z7_RAR_HUFF_DECODE_CHECK_break(len, &m_LenDecoder, k_NumHufTableBits_Len, &_bitStream)
#endif
        if (len >= 8)
          len = SlotToLen(_bitStream, len);
        len += 2;
        // _lastLen = (UInt32)len;
      }
      else if (sym != 256)
      {
        len = (CLenType)_lastLen;
        if (len == 0)
        {
          // we ignore (_lastLen == 0) case, like original-unrar.
          // that case can mean error in stream.
          // lzError = true;
          // return S_FALSE;
          continue;
        }
      }
      else
      {
        _exitType = Z7_RAR_EXIT_TYPE_ADD_FILTER;
        LZ_LOOP_BREAK_OK
      }
    }
#if 0
    else if (sym >= kMainTableSize)
      LZ_LOOP_BREAK_ERROR
#endif
    else
    {
      _reps[3] = _reps[2];
      _reps[2] = _reps[1];
      _reps[1] = rep0;
      len = sym - (kSymbolRep + kNumReps);
      if (len >= 8)
        len = SlotToLen(_bitStream, len);
      len += 2;
      // _lastLen = (UInt32)len;
      
#if 0
      rep0 = (UInt32)m_DistDecoder.Decode(&_bitStream);
#else
      Z7_RAR_HUFF_DECODE_CHECK_break(rep0, &m_DistDecoder, k_NumHufTableBits_Dist, &_bitStream)
#endif

      if (rep0 >= 4)
      {
#if 0
        if (rep0 >= kDistTableSize_MAX)
          LZ_LOOP_BREAK_ERROR
#endif
        const unsigned numBits = ((unsigned)rep0 - 2) >> 1;
        rep0 = (2 | (rep0 & 1)) << numBits;

        const Byte *buf = _bitStream._buf;
#ifdef Z7_RAR5_USE_64BIT
        const UInt64 v = GetBe64(buf);
#else
        const UInt32 v = GetBe32(buf);
#endif

        // _lastLen = (UInt32)len;
        if (numBits < kNumAlignBits)
        {
          rep0 += // _bitStream.ReadBits9(numBits);
            _bitStream.ReadBits_Big25(numBits, v);
        }
        else
        {
          #if !defined(MY_CPU_AMD64)
            len += k_LenPlusTable[numBits];
          #elif 0
            len += k_LenPlusTable_LOC[numBits];
          #elif 1
            len += m_LenPlusTable[numBits];
          #elif 1 && defined(MY_CPU_64BIT) && defined(MY_CPU_AMD64)
            // len += (unsigned)((UInt64)0xfffffffeaa554000 >> (numBits * 2)) & 3;
            len += (unsigned)((UInt64)0xfffffffffeaa5540 >> (numBits * 2 - 8)) & 3;
          #elif 1
            len += 3;
            len -= (unsigned)(numBits -  7) >> (sizeof(unsigned) * 8 - 1);
            len -= (unsigned)(numBits - 12) >> (sizeof(unsigned) * 8 - 1);
            len -= (unsigned)(numBits - 17) >> (sizeof(unsigned) * 8 - 1);
          #elif 1
            len += 3;
            len -= (0x155aabf >> (numBits - 4) >> (numBits - 4)) & 3;
          #elif 1
            len += (numBits >= 7);
            len += (numBits >= 12);
            len += (numBits >= 17);
          #endif
          // _lastLen = (UInt32)len;
          if (_useAlignBits)
          {
            // if (numBits > kNumAlignBits)
            rep0 += (_bitStream.ReadBits_Big25(numBits - kNumAlignBits, v) << kNumAlignBits);
#if 0
            const unsigned a = m_AlignDecoder.Decode(&_bitStream);
            if (a >= kAlignTableSize)
              LZ_LOOP_BREAK_ERROR
#else
            unsigned a;
            Z7_RAR_HUFF_DECODE_CHECK_break(a, &m_AlignDecoder, k_NumHufTableBits_Align, &_bitStream)
#endif
            rep0 += a;
          }
          else
            rep0 += _bitStream.ReadBits_Big(numBits, v);
#ifndef Z7_RAR5_USE_64BIT
          if (numBits >= 30) // we don't want 32-bit overflow case
            rep0 = (size_t)0 - 1 - 1;
#endif
        }
      }
      rep0++;
    }

    {
      _lastLen = (UInt32)len;
      // len != 0

#ifdef Z7_RAR5_SHOW_STAT
      {
        size_t index = rep0;
        if (index >= kNumStats1)
          index = kNumStats1 - 1;
        g_stats1[index]++;
        g_stats2[index][len]++;
      }
#endif

      Byte *dest = winPos;
      winPos += len;
      if (rep0 <= _dictSize_forCheck)
      {
        const Byte *src;
        const size_t winPos_temp = (size_t)(dest - _window);
        if (rep0 > winPos_temp)
        {
          if (_lzSize == 0)
            goto error_dist;
          size_t back = rep0 - winPos_temp;
          // STAT_INC(g_NumOver)
          src = dest + (_winSize - rep0);
          if (back < len)
          {
            // len -= (CLenType)back;
            Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
            do
              *dest++ = *src++;
            while (--back);
            src = dest - rep0;
          }
        }
        else
          src = dest - rep0;
        CopyMatch(rep0, dest, src, winPos);
        continue;
      }

error_dist:
      // LZ_LOOP_BREAK_ERROR;
      _lzError = LZ_ERROR_TYPE_DIST;
      do
        *dest++ = 0;
      while (dest < winPos);
      continue;
    }
  }

  LZ_RESTORE
  return S_OK;

#if 1
decode_error:
  /*
  if (_bitStream._hres != S_OK)
    return _bitStream._hres;
  */
  LZ_RESTORE
  return S_FALSE;
#endif
}



/*
input conditions:
  _winPos < _winSize
return:
  _winPos <  _winSize is expected, if (return_res == S_OK)
  _winPos >= _winSize is possible in (return_res != S_OK)
*/
HRESULT CDecoder::DecodeLZ()
{
  CBitDecoder _bitStream;
  _bitStream._stream = _inStream;
  _bitStream._bufBase = _inputBuf;
  _bitStream.Init();

  // _reps[*] can be larger than _winSize, if _winSize was reduced in solid stream.
  size_t winPos = _winPos;
  Byte *win = _window;
  size_t limit;
  {
    size_t rem = _winSize - winPos;
    if (rem > kWriteStep)
        rem = kWriteStep;
    limit = winPos + rem;
  }
  
  for (;;)
  {
    if (winPos >= limit)
    {
      _winPos = winPos < _winSize ? winPos : _winSize;
      // _winPos == min(winPos, _winSize)
      // we will not write data after _winSize
      RINOK(WriteBuf())
      if (_unpackSize_Defined && _writtenFileSize > _unpackSize)
        break; // return S_FALSE;
      const size_t wp = _winPos;
      size_t rem = _winSize - wp;
      if (rem == 0)
      {
        _lzSize += wp;
        winPos -= wp;
        // (winPos < kMaxMatchLen < _winSize)
        // so memmove is not required here
        if (winPos)
          memcpy(win, win + _winSize, winPos);
        limit = _winSize;
        if (limit >= kWriteStep)
        {
          limit = kWriteStep;
          continue;
        }
        rem = _winSize - winPos;
      }
      if (rem > kWriteStep)
          rem = kWriteStep;
      limit = winPos + rem;
      continue;
    }

    // (winPos < limit)

    if (_bitStream._buf >= _bitStream._bufCheck_Block)
    {
      _winPos = winPos;
      if (_bitStream.InputEofError())
        break; // return S_FALSE;
      _bitStream.Prepare();

      const UInt64 processed = _bitStream.GetProcessedSize_Round();
      if (processed >= _bitStream._blockEnd)
      {
        if (processed > _bitStream._blockEnd)
          break; // return S_FALSE;
        {
          const unsigned bits7 = _bitStream.GetProcessedBits7();
          if (bits7 >= _bitStream._blockEndBits7)
          {
            if (bits7 > _bitStream._blockEndBits7)
            {
#if 1
              // we ignore thar error as original unrar
              _bitStream._minorError = true;
#else
              break; // return S_FALSE;
#endif
            }
            _bitStream.AlignToByte();
            // if (!_bitStream.AlignToByte()) break;
            if (_isLastBlock)
            {
              if (_bitStream.InputEofError())
                break;
              /*
              // packSize can be 15 bytes larger for encrypted archive
              if (_packSize_Defined && _packSize < _bitStream.GetProcessedSize())
                break;
              */
              if (_bitStream._minorError)
                return S_FALSE;
              return _bitStream._hres;
              // break;
            }
            RINOK(ReadTables(_bitStream))
            continue;
          }
        }
      }

      // end of block was not reached.
      // so we must decode more symbols
      // that check is not required, but it can help, if there is BUG in another code
      if (!_tableWasFilled)
        break; // return S_FALSE;
    }

    _limit = limit;
    _winPos = winPos;
    RINOK(DecodeLZ2(_bitStream))
    _bitStream._buf = _buf_Res;
    _bitStream._bitPos = _bitPos_Res;

    winPos = _winPos;
    if (_exitType == Z7_RAR_EXIT_TYPE_ADD_FILTER)
    {
      RINOK(AddFilter(_bitStream))
      continue;
    }
  }

  _winPos = winPos;
  
  if (_bitStream._hres != S_OK)
    return _bitStream._hres;

  return S_FALSE;
}



HRESULT CDecoder::CodeReal()
{
  _unsupportedFilter = false;
  _writeError = false;
  /*
  if (!_isSolid || !_wasInit)
  {
    _wasInit = true;
    // _lzSize = 0;
    _lzWritten = 0;
    _winPos = 0;
    for (unsigned i = 0; i < kNumReps; i++)
      _reps[i] = (size_t)0 - 1;
    _lastLen = 0;
    _tableWasFilled = false;
  }
  */
  _isLastBlock = false;

  InitFilters();

  _filterEnd = 0;
  _writtenFileSize = 0;
  const UInt64 lzSize = _lzSize + _winPos;
  _lzFileStart = lzSize;
  _lzWritten = lzSize;
  
  HRESULT res = DecodeLZ();

  HRESULT res2 = S_OK;
  if (!_writeError && res != E_OUTOFMEMORY)
    res2 = WriteBuf();
  /*
  if (res == S_OK)
    if (InputEofError())
      res = S_FALSE;
  */
  if (res == S_OK)
  {
    // _solidAllowed = true;
    res = res2;
  }
  if (res == S_OK && _unpackSize_Defined && _writtenFileSize != _unpackSize)
    return S_FALSE;
  return res;
}



Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  _lzError = LZ_ERROR_TYPE_NO;
/*
  if file is soild, but decoding of previous file was not finished,
  we still try to decode new file.
  We need correct huffman table at starting block.
  And rar encoder probably writes huffman table at start block, if file is big.
  So we have good chance to get correct huffman table in some file after corruption.
  Also we try to recover window by filling zeros, if previous file
  was decoded to smaller size than required.
  But if filling size is big, we do full reset of window instead.
*/
  #define Z7_RAR_RECOVER_SOLID_LIMIT (1 << 20)
  // #define Z7_RAR_RECOVER_SOLID_LIMIT 0 // do not fill zeros
  {
    // if (_winPos > 100) _winPos -= 100; // for debug: corruption
    const UInt64 lzSize = _lzSize + _winPos;
/*
    if previous file was decoded with error or for some another cases, then
        (lzSize > _lzEnd)    is possible
        (_winPos > _winSize) is possible
        (_winPos < _winSize + kMaxMatchLen)
*/
    if (!_window
        || !_isSolid
        || !_wasInit
        || (lzSize < _lzEnd
#if Z7_RAR_RECOVER_SOLID_LIMIT != 0
         && lzSize + Z7_RAR_RECOVER_SOLID_LIMIT < _lzEnd
#endif
        ))
    {
      if (_isSolid)
        _lzError = LZ_ERROR_TYPE_HEADER;
      _lzSize = 0;
      // _lzEnd = 0;     // it will be set later
      // _lzWritten = 0; // it will be set later
      _winPos = 0;
      for (unsigned i = 0; i < kNumReps; i++)
        _reps[i] = (size_t)0 - 1;
      _lastLen = 0;
      _tableWasFilled = false;
      _wasInit = true;
    }
    else
    {
      const size_t ws = _winSize;
      if (_winPos >= ws)
      {
        // we must normalize (_winPos) and data in _window,
        _winPos -= ws;
        _lzSize += ws;
        // (_winPos < kMaxMatchLen < _winSize)
        // if (_window)
          memcpy(_window, _window + ws, _winPos); // memmove is not required here
      }

#if Z7_RAR_RECOVER_SOLID_LIMIT != 0
      if (lzSize < _lzEnd)
      {
#if 0
        return S_FALSE;
#else
        // we can report that recovering was made:
        // _lzError = LZ_ERROR_TYPE_HEADER;
        // We write zeros to area after corruption:
        // if (_window)
        {
          UInt64 rem = _lzEnd - lzSize;
          if (rem >= ws)
          {
            My_ZeroMemory(_window, ws);
            _lzSize = ws;
            _winPos = 0;
          }
          else
          {
            // rem < _winSize
            // _winPos <= ws
            const size_t cur = ws - _winPos;
            if (cur <= rem)
            {
              rem -= cur;
              My_ZeroMemory(_window + _winPos, cur);
              _lzSize = ws;
              _winPos = 0;
            }
            My_ZeroMemory(_window + _winPos, (size_t)rem);
            _winPos += (size_t)rem;
          }
        }
        // else return S_FALSE;
#endif
      }
    }
#endif
  }

  // _winPos < _winSize
  // we don't want _lzSize overflow
  if (_lzSize >= DICT_SIZE_MAX)
      _lzSize  = DICT_SIZE_MAX;
  _lzEnd = _lzSize + _winPos;
  // _lzSize <= DICT_SIZE_MAX
  // _lzEnd  <  DICT_SIZE_MAX + _winSize

  size_t newSize = _dictSize;
  if (newSize < kWinSize_Min)
      newSize = kWinSize_Min;
  
  _unpackSize = 0;
  _unpackSize_Defined = (outSize != NULL);
  if (_unpackSize_Defined)
    _unpackSize = *outSize;
  
  if ((Int64)_unpackSize >= 0)
    _lzEnd += _unpackSize; // known end after current file
  else
    _lzEnd = 0; // unknown end
  
  if (_isSolid && _window)
  {
    // If dictionary was decreased in solid, we use old dictionary.
    if (newSize > _dictSize_forCheck)
    {
      // If dictionary was increased in solid, we don't want grow.
      return S_FALSE; // E_OUTOFMEMORY
    }
    // (newSize <= _dictSize_forCheck)
  }
  else
  {
    // !_isSolid || !_window
    _dictSize_forCheck = newSize;
    {
      size_t newSize_small = newSize;
      const size_t k_Win_AlignSize = 1u << 18;
      /* here we add (1 << 7) instead of (COPY_CHUNK_SIZE - 1), because
      we want to get same (_winSize) for different COPY_CHUNK_SIZE values. */
      // newSize += (COPY_CHUNK_SIZE - 1) + (k_Win_AlignSize - 1); // for debug : we can get smallest (_winSize)
      newSize += (1 << 7) + k_Win_AlignSize;
      newSize &= ~(size_t)(k_Win_AlignSize - 1);
      if (newSize < newSize_small)
        return E_OUTOFMEMORY;
    }
    // (!_isSolid || !_window)
    const size_t allocSize = newSize + kMaxMatchLen + 64;
    if (allocSize < newSize)
      return E_OUTOFMEMORY;
    if (!_window || allocSize > _winSize_Allocated)
    {
      Z7_RAR_FREE_WINDOW
      _window = NULL;
      _winSize_Allocated = 0;
      Byte *win = (Byte *)::BigAlloc(allocSize);
      if (!win)
        return E_OUTOFMEMORY;
      _window = win;
      _winSize_Allocated = allocSize;
    }
    _winSize = newSize;
  }
  
  if (!_inputBuf)
  {
    _inputBuf = (Byte *)z7_AlignedAlloc(kInputBufSize + kInputBufferPadZone);
    if (!_inputBuf)
      return E_OUTOFMEMORY;
  }
  
  _inStream = inStream;
  _outStream = outStream;
  _progress = progress;
  _progress_Pack = 0;
  _progress_Unpack = 0;
  
  const HRESULT res = CodeReal();
  
  if (res != S_OK)
    return res;
  // _lzError = LZ_ERROR_TYPE_HEADER; // for debug
  if (_lzError)
    return S_FALSE;
  if (_unsupportedFilter)
    return E_NOTIMPL;
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  if (size != 2)
    return E_INVALIDARG;
  const unsigned pow = data[0];
  const unsigned b1 = data[1];
  const unsigned frac = b1 >> 3;
  // unsigned pow = 15 + 8;
  // unsigned frac = 1;
  if (pow + ((frac + 31) >> 5) > MAX_DICT_LOG - 17)
  // if (frac + (pow << 8) >= ((8 * 2 + 7) << 5) + 8 / 8)
    return E_NOTIMPL;
  _dictSize = (size_t)(frac + 32) << (pow + 12);
  _isSolid = (b1 & 1) != 0;
  _is_v7   = (b1 & 2) != 0;
  // printf("\ndict size = %p\n", (void *)(size_t)_dictSize);
  return S_OK;
}

}}

/* ================ unit: CPP/7zip/Compress/ShrinkDecoder.cpp ================ */
// ShrinkDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

namespace NCompress {
namespace NShrink {

static const UInt32 kEmpty = 256; // kNumItems;
static const UInt32 kBufferSize = (1 << 18);
static const unsigned kNumMinBits = 9;

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  NBitl::CBaseDecoder<CInBuffer> inBuffer;
  COutBuffer outBuffer;

  if (!inBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  if (!outBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;

  inBuffer.SetStream(inStream);
  inBuffer.Init();

  outBuffer.SetStream(outStream);
  outBuffer.Init();

  {
    for (unsigned i = 0; i < kNumItems; i++)
      _parents[i] = kEmpty;
  }

  UInt64 outPrev = 0, inPrev = 0;
  unsigned numBits = kNumMinBits;
  unsigned head = 257;
  int lastSym = -1;
  Byte lastChar = 0;
  bool moreOut = false;

  HRESULT res = S_FALSE;

  for (;;)
  {
    _inProcessed = inBuffer.GetProcessedSize();
    const UInt64 nowPos = outBuffer.GetProcessedSize();

    bool eofCheck = false;

    if (outSize && nowPos >= *outSize)
    {
      if (!_fullStreamMode || moreOut)
      {
        res = S_OK;
        break;
      }
      eofCheck = true;
      // Is specSym(=256) allowed after end of stream ?
      // Do we need to read it here ?
    }

    if (progress)
    {
      if (nowPos - outPrev >= (1 << 20) || _inProcessed - inPrev >= (1 << 20))
      {
        outPrev = nowPos;
        inPrev = _inProcessed;
        res = progress->SetRatioInfo(&_inProcessed, &nowPos);
        if (res != SZ_OK)
        {
          // break;
          return res;
        }
      }
    }

    UInt32 sym = inBuffer.ReadBits(numBits);

    if (inBuffer.ExtraBitsWereRead())
    {
      res = S_OK;
      break;
    }
    
    if (sym == 256)
    {
      sym = inBuffer.ReadBits(numBits);

      if (inBuffer.ExtraBitsWereRead())
        break;

      if (sym == 1)
      {
        if (numBits >= kNumMaxBits)
          break;
        numBits++;
        continue;
      }
      if (sym != 2)
      {
        break;
        // continue; // info-zip just ignores such code
      }
      {
        /*
        ---------- Free leaf nodes ----------
        Note : that code can mark _parents[lastSym] as free, and next
        inserted node will be Orphan in that case.
        */

        unsigned i;
        for (i = 256; i < kNumItems; i++)
          _stack[i] = 0;
        for (i = 257; i < kNumItems; i++)
        {
          unsigned par = _parents[i];
          if (par != kEmpty)
            _stack[par] = 1;
        }
        for (i = 257; i < kNumItems; i++)
          if (_stack[i] == 0)
            _parents[i] = kEmpty;
        head = 257;
        continue;
      }
    }

    if (eofCheck)
    {
      // It's can be error case.
      // That error can be detected later in (*inSize != _inProcessed) check.
      res = S_OK;
      break;
    }

    bool needPrev = false;
    if (head < kNumItems && lastSym >= 0)
    {
      while (head < kNumItems && _parents[head] != kEmpty)
        head++;
      if (head < kNumItems)
      {
        /*
        if (head == lastSym), it updates Orphan to self-linked Orphan and creates two problems:
            1) we must check _stack[i++] overflow in code that walks tree nodes.
            2) self-linked node can not be removed. So such self-linked nodes can occupy all _parents items.
        */
        needPrev = true;
        _parents[head] = (UInt16)lastSym;
        _suffixes[head] = (Byte)lastChar;
        head++;
      }
    }

    lastSym = (int)sym;
    unsigned cur = sym;
    unsigned i = 0;
    
    while (cur >= 256)
    {
      _stack[i++] = _suffixes[cur];
      cur = _parents[cur];
      // don't change that code:
      // Orphan Check and self-linked Orphan check (_stack overflow check);
      if (cur == kEmpty || i >= kNumItems)
        break;
    }
    
    if (cur == kEmpty || i >= kNumItems)
      break;

    _stack[i++] = (Byte)cur;
    lastChar = (Byte)cur;

    if (needPrev)
      _suffixes[(size_t)head - 1] = (Byte)cur;

    if (outSize)
    {
      const UInt64 limit = *outSize - nowPos;
      if (i > limit)
      {
        moreOut = true;
        i = (unsigned)limit;
      }
    }

    do
      outBuffer.WriteByte(_stack[--i]);
    while (i);
  }
  
  RINOK(outBuffer.Flush())

  if (res == S_OK)
    if (_fullStreamMode)
    {
      if (moreOut)
        res = S_FALSE;
      const UInt64 nowPos = outBuffer.GetProcessedSize();
      if (outSize && *outSize != nowPos)
        res = S_FALSE;
      if (inSize && *inSize != _inProcessed)
        res = S_FALSE;
    }
  
  return res;
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  // catch(const CInBufferException &e) { return e.ErrorCode; }
  // catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(const CSystemException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  _fullStreamMode = (finishMode != 0);
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inProcessed;
  return S_OK;
}


}}

/* ================ unit: CPP/7zip/Compress/XpressDecoder.cpp ================ */
// XpressDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

#ifdef MY_CPU_LE_UNALIGN
  #define Z7_XPRESS_DEC_USE_UNALIGNED_COPY
#endif

#ifdef Z7_XPRESS_DEC_USE_UNALIGNED_COPY

  #define COPY_CHUNK_SIZE 16

    #define COPY_CHUNK_4_2(dest, src) \
    { \
      ((UInt32 *)(void *)dest)[0] = ((const UInt32 *)(const void *)src)[0]; \
      ((UInt32 *)(void *)dest)[1] = ((const UInt32 *)(const void *)src)[1]; \
      src += 4 * 2; \
      dest += 4 * 2; \
    }

  /* sse2 doesn't help here in GCC and CLANG.
     so we disabled sse2 here */
#if 0
  #if defined(MY_CPU_AMD64)
    #define Z7_XPRESS_DEC_USE_SSE2
  #elif defined(MY_CPU_X86)
    #if defined(_MSC_VER) && _MSC_VER >= 1300 && defined(_M_IX86_FP) && (_M_IX86_FP >= 2) \
      || defined(__SSE2__) \
      // || 1 == 1  // for debug only
      #define Z7_XPRESS_DEC_USE_SSE2
    #endif
  #endif
#endif

  #if defined(MY_CPU_ARM64)
  #include <arm_neon.h>
    #define COPY_OFFSET_MIN  16
    #define COPY_CHUNK1(dest, src) \
    { \
      vst1q_u8((uint8_t *)(void *)dest, \
      vld1q_u8((const uint8_t *)(const void *)src)); \
      src += 16; \
      dest += 16; \
    }
    
    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK1(dest, src) \
      if (dest >= dest_lim) break; \
      COPY_CHUNK1(dest, src) \
    }

  #elif defined(Z7_XPRESS_DEC_USE_SSE2)
    #include <emmintrin.h> // sse2
    #define COPY_OFFSET_MIN  16

    #define COPY_CHUNK1(dest, src) \
    { \
      _mm_storeu_si128((__m128i *)(void *)dest, \
      _mm_loadu_si128((const __m128i *)(const void *)src)); \
      src += 16; \
      dest += 16; \
    }

    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK1(dest, src) \
      if (dest >= dest_lim) break; \
      COPY_CHUNK1(dest, src) \
    }

  #elif defined(MY_CPU_64BIT)
    #define COPY_OFFSET_MIN  8

    #define COPY_CHUNK(dest, src) \
    { \
      ((UInt64 *)(void *)dest)[0] = ((const UInt64 *)(const void *)src)[0]; \
      ((UInt64 *)(void *)dest)[1] = ((const UInt64 *)(const void *)src)[1]; \
      src += 8 * 2; \
      dest += 8 * 2; \
    }

  #else
    #define COPY_OFFSET_MIN  4

    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK_4_2(dest, src); \
      COPY_CHUNK_4_2(dest, src); \
    }

  #endif
#endif


#ifndef COPY_CHUNK_SIZE
    #define COPY_OFFSET_MIN  4
    #define COPY_CHUNK_SIZE  8
    #define COPY_CHUNK_2(dest, src) \
    { \
      const Byte a0 = src[0]; \
      const Byte a1 = src[1]; \
      dest[0] = a0; \
      dest[1] = a1; \
      src += 2; \
      dest += 2; \
    }
    #define COPY_CHUNK(dest, src) \
    { \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
      COPY_CHUNK_2(dest, src) \
    }
#endif


#define COPY_CHUNKS \
{ \
  Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE \
  do { COPY_CHUNK(dest, src) } \
  while (dest < dest_lim); \
}


static
Z7_FORCE_INLINE
// Z7_ATTRIB_NO_VECTOR
void CopyMatch_1(Byte *dest, const Byte *dest_lim)
{
      const unsigned b0 = dest[-1];
      {
#if defined(Z7_XPRESS_DEC_USE_UNALIGNED_COPY) && (COPY_CHUNK_SIZE == 16)
        #if defined(MY_CPU_64BIT)
        {
          const UInt64 v64 = (UInt64)b0 * 0x0101010101010101;
          Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
          do
          {
            ((UInt64 *)(void *)dest)[0] = v64;
            ((UInt64 *)(void *)dest)[1] = v64;
            dest += 16;
          }
          while (dest < dest_lim);
        }
        #else
        {
          UInt32 v = b0;
          v |= v << 8;
          v |= v << 16;
          do
          {
            ((UInt32 *)(void *)dest)[0] = v;
            ((UInt32 *)(void *)dest)[1] = v;
            dest += 8;
            ((UInt32 *)(void *)dest)[0] = v;
            ((UInt32 *)(void *)dest)[1] = v;
            dest += 8;
          }
          while (dest < dest_lim);
        }
        #endif
#else
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = (Byte)b0;
          dest += 2;
          dest[0] = (Byte)b0;
          dest[1] = (Byte)b0;
          dest += 2;
        }
        while (dest < dest_lim);
#endif
      }
}


// (offset != 1)
static
Z7_FORCE_INLINE
// Z7_ATTRIB_NO_VECTOR
void CopyMatch_Non1(Byte *dest, size_t offset, const Byte *dest_lim)
{
  const Byte *src = dest - offset;
  {
    // (COPY_OFFSET_MIN >= 4)
    if (offset >= COPY_OFFSET_MIN)
    {
      COPY_CHUNKS
      // return;
    }
    else
#if (COPY_OFFSET_MIN > 4)
    #if COPY_CHUNK_SIZE < 8
      #error Stop_Compiling_Bad_COPY_CHUNK_SIZE
    #endif
    if (offset >= 4)
    {
      Z7_PRAGMA_OPT_DISABLE_LOOP_UNROLL_VECTORIZE
      do
      {
        COPY_CHUNK_4_2(dest, src)
        #if COPY_CHUNK_SIZE != 16
          if (dest >= dest_lim) break;
        #endif
        COPY_CHUNK_4_2(dest, src)
      }
      while (dest < dest_lim);
      // return;
    }
    else
#endif
    {
      // (offset < 4)
      if (offset == 2)
      {
#if defined(Z7_XPRESS_DEC_USE_UNALIGNED_COPY)
        UInt32 w0 = GetUi16(src);
        w0 += w0 << 16;
        do
        {
          SetUi32(dest, w0)
          dest += 4;
        }
        while (dest < dest_lim);
#else
        const unsigned b0 = src[0];
        const Byte b1 = src[1];
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = b1;
          dest += 2;
        }
        while (dest < dest_lim);
#endif
      }
      else // (offset == 3)
      {
        const unsigned b0 = src[0];
#if defined(Z7_XPRESS_DEC_USE_UNALIGNED_COPY)
        const unsigned w1 = GetUi16(src + 1);
        do
        {
          dest[0] = (Byte)b0;
          SetUi16(dest + 1, (UInt16)w1)
          dest += 3;
        }
        while (dest < dest_lim);
#else
        const Byte b1 = src[1];
        const Byte b2 = src[2];
        do
        {
          dest[0] = (Byte)b0;
          dest[1] = b1;
          dest[2] = b2;
          dest += 3;
        }
        while (dest < dest_lim);
#endif
      }
    }
  }
}


namespace NCompress {
namespace NXpress {

#define BIT_STREAM_NORMALIZE \
    if (BitPos > 16) { \
      if (in >= lim) return S_FALSE; \
      BitPos -= 16; \
      Value |= (UInt32)GetUi16(in) << BitPos; \
      in += 2; }

#define MOVE_POS(bs, numBits) \
    BitPos += (unsigned)numBits; \
    Value <<= numBits; \


static const unsigned kNumHuffBits = 15;
static const unsigned kNumTableBits = 10;
static const unsigned kNumLenBits = 4;
static const unsigned kLenMask = (1 << kNumLenBits) - 1;
static const unsigned kNumPosSlots = 16;
static const unsigned kNumSyms = 256 + (kNumPosSlots << kNumLenBits);

HRESULT Decode_WithExceedWrite(const Byte *in, size_t inSize, Byte *out, size_t outSize)
{
  NCompress::NHuffman::CDecoder<kNumHuffBits, kNumSyms, kNumTableBits> huff;
  
  if (inSize < kNumSyms / 2 + 4)
    return S_FALSE;
  {
    Byte levels[kNumSyms];
    for (unsigned i = 0; i < kNumSyms / 2; i++)
    {
      const unsigned b = in[i];
      levels[(size_t)i * 2    ] = (Byte)(b & 0xf);
      levels[(size_t)i * 2 + 1] = (Byte)(b >> 4);
    }
    if (!huff.Build(levels, NHuffman::k_BuildMode_Full))
      return S_FALSE;
  }

  UInt32 Value;
  unsigned BitPos;  // how many bits in (Value) were processed

  const Byte *lim = in + inSize - 1;  // points to last byte
  in += kNumSyms / 2;
#ifdef MY_CPU_LE_UNALIGN
  Value = GetUi32(in);
  Value = rotlFixed(Value, 16);
#else
  Value = ((UInt32)GetUi16(in) << 16) | GetUi16(in + 2);
#endif
    
  in += 4;
  BitPos = 0;
  Byte *dest = out;
  const Byte *outLim = out + outSize;

  for (;;)
  {
    unsigned sym;
    Z7_HUFF_DECODE_VAL_IN_HIGH32(sym, &huff, kNumHuffBits, kNumTableBits,
        Value, Z7_HUFF_DECODE_ERROR_SYM_CHECK_NO, {}, MOVE_POS, {}, bs)
    // 0 < BitPos <= 31
    BIT_STREAM_NORMALIZE
    // 0 < BitPos <= 16

    if (dest >= outLim)
      return (sym == 256 && Value == 0 && in == lim + 1) ? S_OK : S_FALSE;

    if (sym < 256)
      *dest++ = (Byte)sym;
    else
    {
      const unsigned distBits = (unsigned)(Byte)sym >> kNumLenBits; // (sym - 256) >> kNumLenBits;
      UInt32 len = (UInt32)(sym & kLenMask);
      
      if (len == kLenMask)
      {
        if (in > lim)
          return S_FALSE;
        // here we read input bytes in out-of-order related to main input stream (bits in Value):
        len = *in++;
        if (len == 0xff)
        {
          if (in >= lim)
            return S_FALSE;
          len = GetUi16(in);
          in += 2;
        }
        else
          len += kLenMask;
      }

      len += 3;
      if (len > (size_t)(outLim - dest))
        return S_FALSE;

      if (distBits == 0)
      {
        // d == 1
        if (dest == out)
          return S_FALSE;
        Byte *destTemp = dest;
        dest += len;
        CopyMatch_1(destTemp, dest);
      }
      else
      {
        unsigned d = (unsigned)(Value >> (32 - distBits));
        MOVE_POS(bs, distBits)
        d += 1u << distBits;
        // 0 < BitPos <= 31
        BIT_STREAM_NORMALIZE
        // 0 < BitPos <= 16
        if (d > (size_t)(dest - out))
          return S_FALSE;
        Byte *destTemp = dest;
        dest += len;
        CopyMatch_Non1(destTemp, d, dest);
      }
    }
  }
}

}}

/* ================ unit: CPP/7zip/Compress/XzDecoder.cpp ================ */
// XzDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NXz {

#define RET_IF_WRAP_ERROR_CONFIRMED(wrapRes, sRes, sResErrorCode) \
  if (wrapRes != S_OK && sRes == sResErrorCode) return wrapRes;

#define RET_IF_WRAP_ERROR(wrapRes, sRes, sResErrorCode) \
  if (wrapRes != S_OK /* && (sRes == SZ_OK || sRes == sResErrorCode) */) return wrapRes;

static HRESULT SResToHRESULT_Code(SRes res) throw()
{
  if (res < 0)
    return res;
  switch (res)
  {
    case SZ_OK: return S_OK;
    case SZ_ERROR_MEM: return E_OUTOFMEMORY;
    case SZ_ERROR_UNSUPPORTED: return E_NOTIMPL;
    default: break;
  }
  return S_FALSE;
}


HRESULT CDecoder::Decode(ISequentialInStream *seqInStream, ISequentialOutStream *outStream,
    const UInt64 *outSizeLimit, bool finishStream, ICompressProgressInfo *progress)
{
  MainDecodeSRes = SZ_OK;
  MainDecodeSRes_wasUsed = false;
  XzStatInfo_Clear(&Stat);

  if (!xz)
  {
    xz = XzDecMt_Create(&g_Alloc, &g_MidAlloc);
    if (!xz)
      return E_OUTOFMEMORY;
  }

  CXzDecMtProps props;
  XzDecMtProps_Init(&props);

  int isMT = False;

  #ifndef Z7_ST
  {
    props.numThreads = 1;
    const UInt32 numThreads = _numThreads;

    if (_tryMt && numThreads > 1)
    {
      size_t memUsage = (size_t)_memUsage;
      if (memUsage != _memUsage)
        memUsage = (size_t)0 - 1;
      props.memUseMax = memUsage;
      isMT = (numThreads > 1);
    }

    props.numThreads = numThreads;
  }
  #endif

  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;

  inWrap.Init(seqInStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);

  SRes res = XzDecMt_Decode(xz,
      &props,
      outSizeLimit, finishStream,
      &outWrap.vt,
      &inWrap.vt,
      &Stat,
      &isMT,
      progress ? &progressWrap.vt : NULL);

  MainDecodeSRes = res;

  #ifndef Z7_ST
  // _tryMt = isMT;
  #endif

  RET_IF_WRAP_ERROR(outWrap.Res, res, SZ_ERROR_WRITE)
  RET_IF_WRAP_ERROR(progressWrap.Res, res, SZ_ERROR_PROGRESS)
  RET_IF_WRAP_ERROR_CONFIRMED(inWrap.Res, res, SZ_ERROR_READ)

  // return E_OUTOFMEMORY; // for debug check

  MainDecodeSRes_wasUsed = true;

  if (res == SZ_OK && finishStream)
  {
    /*
    if (inSize && *inSize != Stat.PhySize)
      res = SZ_ERROR_DATA;
    */
    if (outSizeLimit && *outSizeLimit != outWrap.Processed)
      res = SZ_ERROR_DATA;
  }

  return SResToHRESULT_Code(res);
}


Z7_COM7F_IMF(CComDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  return Decode(inStream, outStream, outSize, _finishStream, progress);
}

Z7_COM7F_IMF(CComDecoder::SetFinishMode(UInt32 finishMode))
{
  _finishStream = (finishMode != 0);
  return S_OK;
}

Z7_COM7F_IMF(CComDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = Stat.InSize;
  return S_OK;
}

#ifndef Z7_ST

Z7_COM7F_IMF(CComDecoder::SetNumberOfThreads(UInt32 numThreads))
{
  _numThreads = numThreads;
  return S_OK;
}

Z7_COM7F_IMF(CComDecoder::SetMemLimit(UInt64 memUsage))
{
  _memUsage = memUsage;
  return S_OK;
}

#endif

}}

/* ================ unit: CPP/7zip/Compress/XzEncoder.cpp ================ */
// XzEncoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {

namespace NLzma2 {
HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props);
}

namespace NXz {

void CEncoder::InitCoderProps()
{
  XzProps_Init(&xzProps);
}

CEncoder::CEncoder()
{
  XzProps_Init(&xzProps);
  _encoder = NULL;
  _encoder = XzEnc_Create(&g_Alloc, &g_BigAlloc);
  if (!_encoder)
    throw 1;
}

CEncoder::~CEncoder()
{
  if (_encoder)
    XzEnc_Destroy(_encoder);
}


struct CMethodNamePair
{
  UInt32 Id;
  const char *Name;
};

static const CMethodNamePair g_NamePairs[] =
{
  { XZ_ID_Delta, "Delta" },
  { XZ_ID_X86, "BCJ" },
  { XZ_ID_PPC, "PPC" },
  { XZ_ID_IA64, "IA64" },
  { XZ_ID_ARM, "ARM" },
  { XZ_ID_ARMT, "ARMT" },
  { XZ_ID_SPARC, "SPARC" }
  // { XZ_ID_LZMA2, "LZMA2" }
};

static int FilterIdFromName(const wchar_t *name)
{
  for (unsigned i = 0; i < Z7_ARRAY_SIZE(g_NamePairs); i++)
  {
    const CMethodNamePair &pair = g_NamePairs[i];
    if (StringsAreEqualNoCase_Ascii(name, pair.Name))
      return (int)pair.Id;
  }
  return -1;
}


HRESULT CEncoder::SetCheckSize(UInt32 checkSizeInBytes)
{
  unsigned id;
  switch (checkSizeInBytes)
  {
    case  0: id = XZ_CHECK_NO; break;
    case  4: id = XZ_CHECK_CRC32; break;
    case  8: id = XZ_CHECK_CRC64; break;
    case 32: id = XZ_CHECK_SHA256; break;
    default: return E_INVALIDARG;
  }
  xzProps.checkId = id;
  return S_OK;
}


HRESULT CEncoder::SetCoderProp(PROPID propID, const PROPVARIANT &prop)
{
  if (propID == NCoderPropID::kNumThreads)
  {
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    xzProps.numTotalThreads = (int)(prop.ulVal);
    return S_OK;
  }

  if (propID == NCoderPropID::kCheckSize)
  {
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    return SetCheckSize(prop.ulVal);
  }

  if (propID == NCoderPropID::kBlockSize2)
  {
    if (prop.vt == VT_UI4)
      xzProps.blockSize = prop.ulVal;
    else if (prop.vt == VT_UI8)
      xzProps.blockSize = prop.uhVal.QuadPart;
    else
      return E_INVALIDARG;
    return S_OK;
  }

  if (propID == NCoderPropID::kReduceSize)
  {
    if (prop.vt == VT_UI8)
      xzProps.reduceSize = prop.uhVal.QuadPart;
    else
      return E_INVALIDARG;
    return S_OK;
  }
 
  if (propID == NCoderPropID::kFilter)
  {
    if (prop.vt == VT_UI4)
    {
      const UInt32 id32 = prop.ulVal;
      if (id32 == XZ_ID_Delta)
        return E_INVALIDARG;
      xzProps.filterProps.id = prop.ulVal;
    }
    else
    {
      if (prop.vt != VT_BSTR)
        return E_INVALIDARG;
      
      const wchar_t *name = prop.bstrVal;
      const wchar_t *end;

      UInt32 id32 = ConvertStringToUInt32(name, &end);
      
      if (end != name)
        name = end;
      else
      {
        if (IsString1PrefixedByString2_NoCase_Ascii(name, "Delta"))
        {
          name += 5; // strlen("Delta");
          id32 = XZ_ID_Delta;
        }
        else
        {
          const int filterId = FilterIdFromName(prop.bstrVal);
          if (filterId < 0 /* || filterId == XZ_ID_LZMA2 */)
            return E_INVALIDARG;
          id32 = (UInt32)(unsigned)filterId;
        }
      }
      
      if (id32 == XZ_ID_Delta)
      {
        const wchar_t c = *name;
        if (c != '-' && c != ':')
          return E_INVALIDARG;
        name++;
        const UInt32 delta = ConvertStringToUInt32(name, &end);
        if (end == name || *end != 0 || delta == 0 || delta > 256)
          return E_INVALIDARG;
        xzProps.filterProps.delta = delta;
      }
      
      xzProps.filterProps.id = id32;
    }
    
    return S_OK;
  }

  return NLzma2::SetLzma2Prop(propID, prop, xzProps.lzma2Props);
}


Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  XzProps_Init(&xzProps);

  for (UInt32 i = 0; i < numProps; i++)
  {
    RINOK(SetCoderProp(propIDs[i], coderProps[i]))
  }
  
  return S_OK;
  // return SResToHRESULT(XzEnc_SetProps(_encoder, &xzProps));
}


Z7_COM7F_IMF(CEncoder::SetCoderPropertiesOpt(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];
    if (propID == NCoderPropID::kExpectedDataSize)
      if (prop.vt == VT_UI8)
        XzEnc_SetDataSize(_encoder, prop.uhVal.QuadPart);
  }
  return S_OK;
}


#define RET_IF_WRAP_ERROR(wrapRes, sRes, sResErrorCode) \
  if (wrapRes != S_OK /* && (sRes == SZ_OK || sRes == sResErrorCode) */) return wrapRes;

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;

  inWrap.Init(inStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);

  SRes res = XzEnc_SetProps(_encoder, &xzProps);
  if (res == SZ_OK)
    res = XzEnc_Encode(_encoder, &outWrap.vt, &inWrap.vt, progress ? &progressWrap.vt : NULL);

  // SRes res = Xz_Encode(&outWrap.vt, &inWrap.vt, &xzProps, progress ? &progressWrap.vt : NULL);

  RET_IF_WRAP_ERROR(inWrap.Res, res, SZ_ERROR_READ)
  RET_IF_WRAP_ERROR(outWrap.Res, res, SZ_ERROR_WRITE)
  RET_IF_WRAP_ERROR(progressWrap.Res, res, SZ_ERROR_PROGRESS)

  return SResToHRESULT(res);
}
  
}}

/* ================ unit: CPP/7zip/Compress/ZDecoder.cpp ================ */
// ZDecoder.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NZ {

static const size_t kBufferSize = 1 << 20;
static const Byte kNumBitsMask = 0x1F;
static const Byte kBlockModeMask = 0x80;
static const unsigned kNumMinBits = 9;
static const unsigned kNumMaxBits = 16;

void CDecoder::Free()
{
  MyFree(_parents); _parents = NULL;
  MyFree(_suffixes); _suffixes = NULL;
  MyFree(_stack); _stack = NULL;
}

CDecoder::~CDecoder() { Free(); }

HRESULT CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    ICompressProgressInfo *progress)
{
  try {
  // PackSize = 0;

  CInBuffer inBuffer;
  COutBuffer outBuffer;

  if (!inBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  inBuffer.SetStream(inStream);
  inBuffer.Init();

  if (!outBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  outBuffer.SetStream(outStream);
  outBuffer.Init();

  Byte buf[kNumMaxBits + 4];
  {
    if (inBuffer.ReadBytes(buf, 3) < 3)
      return S_FALSE;
    if (buf[0] != 0x1F || buf[1] != 0x9D)
      return S_FALSE;
  }
  const Byte prop = buf[2];

  if ((prop & 0x60) != 0)
    return S_FALSE;
  const unsigned maxbits = prop & kNumBitsMask;
  if (maxbits < kNumMinBits || maxbits > kNumMaxBits)
    return S_FALSE;
  const UInt32 numItems = (UInt32)1 << maxbits;
  // Speed optimization: blockSymbol can contain unused velue.

  if (maxbits != _numMaxBits || !_parents || !_suffixes || !_stack)
  {
    Free();
    _parents = (UInt16 *)MyAlloc(numItems * sizeof(UInt16)); if (!_parents) return E_OUTOFMEMORY;
    _suffixes = (Byte *)MyAlloc(numItems * sizeof(Byte)); if (!_suffixes) return E_OUTOFMEMORY;
    _stack = (Byte *)MyAlloc(numItems * sizeof(Byte)); if (!_stack) return E_OUTOFMEMORY;
    _numMaxBits = maxbits;
  }

  UInt64 prevPos = 0;
  const UInt32 blockSymbol = ((prop & kBlockModeMask) != 0) ? 256 : ((UInt32)1 << kNumMaxBits);
  unsigned numBits = kNumMinBits;
  UInt32 head = (blockSymbol == 256) ? 257 : 256;
  bool needPrev = false;
  unsigned bitPos = 0;
  unsigned numBufBits = 0;

  _parents[256] = 0; // virus protection
  _suffixes[256] = 0;
  HRESULT res = S_OK;

  for (;;)
  {
    if (numBufBits == bitPos)
    {
      numBufBits = (unsigned)inBuffer.ReadBytes(buf, numBits) * 8;
      bitPos = 0;
      const UInt64 nowPos = outBuffer.GetProcessedSize();
      if (progress && nowPos - prevPos >= (1 << 13))
      {
        prevPos = nowPos;
        const UInt64 packSize = inBuffer.GetProcessedSize();
        RINOK(progress->SetRatioInfo(&packSize, &nowPos))
      }
    }
    const unsigned bytePos = bitPos >> 3;
    UInt32 symbol = buf[bytePos] | ((UInt32)buf[(size_t)bytePos + 1] << 8) | ((UInt32)buf[(size_t)bytePos + 2] << 16);
    symbol >>= (bitPos & 7);
    symbol &= ((UInt32)1 << numBits) - 1;
    bitPos += numBits;
    if (bitPos > numBufBits)
      break;
    if (symbol >= head)
    {
      res = S_FALSE;
      break;
    }
    if (symbol == blockSymbol)
    {
      numBufBits = bitPos = 0;
      numBits = kNumMinBits;
      head = 257;
      needPrev = false;
      continue;
    }
    UInt32 cur = symbol;
    unsigned i = 0;
    while (cur >= 256)
    {
      _stack[i++] = _suffixes[cur];
      cur = _parents[cur];
    }
    _stack[i++] = (Byte)cur;
    if (needPrev)
    {
      _suffixes[(size_t)head - 1] = (Byte)cur;
      if (symbol == head - 1)
        _stack[0] = (Byte)cur;
    }
    do
      outBuffer.WriteByte((_stack[--i]));
    while (i > 0);
    if (head < numItems)
    {
      needPrev = true;
      _parents[head++] = (UInt16)symbol;
      if (head > ((UInt32)1 << numBits))
      {
        if (numBits < maxbits)
        {
          numBufBits = bitPos = 0;
          numBits++;
        }
      }
    }
    else
      needPrev = false;
  }
  // PackSize = inBuffer.GetProcessedSize();
  const HRESULT res2 = outBuffer.Flush();
  return (res == S_OK) ? res2 : res;
 
  }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}


bool CheckStream(const Byte *data, size_t size)
{
  if (size < 3)
    return false;
  if (data[0] != 0x1F || data[1] != 0x9D)
    return false;
  const Byte prop = data[2];
  if ((prop & 0x60) != 0)
    return false;
  const unsigned maxbits = prop & kNumBitsMask;
  if (maxbits < kNumMinBits || maxbits > kNumMaxBits)
    return false;
  const UInt32 numItems = (UInt32)1 << maxbits;
  const UInt32 blockSymbol = ((prop & kBlockModeMask) != 0) ? 256 : ((UInt32)1 << kNumMaxBits);
  unsigned numBits = kNumMinBits;
  UInt32 head = (blockSymbol == 256) ? 257 : 256;
  unsigned bitPos = 0;
  unsigned numBufBits = 0;
  Byte buf[kNumMaxBits + 4];
  data += 3;
  size -= 3;
  // printf("\n\n");
  for (;;)
  {
    if (numBufBits == bitPos)
    {
      const unsigned num = (numBits < size) ? numBits : (unsigned)size;
      memcpy(buf, data, num);
      data += num;
      size -= num;
      numBufBits = num * 8;
      bitPos = 0;
    }
    const unsigned bytePos = bitPos >> 3;
    UInt32 symbol = buf[bytePos] | ((UInt32)buf[bytePos + 1] << 8) | ((UInt32)buf[bytePos + 2] << 16);
    symbol >>= (bitPos & 7);
    symbol &= ((UInt32)1 << numBits) - 1;
    bitPos += numBits;
    if (bitPos > numBufBits)
    {
      // printf("  OK", symbol);
      return true;
    }
    // printf("%3X ", symbol);
    if (symbol >= head)
      return false;
    if (symbol == blockSymbol)
    {
      numBufBits = bitPos = 0;
      numBits = kNumMinBits;
      head = 257;
      continue;
    }
    if (head < numItems)
    {
      head++;
      if (head > ((UInt32)1 << numBits))
      {
        if (numBits < maxbits)
        {
          numBufBits = bitPos = 0;
          numBits++;
        }
      }
    }
  }
}

}}

/* ================ unit: CPP/7zip/Compress/ZlibDecoder.cpp ================ */
// ZlibDecoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NZlib {

#define DEFLATE_TRY_BEGIN try {
#define DEFLATE_TRY_END } catch(...) { return S_FALSE; }

#define ADLER_MOD 65521
#define ADLER_LOOP_MAX 5550

UInt32 Adler32_Update(UInt32 adler, const Byte *data, size_t size);
UInt32 Adler32_Update(UInt32 adler, const Byte *data, size_t size)
{
  if (size == 0)
    return adler;
  UInt32 a = adler & 0xffff;
  UInt32 b = adler >> 16;
  do
  {
    size_t cur = size;
    if (cur > ADLER_LOOP_MAX)
        cur = ADLER_LOOP_MAX;
    size -= cur;
    const Byte *lim = data + cur;
    if (cur >= 4)
    {
      lim -= 4 - 1;
      do
      {
        a += data[0];  b += a;
        a += data[1];  b += a;
        a += data[2];  b += a;
        a += data[3];  b += a;
        data += 4;
      }
      while (data < lim);
      lim += 4 - 1;
    }
    if (data != lim) { a += *data++;  b += a;
    if (data != lim) { a += *data++;  b += a;
    if (data != lim) { a += *data++;  b += a; }}}
    a %= ADLER_MOD;
    b %= ADLER_MOD;
  }
  while (size);
  return (b << 16) + a;
}

Z7_COM7F_IMF(COutStreamWithAdler::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  HRESULT result = S_OK;
  if (_stream)
    result = _stream->Write(data, size, &size);
  _adler = Adler32_Update(_adler, (const Byte *)data, size);
  _size += size;
  if (processedSize)
    *processedSize = size;
  return result;
}

Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  DEFLATE_TRY_BEGIN
  _inputProcessedSize_Additional = 0;
  AdlerStream.Create_if_Empty();
  DeflateDecoder.Create_if_Empty();
  DeflateDecoder->Set_NeedFinishInput(true);

  if (inSize && *inSize < 2)
    return S_FALSE;
  {
    Byte buf[2];
    RINOK(ReadStream_FALSE(inStream, buf, 2))
    if (!IsZlib(buf))
      return S_FALSE;
  }
  _inputProcessedSize_Additional = 2;
  AdlerStream->SetStream(outStream);
  AdlerStream->Init();
  // NDeflate::NDecoder::Code() ignores inSize
  /*
  UInt64 inSize2 = 0;
  if (inSize)
    inSize2 = *inSize - 2;
  */
  const HRESULT res = DeflateDecoder.Interface()->Code(inStream, AdlerStream,
      /* inSize ? &inSize2 : */ NULL, outSize, progress);
  AdlerStream->ReleaseStream();

  if (res == S_OK)
  {
    UInt32 footer32[1];
    UInt32 processedSize;
    RINOK(DeflateDecoder->ReadUnusedFromInBuf(footer32, 4, &processedSize))
    if (processedSize != 4)
    {
      size_t processedSize2 = 4 - processedSize;
      RINOK(ReadStream(inStream, (Byte *)(void *)footer32 + processedSize, &processedSize2))
      _inputProcessedSize_Additional += (Int32)processedSize2;
      processedSize += (UInt32)processedSize2;
    }
    
    if (processedSize == 4)
    {
      const UInt32 adler = GetBe32a(footer32);
      if (adler != AdlerStream->GetAdler())
        return S_FALSE; // adler error
    }
    else if (!IsAdlerOptional)
      return S_FALSE; // unexpeced end of stream (can't read adler)
    else
    {
      // IsAdlerOptional == true
      if (processedSize != 0)
      {
         // we exclude adler bytes from processed size:
        _inputProcessedSize_Additional -= (Int32)processedSize;
        return S_FALSE;
      }
    }
  }
  return res;
  DEFLATE_TRY_END
}

}}

/* ================ unit: CPP/7zip/Compress/ZlibEncoder.cpp ================ */
// ZlibEncoder.cpp

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NZlib {

#define DEFLATE_TRY_BEGIN try {
#define DEFLATE_TRY_END } catch(...) { return S_FALSE; }

UInt32 Adler32_Update(UInt32 adler, const Byte *buf, size_t size);

Z7_COM7F_IMF(CInStreamWithAdler::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  const HRESULT result = _stream->Read(data, size, &size);
  _adler = Adler32_Update(_adler, (const Byte *)data, size);
  _size += size;
  if (processedSize)
    *processedSize = size;
  return result;
}

void CEncoder::Create()
{
  if (!DeflateEncoder)
    DeflateEncoder = DeflateEncoderSpec = new NDeflate::NEncoder::CCOMCoder;
}

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  DEFLATE_TRY_BEGIN
  if (!AdlerStream)
    AdlerStream = AdlerSpec = new CInStreamWithAdler;
  Create();

  {
    Byte buf[2] = { 0x78, 0xDA };
    RINOK(WriteStream(outStream, buf, 2))
  }

  AdlerSpec->SetStream(inStream);
  AdlerSpec->Init();
  const HRESULT res = DeflateEncoder->Code(AdlerStream, outStream, inSize, NULL, progress);
  AdlerSpec->ReleaseStream();

  RINOK(res)

  {
    const UInt32 a = AdlerSpec->GetAdler();
    const Byte buf[4] = { (Byte)(a >> 24), (Byte)(a >> 16), (Byte)(a >> 8), (Byte)(a) };
    return WriteStream(outStream, buf, 4);
  }
  DEFLATE_TRY_END
}

}}

/* ================ unit: CPP/7zip/Compress/ZstdDecoder.cpp ================ */
// ZstdDecoder.cpp

// amalgamation: header emitted in prologue

// #include <stdio.h>

// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue
// amalgamation: header emitted in prologue

// amalgamation: header emitted in prologue

namespace NCompress {
namespace NZstd {

static const size_t k_Zstd_BlockSizeMax = 1 << 17;
/*
  we set _outStepMask as (k_Zstd_BlockSizeMax - 1), because:
    - cycSize in zstd decoder for isCyclicMode is aligned for (1 << 17) only.
      So some write sizes will be multiple of ((1 << 17) * n).
    - Also it can be optimal to flush data after each block decoding.
*/

CDecoder::CDecoder():
    _outStepMask(k_Zstd_BlockSizeMax - 1) // must be = (1 << x) - 1
    , _dec(NULL)
    , _inProcessed(0)
    , _inBufSize(1u << 19) // larger value will reduce the number of memcpy() calls in CZstdDec code
    , _inBuf(NULL)
    , FinishMode(false)
    , DisableHash(False)
    // , DisableHash(True) // for debug : fast decoding without hash calculation
{
  // ZstdDecInfo_Clear(&ResInfo);
}

CDecoder::~CDecoder()
{
  if (_dec)
    ZstdDec_Destroy(_dec);
  MidFree(_inBuf);
}


Z7_COM7F_IMF(CDecoder::SetInBufSize(UInt32 , UInt32 size))
  { _inBufSize = size;  return S_OK; }
Z7_COM7F_IMF(CDecoder::SetOutBufSize(UInt32 , UInt32 size))
{
  // we round it down:
  size >>= 1;
  size |= size >> (1 << 0);
  size |= size >> (1 << 1);
  size |= size >> (1 << 2);
  size |= size >> (1 << 3);
  size |= size >> (1 << 4);
  _outStepMask = size; // it's (1 << x) - 1 now
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte * /* prop */, UInt32 /* size */))
{
  // if (size != 3 && size != 5) return E_NOTIMPL;
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::SetFinishMode(UInt32 finishMode))
{
  FinishMode = (finishMode != 0);
  // FinishMode = false; // for debug
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::ReadUnusedFromInBuf(void *data, UInt32 size, UInt32 *processedSize))
{
  size_t cur = ZstdDec_ReadUnusedFromInBuf(_dec, _afterDecoding_tempPos, data, size);
  _afterDecoding_tempPos += cur;
  size -= (UInt32)cur;
  if (size)
  {
    const size_t rem = _state.inLim - _state.inPos;
    if (size > rem)
      size = (UInt32)rem;
    if (size)
    {
      memcpy((Byte *)data + cur, _state.inBuf + _state.inPos, size);
      _state.inPos += size;
      cur += size;
    }
  }
  *processedSize = (UInt32)cur;
  return S_OK;
}



HRESULT CDecoder::Prepare(const UInt64 *outSize)
{
  _inProcessed = 0;
  _afterDecoding_tempPos = 0;
  ZstdDecState_Clear(&_state);
  ZstdDecInfo_CLEAR(&ResInfo)
  // _state.outStep = _outStepMask + 1; // must be = (1 << x)
  _state.disableHash = DisableHash;
  if (outSize)
  {
    _state.outSize_Defined = True;
    _state.outSize = *outSize;
    // _state.outSize = 0; // for debug
  }
  if (!_dec)
  {
    _dec = ZstdDec_Create(&g_AlignedAlloc, &g_BigAlloc);
    if (!_dec)
      return E_OUTOFMEMORY;
  }
  if (!_inBuf || _inBufSize != _inBufSize_Allocated)
  {
    MidFree(_inBuf);
    _inBuf = NULL;
    _inBufSize_Allocated = 0;
    _inBuf = (Byte *)MidAlloc(_inBufSize);
    if (!_inBuf)
      return E_OUTOFMEMORY;
    _inBufSize_Allocated = _inBufSize;
  }
  _state.inBuf = _inBuf;
  ZstdDec_Init(_dec);
  return S_OK;
}


Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress))
{
  RINOK(Prepare(outSize))
  
  UInt64 inPrev = 0;
  UInt64 outPrev = 0;
  UInt64 writtenSize = 0;
  bool readWasFinished = false;
  SRes sres = SZ_OK;
  HRESULT hres = S_OK;
  HRESULT hres_Read = S_OK;
  
  for (;;)
  {
    if (_state.inPos == _state.inLim && !readWasFinished)
    {
      _state.inPos = 0;
      _state.inLim = _inBufSize;
      hres_Read = ReadStream(inStream, _inBuf, &_state.inLim);
      // _state.inLim -= 5; readWasFinished = True; // for debug
      if (_state.inLim != _inBufSize || hres_Read != S_OK)
      {
        // hres_Read = 99; // for debug
        readWasFinished = True;
      }
    }
    {
      const size_t inPos_Start = _state.inPos;
      sres = ZstdDec_Decode(_dec, &_state);
      _inProcessed += _state.inPos - inPos_Start;
    }
    /*
    if (_state.status == ZSTD_STATUS_FINISHED_FRAME)
      printf("\nfinished frame pos=%8x, checksum=%08x\n", (unsigned)_state.outProcessed, (unsigned)_state.info.checksum);
    */
    const bool needStop = (sres != SZ_OK)
        || _state.status == ZSTD_STATUS_OUT_REACHED
        || (outSize && *outSize < _state.outProcessed)
        || (readWasFinished && _state.inPos == _state.inLim
            && ZstdDecState_DOES_NEED_MORE_INPUT_OR_FINISHED_FRAME(&_state));
   
    size_t size = _state.winPos - _state.wrPos; // full write size
    if (size)
    {
      if (!needStop)
      {
        // we try to flush on aligned positions, if possible
        size = _state.needWrite_Size; // minimal required write size
        const size_t alignedPos = _state.winPos & ~(size_t)_outStepMask;
        if (alignedPos > _state.wrPos)
        {
          const size_t size2 = alignedPos - _state.wrPos;  // optimized aligned size
          if (size < size2)
              size = size2;
        }
      }
      if (size)
      {
        {
          size_t curSize = size;
          if (outSize)
          {
            const UInt64 rem = *outSize - writtenSize;
            if (curSize > rem)
              curSize = (size_t)rem;
          }
          if (curSize)
          {
            // printf("Write wrPos=%8x, size=%8x\n", (unsigned)_state.wrPos, (unsigned)size);
            hres = WriteStream(outStream, _state.win + _state.wrPos, curSize);
            if (hres != S_OK)
              break;
            writtenSize += curSize; // it's real size of data that was written to stream
          }
        }
        _state.wrPos += size; // virtual written size, that will be reported to CZstdDec
        // _state.needWrite_Size = 0; // optional
      }
    }
    
    if (needStop)
      break;
    if (progress)
    if (_inProcessed - inPrev >= (1 << 27)
        || _state.outProcessed - outPrev >= (1 << 28))
    {
      inPrev = _inProcessed;
      outPrev = _state.outProcessed;
      RINOK(progress->SetRatioInfo(&inPrev, &outPrev))
    }
  }

  if (hres == S_OK)
  {
    ZstdDec_GetResInfo(_dec, &_state, sres, &ResInfo);
    sres = ResInfo.decode_SRes;
    /* now (ResInfo.decode_SRes) can contain 2 extra error codes:
         - SZ_ERROR_NO_ARCHIVE  : if no frames
         - SZ_ERROR_INPUT_EOF   : if ZSTD_STATUS_NEEDS_MORE_INPUT
    */
    _inProcessed -= ResInfo.extraSize;
    if (hres_Read != S_OK && _state.inLim == _state.inPos && readWasFinished)
    {
      /* if (there is stream reading error,
           and decoding was stopped because of end of input stream),
           then we use reading error as main error code */
      if (sres == SZ_OK ||
          sres == SZ_ERROR_INPUT_EOF ||
          sres == SZ_ERROR_NO_ARCHIVE)
        hres = hres_Read;
    }
    if (sres == SZ_ERROR_INPUT_EOF && !FinishMode)
    {
      /* SZ_ERROR_INPUT_EOF case is allowed case for (!FinishMode) mode.
         So we restore SZ_OK result for that case: */
      ResInfo.decode_SRes = sres = SZ_OK;
    }
    if (hres == S_OK)
    {
      hres = SResToHRESULT(sres);
      if (hres == S_OK && FinishMode)
      {
        if ((inSize && *inSize != _inProcessed)
            || ResInfo.is_NonFinishedFrame
            || (outSize && (*outSize != writtenSize || writtenSize != _state.outProcessed)))
          hres = S_FALSE;
      }
    }
  }
  return hres;
}


Z7_COM7F_IMF(CDecoder::GetInStreamProcessedSize(UInt64 *value))
{
  *value = _inProcessed;
  return S_OK;
}


#ifndef Z7_NO_READ_FROM_CODER_ZSTD

Z7_COM7F_IMF(CDecoder::SetOutStreamSize(const UInt64 *outSize))
{
  _inProcessed = 0;
  _hres_Read = S_OK;
  _hres_Decode = S_OK;
  _writtenSize = 0;
  _readWasFinished = false;
  _wasFinished = false;
  return Prepare(outSize);
}


Z7_COM7F_IMF(CDecoder::SetInStream(ISequentialInStream *inStream))
  { _inStream = inStream; return S_OK; }
Z7_COM7F_IMF(CDecoder::ReleaseInStream())
  { _inStream.Release(); return S_OK; }


// if SetInStream() mode: the caller must call GetFinishResult() after full decoding
// to check that there decoding was finished correctly

HRESULT CDecoder::GetFinishResult()
{
  if (_state.winPos != _state.wrPos || !_wasFinished)
    return FinishMode ? S_FALSE : S_OK;
  // _state.winPos == _state.wrPos
  // _wasFinished == true
  if (FinishMode && _hres_Decode == S_OK && _state.outSize_Defined && _state.outSize != _writtenSize)
    _hres_Decode = S_FALSE;
  return _hres_Decode;
}
  

Z7_COM7F_IMF(CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;

  for (;;)
  {
    if (_state.outSize_Defined)
    {
      // _writtenSize <= _state.outSize
      const UInt64 rem = _state.outSize - _writtenSize;
      if (size > rem)
        size = (UInt32)rem;
    }
    {
      size_t cur = _state.winPos - _state.wrPos;
      if (cur)
      {
        // _state.winPos != _state.wrPos;
        // so there is some decoded data that was not written still
        if (size == 0)
        {
          // if (FinishMode) and we are not allowed to write more, then it's data error
          if (FinishMode && _state.outSize_Defined && _state.outSize == _writtenSize)
            return S_FALSE;
          return S_OK;
        }
        if (cur > size)
          cur = (size_t)size;
        // cur != 0
        memcpy(data, _state.win + _state.wrPos, cur);
        _state.wrPos += cur;
        _writtenSize += cur;
        data = (void *)((Byte *)data + cur);
        if (processedSize)
          *processedSize += (UInt32)cur;
        size -= (UInt32)cur;
        continue;
      }
    }

    // _state.winPos == _state.wrPos
    if (_wasFinished)
    {
      if (_hres_Decode == S_OK && FinishMode
          && _state.outSize_Defined && _state.outSize != _writtenSize)
        _hres_Decode = S_FALSE;
      return _hres_Decode;
    }

    // _wasFinished == false
    if (size == 0 && _state.outSize_Defined && _state.outSize != _state.outProcessed)
    {
      /* size == 0 : so the caller don't need more data now.
         _state.outSize > _state.outProcessed : so more data will be requested
         later by caller for full processing.
         So we exit without ZstdDec_Decode() call, because we don't want
         ZstdDec_Decode() to start new block decoding
      */
      return S_OK;
    }
    // size != 0  || !_state.outSize_Defined || _state.outSize == _state.outProcessed)

    if (_state.inPos == _state.inLim && !_readWasFinished)
    {
      _state.inPos = 0;
      _state.inLim = _inBufSize;
      _hres_Read = ReadStream(_inStream, _inBuf, &_state.inLim);
      if (_state.inLim != _inBufSize || _hres_Read != S_OK)
      {
        // _hres_Read = 99; // for debug
        _readWasFinished = True;
      }
    }

    SRes sres;
    {
      const SizeT inPos_Start = _state.inPos;
      sres = ZstdDec_Decode(_dec, &_state);
      _inProcessed += _state.inPos - inPos_Start;
    }

    const bool inFinished = (_state.inPos == _state.inLim) && _readWasFinished;

    _wasFinished = (sres != SZ_OK)
        || _state.status == ZSTD_STATUS_OUT_REACHED
        || (_state.outSize_Defined && _state.outSize < _state.outProcessed)
        || (inFinished
            && ZstdDecState_DOES_NEED_MORE_INPUT_OR_FINISHED_FRAME(&_state));

    if (!_wasFinished)
      continue;
    
    // _wasFinished == true
    /* (_state.winPos != _state.wrPos) is possible here.
       So we still can have some data to flush,
       but we must all result variables .
    */
    HRESULT hres = S_OK;
    ZstdDec_GetResInfo(_dec, &_state, sres, &ResInfo);
    sres = ResInfo.decode_SRes;
    _inProcessed -= ResInfo.extraSize;
    if (_hres_Read != S_OK && inFinished)
    {
      if (sres == SZ_OK ||
          sres == SZ_ERROR_INPUT_EOF ||
          sres == SZ_ERROR_NO_ARCHIVE)
        hres = _hres_Read;
    }
    if (sres == SZ_ERROR_INPUT_EOF && !FinishMode)
      ResInfo.decode_SRes = sres = SZ_OK;
    if (hres == S_OK)
    {
      hres = SResToHRESULT(sres);
      if (hres == S_OK && FinishMode)
        if (!inFinished
            || ResInfo.is_NonFinishedFrame
            || (_state.outSize_Defined && _state.outSize != _state.outProcessed))
          hres = S_FALSE;
    }
    _hres_Decode = hres;
  }
}

#endif

}}
