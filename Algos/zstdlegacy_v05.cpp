/* XArchive self-contained copy of legacy/zstd_v05.c (Zstandard 1.5.7).
 *
 * Headers are inlined so the unit no longer reaches outside Algos. The code is
 * upstream, verbatim; only the #include boundaries are gone. It remains its own
 * translation unit because the legacy decoders share unsuffixed file-scope
 * types (sortedSymbol_t, algo_time_t, blockProperties_t) and can only coexist
 * as separate units.
 *
 * Zstandard is BSD 3-Clause / GPL-2.0 dual licensed, Copyright (c) Meta
 * Platforms, Inc.; XArchive elects BSD, reproduced in THIRD_PARTY_NOTICES.md.
 *
 * This file is generated -- do not edit by hand.
 */

/* Converted from C to C++ on 2026-08-17 and must stay C++. Several of the
 * internal symbols defined below are declared in no header at all, so their
 * linkage names follow whatever language this file is compiled as; building
 * it as C would not fail here, it would fail much later at link, in whichever
 * unit calls them. Fail now instead.
 */
#ifndef __cplusplus
#error "XArchive: this amalgamation must be compiled as C++, not C. Its contract-free \
internal symbols (LzmaEnc_*, Sha*_UpdateBlocks, Ppmd7/8_UpdateModel, ZSTDv0*_*, ...) have \
no header declaration, so C and C++ builds of these units cannot be mixed."
#endif

/* ---- hoisted system includes ---- */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* ================ unit: legacy/zstd_v05.c ================ */
/*
 * Copyright (c) Yann Collet, Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */


/*- Dependencies -*/
/* ---- begin legacy/zstd_v05.h ---- */
/*
 * Copyright (c) Yann Collet, Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef ZSTDv05_H
#define ZSTDv05_H

#if defined (__cplusplus)
extern "C" {
#endif

/*-*************************************
*  Dependencies
***************************************/
#include <stddef.h>   /* size_t */
/* ---- begin common/mem.h ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef MEM_H_MODULE
#define MEM_H_MODULE

/*-****************************************
*  Dependencies
******************************************/
#include <stddef.h>  /* size_t, ptrdiff_t */
/* ---- begin common/compiler.h ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef ZSTD_COMPILER_H
#define ZSTD_COMPILER_H

#include <stddef.h>

/* ---- begin common/portability_macros.h ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef ZSTD_PORTABILITY_MACROS_H
#define ZSTD_PORTABILITY_MACROS_H

/**
 * This header file contains macro definitions to support portability.
 * This header is shared between C and ASM code, so it MUST only
 * contain macro definitions. It MUST not contain any C code.
 *
 * This header ONLY defines macros to detect platforms/feature support.
 *
 */


/* compat. with non-clang compilers */
#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif

/* compat. with non-clang compilers */
#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

/* compat. with non-clang compilers */
#ifndef __has_feature
#  define __has_feature(x) 0
#endif

/* detects whether we are being compiled under msan */
#ifndef ZSTD_MEMORY_SANITIZER
#  if __has_feature(memory_sanitizer)
#    define ZSTD_MEMORY_SANITIZER 1
#  else
#    define ZSTD_MEMORY_SANITIZER 0
#  endif
#endif

/* detects whether we are being compiled under asan */
#ifndef ZSTD_ADDRESS_SANITIZER
#  if __has_feature(address_sanitizer)
#    define ZSTD_ADDRESS_SANITIZER 1
#  elif defined(__SANITIZE_ADDRESS__)
#    define ZSTD_ADDRESS_SANITIZER 1
#  else
#    define ZSTD_ADDRESS_SANITIZER 0
#  endif
#endif

/* detects whether we are being compiled under dfsan */
#ifndef ZSTD_DATAFLOW_SANITIZER
# if __has_feature(dataflow_sanitizer)
#  define ZSTD_DATAFLOW_SANITIZER 1
# else
#  define ZSTD_DATAFLOW_SANITIZER 0
# endif
#endif

/* Mark the internal assembly functions as hidden  */
#ifdef __ELF__
# define ZSTD_HIDE_ASM_FUNCTION(func) .hidden func
#elif defined(__APPLE__)
# define ZSTD_HIDE_ASM_FUNCTION(func) .private_extern func
#else
# define ZSTD_HIDE_ASM_FUNCTION(func)
#endif

/* Compile time determination of BMI2 support */
#ifndef STATIC_BMI2
#  if defined(__BMI2__)
#    define STATIC_BMI2 1
#  elif defined(_MSC_VER) && defined(__AVX2__)
#    define STATIC_BMI2 1 /* MSVC does not have a BMI2 specific flag, but every CPU that supports AVX2 also supports BMI2 */
#  endif
#endif

#ifndef STATIC_BMI2
#  define STATIC_BMI2 0
#endif

/* Enable runtime BMI2 dispatch based on the CPU.
 * Enabled for clang & gcc >=4.8 on x86 when BMI2 isn't enabled by default.
 */
#ifndef DYNAMIC_BMI2
#  if ((defined(__clang__) && __has_attribute(__target__)) \
      || (defined(__GNUC__) \
          && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))) \
      && (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)) \
      && !defined(__BMI2__)
#    define DYNAMIC_BMI2 1
#  else
#    define DYNAMIC_BMI2 0
#  endif
#endif

/**
 * Only enable assembly for GNU C compatible compilers,
 * because other platforms may not support GAS assembly syntax.
 *
 * Only enable assembly for Linux / MacOS / Win32, other platforms may
 * work, but they haven't been tested. This could likely be
 * extended to BSD systems.
 *
 * Disable assembly when MSAN is enabled, because MSAN requires
 * 100% of code to be instrumented to work.
 */
#if defined(__GNUC__)
#  if defined(__linux__) || defined(__linux) || defined(__APPLE__) || defined(_WIN32)
#    if ZSTD_MEMORY_SANITIZER
#      define ZSTD_ASM_SUPPORTED 0
#    elif ZSTD_DATAFLOW_SANITIZER
#      define ZSTD_ASM_SUPPORTED 0
#    else
#      define ZSTD_ASM_SUPPORTED 1
#    endif
#  else
#    define ZSTD_ASM_SUPPORTED 0
#  endif
#else
#  define ZSTD_ASM_SUPPORTED 0
#endif

/**
 * Determines whether we should enable assembly for x86-64
 * with BMI2.
 *
 * Enable if all of the following conditions hold:
 * - ASM hasn't been explicitly disabled by defining ZSTD_DISABLE_ASM
 * - Assembly is supported
 * - We are compiling for x86-64 and either:
 *   - DYNAMIC_BMI2 is enabled
 *   - BMI2 is supported at compile time
 */
#if !defined(ZSTD_DISABLE_ASM) &&                                 \
    ZSTD_ASM_SUPPORTED &&                                         \
    defined(__x86_64__) &&                                        \
    (DYNAMIC_BMI2 || defined(__BMI2__))
# define ZSTD_ENABLE_ASM_X86_64_BMI2 1
#else
# define ZSTD_ENABLE_ASM_X86_64_BMI2 0
#endif

/*
 * For x86 ELF targets, add .note.gnu.property section for Intel CET in
 * assembly sources when CET is enabled.
 *
 * Additionally, any function that may be called indirectly must begin
 * with ZSTD_CET_ENDBRANCH.
 */
#if defined(__ELF__) && (defined(__x86_64__) || defined(__i386__)) \
    && defined(__has_include)
# if __has_include(<cet.h>)
#  include <cet.h>
#  define ZSTD_CET_ENDBRANCH _CET_ENDBR
# endif
#endif

#ifndef ZSTD_CET_ENDBRANCH
# define ZSTD_CET_ENDBRANCH
#endif

#endif /* ZSTD_PORTABILITY_MACROS_H */
/* ---- end common/portability_macros.h ---- */

/*-*******************************************************
*  Compiler specifics
*********************************************************/
/* force inlining */

#if !defined(ZSTD_NO_INLINE)
#if (defined(__GNUC__) && !defined(__STRICT_ANSI__)) || defined(__cplusplus) || defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
#  define INLINE_KEYWORD inline
#else
#  define INLINE_KEYWORD
#endif

#if defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
#  define FORCE_INLINE_ATTR __attribute__((always_inline))
#elif defined(_MSC_VER)
#  define FORCE_INLINE_ATTR __forceinline
#else
#  define FORCE_INLINE_ATTR
#endif

#else

#define INLINE_KEYWORD
#define FORCE_INLINE_ATTR

#endif

/**
  On MSVC qsort requires that functions passed into it use the __cdecl calling conversion(CC).
  This explicitly marks such functions as __cdecl so that the code will still compile
  if a CC other than __cdecl has been made the default.
*/
#if  defined(_MSC_VER)
#  define WIN_CDECL __cdecl
#else
#  define WIN_CDECL
#endif

/* UNUSED_ATTR tells the compiler it is okay if the function is unused. */
#if defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
#  define UNUSED_ATTR __attribute__((unused))
#else
#  define UNUSED_ATTR
#endif

/**
 * FORCE_INLINE_TEMPLATE is used to define C "templates", which take constant
 * parameters. They must be inlined for the compiler to eliminate the constant
 * branches.
 */
#define FORCE_INLINE_TEMPLATE static INLINE_KEYWORD FORCE_INLINE_ATTR UNUSED_ATTR
/**
 * HINT_INLINE is used to help the compiler generate better code. It is *not*
 * used for "templates", so it can be tweaked based on the compilers
 * performance.
 *
 * gcc-4.8 and gcc-4.9 have been shown to benefit from leaving off the
 * always_inline attribute.
 *
 * clang up to 5.0.0 (trunk) benefit tremendously from the always_inline
 * attribute.
 */
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 8 && __GNUC__ < 5
#  define HINT_INLINE static INLINE_KEYWORD
#else
#  define HINT_INLINE FORCE_INLINE_TEMPLATE
#endif

/* "soft" inline :
 * The compiler is free to select if it's a good idea to inline or not.
 * The main objective is to silence compiler warnings
 * when a defined function in included but not used.
 *
 * Note : this macro is prefixed `MEM_` because it used to be provided by `mem.h` unit.
 * Updating the prefix is probably preferable, but requires a fairly large codemod,
 * since this name is used everywhere.
 */
#ifndef MEM_STATIC  /* already defined in Linux Kernel mem.h */
#if defined(__GNUC__)
#  define MEM_STATIC static __inline UNUSED_ATTR
#elif defined(__IAR_SYSTEMS_ICC__)
#  define MEM_STATIC static inline UNUSED_ATTR
#elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
#  define MEM_STATIC static inline
#elif defined(_MSC_VER)
#  define MEM_STATIC static __inline
#else
#  define MEM_STATIC static  /* this version may generate warnings for unused static functions; disable the relevant warning */
#endif
#endif

/* force no inlining */
#ifdef _MSC_VER
#  define FORCE_NOINLINE static __declspec(noinline)
#else
#  if defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
#    define FORCE_NOINLINE static __attribute__((__noinline__))
#  else
#    define FORCE_NOINLINE static
#  endif
#endif


/* target attribute */
#if defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
#  define TARGET_ATTRIBUTE(target) __attribute__((__target__(target)))
#else
#  define TARGET_ATTRIBUTE(target)
#endif

/* Target attribute for BMI2 dynamic dispatch.
 * Enable lzcnt, bmi, and bmi2.
 * We test for bmi1 & bmi2. lzcnt is included in bmi1.
 */
#define BMI2_TARGET_ATTRIBUTE TARGET_ATTRIBUTE("lzcnt,bmi,bmi2")

/* prefetch
 * can be disabled, by declaring NO_PREFETCH build macro */
#if defined(NO_PREFETCH)
#  define PREFETCH_L1(ptr)  do { (void)(ptr); } while (0)  /* disabled */
#  define PREFETCH_L2(ptr)  do { (void)(ptr); } while (0)  /* disabled */
#else
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_I86)) && !defined(_M_ARM64EC)  /* _mm_prefetch() is not defined outside of x86/x64 */
#    include <mmintrin.h>   /* https://msdn.microsoft.com/fr-fr/library/84szxsww(v=vs.90).aspx */
#    define PREFETCH_L1(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T0)
#    define PREFETCH_L2(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T1)
#  elif defined(__GNUC__) && ( (__GNUC__ >= 4) || ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) ) )
#    define PREFETCH_L1(ptr)  __builtin_prefetch((ptr), 0 /* rw==read */, 3 /* locality */)
#    define PREFETCH_L2(ptr)  __builtin_prefetch((ptr), 0 /* rw==read */, 2 /* locality */)
#  elif defined(__aarch64__)
#    define PREFETCH_L1(ptr)  do { __asm__ __volatile__("prfm pldl1keep, %0" ::"Q"(*(ptr))); } while (0)
#    define PREFETCH_L2(ptr)  do { __asm__ __volatile__("prfm pldl2keep, %0" ::"Q"(*(ptr))); } while (0)
#  else
#    define PREFETCH_L1(ptr) do { (void)(ptr); } while (0)  /* disabled */
#    define PREFETCH_L2(ptr) do { (void)(ptr); } while (0)  /* disabled */
#  endif
#endif  /* NO_PREFETCH */

#define CACHELINE_SIZE 64

#define PREFETCH_AREA(p, s)                              \
    do {                                                 \
        const char* const _ptr = (const char*)(p);       \
        size_t const _size = (size_t)(s);                \
        size_t _pos;                                     \
        for (_pos=0; _pos<_size; _pos+=CACHELINE_SIZE) { \
            PREFETCH_L2(_ptr + _pos);                    \
        }                                                \
    } while (0)

/* vectorization
 * older GCC (pre gcc-4.3 picked as the cutoff) uses a different syntax,
 * and some compilers, like Intel ICC and MCST LCC, do not support it at all. */
#if !defined(__INTEL_COMPILER) && !defined(__clang__) && defined(__GNUC__) && !defined(__LCC__)
#  if (__GNUC__ == 4 && __GNUC_MINOR__ > 3) || (__GNUC__ >= 5)
#    define DONT_VECTORIZE __attribute__((optimize("no-tree-vectorize")))
#  else
#    define DONT_VECTORIZE _Pragma("GCC optimize(\"no-tree-vectorize\")")
#  endif
#else
#  define DONT_VECTORIZE
#endif

/* Tell the compiler that a branch is likely or unlikely.
 * Only use these macros if it causes the compiler to generate better code.
 * If you can remove a LIKELY/UNLIKELY annotation without speed changes in gcc
 * and clang, please do.
 */
#if defined(__GNUC__)
#define LIKELY(x) (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

#if __has_builtin(__builtin_unreachable) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)))
#  define ZSTD_UNREACHABLE do { assert(0), __builtin_unreachable(); } while (0)
#else
#  define ZSTD_UNREACHABLE do { assert(0); } while (0)
#endif

/* disable warnings */
#ifdef _MSC_VER    /* Visual Studio */
#  include <intrin.h>                    /* For Visual 2005 */
#  pragma warning(disable : 4100)        /* disable: C4100: unreferenced formal parameter */
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#  pragma warning(disable : 4204)        /* disable: C4204: non-constant aggregate initializer */
#  pragma warning(disable : 4214)        /* disable: C4214: non-int bitfields */
#  pragma warning(disable : 4324)        /* disable: C4324: padded structure */
#endif

/* compile time determination of SIMD support */
#if !defined(ZSTD_NO_INTRINSICS)
#  if defined(__AVX2__)
#    define ZSTD_ARCH_X86_AVX2
#  endif
#  if defined(__SSE2__) || defined(_M_X64) || (defined (_M_IX86) && defined(_M_IX86_FP) && (_M_IX86_FP >= 2))
#    define ZSTD_ARCH_X86_SSE2
#  endif
#  if defined(__ARM_NEON) || defined(_M_ARM64)
#    define ZSTD_ARCH_ARM_NEON
#  endif
#
#  if defined(ZSTD_ARCH_X86_AVX2)
#    include <immintrin.h>
#  endif
#  if defined(ZSTD_ARCH_X86_SSE2)
#    include <emmintrin.h>
#  elif defined(ZSTD_ARCH_ARM_NEON)
#    include <arm_neon.h>
#  endif
#endif

/* C-language Attributes are added in C23. */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ > 201710L) && defined(__has_c_attribute)
# define ZSTD_HAS_C_ATTRIBUTE(x) __has_c_attribute(x)
#else
# define ZSTD_HAS_C_ATTRIBUTE(x) 0
#endif

/* Only use C++ attributes in C++. Some compilers report support for C++
 * attributes when compiling with C.
 */
#if defined(__cplusplus) && defined(__has_cpp_attribute)
# define ZSTD_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
# define ZSTD_HAS_CPP_ATTRIBUTE(x) 0
#endif

/* Define ZSTD_FALLTHROUGH macro for annotating switch case with the 'fallthrough' attribute.
 * - C23: https://en.cppreference.com/w/c/language/attributes/fallthrough
 * - CPP17: https://en.cppreference.com/w/cpp/language/attributes/fallthrough
 * - Else: __attribute__((__fallthrough__))
 */
#ifndef ZSTD_FALLTHROUGH
# if ZSTD_HAS_C_ATTRIBUTE(fallthrough)
#  define ZSTD_FALLTHROUGH [[fallthrough]]
# elif ZSTD_HAS_CPP_ATTRIBUTE(fallthrough)
#  define ZSTD_FALLTHROUGH [[fallthrough]]
# elif __has_attribute(__fallthrough__)
/* Leading semicolon is to satisfy gcc-11 with -pedantic. Without the semicolon
 * gcc complains about: a label can only be part of a statement and a declaration is not a statement.
 */
#  define ZSTD_FALLTHROUGH ; __attribute__((__fallthrough__))
# else
#  define ZSTD_FALLTHROUGH
# endif
#endif

/*-**************************************************************
*  Alignment
*****************************************************************/

/* @return 1 if @u is a 2^n value, 0 otherwise
 * useful to check a value is valid for alignment restrictions */
MEM_STATIC int ZSTD_isPower2(size_t u) {
    return (u & (u-1)) == 0;
}

/* this test was initially positioned in mem.h,
 * but this file is removed (or replaced) for linux kernel
 * so it's now hosted in compiler.h,
 * which remains valid for both user & kernel spaces.
 */

#ifndef ZSTD_ALIGNOF
# if defined(__GNUC__) || defined(_MSC_VER)
/* covers gcc, clang & MSVC */
/* note : this section must come first, before C11,
 * due to a limitation in the kernel source generator */
#  define ZSTD_ALIGNOF(T) __alignof(T)

# elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
/* C11 support */
#  include <stdalign.h>
#  define ZSTD_ALIGNOF(T) alignof(T)

# else
/* No known support for alignof() - imperfect backup */
#  define ZSTD_ALIGNOF(T) (sizeof(void*) < sizeof(T) ? sizeof(void*) : sizeof(T))

# endif
#endif /* ZSTD_ALIGNOF */

#ifndef ZSTD_ALIGNED
/* C90-compatible alignment macro (GCC/Clang). Adjust for other compilers if needed. */
# if defined(__GNUC__) || defined(__clang__)
#  define ZSTD_ALIGNED(a) __attribute__((aligned(a)))
# elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) /* C11 */
#  define ZSTD_ALIGNED(a) _Alignas(a)
#elif defined(_MSC_VER)
#  define ZSTD_ALIGNED(n) __declspec(align(n))
# else
   /* this compiler will require its own alignment instruction */
#  define ZSTD_ALIGNED(...)
# endif
#endif /* ZSTD_ALIGNED */


/*-**************************************************************
*  Sanitizer
*****************************************************************/

/**
 * Zstd relies on pointer overflow in its decompressor.
 * We add this attribute to functions that rely on pointer overflow.
 */
#ifndef ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
#  if __has_attribute(no_sanitize)
#    if !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 8
       /* gcc < 8 only has signed-integer-overlow which triggers on pointer overflow */
#      define ZSTD_ALLOW_POINTER_OVERFLOW_ATTR __attribute__((no_sanitize("signed-integer-overflow")))
#    else
       /* older versions of clang [3.7, 5.0) will warn that pointer-overflow is ignored. */
#      define ZSTD_ALLOW_POINTER_OVERFLOW_ATTR __attribute__((no_sanitize("pointer-overflow")))
#    endif
#  else
#    define ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
#  endif
#endif

/**
 * Helper function to perform a wrapped pointer difference without triggering
 * UBSAN.
 *
 * @returns lhs - rhs with wrapping
 */
MEM_STATIC
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
ptrdiff_t ZSTD_wrappedPtrDiff(unsigned char const* lhs, unsigned char const* rhs)
{
    return lhs - rhs;
}

/**
 * Helper function to perform a wrapped pointer add without triggering UBSAN.
 *
 * @return ptr + add with wrapping
 */
MEM_STATIC
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
unsigned char const* ZSTD_wrappedPtrAdd(unsigned char const* ptr, ptrdiff_t add)
{
    return ptr + add;
}

/**
 * Helper function to perform a wrapped pointer subtraction without triggering
 * UBSAN.
 *
 * @return ptr - sub with wrapping
 */
MEM_STATIC
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
unsigned char const* ZSTD_wrappedPtrSub(unsigned char const* ptr, ptrdiff_t sub)
{
    return ptr - sub;
}

/**
 * Helper function to add to a pointer that works around C's undefined behavior
 * of adding 0 to NULL.
 *
 * @returns `ptr + add` except it defines `NULL + 0 == NULL`.
 */
MEM_STATIC
unsigned char* ZSTD_maybeNullPtrAdd(unsigned char* ptr, ptrdiff_t add)
{
    return add > 0 ? ptr + add : ptr;
}

/* Issue #3240 reports an ASAN failure on an llvm-mingw build. Out of an
 * abundance of caution, disable our custom poisoning on mingw. */
#ifdef __MINGW32__
#ifndef ZSTD_ASAN_DONT_POISON_WORKSPACE
#define ZSTD_ASAN_DONT_POISON_WORKSPACE 1
#endif
#ifndef ZSTD_MSAN_DONT_POISON_WORKSPACE
#define ZSTD_MSAN_DONT_POISON_WORKSPACE 1
#endif
#endif

#if ZSTD_MEMORY_SANITIZER && !defined(ZSTD_MSAN_DONT_POISON_WORKSPACE)
/* Not all platforms that support msan provide sanitizers/msan_interface.h.
 * We therefore declare the functions we need ourselves, rather than trying to
 * include the header file... */
#include <stddef.h>  /* size_t */
#define ZSTD_DEPS_NEED_STDINT
/* ---- begin common/zstd_deps.h (re-included on purpose) ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/* This file provides common libc dependencies that zstd requires.
 * The purpose is to allow replacing this file with a custom implementation
 * to compile zstd without libc support.
 */

/* Need:
 * NULL
 * INT_MAX
 * UINT_MAX
 * ZSTD_memcpy()
 * ZSTD_memset()
 * ZSTD_memmove()
 */
#ifndef ZSTD_DEPS_COMMON
#define ZSTD_DEPS_COMMON

/* Even though we use qsort_r only for the dictionary builder, the macro
 * _GNU_SOURCE has to be declared *before* the inclusion of any standard
 * header and the script 'combine.sh' combines the whole zstd source code
 * in a single file.
 */
#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__gnu_linux__) || \
    defined(__CYGWIN__) || defined(__MSYS__)
#if !defined(_GNU_SOURCE) && !defined(__ANDROID__) /* NDK doesn't ship qsort_r(). */
#define _GNU_SOURCE
#endif
#endif

#include <limits.h>
#include <stddef.h>
#include <string.h>

#if defined(__GNUC__) && __GNUC__ >= 4
# define ZSTD_memcpy(d,s,l) __builtin_memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) __builtin_memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) __builtin_memset((p),(v),(l))
#else
# define ZSTD_memcpy(d,s,l) memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) memset((p),(v),(l))
#endif

#endif /* ZSTD_DEPS_COMMON */

/* Need:
 * ZSTD_malloc()
 * ZSTD_free()
 * ZSTD_calloc()
 */
#ifdef ZSTD_DEPS_NEED_MALLOC
#ifndef ZSTD_DEPS_MALLOC
#define ZSTD_DEPS_MALLOC

#include <stdlib.h>

#define ZSTD_malloc(s) malloc(s)
#define ZSTD_calloc(n,s) calloc((n), (s))
#define ZSTD_free(p) free((p))

#endif /* ZSTD_DEPS_MALLOC */
#endif /* ZSTD_DEPS_NEED_MALLOC */

/*
 * Provides 64-bit math support.
 * Need:
 * U64 ZSTD_div64(U64 dividend, U32 divisor)
 */
#ifdef ZSTD_DEPS_NEED_MATH64
#ifndef ZSTD_DEPS_MATH64
#define ZSTD_DEPS_MATH64

#define ZSTD_div64(dividend, divisor) ((dividend) / (divisor))

#endif /* ZSTD_DEPS_MATH64 */
#endif /* ZSTD_DEPS_NEED_MATH64 */

/* Need:
 * assert()
 */
#ifdef ZSTD_DEPS_NEED_ASSERT
#ifndef ZSTD_DEPS_ASSERT
#define ZSTD_DEPS_ASSERT

#include <assert.h>

#endif /* ZSTD_DEPS_ASSERT */
#endif /* ZSTD_DEPS_NEED_ASSERT */

/* Need:
 * ZSTD_DEBUG_PRINT()
 */
#ifdef ZSTD_DEPS_NEED_IO
#ifndef ZSTD_DEPS_IO
#define ZSTD_DEPS_IO

#include <stdio.h>
#define ZSTD_DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)

#endif /* ZSTD_DEPS_IO */
#endif /* ZSTD_DEPS_NEED_IO */

/* Only requested when <stdint.h> is known to be present.
 * Need:
 * intptr_t
 */
#ifdef ZSTD_DEPS_NEED_STDINT
#ifndef ZSTD_DEPS_STDINT
#define ZSTD_DEPS_STDINT

#include <stdint.h>

#endif /* ZSTD_DEPS_STDINT */
#endif /* ZSTD_DEPS_NEED_STDINT */
/* ---- end common/zstd_deps.h ---- */

/* Make memory region fully initialized (without changing its contents). */
void __msan_unpoison(const volatile void *a, size_t size);

/* Make memory region fully uninitialized (without changing its contents).
   This is a legacy interface that does not update origin information. Use
   __msan_allocated_memory() instead. */
void __msan_poison(const volatile void *a, size_t size);

/* Returns the offset of the first (at least partially) poisoned byte in the
   memory range, or -1 if the whole range is good. */
intptr_t __msan_test_shadow(const volatile void *x, size_t size);

/* Print shadow and origin for the memory range to stderr in a human-readable
   format. */
void __msan_print_shadow(const volatile void *x, size_t size);
#endif

#if ZSTD_ADDRESS_SANITIZER && !defined(ZSTD_ASAN_DONT_POISON_WORKSPACE)
/* Not all platforms that support asan provide sanitizers/asan_interface.h.
 * We therefore declare the functions we need ourselves, rather than trying to
 * include the header file... */
#include <stddef.h>  /* size_t */

/**
 * Marks a memory region (<c>[addr, addr+size)</c>) as unaddressable.
 *
 * This memory must be previously allocated by your program. Instrumented
 * code is forbidden from accessing addresses in this region until it is
 * unpoisoned. This function is not guaranteed to poison the entire region -
 * it could poison only a subregion of <c>[addr, addr+size)</c> due to ASan
 * alignment restrictions.
 *
 * \note This function is not thread-safe because no two threads can poison or
 * unpoison memory in the same memory region simultaneously.
 *
 * \param addr Start of memory region.
 * \param size Size of memory region. */
void __asan_poison_memory_region(void const volatile *addr, size_t size);

/**
 * Marks a memory region (<c>[addr, addr+size)</c>) as addressable.
 *
 * This memory must be previously allocated by your program. Accessing
 * addresses in this region is allowed until this region is poisoned again.
 * This function could unpoison a super-region of <c>[addr, addr+size)</c> due
 * to ASan alignment restrictions.
 *
 * \note This function is not thread-safe because no two threads can
 * poison or unpoison memory in the same memory region simultaneously.
 *
 * \param addr Start of memory region.
 * \param size Size of memory region. */
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#endif

#endif /* ZSTD_COMPILER_H */
/* ---- end common/compiler.h ---- */
/* ---- begin common/debug.h ---- */
/* ******************************************************************
 * debug
 * Part of FSE library
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * You can contact the author at :
 * - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
****************************************************************** */


/*
 * The purpose of this header is to enable debug functions.
 * They regroup assert(), DEBUGLOG() and RAWLOG() for run-time,
 * and DEBUG_STATIC_ASSERT() for compile-time.
 *
 * By default, DEBUGLEVEL==0, which means run-time debug is disabled.
 *
 * Level 1 enables assert() only.
 * Starting level 2, traces can be generated and pushed to stderr.
 * The higher the level, the more verbose the traces.
 *
 * It's possible to dynamically adjust level using variable g_debug_level,
 * which is only declared if DEBUGLEVEL>=2,
 * and is a global variable, not multi-thread protected (use with care)
 */

#ifndef DEBUG_H_12987983217
#define DEBUG_H_12987983217


/* static assert is triggered at compile time, leaving no runtime artefact.
 * static assert only works with compile-time constants.
 * Also, this variant can only be used inside a function. */
#define DEBUG_STATIC_ASSERT(c) (void)sizeof(char[(c) ? 1 : -1])


/* DEBUGLEVEL is expected to be defined externally,
 * typically through compiler command line.
 * Value must be a number. */
#ifndef DEBUGLEVEL
#  define DEBUGLEVEL 0
#endif


/* recommended values for DEBUGLEVEL :
 * 0 : release mode, no debug, all run-time checks disabled
 * 1 : enables assert() only, no display
 * 2 : reserved, for currently active debug path
 * 3 : events once per object lifetime (CCtx, CDict, etc.)
 * 4 : events once per frame
 * 5 : events once per block
 * 6 : events once per sequence (verbose)
 * 7+: events at every position (*very* verbose)
 *
 * It's generally inconvenient to output traces > 5.
 * In which case, it's possible to selectively trigger high verbosity levels
 * by modifying g_debug_level.
 */

#if (DEBUGLEVEL>=1)
#  define ZSTD_DEPS_NEED_ASSERT
/* ---- begin common/zstd_deps.h (re-included on purpose) ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/* This file provides common libc dependencies that zstd requires.
 * The purpose is to allow replacing this file with a custom implementation
 * to compile zstd without libc support.
 */

/* Need:
 * NULL
 * INT_MAX
 * UINT_MAX
 * ZSTD_memcpy()
 * ZSTD_memset()
 * ZSTD_memmove()
 */
#ifndef ZSTD_DEPS_COMMON
#define ZSTD_DEPS_COMMON

/* Even though we use qsort_r only for the dictionary builder, the macro
 * _GNU_SOURCE has to be declared *before* the inclusion of any standard
 * header and the script 'combine.sh' combines the whole zstd source code
 * in a single file.
 */
#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__gnu_linux__) || \
    defined(__CYGWIN__) || defined(__MSYS__)
#if !defined(_GNU_SOURCE) && !defined(__ANDROID__) /* NDK doesn't ship qsort_r(). */
#define _GNU_SOURCE
#endif
#endif

#include <limits.h>
#include <stddef.h>
#include <string.h>

#if defined(__GNUC__) && __GNUC__ >= 4
# define ZSTD_memcpy(d,s,l) __builtin_memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) __builtin_memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) __builtin_memset((p),(v),(l))
#else
# define ZSTD_memcpy(d,s,l) memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) memset((p),(v),(l))
#endif

#endif /* ZSTD_DEPS_COMMON */

/* Need:
 * ZSTD_malloc()
 * ZSTD_free()
 * ZSTD_calloc()
 */
#ifdef ZSTD_DEPS_NEED_MALLOC
#ifndef ZSTD_DEPS_MALLOC
#define ZSTD_DEPS_MALLOC

#include <stdlib.h>

#define ZSTD_malloc(s) malloc(s)
#define ZSTD_calloc(n,s) calloc((n), (s))
#define ZSTD_free(p) free((p))

#endif /* ZSTD_DEPS_MALLOC */
#endif /* ZSTD_DEPS_NEED_MALLOC */

/*
 * Provides 64-bit math support.
 * Need:
 * U64 ZSTD_div64(U64 dividend, U32 divisor)
 */
#ifdef ZSTD_DEPS_NEED_MATH64
#ifndef ZSTD_DEPS_MATH64
#define ZSTD_DEPS_MATH64

#define ZSTD_div64(dividend, divisor) ((dividend) / (divisor))

#endif /* ZSTD_DEPS_MATH64 */
#endif /* ZSTD_DEPS_NEED_MATH64 */

/* Need:
 * assert()
 */
#ifdef ZSTD_DEPS_NEED_ASSERT
#ifndef ZSTD_DEPS_ASSERT
#define ZSTD_DEPS_ASSERT

#include <assert.h>

#endif /* ZSTD_DEPS_ASSERT */
#endif /* ZSTD_DEPS_NEED_ASSERT */

/* Need:
 * ZSTD_DEBUG_PRINT()
 */
#ifdef ZSTD_DEPS_NEED_IO
#ifndef ZSTD_DEPS_IO
#define ZSTD_DEPS_IO

#include <stdio.h>
#define ZSTD_DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)

#endif /* ZSTD_DEPS_IO */
#endif /* ZSTD_DEPS_NEED_IO */

/* Only requested when <stdint.h> is known to be present.
 * Need:
 * intptr_t
 */
#ifdef ZSTD_DEPS_NEED_STDINT
#ifndef ZSTD_DEPS_STDINT
#define ZSTD_DEPS_STDINT

#include <stdint.h>

#endif /* ZSTD_DEPS_STDINT */
#endif /* ZSTD_DEPS_NEED_STDINT */
/* ---- end common/zstd_deps.h ---- */
#else
#  ifndef assert   /* assert may be already defined, due to prior #include <assert.h> */
#    define assert(condition) ((void)0)   /* disable assert (default) */
#  endif
#endif

#if (DEBUGLEVEL>=2)
#  define ZSTD_DEPS_NEED_IO
/* ---- begin common/zstd_deps.h (re-included on purpose) ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/* This file provides common libc dependencies that zstd requires.
 * The purpose is to allow replacing this file with a custom implementation
 * to compile zstd without libc support.
 */

/* Need:
 * NULL
 * INT_MAX
 * UINT_MAX
 * ZSTD_memcpy()
 * ZSTD_memset()
 * ZSTD_memmove()
 */
#ifndef ZSTD_DEPS_COMMON
#define ZSTD_DEPS_COMMON

/* Even though we use qsort_r only for the dictionary builder, the macro
 * _GNU_SOURCE has to be declared *before* the inclusion of any standard
 * header and the script 'combine.sh' combines the whole zstd source code
 * in a single file.
 */
#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__gnu_linux__) || \
    defined(__CYGWIN__) || defined(__MSYS__)
#if !defined(_GNU_SOURCE) && !defined(__ANDROID__) /* NDK doesn't ship qsort_r(). */
#define _GNU_SOURCE
#endif
#endif

#include <limits.h>
#include <stddef.h>
#include <string.h>

#if defined(__GNUC__) && __GNUC__ >= 4
# define ZSTD_memcpy(d,s,l) __builtin_memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) __builtin_memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) __builtin_memset((p),(v),(l))
#else
# define ZSTD_memcpy(d,s,l) memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) memset((p),(v),(l))
#endif

#endif /* ZSTD_DEPS_COMMON */

/* Need:
 * ZSTD_malloc()
 * ZSTD_free()
 * ZSTD_calloc()
 */
#ifdef ZSTD_DEPS_NEED_MALLOC
#ifndef ZSTD_DEPS_MALLOC
#define ZSTD_DEPS_MALLOC

#include <stdlib.h>

#define ZSTD_malloc(s) malloc(s)
#define ZSTD_calloc(n,s) calloc((n), (s))
#define ZSTD_free(p) free((p))

#endif /* ZSTD_DEPS_MALLOC */
#endif /* ZSTD_DEPS_NEED_MALLOC */

/*
 * Provides 64-bit math support.
 * Need:
 * U64 ZSTD_div64(U64 dividend, U32 divisor)
 */
#ifdef ZSTD_DEPS_NEED_MATH64
#ifndef ZSTD_DEPS_MATH64
#define ZSTD_DEPS_MATH64

#define ZSTD_div64(dividend, divisor) ((dividend) / (divisor))

#endif /* ZSTD_DEPS_MATH64 */
#endif /* ZSTD_DEPS_NEED_MATH64 */

/* Need:
 * assert()
 */
#ifdef ZSTD_DEPS_NEED_ASSERT
#ifndef ZSTD_DEPS_ASSERT
#define ZSTD_DEPS_ASSERT

#include <assert.h>

#endif /* ZSTD_DEPS_ASSERT */
#endif /* ZSTD_DEPS_NEED_ASSERT */

/* Need:
 * ZSTD_DEBUG_PRINT()
 */
#ifdef ZSTD_DEPS_NEED_IO
#ifndef ZSTD_DEPS_IO
#define ZSTD_DEPS_IO

#include <stdio.h>
#define ZSTD_DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)

#endif /* ZSTD_DEPS_IO */
#endif /* ZSTD_DEPS_NEED_IO */

/* Only requested when <stdint.h> is known to be present.
 * Need:
 * intptr_t
 */
#ifdef ZSTD_DEPS_NEED_STDINT
#ifndef ZSTD_DEPS_STDINT
#define ZSTD_DEPS_STDINT

#include <stdint.h>

#endif /* ZSTD_DEPS_STDINT */
#endif /* ZSTD_DEPS_NEED_STDINT */
/* ---- end common/zstd_deps.h ---- */
extern int g_debuglevel; /* the variable is only declared,
                            it actually lives in debug.c,
                            and is shared by the whole process.
                            It's not thread-safe.
                            It's useful when enabling very verbose levels
                            on selective conditions (such as position in src) */

#  define RAWLOG(l, ...)                   \
    do {                                   \
        if (l<=g_debuglevel) {             \
            ZSTD_DEBUG_PRINT(__VA_ARGS__); \
        }                                  \
    } while (0)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define LINE_AS_STRING TOSTRING(__LINE__)

#  define DEBUGLOG(l, ...)                               \
    do {                                                 \
        if (l<=g_debuglevel) {                           \
            ZSTD_DEBUG_PRINT(__FILE__ ":" LINE_AS_STRING ": " __VA_ARGS__); \
            ZSTD_DEBUG_PRINT(" \n");                     \
        }                                                \
    } while (0)
#else
#  define RAWLOG(l, ...)   do { } while (0)    /* disabled */
#  define DEBUGLOG(l, ...) do { } while (0)    /* disabled */
#endif

#endif /* DEBUG_H_12987983217 */
/* ---- end common/debug.h ---- */
/* ---- begin common/zstd_deps.h (re-included on purpose) ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/* This file provides common libc dependencies that zstd requires.
 * The purpose is to allow replacing this file with a custom implementation
 * to compile zstd without libc support.
 */

/* Need:
 * NULL
 * INT_MAX
 * UINT_MAX
 * ZSTD_memcpy()
 * ZSTD_memset()
 * ZSTD_memmove()
 */
#ifndef ZSTD_DEPS_COMMON
#define ZSTD_DEPS_COMMON

/* Even though we use qsort_r only for the dictionary builder, the macro
 * _GNU_SOURCE has to be declared *before* the inclusion of any standard
 * header and the script 'combine.sh' combines the whole zstd source code
 * in a single file.
 */
#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__gnu_linux__) || \
    defined(__CYGWIN__) || defined(__MSYS__)
#if !defined(_GNU_SOURCE) && !defined(__ANDROID__) /* NDK doesn't ship qsort_r(). */
#define _GNU_SOURCE
#endif
#endif

#include <limits.h>
#include <stddef.h>
#include <string.h>

#if defined(__GNUC__) && __GNUC__ >= 4
# define ZSTD_memcpy(d,s,l) __builtin_memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) __builtin_memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) __builtin_memset((p),(v),(l))
#else
# define ZSTD_memcpy(d,s,l) memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) memset((p),(v),(l))
#endif

#endif /* ZSTD_DEPS_COMMON */

/* Need:
 * ZSTD_malloc()
 * ZSTD_free()
 * ZSTD_calloc()
 */
#ifdef ZSTD_DEPS_NEED_MALLOC
#ifndef ZSTD_DEPS_MALLOC
#define ZSTD_DEPS_MALLOC

#include <stdlib.h>

#define ZSTD_malloc(s) malloc(s)
#define ZSTD_calloc(n,s) calloc((n), (s))
#define ZSTD_free(p) free((p))

#endif /* ZSTD_DEPS_MALLOC */
#endif /* ZSTD_DEPS_NEED_MALLOC */

/*
 * Provides 64-bit math support.
 * Need:
 * U64 ZSTD_div64(U64 dividend, U32 divisor)
 */
#ifdef ZSTD_DEPS_NEED_MATH64
#ifndef ZSTD_DEPS_MATH64
#define ZSTD_DEPS_MATH64

#define ZSTD_div64(dividend, divisor) ((dividend) / (divisor))

#endif /* ZSTD_DEPS_MATH64 */
#endif /* ZSTD_DEPS_NEED_MATH64 */

/* Need:
 * assert()
 */
#ifdef ZSTD_DEPS_NEED_ASSERT
#ifndef ZSTD_DEPS_ASSERT
#define ZSTD_DEPS_ASSERT

#include <assert.h>

#endif /* ZSTD_DEPS_ASSERT */
#endif /* ZSTD_DEPS_NEED_ASSERT */

/* Need:
 * ZSTD_DEBUG_PRINT()
 */
#ifdef ZSTD_DEPS_NEED_IO
#ifndef ZSTD_DEPS_IO
#define ZSTD_DEPS_IO

#include <stdio.h>
#define ZSTD_DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)

#endif /* ZSTD_DEPS_IO */
#endif /* ZSTD_DEPS_NEED_IO */

/* Only requested when <stdint.h> is known to be present.
 * Need:
 * intptr_t
 */
#ifdef ZSTD_DEPS_NEED_STDINT
#ifndef ZSTD_DEPS_STDINT
#define ZSTD_DEPS_STDINT

#include <stdint.h>

#endif /* ZSTD_DEPS_STDINT */
#endif /* ZSTD_DEPS_NEED_STDINT */
/* ---- end common/zstd_deps.h ---- */


/*-****************************************
*  Compiler specifics
******************************************/
#if defined(_MSC_VER)   /* Visual Studio */
#   include <stdlib.h>  /* _byteswap_ulong */
#   include <intrin.h>  /* _byteswap_* */
#elif defined(__ICCARM__)
#   include <intrinsics.h>
#endif

/*-**************************************************************
*  Basic Types
*****************************************************************/
#if  !defined (__VMS) && (defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
#  if defined(_AIX)
#    include <inttypes.h>
#  else
#    include <stdint.h> /* intptr_t */
#  endif
  typedef   uint8_t BYTE;
  typedef   uint8_t U8;
  typedef    int8_t S8;
  typedef  uint16_t U16;
  typedef   int16_t S16;
  typedef  uint32_t U32;
  typedef   int32_t S32;
  typedef  uint64_t U64;
  typedef   int64_t S64;
#else
# include <limits.h>
#if CHAR_BIT != 8
#  error "this implementation requires char to be exactly 8-bit type"
#endif
  typedef unsigned char      BYTE;
  typedef unsigned char      U8;
  typedef   signed char      S8;
#if USHRT_MAX != 65535
#  error "this implementation requires short to be exactly 16-bit type"
#endif
  typedef unsigned short      U16;
  typedef   signed short      S16;
#if UINT_MAX != 4294967295
#  error "this implementation requires int to be exactly 32-bit type"
#endif
  typedef unsigned int        U32;
  typedef   signed int        S32;
/* note : there are no limits defined for long long type in C90.
 * limits exist in C99, however, in such case, <stdint.h> is preferred */
  typedef unsigned long long  U64;
  typedef   signed long long  S64;
#endif

/*-**************************************************************
*  Memory I/O API
*****************************************************************/
/*=== Static platform detection ===*/
MEM_STATIC unsigned MEM_32bits(void);
MEM_STATIC unsigned MEM_64bits(void);
MEM_STATIC unsigned MEM_isLittleEndian(void);

/*=== Native unaligned read/write ===*/
MEM_STATIC U16 MEM_read16(const void* memPtr);
MEM_STATIC U32 MEM_read32(const void* memPtr);
MEM_STATIC U64 MEM_read64(const void* memPtr);
MEM_STATIC size_t MEM_readST(const void* memPtr);

MEM_STATIC void MEM_write16(void* memPtr, U16 value);
MEM_STATIC void MEM_write32(void* memPtr, U32 value);
MEM_STATIC void MEM_write64(void* memPtr, U64 value);

/*=== Little endian unaligned read/write ===*/
MEM_STATIC U16 MEM_readLE16(const void* memPtr);
MEM_STATIC U32 MEM_readLE24(const void* memPtr);
MEM_STATIC U32 MEM_readLE32(const void* memPtr);
MEM_STATIC U64 MEM_readLE64(const void* memPtr);
MEM_STATIC size_t MEM_readLEST(const void* memPtr);

MEM_STATIC void MEM_writeLE16(void* memPtr, U16 val);
MEM_STATIC void MEM_writeLE24(void* memPtr, U32 val);
MEM_STATIC void MEM_writeLE32(void* memPtr, U32 val32);
MEM_STATIC void MEM_writeLE64(void* memPtr, U64 val64);
MEM_STATIC void MEM_writeLEST(void* memPtr, size_t val);

/*=== Big endian unaligned read/write ===*/
MEM_STATIC U32 MEM_readBE32(const void* memPtr);
MEM_STATIC U64 MEM_readBE64(const void* memPtr);
MEM_STATIC size_t MEM_readBEST(const void* memPtr);

MEM_STATIC void MEM_writeBE32(void* memPtr, U32 val32);
MEM_STATIC void MEM_writeBE64(void* memPtr, U64 val64);
MEM_STATIC void MEM_writeBEST(void* memPtr, size_t val);

/*=== Byteswap ===*/
MEM_STATIC U32 MEM_swap32(U32 in);
MEM_STATIC U64 MEM_swap64(U64 in);
MEM_STATIC size_t MEM_swapST(size_t in);


/*-**************************************************************
*  Memory I/O Implementation
*****************************************************************/
/* MEM_FORCE_MEMORY_ACCESS : For accessing unaligned memory:
 * Method 0 : always use `memcpy()`. Safe and portable.
 * Method 1 : Use compiler extension to set unaligned access.
 * Method 2 : direct access. This method is portable but violate C standard.
 *            It can generate buggy code on targets depending on alignment.
 * Default  : method 1 if supported, else method 0
 */
#ifndef MEM_FORCE_MEMORY_ACCESS   /* can be defined externally, on command line for example */
#  ifdef __GNUC__
#    define MEM_FORCE_MEMORY_ACCESS 1
#  endif
#endif

MEM_STATIC unsigned MEM_32bits(void) { return sizeof(size_t)==4; }
MEM_STATIC unsigned MEM_64bits(void) { return sizeof(size_t)==8; }

MEM_STATIC unsigned MEM_isLittleEndian(void)
{
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    return 1;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    return 0;
#elif defined(__clang__) && __LITTLE_ENDIAN__
    return 1;
#elif defined(__clang__) && __BIG_ENDIAN__
    return 0;
#elif defined(_MSC_VER) && (_M_X64 || _M_IX86)
    return 1;
#elif defined(__DMC__) && defined(_M_IX86)
    return 1;
#elif defined(__IAR_SYSTEMS_ICC__) && __LITTLE_ENDIAN__
    return 1;
#else
    const union { U32 u; BYTE c[4]; } one = { 1 };   /* don't use static : performance detrimental  */
    return one.c[0];
#endif
}

#if defined(MEM_FORCE_MEMORY_ACCESS) && (MEM_FORCE_MEMORY_ACCESS==2)

/* violates C standard, by lying on structure alignment.
Only use if no other choice to achieve best performance on target platform */
MEM_STATIC U16 MEM_read16(const void* memPtr) { return *(const U16*) memPtr; }
MEM_STATIC U32 MEM_read32(const void* memPtr) { return *(const U32*) memPtr; }
MEM_STATIC U64 MEM_read64(const void* memPtr) { return *(const U64*) memPtr; }
MEM_STATIC size_t MEM_readST(const void* memPtr) { return *(const size_t*) memPtr; }

MEM_STATIC void MEM_write16(void* memPtr, U16 value) { *(U16*)memPtr = value; }
MEM_STATIC void MEM_write32(void* memPtr, U32 value) { *(U32*)memPtr = value; }
MEM_STATIC void MEM_write64(void* memPtr, U64 value) { *(U64*)memPtr = value; }

#elif defined(MEM_FORCE_MEMORY_ACCESS) && (MEM_FORCE_MEMORY_ACCESS==1)

typedef __attribute__((aligned(1))) U16 unalign16;
typedef __attribute__((aligned(1))) U32 unalign32;
typedef __attribute__((aligned(1))) U64 unalign64;
typedef __attribute__((aligned(1))) size_t unalignArch;

MEM_STATIC U16 MEM_read16(const void* ptr) { return *(const unalign16*)ptr; }
MEM_STATIC U32 MEM_read32(const void* ptr) { return *(const unalign32*)ptr; }
MEM_STATIC U64 MEM_read64(const void* ptr) { return *(const unalign64*)ptr; }
MEM_STATIC size_t MEM_readST(const void* ptr) { return *(const unalignArch*)ptr; }

MEM_STATIC void MEM_write16(void* memPtr, U16 value) { *(unalign16*)memPtr = value; }
MEM_STATIC void MEM_write32(void* memPtr, U32 value) { *(unalign32*)memPtr = value; }
MEM_STATIC void MEM_write64(void* memPtr, U64 value) { *(unalign64*)memPtr = value; }

#else

/* default method, safe and standard.
   can sometimes prove slower */

MEM_STATIC U16 MEM_read16(const void* memPtr)
{
    U16 val; ZSTD_memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC U32 MEM_read32(const void* memPtr)
{
    U32 val; ZSTD_memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC U64 MEM_read64(const void* memPtr)
{
    U64 val; ZSTD_memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC size_t MEM_readST(const void* memPtr)
{
    size_t val; ZSTD_memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC void MEM_write16(void* memPtr, U16 value)
{
    ZSTD_memcpy(memPtr, &value, sizeof(value));
}

MEM_STATIC void MEM_write32(void* memPtr, U32 value)
{
    ZSTD_memcpy(memPtr, &value, sizeof(value));
}

MEM_STATIC void MEM_write64(void* memPtr, U64 value)
{
    ZSTD_memcpy(memPtr, &value, sizeof(value));
}

#endif /* MEM_FORCE_MEMORY_ACCESS */

MEM_STATIC U32 MEM_swap32_fallback(U32 in)
{
    return  ((in << 24) & 0xff000000 ) |
            ((in <<  8) & 0x00ff0000 ) |
            ((in >>  8) & 0x0000ff00 ) |
            ((in >> 24) & 0x000000ff );
}

MEM_STATIC U32 MEM_swap32(U32 in)
{
#if defined(_MSC_VER)     /* Visual Studio */
    return _byteswap_ulong(in);
#elif (defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 403)) \
  || (defined(__clang__) && __has_builtin(__builtin_bswap32))
    return __builtin_bswap32(in);
#elif defined(__ICCARM__)
    return __REV(in);
#else
    return MEM_swap32_fallback(in);
#endif
}

MEM_STATIC U64 MEM_swap64_fallback(U64 in)
{
     return  ((in << 56) & 0xff00000000000000ULL) |
            ((in << 40) & 0x00ff000000000000ULL) |
            ((in << 24) & 0x0000ff0000000000ULL) |
            ((in << 8)  & 0x000000ff00000000ULL) |
            ((in >> 8)  & 0x00000000ff000000ULL) |
            ((in >> 24) & 0x0000000000ff0000ULL) |
            ((in >> 40) & 0x000000000000ff00ULL) |
            ((in >> 56) & 0x00000000000000ffULL);
}

MEM_STATIC U64 MEM_swap64(U64 in)
{
#if defined(_MSC_VER)     /* Visual Studio */
    return _byteswap_uint64(in);
#elif (defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 403)) \
  || (defined(__clang__) && __has_builtin(__builtin_bswap64))
    return __builtin_bswap64(in);
#else
    return MEM_swap64_fallback(in);
#endif
}

MEM_STATIC size_t MEM_swapST(size_t in)
{
    if (MEM_32bits())
        return (size_t)MEM_swap32((U32)in);
    else
        return (size_t)MEM_swap64((U64)in);
}

/*=== Little endian r/w ===*/

MEM_STATIC U16 MEM_readLE16(const void* memPtr)
{
    if (MEM_isLittleEndian())
        return MEM_read16(memPtr);
    else {
        const BYTE* p = (const BYTE*)memPtr;
        return (U16)(p[0] + (p[1]<<8));
    }
}

MEM_STATIC void MEM_writeLE16(void* memPtr, U16 val)
{
    if (MEM_isLittleEndian()) {
        MEM_write16(memPtr, val);
    } else {
        BYTE* p = (BYTE*)memPtr;
        p[0] = (BYTE)val;
        p[1] = (BYTE)(val>>8);
    }
}

MEM_STATIC U32 MEM_readLE24(const void* memPtr)
{
    return (U32)MEM_readLE16(memPtr) + ((U32)(((const BYTE*)memPtr)[2]) << 16);
}

MEM_STATIC void MEM_writeLE24(void* memPtr, U32 val)
{
    MEM_writeLE16(memPtr, (U16)val);
    ((BYTE*)memPtr)[2] = (BYTE)(val>>16);
}

MEM_STATIC U32 MEM_readLE32(const void* memPtr)
{
    if (MEM_isLittleEndian())
        return MEM_read32(memPtr);
    else
        return MEM_swap32(MEM_read32(memPtr));
}

MEM_STATIC void MEM_writeLE32(void* memPtr, U32 val32)
{
    if (MEM_isLittleEndian())
        MEM_write32(memPtr, val32);
    else
        MEM_write32(memPtr, MEM_swap32(val32));
}

MEM_STATIC U64 MEM_readLE64(const void* memPtr)
{
    if (MEM_isLittleEndian())
        return MEM_read64(memPtr);
    else
        return MEM_swap64(MEM_read64(memPtr));
}

MEM_STATIC void MEM_writeLE64(void* memPtr, U64 val64)
{
    if (MEM_isLittleEndian())
        MEM_write64(memPtr, val64);
    else
        MEM_write64(memPtr, MEM_swap64(val64));
}

MEM_STATIC size_t MEM_readLEST(const void* memPtr)
{
    if (MEM_32bits())
        return (size_t)MEM_readLE32(memPtr);
    else
        return (size_t)MEM_readLE64(memPtr);
}

MEM_STATIC void MEM_writeLEST(void* memPtr, size_t val)
{
    if (MEM_32bits())
        MEM_writeLE32(memPtr, (U32)val);
    else
        MEM_writeLE64(memPtr, (U64)val);
}

/*=== Big endian r/w ===*/

MEM_STATIC U32 MEM_readBE32(const void* memPtr)
{
    if (MEM_isLittleEndian())
        return MEM_swap32(MEM_read32(memPtr));
    else
        return MEM_read32(memPtr);
}

MEM_STATIC void MEM_writeBE32(void* memPtr, U32 val32)
{
    if (MEM_isLittleEndian())
        MEM_write32(memPtr, MEM_swap32(val32));
    else
        MEM_write32(memPtr, val32);
}

MEM_STATIC U64 MEM_readBE64(const void* memPtr)
{
    if (MEM_isLittleEndian())
        return MEM_swap64(MEM_read64(memPtr));
    else
        return MEM_read64(memPtr);
}

MEM_STATIC void MEM_writeBE64(void* memPtr, U64 val64)
{
    if (MEM_isLittleEndian())
        MEM_write64(memPtr, MEM_swap64(val64));
    else
        MEM_write64(memPtr, val64);
}

MEM_STATIC size_t MEM_readBEST(const void* memPtr)
{
    if (MEM_32bits())
        return (size_t)MEM_readBE32(memPtr);
    else
        return (size_t)MEM_readBE64(memPtr);
}

MEM_STATIC void MEM_writeBEST(void* memPtr, size_t val)
{
    if (MEM_32bits())
        MEM_writeBE32(memPtr, (U32)val);
    else
        MEM_writeBE64(memPtr, (U64)val);
}

/* code only tested on 32 and 64 bits systems */
MEM_STATIC void MEM_check(void) { DEBUG_STATIC_ASSERT((sizeof(size_t)==4) || (sizeof(size_t)==8)); }

#endif /* MEM_H_MODULE */
/* ---- end common/mem.h ---- */


/* *************************************
*  Simple functions
***************************************/
/*! ZSTDv05_decompress() :
    `compressedSize` : is the _exact_ size of the compressed blob, otherwise decompression will fail.
    `dstCapacity` must be large enough, equal or larger than originalSize.
    @return : the number of bytes decompressed into `dst` (<= `dstCapacity`),
              or an errorCode if it fails (which can be tested using ZSTDv05_isError()) */
size_t ZSTDv05_decompress( void* dst, size_t dstCapacity,
                     const void* src, size_t compressedSize);

 /**
 ZSTDv05_findFrameSizeInfoLegacy() : get the source length and decompressed bound of a ZSTD frame compliant with v0.5.x format
     srcSize : The size of the 'src' buffer, at least as large as the frame pointed to by 'src'
     cSize (output parameter)  : the number of bytes that would be read to decompress this frame
                                 or an error code if it fails (which can be tested using ZSTDv01_isError())
     dBound (output parameter) : an upper-bound for the decompressed size of the data in the frame
                                 or ZSTD_CONTENTSIZE_ERROR if an error occurs

    note : assumes `cSize` and `dBound` are _not_ NULL.
 */
void ZSTDv05_findFrameSizeInfoLegacy(const void *src, size_t srcSize,
                                     size_t* cSize, unsigned long long* dBound);

/* *************************************
*  Helper functions
***************************************/
/* Error Management */
unsigned    ZSTDv05_isError(size_t code);          /*!< tells if a `size_t` function result is an error code */
const char* ZSTDv05_getErrorName(size_t code);     /*!< provides readable string for an error code */


/* *************************************
*  Explicit memory management
***************************************/
/** Decompression context */
typedef struct ZSTDv05_DCtx_s ZSTDv05_DCtx;
ZSTDv05_DCtx* ZSTDv05_createDCtx(void);
size_t ZSTDv05_freeDCtx(ZSTDv05_DCtx* dctx);      /*!< @return : errorCode */

/** ZSTDv05_decompressDCtx() :
*   Same as ZSTDv05_decompress(), but requires an already allocated ZSTDv05_DCtx (see ZSTDv05_createDCtx()) */
size_t ZSTDv05_decompressDCtx(ZSTDv05_DCtx* ctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);


/*-***********************
*  Simple Dictionary API
*************************/
/*! ZSTDv05_decompress_usingDict() :
*   Decompression using a pre-defined Dictionary content (see dictBuilder).
*   Dictionary must be identical to the one used during compression, otherwise regenerated data will be corrupted.
*   Note : dict can be NULL, in which case, it's equivalent to ZSTDv05_decompressDCtx() */
size_t ZSTDv05_decompress_usingDict(ZSTDv05_DCtx* dctx,
                                            void* dst, size_t dstCapacity,
                                      const void* src, size_t srcSize,
                                      const void* dict,size_t dictSize);

/*-************************
*  Advanced Streaming API
***************************/
typedef enum { ZSTDv05_fast, ZSTDv05_greedy, ZSTDv05_lazy, ZSTDv05_lazy2, ZSTDv05_btlazy2, ZSTDv05_opt, ZSTDv05_btopt } ZSTDv05_strategy;
typedef struct {
    U64 srcSize;
    U32 windowLog;     /* the only useful information to retrieve */
    U32 contentLog; U32 hashLog; U32 searchLog; U32 searchLength; U32 targetLength; ZSTDv05_strategy strategy;
} ZSTDv05_parameters;
size_t ZSTDv05_getFrameParams(ZSTDv05_parameters* params, const void* src, size_t srcSize);

size_t ZSTDv05_decompressBegin_usingDict(ZSTDv05_DCtx* dctx, const void* dict, size_t dictSize);
void   ZSTDv05_copyDCtx(ZSTDv05_DCtx* dstDCtx, const ZSTDv05_DCtx* srcDCtx);
size_t ZSTDv05_nextSrcSizeToDecompress(ZSTDv05_DCtx* dctx);
size_t ZSTDv05_decompressContinue(ZSTDv05_DCtx* dctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);


/*-***********************
*  ZBUFF API
*************************/
typedef struct ZBUFFv05_DCtx_s ZBUFFv05_DCtx;
ZBUFFv05_DCtx* ZBUFFv05_createDCtx(void);
size_t         ZBUFFv05_freeDCtx(ZBUFFv05_DCtx* dctx);

size_t ZBUFFv05_decompressInit(ZBUFFv05_DCtx* dctx);
size_t ZBUFFv05_decompressInitDictionary(ZBUFFv05_DCtx* dctx, const void* dict, size_t dictSize);

size_t ZBUFFv05_decompressContinue(ZBUFFv05_DCtx* dctx,
                                            void* dst, size_t* dstCapacityPtr,
                                      const void* src, size_t* srcSizePtr);

/*-***************************************************************************
*  Streaming decompression
*
*  A ZBUFFv05_DCtx object is required to track streaming operations.
*  Use ZBUFFv05_createDCtx() and ZBUFFv05_freeDCtx() to create/release resources.
*  Use ZBUFFv05_decompressInit() to start a new decompression operation,
*   or ZBUFFv05_decompressInitDictionary() if decompression requires a dictionary.
*  Note that ZBUFFv05_DCtx objects can be reused multiple times.
*
*  Use ZBUFFv05_decompressContinue() repetitively to consume your input.
*  *srcSizePtr and *dstCapacityPtr can be any size.
*  The function will report how many bytes were read or written by modifying *srcSizePtr and *dstCapacityPtr.
*  Note that it may not consume the entire input, in which case it's up to the caller to present remaining input again.
*  The content of @dst will be overwritten (up to *dstCapacityPtr) at each function call, so save its content if it matters or change @dst.
*  @return : a hint to preferred nb of bytes to use as input for next function call (it's only a hint, to help latency)
*            or 0 when a frame is completely decoded
*            or an error code, which can be tested using ZBUFFv05_isError().
*
*  Hint : recommended buffer sizes (not compulsory) : ZBUFFv05_recommendedDInSize() / ZBUFFv05_recommendedDOutSize()
*  output : ZBUFFv05_recommendedDOutSize==128 KB block size is the internal unit, it ensures it's always possible to write a full block when decoded.
*  input  : ZBUFFv05_recommendedDInSize==128Kb+3; just follow indications from ZBUFFv05_decompressContinue() to minimize latency. It should always be <= 128 KB + 3 .
* *******************************************************************************/


/* *************************************
*  Tool functions
***************************************/
unsigned ZBUFFv05_isError(size_t errorCode);
const char* ZBUFFv05_getErrorName(size_t errorCode);

/** Functions below provide recommended buffer sizes for Compression or Decompression operations.
*   These sizes are just hints, and tend to offer better latency */
size_t ZBUFFv05_recommendedDInSize(void);
size_t ZBUFFv05_recommendedDOutSize(void);



/*-*************************************
*  Constants
***************************************/
#define ZSTDv05_MAGICNUMBER 0xFD2FB525   /* v0.5 */




#if defined (__cplusplus)
}
#endif

#endif  /* ZSTDv0505_H */
/* ---- end legacy/zstd_v05.h ---- */
/* ---- begin common/error_private.h ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/* Note : this module is expected to remain private, do not expose it */

#ifndef ERROR_H_MODULE
#define ERROR_H_MODULE

/* ****************************************
*  Dependencies
******************************************/
/* ---- begin zstd_errors.h ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef ZSTD_ERRORS_H_398273423
#define ZSTD_ERRORS_H_398273423

#if defined (__cplusplus)
extern "C" {
#endif

/* =====   ZSTDERRORLIB_API : control library symbols visibility   ===== */
#ifndef ZSTDERRORLIB_VISIBLE
   /* Backwards compatibility with old macro name */
#  ifdef ZSTDERRORLIB_VISIBILITY
#    define ZSTDERRORLIB_VISIBLE ZSTDERRORLIB_VISIBILITY
#  elif defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__MINGW32__)
#    define ZSTDERRORLIB_VISIBLE __attribute__ ((visibility ("default")))
#  else
#    define ZSTDERRORLIB_VISIBLE
#  endif
#endif

#ifndef ZSTDERRORLIB_HIDDEN
#  if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__MINGW32__)
#    define ZSTDERRORLIB_HIDDEN __attribute__ ((visibility ("hidden")))
#  else
#    define ZSTDERRORLIB_HIDDEN
#  endif
#endif

#if defined(ZSTD_DLL_EXPORT) && (ZSTD_DLL_EXPORT==1)
#  define ZSTDERRORLIB_API __declspec(dllexport) ZSTDERRORLIB_VISIBLE
#elif defined(ZSTD_DLL_IMPORT) && (ZSTD_DLL_IMPORT==1)
#  define ZSTDERRORLIB_API __declspec(dllimport) ZSTDERRORLIB_VISIBLE /* It isn't required but allows to generate better code, saving a function pointer load from the IAT and an indirect jump.*/
#else
#  define ZSTDERRORLIB_API ZSTDERRORLIB_VISIBLE
#endif

/*-*********************************************
 *  Error codes list
 *-*********************************************
 *  Error codes _values_ are pinned down since v1.3.1 only.
 *  Therefore, don't rely on values if you may link to any version < v1.3.1.
 *
 *  Only values < 100 are considered stable.
 *
 *  note 1 : this API shall be used with static linking only.
 *           dynamic linking is not yet officially supported.
 *  note 2 : Prefer relying on the enum than on its value whenever possible
 *           This is the only supported way to use the error list < v1.3.1
 *  note 3 : ZSTD_isError() is always correct, whatever the library version.
 **********************************************/
typedef enum {
  ZSTD_error_no_error = 0,
  ZSTD_error_GENERIC  = 1,
  ZSTD_error_prefix_unknown                = 10,
  ZSTD_error_version_unsupported           = 12,
  ZSTD_error_frameParameter_unsupported    = 14,
  ZSTD_error_frameParameter_windowTooLarge = 16,
  ZSTD_error_corruption_detected = 20,
  ZSTD_error_checksum_wrong      = 22,
  ZSTD_error_literals_headerWrong = 24,
  ZSTD_error_dictionary_corrupted      = 30,
  ZSTD_error_dictionary_wrong          = 32,
  ZSTD_error_dictionaryCreation_failed = 34,
  ZSTD_error_parameter_unsupported   = 40,
  ZSTD_error_parameter_combination_unsupported = 41,
  ZSTD_error_parameter_outOfBound    = 42,
  ZSTD_error_tableLog_tooLarge       = 44,
  ZSTD_error_maxSymbolValue_tooLarge = 46,
  ZSTD_error_maxSymbolValue_tooSmall = 48,
  ZSTD_error_cannotProduce_uncompressedBlock = 49,
  ZSTD_error_stabilityCondition_notRespected = 50,
  ZSTD_error_stage_wrong       = 60,
  ZSTD_error_init_missing      = 62,
  ZSTD_error_memory_allocation = 64,
  ZSTD_error_workSpace_tooSmall= 66,
  ZSTD_error_dstSize_tooSmall = 70,
  ZSTD_error_srcSize_wrong    = 72,
  ZSTD_error_dstBuffer_null   = 74,
  ZSTD_error_noForwardProgress_destFull = 80,
  ZSTD_error_noForwardProgress_inputEmpty = 82,
  /* following error codes are __NOT STABLE__, they can be removed or changed in future versions */
  ZSTD_error_frameIndex_tooLarge = 100,
  ZSTD_error_seekableIO          = 102,
  ZSTD_error_dstBuffer_wrong     = 104,
  ZSTD_error_srcBuffer_wrong     = 105,
  ZSTD_error_sequenceProducer_failed = 106,
  ZSTD_error_externalSequences_invalid = 107,
  ZSTD_error_maxCode = 120  /* never EVER use this value directly, it can change in future versions! Use ZSTD_isError() instead */
} ZSTD_ErrorCode;

ZSTDERRORLIB_API const char* ZSTD_getErrorString(ZSTD_ErrorCode code);   /**< Same as ZSTD_getErrorName, but using a `ZSTD_ErrorCode` enum argument */


#if defined (__cplusplus)
}
#endif

#endif /* ZSTD_ERRORS_H_398273423 */
/* ---- end zstd_errors.h ---- */
/* amalgamation: common/compiler.h already inlined */
/* amalgamation: common/debug.h already inlined */
/* ---- begin common/zstd_deps.h (re-included on purpose) ---- */
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/* This file provides common libc dependencies that zstd requires.
 * The purpose is to allow replacing this file with a custom implementation
 * to compile zstd without libc support.
 */

/* Need:
 * NULL
 * INT_MAX
 * UINT_MAX
 * ZSTD_memcpy()
 * ZSTD_memset()
 * ZSTD_memmove()
 */
#ifndef ZSTD_DEPS_COMMON
#define ZSTD_DEPS_COMMON

/* Even though we use qsort_r only for the dictionary builder, the macro
 * _GNU_SOURCE has to be declared *before* the inclusion of any standard
 * header and the script 'combine.sh' combines the whole zstd source code
 * in a single file.
 */
#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__gnu_linux__) || \
    defined(__CYGWIN__) || defined(__MSYS__)
#if !defined(_GNU_SOURCE) && !defined(__ANDROID__) /* NDK doesn't ship qsort_r(). */
#define _GNU_SOURCE
#endif
#endif

#include <limits.h>
#include <stddef.h>
#include <string.h>

#if defined(__GNUC__) && __GNUC__ >= 4
# define ZSTD_memcpy(d,s,l) __builtin_memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) __builtin_memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) __builtin_memset((p),(v),(l))
#else
# define ZSTD_memcpy(d,s,l) memcpy((d),(s),(l))
# define ZSTD_memmove(d,s,l) memmove((d),(s),(l))
# define ZSTD_memset(p,v,l) memset((p),(v),(l))
#endif

#endif /* ZSTD_DEPS_COMMON */

/* Need:
 * ZSTD_malloc()
 * ZSTD_free()
 * ZSTD_calloc()
 */
#ifdef ZSTD_DEPS_NEED_MALLOC
#ifndef ZSTD_DEPS_MALLOC
#define ZSTD_DEPS_MALLOC

#include <stdlib.h>

#define ZSTD_malloc(s) malloc(s)
#define ZSTD_calloc(n,s) calloc((n), (s))
#define ZSTD_free(p) free((p))

#endif /* ZSTD_DEPS_MALLOC */
#endif /* ZSTD_DEPS_NEED_MALLOC */

/*
 * Provides 64-bit math support.
 * Need:
 * U64 ZSTD_div64(U64 dividend, U32 divisor)
 */
#ifdef ZSTD_DEPS_NEED_MATH64
#ifndef ZSTD_DEPS_MATH64
#define ZSTD_DEPS_MATH64

#define ZSTD_div64(dividend, divisor) ((dividend) / (divisor))

#endif /* ZSTD_DEPS_MATH64 */
#endif /* ZSTD_DEPS_NEED_MATH64 */

/* Need:
 * assert()
 */
#ifdef ZSTD_DEPS_NEED_ASSERT
#ifndef ZSTD_DEPS_ASSERT
#define ZSTD_DEPS_ASSERT

#include <assert.h>

#endif /* ZSTD_DEPS_ASSERT */
#endif /* ZSTD_DEPS_NEED_ASSERT */

/* Need:
 * ZSTD_DEBUG_PRINT()
 */
#ifdef ZSTD_DEPS_NEED_IO
#ifndef ZSTD_DEPS_IO
#define ZSTD_DEPS_IO

#include <stdio.h>
#define ZSTD_DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)

#endif /* ZSTD_DEPS_IO */
#endif /* ZSTD_DEPS_NEED_IO */

/* Only requested when <stdint.h> is known to be present.
 * Need:
 * intptr_t
 */
#ifdef ZSTD_DEPS_NEED_STDINT
#ifndef ZSTD_DEPS_STDINT
#define ZSTD_DEPS_STDINT

#include <stdint.h>

#endif /* ZSTD_DEPS_STDINT */
#endif /* ZSTD_DEPS_NEED_STDINT */
/* ---- end common/zstd_deps.h ---- */

/* ****************************************
*  Compiler-specific
******************************************/
#if defined(__GNUC__)
#  define ERR_STATIC static __attribute__((unused))
#elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
#  define ERR_STATIC static inline
#elif defined(_MSC_VER)
#  define ERR_STATIC static __inline
#else
#  define ERR_STATIC static  /* this version may generate warnings for unused static functions; disable the relevant warning */
#endif


/*-****************************************
*  Customization (error_public.h)
******************************************/
typedef ZSTD_ErrorCode ERR_enum;
#define PREFIX(name) ZSTD_error_##name


/*-****************************************
*  Error codes handling
******************************************/
#undef ERROR   /* already defined on Visual Studio */
#define ERROR(name) ZSTD_ERROR(name)
#define ZSTD_ERROR(name) ((size_t)-PREFIX(name))

ERR_STATIC unsigned ERR_isError(size_t code) { return (code > ERROR(maxCode)); }

ERR_STATIC ERR_enum ERR_getErrorCode(size_t code) { if (!ERR_isError(code)) return (ERR_enum)0; return (ERR_enum) (0-code); }

/* check and forward error code */
#define CHECK_V_F(e, f)     \
    size_t const e = f;     \
    do {                    \
        if (ERR_isError(e)) \
            return e;       \
    } while (0)
#define CHECK_F(f)   do { CHECK_V_F(_var_err__, f); } while (0)


/*-****************************************
*  Error Strings
******************************************/

extern "C" const char* ERR_getErrorString(ERR_enum code);   /* error_private.c */

ERR_STATIC const char* ERR_getErrorName(size_t code)
{
    return ERR_getErrorString(ERR_getErrorCode(code));
}

/**
 * Ignore: this is an internal helper.
 *
 * This is a helper function to help force C99-correctness during compilation.
 * Under strict compilation modes, variadic macro arguments can't be empty.
 * However, variadic function arguments can be. Using a function therefore lets
 * us statically check that at least one (string) argument was passed,
 * independent of the compilation flags.
 */
static INLINE_KEYWORD UNUSED_ATTR
void _force_has_format_string(const char *format, ...) {
  (void)format;
}

/**
 * Ignore: this is an internal helper.
 *
 * We want to force this function invocation to be syntactically correct, but
 * we don't want to force runtime evaluation of its arguments.
 */
#define _FORCE_HAS_FORMAT_STRING(...)              \
    do {                                           \
        if (0) {                                   \
            _force_has_format_string(__VA_ARGS__); \
        }                                          \
    } while (0)

#define ERR_QUOTE(str) #str

/**
 * Return the specified error if the condition evaluates to true.
 *
 * In debug modes, prints additional information.
 * In order to do that (particularly, printing the conditional that failed),
 * this can't just wrap RETURN_ERROR().
 */
#define RETURN_ERROR_IF(cond, err, ...)                                        \
    do {                                                                       \
        if (cond) {                                                            \
            RAWLOG(3, "%s:%d: ERROR!: check %s failed, returning %s",          \
                  __FILE__, __LINE__, ERR_QUOTE(cond), ERR_QUOTE(ERROR(err))); \
            _FORCE_HAS_FORMAT_STRING(__VA_ARGS__);                             \
            RAWLOG(3, ": " __VA_ARGS__);                                       \
            RAWLOG(3, "\n");                                                   \
            return ERROR(err);                                                 \
        }                                                                      \
    } while (0)

/**
 * Unconditionally return the specified error.
 *
 * In debug modes, prints additional information.
 */
#define RETURN_ERROR(err, ...)                                               \
    do {                                                                     \
        RAWLOG(3, "%s:%d: ERROR!: unconditional check failed, returning %s", \
              __FILE__, __LINE__, ERR_QUOTE(ERROR(err)));                    \
        _FORCE_HAS_FORMAT_STRING(__VA_ARGS__);                               \
        RAWLOG(3, ": " __VA_ARGS__);                                         \
        RAWLOG(3, "\n");                                                     \
        return ERROR(err);                                                   \
    } while(0)

/**
 * If the provided expression evaluates to an error code, returns that error code.
 *
 * In debug modes, prints additional information.
 */
#define FORWARD_IF_ERROR(err, ...)                                                 \
    do {                                                                           \
        size_t const err_code = (err);                                             \
        if (ERR_isError(err_code)) {                                               \
            RAWLOG(3, "%s:%d: ERROR!: forwarding error in %s: %s",                 \
                  __FILE__, __LINE__, ERR_QUOTE(err), ERR_getErrorName(err_code)); \
            _FORCE_HAS_FORMAT_STRING(__VA_ARGS__);                                 \
            RAWLOG(3, ": " __VA_ARGS__);                                           \
            RAWLOG(3, "\n");                                                       \
            return err_code;                                                       \
        }                                                                          \
    } while(0)

#endif /* ERROR_H_MODULE */
/* ---- end common/error_private.h ---- */


/* ******************************************************************
   mem.h
   low-level memory access routines
   Copyright (C) 2013-2015, Yann Collet.

   BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - FSEv05 source repository : https://github.com/Cyan4973/FiniteStateEntropy
    - Public forum : https://groups.google.com/forum/#!forum/lz4c
****************************************************************** */
#ifndef MEM_H_MODULE
#define MEM_H_MODULE

#if defined (__cplusplus)
extern "C" {
#endif

/*-****************************************
*  Dependencies
******************************************/
#include <stddef.h>    /* size_t, ptrdiff_t */
#include <string.h>    /* memcpy */


/*-****************************************
*  Compiler specifics
******************************************/
#if defined(__GNUC__)
#  define MEM_STATIC static __attribute__((unused))
#elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
#  define MEM_STATIC static inline
#elif defined(_MSC_VER)
#  define MEM_STATIC static __inline
#else
#  define MEM_STATIC static  /* this version may generate warnings for unused static functions; disable the relevant warning */
#endif


/*-**************************************************************
*  Basic Types
*****************************************************************/
#if defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
# if defined(_AIX)
#  include <inttypes.h>
# else
#  include <stdint.h> /* intptr_t */
# endif
  typedef  uint8_t BYTE;
  typedef uint16_t U16;
  typedef  int16_t S16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
  typedef  int64_t S64;
#else
  typedef unsigned char       BYTE;
  typedef unsigned short      U16;
  typedef   signed short      S16;
  typedef unsigned int        U32;
  typedef   signed int        S32;
  typedef unsigned long long  U64;
  typedef   signed long long  S64;
#endif


/*-**************************************************************
*  Memory I/O
*****************************************************************/

MEM_STATIC unsigned MEM_32bits(void) { return sizeof(void*)==4; }
MEM_STATIC unsigned MEM_64bits(void) { return sizeof(void*)==8; }

MEM_STATIC unsigned MEM_isLittleEndian(void)
{
    const union { U32 u; BYTE c[4]; } one = { 1 };   /* don't use static : performance detrimental  */
    return one.c[0];
}

MEM_STATIC U16 MEM_read16(const void* memPtr)
{
    U16 val; memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC U32 MEM_read32(const void* memPtr)
{
    U32 val; memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC U64 MEM_read64(const void* memPtr)
{
    U64 val; memcpy(&val, memPtr, sizeof(val)); return val;
}

MEM_STATIC void MEM_write16(void* memPtr, U16 value)
{
    memcpy(memPtr, &value, sizeof(value));
}

MEM_STATIC void MEM_write32(void* memPtr, U32 value)
{
    memcpy(memPtr, &value, sizeof(value));
}

MEM_STATIC void MEM_write64(void* memPtr, U64 value)
{
    memcpy(memPtr, &value, sizeof(value));
}

MEM_STATIC U16 MEM_readLE16(const void* memPtr)
{
    if (MEM_isLittleEndian())
        return MEM_read16(memPtr);
    else {
        const BYTE* p = (const BYTE*)memPtr;
        return (U16)(p[0] + (p[1]<<8));
    }
}

MEM_STATIC void MEM_writeLE16(void* memPtr, U16 val)
{
    if (MEM_isLittleEndian()) {
        MEM_write16(memPtr, val);
    } else {
        BYTE* p = (BYTE*)memPtr;
        p[0] = (BYTE)val;
        p[1] = (BYTE)(val>>8);
    }
}

MEM_STATIC U32 MEM_readLE32(const void* memPtr)
{
    if (MEM_isLittleEndian())
        return MEM_read32(memPtr);
    else {
        const BYTE* p = (const BYTE*)memPtr;
        return (U32)((U32)p[0] + ((U32)p[1]<<8) + ((U32)p[2]<<16) + ((U32)p[3]<<24));
    }
}


MEM_STATIC U64 MEM_readLE64(const void* memPtr)
{
    if (MEM_isLittleEndian())
        return MEM_read64(memPtr);
    else {
        const BYTE* p = (const BYTE*)memPtr;
        return (U64)((U64)p[0] + ((U64)p[1]<<8) + ((U64)p[2]<<16) + ((U64)p[3]<<24)
                     + ((U64)p[4]<<32) + ((U64)p[5]<<40) + ((U64)p[6]<<48) + ((U64)p[7]<<56));
    }
}


MEM_STATIC size_t MEM_readLEST(const void* memPtr)
{
    if (MEM_32bits())
        return (size_t)MEM_readLE32(memPtr);
    else
        return (size_t)MEM_readLE64(memPtr);
}


#if defined (__cplusplus)
}
#endif

#endif /* MEM_H_MODULE */

/*
    zstd - standard compression library
    Header File for static linking only
    Copyright (C) 2014-2016, Yann Collet.

    BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - zstd homepage : https://facebook.github.io/zstd
*/
#ifndef ZSTD_STATIC_H
#define ZSTD_STATIC_H

/* The prototypes defined within this file are considered experimental.
 * They should not be used in the context DLL as they may change in the future.
 * Prefer static linking if you need them, to control breaking version changes issues.
 */

#if defined (__cplusplus)
extern "C" {
#endif



/*-*************************************
*  Types
***************************************/
#define ZSTDv05_WINDOWLOG_ABSOLUTEMIN 11


/*-*************************************
*  Advanced functions
***************************************/
/*- Advanced Decompression functions -*/

/*! ZSTDv05_decompress_usingPreparedDCtx() :
*   Same as ZSTDv05_decompress_usingDict, but using a reference context `preparedDCtx`, where dictionary has been loaded.
*   It avoids reloading the dictionary each time.
*   `preparedDCtx` must have been properly initialized using ZSTDv05_decompressBegin_usingDict().
*   Requires 2 contexts : 1 for reference, which will not be modified, and 1 to run the decompression operation */
size_t ZSTDv05_decompress_usingPreparedDCtx(
                                             ZSTDv05_DCtx* dctx, const ZSTDv05_DCtx* preparedDCtx,
                                             void* dst, size_t dstCapacity,
                                       const void* src, size_t srcSize);


/* **************************************
*  Streaming functions (direct mode)
****************************************/
size_t ZSTDv05_decompressBegin(ZSTDv05_DCtx* dctx);

/*
  Streaming decompression, direct mode (bufferless)

  A ZSTDv05_DCtx object is required to track streaming operations.
  Use ZSTDv05_createDCtx() / ZSTDv05_freeDCtx() to manage it.
  A ZSTDv05_DCtx object can be re-used multiple times.

  First typical operation is to retrieve frame parameters, using ZSTDv05_getFrameParams().
  This operation is independent, and just needs enough input data to properly decode the frame header.
  Objective is to retrieve *params.windowlog, to know minimum amount of memory required during decoding.
  Result : 0 when successful, it means the ZSTDv05_parameters structure has been filled.
           >0 : means there is not enough data into src. Provides the expected size to successfully decode header.
           errorCode, which can be tested using ZSTDv05_isError()

  Start decompression, with ZSTDv05_decompressBegin() or ZSTDv05_decompressBegin_usingDict()
  Alternatively, you can copy a prepared context, using ZSTDv05_copyDCtx()

  Then use ZSTDv05_nextSrcSizeToDecompress() and ZSTDv05_decompressContinue() alternatively.
  ZSTDv05_nextSrcSizeToDecompress() tells how much bytes to provide as 'srcSize' to ZSTDv05_decompressContinue().
  ZSTDv05_decompressContinue() requires this exact amount of bytes, or it will fail.
  ZSTDv05_decompressContinue() needs previous data blocks during decompression, up to (1 << windowlog).
  They should preferably be located contiguously, prior to current block. Alternatively, a round buffer is also possible.

  @result of ZSTDv05_decompressContinue() is the number of bytes regenerated within 'dst'.
  It can be zero, which is not an error; it just means ZSTDv05_decompressContinue() has decoded some header.

  A frame is fully decoded when ZSTDv05_nextSrcSizeToDecompress() returns zero.
  Context can then be reset to start a new decompression.
*/


/* **************************************
*  Block functions
****************************************/
/*! Block functions produce and decode raw zstd blocks, without frame metadata.
    User will have to take in charge required information to regenerate data, such as block sizes.

    A few rules to respect :
    - Uncompressed block size must be <= 128 KB
    - Compressing or decompressing requires a context structure
      + Use ZSTDv05_createCCtx() and ZSTDv05_createDCtx()
    - It is necessary to init context before starting
      + compression : ZSTDv05_compressBegin()
      + decompression : ZSTDv05_decompressBegin()
      + variants _usingDict() are also allowed
      + copyCCtx() and copyDCtx() work too
    - When a block is considered not compressible enough, ZSTDv05_compressBlock() result will be zero.
      In which case, nothing is produced into `dst`.
      + User must test for such outcome and deal directly with uncompressed data
      + ZSTDv05_decompressBlock() doesn't accept uncompressed data as input !!
*/

size_t ZSTDv05_decompressBlock(ZSTDv05_DCtx* dctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);




#if defined (__cplusplus)
}
#endif

#endif  /* ZSTDv05_STATIC_H */


/*
    zstd_internal - common functions to include
    Header File for include
    Copyright (C) 2014-2016, Yann Collet.

    BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - zstd source repository : https://github.com/Cyan4973/zstd
*/
#ifndef ZSTD_CCOMMON_H_MODULE
#define ZSTD_CCOMMON_H_MODULE



/*-*************************************
*  Common macros
***************************************/
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))


/*-*************************************
*  Common constants
***************************************/
#define ZSTDv05_DICT_MAGIC  0xEC30A435

#define KB *(1 <<10)
#define MB *(1 <<20)
#define GB *(1U<<30)

#define BLOCKSIZE (128 KB)                 /* define, for static allocation */

static const size_t ZSTDv05_blockHeaderSize = 3;
static const size_t ZSTDv05_frameHeaderSize_min = 5;
#define ZSTDv05_frameHeaderSize_max 5         /* define, for static allocation */

#define BITv057 128
#define BITv056  64
#define BITv055  32
#define BITv054  16
#define BITv051   2
#define BITv050   1

#define IS_HUFv05 0
#define IS_PCH 1
#define IS_RAW 2
#define IS_RLE 3

#define MINMATCH 4
#define REPCODE_STARTVALUE 1

#define Litbits  8
#define MLbits   7
#define LLbits   6
#define Offbits  5
#define MaxLit ((1<<Litbits) - 1)
#define MaxML  ((1<<MLbits) - 1)
#define MaxLL  ((1<<LLbits) - 1)
#define MaxOff ((1<<Offbits)- 1)
#define MLFSEv05Log   10
#define LLFSEv05Log   10
#define OffFSEv05Log   9
#define MaxSeq MAX(MaxLL, MaxML)

#define FSEv05_ENCODING_RAW     0
#define FSEv05_ENCODING_RLE     1
#define FSEv05_ENCODING_STATIC  2
#define FSEv05_ENCODING_DYNAMIC 3


#define ZSTD_HUFFDTABLE_CAPACITY_LOG 12

#define MIN_SEQUENCES_SIZE 1 /* nbSeq==0 */
#define MIN_CBLOCK_SIZE (1 /*litCSize*/ + 1 /* RLE or RAW */ + MIN_SEQUENCES_SIZE /* nbSeq==0 */)   /* for a non-null block */

#define WILDCOPY_OVERLENGTH 8

#define ZSTD_CONTENTSIZE_ERROR   (0ULL - 2)

typedef enum { bt_compressed, bt_raw, bt_rle, bt_end } blockType_t;


/*-*******************************************
*  Shared functions to include for inlining
*********************************************/
static void ZSTDv05_copy8(void* dst, const void* src) { memcpy(dst, src, 8); }

#define COPY8(d,s) { ZSTDv05_copy8(d,s); d+=8; s+=8; }

/*! ZSTDv05_wildcopy() :
*   custom version of memcpy(), can copy up to 7 bytes too many (8 bytes if length==0) */
MEM_STATIC void ZSTDv05_wildcopy(void* dst, const void* src, ptrdiff_t length)
{
    const BYTE* ip = (const BYTE*)src;
    BYTE* op = (BYTE*)dst;
    BYTE* const oend = op + length;
    do
        COPY8(op, ip)
    while (op < oend);
}


/*-*******************************************
*  Private interfaces
*********************************************/
typedef struct {
    void* buffer;
    U32*  offsetStart;
    U32*  offset;
    BYTE* offCodeStart;
    BYTE* offCode;
    BYTE* litStart;
    BYTE* lit;
    BYTE* litLengthStart;
    BYTE* litLength;
    BYTE* matchLengthStart;
    BYTE* matchLength;
    BYTE* dumpsStart;
    BYTE* dumps;
    /* opt */
    U32* matchLengthFreq;
    U32* litLengthFreq;
    U32* litFreq;
    U32* offCodeFreq;
    U32  matchLengthSum;
    U32  litLengthSum;
    U32  litSum;
    U32  offCodeSum;
} SeqStore_t;



#endif   /* ZSTDv05_CCOMMON_H_MODULE */
/* ******************************************************************
   FSEv05 : Finite State Entropy coder
   header file
   Copyright (C) 2013-2015, Yann Collet.

   BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
   - Public forum : https://groups.google.com/forum/#!forum/lz4c
****************************************************************** */
#ifndef FSEv05_H
#define FSEv05_H

#if defined (__cplusplus)
extern "C" {
#endif


/* *****************************************
*  Includes
******************************************/
#include <stddef.h>    /* size_t, ptrdiff_t */


/*-****************************************
*  FSEv05 simple functions
******************************************/
size_t FSEv05_decompress(void* dst,  size_t maxDstSize,
                const void* cSrc, size_t cSrcSize);
/*!
FSEv05_decompress():
    Decompress FSEv05 data from buffer 'cSrc', of size 'cSrcSize',
    into already allocated destination buffer 'dst', of size 'maxDstSize'.
    return : size of regenerated data (<= maxDstSize)
             or an error code, which can be tested using FSEv05_isError()

    ** Important ** : FSEv05_decompress() doesn't decompress non-compressible nor RLE data !!!
    Why ? : making this distinction requires a header.
    Header management is intentionally delegated to the user layer, which can better manage special cases.
*/


/* *****************************************
*  Tool functions
******************************************/
/* Error Management */
unsigned    FSEv05_isError(size_t code);        /* tells if a return value is an error code */
const char* FSEv05_getErrorName(size_t code);   /* provides error code string (useful for debugging) */




/* *****************************************
*  FSEv05 detailed API
******************************************/
/* *** DECOMPRESSION *** */

/*!
FSEv05_readNCount():
   Read compactly saved 'normalizedCounter' from 'rBuffer'.
   return : size read from 'rBuffer'
            or an errorCode, which can be tested using FSEv05_isError()
            maxSymbolValuePtr[0] and tableLogPtr[0] will also be updated with their respective values */
size_t FSEv05_readNCount (short* normalizedCounter, unsigned* maxSymbolValuePtr, unsigned* tableLogPtr, const void* rBuffer, size_t rBuffSize);

/*!
Constructor and Destructor of type FSEv05_DTable
    Note that its size depends on 'tableLog' */
typedef unsigned FSEv05_DTable;   /* don't allocate that. It's just a way to be more restrictive than void* */
FSEv05_DTable* FSEv05_createDTable(unsigned tableLog);
void        FSEv05_freeDTable(FSEv05_DTable* dt);

/*!
FSEv05_buildDTable():
   Builds 'dt', which must be already allocated, using FSEv05_createDTable()
   @return : 0,
             or an errorCode, which can be tested using FSEv05_isError() */
size_t FSEv05_buildDTable (FSEv05_DTable* dt, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog);

/*!
FSEv05_decompress_usingDTable():
   Decompress compressed source @cSrc of size @cSrcSize using `dt`
   into `dst` which must be already allocated.
   @return : size of regenerated data (necessarily <= @dstCapacity)
             or an errorCode, which can be tested using FSEv05_isError() */
size_t FSEv05_decompress_usingDTable(void* dst, size_t dstCapacity, const void* cSrc, size_t cSrcSize, const FSEv05_DTable* dt);



#if defined (__cplusplus)
}
#endif

#endif  /* FSEv05_H */
/* ******************************************************************
   bitstream
   Part of FSEv05 library
   header file (to include)
   Copyright (C) 2013-2016, Yann Collet.

   BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
****************************************************************** */
#ifndef BITv05STREAM_H_MODULE
#define BITv05STREAM_H_MODULE

#if defined (__cplusplus)
extern "C" {
#endif


/*
*  This API consists of small unitary functions, which highly benefit from being inlined.
*  Since link-time-optimization is not available for all compilers,
*  these functions are defined into a .h to be included.
*/



/*-********************************************
*  bitStream decoding API (read backward)
**********************************************/
typedef struct
{
    size_t   bitContainer;
    unsigned bitsConsumed;
    const char* ptr;
    const char* start;
} BITv05_DStream_t;

typedef enum { BITv05_DStream_unfinished = 0,
               BITv05_DStream_endOfBuffer = 1,
               BITv05_DStream_completed = 2,
               BITv05_DStream_overflow = 3 } BITv05_DStream_status;  /* result of BITv05_reloadDStream() */
               /* 1,2,4,8 would be better for bitmap combinations, but slows down performance a bit ... :( */

MEM_STATIC size_t   BITv05_initDStream(BITv05_DStream_t* bitD, const void* srcBuffer, size_t srcSize);
MEM_STATIC size_t   BITv05_readBits(BITv05_DStream_t* bitD, unsigned nbBits);
MEM_STATIC BITv05_DStream_status BITv05_reloadDStream(BITv05_DStream_t* bitD);
MEM_STATIC unsigned BITv05_endOfDStream(const BITv05_DStream_t* bitD);


/*-****************************************
*  unsafe API
******************************************/
MEM_STATIC size_t BITv05_readBitsFast(BITv05_DStream_t* bitD, unsigned nbBits);
/* faster, but works only if nbBits >= 1 */



/*-**************************************************************
*  Helper functions
****************************************************************/
MEM_STATIC unsigned BITv05_highbit32 (U32 val)
{
#   if defined(_MSC_VER)   /* Visual */
    unsigned long r;
    return _BitScanReverse(&r, val) ? (unsigned)r : 0;
#   elif defined(__GNUC__) && (__GNUC__ >= 3)   /* Use GCC Intrinsic */
    return __builtin_clz (val) ^ 31;
#   else   /* Software version */
    static const unsigned DeBruijnClz[32] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 };
    U32 v = val;
    unsigned r;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    r = DeBruijnClz[ (U32) (v * 0x07C4ACDDU) >> 27];
    return r;
#   endif
}



/*-********************************************************
* bitStream decoding
**********************************************************/
/*!BITv05_initDStream
*  Initialize a BITv05_DStream_t.
*  @bitD : a pointer to an already allocated BITv05_DStream_t structure
*  @srcBuffer must point at the beginning of a bitStream
*  @srcSize must be the exact size of the bitStream
*  @result : size of stream (== srcSize) or an errorCode if a problem is detected
*/
MEM_STATIC size_t BITv05_initDStream(BITv05_DStream_t* bitD, const void* srcBuffer, size_t srcSize)
{
    if (srcSize < 1) { memset(bitD, 0, sizeof(*bitD)); return ERROR(srcSize_wrong); }

    if (srcSize >=  sizeof(size_t)) {  /* normal case */
        U32 contain32;
        bitD->start = (const char*)srcBuffer;
        bitD->ptr   = (const char*)srcBuffer + srcSize - sizeof(size_t);
        bitD->bitContainer = MEM_readLEST(bitD->ptr);
        contain32 = ((const BYTE*)srcBuffer)[srcSize-1];
        if (contain32 == 0) return ERROR(GENERIC);   /* endMark not present */
        bitD->bitsConsumed = 8 - BITv05_highbit32(contain32);
    } else {
        U32 contain32;
        bitD->start = (const char*)srcBuffer;
        bitD->ptr   = bitD->start;
        bitD->bitContainer = *(const BYTE*)(bitD->start);
        switch(srcSize)
        {
            case 7: bitD->bitContainer += (size_t)(((const BYTE*)(bitD->start))[6]) << (sizeof(size_t)*8 - 16);/* fall-through */
            case 6: bitD->bitContainer += (size_t)(((const BYTE*)(bitD->start))[5]) << (sizeof(size_t)*8 - 24);/* fall-through */
            case 5: bitD->bitContainer += (size_t)(((const BYTE*)(bitD->start))[4]) << (sizeof(size_t)*8 - 32);/* fall-through */
            case 4: bitD->bitContainer += (size_t)(((const BYTE*)(bitD->start))[3]) << 24; /* fall-through */
            case 3: bitD->bitContainer += (size_t)(((const BYTE*)(bitD->start))[2]) << 16; /* fall-through */
            case 2: bitD->bitContainer += (size_t)(((const BYTE*)(bitD->start))[1]) <<  8; /* fall-through */
            default: break;
        }
        contain32 = ((const BYTE*)srcBuffer)[srcSize-1];
        if (contain32 == 0) return ERROR(GENERIC);   /* endMark not present */
        bitD->bitsConsumed = 8 - BITv05_highbit32(contain32);
        bitD->bitsConsumed += (U32)(sizeof(size_t) - srcSize)*8;
    }

    return srcSize;
}

MEM_STATIC size_t BITv05_lookBits(BITv05_DStream_t* bitD, U32 nbBits)
{
    const U32 bitMask = sizeof(bitD->bitContainer)*8 - 1;
    return ((bitD->bitContainer << (bitD->bitsConsumed & bitMask)) >> 1) >> ((bitMask-nbBits) & bitMask);
}

/*! BITv05_lookBitsFast :
*   unsafe version; only works if nbBits >= 1 */
MEM_STATIC size_t BITv05_lookBitsFast(BITv05_DStream_t* bitD, U32 nbBits)
{
    const U32 bitMask = sizeof(bitD->bitContainer)*8 - 1;
    return (bitD->bitContainer << (bitD->bitsConsumed & bitMask)) >> (((bitMask+1)-nbBits) & bitMask);
}

MEM_STATIC void BITv05_skipBits(BITv05_DStream_t* bitD, U32 nbBits)
{
    bitD->bitsConsumed += nbBits;
}

MEM_STATIC size_t BITv05_readBits(BITv05_DStream_t* bitD, unsigned nbBits)
{
    size_t value = BITv05_lookBits(bitD, nbBits);
    BITv05_skipBits(bitD, nbBits);
    return value;
}

/*!BITv05_readBitsFast :
*  unsafe version; only works if nbBits >= 1 */
MEM_STATIC size_t BITv05_readBitsFast(BITv05_DStream_t* bitD, unsigned nbBits)
{
    size_t value = BITv05_lookBitsFast(bitD, nbBits);
    BITv05_skipBits(bitD, nbBits);
    return value;
}

MEM_STATIC BITv05_DStream_status BITv05_reloadDStream(BITv05_DStream_t* bitD)
{
    if (bitD->bitsConsumed > (sizeof(bitD->bitContainer)*8))  /* should never happen */
        return BITv05_DStream_overflow;

    if (bitD->ptr >= bitD->start + sizeof(bitD->bitContainer)) {
        bitD->ptr -= bitD->bitsConsumed >> 3;
        bitD->bitsConsumed &= 7;
        bitD->bitContainer = MEM_readLEST(bitD->ptr);
        return BITv05_DStream_unfinished;
    }
    if (bitD->ptr == bitD->start) {
        if (bitD->bitsConsumed < sizeof(bitD->bitContainer)*8) return BITv05_DStream_endOfBuffer;
        return BITv05_DStream_completed;
    }
    {
        U32 nbBytes = bitD->bitsConsumed >> 3;
        BITv05_DStream_status result = BITv05_DStream_unfinished;
        if (bitD->ptr - nbBytes < bitD->start) {
            nbBytes = (U32)(bitD->ptr - bitD->start);  /* ptr > start */
            result = BITv05_DStream_endOfBuffer;
        }
        bitD->ptr -= nbBytes;
        bitD->bitsConsumed -= nbBytes*8;
        bitD->bitContainer = MEM_readLEST(bitD->ptr);   /* reminder : srcSize > sizeof(bitD) */
        return result;
    }
}

/*! BITv05_endOfDStream
*   @return Tells if DStream has reached its exact end
*/
MEM_STATIC unsigned BITv05_endOfDStream(const BITv05_DStream_t* DStream)
{
    return ((DStream->ptr == DStream->start) && (DStream->bitsConsumed == sizeof(DStream->bitContainer)*8));
}

#if defined (__cplusplus)
}
#endif

#endif /* BITv05STREAM_H_MODULE */
/* ******************************************************************
   FSEv05 : Finite State Entropy coder
   header file for static linking (only)
   Copyright (C) 2013-2015, Yann Collet

   BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
   - Public forum : https://groups.google.com/forum/#!forum/lz4c
****************************************************************** */
#ifndef FSEv05_STATIC_H
#define FSEv05_STATIC_H

#if defined (__cplusplus)
extern "C" {
#endif



/* *****************************************
*  Static allocation
*******************************************/
/* It is possible to statically allocate FSEv05 CTable/DTable as a table of unsigned using below macros */
#define FSEv05_DTABLE_SIZE_U32(maxTableLog)                   (1 + (1<<maxTableLog))


/* *****************************************
*  FSEv05 advanced API
*******************************************/
size_t FSEv05_buildDTable_raw (FSEv05_DTable* dt, unsigned nbBits);
/* build a fake FSEv05_DTable, designed to read an uncompressed bitstream where each symbol uses nbBits */

size_t FSEv05_buildDTable_rle (FSEv05_DTable* dt, unsigned char symbolValue);
/* build a fake FSEv05_DTable, designed to always generate the same symbolValue */



/* *****************************************
*  FSEv05 symbol decompression API
*******************************************/
typedef struct
{
    size_t      state;
    const void* table;   /* precise table may vary, depending on U16 */
} FSEv05_DState_t;


static void     FSEv05_initDState(FSEv05_DState_t* DStatePtr, BITv05_DStream_t* bitD, const FSEv05_DTable* dt);

static unsigned char FSEv05_decodeSymbol(FSEv05_DState_t* DStatePtr, BITv05_DStream_t* bitD);

static unsigned FSEv05_endOfDState(const FSEv05_DState_t* DStatePtr);



/* *****************************************
*  FSEv05 unsafe API
*******************************************/
static unsigned char FSEv05_decodeSymbolFast(FSEv05_DState_t* DStatePtr, BITv05_DStream_t* bitD);
/* faster, but works only if nbBits is always >= 1 (otherwise, result will be corrupted) */


/* *****************************************
*  Implementation of inlined functions
*******************************************/
/* decompression */

typedef struct {
    U16 tableLog;
    U16 fastMode;
} FSEv05_DTableHeader;   /* sizeof U32 */

typedef struct
{
    unsigned short newState;
    unsigned char  symbol;
    unsigned char  nbBits;
} FSEv05_decode_t;   /* size == U32 */

MEM_STATIC void FSEv05_initDState(FSEv05_DState_t* DStatePtr, BITv05_DStream_t* bitD, const FSEv05_DTable* dt)
{
    const void* ptr = dt;
    const FSEv05_DTableHeader* const DTableH = (const FSEv05_DTableHeader*)ptr;
    DStatePtr->state = BITv05_readBits(bitD, DTableH->tableLog);
    BITv05_reloadDStream(bitD);
    DStatePtr->table = dt + 1;
}

MEM_STATIC BYTE FSEv05_peakSymbol(FSEv05_DState_t* DStatePtr)
{
    const FSEv05_decode_t DInfo = ((const FSEv05_decode_t*)(DStatePtr->table))[DStatePtr->state];
    return DInfo.symbol;
}

MEM_STATIC BYTE FSEv05_decodeSymbol(FSEv05_DState_t* DStatePtr, BITv05_DStream_t* bitD)
{
    const FSEv05_decode_t DInfo = ((const FSEv05_decode_t*)(DStatePtr->table))[DStatePtr->state];
    const U32  nbBits = DInfo.nbBits;
    BYTE symbol = DInfo.symbol;
    size_t lowBits = BITv05_readBits(bitD, nbBits);

    DStatePtr->state = DInfo.newState + lowBits;
    return symbol;
}

MEM_STATIC BYTE FSEv05_decodeSymbolFast(FSEv05_DState_t* DStatePtr, BITv05_DStream_t* bitD)
{
    const FSEv05_decode_t DInfo = ((const FSEv05_decode_t*)(DStatePtr->table))[DStatePtr->state];
    const U32 nbBits = DInfo.nbBits;
    BYTE symbol = DInfo.symbol;
    size_t lowBits = BITv05_readBitsFast(bitD, nbBits);

    DStatePtr->state = DInfo.newState + lowBits;
    return symbol;
}

MEM_STATIC unsigned FSEv05_endOfDState(const FSEv05_DState_t* DStatePtr)
{
    return DStatePtr->state == 0;
}


#if defined (__cplusplus)
}
#endif

#endif  /* FSEv05_STATIC_H */
/* ******************************************************************
   FSEv05 : Finite State Entropy coder
   Copyright (C) 2013-2015, Yann Collet.

   BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - FSEv05 source repository : https://github.com/Cyan4973/FiniteStateEntropy
    - Public forum : https://groups.google.com/forum/#!forum/lz4c
****************************************************************** */

#ifndef FSEv05_COMMONDEFS_ONLY

/* **************************************************************
*  Tuning parameters
****************************************************************/
/*!MEMORY_USAGE :
*  Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
*  Increasing memory usage improves compression ratio
*  Reduced memory usage can improve speed, due to cache effect
*  Recommended max value is 14, for 16KB, which nicely fits into Intel x86 L1 cache */
#define FSEv05_MAX_MEMORY_USAGE 14
#define FSEv05_DEFAULT_MEMORY_USAGE 13

/*!FSEv05_MAX_SYMBOL_VALUE :
*  Maximum symbol value authorized.
*  Required for proper stack allocation */
#define FSEv05_MAX_SYMBOL_VALUE 255


/* **************************************************************
*  template functions type & suffix
****************************************************************/
#define FSEv05_FUNCTION_TYPE BYTE
#define FSEv05_FUNCTION_EXTENSION
#define FSEv05_DECODE_TYPE FSEv05_decode_t


#endif   /* !FSEv05_COMMONDEFS_ONLY */

/* **************************************************************
*  Compiler specifics
****************************************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  define FORCE_INLINE static __forceinline
#  include <intrin.h>                    /* For Visual 2005 */
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#  pragma warning(disable : 4214)        /* disable: C4214: non-int bitfields */
#else
#  if defined (__cplusplus) || defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
#    ifdef __GNUC__
#      define FORCE_INLINE static inline __attribute__((always_inline))
#    else
#      define FORCE_INLINE static inline
#    endif
#  else
#    define FORCE_INLINE static
#  endif /* __STDC_VERSION__ */
#endif


/* **************************************************************
*  Includes
****************************************************************/
#include <stdlib.h>     /* malloc, free, qsort */
#include <string.h>     /* memcpy, memset */
#include <stdio.h>      /* printf (debug) */



/* ***************************************************************
*  Constants
*****************************************************************/
#define FSEv05_MAX_TABLELOG  (FSEv05_MAX_MEMORY_USAGE-2)
#define FSEv05_MAX_TABLESIZE (1U<<FSEv05_MAX_TABLELOG)
#define FSEv05_MAXTABLESIZE_MASK (FSEv05_MAX_TABLESIZE-1)
#define FSEv05_DEFAULT_TABLELOG (FSEv05_DEFAULT_MEMORY_USAGE-2)
#define FSEv05_MIN_TABLELOG 5

#define FSEv05_TABLELOG_ABSOLUTE_MAX 15
#if FSEv05_MAX_TABLELOG > FSEv05_TABLELOG_ABSOLUTE_MAX
#error "FSEv05_MAX_TABLELOG > FSEv05_TABLELOG_ABSOLUTE_MAX is not supported"
#endif


/* **************************************************************
*  Error Management
****************************************************************/
#define FSEv05_STATIC_ASSERT(c) { enum { FSEv05_static_assert = 1/(int)(!!(c)) }; }   /* use only *after* variable declarations */


/* **************************************************************
*  Complex types
****************************************************************/
typedef unsigned DTable_max_t[FSEv05_DTABLE_SIZE_U32(FSEv05_MAX_TABLELOG)];


/* **************************************************************
*  Templates
****************************************************************/
/*
  designed to be included
  for type-specific functions (template emulation in C)
  Objective is to write these functions only once, for improved maintenance
*/

/* safety checks */
#ifndef FSEv05_FUNCTION_EXTENSION
#  error "FSEv05_FUNCTION_EXTENSION must be defined"
#endif
#ifndef FSEv05_FUNCTION_TYPE
#  error "FSEv05_FUNCTION_TYPE must be defined"
#endif

/* Function names */
#define FSEv05_CAT(X,Y) X##Y
#define FSEv05_FUNCTION_NAME(X,Y) FSEv05_CAT(X,Y)
#define FSEv05_TYPE_NAME(X,Y) FSEv05_CAT(X,Y)


/* Function templates */
static U32 FSEv05_tableStep(U32 tableSize) { return (tableSize>>1) + (tableSize>>3) + 3; }



FSEv05_DTable* FSEv05_createDTable (unsigned tableLog)
{
    if (tableLog > FSEv05_TABLELOG_ABSOLUTE_MAX) tableLog = FSEv05_TABLELOG_ABSOLUTE_MAX;
    return (FSEv05_DTable*)malloc( FSEv05_DTABLE_SIZE_U32(tableLog) * sizeof (U32) );
}

void FSEv05_freeDTable (FSEv05_DTable* dt)
{
    free(dt);
}

size_t FSEv05_buildDTable(FSEv05_DTable* dt, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog)
{
    FSEv05_DTableHeader DTableH;
    void* const tdPtr = dt+1;   /* because dt is unsigned, 32-bits aligned on 32-bits */
    FSEv05_DECODE_TYPE* const tableDecode = (FSEv05_DECODE_TYPE*) (tdPtr);
    const U32 tableSize = 1 << tableLog;
    const U32 tableMask = tableSize-1;
    const U32 step = FSEv05_tableStep(tableSize);
    U16 symbolNext[FSEv05_MAX_SYMBOL_VALUE+1];
    U32 position = 0;
    U32 highThreshold = tableSize-1;
    const S16 largeLimit= (S16)(1 << (tableLog-1));
    U32 noLarge = 1;
    U32 s;

    /* Sanity Checks */
    if (maxSymbolValue > FSEv05_MAX_SYMBOL_VALUE) return ERROR(maxSymbolValue_tooLarge);
    if (tableLog > FSEv05_MAX_TABLELOG) return ERROR(tableLog_tooLarge);

    /* Init, lay down lowprob symbols */
    memset(tableDecode, 0, sizeof(FSEv05_FUNCTION_TYPE) * (maxSymbolValue+1) );   /* useless init, but keep static analyzer happy, and we don't need to performance optimize legacy decoders */
    DTableH.tableLog = (U16)tableLog;
    for (s=0; s<=maxSymbolValue; s++) {
        if (normalizedCounter[s]==-1) {
            tableDecode[highThreshold--].symbol = (FSEv05_FUNCTION_TYPE)s;
            symbolNext[s] = 1;
        } else {
            if (normalizedCounter[s] >= largeLimit) noLarge=0;
            symbolNext[s] = normalizedCounter[s];
    }   }

    /* Spread symbols */
    for (s=0; s<=maxSymbolValue; s++) {
        int i;
        for (i=0; i<normalizedCounter[s]; i++) {
            tableDecode[position].symbol = (FSEv05_FUNCTION_TYPE)s;
            position = (position + step) & tableMask;
            while (position > highThreshold) position = (position + step) & tableMask;   /* lowprob area */
    }   }

    if (position!=0) return ERROR(GENERIC);   /* position must reach all cells once, otherwise normalizedCounter is incorrect */

    /* Build Decoding table */
    {
        U32 i;
        for (i=0; i<tableSize; i++) {
            FSEv05_FUNCTION_TYPE symbol = (FSEv05_FUNCTION_TYPE)(tableDecode[i].symbol);
            U16 nextState = symbolNext[symbol]++;
            tableDecode[i].nbBits = (BYTE) (tableLog - BITv05_highbit32 ((U32)nextState) );
            tableDecode[i].newState = (U16) ( (nextState << tableDecode[i].nbBits) - tableSize);
    }   }

    DTableH.fastMode = (U16)noLarge;
    memcpy(dt, &DTableH, sizeof(DTableH));
    return 0;
}


#ifndef FSEv05_COMMONDEFS_ONLY
/*-****************************************
*  FSEv05 helper functions
******************************************/
unsigned FSEv05_isError(size_t code) { return ERR_isError(code); }

const char* FSEv05_getErrorName(size_t code) { return ERR_getErrorName(code); }


/*-**************************************************************
*  FSEv05 NCount encoding-decoding
****************************************************************/
static short FSEv05_abs(short a) { return a<0 ? -a : a; }


size_t FSEv05_readNCount (short* normalizedCounter, unsigned* maxSVPtr, unsigned* tableLogPtr,
                 const void* headerBuffer, size_t hbSize)
{
    const BYTE* const istart = (const BYTE*) headerBuffer;
    const BYTE* const iend = istart + hbSize;
    const BYTE* ip = istart;
    int nbBits;
    int remaining;
    int threshold;
    U32 bitStream;
    int bitCount;
    unsigned charnum = 0;
    int previous0 = 0;

    if (hbSize < 4) return ERROR(srcSize_wrong);
    bitStream = MEM_readLE32(ip);
    nbBits = (bitStream & 0xF) + FSEv05_MIN_TABLELOG;   /* extract tableLog */
    if (nbBits > FSEv05_TABLELOG_ABSOLUTE_MAX) return ERROR(tableLog_tooLarge);
    bitStream >>= 4;
    bitCount = 4;
    *tableLogPtr = nbBits;
    remaining = (1<<nbBits)+1;
    threshold = 1<<nbBits;
    nbBits++;

    while ((remaining>1) && (charnum<=*maxSVPtr)) {
        if (previous0) {
            unsigned n0 = charnum;
            while ((bitStream & 0xFFFF) == 0xFFFF) {
                n0+=24;
                if (ip < iend-5) {
                    ip+=2;
                    bitStream = MEM_readLE32(ip) >> bitCount;
                } else {
                    bitStream >>= 16;
                    bitCount+=16;
            }   }
            while ((bitStream & 3) == 3) {
                n0+=3;
                bitStream>>=2;
                bitCount+=2;
            }
            n0 += bitStream & 3;
            bitCount += 2;
            if (n0 > *maxSVPtr) return ERROR(maxSymbolValue_tooSmall);
            while (charnum < n0) normalizedCounter[charnum++] = 0;
            if ((ip <= iend-7) || (ip + (bitCount>>3) <= iend-4)) {
                ip += bitCount>>3;
                bitCount &= 7;
                bitStream = MEM_readLE32(ip) >> bitCount;
            }
            else
                bitStream >>= 2;
        }
        {
            const short max = (short)((2*threshold-1)-remaining);
            short count;

            if ((bitStream & (threshold-1)) < (U32)max) {
                count = (short)(bitStream & (threshold-1));
                bitCount   += nbBits-1;
            } else {
                count = (short)(bitStream & (2*threshold-1));
                if (count >= threshold) count -= max;
                bitCount   += nbBits;
            }

            count--;   /* extra accuracy */
            remaining -= FSEv05_abs(count);
            normalizedCounter[charnum++] = count;
            previous0 = !count;
            while (remaining < threshold) {
                nbBits--;
                threshold >>= 1;
            }

            if ((ip <= iend-7) || (ip + (bitCount>>3) <= iend-4)) {
                ip += bitCount>>3;
                bitCount &= 7;
            } else {
                bitCount -= (int)(8 * (iend - 4 - ip));
                ip = iend - 4;
            }
            bitStream = MEM_readLE32(ip) >> (bitCount & 31);
    }   }
    if (remaining != 1) return ERROR(GENERIC);
    *maxSVPtr = charnum-1;

    ip += (bitCount+7)>>3;
    if ((size_t)(ip-istart) > hbSize) return ERROR(srcSize_wrong);
    return ip-istart;
}



/*-*******************************************************
*  Decompression (Byte symbols)
*********************************************************/
size_t FSEv05_buildDTable_rle (FSEv05_DTable* dt, BYTE symbolValue)
{
    void* ptr = dt;
    FSEv05_DTableHeader* const DTableH = (FSEv05_DTableHeader*)ptr;
    void* dPtr = dt + 1;
    FSEv05_decode_t* const cell = (FSEv05_decode_t*)dPtr;

    DTableH->tableLog = 0;
    DTableH->fastMode = 0;

    cell->newState = 0;
    cell->symbol = symbolValue;
    cell->nbBits = 0;

    return 0;
}


size_t FSEv05_buildDTable_raw (FSEv05_DTable* dt, unsigned nbBits)
{
    void* ptr = dt;
    FSEv05_DTableHeader* const DTableH = (FSEv05_DTableHeader*)ptr;
    void* dPtr = dt + 1;
    FSEv05_decode_t* const dinfo = (FSEv05_decode_t*)dPtr;
    const unsigned tableSize = 1 << nbBits;
    const unsigned tableMask = tableSize - 1;
    const unsigned maxSymbolValue = tableMask;
    unsigned s;

    /* Sanity checks */
    if (nbBits < 1) return ERROR(GENERIC);         /* min size */

    /* Build Decoding Table */
    DTableH->tableLog = (U16)nbBits;
    DTableH->fastMode = 1;
    for (s=0; s<=maxSymbolValue; s++) {
        dinfo[s].newState = 0;
        dinfo[s].symbol = (BYTE)s;
        dinfo[s].nbBits = (BYTE)nbBits;
    }

    return 0;
}

FORCE_INLINE size_t FSEv05_decompress_usingDTable_generic(
          void* dst, size_t maxDstSize,
    const void* cSrc, size_t cSrcSize,
    const FSEv05_DTable* dt, const unsigned fast)
{
    BYTE* const ostart = (BYTE*) dst;
    BYTE* op = ostart;
    BYTE* const omax = op + maxDstSize;
    BYTE* const olimit = omax-3;

    BITv05_DStream_t bitD;
    FSEv05_DState_t state1;
    FSEv05_DState_t state2;
    size_t errorCode;

    /* Init */
    errorCode = BITv05_initDStream(&bitD, cSrc, cSrcSize);   /* replaced last arg by maxCompressed Size */
    if (FSEv05_isError(errorCode)) return errorCode;

    FSEv05_initDState(&state1, &bitD, dt);
    FSEv05_initDState(&state2, &bitD, dt);

#define FSEv05_GETSYMBOL(statePtr) fast ? FSEv05_decodeSymbolFast(statePtr, &bitD) : FSEv05_decodeSymbol(statePtr, &bitD)

    /* 4 symbols per loop */
    for ( ; (BITv05_reloadDStream(&bitD)==BITv05_DStream_unfinished) && (op<olimit) ; op+=4) {
        op[0] = FSEv05_GETSYMBOL(&state1);

        if (FSEv05_MAX_TABLELOG*2+7 > sizeof(bitD.bitContainer)*8)    /* This test must be static */
            BITv05_reloadDStream(&bitD);

        op[1] = FSEv05_GETSYMBOL(&state2);

        if (FSEv05_MAX_TABLELOG*4+7 > sizeof(bitD.bitContainer)*8)    /* This test must be static */
            { if (BITv05_reloadDStream(&bitD) > BITv05_DStream_unfinished) { op+=2; break; } }

        op[2] = FSEv05_GETSYMBOL(&state1);

        if (FSEv05_MAX_TABLELOG*2+7 > sizeof(bitD.bitContainer)*8)    /* This test must be static */
            BITv05_reloadDStream(&bitD);

        op[3] = FSEv05_GETSYMBOL(&state2);
    }

    /* tail */
    /* note : BITv05_reloadDStream(&bitD) >= FSEv05_DStream_partiallyFilled; Ends at exactly BITv05_DStream_completed */
    while (1) {
        if ( (BITv05_reloadDStream(&bitD)>BITv05_DStream_completed) || (op==omax) || (BITv05_endOfDStream(&bitD) && (fast || FSEv05_endOfDState(&state1))) )
            break;

        *op++ = FSEv05_GETSYMBOL(&state1);

        if ( (BITv05_reloadDStream(&bitD)>BITv05_DStream_completed) || (op==omax) || (BITv05_endOfDStream(&bitD) && (fast || FSEv05_endOfDState(&state2))) )
            break;

        *op++ = FSEv05_GETSYMBOL(&state2);
    }

    /* end ? */
    if (BITv05_endOfDStream(&bitD) && FSEv05_endOfDState(&state1) && FSEv05_endOfDState(&state2))
        return op-ostart;

    if (op==omax) return ERROR(dstSize_tooSmall);   /* dst buffer is full, but cSrc unfinished */

    return ERROR(corruption_detected);
}


size_t FSEv05_decompress_usingDTable(void* dst, size_t originalSize,
                            const void* cSrc, size_t cSrcSize,
                            const FSEv05_DTable* dt)
{
    const void* ptr = dt;
    const FSEv05_DTableHeader* DTableH = (const FSEv05_DTableHeader*)ptr;
    const U32 fastMode = DTableH->fastMode;

    /* select fast mode (static) */
    if (fastMode) return FSEv05_decompress_usingDTable_generic(dst, originalSize, cSrc, cSrcSize, dt, 1);
    return FSEv05_decompress_usingDTable_generic(dst, originalSize, cSrc, cSrcSize, dt, 0);
}


size_t FSEv05_decompress(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize)
{
    const BYTE* const istart = (const BYTE*)cSrc;
    const BYTE* ip = istart;
    short counting[FSEv05_MAX_SYMBOL_VALUE+1];
    DTable_max_t dt;   /* Static analyzer seems unable to understand this table will be properly initialized later */
    unsigned tableLog;
    unsigned maxSymbolValue = FSEv05_MAX_SYMBOL_VALUE;
    size_t errorCode;

    if (cSrcSize<2) return ERROR(srcSize_wrong);   /* too small input size */

    /* normal FSEv05 decoding mode */
    errorCode = FSEv05_readNCount (counting, &maxSymbolValue, &tableLog, istart, cSrcSize);
    if (FSEv05_isError(errorCode)) return errorCode;
    if (errorCode >= cSrcSize) return ERROR(srcSize_wrong);   /* too small input size */
    ip += errorCode;
    cSrcSize -= errorCode;

    errorCode = FSEv05_buildDTable (dt, counting, maxSymbolValue, tableLog);
    if (FSEv05_isError(errorCode)) return errorCode;

    /* always return, even if it is an error code */
    return FSEv05_decompress_usingDTable (dst, maxDstSize, ip, cSrcSize, dt);
}



#endif   /* FSEv05_COMMONDEFS_ONLY */
/* ******************************************************************
   Huff0 : Huffman coder, part of New Generation Entropy library
   header file
   Copyright (C) 2013-2016, Yann Collet.

   BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
****************************************************************** */
#ifndef HUFF0_H
#define HUFF0_H

#if defined (__cplusplus)
extern "C" {
#endif



/* ****************************************
*  Huff0 simple functions
******************************************/
size_t HUFv05_decompress(void* dst,  size_t dstSize,
                const void* cSrc, size_t cSrcSize);
/*!
HUFv05_decompress():
    Decompress Huff0 data from buffer 'cSrc', of size 'cSrcSize',
    into already allocated destination buffer 'dst', of size 'dstSize'.
    @dstSize : must be the **exact** size of original (uncompressed) data.
    Note : in contrast with FSEv05, HUFv05_decompress can regenerate
           RLE (cSrcSize==1) and uncompressed (cSrcSize==dstSize) data,
           because it knows size to regenerate.
    @return : size of regenerated data (== dstSize)
              or an error code, which can be tested using HUFv05_isError()
*/


/* ****************************************
*  Tool functions
******************************************/
/* Error Management */
unsigned    HUFv05_isError(size_t code);        /* tells if a return value is an error code */
const char* HUFv05_getErrorName(size_t code);   /* provides error code string (useful for debugging) */


#if defined (__cplusplus)
}
#endif

#endif   /* HUF0_H */
/* ******************************************************************
   Huff0 : Huffman codec, part of New Generation Entropy library
   header file, for static linking only
   Copyright (C) 2013-2016, Yann Collet

   BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
****************************************************************** */
#ifndef HUF0_STATIC_H
#define HUF0_STATIC_H

#if defined (__cplusplus)
extern "C" {
#endif



/* ****************************************
*  Static allocation
******************************************/
/* static allocation of Huff0's DTable */
#define HUFv05_DTABLE_SIZE(maxTableLog)   (1 + (1<<maxTableLog))
#define HUFv05_CREATE_STATIC_DTABLEX2(DTable, maxTableLog) \
        unsigned short DTable[HUFv05_DTABLE_SIZE(maxTableLog)] = { maxTableLog }
#define HUFv05_CREATE_STATIC_DTABLEX4(DTable, maxTableLog) \
        unsigned int DTable[HUFv05_DTABLE_SIZE(maxTableLog)] = { maxTableLog }
#define HUFv05_CREATE_STATIC_DTABLEX6(DTable, maxTableLog) \
        unsigned int DTable[HUFv05_DTABLE_SIZE(maxTableLog) * 3 / 2] = { maxTableLog }


/* ****************************************
*  Advanced decompression functions
******************************************/
size_t HUFv05_decompress4X2 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /* single-symbol decoder */
size_t HUFv05_decompress4X4 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /* double-symbols decoder */


/* ****************************************
*  Huff0 detailed API
******************************************/
/*!
HUFv05_decompress() does the following:
1. select the decompression algorithm (X2, X4, X6) based on pre-computed heuristics
2. build Huffman table from save, using HUFv05_readDTableXn()
3. decode 1 or 4 segments in parallel using HUFv05_decompressSXn_usingDTable
*/
size_t HUFv05_readDTableX2 (unsigned short* DTable, const void* src, size_t srcSize);
size_t HUFv05_readDTableX4 (unsigned* DTable, const void* src, size_t srcSize);

size_t HUFv05_decompress4X2_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const unsigned short* DTable);
size_t HUFv05_decompress4X4_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const unsigned* DTable);


/* single stream variants */

size_t HUFv05_decompress1X2 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /* single-symbol decoder */
size_t HUFv05_decompress1X4 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);   /* double-symbol decoder */

size_t HUFv05_decompress1X2_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const unsigned short* DTable);
size_t HUFv05_decompress1X4_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const unsigned* DTable);



#if defined (__cplusplus)
}
#endif

#endif /* HUF0_STATIC_H */
/* ******************************************************************
   Huff0 : Huffman coder, part of New Generation Entropy library
   Copyright (C) 2013-2015, Yann Collet.

   BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - FSEv05+Huff0 source repository : https://github.com/Cyan4973/FiniteStateEntropy
    - Public forum : https://groups.google.com/forum/#!forum/lz4c
****************************************************************** */

/* **************************************************************
*  Compiler specifics
****************************************************************/
#if defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
/* inline is defined */
#elif defined(_MSC_VER)
#  define inline __inline
#else
#  define inline /* disable inline */
#endif


#ifdef _MSC_VER    /* Visual Studio */
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#endif


/* **************************************************************
*  Includes
****************************************************************/
#include <stdlib.h>     /* malloc, free, qsort */
#include <string.h>     /* memcpy, memset */
#include <stdio.h>      /* printf (debug) */


/* **************************************************************
*  Constants
****************************************************************/
#define HUFv05_ABSOLUTEMAX_TABLELOG  16   /* absolute limit of HUFv05_MAX_TABLELOG. Beyond that value, code does not work */
#define HUFv05_MAX_TABLELOG  12           /* max configured tableLog (for static allocation); can be modified up to HUFv05_ABSOLUTEMAX_TABLELOG */
#define HUFv05_DEFAULT_TABLELOG  HUFv05_MAX_TABLELOG   /* tableLog by default, when not specified */
#define HUFv05_MAX_SYMBOL_VALUE 255
#if (HUFv05_MAX_TABLELOG > HUFv05_ABSOLUTEMAX_TABLELOG)
#  error "HUFv05_MAX_TABLELOG is too large !"
#endif


/* **************************************************************
*  Error Management
****************************************************************/
unsigned HUFv05_isError(size_t code) { return ERR_isError(code); }
const char* HUFv05_getErrorName(size_t code) { return ERR_getErrorName(code); }
#define HUFv05_STATIC_ASSERT(c) { enum { HUFv05_static_assert = 1/(int)(!!(c)) }; }   /* use only *after* variable declarations */


/* *******************************************************
*  Huff0 : Huffman block decompression
*********************************************************/
typedef struct { BYTE byte; BYTE nbBits; } HUFv05_DEltX2;   /* single-symbol decoding */

typedef struct { U16 sequence; BYTE nbBits; BYTE length; } HUFv05_DEltX4;  /* double-symbols decoding */

typedef struct { BYTE symbol; BYTE weight; } sortedSymbol_t;

/*! HUFv05_readStats
    Read compact Huffman tree, saved by HUFv05_writeCTable
    @huffWeight : destination buffer
    @return : size read from `src`
*/
static size_t HUFv05_readStats(BYTE* huffWeight, size_t hwSize, U32* rankStats,
                            U32* nbSymbolsPtr, U32* tableLogPtr,
                            const void* src, size_t srcSize)
{
    U32 weightTotal;
    U32 tableLog;
    const BYTE* ip = (const BYTE*) src;
    size_t iSize;
    size_t oSize;
    U32 n;

    if (!srcSize) return ERROR(srcSize_wrong);
    iSize = ip[0];
    /* memset(huffWeight, 0, hwSize); */   /* is not necessary, even though some analyzer complain ... */

    if (iSize >= 128)  { /* special header */
        if (iSize >= (242)) {  /* RLE */
            static int l[14] = { 1, 2, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128 };
            oSize = l[iSize-242];
            memset(huffWeight, 1, hwSize);
            iSize = 0;
        }
        else {   /* Incompressible */
            oSize = iSize - 127;
            iSize = ((oSize+1)/2);
            if (iSize+1 > srcSize) return ERROR(srcSize_wrong);
            if (oSize >= hwSize) return ERROR(corruption_detected);
            ip += 1;
            for (n=0; n<oSize; n+=2) {
                huffWeight[n]   = ip[n/2] >> 4;
                huffWeight[n+1] = ip[n/2] & 15;
    }   }   }
    else  {   /* header compressed with FSEv05 (normal case) */
        if (iSize+1 > srcSize) return ERROR(srcSize_wrong);
        oSize = FSEv05_decompress(huffWeight, hwSize-1, ip+1, iSize);   /* max (hwSize-1) values decoded, as last one is implied */
        if (FSEv05_isError(oSize)) return oSize;
    }

    /* collect weight stats */
    memset(rankStats, 0, (HUFv05_ABSOLUTEMAX_TABLELOG + 1) * sizeof(U32));
    weightTotal = 0;
    for (n=0; n<oSize; n++) {
        if (huffWeight[n] >= HUFv05_ABSOLUTEMAX_TABLELOG) return ERROR(corruption_detected);
        rankStats[huffWeight[n]]++;
        weightTotal += (1 << huffWeight[n]) >> 1;
    }
    if (weightTotal == 0) return ERROR(corruption_detected);

    /* get last non-null symbol weight (implied, total must be 2^n) */
    tableLog = BITv05_highbit32(weightTotal) + 1;
    if (tableLog > HUFv05_ABSOLUTEMAX_TABLELOG) return ERROR(corruption_detected);
    {   /* determine last weight */
        U32 total = 1 << tableLog;
        U32 rest = total - weightTotal;
        U32 verif = 1 << BITv05_highbit32(rest);
        U32 lastWeight = BITv05_highbit32(rest) + 1;
        if (verif != rest) return ERROR(corruption_detected);    /* last value must be a clean power of 2 */
        huffWeight[oSize] = (BYTE)lastWeight;
        rankStats[lastWeight]++;
    }

    /* check tree construction validity */
    if ((rankStats[1] < 2) || (rankStats[1] & 1)) return ERROR(corruption_detected);   /* by construction : at least 2 elts of rank 1, must be even */

    /* results */
    *nbSymbolsPtr = (U32)(oSize+1);
    *tableLogPtr = tableLog;
    return iSize+1;
}


/*-***************************/
/*  single-symbol decoding   */
/*-***************************/

size_t HUFv05_readDTableX2 (U16* DTable, const void* src, size_t srcSize)
{
    BYTE huffWeight[HUFv05_MAX_SYMBOL_VALUE + 1];
    U32 rankVal[HUFv05_ABSOLUTEMAX_TABLELOG + 1];   /* large enough for values from 0 to 16 */
    U32 tableLog = 0;
    size_t iSize;
    U32 nbSymbols = 0;
    U32 n;
    U32 nextRankStart;
    void* const dtPtr = DTable + 1;
    HUFv05_DEltX2* const dt = (HUFv05_DEltX2*)dtPtr;

    HUFv05_STATIC_ASSERT(sizeof(HUFv05_DEltX2) == sizeof(U16));   /* if compilation fails here, assertion is false */
    /* memset(huffWeight, 0, sizeof(huffWeight)); */   /* is not necessary, even though some analyzer complain ... */

    iSize = HUFv05_readStats(huffWeight, HUFv05_MAX_SYMBOL_VALUE + 1, rankVal, &nbSymbols, &tableLog, src, srcSize);
    if (HUFv05_isError(iSize)) return iSize;

    /* check result */
    if (tableLog > DTable[0]) return ERROR(tableLog_tooLarge);   /* DTable is too small */
    DTable[0] = (U16)tableLog;   /* maybe should separate sizeof allocated DTable, from used size of DTable, in case of re-use */

    /* Prepare ranks */
    nextRankStart = 0;
    for (n=1; n<=tableLog; n++) {
        U32 current = nextRankStart;
        nextRankStart += (rankVal[n] << (n-1));
        rankVal[n] = current;
    }

    /* fill DTable */
    for (n=0; n<nbSymbols; n++) {
        const U32 w = huffWeight[n];
        const U32 length = (1 << w) >> 1;
        U32 i;
        HUFv05_DEltX2 D;
        D.byte = (BYTE)n; D.nbBits = (BYTE)(tableLog + 1 - w);
        for (i = rankVal[w]; i < rankVal[w] + length; i++)
            dt[i] = D;
        rankVal[w] += length;
    }

    return iSize;
}

static BYTE HUFv05_decodeSymbolX2(BITv05_DStream_t* Dstream, const HUFv05_DEltX2* dt, const U32 dtLog)
{
        const size_t val = BITv05_lookBitsFast(Dstream, dtLog); /* note : dtLog >= 1 */
        const BYTE c = dt[val].byte;
        BITv05_skipBits(Dstream, dt[val].nbBits);
        return c;
}

#define HUFv05_DECODE_SYMBOLX2_0(ptr, DStreamPtr) \
    *ptr++ = HUFv05_decodeSymbolX2(DStreamPtr, dt, dtLog)

#define HUFv05_DECODE_SYMBOLX2_1(ptr, DStreamPtr) \
    if (MEM_64bits() || (HUFv05_MAX_TABLELOG<=12)) \
        HUFv05_DECODE_SYMBOLX2_0(ptr, DStreamPtr)

#define HUFv05_DECODE_SYMBOLX2_2(ptr, DStreamPtr) \
    if (MEM_64bits()) \
        HUFv05_DECODE_SYMBOLX2_0(ptr, DStreamPtr)

static inline size_t HUFv05_decodeStreamX2(BYTE* p, BITv05_DStream_t* const bitDPtr, BYTE* const pEnd, const HUFv05_DEltX2* const dt, const U32 dtLog)
{
    BYTE* const pStart = p;

    /* up to 4 symbols at a time */
    while ((BITv05_reloadDStream(bitDPtr) == BITv05_DStream_unfinished) && (p <= pEnd-4)) {
        HUFv05_DECODE_SYMBOLX2_2(p, bitDPtr);
        HUFv05_DECODE_SYMBOLX2_1(p, bitDPtr);
        HUFv05_DECODE_SYMBOLX2_2(p, bitDPtr);
        HUFv05_DECODE_SYMBOLX2_0(p, bitDPtr);
    }

    /* closer to the end */
    while ((BITv05_reloadDStream(bitDPtr) == BITv05_DStream_unfinished) && (p < pEnd))
        HUFv05_DECODE_SYMBOLX2_0(p, bitDPtr);

    /* no more data to retrieve from bitstream, hence no need to reload */
    while (p < pEnd)
        HUFv05_DECODE_SYMBOLX2_0(p, bitDPtr);

    return pEnd-pStart;
}

size_t HUFv05_decompress1X2_usingDTable(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const U16* DTable)
{
    BYTE* op = (BYTE*)dst;
    BYTE* const oend = op + dstSize;
    const U32 dtLog = DTable[0];
    const void* dtPtr = DTable;
    const HUFv05_DEltX2* const dt = ((const HUFv05_DEltX2*)dtPtr)+1;
    BITv05_DStream_t bitD;

    if (dstSize <= cSrcSize) return ERROR(dstSize_tooSmall);
    { size_t const errorCode = BITv05_initDStream(&bitD, cSrc, cSrcSize);
      if (HUFv05_isError(errorCode)) return errorCode; }

    HUFv05_decodeStreamX2(op, &bitD, oend, dt, dtLog);

    /* check */
    if (!BITv05_endOfDStream(&bitD)) return ERROR(corruption_detected);

    return dstSize;
}

size_t HUFv05_decompress1X2 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize)
{
    HUFv05_CREATE_STATIC_DTABLEX2(DTable, HUFv05_MAX_TABLELOG);
    const BYTE* ip = (const BYTE*) cSrc;
    size_t errorCode;

    errorCode = HUFv05_readDTableX2 (DTable, cSrc, cSrcSize);
    if (HUFv05_isError(errorCode)) return errorCode;
    if (errorCode >= cSrcSize) return ERROR(srcSize_wrong);
    ip += errorCode;
    cSrcSize -= errorCode;

    return HUFv05_decompress1X2_usingDTable (dst, dstSize, ip, cSrcSize, DTable);
}


size_t HUFv05_decompress4X2_usingDTable(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const U16* DTable)
{
    /* Check */
    if (cSrcSize < 10) return ERROR(corruption_detected);   /* strict minimum : jump table + 1 byte per stream */
    {
        const BYTE* const istart = (const BYTE*) cSrc;
        BYTE* const ostart = (BYTE*) dst;
        BYTE* const oend = ostart + dstSize;
        const void* const dtPtr = DTable;
        const HUFv05_DEltX2* const dt = ((const HUFv05_DEltX2*)dtPtr) +1;
        const U32 dtLog = DTable[0];
        size_t errorCode;

        /* Init */
        BITv05_DStream_t bitD1;
        BITv05_DStream_t bitD2;
        BITv05_DStream_t bitD3;
        BITv05_DStream_t bitD4;
        const size_t length1 = MEM_readLE16(istart);
        const size_t length2 = MEM_readLE16(istart+2);
        const size_t length3 = MEM_readLE16(istart+4);
        size_t length4;
        const BYTE* const istart1 = istart + 6;  /* jumpTable */
        const BYTE* const istart2 = istart1 + length1;
        const BYTE* const istart3 = istart2 + length2;
        const BYTE* const istart4 = istart3 + length3;
        const size_t segmentSize = (dstSize+3) / 4;
        BYTE* const opStart2 = ostart + segmentSize;
        BYTE* const opStart3 = opStart2 + segmentSize;
        BYTE* const opStart4 = opStart3 + segmentSize;
        BYTE* op1 = ostart;
        BYTE* op2 = opStart2;
        BYTE* op3 = opStart3;
        BYTE* op4 = opStart4;
        U32 endSignal;

        length4 = cSrcSize - (length1 + length2 + length3 + 6);
        if (length4 > cSrcSize) return ERROR(corruption_detected);   /* overflow */
        errorCode = BITv05_initDStream(&bitD1, istart1, length1);
        if (HUFv05_isError(errorCode)) return errorCode;
        errorCode = BITv05_initDStream(&bitD2, istart2, length2);
        if (HUFv05_isError(errorCode)) return errorCode;
        errorCode = BITv05_initDStream(&bitD3, istart3, length3);
        if (HUFv05_isError(errorCode)) return errorCode;
        errorCode = BITv05_initDStream(&bitD4, istart4, length4);
        if (HUFv05_isError(errorCode)) return errorCode;

        /* 16-32 symbols per loop (4-8 symbols per stream) */
        endSignal = BITv05_reloadDStream(&bitD1) | BITv05_reloadDStream(&bitD2) | BITv05_reloadDStream(&bitD3) | BITv05_reloadDStream(&bitD4);
        for ( ; (endSignal==BITv05_DStream_unfinished) && (op4<(oend-7)) ; ) {
            HUFv05_DECODE_SYMBOLX2_2(op1, &bitD1);
            HUFv05_DECODE_SYMBOLX2_2(op2, &bitD2);
            HUFv05_DECODE_SYMBOLX2_2(op3, &bitD3);
            HUFv05_DECODE_SYMBOLX2_2(op4, &bitD4);
            HUFv05_DECODE_SYMBOLX2_1(op1, &bitD1);
            HUFv05_DECODE_SYMBOLX2_1(op2, &bitD2);
            HUFv05_DECODE_SYMBOLX2_1(op3, &bitD3);
            HUFv05_DECODE_SYMBOLX2_1(op4, &bitD4);
            HUFv05_DECODE_SYMBOLX2_2(op1, &bitD1);
            HUFv05_DECODE_SYMBOLX2_2(op2, &bitD2);
            HUFv05_DECODE_SYMBOLX2_2(op3, &bitD3);
            HUFv05_DECODE_SYMBOLX2_2(op4, &bitD4);
            HUFv05_DECODE_SYMBOLX2_0(op1, &bitD1);
            HUFv05_DECODE_SYMBOLX2_0(op2, &bitD2);
            HUFv05_DECODE_SYMBOLX2_0(op3, &bitD3);
            HUFv05_DECODE_SYMBOLX2_0(op4, &bitD4);
            endSignal = BITv05_reloadDStream(&bitD1) | BITv05_reloadDStream(&bitD2) | BITv05_reloadDStream(&bitD3) | BITv05_reloadDStream(&bitD4);
        }

        /* check corruption */
        if (op1 > opStart2) return ERROR(corruption_detected);
        if (op2 > opStart3) return ERROR(corruption_detected);
        if (op3 > opStart4) return ERROR(corruption_detected);
        /* note : op4 supposed already verified within main loop */

        /* finish bitStreams one by one */
        HUFv05_decodeStreamX2(op1, &bitD1, opStart2, dt, dtLog);
        HUFv05_decodeStreamX2(op2, &bitD2, opStart3, dt, dtLog);
        HUFv05_decodeStreamX2(op3, &bitD3, opStart4, dt, dtLog);
        HUFv05_decodeStreamX2(op4, &bitD4, oend,     dt, dtLog);

        /* check */
        endSignal = BITv05_endOfDStream(&bitD1) & BITv05_endOfDStream(&bitD2) & BITv05_endOfDStream(&bitD3) & BITv05_endOfDStream(&bitD4);
        if (!endSignal) return ERROR(corruption_detected);

        /* decoded size */
        return dstSize;
    }
}


size_t HUFv05_decompress4X2 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize)
{
    HUFv05_CREATE_STATIC_DTABLEX2(DTable, HUFv05_MAX_TABLELOG);
    const BYTE* ip = (const BYTE*) cSrc;
    size_t errorCode;

    errorCode = HUFv05_readDTableX2 (DTable, cSrc, cSrcSize);
    if (HUFv05_isError(errorCode)) return errorCode;
    if (errorCode >= cSrcSize) return ERROR(srcSize_wrong);
    ip += errorCode;
    cSrcSize -= errorCode;

    return HUFv05_decompress4X2_usingDTable (dst, dstSize, ip, cSrcSize, DTable);
}


/* *************************/
/* double-symbols decoding */
/* *************************/

static void HUFv05_fillDTableX4Level2(HUFv05_DEltX4* DTable, U32 sizeLog, const U32 consumed,
                           const U32* rankValOrigin, const int minWeight,
                           const sortedSymbol_t* sortedSymbols, const U32 sortedListSize,
                           U32 nbBitsBaseline, U16 baseSeq)
{
    HUFv05_DEltX4 DElt;
    U32 rankVal[HUFv05_ABSOLUTEMAX_TABLELOG + 1];
    U32 s;

    /* get pre-calculated rankVal */
    memcpy(rankVal, rankValOrigin, sizeof(rankVal));

    /* fill skipped values */
    if (minWeight>1) {
        U32 i, skipSize = rankVal[minWeight];
        MEM_writeLE16(&(DElt.sequence), baseSeq);
        DElt.nbBits   = (BYTE)(consumed);
        DElt.length   = 1;
        for (i = 0; i < skipSize; i++)
            DTable[i] = DElt;
    }

    /* fill DTable */
    for (s=0; s<sortedListSize; s++) {   /* note : sortedSymbols already skipped */
        const U32 symbol = sortedSymbols[s].symbol;
        const U32 weight = sortedSymbols[s].weight;
        const U32 nbBits = nbBitsBaseline - weight;
        const U32 length = 1 << (sizeLog-nbBits);
        const U32 start = rankVal[weight];
        U32 i = start;
        const U32 end = start + length;

        MEM_writeLE16(&(DElt.sequence), (U16)(baseSeq + (symbol << 8)));
        DElt.nbBits = (BYTE)(nbBits + consumed);
        DElt.length = 2;
        do { DTable[i++] = DElt; } while (i<end);   /* since length >= 1 */

        rankVal[weight] += length;
    }
}

typedef U32 rankVal_t[HUFv05_ABSOLUTEMAX_TABLELOG][HUFv05_ABSOLUTEMAX_TABLELOG + 1];

static void HUFv05_fillDTableX4(HUFv05_DEltX4* DTable, const U32 targetLog,
                           const sortedSymbol_t* sortedList, const U32 sortedListSize,
                           const U32* rankStart, rankVal_t rankValOrigin, const U32 maxWeight,
                           const U32 nbBitsBaseline)
{
    U32 rankVal[HUFv05_ABSOLUTEMAX_TABLELOG + 1];
    const int scaleLog = nbBitsBaseline - targetLog;   /* note : targetLog >= srcLog, hence scaleLog <= 1 */
    const U32 minBits  = nbBitsBaseline - maxWeight;
    U32 s;

    memcpy(rankVal, rankValOrigin, sizeof(rankVal));

    /* fill DTable */
    for (s=0; s<sortedListSize; s++) {
        const U16 symbol = sortedList[s].symbol;
        const U32 weight = sortedList[s].weight;
        const U32 nbBits = nbBitsBaseline - weight;
        const U32 start = rankVal[weight];
        const U32 length = 1 << (targetLog-nbBits);

        if (targetLog-nbBits >= minBits) {   /* enough room for a second symbol */
            U32 sortedRank;
            int minWeight = nbBits + scaleLog;
            if (minWeight < 1) minWeight = 1;
            sortedRank = rankStart[minWeight];
            HUFv05_fillDTableX4Level2(DTable+start, targetLog-nbBits, nbBits,
                           rankValOrigin[nbBits], minWeight,
                           sortedList+sortedRank, sortedListSize-sortedRank,
                           nbBitsBaseline, symbol);
        } else {
            U32 i;
            const U32 end = start + length;
            HUFv05_DEltX4 DElt;

            MEM_writeLE16(&(DElt.sequence), symbol);
            DElt.nbBits   = (BYTE)(nbBits);
            DElt.length   = 1;
            for (i = start; i < end; i++)
                DTable[i] = DElt;
        }
        rankVal[weight] += length;
    }
}

size_t HUFv05_readDTableX4 (unsigned* DTable, const void* src, size_t srcSize)
{
    BYTE weightList[HUFv05_MAX_SYMBOL_VALUE + 1];
    sortedSymbol_t sortedSymbol[HUFv05_MAX_SYMBOL_VALUE + 1];
    U32 rankStats[HUFv05_ABSOLUTEMAX_TABLELOG + 1] = { 0 };
    U32 rankStart0[HUFv05_ABSOLUTEMAX_TABLELOG + 2] = { 0 };
    U32* const rankStart = rankStart0+1;
    rankVal_t rankVal;
    U32 tableLog, maxW, sizeOfSort, nbSymbols;
    const U32 memLog = DTable[0];
    size_t iSize;
    void* dtPtr = DTable;
    HUFv05_DEltX4* const dt = ((HUFv05_DEltX4*)dtPtr) + 1;

    HUFv05_STATIC_ASSERT(sizeof(HUFv05_DEltX4) == sizeof(unsigned));   /* if compilation fails here, assertion is false */
    if (memLog > HUFv05_ABSOLUTEMAX_TABLELOG) return ERROR(tableLog_tooLarge);
    /* memset(weightList, 0, sizeof(weightList)); */   /* is not necessary, even though some analyzer complain ... */

    iSize = HUFv05_readStats(weightList, HUFv05_MAX_SYMBOL_VALUE + 1, rankStats, &nbSymbols, &tableLog, src, srcSize);
    if (HUFv05_isError(iSize)) return iSize;

    /* check result */
    if (tableLog > memLog) return ERROR(tableLog_tooLarge);   /* DTable can't fit code depth */

    /* find maxWeight */
    for (maxW = tableLog; rankStats[maxW]==0; maxW--) {}  /* necessarily finds a solution before 0 */

    /* Get start index of each weight */
    {
        U32 w, nextRankStart = 0;
        for (w=1; w<=maxW; w++) {
            U32 current = nextRankStart;
            nextRankStart += rankStats[w];
            rankStart[w] = current;
        }
        rankStart[0] = nextRankStart;   /* put all 0w symbols at the end of sorted list*/
        sizeOfSort = nextRankStart;
    }

    /* sort symbols by weight */
    {
        U32 s;
        for (s=0; s<nbSymbols; s++) {
            U32 w = weightList[s];
            U32 r = rankStart[w]++;
            sortedSymbol[r].symbol = (BYTE)s;
            sortedSymbol[r].weight = (BYTE)w;
        }
        rankStart[0] = 0;   /* forget 0w symbols; this is beginning of weight(1) */
    }

    /* Build rankVal */
    {
        const U32 minBits = tableLog+1 - maxW;
        U32 nextRankVal = 0;
        U32 w, consumed;
        const int rescale = (memLog-tableLog) - 1;   /* tableLog <= memLog */
        U32* rankVal0 = rankVal[0];
        for (w=1; w<=maxW; w++) {
            U32 current = nextRankVal;
            nextRankVal += rankStats[w] << (w+rescale);
            rankVal0[w] = current;
        }
        for (consumed = minBits; consumed <= memLog - minBits; consumed++) {
            U32* rankValPtr = rankVal[consumed];
            for (w = 1; w <= maxW; w++) {
                rankValPtr[w] = rankVal0[w] >> consumed;
    }   }   }

    HUFv05_fillDTableX4(dt, memLog,
                   sortedSymbol, sizeOfSort,
                   rankStart0, rankVal, maxW,
                   tableLog+1);

    return iSize;
}


static U32 HUFv05_decodeSymbolX4(void* op, BITv05_DStream_t* DStream, const HUFv05_DEltX4* dt, const U32 dtLog)
{
    const size_t val = BITv05_lookBitsFast(DStream, dtLog);   /* note : dtLog >= 1 */
    memcpy(op, dt+val, 2);
    BITv05_skipBits(DStream, dt[val].nbBits);
    return dt[val].length;
}

static U32 HUFv05_decodeLastSymbolX4(void* op, BITv05_DStream_t* DStream, const HUFv05_DEltX4* dt, const U32 dtLog)
{
    const size_t val = BITv05_lookBitsFast(DStream, dtLog);   /* note : dtLog >= 1 */
    memcpy(op, dt+val, 1);
    if (dt[val].length==1) BITv05_skipBits(DStream, dt[val].nbBits);
    else {
        if (DStream->bitsConsumed < (sizeof(DStream->bitContainer)*8)) {
            BITv05_skipBits(DStream, dt[val].nbBits);
            if (DStream->bitsConsumed > (sizeof(DStream->bitContainer)*8))
                DStream->bitsConsumed = (sizeof(DStream->bitContainer)*8);   /* ugly hack; works only because it's the last symbol. Note : can't easily extract nbBits from just this symbol */
    }   }
    return 1;
}


#define HUFv05_DECODE_SYMBOLX4_0(ptr, DStreamPtr) \
    ptr += HUFv05_decodeSymbolX4(ptr, DStreamPtr, dt, dtLog)

#define HUFv05_DECODE_SYMBOLX4_1(ptr, DStreamPtr) \
    if (MEM_64bits() || (HUFv05_MAX_TABLELOG<=12)) \
        ptr += HUFv05_decodeSymbolX4(ptr, DStreamPtr, dt, dtLog)

#define HUFv05_DECODE_SYMBOLX4_2(ptr, DStreamPtr) \
    if (MEM_64bits()) \
        ptr += HUFv05_decodeSymbolX4(ptr, DStreamPtr, dt, dtLog)

static inline size_t HUFv05_decodeStreamX4(BYTE* p, BITv05_DStream_t* bitDPtr, BYTE* const pEnd, const HUFv05_DEltX4* const dt, const U32 dtLog)
{
    BYTE* const pStart = p;

    /* up to 8 symbols at a time */
    while ((BITv05_reloadDStream(bitDPtr) == BITv05_DStream_unfinished) && (p < pEnd-7)) {
        HUFv05_DECODE_SYMBOLX4_2(p, bitDPtr);
        HUFv05_DECODE_SYMBOLX4_1(p, bitDPtr);
        HUFv05_DECODE_SYMBOLX4_2(p, bitDPtr);
        HUFv05_DECODE_SYMBOLX4_0(p, bitDPtr);
    }

    /* closer to the end */
    while ((BITv05_reloadDStream(bitDPtr) == BITv05_DStream_unfinished) && (p <= pEnd-2))
        HUFv05_DECODE_SYMBOLX4_0(p, bitDPtr);

    while (p <= pEnd-2)
        HUFv05_DECODE_SYMBOLX4_0(p, bitDPtr);   /* no need to reload : reached the end of DStream */

    if (p < pEnd)
        p += HUFv05_decodeLastSymbolX4(p, bitDPtr, dt, dtLog);

    return p-pStart;
}


size_t HUFv05_decompress1X4_usingDTable(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const unsigned* DTable)
{
    const BYTE* const istart = (const BYTE*) cSrc;
    BYTE* const ostart = (BYTE*) dst;
    BYTE* const oend = ostart + dstSize;

    const U32 dtLog = DTable[0];
    const void* const dtPtr = DTable;
    const HUFv05_DEltX4* const dt = ((const HUFv05_DEltX4*)dtPtr) +1;
    size_t errorCode;

    /* Init */
    BITv05_DStream_t bitD;
    errorCode = BITv05_initDStream(&bitD, istart, cSrcSize);
    if (HUFv05_isError(errorCode)) return errorCode;

    /* finish bitStreams one by one */
    HUFv05_decodeStreamX4(ostart, &bitD, oend,     dt, dtLog);

    /* check */
    if (!BITv05_endOfDStream(&bitD)) return ERROR(corruption_detected);

    /* decoded size */
    return dstSize;
}

size_t HUFv05_decompress1X4 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize)
{
    HUFv05_CREATE_STATIC_DTABLEX4(DTable, HUFv05_MAX_TABLELOG);
    const BYTE* ip = (const BYTE*) cSrc;

    size_t hSize = HUFv05_readDTableX4 (DTable, cSrc, cSrcSize);
    if (HUFv05_isError(hSize)) return hSize;
    if (hSize >= cSrcSize) return ERROR(srcSize_wrong);
    ip += hSize;
    cSrcSize -= hSize;

    return HUFv05_decompress1X4_usingDTable (dst, dstSize, ip, cSrcSize, DTable);
}

size_t HUFv05_decompress4X4_usingDTable(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const unsigned* DTable)
{
    if (cSrcSize < 10) return ERROR(corruption_detected);   /* strict minimum : jump table + 1 byte per stream */

    {
        const BYTE* const istart = (const BYTE*) cSrc;
        BYTE* const ostart = (BYTE*) dst;
        BYTE* const oend = ostart + dstSize;
        const void* const dtPtr = DTable;
        const HUFv05_DEltX4* const dt = ((const HUFv05_DEltX4*)dtPtr) +1;
        const U32 dtLog = DTable[0];
        size_t errorCode;

        /* Init */
        BITv05_DStream_t bitD1;
        BITv05_DStream_t bitD2;
        BITv05_DStream_t bitD3;
        BITv05_DStream_t bitD4;
        const size_t length1 = MEM_readLE16(istart);
        const size_t length2 = MEM_readLE16(istart+2);
        const size_t length3 = MEM_readLE16(istart+4);
        size_t length4;
        const BYTE* const istart1 = istart + 6;  /* jumpTable */
        const BYTE* const istart2 = istart1 + length1;
        const BYTE* const istart3 = istart2 + length2;
        const BYTE* const istart4 = istart3 + length3;
        const size_t segmentSize = (dstSize+3) / 4;
        BYTE* const opStart2 = ostart + segmentSize;
        BYTE* const opStart3 = opStart2 + segmentSize;
        BYTE* const opStart4 = opStart3 + segmentSize;
        BYTE* op1 = ostart;
        BYTE* op2 = opStart2;
        BYTE* op3 = opStart3;
        BYTE* op4 = opStart4;
        U32 endSignal;

        length4 = cSrcSize - (length1 + length2 + length3 + 6);
        if (length4 > cSrcSize) return ERROR(corruption_detected);   /* overflow */
        errorCode = BITv05_initDStream(&bitD1, istart1, length1);
        if (HUFv05_isError(errorCode)) return errorCode;
        errorCode = BITv05_initDStream(&bitD2, istart2, length2);
        if (HUFv05_isError(errorCode)) return errorCode;
        errorCode = BITv05_initDStream(&bitD3, istart3, length3);
        if (HUFv05_isError(errorCode)) return errorCode;
        errorCode = BITv05_initDStream(&bitD4, istart4, length4);
        if (HUFv05_isError(errorCode)) return errorCode;

        /* 16-32 symbols per loop (4-8 symbols per stream) */
        endSignal = BITv05_reloadDStream(&bitD1) | BITv05_reloadDStream(&bitD2) | BITv05_reloadDStream(&bitD3) | BITv05_reloadDStream(&bitD4);
        for ( ; (endSignal==BITv05_DStream_unfinished) && (op4<(oend-7)) ; ) {
            HUFv05_DECODE_SYMBOLX4_2(op1, &bitD1);
            HUFv05_DECODE_SYMBOLX4_2(op2, &bitD2);
            HUFv05_DECODE_SYMBOLX4_2(op3, &bitD3);
            HUFv05_DECODE_SYMBOLX4_2(op4, &bitD4);
            HUFv05_DECODE_SYMBOLX4_1(op1, &bitD1);
            HUFv05_DECODE_SYMBOLX4_1(op2, &bitD2);
            HUFv05_DECODE_SYMBOLX4_1(op3, &bitD3);
            HUFv05_DECODE_SYMBOLX4_1(op4, &bitD4);
            HUFv05_DECODE_SYMBOLX4_2(op1, &bitD1);
            HUFv05_DECODE_SYMBOLX4_2(op2, &bitD2);
            HUFv05_DECODE_SYMBOLX4_2(op3, &bitD3);
            HUFv05_DECODE_SYMBOLX4_2(op4, &bitD4);
            HUFv05_DECODE_SYMBOLX4_0(op1, &bitD1);
            HUFv05_DECODE_SYMBOLX4_0(op2, &bitD2);
            HUFv05_DECODE_SYMBOLX4_0(op3, &bitD3);
            HUFv05_DECODE_SYMBOLX4_0(op4, &bitD4);

            endSignal = BITv05_reloadDStream(&bitD1) | BITv05_reloadDStream(&bitD2) | BITv05_reloadDStream(&bitD3) | BITv05_reloadDStream(&bitD4);
        }

        /* check corruption */
        if (op1 > opStart2) return ERROR(corruption_detected);
        if (op2 > opStart3) return ERROR(corruption_detected);
        if (op3 > opStart4) return ERROR(corruption_detected);
        /* note : op4 supposed already verified within main loop */

        /* finish bitStreams one by one */
        HUFv05_decodeStreamX4(op1, &bitD1, opStart2, dt, dtLog);
        HUFv05_decodeStreamX4(op2, &bitD2, opStart3, dt, dtLog);
        HUFv05_decodeStreamX4(op3, &bitD3, opStart4, dt, dtLog);
        HUFv05_decodeStreamX4(op4, &bitD4, oend,     dt, dtLog);

        /* check */
        endSignal = BITv05_endOfDStream(&bitD1) & BITv05_endOfDStream(&bitD2) & BITv05_endOfDStream(&bitD3) & BITv05_endOfDStream(&bitD4);
        if (!endSignal) return ERROR(corruption_detected);

        /* decoded size */
        return dstSize;
    }
}


size_t HUFv05_decompress4X4 (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize)
{
    HUFv05_CREATE_STATIC_DTABLEX4(DTable, HUFv05_MAX_TABLELOG);
    const BYTE* ip = (const BYTE*) cSrc;

    size_t hSize = HUFv05_readDTableX4 (DTable, cSrc, cSrcSize);
    if (HUFv05_isError(hSize)) return hSize;
    if (hSize >= cSrcSize) return ERROR(srcSize_wrong);
    ip += hSize;
    cSrcSize -= hSize;

    return HUFv05_decompress4X4_usingDTable (dst, dstSize, ip, cSrcSize, DTable);
}


/* ********************************/
/* Generic decompression selector */
/* ********************************/

typedef struct { U32 tableTime; U32 decode256Time; } algo_time_t;
static const algo_time_t algoTime[16 /* Quantization */][3 /* single, double, quad */] =
{
    /* single, double, quad */
    {{0,0}, {1,1}, {2,2}},  /* Q==0 : impossible */
    {{0,0}, {1,1}, {2,2}},  /* Q==1 : impossible */
    {{  38,130}, {1313, 74}, {2151, 38}},   /* Q == 2 : 12-18% */
    {{ 448,128}, {1353, 74}, {2238, 41}},   /* Q == 3 : 18-25% */
    {{ 556,128}, {1353, 74}, {2238, 47}},   /* Q == 4 : 25-32% */
    {{ 714,128}, {1418, 74}, {2436, 53}},   /* Q == 5 : 32-38% */
    {{ 883,128}, {1437, 74}, {2464, 61}},   /* Q == 6 : 38-44% */
    {{ 897,128}, {1515, 75}, {2622, 68}},   /* Q == 7 : 44-50% */
    {{ 926,128}, {1613, 75}, {2730, 75}},   /* Q == 8 : 50-56% */
    {{ 947,128}, {1729, 77}, {3359, 77}},   /* Q == 9 : 56-62% */
    {{1107,128}, {2083, 81}, {4006, 84}},   /* Q ==10 : 62-69% */
    {{1177,128}, {2379, 87}, {4785, 88}},   /* Q ==11 : 69-75% */
    {{1242,128}, {2415, 93}, {5155, 84}},   /* Q ==12 : 75-81% */
    {{1349,128}, {2644,106}, {5260,106}},   /* Q ==13 : 81-87% */
    {{1455,128}, {2422,124}, {4174,124}},   /* Q ==14 : 87-93% */
    {{ 722,128}, {1891,145}, {1936,146}},   /* Q ==15 : 93-99% */
};

typedef size_t (*decompressionAlgo)(void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize);

size_t HUFv05_decompress (void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize)
{
    static const decompressionAlgo decompress[3] = { HUFv05_decompress4X2, HUFv05_decompress4X4, NULL };
    /* estimate decompression time */
    U32 Q;
    const U32 D256 = (U32)(dstSize >> 8);
    U32 Dtime[3];
    U32 algoNb = 0;
    int n;

    /* validation checks */
    if (dstSize == 0) return ERROR(dstSize_tooSmall);
    if (cSrcSize >= dstSize) return ERROR(corruption_detected);   /* invalid, or not compressed, but not compressed already dealt with */
    if (cSrcSize == 1) { memset(dst, *(const BYTE*)cSrc, dstSize); return dstSize; }   /* RLE */

    /* decoder timing evaluation */
    Q = (U32)(cSrcSize * 16 / dstSize);   /* Q < 16 since dstSize > cSrcSize */
    for (n=0; n<3; n++)
        Dtime[n] = algoTime[Q][n].tableTime + (algoTime[Q][n].decode256Time * D256);

    Dtime[1] += Dtime[1] >> 4; Dtime[2] += Dtime[2] >> 3; /* advantage to algorithms using less memory, for cache eviction */

    if (Dtime[1] < Dtime[0]) algoNb = 1;

    return decompress[algoNb](dst, dstSize, cSrc, cSrcSize);

    /* return HUFv05_decompress4X2(dst, dstSize, cSrc, cSrcSize); */   /* multi-streams single-symbol decoding */
    /* return HUFv05_decompress4X4(dst, dstSize, cSrc, cSrcSize); */   /* multi-streams double-symbols decoding */
    /* return HUFv05_decompress4X6(dst, dstSize, cSrc, cSrcSize); */   /* multi-streams quad-symbols decoding */
}
/*
    zstd - standard compression library
    Copyright (C) 2014-2016, Yann Collet.

    BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - zstd source repository : https://github.com/Cyan4973/zstd
*/

/* ***************************************************************
*  Tuning parameters
*****************************************************************/
/*!
 * HEAPMODE :
 * Select how default decompression function ZSTDv05_decompress() will allocate memory,
 * in memory stack (0), or in memory heap (1, requires malloc())
 */
#ifndef ZSTDv05_HEAPMODE
#  define ZSTDv05_HEAPMODE 1
#endif


/*-*******************************************************
*  Dependencies
*********************************************************/
#include <stdlib.h>      /* calloc */
#include <string.h>      /* memcpy, memmove */
#include <stdio.h>       /* debug only : printf */


/*-*******************************************************
*  Compiler specifics
*********************************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  include <intrin.h>                    /* For Visual 2005 */
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#  pragma warning(disable : 4324)        /* disable: C4324: padded structure */
#endif


/*-*************************************
*  Local types
***************************************/
typedef struct
{
    blockType_t blockType;
    U32 origSize;
} blockProperties_t;


/* *******************************************************
*  Memory operations
**********************************************************/
static void ZSTDv05_copy4(void* dst, const void* src) { memcpy(dst, src, 4); }


/* *************************************
*  Error Management
***************************************/
/*! ZSTDv05_isError() :
*   tells if a return value is an error code */
unsigned ZSTDv05_isError(size_t code) { return ERR_isError(code); }


/*! ZSTDv05_getErrorName() :
*   provides error code string (useful for debugging) */
const char* ZSTDv05_getErrorName(size_t code) { return ERR_getErrorName(code); }


/* *************************************************************
*   Context management
***************************************************************/
typedef enum { ZSTDv05ds_getFrameHeaderSize, ZSTDv05ds_decodeFrameHeader,
               ZSTDv05ds_decodeBlockHeader, ZSTDv05ds_decompressBlock } ZSTDv05_dStage;

struct ZSTDv05_DCtx_s
{
    FSEv05_DTable LLTable[FSEv05_DTABLE_SIZE_U32(LLFSEv05Log)];
    FSEv05_DTable OffTable[FSEv05_DTABLE_SIZE_U32(OffFSEv05Log)];
    FSEv05_DTable MLTable[FSEv05_DTABLE_SIZE_U32(MLFSEv05Log)];
    unsigned   hufTableX4[HUFv05_DTABLE_SIZE(ZSTD_HUFFDTABLE_CAPACITY_LOG)];
    const void* previousDstEnd;
    const void* base;
    const void* vBase;
    const void* dictEnd;
    size_t expected;
    size_t headerSize;
    ZSTDv05_parameters params;
    blockType_t bType;   /* used in ZSTDv05_decompressContinue(), to transfer blockType between header decoding and block decoding stages */
    ZSTDv05_dStage stage;
    U32 flagStaticTables;
    const BYTE* litPtr;
    size_t litSize;
    BYTE litBuffer[BLOCKSIZE + WILDCOPY_OVERLENGTH];
    BYTE headerBuffer[ZSTDv05_frameHeaderSize_max];
};  /* typedef'd to ZSTDv05_DCtx within "zstd_static.h" */

size_t ZSTDv05_sizeofDCtx (void); /* Hidden declaration */
size_t ZSTDv05_sizeofDCtx (void) { return sizeof(ZSTDv05_DCtx); }

size_t ZSTDv05_decompressBegin(ZSTDv05_DCtx* dctx)
{
    dctx->expected = ZSTDv05_frameHeaderSize_min;
    dctx->stage = ZSTDv05ds_getFrameHeaderSize;
    dctx->previousDstEnd = NULL;
    dctx->base = NULL;
    dctx->vBase = NULL;
    dctx->dictEnd = NULL;
    dctx->hufTableX4[0] = ZSTD_HUFFDTABLE_CAPACITY_LOG;
    dctx->flagStaticTables = 0;
    return 0;
}

ZSTDv05_DCtx* ZSTDv05_createDCtx(void)
{
    ZSTDv05_DCtx* dctx = (ZSTDv05_DCtx*)malloc(sizeof(ZSTDv05_DCtx));
    if (dctx==NULL) return NULL;
    ZSTDv05_decompressBegin(dctx);
    return dctx;
}

size_t ZSTDv05_freeDCtx(ZSTDv05_DCtx* dctx)
{
    free(dctx);
    return 0;   /* reserved as a potential error code in the future */
}

void ZSTDv05_copyDCtx(ZSTDv05_DCtx* dstDCtx, const ZSTDv05_DCtx* srcDCtx)
{
    memcpy(dstDCtx, srcDCtx,
           sizeof(ZSTDv05_DCtx) - (BLOCKSIZE+WILDCOPY_OVERLENGTH + ZSTDv05_frameHeaderSize_max));  /* no need to copy workspace */
}


/* *************************************************************
*   Decompression section
***************************************************************/

/* Frame format description
   Frame Header -  [ Block Header - Block ] - Frame End
   1) Frame Header
      - 4 bytes - Magic Number : ZSTDv05_MAGICNUMBER (defined within zstd_internal.h)
      - 1 byte  - Window Descriptor
   2) Block Header
      - 3 bytes, starting with a 2-bits descriptor
                 Uncompressed, Compressed, Frame End, unused
   3) Block
      See Block Format Description
   4) Frame End
      - 3 bytes, compatible with Block Header
*/

/* Block format description

   Block = Literal Section - Sequences Section
   Prerequisite : size of (compressed) block, maximum size of regenerated data

   1) Literal Section

   1.1) Header : 1-5 bytes
        flags: 2 bits
            00 compressed by Huff0
            01 unused
            10 is Raw (uncompressed)
            11 is Rle
            Note : using 01 => Huff0 with precomputed table ?
            Note : delta map ? => compressed ?

   1.1.1) Huff0-compressed literal block : 3-5 bytes
            srcSize < 1 KB => 3 bytes (2-2-10-10) => single stream
            srcSize < 1 KB => 3 bytes (2-2-10-10)
            srcSize < 16KB => 4 bytes (2-2-14-14)
            else           => 5 bytes (2-2-18-18)
            big endian convention

   1.1.2) Raw (uncompressed) literal block header : 1-3 bytes
        size :  5 bits: (IS_RAW<<6) + (0<<4) + size
               12 bits: (IS_RAW<<6) + (2<<4) + (size>>8)
                        size&255
               20 bits: (IS_RAW<<6) + (3<<4) + (size>>16)
                        size>>8&255
                        size&255

   1.1.3) Rle (repeated single byte) literal block header : 1-3 bytes
        size :  5 bits: (IS_RLE<<6) + (0<<4) + size
               12 bits: (IS_RLE<<6) + (2<<4) + (size>>8)
                        size&255
               20 bits: (IS_RLE<<6) + (3<<4) + (size>>16)
                        size>>8&255
                        size&255

   1.1.4) Huff0-compressed literal block, using precomputed CTables : 3-5 bytes
            srcSize < 1 KB => 3 bytes (2-2-10-10) => single stream
            srcSize < 1 KB => 3 bytes (2-2-10-10)
            srcSize < 16KB => 4 bytes (2-2-14-14)
            else           => 5 bytes (2-2-18-18)
            big endian convention

        1- CTable available (stored into workspace ?)
        2- Small input (fast heuristic ? Full comparison ? depend on clevel ?)


   1.2) Literal block content

   1.2.1) Huff0 block, using sizes from header
        See Huff0 format

   1.2.2) Huff0 block, using prepared table

   1.2.3) Raw content

   1.2.4) single byte


   2) Sequences section
      TO DO
*/


/** ZSTDv05_decodeFrameHeader_Part1() :
*   decode the 1st part of the Frame Header, which tells Frame Header size.
*   srcSize must be == ZSTDv05_frameHeaderSize_min.
*   @return : the full size of the Frame Header */
static size_t ZSTDv05_decodeFrameHeader_Part1(ZSTDv05_DCtx* zc, const void* src, size_t srcSize)
{
    U32 magicNumber;
    if (srcSize != ZSTDv05_frameHeaderSize_min)
        return ERROR(srcSize_wrong);
    magicNumber = MEM_readLE32(src);
    if (magicNumber != ZSTDv05_MAGICNUMBER) return ERROR(prefix_unknown);
    zc->headerSize = ZSTDv05_frameHeaderSize_min;
    return zc->headerSize;
}


size_t ZSTDv05_getFrameParams(ZSTDv05_parameters* params, const void* src, size_t srcSize)
{
    U32 magicNumber;
    if (srcSize < ZSTDv05_frameHeaderSize_min) return ZSTDv05_frameHeaderSize_max;
    magicNumber = MEM_readLE32(src);
    if (magicNumber != ZSTDv05_MAGICNUMBER) return ERROR(prefix_unknown);
    memset(params, 0, sizeof(*params));
    params->windowLog = (((const BYTE*)src)[4] & 15) + ZSTDv05_WINDOWLOG_ABSOLUTEMIN;
    if ((((const BYTE*)src)[4] >> 4) != 0) return ERROR(frameParameter_unsupported);   /* reserved bits */
    return 0;
}

/** ZSTDv05_decodeFrameHeader_Part2() :
*   decode the full Frame Header.
*   srcSize must be the size provided by ZSTDv05_decodeFrameHeader_Part1().
*   @return : 0, or an error code, which can be tested using ZSTDv05_isError() */
static size_t ZSTDv05_decodeFrameHeader_Part2(ZSTDv05_DCtx* zc, const void* src, size_t srcSize)
{
    size_t result;
    if (srcSize != zc->headerSize)
        return ERROR(srcSize_wrong);
    result = ZSTDv05_getFrameParams(&(zc->params), src, srcSize);
    if ((MEM_32bits()) && (zc->params.windowLog > 25)) return ERROR(frameParameter_unsupported);
    return result;
}


static size_t ZSTDv05_getcBlockSize(const void* src, size_t srcSize, blockProperties_t* bpPtr)
{
    const BYTE* const in = (const BYTE*)src;
    BYTE headerFlags;
    U32 cSize;

    if (srcSize < 3)
        return ERROR(srcSize_wrong);

    headerFlags = *in;
    cSize = in[2] + (in[1]<<8) + ((in[0] & 7)<<16);

    bpPtr->blockType = (blockType_t)(headerFlags >> 6);
    bpPtr->origSize = (bpPtr->blockType == bt_rle) ? cSize : 0;

    if (bpPtr->blockType == bt_end) return 0;
    if (bpPtr->blockType == bt_rle) return 1;
    return cSize;
}


static size_t ZSTDv05_copyRawBlock(void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    if (dst==NULL) return ERROR(dstSize_tooSmall);
    if (srcSize > maxDstSize) return ERROR(dstSize_tooSmall);
    memcpy(dst, src, srcSize);
    return srcSize;
}


/*! ZSTDv05_decodeLiteralsBlock() :
    @return : nb of bytes read from src (< srcSize ) */
static size_t ZSTDv05_decodeLiteralsBlock(ZSTDv05_DCtx* dctx,
                                    const void* src, size_t srcSize)   /* note : srcSize < BLOCKSIZE */
{
    const BYTE* const istart = (const BYTE*) src;

    /* any compressed block with literals segment must be at least this size */
    if (srcSize < MIN_CBLOCK_SIZE) return ERROR(corruption_detected);

    switch(istart[0]>> 6)
    {
    case IS_HUFv05:
        {
            size_t litSize, litCSize, singleStream=0;
            U32 lhSize = ((istart[0]) >> 4) & 3;
            if (srcSize < 5) return ERROR(corruption_detected);   /* srcSize >= MIN_CBLOCK_SIZE == 3; here we need up to 5 for case 3 */
            switch(lhSize)
            {
            case 0: case 1: default:   /* note : default is impossible, since lhSize into [0..3] */
                /* 2 - 2 - 10 - 10 */
                lhSize=3;
                singleStream = istart[0] & 16;
                litSize  = ((istart[0] & 15) << 6) + (istart[1] >> 2);
                litCSize = ((istart[1] &  3) << 8) + istart[2];
                break;
            case 2:
                /* 2 - 2 - 14 - 14 */
                lhSize=4;
                litSize  = ((istart[0] & 15) << 10) + (istart[1] << 2) + (istart[2] >> 6);
                litCSize = ((istart[2] & 63) <<  8) + istart[3];
                break;
            case 3:
                /* 2 - 2 - 18 - 18 */
                lhSize=5;
                litSize  = ((istart[0] & 15) << 14) + (istart[1] << 6) + (istart[2] >> 2);
                litCSize = ((istart[2] &  3) << 16) + (istart[3] << 8) + istart[4];
                break;
            }
            if (litSize > BLOCKSIZE) return ERROR(corruption_detected);
            if (litCSize + lhSize > srcSize) return ERROR(corruption_detected);

            if (HUFv05_isError(singleStream ?
                            HUFv05_decompress1X2(dctx->litBuffer, litSize, istart+lhSize, litCSize) :
                            HUFv05_decompress   (dctx->litBuffer, litSize, istart+lhSize, litCSize) ))
                return ERROR(corruption_detected);

            dctx->litPtr = dctx->litBuffer;
            dctx->litSize = litSize;
            memset(dctx->litBuffer + dctx->litSize, 0, WILDCOPY_OVERLENGTH);
            return litCSize + lhSize;
        }
    case IS_PCH:
        {
            size_t errorCode;
            size_t litSize, litCSize;
            U32 lhSize = ((istart[0]) >> 4) & 3;
            if (lhSize != 1)  /* only case supported for now : small litSize, single stream */
                return ERROR(corruption_detected);
            if (!dctx->flagStaticTables)
                return ERROR(dictionary_corrupted);

            /* 2 - 2 - 10 - 10 */
            lhSize=3;
            litSize  = ((istart[0] & 15) << 6) + (istart[1] >> 2);
            litCSize = ((istart[1] &  3) << 8) + istart[2];
            if (litCSize + lhSize > srcSize) return ERROR(corruption_detected);

            errorCode = HUFv05_decompress1X4_usingDTable(dctx->litBuffer, litSize, istart+lhSize, litCSize, dctx->hufTableX4);
            if (HUFv05_isError(errorCode)) return ERROR(corruption_detected);

            dctx->litPtr = dctx->litBuffer;
            dctx->litSize = litSize;
            memset(dctx->litBuffer + dctx->litSize, 0, WILDCOPY_OVERLENGTH);
            return litCSize + lhSize;
        }
    case IS_RAW:
        {
            size_t litSize;
            U32 lhSize = ((istart[0]) >> 4) & 3;
            switch(lhSize)
            {
            case 0: case 1: default:   /* note : default is impossible, since lhSize into [0..3] */
                lhSize=1;
                litSize = istart[0] & 31;
                break;
            case 2:
                litSize = ((istart[0] & 15) << 8) + istart[1];
                break;
            case 3:
                litSize = ((istart[0] & 15) << 16) + (istart[1] << 8) + istart[2];
                break;
            }

            if (lhSize+litSize+WILDCOPY_OVERLENGTH > srcSize) {  /* risk reading beyond src buffer with wildcopy */
                if (litSize+lhSize > srcSize) return ERROR(corruption_detected);
                memcpy(dctx->litBuffer, istart+lhSize, litSize);
                dctx->litPtr = dctx->litBuffer;
                dctx->litSize = litSize;
                memset(dctx->litBuffer + dctx->litSize, 0, WILDCOPY_OVERLENGTH);
                return lhSize+litSize;
            }
            /* direct reference into compressed stream */
            dctx->litPtr = istart+lhSize;
            dctx->litSize = litSize;
            return lhSize+litSize;
        }
    case IS_RLE:
        {
            size_t litSize;
            U32 lhSize = ((istart[0]) >> 4) & 3;
            switch(lhSize)
            {
            case 0: case 1: default:   /* note : default is impossible, since lhSize into [0..3] */
                lhSize = 1;
                litSize = istart[0] & 31;
                break;
            case 2:
                litSize = ((istart[0] & 15) << 8) + istart[1];
                break;
            case 3:
                litSize = ((istart[0] & 15) << 16) + (istart[1] << 8) + istart[2];
                if (srcSize<4) return ERROR(corruption_detected);   /* srcSize >= MIN_CBLOCK_SIZE == 3; here we need lhSize+1 = 4 */
                break;
            }
            if (litSize > BLOCKSIZE) return ERROR(corruption_detected);
            memset(dctx->litBuffer, istart[lhSize], litSize + WILDCOPY_OVERLENGTH);
            dctx->litPtr = dctx->litBuffer;
            dctx->litSize = litSize;
            return lhSize+1;
        }
    default:
        return ERROR(corruption_detected);   /* impossible */
    }
}


static size_t ZSTDv05_decodeSeqHeaders(int* nbSeq, const BYTE** dumpsPtr, size_t* dumpsLengthPtr,
                         FSEv05_DTable* DTableLL, FSEv05_DTable* DTableML, FSEv05_DTable* DTableOffb,
                         const void* src, size_t srcSize, U32 flagStaticTable)
{
    const BYTE* const istart = (const BYTE*)src;
    const BYTE* ip = istart;
    const BYTE* const iend = istart + srcSize;
    U32 LLtype, Offtype, MLtype;
    unsigned LLlog, Offlog, MLlog;
    size_t dumpsLength;

    /* check */
    if (srcSize < MIN_SEQUENCES_SIZE)
        return ERROR(srcSize_wrong);

    /* SeqHead */
    *nbSeq = *ip++;
    if (*nbSeq==0) return 1;
    if (*nbSeq >= 128) {
        if (ip >= iend) return ERROR(srcSize_wrong);
        *nbSeq = ((nbSeq[0]-128)<<8) + *ip++;
    }

    if (ip >= iend) return ERROR(srcSize_wrong);
    LLtype  = *ip >> 6;
    Offtype = (*ip >> 4) & 3;
    MLtype  = (*ip >> 2) & 3;
    if (*ip & 2) {
        if (ip+3 > iend) return ERROR(srcSize_wrong);
        dumpsLength  = ip[2];
        dumpsLength += ip[1] << 8;
        ip += 3;
    } else {
        if (ip+2 > iend) return ERROR(srcSize_wrong);
        dumpsLength  = ip[1];
        dumpsLength += (ip[0] & 1) << 8;
        ip += 2;
    }
    *dumpsPtr = ip;
    ip += dumpsLength;
    *dumpsLengthPtr = dumpsLength;

    /* check */
    if (ip > iend-3) return ERROR(srcSize_wrong); /* min : all 3 are "raw", hence no header, but at least xxLog bits per type */

    /* sequences */
    {
        S16 norm[MaxML+1];    /* assumption : MaxML >= MaxLL >= MaxOff */
        size_t headerSize;

        /* Build DTables */
        switch(LLtype)
        {
        case FSEv05_ENCODING_RLE :
            LLlog = 0;
            FSEv05_buildDTable_rle(DTableLL, *ip++);
            break;
        case FSEv05_ENCODING_RAW :
            LLlog = LLbits;
            FSEv05_buildDTable_raw(DTableLL, LLbits);
            break;
        case FSEv05_ENCODING_STATIC:
            if (!flagStaticTable) return ERROR(corruption_detected);
            break;
        case FSEv05_ENCODING_DYNAMIC :
        default :   /* impossible */
            {   unsigned max = MaxLL;
                headerSize = FSEv05_readNCount(norm, &max, &LLlog, ip, iend-ip);
                if (FSEv05_isError(headerSize)) return ERROR(GENERIC);
                if (LLlog > LLFSEv05Log) return ERROR(corruption_detected);
                ip += headerSize;
                FSEv05_buildDTable(DTableLL, norm, max, LLlog);
        }   }

        switch(Offtype)
        {
        case FSEv05_ENCODING_RLE :
            Offlog = 0;
            if (ip > iend-2) return ERROR(srcSize_wrong);   /* min : "raw", hence no header, but at least xxLog bits */
            FSEv05_buildDTable_rle(DTableOffb, *ip++ & MaxOff); /* if *ip > MaxOff, data is corrupted */
            break;
        case FSEv05_ENCODING_RAW :
            Offlog = Offbits;
            FSEv05_buildDTable_raw(DTableOffb, Offbits);
            break;
        case FSEv05_ENCODING_STATIC:
            if (!flagStaticTable) return ERROR(corruption_detected);
            break;
        case FSEv05_ENCODING_DYNAMIC :
        default :   /* impossible */
            {   unsigned max = MaxOff;
                headerSize = FSEv05_readNCount(norm, &max, &Offlog, ip, iend-ip);
                if (FSEv05_isError(headerSize)) return ERROR(GENERIC);
                if (Offlog > OffFSEv05Log) return ERROR(corruption_detected);
                ip += headerSize;
                FSEv05_buildDTable(DTableOffb, norm, max, Offlog);
        }   }

        switch(MLtype)
        {
        case FSEv05_ENCODING_RLE :
            MLlog = 0;
            if (ip > iend-2) return ERROR(srcSize_wrong); /* min : "raw", hence no header, but at least xxLog bits */
            FSEv05_buildDTable_rle(DTableML, *ip++);
            break;
        case FSEv05_ENCODING_RAW :
            MLlog = MLbits;
            FSEv05_buildDTable_raw(DTableML, MLbits);
            break;
        case FSEv05_ENCODING_STATIC:
            if (!flagStaticTable) return ERROR(corruption_detected);
            break;
        case FSEv05_ENCODING_DYNAMIC :
        default :   /* impossible */
            {   unsigned max = MaxML;
                headerSize = FSEv05_readNCount(norm, &max, &MLlog, ip, iend-ip);
                if (FSEv05_isError(headerSize)) return ERROR(GENERIC);
                if (MLlog > MLFSEv05Log) return ERROR(corruption_detected);
                ip += headerSize;
                FSEv05_buildDTable(DTableML, norm, max, MLlog);
    }   }   }

    return ip-istart;
}


typedef struct {
    size_t litLength;
    size_t matchLength;
    size_t offset;
} seq_t;

typedef struct {
    BITv05_DStream_t DStream;
    FSEv05_DState_t stateLL;
    FSEv05_DState_t stateOffb;
    FSEv05_DState_t stateML;
    size_t prevOffset;
    const BYTE* dumps;
    const BYTE* dumpsEnd;
} seqState_t;



static void ZSTDv05_decodeSequence(seq_t* seq, seqState_t* seqState)
{
    size_t litLength;
    size_t prevOffset;
    size_t offset;
    size_t matchLength;
    const BYTE* dumps = seqState->dumps;
    const BYTE* const de = seqState->dumpsEnd;

    /* Literal length */
    litLength = FSEv05_peakSymbol(&(seqState->stateLL));
    prevOffset = litLength ? seq->offset : seqState->prevOffset;
    if (litLength == MaxLL) {
        const U32 add = *dumps++;
        if (add < 255) litLength += add;
        else if (dumps + 2 <= de) {
            litLength = MEM_readLE16(dumps);
            dumps += 2;
            if ((litLength & 1) && dumps < de) {
                litLength += *dumps << 16;
                dumps += 1;
            }
            litLength>>=1;
        }
        if (dumps >= de) { dumps = de-1; }  /* late correction, to avoid read overflow (data is now corrupted anyway) */
    }

    /* Offset */
    {
        static const U32 offsetPrefix[MaxOff+1] = {
                1 /*fake*/, 1, 2, 4, 8, 16, 32, 64, 128, 256,
                512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144,
                524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432, /*fake*/ 1, 1, 1, 1, 1 };
        U32 offsetCode = FSEv05_peakSymbol(&(seqState->stateOffb));   /* <= maxOff, by table construction */
        U32 nbBits = offsetCode - 1;
        if (offsetCode==0) nbBits = 0;   /* cmove */
        offset = offsetPrefix[offsetCode] + BITv05_readBits(&(seqState->DStream), nbBits);
        if (MEM_32bits()) BITv05_reloadDStream(&(seqState->DStream));
        if (offsetCode==0) offset = prevOffset;   /* repcode, cmove */
        if (offsetCode | !litLength) seqState->prevOffset = seq->offset;   /* cmove */
        FSEv05_decodeSymbol(&(seqState->stateOffb), &(seqState->DStream));    /* update */
    }

    /* Literal length update */
    FSEv05_decodeSymbol(&(seqState->stateLL), &(seqState->DStream));   /* update */
    if (MEM_32bits()) BITv05_reloadDStream(&(seqState->DStream));

    /* MatchLength */
    matchLength = FSEv05_decodeSymbol(&(seqState->stateML), &(seqState->DStream));
    if (matchLength == MaxML) {
        const U32 add = dumps<de ? *dumps++ : 0;
        if (add < 255) matchLength += add;
        else if (dumps + 2 <= de) {
            matchLength = MEM_readLE16(dumps);
            dumps += 2;
            if ((matchLength & 1) && dumps < de) {
                matchLength += *dumps << 16;
                dumps += 1;
            }
            matchLength >>= 1;
        }
        if (dumps >= de) { dumps = de-1; }  /* late correction, to avoid read overflow (data is now corrupted anyway) */
    }
    matchLength += MINMATCH;

    /* save result */
    seq->litLength = litLength;
    seq->offset = offset;
    seq->matchLength = matchLength;
    seqState->dumps = dumps;

#if 0   /* debug */
    {
        static U64 totalDecoded = 0;
        printf("pos %6u : %3u literals & match %3u bytes at distance %6u \n",
           (U32)(totalDecoded), (U32)litLength, (U32)matchLength, (U32)offset);
        totalDecoded += litLength + matchLength;
    }
#endif
}


static size_t ZSTDv05_execSequence(BYTE* op,
                                BYTE* const oend, seq_t sequence,
                                const BYTE** litPtr, const BYTE* const litLimit,
                                const BYTE* const base, const BYTE* const vBase, const BYTE* const dictEnd)
{
    static const int dec32table[] = { 0, 1, 2, 1, 4, 4, 4, 4 };   /* added */
    static const int dec64table[] = { 8, 8, 8, 7, 8, 9,10,11 };   /* subtracted */
    BYTE* const oLitEnd = op + sequence.litLength;
    const size_t sequenceLength = sequence.litLength + sequence.matchLength;
    BYTE* const oMatchEnd = op + sequenceLength;   /* risk : address space overflow (32-bits) */
    BYTE* const oend_8 = oend-8;
    const BYTE* const litEnd = *litPtr + sequence.litLength;
    const BYTE* match = oLitEnd - sequence.offset;

    /* checks */
    size_t const seqLength = sequence.litLength + sequence.matchLength;

    if (seqLength > (size_t)(oend - op)) return ERROR(dstSize_tooSmall);
    if (sequence.litLength > (size_t)(litLimit - *litPtr)) return ERROR(corruption_detected);
    /* Now we know there are no overflow in literal nor match lengths, can use pointer checks */
    if (oLitEnd > oend_8) return ERROR(dstSize_tooSmall);

    if (oMatchEnd > oend) return ERROR(dstSize_tooSmall);   /* overwrite beyond dst buffer */
    if (litEnd > litLimit) return ERROR(corruption_detected);   /* overRead beyond lit buffer */

    /* copy Literals */
    ZSTDv05_wildcopy(op, *litPtr, (ptrdiff_t)sequence.litLength);   /* note : oLitEnd <= oend-8 : no risk of overwrite beyond oend */
    op = oLitEnd;
    *litPtr = litEnd;   /* update for next sequence */

    /* copy Match */
    if (sequence.offset > (size_t)(oLitEnd - base)) {
        /* offset beyond prefix */
        if (sequence.offset > (size_t)(oLitEnd - vBase))
            return ERROR(corruption_detected);
        match = dictEnd - (base-match);
        if (match + sequence.matchLength <= dictEnd) {
            memmove(oLitEnd, match, sequence.matchLength);
            return sequenceLength;
        }
        /* span extDict & currentPrefixSegment */
        {
            size_t length1 = dictEnd - match;
            memmove(oLitEnd, match, length1);
            op = oLitEnd + length1;
            sequence.matchLength -= length1;
            match = base;
            if (op > oend_8 || sequence.matchLength < MINMATCH) {
              while (op < oMatchEnd) *op++ = *match++;
              return sequenceLength;
            }
    }   }
    /* Requirement: op <= oend_8 */

    /* match within prefix */
    if (sequence.offset < 8) {
        /* close range match, overlap */
        const int sub2 = dec64table[sequence.offset];
        op[0] = match[0];
        op[1] = match[1];
        op[2] = match[2];
        op[3] = match[3];
        match += dec32table[sequence.offset];
        ZSTDv05_copy4(op+4, match);
        match -= sub2;
    } else {
        ZSTDv05_copy8(op, match);
    }
    op += 8; match += 8;

    if (oMatchEnd > oend-(16-MINMATCH)) {
        if (op < oend_8) {
            ZSTDv05_wildcopy(op, match, oend_8 - op);
            match += oend_8 - op;
            op = oend_8;
        }
        while (op < oMatchEnd)
            *op++ = *match++;
    } else {
        ZSTDv05_wildcopy(op, match, (ptrdiff_t)sequence.matchLength-8);   /* works even if matchLength < 8 */
    }
    return sequenceLength;
}


static size_t ZSTDv05_decompressSequences(
                               ZSTDv05_DCtx* dctx,
                               void* dst, size_t maxDstSize,
                         const void* seqStart, size_t seqSize)
{
    const BYTE* ip = (const BYTE*)seqStart;
    const BYTE* const iend = ip + seqSize;
    BYTE* const ostart = (BYTE*)dst;
    BYTE* op = ostart;
    BYTE* const oend = ostart + maxDstSize;
    size_t errorCode, dumpsLength=0;
    const BYTE* litPtr = dctx->litPtr;
    const BYTE* const litEnd = litPtr + dctx->litSize;
    int nbSeq=0;
    const BYTE* dumps = NULL;
    unsigned* DTableLL = dctx->LLTable;
    unsigned* DTableML = dctx->MLTable;
    unsigned* DTableOffb = dctx->OffTable;
    const BYTE* const base = (const BYTE*) (dctx->base);
    const BYTE* const vBase = (const BYTE*) (dctx->vBase);
    const BYTE* const dictEnd = (const BYTE*) (dctx->dictEnd);

    /* Build Decoding Tables */
    errorCode = ZSTDv05_decodeSeqHeaders(&nbSeq, &dumps, &dumpsLength,
                                      DTableLL, DTableML, DTableOffb,
                                      ip, seqSize, dctx->flagStaticTables);
    if (ZSTDv05_isError(errorCode)) return errorCode;
    ip += errorCode;

    /* Regen sequences */
    if (nbSeq) {
        seq_t sequence;
        seqState_t seqState;

        memset(&sequence, 0, sizeof(sequence));
        sequence.offset = REPCODE_STARTVALUE;
        seqState.dumps = dumps;
        seqState.dumpsEnd = dumps + dumpsLength;
        seqState.prevOffset = REPCODE_STARTVALUE;
        errorCode = BITv05_initDStream(&(seqState.DStream), ip, iend-ip);
        if (ERR_isError(errorCode)) return ERROR(corruption_detected);
        FSEv05_initDState(&(seqState.stateLL), &(seqState.DStream), DTableLL);
        FSEv05_initDState(&(seqState.stateOffb), &(seqState.DStream), DTableOffb);
        FSEv05_initDState(&(seqState.stateML), &(seqState.DStream), DTableML);

        for ( ; (BITv05_reloadDStream(&(seqState.DStream)) <= BITv05_DStream_completed) && nbSeq ; ) {
            size_t oneSeqSize;
            nbSeq--;
            ZSTDv05_decodeSequence(&sequence, &seqState);
            oneSeqSize = ZSTDv05_execSequence(op, oend, sequence, &litPtr, litEnd, base, vBase, dictEnd);
            if (ZSTDv05_isError(oneSeqSize)) return oneSeqSize;
            op += oneSeqSize;
        }

        /* check if reached exact end */
        if (nbSeq) return ERROR(corruption_detected);
    }

    /* last literal segment */
    {
        size_t lastLLSize = litEnd - litPtr;
        if (litPtr > litEnd) return ERROR(corruption_detected);   /* too many literals already used */
        if (op+lastLLSize > oend) return ERROR(dstSize_tooSmall);
        if (lastLLSize > 0) {
            memcpy(op, litPtr, lastLLSize);
            op += lastLLSize;
        }
    }

    return op-ostart;
}


static void ZSTDv05_checkContinuity(ZSTDv05_DCtx* dctx, const void* dst)
{
    if (dst != dctx->previousDstEnd) {   /* not contiguous */
        dctx->dictEnd = dctx->previousDstEnd;
        dctx->vBase = (const char*)dst - ((const char*)(dctx->previousDstEnd) - (const char*)(dctx->base));
        dctx->base = dst;
        dctx->previousDstEnd = dst;
    }
}


static size_t ZSTDv05_decompressBlock_internal(ZSTDv05_DCtx* dctx,
                            void* dst, size_t dstCapacity,
                      const void* src, size_t srcSize)
{   /* blockType == blockCompressed */
    const BYTE* ip = (const BYTE*)src;
    size_t litCSize;

    if (srcSize >= BLOCKSIZE) return ERROR(srcSize_wrong);

    /* Decode literals sub-block */
    litCSize = ZSTDv05_decodeLiteralsBlock(dctx, src, srcSize);
    if (ZSTDv05_isError(litCSize)) return litCSize;
    ip += litCSize;
    srcSize -= litCSize;

    return ZSTDv05_decompressSequences(dctx, dst, dstCapacity, ip, srcSize);
}


size_t ZSTDv05_decompressBlock(ZSTDv05_DCtx* dctx,
                            void* dst, size_t dstCapacity,
                      const void* src, size_t srcSize)
{
    ZSTDv05_checkContinuity(dctx, dst);
    return ZSTDv05_decompressBlock_internal(dctx, dst, dstCapacity, src, srcSize);
}


/*! ZSTDv05_decompress_continueDCtx
*   dctx must have been properly initialized */
static size_t ZSTDv05_decompress_continueDCtx(ZSTDv05_DCtx* dctx,
                                 void* dst, size_t maxDstSize,
                                 const void* src, size_t srcSize)
{
    const BYTE* ip = (const BYTE*)src;
    const BYTE* iend = ip + srcSize;
    BYTE* const ostart = (BYTE*)dst;
    BYTE* op = ostart;
    BYTE* const oend = ostart + maxDstSize;
    size_t remainingSize = srcSize;
    blockProperties_t blockProperties;
    memset(&blockProperties, 0, sizeof(blockProperties));

    /* Frame Header */
    {   size_t frameHeaderSize;
        if (srcSize < ZSTDv05_frameHeaderSize_min+ZSTDv05_blockHeaderSize) return ERROR(srcSize_wrong);
        frameHeaderSize = ZSTDv05_decodeFrameHeader_Part1(dctx, src, ZSTDv05_frameHeaderSize_min);
        if (ZSTDv05_isError(frameHeaderSize)) return frameHeaderSize;
        if (srcSize < frameHeaderSize+ZSTDv05_blockHeaderSize) return ERROR(srcSize_wrong);
        ip += frameHeaderSize; remainingSize -= frameHeaderSize;
        frameHeaderSize = ZSTDv05_decodeFrameHeader_Part2(dctx, src, frameHeaderSize);
        if (ZSTDv05_isError(frameHeaderSize)) return frameHeaderSize;
    }

    /* Loop on each block */
    while (1)
    {
        size_t decodedSize=0;
        size_t cBlockSize = ZSTDv05_getcBlockSize(ip, iend-ip, &blockProperties);
        if (ZSTDv05_isError(cBlockSize)) return cBlockSize;

        ip += ZSTDv05_blockHeaderSize;
        remainingSize -= ZSTDv05_blockHeaderSize;
        if (cBlockSize > remainingSize) return ERROR(srcSize_wrong);

        switch(blockProperties.blockType)
        {
        case bt_compressed:
            decodedSize = ZSTDv05_decompressBlock_internal(dctx, op, oend-op, ip, cBlockSize);
            break;
        case bt_raw :
            decodedSize = ZSTDv05_copyRawBlock(op, oend-op, ip, cBlockSize);
            break;
        case bt_rle :
            return ERROR(GENERIC);   /* not yet supported */
            break;
        case bt_end :
            /* end of frame */
            if (remainingSize) return ERROR(srcSize_wrong);
            break;
        default:
            return ERROR(GENERIC);   /* impossible */
        }
        if (cBlockSize == 0) break;   /* bt_end */

        if (ZSTDv05_isError(decodedSize)) return decodedSize;
        op += decodedSize;
        ip += cBlockSize;
        remainingSize -= cBlockSize;
    }

    return op-ostart;
}


size_t ZSTDv05_decompress_usingPreparedDCtx(ZSTDv05_DCtx* dctx, const ZSTDv05_DCtx* refDCtx,
                                         void* dst, size_t maxDstSize,
                                   const void* src, size_t srcSize)
{
    ZSTDv05_copyDCtx(dctx, refDCtx);
    ZSTDv05_checkContinuity(dctx, dst);
    return ZSTDv05_decompress_continueDCtx(dctx, dst, maxDstSize, src, srcSize);
}


size_t ZSTDv05_decompress_usingDict(ZSTDv05_DCtx* dctx,
                                 void* dst, size_t maxDstSize,
                                 const void* src, size_t srcSize,
                                 const void* dict, size_t dictSize)
{
    ZSTDv05_decompressBegin_usingDict(dctx, dict, dictSize);
    ZSTDv05_checkContinuity(dctx, dst);
    return ZSTDv05_decompress_continueDCtx(dctx, dst, maxDstSize, src, srcSize);
}


size_t ZSTDv05_decompressDCtx(ZSTDv05_DCtx* dctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    return ZSTDv05_decompress_usingDict(dctx, dst, maxDstSize, src, srcSize, NULL, 0);
}

size_t ZSTDv05_decompress(void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
#if defined(ZSTDv05_HEAPMODE) && (ZSTDv05_HEAPMODE==1)
    size_t regenSize;
    ZSTDv05_DCtx* dctx = ZSTDv05_createDCtx();
    if (dctx==NULL) return ERROR(memory_allocation);
    regenSize = ZSTDv05_decompressDCtx(dctx, dst, maxDstSize, src, srcSize);
    ZSTDv05_freeDCtx(dctx);
    return regenSize;
#else
    ZSTDv05_DCtx dctx;
    return ZSTDv05_decompressDCtx(&dctx, dst, maxDstSize, src, srcSize);
#endif
}

/* ZSTD_errorFrameSizeInfoLegacy() :
   assumes `cSize` and `dBound` are _not_ NULL */
static void ZSTD_errorFrameSizeInfoLegacy(size_t* cSize, unsigned long long* dBound, size_t ret)
{
    *cSize = ret;
    *dBound = ZSTD_CONTENTSIZE_ERROR;
}

void ZSTDv05_findFrameSizeInfoLegacy(const void *src, size_t srcSize, size_t* cSize, unsigned long long* dBound)
{
    const BYTE* ip = (const BYTE*)src;
    size_t remainingSize = srcSize;
    size_t nbBlocks = 0;
    blockProperties_t blockProperties;

    /* Frame Header */
    if (srcSize < ZSTDv05_frameHeaderSize_min) {
        ZSTD_errorFrameSizeInfoLegacy(cSize, dBound, ERROR(srcSize_wrong));
        return;
    }
    if (MEM_readLE32(src) != ZSTDv05_MAGICNUMBER) {
        ZSTD_errorFrameSizeInfoLegacy(cSize, dBound, ERROR(prefix_unknown));
        return;
    }
    ip += ZSTDv05_frameHeaderSize_min; remainingSize -= ZSTDv05_frameHeaderSize_min;

    /* Loop on each block */
    while (1)
    {
        size_t cBlockSize = ZSTDv05_getcBlockSize(ip, remainingSize, &blockProperties);
        if (ZSTDv05_isError(cBlockSize)) {
            ZSTD_errorFrameSizeInfoLegacy(cSize, dBound, cBlockSize);
            return;
        }

        ip += ZSTDv05_blockHeaderSize;
        remainingSize -= ZSTDv05_blockHeaderSize;
        if (cBlockSize > remainingSize) {
            ZSTD_errorFrameSizeInfoLegacy(cSize, dBound, ERROR(srcSize_wrong));
            return;
        }

        if (cBlockSize == 0) break;   /* bt_end */

        ip += cBlockSize;
        remainingSize -= cBlockSize;
        nbBlocks++;
    }

    *cSize = ip - (const BYTE*)src;
    *dBound = nbBlocks * BLOCKSIZE;
}

/* ******************************
*  Streaming Decompression API
********************************/
size_t ZSTDv05_nextSrcSizeToDecompress(ZSTDv05_DCtx* dctx)
{
    return dctx->expected;
}

size_t ZSTDv05_decompressContinue(ZSTDv05_DCtx* dctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    /* Sanity check */
    if (srcSize != dctx->expected) return ERROR(srcSize_wrong);
    ZSTDv05_checkContinuity(dctx, dst);

    /* Decompress : frame header; part 1 */
    switch (dctx->stage)
    {
    case ZSTDv05ds_getFrameHeaderSize :
        /* get frame header size */
        if (srcSize != ZSTDv05_frameHeaderSize_min) return ERROR(srcSize_wrong);   /* impossible */
        dctx->headerSize = ZSTDv05_decodeFrameHeader_Part1(dctx, src, ZSTDv05_frameHeaderSize_min);
        if (ZSTDv05_isError(dctx->headerSize)) return dctx->headerSize;
        memcpy(dctx->headerBuffer, src, ZSTDv05_frameHeaderSize_min);
        if (dctx->headerSize > ZSTDv05_frameHeaderSize_min) return ERROR(GENERIC); /* should never happen */
        dctx->expected = 0;   /* not necessary to copy more */
        /* fallthrough */
    case ZSTDv05ds_decodeFrameHeader:
        /* get frame header */
        {   size_t const result = ZSTDv05_decodeFrameHeader_Part2(dctx, dctx->headerBuffer, dctx->headerSize);
            if (ZSTDv05_isError(result)) return result;
            dctx->expected = ZSTDv05_blockHeaderSize;
            dctx->stage = ZSTDv05ds_decodeBlockHeader;
            return 0;
        }
    case ZSTDv05ds_decodeBlockHeader:
        {
            /* Decode block header */
            blockProperties_t bp;
            size_t blockSize = ZSTDv05_getcBlockSize(src, ZSTDv05_blockHeaderSize, &bp);
            if (ZSTDv05_isError(blockSize)) return blockSize;
            if (bp.blockType == bt_end) {
                dctx->expected = 0;
                dctx->stage = ZSTDv05ds_getFrameHeaderSize;
            }
            else {
                dctx->expected = blockSize;
                dctx->bType = bp.blockType;
                dctx->stage = ZSTDv05ds_decompressBlock;
            }
            return 0;
        }
    case ZSTDv05ds_decompressBlock:
        {
            /* Decompress : block content */
            size_t rSize;
            switch(dctx->bType)
            {
            case bt_compressed:
                rSize = ZSTDv05_decompressBlock_internal(dctx, dst, maxDstSize, src, srcSize);
                break;
            case bt_raw :
                rSize = ZSTDv05_copyRawBlock(dst, maxDstSize, src, srcSize);
                break;
            case bt_rle :
                return ERROR(GENERIC);   /* not yet handled */
                break;
            case bt_end :   /* should never happen (filtered at phase 1) */
                rSize = 0;
                break;
            default:
                return ERROR(GENERIC);   /* impossible */
            }
            dctx->stage = ZSTDv05ds_decodeBlockHeader;
            dctx->expected = ZSTDv05_blockHeaderSize;
            if (ZSTDv05_isError(rSize)) return rSize;
            dctx->previousDstEnd = (char*)dst + rSize;
            return rSize;
        }
    default:
        return ERROR(GENERIC);   /* impossible */
    }
}


static void ZSTDv05_refDictContent(ZSTDv05_DCtx* dctx, const void* dict, size_t dictSize)
{
    dctx->dictEnd = dctx->previousDstEnd;
    dctx->vBase = (const char*)dict - ((const char*)(dctx->previousDstEnd) - (const char*)(dctx->base));
    dctx->base = dict;
    dctx->previousDstEnd = (const char*)dict + dictSize;
}

static size_t ZSTDv05_loadEntropy(ZSTDv05_DCtx* dctx, const void* dict, size_t dictSize)
{
    size_t hSize, offcodeHeaderSize, matchlengthHeaderSize, errorCode, litlengthHeaderSize;
    short offcodeNCount[MaxOff+1];
    unsigned offcodeMaxValue=MaxOff, offcodeLog;
    short matchlengthNCount[MaxML+1];
    unsigned matchlengthMaxValue = MaxML, matchlengthLog;
    short litlengthNCount[MaxLL+1];
    unsigned litlengthMaxValue = MaxLL, litlengthLog;

    hSize = HUFv05_readDTableX4(dctx->hufTableX4, dict, dictSize);
    if (HUFv05_isError(hSize)) return ERROR(dictionary_corrupted);
    dict = (const char*)dict + hSize;
    dictSize -= hSize;

    offcodeHeaderSize = FSEv05_readNCount(offcodeNCount, &offcodeMaxValue, &offcodeLog, dict, dictSize);
    if (FSEv05_isError(offcodeHeaderSize)) return ERROR(dictionary_corrupted);
    if (offcodeLog > OffFSEv05Log) return ERROR(dictionary_corrupted);
    errorCode = FSEv05_buildDTable(dctx->OffTable, offcodeNCount, offcodeMaxValue, offcodeLog);
    if (FSEv05_isError(errorCode)) return ERROR(dictionary_corrupted);
    dict = (const char*)dict + offcodeHeaderSize;
    dictSize -= offcodeHeaderSize;

    matchlengthHeaderSize = FSEv05_readNCount(matchlengthNCount, &matchlengthMaxValue, &matchlengthLog, dict, dictSize);
    if (FSEv05_isError(matchlengthHeaderSize)) return ERROR(dictionary_corrupted);
    if (matchlengthLog > MLFSEv05Log) return ERROR(dictionary_corrupted);
    errorCode = FSEv05_buildDTable(dctx->MLTable, matchlengthNCount, matchlengthMaxValue, matchlengthLog);
    if (FSEv05_isError(errorCode)) return ERROR(dictionary_corrupted);
    dict = (const char*)dict + matchlengthHeaderSize;
    dictSize -= matchlengthHeaderSize;

    litlengthHeaderSize = FSEv05_readNCount(litlengthNCount, &litlengthMaxValue, &litlengthLog, dict, dictSize);
    if (litlengthLog > LLFSEv05Log) return ERROR(dictionary_corrupted);
    if (FSEv05_isError(litlengthHeaderSize)) return ERROR(dictionary_corrupted);
    errorCode = FSEv05_buildDTable(dctx->LLTable, litlengthNCount, litlengthMaxValue, litlengthLog);
    if (FSEv05_isError(errorCode)) return ERROR(dictionary_corrupted);

    dctx->flagStaticTables = 1;
    return hSize + offcodeHeaderSize + matchlengthHeaderSize + litlengthHeaderSize;
}

static size_t ZSTDv05_decompress_insertDictionary(ZSTDv05_DCtx* dctx, const void* dict, size_t dictSize)
{
    size_t eSize;
    U32 magic = MEM_readLE32(dict);
    if (magic != ZSTDv05_DICT_MAGIC) {
        /* pure content mode */
        ZSTDv05_refDictContent(dctx, dict, dictSize);
        return 0;
    }
    /* load entropy tables */
    dict = (const char*)dict + 4;
    dictSize -= 4;
    eSize = ZSTDv05_loadEntropy(dctx, dict, dictSize);
    if (ZSTDv05_isError(eSize)) return ERROR(dictionary_corrupted);

    /* reference dictionary content */
    dict = (const char*)dict + eSize;
    dictSize -= eSize;
    ZSTDv05_refDictContent(dctx, dict, dictSize);

    return 0;
}


size_t ZSTDv05_decompressBegin_usingDict(ZSTDv05_DCtx* dctx, const void* dict, size_t dictSize)
{
    size_t errorCode;
    errorCode = ZSTDv05_decompressBegin(dctx);
    if (ZSTDv05_isError(errorCode)) return errorCode;

    if (dict && dictSize) {
        errorCode = ZSTDv05_decompress_insertDictionary(dctx, dict, dictSize);
        if (ZSTDv05_isError(errorCode)) return ERROR(dictionary_corrupted);
    }

    return 0;
}

/*
    Buffered version of Zstd compression library
    Copyright (C) 2015-2016, Yann Collet.

    BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - zstd source repository : https://github.com/Cyan4973/zstd
    - ztsd public forum : https://groups.google.com/forum/#!forum/lz4c
*/

/* The objects defined into this file should be considered experimental.
 * They are not labelled stable, as their prototype may change in the future.
 * You can use them for tests, provide feedback, or if you can endure risk of future changes.
 */



/* *************************************
*  Constants
***************************************/
static size_t ZBUFFv05_blockHeaderSize = 3;



/* *** Compression *** */

static size_t ZBUFFv05_limitCopy(void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    size_t length = MIN(maxDstSize, srcSize);
    if (length > 0) {
        memcpy(dst, src, length);
    }
    return length;
}




/** ************************************************
*  Streaming decompression
*
*  A ZBUFFv05_DCtx object is required to track streaming operation.
*  Use ZBUFFv05_createDCtx() and ZBUFFv05_freeDCtx() to create/release resources.
*  Use ZBUFFv05_decompressInit() to start a new decompression operation.
*  ZBUFFv05_DCtx objects can be reused multiple times.
*
*  Use ZBUFFv05_decompressContinue() repetitively to consume your input.
*  *srcSizePtr and *maxDstSizePtr can be any size.
*  The function will report how many bytes were read or written by modifying *srcSizePtr and *maxDstSizePtr.
*  Note that it may not consume the entire input, in which case it's up to the caller to call again the function with remaining input.
*  The content of dst will be overwritten (up to *maxDstSizePtr) at each function call, so save its content if it matters or change dst .
*  return : a hint to preferred nb of bytes to use as input for next function call (it's only a hint, to improve latency)
*            or 0 when a frame is completely decoded
*            or an error code, which can be tested using ZBUFFv05_isError().
*
*  Hint : recommended buffer sizes (not compulsory)
*  output : 128 KB block size is the internal unit, it ensures it's always possible to write a full block when it's decoded.
*  input : just follow indications from ZBUFFv05_decompressContinue() to minimize latency. It should always be <= 128 KB + 3 .
* **************************************************/

typedef enum { ZBUFFv05ds_init, ZBUFFv05ds_readHeader, ZBUFFv05ds_loadHeader, ZBUFFv05ds_decodeHeader,
               ZBUFFv05ds_read, ZBUFFv05ds_load, ZBUFFv05ds_flush } ZBUFFv05_dStage;

/* *** Resource management *** */

#define ZSTDv05_frameHeaderSize_max 5   /* too magical, should come from reference */
struct ZBUFFv05_DCtx_s {
    ZSTDv05_DCtx* zc;
    ZSTDv05_parameters params;
    char* inBuff;
    size_t inBuffSize;
    size_t inPos;
    char* outBuff;
    size_t outBuffSize;
    size_t outStart;
    size_t outEnd;
    size_t hPos;
    ZBUFFv05_dStage stage;
    unsigned char headerBuffer[ZSTDv05_frameHeaderSize_max];
};   /* typedef'd to ZBUFFv05_DCtx within "zstd_buffered.h" */


ZBUFFv05_DCtx* ZBUFFv05_createDCtx(void)
{
    ZBUFFv05_DCtx* zbc = (ZBUFFv05_DCtx*)malloc(sizeof(ZBUFFv05_DCtx));
    if (zbc==NULL) return NULL;
    memset(zbc, 0, sizeof(*zbc));
    zbc->zc = ZSTDv05_createDCtx();
    zbc->stage = ZBUFFv05ds_init;
    return zbc;
}

size_t ZBUFFv05_freeDCtx(ZBUFFv05_DCtx* zbc)
{
    if (zbc==NULL) return 0;   /* support free on null */
    ZSTDv05_freeDCtx(zbc->zc);
    free(zbc->inBuff);
    free(zbc->outBuff);
    free(zbc);
    return 0;
}


/* *** Initialization *** */

size_t ZBUFFv05_decompressInitDictionary(ZBUFFv05_DCtx* zbc, const void* dict, size_t dictSize)
{
    zbc->stage = ZBUFFv05ds_readHeader;
    zbc->hPos = zbc->inPos = zbc->outStart = zbc->outEnd = 0;
    return ZSTDv05_decompressBegin_usingDict(zbc->zc, dict, dictSize);
}

size_t ZBUFFv05_decompressInit(ZBUFFv05_DCtx* zbc)
{
    return ZBUFFv05_decompressInitDictionary(zbc, NULL, 0);
}


/* *** Decompression *** */

size_t ZBUFFv05_decompressContinue(ZBUFFv05_DCtx* zbc, void* dst, size_t* maxDstSizePtr, const void* src, size_t* srcSizePtr)
{
    const char* const istart = (const char*)src;
    const char* ip = istart;
    const char* const iend = istart + *srcSizePtr;
    char* const ostart = (char*)dst;
    char* op = ostart;
    char* const oend = ostart + *maxDstSizePtr;
    U32 notDone = 1;

    while (notDone) {
        switch(zbc->stage)
        {
        case ZBUFFv05ds_init :
            return ERROR(init_missing);

        case ZBUFFv05ds_readHeader :
            /* read header from src */
            {
                size_t headerSize = ZSTDv05_getFrameParams(&(zbc->params), src, *srcSizePtr);
                if (ZSTDv05_isError(headerSize)) return headerSize;
                if (headerSize) {
                    /* not enough input to decode header : tell how many bytes would be necessary */
                    memcpy(zbc->headerBuffer+zbc->hPos, src, *srcSizePtr);
                    zbc->hPos += *srcSizePtr;
                    *maxDstSizePtr = 0;
                    zbc->stage = ZBUFFv05ds_loadHeader;
                    return headerSize - zbc->hPos;
                }
                zbc->stage = ZBUFFv05ds_decodeHeader;
                break;
            }
	    /* fall-through */
        case ZBUFFv05ds_loadHeader:
            /* complete header from src */
            {
                size_t headerSize = ZBUFFv05_limitCopy(
                    zbc->headerBuffer + zbc->hPos, ZSTDv05_frameHeaderSize_max - zbc->hPos,
                    src, *srcSizePtr);
                zbc->hPos += headerSize;
                ip += headerSize;
                headerSize = ZSTDv05_getFrameParams(&(zbc->params), zbc->headerBuffer, zbc->hPos);
                if (ZSTDv05_isError(headerSize)) return headerSize;
                if (headerSize) {
                    /* not enough input to decode header : tell how many bytes would be necessary */
                    *maxDstSizePtr = 0;
                    return headerSize - zbc->hPos;
                }
                /* zbc->stage = ZBUFFv05ds_decodeHeader; break; */   /* useless : stage follows */
            }
	    /* fall-through */
        case ZBUFFv05ds_decodeHeader:
                /* apply header to create / resize buffers */
                {
                    size_t neededOutSize = (size_t)1 << zbc->params.windowLog;
                    size_t neededInSize = BLOCKSIZE;   /* a block is never > BLOCKSIZE */
                    if (zbc->inBuffSize < neededInSize) {
                        free(zbc->inBuff);
                        zbc->inBuffSize = neededInSize;
                        zbc->inBuff = (char*)malloc(neededInSize);
                        if (zbc->inBuff == NULL) return ERROR(memory_allocation);
                    }
                    if (zbc->outBuffSize < neededOutSize) {
                        free(zbc->outBuff);
                        zbc->outBuffSize = neededOutSize;
                        zbc->outBuff = (char*)malloc(neededOutSize);
                        if (zbc->outBuff == NULL) return ERROR(memory_allocation);
                }   }
                if (zbc->hPos) {
                    /* some data already loaded into headerBuffer : transfer into inBuff */
                    memcpy(zbc->inBuff, zbc->headerBuffer, zbc->hPos);
                    zbc->inPos = zbc->hPos;
                    zbc->hPos = 0;
                    zbc->stage = ZBUFFv05ds_load;
                    break;
                }
                zbc->stage = ZBUFFv05ds_read;
		/* fall-through */
        case ZBUFFv05ds_read:
            {
                size_t neededInSize = ZSTDv05_nextSrcSizeToDecompress(zbc->zc);
                if (neededInSize==0) {  /* end of frame */
                    zbc->stage = ZBUFFv05ds_init;
                    notDone = 0;
                    break;
                }
                if ((size_t)(iend-ip) >= neededInSize) {
                    /* directly decode from src */
                    size_t decodedSize = ZSTDv05_decompressContinue(zbc->zc,
                        zbc->outBuff + zbc->outStart, zbc->outBuffSize - zbc->outStart,
                        ip, neededInSize);
                    if (ZSTDv05_isError(decodedSize)) return decodedSize;
                    ip += neededInSize;
                    if (!decodedSize) break;   /* this was just a header */
                    zbc->outEnd = zbc->outStart +  decodedSize;
                    zbc->stage = ZBUFFv05ds_flush;
                    break;
                }
                if (ip==iend) { notDone = 0; break; }   /* no more input */
                zbc->stage = ZBUFFv05ds_load;
            }
	    /* fall-through */
        case ZBUFFv05ds_load:
            {
                size_t neededInSize = ZSTDv05_nextSrcSizeToDecompress(zbc->zc);
                size_t toLoad = neededInSize - zbc->inPos;   /* should always be <= remaining space within inBuff */
                size_t loadedSize;
                if (toLoad > zbc->inBuffSize - zbc->inPos) return ERROR(corruption_detected);   /* should never happen */
                loadedSize = ZBUFFv05_limitCopy(zbc->inBuff + zbc->inPos, toLoad, ip, iend-ip);
                ip += loadedSize;
                zbc->inPos += loadedSize;
                if (loadedSize < toLoad) { notDone = 0; break; }   /* not enough input, wait for more */
                {
                    size_t decodedSize = ZSTDv05_decompressContinue(zbc->zc,
                        zbc->outBuff + zbc->outStart, zbc->outBuffSize - zbc->outStart,
                        zbc->inBuff, neededInSize);
                    if (ZSTDv05_isError(decodedSize)) return decodedSize;
                    zbc->inPos = 0;   /* input is consumed */
                    if (!decodedSize) { zbc->stage = ZBUFFv05ds_read; break; }   /* this was just a header */
                    zbc->outEnd = zbc->outStart +  decodedSize;
                    zbc->stage = ZBUFFv05ds_flush;
                    /* break; */  /* ZBUFFv05ds_flush follows */
                }
	    }
	    /* fall-through */
        case ZBUFFv05ds_flush:
            {
                size_t toFlushSize = zbc->outEnd - zbc->outStart;
                size_t flushedSize = ZBUFFv05_limitCopy(op, oend-op, zbc->outBuff + zbc->outStart, toFlushSize);
                op += flushedSize;
                zbc->outStart += flushedSize;
                if (flushedSize == toFlushSize) {
                    zbc->stage = ZBUFFv05ds_read;
                    if (zbc->outStart + BLOCKSIZE > zbc->outBuffSize)
                        zbc->outStart = zbc->outEnd = 0;
                    break;
                }
                /* cannot flush everything */
                notDone = 0;
                break;
            }
        default: return ERROR(GENERIC);   /* impossible */
    }   }

    *srcSizePtr = ip-istart;
    *maxDstSizePtr = op-ostart;

    {   size_t nextSrcSizeHint = ZSTDv05_nextSrcSizeToDecompress(zbc->zc);
        if (nextSrcSizeHint > ZBUFFv05_blockHeaderSize) nextSrcSizeHint+= ZBUFFv05_blockHeaderSize;   /* get next block header too */
        nextSrcSizeHint -= zbc->inPos;   /* already loaded*/
        return nextSrcSizeHint;
    }
}



/* *************************************
*  Tool functions
***************************************/
unsigned ZBUFFv05_isError(size_t errorCode) { return ERR_isError(errorCode); }
const char* ZBUFFv05_getErrorName(size_t errorCode) { return ERR_getErrorName(errorCode); }

size_t ZBUFFv05_recommendedDInSize(void)  { return BLOCKSIZE + ZBUFFv05_blockHeaderSize /* block header size*/ ; }
size_t ZBUFFv05_recommendedDOutSize(void) { return BLOCKSIZE; }
