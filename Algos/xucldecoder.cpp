/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xucldecoder.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#endif


extern "C" {

#ifndef __UCL_CONF_H
#define __UCL_CONF_H


/***********************************************************************
//
************************************************************************/

#if defined(__UCLCONF_H_INCLUDED)
#  error "include this file first"
#endif

#ifndef __UCLCONF_H_INCLUDED
#define __UCLCONF_H_INCLUDED

#define UCL_VERSION             0x010300L
#define UCL_VERSION_STRING      "1.03"
#define UCL_VERSION_DATE        "Jul 20 2004"

/* internal Autoconf configuration file - only used when building UCL */
#if defined(UCL_HAVE_CONFIG_H)
#  include <config.h>
#endif
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif


#if !defined(CHAR_BIT) || (CHAR_BIT != 8)
#  error "invalid CHAR_BIT"
#endif
#if !defined(UCHAR_MAX) || !defined(UINT_MAX) || !defined(ULONG_MAX)
#  error "check your compiler installation"
#endif
#if (USHRT_MAX < 1) || (UINT_MAX < 1) || (ULONG_MAX < 1)
#  error "your limits.h macros are broken"
#endif

/* workaround a compiler bug under hpux 10.20 */
#define UCL_0xffffL             65535ul
#define UCL_0xffffffffL         4294967295ul

#if !defined(UCL_UINT32_C)
#  if (UINT_MAX < UCL_0xffffffffL)
#    define UCL_UINT32_C(c)     c ## UL
#  else
#    define UCL_UINT32_C(c)     ((c) + 0U)
#  endif
#endif

#if (defined(__CYGWIN__) || defined(__CYGWIN32__)) && defined(__GNUC__)
#  define UCL_OS_CYGWIN         1
#elif defined(__EMX__) && defined(__GNUC__)
#  define UCL_OS_EMX            1
#elif defined(__BORLANDC__) && defined(__DPMI32__) && (__BORLANDC__ >= 0x0460)
#  define UCL_OS_DOS32          1
#elif defined(__BORLANDC__) && defined(__DPMI16__)
#  define UCL_OS_DOS16          1
#elif defined(__ZTC__) && defined(DOS386)
#  define UCL_OS_DOS32          1
#elif defined(__OS2__) || defined(__OS2V2__)
#  if (UINT_MAX == UCL_0xffffL)
#    define UCL_OS_OS216        1
#  elif (UINT_MAX == UCL_0xffffffffL)
#    define UCL_OS_OS2          1
#  else
#    error "check your limits.h header"
#  endif
#elif defined(__WIN64__) || defined(_WIN64) || defined(WIN64)
#  define UCL_OS_WIN64          1
#elif defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS_386__)
#  define UCL_OS_WIN32          1
#elif defined(__MWERKS__) && defined(__INTEL__)
#  define UCL_OS_WIN32          1
#elif defined(__WINDOWS__) || defined(_WINDOWS) || defined(_Windows)
#  if (UINT_MAX == UCL_0xffffL)
#    define UCL_OS_WIN16        1
#  elif (UINT_MAX == UCL_0xffffffffL)
#    define UCL_OS_WIN32        1
#  else
#    error "check your limits.h header"
#  endif
#elif defined(__DOS__) || defined(__MSDOS__) || defined(_MSDOS) || defined(MSDOS) || (defined(__PACIFIC__) && defined(DOS))
#  if (UINT_MAX == UCL_0xffffL)
#    define UCL_OS_DOS16        1
#  elif (UINT_MAX == UCL_0xffffffffL)
#    define UCL_OS_DOS32        1
#  else
#    error "check your limits.h header"
#  endif
#elif defined(__WATCOMC__)
#  if defined(__NT__) && (UINT_MAX == UCL_0xffffL)
     /* wcl: NT host defaults to DOS target */
#    define UCL_OS_DOS16        1
#  elif defined(__NT__) && (__WATCOMC__ < 1100)
     /* wcl386: Watcom C 11 defines _WIN32 */
#    define UCL_OS_WIN32        1
#  else
#    error "please specify a target using the -bt compiler option"
#  endif
#elif defined(__palmos__)
#  if (UINT_MAX == UCL_0xffffL)
#    define UCL_OS_PALMOS       1
#  else
#    error "check your limits.h header"
#  endif
#elif defined(__TOS__) || defined(__atarist__)
#  define UCL_OS_TOS            1
#elif defined(macintosh)
#  define UCL_OS_MACCLASSIC     1
#elif defined(__VMS)
#  define UCL_OS_VMS            1
#else
#  define UCL_OS_POSIX          1
#endif

/* memory checkers */
#if !defined(__UCL_CHECKER)
#  if defined(__BOUNDS_CHECKING_ON)
#    define __UCL_CHECKER       1
#  elif defined(__CHECKER__)
#    define __UCL_CHECKER       1
#  elif defined(__INSURE__)
#    define __UCL_CHECKER       1
#  elif defined(__PURIFY__)
#    define __UCL_CHECKER       1
#  endif
#endif

/* fix ancient compiler versions */
#if (UINT_MAX == UCL_0xffffL)
#if (defined(__MSDOS__) && defined(__TURBOC__) && (__TURBOC__ < 0x0410)) || (defined(MSDOS) && defined(_MSC_VER) && (_MSC_VER < 700))
#  if !defined(__cdecl)
#    define __cdecl cdecl
#  endif
#  if !defined(__far)
#    define __far far
#  endif
#  if !defined(__huge)
#    define __huge huge
#  endif
#  if !defined(__near)
#    define __near near
#  endif
#endif
#endif


/***********************************************************************
// integral and pointer types
************************************************************************/

/* Integral types with 32 bits or more */
#if !defined(UCL_UINT32_MAX)
#  if (UINT_MAX >= UCL_0xffffffffL)
     typedef unsigned int       ucl_uint32;
     typedef int                ucl_int32;
#    define UCL_UINT32_MAX      UINT_MAX
#    define UCL_INT32_MAX       INT_MAX
#    define UCL_INT32_MIN       INT_MIN
#  elif (ULONG_MAX >= UCL_0xffffffffL)
     typedef unsigned long      ucl_uint32;
     typedef long               ucl_int32;
#    define UCL_UINT32_MAX      ULONG_MAX
#    define UCL_INT32_MAX       LONG_MAX
#    define UCL_INT32_MIN       LONG_MIN
#  else
#    error "ucl_uint32"
#  endif
#endif

/* ucl_uint is used like size_t */
#if !defined(UCL_UINT_MAX)
#  if (UINT_MAX >= UCL_0xffffffffL)
     typedef unsigned int       ucl_uint;
     typedef int                ucl_int;
#    define UCL_UINT_MAX        UINT_MAX
#    define UCL_INT_MAX         INT_MAX
#    define UCL_INT_MIN         INT_MIN
#  elif (ULONG_MAX >= UCL_0xffffffffL)
     typedef unsigned long      ucl_uint;
     typedef long               ucl_int;
#    define UCL_UINT_MAX        ULONG_MAX
#    define UCL_INT_MAX         LONG_MAX
#    define UCL_INT_MIN         LONG_MIN
#  else
#    error "ucl_uint"
#  endif
#endif

/* Memory model that allows to access memory at offsets of ucl_uint. */
#if !defined(__UCL_MMODEL)
#  if (UCL_UINT_MAX <= UINT_MAX)
#    define __UCL_MMODEL
#  elif defined(UCL_OS_DOS16) || defined(UCL_OS_OS216) || defined(UCL_OS_WIN16)
#    define __UCL_MMODEL_HUGE   1
#    define __UCL_MMODEL        __huge
#    define ucl_uintptr_t       unsigned long
#  else
#    define __UCL_MMODEL
#  endif
#endif

/* no typedef here because of const-pointer issues */
#define ucl_bytep               unsigned char __UCL_MMODEL *
#define ucl_charp               char __UCL_MMODEL *
#define ucl_voidp               void __UCL_MMODEL *
#define ucl_shortp              short __UCL_MMODEL *
#define ucl_ushortp             unsigned short __UCL_MMODEL *
#define ucl_uint32p             ucl_uint32 __UCL_MMODEL *
#define ucl_int32p              ucl_int32 __UCL_MMODEL *
#define ucl_uintp               ucl_uint __UCL_MMODEL *
#define ucl_intp                ucl_int __UCL_MMODEL *
#define ucl_voidpp              ucl_voidp __UCL_MMODEL *
#define ucl_bytepp              ucl_bytep __UCL_MMODEL *
/* deprecated - use `ucl_bytep' instead of `ucl_byte *' */
#define ucl_byte                unsigned char __UCL_MMODEL

typedef int ucl_bool;


/***********************************************************************
// function types
************************************************************************/

/* name mangling */
#if !defined(__UCL_EXTERN_C)
#  ifdef __cplusplus
#    define __UCL_EXTERN_C      extern "C"
#  else
#    define __UCL_EXTERN_C      extern
#  endif
#endif

/* calling convention */
#if !defined(__UCL_CDECL)
#  if defined(__GNUC__) || defined(__HIGHC__) || defined(__NDPC__)
#    define __UCL_CDECL
#  elif defined(UCL_OS_DOS16) || defined(UCL_OS_OS216) || defined(UCL_OS_WIN16)
#    define __UCL_CDECL         __far __cdecl
#  elif defined(UCL_OS_DOS32) || defined(UCL_OS_OS2) || defined(UCL_OS_WIN32) || defined(UCL_OS_WIN64)
#    define __UCL_CDECL         __cdecl
#  else
#    define __UCL_CDECL
#  endif
#endif

/* DLL export information */
#if !defined(__UCL_EXPORT1)
#  define __UCL_EXPORT1
#endif
#if !defined(__UCL_EXPORT2)
#  define __UCL_EXPORT2
#endif

/* __cdecl calling convention for public C and assembly functions */
#if !defined(UCL_PUBLIC)
#  define UCL_PUBLIC(_rettype)  __UCL_EXPORT1 _rettype __UCL_EXPORT2 __UCL_CDECL
#endif
#if !defined(UCL_EXTERN)
#  define UCL_EXTERN(_rettype)  __UCL_EXTERN_C UCL_PUBLIC(_rettype)
#endif
#if !defined(UCL_PRIVATE)
#  define UCL_PRIVATE(_rettype) static _rettype __UCL_CDECL
#endif

/* C++ exception specification for extern "C" function types */
#if !defined(__cplusplus)
#  undef UCL_NOTHROW
#  define UCL_NOTHROW
#elif !defined(UCL_NOTHROW)
#  define UCL_NOTHROW
#endif

/* function types */
typedef int
(__UCL_CDECL *ucl_compress_t)   ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

typedef int
(__UCL_CDECL *ucl_decompress_t) ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

typedef int
(__UCL_CDECL *ucl_optimize_t)   (       ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

typedef int
(__UCL_CDECL *ucl_compress_dict_t)(const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem,
                                  const ucl_bytep dict, ucl_uint dict_len );

typedef int
(__UCL_CDECL *ucl_decompress_dict_t)(const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem,
                                  const ucl_bytep dict, ucl_uint dict_len );

/* a progress indicator callback function */
typedef struct
{
    void (__UCL_CDECL *callback) (ucl_uint, ucl_uint, int, ucl_voidp);
    ucl_voidp user;
}
ucl_progress_callback_t;
#define ucl_progress_callback_p ucl_progress_callback_t __UCL_MMODEL *


/***********************************************************************
// error codes and prototypes
************************************************************************/

/* Error codes for the compression/decompression functions. Negative
 * values are errors, positive values will be used for special but
 * normal events.
 */
#define UCL_E_OK                    0
#define UCL_E_ERROR                 (-1)
#define UCL_E_INVALID_ARGUMENT      (-2)
#define UCL_E_OUT_OF_MEMORY         (-3)
/* compression errors */
#define UCL_E_NOT_COMPRESSIBLE      (-101)
/* decompression errors */
#define UCL_E_INPUT_OVERRUN         (-201)
#define UCL_E_OUTPUT_OVERRUN        (-202)
#define UCL_E_LOOKBEHIND_OVERRUN    (-203)
#define UCL_E_EOF_NOT_FOUND         (-204)
#define UCL_E_INPUT_NOT_CONSUMED    (-205)
#define UCL_E_OVERLAP_OVERRUN       (-206)


/* ucl_init() should be the first function you call.
 * Check the return code !
 *
 * ucl_init() is a macro to allow checking that the library and the
 * compiler's view of various types are consistent.
 */
#define ucl_init() __ucl_init2(UCL_VERSION,(int)sizeof(short),(int)sizeof(int),\
    (int)sizeof(long),(int)sizeof(ucl_uint32),(int)sizeof(ucl_uint),\
    (int)-1,(int)sizeof(char *),(int)sizeof(ucl_voidp),\
    (int)sizeof(ucl_compress_t))
UCL_EXTERN(int) __ucl_init2(ucl_uint32,int,int,int,int,int,int,int,int,int);

/* version functions (useful for shared libraries) */
UCL_EXTERN(ucl_uint32) ucl_version(void);
UCL_EXTERN(const char *) ucl_version_string(void);
UCL_EXTERN(const char *) ucl_version_date(void);
UCL_EXTERN(const ucl_charp) _ucl_version_string(void);
UCL_EXTERN(const ucl_charp) _ucl_version_date(void);

/* string functions */
UCL_EXTERN(int)
ucl_memcmp(const ucl_voidp _s1, const ucl_voidp _s2, ucl_uint _len);
UCL_EXTERN(ucl_voidp)
ucl_memcpy(ucl_voidp _dest, const ucl_voidp _src, ucl_uint _len);
UCL_EXTERN(ucl_voidp)
ucl_memmove(ucl_voidp _dest, const ucl_voidp _src, ucl_uint _len);
UCL_EXTERN(ucl_voidp)
ucl_memset(ucl_voidp _s, int _c, ucl_uint _len);

/* checksum functions */
UCL_EXTERN(ucl_uint32)
ucl_adler32(ucl_uint32 _adler, const ucl_bytep _buf, ucl_uint _len);
UCL_EXTERN(ucl_uint32)
ucl_crc32(ucl_uint32 _c, const ucl_bytep _buf, ucl_uint _len);
UCL_EXTERN(const ucl_uint32p)
ucl_get_crc32_table(void);

/* memory allocation hooks */
typedef ucl_voidp (__UCL_CDECL *ucl_malloc_hook_t) (ucl_uint);
typedef void (__UCL_CDECL *ucl_free_hook_t) (ucl_voidp);
UCL_EXTERN(void)
ucl_set_malloc_hooks(ucl_malloc_hook_t, ucl_free_hook_t);
UCL_EXTERN(void)
ucl_get_malloc_hooks(ucl_malloc_hook_t*, ucl_free_hook_t*);

/* memory allocation functions */
UCL_EXTERN(ucl_voidp) ucl_malloc(ucl_uint);
UCL_EXTERN(ucl_voidp) ucl_alloc(ucl_uint, ucl_uint);
UCL_EXTERN(void) ucl_free(ucl_voidp);


/* misc. */
UCL_EXTERN(ucl_bool) ucl_assert(int _expr);
UCL_EXTERN(int) _ucl_config_check(void);
typedef union { ucl_bytep p; ucl_uint u; } __ucl_pu_u;
typedef union { ucl_bytep p; ucl_uint32 u32; } __ucl_pu32_u;

/* align a char pointer on a boundary that is a multiple of `size' */
UCL_EXTERN(unsigned) __ucl_align_gap(const ucl_voidp _ptr, ucl_uint _size);
#define UCL_PTR_ALIGN_UP(_ptr,_size) \
    ((_ptr) + (ucl_uint) __ucl_align_gap((const ucl_voidp)(_ptr),(ucl_uint)(_size)))


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

#if defined(UCL_HAVE_CONFIG_H)
#  define ACC_CONFIG_NO_HEADER 1
#endif
#define __ACCLIB_FUNCNAME(f)        error_do_not_use_acclib
#include "xucldecoder_acc.h"

#if (ACC_CC_MSC && (_MSC_VER >= 1300))
   /* avoid `-Wall' warnings in system header files */
#  pragma warning(disable: 4820)
   /* avoid warnings about inlining */
#  pragma warning(disable: 4710 4711)
#endif

#if defined(__UCL_MMODEL_HUGE) && (!ACC_HAVE_MM_HUGE_PTR)
#  error "this should not happen - check defines for __huge"
#endif
#if !defined(__UCL_MMODEL_HUGE) && defined(HAVE_MEMCMP)
#  define ucl_memcmp(a,b,c)     memcmp(a,b,c)
#endif
#if !defined(__UCL_MMODEL_HUGE) && defined(HAVE_MEMCPY)
#  define ucl_memcpy(a,b,c)     memcpy(a,b,c)
#endif
#if !defined(__UCL_MMODEL_HUGE) && defined(HAVE_MEMMOVE)
#  define ucl_memmove(a,b,c)    memmove(a,b,c)
#endif
#if !defined(__UCL_MMODEL_HUGE) && defined(HAVE_MEMSET)
#  define ucl_memset(a,b,c)     memset(a,b,c)
#endif

#if (ACC_OS_DOS16 + 0 != UCL_OS_DOS16 + 0)
#  error "DOS16"
#endif
#if (ACC_OS_OS216 + 0 != UCL_OS_OS216 + 0)
#  error "OS216"
#endif
#if (ACC_OS_WIN16 + 0 != UCL_OS_WIN16 + 0)
#  error "WIN16"
#endif
#if (ACC_OS_DOS32 + 0 != UCL_OS_DOS32 + 0)
#  error "DOS32"
#endif
#if (ACC_OS_OS2 + 0 != UCL_OS_OS2 + 0)
#  error "DOS32"
#endif
#if (ACC_OS_WIN32 + 0 != UCL_OS_WIN32 + 0)
#  error "WIN32"
#endif
#if (ACC_OS_WIN64 + 0 != UCL_OS_WIN64 + 0)
#  error "WIN64"
#endif


#define ACC_WANT_INCD 1
#include "xucldecoder_acc.h"
#undef ACC_WANT_INCD
#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)
#  define ACC_WANT_INCE 1
#  include "xucldecoder_acc.h"
#  undef ACC_WANT_INCE
#  define ACC_WANT_INCI 1
#  include "xucldecoder_acc.h"
#  undef ACC_WANT_INCI
#endif

#undef NDEBUG
#if !defined(UCL_DEBUG)
#  define NDEBUG 1
#endif
#include <assert.h>


#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16) && (ACC_CC_BORLANDC)
#  if (__BORLANDC__ >= 0x0450)  /* v4.00 */
#    pragma option -h           /* enable fast huge pointers */
#  else
#    pragma option -h-          /* disable fast huge pointers - compiler bug */
#  endif
#endif


/***********************************************************************
//
************************************************************************/

#if 1
#  define UCL_BYTE(x)       ((unsigned char) (x))
#else
#  define UCL_BYTE(x)       ((unsigned char) ((x) & 0xff))
#endif
#if 0
#  define UCL_USHORT(x)     ((unsigned short) (x))
#else
#  define UCL_USHORT(x)     ((unsigned short) ((x) & 0xffff))
#endif

#define UCL_MAX(a,b)        ((a) >= (b) ? (a) : (b))
#define UCL_MIN(a,b)        ((a) <= (b) ? (a) : (b))
#define UCL_MAX3(a,b,c)     ((a) >= (b) ? UCL_MAX(a,c) : UCL_MAX(b,c))
#define UCL_MIN3(a,b,c)     ((a) <= (b) ? UCL_MIN(a,c) : UCL_MIN(b,c))

#define ucl_sizeof(type)    ((ucl_uint) (sizeof(type)))

#define UCL_HIGH(array)     ((ucl_uint) (sizeof(array)/sizeof(*(array))))

/* this always fits into 16 bits */
#define UCL_SIZE(bits)      (1u << (bits))
#define UCL_MASK(bits)      (UCL_SIZE(bits) - 1)

#define UCL_LSIZE(bits)     (1ul << (bits))
#define UCL_LMASK(bits)     (UCL_LSIZE(bits) - 1)

#define UCL_USIZE(bits)     ((ucl_uint) 1 << (bits))
#define UCL_UMASK(bits)     (UCL_USIZE(bits) - 1)

/* Maximum value of a signed/unsigned type.
   Do not use casts, avoid overflows ! */
#define UCL_STYPE_MAX(b)    (((1l  << (8*(b)-2)) - 1l)  + (1l  << (8*(b)-2)))
#define UCL_UTYPE_MAX(b)    (((1ul << (8*(b)-1)) - 1ul) + (1ul << (8*(b)-1)))


/***********************************************************************
// compiler and architecture specific stuff
************************************************************************/

/* Some defines that indicate if memory can be accessed at unaligned
 * memory addresses. You should also test that this is actually faster
 * even if it is allowed by your system.
 */

#undef UA_GET2
#undef UA_SET2
#undef UA_GET4
#undef UA_SET4
#if 1 && defined(__GNUC__)
# if ACC_ARCH_AMD64 || ACC_ARCH_IA32 || (__ARM_FEATURE_UNALIGNED) || defined(__powerpc64__)
#  define UCL_UA_ATTR __attribute__((__packed__,__aligned__(1),__may_alias__))
   typedef struct UCL_UA_ATTR { unsigned short v; } UCL_UA2_T;
   typedef struct UCL_UA_ATTR { acc_uint32e_t v; } UCL_UA4_T;
   ACC_COMPILE_TIME_ASSERT_HEADER(sizeof(UCL_UA2_T) == 2)
   ACC_COMPILE_TIME_ASSERT_HEADER(sizeof(UCL_UA4_T) == 4)
   ACC_COMPILE_TIME_ASSERT_HEADER(__alignof__(UCL_UA2_T) == 1)
   ACC_COMPILE_TIME_ASSERT_HEADER(__alignof__(UCL_UA4_T) == 1)
#  define UA_GET2(p)    (((const UCL_UA2_T *)(p))->v)
#  define UA_GET4(p)    (((const UCL_UA4_T *)(p))->v)
# endif
#endif


/***********************************************************************
// some globals
************************************************************************/

__UCL_EXTERN_C int __ucl_init_done;
UCL_EXTERN(const ucl_bytep) ucl_copyright(void);


/***********************************************************************
// ANSI C preprocessor macros
************************************************************************/

#define _UCL_STRINGIZE(x)           #x
#define _UCL_MEXPAND(x)             _UCL_STRINGIZE(x)

#ifndef __UCL_PTR_H
#define __UCL_PTR_H

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
// Integral types
************************************************************************/

#if !defined(ucl_uintptr_t)
#  define ucl_uintptr_t     acc_uintptr_t
#endif


/***********************************************************************
//
************************************************************************/

/* Always use the safe (=integral) version for pointer-comparisions.
 * The compiler should optimize away the additional casts anyway.
 *
 * Note that this only works if the representation and ordering
 * of the pointer and the integral is the same (at bit level).
 *
 * Most 16-bit compilers have their own view about pointers -
 * fortunately they don't care about comparing pointers
 * that are pointing to Nirvana.
 */

#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)
#define PTR(a)              ((ucl_bytep) (a))
/* only need the low bits of the pointer -> offset is ok */
#define PTR_ALIGNED_4(a)    ((ACC_FP_OFF(a) & 3) == 0)
#define PTR_ALIGNED2_4(a,b) (((ACC_FP_OFF(a) | ACC_FP_OFF(b)) & 3) == 0)
#else
#define PTR(a)              ((ucl_uintptr_t) (a))
#define PTR_LINEAR(a)       PTR(a)
#define PTR_ALIGNED_4(a)    ((PTR_LINEAR(a) & 3) == 0)
#define PTR_ALIGNED_8(a)    ((PTR_LINEAR(a) & 7) == 0)
#define PTR_ALIGNED2_4(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 3) == 0)
#define PTR_ALIGNED2_8(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 7) == 0)
#endif

#define PTR_LT(a,b)         (PTR(a) < PTR(b))
#define PTR_GE(a,b)         (PTR(a) >= PTR(b))


UCL_EXTERN(ucl_uintptr_t)
__ucl_ptr_linear(const ucl_voidp ptr);


typedef union
{
    char            a_char;
    unsigned char   a_uchar;
    short           a_short;
    unsigned short  a_ushort;
    int             a_int;
    unsigned int    a_uint;
    long            a_long;
    unsigned long   a_ulong;
    ucl_int         a_ucl_int;
    ucl_uint        a_ucl_uint;
    ucl_int32       a_ucl_int32;
    ucl_uint32      a_ucl_uint32;
    ptrdiff_t       a_ptrdiff_t;
    ucl_uintptr_t   a_ucl_uintptr_t;
    ucl_voidp       a_ucl_voidp;
    void *          a_void_p;
    ucl_bytep       a_ucl_bytep;
    ucl_bytepp      a_ucl_bytepp;
    ucl_uintp       a_ucl_uintp;
    ucl_uint *      a_ucl_uint_p;
    ucl_uint32p     a_ucl_uint32p;
    ucl_uint32 *    a_ucl_uint32_p;
    unsigned char * a_uchar_p;
    char *          a_char_p;
}
ucl_align_t;



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

/*
vi:ts=4:et
*/


#endif /* already included */


#ifndef __UCL_H_INCLUDED
#define __UCL_H_INCLUDED

#ifndef __UCLCONF_H_INCLUDED

#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
// Compression fine-tuning configuration.
//
// Pass a NULL pointer to the compression functions for default values.
// Otherwise set all values to -1 [i.e. initialize the struct by a
// `memset(x,0xff,sizeof(x))'] and then set the required values.
************************************************************************/

struct ucl_compress_config_t
{
    int bb_endian;
    int bb_size;
    ucl_uint max_offset;
    ucl_uint max_match;
    int s_level;
    int h_level;
    int p_level;
    int c_flags;
    ucl_uint m_size;
};

#define ucl_compress_config_p   ucl_compress_config_t __UCL_MMODEL *


/***********************************************************************
// compressors
//
// Pass NULL for `cb' (no progress callback), `conf' (default compression
// configuration) and `result' (no statistical result).
************************************************************************/

UCL_EXTERN(int)
ucl_nrv2b_99_compress      ( const ucl_bytep src, ucl_uint src_len,
                                   ucl_bytep dst, ucl_uintp dst_len,
                                   ucl_progress_callback_p cb,
                                   int level,
                             const struct ucl_compress_config_p conf,
                                   ucl_uintp result );

UCL_EXTERN(int)
ucl_nrv2d_99_compress      ( const ucl_bytep src, ucl_uint src_len,
                                   ucl_bytep dst, ucl_uintp dst_len,
                                   ucl_progress_callback_p cb,
                                   int level,
                             const struct ucl_compress_config_p conf,
                                   ucl_uintp result );

UCL_EXTERN(int)
ucl_nrv2e_99_compress      ( const ucl_bytep src, ucl_uint src_len,
                                   ucl_bytep dst, ucl_uintp dst_len,
                                   ucl_progress_callback_p cb,
                                   int level,
                             const struct ucl_compress_config_p conf,
                                   ucl_uintp result );


/***********************************************************************
// decompressors
//
// Always pass NULL for `wrkmem'. This parameter is for symetry
// with my other compression libaries and is not used in UCL -
// UCL does not need any additional memory (or even local stack space)
// for decompression.
************************************************************************/

UCL_EXTERN(int)
ucl_nrv2b_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_decompress_safe_8     ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_decompress_safe_le16  ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_decompress_safe_le32  ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

UCL_EXTERN(int)
ucl_nrv2d_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_decompress_safe_8     ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_decompress_safe_le16  ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_decompress_safe_le32  ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

UCL_EXTERN(int)
ucl_nrv2e_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_decompress_safe_8     ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_decompress_safe_le16  ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_decompress_safe_le32  ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );


/***********************************************************************
// assembler decompressors [TO BE ADDED]
************************************************************************/


/***********************************************************************
// test an overlapping in-place decompression within a buffer:
//   - try a virtual decompression from &buf[src_off] -> &buf[0]
//   - no data is actually written
//   - only the bytes at buf[src_off..src_off+src_len-1] will get accessed
//
// NOTE: always pass NULL for `wrkmem' - see above.
************************************************************************/

UCL_EXTERN(int)
ucl_nrv2b_test_overlap_8        ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_test_overlap_le16     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2b_test_overlap_le32     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

UCL_EXTERN(int)
ucl_nrv2d_test_overlap_8        ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_test_overlap_le16     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2d_test_overlap_le32     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );

UCL_EXTERN(int)
ucl_nrv2e_test_overlap_8        ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_test_overlap_le16     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
UCL_EXTERN(int)
ucl_nrv2e_test_overlap_le32     ( const ucl_bytep buf, ucl_uint src_off,
                                        ucl_uint  src_len, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

#if 1
#define getbit_8(bb, src, ilen) \
    (((bb = bb & 0x7f ? bb*2 : ((unsigned)src[ilen++]*2+1)) >> 8) & 1)
#elif 1
#define getbit_8(bb, src, ilen) \
    (bb*=2,bb&0xff ? (bb>>8)&1 : ((bb=src[ilen++]*2+1)>>8)&1)
#else
#define getbit_8(bb, src, ilen) \
    (((bb*=2, (bb&0xff ? bb : (bb=src[ilen++]*2+1,bb))) >> 8) & 1)
#endif


#define getbit_le16(bb, src, ilen) \
    (bb*=2,bb&0xffff ? (bb>>16)&1 : (ilen+=2,((bb=(src[ilen-2]+src[ilen-1]*256u)*2+1)>>16)&1))


#if 1 && (ACC_ENDIAN_LITTLE_ENDIAN) && defined(UA_GET4)
#define getbit_le32(bb, bc, src, ilen) \
    (bc > 0 ? ((bb>>--bc)&1) : (bc=31,\
    bb=UA_GET4((src)+ilen),ilen+=4,(bb>>31)&1))
#else
#define getbit_le32(bb, bc, src, ilen) \
    (bc > 0 ? ((bb>>--bc)&1) : (bc=31,\
    bb=src[ilen]+src[ilen+1]*0x100+src[ilen+2]*UCL_UINT32_C(0x10000)+src[ilen+3]*UCL_UINT32_C(0x1000000),\
    ilen+=4,(bb>>31)&1))
#endif


/***********************************************************************
// implementation
************************************************************************/

#if defined(__UCL_MMODEL_HUGE)

#define acc_hsize_t             ucl_uint
#define acc_hvoid_p             ucl_voidp
#define ACCLIB_PUBLIC(r,f)      static r __UCL_CDECL f
#define acc_halloc              ucl_malloc_internal
#define acc_hfree               ucl_free_internal
/* ACC -- Automatic Compiler Configuration

   Copyright (C) 1996-2004 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   This software is a copyrighted work licensed under the terms of
   the GNU General Public License. Please consult the file "ACC_LICENSE"
   for details.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/
 */


#define __ACCLIB_HALLOC_CH_INCLUDED 1
#if !defined(ACCLIB_PUBLIC)
#  define ACCLIB_PUBLIC(r,f)    r __ACCLIB_FUNCNAME(f)
#endif


#if (ACC_HAVE_MM_HUGE_PTR)
#if 1 && (ACC_OS_DOS16 && defined(BLX286))
#  define __ACCLIB_HALLOC_USE_DAH 1
#elif 1 && (ACC_OS_DOS16 && defined(DOSX286))
#  define __ACCLIB_HALLOC_USE_DAH 1
#elif 1 && (ACC_OS_OS216)
#  define __ACCLIB_HALLOC_USE_DAH 1
#elif 1 && (ACC_OS_WIN16)
#  define __ACCLIB_HALLOC_USE_GA 1
#elif 1 && (ACC_OS_DOS16) && (ACC_CC_BORLANDC) && defined(__DPMI16__)
#  define __ACCLIB_HALLOC_USE_GA 1
#endif
#endif


#if (__ACCLIB_HALLOC_USE_DAH)
#if 0 && (ACC_OS_OS216)
#include <os2.h>
#else
ACC_EXTERN_C unsigned short __far __pascal DosAllocHuge(unsigned short, unsigned short, unsigned short __far *, unsigned short, unsigned short);
ACC_EXTERN_C unsigned short __far __pascal DosFreeSeg(unsigned short);
#endif
#endif

#if (__ACCLIB_HALLOC_USE_GA)
#if 0
#define STRICT 1
#include <windows.h>
#else
ACC_EXTERN_C const void __near* __far __pascal GlobalAlloc(unsigned, unsigned long);
ACC_EXTERN_C const void __near* __far __pascal GlobalFree(const void __near*);
ACC_EXTERN_C unsigned long __far __pascal GlobalHandle(unsigned);
ACC_EXTERN_C void __far* __far __pascal GlobalLock(const void __near*);
ACC_EXTERN_C int __far __pascal GlobalUnlock(const void __near*);
#endif
#endif


/***********************************************************************
// halloc
************************************************************************/

ACCLIB_PUBLIC(acc_hvoid_p, acc_halloc) (acc_hsize_t size)
{
    acc_hvoid_p p = 0;

    if (size <= 0)
        return p;

#if 0 && defined(__palmos__)
    p = MemPtrNew(size);
#elif !defined(ACC_HAVE_MM_HUGE_PTR)
    if (size < (size_t) -1)
        p = malloc((size_t) size);
#else
    if ((long)size <= 0)
        return p;
{
#if (__ACCLIB_HALLOC_USE_DAH)
    unsigned short sel = 0;
    if (DosAllocHuge((unsigned short)(size >> 16), (unsigned short)size, &sel, 0, 0) == 0)
        p = (acc_hvoid_p) ACC_MK_FP(sel, 0);
#elif (__ACCLIB_HALLOC_USE_GA)
    const void __near* h = GlobalAlloc(2, size);
    if (h) {
        p = GlobalLock(h);
        if (p && ACC_FP_OFF(p) != 0) {
            GlobalUnlock(h);
            p = 0;
        }
        if (!p)
            GlobalFree(h);
    }
#elif (ACC_CC_MSC && (_MSC_VER >= 700))
    p = _halloc(size, 1);
#elif (ACC_CC_MSC || ACC_CC_WATCOMC)
    p = halloc(size, 1);
#elif (ACC_CC_DMC || ACC_CC_SYMANTECC || ACC_CC_ZORTECHC)
    p = farmalloc(size);
#elif (ACC_CC_BORLANDC || ACC_CC_TURBOC)
    p = farmalloc(size);
#elif defined(ACC_CC_AZTECC)
    p = lmalloc(size);
#else
    if (size < (size_t) -1)
        p = malloc((size_t) size);
#endif
}
#endif

    return p;
}


ACCLIB_PUBLIC(void, acc_hfree) (acc_hvoid_p p)
{
    if (!p)
        return;

#if 0 && defined(__palmos__)
    MemPtrFree(p);
#elif !defined(ACC_HAVE_MM_HUGE_PTR)
    free(p);
#else
#if (__ACCLIB_HALLOC_USE_DAH)
    if (ACC_FP_OFF(p) == 0)
        DosFreeSeg((unsigned short) ACC_FP_SEG(p));
#elif (__ACCLIB_HALLOC_USE_GA)
    if (ACC_FP_OFF(p) == 0) {
        const void __near* h = (const void __near*) (unsigned) GlobalHandle(ACC_FP_SEG(p));
        if (h) {
            GlobalUnlock(h);
            GlobalFree(h);
        }
    }
#elif (ACC_CC_MSC && (_MSC_VER >= 700))
    _hfree(p);
#elif (ACC_CC_MSC || ACC_CC_WATCOMC)
    hfree(p);
#elif (ACC_CC_DMC || ACC_CC_SYMANTECC || ACC_CC_ZORTECHC)
    farfree((void __far*) p);
#elif (ACC_CC_BORLANDC || ACC_CC_TURBOC)
    farfree((void __far*) p);
#elif defined(ACC_CC_AZTECC)
    lfree(p);
#else
    free(p);
#endif
#endif
}



/*
vi:ts=4:et
*/
#undef ACCLIB_PUBLIC

#else

UCL_PRIVATE(ucl_voidp)
ucl_malloc_internal(ucl_uint size)
{
    ucl_voidp p = NULL;
    if (size < ((~(size_t)0) & (~(ucl_uint)0)))
        p = (ucl_voidp) malloc((size_t) size);
    return p;
}


UCL_PRIVATE(void)
ucl_free_internal(ucl_voidp p)
{
    if (p)
        free(p);
}

#endif


/***********************************************************************
// public interface using the global hooks
************************************************************************/

/* global allocator hooks */
static ucl_malloc_hook_t ucl_malloc_hook = ucl_malloc_internal;
static ucl_free_hook_t ucl_free_hook = ucl_free_internal;

UCL_PUBLIC(void)
ucl_set_malloc_hooks(ucl_malloc_hook_t a, ucl_free_hook_t f)
{
    ucl_malloc_hook = ucl_malloc_internal;
    ucl_free_hook = ucl_free_internal;
    if (a)
        ucl_malloc_hook = a;
    if (f)
        ucl_free_hook = f;
}

UCL_PUBLIC(void)
ucl_get_malloc_hooks(ucl_malloc_hook_t* a, ucl_free_hook_t* f)
{
    if (a)
        *a = ucl_malloc_hook;
    if (f)
        *f = ucl_free_hook;
}


UCL_PUBLIC(ucl_voidp)
ucl_malloc(ucl_uint size)
{
    if (size <= 0)
        return NULL;
    return ucl_malloc_hook(size);
}

UCL_PUBLIC(ucl_voidp)
ucl_alloc(ucl_uint nelems, ucl_uint size)
{
    ucl_uint s = nelems * size;
    if (nelems <= 0 || s / nelems != size)
        return NULL;
    return ucl_malloc(s);
}


UCL_PUBLIC(void)
ucl_free(ucl_voidp p)
{
    if (p)
        ucl_free_hook(p);
}


#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        do {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
        } while (!getbit(bb));
        if (m_off == 2)
        {
            m_off = last_m_off;
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            last_m_off = ++m_off;
        }
        m_len = getbit(bb);
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0xd00);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */


/***********************************************************************
// decompressor entries for the different bit-buffer sizes
************************************************************************/

#ifndef getbit



UCL_PUBLIC(int)
ucl_nrv2b_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_8(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        do {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
        } while (!getbit(bb));
        if (m_off == 2)
        {
            m_off = last_m_off;
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            last_m_off = ++m_off;
        }
        m_len = getbit(bb);
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0xd00);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2b_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_le16(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        do {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
        } while (!getbit(bb));
        if (m_off == 2)
        {
            m_off = last_m_off;
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            last_m_off = ++m_off;
        }
        m_len = getbit(bb);
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0xd00);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2b_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
    unsigned bc = 0;
#define getbit(bb)      getbit_le32(bb,bc,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        do {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
        } while (!getbit(bb));
        if (m_off == 2)
        {
            m_off = last_m_off;
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            last_m_off = ++m_off;
        }
        m_len = getbit(bb);
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0xd00);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


#endif /* !getbit */

#define SAFE
#define ucl_nrv2b_decompress_8      ucl_nrv2b_decompress_safe_8
#define ucl_nrv2b_decompress_le16   ucl_nrv2b_decompress_safe_le16
#define ucl_nrv2b_decompress_le32   ucl_nrv2b_decompress_safe_le32

/***********************************************************************
// actual implementation used by a recursive #include
************************************************************************/

#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        do {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
        } while (!getbit(bb));
        if (m_off == 2)
        {
            m_off = last_m_off;
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            last_m_off = ++m_off;
        }
        m_len = getbit(bb);
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0xd00);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */


/***********************************************************************
// decompressor entries for the different bit-buffer sizes
************************************************************************/

#ifndef getbit



UCL_PUBLIC(int)
ucl_nrv2b_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_8(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        do {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
        } while (!getbit(bb));
        if (m_off == 2)
        {
            m_off = last_m_off;
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            last_m_off = ++m_off;
        }
        m_len = getbit(bb);
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0xd00);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2b_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_le16(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        do {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
        } while (!getbit(bb));
        if (m_off == 2)
        {
            m_off = last_m_off;
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            last_m_off = ++m_off;
        }
        m_len = getbit(bb);
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0xd00);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2b_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
    unsigned bc = 0;
#define getbit(bb)      getbit_le32(bb,bc,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        do {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
        } while (!getbit(bb));
        if (m_off == 2)
        {
            m_off = last_m_off;
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            last_m_off = ++m_off;
        }
        m_len = getbit(bb);
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0xd00);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


#endif /* !getbit */


/*
vi:ts=4:et
*/


#undef SAFE

/***********************************************************************
// actual implementation used by a recursive #include
************************************************************************/

#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */


/***********************************************************************
// decompressor entries for the different bit-buffer sizes
************************************************************************/

#ifndef getbit



UCL_PUBLIC(int)
ucl_nrv2d_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_8(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2d_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_le16(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2d_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
    unsigned bc = 0;
#define getbit(bb)      getbit_le32(bb,bc,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


#endif /* !getbit */

#define SAFE
#define ucl_nrv2d_decompress_8      ucl_nrv2d_decompress_safe_8
#define ucl_nrv2d_decompress_le16   ucl_nrv2d_decompress_safe_le16
#define ucl_nrv2d_decompress_le32   ucl_nrv2d_decompress_safe_le32

#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */


/***********************************************************************
// decompressor entries for the different bit-buffer sizes
************************************************************************/

#ifndef getbit



UCL_PUBLIC(int)
ucl_nrv2d_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_8(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2d_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_le16(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2d_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
    unsigned bc = 0;
#define getbit(bb)      getbit_le32(bb,bc,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


#endif /* !getbit */

#undef SAFE

/***********************************************************************
// actual implementation used by a recursive #include
************************************************************************/

#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */


/***********************************************************************
// decompressor entries for the different bit-buffer sizes
************************************************************************/

#ifndef getbit



UCL_PUBLIC(int)
ucl_nrv2e_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_8(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2e_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_le16(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2e_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
    unsigned bc = 0;
#define getbit(bb)      getbit_le32(bb,bc,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


#endif /* !getbit */

#define SAFE
#define ucl_nrv2e_decompress_8      ucl_nrv2e_decompress_safe_8
#define ucl_nrv2e_decompress_le16   ucl_nrv2e_decompress_safe_le16
#define ucl_nrv2e_decompress_le32   ucl_nrv2e_decompress_safe_le32

/***********************************************************************
// actual implementation used by a recursive #include
************************************************************************/

#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */


/***********************************************************************
// decompressor entries for the different bit-buffer sizes
************************************************************************/

#ifndef getbit



UCL_PUBLIC(int)
ucl_nrv2e_decompress_8          ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_8(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2e_decompress_le16       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
#define getbit(bb)      getbit_le16(bb,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


UCL_PUBLIC(int)
ucl_nrv2e_decompress_le32       ( const ucl_bytep src, ucl_uint  src_len,
                                        ucl_bytep dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem )
{
    unsigned bc = 0;
#define getbit(bb)      getbit_le32(bb,bc,src,ilen)
#ifdef getbit

#ifdef SAFE
#define fail(x,r)   if (x) { *dst_len = olen; return r; }
#else
#define fail(x,r)
#endif

{
    ucl_uint32 bb = 0;
#ifdef TEST_OVERLAP
    ucl_uint ilen = src_off, olen = 0, last_m_off = 1;
#else
    ucl_uint ilen = 0, olen = 0, last_m_off = 1;
#endif
#ifdef SAFE
    const ucl_uint oend = *dst_len;
#endif
    ACC_UNUSED(wrkmem);

#ifdef TEST_OVERLAP
    src_len += src_off;
    fail(oend >= src_len, UCL_E_OVERLAP_OVERRUN);
#endif

    for (;;)
    {
        ucl_uint m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
#ifdef TEST_OVERLAP
            fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
            olen++; ilen++;
#else
            dst[olen++] = src[ilen++];
#endif
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len >= oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
#ifdef TEST_OVERLAP
        olen += m_len + 1;
        fail(olen > ilen, UCL_E_OVERLAP_OVERRUN);
#else
        {
            const ucl_bytep m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
#endif
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}

#undef fail

#endif /* getbit */

#undef getbit
}


#endif /* !getbit */


/*
vi:ts=4:et
*/


#undef SAFE

/***********************************************************************
// crc32 checksum
// adapted from free code by Mark Adler <madler@alumni.caltech.edu>
// see http://www.cdrom.com/pub/infozip/zlib/
************************************************************************/

static const ucl_uint32 __ucl_crc32_table[256] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};


UCL_PUBLIC(const ucl_uint32p)
ucl_get_crc32_table(void)
{
    return __ucl_crc32_table;
}


#if 1
#define UCL_DO1(buf,i) \
    crc = table[((int)crc ^ buf[i]) & 0xff] ^ (crc >> 8)
#else
#define UCL_DO1(buf,i) \
    crc = table[(unsigned char)((unsigned char)crc ^ buf[i])] ^ (crc >> 8)
#endif
#define UCL_DO2(buf,i)  UCL_DO1(buf,i); UCL_DO1(buf,i+1);
#define UCL_DO4(buf,i)  UCL_DO2(buf,i); UCL_DO2(buf,i+2);
#define UCL_DO8(buf,i)  UCL_DO4(buf,i); UCL_DO4(buf,i+4);
#define UCL_DO16(buf,i) UCL_DO8(buf,i); UCL_DO8(buf,i+8);


UCL_PUBLIC(ucl_uint32)
ucl_crc32(ucl_uint32 c, const ucl_bytep buf, ucl_uint len)
{
    ucl_uint32 crc;
#undef table
#if 1
#  define table __ucl_crc32_table
#else
   const ucl_uint32 * table = __ucl_crc32_table;
#endif

    if (buf == NULL)
        return 0;

    crc = (c & UCL_UINT32_C(0xffffffff)) ^ UCL_UINT32_C(0xffffffff);
    if (len >= 16) do
    {
        UCL_DO16(buf,0);
        buf += 16;
        len -= 16;
    } while (len >= 16);
    if (len != 0) do
    {
        UCL_DO1(buf,0);
        buf += 1;
        len -= 1;
    } while (len > 0);

    return crc ^ UCL_UINT32_C(0xffffffff);
#undef table
}

/***********************************************************************
// Runtime check of the assumptions about the size of builtin types,
// memory model, byte order and other low-level constructs.
//
// We are really paranoid here - UCL should either fail
// at startup or not at all.
//
// Because of inlining much of this evaluates to nothing at compile time.
//
// And while many of the tests seem highly obvious and redundant they are
// here to catch compiler/optimizer bugs. Yes, these do exist.
************************************************************************/

static ucl_bool schedule_insns_bug(void);   /* avoid inlining */
static ucl_bool strength_reduce_bug(int *); /* avoid inlining */


#if 0 || defined(UCL_DEBUG)
#include <stdio.h>
static ucl_bool __ucl_assert_fail(const char *s, unsigned line)
{
#if defined(__palmos__)
    printf("UCL assertion failed in line %u: '%s'\n",line,s);
#else
    fprintf(stderr,"UCL assertion failed in line %u: '%s'\n",line,s);
#endif
    return 0;
}
#  define __ucl_assert(x)   ((x) ? 1 : __ucl_assert_fail(#x,__LINE__))
#else
#  define __ucl_assert(x)   ((x) ? 1 : 0)
#endif


/***********************************************************************
// basic_check - compile time assertions
************************************************************************/

#if 1

#undef ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)     ACC_COMPILE_TIME_ASSERT_HEADER(expr)

#define ACC_WANT_CHK 1
#include "xucldecoder_acc.h"
#undef ACC_WANT_CHK

    ACCCHK_ASSERT_IS_SIGNED_T(ucl_int)
    ACCCHK_ASSERT_IS_UNSIGNED_T(ucl_uint)

    ACCCHK_ASSERT_IS_SIGNED_T(ucl_int32)
    ACCCHK_ASSERT_IS_UNSIGNED_T(ucl_uint32)
    ACCCHK_ASSERT((UCL_UINT32_C(1) << (int)(8*sizeof(UCL_UINT32_C(1))-1)) > 0)

    ACCCHK_ASSERT_IS_UNSIGNED_T(ucl_uintptr_t)
    ACCCHK_ASSERT(sizeof(ucl_uintptr_t) >= sizeof(ucl_voidp))

#endif
#undef ACCCHK_ASSERT


/***********************************************************************
//
************************************************************************/

static ucl_bool ptr_check(void)
{
    ucl_bool r = 1;
    int i;
    unsigned char _wrkmem[10 * sizeof(ucl_bytep) + sizeof(ucl_align_t)];
    ucl_bytep wrkmem;
    ucl_bytepp dict;
    unsigned char x[4 * sizeof(ucl_align_t)];
    long d;
    ucl_align_t a;

    for (i = 0; i < (int) sizeof(x); i++)
        x[i] = UCL_BYTE(i);

    memset(_wrkmem,0xff,sizeof(_wrkmem));
    wrkmem = UCL_PTR_ALIGN_UP((ucl_bytep)_wrkmem, sizeof(ucl_align_t));

    dict = (ucl_bytepp) (ucl_voidp) wrkmem;

    d = (long) ((const ucl_bytep) dict - (const ucl_bytep) _wrkmem);
    r &= __ucl_assert(d >= 0);
    r &= __ucl_assert(d < (long) sizeof(ucl_align_t));

    /* this may seem obvious, but some compilers incorrectly inline memset */
    memset(&a,0xff,sizeof(a));
    r &= __ucl_assert(a.a_ushort == USHRT_MAX);
    r &= __ucl_assert(a.a_uint == UINT_MAX);
    r &= __ucl_assert(a.a_ulong == ULONG_MAX);
    r &= __ucl_assert(a.a_ucl_uint == UCL_UINT_MAX);

    /* sanity check of the memory model */
    if (r == 1)
    {
        for (i = 0; i < 8; i++)
            r &= __ucl_assert((const ucl_voidp) (&dict[i]) == (const ucl_voidp) (&wrkmem[i * sizeof(ucl_bytep)]));
    }

    /* check that NULL == 0 */
    memset(&a,0,sizeof(a));
    r &= __ucl_assert(a.a_char_p == NULL);
    r &= __ucl_assert(a.a_ucl_bytep == NULL);

    /* check that the pointer constructs work as expected */
    if (r == 1)
    {
        unsigned k = 1;
        const unsigned n = (unsigned) sizeof(ucl_uint32);
        ucl_bytep p0;
        ucl_bytep p1;

        k += __ucl_align_gap(&x[k],n);
        p0 = (ucl_bytep) &x[k];
#if defined(PTR_LINEAR)
        r &= __ucl_assert((PTR_LINEAR(p0) & (n-1)) == 0);
#else
        r &= __ucl_assert(n == 4);
        r &= __ucl_assert(PTR_ALIGNED_4(p0));
#endif

        r &= __ucl_assert(k >= 1);
        p1 = (ucl_bytep) &x[1];
        r &= __ucl_assert(PTR_GE(p0,p1));

        r &= __ucl_assert(k < 1u+n);
        p1 = (ucl_bytep) &x[1+n];
        r &= __ucl_assert(PTR_LT(p0,p1));

        /* now check that aligned memory access doesn't core dump */
        if (r == 1)
        {
            ucl_uint32 v0, v1;
            v0 = * (ucl_uint32p) (ucl_voidp) &x[k];
            v1 = * (ucl_uint32p) (ucl_voidp) &x[k+n];
            r &= __ucl_assert(v0 > 0);
            r &= __ucl_assert(v1 > 0);
        }
    }

    return r;
}


/***********************************************************************
//
************************************************************************/

UCL_PUBLIC(int)
_ucl_config_check(void)
{
    ucl_bool r = 1;
    int i;
    union {
        ucl_uint32 a;
        unsigned short b;
        ucl_uint32 aa[4];
        unsigned char x[4*sizeof(ucl_align_t)];
    } u;

    u.a = 0; u.b = 0;
    for (i = 0; i < (int) sizeof(u.x); i++)
        u.x[i] = UCL_BYTE(i);

#if defined(ACC_ENDIAN_BIG_ENDIAN) || defined(ACC_ENDIAN_LITTLE_ENDIAN)
    if (r == 1)
    {
#  if defined(ACC_ENDIAN_BIG_ENDIAN)
        ucl_uint32 a = u.a >> (8 * sizeof(u.a) - 32);
        unsigned short b = u.b >> (8 * sizeof(u.b) - 16);
        r &= __ucl_assert(a == UCL_UINT32_C(0x00010203));
        r &= __ucl_assert(b == 0x0001);
#  endif
#  if defined(ACC_ENDIAN_LITTLE_ENDIAN)
        ucl_uint32 a = (ucl_uint32) (u.a & UCL_UINT32_C(0xffffffff));
        unsigned short b = (unsigned short) (u.b & 0xffff);
        r &= __ucl_assert(a == UCL_UINT32_C(0x03020100));
        r &= __ucl_assert(b == 0x0100);
#  endif
    }
#endif

    /* check that unaligned memory access works as expected */
#if defined(UA_GET2)
    if (r == 1)
    {
        unsigned short b[4];
        for (i = 0; i < 4; i++)
            b[i] = UA_GET2(&u.x[i]);
#  if defined(ACC_ENDIAN_LITTLE_ENDIAN)
        r &= __ucl_assert(b[0] == 0x0100);
        r &= __ucl_assert(b[1] == 0x0201);
        r &= __ucl_assert(b[2] == 0x0302);
        r &= __ucl_assert(b[3] == 0x0403);
#  endif
#  if defined(ACC_ENDIAN_BIG_ENDIAN)
        r &= __ucl_assert(b[0] == 0x0001);
        r &= __ucl_assert(b[1] == 0x0102);
        r &= __ucl_assert(b[2] == 0x0203);
        r &= __ucl_assert(b[3] == 0x0304);
#  endif
        ACC_UNUSED(b);
    }
#endif

#if defined(UA_GET4)
    if (r == 1)
    {
        ucl_uint32 a[4];
        for (i = 0; i < 4; i++)
            a[i] = UA_GET4(&u.x[i]);
#  if defined(ACC_ENDIAN_LITTLE_ENDIAN)
        r &= __ucl_assert(a[0] == UCL_UINT32_C(0x03020100));
        r &= __ucl_assert(a[1] == UCL_UINT32_C(0x04030201));
        r &= __ucl_assert(a[2] == UCL_UINT32_C(0x05040302));
        r &= __ucl_assert(a[3] == UCL_UINT32_C(0x06050403));
#  endif
#  if defined(ACC_ENDIAN_BIG_ENDIAN)
        r &= __ucl_assert(a[0] == UCL_UINT32_C(0x00010203));
        r &= __ucl_assert(a[1] == UCL_UINT32_C(0x01020304));
        r &= __ucl_assert(a[2] == UCL_UINT32_C(0x02030405));
        r &= __ucl_assert(a[3] == UCL_UINT32_C(0x03040506));
#  endif
        ACC_UNUSED(a);
    }
#endif

    /* check the ucl_adler32() function */
    if (r == 1)
    {
        ucl_uint32 adler;
        adler = ucl_adler32(0, NULL, 0);
        adler = ucl_adler32(adler, ucl_copyright(), 195);
        r &= __ucl_assert(adler == UCL_UINT32_C(0x52ca3a75));
    }

    /* check for the gcc schedule-insns optimization bug */
    if (r == 1)
    {
        r &= __ucl_assert(!schedule_insns_bug());
    }

    /* check for the gcc strength-reduce optimization bug */
    if (r == 1)
    {
        static int x[3];
        static unsigned xn = 3;
        register unsigned j;

        for (j = 0; j < xn; j++)
            x[j] = (int)j - 3;
        r &= __ucl_assert(!strength_reduce_bug(x));
    }

    /* now for the low-level pointer checks */
    if (r == 1)
    {
        r &= ptr_check();
    }

    ACC_UNUSED(u);
    return r == 1 ? UCL_E_OK : UCL_E_ERROR;
}


static ucl_bool schedule_insns_bug(void)
{
#if defined(__UCL_CHECKER)
    /* for some reason checker complains about uninitialized memory access */
    return 0;
#else
    const int clone[] = {1, 2, 0};
    const int *q;
    q = clone;
    return (*q) ? 0 : 1;
#endif
}


static ucl_bool strength_reduce_bug(int *x)
{
#if 1 && (ACC_CC_DMC || ACC_CC_SYMANTECC || ACC_CC_ZORTECHC)
    return 0;
#else
    return x[0] != -3 || x[1] != -2 || x[2] != -1;
#endif
}


/***********************************************************************
//
************************************************************************/

int __ucl_init_done = 0;

UCL_PUBLIC(int)
__ucl_init2(ucl_uint32 v, int s1, int s2, int s3, int s4, int s5,
                          int s6, int s7, int s8, int s9)
{
    int r;

#if (ACC_CC_MSC && ((_MSC_VER) < 700))
#else
#define ACC_WANT_CHK 1
#include "xucldecoder_acc.h"
#undef ACC_WANT_CHK
#undef ACCCHK_ASSERT
#endif

    __ucl_init_done = 1;

    if (v == 0)
        return UCL_E_ERROR;

    r = (s1 == -1 || s1 == (int) sizeof(short)) &&
        (s2 == -1 || s2 == (int) sizeof(int)) &&
        (s3 == -1 || s3 == (int) sizeof(long)) &&
        (s4 == -1 || s4 == (int) sizeof(ucl_uint32)) &&
        (s5 == -1 || s5 == (int) sizeof(ucl_uint)) &&
        (s6 == -1 || s6 > 0) &&
        (s7 == -1 || s7 == (int) sizeof(char *)) &&
        (s8 == -1 || s8 == (int) sizeof(ucl_voidp)) &&
        (s9 == -1 || s9 == (int) sizeof(ucl_compress_t));
    if (!r)
        return UCL_E_ERROR;

    r = _ucl_config_check();
    if (r != UCL_E_OK)
        return r;

    return r;
}

/***********************************************************************
// Windows 16 bit + Watcom C + DLL
************************************************************************/

#if (ACC_OS_WIN16 && ACC_CC_WATCOMC) && defined(__SW_BD)

/* don't pull in <windows.h> - we don't need it */
#if 0
BOOL FAR PASCAL LibMain ( HANDLE hInstance, WORD wDataSegment,
                          WORD wHeapSize, LPSTR lpszCmdLine )
#else
int __far __pascal LibMain ( int a, short b, short c, long d )
#endif
{
    ACC_UNUSED(a); ACC_UNUSED(b); ACC_UNUSED(c); ACC_UNUSED(d);
    return 1;
}

#endif


/***********************************************************************
//
************************************************************************/

UCL_PUBLIC(ucl_uintptr_t)
__ucl_ptr_linear(const ucl_voidp ptr)
{
    ucl_uintptr_t p;

#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)
    p = (((ucl_uintptr_t)(ACC_FP_SEG(ptr))) << (16 - ACC_MM_AHSHIFT)) + (ACC_FP_OFF(ptr));
#else
    p = PTR_LINEAR(ptr);
#endif

    return p;
}


/***********************************************************************
//
************************************************************************/

UCL_PUBLIC(unsigned)
__ucl_align_gap(const ucl_voidp ptr, ucl_uint size)
{
#if defined(__CHERI__) && defined(__CHERI_PURE_CAPABILITY__) && defined(__PTRADDR_TYPE__)
    typedef __PTRADDR_TYPE__ my_ptraddr_t;
#else
    typedef ucl_uintptr_t my_ptraddr_t;
#endif
    my_ptraddr_t p, s, n;

    assert(size > 0);

#if defined(__CHERI__) && defined(__CHERI_PURE_CAPABILITY__) && defined(__PTRADDR_TYPE__)
    p = __builtin_cheri_address_get(ptr);
#else
    p = __ucl_ptr_linear(ptr);
#endif
    s = (my_ptraddr_t) (size - 1);
#if 0
    assert((size & (size - 1)) == 0);
    n = ((p + s) & ~s) - p;
#else
    n = (((p + s) / size) * size) - p;
#endif

    assert((long)n >= 0);
    assert(n <= s);

    return (unsigned)n;
}

#undef ucl_memcmp
#undef ucl_memcpy
#undef ucl_memmove
#undef ucl_memset


/***********************************************************************
// slow but portable <string.h> stuff, only used in assertions
************************************************************************/

#if !defined(__UCL_MMODEL_HUGE)
#  undef ACC_HAVE_MM_HUGE_PTR
#endif
#define acc_hsize_t             ucl_uint
#define acc_hvoid_p             ucl_voidp
#define acc_hbyte_p             ucl_bytep
#define ACCLIB_PUBLIC(r,f)      UCL_PUBLIC(r) f
#define acc_hmemcmp             ucl_memcmp
#define acc_hmemcpy             ucl_memcpy
#define acc_hmemmove            ucl_memmove
#define acc_hmemset             ucl_memset
#define ACC_WANT_HMEMCPY 1
#include "xucldecoder_acc.h"
#undef ACC_WANT_HMEMCPY
#undef ACCLIB_PUBLIC


/*
vi:ts=4:et
*/
#undef UCL_DO1
#undef UCL_DO2
#undef UCL_DO4
#undef UCL_DO8
#undef UCL_DO16

/***********************************************************************
//
************************************************************************/

UCL_PUBLIC(ucl_bool)
ucl_assert(int expr)
{
    return (expr) ? 1 : 0;
}


/***********************************************************************
//
************************************************************************/

/* If you use the UCL library in a product, you *must* keep this
 * copyright string in the executable of your product.
.*/

static const char __ucl_copyright[] =
    "\r\n\n"
    "UCL data compression library.\n"
    "$Copyright: UCL (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004 Markus Franz Xaver Johannes Oberhumer\n"
    "<markus@oberhumer.com>\n"
    "http://www.oberhumer.com $\n\n"
    "$Id: UCL version: v" UCL_VERSION_STRING ", " UCL_VERSION_DATE " $\n"
    //"$Built: " __DATE__ " " __TIME__ " $\n"
#if defined(UCL_CONFIG_BUILD_DATE)
    "$Built: " UCL_CONFIG_BUILD_DATE " $\n"
#endif
    "$Info: " ACC_INFO_OS
#if defined(ACC_INFO_OS_POSIX)
    "/" ACC_INFO_OS_POSIX
#endif
    " " ACC_INFO_ARCH
#if defined(ACC_INFO_ENDIAN)
    "/" ACC_INFO_ENDIAN
#endif
    " " ACC_INFO_MM
    " " ACC_INFO_CC
#if defined(ACC_INFO_CCVER)
    " " ACC_INFO_CCVER
#endif
    " $\n";

UCL_PUBLIC(const ucl_bytep)
ucl_copyright(void)
{
#if (ACC_OS_DOS16 && ACC_CC_TURBOC)
    return (ucl_voidp) __ucl_copyright;
#else
    return (const ucl_bytep) __ucl_copyright;
#endif
}

UCL_PUBLIC(ucl_uint32)
ucl_version(void)
{
    return UCL_VERSION;
}

UCL_PUBLIC(const char *)
ucl_version_string(void)
{
    return UCL_VERSION_STRING;
}

UCL_PUBLIC(const char *)
ucl_version_date(void)
{
    return UCL_VERSION_DATE;
}

UCL_PUBLIC(const ucl_charp)
_ucl_version_string(void)
{
    return UCL_VERSION_STRING;
}

UCL_PUBLIC(const ucl_charp)
_ucl_version_date(void)
{
    return UCL_VERSION_DATE;
}


/***********************************************************************
// adler32 checksum
// adapted from free code by Mark Adler <madler@alumni.caltech.edu>
// see http://www.cdrom.com/pub/infozip/zlib/
************************************************************************/

#define UCL_BASE 65521u /* largest prime smaller than 65536 */
#define UCL_NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define UCL_DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define UCL_DO2(buf,i)  UCL_DO1(buf,i); UCL_DO1(buf,i+1);
#define UCL_DO4(buf,i)  UCL_DO2(buf,i); UCL_DO2(buf,i+2);
#define UCL_DO8(buf,i)  UCL_DO4(buf,i); UCL_DO4(buf,i+4);
#define UCL_DO16(buf,i) UCL_DO8(buf,i); UCL_DO8(buf,i+8);

UCL_PUBLIC(ucl_uint32)
ucl_adler32(ucl_uint32 adler, const ucl_bytep buf, ucl_uint len)
{
    ucl_uint32 s1 = adler & 0xffff;
    ucl_uint32 s2 = (adler >> 16) & 0xffff;
    int k;

    if (buf == NULL)
        return 1;

    while (len > 0)
    {
        k = len < UCL_NMAX ? (int) len : UCL_NMAX;
        len -= k;
        if (k >= 16) do
        {
            UCL_DO16(buf,0);
            buf += 16;
            k -= 16;
        } while (k >= 16);
        if (k != 0) do
        {
            s1 += *buf++;
            s2 += s1;
        } while (--k > 0);
        s1 %= UCL_BASE;
        s2 %= UCL_BASE;
    }
    return (s2 << 16) | s1;
}


/*
vi:ts=4:et
*/
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace {
typedef int (*UCL_DECOMPRESS_ROUTINE)(const unsigned char *pSrc, ucl_uint nSrcSize, unsigned char *pDst, ucl_uintp pnDstSize, ucl_voidp pWorkMem);

struct UCL_METHOD_RECORD {
    XUCLDecoder::METHOD method;
    UCL_DECOMPRESS_ROUTINE pRoutine;
};

static const UCL_METHOD_RECORD g_methodRecords[] = {
    {XUCLDecoder::METHOD_NRV2B_8, ucl_nrv2b_decompress_safe_8},
    {XUCLDecoder::METHOD_NRV2B_LE16, ucl_nrv2b_decompress_safe_le16},
    {XUCLDecoder::METHOD_NRV2B_LE32, ucl_nrv2b_decompress_safe_le32},
    {XUCLDecoder::METHOD_NRV2D_8, ucl_nrv2d_decompress_safe_8},
    {XUCLDecoder::METHOD_NRV2D_LE16, ucl_nrv2d_decompress_safe_le16},
    {XUCLDecoder::METHOD_NRV2D_LE32, ucl_nrv2d_decompress_safe_le32},
    {XUCLDecoder::METHOD_NRV2E_8, ucl_nrv2e_decompress_safe_8},
    {XUCLDecoder::METHOD_NRV2E_LE16, ucl_nrv2e_decompress_safe_le16},
    {XUCLDecoder::METHOD_NRV2E_LE32, ucl_nrv2e_decompress_safe_le32},
};

static UCL_DECOMPRESS_ROUTINE _getDecompressRoutine(XUCLDecoder::METHOD method)
{
    qint32 nCount = sizeof(g_methodRecords) / sizeof(g_methodRecords[0]);

    for (qint32 i = 0; i < nCount; i++) {
        if (g_methodRecords[i].method == method) {
            return g_methodRecords[i].pRoutine;
        }
    }

    return nullptr;
}

static bool _decompressBuffer(UCL_DECOMPRESS_ROUTINE pRoutine, const unsigned char *pSrc, quint32 nSrcSize, unsigned char *pDst, quint32 *pnDstSize)
{
    if ((!pRoutine) || (!pSrc) || (!pDst) || (!pnDstSize)) {
        return false;
    }

    ucl_uint nOutputSize = (ucl_uint)(*pnDstSize);
    int nResult = pRoutine(pSrc, (ucl_uint)nSrcSize, pDst, &nOutputSize, nullptr);

    *pnDstSize = (quint32)nOutputSize;

    return (nResult == UCL_E_OK);
}

static bool _getMethodFromState(const XBinary::DATAPROCESS_STATE *pDecompressState, XUCLDecoder::METHOD *pMethod)
{
    QVariant vMethod = pDecompressState->mapProperties.value(XBinary::FPART_PROP_TYPE);

    if (!vMethod.isValid()) {
        vMethod = pDecompressState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES);
    }

    bool bIsValid = false;
    qint32 nMethod = vMethod.toInt(&bIsValid);

    if ((!bIsValid) || (nMethod < XUCLDecoder::METHOD_NRV2B_8) || (nMethod > XUCLDecoder::METHOD_NRV2E_LE32)) {
        return false;
    }

    *pMethod = (XUCLDecoder::METHOD)nMethod;

    return true;
}

static bool _readInputData(XBinary::DATAPROCESS_STATE *pDecompressState, QByteArray *pbaInput, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pbaInput) {
        qint32 nChunkSize = XBinary::getBufferSize(pPdStruct);
        qint64 nRemaining = pDecompressState->nInputLimit;

        pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);

        pbaInput->clear();

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint32 nReadSize = nChunkSize;

            if (nRemaining != -1) {
                if (nRemaining <= 0) {
                    bResult = true;
                    break;
                }

                nReadSize = qMin((qint32)nRemaining, nChunkSize);
            }

            QByteArray baChunk = pDecompressState->pDeviceInput->read(nReadSize);

            if (baChunk.size() < 0) {
                pDecompressState->bReadError = true;
                break;
            }

            if (baChunk.isEmpty()) {
                bResult = (nRemaining == -1) || (nRemaining == 0);

                if (!bResult) {
                    pDecompressState->bReadError = true;
                }

                break;
            }

            pbaInput->append(baChunk);
            pDecompressState->nCountInput += baChunk.size();

            if (nRemaining != -1) {
                nRemaining -= baChunk.size();

                if (nRemaining == 0) {
                    bResult = true;
                    break;
                }
            }
        }
    }

    return bResult;
}
}  // namespace

bool XUCLDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput) {
        pDecompressState->bReadError = false;
        pDecompressState->bWriteError = false;
        pDecompressState->nCountInput = 0;
        pDecompressState->nCountOutput = 0;

        XUCLDecoder::METHOD method = XUCLDecoder::METHOD_NRV2B_8;

        if (_getMethodFromState(pDecompressState, &method)) {
            qint64 nExpectedOutputSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();

            if ((nExpectedOutputSize <= 0) && (pDecompressState->nProcessedLimit > 0)) {
                nExpectedOutputSize = pDecompressState->nProcessedLimit;
            }

            if (nExpectedOutputSize > 0) {
                QByteArray baInput;

                if (_readInputData(pDecompressState, &baInput, pPdStruct)) {
                    QByteArray baOutput;
                    baOutput.resize(nExpectedOutputSize);

                    quint32 nDecodedSize = (quint32)nExpectedOutputSize;
                    UCL_DECOMPRESS_ROUTINE pRoutine = _getDecompressRoutine(method);

                    if (_decompressBuffer(pRoutine, (const unsigned char *)baInput.constData(), (quint32)baInput.size(), (unsigned char *)baOutput.data(), &nDecodedSize)) {
                        if (baOutput.size() != (qint32)nDecodedSize) {
                            baOutput.resize(nDecodedSize);
                        }

                        pDecompressState->pDeviceOutput->seek(0);
                        XBinary::_writeDevice(baOutput.data(), baOutput.size(), pDecompressState);

                        bResult = !pDecompressState->bWriteError;
                    }
                }
            }
        }
    }

    return bResult;
}
