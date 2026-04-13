#ifndef __ACC_H_INCLUDED
#define __ACC_H_INCLUDED 1

#define ACC_VERSION     20040715L

#if !defined(ACC_CONFIG_INCLUDE)
#  define ACC_CONFIG_INCLUDE(file)     file
#endif


#if defined(__CYGWIN32__) && !defined(__CYGWIN__)
#  define __CYGWIN__ __CYGWIN32__
#endif
#if defined(__IBMCPP__) && !defined(__IBMC__)
#  define __IBMC__ __IBMCPP__
#endif
#if defined(__ICL) && defined(_WIN32) && !defined(__INTEL_COMPILER)
#  define __INTEL_COMPILER __ICL
#endif

#if 1 && defined(__INTERIX) && defined(__GNUC__) && !defined(_ALL_SOURCE)
#  define _ALL_SOURCE 1
#endif

/* disable pedantic warnings for undefined preprocessing symbols */
#if defined(__INTEL_COMPILER) && defined(__linux__)
#  pragma warning(disable: 193)
#endif
#if defined(__KEIL__) && defined(__C166__)
#  pragma warning disable = 322
#elif 0 && defined(__C251__)
#  pragma warning disable = 322
#endif
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && !defined(__MWERKS__)
#  if (_MSC_VER >= 1300)
#    pragma warning(disable: 4668)
#  endif
#endif
#if 0 && defined(__WATCOMC__)
#  if (__WATCOMC__ < 1060)
#    pragma warning 203 9
#  endif
#endif


#include <limits.h>

/*************************************************************************
// merged from xucldecoder_acc_init.h
**************************************************************************/

/***********************************************************************
// preprocessor
************************************************************************/

/* workaround for preprocessor bugs in some compilers */
#if 0
#define ACC_0xffffL             0xfffful
#define ACC_0xffffffffL         0xfffffffful
#else
#define ACC_0xffffL             65535ul
#define ACC_0xffffffffL         4294967295ul
#endif

/* some things we cannot work around */
#if (ACC_0xffffL == ACC_0xffffffffL)
#  error "your preprocessor is broken 1"
#endif
#if (16ul * 16384ul != 262144ul)
#  error "your preprocessor is broken 2"
#endif
#if 0
#if (32767 >= 4294967295ul)
#  error "your preprocessor is broken 3"
#endif
#if (65535u >= 4294967295ul)
#  error "your preprocessor is broken 4"
#endif
#endif


/***********************************************************************
// try to detect specific compilers
************************************************************************/

#if (UINT_MAX == ACC_0xffffL)
#if defined(__ZTC__) && defined(__I86__) && !defined(__OS2__)
#  if !defined(MSDOS)
#    define MSDOS 1
#  endif
#  if !defined(_MSDOS)
#    define _MSDOS 1
#  endif
#elif defined(__VERSION) && defined(MB_LEN_MAX)
#  if (__VERSION == 520) && (MB_LEN_MAX == 1)
#    if !defined(__AZTEC_C__)
#      define __AZTEC_C__ __VERSION
#    endif
#    if !defined(__DOS__)
#      define __DOS__ 1
#    endif
#  endif
#endif
#endif


/***********************************************************************
// fix incorrect and missing stuff
************************************************************************/

/* Microsoft C does not correctly define ptrdiff_t for
 * the 16-bit huge memory model.
 */
#if defined(_MSC_VER) && defined(M_I86HM) && (UINT_MAX == ACC_0xffffL)
#  define ptrdiff_t long
#  define _PTRDIFF_T_DEFINED
#endif


/* Fix old compiler versions. */
#if (UINT_MAX == ACC_0xffffL)
#  undef __ACC_RENAME_A
#  undef __ACC_RENAME_B
#  if defined(__AZTEC_C__) && defined(__DOS__)
#    define __ACC_RENAME_A 1
#  elif defined(_MSC_VER) && defined(MSDOS)
#    if (_MSC_VER < 600)
#      define __ACC_RENAME_A 1
#    elif (_MSC_VER < 700)
#      define __ACC_RENAME_B 1
#    endif
#  elif defined(__TSC__) && defined(__OS2__)
#    define __ACC_RENAME_A 1
#  elif defined(__MSDOS__) && defined(__TURBOC__) && (__TURBOC__ < 0x0410)
#    define __ACC_RENAME_A 1
#  elif defined(__PACIFIC__) && defined(DOS)
#    if !defined(__far)
#      define __far far
#    endif
#    if !defined(__near)
#      define __near near
#    endif
#  endif
#  if defined(__ACC_RENAME_A)
#    if !defined(__cdecl)
#      define __cdecl cdecl
#    endif
#    if !defined(__far)
#      define __far far
#    endif
#    if !defined(__huge)
#      define __huge huge
#    endif
#    if !defined(__near)
#      define __near near
#    endif
#    if !defined(__pascal)
#      define __pascal pascal
#    endif
#    if !defined(__huge)
#      define __huge huge
#    endif
#  elif defined(__ACC_RENAME_B)
#    if !defined(__cdecl)
#      define __cdecl _cdecl
#    endif
#    if !defined(__far)
#      define __far _far
#    endif
#    if !defined(__huge)
#      define __huge _huge
#    endif
#    if !defined(__near)
#      define __near _near
#    endif
#    if !defined(__pascal)
#      define __pascal _pascal
#    endif
#  elif (defined(__PUREC__) || defined(__TURBOC__)) && defined(__TOS__)
#    if !defined(__cdecl)
#      define __cdecl cdecl
#    endif
#    if !defined(__pascal)
#      define __pascal pascal
#    endif
#  endif
#  undef __ACC_RENAME_A
#  undef __ACC_RENAME_B
#endif


#if (UINT_MAX == ACC_0xffffL)
#if defined(__AZTEC_C__) && defined(__DOS__)
#  define ACC_BROKEN_CDECL_ALT_SYNTAX 1
#elif defined(_MSC_VER) && defined(MSDOS)
#  if (_MSC_VER < 600)
#    define ACC_BROKEN_INTEGRAL_CONSTANTS 1
#  endif
#  if (_MSC_VER < 700)
#    define ACC_BROKEN_INTEGRAL_PROMOTION 1
#    define ACC_BROKEN_SIZEOF 1
#  endif
#elif defined(__PACIFIC__) && defined(DOS)
#  define ACC_BROKEN_INTEGRAL_CONSTANTS 1
#elif defined(__TURBOC__) && defined(__MSDOS__)
#  if (__TURBOC__ < 0x0150)
#    define ACC_BROKEN_CDECL_ALT_SYNTAX 1
#    define ACC_BROKEN_INTEGRAL_CONSTANTS 1
#    define ACC_BROKEN_INTEGRAL_PROMOTION 1
#  endif
#  if (__TURBOC__ < 0x0200)
#    define ACC_BROKEN_SIZEOF 1
#  endif
#  if (__TURBOC__ < 0x0400) && defined(__cplusplus)
#    define ACC_BROKEN_CDECL_ALT_SYNTAX 1
#  endif
#elif (defined(__PUREC__) || defined(__TURBOC__)) && defined(__TOS__)
#  define ACC_BROKEN_CDECL_ALT_SYNTAX 1
#  define ACC_BROKEN_SIZEOF 1
#endif
#endif
#if defined(__WATCOMC__) && (__WATCOMC__ < 900)
#  define ACC_BROKEN_INTEGRAL_CONSTANTS 1
#endif


/***********************************************************************
// ANSI C preprocessor macros (cpp)
************************************************************************/

#define ACC_CPP_STRINGIZE(x)            #x
#define ACC_CPP_MACRO_EXPAND(x)         ACC_CPP_STRINGIZE(x)

/* concatenate */
#define ACC_CPP_CONCAT2(a,b)            a ## b
#define ACC_CPP_CONCAT3(a,b,c)          a ## b ## c
#define ACC_CPP_CONCAT4(a,b,c,d)        a ## b ## c ## d
#define ACC_CPP_CONCAT5(a,b,c,d,e)      a ## b ## c ## d ## e

/* expand and concatenate (by using one level of indirection) */
#define ACC_CPP_ECONCAT2(a,b)           ACC_CPP_CONCAT2(a,b)
#define ACC_CPP_ECONCAT3(a,b,c)         ACC_CPP_CONCAT3(a,b,c)
#define ACC_CPP_ECONCAT4(a,b,c,d)       ACC_CPP_CONCAT4(a,b,c,d)
#define ACC_CPP_ECONCAT5(a,b,c,d,e)     ACC_CPP_CONCAT5(a,b,c,d,e)


/***********************************************************************
// stdc macros
************************************************************************/

#if defined(__cplusplus)
#  undef __STDC_CONSTANT_MACROS
#  undef __STDC_LIMIT_MACROS
#  define __STDC_CONSTANT_MACROS 1
#  define __STDC_LIMIT_MACROS 1
#endif


/***********************************************************************
// misc macros
************************************************************************/

#if defined(__cplusplus)
#  define ACC_EXTERN_C extern "C"
#else
#  define ACC_EXTERN_C extern
#endif


/*
vi:ts=4:et
*/

/*************************************************************************
// merged from xucldecoder_acc_os.h
**************************************************************************/

/*
 * Operating System - exactly one of:
 *
 *   ACC_OS_POSIX           [default]
 *   ACC_OS_AMIGAOS
 *   ACC_OS_BEOS
 *   ACC_OS_CYGWIN          hybrid WIN32 and POSIX
 *   ACC_OS_DOS16           16-bit DOS (segmented memory model)
 *   ACC_OS_DOS32
 *   ACC_OS_EMX             hybrid OS/2, DOS32, WIN32 (with RSX) and POSIX
 *   ACC_OS_MACCLASSIC      Macintosh Classic
 *   ACC_OS_PALMOS
 *   ACC_OS_OS2             OS/2
 *   ACC_OS_OS216           16-bit OS/2 1.x (segmented memory model)
 *   ACC_OS_QNX
 *   ACC_OS_RISCOS
 *   ACC_OS_TOS             Atari TOS / MiNT
 *   ACC_OS_VMS
 *   ACC_OS_WIN16           16-bit Windows 3.x (segmented memory model)
 *   ACC_OS_WIN32
 *   ACC_OS_WIN64           64-bit Windows (LLP64 programming model)
 */


#if defined(__CYGWIN__) && defined(__GNUC__)
#  define ACC_OS_CYGWIN         1
#  define ACC_INFO_OS           "cygwin"
#elif defined(__EMX__) && defined(__GNUC__)
#  define ACC_OS_EMX            1
#  define ACC_INFO_OS           "emx"
#elif defined(__BEOS__)
#  define ACC_OS_BEOS           1
#  define ACC_INFO_OS           "beos"
#elif defined(__QNX__)
#  define ACC_OS_QNX            1
#  define ACC_INFO_OS           "qnx"
#elif defined(__BORLANDC__) && defined(__DPMI32__) && (__BORLANDC__ >= 0x0460)
#  define ACC_OS_DOS32          1
#  define ACC_INFO_OS           "dos32"
#elif defined(__BORLANDC__) && defined(__DPMI16__)
#  define ACC_OS_DOS16          1
#  define ACC_INFO_OS           "dos16"
#elif defined(__ZTC__) && defined(DOS386)
#  define ACC_OS_DOS32          1
#  define ACC_INFO_OS           "dos32"
#elif defined(__OS2__) || defined(__OS2V2__)
#  if (UINT_MAX == ACC_0xffffL)
#    define ACC_OS_OS216        1
#    define ACC_INFO_OS         "os216"
#  elif (UINT_MAX == ACC_0xffffffffL)
#    define ACC_OS_OS2          1
#    define ACC_INFO_OS         "os2"
#  else
#    error "check your limits.h header"
#  endif
#elif defined(__WIN64__) || defined(_WIN64) || defined(WIN64)
#  define ACC_OS_WIN64          1
#  define ACC_INFO_OS           "win64"
#elif defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS_386__)
#  define ACC_OS_WIN32          1
#  define ACC_INFO_OS           "win32"
#elif defined(__MWERKS__) && defined(__INTEL__)
#  define ACC_OS_WIN32          1
#  define ACC_INFO_OS           "win32"
#elif defined(__WINDOWS__) || defined(_WINDOWS) || defined(_Windows)
#  if (UINT_MAX == ACC_0xffffL)
#    define ACC_OS_WIN16        1
#    define ACC_INFO_OS         "win16"
#  elif (UINT_MAX == ACC_0xffffffffL)
#    define ACC_OS_WIN32        1
#    define ACC_INFO_OS         "win32"
#  else
#    error "check your limits.h header"
#  endif
#elif defined(__DOS__) || defined(__MSDOS__) || defined(_MSDOS) || defined(MSDOS) || (defined(__PACIFIC__) && defined(DOS))
#  if (UINT_MAX == ACC_0xffffL)
#    define ACC_OS_DOS16        1
#    define ACC_INFO_OS         "dos16"
#  elif (UINT_MAX == ACC_0xffffffffL)
#    define ACC_OS_DOS32        1
#    define ACC_INFO_OS         "dos32"
#  else
#    error "check your limits.h header"
#  endif
#elif defined(__WATCOMC__)
#  if defined(__NT__) && (UINT_MAX == ACC_0xffffL)
     /* wcl: NT host defaults to DOS target */
#    define ACC_OS_DOS16        1
#    define ACC_INFO_OS         "dos16"
#  elif defined(__NT__) && (__WATCOMC__ < 1100)
     /* wcl386: Watcom C 11 defines _WIN32 */
#    define ACC_OS_WIN32        1
#    define ACC_INFO_OS         "win32"
#  else
#    error "please specify a target using the -bt compiler option"
#  endif
#elif defined(__palmos__)
#  if (UINT_MAX == ACC_0xffffL)
#    define ACC_OS_PALMOS       1
#    define ACC_INFO_OS         "palmos"
#  else
#    error "check your limits.h header"
#  endif
#elif defined(__TOS__) || defined(__atarist__)
#  define ACC_OS_TOS            1
#  define ACC_INFO_OS           "tos"
#elif defined(macintosh) && !defined(__ppc__)
#  define ACC_OS_MACCLASSIC     1
#  define ACC_INFO_OS           "macclassic"
#elif defined(__VMS)
#  define ACC_OS_VMS            1
#  define ACC_INFO_OS           "vms"
#else
#  define ACC_OS_POSIX          1
#  define ACC_INFO_OS           "posix"
#endif


#if (ACC_OS_POSIX)
#  if defined(_AIX) || defined(__AIX__) || defined(__aix__)
#    define ACC_OS_POSIX_AIX        1
#    define ACC_INFO_OS_POSIX       "aix"
#  elif defined(__FreeBSD__)
#    define ACC_OS_POSIX_FREEBSD    1
#    define ACC_INFO_OS_POSIX       "freebsd"
#  elif defined(__hpux__) || defined(__hpux)
#    define ACC_OS_POSIX_HPUX       1
#    define ACC_INFO_OS_POSIX       "hpux"
#  elif defined(__INTERIX)
#    define ACC_OS_POSIX_INTERIX    1
#    define ACC_INFO_OS_POSIX       "interix"
#  elif defined(__IRIX__) || defined(__irix__)
#    define ACC_OS_POSIX_IRIX       1
#    define ACC_INFO_OS_POSIX       "irix"
#  elif defined(__linux__) || defined(__linux)
#    define ACC_OS_POSIX_LINUX      1
#    define ACC_INFO_OS_POSIX       "linux"
#  elif defined(__APPLE__) || defined(__MACOS__)
#    define ACC_OS_POSIX_MACOSX     1
#    define ACC_INFO_OS_POSIX       "macosx"
#  elif defined(__NetBSD__)
#    define ACC_OS_POSIX_NETBSD     1
#    define ACC_INFO_OS_POSIX       "netbsd"
#  elif defined(__OpenBSD__)
#    define ACC_OS_POSIX_OPENBSD    1
#    define ACC_INFO_OS_POSIX       "openbsd"
#  elif defined(__osf__)
#    define ACC_OS_POSIX_OSF        1
#    define ACC_INFO_OS_POSIX       "osf"
#  elif defined(__solaris__) || defined(__sun)
#    if defined(__SVR4) || defined(__svr4__)
#      define ACC_OS_POSIX_SOLARIS  1
#      define ACC_INFO_OS_POSIX     "solaris"
#    else
#      define ACC_OS_POSIX_SUNOS    1
#      define ACC_INFO_OS_POSIX     "sunos"
#    endif
#  elif defined(__ultrix__) || defined(__ultrix)
#    define ACC_OS_POSIX_ULTRIX     1
#    define ACC_INFO_OS_POSIX       "ultrix"
#  else
#    define ACC_OS_POSIX_UNKNOWN    1
#    define ACC_INFO_OS_POSIX       "unknown"
#  endif
#endif


#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)
#  if (UINT_MAX != ACC_0xffffL)
#    error "this should not happen"
#  endif
#  if (ULONG_MAX != ACC_0xffffffffL)
#    error "this should not happen"
#  endif
#endif
#if (ACC_OS_DOS32 || ACC_OS_OS2 || ACC_OS_WIN32 || ACC_OS_WIN64)
#  if (UINT_MAX != ACC_0xffffffffL)
#    error "this should not happen"
#  endif
#  if (ULONG_MAX != ACC_0xffffffffL)
#    error "this should not happen"
#  endif
#endif


/*
vi:ts=4:et
*/

/*************************************************************************
// merged from xucldecoder_acc_cc.h
**************************************************************************/

/*
 * C/C++ Compiler - exactly one of:
 *
 *   ACC_CC_UNKNOWN         [default]
 *   ACC_CC_GNUC
 *   ...
 */

#if defined(CIL) && defined(_GNUCC) && defined(__GNUC__)
#  define ACC_CC_CILLY          1
#  define ACC_INFO_CC           "Cilly"
#  define ACC_INFO_CCVER        "unknown"
#elif defined(__INTEL_COMPILER)
#  define ACC_CC_INTELC         1
#  define ACC_INFO_CC           "Intel C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__INTEL_COMPILER)
#elif defined(__POCC__)
#  define ACC_CC_PELLESC        1
#  define ACC_INFO_CC           "Pelles C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__POCC__)
#elif defined(__GNUC__) && defined(__VERSION__)
#  if defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#    define ACC_CC_GNUC         (__GNUC__ * 0x10000L + __GNUC_MINOR__ * 0x100 + __GNUC_PATCHLEVEL__)
#  elif defined(__GNUC_MINOR__)
#    define ACC_CC_GNUC         (__GNUC__ * 0x10000L + __GNUC_MINOR__ * 0x100)
#  else
#    define ACC_CC_GNUC         (__GNUC__ * 0x10000L)
#  endif
#  define ACC_INFO_CC           "gcc"
#  define ACC_INFO_CCVER        __VERSION__
#elif defined(__AZTEC_C__)
#  define ACC_CC_AZTECC         1
#  define ACC_INFO_CC           "Aztec C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__AZTEC_C__)
#elif defined(__BORLANDC__)
#  define ACC_CC_BORLANDC       1
#  define ACC_INFO_CC           "Borland C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__BORLANDC__)
#elif defined(__DMC__)
#  define ACC_CC_DMC            1
#  define ACC_INFO_CC           "Digital Mars C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__DMC__)
#elif defined(__DECC)
#  define ACC_CC_DECC           1
#  define ACC_INFO_CC           "DEC C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__DECC)
#elif defined(__HIGHC__)
#  define ACC_CC_HIGHC          1
#  define ACC_INFO_CC           "MetaWare High C"
#  define ACC_INFO_CCVER        "unknown"
#elif defined(__IBMC__)
#  define ACC_CC_IBMC           1
#  define ACC_INFO_CC           "IBM C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__IBMC__)
#elif defined(__KEIL__) && defined(__C166__)
#  define ACC_CC_KEILC          1
#  define ACC_INFO_CC           "Keil C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__C166__)
#elif defined(__LCC__)
#  define ACC_CC_LCC            1
#  define ACC_INFO_CC           "lcc"
#  define ACC_INFO_CCVER        "unknown"
#elif defined(_MSC_VER)
#  define ACC_CC_MSC            1
#  define ACC_INFO_CC           "Microsoft C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(_MSC_VER)
#elif defined(__MWERKS__)
#  define ACC_CC_MWERKS         1
#  define ACC_INFO_CC           "Metrowerks C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__MWERKS__)
#elif (defined(__NDPC__) || defined(__NDPX__)) && defined(__i386)
#  define ACC_CC_NDPC           1
#  define ACC_INFO_CC           "Microway NDP C"
#  define ACC_INFO_CCVER        "unknown"
#elif defined(__PACIFIC__)
#  define ACC_CC_PACIFICC       1
#  define ACC_INFO_CC           "Pacific C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__PACIFIC__)
#elif defined(__PGI) && (defined(__linux__) || defined(__WIN32__))
#  define ACC_CC_PGI            1
#  define ACC_INFO_CC           "Portland Group PGI C"
#  define ACC_INFO_CCVER        "unknown"
#elif defined(__PUREC__)
#  define ACC_CC_PUREC          1
#  define ACC_INFO_CC           "Pure C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__PUREC__)
#elif defined(__SC__)
#  define ACC_CC_SYMANTECC      1
#  define ACC_INFO_CC           "Symantec C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__SC__)
#elif defined(__SUNPRO_C)
#  define ACC_CC_SUNPROC        1
#  define ACC_INFO_CC           "Sun C"
#  define ACC_INFO_CCVER        "unknown"
#elif defined(__TINYC__)
#  define ACC_CC_TINYC          1
#  define ACC_INFO_CC           "Tiny C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__TINYC__)
#elif defined(__TSC__)
#  define ACC_CC_TOPSPEEDC      1
#  define ACC_INFO_CC           "TopSpeed C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__TSC__)
#elif defined(__WATCOMC__)
#  define ACC_CC_WATCOMC        1
#  define ACC_INFO_CC           "Watcom C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__WATCOMC__)
#elif defined(__TURBOC__)
#  define ACC_CC_TURBOC         1
#  define ACC_INFO_CC           "Turbo C"
#  define ACC_INFO_CCVER        ACC_CPP_MACRO_EXPAND(__TURBOC__)
#elif defined(__ZTC__)
#  define ACC_CC_ZORTECHC       1
#  define ACC_INFO_CC           "Zortech C"
#  if (__ZTC__ == 0x310)
#    define ACC_INFO_CCVER      "0x310"
#  else
#    define ACC_INFO_CCVER      ACC_CPP_MACRO_EXPAND(__ZTC__)
#  endif
#else
#  define ACC_CC_UNKNOWN        1
#  define ACC_INFO_CC           "unknown"
#  define ACC_INFO_CCVER        "unknown"
#endif


/*
vi:ts=4:et
*/

/*************************************************************************
// merged from xucldecoder_acc_arch.h
**************************************************************************/

/*
 * CPU architecture - exactly one of:
 *
 *   ACC_ARCH_UNKNOWN       [default]
 *   ACC_ARCH_ALPHA
 *   ACC_ARCH_AMD64         aka x86-64 or ia32e
 *   ACC_ARCH_C166
 *   ACC_ARCH_IA16          Intel Architecture (8088, 8086, 80186, 80286)
 *   ACC_ARCH_IA32          Intel Architecture (80386+)
 *   ACC_ARCH_IA64          Intel Architecture (Itanium)
 *   ACC_ARCH_M68K          Motorola 680x0
 *   ACC_ARCH_MCS251
 *   ACC_ARCH_MCS51
 *   ACC_ARCH_PPC64         Power PC
 *   ACC_ARCH_SPARC64
 *
 * Optionally define one of:
 *   ACC_ENDIAN_LITTLE_ENDIAN
 *   ACC_ENDIAN_BIG_ENDIAN
 *
 * Note that this list is not exhaustive - actually we only really care
 * about architectures which allow unaligned memory access at reasonable
 * speed (for the moment this means IA16, IA32 and AMD64).
 */

#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)
#  define ACC_ARCH_IA16             1
#  define ACC_INFO_ARCH             "ia16"
#elif defined(__amd64__) || defined(__x86_64__) || defined(_M_AMD64)
#  define ACC_ARCH_AMD64            1
#  define ACC_INFO_ARCH             "amd64"
#elif (UINT_MAX <= ACC_0xffffL) && defined(__AVR__)
#  define ACC_ARCH_AVR              1
#  define ACC_INFO_ARCH             "avr"
#elif (UINT_MAX == ACC_0xffffL) && defined(__C166__)
#  define ACC_ARCH_C166             1
#  define ACC_INFO_ARCH             "c166"
#elif (UINT_MAX == ACC_0xffffL) && defined(__C251__)
#  define ACC_ARCH_MCS251           1
#  define ACC_INFO_ARCH             "mcs-251"
#elif (UINT_MAX == ACC_0xffffL) && defined(__C51__)
#  define ACC_ARCH_MCS51            1
#  define ACC_INFO_ARCH             "mcs-51"
#elif defined(__386__) || defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(_M_I386)
#  define ACC_ARCH_IA32             1
#  define ACC_INFO_ARCH             "ia32"
#elif (ACC_CC_ZORTECHC && defined(__I86__))
#  define ACC_ARCH_IA32             1
#  define ACC_INFO_ARCH             "ia32"
#elif defined(__ia64__) || defined(__ia64) || defined(_M_IA64)
#  define ACC_ARCH_IA64             1
#  define ACC_INFO_ARCH             "ia64"
#elif (ACC_OS_DOS32 && ACC_CC_HIGHC) && defined(_I386)
#  define ACC_ARCH_IA32             1
#  define ACC_INFO_ARCH             "ia32"
#elif (ACC_OS_DOS32 || ACC_OS_OS2)
#  error "missing define for CPU architecture"
//#elif (ACC_OS_WIN32)
//#  error "missing define for CPU architecture"
//#elif (ACC_OS_WIN64)
/* #  error "missing define for CPU architecture" */
#elif (ACC_OS_TOS) || defined(__m68000__)
#  define ACC_ARCH_M68K             1
#  define ACC_INFO_ARCH             "m68k"
#elif defined(__alpha__) || defined(__alpha)
#  define ACC_ARCH_ALPHA            1
#  define ACC_INFO_ARCH             "alpha"
#elif defined(__ppc64__) || defined(__ppc64)
#  define ACC_ARCH_PPC64            1
#  define ACC_INFO_ARCH             "ppc64"
#elif defined(__sparc64__) || defined(__sparc64)
#  define ACC_ARCH_SPARC64          1
#  define ACC_INFO_ARCH             "sparc64"
#else
#  define ACC_ARCH_UNKNOWN          1
#  define ACC_INFO_ARCH             "unknown"
#endif


#if (ACC_ARCH_AMD64 || ACC_ARCH_IA16 || ACC_ARCH_IA32)
#  define ACC_ENDIAN_LITTLE_ENDIAN  1
#  define ACC_INFO_ENDIAN           "little-endian"
#elif (ACC_ARCH_M68K)
#  define ACC_ENDIAN_BIG_ENDIAN     1
#  define ACC_INFO_ENDIAN           "big-endian"
#endif


#if (ACC_ARCH_IA16)
#  if (UINT_MAX != ACC_0xffffL)
#    error "this should not happen"
#  endif
#  if (ULONG_MAX != ACC_0xffffffffL)
#    error "this should not happen"
#  endif
#endif
#if (ACC_ARCH_IA32)
#  if (UINT_MAX != ACC_0xffffffffL)
#    error "this should not happen"
#  endif
#  if (ULONG_MAX != ACC_0xffffffffL)
#    error "this should not happen"
#  endif
#endif


/*
vi:ts=4:et
*/

/*************************************************************************
// merged from xucldecoder_acc_mm.h
**************************************************************************/

/*
 * Memory Model - exactly one of:
 *
 *   ACC_MM_FLAT            [default]
 *   ACC_MM_TINY
 *   ACC_MM_SMALL
 *   ACC_MM_MEDIUM
 *   ACC_MM_COMPACT
 *   ACC_MM_LARGE
 *   ACC_MM_HUGE
 */

#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)

#if (UINT_MAX != ACC_0xffffL)
#  error "this should not happen"
#endif
#if defined(__TINY__) || defined(M_I86TM) || defined(_M_I86TM)
#  define ACC_MM_TINY           1
#elif defined(__HUGE__) || defined(_HUGE_) || defined(M_I86HM) || defined(_M_I86HM)
#  define ACC_MM_HUGE           1
#elif defined(__SMALL__) || defined(M_I86SM) || defined(_M_I86SM) || defined(SMALL_MODEL)
#  define ACC_MM_SMALL          1
#elif defined(__MEDIUM__) || defined(M_I86MM) || defined(_M_I86MM)
#  define ACC_MM_MEDIUM         1
#elif defined(__COMPACT__) || defined(M_I86CM) || defined(_M_I86CM)
#  define ACC_MM_COMPACT        1
#elif defined(__LARGE__) || defined(M_I86LM) || defined(_M_I86LM) || defined(LARGE_MODEL)
#  define ACC_MM_LARGE          1
#elif (ACC_CC_AZTECC)
#  if defined(_LARGE_CODE) && defined(_LARGE_DATA)
#    define ACC_MM_LARGE        1
#  elif defined(_LARGE_CODE)
#    define ACC_MM_MEDIUM       1
#  elif defined(_LARGE_DATA)
#    define ACC_MM_COMPACT      1
#  else
#    define ACC_MM_SMALL        1
#  endif
#elif (ACC_CC_ZORTECHC && defined(__VCM__))
#  define ACC_MM_LARGE          1
#else
#  error "unknown memory model"
#endif


/* ACC_HAVE_MM_HUGE_PTR   ... working __huge pointers
 * ACC_HAVE_MM_HUGE_ARRAY ... char __huge x[256*1024L] works */
#define ACC_HAVE_MM_HUGE_PTR        1
#define ACC_HAVE_MM_HUGE_ARRAY      1

#if (ACC_MM_TINY)
#  undef ACC_HAVE_MM_HUGE_ARRAY
#endif

#if (ACC_CC_AZTECC || ACC_CC_PACIFICC || ACC_CC_ZORTECHC)
#  undef ACC_HAVE_MM_HUGE_PTR
#  undef ACC_HAVE_MM_HUGE_ARRAY
#elif (ACC_CC_DMC || ACC_CC_SYMANTECC)
#  undef ACC_HAVE_MM_HUGE_ARRAY
#elif (ACC_CC_MSC && defined(_QC))
#  undef ACC_HAVE_MM_HUGE_ARRAY
#  if (_MSC_VER < 600)
#    undef ACC_HAVE_MM_HUGE_PTR
#  endif
#elif (ACC_CC_TURBOC && (__TURBOC__ < 0x0295))
#  undef ACC_HAVE_MM_HUGE_ARRAY
#elif (ACC_CC_WATCOMC && (__WATCOMC__ >= 1200))
   /* pointer arithmetics with __huge arrays seems broken in OpenWatcom 1.x */
#  undef ACC_HAVE_MM_HUGE_ARRAY
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if (ACC_CC_BORLANDC && (__BORLANDC__ >= 0x0200))
   extern void __near __cdecl _AHSHIFT(void);
#  define ACC_MM_AHSHIFT      ((unsigned) _AHSHIFT)
#elif (ACC_CC_DMC || ACC_CC_SYMANTECC || ACC_CC_ZORTECHC)
   extern void __near __cdecl _AHSHIFT(void);
#  define ACC_MM_AHSHIFT      ((unsigned) _AHSHIFT)
#elif (ACC_CC_MSC || ACC_CC_TOPSPEEDC)
   extern void __near __cdecl _AHSHIFT(void);
#  define ACC_MM_AHSHIFT      ((unsigned) _AHSHIFT)
#elif (ACC_CC_TURBOC && (__TURBOC__ >= 0x0295))
   extern void __near __cdecl _AHSHIFT(void);
#  define ACC_MM_AHSHIFT      ((unsigned) _AHSHIFT)
#elif ((ACC_CC_AZTECC || ACC_CC_PACIFICC || ACC_CC_TURBOC) && ACC_OS_DOS16)
#  define ACC_MM_AHSHIFT      12
#elif (ACC_CC_WATCOMC)
   extern unsigned char _HShift;
#  define ACC_MM_AHSHIFT      ((unsigned) _HShift)
#else
#  error "FIXME - implement ACC_MM_AHSHIFT"
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif


#elif (ACC_ARCH_C166)
#if !defined(__MODEL__)
#  error "FIXME - C166 __MODEL__"
#elif ((__MODEL__) == 0)
#  define ACC_MM_SMALL          1
#elif ((__MODEL__) == 1)
#  define ACC_MM_SMALL          1
#elif ((__MODEL__) == 2)
#  define ACC_MM_LARGE          1
#elif ((__MODEL__) == 3)
#  define ACC_MM_TINY           1
#elif ((__MODEL__) == 4)
#  define ACC_MM_XTINY          1
#elif ((__MODEL__) == 5)
#  define ACC_MM_XSMALL         1
#else
#  error "FIXME - C166 __MODEL__"
#endif


#elif (ACC_ARCH_MCS251)
#if !defined(__MODEL__)
#  error "FIXME - MCS251 __MODEL__"
#elif ((__MODEL__) == 0)
#  define ACC_MM_SMALL          1
#elif ((__MODEL__) == 2)
#  define ACC_MM_LARGE          1
#elif ((__MODEL__) == 3)
#  define ACC_MM_TINY           1
#elif ((__MODEL__) == 4)
#  define ACC_MM_XTINY          1
#elif ((__MODEL__) == 5)
#  define ACC_MM_XSMALL         1
#else
#  error "FIXME - MCS251 __MODEL__"
#endif


#elif (ACC_ARCH_MCS51)
#if !defined(__MODEL__)
#  error "FIXME - MCS51 __MODEL__"
#elif ((__MODEL__) == 1)
#  define ACC_MM_SMALL          1
#elif ((__MODEL__) == 2)
#  define ACC_MM_LARGE          1
#elif ((__MODEL__) == 3)
#  define ACC_MM_TINY           1
#elif ((__MODEL__) == 4)
#  define ACC_MM_XTINY          1
#elif ((__MODEL__) == 5)
#  define ACC_MM_XSMALL         1
#else
#  error "FIXME - MCS51 __MODEL__"
#endif

#else

#  define ACC_MM_FLAT           1

#endif


#if (ACC_MM_FLAT)
#  define ACC_INFO_MM           "flat"
#elif (ACC_MM_TINY)
#  define ACC_INFO_MM           "tiny"
#elif (ACC_MM_SMALL)
#  define ACC_INFO_MM           "small"
#elif (ACC_MM_MEDIUM)
#  define ACC_INFO_MM           "medium"
#elif (ACC_MM_COMPACT)
#  define ACC_INFO_MM           "compact"
#elif (ACC_MM_LARGE)
#  define ACC_INFO_MM           "large"
#elif (ACC_MM_HUGE)
#  define ACC_INFO_MM           "huge"
#else
#  error "unknown memory model"
#endif


/*
vi:ts=4:et
*/

/*************************************************************************
// merged from xucldecoder_acc_defs.h
**************************************************************************/

/***********************************************************************
// acc_alignof() / acc_inline
************************************************************************/

#if (ACC_CC_CILLY || ACC_CC_GNUC || ACC_CC_PGI)
#  define acc_alignof(e)        __alignof__(e)
#elif (ACC_CC_INTELC && (__INTEL_COMPILER >= 700))
#  define acc_alignof(e)        __alignof__(e)
#elif (ACC_CC_MSC && (_MSC_VER >= 1300))
#  define acc_alignof(e)        __alignof(e)
#endif

#if (ACC_CC_TURBOC && (__TURBOC__ <= 0x0295))
#elif defined(__cplusplus)
#  define acc_inline            inline
#elif (ACC_CC_BORLANDC && (__BORLANDC__ >= 0x0550))
#  define acc_inline            __inline
#elif (ACC_CC_CILLY || ACC_CC_GNUC || ACC_CC_PGI)
#  define acc_inline            __inline__
#elif (ACC_CC_DMC)
#  define acc_inline            __inline
#elif (ACC_CC_INTELC)
#  define acc_inline            __inline
#elif (ACC_CC_MSC && (_MSC_VER >= 900))
#  define acc_inline            __inline
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#  define acc_inline            inline
#endif


/***********************************************************************
// ACC_UNUSED / ACC_UNUSED_FUNC
************************************************************************/

#if !defined(ACC_UNUSED)
#  if (ACC_CC_BORLANDC && (__BORLANDC__ >= 0x0600))
#    define ACC_UNUSED(var)         ((void) &var)
#  elif (ACC_CC_BORLANDC || ACC_CC_HIGHC || ACC_CC_NDPC || ACC_CC_TURBOC)
#    define ACC_UNUSED(var)         if (&var) ; else
#  elif (ACC_CC_MSC && (_MSC_VER < 900))
#    define ACC_UNUSED(var)         if (&var) ; else
#  elif (ACC_CC_GNUC)
#    define ACC_UNUSED(var)         ((void) var)
#  elif (ACC_CC_KEILC)
#    define ACC_UNUSED(var)
#  else
#    define ACC_UNUSED(var)         ((void) &var)
#  endif
#endif
#if !defined(ACC_UNUSED_FUNC)
#  if (ACC_CC_BORLANDC && (__BORLANDC__ >= 0x0600))
#    define ACC_UNUSED_FUNC(func)   ((void) func)
#  elif (ACC_CC_BORLANDC || ACC_CC_NDPC || ACC_CC_TURBOC)
#    define ACC_UNUSED_FUNC(func)   if (func) ; else
#  elif (ACC_CC_GNUC == 0x030400ul) && defined(__llvm__)
#    define ACC_UNUSED_FUNC(func)   ((void) &func)
#  elif (ACC_CC_MSC && (_MSC_VER < 900))
#    define ACC_UNUSED_FUNC(func)   if (func) ; else
#  elif (ACC_CC_MSC)
#    define ACC_UNUSED_FUNC(func)   ((void) &func)
#  elif (ACC_CC_KEILC)
#    define ACC_UNUSED_FUNC(func)
#  else
#    define ACC_UNUSED_FUNC(func)   ((void) func)
#  endif
#endif


/***********************************************************************
// compile-time-assertions
************************************************************************/

/* This can be put into a header file but may get ignored by some compilers. */
#if !defined(ACC_COMPILE_TIME_ASSERT_HEADER)
#  if defined(__cplusplus) && (__cplusplus+0 >= 201103L)
#    define ACC_COMPILE_TIME_ASSERT_HEADER(e)  static_assert(e, #e);
#  elif defined(__STDC_VERSION__) && (__STDC_VERSION__+0 >= 201112L)
#    define ACC_COMPILE_TIME_ASSERT_HEADER(e)  _Static_assert(e, #e);
#  elif (ACC_CC_AZTECC || ACC_CC_ZORTECHC)
#    define ACC_COMPILE_TIME_ASSERT_HEADER(e)  extern int __acc_cta[1-!(e)];
#  elif (ACC_CC_DMC || ACC_CC_SYMANTECC)
#    define ACC_COMPILE_TIME_ASSERT_HEADER(e)  extern int __acc_cta[1u-2*!(e)];
#  elif (ACC_CC_TURBOC && (__TURBOC__ == 0x0295))
#    define ACC_COMPILE_TIME_ASSERT_HEADER(e)  extern int __acc_cta[1-!(e)];
#  else
#    define ACC_COMPILE_TIME_ASSERT_HEADER(e)  extern int __acc_cta[1-2*!(e)];
#  endif
#endif

/* This must appear within a function body. */
#if !defined(ACC_COMPILE_TIME_ASSERT)
#  if defined(__cplusplus) && (__cplusplus+0 >= 201103L)
#    define ACC_COMPILE_TIME_ASSERT(e)  {static_assert(e, #e);}
#  elif defined(__STDC_VERSION__) && (__STDC_VERSION__+0 >= 201112L)
#    define ACC_COMPILE_TIME_ASSERT(e)  {_Static_assert(e, #e);}
#  elif (ACC_CC_AZTECC)
#    define ACC_COMPILE_TIME_ASSERT(e)  {typedef int __acc_cta_t[1-!(e)];}
#  elif (ACC_CC_DMC || ACC_CC_PACIFICC || ACC_CC_SYMANTECC || ACC_CC_ZORTECHC)
#    define ACC_COMPILE_TIME_ASSERT(e)  switch(0) case 1:case !(e):break;
#  elif (ACC_CC_MSC && (_MSC_VER < 900))
#    define ACC_COMPILE_TIME_ASSERT(e)  switch(0) case 1:case !(e):break;
#  elif (ACC_CC_TURBOC && (__TURBOC__ == 0x0295))
#    define ACC_COMPILE_TIME_ASSERT(e)  switch(0) case 1:case !(e):break;
#  else
#    define ACC_COMPILE_TIME_ASSERT(e)  {typedef int __acc_cta_t[1-2*!(e)];}
#  endif
#endif


/***********************************************************************
// macros
************************************************************************/

#if !defined(__ACC_UINT_MAX)
#  define __ACC_INT_MAX(b)      ((((1l  << ((b)-2)) - 1l)  * 2l)  + 1l)
#  define __ACC_UINT_MAX(b)     ((((1ul << ((b)-1)) - 1ul) * 2ul) + 1ul)
#endif


/***********************************************************************
// get sizes of builtin integral types from <limits.h>
************************************************************************/

#if !defined(__ACC_SHORT_BIT)
#  if (USHRT_MAX == ACC_0xffffL)
#    define __ACC_SHORT_BIT     16
#  elif (USHRT_MAX == ACC_0xffffffffL)
#    define __ACC_SHORT_BIT     32
#  elif (USHRT_MAX == __ACC_UINT_MAX(64))
#    define __ACC_SHORT_BIT     64
#  elif (USHRT_MAX == __ACC_UINT_MAX(128))
#    define __ACC_SHORT_BIT     128
#  else
#    error "check your compiler installation: USHRT_MAX"
#  endif
#endif

#if !defined(__ACC_INT_BIT)
#  if (UINT_MAX == ACC_0xffffL)
#    define __ACC_INT_BIT       16
#  elif (UINT_MAX == ACC_0xffffffffL)
#    define __ACC_INT_BIT       32
#  elif (UINT_MAX == __ACC_UINT_MAX(64))
#    define __ACC_INT_BIT       64
#  elif (UINT_MAX == __ACC_UINT_MAX(128))
#    define __ACC_INT_BIT       128
#  else
#    error "check your compiler installation: UINT_MAX"
#  endif
#endif

#if !defined(__ACC_LONG_BIT)
#  if (ULONG_MAX == ACC_0xffffffffL)
#    define __ACC_LONG_BIT      32
#  elif (ULONG_MAX == __ACC_UINT_MAX(64))
#    define __ACC_LONG_BIT      64
#  elif (ULONG_MAX == __ACC_UINT_MAX(128))
#    define __ACC_LONG_BIT      128
#  else
#    error "check your compiler installation: ULONG_MAX"
#  endif
#endif


/***********************************************************************
// acc_auto.h supplements
************************************************************************/

#if (ACC_OS_CYGWIN || (ACC_OS_EMX && defined(__RSXNT__)) || ACC_OS_WIN32 || ACC_OS_WIN64)
#  if (ACC_CC_WATCOMC && (__WATCOMC__ < 1000))
#  elif (ACC_OS_WIN32 && ACC_CC_GNUC) && defined(__PW32__)
     /* ancient pw32 version */
#  elif ((ACC_OS_CYGWIN || defined(__MINGW32__)) && (ACC_CC_GNUC && (ACC_CC_GNUC < 0x025f00ul)))
     /* ancient cygwin/mingw version */
#  else
#    define ACC_HAVE_WINDOWS_H 1
#  endif
#endif



/*
vi:ts=4:et
*/

#if defined(ACC_CONFIG_NO_HEADER)
#elif defined(ACC_CONFIG_HEADER)
#  include ACC_CONFIG_HEADER
#else

/*************************************************************************
// merged from xucldecoder_acc_auto.h
**************************************************************************/

/*
 * Possible configuration values:
 *
 *   ACC_CONFIG_AUTO_NO_HEADERS
 *   ACC_CONFIG_AUTO_NO_FUNCTIONS
 *   ACC_CONFIG_AUTO_NO_SIZES
 */


/*************************************************************************
// Checks for <stdint.h>
**************************************************************************/

#if !defined(ACC_CONFIG_AUTO_NO_HEADERS)

#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
#  if (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1))
#    define HAVE_STDINT_H 1
#  endif
#elif defined(__dietlibc__)
#  undef HAVE_STDINT_H /* incomplete */
#elif (ACC_CC_BORLANDC) && (__BORLANDC__ >= 0x560)
#  undef HAVE_STDINT_H /* broken & incomplete */
#elif (ACC_CC_DMC) && (__DMC__ >= 0x825)
#  define HAVE_STDINT_H 1
#endif

#if HAVE_STDINT_H
#  include <stdint.h>
#endif

#endif /* !defined(ACC_CONFIG_AUTO_NO_HEADERS) */


/*************************************************************************
// Checks for header files
**************************************************************************/

#if !defined(ACC_CONFIG_AUTO_NO_HEADERS)

#define STDC_HEADERS 1

#define HAVE_ASSERT_H 1
#define HAVE_CTYPE_H 1
#define HAVE_DIRENT_H 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMORY_H 1
#define HAVE_SETJMP_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_UTIME_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1

#undef HAVE_ALLOCA_H
#undef HAVE_CONIO_H
#undef HAVE_DIRECT_H
#undef HAVE_DOS_H
#undef HAVE_IO_H
#undef HAVE_SHARE_H
#undef HAVE_STDINT_H
#undef HAVE_STRINGS_H
#undef HAVE_SYS_UTIME_H


#if (ACC_OS_POSIX)
#  define HAVE_STRINGS_H 1
#  if (ACC_OS_POSIX_FREEBSD || ACC_OS_POSIX_MACOSX || ACC_OS_POSIX_OPENBSD)
#    undef HAVE_MALLOC_H /* deprecated */
#  elif (ACC_OS_POSIX_HPUX || ACC_OS_POSIX_INTERIX)
#    define HAVE_ALLOCA_H 1
#  endif
#  if (ACC_OS_POSIX_MACOSX && ACC_CC_MWERKS) && defined(__MSL__)
     /* FIXME ??? */
#    undef HAVE_SYS_TIME_H
#    undef HAVE_SYS_TYPES_H
#  endif
#elif (ACC_OS_CYGWIN)
#  define HAVE_IO_H 1
#elif (ACC_OS_EMX)
#  define HAVE_ALLOCA_H 1
#  define HAVE_IO_H 1
#elif (ACC_OS_TOS && ACC_CC_GNUC)
#  if !defined(__MINT__)
#    undef HAVE_MALLOC_H
#  endif
#elif (ACC_OS_TOS && (ACC_CC_PUREC || ACC_CC_TURBOC))
#  undef HAVE_DIRENT_H
#  undef HAVE_FCNTL_H
#  undef HAVE_MALLOC_H
#  undef HAVE_MEMORY_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#endif


/* DOS, OS/2 & Windows */
#if (ACC_OS_DOS16 || ACC_OS_DOS32 || ACC_OS_OS2 || ACC_OS_OS216 || ACC_OS_WIN16 || ACC_OS_WIN32 || ACC_OS_WIN64)

#define HAVE_CONIO_H 1
#define HAVE_DIRECT_H 1
#define HAVE_DOS_H 1
#define HAVE_IO_H 1
#define HAVE_SHARE_H 1

#if (ACC_CC_AZTECC)
#  undef HAVE_CONIO_H
#  undef HAVE_DIRECT_H
#  undef HAVE_DIRENT_H
#  undef HAVE_MALLOC_H
#  undef HAVE_SHARE_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#elif (ACC_CC_BORLANDC)
#  undef HAVE_UNISTD_H
#  undef HAVE_SYS_TIME_H
#  if (ACC_OS_WIN32 || ACC_OS_WIN64)
#    undef HAVE_DIRENT_H /* pulls in <windows.h>; use <dir.h> instead */
#  endif
#  if (__BORLANDC__ < 0x0400)
#    undef HAVE_DIRENT_H
#    undef HAVE_UTIME_H
#  endif
#elif (ACC_CC_DMC)
#  undef HAVE_DIRENT_H /* not working */
#  undef HAVE_UNISTD_H /* not working */
#  define HAVE_SYS_DIRENT_H 1
#elif (ACC_OS_DOS32 && ACC_CC_GNUC) && defined(__DJGPP__)
#elif (ACC_OS_DOS32 && ACC_CC_HIGHC)
#  define HAVE_ALLOCA_H 1
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#elif (ACC_CC_IBMC && ACC_OS_OS2)
#  undef HAVE_DOS_H
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#  define HAVE_SYS_UTIME_H 1
#elif (ACC_CC_INTELC || ACC_CC_MSC)
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#  define HAVE_SYS_UTIME_H 1
#elif (ACC_CC_LCC)
#  undef HAVE_DIRENT_H
#  undef HAVE_DOS_H
#  undef HAVE_SYS_TIME_H
#elif (ACC_OS_WIN32 && ACC_CC_GNUC) && defined(__MINGW32__)
#  undef HAVE_UTIME_H
#  define HAVE_SYS_UTIME_H 1
#elif (ACC_OS_WIN32 && ACC_CC_MWERKS) && defined(__MSL__)
#  define HAVE_ALLOCA_H 1
#  undef HAVE_DOS_H
#  undef HAVE_SHARE_H
#  undef HAVE_SYS_TIME_H
#elif (ACC_CC_NDPC)
#  undef HAVE_DIRENT_H
#  undef HAVE_DOS_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#elif (ACC_CC_PACIFICC)
#  undef HAVE_DIRECT_H
#  undef HAVE_DIRENT_H
#  undef HAVE_FCNTL_H
#  undef HAVE_IO_H
#  undef HAVE_MALLOC_H
#  undef HAVE_MEMORY_H
#  undef HAVE_SHARE_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#elif (ACC_OS_WIN32 && ACC_CC_PELLESC)
#  undef HAVE_DIRENT_H
#  undef HAVE_DOS_H
#  undef HAVE_MALLOC_H
#  undef HAVE_SHARE_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#  if (__POCC__ < 280)
#  else
#    define HAVE_SYS_UTIME_H 1
#  endif
#elif (ACC_OS_WIN32 && ACC_CC_PGI) && defined(__MINGW32__)
#  undef HAVE_UTIME_H
#  define HAVE_SYS_UTIME_H 1
#elif (ACC_OS_WIN32 && ACC_CC_GNUC) && defined(__PW32__)
#elif (ACC_CC_SYMANTECC)
#  undef HAVE_DIRENT_H /* opendir() not implemented in libc */
#  undef HAVE_UNISTD_H /* not working */
#  if (__SC__ < 0x700)
#    undef HAVE_UTIME_H
#    undef HAVE_SYS_TIME_H
#  endif
#elif (ACC_CC_TOPSPEEDC)
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#elif (ACC_CC_TURBOC)
#  undef HAVE_UNISTD_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H /* useless */
#  if (ACC_OS_WIN32 || ACC_OS_WIN64)
#    undef HAVE_DIRENT_H /* pulls in <windows.h>; use <dir.h> instead */
#  endif
#  if (__TURBOC__ < 0x0200)
#    undef HAVE_SIGNAL_H /* not working */
#  endif
#  if (__TURBOC__ < 0x0400)
#    undef HAVE_DIRECT_H
#    undef HAVE_DIRENT_H
#    undef HAVE_MALLOC_H
#    undef HAVE_MEMORY_H
#    undef HAVE_UTIME_H
#  endif
#elif (ACC_CC_WATCOMC)
#  undef HAVE_DIRENT_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#  define HAVE_SYS_UTIME_H 1
#  if (__WATCOMC__ < 950)
#    undef HAVE_UNISTD_H
#  endif
#elif (ACC_CC_ZORTECHC)
#  undef HAVE_DIRENT_H
#  undef HAVE_MEMORY_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#endif

#endif /* DOS, OS/2 & Windows */


#if (HAVE_SYS_TIME_H && HAVE_TIME_H)
#  define TIME_WITH_SYS_TIME 1
#endif

#endif /* !defined(ACC_CONFIG_AUTO_NO_HEADERS) */


/*************************************************************************
// Checks for library functions
**************************************************************************/

#if !defined(ACC_CONFIG_AUTO_NO_FUNCTIONS)

#define HAVE_ACCESS 1
#define HAVE_ALLOCA 1
#define HAVE_ATEXIT 1
#define HAVE_ATOI 1
#define HAVE_ATOL 1
#define HAVE_CHMOD 1
#define HAVE_CHOWN 1
#define HAVE_CTIME 1
#define HAVE_DIFFTIME 1
#define HAVE_FILENO 1
#define HAVE_FSTAT 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_GMTIME 1
#define HAVE_LOCALTIME 1
#define HAVE_LONGJMP 1
#define HAVE_LSTAT 1
#define HAVE_MEMCMP 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_MKTIME 1
#define HAVE_QSORT 1
#define HAVE_RAISE 1
#define HAVE_SETJMP 1
#define HAVE_SIGNAL 1
#define HAVE_SNPRINTF 1
#define HAVE_STAT 1
#define HAVE_STRCHR 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRFTIME 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_TIME 1
#define HAVE_UMASK 1
#define HAVE_UTIME 1
#define HAVE_VSNPRINTF 1

#if (ACC_OS_BEOS || ACC_OS_CYGWIN || ACC_OS_POSIX || ACC_OS_QNX)
#  define HAVE_STRCASECMP 1
#  define HAVE_STRNCASECMP 1
#elif (ACC_OS_WIN32 && ACC_CC_GNUC) && defined(__PW32__)
#  define HAVE_STRCASECMP 1
#  define HAVE_STRNCASECMP 1
#else
#  define HAVE_STRICMP 1
#  define HAVE_STRNICMP 1
#endif


#if (ACC_OS_POSIX)
#  if (ACC_CC_TINYC)
#    undef HAVE_ALLOCA
#  elif defined(__dietlibc__)
#  endif
#  if (ACC_OS_POSIX_MACOSX && ACC_CC_MWERKS) && defined(__MSL__)
     /* FIXME ??? */
#    undef HAVE_CHOWN
#    undef HAVE_LSTAT
#  endif
#elif (ACC_OS_CYGWIN)
#  if (ACC_CC_GNUC < 0x025a00ul)
#    undef HAVE_GETTIMEOFDAY
#    undef HAVE_LSTAT
#  endif
#  if (ACC_CC_GNUC < 0x025f00ul)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  endif
#elif (ACC_OS_EMX)
#  undef HAVE_CHOWN
#  undef HAVE_LSTAT
#elif (ACC_OS_TOS && ACC_CC_GNUC)
#  if !defined(__MINT__)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  endif
#elif (ACC_OS_TOS && (ACC_CC_PUREC || ACC_CC_TURBOC))
#  undef HAVE_ALLOCA
#  undef HAVE_ACCESS
#  undef HAVE_CHMOD
#  undef HAVE_CHOWN
#  undef HAVE_FSTAT
#  undef HAVE_GETTIMEOFDAY
#  undef HAVE_LSTAT
#  undef HAVE_SNPRINTF
#  undef HAVE_UMASK
#  undef HAVE_UTIME
#  undef HAVE_VSNPRINTF
#endif


/* DOS, OS/2 & Windows */
#if (ACC_OS_DOS16 || ACC_OS_DOS32 || ACC_OS_OS2 || ACC_OS_OS216 || ACC_OS_WIN16 || ACC_OS_WIN32 || ACC_OS_WIN64)

#undef HAVE_CHOWN
#undef HAVE_GETTIMEOFDAY
#undef HAVE_LSTAT
#undef HAVE_UMASK

#if (ACC_CC_AZTECC)
#  undef HAVE_ALLOCA
#  undef HAVE_DIFFTIME /* difftime() is in the math library */
#  undef HAVE_FSTAT
#  undef HAVE_STRDUP /* missing in 5.2a */
#  undef HAVE_SNPRINTF
#  undef HAVE_UTIME /* struct utimbuf is missing */
#  undef HAVE_VSNPRINTF
#elif (ACC_CC_BORLANDC)
#  if (__BORLANDC__ < 0x0400)
#    undef HAVE_ALLOCA
#    undef HAVE_UTIME
#  endif
#  if ((__BORLANDC__ < 0x0410) && ACC_OS_WIN16)
#    undef HAVE_ALLOCA
#  endif
#  if (__BORLANDC__ < 0x0550)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  endif
#elif (ACC_CC_DMC)
#  if (ACC_OS_WIN16)
#    undef HAVE_ALLOCA
#  endif
#  define snprintf _snprintf
#  define vsnprintf _vsnprintf
#elif (ACC_OS_DOS32 && ACC_CC_GNUC) && defined(__DJGPP__)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (ACC_OS_DOS32 && ACC_CC_HIGHC)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (ACC_CC_IBMC)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (ACC_CC_INTELC)
#  define snprintf _snprintf
#  define vsnprintf _vsnprintf
#elif (ACC_CC_LCC)
#  define utime _utime
#elif (ACC_CC_MSC)
#  if (_MSC_VER < 600)
#    undef HAVE_STRFTIME
#  endif
#  if (_MSC_VER < 700)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  else
//#    define snprintf _snprintf
//#    define vsnprintf _vsnprintf
#  endif
#  if ((_MSC_VER < 800) && ACC_OS_WIN16)
#    undef HAVE_ALLOCA
#  endif
#  if (ACC_ARCH_IA16) && defined(__cplusplus)
#    undef HAVE_LONGJMP
#    undef HAVE_SETJMP
#  endif
#elif (ACC_OS_WIN32 && ACC_CC_GNUC) && defined(__MINGW32__)
#  if (ACC_CC_GNUC < 0x025f00ul)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  else
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif
#elif (ACC_OS_WIN32 && ACC_CC_MWERKS) && defined(__MSL__)
#  if (__MSL__ < 0x8000ul)
#    undef HAVE_CHMOD /* <unix.h> which in turn pulls in <windows.h> */
#  endif
#elif (ACC_CC_NDPC)
#  undef HAVE_ALLOCA
#  undef HAVE_SNPRINTF
#  undef HAVE_STRNICMP
#  undef HAVE_UTIME
#  undef HAVE_VSNPRINTF
#  if defined(__cplusplus)
#    undef HAVE_STAT
#  endif
#elif (ACC_CC_PACIFICC)
#  undef HAVE_ACCESS
#  undef HAVE_ALLOCA
#  undef HAVE_CHMOD
#  undef HAVE_DIFFTIME
#  undef HAVE_FSTAT
#  undef HAVE_MKTIME
#  undef HAVE_RAISE
#  undef HAVE_SNPRINTF
#  undef HAVE_STRFTIME
#  undef HAVE_UTIME
#  undef HAVE_VSNPRINTF
#elif (ACC_OS_WIN32 && ACC_CC_PELLESC)
#  if (__POCC__ < 280)
#    define alloca _alloca
#    undef HAVE_UTIME
#  endif
#elif (ACC_OS_WIN32 && ACC_CC_PGI) && defined(__MINGW32__)
#  define snprintf _snprintf
#  define vsnprintf _vsnprintf
#elif (ACC_OS_WIN32 && ACC_CC_GNUC) && defined(__PW32__)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (ACC_CC_SYMANTECC)
#  if (ACC_OS_WIN16 && (ACC_MM_MEDIUM || ACC_MM_LARGE || ACC_MM_HUGE))
#    undef HAVE_ALLOCA
#  endif
#  if (__SC__ < 0x600)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  else
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif
#  if (__SC__ < 0x700)
#    undef HAVE_DIFFTIME /* difftime() is broken */
#    undef HAVE_UTIME /* struct utimbuf is missing */
#  endif
#elif (ACC_CC_TOPSPEEDC)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (ACC_CC_TURBOC)
#  undef HAVE_ALLOCA
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#  if (__TURBOC__ < 0x0200)
#    undef HAVE_RAISE
#    undef HAVE_SIGNAL
#  endif
#  if (__TURBOC__ < 0x0295)
#    undef HAVE_MKTIME
#    undef HAVE_STRFTIME
#  endif
#  if (__TURBOC__ < 0x0400)
#    undef HAVE_UTIME
#  endif
#elif (ACC_CC_WATCOMC)
#  if (__WATCOMC__ < 1100)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  elif (__WATCOMC__ < 1200)
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif
#elif (ACC_CC_ZORTECHC)
#  if (ACC_OS_WIN16 && (ACC_MM_MEDIUM || ACC_MM_LARGE || ACC_MM_HUGE))
#    undef HAVE_ALLOCA
#  endif
#  undef HAVE_DIFFTIME /* difftime() is broken */
#  undef HAVE_SNPRINTF
#  undef HAVE_UTIME /* struct utimbuf is missing */
#  undef HAVE_VSNPRINTF
#endif

#endif /* DOS, OS/2 & Windows */


#endif /* !defined(ACC_CONFIG_AUTO_NO_FUNCTIONS) */


/*************************************************************************
// Checks for sizes
**************************************************************************/

#if !defined(ACC_CONFIG_AUTO_NO_SIZES)

#define SIZEOF_SHORT            (__ACC_SHORT_BIT / 8)
#define SIZEOF_INT              (__ACC_INT_BIT / 8)
#define SIZEOF_LONG             (__ACC_LONG_BIT / 8)

#if defined(__SIZEOF_PTRDIFF_T__) && defined(__SIZEOF_SIZE_T__) && defined(__SIZEOF_POINTER__)
#  define SIZEOF_PTRDIFF_T      __SIZEOF_PTRDIFF_T__
#  define SIZEOF_SIZE_T         __SIZEOF_SIZE_T__
#  define SIZEOF_VOID_P         __SIZEOF_POINTER__
#elif (ACC_OS_WIN64) /* LLP64 programming model */
#  define SIZEOF_PTRDIFF_T      8
#  define SIZEOF_SIZE_T         8
#  define SIZEOF_VOID_P         8
#elif (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)
#  define SIZEOF_SIZE_T         2
#  if (ACC_MM_TINY || ACC_MM_SMALL || ACC_MM_MEDIUM)
#    define SIZEOF_VOID_P       2
#  elif (ACC_MM_COMPACT || ACC_MM_LARGE || ACC_MM_HUGE)
#    define SIZEOF_VOID_P       4
#  else
#    error "ACC_MM"
#  endif
#  if (ACC_MM_TINY || ACC_MM_SMALL || ACC_MM_MEDIUM)
#    define SIZEOF_PTRDIFF_T    2
#  elif (ACC_MM_COMPACT || ACC_MM_LARGE)
#    if (ACC_CC_BORLANDC || ACC_CC_TURBOC)
#      define SIZEOF_PTRDIFF_T  4
#    else
#      define SIZEOF_PTRDIFF_T  2
#    endif
#  elif (ACC_MM_HUGE)
#    define SIZEOF_PTRDIFF_T    4
#  else
#    error "ACC_MM"
#  endif
#elif (ACC_ARCH_AVR || ACC_ARCH_C166 || ACC_ARCH_MCS51 || ACC_ARCH_MCS251)
#  define SIZEOF_PTRDIFF_T      2
#  define SIZEOF_SIZE_T         2
#  define SIZEOF_VOID_P         2
#else
#  define SIZEOF_PTRDIFF_T      SIZEOF_LONG
#  define SIZEOF_SIZE_T         SIZEOF_LONG
#  define SIZEOF_VOID_P         SIZEOF_LONG
#endif

#if !defined(SIZEOF_CHAR_P) && (SIZEOF_VOID_P > 0)
#  define SIZEOF_CHAR_P         SIZEOF_VOID_P
#endif


#if ((SIZEOF_LONG) > 0 && (SIZEOF_LONG) < 8)
#if (ACC_ARCH_IA16 && ACC_CC_DMC)
#elif (ACC_CC_GNUC)
#  define SIZEOF_LONG_LONG          8
#  define SIZEOF_UNSIGNED_LONG_LONG 8
#elif ((ACC_OS_WIN32 || ACC_OS_WIN64) && ACC_CC_MSC && (_MSC_VER >= 1400))
#  define SIZEOF_LONG_LONG          8
#  define SIZEOF_UNSIGNED_LONG_LONG 8
#elif (ACC_OS_WIN64)
#  define SIZEOF___INT64            8
#  define SIZEOF_UNSIGNED___INT64   8
#elif (ACC_ARCH_IA32 && (ACC_CC_DMC))
#  define SIZEOF_LONG_LONG          8
#  define SIZEOF_UNSIGNED_LONG_LONG 8
#elif (ACC_ARCH_IA32 && (ACC_CC_SYMANTECC && (__SC__ >= 0x700)))
#  define SIZEOF_LONG_LONG          8
#  define SIZEOF_UNSIGNED_LONG_LONG 8
#elif (ACC_ARCH_IA32 && (ACC_CC_INTELC && defined(__linux__)))
#  define SIZEOF_LONG_LONG          8
#  define SIZEOF_UNSIGNED_LONG_LONG 8
#elif (ACC_ARCH_IA32 && (ACC_CC_MWERKS || ACC_CC_PELLESC || ACC_CC_PGI))
#  define SIZEOF_LONG_LONG          8
#  define SIZEOF_UNSIGNED_LONG_LONG 8
#elif (ACC_ARCH_IA32 && (ACC_CC_INTELC || ACC_CC_MSC))
#  define SIZEOF___INT64            8
#  define SIZEOF_UNSIGNED___INT64   8
#elif (ACC_ARCH_IA32 && (ACC_CC_BORLANDC && (__BORLANDC__ >= 0x0520)))
   /* INFO: unsigned __int64 is somewhat broken in 0x0520; fixed in 0x0530 */
#  define SIZEOF___INT64            8
#  define SIZEOF_UNSIGNED___INT64   8
#elif (ACC_ARCH_IA32 && (ACC_CC_WATCOMC && (__WATCOMC__ >= 1100)))
#  define SIZEOF___INT64            8
#  define SIZEOF_UNSIGNED___INT64   8
#elif (ACC_CC_WATCOMC && defined(_INTEGRAL_MAX_BITS) && (_INTEGRAL_MAX_BITS == 64))
#  define SIZEOF___INT64            8
#  define SIZEOF_UNSIGNED___INT64   8
#elif 1 && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#  define SIZEOF_LONG_LONG          8
#  define SIZEOF_UNSIGNED_LONG_LONG 8
#endif
#endif

#if defined(__cplusplus) && defined(ACC_CC_GNUC)
#  if (ACC_CC_GNUC < 0x020800ul)
#    undef SIZEOF_LONG_LONG
#    undef SIZEOF_UNSIGNED_LONG_LONG
#  endif
#endif

#endif /* !defined(ACC_CONFIG_AUTO_NO_SIZES) */


/*************************************************************************
// misc
**************************************************************************/

#if defined(HAVE_SIGNAL) && !defined(RETSIGTYPE)
#  define RETSIGTYPE void
#endif


/*
vi:ts=4:et
*/

#endif

/*************************************************************************
// merged from xucldecoder_acc_type.h
**************************************************************************/

/***********************************************************************
//
************************************************************************/

#if (ACC_CC_GNUC >= 0x020800ul)     /* 2.8.0 */
#  define __acc_gnuc_extension__ __extension__
#else
#  define __acc_gnuc_extension__
#endif

#if (SIZEOF_LONG_LONG > 0)
__acc_gnuc_extension__ typedef long long acc_llong_t;
#endif
#if (SIZEOF_UNSIGNED_LONG_LONG > 0)
__acc_gnuc_extension__ typedef unsigned long long acc_ullong_t;
#endif

#if (!(SIZEOF_SHORT > 0 && SIZEOF_INT > 0 && SIZEOF_LONG > 0))
#  error "missing defines for sizes"
#endif
#if (!(SIZEOF_PTRDIFF_T > 0 && SIZEOF_SIZE_T > 0 && SIZEOF_VOID_P > 0 && SIZEOF_CHAR_P > 0))
#  error "missing defines for sizes"
#endif


/***********************************************************************
// some <stdint.h> types:
//   required: least & fast: acc_int32l_t, acc_int32f_t
//   optional: exact32 acc_int32e_t
//   optional: least64 acc_int64l_t
************************************************************************/

/* acc_int32e_t is int32_t in <stdint.h> terminology */
#if !defined(acc_int32e_t)
#if (SIZEOF_INT == 4)
#  define acc_int32e_t          int
#  define acc_uint32e_t         unsigned int
#  define ACC_INT32E_C(c)       c
#  define ACC_UINT32E_C(c)      c##U
#elif (SIZEOF_LONG == 4)
#  define acc_int32e_t          long int
#  define acc_uint32e_t         unsigned long int
#  define ACC_INT32E_C(c)       c##L
#  define ACC_UINT32E_C(c)      c##UL
#elif (SIZEOF_SHORT == 4)
#  define acc_int32e_t          short int
#  define acc_uint32e_t         unsigned short int
#  define ACC_INT32E_C(c)       c
#  define ACC_UINT32E_C(c)      c##U
#elif (SIZEOF_LONG_LONG == 4 && SIZEOF_UNSIGNED_LONG_LONG == 4)
#  define acc_int32e_t          acc_llong_t
#  define acc_uint32e_t         acc_ullong_t
#  define ACC_INT32E_C(c)       c##LL
#  define ACC_UINT32E_C(c)      c##ULL
#elif (SIZEOF___INT32 == 4 && SIZEOF_UNSIGNED___INT32 == 4)
#  define acc_int32e_t          __int32
#  define acc_uint32e_t         unsigned __int32
#  if (SIZEOF_INT > 4)
#    define ACC_INT32E_C(c)     c
#    define ACC_UINT32E_C(c)    c##U
#  elif (SIZEOF_LONG > 4)
#    define ACC_INT32E_C(c)     c##L
#    define ACC_UINT32E_C(c)    c##UL
#  else
#    define ACC_INT32E_C(c)     c##i32
#    define ACC_UINT32E_C(c)    c##ui32
#  endif
#else
  /* no exact 32-bit integral type on this machine */
#endif
#endif
#if defined(acc_int32e_t)
#  define SIZEOF_ACC_INT32E_T   4
#endif


/* acc_int32l_t is int_least32_t in <stdint.h> terminology */
#if !defined(acc_int32l_t)
#if defined(acc_int32e_t)
#  define acc_int32l_t          acc_int32e_t
#  define acc_uint32l_t         acc_uint32e_t
#  define ACC_INT32L_C(c)       ACC_INT32E_C(c)
#  define ACC_UINT32L_C(c)      ACC_UINT32E_C(c)
#  define SIZEOF_ACC_INT32L_T   SIZEOF_ACC_INT32E_T
#elif (SIZEOF_INT > 4)
#  define acc_int32l_t          int
#  define acc_uint32l_t         unsigned int
#  define ACC_INT32L_C(c)       c
#  define ACC_UINT32L_C(c)      c##U
#  define SIZEOF_ACC_INT32L_T   SIZEOF_INT
#elif (SIZEOF_LONG > 4)
#  define acc_int32l_t          long int
#  define acc_uint32l_t         unsigned long int
#  define ACC_INT32L_C(c)       c##L
#  define ACC_UINT32L_C(c)      c##UL
#  define SIZEOF_ACC_INT32L_T   SIZEOF_LONG
#else
#  error "acc_int32l_t"
#endif
#endif


/* acc_int32f_t is int_fast32_t in <stdint.h> terminology */
#if !defined(acc_int32f_t)
#if (SIZEOF_INT >= 4)
#  define acc_int32f_t          int
#  define acc_uint32f_t         unsigned int
#  define ACC_INT32F_C(c)       c
#  define ACC_UINT32F_C(c)      c##U
#  define SIZEOF_ACC_INT32F_T   SIZEOF_INT
#elif (SIZEOF_LONG >= 4)
#  define acc_int32f_t          long int
#  define acc_uint32f_t         unsigned long int
#  define ACC_INT32F_C(c)       c##L
#  define ACC_UINT32F_C(c)      c##UL
#  define SIZEOF_ACC_INT32F_T   SIZEOF_LONG
#elif defined(acc_int32e_t)
#  define acc_int32f_t          acc_int32e_t
#  define acc_uint32f_t         acc_uint32e_t
#  define ACC_INT32F_C(c)       ACC_INT32E_C(c)
#  define ACC_UINT32F_C(c)      ACC_UINT32E_C(c)
#  define SIZEOF_ACC_INT32F_T   SIZEOF_ACC_INT32E_T
#else
#  error "acc_int32f_t"
#endif
#endif


/* acc_int64l_t is int_least64_t in <stdint.h> terminology */
#if !defined(acc_int64l_t)
#if (SIZEOF___INT64 >= 8 && SIZEOF_UNSIGNED___INT64 >= 8)
#  if (ACC_CC_BORLANDC) && !defined(ACC_CONFIG_PREFER___INT64)
#    define ACC_CONFIG_PREFER___INT64 1
#  endif
#endif
#if (SIZEOF_INT >= 8)
#  define acc_int64l_t          int
#  define acc_uint64l_t         unsigned int
#  define ACC_INT64L_C(c)       c
#  define ACC_UINT64L_C(c)      c##U
#  define SIZEOF_ACC_INT64L_T   SIZEOF_INT
#elif (SIZEOF_LONG >= 8)
#  define acc_int64l_t          long int
#  define acc_uint64l_t         unsigned long int
#  define ACC_INT64L_C(c)       c##L
#  define ACC_UINT64L_C(c)      c##UL
#  define SIZEOF_ACC_INT64L_T   SIZEOF_LONG
#elif (SIZEOF_LONG_LONG >= 8 && SIZEOF_UNSIGNED_LONG_LONG >= 8) && !defined(ACC_CONFIG_PREFER___INT64)
#  define acc_int64l_t          acc_llong_t
#  define acc_uint64l_t         acc_ullong_t
#  if (ACC_CC_BORLANDC)
#    define ACC_INT64L_C(c)     ((c) + 0ll)
#    define ACC_UINT64L_C(c)    ((c) + 0ull)
#  else
#    define ACC_INT64L_C(c)     c##LL
#    define ACC_UINT64L_C(c)    c##ULL
#  endif
#  define SIZEOF_ACC_INT64L_T   SIZEOF_LONG_LONG
#elif (SIZEOF___INT64 >= 8 && SIZEOF_UNSIGNED___INT64 >= 8)
#  define acc_int64l_t          __int64
#  define acc_uint64l_t         unsigned __int64
#  if (ACC_CC_BORLANDC)
#    define ACC_INT64L_C(c)     ((c) + 0i64)
#    define ACC_UINT64L_C(c)    ((c) + 0ui64)
#  else
#    define ACC_INT64L_C(c)     c##i64
#    define ACC_UINT64L_C(c)    c##ui64
#  endif
#  define SIZEOF_ACC_INT64L_T   SIZEOF___INT64
#else
  /* no least 64-bit integral type on this machine */
#endif
#endif


#if !defined(acc_intptr_t)
#if defined(__INTPTR_TYPE__) && defined(__UINTPTR_TYPE__)
   typedef __INTPTR_TYPE__      acc_intptr_t;
   typedef __UINTPTR_TYPE__     acc_uintptr_t;
#  define acc_intptr_t          acc_intptr_t
#  define acc_uintptr_t         acc_uintptr_t
#  define SIZEOF_ACC_INTPTR_T   SIZEOF_VOID_P
#elif (ACC_ARCH_IA32 && ACC_CC_MSC && (_MSC_VER >= 1300))
   typedef __w64 int            acc_intptr_t;
   typedef __w64 unsigned int   acc_uintptr_t;
#  define acc_intptr_t          acc_intptr_t
#  define acc_uintptr_t         acc_uintptr_t
#  define SIZEOF_ACC_INTPTR_T   SIZEOF_INT
#elif (SIZEOF_INT >= SIZEOF_VOID_P)
#  define acc_intptr_t          int
#  define acc_uintptr_t         unsigned int
#  define SIZEOF_ACC_INTPTR_T   SIZEOF_INT
#elif (SIZEOF_LONG >= SIZEOF_VOID_P)
#  define acc_intptr_t          long
#  define acc_uintptr_t         unsigned long
#  define SIZEOF_ACC_INTPTR_T   SIZEOF_LONG
#elif (SIZEOF_ACC_INT64L_T >= SIZEOF_VOID_P)
#  define acc_intptr_t          acc_int64l_t
#  define acc_uintptr_t         acc_uint64l_t
#  define SIZEOF_ACC_INTPTR_T   SIZEOF_ACC_INT64L_T
#else
#  error "acc_intptr_t"
#endif
#endif


/* workaround for broken compilers */
#if (ACC_BROKEN_INTEGRAL_CONSTANTS)
#  undef ACC_INT32E_C
#  undef ACC_UINT32E_C
#  undef ACC_INT32L_C
#  undef ACC_UINT32L_C
#  undef ACC_INT32F_C
#  undef ACC_UINT32F_C
#  if (SIZEOF_INT == 4)
#    define ACC_INT32E_C(c)     ((c) + 0)
#    define ACC_UINT32E_C(c)    ((c) + 0U)
#    define ACC_INT32L_C(c)     ((c) + 0)
#    define ACC_UINT32L_C(c)    ((c) + 0U)
#    define ACC_INT32F_C(c)     ((c) + 0)
#    define ACC_UINT32F_C(c)    ((c) + 0U)
#  elif (SIZEOF_LONG == 4)
#    define ACC_INT32E_C(c)     ((c) + 0L)
#    define ACC_UINT32E_C(c)    ((c) + 0UL)
#    define ACC_INT32L_C(c)     ((c) + 0L)
#    define ACC_UINT32L_C(c)    ((c) + 0UL)
#    define ACC_INT32F_C(c)     ((c) + 0L)
#    define ACC_UINT32F_C(c)    ((c) + 0UL)
#  else
#    error "integral constants"
#  endif
#endif


/***********************************************************************
// calling conventions
************************************************************************/

#if (ACC_OS_DOS16 || ACC_OS_DOS32 || ACC_OS_OS2 || ACC_OS_OS216 || ACC_OS_WIN16 || ACC_OS_WIN32 || ACC_OS_WIN64)
#  if (ACC_CC_GNUC || ACC_CC_HIGHC || ACC_CC_NDPC || ACC_CC_PACIFICC)
#  elif (ACC_CC_DMC || ACC_CC_SYMANTECC || ACC_CC_ZORTECHC)
#    define __acc_cdecl                 __cdecl
#    define __acc_cdecl_atexit
#    define __acc_cdecl_main            __cdecl
#    if (ACC_OS_OS2 && (ACC_CC_DMC || ACC_CC_SYMANTECC))
#      define __acc_cdecl_qsort         __pascal
#    elif (ACC_OS_OS2 && (ACC_CC_ZORTECHC))
#      define __acc_cdecl_qsort         _stdcall
#    else
#      define __acc_cdecl_qsort         __cdecl
#    endif
#  elif (ACC_CC_WATCOMC)
#    define __acc_cdecl                 __cdecl
#  else
#    define __acc_cdecl                 __cdecl
#    define __acc_cdecl_atexit          __cdecl
#    define __acc_cdecl_main            __cdecl
#    define __acc_cdecl_qsort           __cdecl
#  endif
#  if (ACC_CC_GNUC || ACC_CC_HIGHC || ACC_CC_NDPC || ACC_CC_PACIFICC || ACC_CC_WATCOMC)
#  elif (ACC_OS_OS2 && (ACC_CC_DMC || ACC_CC_SYMANTECC))
#    define __acc_cdecl_sighandler      __pascal
#  elif (ACC_OS_OS2 && (ACC_CC_ZORTECHC))
#    define __acc_cdecl_sighandler      _stdcall
#  elif (ACC_CC_MSC && (_MSC_VER >= 1400)) && defined(_M_CEE_PURE)
#    define __acc_cdecl_sighandler      __clrcall
#  elif (ACC_CC_MSC && (_MSC_VER >= 600 && _MSC_VER < 700))
#    if defined(_DLL)
#      define __acc_cdecl_sighandler    _far _cdecl _loadds
#    elif defined(_MT)
#      define __acc_cdecl_sighandler    _far _cdecl
#    else
#      define __acc_cdecl_sighandler    _cdecl
#    endif
#  else
#    define __acc_cdecl_sighandler      __cdecl
#  endif
#elif (ACC_OS_TOS && (ACC_CC_PUREC || ACC_CC_TURBOC))
#  define __acc_cdecl                   cdecl
#endif

#if !defined(__acc_cdecl)
#  define __acc_cdecl
#endif
#if !defined(__acc_cdecl_atexit)
#  define __acc_cdecl_atexit
#endif
#if !defined(__acc_cdecl_main)
#  define __acc_cdecl_main
#endif
#if !defined(__acc_cdecl_qsort)
#  define __acc_cdecl_qsort
#endif
#if !defined(__acc_cdecl_sighandler)
#  define __acc_cdecl_sighandler
#endif
#if !defined(__acc_cdecl_va)
#  define __acc_cdecl_va                __acc_cdecl
#endif

#if (ACC_BROKEN_CDECL_ALT_SYNTAX)
typedef void __acc_cdecl_sighandler (*acc_sighandler_t)(int);
#elif defined(RETSIGTYPE)
typedef RETSIGTYPE (__acc_cdecl_sighandler *acc_sighandler_t)(int);
#else
typedef void (__acc_cdecl_sighandler *acc_sighandler_t)(int);
#endif


/*
vi:ts=4:et
*/

#endif /* already included */

/*************************************************************************
// optional merged sections from former xucldecoder_acc_*.h fragments
**************************************************************************/

#if defined(ACC_WANT_INCD)

/*************************************************************************
// merged from xucldecoder_acc_incd.h
**************************************************************************/

#ifndef __ACC_INCD_H_INCLUDED
#define __ACC_INCD_H_INCLUDED 1

/* default system includes */
/* see Autoconf:
 *   headers.m4, _AC_INCLUDES_DEFAULT_REQUIREMENTS ac_includes_default
 */

#include <stdio.h>
#if defined(HAVE_TIME_H) && defined(__MSL__) && defined(__cplusplus)
# include <time.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#endif /* already included */


/*
vi:ts=4:et
*/

#endif

#if defined(ACC_WANT_INCE)

/*************************************************************************
// merged from xucldecoder_acc_ince.h
**************************************************************************/

#ifndef __ACC_INCE_H_INCLUDED
#define __ACC_INCE_H_INCLUDED 1

/* extra system includes */

#if defined(HAVE_STDARG_H)
#  include <stdarg.h>
#endif
#if defined(HAVE_CTYPE_H)
#  include <ctype.h>
#endif
#if defined(HAVE_ERRNO_H)
#  include <errno.h>
#endif
#if defined(HAVE_MALLOC_H)
#  include <malloc.h>
#endif
#if defined(HAVE_ALLOCA_H)
#  include <alloca.h>
#endif
#if defined(HAVE_FCNTL_H)
#  include <fcntl.h>
#endif
#if defined(HAVE_DIRENT_H)
#  include <dirent.h>
#endif
#if defined(HAVE_SETJMP_H)
#  include <setjmp.h>
#endif
#if defined(HAVE_SIGNAL_H)
#  include <signal.h>
#endif
#if defined(TIME_WITH_SYS_TIME)
#  include <sys/time.h>
#  include <time.h>
#elif defined(HAVE_TIME_H)
#  include <time.h>
#endif
#if defined(HAVE_UTIME_H)
#  include <utime.h>
#elif defined(HAVE_SYS_UTIME_H)
#  include <sys/utime.h>
#endif

/* DOS, OS/2 & Windows */
#if defined(HAVE_IO_H)
#  include <io.h>
#endif
#if defined(HAVE_DOS_H)
#  include <dos.h>
#endif
#if defined(HAVE_DIRECT_H)
#  include <direct.h>
#endif
#if defined(HAVE_SHARE_H)
#  include <share.h>
#endif
#if defined(ACC_CC_NDPC)
#  include <os.h>
#endif

/* TOS */
#if defined(__TOS__) && (defined(__PUREC__) || defined(__TURBOC__))
#  include <ext.h>
#endif

#endif /* already included */


/*
vi:ts=4:et
*/

#endif

#if defined(ACC_WANT_INCI)

/*************************************************************************
// merged from xucldecoder_acc_inci.h
**************************************************************************/

#ifndef __ACC_INCI_H_INCLUDED
#define __ACC_INCI_H_INCLUDED 1


/*************************************************************************
// internal system includes
**************************************************************************/

#if (ACC_OS_TOS && (ACC_CC_PUREC || ACC_CC_TURBOC))
#  include <tos.h>
#elif (ACC_HAVE_WINDOWS_H)
#  if 1 && !defined(WIN32_LEAN_AND_MEAN)
#    define WIN32_LEAN_AND_MEAN 1
#  endif
#  if 1 && !defined(_WIN32_WINNT)
     /* Restrict to a subset of Windows NT 4.0 header files */
#    define _WIN32_WINNT 0x0400
#  endif
#  include <windows.h>
#  if (ACC_CC_BORLANDC || ACC_CC_TURBOC)
#    include <dir.h>
#  endif
#elif (ACC_OS_DOS16 || ACC_OS_DOS32 || ACC_OS_WIN16)
#  if (ACC_CC_AZTECC)
#    include <model.h>
#    include <stat.h>
#  elif (ACC_CC_BORLANDC || ACC_CC_TURBOC)
#    include <alloc.h>
#    include <dir.h>
#  elif (ACC_OS_DOS32 && ACC_CC_GNUC) && defined(__DJGPP__)
#    include <sys/exceptn.h>
#  elif (ACC_CC_PACIFICC)
#    include <unixio.h>
#    include <stat.h>
#    include <sys.h>
#  elif (ACC_CC_WATCOMC)
#    include <i86.h>
#  endif
#elif (ACC_OS_OS216)
#  if (ACC_CC_WATCOMC)
#    include <i86.h>
#  endif
#endif


/*************************************************************************
//
**************************************************************************/

#if (ACC_OS_DOS16 || ACC_OS_OS216 || ACC_OS_WIN16)
#  if defined(FP_OFF)
#    define ACC_FP_OFF(x)   FP_OFF(x)
#  elif defined(_FP_OFF)
#    define ACC_FP_OFF(x)   _FP_OFF(x)
#  else
#    define ACC_FP_OFF(x)   (((const unsigned __far*)&(x))[0])
#  endif
#  if defined(FP_SEG)
#    define ACC_FP_SEG(x)   FP_SEG(x)
#  elif defined(_FP_SEG)
#    define ACC_FP_SEG(x)   _FP_SEG(x)
#  else
#    define ACC_FP_SEG(x)   (((const unsigned __far*)&(x))[1])
#  endif
#  if defined(MK_FP)
#    define ACC_MK_FP(s,o)  MK_FP(s,o)
#  elif defined(_MK_FP)
#    define ACC_MK_FP(s,o)  _MK_FP(s,o)
#  else
#    define ACC_MK_FP(s,o)  ((void __far*)(((unsigned long)(s)<<16)+(unsigned)(o)))
#  endif
#  if 0
#    undef ACC_FP_OFF
#    undef ACC_FP_SEG
#    undef ACC_MK_FP
#    define ACC_FP_OFF(x)   (((const unsigned __far*)&(x))[0])
#    define ACC_FP_SEG(x)   (((const unsigned __far*)&(x))[1])
#    define ACC_MK_FP(s,o)  ((void __far*)(((unsigned long)(s)<<16)+(unsigned)(o)))
#  endif
#endif


#endif /* already included */


/*
vi:ts=4:et
*/

#endif

#if defined(ACC_WANT_CHK)

/*************************************************************************
// merged from xucldecoder_acc_chk.h
**************************************************************************/

#if !defined(ACCCHK_ASSERT)
#  define ACCCHK_ASSERT(expr)   ACC_COMPILE_TIME_ASSERT(expr)
#endif

/* compile-time sign */
#if !defined(ACCCHK_ASSERT_SIGN_T)
#  define ACCCHK_ASSERT_SIGN_T(type,relop) \
        ACCCHK_ASSERT( (type) (-1)       relop  (type) 0 ) \
        ACCCHK_ASSERT( (type) (~(type)0) relop  (type) 0 ) \
        ACCCHK_ASSERT( (type) (~(type)0) ==     (type) (-1) )
#endif

#if !defined(ACCCHK_IS_SIGNED_T)
#  define ACCCHK_ASSERT_IS_SIGNED_T(type)       ACCCHK_ASSERT_SIGN_T(type,<)
#endif

#if !defined(ACCCHK_IS_UNSIGNED_T)
#  if (ACC_BROKEN_INTEGRAL_PROMOTION)
#    define ACCCHK_ASSERT_IS_UNSIGNED_T(type) \
        ACCCHK_ASSERT( (type) (-1) > (type) 0 )
#  else
#    define ACCCHK_ASSERT_IS_UNSIGNED_T(type)   ACCCHK_ASSERT_SIGN_T(type,>)
#  endif
#endif


/*************************************************************************
// check preprocessor
**************************************************************************/

#if (ACC_0xffffffffL - ACC_UINT32L_C(4294967294) != 1)
#  error "preprocessor error 1"
#endif
#if (ACC_0xffffffffL - ACC_UINT32L_C(0xfffffffd) != 2)
#  error "preprocessor error 2"
#endif


#define ACCCHK_VAL  1
#define ACCCHK_TMP1 ACCCHK_VAL
#undef ACCCHK_VAL
#define ACCCHK_VAL  2
#define ACCCHK_TMP2 ACCCHK_VAL
#if (ACCCHK_TMP1 != 2)
#  error "preprocessor error 3a"
#endif
#if (ACCCHK_TMP2 != 2)
#  error "preprocessor error 3b"
#endif
#undef ACCCHK_VAL
#if (ACCCHK_TMP2)
#  error "preprocessor error 3c"
#endif
#if (ACCCHK_TMP2 + 0 != 0)
#  error "preprocessor error 3d"
#endif
#undef ACCCHK_TMP1
#undef ACCCHK_TMP2


/*************************************************************************
// check basic arithmetics
**************************************************************************/

    ACCCHK_ASSERT(1 == 1)

    ACCCHK_ASSERT(__ACC_INT_MAX(2) == 1)
    ACCCHK_ASSERT(__ACC_INT_MAX(8) == 127)
    ACCCHK_ASSERT(__ACC_INT_MAX(16) == 32767)

    ACCCHK_ASSERT(__ACC_UINT_MAX(2) == 3)
    ACCCHK_ASSERT(__ACC_UINT_MAX(16) == 0xffffU)
    ACCCHK_ASSERT(__ACC_UINT_MAX(32) == 0xffffffffUL)
#if !defined(ACC_BROKEN_INTEGRAL_PROMOTION)
    ACCCHK_ASSERT(__ACC_UINT_MAX(__ACC_INT_BIT) == ~(0u))
    ACCCHK_ASSERT(__ACC_UINT_MAX(__ACC_LONG_BIT) == ~(0ul))
#endif


/*************************************************************************
// check basic types
**************************************************************************/

    ACCCHK_ASSERT_IS_SIGNED_T(signed char)
    ACCCHK_ASSERT_IS_UNSIGNED_T(unsigned char)
    ACCCHK_ASSERT(sizeof(signed char) == sizeof(char))
    ACCCHK_ASSERT(sizeof(unsigned char) == sizeof(char))
    ACCCHK_ASSERT(sizeof(char) == 1)
#if (ACC_CC_CILLY)
    /* CIL is broken */
#else
    ACCCHK_ASSERT(sizeof(char) == sizeof((char)0))
#endif
#if defined(__cplusplus)
    ACCCHK_ASSERT(sizeof('\0') == sizeof(char))
#else
#  if (ACC_CC_DMC)
    /* Digital Mars C is broken */
#  else
    ACCCHK_ASSERT(sizeof('\0') == sizeof(int))
#  endif
#endif
#if defined(acc_alignof)
    ACCCHK_ASSERT(acc_alignof(char) == 1)
    ACCCHK_ASSERT(acc_alignof(signed char) == 1)
    ACCCHK_ASSERT(acc_alignof(unsigned char) == 1)
#endif

    ACCCHK_ASSERT_IS_SIGNED_T(short)
    ACCCHK_ASSERT_IS_UNSIGNED_T(unsigned short)
    ACCCHK_ASSERT(sizeof(short) == sizeof(unsigned short))
    ACCCHK_ASSERT(sizeof(short) >= 2)
    ACCCHK_ASSERT(sizeof(short) >= sizeof(char))
#if (ACC_CC_CILLY)
    /* CIL is broken */
#else
    ACCCHK_ASSERT(sizeof(short) == sizeof((short)0))
#endif
#if (SIZEOF_SHORT > 0)
    ACCCHK_ASSERT(sizeof(short) == SIZEOF_SHORT)
#endif

    ACCCHK_ASSERT_IS_SIGNED_T(int)
    ACCCHK_ASSERT_IS_UNSIGNED_T(unsigned int)
    ACCCHK_ASSERT(sizeof(int) == sizeof(unsigned int))
    ACCCHK_ASSERT(sizeof(int) >= 2)
    ACCCHK_ASSERT(sizeof(int) >= sizeof(short))
    ACCCHK_ASSERT(sizeof(int) == sizeof(0))
    ACCCHK_ASSERT(sizeof(int) == sizeof((int)0))
#if (SIZEOF_INT > 0)
    ACCCHK_ASSERT(sizeof(int) == SIZEOF_INT)
#endif

    ACCCHK_ASSERT_IS_SIGNED_T(long)
    ACCCHK_ASSERT_IS_UNSIGNED_T(unsigned long)
    ACCCHK_ASSERT(sizeof(long) == sizeof(unsigned long))
    ACCCHK_ASSERT(sizeof(long) >= 4)
    ACCCHK_ASSERT(sizeof(long) >= sizeof(int))
    ACCCHK_ASSERT(sizeof(long) == sizeof(0L))
    ACCCHK_ASSERT(sizeof(long) == sizeof((long)0))
#if (SIZEOF_LONG > 0)
    ACCCHK_ASSERT(sizeof(long) == SIZEOF_LONG)
#endif

    ACCCHK_ASSERT_IS_UNSIGNED_T(size_t)
    ACCCHK_ASSERT(sizeof(size_t) >= sizeof(int))
    ACCCHK_ASSERT(sizeof(size_t) == sizeof(sizeof(0))) /* sizeof() returns size_t */
#if (SIZEOF_SIZE_T > 0)
    ACCCHK_ASSERT(sizeof(size_t) == SIZEOF_SIZE_T)
#endif

    ACCCHK_ASSERT_IS_SIGNED_T(ptrdiff_t)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) >= sizeof(int))
    ACCCHK_ASSERT(sizeof(ptrdiff_t) >= sizeof(size_t))
#if !defined(ACC_BROKEN_SIZEOF)
    //ACCCHK_ASSERT(sizeof(ptrdiff_t) == sizeof((char*)0 - (char*)0))
# if (ACC_HAVE_MM_HUGE_PTR)
    ACCCHK_ASSERT(4 == sizeof((char __huge*)0 - (char __huge*)0))
# endif
#endif
#if (SIZEOF_PTRDIFF_T > 0)
    ACCCHK_ASSERT(sizeof(ptrdiff_t) == SIZEOF_PTRDIFF_T)
#endif

    ACCCHK_ASSERT(sizeof(void*) >= sizeof(char*))
#if (SIZEOF_VOID_P > 0)
    ACCCHK_ASSERT(sizeof(void*) == SIZEOF_VOID_P)
#endif
#if (SIZEOF_CHAR_P > 0)
    ACCCHK_ASSERT(sizeof(char*) == SIZEOF_CHAR_P)
#endif
#if (ACC_HAVE_MM_HUGE_PTR)
    ACCCHK_ASSERT(4 == sizeof(void __huge*))
    ACCCHK_ASSERT(4 == sizeof(char __huge*))
#endif


/*************************************************************************
// check arithmetics
**************************************************************************/

    ACCCHK_ASSERT((((1u  << 15) + 1) >> 15) == 1)
    ACCCHK_ASSERT((((1ul << 31) + 1) >> 31) == 1)

#if (ACC_CC_TURBOC && (__TURBOC__ < 0x0150))
    /* TC 1.0 bug, probably due to ACC_BROKEN_INTEGRAL_PROMOTION ?? */
#else
    //ACCCHK_ASSERT((1   << (8*SIZEOF_INT-1)) < 0)
#endif
    ACCCHK_ASSERT((1u  << (8*SIZEOF_INT-1)) > 0)

    //ACCCHK_ASSERT((1l  << (8*SIZEOF_LONG-1)) < 0)
    ACCCHK_ASSERT((1ul << (8*SIZEOF_LONG-1)) > 0)

#if defined(acc_int32e_t)
    ACCCHK_ASSERT(sizeof(acc_int32e_t) == 4)
    ACCCHK_ASSERT(sizeof(acc_int32e_t) == SIZEOF_ACC_INT32E_T)
    ACCCHK_ASSERT(sizeof(acc_uint32e_t) == 4)
    ACCCHK_ASSERT(sizeof(acc_int32e_t) == sizeof(acc_uint32e_t))

    ACCCHK_ASSERT_IS_SIGNED_T(acc_int32e_t)
    ACCCHK_ASSERT(((( (acc_int32e_t)1 << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT(((( ACC_INT32E_C(1) << 30) + 1) >> 30) == 1)

    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uint32e_t)
    ACCCHK_ASSERT(((( (acc_uint32e_t)1 << 31) + 1) >> 31) == 1)
    ACCCHK_ASSERT(((( ACC_UINT32E_C(1) << 31) + 1) >> 31) == 1)

    ACCCHK_ASSERT( (acc_int32e_t) (1 + ~(acc_int32e_t)0) == 0)
#if defined(ACCCHK_CONFIG_PEDANTIC)
    /* compiler may warn about overflow */
    ACCCHK_ASSERT( (acc_uint32e_t)(1 + ~(acc_uint32e_t)0) == 0)
#endif /* ACCCHK_CONFIG_PEDANTIC */

#if (SIZEOF_ACC_INT32E_T < SIZEOF_INT)
    ACCCHK_ASSERT(sizeof(ACC_INT32E_C(0)) == sizeof(int))
    ACCCHK_ASSERT(sizeof(ACC_UINT32E_C(0)) == sizeof(int))
#else
    ACCCHK_ASSERT(sizeof(ACC_INT32E_C(0)) == SIZEOF_ACC_INT32E_T)
    ACCCHK_ASSERT(sizeof(ACC_UINT32E_C(0)) == SIZEOF_ACC_INT32E_T)
#endif
    //ACCCHK_ASSERT((ACC_INT32E_C(1)  << (8*SIZEOF_ACC_INT32E_T-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32E_C(1) << (8*SIZEOF_ACC_INT32E_T-1)) > 0)
    //ACCCHK_ASSERT((ACC_INT32E_C(1)  << (int)(8*sizeof(ACC_INT32E_C(1))-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32E_C(1) << (int)(8*sizeof(ACC_UINT32E_C(1))-1)) > 0)
    ACCCHK_ASSERT(ACC_INT32E_C(2147483647)      > 0)
    ACCCHK_ASSERT(ACC_INT32E_C(-2147483647) -1  < 0)
    ACCCHK_ASSERT(ACC_UINT32E_C(4294967295)     > 0)
    ACCCHK_ASSERT(ACC_UINT32E_C(4294967295) == ACC_0xffffffffL)
#endif


    ACCCHK_ASSERT(sizeof(acc_int32l_t) >= sizeof(int))
#if defined(acc_int32e_t)
    ACCCHK_ASSERT(sizeof(acc_int32l_t) >= sizeof(acc_int32e_t))
#endif

    ACCCHK_ASSERT(sizeof(acc_int32l_t) >= 4)
    ACCCHK_ASSERT(sizeof(acc_int32l_t) == SIZEOF_ACC_INT32L_T)
    ACCCHK_ASSERT(sizeof(acc_uint32l_t) >= 4)
    ACCCHK_ASSERT(sizeof(acc_int32l_t) == sizeof(acc_uint32l_t))

    ACCCHK_ASSERT_IS_SIGNED_T(acc_int32l_t)
    ACCCHK_ASSERT(((( (acc_int32l_t)1 << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT(((( ACC_INT32L_C(1) << 30) + 1) >> 30) == 1)

    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uint32l_t)
    ACCCHK_ASSERT(((( (acc_uint32l_t)1 << 31) + 1) >> 31) == 1)
    ACCCHK_ASSERT(((( ACC_UINT32L_C(1) << 31) + 1) >> 31) == 1)

    ACCCHK_ASSERT( (acc_int32l_t) (1 + ~(acc_int32l_t)0) == 0)
#if defined(ACCCHK_CONFIG_PEDANTIC)
    /* compiler may warn about overflow */
    ACCCHK_ASSERT( (acc_uint32l_t)(1 + ~(acc_uint32l_t)0) == 0)
#endif /* ACCCHK_CONFIG_PEDANTIC */

#if (SIZEOF_ACC_INT32L_T < SIZEOF_INT)
    ACCCHK_ASSERT(sizeof(ACC_INT32L_C(0)) == sizeof(int))
    ACCCHK_ASSERT(sizeof(ACC_UINT32L_C(0)) == sizeof(int))
#else
    ACCCHK_ASSERT(sizeof(ACC_INT32L_C(0)) == SIZEOF_ACC_INT32L_T)
    ACCCHK_ASSERT(sizeof(ACC_UINT32L_C(0)) == SIZEOF_ACC_INT32L_T)
#endif
    //ACCCHK_ASSERT((ACC_INT32L_C(1)  << (8*SIZEOF_ACC_INT32L_T-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32L_C(1) << (8*SIZEOF_ACC_INT32L_T-1)) > 0)
    //ACCCHK_ASSERT((ACC_INT32L_C(1)  << (int)(8*sizeof(ACC_INT32L_C(1))-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32L_C(1) << (int)(8*sizeof(ACC_UINT32L_C(1))-1)) > 0)
    ACCCHK_ASSERT(ACC_INT32L_C(2147483647)      > 0)
    ACCCHK_ASSERT(ACC_INT32L_C(-2147483647) -1  < 0)
    ACCCHK_ASSERT(ACC_UINT32L_C(4294967295)     > 0)
    ACCCHK_ASSERT(ACC_UINT32L_C(4294967295) == ACC_0xffffffffL)


    ACCCHK_ASSERT(sizeof(acc_int32f_t) >= sizeof(int))
#if defined(acc_int32e_t)
    ACCCHK_ASSERT(sizeof(acc_int32f_t) >= sizeof(acc_int32e_t))
#endif

    ACCCHK_ASSERT(sizeof(acc_int32f_t) >= 4)
    ACCCHK_ASSERT(sizeof(acc_int32f_t) == SIZEOF_ACC_INT32F_T)
    ACCCHK_ASSERT(sizeof(acc_uint32f_t) >= 4)
    ACCCHK_ASSERT(sizeof(acc_int32f_t) == sizeof(acc_uint32f_t))

    ACCCHK_ASSERT_IS_SIGNED_T(acc_int32f_t)
    ACCCHK_ASSERT(((( (acc_int32f_t)1 << 30) + 1) >> 30) == 1)
    ACCCHK_ASSERT(((( ACC_INT32F_C(1) << 30) + 1) >> 30) == 1)

    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uint32f_t)
    ACCCHK_ASSERT(((( (acc_uint32f_t)1 << 31) + 1) >> 31) == 1)
    ACCCHK_ASSERT(((( ACC_UINT32F_C(1) << 31) + 1) >> 31) == 1)

    ACCCHK_ASSERT( (acc_int32f_t) (1 + ~(acc_int32f_t)0) == 0)
#if defined(ACCCHK_CONFIG_PEDANTIC)
    /* compiler may warn about overflow */
    ACCCHK_ASSERT( (acc_uint32f_t)(1 + ~(acc_uint32f_t)0) == 0)
#endif /* ACCCHK_CONFIG_PEDANTIC */

#if (SIZEOF_ACC_INT32F_T < SIZEOF_INT)
    ACCCHK_ASSERT(sizeof(ACC_INT32F_C(0)) == sizeof(int))
    ACCCHK_ASSERT(sizeof(ACC_UINT32F_C(0)) == sizeof(int))
#else
    ACCCHK_ASSERT(sizeof(ACC_INT32F_C(0)) == SIZEOF_ACC_INT32F_T)
    ACCCHK_ASSERT(sizeof(ACC_UINT32F_C(0)) == SIZEOF_ACC_INT32F_T)
#endif
    //ACCCHK_ASSERT((ACC_INT32F_C(1)  << (8*SIZEOF_ACC_INT32F_T-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32F_C(1) << (8*SIZEOF_ACC_INT32F_T-1)) > 0)
    //ACCCHK_ASSERT((ACC_INT32F_C(1)  << (int)(8*sizeof(ACC_INT32F_C(1))-1)) < 0)
    ACCCHK_ASSERT((ACC_UINT32F_C(1) << (int)(8*sizeof(ACC_UINT32F_C(1))-1)) > 0)
    ACCCHK_ASSERT(ACC_INT32F_C(2147483647)      > 0)
    ACCCHK_ASSERT(ACC_INT32F_C(-2147483647) -1  < 0)
    ACCCHK_ASSERT(ACC_UINT32F_C(4294967295)     > 0)
    ACCCHK_ASSERT(ACC_UINT32F_C(4294967295) == ACC_0xffffffffL)


#if defined(acc_int64l_t)
    ACCCHK_ASSERT(sizeof(acc_int64l_t) >= 8)
    ACCCHK_ASSERT(sizeof(acc_int64l_t) == SIZEOF_ACC_INT64L_T)
    ACCCHK_ASSERT(sizeof(acc_uint64l_t) >= 8)
    ACCCHK_ASSERT(sizeof(acc_int64l_t) == sizeof(acc_uint64l_t))
#endif

    ACCCHK_ASSERT(sizeof(acc_intptr_t) >= sizeof(void *))
    ACCCHK_ASSERT(sizeof(acc_intptr_t) == SIZEOF_ACC_INTPTR_T)
    ACCCHK_ASSERT(sizeof(acc_uintptr_t) >= sizeof(void *))
    ACCCHK_ASSERT(sizeof(acc_intptr_t) == sizeof(acc_uintptr_t))

    ACCCHK_ASSERT_IS_SIGNED_T(acc_intptr_t)
    ACCCHK_ASSERT_IS_UNSIGNED_T(acc_uintptr_t)


/*************************************************************************
// check declarations
**************************************************************************/

    ACCCHK_ASSERT( sizeof(ACC_INFO_CC) > 0 )
    ACCCHK_ASSERT( sizeof(ACC_INFO_ARCH) > 0 )
    ACCCHK_ASSERT( sizeof(ACC_INFO_MM) > 0 )
    ACCCHK_ASSERT( sizeof(ACC_INFO_OS) > 0 )
#if defined(ACC_INFO_OS_POSIX)
    ACCCHK_ASSERT( sizeof(ACC_INFO_OS_POSIX) > 0 )
#endif


/*************************************************************************
// check memory model
**************************************************************************/

#if (ACC_HAVE_MM_HUGE_PTR)
    ACCCHK_ASSERT(ACC_MM_AHSHIFT > 0)
#endif


/*************************************************************************
// check builtin functions
**************************************************************************/

#if defined(__builtin_expect)
    ACCCHK_ASSERT(__builtin_expect(1,1) == 1)
#endif


/*
vi:ts=4:et
*/

#endif

#if defined(ACC_WANT_HMEMCPY) && !defined(__ACCLIB_HMEMCPY_CH_INCLUDED)

/*************************************************************************
// merged from xucldecoder_acc_hmemcpy.h
**************************************************************************/

#define __ACCLIB_HMEMCPY_CH_INCLUDED 1
#if !defined(ACCLIB_PUBLIC)
#  define ACCLIB_PUBLIC(r,f)    r __ACCLIB_FUNCNAME(f)
#endif


/***********************************************************************
// memcmp, memcpy, memmove, memset
************************************************************************/

ACCLIB_PUBLIC(int, acc_hmemcmp) (const acc_hvoid_p s1, const acc_hvoid_p s2, acc_hsize_t len)
{
#if (ACC_HAVE_MM_HUGE_PTR) || !defined(HAVE_MEMCMP)
    const acc_hbyte_p p1 = (const acc_hbyte_p) s1;
    const acc_hbyte_p p2 = (const acc_hbyte_p) s2;

    if (len > 0) do
    {
        int d = *p1 - *p2;
        if (d != 0)
            return d;
        p1++; p2++;
    } while (--len > 0);
    return 0;
#else
    return memcmp(s1, s2, len);
#endif
}


ACCLIB_PUBLIC(acc_hvoid_p, acc_hmemcpy) (acc_hvoid_p dest, const acc_hvoid_p src, acc_hsize_t len)
{
#if (ACC_HAVE_MM_HUGE_PTR) || !defined(HAVE_MEMCPY)
    acc_hbyte_p p1 = (acc_hbyte_p) dest;
    const acc_hbyte_p p2 = (const acc_hbyte_p) src;

    if (len <= 0 || p1 == p2)
        return dest;
    do
        *p1++ = *p2++;
    while (--len > 0);
    return dest;
#else
    return memcpy(dest, src, len);
#endif
}


ACCLIB_PUBLIC(acc_hvoid_p, acc_hmemmove) (acc_hvoid_p dest, const acc_hvoid_p src, acc_hsize_t len)
{
#if (ACC_HAVE_MM_HUGE_PTR) || !defined(HAVE_MEMMOVE)
    acc_hbyte_p p1 = (acc_hbyte_p) dest;
    const acc_hbyte_p p2 = (const acc_hbyte_p) src;

    if (len <= 0 || p1 == p2)
        return dest;

    if (p1 < p2)
    {
        do
            *p1++ = *p2++;
        while (--len > 0);
    }
    else
    {
        p1 += len;
        p2 += len;
        do
            *--p1 = *--p2;
        while (--len > 0);
    }
    return dest;
#else
    return memmove(dest, src, len);
#endif
}


ACCLIB_PUBLIC(acc_hvoid_p, acc_hmemset) (acc_hvoid_p s, int c, acc_hsize_t len)
{
#if (ACC_HAVE_MM_HUGE_PTR) || !defined(HAVE_MEMSET)
    acc_hbyte_p p = (acc_hbyte_p) s;

    if (len > 0) do
        *p++ = (unsigned char) c;
    while (--len > 0);
    return s;
#else
    return memset(s, c, len);
#endif
}


/*
vi:ts=4:et
*/

#endif
