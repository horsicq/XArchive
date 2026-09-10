
/*-------------------------------------------------------------*/
/*--- Public header file for the library.                   ---*/
/*---                                               bzlib.h ---*/
/*-------------------------------------------------------------*/

/* ------------------------------------------------------------------
   This file is part of bzip2/libbzip2, a program and library for
   lossless, block-sorting data compression.

   bzip2/libbzip2 version 1.0.4 of 20 December 2006
   Copyright (C) 1996-2006 Julian Seward <jseward@bzip.org>
   This file was modified for ClamAV by aCaB <acab@clamav.net>

   This program is released under the terms of the license contained
   in the file COPYING.bzip2.
   ------------------------------------------------------------------ */

#ifndef _NSIS_BZLIB_H
#define _NSIS_BZLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#define BZ_RUN 0
#define BZ_FLUSH 1
#define BZ_FINISH 2

#define BZ_OK 0
#define BZ_RUN_OK 1
#define BZ_FLUSH_OK 2
#define BZ_FINISH_OK 3
#define BZ_STREAM_END 4
#define BZ_SEQUENCE_ERROR (-1)
#define BZ_PARAM_ERROR (-2)
#define BZ_MEM_ERROR (-3)
#define BZ_DATA_ERROR (-4)
#define BZ_DATA_ERROR_MAGIC (-5)
#define BZ_IO_ERROR (-6)
#define BZ_UNEXPECTED_EOF (-7)
#define BZ_OUTBUFF_FULL (-8)
#define BZ_CONFIG_ERROR (-9)

typedef struct {
    unsigned char *next_in;
    unsigned int avail_in;
    unsigned int total_in_lo32;
    unsigned int total_in_hi32;

    unsigned char *next_out;
    unsigned int avail_out;
    unsigned int total_out_lo32;
    unsigned int total_out_hi32;

    void *state;

    void *(*bzalloc)(void *, int, int);
    void (*bzfree)(void *, void *);
    void *opaque;
} nsis_bzstream;

#ifndef BZ_NO_STDIO
/* Need a definition for FILE */
#include <stdio.h>
#endif

/* Keep the NSIS C calling convention independent of ordinary libbzip2. */
#define NSIS_BZ_API(func) func
#define NSIS_BZ_EXTERN extern

/*-- Core (low-level) library functions --*/
/* aCaB
NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzCompressInit) (
      nsis_bzstream* strm,
      int        blockSize100k,
      int        verbosity,
      int        workFactor
   );

NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzCompress) (
      nsis_bzstream* strm,
      int action
   );

NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzCompressEnd) (
      nsis_bzstream* strm
   );
*/
NSIS_BZ_EXTERN int NSIS_BZ_API(nsis_BZ2_bzDecompressInit)(nsis_bzstream *strm, int verbosity, int small);

NSIS_BZ_EXTERN int NSIS_BZ_API(nsis_BZ2_bzDecompress)(nsis_bzstream *strm);

NSIS_BZ_EXTERN int NSIS_BZ_API(nsis_BZ2_bzDecompressEnd)(nsis_bzstream *strm);

/*-- High(er) level library functions --*/
/* aCaB
#ifndef BZ_NO_STDIO
#define BZ_MAX_UNUSED 5000

typedef void BZFILE;

NSIS_BZ_EXTERN BZFILE* NSIS_BZ_API(BZ2_bzReadOpen) (
      int*  bzerror,
      FILE* f,
      int   verbosity,
      int   small,
      void* unused,
      int   nUnused
   );

NSIS_BZ_EXTERN void NSIS_BZ_API(BZ2_bzReadClose) (
      int*    bzerror,
      BZFILE* b
   );

NSIS_BZ_EXTERN void NSIS_BZ_API(BZ2_bzReadGetUnused) (
      int*    bzerror,
      BZFILE* b,
      void**  unused,
      int*    nUnused
   );

NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzRead) (
      int*    bzerror,
      BZFILE* b,
      void*   buf,
      int     len
   );

NSIS_BZ_EXTERN BZFILE* NSIS_BZ_API(BZ2_bzWriteOpen) (
      int*  bzerror,
      FILE* f,
      int   blockSize100k,
      int   verbosity,
      int   workFactor
   );

NSIS_BZ_EXTERN void NSIS_BZ_API(BZ2_bzWrite) (
      int*    bzerror,
      BZFILE* b,
      void*   buf,
      int     len
   );

NSIS_BZ_EXTERN void NSIS_BZ_API(BZ2_bzWriteClose) (
      int*          bzerror,
      BZFILE*       b,
      int           abandon,
      unsigned int* nbytes_in,
      unsigned int* nbytes_out
   );

NSIS_BZ_EXTERN void NSIS_BZ_API(BZ2_bzWriteClose64) (
      int*          bzerror,
      BZFILE*       b,
      int           abandon,
      unsigned int* nbytes_in_lo32,
      unsigned int* nbytes_in_hi32,
      unsigned int* nbytes_out_lo32,
      unsigned int* nbytes_out_hi32
   );
#endif
*/
/*-- Utility functions --*/
/* aCaB
NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzBuffToBuffCompress) (
      char*         dest,
      unsigned int* destLen,
      char*         source,
      unsigned int  sourceLen,
      int           blockSize100k,
      int           verbosity,
      int           workFactor
   );

NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzBuffToBuffDecompress) (
      char*         dest,
      unsigned int* destLen,
      char*         source,
      unsigned int  sourceLen,
      int           small,
      int           verbosity
   );
*/

/*--
   Code contributed by Yoshioka Tsuneo (tsuneo@rr.iij4u.or.jp)
   to support better zlib compatibility.
   This code is not _officially_ part of libbzip2 (yet);
   I haven't tested it, documented it, or considered the
   threading-safeness of it.
   If this code breaks, please contact both Yoshioka and me.
--*/
/* aCaB
NSIS_BZ_EXTERN const char * NSIS_BZ_API(BZ2_bzlibVersion) (
      void
   );

#ifndef BZ_NO_STDIO
NSIS_BZ_EXTERN BZFILE * NSIS_BZ_API(BZ2_bzopen) (
      const char *path,
      const char *mode
   );

NSIS_BZ_EXTERN BZFILE * NSIS_BZ_API(BZ2_bzdopen) (
      int        fd,
      const char *mode
   );

NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzread) (
      BZFILE* b,
      void* buf,
      int len
   );

NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzwrite) (
      BZFILE* b,
      void*   buf,
      int     len
   );

NSIS_BZ_EXTERN int NSIS_BZ_API(BZ2_bzflush) (
      BZFILE* b
   );

NSIS_BZ_EXTERN void NSIS_BZ_API(BZ2_bzclose) (
      BZFILE* b
   );

NSIS_BZ_EXTERN const char * NSIS_BZ_API(BZ2_bzerror) (
      BZFILE *b,
      int    *errnum
   );
#endif
*/
#ifdef __cplusplus
}
#endif

#endif

/*-------------------------------------------------------------*/
/*--- end                                           bzlib.h ---*/
/*-------------------------------------------------------------*/
