extern "C" {


#define DEBUGLEVEL 0
#define MEM_MODULE
#undef  XXH_NAMESPACE
#define XXH_NAMESPACE ZSTD_
#undef  XXH_PRIVATE_API
#define XXH_PRIVATE_API
#undef  XXH_INLINE_ALL
#define XXH_INLINE_ALL
#define ZSTD_LEGACY_SUPPORT 0
#define ZSTD_STRIP_ERROR_STRINGS
#define ZSTD_TRACE 0

#define ZSTD_DISABLE_ASM 1


#define ZSTD_DEPS_NEED_MALLOC


#ifndef ZSTD_DEPS_COMMON
#define ZSTD_DEPS_COMMON


#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__gnu_linux__) || \
    defined(__CYGWIN__) || defined(__MSYS__)
#if !defined(_GNU_SOURCE) && !defined(__ANDROID__)
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

#endif


#ifdef ZSTD_DEPS_NEED_MALLOC
#ifndef ZSTD_DEPS_MALLOC
#define ZSTD_DEPS_MALLOC

#include <stdlib.h>

#define ZSTD_malloc(s) malloc(s)
#define ZSTD_calloc(n,s) calloc((n), (s))
#define ZSTD_free(p) free((p))

#endif
#endif


#ifdef ZSTD_DEPS_NEED_MATH64
#ifndef ZSTD_DEPS_MATH64
#define ZSTD_DEPS_MATH64

#define ZSTD_div64(dividend, divisor) ((dividend) / (divisor))

#endif
#endif


#ifdef ZSTD_DEPS_NEED_ASSERT
#ifndef ZSTD_DEPS_ASSERT
#define ZSTD_DEPS_ASSERT

#include <assert.h>

#endif
#endif


#ifdef ZSTD_DEPS_NEED_IO
#ifndef ZSTD_DEPS_IO
#define ZSTD_DEPS_IO

#include <stdio.h>
#define ZSTD_DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)

#endif
#endif


#ifdef ZSTD_DEPS_NEED_STDINT
#ifndef ZSTD_DEPS_STDINT
#define ZSTD_DEPS_STDINT

#include <stdint.h>

#endif
#endif


#ifndef DEBUG_H_12987983217
#define DEBUG_H_12987983217


#define DEBUG_STATIC_ASSERT(c) (void)sizeof(char[(c) ? 1 : -1])


#ifndef DEBUGLEVEL
#  define DEBUGLEVEL 0
#endif


#if (DEBUGLEVEL>=1)
#  define ZSTD_DEPS_NEED_ASSERT

#else
#  ifndef assert
#    define assert(condition) ((void)0)
#  endif
#endif

#if (DEBUGLEVEL>=2)
#  define ZSTD_DEPS_NEED_IO

extern int g_debuglevel;

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
#  define RAWLOG(l, ...)   do { } while (0)
#  define DEBUGLOG(l, ...) do { } while (0)
#endif

#endif


#if !defined(ZSTD_LINUX_KERNEL) || (DEBUGLEVEL>=2)

int g_debuglevel = DEBUGLEVEL;
#endif


#ifndef MEM_H_MODULE
#define MEM_H_MODULE


#include <stddef.h>


#ifndef ZSTD_COMPILER_H
#define ZSTD_COMPILER_H

#include <stddef.h>


#ifndef ZSTD_PORTABILITY_MACROS_H
#define ZSTD_PORTABILITY_MACROS_H


#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif


#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif


#ifndef __has_feature
#  define __has_feature(x) 0
#endif


#ifndef ZSTD_MEMORY_SANITIZER
#  if __has_feature(memory_sanitizer)
#    define ZSTD_MEMORY_SANITIZER 1
#  else
#    define ZSTD_MEMORY_SANITIZER 0
#  endif
#endif


#ifndef ZSTD_ADDRESS_SANITIZER
#  if __has_feature(address_sanitizer)
#    define ZSTD_ADDRESS_SANITIZER 1
#  elif defined(__SANITIZE_ADDRESS__)
#    define ZSTD_ADDRESS_SANITIZER 1
#  else
#    define ZSTD_ADDRESS_SANITIZER 0
#  endif
#endif


#ifndef ZSTD_DATAFLOW_SANITIZER
# if __has_feature(dataflow_sanitizer)
#  define ZSTD_DATAFLOW_SANITIZER 1
# else
#  define ZSTD_DATAFLOW_SANITIZER 0
# endif
#endif


#ifdef __ELF__
# define ZSTD_HIDE_ASM_FUNCTION(func) .hidden func
#elif defined(__APPLE__)
# define ZSTD_HIDE_ASM_FUNCTION(func) .private_extern func
#else
# define ZSTD_HIDE_ASM_FUNCTION(func)
#endif


#ifndef STATIC_BMI2
#  if defined(__BMI2__)
#    define STATIC_BMI2 1
#  elif defined(_MSC_VER) && defined(__AVX2__)
#    define STATIC_BMI2 1
#  endif
#endif

#ifndef STATIC_BMI2
#  define STATIC_BMI2 0
#endif


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


#if !defined(ZSTD_DISABLE_ASM) &&                                 \
    ZSTD_ASM_SUPPORTED &&                                         \
    defined(__x86_64__) &&                                        \
    (DYNAMIC_BMI2 || defined(__BMI2__))
# define ZSTD_ENABLE_ASM_X86_64_BMI2 1
#else
# define ZSTD_ENABLE_ASM_X86_64_BMI2 0
#endif


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


#if defined(ZSTD_CLEVEL_DEFAULT) \
    || defined(ZSTD_EXCLUDE_DFAST_BLOCK_COMPRESSOR) \
    || defined(ZSTD_EXCLUDE_GREEDY_BLOCK_COMPRESSOR) \
    || defined(ZSTD_EXCLUDE_LAZY_BLOCK_COMPRESSOR) \
    || defined(ZSTD_EXCLUDE_LAZY2_BLOCK_COMPRESSOR) \
    || defined(ZSTD_EXCLUDE_BTLAZY2_BLOCK_COMPRESSOR) \
    || defined(ZSTD_EXCLUDE_BTOPT_BLOCK_COMPRESSOR) \
    || defined(ZSTD_EXCLUDE_BTULTRA_BLOCK_COMPRESSOR)
# define ZSTD_IS_DETERMINISTIC_BUILD 0
#else
# define ZSTD_IS_DETERMINISTIC_BUILD 1
#endif

#endif


#if !defined(ZSTD_NO_INLINE)
#if (defined(__GNUC__) && !defined(__STRICT_ANSI__)) || defined(__cplusplus) || defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
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


#if  defined(_MSC_VER)
#  define WIN_CDECL __cdecl
#else
#  define WIN_CDECL
#endif


#if defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
#  define UNUSED_ATTR __attribute__((unused))
#else
#  define UNUSED_ATTR
#endif


#define FORCE_INLINE_TEMPLATE static INLINE_KEYWORD FORCE_INLINE_ATTR UNUSED_ATTR

#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 8 && __GNUC__ < 5
#  define HINT_INLINE static INLINE_KEYWORD
#else
#  define HINT_INLINE FORCE_INLINE_TEMPLATE
#endif


#ifndef MEM_STATIC
#if defined(__GNUC__)
#  define MEM_STATIC static __inline UNUSED_ATTR
#elif defined(__IAR_SYSTEMS_ICC__)
#  define MEM_STATIC static inline UNUSED_ATTR
#elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) )
#  define MEM_STATIC static inline
#elif defined(_MSC_VER)
#  define MEM_STATIC static __inline
#else
#  define MEM_STATIC static
#endif
#endif


#ifdef _MSC_VER
#  define FORCE_NOINLINE static __declspec(noinline)
#else
#  if defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
#    define FORCE_NOINLINE static __attribute__((__noinline__))
#  else
#    define FORCE_NOINLINE static
#  endif
#endif


#if defined(__GNUC__) || defined(__IAR_SYSTEMS_ICC__)
#  define TARGET_ATTRIBUTE(target) __attribute__((__target__(target)))
#else
#  define TARGET_ATTRIBUTE(target)
#endif


#define BMI2_TARGET_ATTRIBUTE TARGET_ATTRIBUTE("lzcnt,bmi,bmi2")


#if defined(NO_PREFETCH)
#  define PREFETCH_L1(ptr)  do { (void)(ptr); } while (0)
#  define PREFETCH_L2(ptr)  do { (void)(ptr); } while (0)
#else
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_I86)) && !defined(_M_ARM64EC)
#    include <mmintrin.h>
#    define PREFETCH_L1(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T0)
#    define PREFETCH_L2(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T1)
#  elif defined(__GNUC__) && ( (__GNUC__ >= 4) || ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) ) )
#    define PREFETCH_L1(ptr)  __builtin_prefetch((ptr), 0 , 3 )
#    define PREFETCH_L2(ptr)  __builtin_prefetch((ptr), 0 , 2 )
#  elif defined(__aarch64__)
#    define PREFETCH_L1(ptr)  do { __asm__ __volatile__("prfm pldl1keep, %0" ::"Q"(*(ptr))); } while (0)
#    define PREFETCH_L2(ptr)  do { __asm__ __volatile__("prfm pldl2keep, %0" ::"Q"(*(ptr))); } while (0)
#  else
#    define PREFETCH_L1(ptr) do { (void)(ptr); } while (0)
#    define PREFETCH_L2(ptr) do { (void)(ptr); } while (0)
#  endif
#endif

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


#if !defined(__INTEL_COMPILER) && !defined(__clang__) && defined(__GNUC__) && !defined(__LCC__)
#  if (__GNUC__ == 4 && __GNUC_MINOR__ > 3) || (__GNUC__ >= 5)
#    define DONT_VECTORIZE __attribute__((optimize("no-tree-vectorize")))
#  else
#    define DONT_VECTORIZE _Pragma("GCC optimize(\"no-tree-vectorize\")")
#  endif
#else
#  define DONT_VECTORIZE
#endif


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


#ifdef _MSC_VER
#  include <intrin.h>
#  pragma warning(disable : 4100)
#  pragma warning(disable : 4127)
#  pragma warning(disable : 4204)
#  pragma warning(disable : 4214)
#  pragma warning(disable : 4324)
#endif


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
#  if defined(__ARM_FEATURE_SVE)
#    define ZSTD_ARCH_ARM_SVE
#  endif
#  if defined(__ARM_FEATURE_SVE2)
#    define ZSTD_ARCH_ARM_SVE2
#  endif
#  if defined(__riscv) && defined(__riscv_vector)
#    if ((defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 14) || \
        (defined(__clang__) && __clang_major__ >= 19))
        #define ZSTD_ARCH_RISCV_RVV
#  endif
#endif
#
#  if defined(ZSTD_ARCH_X86_AVX2)
#    include <immintrin.h>
#  endif
#  if defined(ZSTD_ARCH_X86_SSE2)
#    include <emmintrin.h>
#  elif defined(ZSTD_ARCH_ARM_NEON)
#    include <arm_neon.h>
#  endif
#  if defined(ZSTD_ARCH_ARM_SVE) || defined(ZSTD_ARCH_ARM_SVE2)
#    include <arm_sve.h>
#  endif
#  if defined(ZSTD_ARCH_RISCV_RVV)
#    include <riscv_vector.h>
#  endif
#endif


#if defined(__STDC_VERSION__) && (__STDC_VERSION__ > 201710L) && defined(__has_c_attribute)
# define ZSTD_HAS_C_ATTRIBUTE(x) __has_c_attribute(x)
#else
# define ZSTD_HAS_C_ATTRIBUTE(x) 0
#endif


#if defined(__cplusplus) && defined(__has_cpp_attribute)
# define ZSTD_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
# define ZSTD_HAS_CPP_ATTRIBUTE(x) 0
#endif


#ifndef ZSTD_FALLTHROUGH
# if ZSTD_HAS_C_ATTRIBUTE(fallthrough)
#  define ZSTD_FALLTHROUGH [[fallthrough]]
# elif ZSTD_HAS_CPP_ATTRIBUTE(fallthrough)
#  define ZSTD_FALLTHROUGH [[fallthrough]]
# elif __has_attribute(__fallthrough__)

#  define ZSTD_FALLTHROUGH ; __attribute__((__fallthrough__))
# else
#  define ZSTD_FALLTHROUGH
# endif
#endif


MEM_STATIC int ZSTD_isPower2(size_t u) {
    return (u & (u-1)) == 0;
}


#ifndef ZSTD_ALIGNOF
# if defined(__GNUC__) || defined(_MSC_VER)


#  define ZSTD_ALIGNOF(T) __alignof(T)

# elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)

#  include <stdalign.h>
#  define ZSTD_ALIGNOF(T) alignof(T)

# else

#  define ZSTD_ALIGNOF(T) (sizeof(void*) < sizeof(T) ? sizeof(void*) : sizeof(T))

# endif
#endif

#ifndef ZSTD_ALIGNED

# if defined(__GNUC__) || defined(__clang__)
#  define ZSTD_ALIGNED(a) __attribute__((aligned(a)))
# elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  define ZSTD_ALIGNED(a) _Alignas(a)
#elif defined(_MSC_VER)
#  define ZSTD_ALIGNED(n) __declspec(align(n))
# else

#  define ZSTD_ALIGNED(...)
# endif
#endif


#ifndef ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
#  if __has_attribute(no_sanitize)
#    if !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 8

#      define ZSTD_ALLOW_POINTER_OVERFLOW_ATTR __attribute__((no_sanitize("signed-integer-overflow")))
#    else

#      define ZSTD_ALLOW_POINTER_OVERFLOW_ATTR __attribute__((no_sanitize("pointer-overflow")))
#    endif
#  else
#    define ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
#  endif
#endif


MEM_STATIC
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
ptrdiff_t ZSTD_wrappedPtrDiff(unsigned char const* lhs, unsigned char const* rhs)
{
    return lhs - rhs;
}


MEM_STATIC
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
const void* ZSTD_wrappedPtrAdd(const void* ptr, ptrdiff_t add)
{
    return (const char*)ptr + add;
}


MEM_STATIC
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
const void* ZSTD_wrappedPtrSub(const void* ptr, ptrdiff_t sub)
{
    return (const char*)ptr - sub;
}


MEM_STATIC
void* ZSTD_maybeNullPtrAdd(void* ptr, ptrdiff_t add)
{
    return add > 0 ? (char*)ptr + add : ptr;
}


#ifdef __MINGW32__
#ifndef ZSTD_ASAN_DONT_POISON_WORKSPACE
#define ZSTD_ASAN_DONT_POISON_WORKSPACE 1
#endif
#ifndef ZSTD_MSAN_DONT_POISON_WORKSPACE
#define ZSTD_MSAN_DONT_POISON_WORKSPACE 1
#endif
#endif

#if ZSTD_MEMORY_SANITIZER && !defined(ZSTD_MSAN_DONT_POISON_WORKSPACE)

#include <stddef.h>
#define ZSTD_DEPS_NEED_STDINT


void __msan_unpoison(const volatile void *a, size_t size);


void __msan_poison(const volatile void *a, size_t size);


intptr_t __msan_test_shadow(const volatile void *x, size_t size);


void __msan_print_shadow(const volatile void *x, size_t size);
#endif

#if ZSTD_ADDRESS_SANITIZER && !defined(ZSTD_ASAN_DONT_POISON_WORKSPACE)

#include <stddef.h>


void __asan_poison_memory_region(void const volatile *addr, size_t size);


void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#endif

#endif


#if defined(_MSC_VER)
#   include <stdlib.h>
#   include <intrin.h>
#elif defined(__ICCARM__)
#   include <intrinsics.h>
#endif


#if  !defined (__VMS) && (defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) ) )
#  if defined(_AIX)
#    include <inttypes.h>
#  else
#    include <stdint.h>
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

  typedef unsigned long long  U64;
  typedef   signed long long  S64;
#endif


MEM_STATIC unsigned MEM_32bits(void);
MEM_STATIC unsigned MEM_64bits(void);
MEM_STATIC unsigned MEM_isLittleEndian(void);


MEM_STATIC U16 MEM_read16(const void* memPtr);
MEM_STATIC U32 MEM_read32(const void* memPtr);
MEM_STATIC U64 MEM_read64(const void* memPtr);
MEM_STATIC size_t MEM_readST(const void* memPtr);

MEM_STATIC void MEM_write16(void* memPtr, U16 value);
MEM_STATIC void MEM_write32(void* memPtr, U32 value);
MEM_STATIC void MEM_write64(void* memPtr, U64 value);


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


MEM_STATIC U32 MEM_readBE32(const void* memPtr);
MEM_STATIC U64 MEM_readBE64(const void* memPtr);
MEM_STATIC size_t MEM_readBEST(const void* memPtr);

MEM_STATIC void MEM_writeBE32(void* memPtr, U32 val32);
MEM_STATIC void MEM_writeBE64(void* memPtr, U64 val64);
MEM_STATIC void MEM_writeBEST(void* memPtr, size_t val);


MEM_STATIC U32 MEM_swap32(U32 in);
MEM_STATIC U64 MEM_swap64(U64 in);
MEM_STATIC size_t MEM_swapST(size_t in);


#ifndef MEM_FORCE_MEMORY_ACCESS
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
    const union { U32 u; BYTE c[4]; } one = { 1 };
    return one.c[0];
#endif
}

#if defined(MEM_FORCE_MEMORY_ACCESS) && (MEM_FORCE_MEMORY_ACCESS==2)


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

#endif

MEM_STATIC U32 MEM_swap32_fallback(U32 in)
{
    return  ((in << 24) & 0xff000000 ) |
            ((in <<  8) & 0x00ff0000 ) |
            ((in >>  8) & 0x0000ff00 ) |
            ((in >> 24) & 0x000000ff );
}

MEM_STATIC U32 MEM_swap32(U32 in)
{
#if defined(_MSC_VER)
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
#if defined(_MSC_VER)
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


MEM_STATIC void MEM_check(void) { DEBUG_STATIC_ASSERT((sizeof(size_t)==4) || (sizeof(size_t)==8)); }

#endif


#ifndef ERROR_H_MODULE
#define ERROR_H_MODULE


#ifndef ZSTD_ERRORS_H_398273423
#define ZSTD_ERRORS_H_398273423

#if defined (__cplusplus)
extern "C" {
#endif


#ifndef ZSTDERRORLIB_VISIBLE

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
#  define ZSTDERRORLIB_API __declspec(dllimport) ZSTDERRORLIB_VISIBLE
#else
#  define ZSTDERRORLIB_API ZSTDERRORLIB_VISIBLE
#endif


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

  ZSTD_error_frameIndex_tooLarge = 100,
  ZSTD_error_seekableIO          = 102,
  ZSTD_error_dstBuffer_wrong     = 104,
  ZSTD_error_srcBuffer_wrong     = 105,
  ZSTD_error_sequenceProducer_failed = 106,
  ZSTD_error_externalSequences_invalid = 107,
  ZSTD_error_maxCode = 120
} ZSTD_ErrorCode;

ZSTDERRORLIB_API const char* ZSTD_getErrorString(ZSTD_ErrorCode code);


#if defined (__cplusplus)
}
#endif

#endif


#if defined(__GNUC__)
#  define ERR_STATIC static __attribute__((unused))
#elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) )
#  define ERR_STATIC static inline
#elif defined(_MSC_VER)
#  define ERR_STATIC static __inline
#else
#  define ERR_STATIC static
#endif


typedef ZSTD_ErrorCode ERR_enum;
#define PREFIX(name) ZSTD_error_##name


#undef ERROR
#define ERROR(name) ZSTD_ERROR(name)
#define ZSTD_ERROR(name) ((size_t)-PREFIX(name))

ERR_STATIC unsigned ERR_isError(size_t code) { return (code > ERROR(maxCode)); }

ERR_STATIC ERR_enum ERR_getErrorCode(size_t code) { if (!ERR_isError(code)) return (ERR_enum)0; return (ERR_enum) (0-code); }


#define CHECK_V_F(e, f)     \
    size_t const e = f;     \
    do {                    \
        if (ERR_isError(e)) \
            return e;       \
    } while (0)
#define CHECK_F(f)   do { CHECK_V_F(_var_err__, f); } while (0)


const char* ERR_getErrorString(ERR_enum code);

ERR_STATIC const char* ERR_getErrorName(size_t code)
{
    return ERR_getErrorString(ERR_getErrorCode(code));
}


static INLINE_KEYWORD UNUSED_ATTR
void _force_has_format_string(const char *format, ...) {
  (void)format;
}


#define _FORCE_HAS_FORMAT_STRING(...)              \
    do {                                           \
        if (0) {                                   \
            _force_has_format_string(__VA_ARGS__); \
        }                                          \
    } while (0)

#define ERR_QUOTE(str) #str


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


#define RETURN_ERROR(err, ...)                                               \
    do {                                                                     \
        RAWLOG(3, "%s:%d: ERROR!: unconditional check failed, returning %s", \
              __FILE__, __LINE__, ERR_QUOTE(ERROR(err)));                    \
        _FORCE_HAS_FORMAT_STRING(__VA_ARGS__);                               \
        RAWLOG(3, ": " __VA_ARGS__);                                         \
        RAWLOG(3, "\n");                                                     \
        return ERROR(err);                                                   \
    } while(0)


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

#endif

#define FSE_STATIC_LINKING_ONLY


#ifndef FSE_H
#define FSE_H


#if defined(FSE_DLL_EXPORT) && (FSE_DLL_EXPORT==1) && defined(__GNUC__) && (__GNUC__ >= 4)
#  define FSE_PUBLIC_API __attribute__ ((visibility ("default")))
#elif defined(FSE_DLL_EXPORT) && (FSE_DLL_EXPORT==1)
#  define FSE_PUBLIC_API __declspec(dllexport)
#elif defined(FSE_DLL_IMPORT) && (FSE_DLL_IMPORT==1)
#  define FSE_PUBLIC_API __declspec(dllimport)
#else
#  define FSE_PUBLIC_API
#endif


#define FSE_VERSION_MAJOR    0
#define FSE_VERSION_MINOR    9
#define FSE_VERSION_RELEASE  0

#define FSE_LIB_VERSION FSE_VERSION_MAJOR.FSE_VERSION_MINOR.FSE_VERSION_RELEASE
#define FSE_QUOTE(str) #str
#define FSE_EXPAND_AND_QUOTE(str) FSE_QUOTE(str)
#define FSE_VERSION_STRING FSE_EXPAND_AND_QUOTE(FSE_LIB_VERSION)

#define FSE_VERSION_NUMBER  (FSE_VERSION_MAJOR *100*100 + FSE_VERSION_MINOR *100 + FSE_VERSION_RELEASE)
FSE_PUBLIC_API unsigned FSE_versionNumber(void);


FSE_PUBLIC_API size_t FSE_compressBound(size_t size);


FSE_PUBLIC_API unsigned    FSE_isError(size_t code);
FSE_PUBLIC_API const char* FSE_getErrorName(size_t code);


FSE_PUBLIC_API unsigned FSE_optimalTableLog(unsigned maxTableLog, size_t srcSize, unsigned maxSymbolValue);


FSE_PUBLIC_API size_t FSE_normalizeCount(short* normalizedCounter, unsigned tableLog,
                    const unsigned* count, size_t srcSize, unsigned maxSymbolValue, unsigned useLowProbCount);


FSE_PUBLIC_API size_t FSE_NCountWriteBound(unsigned maxSymbolValue, unsigned tableLog);


FSE_PUBLIC_API size_t FSE_writeNCount (void* buffer, size_t bufferSize,
                                 const short* normalizedCounter,
                                 unsigned maxSymbolValue, unsigned tableLog);


typedef unsigned FSE_CTable;


FSE_PUBLIC_API size_t FSE_buildCTable(FSE_CTable* ct, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog);


FSE_PUBLIC_API size_t FSE_compress_usingCTable (void* dst, size_t dstCapacity, const void* src, size_t srcSize, const FSE_CTable* ct);


FSE_PUBLIC_API size_t FSE_readNCount (short* normalizedCounter,
                           unsigned* maxSymbolValuePtr, unsigned* tableLogPtr,
                           const void* rBuffer, size_t rBuffSize);


FSE_PUBLIC_API size_t FSE_readNCount_bmi2(short* normalizedCounter,
                           unsigned* maxSymbolValuePtr, unsigned* tableLogPtr,
                           const void* rBuffer, size_t rBuffSize, int bmi2);

typedef unsigned FSE_DTable;


#endif


#if defined(FSE_STATIC_LINKING_ONLY) && !defined(FSE_H_FSE_STATIC_LINKING_ONLY)
#define FSE_H_FSE_STATIC_LINKING_ONLY


#ifndef BITSTREAM_H_MODULE
#define BITSTREAM_H_MODULE


#ifndef ZSTD_BITS_H
#define ZSTD_BITS_H


MEM_STATIC unsigned ZSTD_countTrailingZeros32_fallback(U32 val)
{
    assert(val != 0);
    {
        static const U32 DeBruijnBytePos[32] = {0, 1, 28, 2, 29, 14, 24, 3,
                                                30, 22, 20, 15, 25, 17, 4, 8,
                                                31, 27, 13, 23, 21, 19, 16, 7,
                                                26, 12, 18, 6, 11, 5, 10, 9};
        return DeBruijnBytePos[((U32) ((val & (0-val)) * 0x077CB531U)) >> 27];
    }
}

MEM_STATIC unsigned ZSTD_countTrailingZeros32(U32 val)
{
    assert(val != 0);
#if defined(_MSC_VER)
#  if STATIC_BMI2
    return (unsigned)_tzcnt_u32(val);
#  else
    if (val != 0) {
        unsigned long r;
        _BitScanForward(&r, val);
        return (unsigned)r;
    } else {
        __assume(0);
    }
#  endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
    return (unsigned)__builtin_ctz(val);
#elif defined(__ICCARM__)
    return (unsigned)__builtin_ctz(val);
#else
    return ZSTD_countTrailingZeros32_fallback(val);
#endif
}

MEM_STATIC unsigned ZSTD_countLeadingZeros32_fallback(U32 val)
{
    assert(val != 0);
    {
        static const U32 DeBruijnClz[32] = {0, 9, 1, 10, 13, 21, 2, 29,
                                            11, 14, 16, 18, 22, 25, 3, 30,
                                            8, 12, 20, 28, 15, 17, 24, 7,
                                            19, 27, 23, 6, 26, 5, 4, 31};
        val |= val >> 1;
        val |= val >> 2;
        val |= val >> 4;
        val |= val >> 8;
        val |= val >> 16;
        return 31 - DeBruijnClz[(val * 0x07C4ACDDU) >> 27];
    }
}

MEM_STATIC unsigned ZSTD_countLeadingZeros32(U32 val)
{
    assert(val != 0);
#if defined(_MSC_VER)
#  if STATIC_BMI2
    return (unsigned)_lzcnt_u32(val);
#  else
    if (val != 0) {
        unsigned long r;
        _BitScanReverse(&r, val);
        return (unsigned)(31 - r);
    } else {
        __assume(0);
    }
#  endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
    return (unsigned)__builtin_clz(val);
#elif defined(__ICCARM__)
    return (unsigned)__builtin_clz(val);
#else
    return ZSTD_countLeadingZeros32_fallback(val);
#endif
}

MEM_STATIC unsigned ZSTD_countTrailingZeros64(U64 val)
{
    assert(val != 0);
#if defined(_MSC_VER) && defined(_WIN64)
#  if STATIC_BMI2
    return (unsigned)_tzcnt_u64(val);
#  else
    if (val != 0) {
        unsigned long r;
        _BitScanForward64(&r, val);
        return (unsigned)r;
    } else {
        __assume(0);
    }
#  endif
#elif defined(__GNUC__) && (__GNUC__ >= 4) && defined(__LP64__)
    return (unsigned)__builtin_ctzll(val);
#elif defined(__ICCARM__)
    return (unsigned)__builtin_ctzll(val);
#else
    {
        U32 mostSignificantWord = (U32)(val >> 32);
        U32 leastSignificantWord = (U32)val;
        if (leastSignificantWord == 0) {
            return 32 + ZSTD_countTrailingZeros32(mostSignificantWord);
        } else {
            return ZSTD_countTrailingZeros32(leastSignificantWord);
        }
    }
#endif
}

MEM_STATIC unsigned ZSTD_countLeadingZeros64(U64 val)
{
    assert(val != 0);
#if defined(_MSC_VER) && defined(_WIN64)
#  if STATIC_BMI2
    return (unsigned)_lzcnt_u64(val);
#  else
    if (val != 0) {
        unsigned long r;
        _BitScanReverse64(&r, val);
        return (unsigned)(63 - r);
    } else {
        __assume(0);
    }
#  endif
#elif defined(__GNUC__) && (__GNUC__ >= 4)
    return (unsigned)(__builtin_clzll(val));
#elif defined(__ICCARM__)
    return (unsigned)(__builtin_clzll(val));
#else
    {
        U32 mostSignificantWord = (U32)(val >> 32);
        U32 leastSignificantWord = (U32)val;
        if (mostSignificantWord == 0) {
            return 32 + ZSTD_countLeadingZeros32(leastSignificantWord);
        } else {
            return ZSTD_countLeadingZeros32(mostSignificantWord);
        }
    }
#endif
}

MEM_STATIC unsigned ZSTD_NbCommonBytes(size_t val)
{
    if (MEM_isLittleEndian()) {
        if (MEM_64bits()) {
            return ZSTD_countTrailingZeros64((U64)val) >> 3;
        } else {
            return ZSTD_countTrailingZeros32((U32)val) >> 3;
        }
    } else {
        if (MEM_64bits()) {
            return ZSTD_countLeadingZeros64((U64)val) >> 3;
        } else {
            return ZSTD_countLeadingZeros32((U32)val) >> 3;
        }
    }
}

MEM_STATIC unsigned ZSTD_highbit32(U32 val)
{
    assert(val != 0);
    return 31 - ZSTD_countLeadingZeros32(val);
}


MEM_STATIC
U64 ZSTD_rotateRight_U64(U64 const value, U32 count) {
    assert(count < 64);
    count &= 0x3F;
    return (value >> count) | (U64)(value << ((0U - count) & 0x3F));
}

MEM_STATIC
U32 ZSTD_rotateRight_U32(U32 const value, U32 count) {
    assert(count < 32);
    count &= 0x1F;
    return (value >> count) | (U32)(value << ((0U - count) & 0x1F));
}

MEM_STATIC
U16 ZSTD_rotateRight_U16(U16 const value, U32 count) {
    assert(count < 16);
    count &= 0x0F;
    return (value >> count) | (U16)(value << ((0U - count) & 0x0F));
}

#endif


#ifndef ZSTD_NO_INTRINSICS
#  if (defined(__BMI__) || defined(__BMI2__)) && defined(__GNUC__)
#    include <immintrin.h>
#  elif defined(__ICCARM__)
#    include <intrinsics.h>
#  endif
#endif

#define STREAM_ACCUMULATOR_MIN_32  25
#define STREAM_ACCUMULATOR_MIN_64  57
#define STREAM_ACCUMULATOR_MIN    ((U32)(MEM_32bits() ? STREAM_ACCUMULATOR_MIN_32 : STREAM_ACCUMULATOR_MIN_64))


typedef size_t BitContainerType;

typedef struct {
    BitContainerType bitContainer;
    unsigned bitPos;
    char*  startPtr;
    char*  ptr;
    char*  endPtr;
} BIT_CStream_t;

MEM_STATIC size_t BIT_initCStream(BIT_CStream_t* bitC, void* dstBuffer, size_t dstCapacity);
MEM_STATIC void   BIT_addBits(BIT_CStream_t* bitC, BitContainerType value, unsigned nbBits);
MEM_STATIC void   BIT_flushBits(BIT_CStream_t* bitC);
MEM_STATIC size_t BIT_closeCStream(BIT_CStream_t* bitC);


typedef struct {
    BitContainerType bitContainer;
    unsigned bitsConsumed;
    const char* ptr;
    const char* start;
    const char* limitPtr;
} BIT_DStream_t;

typedef enum { BIT_DStream_unfinished = 0,
               BIT_DStream_endOfBuffer = 1,
               BIT_DStream_completed = 2,
               BIT_DStream_overflow = 3
    } BIT_DStream_status;

MEM_STATIC size_t   BIT_initDStream(BIT_DStream_t* bitD, const void* srcBuffer, size_t srcSize);
FORCE_INLINE_TEMPLATE BitContainerType BIT_readBits(BIT_DStream_t* bitD, unsigned nbBits);
FORCE_INLINE_TEMPLATE BIT_DStream_status BIT_reloadDStream(BIT_DStream_t* bitD);
MEM_STATIC unsigned BIT_endOfDStream(const BIT_DStream_t* bitD);


MEM_STATIC void BIT_addBitsFast(BIT_CStream_t* bitC, BitContainerType value, unsigned nbBits);


MEM_STATIC void BIT_flushBitsFast(BIT_CStream_t* bitC);


MEM_STATIC size_t BIT_readBitsFast(BIT_DStream_t* bitD, unsigned nbBits);


static const unsigned BIT_mask[] = {
    0,          1,         3,         7,         0xF,       0x1F,
    0x3F,       0x7F,      0xFF,      0x1FF,     0x3FF,     0x7FF,
    0xFFF,      0x1FFF,    0x3FFF,    0x7FFF,    0xFFFF,    0x1FFFF,
    0x3FFFF,    0x7FFFF,   0xFFFFF,   0x1FFFFF,  0x3FFFFF,  0x7FFFFF,
    0xFFFFFF,   0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF, 0x1FFFFFFF,
    0x3FFFFFFF, 0x7FFFFFFF};
#define BIT_MASK_SIZE (sizeof(BIT_mask) / sizeof(BIT_mask[0]))


MEM_STATIC size_t BIT_initCStream(BIT_CStream_t* bitC,
                                  void* startPtr, size_t dstCapacity)
{
    bitC->bitContainer = 0;
    bitC->bitPos = 0;
    bitC->startPtr = (char*)startPtr;
    bitC->ptr = bitC->startPtr;
    bitC->endPtr = bitC->startPtr + dstCapacity - sizeof(bitC->bitContainer);
    if (dstCapacity <= sizeof(bitC->bitContainer)) return ERROR(dstSize_tooSmall);
    return 0;
}

FORCE_INLINE_TEMPLATE BitContainerType BIT_getLowerBits(BitContainerType bitContainer, U32 const nbBits)
{
#if STATIC_BMI2 && !defined(ZSTD_NO_INTRINSICS)
#  if (defined(__x86_64__) || defined(_M_X64)) && !defined(__ILP32__)
    return _bzhi_u64(bitContainer, nbBits);
#  else
    DEBUG_STATIC_ASSERT(sizeof(bitContainer) == sizeof(U32));
    return _bzhi_u32(bitContainer, nbBits);
#  endif
#else
    assert(nbBits < BIT_MASK_SIZE);
    return bitContainer & BIT_mask[nbBits];
#endif
}


MEM_STATIC void BIT_addBits(BIT_CStream_t* bitC,
                            BitContainerType value, unsigned nbBits)
{
    DEBUG_STATIC_ASSERT(BIT_MASK_SIZE == 32);
    assert(nbBits < BIT_MASK_SIZE);
    assert(nbBits + bitC->bitPos < sizeof(bitC->bitContainer) * 8);
    bitC->bitContainer |= BIT_getLowerBits(value, nbBits) << bitC->bitPos;
    bitC->bitPos += nbBits;
}


MEM_STATIC void BIT_addBitsFast(BIT_CStream_t* bitC,
                                BitContainerType value, unsigned nbBits)
{
    assert((value>>nbBits) == 0);
    assert(nbBits + bitC->bitPos < sizeof(bitC->bitContainer) * 8);
    bitC->bitContainer |= value << bitC->bitPos;
    bitC->bitPos += nbBits;
}


MEM_STATIC void BIT_flushBitsFast(BIT_CStream_t* bitC)
{
    size_t const nbBytes = bitC->bitPos >> 3;
    assert(bitC->bitPos < sizeof(bitC->bitContainer) * 8);
    assert(bitC->ptr <= bitC->endPtr);
    MEM_writeLEST(bitC->ptr, bitC->bitContainer);
    bitC->ptr += nbBytes;
    bitC->bitPos &= 7;
    bitC->bitContainer >>= nbBytes*8;
}


MEM_STATIC void BIT_flushBits(BIT_CStream_t* bitC)
{
    size_t const nbBytes = bitC->bitPos >> 3;
    assert(bitC->bitPos < sizeof(bitC->bitContainer) * 8);
    assert(bitC->ptr <= bitC->endPtr);
    MEM_writeLEST(bitC->ptr, bitC->bitContainer);
    bitC->ptr += nbBytes;
    if (bitC->ptr > bitC->endPtr) bitC->ptr = bitC->endPtr;
    bitC->bitPos &= 7;
    bitC->bitContainer >>= nbBytes*8;
}


MEM_STATIC size_t BIT_closeCStream(BIT_CStream_t* bitC)
{
    BIT_addBitsFast(bitC, 1, 1);
    BIT_flushBits(bitC);
    if (bitC->ptr >= bitC->endPtr) return 0;
    return (size_t)(bitC->ptr - bitC->startPtr) + (bitC->bitPos > 0);
}


MEM_STATIC size_t BIT_initDStream(BIT_DStream_t* bitD, const void* srcBuffer, size_t srcSize)
{
    if (srcSize < 1) { ZSTD_memset(bitD, 0, sizeof(*bitD)); return ERROR(srcSize_wrong); }

    bitD->start = (const char*)srcBuffer;
    bitD->limitPtr = bitD->start + sizeof(bitD->bitContainer);

    if (srcSize >=  sizeof(bitD->bitContainer)) {
        bitD->ptr   = (const char*)srcBuffer + srcSize - sizeof(bitD->bitContainer);
        bitD->bitContainer = MEM_readLEST(bitD->ptr);
        { BYTE const lastByte = ((const BYTE*)srcBuffer)[srcSize-1];
          bitD->bitsConsumed = lastByte ? 8 - ZSTD_highbit32(lastByte) : 0;
          if (lastByte == 0) return ERROR(GENERIC);  }
    } else {
        bitD->ptr   = bitD->start;
        bitD->bitContainer = *(const BYTE*)(bitD->start);
        switch(srcSize)
        {
        case 7: bitD->bitContainer += (BitContainerType)(((const BYTE*)(srcBuffer))[6]) << (sizeof(bitD->bitContainer)*8 - 16);
                ZSTD_FALLTHROUGH;

        case 6: bitD->bitContainer += (BitContainerType)(((const BYTE*)(srcBuffer))[5]) << (sizeof(bitD->bitContainer)*8 - 24);
                ZSTD_FALLTHROUGH;

        case 5: bitD->bitContainer += (BitContainerType)(((const BYTE*)(srcBuffer))[4]) << (sizeof(bitD->bitContainer)*8 - 32);
                ZSTD_FALLTHROUGH;

        case 4: bitD->bitContainer += (BitContainerType)(((const BYTE*)(srcBuffer))[3]) << 24;
                ZSTD_FALLTHROUGH;

        case 3: bitD->bitContainer += (BitContainerType)(((const BYTE*)(srcBuffer))[2]) << 16;
                ZSTD_FALLTHROUGH;

        case 2: bitD->bitContainer += (BitContainerType)(((const BYTE*)(srcBuffer))[1]) <<  8;
                ZSTD_FALLTHROUGH;

        default: break;
        }
        {   BYTE const lastByte = ((const BYTE*)srcBuffer)[srcSize-1];
            bitD->bitsConsumed = lastByte ? 8 - ZSTD_highbit32(lastByte) : 0;
            if (lastByte == 0) return ERROR(corruption_detected);
        }
        bitD->bitsConsumed += (U32)(sizeof(bitD->bitContainer) - srcSize)*8;
    }

    return srcSize;
}

FORCE_INLINE_TEMPLATE BitContainerType BIT_getUpperBits(BitContainerType bitContainer, U32 const start)
{
    return bitContainer >> start;
}

FORCE_INLINE_TEMPLATE BitContainerType BIT_getMiddleBits(BitContainerType bitContainer, U32 const start, U32 const nbBits)
{
    U32 const regMask = sizeof(bitContainer)*8 - 1;

    assert(nbBits < BIT_MASK_SIZE);

#if defined(__x86_64__) || defined(_M_X64)
    return (bitContainer >> (start & regMask)) & ((((U64)1) << nbBits) - 1);
#else
    return (bitContainer >> (start & regMask)) & BIT_mask[nbBits];
#endif
}


FORCE_INLINE_TEMPLATE BitContainerType BIT_lookBits(const BIT_DStream_t*  bitD, U32 nbBits)
{

#if 1

    return BIT_getMiddleBits(bitD->bitContainer, (sizeof(bitD->bitContainer)*8) - bitD->bitsConsumed - nbBits, nbBits);
#else

    U32 const regMask = sizeof(bitD->bitContainer)*8 - 1;
    return ((bitD->bitContainer << (bitD->bitsConsumed & regMask)) >> 1) >> ((regMask-nbBits) & regMask);
#endif
}


MEM_STATIC BitContainerType BIT_lookBitsFast(const BIT_DStream_t* bitD, U32 nbBits)
{
    U32 const regMask = sizeof(bitD->bitContainer)*8 - 1;
    assert(nbBits >= 1);
    return (bitD->bitContainer << (bitD->bitsConsumed & regMask)) >> (((regMask+1)-nbBits) & regMask);
}

FORCE_INLINE_TEMPLATE void BIT_skipBits(BIT_DStream_t* bitD, U32 nbBits)
{
    bitD->bitsConsumed += nbBits;
}


FORCE_INLINE_TEMPLATE BitContainerType BIT_readBits(BIT_DStream_t* bitD, unsigned nbBits)
{
    BitContainerType const value = BIT_lookBits(bitD, nbBits);
    BIT_skipBits(bitD, nbBits);
    return value;
}


MEM_STATIC BitContainerType BIT_readBitsFast(BIT_DStream_t* bitD, unsigned nbBits)
{
    BitContainerType const value = BIT_lookBitsFast(bitD, nbBits);
    assert(nbBits >= 1);
    BIT_skipBits(bitD, nbBits);
    return value;
}


MEM_STATIC BIT_DStream_status BIT_reloadDStream_internal(BIT_DStream_t* bitD)
{
    assert(bitD->bitsConsumed <= sizeof(bitD->bitContainer)*8);
    bitD->ptr -= bitD->bitsConsumed >> 3;
    assert(bitD->ptr >= bitD->start);
    bitD->bitsConsumed &= 7;
    bitD->bitContainer = MEM_readLEST(bitD->ptr);
    return BIT_DStream_unfinished;
}


MEM_STATIC BIT_DStream_status BIT_reloadDStreamFast(BIT_DStream_t* bitD)
{
    if (UNLIKELY(bitD->ptr < bitD->limitPtr))
        return BIT_DStream_overflow;
    return BIT_reloadDStream_internal(bitD);
}


FORCE_INLINE_TEMPLATE BIT_DStream_status BIT_reloadDStream(BIT_DStream_t* bitD)
{

    if (UNLIKELY(bitD->bitsConsumed > (sizeof(bitD->bitContainer)*8))) {
        static const BitContainerType zeroFilled = 0;
        bitD->ptr = (const char*)&zeroFilled;

        return BIT_DStream_overflow;
    }

    assert(bitD->ptr >= bitD->start);

    if (bitD->ptr >= bitD->limitPtr) {
        return BIT_reloadDStream_internal(bitD);
    }
    if (bitD->ptr == bitD->start) {

        if (bitD->bitsConsumed < sizeof(bitD->bitContainer)*8) return BIT_DStream_endOfBuffer;
        return BIT_DStream_completed;
    }

    {   U32 nbBytes = bitD->bitsConsumed >> 3;
        BIT_DStream_status result = BIT_DStream_unfinished;
        if (bitD->ptr - nbBytes < bitD->start) {
            nbBytes = (U32)(bitD->ptr - bitD->start);
            result = BIT_DStream_endOfBuffer;
        }
        bitD->ptr -= nbBytes;
        bitD->bitsConsumed -= nbBytes*8;
        bitD->bitContainer = MEM_readLEST(bitD->ptr);
        return result;
    }
}


MEM_STATIC unsigned BIT_endOfDStream(const BIT_DStream_t* DStream)
{
    return ((DStream->ptr == DStream->start) && (DStream->bitsConsumed == sizeof(DStream->bitContainer)*8));
}

#endif


#define FSE_NCOUNTBOUND 512
#define FSE_BLOCKBOUND(size) ((size) + ((size)>>7) + 4  + sizeof(size_t) )
#define FSE_COMPRESSBOUND(size) (FSE_NCOUNTBOUND + FSE_BLOCKBOUND(size))


#define FSE_CTABLE_SIZE_U32(maxTableLog, maxSymbolValue)   (1 + (1<<((maxTableLog)-1)) + (((maxSymbolValue)+1)*2))
#define FSE_DTABLE_SIZE_U32(maxTableLog)                   (1 + (1<<(maxTableLog)))


#define FSE_CTABLE_SIZE(maxTableLog, maxSymbolValue)   (FSE_CTABLE_SIZE_U32(maxTableLog, maxSymbolValue) * sizeof(FSE_CTable))
#define FSE_DTABLE_SIZE(maxTableLog)                   (FSE_DTABLE_SIZE_U32(maxTableLog) * sizeof(FSE_DTable))


unsigned FSE_optimalTableLog_internal(unsigned maxTableLog, size_t srcSize, unsigned maxSymbolValue, unsigned minus);


size_t FSE_buildCTable_rle (FSE_CTable* ct, unsigned char symbolValue);


#define FSE_BUILD_CTABLE_WORKSPACE_SIZE_U32(maxSymbolValue, tableLog) (((maxSymbolValue + 2) + (1ull << (tableLog)))/2 + sizeof(U64)/sizeof(U32) )
#define FSE_BUILD_CTABLE_WORKSPACE_SIZE(maxSymbolValue, tableLog) (sizeof(unsigned) * FSE_BUILD_CTABLE_WORKSPACE_SIZE_U32(maxSymbolValue, tableLog))
size_t FSE_buildCTable_wksp(FSE_CTable* ct, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog, void* workSpace, size_t wkspSize);

#define FSE_BUILD_DTABLE_WKSP_SIZE(maxTableLog, maxSymbolValue) (sizeof(short) * (maxSymbolValue + 1) + (1ULL << maxTableLog) + 8)
#define FSE_BUILD_DTABLE_WKSP_SIZE_U32(maxTableLog, maxSymbolValue) ((FSE_BUILD_DTABLE_WKSP_SIZE(maxTableLog, maxSymbolValue) + sizeof(unsigned) - 1) / sizeof(unsigned))
FSE_PUBLIC_API size_t FSE_buildDTable_wksp(FSE_DTable* dt, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog, void* workSpace, size_t wkspSize);


#define FSE_DECOMPRESS_WKSP_SIZE_U32(maxTableLog, maxSymbolValue) (FSE_DTABLE_SIZE_U32(maxTableLog) + 1 + FSE_BUILD_DTABLE_WKSP_SIZE_U32(maxTableLog, maxSymbolValue) + (FSE_MAX_SYMBOL_VALUE + 1) / 2 + 1)
#define FSE_DECOMPRESS_WKSP_SIZE(maxTableLog, maxSymbolValue) (FSE_DECOMPRESS_WKSP_SIZE_U32(maxTableLog, maxSymbolValue) * sizeof(unsigned))
size_t FSE_decompress_wksp_bmi2(void* dst, size_t dstCapacity, const void* cSrc, size_t cSrcSize, unsigned maxLog, void* workSpace, size_t wkspSize, int bmi2);


typedef enum {
   FSE_repeat_none,
   FSE_repeat_check,
   FSE_repeat_valid
 } FSE_repeat;


typedef struct {
    ptrdiff_t   value;
    const void* stateTable;
    const void* symbolTT;
    unsigned    stateLog;
} FSE_CState_t;

static void FSE_initCState(FSE_CState_t* CStatePtr, const FSE_CTable* ct);

static void FSE_encodeSymbol(BIT_CStream_t* bitC, FSE_CState_t* CStatePtr, unsigned symbol);

static void FSE_flushCState(BIT_CStream_t* bitC, const FSE_CState_t* CStatePtr);


typedef struct {
    size_t      state;
    const void* table;
} FSE_DState_t;


static void     FSE_initDState(FSE_DState_t* DStatePtr, BIT_DStream_t* bitD, const FSE_DTable* dt);

static unsigned char FSE_decodeSymbol(FSE_DState_t* DStatePtr, BIT_DStream_t* bitD);

static unsigned FSE_endOfDState(const FSE_DState_t* DStatePtr);


static unsigned char FSE_decodeSymbolFast(FSE_DState_t* DStatePtr, BIT_DStream_t* bitD);


typedef struct {
    int deltaFindState;
    U32 deltaNbBits;
} FSE_symbolCompressionTransform;

MEM_STATIC void FSE_initCState(FSE_CState_t* statePtr, const FSE_CTable* ct)
{
    const void* ptr = ct;
    const U16* u16ptr = (const U16*) ptr;
    const U32 tableLog = MEM_read16(ptr);
    statePtr->value = (ptrdiff_t)1<<tableLog;
    statePtr->stateTable = u16ptr+2;
    statePtr->symbolTT = ct + 1 + (tableLog ? (1<<(tableLog-1)) : 1);
    statePtr->stateLog = tableLog;
}


MEM_STATIC void FSE_initCState2(FSE_CState_t* statePtr, const FSE_CTable* ct, U32 symbol)
{
    FSE_initCState(statePtr, ct);
    {   const FSE_symbolCompressionTransform symbolTT = ((const FSE_symbolCompressionTransform*)(statePtr->symbolTT))[symbol];
        const U16* stateTable = (const U16*)(statePtr->stateTable);
        U32 nbBitsOut  = (U32)((symbolTT.deltaNbBits + (1<<15)) >> 16);
        statePtr->value = (nbBitsOut << 16) - symbolTT.deltaNbBits;
        statePtr->value = stateTable[(statePtr->value >> nbBitsOut) + symbolTT.deltaFindState];
    }
}

MEM_STATIC void FSE_encodeSymbol(BIT_CStream_t* bitC, FSE_CState_t* statePtr, unsigned symbol)
{
    FSE_symbolCompressionTransform const symbolTT = ((const FSE_symbolCompressionTransform*)(statePtr->symbolTT))[symbol];
    const U16* const stateTable = (const U16*)(statePtr->stateTable);
    U32 const nbBitsOut  = (U32)((statePtr->value + symbolTT.deltaNbBits) >> 16);
    BIT_addBits(bitC, (BitContainerType)statePtr->value, nbBitsOut);
    statePtr->value = stateTable[ (statePtr->value >> nbBitsOut) + symbolTT.deltaFindState];
}

MEM_STATIC void FSE_flushCState(BIT_CStream_t* bitC, const FSE_CState_t* statePtr)
{
    BIT_addBits(bitC, (BitContainerType)statePtr->value, statePtr->stateLog);
    BIT_flushBits(bitC);
}


MEM_STATIC U32 FSE_getMaxNbBits(const void* symbolTTPtr, U32 symbolValue)
{
    const FSE_symbolCompressionTransform* symbolTT = (const FSE_symbolCompressionTransform*) symbolTTPtr;
    return (symbolTT[symbolValue].deltaNbBits + ((1<<16)-1)) >> 16;
}


MEM_STATIC U32 FSE_bitCost(const void* symbolTTPtr, U32 tableLog, U32 symbolValue, U32 accuracyLog)
{
    const FSE_symbolCompressionTransform* symbolTT = (const FSE_symbolCompressionTransform*) symbolTTPtr;
    U32 const minNbBits = symbolTT[symbolValue].deltaNbBits >> 16;
    U32 const threshold = (minNbBits+1) << 16;
    assert(tableLog < 16);
    assert(accuracyLog < 31-tableLog);
    {   U32 const tableSize = 1 << tableLog;
        U32 const deltaFromThreshold = threshold - (symbolTT[symbolValue].deltaNbBits + tableSize);
        U32 const normalizedDeltaFromThreshold = (deltaFromThreshold << accuracyLog) >> tableLog;
        U32 const bitMultiplier = 1 << accuracyLog;
        assert(symbolTT[symbolValue].deltaNbBits + tableSize <= threshold);
        assert(normalizedDeltaFromThreshold <= bitMultiplier);
        return (minNbBits+1)*bitMultiplier - normalizedDeltaFromThreshold;
    }
}


typedef struct {
    U16 tableLog;
    U16 fastMode;
} FSE_DTableHeader;

typedef struct
{
    unsigned short newState;
    unsigned char  symbol;
    unsigned char  nbBits;
} FSE_decode_t;

MEM_STATIC void FSE_initDState(FSE_DState_t* DStatePtr, BIT_DStream_t* bitD, const FSE_DTable* dt)
{
    const void* ptr = dt;
    const FSE_DTableHeader* const DTableH = (const FSE_DTableHeader*)ptr;
    DStatePtr->state = BIT_readBits(bitD, DTableH->tableLog);
    BIT_reloadDStream(bitD);
    DStatePtr->table = dt + 1;
}

MEM_STATIC BYTE FSE_peekSymbol(const FSE_DState_t* DStatePtr)
{
    FSE_decode_t const DInfo = ((const FSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    return DInfo.symbol;
}

MEM_STATIC void FSE_updateState(FSE_DState_t* DStatePtr, BIT_DStream_t* bitD)
{
    FSE_decode_t const DInfo = ((const FSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    U32 const nbBits = DInfo.nbBits;
    size_t const lowBits = BIT_readBits(bitD, nbBits);
    DStatePtr->state = DInfo.newState + lowBits;
}

MEM_STATIC BYTE FSE_decodeSymbol(FSE_DState_t* DStatePtr, BIT_DStream_t* bitD)
{
    FSE_decode_t const DInfo = ((const FSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    U32 const nbBits = DInfo.nbBits;
    BYTE const symbol = DInfo.symbol;
    size_t const lowBits = BIT_readBits(bitD, nbBits);

    DStatePtr->state = DInfo.newState + lowBits;
    return symbol;
}


MEM_STATIC BYTE FSE_decodeSymbolFast(FSE_DState_t* DStatePtr, BIT_DStream_t* bitD)
{
    FSE_decode_t const DInfo = ((const FSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    U32 const nbBits = DInfo.nbBits;
    BYTE const symbol = DInfo.symbol;
    size_t const lowBits = BIT_readBitsFast(bitD, nbBits);

    DStatePtr->state = DInfo.newState + lowBits;
    return symbol;
}

MEM_STATIC unsigned FSE_endOfDState(const FSE_DState_t* DStatePtr)
{
    return DStatePtr->state == 0;
}


#ifndef FSE_COMMONDEFS_ONLY


#ifndef FSE_MAX_MEMORY_USAGE
#  define FSE_MAX_MEMORY_USAGE 14
#endif
#ifndef FSE_DEFAULT_MEMORY_USAGE
#  define FSE_DEFAULT_MEMORY_USAGE 13
#endif
#if (FSE_DEFAULT_MEMORY_USAGE > FSE_MAX_MEMORY_USAGE)
#  error "FSE_DEFAULT_MEMORY_USAGE must be <= FSE_MAX_MEMORY_USAGE"
#endif


#ifndef FSE_MAX_SYMBOL_VALUE
#  define FSE_MAX_SYMBOL_VALUE 255
#endif


#define FSE_FUNCTION_TYPE BYTE
#define FSE_FUNCTION_EXTENSION
#define FSE_DECODE_TYPE FSE_decode_t


#endif


#define FSE_MAX_TABLELOG  (FSE_MAX_MEMORY_USAGE-2)
#define FSE_MAX_TABLESIZE (1U<<FSE_MAX_TABLELOG)
#define FSE_MAXTABLESIZE_MASK (FSE_MAX_TABLESIZE-1)
#define FSE_DEFAULT_TABLELOG (FSE_DEFAULT_MEMORY_USAGE-2)
#define FSE_MIN_TABLELOG 5

#define FSE_TABLELOG_ABSOLUTE_MAX 15
#if FSE_MAX_TABLELOG > FSE_TABLELOG_ABSOLUTE_MAX
#  error "FSE_MAX_TABLELOG > FSE_TABLELOG_ABSOLUTE_MAX is not supported"
#endif

#define FSE_TABLESTEP(tableSize) (((tableSize)>>1) + ((tableSize)>>3) + 3)

#endif


#ifndef HUF_H_298734234
#define HUF_H_298734234


#define FSE_STATIC_LINKING_ONLY


#define HUF_BLOCKSIZE_MAX (128 * 1024)
size_t HUF_compressBound(size_t size);


unsigned    HUF_isError(size_t code);
const char* HUF_getErrorName(size_t code);


#define HUF_WORKSPACE_SIZE ((8 << 10) + 512 )
#define HUF_WORKSPACE_SIZE_U64 (HUF_WORKSPACE_SIZE / sizeof(U64))


#define HUF_TABLELOG_MAX      12
#define HUF_TABLELOG_DEFAULT  11
#define HUF_SYMBOLVALUE_MAX  255

#define HUF_TABLELOG_ABSOLUTEMAX  12
#if (HUF_TABLELOG_MAX > HUF_TABLELOG_ABSOLUTEMAX)
#  error "HUF_TABLELOG_MAX is too large !"
#endif


#define HUF_CTABLEBOUND 129
#define HUF_BLOCKBOUND(size) (size + (size>>8) + 8)
#define HUF_COMPRESSBOUND(size) (HUF_CTABLEBOUND + HUF_BLOCKBOUND(size))


typedef size_t HUF_CElt;
#define HUF_CTABLE_SIZE_ST(maxSymbolValue)   ((maxSymbolValue)+2)
#define HUF_CTABLE_SIZE(maxSymbolValue)       (HUF_CTABLE_SIZE_ST(maxSymbolValue) * sizeof(size_t))
#define HUF_CREATE_STATIC_CTABLE(name, maxSymbolValue) \
    HUF_CElt name[HUF_CTABLE_SIZE_ST(maxSymbolValue)]


typedef U32 HUF_DTable;
#define HUF_DTABLE_SIZE(maxTableLog)   (1 + (1<<(maxTableLog)))
#define HUF_CREATE_STATIC_DTABLEX1(DTable, maxTableLog) \
        HUF_DTable DTable[HUF_DTABLE_SIZE((maxTableLog)-1)] = { ((U32)((maxTableLog)-1) * 0x01000001) }
#define HUF_CREATE_STATIC_DTABLEX2(DTable, maxTableLog) \
        HUF_DTable DTable[HUF_DTABLE_SIZE(maxTableLog)] = { ((U32)(maxTableLog) * 0x01000001) }


typedef enum {

    HUF_flags_bmi2 = (1 << 0),

    HUF_flags_optimalDepth = (1 << 1),

    HUF_flags_preferRepeat = (1 << 2),

    HUF_flags_suspectUncompressible = (1 << 3),

    HUF_flags_disableAsm = (1 << 4),

    HUF_flags_disableFast = (1 << 5)
} HUF_flags_e;


#define HUF_OPTIMAL_DEPTH_THRESHOLD ZSTD_btultra


unsigned HUF_minTableLog(unsigned symbolCardinality);
unsigned HUF_cardinality(const unsigned* count, unsigned maxSymbolValue);
unsigned HUF_optimalTableLog(unsigned maxTableLog, size_t srcSize, unsigned maxSymbolValue, void* workSpace,
 size_t wkspSize, HUF_CElt* table, const unsigned* count, int flags);
size_t HUF_writeCTable_wksp(void* dst, size_t maxDstSize, const HUF_CElt* CTable, unsigned maxSymbolValue, unsigned huffLog, void* workspace, size_t workspaceSize);
size_t HUF_compress4X_usingCTable(void* dst, size_t dstSize, const void* src, size_t srcSize, const HUF_CElt* CTable, int flags);
size_t HUF_estimateCompressedSize(const HUF_CElt* CTable, const unsigned* count, unsigned maxSymbolValue);
int HUF_validateCTable(const HUF_CElt* CTable, const unsigned* count, unsigned maxSymbolValue);

typedef enum {
   HUF_repeat_none,
   HUF_repeat_check,
   HUF_repeat_valid
 } HUF_repeat;


size_t HUF_compress4X_repeat(void* dst, size_t dstSize,
                       const void* src, size_t srcSize,
                       unsigned maxSymbolValue, unsigned tableLog,
                       void* workSpace, size_t wkspSize,
                       HUF_CElt* hufTable, HUF_repeat* repeat, int flags);


#define HUF_CTABLE_WORKSPACE_SIZE_U32 ((4 * (HUF_SYMBOLVALUE_MAX + 1)) + 192)
#define HUF_CTABLE_WORKSPACE_SIZE (HUF_CTABLE_WORKSPACE_SIZE_U32 * sizeof(unsigned))
size_t HUF_buildCTable_wksp (HUF_CElt* tree,
                       const unsigned* count, U32 maxSymbolValue, U32 maxNbBits,
                             void* workSpace, size_t wkspSize);


size_t HUF_readStats(BYTE* huffWeight, size_t hwSize,
                     U32* rankStats, U32* nbSymbolsPtr, U32* tableLogPtr,
                     const void* src, size_t srcSize);


#define HUF_READ_STATS_WORKSPACE_SIZE_U32 FSE_DECOMPRESS_WKSP_SIZE_U32(6, HUF_TABLELOG_MAX-1)
#define HUF_READ_STATS_WORKSPACE_SIZE (HUF_READ_STATS_WORKSPACE_SIZE_U32 * sizeof(unsigned))
size_t HUF_readStats_wksp(BYTE* huffWeight, size_t hwSize,
                          U32* rankStats, U32* nbSymbolsPtr, U32* tableLogPtr,
                          const void* src, size_t srcSize,
                          void* workspace, size_t wkspSize,
                          int flags);


size_t HUF_readCTable (HUF_CElt* CTable, unsigned* maxSymbolValuePtr, const void* src, size_t srcSize, unsigned *hasZeroWeights);


U32 HUF_getNbBitsFromCTable(const HUF_CElt* symbolTable, U32 symbolValue);

typedef struct {
    BYTE tableLog;
    BYTE maxSymbolValue;
    BYTE unused[sizeof(size_t) - 2];
} HUF_CTableHeader;


HUF_CTableHeader HUF_readCTableHeader(HUF_CElt const* ctable);


U32 HUF_selectDecoder (size_t dstSize, size_t cSrcSize);


#define HUF_DECOMPRESS_WORKSPACE_SIZE ((2 << 10) + (1 << 9))
#define HUF_DECOMPRESS_WORKSPACE_SIZE_U32 (HUF_DECOMPRESS_WORKSPACE_SIZE / sizeof(U32))


size_t HUF_compress1X_usingCTable(void* dst, size_t dstSize, const void* src, size_t srcSize, const HUF_CElt* CTable, int flags);

size_t HUF_compress1X_repeat(void* dst, size_t dstSize,
                       const void* src, size_t srcSize,
                       unsigned maxSymbolValue, unsigned tableLog,
                       void* workSpace, size_t wkspSize,
                       HUF_CElt* hufTable, HUF_repeat* repeat, int flags);

size_t HUF_decompress1X_DCtx_wksp(HUF_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize, void* workSpace, size_t wkspSize, int flags);
#ifndef HUF_FORCE_DECOMPRESS_X1
size_t HUF_decompress1X2_DCtx_wksp(HUF_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize, void* workSpace, size_t wkspSize, int flags);
#endif


size_t HUF_decompress1X_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_DTable* DTable, int flags);
#ifndef HUF_FORCE_DECOMPRESS_X2
size_t HUF_decompress1X1_DCtx_wksp(HUF_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize, void* workSpace, size_t wkspSize, int flags);
#endif
size_t HUF_decompress4X_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_DTable* DTable, int flags);
size_t HUF_decompress4X_hufOnly_wksp(HUF_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize, void* workSpace, size_t wkspSize, int flags);
#ifndef HUF_FORCE_DECOMPRESS_X2
size_t HUF_readDTableX1_wksp(HUF_DTable* DTable, const void* src, size_t srcSize, void* workSpace, size_t wkspSize, int flags);
#endif
#ifndef HUF_FORCE_DECOMPRESS_X1
size_t HUF_readDTableX2_wksp(HUF_DTable* DTable, const void* src, size_t srcSize, void* workSpace, size_t wkspSize, int flags);
#endif

#endif


unsigned FSE_versionNumber(void) { return FSE_VERSION_NUMBER; }


unsigned FSE_isError(size_t code) { return ERR_isError(code); }
const char* FSE_getErrorName(size_t code) { return ERR_getErrorName(code); }

unsigned HUF_isError(size_t code) { return ERR_isError(code); }
const char* HUF_getErrorName(size_t code) { return ERR_getErrorName(code); }


FORCE_INLINE_TEMPLATE
size_t FSE_readNCount_body(short* normalizedCounter, unsigned* maxSVPtr, unsigned* tableLogPtr,
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
    unsigned const maxSV1 = *maxSVPtr + 1;
    int previous0 = 0;

    if (hbSize < 8) {

        char buffer[8] = {0};
        ZSTD_memcpy(buffer, headerBuffer, hbSize);
        {   size_t const countSize = FSE_readNCount(normalizedCounter, maxSVPtr, tableLogPtr,
                                                    buffer, sizeof(buffer));
            if (FSE_isError(countSize)) return countSize;
            if (countSize > hbSize) return ERROR(corruption_detected);
            return countSize;
    }   }
    assert(hbSize >= 8);


    ZSTD_memset(normalizedCounter, 0, (*maxSVPtr+1) * sizeof(normalizedCounter[0]));
    bitStream = MEM_readLE32(ip);
    nbBits = (bitStream & 0xF) + FSE_MIN_TABLELOG;
    if (nbBits > FSE_TABLELOG_ABSOLUTE_MAX) return ERROR(tableLog_tooLarge);
    bitStream >>= 4;
    bitCount = 4;
    *tableLogPtr = nbBits;
    remaining = (1<<nbBits)+1;
    threshold = 1<<nbBits;
    nbBits++;

    for (;;) {
        if (previous0) {

            int repeats = ZSTD_countTrailingZeros32(~bitStream | 0x80000000) >> 1;
            while (repeats >= 12) {
                charnum += 3 * 12;
                if (LIKELY(ip <= iend-7)) {
                    ip += 3;
                } else {
                    bitCount -= (int)(8 * (iend - 7 - ip));
                    bitCount &= 31;
                    ip = iend - 4;
                }
                bitStream = MEM_readLE32(ip) >> bitCount;
                repeats = ZSTD_countTrailingZeros32(~bitStream | 0x80000000) >> 1;
            }
            charnum += 3 * repeats;
            bitStream >>= 2 * repeats;
            bitCount += 2 * repeats;


            assert((bitStream & 3) < 3);
            charnum += bitStream & 3;
            bitCount += 2;


            if (charnum >= maxSV1) break;


            if (LIKELY(ip <= iend-7) || (ip + (bitCount>>3) <= iend-4)) {
                assert((bitCount >> 3) <= 3);
                ip += bitCount>>3;
                bitCount &= 7;
            } else {
                bitCount -= (int)(8 * (iend - 4 - ip));
                bitCount &= 31;
                ip = iend - 4;
            }
            bitStream = MEM_readLE32(ip) >> bitCount;
        }
        {
            int const max = (2*threshold-1) - remaining;
            int count;

            if ((bitStream & (threshold-1)) < (U32)max) {
                count = bitStream & (threshold-1);
                bitCount += nbBits-1;
            } else {
                count = bitStream & (2*threshold-1);
                if (count >= threshold) count -= max;
                bitCount += nbBits;
            }

            count--;

            if (count >= 0) {
                remaining -= count;
            } else {
                assert(count == -1);
                remaining += count;
            }
            normalizedCounter[charnum++] = (short)count;
            previous0 = !count;

            assert(threshold > 1);
            if (remaining < threshold) {

                if (remaining <= 1) break;
                nbBits = ZSTD_highbit32(remaining) + 1;
                threshold = 1 << (nbBits - 1);
            }
            if (charnum >= maxSV1) break;

            if (LIKELY(ip <= iend-7) || (ip + (bitCount>>3) <= iend-4)) {
                ip += bitCount>>3;
                bitCount &= 7;
            } else {
                bitCount -= (int)(8 * (iend - 4 - ip));
                bitCount &= 31;
                ip = iend - 4;
            }
            bitStream = MEM_readLE32(ip) >> bitCount;
    }   }
    if (remaining != 1) return ERROR(corruption_detected);

    if (charnum > maxSV1) return ERROR(maxSymbolValue_tooSmall);
    if (bitCount > 32) return ERROR(corruption_detected);
    *maxSVPtr = charnum-1;

    ip += (bitCount+7)>>3;
    return ip-istart;
}


static size_t FSE_readNCount_body_default(
        short* normalizedCounter, unsigned* maxSVPtr, unsigned* tableLogPtr,
        const void* headerBuffer, size_t hbSize)
{
    return FSE_readNCount_body(normalizedCounter, maxSVPtr, tableLogPtr, headerBuffer, hbSize);
}

#if DYNAMIC_BMI2
BMI2_TARGET_ATTRIBUTE static size_t FSE_readNCount_body_bmi2(
        short* normalizedCounter, unsigned* maxSVPtr, unsigned* tableLogPtr,
        const void* headerBuffer, size_t hbSize)
{
    return FSE_readNCount_body(normalizedCounter, maxSVPtr, tableLogPtr, headerBuffer, hbSize);
}
#endif

size_t FSE_readNCount_bmi2(
        short* normalizedCounter, unsigned* maxSVPtr, unsigned* tableLogPtr,
        const void* headerBuffer, size_t hbSize, int bmi2)
{
#if DYNAMIC_BMI2
    if (bmi2) {
        return FSE_readNCount_body_bmi2(normalizedCounter, maxSVPtr, tableLogPtr, headerBuffer, hbSize);
    }
#endif
    (void)bmi2;
    return FSE_readNCount_body_default(normalizedCounter, maxSVPtr, tableLogPtr, headerBuffer, hbSize);
}

size_t FSE_readNCount(
        short* normalizedCounter, unsigned* maxSVPtr, unsigned* tableLogPtr,
        const void* headerBuffer, size_t hbSize)
{
    return FSE_readNCount_bmi2(normalizedCounter, maxSVPtr, tableLogPtr, headerBuffer, hbSize,  0);
}


size_t HUF_readStats(BYTE* huffWeight, size_t hwSize, U32* rankStats,
                     U32* nbSymbolsPtr, U32* tableLogPtr,
                     const void* src, size_t srcSize)
{
    U32 wksp[HUF_READ_STATS_WORKSPACE_SIZE_U32];
    return HUF_readStats_wksp(huffWeight, hwSize, rankStats, nbSymbolsPtr, tableLogPtr, src, srcSize, wksp, sizeof(wksp),  0);
}

FORCE_INLINE_TEMPLATE size_t
HUF_readStats_body(BYTE* huffWeight, size_t hwSize, U32* rankStats,
                   U32* nbSymbolsPtr, U32* tableLogPtr,
                   const void* src, size_t srcSize,
                   void* workSpace, size_t wkspSize,
                   int bmi2)
{
    U32 weightTotal;
    const BYTE* ip = (const BYTE*) src;
    size_t iSize;
    size_t oSize;

    if (!srcSize) return ERROR(srcSize_wrong);
    iSize = ip[0];


    if (iSize >= 128) {
        oSize = iSize - 127;
        iSize = ((oSize+1)/2);
        if (iSize+1 > srcSize) return ERROR(srcSize_wrong);
        if (oSize >= hwSize) return ERROR(corruption_detected);
        ip += 1;
        {   U32 n;
            for (n=0; n<oSize; n+=2) {
                huffWeight[n]   = ip[n/2] >> 4;
                huffWeight[n+1] = ip[n/2] & 15;
    }   }   }
    else  {
        if (iSize+1 > srcSize) return ERROR(srcSize_wrong);

        oSize = FSE_decompress_wksp_bmi2(huffWeight, hwSize-1, ip+1, iSize, 6, workSpace, wkspSize, bmi2);
        if (FSE_isError(oSize)) return oSize;
    }


    ZSTD_memset(rankStats, 0, (HUF_TABLELOG_MAX + 1) * sizeof(U32));
    weightTotal = 0;
    {   U32 n; for (n=0; n<oSize; n++) {
            if (huffWeight[n] > HUF_TABLELOG_MAX) return ERROR(corruption_detected);
            rankStats[huffWeight[n]]++;
            weightTotal += (1 << huffWeight[n]) >> 1;
    }   }
    if (weightTotal == 0) return ERROR(corruption_detected);


    {   U32 const tableLog = ZSTD_highbit32(weightTotal) + 1;
        if (tableLog > HUF_TABLELOG_MAX) return ERROR(corruption_detected);
        *tableLogPtr = tableLog;

        {   U32 const total = 1 << tableLog;
            U32 const rest = total - weightTotal;
            U32 const verif = 1 << ZSTD_highbit32(rest);
            U32 const lastWeight = ZSTD_highbit32(rest) + 1;
            if (verif != rest) return ERROR(corruption_detected);
            huffWeight[oSize] = (BYTE)lastWeight;
            rankStats[lastWeight]++;
    }   }


    if ((rankStats[1] < 2) || (rankStats[1] & 1)) return ERROR(corruption_detected);


    *nbSymbolsPtr = (U32)(oSize+1);
    return iSize+1;
}


static size_t HUF_readStats_body_default(BYTE* huffWeight, size_t hwSize, U32* rankStats,
                     U32* nbSymbolsPtr, U32* tableLogPtr,
                     const void* src, size_t srcSize,
                     void* workSpace, size_t wkspSize)
{
    return HUF_readStats_body(huffWeight, hwSize, rankStats, nbSymbolsPtr, tableLogPtr, src, srcSize, workSpace, wkspSize, 0);
}

#if DYNAMIC_BMI2
static BMI2_TARGET_ATTRIBUTE size_t HUF_readStats_body_bmi2(BYTE* huffWeight, size_t hwSize, U32* rankStats,
                     U32* nbSymbolsPtr, U32* tableLogPtr,
                     const void* src, size_t srcSize,
                     void* workSpace, size_t wkspSize)
{
    return HUF_readStats_body(huffWeight, hwSize, rankStats, nbSymbolsPtr, tableLogPtr, src, srcSize, workSpace, wkspSize, 1);
}
#endif

size_t HUF_readStats_wksp(BYTE* huffWeight, size_t hwSize, U32* rankStats,
                     U32* nbSymbolsPtr, U32* tableLogPtr,
                     const void* src, size_t srcSize,
                     void* workSpace, size_t wkspSize,
                     int flags)
{
#if DYNAMIC_BMI2
    if (flags & HUF_flags_bmi2) {
        return HUF_readStats_body_bmi2(huffWeight, hwSize, rankStats, nbSymbolsPtr, tableLogPtr, src, srcSize, workSpace, wkspSize);
    }
#endif
    (void)flags;
    return HUF_readStats_body_default(huffWeight, hwSize, rankStats, nbSymbolsPtr, tableLogPtr, src, srcSize, workSpace, wkspSize);
}


const char* ERR_getErrorString(ERR_enum code)
{
#ifdef ZSTD_STRIP_ERROR_STRINGS
    (void)code;
    return "Error strings stripped";
#else
    static const char* const notErrorCode = "Unspecified error code";
    switch( code )
    {
    case PREFIX(no_error): return "No error detected";
    case PREFIX(GENERIC):  return "Error (generic)";
    case PREFIX(prefix_unknown): return "Unknown frame descriptor";
    case PREFIX(version_unsupported): return "Version not supported";
    case PREFIX(frameParameter_unsupported): return "Unsupported frame parameter";
    case PREFIX(frameParameter_windowTooLarge): return "Frame requires too much memory for decoding";
    case PREFIX(corruption_detected): return "Data corruption detected";
    case PREFIX(checksum_wrong): return "Restored data doesn't match checksum";
    case PREFIX(literals_headerWrong): return "Header of Literals' block doesn't respect format specification";
    case PREFIX(parameter_unsupported): return "Unsupported parameter";
    case PREFIX(parameter_combination_unsupported): return "Unsupported combination of parameters";
    case PREFIX(parameter_outOfBound): return "Parameter is out of bound";
    case PREFIX(init_missing): return "Context should be init first";
    case PREFIX(memory_allocation): return "Allocation error : not enough memory";
    case PREFIX(workSpace_tooSmall): return "workSpace buffer is not large enough";
    case PREFIX(stage_wrong): return "Operation not authorized at current processing stage";
    case PREFIX(tableLog_tooLarge): return "tableLog requires too much memory : unsupported";
    case PREFIX(maxSymbolValue_tooLarge): return "Unsupported max Symbol Value : too large";
    case PREFIX(maxSymbolValue_tooSmall): return "Specified maxSymbolValue is too small";
    case PREFIX(cannotProduce_uncompressedBlock): return "This mode cannot generate an uncompressed block";
    case PREFIX(stabilityCondition_notRespected): return "pledged buffer stability condition is not respected";
    case PREFIX(dictionary_corrupted): return "Dictionary is corrupted";
    case PREFIX(dictionary_wrong): return "Dictionary mismatch";
    case PREFIX(dictionaryCreation_failed): return "Cannot create Dictionary from provided samples";
    case PREFIX(dstSize_tooSmall): return "Destination buffer is too small";
    case PREFIX(srcSize_wrong): return "Src size is incorrect";
    case PREFIX(dstBuffer_null): return "Operation on NULL destination buffer";
    case PREFIX(noForwardProgress_destFull): return "Operation made no progress over multiple calls, due to output buffer being full";
    case PREFIX(noForwardProgress_inputEmpty): return "Operation made no progress over multiple calls, due to input being empty";

    case PREFIX(frameIndex_tooLarge): return "Frame index is too large";
    case PREFIX(seekableIO): return "An I/O error occurred when reading/seeking";
    case PREFIX(dstBuffer_wrong): return "Destination buffer is wrong";
    case PREFIX(srcBuffer_wrong): return "Source buffer is wrong";
    case PREFIX(sequenceProducer_failed): return "Block-level external sequence producer returned an error code";
    case PREFIX(externalSequences_invalid): return "External sequences are not valid";
    case PREFIX(maxCode):
    default: return notErrorCode;
    }
#endif
}


#define FSE_STATIC_LINKING_ONLY


#define FSE_isError ERR_isError
#define FSE_STATIC_ASSERT(c) DEBUG_STATIC_ASSERT(c)


#ifndef FSE_FUNCTION_EXTENSION
#  error "FSE_FUNCTION_EXTENSION must be defined"
#endif
#ifndef FSE_FUNCTION_TYPE
#  error "FSE_FUNCTION_TYPE must be defined"
#endif


#define FSE_CAT(X,Y) X##Y
#define FSE_FUNCTION_NAME(X,Y) FSE_CAT(X,Y)
#define FSE_TYPE_NAME(X,Y) FSE_CAT(X,Y)

static size_t FSE_buildDTable_internal(FSE_DTable* dt, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog, void* workSpace, size_t wkspSize)
{
    void* const tdPtr = dt+1;
    FSE_DECODE_TYPE* const tableDecode = (FSE_DECODE_TYPE*) (tdPtr);
    U16* symbolNext = (U16*)workSpace;
    BYTE* spread = (BYTE*)(symbolNext + maxSymbolValue + 1);

    U32 const maxSV1 = maxSymbolValue + 1;
    U32 const tableSize = 1 << tableLog;
    U32 highThreshold = tableSize-1;


    if (FSE_BUILD_DTABLE_WKSP_SIZE(tableLog, maxSymbolValue) > wkspSize) return ERROR(maxSymbolValue_tooLarge);
    if (maxSymbolValue > FSE_MAX_SYMBOL_VALUE) return ERROR(maxSymbolValue_tooLarge);
    if (tableLog > FSE_MAX_TABLELOG) return ERROR(tableLog_tooLarge);


    {   FSE_DTableHeader DTableH;
        DTableH.tableLog = (U16)tableLog;
        DTableH.fastMode = 1;
        {   S16 const largeLimit= (S16)(1 << (tableLog-1));
            U32 s;
            for (s=0; s<maxSV1; s++) {
                if (normalizedCounter[s]==-1) {
                    tableDecode[highThreshold--].symbol = (FSE_FUNCTION_TYPE)s;
                    symbolNext[s] = 1;
                } else {
                    if (normalizedCounter[s] >= largeLimit) DTableH.fastMode=0;
                    symbolNext[s] = (U16)normalizedCounter[s];
        }   }   }
        ZSTD_memcpy(dt, &DTableH, sizeof(DTableH));
    }


    if (highThreshold == tableSize - 1) {
        size_t const tableMask = tableSize-1;
        size_t const step = FSE_TABLESTEP(tableSize);

        {   U64 const add = 0x0101010101010101ull;
            size_t pos = 0;
            U64 sv = 0;
            U32 s;
            for (s=0; s<maxSV1; ++s, sv += add) {
                int i;
                int const n = normalizedCounter[s];
                MEM_write64(spread + pos, sv);
                for (i = 8; i < n; i += 8) {
                    MEM_write64(spread + pos + i, sv);
                }
                pos += (size_t)n;
        }   }

        {
            size_t position = 0;
            size_t s;
            size_t const unroll = 2;
            assert(tableSize % unroll == 0);
            for (s = 0; s < (size_t)tableSize; s += unroll) {
                size_t u;
                for (u = 0; u < unroll; ++u) {
                    size_t const uPosition = (position + (u * step)) & tableMask;
                    tableDecode[uPosition].symbol = spread[s + u];
                }
                position = (position + (unroll * step)) & tableMask;
            }
            assert(position == 0);
        }
    } else {
        U32 const tableMask = tableSize-1;
        U32 const step = FSE_TABLESTEP(tableSize);
        U32 s, position = 0;
        for (s=0; s<maxSV1; s++) {
            int i;
            for (i=0; i<normalizedCounter[s]; i++) {
                tableDecode[position].symbol = (FSE_FUNCTION_TYPE)s;
                position = (position + step) & tableMask;
                while (position > highThreshold) position = (position + step) & tableMask;
        }   }
        if (position!=0) return ERROR(GENERIC);
    }


    {   U32 u;
        for (u=0; u<tableSize; u++) {
            FSE_FUNCTION_TYPE const symbol = (FSE_FUNCTION_TYPE)(tableDecode[u].symbol);
            U32 const nextState = symbolNext[symbol]++;
            tableDecode[u].nbBits = (BYTE) (tableLog - ZSTD_highbit32(nextState) );
            tableDecode[u].newState = (U16) ( (nextState << tableDecode[u].nbBits) - tableSize);
    }   }

    return 0;
}

size_t FSE_buildDTable_wksp(FSE_DTable* dt, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog, void* workSpace, size_t wkspSize)
{
    return FSE_buildDTable_internal(dt, normalizedCounter, maxSymbolValue, tableLog, workSpace, wkspSize);
}


#ifndef FSE_COMMONDEFS_ONLY


FORCE_INLINE_TEMPLATE size_t FSE_decompress_usingDTable_generic(
          void* dst, size_t maxDstSize,
    const void* cSrc, size_t cSrcSize,
    const FSE_DTable* dt, const unsigned fast)
{
    BYTE* const ostart = (BYTE*) dst;
    BYTE* op = ostart;
    BYTE* const omax = op + maxDstSize;
    BYTE* const olimit = omax-3;

    BIT_DStream_t bitD;
    FSE_DState_t state1;
    FSE_DState_t state2;


    CHECK_F(BIT_initDStream(&bitD, cSrc, cSrcSize));

    FSE_initDState(&state1, &bitD, dt);
    FSE_initDState(&state2, &bitD, dt);

    RETURN_ERROR_IF(BIT_reloadDStream(&bitD)==BIT_DStream_overflow, corruption_detected, "");

#define FSE_GETSYMBOL(statePtr) fast ? FSE_decodeSymbolFast(statePtr, &bitD) : FSE_decodeSymbol(statePtr, &bitD)


    for ( ; (BIT_reloadDStream(&bitD)==BIT_DStream_unfinished) & (op<olimit) ; op+=4) {
        op[0] = FSE_GETSYMBOL(&state1);

        if (FSE_MAX_TABLELOG*2+7 > sizeof(bitD.bitContainer)*8)
            BIT_reloadDStream(&bitD);

        op[1] = FSE_GETSYMBOL(&state2);

        if (FSE_MAX_TABLELOG*4+7 > sizeof(bitD.bitContainer)*8)
            { if (BIT_reloadDStream(&bitD) > BIT_DStream_unfinished) { op+=2; break; } }

        op[2] = FSE_GETSYMBOL(&state1);

        if (FSE_MAX_TABLELOG*2+7 > sizeof(bitD.bitContainer)*8)
            BIT_reloadDStream(&bitD);

        op[3] = FSE_GETSYMBOL(&state2);
    }


    while (1) {
        if (op>(omax-2)) return ERROR(dstSize_tooSmall);
        *op++ = FSE_GETSYMBOL(&state1);
        if (BIT_reloadDStream(&bitD)==BIT_DStream_overflow) {
            *op++ = FSE_GETSYMBOL(&state2);
            break;
        }

        if (op>(omax-2)) return ERROR(dstSize_tooSmall);
        *op++ = FSE_GETSYMBOL(&state2);
        if (BIT_reloadDStream(&bitD)==BIT_DStream_overflow) {
            *op++ = FSE_GETSYMBOL(&state1);
            break;
    }   }

    assert(op >= ostart);
    return (size_t)(op-ostart);
}

typedef struct {
    short ncount[FSE_MAX_SYMBOL_VALUE + 1];
} FSE_DecompressWksp;


FORCE_INLINE_TEMPLATE size_t FSE_decompress_wksp_body(
        void* dst, size_t dstCapacity,
        const void* cSrc, size_t cSrcSize,
        unsigned maxLog, void* workSpace, size_t wkspSize,
        int bmi2)
{
    const BYTE* const istart = (const BYTE*)cSrc;
    const BYTE* ip = istart;
    unsigned tableLog;
    unsigned maxSymbolValue = FSE_MAX_SYMBOL_VALUE;
    FSE_DecompressWksp* const wksp = (FSE_DecompressWksp*)workSpace;
    size_t const dtablePos = sizeof(FSE_DecompressWksp) / sizeof(FSE_DTable);
    FSE_DTable* const dtable = (FSE_DTable*)workSpace + dtablePos;

    FSE_STATIC_ASSERT((FSE_MAX_SYMBOL_VALUE + 1) % 2 == 0);
    if (wkspSize < sizeof(*wksp)) return ERROR(GENERIC);


    FSE_STATIC_ASSERT(sizeof(FSE_DecompressWksp) % sizeof(FSE_DTable) == 0);


    {   size_t const NCountLength =
            FSE_readNCount_bmi2(wksp->ncount, &maxSymbolValue, &tableLog, istart, cSrcSize, bmi2);
        if (FSE_isError(NCountLength)) return NCountLength;
        if (tableLog > maxLog) return ERROR(tableLog_tooLarge);
        assert(NCountLength <= cSrcSize);
        ip += NCountLength;
        cSrcSize -= NCountLength;
    }

    if (FSE_DECOMPRESS_WKSP_SIZE(tableLog, maxSymbolValue) > wkspSize) return ERROR(tableLog_tooLarge);
    assert(sizeof(*wksp) + FSE_DTABLE_SIZE(tableLog) <= wkspSize);
    workSpace = (BYTE*)workSpace + sizeof(*wksp) + FSE_DTABLE_SIZE(tableLog);
    wkspSize -= sizeof(*wksp) + FSE_DTABLE_SIZE(tableLog);

    CHECK_F( FSE_buildDTable_internal(dtable, wksp->ncount, maxSymbolValue, tableLog, workSpace, wkspSize) );

    {
        const void* ptr = dtable;
        const FSE_DTableHeader* DTableH = (const FSE_DTableHeader*)ptr;
        const U32 fastMode = DTableH->fastMode;


        if (fastMode) return FSE_decompress_usingDTable_generic(dst, dstCapacity, ip, cSrcSize, dtable, 1);
        return FSE_decompress_usingDTable_generic(dst, dstCapacity, ip, cSrcSize, dtable, 0);
    }
}


static size_t FSE_decompress_wksp_body_default(void* dst, size_t dstCapacity, const void* cSrc, size_t cSrcSize, unsigned maxLog, void* workSpace, size_t wkspSize)
{
    return FSE_decompress_wksp_body(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize, 0);
}

#if DYNAMIC_BMI2
BMI2_TARGET_ATTRIBUTE static size_t FSE_decompress_wksp_body_bmi2(void* dst, size_t dstCapacity, const void* cSrc, size_t cSrcSize, unsigned maxLog, void* workSpace, size_t wkspSize)
{
    return FSE_decompress_wksp_body(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize, 1);
}
#endif

size_t FSE_decompress_wksp_bmi2(void* dst, size_t dstCapacity, const void* cSrc, size_t cSrcSize, unsigned maxLog, void* workSpace, size_t wkspSize, int bmi2)
{
#if DYNAMIC_BMI2
    if (bmi2) {
        return FSE_decompress_wksp_body_bmi2(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize);
    }
#endif
    (void)bmi2;
    return FSE_decompress_wksp_body_default(dst, dstCapacity, cSrc, cSrcSize, maxLog, workSpace, wkspSize);
}

#endif


#define ZSTD_DEPS_NEED_MALLOC


#ifndef ZSTD_CCOMMON_H_MODULE
#define ZSTD_CCOMMON_H_MODULE


#ifndef ZSTD_COMMON_CPU_H
#define ZSTD_COMMON_CPU_H


#ifdef _MSC_VER
#include <intrin.h>
#endif

typedef struct {
    U32 f1c;
    U32 f1d;
    U32 f7b;
    U32 f7c;
} ZSTD_cpuid_t;

MEM_STATIC ZSTD_cpuid_t ZSTD_cpuid(void) {
    U32 f1c = 0;
    U32 f1d = 0;
    U32 f7b = 0;
    U32 f7c = 0;
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#if !defined(_M_X64) || !defined(__clang__) || __clang_major__ >= 16
    int reg[4];
    __cpuid((int*)reg, 0);
    {
        int const n = reg[0];
        if (n >= 1) {
            __cpuid((int*)reg, 1);
            f1c = (U32)reg[2];
            f1d = (U32)reg[3];
        }
        if (n >= 7) {
            __cpuidex((int*)reg, 7, 0);
            f7b = (U32)reg[1];
            f7c = (U32)reg[2];
        }
    }
#else

    U32 n;
    __asm__(
        "pushq %%rbx\n\t"
        "cpuid\n\t"
        "popq %%rbx\n\t"
        : "=a"(n)
        : "a"(0)
        : "rcx", "rdx");
    if (n >= 1) {
      U32 f1a;
      __asm__(
          "pushq %%rbx\n\t"
          "cpuid\n\t"
          "popq %%rbx\n\t"
          : "=a"(f1a), "=c"(f1c), "=d"(f1d)
          : "a"(1)
          :);
    }
    if (n >= 7) {
      __asm__(
          "pushq %%rbx\n\t"
          "cpuid\n\t"
          "movq %%rbx, %%rax\n\t"
          "popq %%rbx"
          : "=a"(f7b), "=c"(f7c)
          : "a"(7), "c"(0)
          : "rdx");
    }
#endif
#elif defined(__i386__) && defined(__PIC__) && !defined(__clang__) && defined(__GNUC__)

    U32 n;
    __asm__(
        "pushl %%ebx\n\t"
        "cpuid\n\t"
        "popl %%ebx\n\t"
        : "=a"(n)
        : "a"(0)
        : "ecx", "edx");
    if (n >= 1) {
      U32 f1a;
      __asm__(
          "pushl %%ebx\n\t"
          "cpuid\n\t"
          "popl %%ebx\n\t"
          : "=a"(f1a), "=c"(f1c), "=d"(f1d)
          : "a"(1));
    }
    if (n >= 7) {
      __asm__(
          "pushl %%ebx\n\t"
          "cpuid\n\t"
          "movl %%ebx, %%eax\n\t"
          "popl %%ebx"
          : "=a"(f7b), "=c"(f7c)
          : "a"(7), "c"(0)
          : "edx");
    }
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
    U32 n;
    __asm__("cpuid" : "=a"(n) : "a"(0) : "ebx", "ecx", "edx");
    if (n >= 1) {
      U32 f1a;
      __asm__("cpuid" : "=a"(f1a), "=c"(f1c), "=d"(f1d) : "a"(1) : "ebx");
    }
    if (n >= 7) {
      U32 f7a;
      __asm__("cpuid"
              : "=a"(f7a), "=b"(f7b), "=c"(f7c)
              : "a"(7), "c"(0)
              : "edx");
    }
#endif
    {
        ZSTD_cpuid_t cpuid;
        cpuid.f1c = f1c;
        cpuid.f1d = f1d;
        cpuid.f7b = f7b;
        cpuid.f7c = f7c;
        return cpuid;
    }
}

#define X(name, r, bit)                                                        \
  MEM_STATIC int ZSTD_cpuid_##name(ZSTD_cpuid_t const cpuid) {                 \
    return ((cpuid.r) & (1U << bit)) != 0;                                     \
  }


#define C(name, bit) X(name, f1c, bit)
  C(sse3, 0)
  C(pclmuldq, 1)
  C(dtes64, 2)
  C(monitor, 3)
  C(dscpl, 4)
  C(vmx, 5)
  C(smx, 6)
  C(eist, 7)
  C(tm2, 8)
  C(ssse3, 9)
  C(cnxtid, 10)
  C(fma, 12)
  C(cx16, 13)
  C(xtpr, 14)
  C(pdcm, 15)
  C(pcid, 17)
  C(dca, 18)
  C(sse41, 19)
  C(sse42, 20)
  C(x2apic, 21)
  C(movbe, 22)
  C(popcnt, 23)
  C(tscdeadline, 24)
  C(aes, 25)
  C(xsave, 26)
  C(osxsave, 27)
  C(avx, 28)
  C(f16c, 29)
  C(rdrand, 30)
#undef C
#define D(name, bit) X(name, f1d, bit)
  D(fpu, 0)
  D(vme, 1)
  D(de, 2)
  D(pse, 3)
  D(tsc, 4)
  D(msr, 5)
  D(pae, 6)
  D(mce, 7)
  D(cx8, 8)
  D(apic, 9)
  D(sep, 11)
  D(mtrr, 12)
  D(pge, 13)
  D(mca, 14)
  D(cmov, 15)
  D(pat, 16)
  D(pse36, 17)
  D(psn, 18)
  D(clfsh, 19)
  D(ds, 21)
  D(acpi, 22)
  D(mmx, 23)
  D(fxsr, 24)
  D(sse, 25)
  D(sse2, 26)
  D(ss, 27)
  D(htt, 28)
  D(tm, 29)
  D(pbe, 31)
#undef D


#define B(name, bit) X(name, f7b, bit)
  B(bmi1, 3)
  B(hle, 4)
  B(avx2, 5)
  B(smep, 7)
  B(bmi2, 8)
  B(erms, 9)
  B(invpcid, 10)
  B(rtm, 11)
  B(mpx, 14)
  B(avx512f, 16)
  B(avx512dq, 17)
  B(rdseed, 18)
  B(adx, 19)
  B(smap, 20)
  B(avx512ifma, 21)
  B(pcommit, 22)
  B(clflushopt, 23)
  B(clwb, 24)
  B(avx512pf, 26)
  B(avx512er, 27)
  B(avx512cd, 28)
  B(sha, 29)
  B(avx512bw, 30)
  B(avx512vl, 31)
#undef B
#define C(name, bit) X(name, f7c, bit)
  C(prefetchwt1, 0)
  C(avx512vbmi, 1)
#undef C

#undef X

#endif


#define ZSTD_STATIC_LINKING_ONLY


#ifndef ZSTD_H_235446
#define ZSTD_H_235446


#include <stddef.h>


#if defined(ZSTD_STATIC_LINKING_ONLY) && !defined(ZSTD_H_ZSTD_STATIC_LINKING_ONLY)
#include <limits.h>
#endif

#if defined (__cplusplus)
extern "C" {
#endif


#ifndef ZSTDLIB_VISIBLE

#  ifdef ZSTDLIB_VISIBILITY
#    define ZSTDLIB_VISIBLE ZSTDLIB_VISIBILITY
#  elif defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__MINGW32__)
#    define ZSTDLIB_VISIBLE __attribute__ ((visibility ("default")))
#  else
#    define ZSTDLIB_VISIBLE
#  endif
#endif

#ifndef ZSTDLIB_HIDDEN
#  if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__MINGW32__)
#    define ZSTDLIB_HIDDEN __attribute__ ((visibility ("hidden")))
#  else
#    define ZSTDLIB_HIDDEN
#  endif
#endif

#if defined(ZSTD_DLL_EXPORT) && (ZSTD_DLL_EXPORT==1)
#  define ZSTDLIB_API __declspec(dllexport) ZSTDLIB_VISIBLE
#elif defined(ZSTD_DLL_IMPORT) && (ZSTD_DLL_IMPORT==1)
#  define ZSTDLIB_API __declspec(dllimport) ZSTDLIB_VISIBLE
#else
#  define ZSTDLIB_API ZSTDLIB_VISIBLE
#endif


#ifdef ZSTD_DISABLE_DEPRECATE_WARNINGS
#  define ZSTD_DEPRECATED(message)
#else
#  if defined (__cplusplus) && (__cplusplus >= 201402)
#    define ZSTD_DEPRECATED(message) [[deprecated(message)]]
#  elif (defined(GNUC) && (GNUC > 4 || (GNUC == 4 && GNUC_MINOR >= 5))) || defined(__clang__) || defined(__IAR_SYSTEMS_ICC__)
#    define ZSTD_DEPRECATED(message) __attribute__((deprecated(message)))
#  elif defined(__GNUC__) && (__GNUC__ >= 3)
#    define ZSTD_DEPRECATED(message) __attribute__((deprecated))
#  elif defined(_MSC_VER)
#    define ZSTD_DEPRECATED(message) __declspec(deprecated(message))
#  else
#    pragma message("WARNING: You need to implement ZSTD_DEPRECATED for this compiler")
#    define ZSTD_DEPRECATED(message)
#  endif
#endif


#define ZSTD_VERSION_MAJOR    1
#define ZSTD_VERSION_MINOR    6
#define ZSTD_VERSION_RELEASE  0
#define ZSTD_VERSION_NUMBER  (ZSTD_VERSION_MAJOR *100*100 + ZSTD_VERSION_MINOR *100 + ZSTD_VERSION_RELEASE)


ZSTDLIB_API unsigned ZSTD_versionNumber(void);

#define ZSTD_LIB_VERSION ZSTD_VERSION_MAJOR.ZSTD_VERSION_MINOR.ZSTD_VERSION_RELEASE
#define ZSTD_QUOTE(str) #str
#define ZSTD_EXPAND_AND_QUOTE(str) ZSTD_QUOTE(str)
#define ZSTD_VERSION_STRING ZSTD_EXPAND_AND_QUOTE(ZSTD_LIB_VERSION)


ZSTDLIB_API const char* ZSTD_versionString(void);


#ifndef ZSTD_CLEVEL_DEFAULT
#  define ZSTD_CLEVEL_DEFAULT 3
#endif


#define ZSTD_MAGICNUMBER            0xFD2FB528
#define ZSTD_MAGIC_DICTIONARY       0xEC30A437
#define ZSTD_MAGIC_SKIPPABLE_START  0x184D2A50
#define ZSTD_MAGIC_SKIPPABLE_MASK   0xFFFFFFF0

#define ZSTD_BLOCKSIZELOG_MAX  17
#define ZSTD_BLOCKSIZE_MAX     (1<<ZSTD_BLOCKSIZELOG_MAX)


ZSTDLIB_API size_t ZSTD_compress( void* dst, size_t dstCapacity,
                            const void* src, size_t srcSize,
                                  int compressionLevel);


ZSTDLIB_API size_t ZSTD_decompress( void* dst, size_t dstCapacity,
                              const void* src, size_t compressedSize);


#define ZSTD_CONTENTSIZE_UNKNOWN (0ULL - 1)
#define ZSTD_CONTENTSIZE_ERROR   (0ULL - 2)
ZSTDLIB_API unsigned long long ZSTD_getFrameContentSize(const void *src, size_t srcSize);


ZSTD_DEPRECATED("Replaced by ZSTD_getFrameContentSize")
ZSTDLIB_API unsigned long long ZSTD_getDecompressedSize(const void* src, size_t srcSize);


ZSTDLIB_API size_t ZSTD_findFrameCompressedSize(const void* src, size_t srcSize);


#define ZSTD_MAX_INPUT_SIZE ((sizeof(size_t)==8) ? 0xFF00FF00FF00FF00ULL : 0xFF00FF00U)
#define ZSTD_COMPRESSBOUND(srcSize)   (((size_t)(srcSize) >= ZSTD_MAX_INPUT_SIZE) ? 0 : (srcSize) + ((srcSize)>>8) + (((srcSize) < (128<<10)) ? (((128<<10) - (srcSize)) >> 11)  : 0))
ZSTDLIB_API size_t ZSTD_compressBound(size_t srcSize);


ZSTDLIB_API unsigned     ZSTD_isError(size_t result);
ZSTDLIB_API ZSTD_ErrorCode ZSTD_getErrorCode(size_t functionResult);
ZSTDLIB_API const char*  ZSTD_getErrorName(size_t result);
ZSTDLIB_API int          ZSTD_minCLevel(void);
ZSTDLIB_API int          ZSTD_maxCLevel(void);
ZSTDLIB_API int          ZSTD_defaultCLevel(void);


typedef struct ZSTD_CCtx_s ZSTD_CCtx;
ZSTDLIB_API ZSTD_CCtx* ZSTD_createCCtx(void);
ZSTDLIB_API size_t     ZSTD_freeCCtx(ZSTD_CCtx* cctx);


ZSTDLIB_API size_t ZSTD_compressCCtx(ZSTD_CCtx* cctx,
                                     void* dst, size_t dstCapacity,
                               const void* src, size_t srcSize,
                                     int compressionLevel);


typedef struct ZSTD_DCtx_s ZSTD_DCtx;
ZSTDLIB_API ZSTD_DCtx* ZSTD_createDCtx(void);
ZSTDLIB_API size_t     ZSTD_freeDCtx(ZSTD_DCtx* dctx);


ZSTDLIB_API size_t ZSTD_decompressDCtx(ZSTD_DCtx* dctx,
                                       void* dst, size_t dstCapacity,
                                 const void* src, size_t srcSize);


typedef enum { ZSTD_fast=1,
               ZSTD_dfast=2,
               ZSTD_greedy=3,
               ZSTD_lazy=4,
               ZSTD_lazy2=5,
               ZSTD_btlazy2=6,
               ZSTD_btopt=7,
               ZSTD_btultra=8,
               ZSTD_btultra2=9

} ZSTD_strategy;

typedef enum {


    ZSTD_c_compressionLevel=100,

    ZSTD_c_windowLog=101,
    ZSTD_c_hashLog=102,
    ZSTD_c_chainLog=103,
    ZSTD_c_searchLog=104,
    ZSTD_c_minMatch=105,
    ZSTD_c_targetLength=106,
    ZSTD_c_strategy=107,

    ZSTD_c_targetCBlockSize=130,

    ZSTD_c_enableLongDistanceMatching=160,
    ZSTD_c_ldmHashLog=161,
    ZSTD_c_ldmMinMatch=162,
    ZSTD_c_ldmBucketSizeLog=163,
    ZSTD_c_ldmHashRateLog=164,


    ZSTD_c_contentSizeFlag=200,
    ZSTD_c_checksumFlag=201,
    ZSTD_c_dictIDFlag=202,


    ZSTD_c_nbWorkers=400,
    ZSTD_c_jobSize=401,
    ZSTD_c_overlapLog=402,


     ZSTD_c_experimentalParam1=500,
     ZSTD_c_experimentalParam2=10,
     ZSTD_c_experimentalParam3=1000,
     ZSTD_c_experimentalParam4=1001,
     ZSTD_c_experimentalParam5=1002,

     ZSTD_c_experimentalParam7=1004,
     ZSTD_c_experimentalParam8=1005,
     ZSTD_c_experimentalParam9=1006,
     ZSTD_c_experimentalParam10=1007,
     ZSTD_c_experimentalParam11=1008,
     ZSTD_c_experimentalParam12=1009,
     ZSTD_c_experimentalParam13=1010,
     ZSTD_c_experimentalParam14=1011,
     ZSTD_c_experimentalParam15=1012,
     ZSTD_c_experimentalParam16=1013,
     ZSTD_c_experimentalParam17=1014,
     ZSTD_c_experimentalParam18=1015,
     ZSTD_c_experimentalParam19=1016,
     ZSTD_c_experimentalParam20=1017
} ZSTD_cParameter;

typedef struct {
    size_t error;
    int lowerBound;
    int upperBound;
} ZSTD_bounds;


ZSTDLIB_API ZSTD_bounds ZSTD_cParam_getBounds(ZSTD_cParameter cParam);


ZSTDLIB_API size_t ZSTD_CCtx_setParameter(ZSTD_CCtx* cctx, ZSTD_cParameter param, int value);


ZSTDLIB_API size_t ZSTD_CCtx_setPledgedSrcSize(ZSTD_CCtx* cctx, unsigned long long pledgedSrcSize);

typedef enum {
    ZSTD_reset_session_only = 1,
    ZSTD_reset_parameters = 2,
    ZSTD_reset_session_and_parameters = 3
} ZSTD_ResetDirective;


ZSTDLIB_API size_t ZSTD_CCtx_reset(ZSTD_CCtx* cctx, ZSTD_ResetDirective reset);


ZSTDLIB_API size_t ZSTD_compress2( ZSTD_CCtx* cctx,
                                   void* dst, size_t dstCapacity,
                             const void* src, size_t srcSize);


typedef enum {

    ZSTD_d_windowLogMax=100,


     ZSTD_d_experimentalParam1=1000,
     ZSTD_d_experimentalParam2=1001,
     ZSTD_d_experimentalParam3=1002,
     ZSTD_d_experimentalParam4=1003,
     ZSTD_d_experimentalParam5=1004,
     ZSTD_d_experimentalParam6=1005

} ZSTD_dParameter;


ZSTDLIB_API ZSTD_bounds ZSTD_dParam_getBounds(ZSTD_dParameter dParam);


ZSTDLIB_API size_t ZSTD_DCtx_setParameter(ZSTD_DCtx* dctx, ZSTD_dParameter param, int value);


ZSTDLIB_API size_t ZSTD_DCtx_reset(ZSTD_DCtx* dctx, ZSTD_ResetDirective reset);


typedef struct ZSTD_inBuffer_s {
  const void* src;
  size_t size;
  size_t pos;
} ZSTD_inBuffer;

typedef struct ZSTD_outBuffer_s {
  void*  dst;
  size_t size;
  size_t pos;
} ZSTD_outBuffer;


typedef ZSTD_CCtx ZSTD_CStream;


ZSTDLIB_API ZSTD_CStream* ZSTD_createCStream(void);
ZSTDLIB_API size_t ZSTD_freeCStream(ZSTD_CStream* zcs);


typedef enum {
    ZSTD_e_continue=0,
    ZSTD_e_flush=1,
    ZSTD_e_end=2
} ZSTD_EndDirective;


ZSTDLIB_API size_t ZSTD_compressStream2( ZSTD_CCtx* cctx,
                                         ZSTD_outBuffer* output,
                                         ZSTD_inBuffer* input,
                                         ZSTD_EndDirective endOp);


ZSTDLIB_API size_t ZSTD_CStreamInSize(void);
ZSTDLIB_API size_t ZSTD_CStreamOutSize(void);


ZSTDLIB_API size_t ZSTD_initCStream(ZSTD_CStream* zcs, int compressionLevel);

ZSTDLIB_API size_t ZSTD_compressStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output, ZSTD_inBuffer* input);

ZSTDLIB_API size_t ZSTD_flushStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output);

ZSTDLIB_API size_t ZSTD_endStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output);


typedef ZSTD_DCtx ZSTD_DStream;


ZSTDLIB_API ZSTD_DStream* ZSTD_createDStream(void);
ZSTDLIB_API size_t ZSTD_freeDStream(ZSTD_DStream* zds);


ZSTDLIB_API size_t ZSTD_initDStream(ZSTD_DStream* zds);


ZSTDLIB_API size_t ZSTD_decompressStream(ZSTD_DStream* zds, ZSTD_outBuffer* output, ZSTD_inBuffer* input);

ZSTDLIB_API size_t ZSTD_DStreamInSize(void);
ZSTDLIB_API size_t ZSTD_DStreamOutSize(void);


ZSTDLIB_API size_t ZSTD_compress_usingDict(ZSTD_CCtx* ctx,
                                           void* dst, size_t dstCapacity,
                                     const void* src, size_t srcSize,
                                     const void* dict,size_t dictSize,
                                           int compressionLevel);


ZSTDLIB_API size_t ZSTD_decompress_usingDict(ZSTD_DCtx* dctx,
                                             void* dst, size_t dstCapacity,
                                       const void* src, size_t srcSize,
                                       const void* dict,size_t dictSize);


typedef struct ZSTD_CDict_s ZSTD_CDict;


ZSTDLIB_API ZSTD_CDict* ZSTD_createCDict(const void* dictBuffer, size_t dictSize,
                                         int compressionLevel);


ZSTDLIB_API size_t      ZSTD_freeCDict(ZSTD_CDict* CDict);


ZSTDLIB_API size_t ZSTD_compress_usingCDict(ZSTD_CCtx* cctx,
                                            void* dst, size_t dstCapacity,
                                      const void* src, size_t srcSize,
                                      const ZSTD_CDict* cdict);


typedef struct ZSTD_DDict_s ZSTD_DDict;


ZSTDLIB_API ZSTD_DDict* ZSTD_createDDict(const void* dictBuffer, size_t dictSize);


ZSTDLIB_API size_t      ZSTD_freeDDict(ZSTD_DDict* ddict);


ZSTDLIB_API size_t ZSTD_decompress_usingDDict(ZSTD_DCtx* dctx,
                                              void* dst, size_t dstCapacity,
                                        const void* src, size_t srcSize,
                                        const ZSTD_DDict* ddict);


ZSTDLIB_API unsigned ZSTD_getDictID_fromDict(const void* dict, size_t dictSize);


ZSTDLIB_API unsigned ZSTD_getDictID_fromCDict(const ZSTD_CDict* cdict);


ZSTDLIB_API unsigned ZSTD_getDictID_fromDDict(const ZSTD_DDict* ddict);


ZSTDLIB_API unsigned ZSTD_getDictID_fromFrame(const void* src, size_t srcSize);


ZSTDLIB_API size_t ZSTD_CCtx_loadDictionary(ZSTD_CCtx* cctx, const void* dict, size_t dictSize);


ZSTDLIB_API size_t ZSTD_CCtx_refCDict(ZSTD_CCtx* cctx, const ZSTD_CDict* cdict);


ZSTDLIB_API size_t ZSTD_CCtx_refPrefix(ZSTD_CCtx* cctx,
                                 const void* prefix, size_t prefixSize);


ZSTDLIB_API size_t ZSTD_DCtx_loadDictionary(ZSTD_DCtx* dctx, const void* dict, size_t dictSize);


ZSTDLIB_API size_t ZSTD_DCtx_refDDict(ZSTD_DCtx* dctx, const ZSTD_DDict* ddict);


ZSTDLIB_API size_t ZSTD_DCtx_refPrefix(ZSTD_DCtx* dctx,
                                 const void* prefix, size_t prefixSize);


ZSTDLIB_API size_t ZSTD_sizeof_CCtx(const ZSTD_CCtx* cctx);
ZSTDLIB_API size_t ZSTD_sizeof_DCtx(const ZSTD_DCtx* dctx);
ZSTDLIB_API size_t ZSTD_sizeof_CStream(const ZSTD_CStream* zcs);
ZSTDLIB_API size_t ZSTD_sizeof_DStream(const ZSTD_DStream* zds);
ZSTDLIB_API size_t ZSTD_sizeof_CDict(const ZSTD_CDict* cdict);
ZSTDLIB_API size_t ZSTD_sizeof_DDict(const ZSTD_DDict* ddict);

#if defined (__cplusplus)
}
#endif

#endif


#if defined(ZSTD_STATIC_LINKING_ONLY) && !defined(ZSTD_H_ZSTD_STATIC_LINKING_ONLY)
#define ZSTD_H_ZSTD_STATIC_LINKING_ONLY

#if defined (__cplusplus)
extern "C" {
#endif


#ifndef ZSTDLIB_STATIC_API
#  if defined(ZSTD_DLL_EXPORT) && (ZSTD_DLL_EXPORT==1)
#    define ZSTDLIB_STATIC_API __declspec(dllexport) ZSTDLIB_VISIBLE
#  elif defined(ZSTD_DLL_IMPORT) && (ZSTD_DLL_IMPORT==1)
#    define ZSTDLIB_STATIC_API __declspec(dllimport) ZSTDLIB_VISIBLE
#  else
#    define ZSTDLIB_STATIC_API ZSTDLIB_VISIBLE
#  endif
#endif


#define ZSTD_FRAMEHEADERSIZE_PREFIX(format) ((format) == ZSTD_f_zstd1 ? 5 : 1)
#define ZSTD_FRAMEHEADERSIZE_MIN(format)    ((format) == ZSTD_f_zstd1 ? 6 : 2)
#define ZSTD_FRAMEHEADERSIZE_MAX   18
#define ZSTD_SKIPPABLEHEADERSIZE    8


#define ZSTD_WINDOWLOG_MAX_32    30
#define ZSTD_WINDOWLOG_MAX_64    31
#define ZSTD_WINDOWLOG_MAX     ((int)(sizeof(size_t) == 4 ? ZSTD_WINDOWLOG_MAX_32 : ZSTD_WINDOWLOG_MAX_64))
#define ZSTD_WINDOWLOG_MIN       10
#define ZSTD_HASHLOG_MAX       ((ZSTD_WINDOWLOG_MAX < 30) ? ZSTD_WINDOWLOG_MAX : 30)
#define ZSTD_HASHLOG_MIN          6
#define ZSTD_CHAINLOG_MAX_32     29
#define ZSTD_CHAINLOG_MAX_64     30
#define ZSTD_CHAINLOG_MAX      ((int)(sizeof(size_t) == 4 ? ZSTD_CHAINLOG_MAX_32 : ZSTD_CHAINLOG_MAX_64))
#define ZSTD_CHAINLOG_MIN        ZSTD_HASHLOG_MIN
#define ZSTD_SEARCHLOG_MAX      (ZSTD_WINDOWLOG_MAX-1)
#define ZSTD_SEARCHLOG_MIN        1
#define ZSTD_MINMATCH_MAX         7
#define ZSTD_MINMATCH_MIN         3
#define ZSTD_TARGETLENGTH_MAX    ZSTD_BLOCKSIZE_MAX
#define ZSTD_TARGETLENGTH_MIN     0
#define ZSTD_STRATEGY_MIN        ZSTD_fast
#define ZSTD_STRATEGY_MAX        ZSTD_btultra2
#define ZSTD_BLOCKSIZE_MAX_MIN (1 << 10)


#define ZSTD_OVERLAPLOG_MIN       0
#define ZSTD_OVERLAPLOG_MAX       9

#define ZSTD_WINDOWLOG_LIMIT_DEFAULT 27


#define ZSTD_LDM_HASHLOG_MIN      ZSTD_HASHLOG_MIN
#define ZSTD_LDM_HASHLOG_MAX      ZSTD_HASHLOG_MAX
#define ZSTD_LDM_MINMATCH_MIN        4
#define ZSTD_LDM_MINMATCH_MAX     4096
#define ZSTD_LDM_BUCKETSIZELOG_MIN   1
#define ZSTD_LDM_BUCKETSIZELOG_MAX   8
#define ZSTD_LDM_HASHRATELOG_MIN     0
#define ZSTD_LDM_HASHRATELOG_MAX (ZSTD_WINDOWLOG_MAX - ZSTD_HASHLOG_MIN)


#define ZSTD_TARGETCBLOCKSIZE_MIN   1340
#define ZSTD_TARGETCBLOCKSIZE_MAX   ZSTD_BLOCKSIZE_MAX
#define ZSTD_SRCSIZEHINT_MIN        0
#define ZSTD_SRCSIZEHINT_MAX        INT_MAX


typedef struct ZSTD_CCtx_params_s ZSTD_CCtx_params;

typedef struct {
    unsigned int offset;

    unsigned int litLength;
    unsigned int matchLength;


    unsigned int rep;
} ZSTD_Sequence;

typedef struct {
    unsigned windowLog;
    unsigned chainLog;
    unsigned hashLog;
    unsigned searchLog;
    unsigned minMatch;
    unsigned targetLength;
    ZSTD_strategy strategy;
} ZSTD_compressionParameters;

typedef struct {
    int contentSizeFlag;
    int checksumFlag;
    int noDictIDFlag;
} ZSTD_frameParameters;

typedef struct {
    ZSTD_compressionParameters cParams;
    ZSTD_frameParameters fParams;
} ZSTD_parameters;

typedef enum {
    ZSTD_dct_auto = 0,
    ZSTD_dct_rawContent = 1,
    ZSTD_dct_fullDict = 2
} ZSTD_dictContentType_e;

typedef enum {
    ZSTD_dlm_byCopy = 0,
    ZSTD_dlm_byRef = 1
} ZSTD_dictLoadMethod_e;

typedef enum {
    ZSTD_f_zstd1 = 0,
    ZSTD_f_zstd1_magicless = 1
} ZSTD_format_e;

typedef enum {

    ZSTD_d_validateChecksum = 0,
    ZSTD_d_ignoreChecksum = 1
} ZSTD_forceIgnoreChecksum_e;

typedef enum {

    ZSTD_rmd_refSingleDDict = 0,
    ZSTD_rmd_refMultipleDDicts = 1
} ZSTD_refMultipleDDicts_e;

typedef enum {

    ZSTD_dictDefaultAttach = 0,
    ZSTD_dictForceAttach   = 1,
    ZSTD_dictForceCopy     = 2,
    ZSTD_dictForceLoad     = 3
} ZSTD_dictAttachPref_e;

typedef enum {
  ZSTD_lcm_auto = 0,
  ZSTD_lcm_huffman = 1,
  ZSTD_lcm_uncompressed = 2
} ZSTD_literalCompressionMode_e;

typedef enum {

  ZSTD_ps_auto = 0,
  ZSTD_ps_enable = 1,
  ZSTD_ps_disable = 2
} ZSTD_ParamSwitch_e;
#define ZSTD_paramSwitch_e ZSTD_ParamSwitch_e


ZSTDLIB_STATIC_API unsigned long long ZSTD_findDecompressedSize(const void* src, size_t srcSize);


ZSTDLIB_STATIC_API unsigned long long ZSTD_decompressBound(const void* src, size_t srcSize);


ZSTDLIB_STATIC_API size_t ZSTD_frameHeaderSize(const void* src, size_t srcSize);

typedef enum { ZSTD_frame, ZSTD_skippableFrame } ZSTD_FrameType_e;
#define ZSTD_frameType_e ZSTD_FrameType_e
typedef struct {
    unsigned long long frameContentSize;
    unsigned long long windowSize;
    unsigned blockSizeMax;
    ZSTD_FrameType_e frameType;
    unsigned headerSize;
    unsigned dictID;
    unsigned checksumFlag;
    unsigned _reserved1;
    unsigned _reserved2;
} ZSTD_FrameHeader;
#define ZSTD_frameHeader ZSTD_FrameHeader


ZSTDLIB_STATIC_API size_t ZSTD_getFrameHeader(ZSTD_FrameHeader* zfhPtr, const void* src, size_t srcSize);

ZSTDLIB_STATIC_API size_t ZSTD_getFrameHeader_advanced(ZSTD_FrameHeader* zfhPtr, const void* src, size_t srcSize, ZSTD_format_e format);


ZSTDLIB_STATIC_API size_t ZSTD_decompressionMargin(const void* src, size_t srcSize);


#define ZSTD_DECOMPRESSION_MARGIN(originalSize, blockSize) ((size_t)(                                              \
        ZSTD_FRAMEHEADERSIZE_MAX                                                               + \
        4                                                                                          + \
        ((originalSize) == 0 ? 0 : 3 * (((originalSize) + (blockSize) - 1) / blockSize))  + \
        (blockSize)                                                                       \
    ))

typedef enum {
  ZSTD_sf_noBlockDelimiters = 0,
  ZSTD_sf_explicitBlockDelimiters = 1
} ZSTD_SequenceFormat_e;
#define ZSTD_sequenceFormat_e ZSTD_SequenceFormat_e


ZSTDLIB_STATIC_API size_t ZSTD_sequenceBound(size_t srcSize);


ZSTD_DEPRECATED("For debugging only, will be replaced by ZSTD_extractSequences()")
ZSTDLIB_STATIC_API size_t
ZSTD_generateSequences(ZSTD_CCtx* zc,
                       ZSTD_Sequence* outSeqs, size_t outSeqsCapacity,
                       const void* src, size_t srcSize);


ZSTDLIB_STATIC_API size_t ZSTD_mergeBlockDelimiters(ZSTD_Sequence* sequences, size_t seqsSize);


ZSTDLIB_STATIC_API size_t
ZSTD_compressSequences(ZSTD_CCtx* cctx,
                       void* dst, size_t dstCapacity,
                 const ZSTD_Sequence* inSeqs, size_t inSeqsSize,
                 const void* src, size_t srcSize);


ZSTDLIB_STATIC_API size_t
ZSTD_compressSequencesAndLiterals(ZSTD_CCtx* cctx,
                                  void* dst, size_t dstCapacity,
                            const ZSTD_Sequence* inSeqs, size_t nbSequences,
                            const void* literals, size_t litSize, size_t litBufCapacity,
                            size_t decompressedSize);


ZSTDLIB_STATIC_API size_t ZSTD_writeSkippableFrame(void* dst, size_t dstCapacity,
                                             const void* src, size_t srcSize,
                                                   unsigned magicVariant);


ZSTDLIB_STATIC_API size_t ZSTD_readSkippableFrame(void* dst, size_t dstCapacity,
                                                  unsigned* magicVariant,
                                                  const void* src, size_t srcSize);


ZSTDLIB_STATIC_API unsigned ZSTD_isSkippableFrame(const void* buffer, size_t size);


ZSTDLIB_STATIC_API size_t ZSTD_estimateCCtxSize(int maxCompressionLevel);
ZSTDLIB_STATIC_API size_t ZSTD_estimateCCtxSize_usingCParams(ZSTD_compressionParameters cParams);
ZSTDLIB_STATIC_API size_t ZSTD_estimateCCtxSize_usingCCtxParams(const ZSTD_CCtx_params* params);
ZSTDLIB_STATIC_API size_t ZSTD_estimateDCtxSize(void);


ZSTDLIB_STATIC_API size_t ZSTD_estimateCStreamSize(int maxCompressionLevel);
ZSTDLIB_STATIC_API size_t ZSTD_estimateCStreamSize_usingCParams(ZSTD_compressionParameters cParams);
ZSTDLIB_STATIC_API size_t ZSTD_estimateCStreamSize_usingCCtxParams(const ZSTD_CCtx_params* params);
ZSTDLIB_STATIC_API size_t ZSTD_estimateDStreamSize(size_t maxWindowSize);
ZSTDLIB_STATIC_API size_t ZSTD_estimateDStreamSize_fromFrame(const void* src, size_t srcSize);


ZSTDLIB_STATIC_API size_t ZSTD_estimateCDictSize(size_t dictSize, int compressionLevel);
ZSTDLIB_STATIC_API size_t ZSTD_estimateCDictSize_advanced(size_t dictSize, ZSTD_compressionParameters cParams, ZSTD_dictLoadMethod_e dictLoadMethod);
ZSTDLIB_STATIC_API size_t ZSTD_estimateDDictSize(size_t dictSize, ZSTD_dictLoadMethod_e dictLoadMethod);


ZSTDLIB_STATIC_API ZSTD_CCtx*    ZSTD_initStaticCCtx(void* workspace, size_t workspaceSize);
ZSTDLIB_STATIC_API ZSTD_CStream* ZSTD_initStaticCStream(void* workspace, size_t workspaceSize);

ZSTDLIB_STATIC_API ZSTD_DCtx*    ZSTD_initStaticDCtx(void* workspace, size_t workspaceSize);
ZSTDLIB_STATIC_API ZSTD_DStream* ZSTD_initStaticDStream(void* workspace, size_t workspaceSize);

ZSTDLIB_STATIC_API const ZSTD_CDict* ZSTD_initStaticCDict(
                                        void* workspace, size_t workspaceSize,
                                        const void* dict, size_t dictSize,
                                        ZSTD_dictLoadMethod_e dictLoadMethod,
                                        ZSTD_dictContentType_e dictContentType,
                                        ZSTD_compressionParameters cParams);

ZSTDLIB_STATIC_API const ZSTD_DDict* ZSTD_initStaticDDict(
                                        void* workspace, size_t workspaceSize,
                                        const void* dict, size_t dictSize,
                                        ZSTD_dictLoadMethod_e dictLoadMethod,
                                        ZSTD_dictContentType_e dictContentType);


typedef void* (*ZSTD_allocFunction) (void* opaque, size_t size);
typedef void  (*ZSTD_freeFunction) (void* opaque, void* address);
typedef struct { ZSTD_allocFunction customAlloc; ZSTD_freeFunction customFree; void* opaque; } ZSTD_customMem;
#if defined(__clang__) && __clang_major__ >= 5
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
static
#ifdef __GNUC__
__attribute__((__unused__))
#endif
ZSTD_customMem const ZSTD_defaultCMem = { NULL, NULL, NULL };
#if defined(__clang__) && __clang_major__ >= 5
#pragma clang diagnostic pop
#endif

ZSTDLIB_STATIC_API ZSTD_CCtx*    ZSTD_createCCtx_advanced(ZSTD_customMem customMem);
ZSTDLIB_STATIC_API ZSTD_CStream* ZSTD_createCStream_advanced(ZSTD_customMem customMem);
ZSTDLIB_STATIC_API ZSTD_DCtx*    ZSTD_createDCtx_advanced(ZSTD_customMem customMem);
ZSTDLIB_STATIC_API ZSTD_DStream* ZSTD_createDStream_advanced(ZSTD_customMem customMem);

ZSTDLIB_STATIC_API ZSTD_CDict* ZSTD_createCDict_advanced(const void* dict, size_t dictSize,
                                                  ZSTD_dictLoadMethod_e dictLoadMethod,
                                                  ZSTD_dictContentType_e dictContentType,
                                                  ZSTD_compressionParameters cParams,
                                                  ZSTD_customMem customMem);


typedef struct POOL_ctx_s ZSTD_threadPool;
ZSTDLIB_STATIC_API ZSTD_threadPool* ZSTD_createThreadPool(size_t numThreads);
ZSTDLIB_STATIC_API void ZSTD_freeThreadPool (ZSTD_threadPool* pool);
ZSTDLIB_STATIC_API size_t ZSTD_CCtx_refThreadPool(ZSTD_CCtx* cctx, ZSTD_threadPool* pool);


ZSTDLIB_STATIC_API ZSTD_CDict* ZSTD_createCDict_advanced2(
    const void* dict, size_t dictSize,
    ZSTD_dictLoadMethod_e dictLoadMethod,
    ZSTD_dictContentType_e dictContentType,
    const ZSTD_CCtx_params* cctxParams,
    ZSTD_customMem customMem);

ZSTDLIB_STATIC_API ZSTD_DDict* ZSTD_createDDict_advanced(
    const void* dict, size_t dictSize,
    ZSTD_dictLoadMethod_e dictLoadMethod,
    ZSTD_dictContentType_e dictContentType,
    ZSTD_customMem customMem);


ZSTDLIB_STATIC_API ZSTD_CDict* ZSTD_createCDict_byReference(const void* dictBuffer, size_t dictSize, int compressionLevel);


ZSTDLIB_STATIC_API ZSTD_compressionParameters ZSTD_getCParams(int compressionLevel, unsigned long long estimatedSrcSize, size_t dictSize);


ZSTDLIB_STATIC_API ZSTD_parameters ZSTD_getParams(int compressionLevel, unsigned long long estimatedSrcSize, size_t dictSize);


ZSTDLIB_STATIC_API size_t ZSTD_checkCParams(ZSTD_compressionParameters params);


ZSTDLIB_STATIC_API ZSTD_compressionParameters ZSTD_adjustCParams(ZSTD_compressionParameters cPar, unsigned long long srcSize, size_t dictSize);


ZSTDLIB_STATIC_API size_t ZSTD_CCtx_setCParams(ZSTD_CCtx* cctx, ZSTD_compressionParameters cparams);


ZSTDLIB_STATIC_API size_t ZSTD_CCtx_setFParams(ZSTD_CCtx* cctx, ZSTD_frameParameters fparams);


ZSTDLIB_STATIC_API size_t ZSTD_CCtx_setParams(ZSTD_CCtx* cctx, ZSTD_parameters params);


ZSTD_DEPRECATED("use ZSTD_compress2")
ZSTDLIB_STATIC_API
size_t ZSTD_compress_advanced(ZSTD_CCtx* cctx,
                              void* dst, size_t dstCapacity,
                        const void* src, size_t srcSize,
                        const void* dict,size_t dictSize,
                              ZSTD_parameters params);


ZSTD_DEPRECATED("use ZSTD_compress2 with ZSTD_CCtx_loadDictionary")
ZSTDLIB_STATIC_API
size_t ZSTD_compress_usingCDict_advanced(ZSTD_CCtx* cctx,
                                              void* dst, size_t dstCapacity,
                                        const void* src, size_t srcSize,
                                        const ZSTD_CDict* cdict,
                                              ZSTD_frameParameters fParams);


ZSTDLIB_STATIC_API size_t ZSTD_CCtx_loadDictionary_byReference(ZSTD_CCtx* cctx, const void* dict, size_t dictSize);


ZSTDLIB_STATIC_API size_t ZSTD_CCtx_loadDictionary_advanced(ZSTD_CCtx* cctx, const void* dict, size_t dictSize, ZSTD_dictLoadMethod_e dictLoadMethod, ZSTD_dictContentType_e dictContentType);


ZSTDLIB_STATIC_API size_t ZSTD_CCtx_refPrefix_advanced(ZSTD_CCtx* cctx, const void* prefix, size_t prefixSize, ZSTD_dictContentType_e dictContentType);


 #define ZSTD_c_rsyncable ZSTD_c_experimentalParam1


#define ZSTD_c_format ZSTD_c_experimentalParam2


#define ZSTD_c_forceMaxWindow ZSTD_c_experimentalParam3


#define ZSTD_c_forceAttachDict ZSTD_c_experimentalParam4


#define ZSTD_c_literalCompressionMode ZSTD_c_experimentalParam5


#define ZSTD_c_srcSizeHint ZSTD_c_experimentalParam7


#define ZSTD_c_enableDedicatedDictSearch ZSTD_c_experimentalParam8


#define ZSTD_c_stableInBuffer ZSTD_c_experimentalParam9


#define ZSTD_c_stableOutBuffer ZSTD_c_experimentalParam10


#define ZSTD_c_blockDelimiters ZSTD_c_experimentalParam11


#define ZSTD_c_validateSequences ZSTD_c_experimentalParam12


#define ZSTD_BLOCKSPLITTER_LEVEL_MAX 6
#define ZSTD_c_blockSplitterLevel ZSTD_c_experimentalParam20


#define ZSTD_c_splitAfterSequences ZSTD_c_experimentalParam13


#define ZSTD_c_useRowMatchFinder ZSTD_c_experimentalParam14


#define ZSTD_c_deterministicRefPrefix ZSTD_c_experimentalParam15


#define ZSTD_c_prefetchCDictTables ZSTD_c_experimentalParam16


#define ZSTD_c_enableSeqProducerFallback ZSTD_c_experimentalParam17


#define ZSTD_c_maxBlockSize ZSTD_c_experimentalParam18


#define ZSTD_c_repcodeResolution ZSTD_c_experimentalParam19
#define ZSTD_c_searchForExternalRepcodes ZSTD_c_experimentalParam19


ZSTDLIB_STATIC_API size_t ZSTD_CCtx_getParameter(const ZSTD_CCtx* cctx, ZSTD_cParameter param, int* value);


ZSTDLIB_STATIC_API ZSTD_CCtx_params* ZSTD_createCCtxParams(void);
ZSTDLIB_STATIC_API size_t ZSTD_freeCCtxParams(ZSTD_CCtx_params* params);


ZSTDLIB_STATIC_API size_t ZSTD_CCtxParams_reset(ZSTD_CCtx_params* params);


ZSTDLIB_STATIC_API size_t ZSTD_CCtxParams_init(ZSTD_CCtx_params* cctxParams, int compressionLevel);


ZSTDLIB_STATIC_API size_t ZSTD_CCtxParams_init_advanced(ZSTD_CCtx_params* cctxParams, ZSTD_parameters params);


ZSTDLIB_STATIC_API size_t ZSTD_CCtxParams_setParameter(ZSTD_CCtx_params* params, ZSTD_cParameter param, int value);


ZSTDLIB_STATIC_API size_t ZSTD_CCtxParams_getParameter(const ZSTD_CCtx_params* params, ZSTD_cParameter param, int* value);


ZSTDLIB_STATIC_API size_t ZSTD_CCtx_setParametersUsingCCtxParams(
        ZSTD_CCtx* cctx, const ZSTD_CCtx_params* params);


ZSTDLIB_STATIC_API size_t ZSTD_compressStream2_simpleArgs (
                            ZSTD_CCtx* cctx,
                            void* dst, size_t dstCapacity, size_t* dstPos,
                      const void* src, size_t srcSize, size_t* srcPos,
                            ZSTD_EndDirective endOp);


ZSTDLIB_STATIC_API unsigned ZSTD_isFrame(const void* buffer, size_t size);


ZSTDLIB_STATIC_API ZSTD_DDict* ZSTD_createDDict_byReference(const void* dictBuffer, size_t dictSize);


ZSTDLIB_STATIC_API size_t ZSTD_DCtx_loadDictionary_byReference(ZSTD_DCtx* dctx, const void* dict, size_t dictSize);


ZSTDLIB_STATIC_API size_t ZSTD_DCtx_loadDictionary_advanced(ZSTD_DCtx* dctx, const void* dict, size_t dictSize, ZSTD_dictLoadMethod_e dictLoadMethod, ZSTD_dictContentType_e dictContentType);


ZSTDLIB_STATIC_API size_t ZSTD_DCtx_refPrefix_advanced(ZSTD_DCtx* dctx, const void* prefix, size_t prefixSize, ZSTD_dictContentType_e dictContentType);


ZSTDLIB_STATIC_API size_t ZSTD_DCtx_setMaxWindowSize(ZSTD_DCtx* dctx, size_t maxWindowSize);


ZSTDLIB_STATIC_API size_t ZSTD_DCtx_getParameter(ZSTD_DCtx* dctx, ZSTD_dParameter param, int* value);


#define ZSTD_d_format ZSTD_d_experimentalParam1

#define ZSTD_d_stableOutBuffer ZSTD_d_experimentalParam2


#define ZSTD_d_forceIgnoreChecksum ZSTD_d_experimentalParam3


#define ZSTD_d_refMultipleDDicts ZSTD_d_experimentalParam4


#define ZSTD_d_disableHuffmanAssembly ZSTD_d_experimentalParam5


#define ZSTD_d_maxBlockSize ZSTD_d_experimentalParam6


ZSTD_DEPRECATED("use ZSTD_DCtx_setParameter() instead")
ZSTDLIB_STATIC_API
size_t ZSTD_DCtx_setFormat(ZSTD_DCtx* dctx, ZSTD_format_e format);


ZSTDLIB_STATIC_API size_t ZSTD_decompressStream_simpleArgs (
                            ZSTD_DCtx* dctx,
                            void* dst, size_t dstCapacity, size_t* dstPos,
                      const void* src, size_t srcSize, size_t* srcPos);


ZSTD_DEPRECATED("use ZSTD_CCtx_reset, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API
size_t ZSTD_initCStream_srcSize(ZSTD_CStream* zcs,
                         int compressionLevel,
                         unsigned long long pledgedSrcSize);


ZSTD_DEPRECATED("use ZSTD_CCtx_reset, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API
size_t ZSTD_initCStream_usingDict(ZSTD_CStream* zcs,
                     const void* dict, size_t dictSize,
                           int compressionLevel);


ZSTD_DEPRECATED("use ZSTD_CCtx_reset, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API
size_t ZSTD_initCStream_advanced(ZSTD_CStream* zcs,
                    const void* dict, size_t dictSize,
                          ZSTD_parameters params,
                          unsigned long long pledgedSrcSize);


ZSTD_DEPRECATED("use ZSTD_CCtx_reset and ZSTD_CCtx_refCDict, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API
size_t ZSTD_initCStream_usingCDict(ZSTD_CStream* zcs, const ZSTD_CDict* cdict);


ZSTD_DEPRECATED("use ZSTD_CCtx_reset and ZSTD_CCtx_refCDict, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API
size_t ZSTD_initCStream_usingCDict_advanced(ZSTD_CStream* zcs,
                               const ZSTD_CDict* cdict,
                                     ZSTD_frameParameters fParams,
                                     unsigned long long pledgedSrcSize);


ZSTD_DEPRECATED("use ZSTD_CCtx_reset, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API
size_t ZSTD_resetCStream(ZSTD_CStream* zcs, unsigned long long pledgedSrcSize);


typedef struct {
    unsigned long long ingested;
    unsigned long long consumed;
    unsigned long long produced;
    unsigned long long flushed;
    unsigned currentJobID;
    unsigned nbActiveWorkers;
} ZSTD_frameProgression;


ZSTDLIB_STATIC_API ZSTD_frameProgression ZSTD_getFrameProgression(const ZSTD_CCtx* cctx);


ZSTDLIB_STATIC_API size_t ZSTD_toFlushNow(ZSTD_CCtx* cctx);


ZSTD_DEPRECATED("use ZSTD_DCtx_reset + ZSTD_DCtx_loadDictionary, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API size_t ZSTD_initDStream_usingDict(ZSTD_DStream* zds, const void* dict, size_t dictSize);


ZSTD_DEPRECATED("use ZSTD_DCtx_reset + ZSTD_DCtx_refDDict, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API size_t ZSTD_initDStream_usingDDict(ZSTD_DStream* zds, const ZSTD_DDict* ddict);


ZSTD_DEPRECATED("use ZSTD_DCtx_reset, see zstd.h for detailed instructions")
ZSTDLIB_STATIC_API size_t ZSTD_resetDStream(ZSTD_DStream* zds);


#define ZSTD_SEQUENCE_PRODUCER_ERROR ((size_t)(-1))

typedef size_t (*ZSTD_sequenceProducer_F) (
  void* sequenceProducerState,
  ZSTD_Sequence* outSeqs, size_t outSeqsCapacity,
  const void* src, size_t srcSize,
  const void* dict, size_t dictSize,
  int compressionLevel,
  size_t windowSize
);


ZSTDLIB_STATIC_API void
ZSTD_registerSequenceProducer(
  ZSTD_CCtx* cctx,
  void* sequenceProducerState,
  ZSTD_sequenceProducer_F sequenceProducer
);


ZSTDLIB_STATIC_API void
ZSTD_CCtxParams_registerSequenceProducer(
  ZSTD_CCtx_params* params,
  void* sequenceProducerState,
  ZSTD_sequenceProducer_F sequenceProducer
);


ZSTD_DEPRECATED("The buffer-less API is deprecated in favor of the normal streaming API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_compressBegin(ZSTD_CCtx* cctx, int compressionLevel);
ZSTD_DEPRECATED("The buffer-less API is deprecated in favor of the normal streaming API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_compressBegin_usingDict(ZSTD_CCtx* cctx, const void* dict, size_t dictSize, int compressionLevel);
ZSTD_DEPRECATED("The buffer-less API is deprecated in favor of the normal streaming API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_compressBegin_usingCDict(ZSTD_CCtx* cctx, const ZSTD_CDict* cdict);

ZSTD_DEPRECATED("This function will likely be removed in a future release. It is misleading and has very limited utility.")
ZSTDLIB_STATIC_API
size_t ZSTD_copyCCtx(ZSTD_CCtx* cctx, const ZSTD_CCtx* preparedCCtx, unsigned long long pledgedSrcSize);

ZSTD_DEPRECATED("The buffer-less API is deprecated in favor of the normal streaming API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_compressContinue(ZSTD_CCtx* cctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);
ZSTD_DEPRECATED("The buffer-less API is deprecated in favor of the normal streaming API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_compressEnd(ZSTD_CCtx* cctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);


ZSTD_DEPRECATED("use advanced API to access custom parameters")
ZSTDLIB_STATIC_API
size_t ZSTD_compressBegin_advanced(ZSTD_CCtx* cctx, const void* dict, size_t dictSize, ZSTD_parameters params, unsigned long long pledgedSrcSize);
ZSTD_DEPRECATED("use advanced API to access custom parameters")
ZSTDLIB_STATIC_API
size_t ZSTD_compressBegin_usingCDict_advanced(ZSTD_CCtx* const cctx, const ZSTD_CDict* const cdict, ZSTD_frameParameters const fParams, unsigned long long const pledgedSrcSize);


ZSTDLIB_STATIC_API size_t ZSTD_decodingBufferSize_min(unsigned long long windowSize, unsigned long long frameContentSize);

ZSTDLIB_STATIC_API size_t ZSTD_decompressBegin(ZSTD_DCtx* dctx);
ZSTDLIB_STATIC_API size_t ZSTD_decompressBegin_usingDict(ZSTD_DCtx* dctx, const void* dict, size_t dictSize);
ZSTDLIB_STATIC_API size_t ZSTD_decompressBegin_usingDDict(ZSTD_DCtx* dctx, const ZSTD_DDict* ddict);

ZSTDLIB_STATIC_API size_t ZSTD_nextSrcSizeToDecompress(ZSTD_DCtx* dctx);
ZSTDLIB_STATIC_API size_t ZSTD_decompressContinue(ZSTD_DCtx* dctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);


ZSTD_DEPRECATED("This function will likely be removed in the next minor release. It is misleading and has very limited utility.")
ZSTDLIB_STATIC_API void   ZSTD_copyDCtx(ZSTD_DCtx* dctx, const ZSTD_DCtx* preparedDCtx);
typedef enum { ZSTDnit_frameHeader, ZSTDnit_blockHeader, ZSTDnit_block, ZSTDnit_lastBlock, ZSTDnit_checksum, ZSTDnit_skippableFrame } ZSTD_nextInputType_e;
ZSTDLIB_STATIC_API ZSTD_nextInputType_e ZSTD_nextInputType(ZSTD_DCtx* dctx);


ZSTDLIB_API int ZSTD_isDeterministicBuild(void);


ZSTD_DEPRECATED("The block API is deprecated in favor of the normal compression API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_getBlockSize   (const ZSTD_CCtx* cctx);
ZSTD_DEPRECATED("The block API is deprecated in favor of the normal compression API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_compressBlock  (ZSTD_CCtx* cctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);
ZSTD_DEPRECATED("The block API is deprecated in favor of the normal compression API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_decompressBlock(ZSTD_DCtx* dctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);
ZSTD_DEPRECATED("The block API is deprecated in favor of the normal compression API. See docs.")
ZSTDLIB_STATIC_API size_t ZSTD_insertBlock    (ZSTD_DCtx* dctx, const void* blockStart, size_t blockSize);

#if defined (__cplusplus)
}
#endif

#endif

#define FSE_STATIC_LINKING_ONLY


#ifndef XXH_STATIC_LINKING_ONLY
#  define XXH_STATIC_LINKING_ONLY
#endif


#ifndef XXH_NO_XXH3
# define XXH_NO_XXH3
#endif

#ifndef XXH_NAMESPACE
# define XXH_NAMESPACE ZSTD_
#endif


#ifdef XXH_DOXYGEN

#  define XXH_STATIC_LINKING_ONLY


#  define XXH_IMPLEMENTATION


#  define XXH_INLINE_ALL
#  undef XXH_INLINE_ALL

#  define XXH_PRIVATE_API
#  undef XXH_PRIVATE_API

#  define XXH_NAMESPACE
#  undef XXH_NAMESPACE
#endif

#if (defined(XXH_INLINE_ALL) || defined(XXH_PRIVATE_API)) \
    && !defined(XXH_INLINE_ALL_31684351384)

#  define XXH_INLINE_ALL_31684351384

#  undef XXH_STATIC_LINKING_ONLY
#  define XXH_STATIC_LINKING_ONLY

#  undef XXH_PUBLIC_API
#  if defined(__GNUC__)
#    define XXH_PUBLIC_API static __inline __attribute__((unused))
#  elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) )
#    define XXH_PUBLIC_API static inline
#  elif defined(_MSC_VER)
#    define XXH_PUBLIC_API static __inline
#  else

#    define XXH_PUBLIC_API static
#  endif


#  undef XXH_versionNumber

#  undef XXH32
#  undef XXH32_createState
#  undef XXH32_freeState
#  undef XXH32_reset
#  undef XXH32_update
#  undef XXH32_digest
#  undef XXH32_copyState
#  undef XXH32_canonicalFromHash
#  undef XXH32_hashFromCanonical

#  undef XXH64
#  undef XXH64_createState
#  undef XXH64_freeState
#  undef XXH64_reset
#  undef XXH64_update
#  undef XXH64_digest
#  undef XXH64_copyState
#  undef XXH64_canonicalFromHash
#  undef XXH64_hashFromCanonical

#  undef XXH3_64bits
#  undef XXH3_64bits_withSecret
#  undef XXH3_64bits_withSeed
#  undef XXH3_64bits_withSecretandSeed
#  undef XXH3_createState
#  undef XXH3_freeState
#  undef XXH3_copyState
#  undef XXH3_64bits_reset
#  undef XXH3_64bits_reset_withSeed
#  undef XXH3_64bits_reset_withSecret
#  undef XXH3_64bits_update
#  undef XXH3_64bits_digest
#  undef XXH3_generateSecret

#  undef XXH128
#  undef XXH3_128bits
#  undef XXH3_128bits_withSeed
#  undef XXH3_128bits_withSecret
#  undef XXH3_128bits_reset
#  undef XXH3_128bits_reset_withSeed
#  undef XXH3_128bits_reset_withSecret
#  undef XXH3_128bits_reset_withSecretandSeed
#  undef XXH3_128bits_update
#  undef XXH3_128bits_digest
#  undef XXH128_isEqual
#  undef XXH128_cmp
#  undef XXH128_canonicalFromHash
#  undef XXH128_hashFromCanonical

#  undef XXH_NAMESPACE


#  define XXH_NAMESPACE XXH_INLINE_

#  define XXH_IPREF(Id)   XXH_NAMESPACE ## Id
#  define XXH_OK XXH_IPREF(XXH_OK)
#  define XXH_ERROR XXH_IPREF(XXH_ERROR)
#  define XXH_errorcode XXH_IPREF(XXH_errorcode)
#  define XXH32_canonical_t  XXH_IPREF(XXH32_canonical_t)
#  define XXH64_canonical_t  XXH_IPREF(XXH64_canonical_t)
#  define XXH128_canonical_t XXH_IPREF(XXH128_canonical_t)
#  define XXH32_state_s XXH_IPREF(XXH32_state_s)
#  define XXH32_state_t XXH_IPREF(XXH32_state_t)
#  define XXH64_state_s XXH_IPREF(XXH64_state_s)
#  define XXH64_state_t XXH_IPREF(XXH64_state_t)
#  define XXH3_state_s  XXH_IPREF(XXH3_state_s)
#  define XXH3_state_t  XXH_IPREF(XXH3_state_t)
#  define XXH128_hash_t XXH_IPREF(XXH128_hash_t)

#  undef XXHASH_H_5627135585666179
#  undef XXHASH_H_STATIC_13879238742
#endif


#ifndef XXHASH_H_5627135585666179
#define XXHASH_H_5627135585666179 1


#if !defined(XXH_INLINE_ALL) && !defined(XXH_PRIVATE_API)
#  if defined(WIN32) && defined(_MSC_VER) && (defined(XXH_IMPORT) || defined(XXH_EXPORT))
#    ifdef XXH_EXPORT
#      define XXH_PUBLIC_API __declspec(dllexport)
#    elif XXH_IMPORT
#      define XXH_PUBLIC_API __declspec(dllimport)
#    endif
#  else
#    define XXH_PUBLIC_API
#  endif
#endif

#ifdef XXH_NAMESPACE
#  define XXH_CAT(A,B) A##B
#  define XXH_NAME2(A,B) XXH_CAT(A,B)
#  define XXH_versionNumber XXH_NAME2(XXH_NAMESPACE, XXH_versionNumber)

#  define XXH32 XXH_NAME2(XXH_NAMESPACE, XXH32)
#  define XXH32_createState XXH_NAME2(XXH_NAMESPACE, XXH32_createState)
#  define XXH32_freeState XXH_NAME2(XXH_NAMESPACE, XXH32_freeState)
#  define XXH32_reset XXH_NAME2(XXH_NAMESPACE, XXH32_reset)
#  define XXH32_update XXH_NAME2(XXH_NAMESPACE, XXH32_update)
#  define XXH32_digest XXH_NAME2(XXH_NAMESPACE, XXH32_digest)
#  define XXH32_copyState XXH_NAME2(XXH_NAMESPACE, XXH32_copyState)
#  define XXH32_canonicalFromHash XXH_NAME2(XXH_NAMESPACE, XXH32_canonicalFromHash)
#  define XXH32_hashFromCanonical XXH_NAME2(XXH_NAMESPACE, XXH32_hashFromCanonical)

#  define XXH64 XXH_NAME2(XXH_NAMESPACE, XXH64)
#  define XXH64_createState XXH_NAME2(XXH_NAMESPACE, XXH64_createState)
#  define XXH64_freeState XXH_NAME2(XXH_NAMESPACE, XXH64_freeState)
#  define XXH64_reset XXH_NAME2(XXH_NAMESPACE, XXH64_reset)
#  define XXH64_update XXH_NAME2(XXH_NAMESPACE, XXH64_update)
#  define XXH64_digest XXH_NAME2(XXH_NAMESPACE, XXH64_digest)
#  define XXH64_copyState XXH_NAME2(XXH_NAMESPACE, XXH64_copyState)
#  define XXH64_canonicalFromHash XXH_NAME2(XXH_NAMESPACE, XXH64_canonicalFromHash)
#  define XXH64_hashFromCanonical XXH_NAME2(XXH_NAMESPACE, XXH64_hashFromCanonical)

#  define XXH3_64bits XXH_NAME2(XXH_NAMESPACE, XXH3_64bits)
#  define XXH3_64bits_withSecret XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_withSecret)
#  define XXH3_64bits_withSeed XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_withSeed)
#  define XXH3_64bits_withSecretandSeed XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_withSecretandSeed)
#  define XXH3_createState XXH_NAME2(XXH_NAMESPACE, XXH3_createState)
#  define XXH3_freeState XXH_NAME2(XXH_NAMESPACE, XXH3_freeState)
#  define XXH3_copyState XXH_NAME2(XXH_NAMESPACE, XXH3_copyState)
#  define XXH3_64bits_reset XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_reset)
#  define XXH3_64bits_reset_withSeed XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_reset_withSeed)
#  define XXH3_64bits_reset_withSecret XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_reset_withSecret)
#  define XXH3_64bits_reset_withSecretandSeed XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_reset_withSecretandSeed)
#  define XXH3_64bits_update XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_update)
#  define XXH3_64bits_digest XXH_NAME2(XXH_NAMESPACE, XXH3_64bits_digest)
#  define XXH3_generateSecret XXH_NAME2(XXH_NAMESPACE, XXH3_generateSecret)
#  define XXH3_generateSecret_fromSeed XXH_NAME2(XXH_NAMESPACE, XXH3_generateSecret_fromSeed)

#  define XXH128 XXH_NAME2(XXH_NAMESPACE, XXH128)
#  define XXH3_128bits XXH_NAME2(XXH_NAMESPACE, XXH3_128bits)
#  define XXH3_128bits_withSeed XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_withSeed)
#  define XXH3_128bits_withSecret XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_withSecret)
#  define XXH3_128bits_withSecretandSeed XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_withSecretandSeed)
#  define XXH3_128bits_reset XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_reset)
#  define XXH3_128bits_reset_withSeed XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_reset_withSeed)
#  define XXH3_128bits_reset_withSecret XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_reset_withSecret)
#  define XXH3_128bits_reset_withSecretandSeed XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_reset_withSecretandSeed)
#  define XXH3_128bits_update XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_update)
#  define XXH3_128bits_digest XXH_NAME2(XXH_NAMESPACE, XXH3_128bits_digest)
#  define XXH128_isEqual XXH_NAME2(XXH_NAMESPACE, XXH128_isEqual)
#  define XXH128_cmp     XXH_NAME2(XXH_NAMESPACE, XXH128_cmp)
#  define XXH128_canonicalFromHash XXH_NAME2(XXH_NAMESPACE, XXH128_canonicalFromHash)
#  define XXH128_hashFromCanonical XXH_NAME2(XXH_NAMESPACE, XXH128_hashFromCanonical)
#endif


#if !defined(XXH_INLINE_ALL) && !defined(XXH_PRIVATE_API)
#  if defined(WIN32) && defined(_MSC_VER) && (defined(XXH_IMPORT) || defined(XXH_EXPORT))
#    ifdef XXH_EXPORT
#      define XXH_PUBLIC_API __declspec(dllexport)
#    elif XXH_IMPORT
#      define XXH_PUBLIC_API __declspec(dllimport)
#    endif
#  else
#    define XXH_PUBLIC_API
#  endif
#endif

#if defined (__GNUC__)
# define XXH_CONSTF  __attribute__((const))
# define XXH_PUREF   __attribute__((pure))
# define XXH_MALLOCF __attribute__((malloc))
#else
# define XXH_CONSTF
# define XXH_PUREF
# define XXH_MALLOCF
#endif


#define XXH_VERSION_MAJOR    0
#define XXH_VERSION_MINOR    8
#define XXH_VERSION_RELEASE  2

#define XXH_VERSION_NUMBER  (XXH_VERSION_MAJOR *100*100 + XXH_VERSION_MINOR *100 + XXH_VERSION_RELEASE)

#if defined (__cplusplus)
extern "C" {
#endif

XXH_PUBLIC_API XXH_CONSTF unsigned XXH_versionNumber (void);

#if defined (__cplusplus)
}
#endif


#include <stddef.h>

typedef enum {
    XXH_OK = 0,
    XXH_ERROR
} XXH_errorcode;


#if defined(XXH_DOXYGEN)

typedef uint32_t XXH32_hash_t;

#elif !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) ) )
#   ifdef _AIX
#     include <inttypes.h>
#   else
#     include <stdint.h>
#   endif
    typedef uint32_t XXH32_hash_t;

#else
#   include <limits.h>
#   if UINT_MAX == 0xFFFFFFFFUL
      typedef unsigned int XXH32_hash_t;
#   elif ULONG_MAX == 0xFFFFFFFFUL
      typedef unsigned long XXH32_hash_t;
#   else
#     error "unsupported platform: need a 32-bit type"
#   endif
#endif

#if defined (__cplusplus)
extern "C" {
#endif


XXH_PUBLIC_API XXH_PUREF XXH32_hash_t XXH32 (const void* input, size_t length, XXH32_hash_t seed);

#ifndef XXH_NO_STREAM

typedef struct XXH32_state_s XXH32_state_t;


XXH_PUBLIC_API XXH_MALLOCF XXH32_state_t* XXH32_createState(void);

XXH_PUBLIC_API XXH_errorcode  XXH32_freeState(XXH32_state_t* statePtr);

XXH_PUBLIC_API void XXH32_copyState(XXH32_state_t* dst_state, const XXH32_state_t* src_state);


XXH_PUBLIC_API XXH_errorcode XXH32_reset  (XXH32_state_t* statePtr, XXH32_hash_t seed);


XXH_PUBLIC_API XXH_errorcode XXH32_update (XXH32_state_t* statePtr, const void* input, size_t length);


XXH_PUBLIC_API XXH_PUREF XXH32_hash_t XXH32_digest (const XXH32_state_t* statePtr);
#endif


typedef struct {
    unsigned char digest[4];
} XXH32_canonical_t;


XXH_PUBLIC_API void XXH32_canonicalFromHash(XXH32_canonical_t* dst, XXH32_hash_t hash);


XXH_PUBLIC_API XXH_PUREF XXH32_hash_t XXH32_hashFromCanonical(const XXH32_canonical_t* src);


#ifdef __has_attribute
# define XXH_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
# define XXH_HAS_ATTRIBUTE(x) 0
#endif


#define XXH_C23_VN 201711L


#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= XXH_C23_VN) && defined(__has_c_attribute)
# define XXH_HAS_C_ATTRIBUTE(x) __has_c_attribute(x)
#else
# define XXH_HAS_C_ATTRIBUTE(x) 0
#endif


#if defined(__cplusplus) && defined(__has_cpp_attribute)
# define XXH_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
# define XXH_HAS_CPP_ATTRIBUTE(x) 0
#endif


#if XXH_HAS_C_ATTRIBUTE(fallthrough) || XXH_HAS_CPP_ATTRIBUTE(fallthrough)
# define XXH_FALLTHROUGH [[fallthrough]]
#elif XXH_HAS_ATTRIBUTE(__fallthrough__)
# define XXH_FALLTHROUGH __attribute__ ((__fallthrough__))
#else
# define XXH_FALLTHROUGH
#endif


#if XXH_HAS_ATTRIBUTE(noescape)
# define XXH_NOESCAPE __attribute__((noescape))
#else
# define XXH_NOESCAPE
#endif


#if defined (__cplusplus)
}
#endif


#ifndef XXH_NO_LONG_LONG

#if defined(XXH_DOXYGEN)

typedef uint64_t XXH64_hash_t;
#elif !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) ) )
#   ifdef _AIX
#     include <inttypes.h>
#   else
#     include <stdint.h>
#   endif
   typedef uint64_t XXH64_hash_t;
#else
#  include <limits.h>
#  if defined(__LP64__) && ULONG_MAX == 0xFFFFFFFFFFFFFFFFULL

     typedef unsigned long XXH64_hash_t;
#  else

     typedef unsigned long long XXH64_hash_t;
#  endif
#endif

#if defined (__cplusplus)
extern "C" {
#endif


XXH_PUBLIC_API XXH_PUREF XXH64_hash_t XXH64(XXH_NOESCAPE const void* input, size_t length, XXH64_hash_t seed);


#ifndef XXH_NO_STREAM

typedef struct XXH64_state_s XXH64_state_t;


XXH_PUBLIC_API XXH_MALLOCF XXH64_state_t* XXH64_createState(void);


XXH_PUBLIC_API XXH_errorcode  XXH64_freeState(XXH64_state_t* statePtr);


XXH_PUBLIC_API void XXH64_copyState(XXH_NOESCAPE XXH64_state_t* dst_state, const XXH64_state_t* src_state);


XXH_PUBLIC_API XXH_errorcode XXH64_reset  (XXH_NOESCAPE XXH64_state_t* statePtr, XXH64_hash_t seed);


XXH_PUBLIC_API XXH_errorcode XXH64_update (XXH_NOESCAPE XXH64_state_t* statePtr, XXH_NOESCAPE const void* input, size_t length);


XXH_PUBLIC_API XXH_PUREF XXH64_hash_t XXH64_digest (XXH_NOESCAPE const XXH64_state_t* statePtr);
#endif


typedef struct { unsigned char digest[sizeof(XXH64_hash_t)]; } XXH64_canonical_t;


XXH_PUBLIC_API void XXH64_canonicalFromHash(XXH_NOESCAPE XXH64_canonical_t* dst, XXH64_hash_t hash);


XXH_PUBLIC_API XXH_PUREF XXH64_hash_t XXH64_hashFromCanonical(XXH_NOESCAPE const XXH64_canonical_t* src);

#ifndef XXH_NO_XXH3


XXH_PUBLIC_API XXH_PUREF XXH64_hash_t XXH3_64bits(XXH_NOESCAPE const void* input, size_t length);


XXH_PUBLIC_API XXH_PUREF XXH64_hash_t XXH3_64bits_withSeed(XXH_NOESCAPE const void* input, size_t length, XXH64_hash_t seed);


#define XXH3_SECRET_SIZE_MIN 136


XXH_PUBLIC_API XXH_PUREF XXH64_hash_t XXH3_64bits_withSecret(XXH_NOESCAPE const void* data, size_t len, XXH_NOESCAPE const void* secret, size_t secretSize);


#ifndef XXH_NO_STREAM


typedef struct XXH3_state_s XXH3_state_t;
XXH_PUBLIC_API XXH_MALLOCF XXH3_state_t* XXH3_createState(void);
XXH_PUBLIC_API XXH_errorcode XXH3_freeState(XXH3_state_t* statePtr);


XXH_PUBLIC_API void XXH3_copyState(XXH_NOESCAPE XXH3_state_t* dst_state, XXH_NOESCAPE const XXH3_state_t* src_state);


XXH_PUBLIC_API XXH_errorcode XXH3_64bits_reset(XXH_NOESCAPE XXH3_state_t* statePtr);


XXH_PUBLIC_API XXH_errorcode XXH3_64bits_reset_withSeed(XXH_NOESCAPE XXH3_state_t* statePtr, XXH64_hash_t seed);


XXH_PUBLIC_API XXH_errorcode XXH3_64bits_reset_withSecret(XXH_NOESCAPE XXH3_state_t* statePtr, XXH_NOESCAPE const void* secret, size_t secretSize);


XXH_PUBLIC_API XXH_errorcode XXH3_64bits_update (XXH_NOESCAPE XXH3_state_t* statePtr, XXH_NOESCAPE const void* input, size_t length);


XXH_PUBLIC_API XXH_PUREF XXH64_hash_t  XXH3_64bits_digest (XXH_NOESCAPE const XXH3_state_t* statePtr);
#endif


typedef struct {
    XXH64_hash_t low64;
    XXH64_hash_t high64;
} XXH128_hash_t;


XXH_PUBLIC_API XXH_PUREF XXH128_hash_t XXH3_128bits(XXH_NOESCAPE const void* data, size_t len);

XXH_PUBLIC_API XXH_PUREF XXH128_hash_t XXH3_128bits_withSeed(XXH_NOESCAPE const void* data, size_t len, XXH64_hash_t seed);

XXH_PUBLIC_API XXH_PUREF XXH128_hash_t XXH3_128bits_withSecret(XXH_NOESCAPE const void* data, size_t len, XXH_NOESCAPE const void* secret, size_t secretSize);


#ifndef XXH_NO_STREAM


XXH_PUBLIC_API XXH_errorcode XXH3_128bits_reset(XXH_NOESCAPE XXH3_state_t* statePtr);


XXH_PUBLIC_API XXH_errorcode XXH3_128bits_reset_withSeed(XXH_NOESCAPE XXH3_state_t* statePtr, XXH64_hash_t seed);

XXH_PUBLIC_API XXH_errorcode XXH3_128bits_reset_withSecret(XXH_NOESCAPE XXH3_state_t* statePtr, XXH_NOESCAPE const void* secret, size_t secretSize);


XXH_PUBLIC_API XXH_errorcode XXH3_128bits_update (XXH_NOESCAPE XXH3_state_t* statePtr, XXH_NOESCAPE const void* input, size_t length);


XXH_PUBLIC_API XXH_PUREF XXH128_hash_t XXH3_128bits_digest (XXH_NOESCAPE const XXH3_state_t* statePtr);
#endif


XXH_PUBLIC_API XXH_PUREF int XXH128_isEqual(XXH128_hash_t h1, XXH128_hash_t h2);


XXH_PUBLIC_API XXH_PUREF int XXH128_cmp(XXH_NOESCAPE const void* h128_1, XXH_NOESCAPE const void* h128_2);


typedef struct { unsigned char digest[sizeof(XXH128_hash_t)]; } XXH128_canonical_t;


XXH_PUBLIC_API void XXH128_canonicalFromHash(XXH_NOESCAPE XXH128_canonical_t* dst, XXH128_hash_t hash);


XXH_PUBLIC_API XXH_PUREF XXH128_hash_t XXH128_hashFromCanonical(XXH_NOESCAPE const XXH128_canonical_t* src);


#endif

#if defined (__cplusplus)
}
#endif

#endif


#endif


#if defined(XXH_STATIC_LINKING_ONLY) && !defined(XXHASH_H_STATIC_13879238742)
#define XXHASH_H_STATIC_13879238742


struct XXH32_state_s {
   XXH32_hash_t total_len_32;
   XXH32_hash_t large_len;
   XXH32_hash_t v[4];
   XXH32_hash_t mem32[4];
   XXH32_hash_t memsize;
   XXH32_hash_t reserved;
};


#ifndef XXH_NO_LONG_LONG


struct XXH64_state_s {
   XXH64_hash_t total_len;
   XXH64_hash_t v[4];
   XXH64_hash_t mem64[4];
   XXH32_hash_t memsize;
   XXH32_hash_t reserved32;
   XXH64_hash_t reserved64;
};

#ifndef XXH_NO_XXH3

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  include <stdalign.h>
#  define XXH_ALIGN(n)      alignas(n)
#elif defined(__cplusplus) && (__cplusplus >= 201103L)

#  define XXH_ALIGN(n)      alignas(n)
#elif defined(__GNUC__)
#  define XXH_ALIGN(n)      __attribute__ ((aligned(n)))
#elif defined(_MSC_VER)
#  define XXH_ALIGN(n)      __declspec(align(n))
#else
#  define XXH_ALIGN(n)
#endif


#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L))    \
    && ! (defined(__cplusplus) && (__cplusplus >= 201103L))  \
    && defined(__GNUC__)
#   define XXH_ALIGN_MEMBER(align, type) type XXH_ALIGN(align)
#else
#   define XXH_ALIGN_MEMBER(align, type) XXH_ALIGN(align) type
#endif


#define XXH3_INTERNALBUFFER_SIZE 256


#define XXH3_SECRET_DEFAULT_SIZE 192


struct XXH3_state_s {
   XXH_ALIGN_MEMBER(64, XXH64_hash_t acc[8]);

   XXH_ALIGN_MEMBER(64, unsigned char customSecret[XXH3_SECRET_DEFAULT_SIZE]);

   XXH_ALIGN_MEMBER(64, unsigned char buffer[XXH3_INTERNALBUFFER_SIZE]);

   XXH32_hash_t bufferedSize;

   XXH32_hash_t useSeed;

   size_t nbStripesSoFar;

   XXH64_hash_t totalLen;

   size_t nbStripesPerBlock;

   size_t secretLimit;

   XXH64_hash_t seed;

   XXH64_hash_t reserved64;

   const unsigned char* extSecret;


};

#undef XXH_ALIGN_MEMBER


#define XXH3_INITSTATE(XXH3_state_ptr)                       \
    do {                                                     \
        XXH3_state_t* tmp_xxh3_state_ptr = (XXH3_state_ptr); \
        tmp_xxh3_state_ptr->seed = 0;                        \
        tmp_xxh3_state_ptr->extSecret = NULL;                \
    } while(0)


#if defined (__cplusplus)
extern "C" {
#endif


XXH_PUBLIC_API XXH_PUREF XXH128_hash_t XXH128(XXH_NOESCAPE const void* data, size_t len, XXH64_hash_t seed);


XXH_PUBLIC_API XXH_errorcode XXH3_generateSecret(XXH_NOESCAPE void* secretBuffer, size_t secretSize, XXH_NOESCAPE const void* customSeed, size_t customSeedSize);


XXH_PUBLIC_API void XXH3_generateSecret_fromSeed(XXH_NOESCAPE void* secretBuffer, XXH64_hash_t seed);


XXH_PUBLIC_API XXH_PUREF XXH64_hash_t
XXH3_64bits_withSecretandSeed(XXH_NOESCAPE const void* data, size_t len,
                              XXH_NOESCAPE const void* secret, size_t secretSize,
                              XXH64_hash_t seed);

XXH_PUBLIC_API XXH_PUREF XXH128_hash_t
XXH3_128bits_withSecretandSeed(XXH_NOESCAPE const void* input, size_t length,
                               XXH_NOESCAPE const void* secret, size_t secretSize,
                               XXH64_hash_t seed64);
#ifndef XXH_NO_STREAM

XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_reset_withSecretandSeed(XXH_NOESCAPE XXH3_state_t* statePtr,
                                    XXH_NOESCAPE const void* secret, size_t secretSize,
                                    XXH64_hash_t seed64);

XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_reset_withSecretandSeed(XXH_NOESCAPE XXH3_state_t* statePtr,
                                     XXH_NOESCAPE const void* secret, size_t secretSize,
                                     XXH64_hash_t seed64);
#endif

#if defined (__cplusplus)
}
#endif

#endif
#endif

#if defined(XXH_INLINE_ALL) || defined(XXH_PRIVATE_API)
#  define XXH_IMPLEMENTATION
#endif

#endif


#if ( defined(XXH_INLINE_ALL) || defined(XXH_PRIVATE_API) \
   || defined(XXH_IMPLEMENTATION) ) && !defined(XXH_IMPLEM_13a8737387)
#  define XXH_IMPLEM_13a8737387


#ifdef XXH_DOXYGEN

#  define XXH_NO_LONG_LONG
#  undef XXH_NO_LONG_LONG

#  define XXH_FORCE_MEMORY_ACCESS 0


#  define XXH_SIZE_OPT 0


#  define XXH_FORCE_ALIGN_CHECK 0


#  define XXH_NO_INLINE_HINTS 0


#  define XXH3_INLINE_SECRET 0


#  define XXH32_ENDJMP 0


#  define XXH_OLD_NAMES
#  undef XXH_OLD_NAMES


#  define XXH_NO_STREAM
#  undef XXH_NO_STREAM
#endif


#ifndef XXH_FORCE_MEMORY_ACCESS

#  if defined(__GNUC__) && !(defined(__ARM_ARCH) && __ARM_ARCH < 7 && defined(__ARM_FEATURE_UNALIGNED))
#    define XXH_FORCE_MEMORY_ACCESS 1
#  endif
#endif

#ifndef XXH_SIZE_OPT

#  if (defined(__GNUC__) || defined(__clang__)) && defined(__OPTIMIZE_SIZE__)
#    define XXH_SIZE_OPT 1
#  else
#    define XXH_SIZE_OPT 0
#  endif
#endif

#ifndef XXH_FORCE_ALIGN_CHECK

#  if XXH_SIZE_OPT >= 1 || \
      defined(__i386)  || defined(__x86_64__) || defined(__aarch64__) || defined(__ARM_FEATURE_UNALIGNED) \
   || defined(_M_IX86) || defined(_M_X64)     || defined(_M_ARM64)    || defined(_M_ARM)
#    define XXH_FORCE_ALIGN_CHECK 0
#  else
#    define XXH_FORCE_ALIGN_CHECK 1
#  endif
#endif

#ifndef XXH_NO_INLINE_HINTS
#  if XXH_SIZE_OPT >= 1 || defined(__NO_INLINE__)
#    define XXH_NO_INLINE_HINTS 1
#  else
#    define XXH_NO_INLINE_HINTS 0
#  endif
#endif

#ifndef XXH3_INLINE_SECRET
#  if (defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 12) \
     || !defined(XXH_INLINE_ALL)
#    define XXH3_INLINE_SECRET 0
#  else
#    define XXH3_INLINE_SECRET 1
#  endif
#endif

#ifndef XXH32_ENDJMP

#  define XXH32_ENDJMP 0
#endif


#include <string.h>
#include <limits.h>

#if defined(XXH_NO_STREAM)

#elif defined(XXH_NO_STDLIB)


#if defined (__cplusplus)
extern "C" {
#endif

static XXH_CONSTF void* XXH_malloc(size_t s) { (void)s; return NULL; }
static void XXH_free(void* p) { (void)p; }

#if defined (__cplusplus)
}
#endif

#else


#include <stdlib.h>

#if defined (__cplusplus)
extern "C" {
#endif

static XXH_MALLOCF void* XXH_malloc(size_t s) { return malloc(s); }


static void XXH_free(void* p) { free(p); }

#if defined (__cplusplus)
}
#endif

#endif

#if defined (__cplusplus)
extern "C" {
#endif

static void* XXH_memcpy(void* dest, const void* src, size_t size)
{
    return memcpy(dest,src,size);
}

#if defined (__cplusplus)
}
#endif


#ifdef _MSC_VER
#  pragma warning(disable : 4127)
#endif

#if XXH_NO_INLINE_HINTS
#  if defined(__GNUC__) || defined(__clang__)
#    define XXH_FORCE_INLINE static __attribute__((unused))
#  else
#    define XXH_FORCE_INLINE static
#  endif
#  define XXH_NO_INLINE static

#elif defined(__GNUC__) || defined(__clang__)
#  define XXH_FORCE_INLINE static __inline__ __attribute__((always_inline, unused))
#  define XXH_NO_INLINE static __attribute__((noinline))
#elif defined(_MSC_VER)
#  define XXH_FORCE_INLINE static __forceinline
#  define XXH_NO_INLINE static __declspec(noinline)
#elif defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#  define XXH_FORCE_INLINE static inline
#  define XXH_NO_INLINE static
#else
#  define XXH_FORCE_INLINE static
#  define XXH_NO_INLINE static
#endif

#if XXH3_INLINE_SECRET
#  define XXH3_WITH_SECRET_INLINE XXH_FORCE_INLINE
#else
#  define XXH3_WITH_SECRET_INLINE XXH_NO_INLINE
#endif


#ifndef XXH_DEBUGLEVEL
#  ifdef DEBUGLEVEL
#    define XXH_DEBUGLEVEL DEBUGLEVEL
#  else
#    define XXH_DEBUGLEVEL 0
#  endif
#endif

#if (XXH_DEBUGLEVEL>=1)
#  include <assert.h>
#  define XXH_ASSERT(c)   assert(c)
#else
#  if defined(__INTEL_COMPILER)
#    define XXH_ASSERT(c)   XXH_ASSUME((unsigned char) (c))
#  else
#    define XXH_ASSERT(c)   XXH_ASSUME(c)
#  endif
#endif


#ifndef XXH_STATIC_ASSERT
#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#    define XXH_STATIC_ASSERT_WITH_MESSAGE(c,m) do { _Static_assert((c),m); } while(0)
#  elif defined(__cplusplus) && (__cplusplus >= 201103L)
#    define XXH_STATIC_ASSERT_WITH_MESSAGE(c,m) do { static_assert((c),m); } while(0)
#  else
#    define XXH_STATIC_ASSERT_WITH_MESSAGE(c,m) do { struct xxh_sa { char x[(c) ? 1 : -1]; }; } while(0)
#  endif
#  define XXH_STATIC_ASSERT(c) XXH_STATIC_ASSERT_WITH_MESSAGE((c),#c)
#endif


#if defined(__GNUC__) || defined(__clang__)
#  define XXH_COMPILER_GUARD(var) __asm__("" : "+r" (var))
#else
#  define XXH_COMPILER_GUARD(var) ((void)0)
#endif


#if defined(__clang__) && defined(__ARM_ARCH) && !defined(__wasm__)
#  define XXH_COMPILER_GUARD_CLANG_NEON(var) __asm__("" : "+w" (var))
#else
#  define XXH_COMPILER_GUARD_CLANG_NEON(var) ((void)0)
#endif


#if !defined (__VMS) \
 && (defined (__cplusplus) \
 || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) ) )
# ifdef _AIX
#   include <inttypes.h>
# else
#   include <stdint.h>
# endif
  typedef uint8_t xxh_u8;
#else
  typedef unsigned char xxh_u8;
#endif
typedef XXH32_hash_t xxh_u32;

#ifdef XXH_OLD_NAMES
#  warning "XXH_OLD_NAMES is planned to be removed starting v0.9. If the program depends on it, consider moving away from it by employing newer type names directly"
#  define BYTE xxh_u8
#  define U8   xxh_u8
#  define U32  xxh_u32
#endif

#if defined (__cplusplus)
extern "C" {
#endif


#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))


static xxh_u32 XXH_read32(const void* memPtr) { return *(const xxh_u32*) memPtr; }

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))


#ifdef XXH_OLD_NAMES
typedef union { xxh_u32 u32; } __attribute__((packed)) unalign;
#endif
static xxh_u32 XXH_read32(const void* ptr)
{
    typedef __attribute__((aligned(1))) xxh_u32 xxh_unalign32;
    return *((const xxh_unalign32*)ptr);
}

#else


static xxh_u32 XXH_read32(const void* memPtr)
{
    xxh_u32 val;
    XXH_memcpy(&val, memPtr, sizeof(val));
    return val;
}

#endif


#ifndef XXH_CPU_LITTLE_ENDIAN

#  if defined(_WIN32)  \
     || defined(__LITTLE_ENDIAN__) \
     || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#    define XXH_CPU_LITTLE_ENDIAN 1
#  elif defined(__BIG_ENDIAN__) \
     || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define XXH_CPU_LITTLE_ENDIAN 0
#  else

static int XXH_isLittleEndian(void)
{

    const union { xxh_u32 u; xxh_u8 c[4]; } one = { 1 };
    return one.c[0];
}
#   define XXH_CPU_LITTLE_ENDIAN   XXH_isLittleEndian()
#  endif
#endif


#define XXH_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

#ifdef __has_builtin
#  define XXH_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define XXH_HAS_BUILTIN(x) 0
#endif


#if XXH_HAS_BUILTIN(__builtin_unreachable)
#  define XXH_UNREACHABLE() __builtin_unreachable()

#elif defined(_MSC_VER)
#  define XXH_UNREACHABLE() __assume(0)

#else
#  define XXH_UNREACHABLE()
#endif

#if XXH_HAS_BUILTIN(__builtin_assume)
#  define XXH_ASSUME(c) __builtin_assume(c)
#else
#  define XXH_ASSUME(c) if (!(c)) { XXH_UNREACHABLE(); }
#endif


#if !defined(NO_CLANG_BUILTIN) && XXH_HAS_BUILTIN(__builtin_rotateleft32) \
                               && XXH_HAS_BUILTIN(__builtin_rotateleft64)
#  define XXH_rotl32 __builtin_rotateleft32
#  define XXH_rotl64 __builtin_rotateleft64

#elif defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
#  define XXH_rotl64(x,r) (((x) << (r)) | ((x) >> (64 - (r))))
#endif


#if defined(_MSC_VER)
#  define XXH_swap32 _byteswap_ulong
#elif XXH_GCC_VERSION >= 403
#  define XXH_swap32 __builtin_bswap32
#else
static xxh_u32 XXH_swap32 (xxh_u32 x)
{
    return  ((x << 24) & 0xff000000 ) |
            ((x <<  8) & 0x00ff0000 ) |
            ((x >>  8) & 0x0000ff00 ) |
            ((x >> 24) & 0x000000ff );
}
#endif


typedef enum {
    XXH_aligned,
    XXH_unaligned
} XXH_alignment;


#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))

XXH_FORCE_INLINE xxh_u32 XXH_readLE32(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[0]
         | ((xxh_u32)bytePtr[1] << 8)
         | ((xxh_u32)bytePtr[2] << 16)
         | ((xxh_u32)bytePtr[3] << 24);
}

XXH_FORCE_INLINE xxh_u32 XXH_readBE32(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[3]
         | ((xxh_u32)bytePtr[2] << 8)
         | ((xxh_u32)bytePtr[1] << 16)
         | ((xxh_u32)bytePtr[0] << 24);
}

#else
XXH_FORCE_INLINE xxh_u32 XXH_readLE32(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_read32(ptr) : XXH_swap32(XXH_read32(ptr));
}

static xxh_u32 XXH_readBE32(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_swap32(XXH_read32(ptr)) : XXH_read32(ptr);
}
#endif

XXH_FORCE_INLINE xxh_u32
XXH_readLE32_align(const void* ptr, XXH_alignment align)
{
    if (align==XXH_unaligned) {
        return XXH_readLE32(ptr);
    } else {
        return XXH_CPU_LITTLE_ENDIAN ? *(const xxh_u32*)ptr : XXH_swap32(*(const xxh_u32*)ptr);
    }
}


XXH_PUBLIC_API unsigned XXH_versionNumber (void) { return XXH_VERSION_NUMBER; }


#define XXH_PRIME32_1  0x9E3779B1U
#define XXH_PRIME32_2  0x85EBCA77U
#define XXH_PRIME32_3  0xC2B2AE3DU
#define XXH_PRIME32_4  0x27D4EB2FU
#define XXH_PRIME32_5  0x165667B1U

#ifdef XXH_OLD_NAMES
#  define PRIME32_1 XXH_PRIME32_1
#  define PRIME32_2 XXH_PRIME32_2
#  define PRIME32_3 XXH_PRIME32_3
#  define PRIME32_4 XXH_PRIME32_4
#  define PRIME32_5 XXH_PRIME32_5
#endif


static xxh_u32 XXH32_round(xxh_u32 acc, xxh_u32 input)
{
    acc += input * XXH_PRIME32_2;
    acc  = XXH_rotl32(acc, 13);
    acc *= XXH_PRIME32_1;
#if (defined(__SSE4_1__) || defined(__aarch64__) || defined(__wasm_simd128__)) && !defined(XXH_ENABLE_AUTOVECTORIZE)

    XXH_COMPILER_GUARD(acc);
#endif
    return acc;
}


static xxh_u32 XXH32_avalanche(xxh_u32 hash)
{
    hash ^= hash >> 15;
    hash *= XXH_PRIME32_2;
    hash ^= hash >> 13;
    hash *= XXH_PRIME32_3;
    hash ^= hash >> 16;
    return hash;
}

#define XXH_get32bits(p) XXH_readLE32_align(p, align)


static XXH_PUREF xxh_u32
XXH32_finalize(xxh_u32 hash, const xxh_u8* ptr, size_t len, XXH_alignment align)
{
#define XXH_PROCESS1 do {                             \
    hash += (*ptr++) * XXH_PRIME32_5;                 \
    hash = XXH_rotl32(hash, 11) * XXH_PRIME32_1;      \
} while (0)

#define XXH_PROCESS4 do {                             \
    hash += XXH_get32bits(ptr) * XXH_PRIME32_3;       \
    ptr += 4;                                         \
    hash  = XXH_rotl32(hash, 17) * XXH_PRIME32_4;     \
} while (0)

    if (ptr==NULL) XXH_ASSERT(len == 0);


    if (!XXH32_ENDJMP) {
        len &= 15;
        while (len >= 4) {
            XXH_PROCESS4;
            len -= 4;
        }
        while (len > 0) {
            XXH_PROCESS1;
            --len;
        }
        return XXH32_avalanche(hash);
    } else {
         switch(len&15)  {
           case 12:      XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 8:       XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 4:       XXH_PROCESS4;
                         return XXH32_avalanche(hash);

           case 13:      XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 9:       XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 5:       XXH_PROCESS4;
                         XXH_PROCESS1;
                         return XXH32_avalanche(hash);

           case 14:      XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 10:      XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 6:       XXH_PROCESS4;
                         XXH_PROCESS1;
                         XXH_PROCESS1;
                         return XXH32_avalanche(hash);

           case 15:      XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 11:      XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 7:       XXH_PROCESS4;
                         XXH_FALLTHROUGH;
           case 3:       XXH_PROCESS1;
                         XXH_FALLTHROUGH;
           case 2:       XXH_PROCESS1;
                         XXH_FALLTHROUGH;
           case 1:       XXH_PROCESS1;
                         XXH_FALLTHROUGH;
           case 0:       return XXH32_avalanche(hash);
        }
        XXH_ASSERT(0);
        return hash;
    }
}

#ifdef XXH_OLD_NAMES
#  define PROCESS1 XXH_PROCESS1
#  define PROCESS4 XXH_PROCESS4
#else
#  undef XXH_PROCESS1
#  undef XXH_PROCESS4
#endif


XXH_FORCE_INLINE XXH_PUREF xxh_u32
XXH32_endian_align(const xxh_u8* input, size_t len, xxh_u32 seed, XXH_alignment align)
{
    xxh_u32 h32;

    if (input==NULL) XXH_ASSERT(len == 0);

    if (len>=16) {
        const xxh_u8* const bEnd = input + len;
        const xxh_u8* const limit = bEnd - 15;
        xxh_u32 v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
        xxh_u32 v2 = seed + XXH_PRIME32_2;
        xxh_u32 v3 = seed + 0;
        xxh_u32 v4 = seed - XXH_PRIME32_1;

        do {
            v1 = XXH32_round(v1, XXH_get32bits(input)); input += 4;
            v2 = XXH32_round(v2, XXH_get32bits(input)); input += 4;
            v3 = XXH32_round(v3, XXH_get32bits(input)); input += 4;
            v4 = XXH32_round(v4, XXH_get32bits(input)); input += 4;
        } while (input < limit);

        h32 = XXH_rotl32(v1, 1)  + XXH_rotl32(v2, 7)
            + XXH_rotl32(v3, 12) + XXH_rotl32(v4, 18);
    } else {
        h32  = seed + XXH_PRIME32_5;
    }

    h32 += (xxh_u32)len;

    return XXH32_finalize(h32, input, len&15, align);
}


XXH_PUBLIC_API XXH32_hash_t XXH32 (const void* input, size_t len, XXH32_hash_t seed)
{
#if !defined(XXH_NO_STREAM) && XXH_SIZE_OPT >= 2

    XXH32_state_t state;
    XXH32_reset(&state, seed);
    XXH32_update(&state, (const xxh_u8*)input, len);
    return XXH32_digest(&state);
#else
    if (XXH_FORCE_ALIGN_CHECK) {
        if ((((size_t)input) & 3) == 0) {
            return XXH32_endian_align((const xxh_u8*)input, len, seed, XXH_aligned);
    }   }

    return XXH32_endian_align((const xxh_u8*)input, len, seed, XXH_unaligned);
#endif
}


#ifndef XXH_NO_STREAM

XXH_PUBLIC_API XXH32_state_t* XXH32_createState(void)
{
    return (XXH32_state_t*)XXH_malloc(sizeof(XXH32_state_t));
}

XXH_PUBLIC_API XXH_errorcode XXH32_freeState(XXH32_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_OK;
}


XXH_PUBLIC_API void XXH32_copyState(XXH32_state_t* dstState, const XXH32_state_t* srcState)
{
    XXH_memcpy(dstState, srcState, sizeof(*dstState));
}


XXH_PUBLIC_API XXH_errorcode XXH32_reset(XXH32_state_t* statePtr, XXH32_hash_t seed)
{
    XXH_ASSERT(statePtr != NULL);
    memset(statePtr, 0, sizeof(*statePtr));
    statePtr->v[0] = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
    statePtr->v[1] = seed + XXH_PRIME32_2;
    statePtr->v[2] = seed + 0;
    statePtr->v[3] = seed - XXH_PRIME32_1;
    return XXH_OK;
}


XXH_PUBLIC_API XXH_errorcode
XXH32_update(XXH32_state_t* state, const void* input, size_t len)
{
    if (input==NULL) {
        XXH_ASSERT(len == 0);
        return XXH_OK;
    }

    {   const xxh_u8* p = (const xxh_u8*)input;
        const xxh_u8* const bEnd = p + len;

        state->total_len_32 += (XXH32_hash_t)len;
        state->large_len |= (XXH32_hash_t)((len>=16) | (state->total_len_32>=16));

        if (state->memsize + len < 16)  {
            XXH_memcpy((xxh_u8*)(state->mem32) + state->memsize, input, len);
            state->memsize += (XXH32_hash_t)len;
            return XXH_OK;
        }

        if (state->memsize) {
            XXH_memcpy((xxh_u8*)(state->mem32) + state->memsize, input, 16-state->memsize);
            {   const xxh_u32* p32 = state->mem32;
                state->v[0] = XXH32_round(state->v[0], XXH_readLE32(p32)); p32++;
                state->v[1] = XXH32_round(state->v[1], XXH_readLE32(p32)); p32++;
                state->v[2] = XXH32_round(state->v[2], XXH_readLE32(p32)); p32++;
                state->v[3] = XXH32_round(state->v[3], XXH_readLE32(p32));
            }
            p += 16-state->memsize;
            state->memsize = 0;
        }

        if (p <= bEnd-16) {
            const xxh_u8* const limit = bEnd - 16;

            do {
                state->v[0] = XXH32_round(state->v[0], XXH_readLE32(p)); p+=4;
                state->v[1] = XXH32_round(state->v[1], XXH_readLE32(p)); p+=4;
                state->v[2] = XXH32_round(state->v[2], XXH_readLE32(p)); p+=4;
                state->v[3] = XXH32_round(state->v[3], XXH_readLE32(p)); p+=4;
            } while (p<=limit);

        }

        if (p < bEnd) {
            XXH_memcpy(state->mem32, p, (size_t)(bEnd-p));
            state->memsize = (unsigned)(bEnd-p);
        }
    }

    return XXH_OK;
}


XXH_PUBLIC_API XXH32_hash_t XXH32_digest(const XXH32_state_t* state)
{
    xxh_u32 h32;

    if (state->large_len) {
        h32 = XXH_rotl32(state->v[0], 1)
            + XXH_rotl32(state->v[1], 7)
            + XXH_rotl32(state->v[2], 12)
            + XXH_rotl32(state->v[3], 18);
    } else {
        h32 = state->v[2]  + XXH_PRIME32_5;
    }

    h32 += state->total_len_32;

    return XXH32_finalize(h32, (const xxh_u8*)state->mem32, state->memsize, XXH_aligned);
}
#endif


XXH_PUBLIC_API void XXH32_canonicalFromHash(XXH32_canonical_t* dst, XXH32_hash_t hash)
{
    XXH_STATIC_ASSERT(sizeof(XXH32_canonical_t) == sizeof(XXH32_hash_t));
    if (XXH_CPU_LITTLE_ENDIAN) hash = XXH_swap32(hash);
    XXH_memcpy(dst, &hash, sizeof(*dst));
}

XXH_PUBLIC_API XXH32_hash_t XXH32_hashFromCanonical(const XXH32_canonical_t* src)
{
    return XXH_readBE32(src);
}


#ifndef XXH_NO_LONG_LONG


typedef XXH64_hash_t xxh_u64;

#ifdef XXH_OLD_NAMES
#  define U64 xxh_u64
#endif

#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))


static xxh_u64 XXH_read64(const void* memPtr)
{
    return *(const xxh_u64*) memPtr;
}

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))


#ifdef XXH_OLD_NAMES
typedef union { xxh_u32 u32; xxh_u64 u64; } __attribute__((packed)) unalign64;
#endif
static xxh_u64 XXH_read64(const void* ptr)
{
    typedef __attribute__((aligned(1))) xxh_u64 xxh_unalign64;
    return *((const xxh_unalign64*)ptr);
}

#else


static xxh_u64 XXH_read64(const void* memPtr)
{
    xxh_u64 val;
    XXH_memcpy(&val, memPtr, sizeof(val));
    return val;
}

#endif

#if defined(_MSC_VER)
#  define XXH_swap64 _byteswap_uint64
#elif XXH_GCC_VERSION >= 403
#  define XXH_swap64 __builtin_bswap64
#else
static xxh_u64 XXH_swap64(xxh_u64 x)
{
    return  ((x << 56) & 0xff00000000000000ULL) |
            ((x << 40) & 0x00ff000000000000ULL) |
            ((x << 24) & 0x0000ff0000000000ULL) |
            ((x << 8)  & 0x000000ff00000000ULL) |
            ((x >> 8)  & 0x00000000ff000000ULL) |
            ((x >> 24) & 0x0000000000ff0000ULL) |
            ((x >> 40) & 0x000000000000ff00ULL) |
            ((x >> 56) & 0x00000000000000ffULL);
}
#endif


#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))

XXH_FORCE_INLINE xxh_u64 XXH_readLE64(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[0]
         | ((xxh_u64)bytePtr[1] << 8)
         | ((xxh_u64)bytePtr[2] << 16)
         | ((xxh_u64)bytePtr[3] << 24)
         | ((xxh_u64)bytePtr[4] << 32)
         | ((xxh_u64)bytePtr[5] << 40)
         | ((xxh_u64)bytePtr[6] << 48)
         | ((xxh_u64)bytePtr[7] << 56);
}

XXH_FORCE_INLINE xxh_u64 XXH_readBE64(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[7]
         | ((xxh_u64)bytePtr[6] << 8)
         | ((xxh_u64)bytePtr[5] << 16)
         | ((xxh_u64)bytePtr[4] << 24)
         | ((xxh_u64)bytePtr[3] << 32)
         | ((xxh_u64)bytePtr[2] << 40)
         | ((xxh_u64)bytePtr[1] << 48)
         | ((xxh_u64)bytePtr[0] << 56);
}

#else
XXH_FORCE_INLINE xxh_u64 XXH_readLE64(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_read64(ptr) : XXH_swap64(XXH_read64(ptr));
}

static xxh_u64 XXH_readBE64(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_swap64(XXH_read64(ptr)) : XXH_read64(ptr);
}
#endif

XXH_FORCE_INLINE xxh_u64
XXH_readLE64_align(const void* ptr, XXH_alignment align)
{
    if (align==XXH_unaligned)
        return XXH_readLE64(ptr);
    else
        return XXH_CPU_LITTLE_ENDIAN ? *(const xxh_u64*)ptr : XXH_swap64(*(const xxh_u64*)ptr);
}


#define XXH_PRIME64_1  0x9E3779B185EBCA87ULL
#define XXH_PRIME64_2  0xC2B2AE3D27D4EB4FULL
#define XXH_PRIME64_3  0x165667B19E3779F9ULL
#define XXH_PRIME64_4  0x85EBCA77C2B2AE63ULL
#define XXH_PRIME64_5  0x27D4EB2F165667C5ULL

#ifdef XXH_OLD_NAMES
#  define PRIME64_1 XXH_PRIME64_1
#  define PRIME64_2 XXH_PRIME64_2
#  define PRIME64_3 XXH_PRIME64_3
#  define PRIME64_4 XXH_PRIME64_4
#  define PRIME64_5 XXH_PRIME64_5
#endif


static xxh_u64 XXH64_round(xxh_u64 acc, xxh_u64 input)
{
    acc += input * XXH_PRIME64_2;
    acc  = XXH_rotl64(acc, 31);
    acc *= XXH_PRIME64_1;
#if (defined(__AVX512F__)) && !defined(XXH_ENABLE_AUTOVECTORIZE)

    XXH_COMPILER_GUARD(acc);
#endif
    return acc;
}

static xxh_u64 XXH64_mergeRound(xxh_u64 acc, xxh_u64 val)
{
    val  = XXH64_round(0, val);
    acc ^= val;
    acc  = acc * XXH_PRIME64_1 + XXH_PRIME64_4;
    return acc;
}


static xxh_u64 XXH64_avalanche(xxh_u64 hash)
{
    hash ^= hash >> 33;
    hash *= XXH_PRIME64_2;
    hash ^= hash >> 29;
    hash *= XXH_PRIME64_3;
    hash ^= hash >> 32;
    return hash;
}


#define XXH_get64bits(p) XXH_readLE64_align(p, align)


static XXH_PUREF xxh_u64
XXH64_finalize(xxh_u64 hash, const xxh_u8* ptr, size_t len, XXH_alignment align)
{
    if (ptr==NULL) XXH_ASSERT(len == 0);
    len &= 31;
    while (len >= 8) {
        xxh_u64 const k1 = XXH64_round(0, XXH_get64bits(ptr));
        ptr += 8;
        hash ^= k1;
        hash  = XXH_rotl64(hash,27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        len -= 8;
    }
    if (len >= 4) {
        hash ^= (xxh_u64)(XXH_get32bits(ptr)) * XXH_PRIME64_1;
        ptr += 4;
        hash = XXH_rotl64(hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        len -= 4;
    }
    while (len > 0) {
        hash ^= (*ptr++) * XXH_PRIME64_5;
        hash = XXH_rotl64(hash, 11) * XXH_PRIME64_1;
        --len;
    }
    return  XXH64_avalanche(hash);
}

#ifdef XXH_OLD_NAMES
#  define PROCESS1_64 XXH_PROCESS1_64
#  define PROCESS4_64 XXH_PROCESS4_64
#  define PROCESS8_64 XXH_PROCESS8_64
#else
#  undef XXH_PROCESS1_64
#  undef XXH_PROCESS4_64
#  undef XXH_PROCESS8_64
#endif


XXH_FORCE_INLINE XXH_PUREF xxh_u64
XXH64_endian_align(const xxh_u8* input, size_t len, xxh_u64 seed, XXH_alignment align)
{
    xxh_u64 h64;
    if (input==NULL) XXH_ASSERT(len == 0);

    if (len>=32) {
        const xxh_u8* const bEnd = input + len;
        const xxh_u8* const limit = bEnd - 31;
        xxh_u64 v1 = seed + XXH_PRIME64_1 + XXH_PRIME64_2;
        xxh_u64 v2 = seed + XXH_PRIME64_2;
        xxh_u64 v3 = seed + 0;
        xxh_u64 v4 = seed - XXH_PRIME64_1;

        do {
            v1 = XXH64_round(v1, XXH_get64bits(input)); input+=8;
            v2 = XXH64_round(v2, XXH_get64bits(input)); input+=8;
            v3 = XXH64_round(v3, XXH_get64bits(input)); input+=8;
            v4 = XXH64_round(v4, XXH_get64bits(input)); input+=8;
        } while (input<limit);

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);
        h64 = XXH64_mergeRound(h64, v1);
        h64 = XXH64_mergeRound(h64, v2);
        h64 = XXH64_mergeRound(h64, v3);
        h64 = XXH64_mergeRound(h64, v4);

    } else {
        h64  = seed + XXH_PRIME64_5;
    }

    h64 += (xxh_u64) len;

    return XXH64_finalize(h64, input, len, align);
}


XXH_PUBLIC_API XXH64_hash_t XXH64 (XXH_NOESCAPE const void* input, size_t len, XXH64_hash_t seed)
{
#if !defined(XXH_NO_STREAM) && XXH_SIZE_OPT >= 2

    XXH64_state_t state;
    XXH64_reset(&state, seed);
    XXH64_update(&state, (const xxh_u8*)input, len);
    return XXH64_digest(&state);
#else
    if (XXH_FORCE_ALIGN_CHECK) {
        if ((((size_t)input) & 7)==0) {
            return XXH64_endian_align((const xxh_u8*)input, len, seed, XXH_aligned);
    }   }

    return XXH64_endian_align((const xxh_u8*)input, len, seed, XXH_unaligned);

#endif
}


#ifndef XXH_NO_STREAM

XXH_PUBLIC_API XXH64_state_t* XXH64_createState(void)
{
    return (XXH64_state_t*)XXH_malloc(sizeof(XXH64_state_t));
}

XXH_PUBLIC_API XXH_errorcode XXH64_freeState(XXH64_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_OK;
}


XXH_PUBLIC_API void XXH64_copyState(XXH_NOESCAPE XXH64_state_t* dstState, const XXH64_state_t* srcState)
{
    XXH_memcpy(dstState, srcState, sizeof(*dstState));
}


XXH_PUBLIC_API XXH_errorcode XXH64_reset(XXH_NOESCAPE XXH64_state_t* statePtr, XXH64_hash_t seed)
{
    XXH_ASSERT(statePtr != NULL);
    memset(statePtr, 0, sizeof(*statePtr));
    statePtr->v[0] = seed + XXH_PRIME64_1 + XXH_PRIME64_2;
    statePtr->v[1] = seed + XXH_PRIME64_2;
    statePtr->v[2] = seed + 0;
    statePtr->v[3] = seed - XXH_PRIME64_1;
    return XXH_OK;
}


XXH_PUBLIC_API XXH_errorcode
XXH64_update (XXH_NOESCAPE XXH64_state_t* state, XXH_NOESCAPE const void* input, size_t len)
{
    if (input==NULL) {
        XXH_ASSERT(len == 0);
        return XXH_OK;
    }

    {   const xxh_u8* p = (const xxh_u8*)input;
        const xxh_u8* const bEnd = p + len;

        state->total_len += len;

        if (state->memsize + len < 32) {
            XXH_memcpy(((xxh_u8*)state->mem64) + state->memsize, input, len);
            state->memsize += (xxh_u32)len;
            return XXH_OK;
        }

        if (state->memsize) {
            XXH_memcpy(((xxh_u8*)state->mem64) + state->memsize, input, 32-state->memsize);
            state->v[0] = XXH64_round(state->v[0], XXH_readLE64(state->mem64+0));
            state->v[1] = XXH64_round(state->v[1], XXH_readLE64(state->mem64+1));
            state->v[2] = XXH64_round(state->v[2], XXH_readLE64(state->mem64+2));
            state->v[3] = XXH64_round(state->v[3], XXH_readLE64(state->mem64+3));
            p += 32 - state->memsize;
            state->memsize = 0;
        }

        if (p+32 <= bEnd) {
            const xxh_u8* const limit = bEnd - 32;

            do {
                state->v[0] = XXH64_round(state->v[0], XXH_readLE64(p)); p+=8;
                state->v[1] = XXH64_round(state->v[1], XXH_readLE64(p)); p+=8;
                state->v[2] = XXH64_round(state->v[2], XXH_readLE64(p)); p+=8;
                state->v[3] = XXH64_round(state->v[3], XXH_readLE64(p)); p+=8;
            } while (p<=limit);

        }

        if (p < bEnd) {
            XXH_memcpy(state->mem64, p, (size_t)(bEnd-p));
            state->memsize = (unsigned)(bEnd-p);
        }
    }

    return XXH_OK;
}


XXH_PUBLIC_API XXH64_hash_t XXH64_digest(XXH_NOESCAPE const XXH64_state_t* state)
{
    xxh_u64 h64;

    if (state->total_len >= 32) {
        h64 = XXH_rotl64(state->v[0], 1) + XXH_rotl64(state->v[1], 7) + XXH_rotl64(state->v[2], 12) + XXH_rotl64(state->v[3], 18);
        h64 = XXH64_mergeRound(h64, state->v[0]);
        h64 = XXH64_mergeRound(h64, state->v[1]);
        h64 = XXH64_mergeRound(h64, state->v[2]);
        h64 = XXH64_mergeRound(h64, state->v[3]);
    } else {
        h64  = state->v[2]  + XXH_PRIME64_5;
    }

    h64 += (xxh_u64) state->total_len;

    return XXH64_finalize(h64, (const xxh_u8*)state->mem64, (size_t)state->total_len, XXH_aligned);
}
#endif


XXH_PUBLIC_API void XXH64_canonicalFromHash(XXH_NOESCAPE XXH64_canonical_t* dst, XXH64_hash_t hash)
{
    XXH_STATIC_ASSERT(sizeof(XXH64_canonical_t) == sizeof(XXH64_hash_t));
    if (XXH_CPU_LITTLE_ENDIAN) hash = XXH_swap64(hash);
    XXH_memcpy(dst, &hash, sizeof(*dst));
}


XXH_PUBLIC_API XXH64_hash_t XXH64_hashFromCanonical(XXH_NOESCAPE const XXH64_canonical_t* src)
{
    return XXH_readBE64(src);
}

#if defined (__cplusplus)
}
#endif

#ifndef XXH_NO_XXH3


#if ((defined(sun) || defined(__sun)) && __cplusplus)
#  define XXH_RESTRICT
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define XXH_RESTRICT   restrict
#elif (defined (__GNUC__) && ((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))) \
   || (defined (__clang__)) \
   || (defined (_MSC_VER) && (_MSC_VER >= 1400)) \
   || (defined (__INTEL_COMPILER) && (__INTEL_COMPILER >= 1300))

#  define XXH_RESTRICT   __restrict
#else
#  define XXH_RESTRICT
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 3))  \
  || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) \
  || defined(__clang__)
#    define XXH_likely(x) __builtin_expect(x, 1)
#    define XXH_unlikely(x) __builtin_expect(x, 0)
#else
#    define XXH_likely(x) (x)
#    define XXH_unlikely(x) (x)
#endif

#ifndef XXH_HAS_INCLUDE
#  ifdef __has_include

#    define XXH_HAS_INCLUDE __has_include
#  else
#    define XXH_HAS_INCLUDE(x) 0
#  endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#  if defined(__ARM_FEATURE_SVE)
#    include <arm_sve.h>
#  endif
#  if defined(__ARM_NEON__) || defined(__ARM_NEON) \
   || (defined(_M_ARM) && _M_ARM >= 7) \
   || defined(_M_ARM64) || defined(_M_ARM64EC) \
   || (defined(__wasm_simd128__) && XXH_HAS_INCLUDE(<arm_neon.h>))
#    define inline __inline__
#    include <arm_neon.h>
#    undef inline
#  elif defined(__AVX2__)
#    include <immintrin.h>
#  elif defined(__SSE2__)
#    include <emmintrin.h>
#  endif
#endif

#if defined(_MSC_VER)
#  include <intrin.h>
#endif


#if defined(__thumb__) && !defined(__thumb2__) && defined(__ARM_ARCH_ISA_ARM)
#   warning "XXH3 is highly inefficient without ARM or Thumb-2."
#endif


#ifdef XXH_DOXYGEN

#  define XXH_VECTOR XXH_SCALAR

enum XXH_VECTOR_TYPE  {
    XXH_SCALAR = 0,
    XXH_SSE2   = 1,
    XXH_AVX2   = 2,
    XXH_AVX512 = 3,
    XXH_NEON   = 4,
    XXH_VSX    = 5,
    XXH_SVE    = 6,
};

#  define XXH_ACC_ALIGN 8
#endif


#ifndef XXH_DOXYGEN
#  define XXH_SCALAR 0
#  define XXH_SSE2   1
#  define XXH_AVX2   2
#  define XXH_AVX512 3
#  define XXH_NEON   4
#  define XXH_VSX    5
#  define XXH_SVE    6
#endif

#ifndef XXH_VECTOR
#  if defined(__ARM_FEATURE_SVE)
#    define XXH_VECTOR XXH_SVE
#  elif ( \
        defined(__ARM_NEON__) || defined(__ARM_NEON)  \
     || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)  \
     || (defined(__wasm_simd128__) && XXH_HAS_INCLUDE(<arm_neon.h>))  \
   ) && ( \
        defined(_WIN32) || defined(__LITTLE_ENDIAN__)  \
    || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) \
   )
#    define XXH_VECTOR XXH_NEON
#  elif defined(__AVX512F__)
#    define XXH_VECTOR XXH_AVX512
#  elif defined(__AVX2__)
#    define XXH_VECTOR XXH_AVX2
#  elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))
#    define XXH_VECTOR XXH_SSE2
#  elif (defined(__PPC64__) && defined(__POWER8_VECTOR__)) \
     || (defined(__s390x__) && defined(__VEC__)) \
     && defined(__GNUC__)
#    define XXH_VECTOR XXH_VSX
#  else
#    define XXH_VECTOR XXH_SCALAR
#  endif
#endif


#if (XXH_VECTOR == XXH_SVE) && !defined(__ARM_FEATURE_SVE)
#  ifdef _MSC_VER
#    pragma warning(once : 4606)
#  else
#    warning "__ARM_FEATURE_SVE isn't supported. Use SCALAR instead."
#  endif
#  undef XXH_VECTOR
#  define XXH_VECTOR XXH_SCALAR
#endif


#ifndef XXH_ACC_ALIGN
#  if defined(XXH_X86DISPATCH)
#     define XXH_ACC_ALIGN 64
#  elif XXH_VECTOR == XXH_SCALAR
#     define XXH_ACC_ALIGN 8
#  elif XXH_VECTOR == XXH_SSE2
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_AVX2
#     define XXH_ACC_ALIGN 32
#  elif XXH_VECTOR == XXH_NEON
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_VSX
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_AVX512
#     define XXH_ACC_ALIGN 64
#  elif XXH_VECTOR == XXH_SVE
#     define XXH_ACC_ALIGN 64
#  endif
#endif

#if defined(XXH_X86DISPATCH) || XXH_VECTOR == XXH_SSE2 \
    || XXH_VECTOR == XXH_AVX2 || XXH_VECTOR == XXH_AVX512
#  define XXH_SEC_ALIGN XXH_ACC_ALIGN
#elif XXH_VECTOR == XXH_SVE
#  define XXH_SEC_ALIGN XXH_ACC_ALIGN
#else
#  define XXH_SEC_ALIGN 8
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define XXH_ALIASING __attribute__((may_alias))
#else
#  define XXH_ALIASING
#endif


#if XXH_VECTOR == XXH_AVX2  \
  && defined(__GNUC__) && !defined(__clang__)  \
  && defined(__OPTIMIZE__) && XXH_SIZE_OPT <= 0
#  pragma GCC push_options
#  pragma GCC optimize("-O2")
#endif

#if defined (__cplusplus)
extern "C" {
#endif

#if XXH_VECTOR == XXH_NEON


typedef uint64x2_t xxh_aliasing_uint64x2_t XXH_ALIASING;


#if defined(__aarch64__) && defined(__GNUC__) && !defined(__clang__)
XXH_FORCE_INLINE uint64x2_t XXH_vld1q_u64(void const* ptr)
{
    return *(xxh_aliasing_uint64x2_t const *)ptr;
}
#else
XXH_FORCE_INLINE uint64x2_t XXH_vld1q_u64(void const* ptr)
{
    return vreinterpretq_u64_u8(vld1q_u8((uint8_t const*)ptr));
}
#endif


#if defined(__aarch64__) && defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 11
XXH_FORCE_INLINE uint64x2_t
XXH_vmlal_low_u32(uint64x2_t acc, uint32x4_t lhs, uint32x4_t rhs)
{

    __asm__("umlal   %0.2d, %1.2s, %2.2s" : "+w" (acc) : "w" (lhs), "w" (rhs));
    return acc;
}
XXH_FORCE_INLINE uint64x2_t
XXH_vmlal_high_u32(uint64x2_t acc, uint32x4_t lhs, uint32x4_t rhs)
{

    return vmlal_high_u32(acc, lhs, rhs);
}
#else

XXH_FORCE_INLINE uint64x2_t
XXH_vmlal_low_u32(uint64x2_t acc, uint32x4_t lhs, uint32x4_t rhs)
{
    return vmlal_u32(acc, vget_low_u32(lhs), vget_low_u32(rhs));
}

XXH_FORCE_INLINE uint64x2_t
XXH_vmlal_high_u32(uint64x2_t acc, uint32x4_t lhs, uint32x4_t rhs)
{
    return vmlal_u32(acc, vget_high_u32(lhs), vget_high_u32(rhs));
}
#endif


# ifndef XXH3_NEON_LANES
#  if (defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64) || defined(_M_ARM64EC)) \
   && !defined(__APPLE__) && XXH_SIZE_OPT <= 0
#   define XXH3_NEON_LANES 6
#  else
#   define XXH3_NEON_LANES XXH_ACC_NB
#  endif
# endif
#endif

#if defined (__cplusplus)
}
#endif


#if XXH_VECTOR == XXH_VSX

#  pragma push_macro("bool")
#  pragma push_macro("vector")
#  pragma push_macro("pixel")

#  undef bool
#  undef vector
#  undef pixel

#  if defined(__s390x__)
#    include <s390intrin.h>
#  else
#    include <altivec.h>
#  endif


#  pragma pop_macro("pixel")
#  pragma pop_macro("vector")
#  pragma pop_macro("bool")

typedef __vector unsigned long long xxh_u64x2;
typedef __vector unsigned char xxh_u8x16;
typedef __vector unsigned xxh_u32x4;


typedef xxh_u64x2 xxh_aliasing_u64x2 XXH_ALIASING;

# ifndef XXH_VSX_BE
#  if defined(__BIG_ENDIAN__) \
  || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define XXH_VSX_BE 1
#  elif defined(__VEC_ELEMENT_REG_ORDER__) && __VEC_ELEMENT_REG_ORDER__ == __ORDER_BIG_ENDIAN__
#    warning "-maltivec=be is not recommended. Please use native endianness."
#    define XXH_VSX_BE 1
#  else
#    define XXH_VSX_BE 0
#  endif
# endif

# if XXH_VSX_BE
#  if defined(__POWER9_VECTOR__) || (defined(__clang__) && defined(__s390x__))
#    define XXH_vec_revb vec_revb
#  else
#if defined (__cplusplus)
extern "C" {
#endif

XXH_FORCE_INLINE xxh_u64x2 XXH_vec_revb(xxh_u64x2 val)
{
    xxh_u8x16 const vByteSwap = { 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
                                  0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08 };
    return vec_perm(val, val, vByteSwap);
}
#if defined (__cplusplus)
}
#endif
#  endif
# endif

#if defined (__cplusplus)
extern "C" {
#endif

XXH_FORCE_INLINE xxh_u64x2 XXH_vec_loadu(const void *ptr)
{
    xxh_u64x2 ret;
    XXH_memcpy(&ret, ptr, sizeof(xxh_u64x2));
# if XXH_VSX_BE
    ret = XXH_vec_revb(ret);
# endif
    return ret;
}


# if defined(__s390x__)

#  define XXH_vec_mulo vec_mulo
#  define XXH_vec_mule vec_mule
# elif defined(__clang__) && XXH_HAS_BUILTIN(__builtin_altivec_vmuleuw) && !defined(__ibmxl__)


#  define XXH_vec_mulo __builtin_altivec_vmulouw
#  define XXH_vec_mule __builtin_altivec_vmuleuw
# else


XXH_FORCE_INLINE xxh_u64x2 XXH_vec_mulo(xxh_u32x4 a, xxh_u32x4 b)
{
    xxh_u64x2 result;
    __asm__("vmulouw %0, %1, %2" : "=v" (result) : "v" (a), "v" (b));
    return result;
}
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_mule(xxh_u32x4 a, xxh_u32x4 b)
{
    xxh_u64x2 result;
    __asm__("vmuleuw %0, %1, %2" : "=v" (result) : "v" (a), "v" (b));
    return result;
}
# endif

#if defined (__cplusplus)
}
#endif

#endif

#if XXH_VECTOR == XXH_SVE
#define ACCRND(acc, offset) \
do { \
    svuint64_t input_vec = svld1_u64(mask, xinput + offset);         \
    svuint64_t secret_vec = svld1_u64(mask, xsecret + offset);       \
    svuint64_t mixed = sveor_u64_x(mask, secret_vec, input_vec);     \
    svuint64_t swapped = svtbl_u64(input_vec, kSwap);                \
    svuint64_t mixed_lo = svextw_u64_x(mask, mixed);                 \
    svuint64_t mixed_hi = svlsr_n_u64_x(mask, mixed, 32);            \
    svuint64_t mul = svmad_u64_x(mask, mixed_lo, mixed_hi, swapped); \
    acc = svadd_u64_x(mask, acc, mul);                               \
} while (0)
#endif


#if defined(XXH_NO_PREFETCH)
#  define XXH_PREFETCH(ptr)  (void)(ptr)
#else
#  if XXH_SIZE_OPT >= 1
#    define XXH_PREFETCH(ptr) (void)(ptr)
#  elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#    include <mmintrin.h>
#    define XXH_PREFETCH(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T0)
#  elif defined(__GNUC__) && ( (__GNUC__ >= 4) || ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) ) )
#    define XXH_PREFETCH(ptr)  __builtin_prefetch((ptr), 0 , 3 )
#  else
#    define XXH_PREFETCH(ptr) (void)(ptr)
#  endif
#endif

#if defined (__cplusplus)
extern "C" {
#endif


#define XXH_SECRET_DEFAULT_SIZE 192

#if (XXH_SECRET_DEFAULT_SIZE < XXH3_SECRET_SIZE_MIN)
#  error "default keyset is not large enough"
#endif


XXH_ALIGN(64) static const xxh_u8 XXH3_kSecret[XXH_SECRET_DEFAULT_SIZE] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};

static const xxh_u64 PRIME_MX1 = 0x165667919E3779F9ULL;
static const xxh_u64 PRIME_MX2 = 0x9FB21C651E98DF25ULL;

#ifdef XXH_OLD_NAMES
#  define kSecret XXH3_kSecret
#endif

#ifdef XXH_DOXYGEN

XXH_FORCE_INLINE xxh_u64
XXH_mult32to64(xxh_u64 x, xxh_u64 y)
{
   return (x & 0xFFFFFFFF) * (y & 0xFFFFFFFF);
}
#elif defined(_MSC_VER) && defined(_M_IX86)
#    define XXH_mult32to64(x, y) __emulu((unsigned)(x), (unsigned)(y))
#else

#    define XXH_mult32to64(x, y) ((xxh_u64)(xxh_u32)(x) * (xxh_u64)(xxh_u32)(y))
#endif


static XXH128_hash_t
XXH_mult64to128(xxh_u64 lhs, xxh_u64 rhs)
{

#if (defined(__GNUC__) || defined(__clang__)) && !defined(__wasm__) \
    && defined(__SIZEOF_INT128__) \
    || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 128)

    __uint128_t const product = (__uint128_t)lhs * (__uint128_t)rhs;
    XXH128_hash_t r128;
    r128.low64  = (xxh_u64)(product);
    r128.high64 = (xxh_u64)(product >> 64);
    return r128;


#elif (defined(_M_X64) || defined(_M_IA64)) && !defined(_M_ARM64EC)

#ifndef _MSC_VER
#   pragma intrinsic(_umul128)
#endif
    xxh_u64 product_high;
    xxh_u64 const product_low = _umul128(lhs, rhs, &product_high);
    XXH128_hash_t r128;
    r128.low64  = product_low;
    r128.high64 = product_high;
    return r128;


#elif defined(_M_ARM64) || defined(_M_ARM64EC)

#ifndef _MSC_VER
#   pragma intrinsic(__umulh)
#endif
    XXH128_hash_t r128;
    r128.low64  = lhs * rhs;
    r128.high64 = __umulh(lhs, rhs);
    return r128;

#else


    xxh_u64 const lo_lo = XXH_mult32to64(lhs & 0xFFFFFFFF, rhs & 0xFFFFFFFF);
    xxh_u64 const hi_lo = XXH_mult32to64(lhs >> 32,        rhs & 0xFFFFFFFF);
    xxh_u64 const lo_hi = XXH_mult32to64(lhs & 0xFFFFFFFF, rhs >> 32);
    xxh_u64 const hi_hi = XXH_mult32to64(lhs >> 32,        rhs >> 32);


    xxh_u64 const cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    xxh_u64 const upper = (hi_lo >> 32) + (cross >> 32)        + hi_hi;
    xxh_u64 const lower = (cross << 32) | (lo_lo & 0xFFFFFFFF);

    XXH128_hash_t r128;
    r128.low64  = lower;
    r128.high64 = upper;
    return r128;
#endif
}


static xxh_u64
XXH3_mul128_fold64(xxh_u64 lhs, xxh_u64 rhs)
{
    XXH128_hash_t product = XXH_mult64to128(lhs, rhs);
    return product.low64 ^ product.high64;
}


XXH_FORCE_INLINE XXH_CONSTF xxh_u64 XXH_xorshift64(xxh_u64 v64, int shift)
{
    XXH_ASSERT(0 <= shift && shift < 64);
    return v64 ^ (v64 >> shift);
}


static XXH64_hash_t XXH3_avalanche(xxh_u64 h64)
{
    h64 = XXH_xorshift64(h64, 37);
    h64 *= PRIME_MX1;
    h64 = XXH_xorshift64(h64, 32);
    return h64;
}


static XXH64_hash_t XXH3_rrmxmx(xxh_u64 h64, xxh_u64 len)
{

    h64 ^= XXH_rotl64(h64, 49) ^ XXH_rotl64(h64, 24);
    h64 *= PRIME_MX2;
    h64 ^= (h64 >> 35) + len ;
    h64 *= PRIME_MX2;
    return XXH_xorshift64(h64, 28);
}


XXH_FORCE_INLINE XXH_PUREF XXH64_hash_t
XXH3_len_1to3_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(1 <= len && len <= 3);
    XXH_ASSERT(secret != NULL);

    {   xxh_u8  const c1 = input[0];
        xxh_u8  const c2 = input[len >> 1];
        xxh_u8  const c3 = input[len - 1];
        xxh_u32 const combined = ((xxh_u32)c1 << 16) | ((xxh_u32)c2  << 24)
                               | ((xxh_u32)c3 <<  0) | ((xxh_u32)len << 8);
        xxh_u64 const bitflip = (XXH_readLE32(secret) ^ XXH_readLE32(secret+4)) + seed;
        xxh_u64 const keyed = (xxh_u64)combined ^ bitflip;
        return XXH64_avalanche(keyed);
    }
}

XXH_FORCE_INLINE XXH_PUREF XXH64_hash_t
XXH3_len_4to8_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(secret != NULL);
    XXH_ASSERT(4 <= len && len <= 8);
    seed ^= (xxh_u64)XXH_swap32((xxh_u32)seed) << 32;
    {   xxh_u32 const input1 = XXH_readLE32(input);
        xxh_u32 const input2 = XXH_readLE32(input + len - 4);
        xxh_u64 const bitflip = (XXH_readLE64(secret+8) ^ XXH_readLE64(secret+16)) - seed;
        xxh_u64 const input64 = input2 + (((xxh_u64)input1) << 32);
        xxh_u64 const keyed = input64 ^ bitflip;
        return XXH3_rrmxmx(keyed, len);
    }
}

XXH_FORCE_INLINE XXH_PUREF XXH64_hash_t
XXH3_len_9to16_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(secret != NULL);
    XXH_ASSERT(9 <= len && len <= 16);
    {   xxh_u64 const bitflip1 = (XXH_readLE64(secret+24) ^ XXH_readLE64(secret+32)) + seed;
        xxh_u64 const bitflip2 = (XXH_readLE64(secret+40) ^ XXH_readLE64(secret+48)) - seed;
        xxh_u64 const input_lo = XXH_readLE64(input)           ^ bitflip1;
        xxh_u64 const input_hi = XXH_readLE64(input + len - 8) ^ bitflip2;
        xxh_u64 const acc = len
                          + XXH_swap64(input_lo) + input_hi
                          + XXH3_mul128_fold64(input_lo, input_hi);
        return XXH3_avalanche(acc);
    }
}

XXH_FORCE_INLINE XXH_PUREF XXH64_hash_t
XXH3_len_0to16_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(len <= 16);
    {   if (XXH_likely(len >  8)) return XXH3_len_9to16_64b(input, len, secret, seed);
        if (XXH_likely(len >= 4)) return XXH3_len_4to8_64b(input, len, secret, seed);
        if (len) return XXH3_len_1to3_64b(input, len, secret, seed);
        return XXH64_avalanche(seed ^ (XXH_readLE64(secret+56) ^ XXH_readLE64(secret+64)));
    }
}


XXH_FORCE_INLINE xxh_u64 XXH3_mix16B(const xxh_u8* XXH_RESTRICT input,
                                     const xxh_u8* XXH_RESTRICT secret, xxh_u64 seed64)
{
#if defined(__GNUC__) && !defined(__clang__)  \
  && defined(__i386__) && defined(__SSE2__)   \
  && !defined(XXH_ENABLE_AUTOVECTORIZE)

    XXH_COMPILER_GUARD(seed64);
#endif
    {   xxh_u64 const input_lo = XXH_readLE64(input);
        xxh_u64 const input_hi = XXH_readLE64(input+8);
        return XXH3_mul128_fold64(
            input_lo ^ (XXH_readLE64(secret)   + seed64),
            input_hi ^ (XXH_readLE64(secret+8) - seed64)
        );
    }
}


XXH_FORCE_INLINE XXH_PUREF XXH64_hash_t
XXH3_len_17to128_64b(const xxh_u8* XXH_RESTRICT input, size_t len,
                     const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                     XXH64_hash_t seed)
{
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN); (void)secretSize;
    XXH_ASSERT(16 < len && len <= 128);

    {   xxh_u64 acc = len * XXH_PRIME64_1;
#if XXH_SIZE_OPT >= 1

        unsigned int i = (unsigned int)(len - 1) / 32;
        do {
            acc += XXH3_mix16B(input+16 * i, secret+32*i, seed);
            acc += XXH3_mix16B(input+len-16*(i+1), secret+32*i+16, seed);
        } while (i-- != 0);
#else
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc += XXH3_mix16B(input+48, secret+96, seed);
                    acc += XXH3_mix16B(input+len-64, secret+112, seed);
                }
                acc += XXH3_mix16B(input+32, secret+64, seed);
                acc += XXH3_mix16B(input+len-48, secret+80, seed);
            }
            acc += XXH3_mix16B(input+16, secret+32, seed);
            acc += XXH3_mix16B(input+len-32, secret+48, seed);
        }
        acc += XXH3_mix16B(input+0, secret+0, seed);
        acc += XXH3_mix16B(input+len-16, secret+16, seed);
#endif
        return XXH3_avalanche(acc);
    }
}


#define XXH3_MIDSIZE_MAX 240

XXH_NO_INLINE XXH_PUREF XXH64_hash_t
XXH3_len_129to240_64b(const xxh_u8* XXH_RESTRICT input, size_t len,
                      const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                      XXH64_hash_t seed)
{
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN); (void)secretSize;
    XXH_ASSERT(128 < len && len <= XXH3_MIDSIZE_MAX);

    #define XXH3_MIDSIZE_STARTOFFSET 3
    #define XXH3_MIDSIZE_LASTOFFSET  17

    {   xxh_u64 acc = len * XXH_PRIME64_1;
        xxh_u64 acc_end;
        unsigned int const nbRounds = (unsigned int)len / 16;
        unsigned int i;
        XXH_ASSERT(128 < len && len <= XXH3_MIDSIZE_MAX);
        for (i=0; i<8; i++) {
            acc += XXH3_mix16B(input+(16*i), secret+(16*i), seed);
        }

        acc_end = XXH3_mix16B(input + len - 16, secret + XXH3_SECRET_SIZE_MIN - XXH3_MIDSIZE_LASTOFFSET, seed);
        XXH_ASSERT(nbRounds >= 8);
        acc = XXH3_avalanche(acc);
#if defined(__clang__)                                 \
    && (defined(__ARM_NEON) || defined(__ARM_NEON__))  \
    && !defined(XXH_ENABLE_AUTOVECTORIZE)

        #pragma clang loop vectorize(disable)
#endif
        for (i=8 ; i < nbRounds; i++) {

            XXH_COMPILER_GUARD(acc);
            acc_end += XXH3_mix16B(input+(16*i), secret+(16*(i-8)) + XXH3_MIDSIZE_STARTOFFSET, seed);
        }
        return XXH3_avalanche(acc + acc_end);
    }
}


#define XXH_STRIPE_LEN 64
#define XXH_SECRET_CONSUME_RATE 8
#define XXH_ACC_NB (XXH_STRIPE_LEN / sizeof(xxh_u64))

#ifdef XXH_OLD_NAMES
#  define STRIPE_LEN XXH_STRIPE_LEN
#  define ACC_NB XXH_ACC_NB
#endif

#ifndef XXH_PREFETCH_DIST
#  ifdef __clang__
#    define XXH_PREFETCH_DIST 320
#  else
#    if (XXH_VECTOR == XXH_AVX512)
#      define XXH_PREFETCH_DIST 512
#    else
#      define XXH_PREFETCH_DIST 384
#    endif
#  endif
#endif


#define XXH3_ACCUMULATE_TEMPLATE(name)                      \
void                                                        \
XXH3_accumulate_##name(xxh_u64* XXH_RESTRICT acc,           \
                       const xxh_u8* XXH_RESTRICT input,    \
                       const xxh_u8* XXH_RESTRICT secret,   \
                       size_t nbStripes)                    \
{                                                           \
    size_t n;                                               \
    for (n = 0; n < nbStripes; n++ ) {                      \
        const xxh_u8* const in = input + n*XXH_STRIPE_LEN;  \
        XXH_PREFETCH(in + XXH_PREFETCH_DIST);               \
        XXH3_accumulate_512_##name(                         \
                 acc,                                       \
                 in,                                        \
                 secret + n*XXH_SECRET_CONSUME_RATE);       \
    }                                                       \
}


XXH_FORCE_INLINE void XXH_writeLE64(void* dst, xxh_u64 v64)
{
    if (!XXH_CPU_LITTLE_ENDIAN) v64 = XXH_swap64(v64);
    XXH_memcpy(dst, &v64, sizeof(v64));
}


#if !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) ) )
    typedef int64_t xxh_i64;
#else

    typedef long long xxh_i64;
#endif


#if (XXH_VECTOR == XXH_AVX512) \
     || (defined(XXH_DISPATCH_AVX512) && XXH_DISPATCH_AVX512 != 0)

#ifndef XXH_TARGET_AVX512
# define XXH_TARGET_AVX512
#endif

XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_accumulate_512_avx512(void* XXH_RESTRICT acc,
                     const void* XXH_RESTRICT input,
                     const void* XXH_RESTRICT secret)
{
    __m512i* const xacc = (__m512i *) acc;
    XXH_ASSERT((((size_t)acc) & 63) == 0);
    XXH_STATIC_ASSERT(XXH_STRIPE_LEN == sizeof(__m512i));

    {

        __m512i const data_vec    = _mm512_loadu_si512   (input);

        __m512i const key_vec     = _mm512_loadu_si512   (secret);

        __m512i const data_key    = _mm512_xor_si512     (data_vec, key_vec);

        __m512i const data_key_lo = _mm512_srli_epi64 (data_key, 32);

        __m512i const product     = _mm512_mul_epu32     (data_key, data_key_lo);

        __m512i const data_swap = _mm512_shuffle_epi32(data_vec, (_MM_PERM_ENUM)_MM_SHUFFLE(1, 0, 3, 2));
        __m512i const sum       = _mm512_add_epi64(*xacc, data_swap);

        *xacc = _mm512_add_epi64(product, sum);
    }
}
XXH_FORCE_INLINE XXH_TARGET_AVX512 XXH3_ACCUMULATE_TEMPLATE(avx512)


XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_scrambleAcc_avx512(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 63) == 0);
    XXH_STATIC_ASSERT(XXH_STRIPE_LEN == sizeof(__m512i));
    {   __m512i* const xacc = (__m512i*) acc;
        const __m512i prime32 = _mm512_set1_epi32((int)XXH_PRIME32_1);


        __m512i const acc_vec     = *xacc;
        __m512i const shifted     = _mm512_srli_epi64    (acc_vec, 47);

        __m512i const key_vec     = _mm512_loadu_si512   (secret);
        __m512i const data_key    = _mm512_ternarylogic_epi32(key_vec, acc_vec, shifted, 0x96 );


        __m512i const data_key_hi = _mm512_srli_epi64 (data_key, 32);
        __m512i const prod_lo     = _mm512_mul_epu32     (data_key, prime32);
        __m512i const prod_hi     = _mm512_mul_epu32     (data_key_hi, prime32);
        *xacc = _mm512_add_epi64(prod_lo, _mm512_slli_epi64(prod_hi, 32));
    }
}

XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_initCustomSecret_avx512(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 63) == 0);
    XXH_STATIC_ASSERT(XXH_SEC_ALIGN == 64);
    XXH_ASSERT(((size_t)customSecret & 63) == 0);
    (void)(&XXH_writeLE64);
    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / sizeof(__m512i);
        __m512i const seed_pos = _mm512_set1_epi64((xxh_i64)seed64);
        __m512i const seed     = _mm512_mask_sub_epi64(seed_pos, 0xAA, _mm512_set1_epi8(0), seed_pos);

        const __m512i* const src  = (const __m512i*) ((const void*) XXH3_kSecret);
              __m512i* const dest = (      __m512i*) customSecret;
        int i;
        XXH_ASSERT(((size_t)src & 63) == 0);
        XXH_ASSERT(((size_t)dest & 63) == 0);
        for (i=0; i < nbRounds; ++i) {
            dest[i] = _mm512_add_epi64(_mm512_load_si512(src + i), seed);
    }   }
}

#endif

#if (XXH_VECTOR == XXH_AVX2) \
    || (defined(XXH_DISPATCH_AVX2) && XXH_DISPATCH_AVX2 != 0)

#ifndef XXH_TARGET_AVX2
# define XXH_TARGET_AVX2
#endif

XXH_FORCE_INLINE XXH_TARGET_AVX2 void
XXH3_accumulate_512_avx2( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 31) == 0);
    {   __m256i* const xacc    =       (__m256i *) acc;

        const         __m256i* const xinput  = (const __m256i *) input;

        const         __m256i* const xsecret = (const __m256i *) secret;

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m256i); i++) {

            __m256i const data_vec    = _mm256_loadu_si256    (xinput+i);

            __m256i const key_vec     = _mm256_loadu_si256   (xsecret+i);

            __m256i const data_key    = _mm256_xor_si256     (data_vec, key_vec);

            __m256i const data_key_lo = _mm256_srli_epi64 (data_key, 32);

            __m256i const product     = _mm256_mul_epu32     (data_key, data_key_lo);

            __m256i const data_swap = _mm256_shuffle_epi32(data_vec, _MM_SHUFFLE(1, 0, 3, 2));
            __m256i const sum       = _mm256_add_epi64(xacc[i], data_swap);

            xacc[i] = _mm256_add_epi64(product, sum);
    }   }
}
XXH_FORCE_INLINE XXH_TARGET_AVX2 XXH3_ACCUMULATE_TEMPLATE(avx2)

XXH_FORCE_INLINE XXH_TARGET_AVX2 void
XXH3_scrambleAcc_avx2(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 31) == 0);
    {   __m256i* const xacc = (__m256i*) acc;

        const         __m256i* const xsecret = (const __m256i *) secret;
        const __m256i prime32 = _mm256_set1_epi32((int)XXH_PRIME32_1);

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m256i); i++) {

            __m256i const acc_vec     = xacc[i];
            __m256i const shifted     = _mm256_srli_epi64    (acc_vec, 47);
            __m256i const data_vec    = _mm256_xor_si256     (acc_vec, shifted);

            __m256i const key_vec     = _mm256_loadu_si256   (xsecret+i);
            __m256i const data_key    = _mm256_xor_si256     (data_vec, key_vec);


            __m256i const data_key_hi = _mm256_srli_epi64 (data_key, 32);
            __m256i const prod_lo     = _mm256_mul_epu32     (data_key, prime32);
            __m256i const prod_hi     = _mm256_mul_epu32     (data_key_hi, prime32);
            xacc[i] = _mm256_add_epi64(prod_lo, _mm256_slli_epi64(prod_hi, 32));
        }
    }
}

XXH_FORCE_INLINE XXH_TARGET_AVX2 void XXH3_initCustomSecret_avx2(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 31) == 0);
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE / sizeof(__m256i)) == 6);
    XXH_STATIC_ASSERT(XXH_SEC_ALIGN <= 64);
    (void)(&XXH_writeLE64);
    XXH_PREFETCH(customSecret);
    {   __m256i const seed = _mm256_set_epi64x((xxh_i64)(0U - seed64), (xxh_i64)seed64, (xxh_i64)(0U - seed64), (xxh_i64)seed64);

        const __m256i* const src  = (const __m256i*) ((const void*) XXH3_kSecret);
              __m256i*       dest = (      __m256i*) customSecret;

#       if defined(__GNUC__) || defined(__clang__)

        XXH_COMPILER_GUARD(dest);
#       endif
        XXH_ASSERT(((size_t)src & 31) == 0);
        XXH_ASSERT(((size_t)dest & 31) == 0);


        dest[0] = _mm256_add_epi64(_mm256_load_si256(src+0), seed);
        dest[1] = _mm256_add_epi64(_mm256_load_si256(src+1), seed);
        dest[2] = _mm256_add_epi64(_mm256_load_si256(src+2), seed);
        dest[3] = _mm256_add_epi64(_mm256_load_si256(src+3), seed);
        dest[4] = _mm256_add_epi64(_mm256_load_si256(src+4), seed);
        dest[5] = _mm256_add_epi64(_mm256_load_si256(src+5), seed);
    }
}

#endif


#if (XXH_VECTOR == XXH_SSE2) || defined(XXH_X86DISPATCH)

#ifndef XXH_TARGET_SSE2
# define XXH_TARGET_SSE2
#endif

XXH_FORCE_INLINE XXH_TARGET_SSE2 void
XXH3_accumulate_512_sse2( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{

    XXH_ASSERT((((size_t)acc) & 15) == 0);
    {   __m128i* const xacc    =       (__m128i *) acc;

        const         __m128i* const xinput  = (const __m128i *) input;

        const         __m128i* const xsecret = (const __m128i *) secret;

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m128i); i++) {

            __m128i const data_vec    = _mm_loadu_si128   (xinput+i);

            __m128i const key_vec     = _mm_loadu_si128   (xsecret+i);

            __m128i const data_key    = _mm_xor_si128     (data_vec, key_vec);

            __m128i const data_key_lo = _mm_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));

            __m128i const product     = _mm_mul_epu32     (data_key, data_key_lo);

            __m128i const data_swap = _mm_shuffle_epi32(data_vec, _MM_SHUFFLE(1,0,3,2));
            __m128i const sum       = _mm_add_epi64(xacc[i], data_swap);

            xacc[i] = _mm_add_epi64(product, sum);
    }   }
}
XXH_FORCE_INLINE XXH_TARGET_SSE2 XXH3_ACCUMULATE_TEMPLATE(sse2)

XXH_FORCE_INLINE XXH_TARGET_SSE2 void
XXH3_scrambleAcc_sse2(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 15) == 0);
    {   __m128i* const xacc = (__m128i*) acc;

        const         __m128i* const xsecret = (const __m128i *) secret;
        const __m128i prime32 = _mm_set1_epi32((int)XXH_PRIME32_1);

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m128i); i++) {

            __m128i const acc_vec     = xacc[i];
            __m128i const shifted     = _mm_srli_epi64    (acc_vec, 47);
            __m128i const data_vec    = _mm_xor_si128     (acc_vec, shifted);

            __m128i const key_vec     = _mm_loadu_si128   (xsecret+i);
            __m128i const data_key    = _mm_xor_si128     (data_vec, key_vec);


            __m128i const data_key_hi = _mm_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            __m128i const prod_lo     = _mm_mul_epu32     (data_key, prime32);
            __m128i const prod_hi     = _mm_mul_epu32     (data_key_hi, prime32);
            xacc[i] = _mm_add_epi64(prod_lo, _mm_slli_epi64(prod_hi, 32));
        }
    }
}

XXH_FORCE_INLINE XXH_TARGET_SSE2 void XXH3_initCustomSecret_sse2(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 15) == 0);
    (void)(&XXH_writeLE64);
    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / sizeof(__m128i);

#       if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER < 1900

        XXH_ALIGN(16) const xxh_i64 seed64x2[2] = { (xxh_i64)seed64, (xxh_i64)(0U - seed64) };
        __m128i const seed = _mm_load_si128((__m128i const*)seed64x2);
#       else
        __m128i const seed = _mm_set_epi64x((xxh_i64)(0U - seed64), (xxh_i64)seed64);
#       endif
        int i;

        const void* const src16 = XXH3_kSecret;
        __m128i* dst16 = (__m128i*) customSecret;
#       if defined(__GNUC__) || defined(__clang__)

        XXH_COMPILER_GUARD(dst16);
#       endif
        XXH_ASSERT(((size_t)src16 & 15) == 0);
        XXH_ASSERT(((size_t)dst16 & 15) == 0);

        for (i=0; i < nbRounds; ++i) {
            dst16[i] = _mm_add_epi64(_mm_load_si128((const __m128i *)src16+i), seed);
    }   }
}

#endif

#if (XXH_VECTOR == XXH_NEON)


XXH_FORCE_INLINE void
XXH3_scalarRound(void* XXH_RESTRICT acc, void const* XXH_RESTRICT input,
                 void const* XXH_RESTRICT secret, size_t lane);

XXH_FORCE_INLINE void
XXH3_scalarScrambleRound(void* XXH_RESTRICT acc,
                         void const* XXH_RESTRICT secret, size_t lane);


XXH_FORCE_INLINE void
XXH3_accumulate_512_neon( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 15) == 0);
    XXH_STATIC_ASSERT(XXH3_NEON_LANES > 0 && XXH3_NEON_LANES <= XXH_ACC_NB && XXH3_NEON_LANES % 2 == 0);
    {
        xxh_aliasing_uint64x2_t* const xacc = (xxh_aliasing_uint64x2_t*) acc;

        uint8_t const* xinput = (const uint8_t *) input;
        uint8_t const* xsecret  = (const uint8_t *) secret;

        size_t i;
#ifdef __wasm_simd128__

        XXH_COMPILER_GUARD(xsecret);
#endif

        for (i = XXH3_NEON_LANES; i < XXH_ACC_NB; i++) {
            XXH3_scalarRound(acc, input, secret, i);
        }
        i = 0;

        for (; i+1 < XXH3_NEON_LANES / 2; i+=2) {

            uint64x2_t data_vec_1 = XXH_vld1q_u64(xinput  + (i * 16));
            uint64x2_t data_vec_2 = XXH_vld1q_u64(xinput  + ((i+1) * 16));

            uint64x2_t key_vec_1  = XXH_vld1q_u64(xsecret + (i * 16));
            uint64x2_t key_vec_2  = XXH_vld1q_u64(xsecret + ((i+1) * 16));

            uint64x2_t data_swap_1 = vextq_u64(data_vec_1, data_vec_1, 1);
            uint64x2_t data_swap_2 = vextq_u64(data_vec_2, data_vec_2, 1);

            uint64x2_t data_key_1 = veorq_u64(data_vec_1, key_vec_1);
            uint64x2_t data_key_2 = veorq_u64(data_vec_2, key_vec_2);


            uint32x4x2_t unzipped = vuzpq_u32(
                vreinterpretq_u32_u64(data_key_1),
                vreinterpretq_u32_u64(data_key_2)
            );

            uint32x4_t data_key_lo = unzipped.val[0];

            uint32x4_t data_key_hi = unzipped.val[1];

            uint64x2_t sum_1 = XXH_vmlal_low_u32(data_swap_1, data_key_lo, data_key_hi);
            uint64x2_t sum_2 = XXH_vmlal_high_u32(data_swap_2, data_key_lo, data_key_hi);

            XXH_COMPILER_GUARD_CLANG_NEON(sum_1);
            XXH_COMPILER_GUARD_CLANG_NEON(sum_2);

            xacc[i]   = vaddq_u64(xacc[i], sum_1);
            xacc[i+1] = vaddq_u64(xacc[i+1], sum_2);
        }

        for (; i < XXH3_NEON_LANES / 2; i++) {

            uint64x2_t data_vec = XXH_vld1q_u64(xinput  + (i * 16));

            uint64x2_t key_vec  = XXH_vld1q_u64(xsecret + (i * 16));

            uint64x2_t data_swap = vextq_u64(data_vec, data_vec, 1);

            uint64x2_t data_key = veorq_u64(data_vec, key_vec);


            uint32x2_t data_key_lo = vmovn_u64(data_key);

            uint32x2_t data_key_hi = vshrn_n_u64(data_key, 32);

            uint64x2_t sum = vmlal_u32(data_swap, data_key_lo, data_key_hi);

            XXH_COMPILER_GUARD_CLANG_NEON(sum);

            xacc[i] = vaddq_u64 (xacc[i], sum);
        }
    }
}
XXH_FORCE_INLINE XXH3_ACCUMULATE_TEMPLATE(neon)

XXH_FORCE_INLINE void
XXH3_scrambleAcc_neon(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 15) == 0);

    {   xxh_aliasing_uint64x2_t* xacc       = (xxh_aliasing_uint64x2_t*) acc;
        uint8_t const* xsecret = (uint8_t const*) secret;

        size_t i;

#ifndef __wasm_simd128__

        uint32x2_t const kPrimeLo = vdup_n_u32(XXH_PRIME32_1);

        uint32x4_t const kPrimeHi = vreinterpretq_u32_u64(vdupq_n_u64((xxh_u64)XXH_PRIME32_1 << 32));
#endif


        for (i = XXH3_NEON_LANES; i < XXH_ACC_NB; i++) {
            XXH3_scalarScrambleRound(acc, secret, i);
        }
        for (i=0; i < XXH3_NEON_LANES / 2; i++) {

            uint64x2_t acc_vec  = xacc[i];
            uint64x2_t shifted  = vshrq_n_u64(acc_vec, 47);
            uint64x2_t data_vec = veorq_u64(acc_vec, shifted);


            uint64x2_t key_vec  = XXH_vld1q_u64(xsecret + (i * 16));
            uint64x2_t data_key = veorq_u64(data_vec, key_vec);

#ifdef __wasm_simd128__

            xacc[i] = data_key * XXH_PRIME32_1;
#else

            uint32x4_t prod_hi = vmulq_u32 (vreinterpretq_u32_u64(data_key), kPrimeHi);

            uint32x2_t data_key_lo = vmovn_u64(data_key);

            xacc[i] = vmlal_u32(vreinterpretq_u64_u32(prod_hi), data_key_lo, kPrimeLo);
#endif
        }
    }
}
#endif

#if (XXH_VECTOR == XXH_VSX)

XXH_FORCE_INLINE void
XXH3_accumulate_512_vsx(  void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{

    xxh_aliasing_u64x2* const xacc = (xxh_aliasing_u64x2*) acc;
    xxh_u8 const* const xinput   = (xxh_u8 const*) input;
    xxh_u8 const* const xsecret  = (xxh_u8 const*) secret;
    xxh_u64x2 const v32 = { 32, 32 };
    size_t i;
    for (i = 0; i < XXH_STRIPE_LEN / sizeof(xxh_u64x2); i++) {

        xxh_u64x2 const data_vec = XXH_vec_loadu(xinput + 16*i);

        xxh_u64x2 const key_vec  = XXH_vec_loadu(xsecret + 16*i);
        xxh_u64x2 const data_key = data_vec ^ key_vec;

        xxh_u32x4 const shuffled = (xxh_u32x4)vec_rl(data_key, v32);

        xxh_u64x2 const product  = XXH_vec_mulo((xxh_u32x4)data_key, shuffled);

        xxh_u64x2 acc_vec        = xacc[i];
        acc_vec += product;


#ifdef __s390x__
        acc_vec += vec_permi(data_vec, data_vec, 2);
#else
        acc_vec += vec_xxpermdi(data_vec, data_vec, 2);
#endif
        xacc[i] = acc_vec;
    }
}
XXH_FORCE_INLINE XXH3_ACCUMULATE_TEMPLATE(vsx)

XXH_FORCE_INLINE void
XXH3_scrambleAcc_vsx(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ASSERT((((size_t)acc) & 15) == 0);

    {   xxh_aliasing_u64x2* const xacc = (xxh_aliasing_u64x2*) acc;
        const xxh_u8* const xsecret = (const xxh_u8*) secret;

        xxh_u64x2 const v32  = { 32, 32 };
        xxh_u64x2 const v47 = { 47, 47 };
        xxh_u32x4 const prime = { XXH_PRIME32_1, XXH_PRIME32_1, XXH_PRIME32_1, XXH_PRIME32_1 };
        size_t i;
        for (i = 0; i < XXH_STRIPE_LEN / sizeof(xxh_u64x2); i++) {

            xxh_u64x2 const acc_vec  = xacc[i];
            xxh_u64x2 const data_vec = acc_vec ^ (acc_vec >> v47);


            xxh_u64x2 const key_vec  = XXH_vec_loadu(xsecret + 16*i);
            xxh_u64x2 const data_key = data_vec ^ key_vec;


            xxh_u64x2 const prod_even  = XXH_vec_mule((xxh_u32x4)data_key, prime);

            xxh_u64x2 const prod_odd  = XXH_vec_mulo((xxh_u32x4)data_key, prime);
            xacc[i] = prod_odd + (prod_even << v32);
    }   }
}

#endif

#if (XXH_VECTOR == XXH_SVE)

XXH_FORCE_INLINE void
XXH3_accumulate_512_sve( void* XXH_RESTRICT acc,
                   const void* XXH_RESTRICT input,
                   const void* XXH_RESTRICT secret)
{
    uint64_t *xacc = (uint64_t *)acc;
    const uint64_t *xinput = (const uint64_t *)(const void *)input;
    const uint64_t *xsecret = (const uint64_t *)(const void *)secret;
    svuint64_t kSwap = sveor_n_u64_z(svptrue_b64(), svindex_u64(0, 1), 1);
    uint64_t element_count = svcntd();
    if (element_count >= 8) {
        svbool_t mask = svptrue_pat_b64(SV_VL8);
        svuint64_t vacc = svld1_u64(mask, xacc);
        ACCRND(vacc, 0);
        svst1_u64(mask, xacc, vacc);
    } else if (element_count == 2) {
        svbool_t mask = svptrue_pat_b64(SV_VL2);
        svuint64_t acc0 = svld1_u64(mask, xacc + 0);
        svuint64_t acc1 = svld1_u64(mask, xacc + 2);
        svuint64_t acc2 = svld1_u64(mask, xacc + 4);
        svuint64_t acc3 = svld1_u64(mask, xacc + 6);
        ACCRND(acc0, 0);
        ACCRND(acc1, 2);
        ACCRND(acc2, 4);
        ACCRND(acc3, 6);
        svst1_u64(mask, xacc + 0, acc0);
        svst1_u64(mask, xacc + 2, acc1);
        svst1_u64(mask, xacc + 4, acc2);
        svst1_u64(mask, xacc + 6, acc3);
    } else {
        svbool_t mask = svptrue_pat_b64(SV_VL4);
        svuint64_t acc0 = svld1_u64(mask, xacc + 0);
        svuint64_t acc1 = svld1_u64(mask, xacc + 4);
        ACCRND(acc0, 0);
        ACCRND(acc1, 4);
        svst1_u64(mask, xacc + 0, acc0);
        svst1_u64(mask, xacc + 4, acc1);
    }
}

XXH_FORCE_INLINE void
XXH3_accumulate_sve(xxh_u64* XXH_RESTRICT acc,
               const xxh_u8* XXH_RESTRICT input,
               const xxh_u8* XXH_RESTRICT secret,
               size_t nbStripes)
{
    if (nbStripes != 0) {
        uint64_t *xacc = (uint64_t *)acc;
        const uint64_t *xinput = (const uint64_t *)(const void *)input;
        const uint64_t *xsecret = (const uint64_t *)(const void *)secret;
        svuint64_t kSwap = sveor_n_u64_z(svptrue_b64(), svindex_u64(0, 1), 1);
        uint64_t element_count = svcntd();
        if (element_count >= 8) {
            svbool_t mask = svptrue_pat_b64(SV_VL8);
            svuint64_t vacc = svld1_u64(mask, xacc + 0);
            do {

                svprfd(mask, xinput + 128, SV_PLDL1STRM);
                ACCRND(vacc, 0);
                xinput += 8;
                xsecret += 1;
                nbStripes--;
           } while (nbStripes != 0);

           svst1_u64(mask, xacc + 0, vacc);
        } else if (element_count == 2) {
            svbool_t mask = svptrue_pat_b64(SV_VL2);
            svuint64_t acc0 = svld1_u64(mask, xacc + 0);
            svuint64_t acc1 = svld1_u64(mask, xacc + 2);
            svuint64_t acc2 = svld1_u64(mask, xacc + 4);
            svuint64_t acc3 = svld1_u64(mask, xacc + 6);
            do {
                svprfd(mask, xinput + 128, SV_PLDL1STRM);
                ACCRND(acc0, 0);
                ACCRND(acc1, 2);
                ACCRND(acc2, 4);
                ACCRND(acc3, 6);
                xinput += 8;
                xsecret += 1;
                nbStripes--;
           } while (nbStripes != 0);

           svst1_u64(mask, xacc + 0, acc0);
           svst1_u64(mask, xacc + 2, acc1);
           svst1_u64(mask, xacc + 4, acc2);
           svst1_u64(mask, xacc + 6, acc3);
        } else {
            svbool_t mask = svptrue_pat_b64(SV_VL4);
            svuint64_t acc0 = svld1_u64(mask, xacc + 0);
            svuint64_t acc1 = svld1_u64(mask, xacc + 4);
            do {
                svprfd(mask, xinput + 128, SV_PLDL1STRM);
                ACCRND(acc0, 0);
                ACCRND(acc1, 4);
                xinput += 8;
                xsecret += 1;
                nbStripes--;
           } while (nbStripes != 0);

           svst1_u64(mask, xacc + 0, acc0);
           svst1_u64(mask, xacc + 4, acc1);
       }
    }
}

#endif


#if defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))

XXH_FORCE_INLINE xxh_u64
XXH_mult32to64_add64(xxh_u64 lhs, xxh_u64 rhs, xxh_u64 acc)
{
    xxh_u64 ret;

    __asm__("umaddl %x0, %w1, %w2, %x3" : "=r" (ret) : "r" (lhs), "r" (rhs), "r" (acc));
    return ret;
}
#else
XXH_FORCE_INLINE xxh_u64
XXH_mult32to64_add64(xxh_u64 lhs, xxh_u64 rhs, xxh_u64 acc)
{
    return XXH_mult32to64((xxh_u32)lhs, (xxh_u32)rhs) + acc;
}
#endif


XXH_FORCE_INLINE void
XXH3_scalarRound(void* XXH_RESTRICT acc,
                 void const* XXH_RESTRICT input,
                 void const* XXH_RESTRICT secret,
                 size_t lane)
{
    xxh_u64* xacc = (xxh_u64*) acc;
    xxh_u8 const* xinput  = (xxh_u8 const*) input;
    xxh_u8 const* xsecret = (xxh_u8 const*) secret;
    XXH_ASSERT(lane < XXH_ACC_NB);
    XXH_ASSERT(((size_t)acc & (XXH_ACC_ALIGN-1)) == 0);
    {
        xxh_u64 const data_val = XXH_readLE64(xinput + lane * 8);
        xxh_u64 const data_key = data_val ^ XXH_readLE64(xsecret + lane * 8);
        xacc[lane ^ 1] += data_val;
        xacc[lane] = XXH_mult32to64_add64(data_key , data_key >> 32, xacc[lane]);
    }
}


XXH_FORCE_INLINE void
XXH3_accumulate_512_scalar(void* XXH_RESTRICT acc,
                     const void* XXH_RESTRICT input,
                     const void* XXH_RESTRICT secret)
{
    size_t i;

#if defined(__GNUC__) && !defined(__clang__) \
  && (defined(__arm__) || defined(__thumb2__)) \
  && defined(__ARM_FEATURE_UNALIGNED)  \
  && XXH_SIZE_OPT <= 0
#  pragma GCC unroll 8
#endif
    for (i=0; i < XXH_ACC_NB; i++) {
        XXH3_scalarRound(acc, input, secret, i);
    }
}
XXH_FORCE_INLINE XXH3_ACCUMULATE_TEMPLATE(scalar)


XXH_FORCE_INLINE void
XXH3_scalarScrambleRound(void* XXH_RESTRICT acc,
                         void const* XXH_RESTRICT secret,
                         size_t lane)
{
    xxh_u64* const xacc = (xxh_u64*) acc;
    const xxh_u8* const xsecret = (const xxh_u8*) secret;
    XXH_ASSERT((((size_t)acc) & (XXH_ACC_ALIGN-1)) == 0);
    XXH_ASSERT(lane < XXH_ACC_NB);
    {
        xxh_u64 const key64 = XXH_readLE64(xsecret + lane * 8);
        xxh_u64 acc64 = xacc[lane];
        acc64 = XXH_xorshift64(acc64, 47);
        acc64 ^= key64;
        acc64 *= XXH_PRIME32_1;
        xacc[lane] = acc64;
    }
}


XXH_FORCE_INLINE void
XXH3_scrambleAcc_scalar(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    size_t i;
    for (i=0; i < XXH_ACC_NB; i++) {
        XXH3_scalarScrambleRound(acc, secret, i);
    }
}

XXH_FORCE_INLINE void
XXH3_initCustomSecret_scalar(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{

    const xxh_u8* kSecretPtr = XXH3_kSecret;
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 15) == 0);

#if defined(__GNUC__) && defined(__aarch64__)

    XXH_COMPILER_GUARD(kSecretPtr);
#endif
    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / 16;
        int i;
        for (i=0; i < nbRounds; i++) {

            xxh_u64 lo = XXH_readLE64(kSecretPtr + 16*i)     + seed64;
            xxh_u64 hi = XXH_readLE64(kSecretPtr + 16*i + 8) - seed64;
            XXH_writeLE64((xxh_u8*)customSecret + 16*i,     lo);
            XXH_writeLE64((xxh_u8*)customSecret + 16*i + 8, hi);
    }   }
}


typedef void (*XXH3_f_accumulate)(xxh_u64* XXH_RESTRICT, const xxh_u8* XXH_RESTRICT, const xxh_u8* XXH_RESTRICT, size_t);
typedef void (*XXH3_f_scrambleAcc)(void* XXH_RESTRICT, const void*);
typedef void (*XXH3_f_initCustomSecret)(void* XXH_RESTRICT, xxh_u64);


#if (XXH_VECTOR == XXH_AVX512)

#define XXH3_accumulate_512 XXH3_accumulate_512_avx512
#define XXH3_accumulate     XXH3_accumulate_avx512
#define XXH3_scrambleAcc    XXH3_scrambleAcc_avx512
#define XXH3_initCustomSecret XXH3_initCustomSecret_avx512

#elif (XXH_VECTOR == XXH_AVX2)

#define XXH3_accumulate_512 XXH3_accumulate_512_avx2
#define XXH3_accumulate     XXH3_accumulate_avx2
#define XXH3_scrambleAcc    XXH3_scrambleAcc_avx2
#define XXH3_initCustomSecret XXH3_initCustomSecret_avx2

#elif (XXH_VECTOR == XXH_SSE2)

#define XXH3_accumulate_512 XXH3_accumulate_512_sse2
#define XXH3_accumulate     XXH3_accumulate_sse2
#define XXH3_scrambleAcc    XXH3_scrambleAcc_sse2
#define XXH3_initCustomSecret XXH3_initCustomSecret_sse2

#elif (XXH_VECTOR == XXH_NEON)

#define XXH3_accumulate_512 XXH3_accumulate_512_neon
#define XXH3_accumulate     XXH3_accumulate_neon
#define XXH3_scrambleAcc    XXH3_scrambleAcc_neon
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#elif (XXH_VECTOR == XXH_VSX)

#define XXH3_accumulate_512 XXH3_accumulate_512_vsx
#define XXH3_accumulate     XXH3_accumulate_vsx
#define XXH3_scrambleAcc    XXH3_scrambleAcc_vsx
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#elif (XXH_VECTOR == XXH_SVE)
#define XXH3_accumulate_512 XXH3_accumulate_512_sve
#define XXH3_accumulate     XXH3_accumulate_sve
#define XXH3_scrambleAcc    XXH3_scrambleAcc_scalar
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#else

#define XXH3_accumulate_512 XXH3_accumulate_512_scalar
#define XXH3_accumulate     XXH3_accumulate_scalar
#define XXH3_scrambleAcc    XXH3_scrambleAcc_scalar
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#endif

#if XXH_SIZE_OPT >= 1
#  undef XXH3_initCustomSecret
#  define XXH3_initCustomSecret XXH3_initCustomSecret_scalar
#endif

XXH_FORCE_INLINE void
XXH3_hashLong_internal_loop(xxh_u64* XXH_RESTRICT acc,
                      const xxh_u8* XXH_RESTRICT input, size_t len,
                      const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                            XXH3_f_accumulate f_acc,
                            XXH3_f_scrambleAcc f_scramble)
{
    size_t const nbStripesPerBlock = (secretSize - XXH_STRIPE_LEN) / XXH_SECRET_CONSUME_RATE;
    size_t const block_len = XXH_STRIPE_LEN * nbStripesPerBlock;
    size_t const nb_blocks = (len - 1) / block_len;

    size_t n;

    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN);

    for (n = 0; n < nb_blocks; n++) {
        f_acc(acc, input + n*block_len, secret, nbStripesPerBlock);
        f_scramble(acc, secret + secretSize - XXH_STRIPE_LEN);
    }


    XXH_ASSERT(len > XXH_STRIPE_LEN);
    {   size_t const nbStripes = ((len - 1) - (block_len * nb_blocks)) / XXH_STRIPE_LEN;
        XXH_ASSERT(nbStripes <= (secretSize / XXH_SECRET_CONSUME_RATE));
        f_acc(acc, input + nb_blocks*block_len, secret, nbStripes);


        {   const xxh_u8* const p = input + len - XXH_STRIPE_LEN;
#define XXH_SECRET_LASTACC_START 7
            XXH3_accumulate_512(acc, p, secret + secretSize - XXH_STRIPE_LEN - XXH_SECRET_LASTACC_START);
    }   }
}

XXH_FORCE_INLINE xxh_u64
XXH3_mix2Accs(const xxh_u64* XXH_RESTRICT acc, const xxh_u8* XXH_RESTRICT secret)
{
    return XXH3_mul128_fold64(
               acc[0] ^ XXH_readLE64(secret),
               acc[1] ^ XXH_readLE64(secret+8) );
}

static XXH64_hash_t
XXH3_mergeAccs(const xxh_u64* XXH_RESTRICT acc, const xxh_u8* XXH_RESTRICT secret, xxh_u64 start)
{
    xxh_u64 result64 = start;
    size_t i = 0;

    for (i = 0; i < 4; i++) {
        result64 += XXH3_mix2Accs(acc+2*i, secret + 16*i);
#if defined(__clang__)                                 \
    && (defined(__arm__) || defined(__thumb__))        \
    && (defined(__ARM_NEON) || defined(__ARM_NEON__))   \
    && !defined(XXH_ENABLE_AUTOVECTORIZE)

        XXH_COMPILER_GUARD(result64);
#endif
    }

    return XXH3_avalanche(result64);
}

#define XXH3_INIT_ACC { XXH_PRIME32_3, XXH_PRIME64_1, XXH_PRIME64_2, XXH_PRIME64_3, \
                        XXH_PRIME64_4, XXH_PRIME32_2, XXH_PRIME64_5, XXH_PRIME32_1 }

XXH_FORCE_INLINE XXH64_hash_t
XXH3_hashLong_64b_internal(const void* XXH_RESTRICT input, size_t len,
                           const void* XXH_RESTRICT secret, size_t secretSize,
                           XXH3_f_accumulate f_acc,
                           XXH3_f_scrambleAcc f_scramble)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64 acc[XXH_ACC_NB] = XXH3_INIT_ACC;

    XXH3_hashLong_internal_loop(acc, (const xxh_u8*)input, len, (const xxh_u8*)secret, secretSize, f_acc, f_scramble);


    XXH_STATIC_ASSERT(sizeof(acc) == 64);

#define XXH_SECRET_MERGEACCS_START 11
    XXH_ASSERT(secretSize >= sizeof(acc) + XXH_SECRET_MERGEACCS_START);
    return XXH3_mergeAccs(acc, (const xxh_u8*)secret + XXH_SECRET_MERGEACCS_START, (xxh_u64)len * XXH_PRIME64_1);
}


XXH3_WITH_SECRET_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSecret(const void* XXH_RESTRICT input, size_t len,
                             XXH64_hash_t seed64, const xxh_u8* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64;
    return XXH3_hashLong_64b_internal(input, len, secret, secretLen, XXH3_accumulate, XXH3_scrambleAcc);
}


XXH_NO_INLINE XXH_PUREF XXH64_hash_t
XXH3_hashLong_64b_default(const void* XXH_RESTRICT input, size_t len,
                          XXH64_hash_t seed64, const xxh_u8* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64; (void)secret; (void)secretLen;
    return XXH3_hashLong_64b_internal(input, len, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_accumulate, XXH3_scrambleAcc);
}


XXH_FORCE_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSeed_internal(const void* input, size_t len,
                                    XXH64_hash_t seed,
                                    XXH3_f_accumulate f_acc,
                                    XXH3_f_scrambleAcc f_scramble,
                                    XXH3_f_initCustomSecret f_initSec)
{
#if XXH_SIZE_OPT <= 0
    if (seed == 0)
        return XXH3_hashLong_64b_internal(input, len,
                                          XXH3_kSecret, sizeof(XXH3_kSecret),
                                          f_acc, f_scramble);
#endif
    {   XXH_ALIGN(XXH_SEC_ALIGN) xxh_u8 secret[XXH_SECRET_DEFAULT_SIZE];
        f_initSec(secret, seed);
        return XXH3_hashLong_64b_internal(input, len, secret, sizeof(secret),
                                          f_acc, f_scramble);
    }
}


XXH_NO_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSeed(const void* XXH_RESTRICT input, size_t len,
                           XXH64_hash_t seed, const xxh_u8* XXH_RESTRICT secret, size_t secretLen)
{
    (void)secret; (void)secretLen;
    return XXH3_hashLong_64b_withSeed_internal(input, len, seed,
                XXH3_accumulate, XXH3_scrambleAcc, XXH3_initCustomSecret);
}


typedef XXH64_hash_t (*XXH3_hashLong64_f)(const void* XXH_RESTRICT, size_t,
                                          XXH64_hash_t, const xxh_u8* XXH_RESTRICT, size_t);

XXH_FORCE_INLINE XXH64_hash_t
XXH3_64bits_internal(const void* XXH_RESTRICT input, size_t len,
                     XXH64_hash_t seed64, const void* XXH_RESTRICT secret, size_t secretLen,
                     XXH3_hashLong64_f f_hashLong)
{
    XXH_ASSERT(secretLen >= XXH3_SECRET_SIZE_MIN);

    if (len <= 16)
        return XXH3_len_0to16_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, seed64);
    if (len <= 128)
        return XXH3_len_17to128_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    if (len <= XXH3_MIDSIZE_MAX)
        return XXH3_len_129to240_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    return f_hashLong(input, len, seed64, (const xxh_u8*)secret, secretLen);
}


XXH_PUBLIC_API XXH64_hash_t XXH3_64bits(XXH_NOESCAPE const void* input, size_t length)
{
    return XXH3_64bits_internal(input, length, 0, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_hashLong_64b_default);
}


XXH_PUBLIC_API XXH64_hash_t
XXH3_64bits_withSecret(XXH_NOESCAPE const void* input, size_t length, XXH_NOESCAPE const void* secret, size_t secretSize)
{
    return XXH3_64bits_internal(input, length, 0, secret, secretSize, XXH3_hashLong_64b_withSecret);
}


XXH_PUBLIC_API XXH64_hash_t
XXH3_64bits_withSeed(XXH_NOESCAPE const void* input, size_t length, XXH64_hash_t seed)
{
    return XXH3_64bits_internal(input, length, seed, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_hashLong_64b_withSeed);
}

XXH_PUBLIC_API XXH64_hash_t
XXH3_64bits_withSecretandSeed(XXH_NOESCAPE const void* input, size_t length, XXH_NOESCAPE const void* secret, size_t secretSize, XXH64_hash_t seed)
{
    if (length <= XXH3_MIDSIZE_MAX)
        return XXH3_64bits_internal(input, length, seed, XXH3_kSecret, sizeof(XXH3_kSecret), NULL);
    return XXH3_hashLong_64b_withSecret(input, length, seed, (const xxh_u8*)secret, secretSize);
}


#ifndef XXH_NO_STREAM

static XXH_MALLOCF void* XXH_alignedMalloc(size_t s, size_t align)
{
    XXH_ASSERT(align <= 128 && align >= 8);
    XXH_ASSERT((align & (align-1)) == 0);
    XXH_ASSERT(s != 0 && s < (s + align));
    {
        xxh_u8* base = (xxh_u8*)XXH_malloc(s + align);
        if (base != NULL) {

            size_t offset = align - ((size_t)base & (align - 1));

            xxh_u8* ptr = base + offset;

            XXH_ASSERT((size_t)ptr % align == 0);


            ptr[-1] = (xxh_u8)offset;
            return ptr;
        }
        return NULL;
    }
}

static void XXH_alignedFree(void* p)
{
    if (p != NULL) {
        xxh_u8* ptr = (xxh_u8*)p;

        xxh_u8 offset = ptr[-1];

        xxh_u8* base = ptr - offset;
        XXH_free(base);
    }
}


XXH_PUBLIC_API XXH3_state_t* XXH3_createState(void)
{
    XXH3_state_t* const state = (XXH3_state_t*)XXH_alignedMalloc(sizeof(XXH3_state_t), 64);
    if (state==NULL) return NULL;
    XXH3_INITSTATE(state);
    return state;
}


XXH_PUBLIC_API XXH_errorcode XXH3_freeState(XXH3_state_t* statePtr)
{
    XXH_alignedFree(statePtr);
    return XXH_OK;
}


XXH_PUBLIC_API void
XXH3_copyState(XXH_NOESCAPE XXH3_state_t* dst_state, XXH_NOESCAPE const XXH3_state_t* src_state)
{
    XXH_memcpy(dst_state, src_state, sizeof(*dst_state));
}

static void
XXH3_reset_internal(XXH3_state_t* statePtr,
                    XXH64_hash_t seed,
                    const void* secret, size_t secretSize)
{
    size_t const initStart = offsetof(XXH3_state_t, bufferedSize);
    size_t const initLength = offsetof(XXH3_state_t, nbStripesPerBlock) - initStart;
    XXH_ASSERT(offsetof(XXH3_state_t, nbStripesPerBlock) > initStart);
    XXH_ASSERT(statePtr != NULL);

    memset((char*)statePtr + initStart, 0, initLength);
    statePtr->acc[0] = XXH_PRIME32_3;
    statePtr->acc[1] = XXH_PRIME64_1;
    statePtr->acc[2] = XXH_PRIME64_2;
    statePtr->acc[3] = XXH_PRIME64_3;
    statePtr->acc[4] = XXH_PRIME64_4;
    statePtr->acc[5] = XXH_PRIME32_2;
    statePtr->acc[6] = XXH_PRIME64_5;
    statePtr->acc[7] = XXH_PRIME32_1;
    statePtr->seed = seed;
    statePtr->useSeed = (seed != 0);
    statePtr->extSecret = (const unsigned char*)secret;
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN);
    statePtr->secretLimit = secretSize - XXH_STRIPE_LEN;
    statePtr->nbStripesPerBlock = statePtr->secretLimit / XXH_SECRET_CONSUME_RATE;
}


XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_reset(XXH_NOESCAPE XXH3_state_t* statePtr)
{
    if (statePtr == NULL) return XXH_ERROR;
    XXH3_reset_internal(statePtr, 0, XXH3_kSecret, XXH_SECRET_DEFAULT_SIZE);
    return XXH_OK;
}


XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_reset_withSecret(XXH_NOESCAPE XXH3_state_t* statePtr, XXH_NOESCAPE const void* secret, size_t secretSize)
{
    if (statePtr == NULL) return XXH_ERROR;
    XXH3_reset_internal(statePtr, 0, secret, secretSize);
    if (secret == NULL) return XXH_ERROR;
    if (secretSize < XXH3_SECRET_SIZE_MIN) return XXH_ERROR;
    return XXH_OK;
}


XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_reset_withSeed(XXH_NOESCAPE XXH3_state_t* statePtr, XXH64_hash_t seed)
{
    if (statePtr == NULL) return XXH_ERROR;
    if (seed==0) return XXH3_64bits_reset(statePtr);
    if ((seed != statePtr->seed) || (statePtr->extSecret != NULL))
        XXH3_initCustomSecret(statePtr->customSecret, seed);
    XXH3_reset_internal(statePtr, seed, NULL, XXH_SECRET_DEFAULT_SIZE);
    return XXH_OK;
}


XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_reset_withSecretandSeed(XXH_NOESCAPE XXH3_state_t* statePtr, XXH_NOESCAPE const void* secret, size_t secretSize, XXH64_hash_t seed64)
{
    if (statePtr == NULL) return XXH_ERROR;
    if (secret == NULL) return XXH_ERROR;
    if (secretSize < XXH3_SECRET_SIZE_MIN) return XXH_ERROR;
    XXH3_reset_internal(statePtr, seed64, secret, secretSize);
    statePtr->useSeed = 1;
    return XXH_OK;
}


XXH_FORCE_INLINE const xxh_u8 *
XXH3_consumeStripes(xxh_u64* XXH_RESTRICT acc,
                    size_t* XXH_RESTRICT nbStripesSoFarPtr, size_t nbStripesPerBlock,
                    const xxh_u8* XXH_RESTRICT input, size_t nbStripes,
                    const xxh_u8* XXH_RESTRICT secret, size_t secretLimit,
                    XXH3_f_accumulate f_acc,
                    XXH3_f_scrambleAcc f_scramble)
{
    const xxh_u8* initialSecret = secret + *nbStripesSoFarPtr * XXH_SECRET_CONSUME_RATE;

    if (nbStripes >= (nbStripesPerBlock - *nbStripesSoFarPtr)) {

        size_t nbStripesThisIter = nbStripesPerBlock - *nbStripesSoFarPtr;

        do {

            f_acc(acc, input, initialSecret, nbStripesThisIter);
            f_scramble(acc, secret + secretLimit);
            input += nbStripesThisIter * XXH_STRIPE_LEN;
            nbStripes -= nbStripesThisIter;

            nbStripesThisIter = nbStripesPerBlock;
            initialSecret = secret;
        } while (nbStripes >= nbStripesPerBlock);
        *nbStripesSoFarPtr = 0;
    }

    if (nbStripes > 0) {
        f_acc(acc, input, initialSecret, nbStripes);
        input += nbStripes * XXH_STRIPE_LEN;
        *nbStripesSoFarPtr += nbStripes;
    }

    return input;
}

#ifndef XXH3_STREAM_USE_STACK
# if XXH_SIZE_OPT <= 0 && !defined(__clang__)
#   define XXH3_STREAM_USE_STACK 1
# endif
#endif

XXH_FORCE_INLINE XXH_errorcode
XXH3_update(XXH3_state_t* XXH_RESTRICT const state,
            const xxh_u8* XXH_RESTRICT input, size_t len,
            XXH3_f_accumulate f_acc,
            XXH3_f_scrambleAcc f_scramble)
{
    if (input==NULL) {
        XXH_ASSERT(len == 0);
        return XXH_OK;
    }

    XXH_ASSERT(state != NULL);
    {   const xxh_u8* const bEnd = input + len;
        const unsigned char* const secret = (state->extSecret == NULL) ? state->customSecret : state->extSecret;
#if defined(XXH3_STREAM_USE_STACK) && XXH3_STREAM_USE_STACK >= 1

        XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64 acc[8];
        XXH_memcpy(acc, state->acc, sizeof(acc));
#else
        xxh_u64* XXH_RESTRICT const acc = state->acc;
#endif
        state->totalLen += len;
        XXH_ASSERT(state->bufferedSize <= XXH3_INTERNALBUFFER_SIZE);


        if (len <= XXH3_INTERNALBUFFER_SIZE - state->bufferedSize) {
            XXH_memcpy(state->buffer + state->bufferedSize, input, len);
            state->bufferedSize += (XXH32_hash_t)len;
            return XXH_OK;
        }


        #define XXH3_INTERNALBUFFER_STRIPES (XXH3_INTERNALBUFFER_SIZE / XXH_STRIPE_LEN)
        XXH_STATIC_ASSERT(XXH3_INTERNALBUFFER_SIZE % XXH_STRIPE_LEN == 0);


        if (state->bufferedSize) {
            size_t const loadSize = XXH3_INTERNALBUFFER_SIZE - state->bufferedSize;
            XXH_memcpy(state->buffer + state->bufferedSize, input, loadSize);
            input += loadSize;
            XXH3_consumeStripes(acc,
                               &state->nbStripesSoFar, state->nbStripesPerBlock,
                                state->buffer, XXH3_INTERNALBUFFER_STRIPES,
                                secret, state->secretLimit,
                                f_acc, f_scramble);
            state->bufferedSize = 0;
        }
        XXH_ASSERT(input < bEnd);
        if (bEnd - input > XXH3_INTERNALBUFFER_SIZE) {
            size_t nbStripes = (size_t)(bEnd - 1 - input) / XXH_STRIPE_LEN;
            input = XXH3_consumeStripes(acc,
                                       &state->nbStripesSoFar, state->nbStripesPerBlock,
                                       input, nbStripes,
                                       secret, state->secretLimit,
                                       f_acc, f_scramble);
            XXH_memcpy(state->buffer + sizeof(state->buffer) - XXH_STRIPE_LEN, input - XXH_STRIPE_LEN, XXH_STRIPE_LEN);

        }

        XXH_ASSERT(input < bEnd);
        XXH_ASSERT(bEnd - input <= XXH3_INTERNALBUFFER_SIZE);
        XXH_ASSERT(state->bufferedSize == 0);
        XXH_memcpy(state->buffer, input, (size_t)(bEnd-input));
        state->bufferedSize = (XXH32_hash_t)(bEnd-input);
#if defined(XXH3_STREAM_USE_STACK) && XXH3_STREAM_USE_STACK >= 1

        XXH_memcpy(state->acc, acc, sizeof(acc));
#endif
    }

    return XXH_OK;
}


XXH_PUBLIC_API XXH_errorcode
XXH3_64bits_update(XXH_NOESCAPE XXH3_state_t* state, XXH_NOESCAPE const void* input, size_t len)
{
    return XXH3_update(state, (const xxh_u8*)input, len,
                       XXH3_accumulate, XXH3_scrambleAcc);
}


XXH_FORCE_INLINE void
XXH3_digest_long (XXH64_hash_t* acc,
                  const XXH3_state_t* state,
                  const unsigned char* secret)
{
    xxh_u8 lastStripe[XXH_STRIPE_LEN];
    const xxh_u8* lastStripePtr;


    XXH_memcpy(acc, state->acc, sizeof(state->acc));
    if (state->bufferedSize >= XXH_STRIPE_LEN) {

        size_t const nbStripes = (state->bufferedSize - 1) / XXH_STRIPE_LEN;
        size_t nbStripesSoFar = state->nbStripesSoFar;
        XXH3_consumeStripes(acc,
                           &nbStripesSoFar, state->nbStripesPerBlock,
                            state->buffer, nbStripes,
                            secret, state->secretLimit,
                            XXH3_accumulate, XXH3_scrambleAcc);
        lastStripePtr = state->buffer + state->bufferedSize - XXH_STRIPE_LEN;
    } else {

        size_t const catchupSize = XXH_STRIPE_LEN - state->bufferedSize;
        XXH_ASSERT(state->bufferedSize > 0);
        XXH_memcpy(lastStripe, state->buffer + sizeof(state->buffer) - catchupSize, catchupSize);
        XXH_memcpy(lastStripe + catchupSize, state->buffer, state->bufferedSize);
        lastStripePtr = lastStripe;
    }

    XXH3_accumulate_512(acc,
                        lastStripePtr,
                        secret + state->secretLimit - XXH_SECRET_LASTACC_START);
}


XXH_PUBLIC_API XXH64_hash_t XXH3_64bits_digest (XXH_NOESCAPE const XXH3_state_t* state)
{
    const unsigned char* const secret = (state->extSecret == NULL) ? state->customSecret : state->extSecret;
    if (state->totalLen > XXH3_MIDSIZE_MAX) {
        XXH_ALIGN(XXH_ACC_ALIGN) XXH64_hash_t acc[XXH_ACC_NB];
        XXH3_digest_long(acc, state, secret);
        return XXH3_mergeAccs(acc,
                              secret + XXH_SECRET_MERGEACCS_START,
                              (xxh_u64)state->totalLen * XXH_PRIME64_1);
    }

    if (state->useSeed)
        return XXH3_64bits_withSeed(state->buffer, (size_t)state->totalLen, state->seed);
    return XXH3_64bits_withSecret(state->buffer, (size_t)(state->totalLen),
                                  secret, state->secretLimit + XXH_STRIPE_LEN);
}
#endif


XXH_FORCE_INLINE XXH_PUREF XXH128_hash_t
XXH3_len_1to3_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{

    XXH_ASSERT(input != NULL);
    XXH_ASSERT(1 <= len && len <= 3);
    XXH_ASSERT(secret != NULL);

    {   xxh_u8 const c1 = input[0];
        xxh_u8 const c2 = input[len >> 1];
        xxh_u8 const c3 = input[len - 1];
        xxh_u32 const combinedl = ((xxh_u32)c1 <<16) | ((xxh_u32)c2 << 24)
                                | ((xxh_u32)c3 << 0) | ((xxh_u32)len << 8);
        xxh_u32 const combinedh = XXH_rotl32(XXH_swap32(combinedl), 13);
        xxh_u64 const bitflipl = (XXH_readLE32(secret) ^ XXH_readLE32(secret+4)) + seed;
        xxh_u64 const bitfliph = (XXH_readLE32(secret+8) ^ XXH_readLE32(secret+12)) - seed;
        xxh_u64 const keyed_lo = (xxh_u64)combinedl ^ bitflipl;
        xxh_u64 const keyed_hi = (xxh_u64)combinedh ^ bitfliph;
        XXH128_hash_t h128;
        h128.low64  = XXH64_avalanche(keyed_lo);
        h128.high64 = XXH64_avalanche(keyed_hi);
        return h128;
    }
}

XXH_FORCE_INLINE XXH_PUREF XXH128_hash_t
XXH3_len_4to8_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(secret != NULL);
    XXH_ASSERT(4 <= len && len <= 8);
    seed ^= (xxh_u64)XXH_swap32((xxh_u32)seed) << 32;
    {   xxh_u32 const input_lo = XXH_readLE32(input);
        xxh_u32 const input_hi = XXH_readLE32(input + len - 4);
        xxh_u64 const input_64 = input_lo + ((xxh_u64)input_hi << 32);
        xxh_u64 const bitflip = (XXH_readLE64(secret+16) ^ XXH_readLE64(secret+24)) + seed;
        xxh_u64 const keyed = input_64 ^ bitflip;


        XXH128_hash_t m128 = XXH_mult64to128(keyed, XXH_PRIME64_1 + (len << 2));

        m128.high64 += (m128.low64 << 1);
        m128.low64  ^= (m128.high64 >> 3);

        m128.low64   = XXH_xorshift64(m128.low64, 35);
        m128.low64  *= PRIME_MX2;
        m128.low64   = XXH_xorshift64(m128.low64, 28);
        m128.high64  = XXH3_avalanche(m128.high64);
        return m128;
    }
}

XXH_FORCE_INLINE XXH_PUREF XXH128_hash_t
XXH3_len_9to16_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(input != NULL);
    XXH_ASSERT(secret != NULL);
    XXH_ASSERT(9 <= len && len <= 16);
    {   xxh_u64 const bitflipl = (XXH_readLE64(secret+32) ^ XXH_readLE64(secret+40)) - seed;
        xxh_u64 const bitfliph = (XXH_readLE64(secret+48) ^ XXH_readLE64(secret+56)) + seed;
        xxh_u64 const input_lo = XXH_readLE64(input);
        xxh_u64       input_hi = XXH_readLE64(input + len - 8);
        XXH128_hash_t m128 = XXH_mult64to128(input_lo ^ input_hi ^ bitflipl, XXH_PRIME64_1);

        m128.low64 += (xxh_u64)(len - 1) << 54;
        input_hi   ^= bitfliph;

        if (sizeof(void *) < sizeof(xxh_u64)) {

            m128.high64 += (input_hi & 0xFFFFFFFF00000000ULL) + XXH_mult32to64((xxh_u32)input_hi, XXH_PRIME32_2);
        } else {

            m128.high64 += input_hi + XXH_mult32to64((xxh_u32)input_hi, XXH_PRIME32_2 - 1);
        }

        m128.low64  ^= XXH_swap64(m128.high64);

        {
            XXH128_hash_t h128 = XXH_mult64to128(m128.low64, XXH_PRIME64_2);
            h128.high64 += m128.high64 * XXH_PRIME64_2;

            h128.low64   = XXH3_avalanche(h128.low64);
            h128.high64  = XXH3_avalanche(h128.high64);
            return h128;
    }   }
}


XXH_FORCE_INLINE XXH_PUREF XXH128_hash_t
XXH3_len_0to16_128b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    XXH_ASSERT(len <= 16);
    {   if (len > 8) return XXH3_len_9to16_128b(input, len, secret, seed);
        if (len >= 4) return XXH3_len_4to8_128b(input, len, secret, seed);
        if (len) return XXH3_len_1to3_128b(input, len, secret, seed);
        {   XXH128_hash_t h128;
            xxh_u64 const bitflipl = XXH_readLE64(secret+64) ^ XXH_readLE64(secret+72);
            xxh_u64 const bitfliph = XXH_readLE64(secret+80) ^ XXH_readLE64(secret+88);
            h128.low64 = XXH64_avalanche(seed ^ bitflipl);
            h128.high64 = XXH64_avalanche( seed ^ bitfliph);
            return h128;
    }   }
}


XXH_FORCE_INLINE XXH128_hash_t
XXH128_mix32B(XXH128_hash_t acc, const xxh_u8* input_1, const xxh_u8* input_2,
              const xxh_u8* secret, XXH64_hash_t seed)
{
    acc.low64  += XXH3_mix16B (input_1, secret+0, seed);
    acc.low64  ^= XXH_readLE64(input_2) + XXH_readLE64(input_2 + 8);
    acc.high64 += XXH3_mix16B (input_2, secret+16, seed);
    acc.high64 ^= XXH_readLE64(input_1) + XXH_readLE64(input_1 + 8);
    return acc;
}


XXH_FORCE_INLINE XXH_PUREF XXH128_hash_t
XXH3_len_17to128_128b(const xxh_u8* XXH_RESTRICT input, size_t len,
                      const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                      XXH64_hash_t seed)
{
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN); (void)secretSize;
    XXH_ASSERT(16 < len && len <= 128);

    {   XXH128_hash_t acc;
        acc.low64 = len * XXH_PRIME64_1;
        acc.high64 = 0;

#if XXH_SIZE_OPT >= 1
        {

            unsigned int i = (unsigned int)(len - 1) / 32;
            do {
                acc = XXH128_mix32B(acc, input+16*i, input+len-16*(i+1), secret+32*i, seed);
            } while (i-- != 0);
        }
#else
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc = XXH128_mix32B(acc, input+48, input+len-64, secret+96, seed);
                }
                acc = XXH128_mix32B(acc, input+32, input+len-48, secret+64, seed);
            }
            acc = XXH128_mix32B(acc, input+16, input+len-32, secret+32, seed);
        }
        acc = XXH128_mix32B(acc, input, input+len-16, secret, seed);
#endif
        {   XXH128_hash_t h128;
            h128.low64  = acc.low64 + acc.high64;
            h128.high64 = (acc.low64    * XXH_PRIME64_1)
                        + (acc.high64   * XXH_PRIME64_4)
                        + ((len - seed) * XXH_PRIME64_2);
            h128.low64  = XXH3_avalanche(h128.low64);
            h128.high64 = (XXH64_hash_t)0 - XXH3_avalanche(h128.high64);
            return h128;
        }
    }
}

XXH_NO_INLINE XXH_PUREF XXH128_hash_t
XXH3_len_129to240_128b(const xxh_u8* XXH_RESTRICT input, size_t len,
                       const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                       XXH64_hash_t seed)
{
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN); (void)secretSize;
    XXH_ASSERT(128 < len && len <= XXH3_MIDSIZE_MAX);

    {   XXH128_hash_t acc;
        unsigned i;
        acc.low64 = len * XXH_PRIME64_1;
        acc.high64 = 0;

        for (i = 32; i < 160; i += 32) {
            acc = XXH128_mix32B(acc,
                                input  + i - 32,
                                input  + i - 16,
                                secret + i - 32,
                                seed);
        }
        acc.low64 = XXH3_avalanche(acc.low64);
        acc.high64 = XXH3_avalanche(acc.high64);

        for (i=160; i <= len; i += 32) {
            acc = XXH128_mix32B(acc,
                                input + i - 32,
                                input + i - 16,
                                secret + XXH3_MIDSIZE_STARTOFFSET + i - 160,
                                seed);
        }

        acc = XXH128_mix32B(acc,
                            input + len - 16,
                            input + len - 32,
                            secret + XXH3_SECRET_SIZE_MIN - XXH3_MIDSIZE_LASTOFFSET - 16,
                            (XXH64_hash_t)0 - seed);

        {   XXH128_hash_t h128;
            h128.low64  = acc.low64 + acc.high64;
            h128.high64 = (acc.low64    * XXH_PRIME64_1)
                        + (acc.high64   * XXH_PRIME64_4)
                        + ((len - seed) * XXH_PRIME64_2);
            h128.low64  = XXH3_avalanche(h128.low64);
            h128.high64 = (XXH64_hash_t)0 - XXH3_avalanche(h128.high64);
            return h128;
        }
    }
}

XXH_FORCE_INLINE XXH128_hash_t
XXH3_hashLong_128b_internal(const void* XXH_RESTRICT input, size_t len,
                            const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                            XXH3_f_accumulate f_acc,
                            XXH3_f_scrambleAcc f_scramble)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64 acc[XXH_ACC_NB] = XXH3_INIT_ACC;

    XXH3_hashLong_internal_loop(acc, (const xxh_u8*)input, len, secret, secretSize, f_acc, f_scramble);


    XXH_STATIC_ASSERT(sizeof(acc) == 64);
    XXH_ASSERT(secretSize >= sizeof(acc) + XXH_SECRET_MERGEACCS_START);
    {   XXH128_hash_t h128;
        h128.low64  = XXH3_mergeAccs(acc,
                                     secret + XXH_SECRET_MERGEACCS_START,
                                     (xxh_u64)len * XXH_PRIME64_1);
        h128.high64 = XXH3_mergeAccs(acc,
                                     secret + secretSize
                                            - sizeof(acc) - XXH_SECRET_MERGEACCS_START,
                                     ~((xxh_u64)len * XXH_PRIME64_2));
        return h128;
    }
}


XXH_NO_INLINE XXH_PUREF XXH128_hash_t
XXH3_hashLong_128b_default(const void* XXH_RESTRICT input, size_t len,
                           XXH64_hash_t seed64,
                           const void* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64; (void)secret; (void)secretLen;
    return XXH3_hashLong_128b_internal(input, len, XXH3_kSecret, sizeof(XXH3_kSecret),
                                       XXH3_accumulate, XXH3_scrambleAcc);
}


XXH3_WITH_SECRET_INLINE XXH128_hash_t
XXH3_hashLong_128b_withSecret(const void* XXH_RESTRICT input, size_t len,
                              XXH64_hash_t seed64,
                              const void* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64;
    return XXH3_hashLong_128b_internal(input, len, (const xxh_u8*)secret, secretLen,
                                       XXH3_accumulate, XXH3_scrambleAcc);
}

XXH_FORCE_INLINE XXH128_hash_t
XXH3_hashLong_128b_withSeed_internal(const void* XXH_RESTRICT input, size_t len,
                                XXH64_hash_t seed64,
                                XXH3_f_accumulate f_acc,
                                XXH3_f_scrambleAcc f_scramble,
                                XXH3_f_initCustomSecret f_initSec)
{
    if (seed64 == 0)
        return XXH3_hashLong_128b_internal(input, len,
                                           XXH3_kSecret, sizeof(XXH3_kSecret),
                                           f_acc, f_scramble);
    {   XXH_ALIGN(XXH_SEC_ALIGN) xxh_u8 secret[XXH_SECRET_DEFAULT_SIZE];
        f_initSec(secret, seed64);
        return XXH3_hashLong_128b_internal(input, len, (const xxh_u8*)secret, sizeof(secret),
                                           f_acc, f_scramble);
    }
}


XXH_NO_INLINE XXH128_hash_t
XXH3_hashLong_128b_withSeed(const void* input, size_t len,
                            XXH64_hash_t seed64, const void* XXH_RESTRICT secret, size_t secretLen)
{
    (void)secret; (void)secretLen;
    return XXH3_hashLong_128b_withSeed_internal(input, len, seed64,
                XXH3_accumulate, XXH3_scrambleAcc, XXH3_initCustomSecret);
}

typedef XXH128_hash_t (*XXH3_hashLong128_f)(const void* XXH_RESTRICT, size_t,
                                            XXH64_hash_t, const void* XXH_RESTRICT, size_t);

XXH_FORCE_INLINE XXH128_hash_t
XXH3_128bits_internal(const void* input, size_t len,
                      XXH64_hash_t seed64, const void* XXH_RESTRICT secret, size_t secretLen,
                      XXH3_hashLong128_f f_hl128)
{
    XXH_ASSERT(secretLen >= XXH3_SECRET_SIZE_MIN);

    if (len <= 16)
        return XXH3_len_0to16_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, seed64);
    if (len <= 128)
        return XXH3_len_17to128_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    if (len <= XXH3_MIDSIZE_MAX)
        return XXH3_len_129to240_128b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    return f_hl128(input, len, seed64, secret, secretLen);
}


XXH_PUBLIC_API XXH128_hash_t XXH3_128bits(XXH_NOESCAPE const void* input, size_t len)
{
    return XXH3_128bits_internal(input, len, 0,
                                 XXH3_kSecret, sizeof(XXH3_kSecret),
                                 XXH3_hashLong_128b_default);
}


XXH_PUBLIC_API XXH128_hash_t
XXH3_128bits_withSecret(XXH_NOESCAPE const void* input, size_t len, XXH_NOESCAPE const void* secret, size_t secretSize)
{
    return XXH3_128bits_internal(input, len, 0,
                                 (const xxh_u8*)secret, secretSize,
                                 XXH3_hashLong_128b_withSecret);
}


XXH_PUBLIC_API XXH128_hash_t
XXH3_128bits_withSeed(XXH_NOESCAPE const void* input, size_t len, XXH64_hash_t seed)
{
    return XXH3_128bits_internal(input, len, seed,
                                 XXH3_kSecret, sizeof(XXH3_kSecret),
                                 XXH3_hashLong_128b_withSeed);
}


XXH_PUBLIC_API XXH128_hash_t
XXH3_128bits_withSecretandSeed(XXH_NOESCAPE const void* input, size_t len, XXH_NOESCAPE const void* secret, size_t secretSize, XXH64_hash_t seed)
{
    if (len <= XXH3_MIDSIZE_MAX)
        return XXH3_128bits_internal(input, len, seed, XXH3_kSecret, sizeof(XXH3_kSecret), NULL);
    return XXH3_hashLong_128b_withSecret(input, len, seed, secret, secretSize);
}


XXH_PUBLIC_API XXH128_hash_t
XXH128(XXH_NOESCAPE const void* input, size_t len, XXH64_hash_t seed)
{
    return XXH3_128bits_withSeed(input, len, seed);
}


#ifndef XXH_NO_STREAM


XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_reset(XXH_NOESCAPE XXH3_state_t* statePtr)
{
    return XXH3_64bits_reset(statePtr);
}


XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_reset_withSecret(XXH_NOESCAPE XXH3_state_t* statePtr, XXH_NOESCAPE const void* secret, size_t secretSize)
{
    return XXH3_64bits_reset_withSecret(statePtr, secret, secretSize);
}


XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_reset_withSeed(XXH_NOESCAPE XXH3_state_t* statePtr, XXH64_hash_t seed)
{
    return XXH3_64bits_reset_withSeed(statePtr, seed);
}


XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_reset_withSecretandSeed(XXH_NOESCAPE XXH3_state_t* statePtr, XXH_NOESCAPE const void* secret, size_t secretSize, XXH64_hash_t seed)
{
    return XXH3_64bits_reset_withSecretandSeed(statePtr, secret, secretSize, seed);
}


XXH_PUBLIC_API XXH_errorcode
XXH3_128bits_update(XXH_NOESCAPE XXH3_state_t* state, XXH_NOESCAPE const void* input, size_t len)
{
    return XXH3_64bits_update(state, input, len);
}


XXH_PUBLIC_API XXH128_hash_t XXH3_128bits_digest (XXH_NOESCAPE const XXH3_state_t* state)
{
    const unsigned char* const secret = (state->extSecret == NULL) ? state->customSecret : state->extSecret;
    if (state->totalLen > XXH3_MIDSIZE_MAX) {
        XXH_ALIGN(XXH_ACC_ALIGN) XXH64_hash_t acc[XXH_ACC_NB];
        XXH3_digest_long(acc, state, secret);
        XXH_ASSERT(state->secretLimit + XXH_STRIPE_LEN >= sizeof(acc) + XXH_SECRET_MERGEACCS_START);
        {   XXH128_hash_t h128;
            h128.low64  = XXH3_mergeAccs(acc,
                                         secret + XXH_SECRET_MERGEACCS_START,
                                         (xxh_u64)state->totalLen * XXH_PRIME64_1);
            h128.high64 = XXH3_mergeAccs(acc,
                                         secret + state->secretLimit + XXH_STRIPE_LEN
                                                - sizeof(acc) - XXH_SECRET_MERGEACCS_START,
                                         ~((xxh_u64)state->totalLen * XXH_PRIME64_2));
            return h128;
        }
    }

    if (state->seed)
        return XXH3_128bits_withSeed(state->buffer, (size_t)state->totalLen, state->seed);
    return XXH3_128bits_withSecret(state->buffer, (size_t)(state->totalLen),
                                   secret, state->secretLimit + XXH_STRIPE_LEN);
}
#endif


XXH_PUBLIC_API int XXH128_isEqual(XXH128_hash_t h1, XXH128_hash_t h2)
{

    return !(memcmp(&h1, &h2, sizeof(h1)));
}


XXH_PUBLIC_API int XXH128_cmp(XXH_NOESCAPE const void* h128_1, XXH_NOESCAPE const void* h128_2)
{
    XXH128_hash_t const h1 = *(const XXH128_hash_t*)h128_1;
    XXH128_hash_t const h2 = *(const XXH128_hash_t*)h128_2;
    int const hcmp = (h1.high64 > h2.high64) - (h2.high64 > h1.high64);

    if (hcmp) return hcmp;
    return (h1.low64 > h2.low64) - (h2.low64 > h1.low64);
}


XXH_PUBLIC_API void
XXH128_canonicalFromHash(XXH_NOESCAPE XXH128_canonical_t* dst, XXH128_hash_t hash)
{
    XXH_STATIC_ASSERT(sizeof(XXH128_canonical_t) == sizeof(XXH128_hash_t));
    if (XXH_CPU_LITTLE_ENDIAN) {
        hash.high64 = XXH_swap64(hash.high64);
        hash.low64  = XXH_swap64(hash.low64);
    }
    XXH_memcpy(dst, &hash.high64, sizeof(hash.high64));
    XXH_memcpy((char*)dst + sizeof(hash.high64), &hash.low64, sizeof(hash.low64));
}


XXH_PUBLIC_API XXH128_hash_t
XXH128_hashFromCanonical(XXH_NOESCAPE const XXH128_canonical_t* src)
{
    XXH128_hash_t h;
    h.high64 = XXH_readBE64(src);
    h.low64  = XXH_readBE64(src->digest + 8);
    return h;
}


#define XXH_MIN(x, y) (((x) > (y)) ? (y) : (x))

XXH_FORCE_INLINE void XXH3_combine16(void* dst, XXH128_hash_t h128)
{
    XXH_writeLE64( dst, XXH_readLE64(dst) ^ h128.low64 );
    XXH_writeLE64( (char*)dst+8, XXH_readLE64((char*)dst+8) ^ h128.high64 );
}


XXH_PUBLIC_API XXH_errorcode
XXH3_generateSecret(XXH_NOESCAPE void* secretBuffer, size_t secretSize, XXH_NOESCAPE const void* customSeed, size_t customSeedSize)
{
#if (XXH_DEBUGLEVEL >= 1)
    XXH_ASSERT(secretBuffer != NULL);
    XXH_ASSERT(secretSize >= XXH3_SECRET_SIZE_MIN);
#else

    if (secretBuffer == NULL) return XXH_ERROR;
    if (secretSize < XXH3_SECRET_SIZE_MIN) return XXH_ERROR;
#endif

    if (customSeedSize == 0) {
        customSeed = XXH3_kSecret;
        customSeedSize = XXH_SECRET_DEFAULT_SIZE;
    }
#if (XXH_DEBUGLEVEL >= 1)
    XXH_ASSERT(customSeed != NULL);
#else
    if (customSeed == NULL) return XXH_ERROR;
#endif


    {   size_t pos = 0;
        while (pos < secretSize) {
            size_t const toCopy = XXH_MIN((secretSize - pos), customSeedSize);
            memcpy((char*)secretBuffer + pos, customSeed, toCopy);
            pos += toCopy;
    }   }

    {   size_t const nbSeg16 = secretSize / 16;
        size_t n;
        XXH128_canonical_t scrambler;
        XXH128_canonicalFromHash(&scrambler, XXH128(customSeed, customSeedSize, 0));
        for (n=0; n<nbSeg16; n++) {
            XXH128_hash_t const h128 = XXH128(&scrambler, sizeof(scrambler), n);
            XXH3_combine16((char*)secretBuffer + n*16, h128);
        }

        XXH3_combine16((char*)secretBuffer + secretSize - 16, XXH128_hashFromCanonical(&scrambler));
    }
    return XXH_OK;
}


XXH_PUBLIC_API void
XXH3_generateSecret_fromSeed(XXH_NOESCAPE void* secretBuffer, XXH64_hash_t seed)
{
    XXH_ALIGN(XXH_SEC_ALIGN) xxh_u8 secret[XXH_SECRET_DEFAULT_SIZE];
    XXH3_initCustomSecret(secret, seed);
    XXH_ASSERT(secretBuffer != NULL);
    memcpy(secretBuffer, secret, XXH_SECRET_DEFAULT_SIZE);
}


#if XXH_VECTOR == XXH_AVX2  \
  && defined(__GNUC__) && !defined(__clang__)  \
  && defined(__OPTIMIZE__) && XXH_SIZE_OPT <= 0
#  pragma GCC pop_options
#endif


#if defined (__cplusplus)
}
#endif

#endif
#endif


#endif

#ifndef ZSTD_NO_TRACE


#ifndef ZSTD_TRACE_H
#define ZSTD_TRACE_H

#include <stddef.h>


#if !defined(ZSTD_HAVE_WEAK_SYMBOLS) && \
    defined(__GNUC__) && defined(__ELF__) && \
    (defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || \
     defined(_M_IX86) || defined(__aarch64__) || defined(__riscv)) && \
    !defined(__APPLE__) && !defined(_WIN32) && !defined(__MINGW32__) && \
    !defined(__CYGWIN__) && !defined(_AIX)
#  define ZSTD_HAVE_WEAK_SYMBOLS 1
#else
#  define ZSTD_HAVE_WEAK_SYMBOLS 0
#endif
#if ZSTD_HAVE_WEAK_SYMBOLS
#  define ZSTD_WEAK_ATTR __attribute__((__weak__))
#else
#  define ZSTD_WEAK_ATTR
#endif


#ifndef ZSTD_TRACE
#  define ZSTD_TRACE ZSTD_HAVE_WEAK_SYMBOLS
#endif

#if ZSTD_TRACE

struct ZSTD_CCtx_s;
struct ZSTD_DCtx_s;
struct ZSTD_CCtx_params_s;

typedef struct {

    unsigned version;

    int streaming;

    unsigned dictionaryID;

    int dictionaryIsCold;

    size_t dictionarySize;

    size_t uncompressedSize;

    size_t compressedSize;

    struct ZSTD_CCtx_params_s const* params;

    struct ZSTD_CCtx_s const* cctx;

    struct ZSTD_DCtx_s const* dctx;
} ZSTD_Trace;


typedef unsigned long long ZSTD_TraceCtx;


ZSTD_WEAK_ATTR ZSTD_TraceCtx ZSTD_trace_compress_begin(
    struct ZSTD_CCtx_s const* cctx);


ZSTD_WEAK_ATTR void ZSTD_trace_compress_end(
    ZSTD_TraceCtx ctx,
    ZSTD_Trace const* trace);


ZSTD_WEAK_ATTR ZSTD_TraceCtx ZSTD_trace_decompress_begin(
    struct ZSTD_DCtx_s const* dctx);


ZSTD_WEAK_ATTR void ZSTD_trace_decompress_end(
    ZSTD_TraceCtx ctx,
    ZSTD_Trace const* trace);

#endif

#endif

#else
#  define ZSTD_TRACE 0
#endif


#define ZSTD_STATIC_ASSERT(c) DEBUG_STATIC_ASSERT(c)
#define ZSTD_isError ERR_isError
#define FSE_isError  ERR_isError
#define HUF_isError  ERR_isError


#undef MIN
#undef MAX
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define BOUNDED(min,val,max) (MAX(min,MIN(val,max)))


#define ZSTD_OPT_NUM    (1<<12)

#define ZSTD_REP_NUM      3
static UNUSED_ATTR const U32 repStartValue[ZSTD_REP_NUM] = { 1, 4, 8 };

#define KB *(1 <<10)
#define MB *(1 <<20)
#define GB *(1U<<30)

#define BIT7 128
#define BIT6  64
#define BIT5  32
#define BIT4  16
#define BIT1   2
#define BIT0   1

#define ZSTD_WINDOWLOG_ABSOLUTEMIN 10
static UNUSED_ATTR const size_t ZSTD_fcs_fieldSize[4] = { 0, 2, 4, 8 };
static UNUSED_ATTR const size_t ZSTD_did_fieldSize[4] = { 0, 1, 2, 4 };

#define ZSTD_FRAMEIDSIZE 4

#define ZSTD_BLOCKHEADERSIZE 3
static UNUSED_ATTR const size_t ZSTD_blockHeaderSize = ZSTD_BLOCKHEADERSIZE;
typedef enum { bt_raw, bt_rle, bt_compressed, bt_reserved } blockType_e;

#define ZSTD_FRAMECHECKSUMSIZE 4

#define MIN_SEQUENCES_SIZE 1
#define MIN_CBLOCK_SIZE (1  + 1 )
#define MIN_LITERALS_FOR_4_STREAMS 6

typedef enum { set_basic, set_rle, set_compressed, set_repeat } SymbolEncodingType_e;

#define LONGNBSEQ 0x7F00

#define MINMATCH 3

#define Litbits  8
#define LitHufLog 11
#define MaxLit ((1<<Litbits) - 1)
#define MaxML   52
#define MaxLL   35
#define DefaultMaxOff 28
#define MaxOff  31
#define MaxSeq MAX(MaxLL, MaxML)
#define MLFSELog    9
#define LLFSELog    9
#define OffFSELog   8
#define MaxFSELog  MAX(MAX(MLFSELog, LLFSELog), OffFSELog)
#define MaxMLBits 16
#define MaxLLBits 16

#define ZSTD_MAX_HUF_HEADER_SIZE 128

#define ZSTD_MAX_FSE_HEADERS_SIZE (((MaxML + 1) * MLFSELog + (MaxLL + 1) * LLFSELog + (MaxOff + 1) * OffFSELog + 7) / 8)

static UNUSED_ATTR const U8 LL_bits[MaxLL+1] = {
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     1, 1, 1, 1, 2, 2, 3, 3,
     4, 6, 7, 8, 9,10,11,12,
    13,14,15,16
};
static UNUSED_ATTR const S16 LL_defaultNorm[MaxLL+1] = {
     4, 3, 2, 2, 2, 2, 2, 2,
     2, 2, 2, 2, 2, 1, 1, 1,
     2, 2, 2, 2, 2, 2, 2, 2,
     2, 3, 2, 1, 1, 1, 1, 1,
    -1,-1,-1,-1
};
#define LL_DEFAULTNORMLOG 6
static UNUSED_ATTR const U32 LL_defaultNormLog = LL_DEFAULTNORMLOG;

static UNUSED_ATTR const U8 ML_bits[MaxML+1] = {
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     1, 1, 1, 1, 2, 2, 3, 3,
     4, 4, 5, 7, 8, 9,10,11,
    12,13,14,15,16
};
static UNUSED_ATTR const S16 ML_defaultNorm[MaxML+1] = {
     1, 4, 3, 2, 2, 2, 2, 2,
     2, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1,-1,-1,
    -1,-1,-1,-1,-1
};
#define ML_DEFAULTNORMLOG 6
static UNUSED_ATTR const U32 ML_defaultNormLog = ML_DEFAULTNORMLOG;

static UNUSED_ATTR const S16 OF_defaultNorm[DefaultMaxOff+1] = {
     1, 1, 1, 1, 1, 1, 2, 2,
     2, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
    -1,-1,-1,-1,-1
};
#define OF_DEFAULTNORMLOG 5
static UNUSED_ATTR const U32 OF_defaultNormLog = OF_DEFAULTNORMLOG;


static void ZSTD_copy8(void* dst, const void* src) {
#if defined(ZSTD_ARCH_ARM_NEON) && !defined(__aarch64__)
    vst1_u8((uint8_t*)dst, vld1_u8((const uint8_t*)src));
#else
    ZSTD_memcpy(dst, src, 8);
#endif
}
#define COPY8(d,s) do { ZSTD_copy8(d,s); d+=8; s+=8; } while (0)


static void ZSTD_copy16(void* dst, const void* src) {
#if defined(ZSTD_ARCH_ARM_NEON)
    vst1q_u8((uint8_t*)dst, vld1q_u8((const uint8_t*)src));
#elif defined(ZSTD_ARCH_X86_SSE2)
    _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((const __m128i*)src));
#elif defined(ZSTD_ARCH_RISCV_RVV)
    __riscv_vse8_v_u8m1((uint8_t*)dst, __riscv_vle8_v_u8m1((const uint8_t*)src, 16), 16);
#elif defined(__clang__)
    ZSTD_memmove(dst, src, 16);
#else

    BYTE copy16_buf[16];
    ZSTD_memcpy(copy16_buf, src, 16);
    ZSTD_memcpy(dst, copy16_buf, 16);
#endif
}
#define COPY16(d,s) do { ZSTD_copy16(d,s); d+=16; s+=16; } while (0)

#define WILDCOPY_OVERLENGTH 32
#define WILDCOPY_VECLEN 16

typedef enum {
    ZSTD_no_overlap,
    ZSTD_overlap_src_before_dst

} ZSTD_overlap_e;


MEM_STATIC FORCE_INLINE_ATTR
void ZSTD_wildcopy(void* dst, const void* src, size_t length, ZSTD_overlap_e const ovtype)
{
    ptrdiff_t diff = (BYTE*)dst - (const BYTE*)src;
    const BYTE* ip = (const BYTE*)src;
    BYTE* op = (BYTE*)dst;
    BYTE* const oend = op + length;

    if (ovtype == ZSTD_overlap_src_before_dst && diff < WILDCOPY_VECLEN) {

        do {
            COPY8(op, ip);
        } while (op < oend);
    } else {
        assert(diff >= WILDCOPY_VECLEN || diff <= -WILDCOPY_VECLEN);

        ZSTD_copy16(op, ip);
        if (16 >= length) return;
        op += 16;
        ip += 16;
        do {
            COPY16(op, ip);
            COPY16(op, ip);
        }
        while (op < oend);
    }
}

MEM_STATIC size_t ZSTD_limitCopy(void* dst, size_t dstCapacity, const void* src, size_t srcSize)
{
    size_t const length = MIN(dstCapacity, srcSize);
    if (length > 0) {
        ZSTD_memcpy(dst, src, length);
    }
    return length;
}


#define ZSTD_WORKSPACETOOLARGE_FACTOR 3


#define ZSTD_WORKSPACETOOLARGE_MAXDURATION 128


typedef enum {
    ZSTD_bm_buffered = 0,
    ZSTD_bm_stable = 1
} ZSTD_bufferMode_e;


typedef struct {
    size_t nbBlocks;
    size_t compressedSize;
    unsigned long long decompressedBound;
} ZSTD_frameSizeInfo;


void ZSTD_invalidateRepCodes(ZSTD_CCtx* cctx);


typedef struct {
    blockType_e blockType;
    U32 lastBlock;
    U32 origSize;
} blockProperties_t;


size_t ZSTD_getcBlockSize(const void* src, size_t srcSize,
                          blockProperties_t* bpPtr);


size_t ZSTD_decodeSeqHeaders(ZSTD_DCtx* dctx, int* nbSeqPtr,
                       const void* src, size_t srcSize);


MEM_STATIC int ZSTD_cpuSupportsBmi2(void)
{
    ZSTD_cpuid_t cpuid = ZSTD_cpuid();
    return ZSTD_cpuid_bmi1(cpuid) && ZSTD_cpuid_bmi2(cpuid);
}

#endif


unsigned ZSTD_versionNumber(void) { return ZSTD_VERSION_NUMBER; }

const char* ZSTD_versionString(void) { return ZSTD_VERSION_STRING; }


#undef ZSTD_isError

unsigned ZSTD_isError(size_t code) { return ERR_isError(code); }


const char* ZSTD_getErrorName(size_t code) { return ERR_getErrorName(code); }


ZSTD_ErrorCode ZSTD_getErrorCode(size_t code) { return ERR_getErrorCode(code); }


const char* ZSTD_getErrorString(ZSTD_ErrorCode code) { return ERR_getErrorString(code); }

int ZSTD_isDeterministicBuild(void)
{
#if ZSTD_IS_DETERMINISTIC_BUILD
    return 1;
#else
    return 0;
#endif
}


#include <stddef.h>


#define HUF_DECODER_FAST_TABLELOG 11


#ifdef HUF_DISABLE_FAST_DECODE
# define HUF_ENABLE_FAST_DECODE 0
#else
# define HUF_ENABLE_FAST_DECODE 1
#endif


#if defined(HUF_FORCE_DECOMPRESS_X1) && \
    defined(HUF_FORCE_DECOMPRESS_X2)
#error "Cannot force the use of the X1 and X2 decoders at the same time!"
#endif


#if DYNAMIC_BMI2
# define HUF_FAST_BMI2_ATTRS BMI2_TARGET_ATTRIBUTE
#else
# define HUF_FAST_BMI2_ATTRS
#endif

#ifdef __cplusplus
# define HUF_EXTERN_C extern "C"
#else
# define HUF_EXTERN_C
#endif
#define HUF_ASM_DECL HUF_EXTERN_C

#if DYNAMIC_BMI2
# define HUF_NEED_BMI2_FUNCTION 1
#else
# define HUF_NEED_BMI2_FUNCTION 0
#endif


#define HUF_isError ERR_isError


#define HUF_ALIGN(x, a)         HUF_ALIGN_MASK((x), (a) - 1)
#define HUF_ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))


typedef size_t (*HUF_DecompressUsingDTableFn)(void *dst, size_t dstSize,
                                              const void *cSrc,
                                              size_t cSrcSize,
                                              const HUF_DTable *DTable);

#if DYNAMIC_BMI2

#define HUF_DGEN(fn)                                                        \
                                                                            \
    static size_t fn##_default(                                             \
                  void* dst,  size_t dstSize,                               \
            const void* cSrc, size_t cSrcSize,                              \
            const HUF_DTable* DTable)                                       \
    {                                                                       \
        return fn##_body(dst, dstSize, cSrc, cSrcSize, DTable);             \
    }                                                                       \
                                                                            \
    static BMI2_TARGET_ATTRIBUTE size_t fn##_bmi2(                          \
                  void* dst,  size_t dstSize,                               \
            const void* cSrc, size_t cSrcSize,                              \
            const HUF_DTable* DTable)                                       \
    {                                                                       \
        return fn##_body(dst, dstSize, cSrc, cSrcSize, DTable);             \
    }                                                                       \
                                                                            \
    static size_t fn(void* dst, size_t dstSize, void const* cSrc,           \
                     size_t cSrcSize, HUF_DTable const* DTable, int flags)  \
    {                                                                       \
        if (flags & HUF_flags_bmi2) {                                       \
            return fn##_bmi2(dst, dstSize, cSrc, cSrcSize, DTable);         \
        }                                                                   \
        return fn##_default(dst, dstSize, cSrc, cSrcSize, DTable);          \
    }

#else

#define HUF_DGEN(fn)                                                        \
    static size_t fn(void* dst, size_t dstSize, void const* cSrc,           \
                     size_t cSrcSize, HUF_DTable const* DTable, int flags)  \
    {                                                                       \
        (void)flags;                                                        \
        return fn##_body(dst, dstSize, cSrc, cSrcSize, DTable);             \
    }

#endif


typedef struct { BYTE maxTableLog; BYTE tableType; BYTE tableLog; BYTE reserved; } DTableDesc;

static DTableDesc HUF_getDTableDesc(const HUF_DTable* table)
{
    DTableDesc dtd;
    ZSTD_memcpy(&dtd, table, sizeof(dtd));
    return dtd;
}

static size_t HUF_initFastDStream(BYTE const* ip) {
    BYTE const lastByte = ip[7];
    size_t const bitsConsumed = lastByte ? 8 - ZSTD_highbit32(lastByte) : 0;
    size_t const value = MEM_readLEST(ip) | 1;
    assert(bitsConsumed <= 8);
    assert(sizeof(size_t) == 8);
    return value << bitsConsumed;
}


typedef struct {
    BYTE const* ip[4];
    BYTE* op[4];
    U64 bits[4];
    void const* dt;
    BYTE const* ilowest;
    BYTE* oend;
    BYTE const* iend[4];
} HUF_DecompressFastArgs;

typedef void (*HUF_DecompressFastLoopFn)(HUF_DecompressFastArgs*);


static size_t HUF_DecompressFastArgs_init(HUF_DecompressFastArgs* args, void* dst, size_t dstSize, void const* src, size_t srcSize, const HUF_DTable* DTable)
{
    void const* dt = DTable + 1;
    U32 const dtLog = HUF_getDTableDesc(DTable).tableLog;

    const BYTE* const istart = (const BYTE*)src;

    BYTE* const oend = (BYTE*)ZSTD_maybeNullPtrAdd(dst, (ptrdiff_t)dstSize);


    if (!MEM_isLittleEndian() || MEM_32bits())
        return 0;


    if (dstSize == 0)
        return 0;
    assert(dst != NULL);


    if (srcSize < 10)
        return ERROR(corruption_detected);


    if (dtLog != HUF_DECODER_FAST_TABLELOG)
        return 0;


    {
        size_t const length1 = MEM_readLE16(istart);
        size_t const length2 = MEM_readLE16(istart+2);
        size_t const length3 = MEM_readLE16(istart+4);
        size_t const length4 = srcSize - (length1 + length2 + length3 + 6);
        args->iend[0] = istart + 6;
        args->iend[1] = args->iend[0] + length1;
        args->iend[2] = args->iend[1] + length2;
        args->iend[3] = args->iend[2] + length3;


        if (length1 < 8 || length2 < 8 || length3 < 8 || length4 < 8)
            return 0;
        if (length4 > srcSize) return ERROR(corruption_detected);
    }

    args->ip[0] = args->iend[1] - sizeof(U64);
    args->ip[1] = args->iend[2] - sizeof(U64);
    args->ip[2] = args->iend[3] - sizeof(U64);
    args->ip[3] = (BYTE const*)src + srcSize - sizeof(U64);


    args->op[0] = (BYTE*)dst;
    args->op[1] = args->op[0] + (dstSize+3)/4;
    args->op[2] = args->op[1] + (dstSize+3)/4;
    args->op[3] = args->op[2] + (dstSize+3)/4;


    if (args->op[3] >= oend)
        return 0;


    args->bits[0] = HUF_initFastDStream(args->ip[0]);
    args->bits[1] = HUF_initFastDStream(args->ip[1]);
    args->bits[2] = HUF_initFastDStream(args->ip[2]);
    args->bits[3] = HUF_initFastDStream(args->ip[3]);


    args->ilowest = istart;

    args->oend = oend;
    args->dt = dt;

    return 1;
}

static size_t HUF_initRemainingDStream(BIT_DStream_t* bit, HUF_DecompressFastArgs const* args, int stream, BYTE* segmentEnd)
{

    if (args->op[stream] > segmentEnd)
        return ERROR(corruption_detected);

    if (args->ip[stream] < args->iend[stream] - 8)
        return ERROR(corruption_detected);


    assert(sizeof(size_t) == 8);
    bit->bitContainer = MEM_readLEST(args->ip[stream]);
    bit->bitsConsumed = ZSTD_countTrailingZeros64(args->bits[stream]);
    bit->start = (const char*)args->ilowest;
    bit->limitPtr = bit->start + sizeof(size_t);
    bit->ptr = (const char*)args->ip[stream];

    return 0;
}


#define HUF_4X_FOR_EACH_STREAM(X) \
    do {                          \
        X(0);                     \
        X(1);                     \
        X(2);                     \
        X(3);                     \
    } while (0)


#define HUF_4X_FOR_EACH_STREAM_WITH_VAR(X, var) \
    do {                                        \
        X(0, (var));                            \
        X(1, (var));                            \
        X(2, (var));                            \
        X(3, (var));                            \
    } while (0)


#ifndef HUF_FORCE_DECOMPRESS_X2


typedef struct { BYTE nbBits; BYTE byte; } HUF_DEltX1;


static U64 HUF_DEltX1_set4(BYTE symbol, BYTE nbBits) {
    U64 D4;
    if (MEM_isLittleEndian()) {
        D4 = (U64)((symbol << 8) + nbBits);
    } else {
        D4 = (U64)(symbol + (nbBits << 8));
    }
    assert(D4 < (1U << 16));
    D4 *= 0x0001000100010001ULL;
    return D4;
}


static U32 HUF_rescaleStats(BYTE* huffWeight, U32* rankVal, U32 nbSymbols, U32 tableLog, U32 targetTableLog)
{
    if (tableLog > targetTableLog)
        return tableLog;
    if (tableLog < targetTableLog) {
        U32 const scale = targetTableLog - tableLog;
        U32 s;

        for (s = 0; s < nbSymbols; ++s) {
            huffWeight[s] += (BYTE)((huffWeight[s] == 0) ? 0 : scale);
        }

        for (s = targetTableLog; s > scale; --s) {
            rankVal[s] = rankVal[s - scale];
        }
        for (s = scale; s > 0; --s) {
            rankVal[s] = 0;
        }
    }
    return targetTableLog;
}

typedef struct {
        U32 rankVal[HUF_TABLELOG_ABSOLUTEMAX + 1];
        U32 rankStart[HUF_TABLELOG_ABSOLUTEMAX + 1];
        U32 statsWksp[HUF_READ_STATS_WORKSPACE_SIZE_U32];
        BYTE symbols[HUF_SYMBOLVALUE_MAX + 1];
        BYTE huffWeight[HUF_SYMBOLVALUE_MAX + 1];
} HUF_ReadDTableX1_Workspace;

size_t HUF_readDTableX1_wksp(HUF_DTable* DTable, const void* src, size_t srcSize, void* workSpace, size_t wkspSize, int flags)
{
    U32 tableLog = 0;
    U32 nbSymbols = 0;
    size_t iSize;
    void* const dtPtr = DTable + 1;
    HUF_DEltX1* const dt = (HUF_DEltX1*)dtPtr;
    HUF_ReadDTableX1_Workspace* wksp = (HUF_ReadDTableX1_Workspace*)workSpace;

    DEBUG_STATIC_ASSERT(HUF_DECOMPRESS_WORKSPACE_SIZE >= sizeof(*wksp));
    if (sizeof(*wksp) > wkspSize) return ERROR(tableLog_tooLarge);

    DEBUG_STATIC_ASSERT(sizeof(DTableDesc) == sizeof(HUF_DTable));


    iSize = HUF_readStats_wksp(wksp->huffWeight, HUF_SYMBOLVALUE_MAX + 1, wksp->rankVal, &nbSymbols, &tableLog, src, srcSize, wksp->statsWksp, sizeof(wksp->statsWksp), flags);
    if (HUF_isError(iSize)) return iSize;


    {   DTableDesc dtd = HUF_getDTableDesc(DTable);
        U32 const maxTableLog = dtd.maxTableLog + 1;
        U32 const targetTableLog = MIN(maxTableLog, HUF_DECODER_FAST_TABLELOG);
        tableLog = HUF_rescaleStats(wksp->huffWeight, wksp->rankVal, nbSymbols, tableLog, targetTableLog);
        if (tableLog > (U32)(dtd.maxTableLog+1)) return ERROR(tableLog_tooLarge);
        dtd.tableType = 0;
        dtd.tableLog = (BYTE)tableLog;
        ZSTD_memcpy(DTable, &dtd, sizeof(dtd));
    }


    {   int n;
        U32 nextRankStart = 0;
        int const unroll = 4;
        int const nLimit = (int)nbSymbols - unroll + 1;
        for (n=0; n<(int)tableLog+1; n++) {
            U32 const curr = nextRankStart;
            nextRankStart += wksp->rankVal[n];
            wksp->rankStart[n] = curr;
        }
        for (n=0; n < nLimit; n += unroll) {
            int u;
            for (u=0; u < unroll; ++u) {
                size_t const w = wksp->huffWeight[n+u];
                wksp->symbols[wksp->rankStart[w]++] = (BYTE)(n+u);
            }
        }
        for (; n < (int)nbSymbols; ++n) {
            size_t const w = wksp->huffWeight[n];
            wksp->symbols[wksp->rankStart[w]++] = (BYTE)n;
        }
    }


    {   U32 w;
        int symbol = wksp->rankVal[0];
        int rankStart = 0;
        for (w=1; w<tableLog+1; ++w) {
            int const symbolCount = wksp->rankVal[w];
            int const length = (1 << w) >> 1;
            int uStart = rankStart;
            BYTE const nbBits = (BYTE)(tableLog + 1 - w);
            int s;
            int u;
            switch (length) {
            case 1:
                for (s=0; s<symbolCount; ++s) {
                    HUF_DEltX1 D;
                    D.byte = wksp->symbols[symbol + s];
                    D.nbBits = nbBits;
                    dt[uStart] = D;
                    uStart += 1;
                }
                break;
            case 2:
                for (s=0; s<symbolCount; ++s) {
                    HUF_DEltX1 D;
                    D.byte = wksp->symbols[symbol + s];
                    D.nbBits = nbBits;
                    dt[uStart+0] = D;
                    dt[uStart+1] = D;
                    uStart += 2;
                }
                break;
            case 4:
                for (s=0; s<symbolCount; ++s) {
                    U64 const D4 = HUF_DEltX1_set4(wksp->symbols[symbol + s], nbBits);
                    MEM_write64(dt + uStart, D4);
                    uStart += 4;
                }
                break;
            case 8:
                for (s=0; s<symbolCount; ++s) {
                    U64 const D4 = HUF_DEltX1_set4(wksp->symbols[symbol + s], nbBits);
                    MEM_write64(dt + uStart, D4);
                    MEM_write64(dt + uStart + 4, D4);
                    uStart += 8;
                }
                break;
            default:
                for (s=0; s<symbolCount; ++s) {
                    U64 const D4 = HUF_DEltX1_set4(wksp->symbols[symbol + s], nbBits);
                    for (u=0; u < length; u += 16) {
                        MEM_write64(dt + uStart + u + 0, D4);
                        MEM_write64(dt + uStart + u + 4, D4);
                        MEM_write64(dt + uStart + u + 8, D4);
                        MEM_write64(dt + uStart + u + 12, D4);
                    }
                    assert(u == length);
                    uStart += length;
                }
                break;
            }
            symbol += symbolCount;
            rankStart += symbolCount * length;
        }
    }
    return iSize;
}

FORCE_INLINE_TEMPLATE BYTE
HUF_decodeSymbolX1(BIT_DStream_t* Dstream, const HUF_DEltX1* dt, const U32 dtLog)
{
    size_t const val = BIT_lookBitsFast(Dstream, dtLog);
    BYTE const c = dt[val].byte;
    BIT_skipBits(Dstream, dt[val].nbBits);
    return c;
}

#define HUF_DECODE_SYMBOLX1_0(ptr, DStreamPtr) \
    do { *ptr++ = HUF_decodeSymbolX1(DStreamPtr, dt, dtLog); } while (0)

#define HUF_DECODE_SYMBOLX1_1(ptr, DStreamPtr)      \
    do {                                            \
        if (MEM_64bits() || (HUF_TABLELOG_MAX<=12)) \
            HUF_DECODE_SYMBOLX1_0(ptr, DStreamPtr); \
    } while (0)

#define HUF_DECODE_SYMBOLX1_2(ptr, DStreamPtr)      \
    do {                                            \
        if (MEM_64bits())                           \
            HUF_DECODE_SYMBOLX1_0(ptr, DStreamPtr); \
    } while (0)

HINT_INLINE size_t
HUF_decodeStreamX1(BYTE* p, BIT_DStream_t* const bitDPtr, BYTE* const pEnd, const HUF_DEltX1* const dt, const U32 dtLog)
{
    BYTE* const pStart = p;


    if ((pEnd - p) > 3) {
        while ((BIT_reloadDStream(bitDPtr) == BIT_DStream_unfinished) & (p < pEnd-3)) {
            HUF_DECODE_SYMBOLX1_2(p, bitDPtr);
            HUF_DECODE_SYMBOLX1_1(p, bitDPtr);
            HUF_DECODE_SYMBOLX1_2(p, bitDPtr);
            HUF_DECODE_SYMBOLX1_0(p, bitDPtr);
        }
    } else {
        BIT_reloadDStream(bitDPtr);
    }


    if (MEM_32bits())
        while ((BIT_reloadDStream(bitDPtr) == BIT_DStream_unfinished) & (p < pEnd))
            HUF_DECODE_SYMBOLX1_0(p, bitDPtr);


    while (p < pEnd)
        HUF_DECODE_SYMBOLX1_0(p, bitDPtr);

    return (size_t)(pEnd-pStart);
}

FORCE_INLINE_TEMPLATE size_t
HUF_decompress1X1_usingDTable_internal_body(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const HUF_DTable* DTable)
{
    BYTE* op = (BYTE*)dst;
    BYTE* const oend = (BYTE*)ZSTD_maybeNullPtrAdd(op, (ptrdiff_t)dstSize);
    const void* dtPtr = DTable + 1;
    const HUF_DEltX1* const dt = (const HUF_DEltX1*)dtPtr;
    BIT_DStream_t bitD;
    DTableDesc const dtd = HUF_getDTableDesc(DTable);
    U32 const dtLog = dtd.tableLog;

    CHECK_F( BIT_initDStream(&bitD, cSrc, cSrcSize) );

    HUF_decodeStreamX1(op, &bitD, oend, dt, dtLog);

    if (!BIT_endOfDStream(&bitD)) return ERROR(corruption_detected);

    return dstSize;
}


FORCE_INLINE_TEMPLATE size_t
HUF_decompress4X1_usingDTable_internal_body(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const HUF_DTable* DTable)
{

    if (cSrcSize < 10) return ERROR(corruption_detected);
    if (dstSize < 6) return ERROR(corruption_detected);

    {   const BYTE* const istart = (const BYTE*) cSrc;
        BYTE* const ostart = (BYTE*) dst;
        BYTE* const oend = ostart + dstSize;
        BYTE* const olimit = oend - 3;
        const void* const dtPtr = DTable + 1;
        const HUF_DEltX1* const dt = (const HUF_DEltX1*)dtPtr;


        BIT_DStream_t bitD1;
        BIT_DStream_t bitD2;
        BIT_DStream_t bitD3;
        BIT_DStream_t bitD4;
        size_t const length1 = MEM_readLE16(istart);
        size_t const length2 = MEM_readLE16(istart+2);
        size_t const length3 = MEM_readLE16(istart+4);
        size_t const length4 = cSrcSize - (length1 + length2 + length3 + 6);
        const BYTE* const istart1 = istart + 6;
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
        DTableDesc const dtd = HUF_getDTableDesc(DTable);
        U32 const dtLog = dtd.tableLog;
        U32 endSignal = 1;

        if (length4 > cSrcSize) return ERROR(corruption_detected);
        if (opStart4 > oend) return ERROR(corruption_detected);
        assert(dstSize >= 6);
        CHECK_F( BIT_initDStream(&bitD1, istart1, length1) );
        CHECK_F( BIT_initDStream(&bitD2, istart2, length2) );
        CHECK_F( BIT_initDStream(&bitD3, istart3, length3) );
        CHECK_F( BIT_initDStream(&bitD4, istart4, length4) );


        if ((size_t)(oend - op4) >= sizeof(size_t)) {
            for ( ; (endSignal) & (op4 < olimit) ; ) {
                HUF_DECODE_SYMBOLX1_2(op1, &bitD1);
                HUF_DECODE_SYMBOLX1_2(op2, &bitD2);
                HUF_DECODE_SYMBOLX1_2(op3, &bitD3);
                HUF_DECODE_SYMBOLX1_2(op4, &bitD4);
                HUF_DECODE_SYMBOLX1_1(op1, &bitD1);
                HUF_DECODE_SYMBOLX1_1(op2, &bitD2);
                HUF_DECODE_SYMBOLX1_1(op3, &bitD3);
                HUF_DECODE_SYMBOLX1_1(op4, &bitD4);
                HUF_DECODE_SYMBOLX1_2(op1, &bitD1);
                HUF_DECODE_SYMBOLX1_2(op2, &bitD2);
                HUF_DECODE_SYMBOLX1_2(op3, &bitD3);
                HUF_DECODE_SYMBOLX1_2(op4, &bitD4);
                HUF_DECODE_SYMBOLX1_0(op1, &bitD1);
                HUF_DECODE_SYMBOLX1_0(op2, &bitD2);
                HUF_DECODE_SYMBOLX1_0(op3, &bitD3);
                HUF_DECODE_SYMBOLX1_0(op4, &bitD4);
                endSignal &= BIT_reloadDStreamFast(&bitD1) == BIT_DStream_unfinished;
                endSignal &= BIT_reloadDStreamFast(&bitD2) == BIT_DStream_unfinished;
                endSignal &= BIT_reloadDStreamFast(&bitD3) == BIT_DStream_unfinished;
                endSignal &= BIT_reloadDStreamFast(&bitD4) == BIT_DStream_unfinished;
            }
        }


        if (op1 > opStart2) return ERROR(corruption_detected);
        if (op2 > opStart3) return ERROR(corruption_detected);
        if (op3 > opStart4) return ERROR(corruption_detected);


        HUF_decodeStreamX1(op1, &bitD1, opStart2, dt, dtLog);
        HUF_decodeStreamX1(op2, &bitD2, opStart3, dt, dtLog);
        HUF_decodeStreamX1(op3, &bitD3, opStart4, dt, dtLog);
        HUF_decodeStreamX1(op4, &bitD4, oend,     dt, dtLog);


        { U32 const endCheck = BIT_endOfDStream(&bitD1) & BIT_endOfDStream(&bitD2) & BIT_endOfDStream(&bitD3) & BIT_endOfDStream(&bitD4);
          if (!endCheck) return ERROR(corruption_detected); }


        return dstSize;
    }
}

#if HUF_NEED_BMI2_FUNCTION
static BMI2_TARGET_ATTRIBUTE
size_t HUF_decompress4X1_usingDTable_internal_bmi2(void* dst, size_t dstSize, void const* cSrc,
                    size_t cSrcSize, HUF_DTable const* DTable) {
    return HUF_decompress4X1_usingDTable_internal_body(dst, dstSize, cSrc, cSrcSize, DTable);
}
#endif

static
size_t HUF_decompress4X1_usingDTable_internal_default(void* dst, size_t dstSize, void const* cSrc,
                    size_t cSrcSize, HUF_DTable const* DTable) {
    return HUF_decompress4X1_usingDTable_internal_body(dst, dstSize, cSrc, cSrcSize, DTable);
}

#if ZSTD_ENABLE_ASM_X86_64_BMI2

HUF_ASM_DECL void HUF_decompress4X1_usingDTable_internal_fast_asm_loop(HUF_DecompressFastArgs* args) ZSTDLIB_HIDDEN;

#endif

static HUF_FAST_BMI2_ATTRS
void HUF_decompress4X1_usingDTable_internal_fast_c_loop(HUF_DecompressFastArgs* args)
{
    U64 bits[4];
    BYTE const* ip[4];
    BYTE* op[4];
    U16 const* const dtable = (U16 const*)args->dt;
    BYTE* const oend = args->oend;
    BYTE const* const ilowest = args->ilowest;


    ZSTD_memcpy(&bits, &args->bits, sizeof(bits));
    ZSTD_memcpy((void*)(&ip), &args->ip, sizeof(ip));
    ZSTD_memcpy(&op, &args->op, sizeof(op));

    assert(MEM_isLittleEndian());
    assert(!MEM_32bits());

    for (;;) {
        BYTE* olimit;
        int stream;


#ifndef NDEBUG
        for (stream = 0; stream < 4; ++stream) {
            assert(op[stream] <= (stream == 3 ? oend : op[stream + 1]));
            assert(ip[stream] >= ilowest);
        }
#endif

        {

            size_t const oiters = (size_t)(oend - op[3]) / 5;

            size_t const iiters = (size_t)(ip[0] - ilowest) / 7;

            size_t const iters = MIN(oiters, iiters);
            size_t const symbols = iters * 5;


            olimit = op[3] + symbols;


            if (op[3] == olimit)
                break;


            for (stream = 1; stream < 4; ++stream) {
                if (ip[stream] < ip[stream - 1])
                    goto _out;
            }
        }

#ifndef NDEBUG
        for (stream = 1; stream < 4; ++stream) {
            assert(ip[stream] >= ip[stream - 1]);
        }
#endif

#define HUF_4X1_DECODE_SYMBOL(_stream, _symbol)        \
    do {                                               \
        U64 const index = bits[(_stream)] >> 53;       \
        U16 const entry = dtable[index];               \
        bits[(_stream)] <<= entry & 0x3F;              \
        op[(_stream)][(_symbol)] = (BYTE)(entry >> 8); \
    } while (0)

#define HUF_5X1_RELOAD_STREAM(_stream)                              \
    do {                                                            \
        U64 const ctz = ZSTD_countTrailingZeros64(bits[(_stream)]); \
        U64 const nbBits = ctz & 7;                                 \
        U64 const nbBytes = ctz >> 3;                               \
        op[(_stream)] += 5;                                         \
        ip[(_stream)] -= nbBytes;                                   \
        bits[(_stream)] = MEM_read64(ip[(_stream)]) | 1;            \
        bits[(_stream)] <<= nbBits;                                 \
    } while (0)


        do {

            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X1_DECODE_SYMBOL, 0);
            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X1_DECODE_SYMBOL, 1);
            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X1_DECODE_SYMBOL, 2);
            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X1_DECODE_SYMBOL, 3);
            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X1_DECODE_SYMBOL, 4);


            HUF_4X_FOR_EACH_STREAM(HUF_5X1_RELOAD_STREAM);
        } while (op[3] < olimit);

#undef HUF_4X1_DECODE_SYMBOL
#undef HUF_5X1_RELOAD_STREAM
    }

_out:


    ZSTD_memcpy(&args->bits, &bits, sizeof(bits));
    ZSTD_memcpy((void*)(&args->ip), &ip, sizeof(ip));
    ZSTD_memcpy(&args->op, &op, sizeof(op));
}


static HUF_FAST_BMI2_ATTRS
size_t
HUF_decompress4X1_usingDTable_internal_fast(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const HUF_DTable* DTable,
    HUF_DecompressFastLoopFn loopFn)
{
    void const* dt = DTable + 1;
    BYTE const* const ilowest = (BYTE const*)cSrc;
    BYTE* const oend = (BYTE*)ZSTD_maybeNullPtrAdd(dst, (ptrdiff_t)dstSize);
    HUF_DecompressFastArgs args;
    {   size_t const ret = HUF_DecompressFastArgs_init(&args, dst, dstSize, cSrc, cSrcSize, DTable);
        FORWARD_IF_ERROR(ret, "Failed to init fast loop args");
        if (ret == 0)
            return 0;
    }

    assert(args.ip[0] >= args.ilowest);
    loopFn(&args);


    assert(args.ip[0] >= ilowest);
    assert(args.ip[0] >= ilowest);
    assert(args.ip[1] >= ilowest);
    assert(args.ip[2] >= ilowest);
    assert(args.ip[3] >= ilowest);
    assert(args.op[3] <= oend);

    assert(ilowest == args.ilowest);
    assert(ilowest + 6 == args.iend[0]);
    (void)ilowest;


    {   size_t const segmentSize = (dstSize+3) / 4;
        BYTE* segmentEnd = (BYTE*)dst;
        int i;
        for (i = 0; i < 4; ++i) {
            BIT_DStream_t bit;
            if (segmentSize <= (size_t)(oend - segmentEnd))
                segmentEnd += segmentSize;
            else
                segmentEnd = oend;
            FORWARD_IF_ERROR(HUF_initRemainingDStream(&bit, &args, i, segmentEnd), "corruption");

            args.op[i] += HUF_decodeStreamX1(args.op[i], &bit, segmentEnd, (HUF_DEltX1 const*)dt, HUF_DECODER_FAST_TABLELOG);
            if (args.op[i] != segmentEnd) return ERROR(corruption_detected);
        }
    }


    assert(dstSize != 0);
    return dstSize;
}

HUF_DGEN(HUF_decompress1X1_usingDTable_internal)

static size_t HUF_decompress4X1_usingDTable_internal(void* dst, size_t dstSize, void const* cSrc,
                    size_t cSrcSize, HUF_DTable const* DTable, int flags)
{
    HUF_DecompressUsingDTableFn fallbackFn = HUF_decompress4X1_usingDTable_internal_default;
    HUF_DecompressFastLoopFn loopFn = HUF_decompress4X1_usingDTable_internal_fast_c_loop;

#if DYNAMIC_BMI2
    if (flags & HUF_flags_bmi2) {
        fallbackFn = HUF_decompress4X1_usingDTable_internal_bmi2;
# if ZSTD_ENABLE_ASM_X86_64_BMI2
        if (!(flags & HUF_flags_disableAsm)) {
            loopFn = HUF_decompress4X1_usingDTable_internal_fast_asm_loop;
        }
# endif
    } else {
        return fallbackFn(dst, dstSize, cSrc, cSrcSize, DTable);
    }
#endif

#if ZSTD_ENABLE_ASM_X86_64_BMI2 && defined(__BMI2__)
    if (!(flags & HUF_flags_disableAsm)) {
        loopFn = HUF_decompress4X1_usingDTable_internal_fast_asm_loop;
    }
#endif

    if (HUF_ENABLE_FAST_DECODE && !(flags & HUF_flags_disableFast)) {
        size_t const ret = HUF_decompress4X1_usingDTable_internal_fast(dst, dstSize, cSrc, cSrcSize, DTable, loopFn);
        if (ret != 0)
            return ret;
    }
    return fallbackFn(dst, dstSize, cSrc, cSrcSize, DTable);
}

static size_t HUF_decompress4X1_DCtx_wksp(HUF_DTable* dctx, void* dst, size_t dstSize,
                                   const void* cSrc, size_t cSrcSize,
                                   void* workSpace, size_t wkspSize, int flags)
{
    const BYTE* ip = (const BYTE*) cSrc;

    size_t const hSize = HUF_readDTableX1_wksp(dctx, cSrc, cSrcSize, workSpace, wkspSize, flags);
    if (HUF_isError(hSize)) return hSize;
    if (hSize >= cSrcSize) return ERROR(srcSize_wrong);
    ip += hSize; cSrcSize -= hSize;

    return HUF_decompress4X1_usingDTable_internal(dst, dstSize, ip, cSrcSize, dctx, flags);
}

#endif


#ifndef HUF_FORCE_DECOMPRESS_X1


typedef struct { U16 sequence; BYTE nbBits; BYTE length; } HUF_DEltX2;
typedef struct { BYTE symbol; } sortedSymbol_t;
typedef U32 rankValCol_t[HUF_TABLELOG_MAX + 1];
typedef rankValCol_t rankVal_t[HUF_TABLELOG_MAX];


static U32 HUF_buildDEltX2U32(U32 symbol, U32 nbBits, U32 baseSeq, int level)
{
    U32 seq;
    DEBUG_STATIC_ASSERT(offsetof(HUF_DEltX2, sequence) == 0);
    DEBUG_STATIC_ASSERT(offsetof(HUF_DEltX2, nbBits) == 2);
    DEBUG_STATIC_ASSERT(offsetof(HUF_DEltX2, length) == 3);
    DEBUG_STATIC_ASSERT(sizeof(HUF_DEltX2) == sizeof(U32));
    if (MEM_isLittleEndian()) {
        seq = level == 1 ? symbol : (baseSeq + (symbol << 8));
        return seq + (nbBits << 16) + ((U32)level << 24);
    } else {
        seq = level == 1 ? (symbol << 8) : ((baseSeq << 8) + symbol);
        return (seq << 16) + (nbBits << 8) + (U32)level;
    }
}


static HUF_DEltX2 HUF_buildDEltX2(U32 symbol, U32 nbBits, U32 baseSeq, int level)
{
    HUF_DEltX2 DElt;
    U32 const val = HUF_buildDEltX2U32(symbol, nbBits, baseSeq, level);
    DEBUG_STATIC_ASSERT(sizeof(DElt) == sizeof(val));
    ZSTD_memcpy(&DElt, &val, sizeof(val));
    return DElt;
}


static U64 HUF_buildDEltX2U64(U32 symbol, U32 nbBits, U16 baseSeq, int level)
{
    U32 DElt = HUF_buildDEltX2U32(symbol, nbBits, baseSeq, level);
    return (U64)DElt + ((U64)DElt << 32);
}


static void HUF_fillDTableX2ForWeight(
    HUF_DEltX2* DTableRank,
    sortedSymbol_t const* begin, sortedSymbol_t const* end,
    U32 nbBits, U32 tableLog,
    U16 baseSeq, int const level)
{
    U32 const length = 1U << ((tableLog - nbBits) & 0x1F );
    const sortedSymbol_t* ptr;
    assert(level >= 1 && level <= 2);
    switch (length) {
    case 1:
        for (ptr = begin; ptr != end; ++ptr) {
            HUF_DEltX2 const DElt = HUF_buildDEltX2(ptr->symbol, nbBits, baseSeq, level);
            *DTableRank++ = DElt;
        }
        break;
    case 2:
        for (ptr = begin; ptr != end; ++ptr) {
            HUF_DEltX2 const DElt = HUF_buildDEltX2(ptr->symbol, nbBits, baseSeq, level);
            DTableRank[0] = DElt;
            DTableRank[1] = DElt;
            DTableRank += 2;
        }
        break;
    case 4:
        for (ptr = begin; ptr != end; ++ptr) {
            U64 const DEltX2 = HUF_buildDEltX2U64(ptr->symbol, nbBits, baseSeq, level);
            ZSTD_memcpy(DTableRank + 0, &DEltX2, sizeof(DEltX2));
            ZSTD_memcpy(DTableRank + 2, &DEltX2, sizeof(DEltX2));
            DTableRank += 4;
        }
        break;
    case 8:
        for (ptr = begin; ptr != end; ++ptr) {
            U64 const DEltX2 = HUF_buildDEltX2U64(ptr->symbol, nbBits, baseSeq, level);
            ZSTD_memcpy(DTableRank + 0, &DEltX2, sizeof(DEltX2));
            ZSTD_memcpy(DTableRank + 2, &DEltX2, sizeof(DEltX2));
            ZSTD_memcpy(DTableRank + 4, &DEltX2, sizeof(DEltX2));
            ZSTD_memcpy(DTableRank + 6, &DEltX2, sizeof(DEltX2));
            DTableRank += 8;
        }
        break;
    default:
        for (ptr = begin; ptr != end; ++ptr) {
            U64 const DEltX2 = HUF_buildDEltX2U64(ptr->symbol, nbBits, baseSeq, level);
            HUF_DEltX2* const DTableRankEnd = DTableRank + length;
            for (; DTableRank != DTableRankEnd; DTableRank += 8) {
                ZSTD_memcpy(DTableRank + 0, &DEltX2, sizeof(DEltX2));
                ZSTD_memcpy(DTableRank + 2, &DEltX2, sizeof(DEltX2));
                ZSTD_memcpy(DTableRank + 4, &DEltX2, sizeof(DEltX2));
                ZSTD_memcpy(DTableRank + 6, &DEltX2, sizeof(DEltX2));
            }
        }
        break;
    }
}


static void HUF_fillDTableX2Level2(HUF_DEltX2* DTable, U32 targetLog, const U32 consumedBits,
                           const U32* rankVal, const int minWeight, const int maxWeight1,
                           const sortedSymbol_t* sortedSymbols, U32 const* rankStart,
                           U32 nbBitsBaseline, U16 baseSeq)
{

    if (minWeight>1) {
        U32 const length = 1U << ((targetLog - consumedBits) & 0x1F );
        U64 const DEltX2 = HUF_buildDEltX2U64(baseSeq, consumedBits,  0,  1);
        int const skipSize = rankVal[minWeight];
        assert(length > 1);
        assert((U32)skipSize < length);
        switch (length) {
        case 2:
            assert(skipSize == 1);
            ZSTD_memcpy(DTable, &DEltX2, sizeof(DEltX2));
            break;
        case 4:
            assert(skipSize <= 4);
            ZSTD_memcpy(DTable + 0, &DEltX2, sizeof(DEltX2));
            ZSTD_memcpy(DTable + 2, &DEltX2, sizeof(DEltX2));
            break;
        default:
            {
                int i;
                for (i = 0; i < skipSize; i += 8) {
                    ZSTD_memcpy(DTable + i + 0, &DEltX2, sizeof(DEltX2));
                    ZSTD_memcpy(DTable + i + 2, &DEltX2, sizeof(DEltX2));
                    ZSTD_memcpy(DTable + i + 4, &DEltX2, sizeof(DEltX2));
                    ZSTD_memcpy(DTable + i + 6, &DEltX2, sizeof(DEltX2));
                }
            }
        }
    }


    {
        int w;
        for (w = minWeight; w < maxWeight1; ++w) {
            int const begin = rankStart[w];
            int const end = rankStart[w+1];
            U32 const nbBits = nbBitsBaseline - w;
            U32 const totalBits = nbBits + consumedBits;
            HUF_fillDTableX2ForWeight(
                DTable + rankVal[w],
                sortedSymbols + begin, sortedSymbols + end,
                totalBits, targetLog,
                baseSeq,  2);
        }
    }
}

static void HUF_fillDTableX2(HUF_DEltX2* DTable, const U32 targetLog,
                           const sortedSymbol_t* sortedList,
                           const U32* rankStart, rankValCol_t* rankValOrigin, const U32 maxWeight,
                           const U32 nbBitsBaseline)
{
    U32* const rankVal = rankValOrigin[0];
    const int scaleLog = nbBitsBaseline - targetLog;
    const U32 minBits  = nbBitsBaseline - maxWeight;
    int w;
    int const wEnd = (int)maxWeight + 1;


    for (w = 1; w < wEnd; ++w) {
        int const begin = (int)rankStart[w];
        int const end = (int)rankStart[w+1];
        U32 const nbBits = nbBitsBaseline - w;

        if (targetLog-nbBits >= minBits) {

            int start = rankVal[w];
            U32 const length = 1U << ((targetLog - nbBits) & 0x1F );
            int minWeight = nbBits + scaleLog;
            int s;
            if (minWeight < 1) minWeight = 1;

            for (s = begin; s != end; ++s) {
                HUF_fillDTableX2Level2(
                    DTable + start, targetLog, nbBits,
                    rankValOrigin[nbBits], minWeight, wEnd,
                    sortedList, rankStart,
                    nbBitsBaseline, sortedList[s].symbol);
                start += length;
            }
        } else {

            HUF_fillDTableX2ForWeight(
                DTable + rankVal[w],
                sortedList + begin, sortedList + end,
                nbBits, targetLog,
                 0,  1);
        }
    }
}

typedef struct {
    rankValCol_t rankVal[HUF_TABLELOG_MAX];
    U32 rankStats[HUF_TABLELOG_MAX + 1];
    U32 rankStart0[HUF_TABLELOG_MAX + 3];
    sortedSymbol_t sortedSymbol[HUF_SYMBOLVALUE_MAX + 1];
    BYTE weightList[HUF_SYMBOLVALUE_MAX + 1];
    U32 calleeWksp[HUF_READ_STATS_WORKSPACE_SIZE_U32];
} HUF_ReadDTableX2_Workspace;

size_t HUF_readDTableX2_wksp(HUF_DTable* DTable,
                       const void* src, size_t srcSize,
                             void* workSpace, size_t wkspSize, int flags)
{
    U32 tableLog, maxW, nbSymbols;
    DTableDesc dtd = HUF_getDTableDesc(DTable);
    U32 maxTableLog = dtd.maxTableLog;
    size_t iSize;
    void* dtPtr = DTable+1;
    HUF_DEltX2* const dt = (HUF_DEltX2*)dtPtr;
    U32 *rankStart;

    HUF_ReadDTableX2_Workspace* const wksp = (HUF_ReadDTableX2_Workspace*)workSpace;

    if (sizeof(*wksp) > wkspSize) return ERROR(GENERIC);

    rankStart = wksp->rankStart0 + 1;
    ZSTD_memset(wksp->rankStats, 0, sizeof(wksp->rankStats));
    ZSTD_memset(wksp->rankStart0, 0, sizeof(wksp->rankStart0));

    DEBUG_STATIC_ASSERT(sizeof(HUF_DEltX2) == sizeof(HUF_DTable));
    if (maxTableLog > HUF_TABLELOG_MAX) return ERROR(tableLog_tooLarge);


    iSize = HUF_readStats_wksp(wksp->weightList, HUF_SYMBOLVALUE_MAX + 1, wksp->rankStats, &nbSymbols, &tableLog, src, srcSize, wksp->calleeWksp, sizeof(wksp->calleeWksp), flags);
    if (HUF_isError(iSize)) return iSize;


    if (tableLog > maxTableLog) return ERROR(tableLog_tooLarge);
    if (tableLog <= HUF_DECODER_FAST_TABLELOG && maxTableLog > HUF_DECODER_FAST_TABLELOG) maxTableLog = HUF_DECODER_FAST_TABLELOG;


    for (maxW = tableLog; wksp->rankStats[maxW]==0; maxW--) {}


    {   U32 w, nextRankStart = 0;
        for (w=1; w<maxW+1; w++) {
            U32 curr = nextRankStart;
            nextRankStart += wksp->rankStats[w];
            rankStart[w] = curr;
        }
        rankStart[0] = nextRankStart;
        rankStart[maxW+1] = nextRankStart;
    }


    {   U32 s;
        for (s=0; s<nbSymbols; s++) {
            U32 const w = wksp->weightList[s];
            U32 const r = rankStart[w]++;
            wksp->sortedSymbol[r].symbol = (BYTE)s;
        }
        rankStart[0] = 0;
    }


    {   U32* const rankVal0 = wksp->rankVal[0];
        {   int const rescale = (maxTableLog-tableLog) - 1;
            U32 nextRankVal = 0;
            U32 w;
            for (w=1; w<maxW+1; w++) {
                U32 curr = nextRankVal;
                nextRankVal += wksp->rankStats[w] << (w+rescale);
                rankVal0[w] = curr;
        }   }
        {   U32 const minBits = tableLog+1 - maxW;
            U32 consumed;
            for (consumed = minBits; consumed < maxTableLog - minBits + 1; consumed++) {
                U32* const rankValPtr = wksp->rankVal[consumed];
                U32 w;
                for (w = 1; w < maxW+1; w++) {
                    rankValPtr[w] = rankVal0[w] >> consumed;
    }   }   }   }

    HUF_fillDTableX2(dt, maxTableLog,
                   wksp->sortedSymbol,
                   wksp->rankStart0, wksp->rankVal, maxW,
                   tableLog+1);

    dtd.tableLog = (BYTE)maxTableLog;
    dtd.tableType = 1;
    ZSTD_memcpy(DTable, &dtd, sizeof(dtd));
    return iSize;
}


FORCE_INLINE_TEMPLATE U32
HUF_decodeSymbolX2(void* op, BIT_DStream_t* DStream, const HUF_DEltX2* dt, const U32 dtLog)
{
    size_t const val = BIT_lookBitsFast(DStream, dtLog);
    ZSTD_memcpy(op, &dt[val].sequence, 2);
    BIT_skipBits(DStream, dt[val].nbBits);
    return dt[val].length;
}

FORCE_INLINE_TEMPLATE U32
HUF_decodeLastSymbolX2(void* op, BIT_DStream_t* DStream, const HUF_DEltX2* dt, const U32 dtLog)
{
    size_t const val = BIT_lookBitsFast(DStream, dtLog);
    ZSTD_memcpy(op, &dt[val].sequence, 1);
    if (dt[val].length==1) {
        BIT_skipBits(DStream, dt[val].nbBits);
    } else {
        if (DStream->bitsConsumed < (sizeof(DStream->bitContainer)*8)) {
            BIT_skipBits(DStream, dt[val].nbBits);
            if (DStream->bitsConsumed > (sizeof(DStream->bitContainer)*8))

                DStream->bitsConsumed = (sizeof(DStream->bitContainer)*8);
        }
    }
    return 1;
}

#define HUF_DECODE_SYMBOLX2_0(ptr, DStreamPtr) \
    do { ptr += HUF_decodeSymbolX2(ptr, DStreamPtr, dt, dtLog); } while (0)

#define HUF_DECODE_SYMBOLX2_1(ptr, DStreamPtr)                     \
    do {                                                           \
        if (MEM_64bits() || (HUF_TABLELOG_MAX<=12))                \
            ptr += HUF_decodeSymbolX2(ptr, DStreamPtr, dt, dtLog); \
    } while (0)

#define HUF_DECODE_SYMBOLX2_2(ptr, DStreamPtr)                     \
    do {                                                           \
        if (MEM_64bits())                                          \
            ptr += HUF_decodeSymbolX2(ptr, DStreamPtr, dt, dtLog); \
    } while (0)

HINT_INLINE size_t
HUF_decodeStreamX2(BYTE* p, BIT_DStream_t* bitDPtr, BYTE* const pEnd,
                const HUF_DEltX2* const dt, const U32 dtLog)
{
    BYTE* const pStart = p;


    if ((size_t)(pEnd - p) >= sizeof(bitDPtr->bitContainer)) {
        if (dtLog <= 11 && MEM_64bits()) {

            while ((BIT_reloadDStream(bitDPtr) == BIT_DStream_unfinished) & (p < pEnd-9)) {
                HUF_DECODE_SYMBOLX2_0(p, bitDPtr);
                HUF_DECODE_SYMBOLX2_0(p, bitDPtr);
                HUF_DECODE_SYMBOLX2_0(p, bitDPtr);
                HUF_DECODE_SYMBOLX2_0(p, bitDPtr);
                HUF_DECODE_SYMBOLX2_0(p, bitDPtr);
            }
        } else {

            while ((BIT_reloadDStream(bitDPtr) == BIT_DStream_unfinished) & (p < pEnd-(sizeof(bitDPtr->bitContainer)-1))) {
                HUF_DECODE_SYMBOLX2_2(p, bitDPtr);
                HUF_DECODE_SYMBOLX2_1(p, bitDPtr);
                HUF_DECODE_SYMBOLX2_2(p, bitDPtr);
                HUF_DECODE_SYMBOLX2_0(p, bitDPtr);
            }
        }
    } else {
        BIT_reloadDStream(bitDPtr);
    }


    if ((size_t)(pEnd - p) >= 2) {
        while ((BIT_reloadDStream(bitDPtr) == BIT_DStream_unfinished) & (p <= pEnd-2))
            HUF_DECODE_SYMBOLX2_0(p, bitDPtr);

        while (p <= pEnd-2)
            HUF_DECODE_SYMBOLX2_0(p, bitDPtr);
    }

    if (p < pEnd)
        p += HUF_decodeLastSymbolX2(p, bitDPtr, dt, dtLog);

    return p-pStart;
}

FORCE_INLINE_TEMPLATE size_t
HUF_decompress1X2_usingDTable_internal_body(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const HUF_DTable* DTable)
{
    BIT_DStream_t bitD;


    CHECK_F( BIT_initDStream(&bitD, cSrc, cSrcSize) );


    {   BYTE* const ostart = (BYTE*) dst;
        BYTE* const oend = (BYTE*)ZSTD_maybeNullPtrAdd(ostart, (ptrdiff_t)dstSize);
        const void* const dtPtr = DTable+1;
        const HUF_DEltX2* const dt = (const HUF_DEltX2*)dtPtr;
        DTableDesc const dtd = HUF_getDTableDesc(DTable);
        HUF_decodeStreamX2(ostart, &bitD, oend, dt, dtd.tableLog);
    }


    if (!BIT_endOfDStream(&bitD)) return ERROR(corruption_detected);


    return dstSize;
}


FORCE_INLINE_TEMPLATE size_t
HUF_decompress4X2_usingDTable_internal_body(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const HUF_DTable* DTable)
{
    if (cSrcSize < 10) return ERROR(corruption_detected);
    if (dstSize < 6) return ERROR(corruption_detected);

    {   const BYTE* const istart = (const BYTE*) cSrc;
        BYTE* const ostart = (BYTE*) dst;
        BYTE* const oend = ostart + dstSize;
        BYTE* const olimit = oend - (sizeof(size_t)-1);
        const void* const dtPtr = DTable+1;
        const HUF_DEltX2* const dt = (const HUF_DEltX2*)dtPtr;


        BIT_DStream_t bitD1;
        BIT_DStream_t bitD2;
        BIT_DStream_t bitD3;
        BIT_DStream_t bitD4;
        size_t const length1 = MEM_readLE16(istart);
        size_t const length2 = MEM_readLE16(istart+2);
        size_t const length3 = MEM_readLE16(istart+4);
        size_t const length4 = cSrcSize - (length1 + length2 + length3 + 6);
        const BYTE* const istart1 = istart + 6;
        const BYTE* const istart2 = istart1 + length1;
        const BYTE* const istart3 = istart2 + length2;
        const BYTE* const istart4 = istart3 + length3;
        size_t const segmentSize = (dstSize+3) / 4;
        BYTE* const opStart2 = ostart + segmentSize;
        BYTE* const opStart3 = opStart2 + segmentSize;
        BYTE* const opStart4 = opStart3 + segmentSize;
        BYTE* op1 = ostart;
        BYTE* op2 = opStart2;
        BYTE* op3 = opStart3;
        BYTE* op4 = opStart4;
        U32 endSignal = 1;
        DTableDesc const dtd = HUF_getDTableDesc(DTable);
        U32 const dtLog = dtd.tableLog;

        if (length4 > cSrcSize) return ERROR(corruption_detected);
        if (opStart4 > oend) return ERROR(corruption_detected);
        assert(dstSize >= 6 );
        CHECK_F( BIT_initDStream(&bitD1, istart1, length1) );
        CHECK_F( BIT_initDStream(&bitD2, istart2, length2) );
        CHECK_F( BIT_initDStream(&bitD3, istart3, length3) );
        CHECK_F( BIT_initDStream(&bitD4, istart4, length4) );


        if ((size_t)(oend - op4) >= sizeof(size_t)) {
            for ( ; (endSignal) & (op4 < olimit); ) {
#if defined(__clang__) && (defined(__x86_64__) || defined(__i386__))
                HUF_DECODE_SYMBOLX2_2(op1, &bitD1);
                HUF_DECODE_SYMBOLX2_1(op1, &bitD1);
                HUF_DECODE_SYMBOLX2_2(op1, &bitD1);
                HUF_DECODE_SYMBOLX2_0(op1, &bitD1);
                HUF_DECODE_SYMBOLX2_2(op2, &bitD2);
                HUF_DECODE_SYMBOLX2_1(op2, &bitD2);
                HUF_DECODE_SYMBOLX2_2(op2, &bitD2);
                HUF_DECODE_SYMBOLX2_0(op2, &bitD2);
                endSignal &= BIT_reloadDStreamFast(&bitD1) == BIT_DStream_unfinished;
                endSignal &= BIT_reloadDStreamFast(&bitD2) == BIT_DStream_unfinished;
                HUF_DECODE_SYMBOLX2_2(op3, &bitD3);
                HUF_DECODE_SYMBOLX2_1(op3, &bitD3);
                HUF_DECODE_SYMBOLX2_2(op3, &bitD3);
                HUF_DECODE_SYMBOLX2_0(op3, &bitD3);
                HUF_DECODE_SYMBOLX2_2(op4, &bitD4);
                HUF_DECODE_SYMBOLX2_1(op4, &bitD4);
                HUF_DECODE_SYMBOLX2_2(op4, &bitD4);
                HUF_DECODE_SYMBOLX2_0(op4, &bitD4);
                endSignal &= BIT_reloadDStreamFast(&bitD3) == BIT_DStream_unfinished;
                endSignal &= BIT_reloadDStreamFast(&bitD4) == BIT_DStream_unfinished;
#else
                HUF_DECODE_SYMBOLX2_2(op1, &bitD1);
                HUF_DECODE_SYMBOLX2_2(op2, &bitD2);
                HUF_DECODE_SYMBOLX2_2(op3, &bitD3);
                HUF_DECODE_SYMBOLX2_2(op4, &bitD4);
                HUF_DECODE_SYMBOLX2_1(op1, &bitD1);
                HUF_DECODE_SYMBOLX2_1(op2, &bitD2);
                HUF_DECODE_SYMBOLX2_1(op3, &bitD3);
                HUF_DECODE_SYMBOLX2_1(op4, &bitD4);
                HUF_DECODE_SYMBOLX2_2(op1, &bitD1);
                HUF_DECODE_SYMBOLX2_2(op2, &bitD2);
                HUF_DECODE_SYMBOLX2_2(op3, &bitD3);
                HUF_DECODE_SYMBOLX2_2(op4, &bitD4);
                HUF_DECODE_SYMBOLX2_0(op1, &bitD1);
                HUF_DECODE_SYMBOLX2_0(op2, &bitD2);
                HUF_DECODE_SYMBOLX2_0(op3, &bitD3);
                HUF_DECODE_SYMBOLX2_0(op4, &bitD4);
                endSignal = (U32)LIKELY((U32)
                            (BIT_reloadDStreamFast(&bitD1) == BIT_DStream_unfinished)
                        & (BIT_reloadDStreamFast(&bitD2) == BIT_DStream_unfinished)
                        & (BIT_reloadDStreamFast(&bitD3) == BIT_DStream_unfinished)
                        & (BIT_reloadDStreamFast(&bitD4) == BIT_DStream_unfinished));
#endif
            }
        }


        if (op1 > opStart2) return ERROR(corruption_detected);
        if (op2 > opStart3) return ERROR(corruption_detected);
        if (op3 > opStart4) return ERROR(corruption_detected);


        HUF_decodeStreamX2(op1, &bitD1, opStart2, dt, dtLog);
        HUF_decodeStreamX2(op2, &bitD2, opStart3, dt, dtLog);
        HUF_decodeStreamX2(op3, &bitD3, opStart4, dt, dtLog);
        HUF_decodeStreamX2(op4, &bitD4, oend,     dt, dtLog);


        { U32 const endCheck = BIT_endOfDStream(&bitD1) & BIT_endOfDStream(&bitD2) & BIT_endOfDStream(&bitD3) & BIT_endOfDStream(&bitD4);
          if (!endCheck) return ERROR(corruption_detected); }


        return dstSize;
    }
}

#if HUF_NEED_BMI2_FUNCTION
static BMI2_TARGET_ATTRIBUTE
size_t HUF_decompress4X2_usingDTable_internal_bmi2(void* dst, size_t dstSize, void const* cSrc,
                    size_t cSrcSize, HUF_DTable const* DTable) {
    return HUF_decompress4X2_usingDTable_internal_body(dst, dstSize, cSrc, cSrcSize, DTable);
}
#endif

static
size_t HUF_decompress4X2_usingDTable_internal_default(void* dst, size_t dstSize, void const* cSrc,
                    size_t cSrcSize, HUF_DTable const* DTable) {
    return HUF_decompress4X2_usingDTable_internal_body(dst, dstSize, cSrc, cSrcSize, DTable);
}

#if ZSTD_ENABLE_ASM_X86_64_BMI2

HUF_ASM_DECL void HUF_decompress4X2_usingDTable_internal_fast_asm_loop(HUF_DecompressFastArgs* args) ZSTDLIB_HIDDEN;

#endif

static HUF_FAST_BMI2_ATTRS
void HUF_decompress4X2_usingDTable_internal_fast_c_loop(HUF_DecompressFastArgs* args)
{
    U64 bits[4];
    BYTE const* ip[4];
    BYTE* op[4];
    BYTE* oend[4];
    HUF_DEltX2 const* const dtable = (HUF_DEltX2 const*)args->dt;
    BYTE const* const ilowest = args->ilowest;


    ZSTD_memcpy(&bits, &args->bits, sizeof(bits));
    ZSTD_memcpy((void*)(&ip), &args->ip, sizeof(ip));
    ZSTD_memcpy(&op, &args->op, sizeof(op));

    oend[0] = op[1];
    oend[1] = op[2];
    oend[2] = op[3];
    oend[3] = args->oend;

    assert(MEM_isLittleEndian());
    assert(!MEM_32bits());

    for (;;) {
        BYTE* olimit;
        int stream;


#ifndef NDEBUG
        for (stream = 0; stream < 4; ++stream) {
            assert(op[stream] <= oend[stream]);
            assert(ip[stream] >= ilowest);
        }
#endif

        {


            size_t iters = (size_t)(ip[0] - ilowest) / 7;

            for (stream = 0; stream < 4; ++stream) {
                size_t const oiters = (size_t)(oend[stream] - op[stream]) / 10;
                iters = MIN(iters, oiters);
            }


            olimit = op[3] + (iters * 5);


            if (op[3] == olimit)
                break;


            for (stream = 1; stream < 4; ++stream) {
                if (ip[stream] < ip[stream - 1])
                    goto _out;
            }
        }

#ifndef NDEBUG
        for (stream = 1; stream < 4; ++stream) {
            assert(ip[stream] >= ip[stream - 1]);
        }
#endif

#define HUF_4X2_DECODE_SYMBOL(_stream, _decode3)               \
    do {                                                       \
        if ((_decode3) || (_stream) != 3) {                    \
            U64 const index = bits[(_stream)] >> 53;           \
            size_t const entry = MEM_readLE32(&dtable[index]); \
            MEM_write16(op[(_stream)], (U16)entry);            \
            bits[(_stream)] <<= (entry >> 16) & 0x3F;          \
            op[(_stream)] += entry >> 24;                      \
        }                                                      \
    } while (0)

#define HUF_5X2_RELOAD_STREAM(_stream, _decode3)                        \
    do {                                                                \
        if (_decode3) HUF_4X2_DECODE_SYMBOL(3, 1);                      \
        {                                                               \
            U64 const ctz = ZSTD_countTrailingZeros64(bits[(_stream)]); \
            U64 const nbBits = ctz & 7;                                 \
            U64 const nbBytes = ctz >> 3;                               \
            ip[(_stream)] -= nbBytes;                                   \
            bits[(_stream)] = MEM_read64(ip[(_stream)]) | 1;            \
            bits[(_stream)] <<= nbBits;                                 \
        }                                                               \
    } while (0)

#if defined(__aarch64__)
#  define HUF_4X2_4WAY 1
#else
#  define HUF_4X2_4WAY 0
#endif
#define HUF_4X2_3WAY !HUF_4X2_4WAY


        do {

            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X2_DECODE_SYMBOL, HUF_4X2_4WAY);
            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X2_DECODE_SYMBOL, HUF_4X2_4WAY);
            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X2_DECODE_SYMBOL, HUF_4X2_4WAY);
            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X2_DECODE_SYMBOL, HUF_4X2_4WAY);
            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_4X2_DECODE_SYMBOL, HUF_4X2_4WAY);


            HUF_4X2_DECODE_SYMBOL(3, HUF_4X2_3WAY);


            HUF_4X_FOR_EACH_STREAM_WITH_VAR(HUF_5X2_RELOAD_STREAM, HUF_4X2_3WAY);
        } while (op[3] < olimit);
    }

#undef HUF_4X2_DECODE_SYMBOL
#undef HUF_5X2_RELOAD_STREAM

_out:


    ZSTD_memcpy(&args->bits, &bits, sizeof(bits));
    ZSTD_memcpy((void*)(&args->ip), &ip, sizeof(ip));
    ZSTD_memcpy(&args->op, &op, sizeof(op));
}


static HUF_FAST_BMI2_ATTRS size_t
HUF_decompress4X2_usingDTable_internal_fast(
          void* dst,  size_t dstSize,
    const void* cSrc, size_t cSrcSize,
    const HUF_DTable* DTable,
    HUF_DecompressFastLoopFn loopFn) {
    void const* dt = DTable + 1;
    const BYTE* const ilowest = (const BYTE*)cSrc;
    BYTE* const oend = (BYTE*)ZSTD_maybeNullPtrAdd(dst, (ptrdiff_t)dstSize);
    HUF_DecompressFastArgs args;
    {
        size_t const ret = HUF_DecompressFastArgs_init(&args, dst, dstSize, cSrc, cSrcSize, DTable);
        FORWARD_IF_ERROR(ret, "Failed to init asm args");
        if (ret == 0)
            return 0;
    }

    assert(args.ip[0] >= args.ilowest);
    loopFn(&args);


    assert(args.ip[0] >= ilowest);
    assert(args.ip[1] >= ilowest);
    assert(args.ip[2] >= ilowest);
    assert(args.ip[3] >= ilowest);
    assert(args.op[3] <= oend);

    assert(ilowest == args.ilowest);
    assert(ilowest + 6 == args.iend[0]);
    (void)ilowest;


    {
        size_t const segmentSize = (dstSize+3) / 4;
        BYTE* segmentEnd = (BYTE*)dst;
        int i;
        for (i = 0; i < 4; ++i) {
            BIT_DStream_t bit;
            if (segmentSize <= (size_t)(oend - segmentEnd))
                segmentEnd += segmentSize;
            else
                segmentEnd = oend;
            FORWARD_IF_ERROR(HUF_initRemainingDStream(&bit, &args, i, segmentEnd), "corruption");
            args.op[i] += HUF_decodeStreamX2(args.op[i], &bit, segmentEnd, (HUF_DEltX2 const*)dt, HUF_DECODER_FAST_TABLELOG);
            if (args.op[i] != segmentEnd)
                return ERROR(corruption_detected);
        }
    }


    return dstSize;
}

static size_t HUF_decompress4X2_usingDTable_internal(void* dst, size_t dstSize, void const* cSrc,
                    size_t cSrcSize, HUF_DTable const* DTable, int flags)
{
    HUF_DecompressUsingDTableFn fallbackFn = HUF_decompress4X2_usingDTable_internal_default;
    HUF_DecompressFastLoopFn loopFn = HUF_decompress4X2_usingDTable_internal_fast_c_loop;

#if DYNAMIC_BMI2
    if (flags & HUF_flags_bmi2) {
        fallbackFn = HUF_decompress4X2_usingDTable_internal_bmi2;
# if ZSTD_ENABLE_ASM_X86_64_BMI2
        if (!(flags & HUF_flags_disableAsm)) {
            loopFn = HUF_decompress4X2_usingDTable_internal_fast_asm_loop;
        }
# endif
    } else {
        return fallbackFn(dst, dstSize, cSrc, cSrcSize, DTable);
    }
#endif

#if ZSTD_ENABLE_ASM_X86_64_BMI2 && defined(__BMI2__)
    if (!(flags & HUF_flags_disableAsm)) {
        loopFn = HUF_decompress4X2_usingDTable_internal_fast_asm_loop;
    }
#endif

    if (HUF_ENABLE_FAST_DECODE && !(flags & HUF_flags_disableFast)) {
        size_t const ret = HUF_decompress4X2_usingDTable_internal_fast(dst, dstSize, cSrc, cSrcSize, DTable, loopFn);
        if (ret != 0)
            return ret;
    }
    return fallbackFn(dst, dstSize, cSrc, cSrcSize, DTable);
}

HUF_DGEN(HUF_decompress1X2_usingDTable_internal)

size_t HUF_decompress1X2_DCtx_wksp(HUF_DTable* DCtx, void* dst, size_t dstSize,
                                   const void* cSrc, size_t cSrcSize,
                                   void* workSpace, size_t wkspSize, int flags)
{
    const BYTE* ip = (const BYTE*) cSrc;

    size_t const hSize = HUF_readDTableX2_wksp(DCtx, cSrc, cSrcSize,
                                               workSpace, wkspSize, flags);
    if (HUF_isError(hSize)) return hSize;
    if (hSize >= cSrcSize) return ERROR(srcSize_wrong);
    ip += hSize; cSrcSize -= hSize;

    return HUF_decompress1X2_usingDTable_internal(dst, dstSize, ip, cSrcSize, DCtx, flags);
}

static size_t HUF_decompress4X2_DCtx_wksp(HUF_DTable* dctx, void* dst, size_t dstSize,
                                   const void* cSrc, size_t cSrcSize,
                                   void* workSpace, size_t wkspSize, int flags)
{
    const BYTE* ip = (const BYTE*) cSrc;

    size_t hSize = HUF_readDTableX2_wksp(dctx, cSrc, cSrcSize,
                                         workSpace, wkspSize, flags);
    if (HUF_isError(hSize)) return hSize;
    if (hSize >= cSrcSize) return ERROR(srcSize_wrong);
    ip += hSize; cSrcSize -= hSize;

    return HUF_decompress4X2_usingDTable_internal(dst, dstSize, ip, cSrcSize, dctx, flags);
}

#endif


#if !defined(HUF_FORCE_DECOMPRESS_X1) && !defined(HUF_FORCE_DECOMPRESS_X2)
typedef struct { U32 tableTime; U32 decode256Time; } algo_time_t;
static const algo_time_t algoTime[16 ][2 ] =
{

    {{0,0}, {1,1}},
    {{0,0}, {1,1}},
    {{ 150,216}, { 381,119}},
    {{ 170,205}, { 514,112}},
    {{ 177,199}, { 539,110}},
    {{ 197,194}, { 644,107}},
    {{ 221,192}, { 735,107}},
    {{ 256,189}, { 881,106}},
    {{ 359,188}, {1167,109}},
    {{ 582,187}, {1570,114}},
    {{ 688,187}, {1712,122}},
    {{ 825,186}, {1965,136}},
    {{ 976,185}, {2131,150}},
    {{1180,186}, {2070,175}},
    {{1377,185}, {1731,202}},
    {{1412,185}, {1695,202}},
};
#endif


U32 HUF_selectDecoder (size_t dstSize, size_t cSrcSize)
{
    assert(dstSize > 0);
    assert(dstSize <= 128*1024);
#if defined(HUF_FORCE_DECOMPRESS_X1)
    (void)dstSize;
    (void)cSrcSize;
    return 0;
#elif defined(HUF_FORCE_DECOMPRESS_X2)
    (void)dstSize;
    (void)cSrcSize;
    return 1;
#else

    {   U32 const Q = (cSrcSize >= dstSize) ? 15 : (U32)(cSrcSize * 16 / dstSize);
        U32 const D256 = (U32)(dstSize >> 8);
        U32 const DTime0 = algoTime[Q][0].tableTime + (algoTime[Q][0].decode256Time * D256);
        U32 DTime1 = algoTime[Q][1].tableTime + (algoTime[Q][1].decode256Time * D256);
        DTime1 += DTime1 >> 5;
        return DTime1 < DTime0;
    }
#endif
}

size_t HUF_decompress1X_DCtx_wksp(HUF_DTable* dctx, void* dst, size_t dstSize,
                                  const void* cSrc, size_t cSrcSize,
                                  void* workSpace, size_t wkspSize, int flags)
{

    if (dstSize == 0) return ERROR(dstSize_tooSmall);
    if (cSrcSize > dstSize) return ERROR(corruption_detected);
    if (cSrcSize == dstSize) { ZSTD_memcpy(dst, cSrc, dstSize); return dstSize; }
    if (cSrcSize == 1) { ZSTD_memset(dst, *(const BYTE*)cSrc, dstSize); return dstSize; }

    {   U32 const algoNb = HUF_selectDecoder(dstSize, cSrcSize);
#if defined(HUF_FORCE_DECOMPRESS_X1)
        (void)algoNb;
        assert(algoNb == 0);
        return HUF_decompress1X1_DCtx_wksp(dctx, dst, dstSize, cSrc,
                                cSrcSize, workSpace, wkspSize, flags);
#elif defined(HUF_FORCE_DECOMPRESS_X2)
        (void)algoNb;
        assert(algoNb == 1);
        return HUF_decompress1X2_DCtx_wksp(dctx, dst, dstSize, cSrc,
                                cSrcSize, workSpace, wkspSize, flags);
#else
        return algoNb ? HUF_decompress1X2_DCtx_wksp(dctx, dst, dstSize, cSrc,
                                cSrcSize, workSpace, wkspSize, flags):
                        HUF_decompress1X1_DCtx_wksp(dctx, dst, dstSize, cSrc,
                                cSrcSize, workSpace, wkspSize, flags);
#endif
    }
}


size_t HUF_decompress1X_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_DTable* DTable, int flags)
{
    DTableDesc const dtd = HUF_getDTableDesc(DTable);
#if defined(HUF_FORCE_DECOMPRESS_X1)
    (void)dtd;
    assert(dtd.tableType == 0);
    return HUF_decompress1X1_usingDTable_internal(dst, maxDstSize, cSrc, cSrcSize, DTable, flags);
#elif defined(HUF_FORCE_DECOMPRESS_X2)
    (void)dtd;
    assert(dtd.tableType == 1);
    return HUF_decompress1X2_usingDTable_internal(dst, maxDstSize, cSrc, cSrcSize, DTable, flags);
#else
    return dtd.tableType ? HUF_decompress1X2_usingDTable_internal(dst, maxDstSize, cSrc, cSrcSize, DTable, flags) :
                           HUF_decompress1X1_usingDTable_internal(dst, maxDstSize, cSrc, cSrcSize, DTable, flags);
#endif
}

#ifndef HUF_FORCE_DECOMPRESS_X2
size_t HUF_decompress1X1_DCtx_wksp(HUF_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize, void* workSpace, size_t wkspSize, int flags)
{
    const BYTE* ip = (const BYTE*) cSrc;

    size_t const hSize = HUF_readDTableX1_wksp(dctx, cSrc, cSrcSize, workSpace, wkspSize, flags);
    if (HUF_isError(hSize)) return hSize;
    if (hSize >= cSrcSize) return ERROR(srcSize_wrong);
    ip += hSize; cSrcSize -= hSize;

    return HUF_decompress1X1_usingDTable_internal(dst, dstSize, ip, cSrcSize, dctx, flags);
}
#endif

size_t HUF_decompress4X_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const HUF_DTable* DTable, int flags)
{
    DTableDesc const dtd = HUF_getDTableDesc(DTable);
#if defined(HUF_FORCE_DECOMPRESS_X1)
    (void)dtd;
    assert(dtd.tableType == 0);
    return HUF_decompress4X1_usingDTable_internal(dst, maxDstSize, cSrc, cSrcSize, DTable, flags);
#elif defined(HUF_FORCE_DECOMPRESS_X2)
    (void)dtd;
    assert(dtd.tableType == 1);
    return HUF_decompress4X2_usingDTable_internal(dst, maxDstSize, cSrc, cSrcSize, DTable, flags);
#else
    return dtd.tableType ? HUF_decompress4X2_usingDTable_internal(dst, maxDstSize, cSrc, cSrcSize, DTable, flags) :
                           HUF_decompress4X1_usingDTable_internal(dst, maxDstSize, cSrc, cSrcSize, DTable, flags);
#endif
}

size_t HUF_decompress4X_hufOnly_wksp(HUF_DTable* dctx, void* dst, size_t dstSize, const void* cSrc, size_t cSrcSize, void* workSpace, size_t wkspSize, int flags)
{

    if (dstSize == 0) return ERROR(dstSize_tooSmall);
    if (cSrcSize == 0) return ERROR(corruption_detected);

    {   U32 const algoNb = HUF_selectDecoder(dstSize, cSrcSize);
#if defined(HUF_FORCE_DECOMPRESS_X1)
        (void)algoNb;
        assert(algoNb == 0);
        return HUF_decompress4X1_DCtx_wksp(dctx, dst, dstSize, cSrc, cSrcSize, workSpace, wkspSize, flags);
#elif defined(HUF_FORCE_DECOMPRESS_X2)
        (void)algoNb;
        assert(algoNb == 1);
        return HUF_decompress4X2_DCtx_wksp(dctx, dst, dstSize, cSrc, cSrcSize, workSpace, wkspSize, flags);
#else
        return algoNb ? HUF_decompress4X2_DCtx_wksp(dctx, dst, dstSize, cSrc, cSrcSize, workSpace, wkspSize, flags) :
                        HUF_decompress4X1_DCtx_wksp(dctx, dst, dstSize, cSrc, cSrcSize, workSpace, wkspSize, flags);
#endif
    }
}


#define ZSTD_DEPS_NEED_MALLOC


#define ZSTD_STATIC_LINKING_ONLY


#ifndef ZSTD_ALLOCATIONS_H
#define ZSTD_ALLOCATIONS_H


MEM_STATIC void* ZSTD_customMalloc(size_t size, ZSTD_customMem customMem)
{
    if (customMem.customAlloc)
        return customMem.customAlloc(customMem.opaque, size);
    return ZSTD_malloc(size);
}

MEM_STATIC void* ZSTD_customCalloc(size_t size, ZSTD_customMem customMem)
{
    if (customMem.customAlloc) {

        void* const ptr = customMem.customAlloc(customMem.opaque, size);

        if (ptr == NULL) {
            return NULL;
        }

        ZSTD_memset(ptr, 0, size);
        return ptr;
    }
    return ZSTD_calloc(1, size);
}

MEM_STATIC void ZSTD_customFree(void* ptr, ZSTD_customMem customMem)
{
    if (ptr!=NULL) {
        if (customMem.customFree)
            customMem.customFree(customMem.opaque, ptr);
        else
            ZSTD_free(ptr);
    }
}

#endif


#define FSE_STATIC_LINKING_ONLY


 #ifndef ZSTD_DECOMPRESS_INTERNAL_H
 #define ZSTD_DECOMPRESS_INTERNAL_H


static UNUSED_ATTR const U32 LL_base[MaxLL+1] = {
                 0,    1,    2,     3,     4,     5,     6,      7,
                 8,    9,   10,    11,    12,    13,    14,     15,
                16,   18,   20,    22,    24,    28,    32,     40,
                48,   64, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000,
                0x2000, 0x4000, 0x8000, 0x10000 };

static UNUSED_ATTR const U32 OF_base[MaxOff+1] = {
                 0,        1,       1,       5,     0xD,     0x1D,     0x3D,     0x7D,
                 0xFD,   0x1FD,   0x3FD,   0x7FD,   0xFFD,   0x1FFD,   0x3FFD,   0x7FFD,
                 0xFFFD, 0x1FFFD, 0x3FFFD, 0x7FFFD, 0xFFFFD, 0x1FFFFD, 0x3FFFFD, 0x7FFFFD,
                 0xFFFFFD, 0x1FFFFFD, 0x3FFFFFD, 0x7FFFFFD, 0xFFFFFFD, 0x1FFFFFFD, 0x3FFFFFFD, 0x7FFFFFFD };

static UNUSED_ATTR const U8 OF_bits[MaxOff+1] = {
                     0,  1,  2,  3,  4,  5,  6,  7,
                     8,  9, 10, 11, 12, 13, 14, 15,
                    16, 17, 18, 19, 20, 21, 22, 23,
                    24, 25, 26, 27, 28, 29, 30, 31 };

static UNUSED_ATTR const U32 ML_base[MaxML+1] = {
                     3,  4,  5,    6,     7,     8,     9,    10,
                    11, 12, 13,   14,    15,    16,    17,    18,
                    19, 20, 21,   22,    23,    24,    25,    26,
                    27, 28, 29,   30,    31,    32,    33,    34,
                    35, 37, 39,   41,    43,    47,    51,    59,
                    67, 83, 99, 0x83, 0x103, 0x203, 0x403, 0x803,
                    0x1003, 0x2003, 0x4003, 0x8003, 0x10003 };


 typedef struct {
     U32 fastMode;
     U32 tableLog;
 } ZSTD_seqSymbol_header;

 typedef struct {
     U16  nextState;
     BYTE nbAdditionalBits;
     BYTE nbBits;
     U32  baseValue;
 } ZSTD_seqSymbol;

 #define SEQSYMBOL_TABLE_SIZE(log)   (1 + (1 << (log)))

#define ZSTD_BUILD_FSE_TABLE_WKSP_SIZE (sizeof(S16) * (MaxSeq + 1) + (1u << MaxFSELog) + sizeof(U64))
#define ZSTD_BUILD_FSE_TABLE_WKSP_SIZE_U32 ((ZSTD_BUILD_FSE_TABLE_WKSP_SIZE + sizeof(U32) - 1) / sizeof(U32))
#define ZSTD_HUFFDTABLE_CAPACITY_LOG 12

typedef struct {
    ZSTD_seqSymbol LLTable[SEQSYMBOL_TABLE_SIZE(LLFSELog)];
    ZSTD_seqSymbol OFTable[SEQSYMBOL_TABLE_SIZE(OffFSELog)];
    ZSTD_seqSymbol MLTable[SEQSYMBOL_TABLE_SIZE(MLFSELog)];
    HUF_DTable hufTable[HUF_DTABLE_SIZE(ZSTD_HUFFDTABLE_CAPACITY_LOG)];
    U32 rep[ZSTD_REP_NUM];
    U32 workspace[ZSTD_BUILD_FSE_TABLE_WKSP_SIZE_U32];
} ZSTD_entropyDTables_t;

typedef enum { ZSTDds_getFrameHeaderSize, ZSTDds_decodeFrameHeader,
               ZSTDds_decodeBlockHeader, ZSTDds_decompressBlock,
               ZSTDds_decompressLastBlock, ZSTDds_checkChecksum,
               ZSTDds_decodeSkippableHeader, ZSTDds_skipFrame } ZSTD_dStage;

typedef enum { zdss_init=0, zdss_loadHeader,
               zdss_read, zdss_load, zdss_flush } ZSTD_dStreamStage;

typedef enum {
    ZSTD_use_indefinitely = -1,
    ZSTD_dont_use = 0,
    ZSTD_use_once = 1
} ZSTD_dictUses_e;


typedef struct {
    const ZSTD_DDict** ddictPtrTable;
    size_t ddictPtrTableSize;
    size_t ddictPtrCount;
} ZSTD_DDictHashSet;

#ifndef ZSTD_DECODER_INTERNAL_BUFFER
#  define ZSTD_DECODER_INTERNAL_BUFFER  (1 << 16)
#endif

#define ZSTD_LBMIN 64
#define ZSTD_LBMAX (128 << 10)


#define ZSTD_LITBUFFEREXTRASIZE  BOUNDED(ZSTD_LBMIN, ZSTD_DECODER_INTERNAL_BUFFER, ZSTD_LBMAX)

typedef enum {
    ZSTD_not_in_dst = 0,
    ZSTD_in_dst = 1,
    ZSTD_split = 2
} ZSTD_litLocation_e;

struct ZSTD_DCtx_s
{
    const ZSTD_seqSymbol* LLTptr;
    const ZSTD_seqSymbol* MLTptr;
    const ZSTD_seqSymbol* OFTptr;
    const HUF_DTable* HUFptr;
    ZSTD_entropyDTables_t entropy;
    U32 workspace[HUF_DECOMPRESS_WORKSPACE_SIZE_U32];
    const void* previousDstEnd;
    const void* prefixStart;
    const void* virtualStart;
    const void* dictEnd;
    size_t expected;
    ZSTD_FrameHeader fParams;
    U64 processedCSize;
    U64 decodedSize;
    blockType_e bType;
    ZSTD_dStage stage;
    U32 litEntropy;
    U32 fseEntropy;
    XXH64_state_t xxhState;
    size_t headerSize;
    ZSTD_format_e format;
    ZSTD_forceIgnoreChecksum_e forceIgnoreChecksum;
    U32 validateChecksum;
    const BYTE* litPtr;
    ZSTD_customMem customMem;
    size_t litSize;
    size_t rleSize;
    size_t staticSize;
    int isFrameDecompression;
#if DYNAMIC_BMI2
    int bmi2;
#endif


    ZSTD_DDict* ddictLocal;
    const ZSTD_DDict* ddict;
    U32 dictID;
    int ddictIsCold;
    ZSTD_dictUses_e dictUses;
    ZSTD_DDictHashSet* ddictSet;
    ZSTD_refMultipleDDicts_e refMultipleDDicts;
    int disableHufAsm;
    int maxBlockSizeParam;


    ZSTD_dStreamStage streamStage;
    char*  inBuff;
    size_t inBuffSize;
    size_t inPos;
    size_t maxWindowSize;
    char*  outBuff;
    size_t outBuffSize;
    size_t outStart;
    size_t outEnd;
    size_t lhSize;
#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT>=1)
    void* legacyContext;
    U32 previousLegacyVersion;
    U32 legacyVersion;
#endif
    U32 hostageByte;
    int noForwardProgress;
    ZSTD_bufferMode_e outBufferMode;
    ZSTD_outBuffer expectedOutBuffer;


    BYTE* litBuffer;
    const BYTE* litBufferEnd;
    ZSTD_litLocation_e litBufferLocation;
    BYTE litExtraBuffer[ZSTD_LITBUFFEREXTRASIZE + WILDCOPY_OVERLENGTH];
    BYTE headerBuffer[ZSTD_FRAMEHEADERSIZE_MAX];

    size_t oversizedDuration;

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    void const* dictContentBeginForFuzzing;
    void const* dictContentEndForFuzzing;
#endif


#if ZSTD_TRACE
    ZSTD_TraceCtx traceCtx;
#endif
};

MEM_STATIC int ZSTD_DCtx_get_bmi2(const struct ZSTD_DCtx_s *dctx) {
#if DYNAMIC_BMI2
    return dctx->bmi2;
#else
    (void)dctx;
    return 0;
#endif
}


size_t ZSTD_loadDEntropy(ZSTD_entropyDTables_t* entropy,
                   const void* const dict, size_t const dictSize);


void ZSTD_checkContinuity(ZSTD_DCtx* dctx, const void* dst, size_t dstSize);


#endif


#ifndef ZSTD_DDICT_H
#define ZSTD_DDICT_H


const void* ZSTD_DDict_dictContent(const ZSTD_DDict* ddict);
size_t ZSTD_DDict_dictSize(const ZSTD_DDict* ddict);

void ZSTD_copyDDictParameters(ZSTD_DCtx* dctx, const ZSTD_DDict* ddict);


#endif


#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT>=1)
#error Using excluded file: ../legacy/zstd_legacy.h (re-amalgamate source to fix)
#endif


struct ZSTD_DDict_s {
    void* dictBuffer;
    const void* dictContent;
    size_t dictSize;
    ZSTD_entropyDTables_t entropy;
    U32 dictID;
    U32 entropyPresent;
    ZSTD_customMem cMem;
};

const void* ZSTD_DDict_dictContent(const ZSTD_DDict* ddict)
{
    assert(ddict != NULL);
    return ddict->dictContent;
}

size_t ZSTD_DDict_dictSize(const ZSTD_DDict* ddict)
{
    assert(ddict != NULL);
    return ddict->dictSize;
}

void ZSTD_copyDDictParameters(ZSTD_DCtx* dctx, const ZSTD_DDict* ddict)
{
    DEBUGLOG(4, "ZSTD_copyDDictParameters");
    assert(dctx != NULL);
    assert(ddict != NULL);
    dctx->dictID = ddict->dictID;
    dctx->prefixStart = ddict->dictContent;
    dctx->virtualStart = ddict->dictContent;
    dctx->dictEnd = (const BYTE*)ddict->dictContent + ddict->dictSize;
    dctx->previousDstEnd = dctx->dictEnd;
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    dctx->dictContentBeginForFuzzing = dctx->prefixStart;
    dctx->dictContentEndForFuzzing = dctx->previousDstEnd;
#endif
    if (ddict->entropyPresent) {
        dctx->litEntropy = 1;
        dctx->fseEntropy = 1;
        dctx->LLTptr = ddict->entropy.LLTable;
        dctx->MLTptr = ddict->entropy.MLTable;
        dctx->OFTptr = ddict->entropy.OFTable;
        dctx->HUFptr = ddict->entropy.hufTable;
        dctx->entropy.rep[0] = ddict->entropy.rep[0];
        dctx->entropy.rep[1] = ddict->entropy.rep[1];
        dctx->entropy.rep[2] = ddict->entropy.rep[2];
    } else {
        dctx->litEntropy = 0;
        dctx->fseEntropy = 0;
    }
}


static size_t
ZSTD_loadEntropy_intoDDict(ZSTD_DDict* ddict,
                           ZSTD_dictContentType_e dictContentType)
{
    ddict->dictID = 0;
    ddict->entropyPresent = 0;
    if (dictContentType == ZSTD_dct_rawContent) return 0;

    if (ddict->dictSize < 8) {
        if (dictContentType == ZSTD_dct_fullDict)
            return ERROR(dictionary_corrupted);
        return 0;
    }
    {   U32 const magic = MEM_readLE32(ddict->dictContent);
        if (magic != ZSTD_MAGIC_DICTIONARY) {
            if (dictContentType == ZSTD_dct_fullDict)
                return ERROR(dictionary_corrupted);
            return 0;
        }
    }
    ddict->dictID = MEM_readLE32((const char*)ddict->dictContent + ZSTD_FRAMEIDSIZE);


    RETURN_ERROR_IF(ZSTD_isError(ZSTD_loadDEntropy(
            &ddict->entropy, ddict->dictContent, ddict->dictSize)),
        dictionary_corrupted, "");
    ddict->entropyPresent = 1;
    return 0;
}


static size_t ZSTD_initDDict_internal(ZSTD_DDict* ddict,
                                      const void* dict, size_t dictSize,
                                      ZSTD_dictLoadMethod_e dictLoadMethod,
                                      ZSTD_dictContentType_e dictContentType)
{
    if ((dictLoadMethod == ZSTD_dlm_byRef) || (!dict) || (!dictSize)) {
        ddict->dictBuffer = NULL;
        ddict->dictContent = dict;
        if (!dict) dictSize = 0;
    } else {
        void* const internalBuffer = ZSTD_customMalloc(dictSize, ddict->cMem);
        ddict->dictBuffer = internalBuffer;
        ddict->dictContent = internalBuffer;
        if (!internalBuffer) return ERROR(memory_allocation);
        ZSTD_memcpy(internalBuffer, dict, dictSize);
    }
    ddict->dictSize = dictSize;
    ddict->entropy.hufTable[0] = (HUF_DTable)((ZSTD_HUFFDTABLE_CAPACITY_LOG)*0x1000001);


    FORWARD_IF_ERROR( ZSTD_loadEntropy_intoDDict(ddict, dictContentType) , "");

    return 0;
}

ZSTD_DDict* ZSTD_createDDict_advanced(const void* dict, size_t dictSize,
                                      ZSTD_dictLoadMethod_e dictLoadMethod,
                                      ZSTD_dictContentType_e dictContentType,
                                      ZSTD_customMem customMem)
{
    if ((!customMem.customAlloc) ^ (!customMem.customFree)) return NULL;

    {   ZSTD_DDict* const ddict = (ZSTD_DDict*) ZSTD_customMalloc(sizeof(ZSTD_DDict), customMem);
        if (ddict == NULL) return NULL;
        ddict->cMem = customMem;
        {   size_t const initResult = ZSTD_initDDict_internal(ddict,
                                            dict, dictSize,
                                            dictLoadMethod, dictContentType);
            if (ZSTD_isError(initResult)) {
                ZSTD_freeDDict(ddict);
                return NULL;
        }   }
        return ddict;
    }
}


ZSTD_DDict* ZSTD_createDDict(const void* dict, size_t dictSize)
{
    ZSTD_customMem const allocator = { NULL, NULL, NULL };
    return ZSTD_createDDict_advanced(dict, dictSize, ZSTD_dlm_byCopy, ZSTD_dct_auto, allocator);
}


ZSTD_DDict* ZSTD_createDDict_byReference(const void* dictBuffer, size_t dictSize)
{
    ZSTD_customMem const allocator = { NULL, NULL, NULL };
    return ZSTD_createDDict_advanced(dictBuffer, dictSize, ZSTD_dlm_byRef, ZSTD_dct_auto, allocator);
}


const ZSTD_DDict* ZSTD_initStaticDDict(
                                void* sBuffer, size_t sBufferSize,
                                const void* dict, size_t dictSize,
                                ZSTD_dictLoadMethod_e dictLoadMethod,
                                ZSTD_dictContentType_e dictContentType)
{
    size_t const neededSpace = sizeof(ZSTD_DDict)
                             + (dictLoadMethod == ZSTD_dlm_byRef ? 0 : dictSize);
    ZSTD_DDict* const ddict = (ZSTD_DDict*)sBuffer;
    assert(sBuffer != NULL);
    assert(dict != NULL);
    if ((size_t)sBuffer & 7) return NULL;
    if (sBufferSize < neededSpace) return NULL;
    if (dictLoadMethod == ZSTD_dlm_byCopy) {
        ZSTD_memcpy(ddict+1, dict, dictSize);
        dict = ddict+1;
    }
    if (ZSTD_isError( ZSTD_initDDict_internal(ddict,
                                              dict, dictSize,
                                              ZSTD_dlm_byRef, dictContentType) ))
        return NULL;
    return ddict;
}


size_t ZSTD_freeDDict(ZSTD_DDict* ddict)
{
    if (ddict==NULL) return 0;
    {   ZSTD_customMem const cMem = ddict->cMem;
        ZSTD_customFree(ddict->dictBuffer, cMem);
        ZSTD_customFree(ddict, cMem);
        return 0;
    }
}


size_t ZSTD_estimateDDictSize(size_t dictSize, ZSTD_dictLoadMethod_e dictLoadMethod)
{
    return sizeof(ZSTD_DDict) + (dictLoadMethod == ZSTD_dlm_byRef ? 0 : dictSize);
}

size_t ZSTD_sizeof_DDict(const ZSTD_DDict* ddict)
{
    if (ddict==NULL) return 0;
    return sizeof(*ddict) + (ddict->dictBuffer ? ddict->dictSize : 0) ;
}


unsigned ZSTD_getDictID_fromDDict(const ZSTD_DDict* ddict)
{
    if (ddict==NULL) return 0;
    return ddict->dictID;
}


#ifndef ZSTD_HEAPMODE
#  define ZSTD_HEAPMODE 1
#endif


#ifndef ZSTD_LEGACY_SUPPORT
#  define ZSTD_LEGACY_SUPPORT 0
#endif


#ifndef ZSTD_MAXWINDOWSIZE_DEFAULT
#  define ZSTD_MAXWINDOWSIZE_DEFAULT (((U32)1 << ZSTD_WINDOWLOG_LIMIT_DEFAULT) + 1)
#endif


#ifndef ZSTD_NO_FORWARD_PROGRESS_MAX
#  define ZSTD_NO_FORWARD_PROGRESS_MAX 16
#endif


#define FSE_STATIC_LINKING_ONLY


#ifndef ZSTD_DEC_BLOCK_H
#define ZSTD_DEC_BLOCK_H


typedef enum {
    not_streaming = 0,
    is_streaming = 1
} streaming_operation;


size_t ZSTD_decompressBlock_internal(ZSTD_DCtx* dctx,
                               void* dst, size_t dstCapacity,
                         const void* src, size_t srcSize, const streaming_operation streaming);


void ZSTD_buildFSETable(ZSTD_seqSymbol* dt,
             const short* normalizedCounter, unsigned maxSymbolValue,
             const U32* baseValue, const U8* nbAdditionalBits,
                   unsigned tableLog, void* wksp, size_t wkspSize,
                   int bmi2);


size_t ZSTD_decompressBlock_deprecated(ZSTD_DCtx* dctx,
                            void* dst, size_t dstCapacity,
                      const void* src, size_t srcSize);


#endif


#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT>=1)
#error Using excluded file: ../legacy/zstd_legacy.h (re-amalgamate source to fix)
#endif


#define DDICT_HASHSET_MAX_LOAD_FACTOR_COUNT_MULT 4
#define DDICT_HASHSET_MAX_LOAD_FACTOR_SIZE_MULT 3

#define DDICT_HASHSET_TABLE_BASE_SIZE 64
#define DDICT_HASHSET_RESIZE_FACTOR 2


static size_t ZSTD_DDictHashSet_getIndex(const ZSTD_DDictHashSet* hashSet, U32 dictID) {
    const U64 hash = XXH64(&dictID, sizeof(U32), 0);

    return hash & (hashSet->ddictPtrTableSize - 1);
}


static size_t ZSTD_DDictHashSet_emplaceDDict(ZSTD_DDictHashSet* hashSet, const ZSTD_DDict* ddict) {
    const U32 dictID = ZSTD_getDictID_fromDDict(ddict);
    size_t idx = ZSTD_DDictHashSet_getIndex(hashSet, dictID);
    const size_t idxRangeMask = hashSet->ddictPtrTableSize - 1;
    RETURN_ERROR_IF(hashSet->ddictPtrCount == hashSet->ddictPtrTableSize, GENERIC, "Hash set is full!");
    DEBUGLOG(4, "Hashed index: for dictID: %u is %zu", dictID, idx);
    while (hashSet->ddictPtrTable[idx] != NULL) {

        if (ZSTD_getDictID_fromDDict(hashSet->ddictPtrTable[idx]) == dictID) {
            DEBUGLOG(4, "DictID already exists, replacing rather than adding");
            hashSet->ddictPtrTable[idx] = ddict;
            return 0;
        }
        idx &= idxRangeMask;
        idx++;
    }
    DEBUGLOG(4, "Final idx after probing for dictID %u is: %zu", dictID, idx);
    hashSet->ddictPtrTable[idx] = ddict;
    hashSet->ddictPtrCount++;
    return 0;
}


static size_t ZSTD_DDictHashSet_expand(ZSTD_DDictHashSet* hashSet, ZSTD_customMem customMem) {
    size_t newTableSize = hashSet->ddictPtrTableSize * DDICT_HASHSET_RESIZE_FACTOR;
    const ZSTD_DDict** newTable = (const ZSTD_DDict**)ZSTD_customCalloc(sizeof(ZSTD_DDict*) * newTableSize, customMem);
    const ZSTD_DDict** oldTable = hashSet->ddictPtrTable;
    size_t oldTableSize = hashSet->ddictPtrTableSize;
    size_t i;

    DEBUGLOG(4, "Expanding DDict hash table! Old size: %zu new size: %zu", oldTableSize, newTableSize);
    RETURN_ERROR_IF(!newTable, memory_allocation, "Expanded hashset allocation failed!");
    hashSet->ddictPtrTable = newTable;
    hashSet->ddictPtrTableSize = newTableSize;
    hashSet->ddictPtrCount = 0;
    for (i = 0; i < oldTableSize; ++i) {
        if (oldTable[i] != NULL) {
            FORWARD_IF_ERROR(ZSTD_DDictHashSet_emplaceDDict(hashSet, oldTable[i]), "");
        }
    }
    ZSTD_customFree((void*)oldTable, customMem);
    DEBUGLOG(4, "Finished re-hash");
    return 0;
}


static const ZSTD_DDict* ZSTD_DDictHashSet_getDDict(ZSTD_DDictHashSet* hashSet, U32 dictID) {
    size_t idx = ZSTD_DDictHashSet_getIndex(hashSet, dictID);
    const size_t idxRangeMask = hashSet->ddictPtrTableSize - 1;
    DEBUGLOG(4, "Hashed index: for dictID: %u is %zu", dictID, idx);
    for (;;) {
        size_t currDictID = ZSTD_getDictID_fromDDict(hashSet->ddictPtrTable[idx]);
        if (currDictID == dictID || currDictID == 0) {

            break;
        } else {
            idx &= idxRangeMask;
            idx++;
        }
    }
    DEBUGLOG(4, "Final idx after probing for dictID %u is: %zu", dictID, idx);
    return hashSet->ddictPtrTable[idx];
}


static ZSTD_DDictHashSet* ZSTD_createDDictHashSet(ZSTD_customMem customMem) {
    ZSTD_DDictHashSet* ret = (ZSTD_DDictHashSet*)ZSTD_customMalloc(sizeof(ZSTD_DDictHashSet), customMem);
    DEBUGLOG(4, "Allocating new hash set");
    if (!ret)
        return NULL;
    ret->ddictPtrTable = (const ZSTD_DDict**)ZSTD_customCalloc(DDICT_HASHSET_TABLE_BASE_SIZE * sizeof(ZSTD_DDict*), customMem);
    if (!ret->ddictPtrTable) {
        ZSTD_customFree(ret, customMem);
        return NULL;
    }
    ret->ddictPtrTableSize = DDICT_HASHSET_TABLE_BASE_SIZE;
    ret->ddictPtrCount = 0;
    return ret;
}


static void ZSTD_freeDDictHashSet(ZSTD_DDictHashSet* hashSet, ZSTD_customMem customMem) {
    DEBUGLOG(4, "Freeing ddict hash set");
    if (hashSet && hashSet->ddictPtrTable) {
        ZSTD_customFree((void*)hashSet->ddictPtrTable, customMem);
    }
    if (hashSet) {
        ZSTD_customFree(hashSet, customMem);
    }
}


static size_t ZSTD_DDictHashSet_addDDict(ZSTD_DDictHashSet* hashSet, const ZSTD_DDict* ddict, ZSTD_customMem customMem) {
    DEBUGLOG(4, "Adding dict ID: %u to hashset with - Count: %zu Tablesize: %zu", ZSTD_getDictID_fromDDict(ddict), hashSet->ddictPtrCount, hashSet->ddictPtrTableSize);
    if (hashSet->ddictPtrCount * DDICT_HASHSET_MAX_LOAD_FACTOR_COUNT_MULT / hashSet->ddictPtrTableSize * DDICT_HASHSET_MAX_LOAD_FACTOR_SIZE_MULT != 0) {
        FORWARD_IF_ERROR(ZSTD_DDictHashSet_expand(hashSet, customMem), "");
    }
    FORWARD_IF_ERROR(ZSTD_DDictHashSet_emplaceDDict(hashSet, ddict), "");
    return 0;
}


size_t ZSTD_sizeof_DCtx (const ZSTD_DCtx* dctx)
{
    if (dctx==NULL) return 0;
    return sizeof(*dctx)
           + ZSTD_sizeof_DDict(dctx->ddictLocal)
           + dctx->inBuffSize + dctx->outBuffSize;
}

size_t ZSTD_estimateDCtxSize(void) { return sizeof(ZSTD_DCtx); }


static size_t ZSTD_startingInputLength(ZSTD_format_e format)
{
    size_t const startingInputLength = ZSTD_FRAMEHEADERSIZE_PREFIX(format);

    assert( (format == ZSTD_f_zstd1) || (format == ZSTD_f_zstd1_magicless) );
    return startingInputLength;
}

static void ZSTD_DCtx_resetParameters(ZSTD_DCtx* dctx)
{
    assert(dctx->streamStage == zdss_init);
    dctx->format = ZSTD_f_zstd1;
    dctx->maxWindowSize = ZSTD_MAXWINDOWSIZE_DEFAULT;
    dctx->outBufferMode = ZSTD_bm_buffered;
    dctx->forceIgnoreChecksum = ZSTD_d_validateChecksum;
    dctx->refMultipleDDicts = ZSTD_rmd_refSingleDDict;
    dctx->disableHufAsm = 0;
    dctx->maxBlockSizeParam = 0;
}

static void ZSTD_initDCtx_internal(ZSTD_DCtx* dctx)
{
    dctx->staticSize  = 0;
    dctx->ddict       = NULL;
    dctx->ddictLocal  = NULL;
    dctx->dictEnd     = NULL;
    dctx->ddictIsCold = 0;
    dctx->dictUses = ZSTD_dont_use;
    dctx->inBuff      = NULL;
    dctx->inBuffSize  = 0;
    dctx->outBuffSize = 0;
    dctx->streamStage = zdss_init;
#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT>=1)
    dctx->legacyContext = NULL;
    dctx->previousLegacyVersion = 0;
#endif
    dctx->noForwardProgress = 0;
    dctx->oversizedDuration = 0;
    dctx->isFrameDecompression = 1;
#if DYNAMIC_BMI2
    dctx->bmi2 = ZSTD_cpuSupportsBmi2();
#endif
    dctx->ddictSet = NULL;
    ZSTD_DCtx_resetParameters(dctx);
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    dctx->dictContentEndForFuzzing = NULL;
#endif
}

ZSTD_DCtx* ZSTD_initStaticDCtx(void *workspace, size_t workspaceSize)
{
    ZSTD_DCtx* const dctx = (ZSTD_DCtx*) workspace;

    if ((size_t)workspace & 7) return NULL;
    if (workspaceSize < sizeof(ZSTD_DCtx)) return NULL;

    ZSTD_initDCtx_internal(dctx);
    dctx->staticSize = workspaceSize;
    dctx->inBuff = (char*)(dctx+1);
    return dctx;
}

static ZSTD_DCtx* ZSTD_createDCtx_internal(ZSTD_customMem customMem) {
    if ((!customMem.customAlloc) ^ (!customMem.customFree)) return NULL;

    {   ZSTD_DCtx* const dctx = (ZSTD_DCtx*)ZSTD_customMalloc(sizeof(*dctx), customMem);
        if (!dctx) return NULL;
        dctx->customMem = customMem;
        ZSTD_initDCtx_internal(dctx);
        return dctx;
    }
}

ZSTD_DCtx* ZSTD_createDCtx_advanced(ZSTD_customMem customMem)
{
    return ZSTD_createDCtx_internal(customMem);
}

ZSTD_DCtx* ZSTD_createDCtx(void)
{
    DEBUGLOG(3, "ZSTD_createDCtx");
    return ZSTD_createDCtx_internal(ZSTD_defaultCMem);
}

static void ZSTD_clearDict(ZSTD_DCtx* dctx)
{
    ZSTD_freeDDict(dctx->ddictLocal);
    dctx->ddictLocal = NULL;
    dctx->ddict = NULL;
    dctx->dictUses = ZSTD_dont_use;
}

size_t ZSTD_freeDCtx(ZSTD_DCtx* dctx)
{
    if (dctx==NULL) return 0;
    RETURN_ERROR_IF(dctx->staticSize, memory_allocation, "not compatible with static DCtx");
    {   ZSTD_customMem const cMem = dctx->customMem;
        ZSTD_clearDict(dctx);
        ZSTD_customFree(dctx->inBuff, cMem);
        dctx->inBuff = NULL;
#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT >= 1)
        if (dctx->legacyContext)
            ZSTD_freeLegacyStreamContext(dctx->legacyContext, dctx->previousLegacyVersion);
#endif
        if (dctx->ddictSet) {
            ZSTD_freeDDictHashSet(dctx->ddictSet, cMem);
            dctx->ddictSet = NULL;
        }
        ZSTD_customFree(dctx, cMem);
        return 0;
    }
}


void ZSTD_copyDCtx(ZSTD_DCtx* dstDCtx, const ZSTD_DCtx* srcDCtx)
{
    size_t const toCopy = (size_t)((char*)(&dstDCtx->inBuff) - (char*)dstDCtx);
    ZSTD_memcpy(dstDCtx, srcDCtx, toCopy);
}


static void ZSTD_DCtx_selectFrameDDict(ZSTD_DCtx* dctx) {
    assert(dctx->refMultipleDDicts && dctx->ddictSet);
    DEBUGLOG(4, "Adjusting DDict based on requested dict ID from frame");
    if (dctx->ddict) {
        const ZSTD_DDict* frameDDict = ZSTD_DDictHashSet_getDDict(dctx->ddictSet, dctx->fParams.dictID);
        if (frameDDict) {
            DEBUGLOG(4, "DDict found!");
            ZSTD_clearDict(dctx);
            dctx->dictID = dctx->fParams.dictID;
            dctx->ddict = frameDDict;
            dctx->dictUses = ZSTD_use_indefinitely;
        }
    }
}


unsigned ZSTD_isFrame(const void* buffer, size_t size)
{
    if (size < ZSTD_FRAMEIDSIZE) return 0;
    {   U32 const magic = MEM_readLE32(buffer);
        if (magic == ZSTD_MAGICNUMBER) return 1;
        if ((magic & ZSTD_MAGIC_SKIPPABLE_MASK) == ZSTD_MAGIC_SKIPPABLE_START) return 1;
    }
#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT >= 1)
    if (ZSTD_isLegacy(buffer, size)) return 1;
#endif
    return 0;
}


unsigned ZSTD_isSkippableFrame(const void* buffer, size_t size)
{
    if (size < ZSTD_FRAMEIDSIZE) return 0;
    {   U32 const magic = MEM_readLE32(buffer);
        if ((magic & ZSTD_MAGIC_SKIPPABLE_MASK) == ZSTD_MAGIC_SKIPPABLE_START) return 1;
    }
    return 0;
}


static size_t ZSTD_frameHeaderSize_internal(const void* src, size_t srcSize, ZSTD_format_e format)
{
    size_t const minInputSize = ZSTD_startingInputLength(format);
    RETURN_ERROR_IF(srcSize < minInputSize, srcSize_wrong, "");

    {   BYTE const fhd = ((const BYTE*)src)[minInputSize-1];
        U32 const dictID= fhd & 3;
        U32 const singleSegment = (fhd >> 5) & 1;
        U32 const fcsId = fhd >> 6;
        return minInputSize + !singleSegment
             + ZSTD_did_fieldSize[dictID] + ZSTD_fcs_fieldSize[fcsId]
             + (singleSegment && !fcsId);
    }
}


size_t ZSTD_frameHeaderSize(const void* src, size_t srcSize)
{
    return ZSTD_frameHeaderSize_internal(src, srcSize, ZSTD_f_zstd1);
}


size_t ZSTD_getFrameHeader_advanced(ZSTD_FrameHeader* zfhPtr, const void* src, size_t srcSize, ZSTD_format_e format)
{
    const BYTE* ip = (const BYTE*)src;
    size_t const minInputSize = ZSTD_startingInputLength(format);

    DEBUGLOG(5, "ZSTD_getFrameHeader_advanced: minInputSize = %zu, srcSize = %zu", minInputSize, srcSize);

    if (srcSize > 0) {

        RETURN_ERROR_IF(src==NULL, GENERIC, "invalid parameter : src==NULL, but srcSize>0");
    }
    if (srcSize < minInputSize) {
        if (srcSize > 0 && format != ZSTD_f_zstd1_magicless) {

            size_t const toCopy = MIN(4, srcSize);
            unsigned char hbuf[4]; MEM_writeLE32(hbuf, ZSTD_MAGICNUMBER);
            assert(src != NULL);
            ZSTD_memcpy(hbuf, src, toCopy);
            if ( MEM_readLE32(hbuf) != ZSTD_MAGICNUMBER ) {

                MEM_writeLE32(hbuf, ZSTD_MAGIC_SKIPPABLE_START);
                ZSTD_memcpy(hbuf, src, toCopy);
                if ((MEM_readLE32(hbuf) & ZSTD_MAGIC_SKIPPABLE_MASK) != ZSTD_MAGIC_SKIPPABLE_START) {
                    RETURN_ERROR(prefix_unknown,
                                "first bytes don't correspond to any supported magic number");
        }   }   }
        return minInputSize;
    }

    ZSTD_memset(zfhPtr, 0, sizeof(*zfhPtr));
    if ( (format != ZSTD_f_zstd1_magicless)
      && (MEM_readLE32(src) != ZSTD_MAGICNUMBER) ) {
        if ((MEM_readLE32(src) & ZSTD_MAGIC_SKIPPABLE_MASK) == ZSTD_MAGIC_SKIPPABLE_START) {

            if (srcSize < ZSTD_SKIPPABLEHEADERSIZE)
                return ZSTD_SKIPPABLEHEADERSIZE;
            ZSTD_memset(zfhPtr, 0, sizeof(*zfhPtr));
            zfhPtr->frameType = ZSTD_skippableFrame;
            zfhPtr->dictID = MEM_readLE32(src) - ZSTD_MAGIC_SKIPPABLE_START;
            zfhPtr->headerSize = ZSTD_SKIPPABLEHEADERSIZE;
            zfhPtr->frameContentSize = MEM_readLE32((const char *)src + ZSTD_FRAMEIDSIZE);
            return 0;
        }
        RETURN_ERROR(prefix_unknown, "");
    }


    {   size_t const fhsize = ZSTD_frameHeaderSize_internal(src, srcSize, format);
        if (srcSize < fhsize) return fhsize;
        zfhPtr->headerSize = (U32)fhsize;
    }

    {   BYTE const fhdByte = ip[minInputSize-1];
        size_t pos = minInputSize;
        U32 const dictIDSizeCode = fhdByte&3;
        U32 const checksumFlag = (fhdByte>>2)&1;
        U32 const singleSegment = (fhdByte>>5)&1;
        U32 const fcsID = fhdByte>>6;
        U64 windowSize = 0;
        U32 dictID = 0;
        U64 frameContentSize = ZSTD_CONTENTSIZE_UNKNOWN;
        RETURN_ERROR_IF((fhdByte & 0x08) != 0, frameParameter_unsupported,
                        "reserved bits, must be zero");

        if (!singleSegment) {
            BYTE const wlByte = ip[pos++];
            U32 const windowLog = (wlByte >> 3) + ZSTD_WINDOWLOG_ABSOLUTEMIN;
            RETURN_ERROR_IF(windowLog > ZSTD_WINDOWLOG_MAX, frameParameter_windowTooLarge, "");
            windowSize = (1ULL << windowLog);
            windowSize += (windowSize >> 3) * (wlByte&7);
        }
        switch(dictIDSizeCode)
        {
            default:
                assert(0);
                ZSTD_FALLTHROUGH;
            case 0 : break;
            case 1 : dictID = ip[pos]; pos++; break;
            case 2 : dictID = MEM_readLE16(ip+pos); pos+=2; break;
            case 3 : dictID = MEM_readLE32(ip+pos); pos+=4; break;
        }
        switch(fcsID)
        {
            default:
                assert(0);
                ZSTD_FALLTHROUGH;
            case 0 : if (singleSegment) frameContentSize = ip[pos]; break;
            case 1 : frameContentSize = MEM_readLE16(ip+pos)+256; break;
            case 2 : frameContentSize = MEM_readLE32(ip+pos); break;
            case 3 : frameContentSize = MEM_readLE64(ip+pos); break;
        }
        if (singleSegment) windowSize = frameContentSize;

        zfhPtr->frameType = ZSTD_frame;
        zfhPtr->frameContentSize = frameContentSize;
        zfhPtr->windowSize = windowSize;
        zfhPtr->blockSizeMax = (unsigned) MIN(windowSize, ZSTD_BLOCKSIZE_MAX);
        zfhPtr->dictID = dictID;
        zfhPtr->checksumFlag = checksumFlag;
    }
    return 0;
}


size_t ZSTD_getFrameHeader(ZSTD_FrameHeader* zfhPtr, const void* src, size_t srcSize)
{
    return ZSTD_getFrameHeader_advanced(zfhPtr, src, srcSize, ZSTD_f_zstd1);
}


unsigned long long ZSTD_getFrameContentSize(const void *src, size_t srcSize)
{
#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT >= 1)
    if (ZSTD_isLegacy(src, srcSize)) {
        unsigned long long const ret = ZSTD_getDecompressedSize_legacy(src, srcSize);
        return ret == 0 ? ZSTD_CONTENTSIZE_UNKNOWN : ret;
    }
#endif
    {   ZSTD_FrameHeader zfh;
        if (ZSTD_getFrameHeader(&zfh, src, srcSize) != 0)
            return ZSTD_CONTENTSIZE_ERROR;
        if (zfh.frameType == ZSTD_skippableFrame) {
            return 0;
        } else {
            return zfh.frameContentSize;
    }   }
}

static size_t readSkippableFrameSize(void const* src, size_t srcSize)
{
    size_t const skippableHeaderSize = ZSTD_SKIPPABLEHEADERSIZE;
    U32 sizeU32;

    RETURN_ERROR_IF(srcSize < ZSTD_SKIPPABLEHEADERSIZE, srcSize_wrong, "");

    sizeU32 = MEM_readLE32((BYTE const*)src + ZSTD_FRAMEIDSIZE);
    RETURN_ERROR_IF((U32)(sizeU32 + ZSTD_SKIPPABLEHEADERSIZE) < sizeU32,
                    frameParameter_unsupported, "");
    {   size_t const skippableSize = skippableHeaderSize + sizeU32;
        RETURN_ERROR_IF(skippableSize > srcSize, srcSize_wrong, "");
        return skippableSize;
    }
}


size_t ZSTD_readSkippableFrame(void* dst, size_t dstCapacity,
                               unsigned* magicVariant,
                         const void* src, size_t srcSize)
{
    RETURN_ERROR_IF(srcSize < ZSTD_SKIPPABLEHEADERSIZE, srcSize_wrong, "");

    {   U32 const magicNumber = MEM_readLE32(src);
        size_t skippableFrameSize = readSkippableFrameSize(src, srcSize);
        size_t skippableContentSize = skippableFrameSize - ZSTD_SKIPPABLEHEADERSIZE;


        RETURN_ERROR_IF(!ZSTD_isSkippableFrame(src, srcSize), frameParameter_unsupported, "");
        RETURN_ERROR_IF(skippableFrameSize < ZSTD_SKIPPABLEHEADERSIZE || skippableFrameSize > srcSize, srcSize_wrong, "");
        RETURN_ERROR_IF(skippableContentSize > dstCapacity, dstSize_tooSmall, "");


        if (skippableContentSize > 0  && dst != NULL)
            ZSTD_memcpy(dst, (const BYTE *)src + ZSTD_SKIPPABLEHEADERSIZE, skippableContentSize);
        if (magicVariant != NULL)
            *magicVariant = magicNumber - ZSTD_MAGIC_SKIPPABLE_START;
        return skippableContentSize;
    }
}


unsigned long long ZSTD_findDecompressedSize(const void* src, size_t srcSize)
{
    unsigned long long totalDstSize = 0;

    while (srcSize >= ZSTD_startingInputLength(ZSTD_f_zstd1)) {
        U32 const magicNumber = MEM_readLE32(src);

        if ((magicNumber & ZSTD_MAGIC_SKIPPABLE_MASK) == ZSTD_MAGIC_SKIPPABLE_START) {
            size_t const skippableSize = readSkippableFrameSize(src, srcSize);
            if (ZSTD_isError(skippableSize)) return ZSTD_CONTENTSIZE_ERROR;
            assert(skippableSize <= srcSize);

            src = (const BYTE *)src + skippableSize;
            srcSize -= skippableSize;
            continue;
        }

        {   unsigned long long const fcs = ZSTD_getFrameContentSize(src, srcSize);
            if (fcs >= ZSTD_CONTENTSIZE_ERROR) return fcs;

            if (totalDstSize + fcs < totalDstSize)
                return ZSTD_CONTENTSIZE_ERROR;
            totalDstSize += fcs;
        }

        {   size_t const frameSrcSize = ZSTD_findFrameCompressedSize(src, srcSize);
            if (ZSTD_isError(frameSrcSize)) return ZSTD_CONTENTSIZE_ERROR;
            assert(frameSrcSize <= srcSize);

            src = (const BYTE *)src + frameSrcSize;
            srcSize -= frameSrcSize;
        }
    }

    if (srcSize) return ZSTD_CONTENTSIZE_ERROR;

    return totalDstSize;
}


unsigned long long ZSTD_getDecompressedSize(const void* src, size_t srcSize)
{
    unsigned long long const ret = ZSTD_getFrameContentSize(src, srcSize);
    ZSTD_STATIC_ASSERT(ZSTD_CONTENTSIZE_ERROR < ZSTD_CONTENTSIZE_UNKNOWN);
    return (ret >= ZSTD_CONTENTSIZE_ERROR) ? 0 : ret;
}


static size_t ZSTD_decodeFrameHeader(ZSTD_DCtx* dctx, const void* src, size_t headerSize)
{
    size_t const result = ZSTD_getFrameHeader_advanced(&(dctx->fParams), src, headerSize, dctx->format);
    if (ZSTD_isError(result)) return result;
    RETURN_ERROR_IF(result>0, srcSize_wrong, "headerSize too small");


    if (dctx->refMultipleDDicts == ZSTD_rmd_refMultipleDDicts && dctx->ddictSet) {
        ZSTD_DCtx_selectFrameDDict(dctx);
    }

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

    RETURN_ERROR_IF(dctx->fParams.dictID && (dctx->dictID != dctx->fParams.dictID),
                    dictionary_wrong, "");
#endif
    dctx->validateChecksum = (dctx->fParams.checksumFlag && !dctx->forceIgnoreChecksum) ? 1 : 0;
    if (dctx->validateChecksum) XXH64_reset(&dctx->xxhState, 0);
    dctx->processedCSize += headerSize;
    return 0;
}

static ZSTD_frameSizeInfo ZSTD_errorFrameSizeInfo(size_t ret)
{
    ZSTD_frameSizeInfo frameSizeInfo;
    frameSizeInfo.compressedSize = ret;
    frameSizeInfo.decompressedBound = ZSTD_CONTENTSIZE_ERROR;
    return frameSizeInfo;
}

static ZSTD_frameSizeInfo ZSTD_findFrameSizeInfo(const void* src, size_t srcSize, ZSTD_format_e format)
{
    ZSTD_frameSizeInfo frameSizeInfo;
    ZSTD_memset(&frameSizeInfo, 0, sizeof(ZSTD_frameSizeInfo));

#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT >= 1)
    if (format == ZSTD_f_zstd1 && ZSTD_isLegacy(src, srcSize))
        return ZSTD_findFrameSizeInfoLegacy(src, srcSize);
#endif

    if (format == ZSTD_f_zstd1 && (srcSize >= ZSTD_SKIPPABLEHEADERSIZE)
        && (MEM_readLE32(src) & ZSTD_MAGIC_SKIPPABLE_MASK) == ZSTD_MAGIC_SKIPPABLE_START) {
        frameSizeInfo.compressedSize = readSkippableFrameSize(src, srcSize);
        assert(ZSTD_isError(frameSizeInfo.compressedSize) ||
               frameSizeInfo.compressedSize <= srcSize);
        return frameSizeInfo;
    } else {
        const BYTE* ip = (const BYTE*)src;
        const BYTE* const ipstart = ip;
        size_t remainingSize = srcSize;
        size_t nbBlocks = 0;
        ZSTD_FrameHeader zfh;


        {   size_t const ret = ZSTD_getFrameHeader_advanced(&zfh, src, srcSize, format);
            if (ZSTD_isError(ret))
                return ZSTD_errorFrameSizeInfo(ret);
            if (ret > 0)
                return ZSTD_errorFrameSizeInfo(ERROR(srcSize_wrong));
        }

        ip += zfh.headerSize;
        remainingSize -= zfh.headerSize;


        while (1) {
            blockProperties_t blockProperties;
            size_t const cBlockSize = ZSTD_getcBlockSize(ip, remainingSize, &blockProperties);
            if (ZSTD_isError(cBlockSize))
                return ZSTD_errorFrameSizeInfo(cBlockSize);

            if (ZSTD_blockHeaderSize + cBlockSize > remainingSize)
                return ZSTD_errorFrameSizeInfo(ERROR(srcSize_wrong));

            ip += ZSTD_blockHeaderSize + cBlockSize;
            remainingSize -= ZSTD_blockHeaderSize + cBlockSize;
            nbBlocks++;

            if (blockProperties.lastBlock) break;
        }


        if (zfh.checksumFlag) {
            if (remainingSize < 4)
                return ZSTD_errorFrameSizeInfo(ERROR(srcSize_wrong));
            ip += 4;
        }

        frameSizeInfo.nbBlocks = nbBlocks;
        frameSizeInfo.compressedSize = (size_t)(ip - ipstart);
        frameSizeInfo.decompressedBound = (zfh.frameContentSize != ZSTD_CONTENTSIZE_UNKNOWN)
                                        ? zfh.frameContentSize
                                        : (unsigned long long)nbBlocks * zfh.blockSizeMax;
        return frameSizeInfo;
    }
}

static size_t ZSTD_findFrameCompressedSize_advanced(const void *src, size_t srcSize, ZSTD_format_e format) {
    ZSTD_frameSizeInfo const frameSizeInfo = ZSTD_findFrameSizeInfo(src, srcSize, format);
    return frameSizeInfo.compressedSize;
}


size_t ZSTD_findFrameCompressedSize(const void *src, size_t srcSize)
{
    return ZSTD_findFrameCompressedSize_advanced(src, srcSize, ZSTD_f_zstd1);
}


unsigned long long ZSTD_decompressBound(const void* src, size_t srcSize)
{
    unsigned long long bound = 0;

    while (srcSize > 0) {
        ZSTD_frameSizeInfo const frameSizeInfo = ZSTD_findFrameSizeInfo(src, srcSize, ZSTD_f_zstd1);
        size_t const compressedSize = frameSizeInfo.compressedSize;
        unsigned long long const decompressedBound = frameSizeInfo.decompressedBound;
        if (ZSTD_isError(compressedSize) || decompressedBound == ZSTD_CONTENTSIZE_ERROR)
            return ZSTD_CONTENTSIZE_ERROR;
        assert(srcSize >= compressedSize);
        src = (const BYTE*)src + compressedSize;
        srcSize -= compressedSize;
        bound += decompressedBound;
    }
    return bound;
}

size_t ZSTD_decompressionMargin(void const* src, size_t srcSize)
{
    size_t margin = 0;
    unsigned maxBlockSize = 0;


    while (srcSize > 0) {
        ZSTD_frameSizeInfo const frameSizeInfo = ZSTD_findFrameSizeInfo(src, srcSize, ZSTD_f_zstd1);
        size_t const compressedSize = frameSizeInfo.compressedSize;
        unsigned long long const decompressedBound = frameSizeInfo.decompressedBound;
        ZSTD_FrameHeader zfh;

        FORWARD_IF_ERROR(ZSTD_getFrameHeader(&zfh, src, srcSize), "");
        if (ZSTD_isError(compressedSize) || decompressedBound == ZSTD_CONTENTSIZE_ERROR)
            return ERROR(corruption_detected);

        if (zfh.frameType == ZSTD_frame) {

            margin += zfh.headerSize;

            margin += zfh.checksumFlag ? 4 : 0;

            margin += 3 * frameSizeInfo.nbBlocks;


            maxBlockSize = MAX(maxBlockSize, zfh.blockSizeMax);
        } else {
            assert(zfh.frameType == ZSTD_skippableFrame);

            margin += compressedSize;
        }

        assert(srcSize >= compressedSize);
        src = (const BYTE*)src + compressedSize;
        srcSize -= compressedSize;
    }


    margin += maxBlockSize;

    return margin;
}


size_t ZSTD_insertBlock(ZSTD_DCtx* dctx, const void* blockStart, size_t blockSize)
{
    DEBUGLOG(5, "ZSTD_insertBlock: %u bytes", (unsigned)blockSize);
    ZSTD_checkContinuity(dctx, blockStart, blockSize);
    dctx->previousDstEnd = (const char*)blockStart + blockSize;
    return blockSize;
}


static size_t ZSTD_copyRawBlock(void* dst, size_t dstCapacity,
                          const void* src, size_t srcSize)
{
    DEBUGLOG(5, "ZSTD_copyRawBlock");
    RETURN_ERROR_IF(srcSize > dstCapacity, dstSize_tooSmall, "");
    if (dst == NULL) {
        if (srcSize == 0) return 0;
        RETURN_ERROR(dstBuffer_null, "");
    }
    ZSTD_memmove(dst, src, srcSize);
    return srcSize;
}

static size_t ZSTD_setRleBlock(void* dst, size_t dstCapacity,
                               BYTE b,
                               size_t regenSize)
{
    RETURN_ERROR_IF(regenSize > dstCapacity, dstSize_tooSmall, "");
    if (dst == NULL) {
        if (regenSize == 0) return 0;
        RETURN_ERROR(dstBuffer_null, "");
    }
    ZSTD_memset(dst, b, regenSize);
    return regenSize;
}

static void ZSTD_DCtx_trace_end(ZSTD_DCtx const* dctx, U64 uncompressedSize, U64 compressedSize, int streaming)
{
#if ZSTD_TRACE
    if (dctx->traceCtx && ZSTD_trace_decompress_end != NULL) {
        ZSTD_Trace trace;
        ZSTD_memset(&trace, 0, sizeof(trace));
        trace.version = ZSTD_VERSION_NUMBER;
        trace.streaming = streaming;
        if (dctx->ddict) {
            trace.dictionaryID = ZSTD_getDictID_fromDDict(dctx->ddict);
            trace.dictionarySize = ZSTD_DDict_dictSize(dctx->ddict);
            trace.dictionaryIsCold = dctx->ddictIsCold;
        }
        trace.uncompressedSize = (size_t)uncompressedSize;
        trace.compressedSize = (size_t)compressedSize;
        trace.dctx = dctx;
        ZSTD_trace_decompress_end(dctx->traceCtx, &trace);
    }
#else
    (void)dctx;
    (void)uncompressedSize;
    (void)compressedSize;
    (void)streaming;
#endif
}


static size_t ZSTD_decompressFrame(ZSTD_DCtx* dctx,
                                   void* dst, size_t dstCapacity,
                             const void** srcPtr, size_t *srcSizePtr)
{
    const BYTE* const istart = (const BYTE*)(*srcPtr);
    const BYTE* ip = istart;
    BYTE* const ostart = (BYTE*)dst;
    BYTE* const oend = dstCapacity != 0 ? ostart + dstCapacity : ostart;
    BYTE* op = ostart;
    size_t remainingSrcSize = *srcSizePtr;

    DEBUGLOG(4, "ZSTD_decompressFrame (srcSize:%i)", (int)*srcSizePtr);


    RETURN_ERROR_IF(
        remainingSrcSize < ZSTD_FRAMEHEADERSIZE_MIN(dctx->format)+ZSTD_blockHeaderSize,
        srcSize_wrong, "");


    {   size_t const frameHeaderSize = ZSTD_frameHeaderSize_internal(
                ip, ZSTD_FRAMEHEADERSIZE_PREFIX(dctx->format), dctx->format);
        if (ZSTD_isError(frameHeaderSize)) return frameHeaderSize;
        RETURN_ERROR_IF(remainingSrcSize < frameHeaderSize+ZSTD_blockHeaderSize,
                        srcSize_wrong, "");
        FORWARD_IF_ERROR( ZSTD_decodeFrameHeader(dctx, ip, frameHeaderSize) , "");
        ip += frameHeaderSize; remainingSrcSize -= frameHeaderSize;
    }


    if (dctx->maxBlockSizeParam != 0)
        dctx->fParams.blockSizeMax = MIN(dctx->fParams.blockSizeMax, (unsigned)dctx->maxBlockSizeParam);


    while (1) {
        BYTE* oBlockEnd = oend;
        size_t decodedSize;
        blockProperties_t blockProperties;
        size_t const cBlockSize = ZSTD_getcBlockSize(ip, remainingSrcSize, &blockProperties);
        if (ZSTD_isError(cBlockSize)) return cBlockSize;

        ip += ZSTD_blockHeaderSize;
        remainingSrcSize -= ZSTD_blockHeaderSize;
        RETURN_ERROR_IF(cBlockSize > remainingSrcSize, srcSize_wrong, "");

        if (ip >= op && ip < oBlockEnd) {

            oBlockEnd = op + (ip - op);
        }

        switch(blockProperties.blockType)
        {
        case bt_compressed:
            assert(dctx->isFrameDecompression == 1);
            decodedSize = ZSTD_decompressBlock_internal(dctx, op, (size_t)(oBlockEnd-op), ip, cBlockSize, not_streaming);
            break;
        case bt_raw :

            decodedSize = ZSTD_copyRawBlock(op, (size_t)(oend-op), ip, cBlockSize);
            break;
        case bt_rle :
            decodedSize = ZSTD_setRleBlock(op, (size_t)(oBlockEnd-op), *ip, blockProperties.origSize);
            break;
        case bt_reserved :
        default:
            RETURN_ERROR(corruption_detected, "invalid block type");
        }
        FORWARD_IF_ERROR(decodedSize, "Block decompression failure");
        DEBUGLOG(5, "Decompressed block of dSize = %u", (unsigned)decodedSize);
        if (dctx->validateChecksum) {
            XXH64_update(&dctx->xxhState, op, decodedSize);
        }
        if (decodedSize)  {
            op += decodedSize;
        }
        assert(ip != NULL);
        ip += cBlockSize;
        remainingSrcSize -= cBlockSize;
        if (blockProperties.lastBlock) break;
    }

    if (dctx->fParams.frameContentSize != ZSTD_CONTENTSIZE_UNKNOWN) {
        RETURN_ERROR_IF((U64)(op-ostart) != dctx->fParams.frameContentSize,
                        corruption_detected, "");
    }
    if (dctx->fParams.checksumFlag) {
        RETURN_ERROR_IF(remainingSrcSize<4, checksum_wrong, "");
        if (!dctx->forceIgnoreChecksum) {
            U32 const checkCalc = (U32)XXH64_digest(&dctx->xxhState);
            U32 checkRead;
            checkRead = MEM_readLE32(ip);
            RETURN_ERROR_IF(checkRead != checkCalc, checksum_wrong, "");
        }
        ip += 4;
        remainingSrcSize -= 4;
    }
    ZSTD_DCtx_trace_end(dctx, (U64)(op-ostart), (U64)(ip-istart),  0);

    DEBUGLOG(4, "ZSTD_decompressFrame: decompressed frame of size %i, consuming %i bytes of input", (int)(op-ostart), (int)(ip - (const BYTE*)*srcPtr));
    *srcPtr = ip;
    *srcSizePtr = remainingSrcSize;
    return (size_t)(op-ostart);
}

static
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
size_t ZSTD_decompressMultiFrame(ZSTD_DCtx* dctx,
                                        void* dst, size_t dstCapacity,
                                  const void* src, size_t srcSize,
                                  const void* dict, size_t dictSize,
                                  const ZSTD_DDict* ddict)
{
    void* const dststart = dst;
    int moreThan1Frame = 0;

    DEBUGLOG(5, "ZSTD_decompressMultiFrame");
    assert(dict==NULL || ddict==NULL);

    if (ddict) {
        dict = ZSTD_DDict_dictContent(ddict);
        dictSize = ZSTD_DDict_dictSize(ddict);
    }

    while (srcSize >= ZSTD_startingInputLength(dctx->format)) {

#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT >= 1)
        if (dctx->format == ZSTD_f_zstd1 && ZSTD_isLegacy(src, srcSize)) {
            size_t decodedSize;
            size_t const frameSize = ZSTD_findFrameCompressedSizeLegacy(src, srcSize);
            if (ZSTD_isError(frameSize)) return frameSize;
            RETURN_ERROR_IF(dctx->staticSize, memory_allocation,
                "legacy support is not compatible with static dctx");

            decodedSize = ZSTD_decompressLegacy(dst, dstCapacity, src, frameSize, dict, dictSize);
            if (ZSTD_isError(decodedSize)) return decodedSize;

            {
                unsigned long long const expectedSize = ZSTD_getFrameContentSize(src, srcSize);
                RETURN_ERROR_IF(expectedSize == ZSTD_CONTENTSIZE_ERROR, corruption_detected, "Corrupted frame header!");
                if (expectedSize != ZSTD_CONTENTSIZE_UNKNOWN) {
                    RETURN_ERROR_IF(expectedSize != decodedSize, corruption_detected,
                        "Frame header size does not match decoded size!");
                }
            }

            assert(decodedSize <= dstCapacity);
            dst = (BYTE*)dst + decodedSize;
            dstCapacity -= decodedSize;

            src = (const BYTE*)src + frameSize;
            srcSize -= frameSize;

            continue;
        }
#endif

        if (dctx->format == ZSTD_f_zstd1 && srcSize >= 4) {
            U32 const magicNumber = MEM_readLE32(src);
            DEBUGLOG(5, "reading magic number %08X", (unsigned)magicNumber);
            if ((magicNumber & ZSTD_MAGIC_SKIPPABLE_MASK) == ZSTD_MAGIC_SKIPPABLE_START) {

                size_t const skippableSize = readSkippableFrameSize(src, srcSize);
                FORWARD_IF_ERROR(skippableSize, "invalid skippable frame");
                assert(skippableSize <= srcSize);

                src = (const BYTE *)src + skippableSize;
                srcSize -= skippableSize;
                continue;
        }   }

        if (ddict) {

            FORWARD_IF_ERROR(ZSTD_decompressBegin_usingDDict(dctx, ddict), "");
        } else {

            FORWARD_IF_ERROR(ZSTD_decompressBegin_usingDict(dctx, dict, dictSize), "");
        }
        ZSTD_checkContinuity(dctx, dst, dstCapacity);

        {   const size_t res = ZSTD_decompressFrame(dctx, dst, dstCapacity,
                                                    &src, &srcSize);
            RETURN_ERROR_IF(
                (ZSTD_getErrorCode(res) == ZSTD_error_prefix_unknown)
             && (moreThan1Frame==1),
                srcSize_wrong,
                "At least one frame successfully completed, "
                "but following bytes are garbage: "
                "it's more likely to be a srcSize error, "
                "specifying more input bytes than size of frame(s). "
                "Note: one could be unlucky, it might be a corruption error instead, "
                "happening right at the place where we expect zstd magic bytes. "
                "But this is _much_ less likely than a srcSize field error.");
            if (ZSTD_isError(res)) return res;
            assert(res <= dstCapacity);
            if (res != 0)
                dst = (BYTE*)dst + res;
            dstCapacity -= res;
        }
        moreThan1Frame = 1;
    }

    RETURN_ERROR_IF(srcSize, srcSize_wrong, "input not entirely consumed");

    return (size_t)((BYTE*)dst - (BYTE*)dststart);
}

size_t ZSTD_decompress_usingDict(ZSTD_DCtx* dctx,
                                 void* dst, size_t dstCapacity,
                           const void* src, size_t srcSize,
                           const void* dict, size_t dictSize)
{
    return ZSTD_decompressMultiFrame(dctx, dst, dstCapacity, src, srcSize, dict, dictSize, NULL);
}


static ZSTD_DDict const* ZSTD_getDDict(ZSTD_DCtx* dctx)
{
    switch (dctx->dictUses) {
    default:
        assert(0 );
        ZSTD_FALLTHROUGH;
    case ZSTD_dont_use:
        ZSTD_clearDict(dctx);
        return NULL;
    case ZSTD_use_indefinitely:
        return dctx->ddict;
    case ZSTD_use_once:
        dctx->dictUses = ZSTD_dont_use;
        return dctx->ddict;
    }
}

size_t ZSTD_decompressDCtx(ZSTD_DCtx* dctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize)
{
    return ZSTD_decompress_usingDDict(dctx, dst, dstCapacity, src, srcSize, ZSTD_getDDict(dctx));
}


size_t ZSTD_decompress(void* dst, size_t dstCapacity, const void* src, size_t srcSize)
{
#if defined(ZSTD_HEAPMODE) && (ZSTD_HEAPMODE>=1)
    size_t regenSize;
    ZSTD_DCtx* const dctx =  ZSTD_createDCtx_internal(ZSTD_defaultCMem);
    RETURN_ERROR_IF(dctx==NULL, memory_allocation, "NULL pointer!");
    regenSize = ZSTD_decompressDCtx(dctx, dst, dstCapacity, src, srcSize);
    ZSTD_freeDCtx(dctx);
    return regenSize;
#else
    ZSTD_DCtx dctx;
    ZSTD_initDCtx_internal(&dctx);
    return ZSTD_decompressDCtx(&dctx, dst, dstCapacity, src, srcSize);
#endif
}


size_t ZSTD_nextSrcSizeToDecompress(ZSTD_DCtx* dctx) { return dctx->expected; }


static size_t ZSTD_nextSrcSizeToDecompressWithInputSize(ZSTD_DCtx* dctx, size_t inputSize) {
    if (!(dctx->stage == ZSTDds_decompressBlock || dctx->stage == ZSTDds_decompressLastBlock))
        return dctx->expected;
    if (dctx->bType != bt_raw)
        return dctx->expected;
    return BOUNDED(1, inputSize, dctx->expected);
}

ZSTD_nextInputType_e ZSTD_nextInputType(ZSTD_DCtx* dctx) {
    switch(dctx->stage)
    {
    default:
        assert(0);
        ZSTD_FALLTHROUGH;
    case ZSTDds_getFrameHeaderSize:
        ZSTD_FALLTHROUGH;
    case ZSTDds_decodeFrameHeader:
        return ZSTDnit_frameHeader;
    case ZSTDds_decodeBlockHeader:
        return ZSTDnit_blockHeader;
    case ZSTDds_decompressBlock:
        return ZSTDnit_block;
    case ZSTDds_decompressLastBlock:
        return ZSTDnit_lastBlock;
    case ZSTDds_checkChecksum:
        return ZSTDnit_checksum;
    case ZSTDds_decodeSkippableHeader:
        ZSTD_FALLTHROUGH;
    case ZSTDds_skipFrame:
        return ZSTDnit_skippableFrame;
    }
}

static int ZSTD_isSkipFrame(ZSTD_DCtx* dctx) { return dctx->stage == ZSTDds_skipFrame; }


size_t ZSTD_decompressContinue(ZSTD_DCtx* dctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize)
{
    DEBUGLOG(5, "ZSTD_decompressContinue (srcSize:%u)", (unsigned)srcSize);

    RETURN_ERROR_IF(srcSize != ZSTD_nextSrcSizeToDecompressWithInputSize(dctx, srcSize), srcSize_wrong, "not allowed");
    ZSTD_checkContinuity(dctx, dst, dstCapacity);

    dctx->processedCSize += srcSize;

    switch (dctx->stage)
    {
    case ZSTDds_getFrameHeaderSize :
        assert(src != NULL);
        if (dctx->format == ZSTD_f_zstd1) {
            assert(srcSize >= ZSTD_FRAMEIDSIZE);
            if ((MEM_readLE32(src) & ZSTD_MAGIC_SKIPPABLE_MASK) == ZSTD_MAGIC_SKIPPABLE_START) {
                ZSTD_memcpy(dctx->headerBuffer, src, srcSize);
                dctx->expected = ZSTD_SKIPPABLEHEADERSIZE - srcSize;
                dctx->stage = ZSTDds_decodeSkippableHeader;
                return 0;
        }   }
        dctx->headerSize = ZSTD_frameHeaderSize_internal(src, srcSize, dctx->format);
        if (ZSTD_isError(dctx->headerSize)) return dctx->headerSize;
        ZSTD_memcpy(dctx->headerBuffer, src, srcSize);
        dctx->expected = dctx->headerSize - srcSize;
        dctx->stage = ZSTDds_decodeFrameHeader;
        return 0;

    case ZSTDds_decodeFrameHeader:
        assert(src != NULL);
        ZSTD_memcpy(dctx->headerBuffer + (dctx->headerSize - srcSize), src, srcSize);
        FORWARD_IF_ERROR(ZSTD_decodeFrameHeader(dctx, dctx->headerBuffer, dctx->headerSize), "");
        dctx->expected = ZSTD_blockHeaderSize;
        dctx->stage = ZSTDds_decodeBlockHeader;
        return 0;

    case ZSTDds_decodeBlockHeader:
        {   blockProperties_t bp;
            size_t const cBlockSize = ZSTD_getcBlockSize(src, ZSTD_blockHeaderSize, &bp);
            if (ZSTD_isError(cBlockSize)) return cBlockSize;
            RETURN_ERROR_IF(cBlockSize > dctx->fParams.blockSizeMax, corruption_detected, "Block Size Exceeds Maximum");
            dctx->expected = cBlockSize;
            dctx->bType = bp.blockType;
            dctx->rleSize = bp.origSize;
            if (cBlockSize) {
                dctx->stage = bp.lastBlock ? ZSTDds_decompressLastBlock : ZSTDds_decompressBlock;
                return 0;
            }

            if (bp.lastBlock) {
                if (dctx->fParams.checksumFlag) {
                    dctx->expected = 4;
                    dctx->stage = ZSTDds_checkChecksum;
                } else {
                    dctx->expected = 0;
                    dctx->stage = ZSTDds_getFrameHeaderSize;
                }
            } else {
                dctx->expected = ZSTD_blockHeaderSize;
                dctx->stage = ZSTDds_decodeBlockHeader;
            }
            return 0;
        }

    case ZSTDds_decompressLastBlock:
    case ZSTDds_decompressBlock:
        DEBUGLOG(5, "ZSTD_decompressContinue: case ZSTDds_decompressBlock");
        {   size_t rSize;
            switch(dctx->bType)
            {
            case bt_compressed:
                DEBUGLOG(5, "ZSTD_decompressContinue: case bt_compressed");
                assert(dctx->isFrameDecompression == 1);
                rSize = ZSTD_decompressBlock_internal(dctx, dst, dstCapacity, src, srcSize, is_streaming);
                dctx->expected = 0;
                break;
            case bt_raw :
                assert(srcSize <= dctx->expected);
                rSize = ZSTD_copyRawBlock(dst, dstCapacity, src, srcSize);
                FORWARD_IF_ERROR(rSize, "ZSTD_copyRawBlock failed");
                assert(rSize == srcSize);
                dctx->expected -= rSize;
                break;
            case bt_rle :
                rSize = ZSTD_setRleBlock(dst, dstCapacity, *(const BYTE*)src, dctx->rleSize);
                dctx->expected = 0;
                break;
            case bt_reserved :
            default:
                RETURN_ERROR(corruption_detected, "invalid block type");
            }
            FORWARD_IF_ERROR(rSize, "");
            RETURN_ERROR_IF(rSize > dctx->fParams.blockSizeMax, corruption_detected, "Decompressed Block Size Exceeds Maximum");
            DEBUGLOG(5, "ZSTD_decompressContinue: decoded size from block : %u", (unsigned)rSize);
            dctx->decodedSize += rSize;
            if (dctx->validateChecksum) XXH64_update(&dctx->xxhState, dst, rSize);
            dctx->previousDstEnd = (char*)dst + rSize;


            if (dctx->expected > 0) {
                return rSize;
            }

            if (dctx->stage == ZSTDds_decompressLastBlock) {
                DEBUGLOG(4, "ZSTD_decompressContinue: decoded size from frame : %u", (unsigned)dctx->decodedSize);
                RETURN_ERROR_IF(
                    dctx->fParams.frameContentSize != ZSTD_CONTENTSIZE_UNKNOWN
                 && dctx->decodedSize != dctx->fParams.frameContentSize,
                    corruption_detected, "");
                if (dctx->fParams.checksumFlag) {
                    dctx->expected = 4;
                    dctx->stage = ZSTDds_checkChecksum;
                } else {
                    ZSTD_DCtx_trace_end(dctx, dctx->decodedSize, dctx->processedCSize,  1);
                    dctx->expected = 0;
                    dctx->stage = ZSTDds_getFrameHeaderSize;
                }
            } else {
                dctx->stage = ZSTDds_decodeBlockHeader;
                dctx->expected = ZSTD_blockHeaderSize;
            }
            return rSize;
        }

    case ZSTDds_checkChecksum:
        assert(srcSize == 4);
        {
            if (dctx->validateChecksum) {
                U32 const h32 = (U32)XXH64_digest(&dctx->xxhState);
                U32 const check32 = MEM_readLE32(src);
                DEBUGLOG(4, "ZSTD_decompressContinue: checksum : calculated %08X :: %08X read", (unsigned)h32, (unsigned)check32);
                RETURN_ERROR_IF(check32 != h32, checksum_wrong, "");
            }
            ZSTD_DCtx_trace_end(dctx, dctx->decodedSize, dctx->processedCSize,  1);
            dctx->expected = 0;
            dctx->stage = ZSTDds_getFrameHeaderSize;
            return 0;
        }

    case ZSTDds_decodeSkippableHeader:
        assert(src != NULL);
        assert(srcSize <= ZSTD_SKIPPABLEHEADERSIZE);
        assert(dctx->format != ZSTD_f_zstd1_magicless);
        ZSTD_memcpy(dctx->headerBuffer + (ZSTD_SKIPPABLEHEADERSIZE - srcSize), src, srcSize);
        dctx->expected = MEM_readLE32(dctx->headerBuffer + ZSTD_FRAMEIDSIZE);
        dctx->stage = ZSTDds_skipFrame;
        return 0;

    case ZSTDds_skipFrame:
        dctx->expected = 0;
        dctx->stage = ZSTDds_getFrameHeaderSize;
        return 0;

    default:
        assert(0);
        RETURN_ERROR(GENERIC, "impossible to reach");
    }
}


static size_t ZSTD_refDictContent(ZSTD_DCtx* dctx, const void* dict, size_t dictSize)
{
    dctx->dictEnd = dctx->previousDstEnd;
    dctx->virtualStart = (const char*)dict - ((const char*)(dctx->previousDstEnd) - (const char*)(dctx->prefixStart));
    dctx->prefixStart = dict;
    dctx->previousDstEnd = (const char*)dict + dictSize;
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    dctx->dictContentBeginForFuzzing = dctx->prefixStart;
    dctx->dictContentEndForFuzzing = dctx->previousDstEnd;
#endif
    return 0;
}


size_t
ZSTD_loadDEntropy(ZSTD_entropyDTables_t* entropy,
                  const void* const dict, size_t const dictSize)
{
    const BYTE* dictPtr = (const BYTE*)dict;
    const BYTE* const dictEnd = dictPtr + dictSize;

    RETURN_ERROR_IF(dictSize <= 8, dictionary_corrupted, "dict is too small");
    assert(MEM_readLE32(dict) == ZSTD_MAGIC_DICTIONARY);
    dictPtr += 8;

    ZSTD_STATIC_ASSERT(offsetof(ZSTD_entropyDTables_t, OFTable) == offsetof(ZSTD_entropyDTables_t, LLTable) + sizeof(entropy->LLTable));
    ZSTD_STATIC_ASSERT(offsetof(ZSTD_entropyDTables_t, MLTable) == offsetof(ZSTD_entropyDTables_t, OFTable) + sizeof(entropy->OFTable));
    ZSTD_STATIC_ASSERT(sizeof(entropy->LLTable) + sizeof(entropy->OFTable) + sizeof(entropy->MLTable) >= HUF_DECOMPRESS_WORKSPACE_SIZE);
    {   void* const workspace = &entropy->LLTable;
        size_t const workspaceSize = sizeof(entropy->LLTable) + sizeof(entropy->OFTable) + sizeof(entropy->MLTable);
#ifdef HUF_FORCE_DECOMPRESS_X1

        size_t const hSize = HUF_readDTableX1_wksp(entropy->hufTable,
                                                dictPtr, dictEnd - dictPtr,
                                                workspace, workspaceSize,  0);
#else
        size_t const hSize = HUF_readDTableX2_wksp(entropy->hufTable,
                                                dictPtr, (size_t)(dictEnd - dictPtr),
                                                workspace, workspaceSize,  0);
#endif
        RETURN_ERROR_IF(HUF_isError(hSize), dictionary_corrupted, "");
        dictPtr += hSize;
    }

    {   short offcodeNCount[MaxOff+1];
        unsigned offcodeMaxValue = MaxOff, offcodeLog;
        size_t const offcodeHeaderSize = FSE_readNCount(offcodeNCount, &offcodeMaxValue, &offcodeLog, dictPtr, (size_t)(dictEnd-dictPtr));
        RETURN_ERROR_IF(FSE_isError(offcodeHeaderSize), dictionary_corrupted, "");
        RETURN_ERROR_IF(offcodeMaxValue > MaxOff, dictionary_corrupted, "");
        RETURN_ERROR_IF(offcodeLog > OffFSELog, dictionary_corrupted, "");
        ZSTD_buildFSETable( entropy->OFTable,
                            offcodeNCount, offcodeMaxValue,
                            OF_base, OF_bits,
                            offcodeLog,
                            entropy->workspace, sizeof(entropy->workspace),
                            0);
        dictPtr += offcodeHeaderSize;
    }

    {   short matchlengthNCount[MaxML+1];
        unsigned matchlengthMaxValue = MaxML, matchlengthLog;
        size_t const matchlengthHeaderSize = FSE_readNCount(matchlengthNCount, &matchlengthMaxValue, &matchlengthLog, dictPtr, (size_t)(dictEnd-dictPtr));
        RETURN_ERROR_IF(FSE_isError(matchlengthHeaderSize), dictionary_corrupted, "");
        RETURN_ERROR_IF(matchlengthMaxValue > MaxML, dictionary_corrupted, "");
        RETURN_ERROR_IF(matchlengthLog > MLFSELog, dictionary_corrupted, "");
        ZSTD_buildFSETable( entropy->MLTable,
                            matchlengthNCount, matchlengthMaxValue,
                            ML_base, ML_bits,
                            matchlengthLog,
                            entropy->workspace, sizeof(entropy->workspace),
                             0);
        dictPtr += matchlengthHeaderSize;
    }

    {   short litlengthNCount[MaxLL+1];
        unsigned litlengthMaxValue = MaxLL, litlengthLog;
        size_t const litlengthHeaderSize = FSE_readNCount(litlengthNCount, &litlengthMaxValue, &litlengthLog, dictPtr, (size_t)(dictEnd-dictPtr));
        RETURN_ERROR_IF(FSE_isError(litlengthHeaderSize), dictionary_corrupted, "");
        RETURN_ERROR_IF(litlengthMaxValue > MaxLL, dictionary_corrupted, "");
        RETURN_ERROR_IF(litlengthLog > LLFSELog, dictionary_corrupted, "");
        ZSTD_buildFSETable( entropy->LLTable,
                            litlengthNCount, litlengthMaxValue,
                            LL_base, LL_bits,
                            litlengthLog,
                            entropy->workspace, sizeof(entropy->workspace),
                             0);
        dictPtr += litlengthHeaderSize;
    }

    RETURN_ERROR_IF(dictPtr+12 > dictEnd, dictionary_corrupted, "");
    {   int i;
        size_t const dictContentSize = (size_t)(dictEnd - (dictPtr+12));
        for (i=0; i<3; i++) {
            U32 const rep = MEM_readLE32(dictPtr); dictPtr += 4;
            RETURN_ERROR_IF(rep==0 || rep > dictContentSize,
                            dictionary_corrupted, "");
            entropy->rep[i] = rep;
    }   }

    return (size_t)(dictPtr - (const BYTE*)dict);
}

static size_t ZSTD_decompress_insertDictionary(ZSTD_DCtx* dctx, const void* dict, size_t dictSize)
{
    if (dictSize < 8) return ZSTD_refDictContent(dctx, dict, dictSize);
    {   U32 const magic = MEM_readLE32(dict);
        if (magic != ZSTD_MAGIC_DICTIONARY) {
            return ZSTD_refDictContent(dctx, dict, dictSize);
    }   }
    dctx->dictID = MEM_readLE32((const char*)dict + ZSTD_FRAMEIDSIZE);


    {   size_t const eSize = ZSTD_loadDEntropy(&dctx->entropy, dict, dictSize);
        RETURN_ERROR_IF(ZSTD_isError(eSize), dictionary_corrupted, "");
        dict = (const char*)dict + eSize;
        dictSize -= eSize;
    }
    dctx->litEntropy = dctx->fseEntropy = 1;


    return ZSTD_refDictContent(dctx, dict, dictSize);
}

size_t ZSTD_decompressBegin(ZSTD_DCtx* dctx)
{
    assert(dctx != NULL);
#if ZSTD_TRACE
    dctx->traceCtx = (ZSTD_trace_decompress_begin != NULL) ? ZSTD_trace_decompress_begin(dctx) : 0;
#endif
    dctx->expected = ZSTD_startingInputLength(dctx->format);
    dctx->stage = ZSTDds_getFrameHeaderSize;
    dctx->processedCSize = 0;
    dctx->decodedSize = 0;
    dctx->previousDstEnd = NULL;
    dctx->prefixStart = NULL;
    dctx->virtualStart = NULL;
    dctx->dictEnd = NULL;
    dctx->entropy.hufTable[0] = (HUF_DTable)((ZSTD_HUFFDTABLE_CAPACITY_LOG)*0x1000001);
    dctx->litEntropy = dctx->fseEntropy = 0;
    dctx->dictID = 0;
    dctx->bType = bt_reserved;
    dctx->isFrameDecompression = 1;
    ZSTD_STATIC_ASSERT(sizeof(dctx->entropy.rep) == sizeof(repStartValue));
    ZSTD_memcpy(dctx->entropy.rep, repStartValue, sizeof(repStartValue));
    dctx->LLTptr = dctx->entropy.LLTable;
    dctx->MLTptr = dctx->entropy.MLTable;
    dctx->OFTptr = dctx->entropy.OFTable;
    dctx->HUFptr = dctx->entropy.hufTable;
    return 0;
}

size_t ZSTD_decompressBegin_usingDict(ZSTD_DCtx* dctx, const void* dict, size_t dictSize)
{
    FORWARD_IF_ERROR( ZSTD_decompressBegin(dctx) , "");
    if (dict && dictSize)
        RETURN_ERROR_IF(
            ZSTD_isError(ZSTD_decompress_insertDictionary(dctx, dict, dictSize)),
            dictionary_corrupted, "");
    return 0;
}


size_t ZSTD_decompressBegin_usingDDict(ZSTD_DCtx* dctx, const ZSTD_DDict* ddict)
{
    DEBUGLOG(4, "ZSTD_decompressBegin_usingDDict");
    assert(dctx != NULL);
    if (ddict) {
        const char* const dictStart = (const char*)ZSTD_DDict_dictContent(ddict);
        size_t const dictSize = ZSTD_DDict_dictSize(ddict);
        const void* const dictEnd = dictStart + dictSize;
        dctx->ddictIsCold = (dctx->dictEnd != dictEnd);
        DEBUGLOG(4, "DDict is %s",
                    dctx->ddictIsCold ? "~cold~" : "hot!");
    }
    FORWARD_IF_ERROR( ZSTD_decompressBegin(dctx) , "");
    if (ddict) {
        ZSTD_copyDDictParameters(dctx, ddict);
    }
    return 0;
}


unsigned ZSTD_getDictID_fromDict(const void* dict, size_t dictSize)
{
    if (dictSize < 8) return 0;
    if (MEM_readLE32(dict) != ZSTD_MAGIC_DICTIONARY) return 0;
    return MEM_readLE32((const char*)dict + ZSTD_FRAMEIDSIZE);
}


unsigned ZSTD_getDictID_fromFrame(const void* src, size_t srcSize)
{
    ZSTD_FrameHeader zfp = { 0, 0, 0, ZSTD_frame, 0, 0, 0, 0, 0 };
    size_t const hError = ZSTD_getFrameHeader(&zfp, src, srcSize);
    if (ZSTD_isError(hError)) return 0;
    return zfp.dictID;
}


size_t ZSTD_decompress_usingDDict(ZSTD_DCtx* dctx,
                                  void* dst, size_t dstCapacity,
                            const void* src, size_t srcSize,
                            const ZSTD_DDict* ddict)
{

    return ZSTD_decompressMultiFrame(dctx, dst, dstCapacity, src, srcSize,
                                     NULL, 0,
                                     ddict);
}


ZSTD_DStream* ZSTD_createDStream(void)
{
    DEBUGLOG(3, "ZSTD_createDStream");
    return ZSTD_createDCtx_internal(ZSTD_defaultCMem);
}

ZSTD_DStream* ZSTD_initStaticDStream(void *workspace, size_t workspaceSize)
{
    return ZSTD_initStaticDCtx(workspace, workspaceSize);
}

ZSTD_DStream* ZSTD_createDStream_advanced(ZSTD_customMem customMem)
{
    return ZSTD_createDCtx_internal(customMem);
}

size_t ZSTD_freeDStream(ZSTD_DStream* zds)
{
    return ZSTD_freeDCtx(zds);
}


size_t ZSTD_DStreamInSize(void)  { return ZSTD_BLOCKSIZE_MAX + ZSTD_blockHeaderSize; }
size_t ZSTD_DStreamOutSize(void) { return ZSTD_BLOCKSIZE_MAX; }

size_t ZSTD_DCtx_loadDictionary_advanced(ZSTD_DCtx* dctx,
                                   const void* dict, size_t dictSize,
                                         ZSTD_dictLoadMethod_e dictLoadMethod,
                                         ZSTD_dictContentType_e dictContentType)
{
    RETURN_ERROR_IF(dctx->streamStage != zdss_init, stage_wrong, "");
    ZSTD_clearDict(dctx);
    if (dict && dictSize != 0) {
        dctx->ddictLocal = ZSTD_createDDict_advanced(dict, dictSize, dictLoadMethod, dictContentType, dctx->customMem);
        RETURN_ERROR_IF(dctx->ddictLocal == NULL, memory_allocation, "NULL pointer!");
        dctx->ddict = dctx->ddictLocal;
        dctx->dictUses = ZSTD_use_indefinitely;
    }
    return 0;
}

size_t ZSTD_DCtx_loadDictionary_byReference(ZSTD_DCtx* dctx, const void* dict, size_t dictSize)
{
    return ZSTD_DCtx_loadDictionary_advanced(dctx, dict, dictSize, ZSTD_dlm_byRef, ZSTD_dct_auto);
}

size_t ZSTD_DCtx_loadDictionary(ZSTD_DCtx* dctx, const void* dict, size_t dictSize)
{
    return ZSTD_DCtx_loadDictionary_advanced(dctx, dict, dictSize, ZSTD_dlm_byCopy, ZSTD_dct_auto);
}

size_t ZSTD_DCtx_refPrefix_advanced(ZSTD_DCtx* dctx, const void* prefix, size_t prefixSize, ZSTD_dictContentType_e dictContentType)
{
    FORWARD_IF_ERROR(ZSTD_DCtx_loadDictionary_advanced(dctx, prefix, prefixSize, ZSTD_dlm_byRef, dictContentType), "");
    dctx->dictUses = ZSTD_use_once;
    return 0;
}

size_t ZSTD_DCtx_refPrefix(ZSTD_DCtx* dctx, const void* prefix, size_t prefixSize)
{
    return ZSTD_DCtx_refPrefix_advanced(dctx, prefix, prefixSize, ZSTD_dct_rawContent);
}


size_t ZSTD_initDStream_usingDict(ZSTD_DStream* zds, const void* dict, size_t dictSize)
{
    DEBUGLOG(4, "ZSTD_initDStream_usingDict");
    FORWARD_IF_ERROR( ZSTD_DCtx_reset(zds, ZSTD_reset_session_only) , "");
    FORWARD_IF_ERROR( ZSTD_DCtx_loadDictionary(zds, dict, dictSize) , "");
    return ZSTD_startingInputLength(zds->format);
}


size_t ZSTD_initDStream(ZSTD_DStream* zds)
{
    DEBUGLOG(4, "ZSTD_initDStream");
    FORWARD_IF_ERROR(ZSTD_DCtx_reset(zds, ZSTD_reset_session_only), "");
    FORWARD_IF_ERROR(ZSTD_DCtx_refDDict(zds, NULL), "");
    return ZSTD_startingInputLength(zds->format);
}


size_t ZSTD_initDStream_usingDDict(ZSTD_DStream* dctx, const ZSTD_DDict* ddict)
{
    DEBUGLOG(4, "ZSTD_initDStream_usingDDict");
    FORWARD_IF_ERROR( ZSTD_DCtx_reset(dctx, ZSTD_reset_session_only) , "");
    FORWARD_IF_ERROR( ZSTD_DCtx_refDDict(dctx, ddict) , "");
    return ZSTD_startingInputLength(dctx->format);
}


size_t ZSTD_resetDStream(ZSTD_DStream* dctx)
{
    DEBUGLOG(4, "ZSTD_resetDStream");
    FORWARD_IF_ERROR(ZSTD_DCtx_reset(dctx, ZSTD_reset_session_only), "");
    return ZSTD_startingInputLength(dctx->format);
}


size_t ZSTD_DCtx_refDDict(ZSTD_DCtx* dctx, const ZSTD_DDict* ddict)
{
    RETURN_ERROR_IF(dctx->streamStage != zdss_init, stage_wrong, "");
    ZSTD_clearDict(dctx);
    if (ddict) {
        dctx->ddict = ddict;
        dctx->dictUses = ZSTD_use_indefinitely;
        if (dctx->refMultipleDDicts == ZSTD_rmd_refMultipleDDicts) {
            if (dctx->ddictSet == NULL) {
                dctx->ddictSet = ZSTD_createDDictHashSet(dctx->customMem);
                if (!dctx->ddictSet) {
                    RETURN_ERROR(memory_allocation, "Failed to allocate memory for hash set!");
                }
            }
            assert(!dctx->staticSize);
            FORWARD_IF_ERROR(ZSTD_DDictHashSet_addDDict(dctx->ddictSet, ddict, dctx->customMem), "");
        }
    }
    return 0;
}


size_t ZSTD_DCtx_setMaxWindowSize(ZSTD_DCtx* dctx, size_t maxWindowSize)
{
    ZSTD_bounds const bounds = ZSTD_dParam_getBounds(ZSTD_d_windowLogMax);
    size_t const min = (size_t)1 << bounds.lowerBound;
    size_t const max = (size_t)1 << bounds.upperBound;
    RETURN_ERROR_IF(dctx->streamStage != zdss_init, stage_wrong, "");
    RETURN_ERROR_IF(maxWindowSize < min, parameter_outOfBound, "");
    RETURN_ERROR_IF(maxWindowSize > max, parameter_outOfBound, "");
    dctx->maxWindowSize = maxWindowSize;
    return 0;
}

size_t ZSTD_DCtx_setFormat(ZSTD_DCtx* dctx, ZSTD_format_e format)
{
    return ZSTD_DCtx_setParameter(dctx, ZSTD_d_format, (int)format);
}

ZSTD_bounds ZSTD_dParam_getBounds(ZSTD_dParameter dParam)
{
    ZSTD_bounds bounds = { 0, 0, 0 };
    switch(dParam) {
        case ZSTD_d_windowLogMax:
            bounds.lowerBound = ZSTD_WINDOWLOG_ABSOLUTEMIN;
            bounds.upperBound = ZSTD_WINDOWLOG_MAX;
            return bounds;
        case ZSTD_d_format:
            bounds.lowerBound = (int)ZSTD_f_zstd1;
            bounds.upperBound = (int)ZSTD_f_zstd1_magicless;
            ZSTD_STATIC_ASSERT(ZSTD_f_zstd1 < ZSTD_f_zstd1_magicless);
            return bounds;
        case ZSTD_d_stableOutBuffer:
            bounds.lowerBound = (int)ZSTD_bm_buffered;
            bounds.upperBound = (int)ZSTD_bm_stable;
            return bounds;
        case ZSTD_d_forceIgnoreChecksum:
            bounds.lowerBound = (int)ZSTD_d_validateChecksum;
            bounds.upperBound = (int)ZSTD_d_ignoreChecksum;
            return bounds;
        case ZSTD_d_refMultipleDDicts:
            bounds.lowerBound = (int)ZSTD_rmd_refSingleDDict;
            bounds.upperBound = (int)ZSTD_rmd_refMultipleDDicts;
            return bounds;
        case ZSTD_d_disableHuffmanAssembly:
            bounds.lowerBound = 0;
            bounds.upperBound = 1;
            return bounds;
        case ZSTD_d_maxBlockSize:
            bounds.lowerBound = ZSTD_BLOCKSIZE_MAX_MIN;
            bounds.upperBound = ZSTD_BLOCKSIZE_MAX;
            return bounds;

        default:;
    }
    bounds.error = ERROR(parameter_unsupported);
    return bounds;
}


static int ZSTD_dParam_withinBounds(ZSTD_dParameter dParam, int value)
{
    ZSTD_bounds const bounds = ZSTD_dParam_getBounds(dParam);
    if (ZSTD_isError(bounds.error)) return 0;
    if (value < bounds.lowerBound) return 0;
    if (value > bounds.upperBound) return 0;
    return 1;
}

#define CHECK_DBOUNDS(p,v) {                \
    RETURN_ERROR_IF(!ZSTD_dParam_withinBounds(p, v), parameter_outOfBound, ""); \
}

size_t ZSTD_DCtx_getParameter(ZSTD_DCtx* dctx, ZSTD_dParameter param, int* value)
{
    switch (param) {
        case ZSTD_d_windowLogMax:
            *value = (int)ZSTD_highbit32((U32)dctx->maxWindowSize);
            return 0;
        case ZSTD_d_format:
            *value = (int)dctx->format;
            return 0;
        case ZSTD_d_stableOutBuffer:
            *value = (int)dctx->outBufferMode;
            return 0;
        case ZSTD_d_forceIgnoreChecksum:
            *value = (int)dctx->forceIgnoreChecksum;
            return 0;
        case ZSTD_d_refMultipleDDicts:
            *value = (int)dctx->refMultipleDDicts;
            return 0;
        case ZSTD_d_disableHuffmanAssembly:
            *value = (int)dctx->disableHufAsm;
            return 0;
        case ZSTD_d_maxBlockSize:
            *value = dctx->maxBlockSizeParam;
            return 0;
        default:;
    }
    RETURN_ERROR(parameter_unsupported, "");
}

size_t ZSTD_DCtx_setParameter(ZSTD_DCtx* dctx, ZSTD_dParameter dParam, int value)
{
    RETURN_ERROR_IF(dctx->streamStage != zdss_init, stage_wrong, "");
    switch(dParam) {
        case ZSTD_d_windowLogMax:
            if (value == 0) value = ZSTD_WINDOWLOG_LIMIT_DEFAULT;
            CHECK_DBOUNDS(ZSTD_d_windowLogMax, value);
            dctx->maxWindowSize = ((size_t)1) << value;
            return 0;
        case ZSTD_d_format:
            CHECK_DBOUNDS(ZSTD_d_format, value);
            dctx->format = (ZSTD_format_e)value;
            return 0;
        case ZSTD_d_stableOutBuffer:
            CHECK_DBOUNDS(ZSTD_d_stableOutBuffer, value);
            dctx->outBufferMode = (ZSTD_bufferMode_e)value;
            return 0;
        case ZSTD_d_forceIgnoreChecksum:
            CHECK_DBOUNDS(ZSTD_d_forceIgnoreChecksum, value);
            dctx->forceIgnoreChecksum = (ZSTD_forceIgnoreChecksum_e)value;
            return 0;
        case ZSTD_d_refMultipleDDicts:
            CHECK_DBOUNDS(ZSTD_d_refMultipleDDicts, value);
            if (dctx->staticSize != 0) {
                RETURN_ERROR(parameter_unsupported, "Static dctx does not support multiple DDicts!");
            }
            dctx->refMultipleDDicts = (ZSTD_refMultipleDDicts_e)value;
            return 0;
        case ZSTD_d_disableHuffmanAssembly:
            CHECK_DBOUNDS(ZSTD_d_disableHuffmanAssembly, value);
            dctx->disableHufAsm = value != 0;
            return 0;
        case ZSTD_d_maxBlockSize:
            if (value != 0) CHECK_DBOUNDS(ZSTD_d_maxBlockSize, value);
            dctx->maxBlockSizeParam = value;
            return 0;
        default:;
    }
    RETURN_ERROR(parameter_unsupported, "");
}

size_t ZSTD_DCtx_reset(ZSTD_DCtx* dctx, ZSTD_ResetDirective reset)
{
    if ( (reset == ZSTD_reset_session_only)
      || (reset == ZSTD_reset_session_and_parameters) ) {
        dctx->streamStage = zdss_init;
        dctx->noForwardProgress = 0;
        dctx->isFrameDecompression = 1;
    }
    if ( (reset == ZSTD_reset_parameters)
      || (reset == ZSTD_reset_session_and_parameters) ) {
        RETURN_ERROR_IF(dctx->streamStage != zdss_init, stage_wrong, "");
        ZSTD_clearDict(dctx);
        ZSTD_DCtx_resetParameters(dctx);
    }
    return 0;
}


size_t ZSTD_sizeof_DStream(const ZSTD_DStream* dctx)
{
    return ZSTD_sizeof_DCtx(dctx);
}

static size_t ZSTD_decodingBufferSize_internal(unsigned long long windowSize, unsigned long long frameContentSize, size_t blockSizeMax)
{
    size_t const blockSize = MIN((size_t)MIN(windowSize, ZSTD_BLOCKSIZE_MAX), blockSizeMax);

    unsigned long long const neededRBSize = windowSize + (blockSize * 2) + (WILDCOPY_OVERLENGTH * 2);
    unsigned long long const neededSize = MIN(frameContentSize, neededRBSize);
    size_t const minRBSize = (size_t) neededSize;
    RETURN_ERROR_IF((unsigned long long)minRBSize != neededSize,
                    frameParameter_windowTooLarge, "");
    return minRBSize;
}

size_t ZSTD_decodingBufferSize_min(unsigned long long windowSize, unsigned long long frameContentSize)
{
    return ZSTD_decodingBufferSize_internal(windowSize, frameContentSize, ZSTD_BLOCKSIZE_MAX);
}

size_t ZSTD_estimateDStreamSize(size_t windowSize)
{
    size_t const blockSize = MIN(windowSize, ZSTD_BLOCKSIZE_MAX);
    size_t const inBuffSize = blockSize;
    size_t const outBuffSize = ZSTD_decodingBufferSize_min(windowSize, ZSTD_CONTENTSIZE_UNKNOWN);
    return ZSTD_estimateDCtxSize() + inBuffSize + outBuffSize;
}

size_t ZSTD_estimateDStreamSize_fromFrame(const void* src, size_t srcSize)
{
    U32 const windowSizeMax = 1U << ZSTD_WINDOWLOG_MAX;
    ZSTD_FrameHeader zfh;
    size_t const err = ZSTD_getFrameHeader(&zfh, src, srcSize);
    if (ZSTD_isError(err)) return err;
    RETURN_ERROR_IF(err>0, srcSize_wrong, "");
    RETURN_ERROR_IF(zfh.windowSize > windowSizeMax,
                    frameParameter_windowTooLarge, "");
    return ZSTD_estimateDStreamSize((size_t)zfh.windowSize);
}


static int ZSTD_DCtx_isOverflow(ZSTD_DStream* zds, size_t const neededInBuffSize, size_t const neededOutBuffSize)
{
    return (zds->inBuffSize + zds->outBuffSize) >= (neededInBuffSize + neededOutBuffSize) * ZSTD_WORKSPACETOOLARGE_FACTOR;
}

static void ZSTD_DCtx_updateOversizedDuration(ZSTD_DStream* zds, size_t const neededInBuffSize, size_t const neededOutBuffSize)
{
    if (ZSTD_DCtx_isOverflow(zds, neededInBuffSize, neededOutBuffSize))
        zds->oversizedDuration++;
    else
        zds->oversizedDuration = 0;
}

static int ZSTD_DCtx_isOversizedTooLong(ZSTD_DStream* zds)
{
    return zds->oversizedDuration >= ZSTD_WORKSPACETOOLARGE_MAXDURATION;
}


static size_t ZSTD_checkOutBuffer(ZSTD_DStream const* zds, ZSTD_outBuffer const* output)
{
    ZSTD_outBuffer const expect = zds->expectedOutBuffer;

    if (zds->outBufferMode != ZSTD_bm_stable)
        return 0;

    if (zds->streamStage == zdss_init)
        return 0;

    if (expect.dst == output->dst && expect.pos == output->pos && expect.size == output->size)
        return 0;
    RETURN_ERROR(dstBuffer_wrong, "ZSTD_d_stableOutBuffer enabled but output differs!");
}


static size_t ZSTD_decompressContinueStream(
            ZSTD_DStream* zds, char** op, char* oend,
            void const* src, size_t srcSize) {
    int const isSkipFrame = ZSTD_isSkipFrame(zds);
    if (zds->outBufferMode == ZSTD_bm_buffered) {
        size_t const dstSize = isSkipFrame ? 0 : zds->outBuffSize - zds->outStart;
        size_t const decodedSize = ZSTD_decompressContinue(zds,
                zds->outBuff + zds->outStart, dstSize, src, srcSize);
        FORWARD_IF_ERROR(decodedSize, "");
        if (!decodedSize && !isSkipFrame) {
            zds->streamStage = zdss_read;
        } else {
            zds->outEnd = zds->outStart + decodedSize;
            zds->streamStage = zdss_flush;
        }
    } else {

        size_t const dstSize = isSkipFrame ? 0 : (size_t)(oend - *op);
        size_t const decodedSize = ZSTD_decompressContinue(zds, *op, dstSize, src, srcSize);
        FORWARD_IF_ERROR(decodedSize, "");
        *op += decodedSize;

        zds->streamStage = zdss_read;
        assert(*op <= oend);
        assert(zds->outBufferMode == ZSTD_bm_stable);
    }
    return 0;
}

size_t ZSTD_decompressStream(ZSTD_DStream* zds, ZSTD_outBuffer* output, ZSTD_inBuffer* input)
{
    const char* const src = (const char*)input->src;
    const char* const istart = input->pos != 0 ? src + input->pos : src;
    const char* const iend = input->size != 0 ? src + input->size : src;
    const char* ip = istart;
    char* const dst = (char*)output->dst;
    char* const ostart = output->pos != 0 ? dst + output->pos : dst;
    char* const oend = output->size != 0 ? dst + output->size : dst;
    char* op = ostart;
    U32 someMoreWork = 1;

    DEBUGLOG(5, "ZSTD_decompressStream");
    assert(zds != NULL);
    RETURN_ERROR_IF(
        input->pos > input->size,
        srcSize_wrong,
        "forbidden. in: pos: %u   vs size: %u",
        (U32)input->pos, (U32)input->size);
    RETURN_ERROR_IF(
        output->pos > output->size,
        dstSize_tooSmall,
        "forbidden. out: pos: %u   vs size: %u",
        (U32)output->pos, (U32)output->size);
    DEBUGLOG(5, "input size : %u", (U32)(input->size - input->pos));
    FORWARD_IF_ERROR(ZSTD_checkOutBuffer(zds, output), "");

    while (someMoreWork) {
        switch(zds->streamStage)
        {
        case zdss_init :
            DEBUGLOG(5, "stage zdss_init => transparent reset ");
            zds->streamStage = zdss_loadHeader;
            zds->lhSize = zds->inPos = zds->outStart = zds->outEnd = 0;
#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT>=1)
            zds->legacyVersion = 0;
#endif
            zds->hostageByte = 0;
            zds->expectedOutBuffer = *output;
            ZSTD_FALLTHROUGH;

        case zdss_loadHeader :
            DEBUGLOG(5, "stage zdss_loadHeader (srcSize : %u)", (U32)(iend - ip));
#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT>=1)
            if (zds->legacyVersion) {
                RETURN_ERROR_IF(zds->staticSize, memory_allocation,
                    "legacy support is incompatible with static dctx");
                {   size_t const hint = ZSTD_decompressLegacyStream(zds->legacyContext, zds->legacyVersion, output, input);
                    if (hint==0) zds->streamStage = zdss_init;
                    return hint;
            }   }
#endif
            {   size_t const hSize = ZSTD_getFrameHeader_advanced(&zds->fParams, zds->headerBuffer, zds->lhSize, zds->format);
                if (zds->refMultipleDDicts && zds->ddictSet) {
                    ZSTD_DCtx_selectFrameDDict(zds);
                }
                if (ZSTD_isError(hSize)) {
#if defined(ZSTD_LEGACY_SUPPORT) && (ZSTD_LEGACY_SUPPORT>=1)
                    U32 const legacyVersion = ZSTD_isLegacy(istart, iend-istart);
                    if (legacyVersion) {
                        ZSTD_DDict const* const ddict = ZSTD_getDDict(zds);
                        const void* const dict = ddict ? ZSTD_DDict_dictContent(ddict) : NULL;
                        size_t const dictSize = ddict ? ZSTD_DDict_dictSize(ddict) : 0;
                        DEBUGLOG(5, "ZSTD_decompressStream: detected legacy version v0.%u", legacyVersion);
                        RETURN_ERROR_IF(zds->staticSize, memory_allocation,
                            "legacy support is incompatible with static dctx");
                        FORWARD_IF_ERROR(ZSTD_initLegacyStream(&zds->legacyContext,
                                    zds->previousLegacyVersion, legacyVersion,
                                    dict, dictSize), "");
                        zds->legacyVersion = zds->previousLegacyVersion = legacyVersion;
                        {   size_t const hint = ZSTD_decompressLegacyStream(zds->legacyContext, legacyVersion, output, input);
                            if (hint==0) zds->streamStage = zdss_init;
                            return hint;
                    }   }
#endif
                    return hSize;
                }
                if (hSize != 0) {
                    size_t const toLoad = hSize - zds->lhSize;
                    size_t const remainingInput = (size_t)(iend-ip);
                    assert(iend >= ip);
                    if (toLoad > remainingInput) {
                        if (remainingInput > 0) {
                            ZSTD_memcpy(zds->headerBuffer + zds->lhSize, ip, remainingInput);
                            zds->lhSize += remainingInput;
                        }
                        input->pos = input->size;

                        FORWARD_IF_ERROR(
                            ZSTD_getFrameHeader_advanced(&zds->fParams, zds->headerBuffer, zds->lhSize, zds->format),
                            "First few bytes detected incorrect" );

                        return (MAX((size_t)ZSTD_FRAMEHEADERSIZE_MIN(zds->format), hSize) - zds->lhSize) + ZSTD_blockHeaderSize;
                    }
                    assert(ip != NULL);
                    ZSTD_memcpy(zds->headerBuffer + zds->lhSize, ip, toLoad); zds->lhSize = hSize; ip += toLoad;
                    break;
            }   }


            if (zds->fParams.frameContentSize != ZSTD_CONTENTSIZE_UNKNOWN
                && zds->fParams.frameType != ZSTD_skippableFrame
                && (U64)(size_t)(oend-op) >= zds->fParams.frameContentSize) {
                size_t const cSize = ZSTD_findFrameCompressedSize_advanced(istart, (size_t)(iend-istart), zds->format);
                if (cSize <= (size_t)(iend-istart)) {

                    size_t const decompressedSize = ZSTD_decompress_usingDDict(zds, op, (size_t)(oend-op), istart, cSize, ZSTD_getDDict(zds));
                    if (ZSTD_isError(decompressedSize)) return decompressedSize;
                    DEBUGLOG(4, "shortcut to single-pass ZSTD_decompress_usingDDict()");
                    assert(istart != NULL);
                    ip = istart + cSize;
                    op = op ? op + decompressedSize : op;
                    zds->expected = 0;
                    zds->streamStage = zdss_init;
                    someMoreWork = 0;
                    break;
            }   }


            if (zds->outBufferMode == ZSTD_bm_stable
                && zds->fParams.frameType != ZSTD_skippableFrame
                && zds->fParams.frameContentSize != ZSTD_CONTENTSIZE_UNKNOWN
                && (U64)(size_t)(oend-op) < zds->fParams.frameContentSize) {
                RETURN_ERROR(dstSize_tooSmall, "ZSTD_obm_stable passed but ZSTD_outBuffer is too small");
            }


            DEBUGLOG(4, "Consume header");
            FORWARD_IF_ERROR(ZSTD_decompressBegin_usingDDict(zds, ZSTD_getDDict(zds)), "");

            if (zds->format == ZSTD_f_zstd1
                && (MEM_readLE32(zds->headerBuffer) & ZSTD_MAGIC_SKIPPABLE_MASK) == ZSTD_MAGIC_SKIPPABLE_START) {
                zds->expected = MEM_readLE32(zds->headerBuffer + ZSTD_FRAMEIDSIZE);
                zds->stage = ZSTDds_skipFrame;
            } else {
                FORWARD_IF_ERROR(ZSTD_decodeFrameHeader(zds, zds->headerBuffer, zds->lhSize), "");
                zds->expected = ZSTD_blockHeaderSize;
                zds->stage = ZSTDds_decodeBlockHeader;
            }


            DEBUGLOG(4, "Control max memory usage (%u KB <= max %u KB)",
                        (U32)(zds->fParams.windowSize >>10),
                        (U32)(zds->maxWindowSize >> 10) );
            zds->fParams.windowSize = MAX(zds->fParams.windowSize, 1U << ZSTD_WINDOWLOG_ABSOLUTEMIN);
            RETURN_ERROR_IF(zds->fParams.windowSize > zds->maxWindowSize,
                            frameParameter_windowTooLarge, "");
            if (zds->maxBlockSizeParam != 0)
                zds->fParams.blockSizeMax = MIN(zds->fParams.blockSizeMax, (unsigned)zds->maxBlockSizeParam);


            {   size_t const neededInBuffSize = MAX(zds->fParams.blockSizeMax, 4 );
                size_t const neededOutBuffSize = zds->outBufferMode == ZSTD_bm_buffered
                        ? ZSTD_decodingBufferSize_internal(zds->fParams.windowSize, zds->fParams.frameContentSize, zds->fParams.blockSizeMax)
                        : 0;

                ZSTD_DCtx_updateOversizedDuration(zds, neededInBuffSize, neededOutBuffSize);

                {   int const tooSmall = (zds->inBuffSize < neededInBuffSize) || (zds->outBuffSize < neededOutBuffSize);
                    int const tooLarge = ZSTD_DCtx_isOversizedTooLong(zds);

                    if (tooSmall || tooLarge) {
                        size_t const bufferSize = neededInBuffSize + neededOutBuffSize;
                        DEBUGLOG(4, "inBuff  : from %u to %u",
                                    (U32)zds->inBuffSize, (U32)neededInBuffSize);
                        DEBUGLOG(4, "outBuff : from %u to %u",
                                    (U32)zds->outBuffSize, (U32)neededOutBuffSize);
                        if (zds->staticSize) {
                            DEBUGLOG(4, "staticSize : %u", (U32)zds->staticSize);
                            assert(zds->staticSize >= sizeof(ZSTD_DCtx));
                            RETURN_ERROR_IF(
                                bufferSize > zds->staticSize - sizeof(ZSTD_DCtx),
                                memory_allocation, "");
                        } else {
                            ZSTD_customFree(zds->inBuff, zds->customMem);
                            zds->inBuffSize = 0;
                            zds->outBuffSize = 0;
                            zds->inBuff = (char*)ZSTD_customMalloc(bufferSize, zds->customMem);
                            RETURN_ERROR_IF(zds->inBuff == NULL, memory_allocation, "");
                        }
                        zds->inBuffSize = neededInBuffSize;
                        zds->outBuff = zds->inBuff + zds->inBuffSize;
                        zds->outBuffSize = neededOutBuffSize;
            }   }   }
            zds->streamStage = zdss_read;
            ZSTD_FALLTHROUGH;

        case zdss_read:
            DEBUGLOG(5, "stage zdss_read");
            {   size_t const neededInSize = ZSTD_nextSrcSizeToDecompressWithInputSize(zds, (size_t)(iend - ip));
                DEBUGLOG(5, "neededInSize = %u", (U32)neededInSize);
                if (neededInSize==0) {
                    zds->streamStage = zdss_init;
                    someMoreWork = 0;
                    break;
                }
                if ((size_t)(iend-ip) >= neededInSize) {
                    FORWARD_IF_ERROR(ZSTD_decompressContinueStream(zds, &op, oend, ip, neededInSize), "");
                    assert(ip != NULL);
                    ip += neededInSize;

                    break;
            }   }
            if (ip==iend) { someMoreWork = 0; break; }
            zds->streamStage = zdss_load;
            ZSTD_FALLTHROUGH;

        case zdss_load:
            {   size_t const neededInSize = ZSTD_nextSrcSizeToDecompress(zds);
                size_t const toLoad = neededInSize - zds->inPos;
                int const isSkipFrame = ZSTD_isSkipFrame(zds);
                size_t loadedSize;

                assert(neededInSize == ZSTD_nextSrcSizeToDecompressWithInputSize(zds, (size_t)(iend - ip)));
                if (isSkipFrame) {
                    loadedSize = MIN(toLoad, (size_t)(iend-ip));
                } else {
                    RETURN_ERROR_IF(toLoad > zds->inBuffSize - zds->inPos,
                                    corruption_detected,
                                    "should never happen");
                    loadedSize = ZSTD_limitCopy(zds->inBuff + zds->inPos, toLoad, ip, (size_t)(iend-ip));
                }
                if (loadedSize != 0) {

                    ip += loadedSize;
                    zds->inPos += loadedSize;
                }
                if (loadedSize < toLoad) { someMoreWork = 0; break; }


                zds->inPos = 0;
                FORWARD_IF_ERROR(ZSTD_decompressContinueStream(zds, &op, oend, zds->inBuff, neededInSize), "");

                break;
            }
        case zdss_flush:
            {
                size_t const toFlushSize = zds->outEnd - zds->outStart;
                size_t const flushedSize = ZSTD_limitCopy(op, (size_t)(oend-op), zds->outBuff + zds->outStart, toFlushSize);

                op = op ? op + flushedSize : op;

                zds->outStart += flushedSize;
                if (flushedSize == toFlushSize) {
                    zds->streamStage = zdss_read;
                    if ( (zds->outBuffSize < zds->fParams.frameContentSize)
                        && (zds->outStart + zds->fParams.blockSizeMax > zds->outBuffSize) ) {
                        DEBUGLOG(5, "restart filling outBuff from beginning (left:%i, needed:%u)",
                                (int)(zds->outBuffSize - zds->outStart),
                                (U32)zds->fParams.blockSizeMax);
                        zds->outStart = zds->outEnd = 0;
                    }
                    break;
            }   }

            someMoreWork = 0;
            break;

        default:
            assert(0);
            RETURN_ERROR(GENERIC, "impossible to reach");
    }   }


    input->pos = (size_t)(ip - (const char*)(input->src));
    output->pos = (size_t)(op - (char*)(output->dst));


    zds->expectedOutBuffer = *output;

    if ((ip==istart) && (op==ostart)) {
        zds->noForwardProgress ++;
        if (zds->noForwardProgress >= ZSTD_NO_FORWARD_PROGRESS_MAX) {
            RETURN_ERROR_IF(op==oend, noForwardProgress_destFull, "");
            RETURN_ERROR_IF(ip==iend, noForwardProgress_inputEmpty, "");
            assert(0);
        }
    } else {
        zds->noForwardProgress = 0;
    }
    {   size_t nextSrcSizeHint = ZSTD_nextSrcSizeToDecompress(zds);
        if (!nextSrcSizeHint) {
            if (zds->outEnd == zds->outStart) {
                if (zds->hostageByte) {
                    if (input->pos >= input->size) {

                        zds->streamStage = zdss_read;
                        return 1;
                    }
                    input->pos++;
                }
                return 0;
            }
            if (!zds->hostageByte) {
                input->pos--;
                zds->hostageByte=1;
            }
            return 1;
        }
        nextSrcSizeHint += ZSTD_blockHeaderSize * (ZSTD_nextInputType(zds) == ZSTDnit_block);
        assert(zds->inPos <= nextSrcSizeHint);
        nextSrcSizeHint -= zds->inPos;
        return nextSrcSizeHint;
    }
}

size_t ZSTD_decompressStream_simpleArgs (
                            ZSTD_DCtx* dctx,
                            void* dst, size_t dstCapacity, size_t* dstPos,
                      const void* src, size_t srcSize, size_t* srcPos)
{
    ZSTD_outBuffer output;
    ZSTD_inBuffer  input;
    output.dst = dst;
    output.size = dstCapacity;
    output.pos = *dstPos;
    input.src = src;
    input.size = srcSize;
    input.pos = *srcPos;
    {   size_t const cErr = ZSTD_decompressStream(dctx, &output, &input);
        *dstPos = output.pos;
        *srcPos = input.pos;
        return cErr;
    }
}


#include <stddef.h>
#define FSE_STATIC_LINKING_ONLY


#if defined(ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT) && \
    defined(ZSTD_FORCE_DECOMPRESS_SEQUENCES_LONG)
#error "Cannot force the use of the short and the long ZSTD_decompressSequences variants!"
#endif


static void ZSTD_copy4(void* dst, const void* src) { ZSTD_memcpy(dst, src, 4); }


static size_t ZSTD_blockSizeMax(ZSTD_DCtx const* dctx)
{
    size_t const blockSizeMax = dctx->isFrameDecompression ? dctx->fParams.blockSizeMax : ZSTD_BLOCKSIZE_MAX;
    assert(blockSizeMax <= ZSTD_BLOCKSIZE_MAX);
    return blockSizeMax;
}


size_t ZSTD_getcBlockSize(const void* src, size_t srcSize,
                          blockProperties_t* bpPtr)
{
    RETURN_ERROR_IF(srcSize < ZSTD_blockHeaderSize, srcSize_wrong, "");

    {   U32 const cBlockHeader = MEM_readLE24(src);
        U32 const cSize = cBlockHeader >> 3;
        bpPtr->lastBlock = cBlockHeader & 1;
        bpPtr->blockType = (blockType_e)((cBlockHeader >> 1) & 3);
        bpPtr->origSize = cSize;
        if (bpPtr->blockType == bt_rle) return 1;
        RETURN_ERROR_IF(bpPtr->blockType == bt_reserved, corruption_detected, "");
        return cSize;
    }
}


static void ZSTD_allocateLiteralsBuffer(ZSTD_DCtx* dctx, void* const dst, const size_t dstCapacity, const size_t litSize,
    const streaming_operation streaming, const size_t expectedWriteSize, const unsigned splitImmediately)
{
    size_t const blockSizeMax = ZSTD_blockSizeMax(dctx);
    assert(litSize <= blockSizeMax);
    assert(dctx->isFrameDecompression || streaming == not_streaming);
    assert(expectedWriteSize <= blockSizeMax);
    if (streaming == not_streaming && dstCapacity > blockSizeMax + WILDCOPY_OVERLENGTH + litSize + WILDCOPY_OVERLENGTH) {

        dctx->litBuffer = (BYTE*)dst + blockSizeMax + WILDCOPY_OVERLENGTH;
        dctx->litBufferEnd = dctx->litBuffer + litSize;
        dctx->litBufferLocation = ZSTD_in_dst;
    } else if (litSize <= ZSTD_LITBUFFEREXTRASIZE) {

        dctx->litBuffer = dctx->litExtraBuffer;
        dctx->litBufferEnd = dctx->litBuffer + litSize;
        dctx->litBufferLocation = ZSTD_not_in_dst;
    } else {
        assert(blockSizeMax > ZSTD_LITBUFFEREXTRASIZE);

        if (splitImmediately) {

            dctx->litBuffer = (BYTE*)dst + expectedWriteSize - litSize + ZSTD_LITBUFFEREXTRASIZE - WILDCOPY_OVERLENGTH;
            dctx->litBufferEnd = dctx->litBuffer + litSize - ZSTD_LITBUFFEREXTRASIZE;
        } else {

            dctx->litBuffer = (BYTE*)dst + expectedWriteSize - litSize;
            dctx->litBufferEnd = (BYTE*)dst + expectedWriteSize;
        }
        dctx->litBufferLocation = ZSTD_split;
        assert(dctx->litBufferEnd <= (BYTE*)dst + expectedWriteSize);
    }
}


static size_t ZSTD_decodeLiteralsBlock(ZSTD_DCtx* dctx,
                          const void* src, size_t srcSize,
                          void* dst, size_t dstCapacity, const streaming_operation streaming)
{
    DEBUGLOG(5, "ZSTD_decodeLiteralsBlock");
    RETURN_ERROR_IF(srcSize < MIN_CBLOCK_SIZE, corruption_detected, "");

    {   const BYTE* const istart = (const BYTE*) src;
        SymbolEncodingType_e const litEncType = (SymbolEncodingType_e)(istart[0] & 3);
        size_t const blockSizeMax = ZSTD_blockSizeMax(dctx);

        switch(litEncType)
        {
        case set_repeat:
            DEBUGLOG(5, "set_repeat flag : re-using stats from previous compressed literals block");
            RETURN_ERROR_IF(dctx->litEntropy==0, dictionary_corrupted, "");
            ZSTD_FALLTHROUGH;

        case set_compressed:
            RETURN_ERROR_IF(srcSize < 5, corruption_detected, "srcSize >= MIN_CBLOCK_SIZE == 2; here we need up to 5 for case 3");
            {   size_t lhSize, litSize, litCSize;
                U32 singleStream=0;
                U32 const lhlCode = (istart[0] >> 2) & 3;
                U32 const lhc = MEM_readLE32(istart);
                size_t hufSuccess;
                size_t expectedWriteSize = MIN(blockSizeMax, dstCapacity);
                int const flags = 0
                    | (ZSTD_DCtx_get_bmi2(dctx) ? HUF_flags_bmi2 : 0)
                    | (dctx->disableHufAsm ? HUF_flags_disableAsm : 0);
                switch(lhlCode)
                {
                case 0: case 1: default:

                    singleStream = !lhlCode;
                    lhSize = 3;
                    litSize  = (lhc >> 4) & 0x3FF;
                    litCSize = (lhc >> 14) & 0x3FF;
                    break;
                case 2:

                    lhSize = 4;
                    litSize  = (lhc >> 4) & 0x3FFF;
                    litCSize = lhc >> 18;
                    break;
                case 3:

                    lhSize = 5;
                    litSize  = (lhc >> 4) & 0x3FFFF;
                    litCSize = (lhc >> 22) + ((size_t)istart[4] << 10);
                    break;
                }
                RETURN_ERROR_IF(litSize > 0 && dst == NULL, dstSize_tooSmall, "NULL not handled");
                RETURN_ERROR_IF(litSize > blockSizeMax, corruption_detected, "");
                if (!singleStream)
                    RETURN_ERROR_IF(litSize < MIN_LITERALS_FOR_4_STREAMS, literals_headerWrong,
                        "Not enough literals (%zu) for the 4-streams mode (min %u)",
                        litSize, MIN_LITERALS_FOR_4_STREAMS);
                RETURN_ERROR_IF(litCSize + lhSize > srcSize, corruption_detected, "");
                RETURN_ERROR_IF(expectedWriteSize < litSize , dstSize_tooSmall, "");
                ZSTD_allocateLiteralsBuffer(dctx, dst, dstCapacity, litSize, streaming, expectedWriteSize, 0);


                if (dctx->ddictIsCold && (litSize > 768 )) {
                    PREFETCH_AREA(dctx->HUFptr, sizeof(dctx->entropy.hufTable));
                }

                if (litEncType==set_repeat) {
                    if (singleStream) {
                        hufSuccess = HUF_decompress1X_usingDTable(
                            dctx->litBuffer, litSize, istart+lhSize, litCSize,
                            dctx->HUFptr, flags);
                    } else {
                        assert(litSize >= MIN_LITERALS_FOR_4_STREAMS);
                        hufSuccess = HUF_decompress4X_usingDTable(
                            dctx->litBuffer, litSize, istart+lhSize, litCSize,
                            dctx->HUFptr, flags);
                    }
                } else {
                    if (singleStream) {
#if defined(HUF_FORCE_DECOMPRESS_X2)
                        hufSuccess = HUF_decompress1X_DCtx_wksp(
                            dctx->entropy.hufTable, dctx->litBuffer, litSize,
                            istart+lhSize, litCSize, dctx->workspace,
                            sizeof(dctx->workspace), flags);
#else
                        hufSuccess = HUF_decompress1X1_DCtx_wksp(
                            dctx->entropy.hufTable, dctx->litBuffer, litSize,
                            istart+lhSize, litCSize, dctx->workspace,
                            sizeof(dctx->workspace), flags);
#endif
                    } else {
                        hufSuccess = HUF_decompress4X_hufOnly_wksp(
                            dctx->entropy.hufTable, dctx->litBuffer, litSize,
                            istart+lhSize, litCSize, dctx->workspace,
                            sizeof(dctx->workspace), flags);
                    }
                }
                if (dctx->litBufferLocation == ZSTD_split)
                {
                    assert(litSize > ZSTD_LITBUFFEREXTRASIZE);
                    ZSTD_memcpy(dctx->litExtraBuffer, dctx->litBufferEnd - ZSTD_LITBUFFEREXTRASIZE, ZSTD_LITBUFFEREXTRASIZE);
                    ZSTD_memmove(dctx->litBuffer + ZSTD_LITBUFFEREXTRASIZE - WILDCOPY_OVERLENGTH, dctx->litBuffer, litSize - ZSTD_LITBUFFEREXTRASIZE);
                    dctx->litBuffer += ZSTD_LITBUFFEREXTRASIZE - WILDCOPY_OVERLENGTH;
                    dctx->litBufferEnd -= WILDCOPY_OVERLENGTH;
                    assert(dctx->litBufferEnd <= (BYTE*)dst + blockSizeMax);
                }

                RETURN_ERROR_IF(HUF_isError(hufSuccess), corruption_detected, "");

                dctx->litPtr = dctx->litBuffer;
                dctx->litSize = litSize;
                dctx->litEntropy = 1;
                if (litEncType==set_compressed) dctx->HUFptr = dctx->entropy.hufTable;
                return litCSize + lhSize;
            }

        case set_basic:
            {   size_t litSize, lhSize;
                U32 const lhlCode = ((istart[0]) >> 2) & 3;
                size_t expectedWriteSize = MIN(blockSizeMax, dstCapacity);
                switch(lhlCode)
                {
                case 0: case 2: default:
                    lhSize = 1;
                    litSize = istart[0] >> 3;
                    break;
                case 1:
                    lhSize = 2;
                    litSize = MEM_readLE16(istart) >> 4;
                    break;
                case 3:
                    lhSize = 3;
                    RETURN_ERROR_IF(srcSize<3, corruption_detected, "srcSize >= MIN_CBLOCK_SIZE == 2; here we need lhSize = 3");
                    litSize = MEM_readLE24(istart) >> 4;
                    break;
                }

                RETURN_ERROR_IF(litSize > 0 && dst == NULL, dstSize_tooSmall, "NULL not handled");
                RETURN_ERROR_IF(litSize > blockSizeMax, corruption_detected, "");
                RETURN_ERROR_IF(expectedWriteSize < litSize, dstSize_tooSmall, "");
                ZSTD_allocateLiteralsBuffer(dctx, dst, dstCapacity, litSize, streaming, expectedWriteSize, 1);
                if (lhSize+litSize+WILDCOPY_OVERLENGTH > srcSize) {
                    RETURN_ERROR_IF(litSize+lhSize > srcSize, corruption_detected, "");
                    if (dctx->litBufferLocation == ZSTD_split)
                    {
                        ZSTD_memcpy(dctx->litBuffer, istart + lhSize, litSize - ZSTD_LITBUFFEREXTRASIZE);
                        ZSTD_memcpy(dctx->litExtraBuffer, istart + lhSize + litSize - ZSTD_LITBUFFEREXTRASIZE, ZSTD_LITBUFFEREXTRASIZE);
                    }
                    else
                    {
                        ZSTD_memcpy(dctx->litBuffer, istart + lhSize, litSize);
                    }
                    dctx->litPtr = dctx->litBuffer;
                    dctx->litSize = litSize;
                    return lhSize+litSize;
                }

                dctx->litPtr = istart+lhSize;
                dctx->litSize = litSize;
                dctx->litBufferEnd = dctx->litPtr + litSize;
                dctx->litBufferLocation = ZSTD_not_in_dst;
                return lhSize+litSize;
            }

        case set_rle:
            {   U32 const lhlCode = ((istart[0]) >> 2) & 3;
                size_t litSize, lhSize;
                size_t expectedWriteSize = MIN(blockSizeMax, dstCapacity);
                switch(lhlCode)
                {
                case 0: case 2: default:
                    lhSize = 1;
                    litSize = istart[0] >> 3;
                    break;
                case 1:
                    lhSize = 2;
                    RETURN_ERROR_IF(srcSize<3, corruption_detected, "srcSize >= MIN_CBLOCK_SIZE == 2; here we need lhSize+1 = 3");
                    litSize = MEM_readLE16(istart) >> 4;
                    break;
                case 3:
                    lhSize = 3;
                    RETURN_ERROR_IF(srcSize<4, corruption_detected, "srcSize >= MIN_CBLOCK_SIZE == 2; here we need lhSize+1 = 4");
                    litSize = MEM_readLE24(istart) >> 4;
                    break;
                }
                RETURN_ERROR_IF(litSize > 0 && dst == NULL, dstSize_tooSmall, "NULL not handled");
                RETURN_ERROR_IF(litSize > blockSizeMax, corruption_detected, "");
                RETURN_ERROR_IF(expectedWriteSize < litSize, dstSize_tooSmall, "");
                ZSTD_allocateLiteralsBuffer(dctx, dst, dstCapacity, litSize, streaming, expectedWriteSize, 1);
                if (dctx->litBufferLocation == ZSTD_split)
                {
                    ZSTD_memset(dctx->litBuffer, istart[lhSize], litSize - ZSTD_LITBUFFEREXTRASIZE);
                    ZSTD_memset(dctx->litExtraBuffer, istart[lhSize], ZSTD_LITBUFFEREXTRASIZE);
                }
                else
                {
                    ZSTD_memset(dctx->litBuffer, istart[lhSize], litSize);
                }
                dctx->litPtr = dctx->litBuffer;
                dctx->litSize = litSize;
                return lhSize+1;
            }
        default:
            RETURN_ERROR(corruption_detected, "impossible");
        }
    }
}


size_t ZSTD_decodeLiteralsBlock_wrapper(ZSTD_DCtx* dctx,
                          const void* src, size_t srcSize,
                          void* dst, size_t dstCapacity);
size_t ZSTD_decodeLiteralsBlock_wrapper(ZSTD_DCtx* dctx,
                          const void* src, size_t srcSize,
                          void* dst, size_t dstCapacity)
{
    dctx->isFrameDecompression = 0;
    return ZSTD_decodeLiteralsBlock(dctx, src, srcSize, dst, dstCapacity, not_streaming);
}


static const ZSTD_seqSymbol LL_defaultDTable[(1<<LL_DEFAULTNORMLOG)+1] = {
     {  1,  1,  1, LL_DEFAULTNORMLOG},

     {  0,  0,  4,    0},  { 16,  0,  4,    0},
     { 32,  0,  5,    1},  {  0,  0,  5,    3},
     {  0,  0,  5,    4},  {  0,  0,  5,    6},
     {  0,  0,  5,    7},  {  0,  0,  5,    9},
     {  0,  0,  5,   10},  {  0,  0,  5,   12},
     {  0,  0,  6,   14},  {  0,  1,  5,   16},
     {  0,  1,  5,   20},  {  0,  1,  5,   22},
     {  0,  2,  5,   28},  {  0,  3,  5,   32},
     {  0,  4,  5,   48},  { 32,  6,  5,   64},
     {  0,  7,  5,  128},  {  0,  8,  6,  256},
     {  0, 10,  6, 1024},  {  0, 12,  6, 4096},
     { 32,  0,  4,    0},  {  0,  0,  4,    1},
     {  0,  0,  5,    2},  { 32,  0,  5,    4},
     {  0,  0,  5,    5},  { 32,  0,  5,    7},
     {  0,  0,  5,    8},  { 32,  0,  5,   10},
     {  0,  0,  5,   11},  {  0,  0,  6,   13},
     { 32,  1,  5,   16},  {  0,  1,  5,   18},
     { 32,  1,  5,   22},  {  0,  2,  5,   24},
     { 32,  3,  5,   32},  {  0,  3,  5,   40},
     {  0,  6,  4,   64},  { 16,  6,  4,   64},
     { 32,  7,  5,  128},  {  0,  9,  6,  512},
     {  0, 11,  6, 2048},  { 48,  0,  4,    0},
     { 16,  0,  4,    1},  { 32,  0,  5,    2},
     { 32,  0,  5,    3},  { 32,  0,  5,    5},
     { 32,  0,  5,    6},  { 32,  0,  5,    8},
     { 32,  0,  5,    9},  { 32,  0,  5,   11},
     { 32,  0,  5,   12},  {  0,  0,  6,   15},
     { 32,  1,  5,   18},  { 32,  1,  5,   20},
     { 32,  2,  5,   24},  { 32,  2,  5,   28},
     { 32,  3,  5,   40},  { 32,  4,  5,   48},
     {  0, 16,  6,65536},  {  0, 15,  6,32768},
     {  0, 14,  6,16384},  {  0, 13,  6, 8192},
};


static const ZSTD_seqSymbol OF_defaultDTable[(1<<OF_DEFAULTNORMLOG)+1] = {
    {  1,  1,  1, OF_DEFAULTNORMLOG},

    {  0,  0,  5,    0},     {  0,  6,  4,   61},
    {  0,  9,  5,  509},     {  0, 15,  5,32765},
    {  0, 21,  5,2097149},   {  0,  3,  5,    5},
    {  0,  7,  4,  125},     {  0, 12,  5, 4093},
    {  0, 18,  5,262141},    {  0, 23,  5,8388605},
    {  0,  5,  5,   29},     {  0,  8,  4,  253},
    {  0, 14,  5,16381},     {  0, 20,  5,1048573},
    {  0,  2,  5,    1},     { 16,  7,  4,  125},
    {  0, 11,  5, 2045},     {  0, 17,  5,131069},
    {  0, 22,  5,4194301},   {  0,  4,  5,   13},
    { 16,  8,  4,  253},     {  0, 13,  5, 8189},
    {  0, 19,  5,524285},    {  0,  1,  5,    1},
    { 16,  6,  4,   61},     {  0, 10,  5, 1021},
    {  0, 16,  5,65533},     {  0, 28,  5,268435453},
    {  0, 27,  5,134217725}, {  0, 26,  5,67108861},
    {  0, 25,  5,33554429},  {  0, 24,  5,16777213},
};


static const ZSTD_seqSymbol ML_defaultDTable[(1<<ML_DEFAULTNORMLOG)+1] = {
    {  1,  1,  1, ML_DEFAULTNORMLOG},

    {  0,  0,  6,    3},  {  0,  0,  4,    4},
    { 32,  0,  5,    5},  {  0,  0,  5,    6},
    {  0,  0,  5,    8},  {  0,  0,  5,    9},
    {  0,  0,  5,   11},  {  0,  0,  6,   13},
    {  0,  0,  6,   16},  {  0,  0,  6,   19},
    {  0,  0,  6,   22},  {  0,  0,  6,   25},
    {  0,  0,  6,   28},  {  0,  0,  6,   31},
    {  0,  0,  6,   34},  {  0,  1,  6,   37},
    {  0,  1,  6,   41},  {  0,  2,  6,   47},
    {  0,  3,  6,   59},  {  0,  4,  6,   83},
    {  0,  7,  6,  131},  {  0,  9,  6,  515},
    { 16,  0,  4,    4},  {  0,  0,  4,    5},
    { 32,  0,  5,    6},  {  0,  0,  5,    7},
    { 32,  0,  5,    9},  {  0,  0,  5,   10},
    {  0,  0,  6,   12},  {  0,  0,  6,   15},
    {  0,  0,  6,   18},  {  0,  0,  6,   21},
    {  0,  0,  6,   24},  {  0,  0,  6,   27},
    {  0,  0,  6,   30},  {  0,  0,  6,   33},
    {  0,  1,  6,   35},  {  0,  1,  6,   39},
    {  0,  2,  6,   43},  {  0,  3,  6,   51},
    {  0,  4,  6,   67},  {  0,  5,  6,   99},
    {  0,  8,  6,  259},  { 32,  0,  4,    4},
    { 48,  0,  4,    4},  { 16,  0,  4,    5},
    { 32,  0,  5,    7},  { 32,  0,  5,    8},
    { 32,  0,  5,   10},  { 32,  0,  5,   11},
    {  0,  0,  6,   14},  {  0,  0,  6,   17},
    {  0,  0,  6,   20},  {  0,  0,  6,   23},
    {  0,  0,  6,   26},  {  0,  0,  6,   29},
    {  0,  0,  6,   32},  {  0, 16,  6,65539},
    {  0, 15,  6,32771},  {  0, 14,  6,16387},
    {  0, 13,  6, 8195},  {  0, 12,  6, 4099},
    {  0, 11,  6, 2051},  {  0, 10,  6, 1027},
};


static void ZSTD_buildSeqTable_rle(ZSTD_seqSymbol* dt, U32 baseValue, U8 nbAddBits)
{
    void* ptr = dt;
    ZSTD_seqSymbol_header* const DTableH = (ZSTD_seqSymbol_header*)ptr;
    ZSTD_seqSymbol* const cell = dt + 1;

    DTableH->tableLog = 0;
    DTableH->fastMode = 0;

    cell->nbBits = 0;
    cell->nextState = 0;
    assert(nbAddBits < 255);
    cell->nbAdditionalBits = nbAddBits;
    cell->baseValue = baseValue;
}


FORCE_INLINE_TEMPLATE
void ZSTD_buildFSETable_body(ZSTD_seqSymbol* dt,
            const short* normalizedCounter, unsigned maxSymbolValue,
            const U32* baseValue, const U8* nbAdditionalBits,
            unsigned tableLog, void* wksp, size_t wkspSize)
{
    ZSTD_seqSymbol* const tableDecode = dt+1;
    U32 const maxSV1 = maxSymbolValue + 1;
    U32 const tableSize = 1 << tableLog;

    U16* symbolNext = (U16*)wksp;
    BYTE* spread = (BYTE*)(symbolNext + MaxSeq + 1);
    U32 highThreshold = tableSize - 1;


    assert(maxSymbolValue <= MaxSeq);
    assert(tableLog <= MaxFSELog);
    assert(wkspSize >= ZSTD_BUILD_FSE_TABLE_WKSP_SIZE);
    (void)wkspSize;

    {   ZSTD_seqSymbol_header DTableH;
        DTableH.tableLog = tableLog;
        DTableH.fastMode = 1;
        {   S16 const largeLimit= (S16)(1 << (tableLog-1));
            U32 s;
            for (s=0; s<maxSV1; s++) {
                if (normalizedCounter[s]==-1) {
                    tableDecode[highThreshold--].baseValue = s;
                    symbolNext[s] = 1;
                } else {
                    if (normalizedCounter[s] >= largeLimit) DTableH.fastMode=0;
                    assert(normalizedCounter[s]>=0);
                    symbolNext[s] = (U16)normalizedCounter[s];
        }   }   }
        ZSTD_memcpy(dt, &DTableH, sizeof(DTableH));
    }


    assert(tableSize <= 512);

    if (highThreshold == tableSize - 1) {
        size_t const tableMask = tableSize-1;
        size_t const step = FSE_TABLESTEP(tableSize);

        {
            U64 const add = 0x0101010101010101ull;
            size_t pos = 0;
            U64 sv = 0;
            U32 s;
            for (s=0; s<maxSV1; ++s, sv += add) {
                int i;
                int const n = normalizedCounter[s];
                MEM_write64(spread + pos, sv);
                for (i = 8; i < n; i += 8) {
                    MEM_write64(spread + pos + i, sv);
                }
                assert(n>=0);
                pos += (size_t)n;
            }
        }

        {
            size_t position = 0;
            size_t s;
            size_t const unroll = 2;
            assert(tableSize % unroll == 0);
            for (s = 0; s < (size_t)tableSize; s += unroll) {
                size_t u;
                for (u = 0; u < unroll; ++u) {
                    size_t const uPosition = (position + (u * step)) & tableMask;
                    tableDecode[uPosition].baseValue = spread[s + u];
                }
                position = (position + (unroll * step)) & tableMask;
            }
            assert(position == 0);
        }
    } else {
        U32 const tableMask = tableSize-1;
        U32 const step = FSE_TABLESTEP(tableSize);
        U32 s, position = 0;
        for (s=0; s<maxSV1; s++) {
            int i;
            int const n = normalizedCounter[s];
            for (i=0; i<n; i++) {
                tableDecode[position].baseValue = s;
                position = (position + step) & tableMask;
                while (UNLIKELY(position > highThreshold)) position = (position + step) & tableMask;
        }   }
        assert(position == 0);
    }


    {
        U32 u;
        for (u=0; u<tableSize; u++) {
            U32 const symbol = tableDecode[u].baseValue;
            U32 const nextState = symbolNext[symbol]++;
            tableDecode[u].nbBits = (BYTE) (tableLog - ZSTD_highbit32(nextState) );
            tableDecode[u].nextState = (U16) ( (nextState << tableDecode[u].nbBits) - tableSize);
            assert(nbAdditionalBits[symbol] < 255);
            tableDecode[u].nbAdditionalBits = nbAdditionalBits[symbol];
            tableDecode[u].baseValue = baseValue[symbol];
        }
    }
}


static void ZSTD_buildFSETable_body_default(ZSTD_seqSymbol* dt,
            const short* normalizedCounter, unsigned maxSymbolValue,
            const U32* baseValue, const U8* nbAdditionalBits,
            unsigned tableLog, void* wksp, size_t wkspSize)
{
    ZSTD_buildFSETable_body(dt, normalizedCounter, maxSymbolValue,
            baseValue, nbAdditionalBits, tableLog, wksp, wkspSize);
}

#if DYNAMIC_BMI2
BMI2_TARGET_ATTRIBUTE static void ZSTD_buildFSETable_body_bmi2(ZSTD_seqSymbol* dt,
            const short* normalizedCounter, unsigned maxSymbolValue,
            const U32* baseValue, const U8* nbAdditionalBits,
            unsigned tableLog, void* wksp, size_t wkspSize)
{
    ZSTD_buildFSETable_body(dt, normalizedCounter, maxSymbolValue,
            baseValue, nbAdditionalBits, tableLog, wksp, wkspSize);
}
#endif

void ZSTD_buildFSETable(ZSTD_seqSymbol* dt,
            const short* normalizedCounter, unsigned maxSymbolValue,
            const U32* baseValue, const U8* nbAdditionalBits,
            unsigned tableLog, void* wksp, size_t wkspSize, int bmi2)
{
#if DYNAMIC_BMI2
    if (bmi2) {
        ZSTD_buildFSETable_body_bmi2(dt, normalizedCounter, maxSymbolValue,
                baseValue, nbAdditionalBits, tableLog, wksp, wkspSize);
        return;
    }
#endif
    (void)bmi2;
    ZSTD_buildFSETable_body_default(dt, normalizedCounter, maxSymbolValue,
            baseValue, nbAdditionalBits, tableLog, wksp, wkspSize);
}


static size_t ZSTD_buildSeqTable(ZSTD_seqSymbol* DTableSpace, const ZSTD_seqSymbol** DTablePtr,
                                 SymbolEncodingType_e type, unsigned max, U32 maxLog,
                                 const void* src, size_t srcSize,
                                 const U32* baseValue, const U8* nbAdditionalBits,
                                 const ZSTD_seqSymbol* defaultTable, U32 flagRepeatTable,
                                 int ddictIsCold, int nbSeq, U32* wksp, size_t wkspSize,
                                 int bmi2)
{
    switch(type)
    {
    case set_rle :
        RETURN_ERROR_IF(!srcSize, srcSize_wrong, "");
        RETURN_ERROR_IF((*(const BYTE*)src) > max, corruption_detected, "");
        {   U32 const symbol = *(const BYTE*)src;
            U32 const baseline = baseValue[symbol];
            U8 const nbBits = nbAdditionalBits[symbol];
            ZSTD_buildSeqTable_rle(DTableSpace, baseline, nbBits);
        }
        *DTablePtr = DTableSpace;
        return 1;
    case set_basic :
        *DTablePtr = defaultTable;
        return 0;
    case set_repeat:
        RETURN_ERROR_IF(!flagRepeatTable, corruption_detected, "");

        if (ddictIsCold && (nbSeq > 24 )) {
            const void* const pStart = *DTablePtr;
            size_t const pSize = sizeof(ZSTD_seqSymbol) * (SEQSYMBOL_TABLE_SIZE(maxLog));
            PREFETCH_AREA(pStart, pSize);
        }
        return 0;
    case set_compressed :
        {   unsigned tableLog;
            S16 norm[MaxSeq+1];
            size_t const headerSize = FSE_readNCount(norm, &max, &tableLog, src, srcSize);
            RETURN_ERROR_IF(FSE_isError(headerSize), corruption_detected, "");
            RETURN_ERROR_IF(tableLog > maxLog, corruption_detected, "");
            ZSTD_buildFSETable(DTableSpace, norm, max, baseValue, nbAdditionalBits, tableLog, wksp, wkspSize, bmi2);
            *DTablePtr = DTableSpace;
            return headerSize;
        }
    default :
        assert(0);
        RETURN_ERROR(GENERIC, "impossible");
    }
}

size_t ZSTD_decodeSeqHeaders(ZSTD_DCtx* dctx, int* nbSeqPtr,
                             const void* src, size_t srcSize)
{
    const BYTE* const istart = (const BYTE*)src;
    const BYTE* const iend = istart + srcSize;
    const BYTE* ip = istart;
    int nbSeq;
    DEBUGLOG(5, "ZSTD_decodeSeqHeaders");


    RETURN_ERROR_IF(srcSize < MIN_SEQUENCES_SIZE, srcSize_wrong, "");


    nbSeq = *ip++;
    if (nbSeq > 0x7F) {
        if (nbSeq == 0xFF) {
            RETURN_ERROR_IF(ip+2 > iend, srcSize_wrong, "");
            nbSeq = MEM_readLE16(ip) + LONGNBSEQ;
            ip+=2;
        } else {
            RETURN_ERROR_IF(ip >= iend, srcSize_wrong, "");
            nbSeq = ((nbSeq-0x80)<<8) + *ip++;
        }
    }
    *nbSeqPtr = nbSeq;

    if (nbSeq == 0) {

        RETURN_ERROR_IF(ip != iend, corruption_detected,
            "extraneous data present in the Sequences section");
        return (size_t)(ip - istart);
    }


    RETURN_ERROR_IF(ip+1 > iend, srcSize_wrong, "");
    RETURN_ERROR_IF(*ip & 3, corruption_detected, "");
    {   SymbolEncodingType_e const LLtype = (SymbolEncodingType_e)(*ip >> 6);
        SymbolEncodingType_e const OFtype = (SymbolEncodingType_e)((*ip >> 4) & 3);
        SymbolEncodingType_e const MLtype = (SymbolEncodingType_e)((*ip >> 2) & 3);
        ip++;


        assert(ip <= iend);
        {   size_t const llhSize = ZSTD_buildSeqTable(dctx->entropy.LLTable, &dctx->LLTptr,
                                                      LLtype, MaxLL, LLFSELog,
                                                      ip, (size_t)(iend-ip),
                                                      LL_base, LL_bits,
                                                      LL_defaultDTable, dctx->fseEntropy,
                                                      dctx->ddictIsCold, nbSeq,
                                                      dctx->workspace, sizeof(dctx->workspace),
                                                      ZSTD_DCtx_get_bmi2(dctx));
            RETURN_ERROR_IF(ZSTD_isError(llhSize), corruption_detected, "ZSTD_buildSeqTable failed");
            ip += llhSize;
        }

        assert(ip <= iend);
        {   size_t const ofhSize = ZSTD_buildSeqTable(dctx->entropy.OFTable, &dctx->OFTptr,
                                                      OFtype, MaxOff, OffFSELog,
                                                      ip, (size_t)(iend-ip),
                                                      OF_base, OF_bits,
                                                      OF_defaultDTable, dctx->fseEntropy,
                                                      dctx->ddictIsCold, nbSeq,
                                                      dctx->workspace, sizeof(dctx->workspace),
                                                      ZSTD_DCtx_get_bmi2(dctx));
            RETURN_ERROR_IF(ZSTD_isError(ofhSize), corruption_detected, "ZSTD_buildSeqTable failed");
            ip += ofhSize;
        }

        assert(ip <= iend);
        {   size_t const mlhSize = ZSTD_buildSeqTable(dctx->entropy.MLTable, &dctx->MLTptr,
                                                      MLtype, MaxML, MLFSELog,
                                                      ip, (size_t)(iend-ip),
                                                      ML_base, ML_bits,
                                                      ML_defaultDTable, dctx->fseEntropy,
                                                      dctx->ddictIsCold, nbSeq,
                                                      dctx->workspace, sizeof(dctx->workspace),
                                                      ZSTD_DCtx_get_bmi2(dctx));
            RETURN_ERROR_IF(ZSTD_isError(mlhSize), corruption_detected, "ZSTD_buildSeqTable failed");
            ip += mlhSize;
        }
    }

    return (size_t)(ip-istart);
}


typedef struct {
    size_t litLength;
    size_t matchLength;
    size_t offset;
} seq_t;

typedef struct {
    size_t state;
    const ZSTD_seqSymbol* table;
} ZSTD_fseState;

typedef struct {
    BIT_DStream_t DStream;
    ZSTD_fseState stateLL;
    ZSTD_fseState stateOffb;
    ZSTD_fseState stateML;
    size_t prevOffset[ZSTD_REP_NUM];
} seqState_t;


HINT_INLINE void ZSTD_overlapCopy8(BYTE** op, BYTE const** ip, size_t offset)
{
    assert(*ip <= *op);
    if (offset < 8) {

        static const U32 dec32table[] = { 0, 1, 2, 1, 4, 4, 4, 4 };
        static const int dec64table[] = { 8, 8, 8, 7, 8, 9,10,11 };
        int const sub2 = dec64table[offset];
        (*op)[0] = (*ip)[0];
        (*op)[1] = (*ip)[1];
        (*op)[2] = (*ip)[2];
        (*op)[3] = (*ip)[3];
        *ip += dec32table[offset];
        ZSTD_copy4(*op+4, *ip);
        *ip -= sub2;
    } else {
        ZSTD_copy8(*op, *ip);
    }
    *ip += 8;
    *op += 8;
    assert(*op - *ip >= 8);
}


static void
ZSTD_safecopy(BYTE* op, const BYTE* const oend_w, BYTE const* ip, size_t length, ZSTD_overlap_e ovtype)
{
    ptrdiff_t const diff = op - ip;
    BYTE* const oend = op + length;

    assert((ovtype == ZSTD_no_overlap && (diff <= -8 || diff >= 8 || op >= oend_w)) ||
           (ovtype == ZSTD_overlap_src_before_dst && diff >= 0));

    if (length < 8) {

        while (op < oend) *op++ = *ip++;
        return;
    }
    if (ovtype == ZSTD_overlap_src_before_dst) {

        assert(length >= 8);
        assert(diff > 0);
        ZSTD_overlapCopy8(&op, &ip, (size_t)diff);
        length -= 8;
        assert(op - ip >= 8);
        assert(op <= oend);
    }

    if (oend <= oend_w) {

        ZSTD_wildcopy(op, ip, length, ovtype);
        return;
    }
    if (op <= oend_w) {

        assert(oend > oend_w);
        ZSTD_wildcopy(op, ip, (size_t)(oend_w - op), ovtype);
        ip += oend_w - op;
        op += oend_w - op;
    }

    while (op < oend) *op++ = *ip++;
}


static void ZSTD_safecopyDstBeforeSrc(BYTE* op, const BYTE* ip, size_t length)
{
    ptrdiff_t const diff = op - ip;
    BYTE* const oend = op + length;

    if (length < 8 || diff > -8) {

        while (op < oend) *op++ = *ip++;
        return;
    }

    if (op <= oend - WILDCOPY_OVERLENGTH && diff < -WILDCOPY_VECLEN) {
        ZSTD_wildcopy(op, ip, (size_t)(oend - WILDCOPY_OVERLENGTH - op), ZSTD_no_overlap);
        ip += oend - WILDCOPY_OVERLENGTH - op;
        op += oend - WILDCOPY_OVERLENGTH - op;
    }


    while (op < oend) *op++ = *ip++;
}


FORCE_NOINLINE
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
size_t ZSTD_execSequenceEnd(BYTE* op,
    BYTE* const oend, seq_t sequence,
    const BYTE** litPtr, const BYTE* const litLimit,
    const BYTE* const prefixStart, const BYTE* const virtualStart, const BYTE* const dictEnd)
{
    BYTE* const oLitEnd = op + sequence.litLength;
    size_t const sequenceLength = sequence.litLength + sequence.matchLength;
    const BYTE* const iLitEnd = *litPtr + sequence.litLength;
    const BYTE* match = oLitEnd - sequence.offset;
    BYTE* const oend_w = oend - WILDCOPY_OVERLENGTH;


    RETURN_ERROR_IF(sequenceLength > (size_t)(oend - op), dstSize_tooSmall, "last match must fit within dstBuffer");
    RETURN_ERROR_IF(sequence.litLength > (size_t)(litLimit - *litPtr), corruption_detected, "try to read beyond literal buffer");
    assert(op < op + sequenceLength);
    assert(oLitEnd < op + sequenceLength);


    ZSTD_safecopy(op, oend_w, *litPtr, sequence.litLength, ZSTD_no_overlap);
    op = oLitEnd;
    *litPtr = iLitEnd;


    if (sequence.offset > (size_t)(oLitEnd - prefixStart)) {

        RETURN_ERROR_IF(sequence.offset > (size_t)(oLitEnd - virtualStart), corruption_detected, "");
        match = dictEnd - (prefixStart - match);
        if (match + sequence.matchLength <= dictEnd) {
            ZSTD_memmove(oLitEnd, match, sequence.matchLength);
            return sequenceLength;
        }

        {   size_t const length1 = (size_t)(dictEnd - match);
            ZSTD_memmove(oLitEnd, match, length1);
            op = oLitEnd + length1;
            sequence.matchLength -= length1;
            match = prefixStart;
        }
    }
    ZSTD_safecopy(op, oend_w, match, sequence.matchLength, ZSTD_overlap_src_before_dst);
    return sequenceLength;
}


FORCE_NOINLINE
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
size_t ZSTD_execSequenceEndSplitLitBuffer(BYTE* op,
    BYTE* const oend, const BYTE* const oend_w, seq_t sequence,
    const BYTE** litPtr, const BYTE* const litLimit,
    const BYTE* const prefixStart, const BYTE* const virtualStart, const BYTE* const dictEnd)
{
    BYTE* const oLitEnd = op + sequence.litLength;
    size_t const sequenceLength = sequence.litLength + sequence.matchLength;
    const BYTE* const iLitEnd = *litPtr + sequence.litLength;
    const BYTE* match = oLitEnd - sequence.offset;


    RETURN_ERROR_IF(sequenceLength > (size_t)(oend - op), dstSize_tooSmall, "last match must fit within dstBuffer");
    RETURN_ERROR_IF(sequence.litLength > (size_t)(litLimit - *litPtr), corruption_detected, "try to read beyond literal buffer");
    assert(op < op + sequenceLength);
    assert(oLitEnd < op + sequenceLength);


    RETURN_ERROR_IF(op > *litPtr && op < *litPtr + sequence.litLength, dstSize_tooSmall, "output should not catch up to and overwrite literal buffer");
    ZSTD_safecopyDstBeforeSrc(op, *litPtr, sequence.litLength);
    op = oLitEnd;
    *litPtr = iLitEnd;


    if (sequence.offset > (size_t)(oLitEnd - prefixStart)) {

        RETURN_ERROR_IF(sequence.offset > (size_t)(oLitEnd - virtualStart), corruption_detected, "");
        match = dictEnd - (prefixStart - match);
        if (match + sequence.matchLength <= dictEnd) {
            ZSTD_memmove(oLitEnd, match, sequence.matchLength);
            return sequenceLength;
        }

        {   size_t const length1 = (size_t)(dictEnd - match);
            ZSTD_memmove(oLitEnd, match, length1);
            op = oLitEnd + length1;
            sequence.matchLength -= length1;
            match = prefixStart;
        }
    }
    ZSTD_safecopy(op, oend_w, match, sequence.matchLength, ZSTD_overlap_src_before_dst);
    return sequenceLength;
}

HINT_INLINE
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
size_t ZSTD_execSequence(BYTE* op,
    BYTE* const oend, seq_t sequence,
    const BYTE** litPtr, const BYTE* const litLimit,
    const BYTE* const prefixStart, const BYTE* const virtualStart, const BYTE* const dictEnd)
{
    BYTE* const oLitEnd = op + sequence.litLength;
    size_t const sequenceLength = sequence.litLength + sequence.matchLength;
    BYTE* const oMatchEnd = op + sequenceLength;
    BYTE* const oend_w = oend - WILDCOPY_OVERLENGTH;
    const BYTE* const iLitEnd = *litPtr + sequence.litLength;
    const BYTE* match = oLitEnd - sequence.offset;

    assert(op != NULL );
    assert(oend_w < oend );

#if defined(__aarch64__)

    PREFETCH_L1(match);
#endif

    if (UNLIKELY(
        iLitEnd > litLimit ||
        oMatchEnd > oend_w ||
        (MEM_32bits() && (size_t)(oend - op) < sequenceLength + WILDCOPY_OVERLENGTH)))
        return ZSTD_execSequenceEnd(op, oend, sequence, litPtr, litLimit, prefixStart, virtualStart, dictEnd);


    assert(op <= oLitEnd );
    assert(oLitEnd < oMatchEnd );
    assert(oMatchEnd <= oend );
    assert(iLitEnd <= litLimit );
    assert(oLitEnd <= oend_w );
    assert(oMatchEnd <= oend_w );


    assert(WILDCOPY_OVERLENGTH >= 16);
    ZSTD_copy16(op, (*litPtr));
    if (UNLIKELY(sequence.litLength > 16)) {
        ZSTD_wildcopy(op + 16, (*litPtr) + 16, sequence.litLength - 16, ZSTD_no_overlap);
    }
    op = oLitEnd;
    *litPtr = iLitEnd;


    if (sequence.offset > (size_t)(oLitEnd - prefixStart)) {

        RETURN_ERROR_IF(UNLIKELY(sequence.offset > (size_t)(oLitEnd - virtualStart)), corruption_detected, "");
        match = dictEnd + (match - prefixStart);
        if (match + sequence.matchLength <= dictEnd) {
            ZSTD_memmove(oLitEnd, match, sequence.matchLength);
            return sequenceLength;
        }

        {   size_t const length1 = (size_t)(dictEnd - match);
            ZSTD_memmove(oLitEnd, match, length1);
            op = oLitEnd + length1;
            sequence.matchLength -= length1;
            match = prefixStart;
        }
    }

    assert(op <= oMatchEnd);
    assert(oMatchEnd <= oend_w);
    assert(match >= prefixStart);
    assert(sequence.matchLength >= 1);


    if (LIKELY(sequence.offset >= WILDCOPY_VECLEN)) {

        ZSTD_wildcopy(op, match, sequence.matchLength, ZSTD_no_overlap);
        return sequenceLength;
    }
    assert(sequence.offset < WILDCOPY_VECLEN);


    ZSTD_overlapCopy8(&op, &match, sequence.offset);


    if (sequence.matchLength > 8) {
        assert(op < oMatchEnd);
        ZSTD_wildcopy(op, match, sequence.matchLength - 8, ZSTD_overlap_src_before_dst);
    }
    return sequenceLength;
}

HINT_INLINE
ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
size_t ZSTD_execSequenceSplitLitBuffer(BYTE* op,
    BYTE* const oend, const BYTE* const oend_w, seq_t sequence,
    const BYTE** litPtr, const BYTE* const litLimit,
    const BYTE* const prefixStart, const BYTE* const virtualStart, const BYTE* const dictEnd)
{
    BYTE* const oLitEnd = op + sequence.litLength;
    size_t const sequenceLength = sequence.litLength + sequence.matchLength;
    BYTE* const oMatchEnd = op + sequenceLength;
    const BYTE* const iLitEnd = *litPtr + sequence.litLength;
    const BYTE* match = oLitEnd - sequence.offset;

    assert(op != NULL );
    assert(oend_w < oend );

    if (UNLIKELY(
            iLitEnd > litLimit ||
            oMatchEnd > oend_w ||
            (MEM_32bits() && (size_t)(oend - op) < sequenceLength + WILDCOPY_OVERLENGTH)))
        return ZSTD_execSequenceEndSplitLitBuffer(op, oend, oend_w, sequence, litPtr, litLimit, prefixStart, virtualStart, dictEnd);


    assert(op <= oLitEnd );
    assert(oLitEnd < oMatchEnd );
    assert(oMatchEnd <= oend );
    assert(iLitEnd <= litLimit );
    assert(oLitEnd <= oend_w );
    assert(oMatchEnd <= oend_w );


    assert(WILDCOPY_OVERLENGTH >= 16);
    ZSTD_copy16(op, (*litPtr));
    if (UNLIKELY(sequence.litLength > 16)) {
        ZSTD_wildcopy(op+16, (*litPtr)+16, sequence.litLength-16, ZSTD_no_overlap);
    }
    op = oLitEnd;
    *litPtr = iLitEnd;


    if (sequence.offset > (size_t)(oLitEnd - prefixStart)) {

        RETURN_ERROR_IF(UNLIKELY(sequence.offset > (size_t)(oLitEnd - virtualStart)), corruption_detected, "");
        match = dictEnd + (match - prefixStart);
        if (match + sequence.matchLength <= dictEnd) {
            ZSTD_memmove(oLitEnd, match, sequence.matchLength);
            return sequenceLength;
        }

        {   size_t const length1 = (size_t)(dictEnd - match);
            ZSTD_memmove(oLitEnd, match, length1);
            op = oLitEnd + length1;
            sequence.matchLength -= length1;
            match = prefixStart;
    }   }

    assert(op <= oMatchEnd);
    assert(oMatchEnd <= oend_w);
    assert(match >= prefixStart);
    assert(sequence.matchLength >= 1);


    if (LIKELY(sequence.offset >= WILDCOPY_VECLEN)) {

        ZSTD_wildcopy(op, match, sequence.matchLength, ZSTD_no_overlap);
        return sequenceLength;
    }
    assert(sequence.offset < WILDCOPY_VECLEN);


    ZSTD_overlapCopy8(&op, &match, sequence.offset);


    if (sequence.matchLength > 8) {
        assert(op < oMatchEnd);
        ZSTD_wildcopy(op, match, sequence.matchLength-8, ZSTD_overlap_src_before_dst);
    }
    return sequenceLength;
}


static void
ZSTD_initFseState(ZSTD_fseState* DStatePtr, BIT_DStream_t* bitD, const ZSTD_seqSymbol* dt)
{
    const void* ptr = dt;
    const ZSTD_seqSymbol_header* const DTableH = (const ZSTD_seqSymbol_header*)ptr;
    DStatePtr->state = BIT_readBits(bitD, DTableH->tableLog);
    DEBUGLOG(6, "ZSTD_initFseState : val=%u using %u bits",
                (U32)DStatePtr->state, DTableH->tableLog);
    BIT_reloadDStream(bitD);
    DStatePtr->table = dt + 1;
}

FORCE_INLINE_TEMPLATE void
ZSTD_updateFseStateWithDInfo(ZSTD_fseState* DStatePtr, BIT_DStream_t* bitD, U16 nextState, U32 nbBits)
{
    size_t const lowBits = BIT_readBits(bitD, nbBits);
    DStatePtr->state = nextState + lowBits;
}


#define LONG_OFFSETS_MAX_EXTRA_BITS_32                       \
    (ZSTD_WINDOWLOG_MAX_32 > STREAM_ACCUMULATOR_MIN_32       \
        ? ZSTD_WINDOWLOG_MAX_32 - STREAM_ACCUMULATOR_MIN_32  \
        : 0)

typedef enum { ZSTD_lo_isRegularOffset, ZSTD_lo_isLongOffset=1 } ZSTD_longOffset_e;


FORCE_INLINE_TEMPLATE seq_t
ZSTD_decodeSequence(seqState_t* seqState, const ZSTD_longOffset_e longOffsets, const int isLastSeq)
{
    seq_t seq;
#if defined(__aarch64__)
    size_t prevOffset0 = seqState->prevOffset[0];
    size_t prevOffset1 = seqState->prevOffset[1];
    size_t prevOffset2 = seqState->prevOffset[2];

#  if defined(__GNUC__) && !defined(__clang__)
    ZSTD_seqSymbol llDInfoS, mlDInfoS, ofDInfoS;
    ZSTD_seqSymbol* const llDInfo = &llDInfoS;
    ZSTD_seqSymbol* const mlDInfo = &mlDInfoS;
    ZSTD_seqSymbol* const ofDInfo = &ofDInfoS;
    ZSTD_memcpy(llDInfo, seqState->stateLL.table + seqState->stateLL.state, sizeof(ZSTD_seqSymbol));
    ZSTD_memcpy(mlDInfo, seqState->stateML.table + seqState->stateML.state, sizeof(ZSTD_seqSymbol));
    ZSTD_memcpy(ofDInfo, seqState->stateOffb.table + seqState->stateOffb.state, sizeof(ZSTD_seqSymbol));
#  else
    const ZSTD_seqSymbol* const llDInfo = seqState->stateLL.table + seqState->stateLL.state;
    const ZSTD_seqSymbol* const mlDInfo = seqState->stateML.table + seqState->stateML.state;
    const ZSTD_seqSymbol* const ofDInfo = seqState->stateOffb.table + seqState->stateOffb.state;
#  endif
    (void)longOffsets;
    seq.matchLength = mlDInfo->baseValue;
    seq.litLength = llDInfo->baseValue;
    {   U32 const ofBase = ofDInfo->baseValue;
        BYTE const llBits = llDInfo->nbAdditionalBits;
        BYTE const mlBits = mlDInfo->nbAdditionalBits;
        BYTE const ofBits = ofDInfo->nbAdditionalBits;
        BYTE const totalBits = llBits+mlBits+ofBits;

        U16 const llNext = llDInfo->nextState;
        U16 const mlNext = mlDInfo->nextState;
        U16 const ofNext = ofDInfo->nextState;
        U32 const llnbBits = llDInfo->nbBits;
        U32 const mlnbBits = mlDInfo->nbBits;
        U32 const ofnbBits = ofDInfo->nbBits;

        assert(llBits <= MaxLLBits);
        assert(mlBits <= MaxMLBits);
        assert(ofBits <= MaxOff);


        {   size_t offset;
            if (ofBits > 1) {
                ZSTD_STATIC_ASSERT(ZSTD_lo_isLongOffset == 1);
                ZSTD_STATIC_ASSERT(LONG_OFFSETS_MAX_EXTRA_BITS_32 == 5);
                ZSTD_STATIC_ASSERT(STREAM_ACCUMULATOR_MIN_32 > LONG_OFFSETS_MAX_EXTRA_BITS_32);
                ZSTD_STATIC_ASSERT(STREAM_ACCUMULATOR_MIN_32 - LONG_OFFSETS_MAX_EXTRA_BITS_32 >= MaxMLBits);
                offset = ofBase + BIT_readBitsFast(&seqState->DStream, ofBits);
                prevOffset2 = prevOffset1;
                prevOffset1 = prevOffset0;
                prevOffset0 = offset;
            } else {
                U32 const ll0 = (llDInfo->baseValue == 0);
                if (LIKELY((ofBits == 0))) {
                    if (ll0) {
                        offset = prevOffset1;
                        prevOffset1 = prevOffset0;
                        prevOffset0 = offset;
                    } else {
                        offset = prevOffset0;
                    }
                } else {
                    offset = ofBase + ll0 + BIT_readBitsFast(&seqState->DStream, 1);
                    {   size_t temp = (offset == 1)   ? prevOffset1
                                      : (offset == 3) ? prevOffset0 - 1
                                      : (offset >= 2) ? prevOffset2
                                      : prevOffset0;

                        temp -= !temp;
                        prevOffset2 = (offset == 1) ? prevOffset2 : prevOffset1;
                        prevOffset1 = prevOffset0;
                        prevOffset0 = offset = temp;
            }   }   }
            seq.offset = offset;
        }

        if (mlBits > 0)
            seq.matchLength += BIT_readBitsFast(&seqState->DStream, mlBits);

        if (UNLIKELY(totalBits >= STREAM_ACCUMULATOR_MIN_64-(LLFSELog+MLFSELog+OffFSELog)))
            BIT_reloadDStream(&seqState->DStream);


        ZSTD_STATIC_ASSERT(16+LLFSELog+MLFSELog+OffFSELog < STREAM_ACCUMULATOR_MIN_64);

        if (llBits > 0)
            seq.litLength += BIT_readBitsFast(&seqState->DStream, llBits);

        DEBUGLOG(6, "seq: litL=%u, matchL=%u, offset=%u",
                    (U32)seq.litLength, (U32)seq.matchLength, (U32)seq.offset);

        if (!isLastSeq) {

            ZSTD_updateFseStateWithDInfo(&seqState->stateLL, &seqState->DStream, llNext, llnbBits);
            ZSTD_updateFseStateWithDInfo(&seqState->stateML, &seqState->DStream, mlNext, mlnbBits);
            ZSTD_updateFseStateWithDInfo(&seqState->stateOffb, &seqState->DStream, ofNext, ofnbBits);
            BIT_reloadDStream(&seqState->DStream);
        }
    }
    seqState->prevOffset[0] = prevOffset0;
    seqState->prevOffset[1] = prevOffset1;
    seqState->prevOffset[2] = prevOffset2;
#else
    const ZSTD_seqSymbol* const llDInfo = seqState->stateLL.table + seqState->stateLL.state;
    const ZSTD_seqSymbol* const mlDInfo = seqState->stateML.table + seqState->stateML.state;
    const ZSTD_seqSymbol* const ofDInfo = seqState->stateOffb.table + seqState->stateOffb.state;
    seq.matchLength = mlDInfo->baseValue;
    seq.litLength = llDInfo->baseValue;
    {   U32 const ofBase = ofDInfo->baseValue;
        BYTE const llBits = llDInfo->nbAdditionalBits;
        BYTE const mlBits = mlDInfo->nbAdditionalBits;
        BYTE const ofBits = ofDInfo->nbAdditionalBits;
        BYTE const totalBits = llBits+mlBits+ofBits;

        U16 const llNext = llDInfo->nextState;
        U16 const mlNext = mlDInfo->nextState;
        U16 const ofNext = ofDInfo->nextState;
        U32 const llnbBits = llDInfo->nbBits;
        U32 const mlnbBits = mlDInfo->nbBits;
        U32 const ofnbBits = ofDInfo->nbBits;

        assert(llBits <= MaxLLBits);
        assert(mlBits <= MaxMLBits);
        assert(ofBits <= MaxOff);


        {   size_t offset;
            if (ofBits > 1) {
                ZSTD_STATIC_ASSERT(ZSTD_lo_isLongOffset == 1);
                ZSTD_STATIC_ASSERT(LONG_OFFSETS_MAX_EXTRA_BITS_32 == 5);
                ZSTD_STATIC_ASSERT(STREAM_ACCUMULATOR_MIN_32 > LONG_OFFSETS_MAX_EXTRA_BITS_32);
                ZSTD_STATIC_ASSERT(STREAM_ACCUMULATOR_MIN_32 - LONG_OFFSETS_MAX_EXTRA_BITS_32 >= MaxMLBits);
                if (MEM_32bits() && longOffsets && (ofBits >= STREAM_ACCUMULATOR_MIN_32)) {

                    U32 const extraBits = LONG_OFFSETS_MAX_EXTRA_BITS_32;
                    offset = ofBase + (BIT_readBitsFast(&seqState->DStream, ofBits - extraBits) << extraBits);
                    BIT_reloadDStream(&seqState->DStream);
                    offset += BIT_readBitsFast(&seqState->DStream, extraBits);
                } else {
                    offset = ofBase + BIT_readBitsFast(&seqState->DStream, ofBits);
                    if (MEM_32bits()) BIT_reloadDStream(&seqState->DStream);
                }
                seqState->prevOffset[2] = seqState->prevOffset[1];
                seqState->prevOffset[1] = seqState->prevOffset[0];
                seqState->prevOffset[0] = offset;
            } else {
                U32 const ll0 = (llDInfo->baseValue == 0);
                if (LIKELY((ofBits == 0))) {
                    offset = seqState->prevOffset[ll0];
                    seqState->prevOffset[1] = seqState->prevOffset[!ll0];
                    seqState->prevOffset[0] = offset;
                } else {
                    offset = ofBase + ll0 + BIT_readBitsFast(&seqState->DStream, 1);
                    {   size_t temp = (offset==3) ? seqState->prevOffset[0] - 1 : seqState->prevOffset[offset];
                        temp -= !temp;
                        if (offset != 1) seqState->prevOffset[2] = seqState->prevOffset[1];
                        seqState->prevOffset[1] = seqState->prevOffset[0];
                        seqState->prevOffset[0] = offset = temp;
            }   }   }
            seq.offset = offset;
        }

        if (mlBits > 0)
            seq.matchLength += BIT_readBitsFast(&seqState->DStream, mlBits);

        if (MEM_32bits() && (mlBits+llBits >= STREAM_ACCUMULATOR_MIN_32-LONG_OFFSETS_MAX_EXTRA_BITS_32))
            BIT_reloadDStream(&seqState->DStream);
        if (MEM_64bits() && UNLIKELY(totalBits >= STREAM_ACCUMULATOR_MIN_64-(LLFSELog+MLFSELog+OffFSELog)))
            BIT_reloadDStream(&seqState->DStream);

        ZSTD_STATIC_ASSERT(16+LLFSELog+MLFSELog+OffFSELog < STREAM_ACCUMULATOR_MIN_64);

        if (llBits > 0)
            seq.litLength += BIT_readBitsFast(&seqState->DStream, llBits);

        if (MEM_32bits())
            BIT_reloadDStream(&seqState->DStream);

        DEBUGLOG(6, "seq: litL=%u, matchL=%u, offset=%u",
                    (U32)seq.litLength, (U32)seq.matchLength, (U32)seq.offset);

        if (!isLastSeq) {

            ZSTD_updateFseStateWithDInfo(&seqState->stateLL, &seqState->DStream, llNext, llnbBits);
            ZSTD_updateFseStateWithDInfo(&seqState->stateML, &seqState->DStream, mlNext, mlnbBits);
            if (MEM_32bits()) BIT_reloadDStream(&seqState->DStream);
            ZSTD_updateFseStateWithDInfo(&seqState->stateOffb, &seqState->DStream, ofNext, ofnbBits);
            BIT_reloadDStream(&seqState->DStream);
        }
    }
#endif

    return seq;
}

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
#if DEBUGLEVEL >= 1
static int ZSTD_dictionaryIsActive(ZSTD_DCtx const* dctx, BYTE const* prefixStart, BYTE const* oLitEnd)
{
    size_t const windowSize = dctx->fParams.windowSize;

    if (dctx->dictContentEndForFuzzing == NULL) return 0;

    if (prefixStart == dctx->dictContentBeginForFuzzing) return 1;

    if (dctx->dictEnd != dctx->dictContentEndForFuzzing) return 0;

    if ((size_t)(oLitEnd - prefixStart) >= windowSize) return 0;

    return 1;
}
#endif

static void ZSTD_assertValidSequence(
        ZSTD_DCtx const* dctx,
        BYTE const* op, BYTE const* oend,
        seq_t const seq,
        BYTE const* prefixStart, BYTE const* virtualStart)
{
#if DEBUGLEVEL >= 1
    if (dctx->isFrameDecompression) {
        size_t const windowSize = dctx->fParams.windowSize;
        size_t const sequenceSize = seq.litLength + seq.matchLength;
        BYTE const* const oLitEnd = op + seq.litLength;
        DEBUGLOG(6, "Checking sequence: litL=%u matchL=%u offset=%u",
                (U32)seq.litLength, (U32)seq.matchLength, (U32)seq.offset);
        assert(op <= oend);
        assert((size_t)(oend - op) >= sequenceSize);
        assert(sequenceSize <= ZSTD_blockSizeMax(dctx));
        if (ZSTD_dictionaryIsActive(dctx, prefixStart, oLitEnd)) {
            size_t const dictSize = (size_t)((char const*)dctx->dictContentEndForFuzzing - (char const*)dctx->dictContentBeginForFuzzing);

            assert(seq.offset <= (size_t)(oLitEnd - virtualStart));
            assert(seq.offset <= windowSize + dictSize);
        } else {

            assert(seq.offset <= windowSize);
        }
    }
#else
    (void)dctx, (void)op, (void)oend, (void)seq, (void)prefixStart, (void)virtualStart;
#endif
}
#endif

#ifndef ZSTD_FORCE_DECOMPRESS_SEQUENCES_LONG


FORCE_INLINE_TEMPLATE size_t
DONT_VECTORIZE
ZSTD_decompressSequences_bodySplitLitBuffer( ZSTD_DCtx* dctx,
                               void* dst, size_t maxDstSize,
                         const void* seqStart, size_t seqSize, int nbSeq,
                         const ZSTD_longOffset_e isLongOffset)
{
    BYTE* const ostart = (BYTE*)dst;
    BYTE* const oend = (BYTE*)ZSTD_maybeNullPtrAdd(ostart, (ptrdiff_t)maxDstSize);
    BYTE* op = ostart;
    const BYTE* litPtr = dctx->litPtr;
    const BYTE* litBufferEnd = dctx->litBufferEnd;
    const BYTE* const prefixStart = (const BYTE*) (dctx->prefixStart);
    const BYTE* const vBase = (const BYTE*) (dctx->virtualStart);
    const BYTE* const dictEnd = (const BYTE*) (dctx->dictEnd);
    DEBUGLOG(5, "ZSTD_decompressSequences_bodySplitLitBuffer (%i seqs)", nbSeq);


    if (nbSeq) {
        seqState_t seqState;
        dctx->fseEntropy = 1;
        { U32 i; for (i=0; i<ZSTD_REP_NUM; i++) seqState.prevOffset[i] = dctx->entropy.rep[i]; }
        RETURN_ERROR_IF(
            ERR_isError(BIT_initDStream(&seqState.DStream, seqStart, seqSize)),
            corruption_detected, "");
        ZSTD_initFseState(&seqState.stateLL, &seqState.DStream, dctx->LLTptr);
        ZSTD_initFseState(&seqState.stateOffb, &seqState.DStream, dctx->OFTptr);
        ZSTD_initFseState(&seqState.stateML, &seqState.DStream, dctx->MLTptr);
        assert(dst != NULL);

        ZSTD_STATIC_ASSERT(
                BIT_DStream_unfinished < BIT_DStream_completed &&
                BIT_DStream_endOfBuffer < BIT_DStream_completed &&
                BIT_DStream_completed < BIT_DStream_overflow);


        {   seq_t sequence = {0,0,0};

#if defined(__GNUC__) && defined(__x86_64__)
            __asm__(".p2align 6");
#  if __GNUC__ >= 7

            __asm__("nop");
            __asm__(".p2align 5");
            __asm__("nop");
            __asm__(".p2align 4");
#    if __GNUC__ == 8 || __GNUC__ == 10

            __asm__("nop");
            __asm__(".p2align 3");
#    endif
#  endif
#endif


            for ( ; nbSeq; nbSeq--) {
                sequence = ZSTD_decodeSequence(&seqState, isLongOffset, nbSeq==1);
                if (litPtr + sequence.litLength > dctx->litBufferEnd) break;
                {   size_t const oneSeqSize = ZSTD_execSequenceSplitLitBuffer(op, oend, litPtr + sequence.litLength - WILDCOPY_OVERLENGTH, sequence, &litPtr, litBufferEnd, prefixStart, vBase, dictEnd);
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
                    assert(!ZSTD_isError(oneSeqSize));
                    ZSTD_assertValidSequence(dctx, op, oend, sequence, prefixStart, vBase);
#endif
                    if (UNLIKELY(ZSTD_isError(oneSeqSize)))
                        return oneSeqSize;
                    DEBUGLOG(6, "regenerated sequence size : %u", (U32)oneSeqSize);
                    op += oneSeqSize;
            }   }
            DEBUGLOG(6, "reached: (litPtr + sequence.litLength > dctx->litBufferEnd)");


            if (nbSeq > 0) {
                const size_t leftoverLit = (size_t)(dctx->litBufferEnd - litPtr);
                assert(dctx->litBufferEnd >= litPtr);
                DEBUGLOG(6, "There are %i sequences left, and %zu/%zu literals left in buffer", nbSeq, leftoverLit, sequence.litLength);
                if (leftoverLit) {
                    RETURN_ERROR_IF(leftoverLit > (size_t)(oend - op), dstSize_tooSmall, "remaining lit must fit within dstBuffer");
                    ZSTD_safecopyDstBeforeSrc(op, litPtr, leftoverLit);
                    sequence.litLength -= leftoverLit;
                    op += leftoverLit;
                }
                litPtr = dctx->litExtraBuffer;
                litBufferEnd = dctx->litExtraBuffer + ZSTD_LITBUFFEREXTRASIZE;
                dctx->litBufferLocation = ZSTD_not_in_dst;
                {   size_t const oneSeqSize = ZSTD_execSequence(op, oend, sequence, &litPtr, litBufferEnd, prefixStart, vBase, dictEnd);
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
                    assert(!ZSTD_isError(oneSeqSize));
                    ZSTD_assertValidSequence(dctx, op, oend, sequence, prefixStart, vBase);
#endif
                    if (UNLIKELY(ZSTD_isError(oneSeqSize)))
                        return oneSeqSize;
                    DEBUGLOG(6, "regenerated sequence size : %u", (U32)oneSeqSize);
                    op += oneSeqSize;
                }
                nbSeq--;
            }
        }

        if (nbSeq > 0) {


#if defined(__GNUC__) && defined(__x86_64__)
            __asm__(".p2align 6");
            __asm__("nop");
#  if __GNUC__ != 7

            __asm__(".p2align 4");
            __asm__("nop");
            __asm__(".p2align 3");
#  elif __GNUC__ >= 11
            __asm__(".p2align 3");
#  else
            __asm__(".p2align 5");
            __asm__("nop");
            __asm__(".p2align 3");
#  endif
#endif

            for ( ; nbSeq ; nbSeq--) {
                seq_t const sequence = ZSTD_decodeSequence(&seqState, isLongOffset, nbSeq==1);
                size_t const oneSeqSize = ZSTD_execSequence(op, oend, sequence, &litPtr, litBufferEnd, prefixStart, vBase, dictEnd);
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
                assert(!ZSTD_isError(oneSeqSize));
                ZSTD_assertValidSequence(dctx, op, oend, sequence, prefixStart, vBase);
#endif
                if (UNLIKELY(ZSTD_isError(oneSeqSize)))
                    return oneSeqSize;
                DEBUGLOG(6, "regenerated sequence size : %u", (U32)oneSeqSize);
                op += oneSeqSize;
            }
        }


        DEBUGLOG(5, "ZSTD_decompressSequences_bodySplitLitBuffer: after decode loop, remaining nbSeq : %i", nbSeq);
        RETURN_ERROR_IF(nbSeq, corruption_detected, "");
        DEBUGLOG(5, "bitStream : start=%p, ptr=%p, bitsConsumed=%u", seqState.DStream.start, seqState.DStream.ptr, seqState.DStream.bitsConsumed);
        RETURN_ERROR_IF(!BIT_endOfDStream(&seqState.DStream), corruption_detected, "");

        { U32 i; for (i=0; i<ZSTD_REP_NUM; i++) dctx->entropy.rep[i] = (U32)(seqState.prevOffset[i]); }
    }


    if (dctx->litBufferLocation == ZSTD_split) {

        size_t const lastLLSize = (size_t)(litBufferEnd - litPtr);
        DEBUGLOG(6, "copy last literals from segment : %u", (U32)lastLLSize);
        RETURN_ERROR_IF(lastLLSize > (size_t)(oend - op), dstSize_tooSmall, "");
        if (op != NULL) {
            ZSTD_memmove(op, litPtr, lastLLSize);
            op += lastLLSize;
        }
        litPtr = dctx->litExtraBuffer;
        litBufferEnd = dctx->litExtraBuffer + ZSTD_LITBUFFEREXTRASIZE;
        dctx->litBufferLocation = ZSTD_not_in_dst;
    }

    {   size_t const lastLLSize = (size_t)(litBufferEnd - litPtr);
        DEBUGLOG(6, "copy last literals from internal buffer : %u", (U32)lastLLSize);
        RETURN_ERROR_IF(lastLLSize > (size_t)(oend-op), dstSize_tooSmall, "");
        if (op != NULL) {
            ZSTD_memcpy(op, litPtr, lastLLSize);
            op += lastLLSize;
    }   }

    DEBUGLOG(6, "decoded block of size %u bytes", (U32)(op - ostart));
    return (size_t)(op - ostart);
}

FORCE_INLINE_TEMPLATE size_t
DONT_VECTORIZE
ZSTD_decompressSequences_body(ZSTD_DCtx* dctx,
    void* dst, size_t maxDstSize,
    const void* seqStart, size_t seqSize, int nbSeq,
    const ZSTD_longOffset_e isLongOffset)
{
    BYTE* const ostart = (BYTE*)dst;
    BYTE* const oend = (dctx->litBufferLocation == ZSTD_not_in_dst) ?
                        (BYTE*)ZSTD_maybeNullPtrAdd(ostart, (ptrdiff_t)maxDstSize) :
                        dctx->litBuffer;
    BYTE* op = ostart;
    const BYTE* litPtr = dctx->litPtr;
    const BYTE* const litEnd = litPtr + dctx->litSize;
    const BYTE* const prefixStart = (const BYTE*)(dctx->prefixStart);
    const BYTE* const vBase = (const BYTE*)(dctx->virtualStart);
    const BYTE* const dictEnd = (const BYTE*)(dctx->dictEnd);
    DEBUGLOG(5, "ZSTD_decompressSequences_body: nbSeq = %d", nbSeq);


    if (nbSeq) {
        seqState_t seqState;
        dctx->fseEntropy = 1;
        { U32 i; for (i = 0; i < ZSTD_REP_NUM; i++) seqState.prevOffset[i] = dctx->entropy.rep[i]; }
        RETURN_ERROR_IF(
            ERR_isError(BIT_initDStream(&seqState.DStream, seqStart, seqSize)),
            corruption_detected, "");
        ZSTD_initFseState(&seqState.stateLL, &seqState.DStream, dctx->LLTptr);
        ZSTD_initFseState(&seqState.stateOffb, &seqState.DStream, dctx->OFTptr);
        ZSTD_initFseState(&seqState.stateML, &seqState.DStream, dctx->MLTptr);
        assert(dst != NULL);

#if defined(__GNUC__) && defined(__x86_64__)
            __asm__(".p2align 6");
            __asm__("nop");
#  if __GNUC__ >= 7
            __asm__(".p2align 5");
            __asm__("nop");
            __asm__(".p2align 3");
#  else
            __asm__(".p2align 4");
            __asm__("nop");
            __asm__(".p2align 3");
#  endif
#endif

        for ( ; nbSeq ; nbSeq--) {
            seq_t const sequence = ZSTD_decodeSequence(&seqState, isLongOffset, nbSeq==1);
            size_t const oneSeqSize = ZSTD_execSequence(op, oend, sequence, &litPtr, litEnd, prefixStart, vBase, dictEnd);
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
            assert(!ZSTD_isError(oneSeqSize));
            ZSTD_assertValidSequence(dctx, op, oend, sequence, prefixStart, vBase);
#endif
            if (UNLIKELY(ZSTD_isError(oneSeqSize)))
                return oneSeqSize;
            DEBUGLOG(6, "regenerated sequence size : %u", (U32)oneSeqSize);
            op += oneSeqSize;
        }


        assert(nbSeq == 0);
        RETURN_ERROR_IF(!BIT_endOfDStream(&seqState.DStream), corruption_detected, "");

        { U32 i; for (i=0; i<ZSTD_REP_NUM; i++) dctx->entropy.rep[i] = (U32)(seqState.prevOffset[i]); }
    }


    {   size_t const lastLLSize = (size_t)(litEnd - litPtr);
        DEBUGLOG(6, "copy last literals : %u", (U32)lastLLSize);
        RETURN_ERROR_IF(lastLLSize > (size_t)(oend-op), dstSize_tooSmall, "");
        if (op != NULL) {
            ZSTD_memcpy(op, litPtr, lastLLSize);
            op += lastLLSize;
    }   }

    DEBUGLOG(6, "decoded block of size %u bytes", (U32)(op - ostart));
    return (size_t)(op - ostart);
}

static size_t
ZSTD_decompressSequences_default(ZSTD_DCtx* dctx,
                                 void* dst, size_t maxDstSize,
                           const void* seqStart, size_t seqSize, int nbSeq,
                           const ZSTD_longOffset_e isLongOffset)
{
    return ZSTD_decompressSequences_body(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}

static size_t
ZSTD_decompressSequencesSplitLitBuffer_default(ZSTD_DCtx* dctx,
                                               void* dst, size_t maxDstSize,
                                         const void* seqStart, size_t seqSize, int nbSeq,
                                         const ZSTD_longOffset_e isLongOffset)
{
    return ZSTD_decompressSequences_bodySplitLitBuffer(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}
#endif

#ifndef ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT

FORCE_INLINE_TEMPLATE

size_t ZSTD_prefetchMatch(size_t prefetchPos, seq_t const sequence,
                   const BYTE* const prefixStart, const BYTE* const dictEnd)
{
    prefetchPos += sequence.litLength;
    {   const BYTE* const matchBase = (sequence.offset > prefetchPos) ? dictEnd : prefixStart;

        const BYTE* const match = (const BYTE*)ZSTD_wrappedPtrSub(ZSTD_wrappedPtrAdd(matchBase, (ptrdiff_t)prefetchPos), (ptrdiff_t)sequence.offset);
        PREFETCH_L1(match); PREFETCH_L1(ZSTD_wrappedPtrAdd(match, CACHELINE_SIZE));
    }
    return prefetchPos + sequence.matchLength;
}


FORCE_INLINE_TEMPLATE size_t
ZSTD_decompressSequencesLong_body(
                               ZSTD_DCtx* dctx,
                               void* dst, size_t maxDstSize,
                         const void* seqStart, size_t seqSize, int nbSeq,
                         const ZSTD_longOffset_e isLongOffset)
{
    BYTE* const ostart = (BYTE*)dst;
    BYTE* const oend = (dctx->litBufferLocation == ZSTD_in_dst) ?
                        dctx->litBuffer :
                        (BYTE*)ZSTD_maybeNullPtrAdd(ostart, (ptrdiff_t)maxDstSize);
    BYTE* op = ostart;
    const BYTE* litPtr = dctx->litPtr;
    const BYTE* litBufferEnd = dctx->litBufferEnd;
    const BYTE* const prefixStart = (const BYTE*) (dctx->prefixStart);
    const BYTE* const dictStart = (const BYTE*) (dctx->virtualStart);
    const BYTE* const dictEnd = (const BYTE*) (dctx->dictEnd);


    if (nbSeq) {
#define STORED_SEQS 8
#define STORED_SEQS_MASK (STORED_SEQS-1)
#define ADVANCED_SEQS STORED_SEQS
        seq_t sequences[STORED_SEQS];
        int const seqAdvance = MIN(nbSeq, ADVANCED_SEQS);
        seqState_t seqState;
        int seqNb;
        size_t prefetchPos = (size_t)(op-prefixStart);

        dctx->fseEntropy = 1;
        { int i; for (i=0; i<ZSTD_REP_NUM; i++) seqState.prevOffset[i] = dctx->entropy.rep[i]; }
        assert(dst != NULL);
        RETURN_ERROR_IF(
            ERR_isError(BIT_initDStream(&seqState.DStream, seqStart, seqSize)),
            corruption_detected, "");
        ZSTD_initFseState(&seqState.stateLL, &seqState.DStream, dctx->LLTptr);
        ZSTD_initFseState(&seqState.stateOffb, &seqState.DStream, dctx->OFTptr);
        ZSTD_initFseState(&seqState.stateML, &seqState.DStream, dctx->MLTptr);


        for (seqNb=0; seqNb<seqAdvance; seqNb++) {
            seq_t const sequence = ZSTD_decodeSequence(&seqState, isLongOffset, seqNb == nbSeq-1);
            prefetchPos = ZSTD_prefetchMatch(prefetchPos, sequence, prefixStart, dictEnd);
            sequences[seqNb] = sequence;
        }


        for (; seqNb < nbSeq; seqNb++) {
            seq_t sequence = ZSTD_decodeSequence(&seqState, isLongOffset, seqNb == nbSeq-1);

            if (dctx->litBufferLocation == ZSTD_split && litPtr + sequences[(seqNb - ADVANCED_SEQS) & STORED_SEQS_MASK].litLength > dctx->litBufferEnd) {

                const size_t leftoverLit = (size_t)(dctx->litBufferEnd - litPtr);
                assert(dctx->litBufferEnd >= litPtr);
                if (leftoverLit) {
                    RETURN_ERROR_IF(leftoverLit > (size_t)(oend - op), dstSize_tooSmall, "remaining lit must fit within dstBuffer");
                    ZSTD_safecopyDstBeforeSrc(op, litPtr, leftoverLit);
                    sequences[(seqNb - ADVANCED_SEQS) & STORED_SEQS_MASK].litLength -= leftoverLit;
                    op += leftoverLit;
                }
                litPtr = dctx->litExtraBuffer;
                litBufferEnd = dctx->litExtraBuffer + ZSTD_LITBUFFEREXTRASIZE;
                dctx->litBufferLocation = ZSTD_not_in_dst;
                {   size_t const oneSeqSize = ZSTD_execSequence(op, oend, sequences[(seqNb - ADVANCED_SEQS) & STORED_SEQS_MASK], &litPtr, litBufferEnd, prefixStart, dictStart, dictEnd);
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
                    assert(!ZSTD_isError(oneSeqSize));
                    ZSTD_assertValidSequence(dctx, op, oend, sequences[(seqNb - ADVANCED_SEQS) & STORED_SEQS_MASK], prefixStart, dictStart);
#endif
                    if (ZSTD_isError(oneSeqSize)) return oneSeqSize;

                    prefetchPos = ZSTD_prefetchMatch(prefetchPos, sequence, prefixStart, dictEnd);
                    sequences[seqNb & STORED_SEQS_MASK] = sequence;
                    op += oneSeqSize;
            }   }
            else
            {

                size_t const oneSeqSize = dctx->litBufferLocation == ZSTD_split ?
                    ZSTD_execSequenceSplitLitBuffer(op, oend, litPtr + sequences[(seqNb - ADVANCED_SEQS) & STORED_SEQS_MASK].litLength - WILDCOPY_OVERLENGTH, sequences[(seqNb - ADVANCED_SEQS) & STORED_SEQS_MASK], &litPtr, litBufferEnd, prefixStart, dictStart, dictEnd) :
                    ZSTD_execSequence(op, oend, sequences[(seqNb - ADVANCED_SEQS) & STORED_SEQS_MASK], &litPtr, litBufferEnd, prefixStart, dictStart, dictEnd);
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
                assert(!ZSTD_isError(oneSeqSize));
                ZSTD_assertValidSequence(dctx, op, oend, sequences[(seqNb - ADVANCED_SEQS) & STORED_SEQS_MASK], prefixStart, dictStart);
#endif
                if (ZSTD_isError(oneSeqSize)) return oneSeqSize;

                prefetchPos = ZSTD_prefetchMatch(prefetchPos, sequence, prefixStart, dictEnd);
                sequences[seqNb & STORED_SEQS_MASK] = sequence;
                op += oneSeqSize;
            }
        }
        RETURN_ERROR_IF(!BIT_endOfDStream(&seqState.DStream), corruption_detected, "");


        seqNb -= seqAdvance;
        for ( ; seqNb<nbSeq ; seqNb++) {
            seq_t *sequence = &(sequences[seqNb&STORED_SEQS_MASK]);
            if (dctx->litBufferLocation == ZSTD_split && litPtr + sequence->litLength > dctx->litBufferEnd) {
                const size_t leftoverLit = (size_t)(dctx->litBufferEnd - litPtr);
                assert(dctx->litBufferEnd >= litPtr);
                if (leftoverLit) {
                    RETURN_ERROR_IF(leftoverLit > (size_t)(oend - op), dstSize_tooSmall, "remaining lit must fit within dstBuffer");
                    ZSTD_safecopyDstBeforeSrc(op, litPtr, leftoverLit);
                    sequence->litLength -= leftoverLit;
                    op += leftoverLit;
                }
                litPtr = dctx->litExtraBuffer;
                litBufferEnd = dctx->litExtraBuffer + ZSTD_LITBUFFEREXTRASIZE;
                dctx->litBufferLocation = ZSTD_not_in_dst;
                {   size_t const oneSeqSize = ZSTD_execSequence(op, oend, *sequence, &litPtr, litBufferEnd, prefixStart, dictStart, dictEnd);
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
                    assert(!ZSTD_isError(oneSeqSize));
                    ZSTD_assertValidSequence(dctx, op, oend, sequences[seqNb&STORED_SEQS_MASK], prefixStart, dictStart);
#endif
                    if (ZSTD_isError(oneSeqSize)) return oneSeqSize;
                    op += oneSeqSize;
                }
            }
            else
            {
                size_t const oneSeqSize = dctx->litBufferLocation == ZSTD_split ?
                    ZSTD_execSequenceSplitLitBuffer(op, oend, litPtr + sequence->litLength - WILDCOPY_OVERLENGTH, *sequence, &litPtr, litBufferEnd, prefixStart, dictStart, dictEnd) :
                    ZSTD_execSequence(op, oend, *sequence, &litPtr, litBufferEnd, prefixStart, dictStart, dictEnd);
#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) && defined(FUZZING_ASSERT_VALID_SEQUENCE)
                assert(!ZSTD_isError(oneSeqSize));
                ZSTD_assertValidSequence(dctx, op, oend, sequences[seqNb&STORED_SEQS_MASK], prefixStart, dictStart);
#endif
                if (ZSTD_isError(oneSeqSize)) return oneSeqSize;
                op += oneSeqSize;
            }
        }


        { U32 i; for (i=0; i<ZSTD_REP_NUM; i++) dctx->entropy.rep[i] = (U32)(seqState.prevOffset[i]); }
    }


    if (dctx->litBufferLocation == ZSTD_split) {
        size_t const lastLLSize = (size_t)(litBufferEnd - litPtr);
        assert(litBufferEnd >= litPtr);
        RETURN_ERROR_IF(lastLLSize > (size_t)(oend - op), dstSize_tooSmall, "");
        if (op != NULL) {
            ZSTD_memmove(op, litPtr, lastLLSize);
            op += lastLLSize;
        }
        litPtr = dctx->litExtraBuffer;
        litBufferEnd = dctx->litExtraBuffer + ZSTD_LITBUFFEREXTRASIZE;
    }
    {   size_t const lastLLSize = (size_t)(litBufferEnd - litPtr);
        assert(litBufferEnd >= litPtr);
        RETURN_ERROR_IF(lastLLSize > (size_t)(oend-op), dstSize_tooSmall, "");
        if (op != NULL) {
            ZSTD_memmove(op, litPtr, lastLLSize);
            op += lastLLSize;
        }
    }

    return (size_t)(op - ostart);
}

static size_t
ZSTD_decompressSequencesLong_default(ZSTD_DCtx* dctx,
                                 void* dst, size_t maxDstSize,
                           const void* seqStart, size_t seqSize, int nbSeq,
                           const ZSTD_longOffset_e isLongOffset)
{
    return ZSTD_decompressSequencesLong_body(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}
#endif


#if DYNAMIC_BMI2

#ifndef ZSTD_FORCE_DECOMPRESS_SEQUENCES_LONG
static BMI2_TARGET_ATTRIBUTE size_t
DONT_VECTORIZE
ZSTD_decompressSequences_bmi2(ZSTD_DCtx* dctx,
                                 void* dst, size_t maxDstSize,
                           const void* seqStart, size_t seqSize, int nbSeq,
                           const ZSTD_longOffset_e isLongOffset)
{
    return ZSTD_decompressSequences_body(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}
static BMI2_TARGET_ATTRIBUTE size_t
DONT_VECTORIZE
ZSTD_decompressSequencesSplitLitBuffer_bmi2(ZSTD_DCtx* dctx,
                                 void* dst, size_t maxDstSize,
                           const void* seqStart, size_t seqSize, int nbSeq,
                           const ZSTD_longOffset_e isLongOffset)
{
    return ZSTD_decompressSequences_bodySplitLitBuffer(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}
#endif

#ifndef ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT
static BMI2_TARGET_ATTRIBUTE size_t
ZSTD_decompressSequencesLong_bmi2(ZSTD_DCtx* dctx,
                                 void* dst, size_t maxDstSize,
                           const void* seqStart, size_t seqSize, int nbSeq,
                           const ZSTD_longOffset_e isLongOffset)
{
    return ZSTD_decompressSequencesLong_body(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}
#endif

#endif

#ifndef ZSTD_FORCE_DECOMPRESS_SEQUENCES_LONG
static size_t
ZSTD_decompressSequences(ZSTD_DCtx* dctx, void* dst, size_t maxDstSize,
                   const void* seqStart, size_t seqSize, int nbSeq,
                   const ZSTD_longOffset_e isLongOffset)
{
    DEBUGLOG(5, "ZSTD_decompressSequences");
#if DYNAMIC_BMI2
    if (ZSTD_DCtx_get_bmi2(dctx)) {
        return ZSTD_decompressSequences_bmi2(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
    }
#endif
    return ZSTD_decompressSequences_default(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}
static size_t
ZSTD_decompressSequencesSplitLitBuffer(ZSTD_DCtx* dctx, void* dst, size_t maxDstSize,
                                 const void* seqStart, size_t seqSize, int nbSeq,
                                 const ZSTD_longOffset_e isLongOffset)
{
    DEBUGLOG(5, "ZSTD_decompressSequencesSplitLitBuffer");
#if DYNAMIC_BMI2
    if (ZSTD_DCtx_get_bmi2(dctx)) {
        return ZSTD_decompressSequencesSplitLitBuffer_bmi2(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
    }
#endif
    return ZSTD_decompressSequencesSplitLitBuffer_default(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}
#endif


#ifndef ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT

static size_t
ZSTD_decompressSequencesLong(ZSTD_DCtx* dctx,
                             void* dst, size_t maxDstSize,
                             const void* seqStart, size_t seqSize, int nbSeq,
                             const ZSTD_longOffset_e isLongOffset)
{
    DEBUGLOG(5, "ZSTD_decompressSequencesLong");
#if DYNAMIC_BMI2
    if (ZSTD_DCtx_get_bmi2(dctx)) {
        return ZSTD_decompressSequencesLong_bmi2(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
    }
#endif
  return ZSTD_decompressSequencesLong_default(dctx, dst, maxDstSize, seqStart, seqSize, nbSeq, isLongOffset);
}
#endif


static size_t ZSTD_totalHistorySize(void* curPtr, const void* virtualStart)
{
    return (size_t)((char*)curPtr - (const char*)virtualStart);
}

typedef struct {
    unsigned longOffsetShare;
    unsigned maxNbAdditionalBits;
} ZSTD_OffsetInfo;


static ZSTD_OffsetInfo
ZSTD_getOffsetInfo(const ZSTD_seqSymbol* offTable, int nbSeq)
{
    ZSTD_OffsetInfo info = {0, 0};

    if (nbSeq != 0) {
        const void* ptr = offTable;
        U32 const tableLog = ((const ZSTD_seqSymbol_header*)ptr)[0].tableLog;
        const ZSTD_seqSymbol* table = offTable + 1;
        U32 const max = 1 << tableLog;
        U32 u;
        DEBUGLOG(5, "ZSTD_getLongOffsetsShare: (tableLog=%u)", tableLog);

        assert(max <= (1 << OffFSELog));
        for (u=0; u<max; u++) {
            info.maxNbAdditionalBits = MAX(info.maxNbAdditionalBits, table[u].nbAdditionalBits);
            if (table[u].nbAdditionalBits > 22) info.longOffsetShare += 1;
        }

        assert(tableLog <= OffFSELog);
        info.longOffsetShare <<= (OffFSELog - tableLog);
    }

    return info;
}


static size_t ZSTD_maxShortOffset(void)
{
    if (MEM_64bits()) {

        ZSTD_STATIC_ASSERT(ZSTD_WINDOWLOG_MAX <= 31);
        return (size_t)-1;
    } else {

        size_t const maxOffbase = ((size_t)1 << (STREAM_ACCUMULATOR_MIN + 1)) - 1;
        size_t const maxOffset = maxOffbase - ZSTD_REP_NUM;
        assert(ZSTD_highbit32((U32)maxOffbase) == STREAM_ACCUMULATOR_MIN);
        return maxOffset;
    }
}

size_t
ZSTD_decompressBlock_internal(ZSTD_DCtx* dctx,
                              void* dst, size_t dstCapacity,
                        const void* src, size_t srcSize, const streaming_operation streaming)
{
    const BYTE* ip = (const BYTE*)src;
    DEBUGLOG(5, "ZSTD_decompressBlock_internal (cSize : %u)", (unsigned)srcSize);


    RETURN_ERROR_IF(srcSize > ZSTD_blockSizeMax(dctx), srcSize_wrong, "");


    {   size_t const litCSize = ZSTD_decodeLiteralsBlock(dctx, src, srcSize, dst, dstCapacity, streaming);
        DEBUGLOG(5, "ZSTD_decodeLiteralsBlock : cSize=%u, nbLiterals=%zu", (U32)litCSize, dctx->litSize);
        if (ZSTD_isError(litCSize)) return litCSize;
        ip += litCSize;
        srcSize -= litCSize;
    }


    {

        size_t const blockSizeMax = MIN(dstCapacity, ZSTD_blockSizeMax(dctx));
        size_t const totalHistorySize = ZSTD_totalHistorySize(ZSTD_maybeNullPtrAdd(dst, (ptrdiff_t)blockSizeMax), (BYTE const*)dctx->virtualStart);

        ZSTD_longOffset_e isLongOffset = (ZSTD_longOffset_e)(MEM_32bits() && (totalHistorySize > ZSTD_maxShortOffset()));

#if !defined(ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT) && \
    !defined(ZSTD_FORCE_DECOMPRESS_SEQUENCES_LONG)
        int usePrefetchDecoder = dctx->ddictIsCold;
#else

        int usePrefetchDecoder = 1;
#endif
        int nbSeq;
        size_t const seqHSize = ZSTD_decodeSeqHeaders(dctx, &nbSeq, ip, srcSize);
        if (ZSTD_isError(seqHSize)) return seqHSize;
        ip += seqHSize;
        srcSize -= seqHSize;

        RETURN_ERROR_IF((dst == NULL || dstCapacity == 0) && nbSeq > 0, dstSize_tooSmall, "NULL not handled");
        RETURN_ERROR_IF(MEM_64bits() && sizeof(size_t) == sizeof(void*) && (size_t)(-1) - (size_t)dst < (size_t)(1 << 20), dstSize_tooSmall,
                "invalid dst");


        if (isLongOffset || (!usePrefetchDecoder && (totalHistorySize > (1u << 24)) && (nbSeq > 8))) {
            ZSTD_OffsetInfo const info = ZSTD_getOffsetInfo(dctx->OFTptr, nbSeq);
            if (isLongOffset && info.maxNbAdditionalBits <= STREAM_ACCUMULATOR_MIN) {

                isLongOffset = ZSTD_lo_isRegularOffset;
            }
            if (!usePrefetchDecoder) {
                U32 const minShare = MEM_64bits() ? 7 : 20;
                usePrefetchDecoder = (info.longOffsetShare >= minShare);
            }
        }

        dctx->ddictIsCold = 0;

#if !defined(ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT) && \
    !defined(ZSTD_FORCE_DECOMPRESS_SEQUENCES_LONG)
        if (usePrefetchDecoder) {
#else
        (void)usePrefetchDecoder;
        {
#endif
#ifndef ZSTD_FORCE_DECOMPRESS_SEQUENCES_SHORT
            return ZSTD_decompressSequencesLong(dctx, dst, dstCapacity, ip, srcSize, nbSeq, isLongOffset);
#endif
        }

#ifndef ZSTD_FORCE_DECOMPRESS_SEQUENCES_LONG

        if (dctx->litBufferLocation == ZSTD_split)
            return ZSTD_decompressSequencesSplitLitBuffer(dctx, dst, dstCapacity, ip, srcSize, nbSeq, isLongOffset);
        else
            return ZSTD_decompressSequences(dctx, dst, dstCapacity, ip, srcSize, nbSeq, isLongOffset);
#endif
    }
}


ZSTD_ALLOW_POINTER_OVERFLOW_ATTR
void ZSTD_checkContinuity(ZSTD_DCtx* dctx, const void* dst, size_t dstSize)
{
    if (dst != dctx->previousDstEnd && dstSize > 0) {
        dctx->dictEnd = dctx->previousDstEnd;
        dctx->virtualStart = (const char*)dst - ((const char*)(dctx->previousDstEnd) - (const char*)(dctx->prefixStart));
        dctx->prefixStart = dst;
        dctx->previousDstEnd = dst;
    }
}


size_t ZSTD_decompressBlock_deprecated(ZSTD_DCtx* dctx,
                                       void* dst, size_t dstCapacity,
                                 const void* src, size_t srcSize)
{
    size_t dSize;
    dctx->isFrameDecompression = 0;
    ZSTD_checkContinuity(dctx, dst, dstCapacity);
    dSize = ZSTD_decompressBlock_internal(dctx, dst, dstCapacity, src, srcSize, not_streaming);
    FORWARD_IF_ERROR(dSize, "");
    dctx->previousDstEnd = (char*)dst + dSize;
    return dSize;
}


size_t ZSTD_decompressBlock(ZSTD_DCtx* dctx,
                            void* dst, size_t dstCapacity,
                      const void* src, size_t srcSize)
{
    return ZSTD_decompressBlock_deprecated(dctx, dst, dstCapacity, src, srcSize);
}


}