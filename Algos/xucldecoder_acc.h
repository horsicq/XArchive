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
#include "xucldecoder_acc_init.h"
#include "xucldecoder_acc_os.h"
#include "xucldecoder_acc_cc.h"
#include "xucldecoder_acc_arch.h"
#include "xucldecoder_acc_mm.h"
#include "xucldecoder_acc_defs.h"

#if defined(ACC_CONFIG_NO_HEADER)
#elif defined(ACC_CONFIG_HEADER)
#  include ACC_CONFIG_HEADER
#else
#  include "xucldecoder_acc_auto.h"
#endif

#include "xucldecoder_acc_type.h"

#endif /* already included */


/*
vi:ts=4:et
*/
