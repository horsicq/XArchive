/* XArchive amalgamation of WavPack 5.9.0 -- decoder only.
 *
 * The ten decoder translation units folded into one, verbatim; only the file
 * boundaries are gone. Section markers name the original file each block came
 * from. Emitted directly as C++, replacing the vendored 3rdparty/wavpack tree.
 *
 * Decoder only. WinZip's ZIP method 97 stores a complete WavPack stream that is
 * only ever read back, so the encoder (pack*.c, extra1/2.c, write_words.c), the
 * WavPack 3 legacy reader (unpack3*.c, open_legacy.c), APEv2 tags (tags.c,
 * tag_utils.c) and open_raw.c are all left out -- 13631 of the upstream
 * library's 19736 lines. decorr_tables.h goes with them: it holds the encoder's
 * decorrelation search tables and pack.c was its only consumer. The retained
 * set is link-complete -- it resolves every symbol it references apart from
 * libc, and defines the whole API XWavPackDecoder calls.
 *
 * wavpack.h stays a real header in Algos/include, as bzlib.h and zlib.h do, and
 * is what XWavPackDecoder includes. It wraps the public API in extern "C", so
 * the eleven entry points keep C language linkage and plain symbol names; the
 * internal helpers declared by wavpack_local.h take C++ linkage, which is
 * invisible outside this translation unit because nothing links to them.
 *
 * Upstream code is verbatim apart from three explicit casts that C accepts
 * implicitly and C++ does not: void pointers assigned to `unsigned char` and
 * `uint16_t` pointers in read_channel_identities() and bs_open_read(). Each is
 * marked XARCHIVE-C++ at the site.
 *
 * WavPack is BSD-3-Clause, Copyright (c) 1998-2025 David Bryant; the text is
 * reproduced in Algos/licenses/wavpack/LICENSE.
 *
 * This file is generated -- do not edit by hand.
 */

/* Upstream's own trim for the APEv2 tag reader. tags.c and tag_utils.c define
 * load_tag/free_tag/valid_tag/editable_tag, and open_utils.c and common_utils.c
 * call them from three #ifndef NO_TAGS blocks; without this the amalgamation
 * leaves those four symbols unresolved and XArchive fails to link. A WavPack
 * stream inside a ZIP member carries no APEv2 tag, and the only mode bit the
 * decoder consults is MODE_LOSSLESS, which this does not touch -- it drops only
 * MODE_VALID_TAG / MODE_APETAG and the tag accessors. */
#define NO_TAGS 1

/* ===================== src/wavpack_version.h ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2026 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wavpack_version.h

#ifndef WAVPACK_VERSION_H
#define WAVPACK_VERSION_H

#define LIBWAVPACK_MAJOR 5
#define LIBWAVPACK_MINOR 9
#define LIBWAVPACK_MICRO 0
#define LIBWAVPACK_VERSION_STRING "5.9.0"

#endif

/* ===================== src/wavpack_local.h ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2024 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wavpack_local.h

#ifndef WAVPACK_LOCAL_H
#define WAVPACK_LOCAL_H

#include "wavpack.h"

#if defined(_WIN32) || (defined(__WATCOMC__) && defined(__OS2__))
#define strdup(x) _strdup(x)
#ifndef FASTCALL
#if defined(_M_IX86) || defined(__i386__)
#define FASTCALL __fastcall
#else /* !x86: */
#define FASTCALL
#endif
#endif /* FASTCALL */
#else
#define FASTCALL
#endif

#if defined(__WATCOMC__) && defined(OPT_ASM_X86)
#define ASMCALL __cdecl
#else
#define ASMCALL
#endif

#if defined(_WIN32) || defined(__WATCOMC__) || \
    (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && (BYTE_ORDER == LITTLE_ENDIAN)) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#define BITSTREAM_SHORTS    // use 16-bit "shorts" for reading/writing bitstreams (instead of chars)
                            //  (only works on little-endian machines)
#endif

#include <sys/types.h>

// This header file contains all the definitions required by WavPack.

#if defined(_MSC_VER) && _MSC_VER < 1600
#include <stdlib.h>
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;
#else
#include <stdint.h>
#endif

// This implements portable multithreading via typedefs and macros for either
// pthreads or native Windows threads. This is easy since the synchronization
// constructs we are using (condition variables and mutexes / critical
// sections) are available on both platforms with similar behavior.

#ifdef ENABLE_THREADS

#ifdef _WIN32

#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0600
#undef _WIN32_WINNT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 /* for CONDITION_VARIABLE & co. */
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

typedef CONDITION_VARIABLE      wp_condvar_t;
#define wp_condvar_init(x)      InitializeConditionVariable(&x)
#define wp_condvar_signal(x)    WakeConditionVariable(&x)
#define wp_condvar_wait(x,y)    SleepConditionVariableCS(&x,&y,INFINITE)
#define wp_condvar_delete(x)

typedef CRITICAL_SECTION        wp_mutex_t;
#define wp_mutex_init(x)        InitializeCriticalSection(&x)
#define wp_mutex_obtain(x)      EnterCriticalSection(&x)
#define wp_mutex_release(x)     LeaveCriticalSection(&x)
#define wp_mutex_delete(x)      DeleteCriticalSection(&x)

typedef HANDLE                  wp_thread_t;
#define wp_thread_create(x,y,z) x=(HANDLE)_beginthreadex(NULL,0,y,z,0,NULL)
#define wp_thread_join(x)       WaitForSingleObject(x,INFINITE)
#define wp_thread_delete(x)     CloseHandle(x);
#define wp_thread_exit(x)       _endthreadex(x);

#else

#include <pthread.h>

typedef pthread_cond_t          wp_condvar_t;
#define wp_condvar_init(x)      pthread_cond_init(&x,NULL);
#define wp_condvar_signal(x)    pthread_cond_signal(&x)
#define wp_condvar_wait(x,y)    pthread_cond_wait(&x,&y)
#define wp_condvar_delete(x)    pthread_cond_destroy(&x)

typedef pthread_mutex_t         wp_mutex_t;
#define wp_mutex_init(x)        pthread_mutex_init(&x,NULL);
#define wp_mutex_obtain(x)      pthread_mutex_lock(&x)
#define wp_mutex_release(x)     pthread_mutex_unlock(&x)
#define wp_mutex_delete(x)      pthread_mutex_destroy(&x)

typedef pthread_t               wp_thread_t;
#define wp_thread_create(x,y,z) do { if (pthread_create(&x,NULL,y,z)) x=0; } while (0)
#define wp_thread_join(x)       pthread_join(x,NULL)
#define wp_thread_delete(x)
#define wp_thread_exit(x)       pthread_exit(x);

#endif

#endif

// Because the C99 specification states that "The order of allocation of
// bit-fields within a unit (high-order to low-order or low-order to
// high-order) is implementation-defined" (6.7.2.1), I decided to change
// the representation of floating-point values from a structure of
// bit-fields to a 32-bit integer with access macros. Note that the WavPack
// library doesn't use any floating-point math to implement compression of
// floating-point data (although a little floating-point math is used in
// high-level functions unrelated to the codec).

typedef int32_t f32;

#define get_mantissa(f)     ((f) & 0x7fffff)
#define get_magnitude(f)    ((f) & 0x7fffffff)
#define get_exponent(f)     (((f) >> 23) & 0xff)
#define get_sign(f)         (((f) >> 31) & 0x1)

#define set_mantissa(f,v)   (f) ^= (((f) ^ (v)) & 0x7fffff)
#define set_exponent(f,v)   (f) ^= (((f) ^ ((uint32_t)(v) << 23)) & 0x7f800000)
#define set_sign(f,v)       (f) ^= (((f) ^ ((uint32_t)(v) << 31)) & 0x80000000)

#include <stdio.h>

#define FALSE 0
#define TRUE 1

// ID3v1 and APEv2 TAG formats (may occur at the end of WavPack files)

typedef struct {
    char tag_id [3], title [30], artist [30], album [30];
    char year [4], comment [30], genre;
} ID3_Tag;

typedef struct {
    char ID [8];
    int32_t version, length, item_count, flags;
    char res [8];
} APE_Tag_Hdr;

#define APE_Tag_Hdr_Format "8LLLL"

#define APE_TAG_TYPE_TEXT       0x0
#define APE_TAG_TYPE_BINARY     0x1
#define APE_TAG_THIS_IS_HEADER  0x20000000
#define APE_TAG_CONTAINS_HEADER 0x80000000
#define APE_TAG_MAX_LENGTH      (1024 * 1024 * 16)

typedef struct {
    int64_t tag_file_pos;
    int tag_begins_file;
    ID3_Tag id3_tag;
    APE_Tag_Hdr ape_tag_hdr;
    unsigned char *ape_tag_data;
} M_Tag;

// or-values for "flags"

#define CUR_STREAM_VERS     0x407       // universally compatible stream version


//////////////////////////// WavPack Metadata /////////////////////////////////

// This is an internal representation of metadata.

typedef struct {
    int32_t byte_length;
    void *data;
    unsigned char id;
} WavpackMetadata;


///////////////////////// WavPack Configuration ///////////////////////////////

/*
 * These config flags are not actually used for external configuration, which is
 * why they're not in the external wavpack.h file, but they are used internally
 * in the flags field of the WavpackConfig struct.
 */

#define CONFIG_MONO_FLAG        4       // not stereo
#define CONFIG_FLOAT_DATA       0x80    // ieee 32-bit floating point data

#define CONFIG_LOSSY_MODE       0x1000000 // obsolete (for information)

/*
 * These config flags were never actually used, or are no longer used, or are
 * used for something else now. They may be used in the future for what they
 * say, or for something else. WavPack files in the wild *may* have some of
 * these bit set in their config flags (with these older meanings), but only
 * if the stream version is 0x410 or less than 0x407. Of course, this is not
 * very important because once the file has been encoded, the config bits are
 * just for information purposes (i.e., they do not affect decoding),
 *
#define CONFIG_ADOBE_MODE       0x100   // "adobe" mode for 32-bit floats
#define CONFIG_VERY_FAST_FLAG   0x400   // double fast
#define CONFIG_COPY_TIME        0x20000 // copy file-time from source
#define CONFIG_QUALITY_MODE     0x200000 // psychoacoustic quality mode
#define CONFIG_RAW_FLAG         0x400000 // raw mode (not implemented yet)
#define CONFIG_QUIET_MODE       0x10000000 // don't report progress %
#define CONFIG_IGNORE_LENGTH    0x20000000 // ignore length in wav header
#define CONFIG_NEW_RIFF_HEADER  0x40000000 // generate new RIFF wav header
 *
 */

#define EXTRA_SCAN_ONLY         1
#define EXTRA_STEREO_MODES      2
#define EXTRA_TRY_DELTAS        8
#define EXTRA_ADJUST_DELTAS     16
#define EXTRA_SORT_FIRST        32
#define EXTRA_BRANCHES          0x1c0
#define EXTRA_SKIP_8TO16        512
#define EXTRA_TERMS             0x3c00
#define EXTRA_DUMP_TERMS        16384
#define EXTRA_SORT_LAST         32768

//////////////////////////////// WavPack Stream ///////////////////////////////

// This internal structure contains everything required to handle a WavPack
// "stream", which is defined as a stereo or mono stream of audio samples. For
// multichannel audio several of these would be required. Each stream contains
// pointers to hold a complete allocated block of WavPack data, although it's
// possible to decode WavPack blocks without buffering an entire block.

typedef struct bs {
#ifdef BITSTREAM_SHORTS
    uint16_t *buf, *end, *ptr;
#else
    unsigned char *buf, *end, *ptr;
#endif
    void (*wrap)(struct bs *bs);
    int error, bc;
    uint32_t sr;
} Bitstream;

#define MAX_WRAPPER_BYTES 16777216
#define NEW_MAX_STREAMS 4096
#define OLD_MAX_STREAMS 8
#define MAX_NTERMS 16
#define MAX_TERM 8

// DSD-specific definitions

#define MAX_HISTORY_BITS    5       // maximum number of history bits in DSD "fast" mode
                                    // note that 5 history bits requires 32 history bins
#define MAX_BYTES_PER_BIN   1280    // maximum bytes for the value lookup array (per bin)
                                    //  such that the total storage per bin = 2K (also
                                    //  counting probabilities and summed_probabilities)

// Note that this structure is directly accessed in assembly files, so modify with care

struct decorr_pass {
    int32_t term, delta, weight_A, weight_B;
    int32_t samples_A [MAX_TERM], samples_B [MAX_TERM];
    int32_t aweight_A, aweight_B;
    int32_t sum_A, sum_B;
};

typedef struct {
    signed char joint_stereo, delta, terms [MAX_NTERMS+1];
} WavpackDecorrSpec;

struct entropy_data {
    uint32_t median [3], slow_level, error_limit;
};

struct words_data {
    uint32_t bitrate_delta [2], bitrate_acc [2];
    uint32_t pend_data, holding_one, zeros_acc;
    int holding_zero, pend_count;
    struct entropy_data c [2];
};

typedef struct {
    int32_t value, filter0, filter1, filter2, filter3, filter4, filter5, filter6, factor;
    unsigned int byte;
} DSDfilters;

typedef struct {
    const WavpackContext *wpc;
    WavpackHeader wphdr;
    struct words_data w;
    int stream_index;

    unsigned char *blockbuff, *blockend;
    unsigned char *block2buff, *block2end;
    int32_t *sample_buffer, *pre_sample_buffer;
    uint32_t num_pre_samples;
    int discontinuous;

    int64_t sample_index;
    int bits, num_terms, mute_error, joint_stereo, false_stereo, shift, lossy_blocks;
    int num_decorrs, num_passes, best_decorr, mask_decorr, extra_flags;
    uint32_t crc, crc_x, crc_wvx;
    Bitstream wvbits, wvcbits, wvxbits;
    int init_done, wvc_skip;
    float delta_decay;

    unsigned char int32_sent_bits, int32_zeros, int32_ones, int32_dups;
    unsigned char float_flags, float_shift, float_max_exp, float_norm_exp;
    unsigned char int32_max_width, float_max_shifted_ones, float_min_shifted_zeros;

    struct {
        int32_t shaping_acc [2], shaping_delta [2], error [2];
        double noise_sum, noise_ave, noise_max;
        int16_t *shaping_data, *shaping_array;
        int32_t shaping_samples;
    } dc;

    struct decorr_pass decorr_passes [MAX_NTERMS], analysis_pass;
    const WavpackDecorrSpec *decorr_specs;

    struct {
        unsigned char *byteptr, *endptr, (*probabilities) [256], *lookup_buffer, **value_lookup, mode, ready;
        int history_bins, p0, p1;
        uint16_t (*summed_probabilities) [256];
        uint32_t low, high, value;
        DSDfilters filters [2];
        int32_t *ptable;
    } dsd;

} WavpackStream;

// flags for float_flags:

#define FLOAT_SHIFT_ONES 1      // bits left-shifted into float = '1'
#define FLOAT_SHIFT_SAME 2      // bits left-shifted into float are the same
#define FLOAT_SHIFT_SENT 4      // bits shifted into float are sent literally
#define FLOAT_ZEROS_SENT 8      // "zeros" are not all real zeros
#define FLOAT_NEG_ZEROS  0x10   // contains negative zeros
#define FLOAT_EXCEPTIONS 0x20   // contains exceptions (inf, nan, etc.)

/////////////////////////////// WavPack Context ///////////////////////////////

// This internal structure holds everything required to encode or decode WavPack
// files. This is an opaque pointer to clients of libwavpack.

#ifdef ENABLE_THREADS

// Each worker thread owns one of these contexts during its lifetime

typedef enum { Uninit, Ready, Running, Done, Quit } WorkerState;

typedef struct {
    WavpackStream *wps;
    WorkerState state;
    int *workers_ready, *worker_errors;
    int32_t *outbuf;
    uint32_t samcnt, offset;
    int result, free_wps;

    wp_condvar_t *global_cond, worker_cond;
    wp_mutex_t *mutex;
    wp_thread_t thread;
} WorkerInfo;

#endif

struct WavpackContext {
    WavpackConfig config;

    WavpackMetadata *metadata;
    uint32_t metabytes;
    int metacount;

    unsigned char *wrapper_data;
    uint32_t wrapper_bytes;

    WavpackBlockOutput blockout;
    void *wv_out, *wvc_out;

    WavpackStreamReader64 *reader;
    void *wv_in, *wvc_in;

    int64_t filelen, file2len, filepos, file2pos, total_samples, initial_index;
    uint32_t crc_errors, first_flags;
    int wvc_flag, open_flags, norm_offset, reduced_channels, lossy_blocks, version_five;
    uint32_t block_samples, ave_block_samples, block_boundary, max_samples, max_pre_samples, acc_samples, riff_trailer_bytes;
    int riff_header_added, riff_header_created;
    M_Tag m_tag;

    int num_streams, max_streams, stream_version;
    WavpackStream **streams;
    void *stream3;

    // these items were added in 5.0 to support alternate file types (especially CAF & DSD)
    unsigned char file_format, *channel_reordering, *channel_identities;
    uint32_t channel_layout, dsd_multiplier;
    void *decimation_context;
    char file_extension [8];

#ifdef ENABLE_THREADS
    // these items support multithreaded operations on multichannel streams
    WorkerInfo *workers;
    int num_workers, workers_ready, worker_errors;
    wp_condvar_t global_cond;
    wp_mutex_t mutex;
#endif

    void (*close_callback)(void *wpc);
    char error_message [80];
};

//////////////////////// function prototypes and macros //////////////////////

#define CLEAR(destin) memset (&destin, 0, sizeof (destin))
#define CLEARA(destin) memset (destin, 0, sizeof (destin))  /* for arrays */

//////////////////////////////// decorrelation //////////////////////////////
// modules: pack.c, unpack.c, unpack_floats.c, extra1.c, extra2.c

// #define SKIP_DECORRELATION   // experimental switch to disable all decorrelation on encode

// These macros implement the weight application and update operations
// that are at the heart of the decorrelation loops. Note that there are
// sometimes two and even three versions of each macro. These should be
// equivalent and produce identical results, but some may perform better
// or worse on a given architecture.

#if 1   // PERFCOND - apply decorrelation weight when no 32-bit overflow possible
#define apply_weight_i(weight, sample) ((weight * sample + 512) >> 10)
#else
#define apply_weight_i(weight, sample) ((((weight * sample) >> 8) + 2) >> 2)
#endif

#if 1   // PERFCOND - apply decorrelation weight when 32-bit overflow is possible
#define apply_weight_f(weight, sample) (((((sample & 0xffff) * weight) >> 9) + \
    (((sample & ~0xffff) >> 9) * weight) + 1) >> 1)
#elif 1
#define apply_weight_f(weight, sample) ((int32_t)((weight * (int64_t) sample + 512) >> 10))
#else
#define apply_weight_f(weight, sample) ((int32_t)floor(((double) weight * sample + 512.0) / 1024.0))
#endif

#if 1   // PERFCOND - universal version that checks input magnitude or always uses long version
#define apply_weight(weight, sample) (sample != (int16_t) sample ? \
    apply_weight_f (weight, sample) : apply_weight_i (weight, sample))
#else
#define apply_weight(weight, sample) (apply_weight_f (weight, sample))
#endif

#if 1   // PERFCOND
#define update_weight(weight, delta, source, result) \
    if (source && result) { int32_t s = (int32_t) (source ^ result) >> 31; weight = (delta ^ s) + (weight - s); }
#elif 1
#define update_weight(weight, delta, source, result) \
    if (source && result) weight += (((source ^ result) >> 30) | 1) * delta;
#else
#define update_weight(weight, delta, source, result) \
    if (source && result) (source ^ result) < 0 ? (weight -= delta) : (weight += delta);
#endif

#define update_weight_clip(weight, delta, source, result) \
    if (source && result) { \
        const int32_t s = (source ^ result) >> 31; \
        if ((weight = (weight ^ s) + (delta - s)) > 1024) weight = 1024; \
        weight = (weight ^ s) - s; \
    }

void pack_init (WavpackStream *wps);
int pack_block (WavpackStream *wps, int32_t *buffer);
void send_general_metadata (WavpackStream *wps);
void send_pending_metadata (WavpackStream *wps);
void free_metadata (WavpackMetadata *wpmd);
int copy_metadata (WavpackMetadata *wpmd, unsigned char *buffer_start, unsigned char *buffer_end);
double WavpackGetEncodedNoise (WavpackContext *wpc, double *peak);
int unpack_init (WavpackContext *wpc, int stream);
int read_decorr_terms (WavpackStream *wps, WavpackMetadata *wpmd);
int read_decorr_weights (WavpackStream *wps, WavpackMetadata *wpmd);
int read_decorr_samples (WavpackStream *wps, WavpackMetadata *wpmd);
int read_shaping_info (WavpackStream *wps, WavpackMetadata *wpmd);
int32_t unpack_samples (WavpackStream *wps, int32_t *buffer, uint32_t sample_count);
int scan_float_data (WavpackStream *wps, f32 *values, int32_t num_values);
void send_float_data (WavpackStream *wps, f32 *values, int32_t num_values);
void float_values (WavpackStream *wps, int32_t *values, int32_t num_values);
void dynamic_noise_shaping (WavpackStream *wps, const int32_t *buffer, int shortening_allowed);
void execute_stereo (WavpackStream *wps, int32_t *samples, int no_history, int do_samples);
void execute_mono (WavpackStream *wps, int32_t *samples, int no_history, int do_samples);

////////////////////////// DSD related (including decimation) //////////////////////////
// modules: pack_dsd.c unpack_dsd.c

void pack_dsd_init (WavpackStream *wps);
int pack_dsd_block (WavpackStream *wps, int32_t *buffer);
int init_dsd_block (WavpackStream *wps, WavpackMetadata *wpmd);
int32_t unpack_dsd_samples (WavpackStream *wps, int32_t *buffer, uint32_t sample_count);

void *decimate_dsd_init (int num_channels);
void decimate_dsd_reset (void *decimate_context);
void decimate_dsd_run (void *decimate_context, int32_t *samples, int num_samples);
void decimate_dsd_destroy (void *decimate_context);

///////////////////////////////// CPU feature detection ////////////////////////////////

int ASMCALL unpack_cpu_has_feature_x86 (int findex);
int ASMCALL pack_cpu_has_feature_x86 (int findex);

#define CPU_FEATURE_MMX     23

///////////////////////////// pre-4.0 version decoding ////////////////////////////
// modules: unpack3.c, unpack3_open.c, unpack3_seek.c

WavpackContext *open_file3 (WavpackContext *wpc, char *error);
int32_t unpack_samples3 (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count);
int seek_sample3 (WavpackContext *wpc, uint32_t desired_index);
uint32_t get_sample_index3 (WavpackContext *wpc);
void free_stream3 (WavpackContext *wpc);
int get_version3 (WavpackContext *wpc);

////////////////////////////// bitstream macros & functions /////////////////////////////

#define bs_is_open(bs) ((bs)->ptr != NULL)
uint32_t bs_close_read (Bitstream *bs);

#define getbit(bs) ( \
    (((bs)->bc) ? \
        ((bs)->bc--, (bs)->sr & 1) : \
            (((++((bs)->ptr) != (bs)->end) ? (void) 0 : (bs)->wrap (bs)), (bs)->bc = sizeof (*((bs)->ptr)) * 8 - 1, ((bs)->sr = *((bs)->ptr)) & 1) \
    ) ? \
        ((bs)->sr >>= 1, 1) : \
        ((bs)->sr >>= 1, 0) \
)

#define getbits(value, nbits, bs) do { \
    while ((nbits) > (bs)->bc) { \
        if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
        (bs)->sr |= (uint32_t)*((bs)->ptr) << (bs)->bc; \
        (bs)->bc += sizeof (*((bs)->ptr)) * 8; \
    } \
    *(value) = (bs)->sr; \
    if ((bs)->bc > 32) { \
        (bs)->bc -= (nbits); \
        (bs)->sr = *((bs)->ptr) >> (sizeof (*((bs)->ptr)) * 8 - (bs)->bc); \
    } \
    else { \
        (bs)->bc -= (nbits); \
        (bs)->sr >>= (nbits); \
    } \
} while (0)

#define putbit(bit, bs) do { if (bit) (bs)->sr |= (1U << (bs)->bc); \
    if (++((bs)->bc) == sizeof (*((bs)->ptr)) * 8) { \
        *((bs)->ptr) = (bs)->sr; \
        (bs)->sr = (bs)->bc = 0; \
        if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }} while (0)

#define putbit_0(bs) do { \
    if (++((bs)->bc) == sizeof (*((bs)->ptr)) * 8) { \
        *((bs)->ptr) = (bs)->sr; \
        (bs)->sr = (bs)->bc = 0; \
        if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }} while (0)

#define putbit_1(bs) do { (bs)->sr |= (1U << (bs)->bc); \
    if (++((bs)->bc) == sizeof (*((bs)->ptr)) * 8) { \
        *((bs)->ptr) = (bs)->sr; \
        (bs)->sr = (bs)->bc = 0; \
        if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }} while (0)

#define putbits(value, nbits, bs) do { \
    (bs)->sr |= (uint32_t)(value) << (bs)->bc; \
    if (((bs)->bc += (nbits)) >= sizeof (*((bs)->ptr)) * 8) \
        do { \
            *((bs)->ptr) = (bs)->sr; \
            (bs)->sr >>= sizeof (*((bs)->ptr)) * 8; \
            if (((bs)->bc -= sizeof (*((bs)->ptr)) * 8) > 32 - sizeof (*((bs)->ptr)) * 8) \
                (bs)->sr |= ((value) >> ((nbits) - (bs)->bc)); \
            if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
        } while ((bs)->bc >= sizeof (*((bs)->ptr)) * 8); \
} while (0)

///////////////////////////// entropy encoder / decoder ////////////////////////////
// modules: entropy_utils.c, read_words.c, write_words.c

// these control the time constant "slow_level" which is used for hybrid mode
// that controls bitrate as a function of residual level (HYBRID_BITRATE).
#define SLS 8
#define SLO ((1 << (SLS - 1)))

#define LIMIT_ONES 16   // maximum consecutive 1s sent for "div" data

// these control the time constant of the 3 median level breakpoints
#define DIV0 128        // 5/7 of samples
#define DIV1 64         // 10/49 of samples
#define DIV2 32         // 20/343 of samples

// this macro retrieves the specified median breakpoint (without frac; min = 1)
#define GET_MED(med) (((c->median [med]) >> 4) + 1)

// These macros update the specified median breakpoints. Note that the median
// is incremented when the sample is higher than the median, else decremented.
// They are designed so that the median will never drop below 1 and the value
// is essentially stationary if there are 2 increments for every 5 decrements.

#define INC_MED0() (c->median [0] += ((c->median [0] + DIV0) / DIV0) * 5)
#define DEC_MED0() (c->median [0] -= ((c->median [0] + (DIV0-2)) / DIV0) * 2)
#define INC_MED1() (c->median [1] += ((c->median [1] + DIV1) / DIV1) * 5)
#define DEC_MED1() (c->median [1] -= ((c->median [1] + (DIV1-2)) / DIV1) * 2)
#define INC_MED2() (c->median [2] += ((c->median [2] + DIV2) / DIV2) * 5)
#define DEC_MED2() (c->median [2] -= ((c->median [2] + (DIV2-2)) / DIV2) * 2)

#ifdef HAVE___BUILTIN_CLZ
#define count_bits(av) ((av) ? 32 - __builtin_clz (av) : 0)
#elif defined (__WATCOMC__) && defined(__386__)
extern __inline int _bsr_watcom(uint32_t);
#pragma aux _bsr_watcom = \
  "bsr eax, eax" \
  parm [eax] nomemory \
  value [eax] \
  modify exact [eax] nomemory;
#define count_bits(av) ((av) ? _bsr_watcom((av)) + 1 : 0)
#elif defined (_WIN64)
 #ifdef _MSC_VER
 #include <intrin.h>
 #endif
static __inline int count_bits (uint32_t av) { unsigned long res; return _BitScanReverse (&res, av) ? (int)(res + 1) : 0; }
#else
#define count_bits(av) ( \
 (av) < (1 << 8) ? nbits_table [av] : \
  ( \
   (av) < (1L << 16) ? nbits_table [(av) >> 8] + 8 : \
   ((av) < (1L << 24) ? nbits_table [(av) >> 16] + 16 : nbits_table [(av) >> 24] + 24) \
  ) \
)
#endif

void init_words (WavpackStream *wps);
void write_entropy_vars (WavpackStream *wps, WavpackMetadata *wpmd);
void write_hybrid_profile (WavpackStream *wps, WavpackMetadata *wpmd);
int read_entropy_vars (WavpackStream *wps, WavpackMetadata *wpmd);
int read_hybrid_profile (WavpackStream *wps, WavpackMetadata *wpmd);
int32_t FASTCALL send_word (WavpackStream *wps, int32_t value, int chan);
void send_words_lossless (WavpackStream *wps, int32_t *buffer, int32_t nsamples);
int32_t FASTCALL get_word (WavpackStream *wps, int chan, int32_t *correction);
int32_t get_words_lossless (WavpackStream *wps, int32_t *buffer, int32_t nsamples);
void flush_word (WavpackStream *wps);
int32_t nosend_word (WavpackStream *wps, int32_t value, int chan);
void scan_word (WavpackStream *wps, int32_t *samples, uint32_t num_samples, int dir);
void update_error_limit (WavpackStream *wps);

extern const uint32_t bitset [32];
extern const uint32_t bitmask [32];
extern const char nbits_table [256];

int wp_log2s (int32_t value);
int32_t wp_exp2s (int log);
int FASTCALL wp_log2 (uint32_t avalue);

#ifdef OPT_ASM_X86
#define LOG2BUFFER log2buffer_x86
#elif defined(OPT_ASM_X64) && (defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW64__) || defined(__midipix__))
#define LOG2BUFFER log2buffer_x64win
#elif defined(OPT_ASM_X64)
#define LOG2BUFFER log2buffer_x64
#else
#define LOG2BUFFER log2buffer
#endif

uint32_t ASMCALL LOG2BUFFER (int32_t *samples, uint32_t num_samples, int limit);

signed char store_weight (int weight);
int restore_weight (signed char weight);

#define WORD_EOF ((int32_t)(1U << 31))

void WavpackFloatNormalize (int32_t *values, int32_t num_values, int delta_exp);

/////////////////////////// high-level unpacking API and support ////////////////////////////
// modules: open_utils.c, unpack_utils.c, unpack_seek.c, unpack_floats.c

WavpackContext *WavpackOpenFileInputEx64 (WavpackStreamReader64 *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset);
WavpackContext *WavpackOpenFileInputEx (WavpackStreamReader *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset);
WavpackContext *WavpackOpenFileInput (const char *infilename, char *error, int flags, int norm_offset);

#define OPEN_WVC        0x1     // open/read "correction" file
#define OPEN_TAGS       0x2     // read ID3v1 / APEv2 tags (seekable file)
#define OPEN_WRAPPER    0x4     // make audio wrapper available (i.e. RIFF)
#define OPEN_2CH_MAX    0x8     // open multichannel as stereo (no downmix)
#define OPEN_NORMALIZE  0x10    // normalize floating point data to +/- 1.0
#define OPEN_STREAMING  0x20    // "streaming" mode blindly unpacks blocks
                                // w/o regard to header file position info
#define OPEN_EDIT_TAGS  0x40    // allow editing of tags
#define OPEN_FILE_UTF8  0x80    // assume filenames are UTF-8 encoded, not ANSI (Windows only)

// new for version 5

#define OPEN_DSD_NATIVE 0x100   // open DSD files as bitstreams
                                // (returned as 8-bit "samples" stored in 32-bit words)
#define OPEN_DSD_AS_PCM 0x200   // open DSD files as 24-bit PCM (decimated 8x)
#define OPEN_ALT_TYPES  0x400   // application is aware of alternate file types & qmode
                                // (just affects retrieving wrappers & MD5 checksums)
#define OPEN_NO_CHECKSUM 0x800  // don't verify block checksums before decoding

int WavpackGetMode (WavpackContext *wpc);

int WavpackGetQualifyMode (WavpackContext *wpc);
int WavpackGetVersion (WavpackContext *wpc);
uint32_t WavpackUnpackSamples (WavpackContext *wpc, int32_t *buffer, uint32_t samples);
int WavpackSeekSample (WavpackContext *wpc, uint32_t sample);
int WavpackSeekSample64 (WavpackContext *wpc, int64_t sample);
int WavpackGetMD5Sum (WavpackContext *wpc, unsigned char data [16]);

int WavpackVerifySingleBlock (unsigned char *buffer, int verify_checksum);
uint32_t read_next_header (WavpackStreamReader64 *reader, void *id, WavpackHeader *wphdr);
int read_wvc_block (WavpackContext *wpc, int stream);

/////////////////////////// high-level packing API and support ////////////////////////////
// modules: pack_utils.c, pack_floats.c

WavpackContext *WavpackOpenFileOutput (WavpackBlockOutput blockout, void *wv_id, void *wvc_id);
int WavpackSetConfiguration (WavpackContext *wpc, WavpackConfig *config, uint32_t total_samples);
int WavpackSetConfiguration64 (WavpackContext *wpc, WavpackConfig *config, int64_t total_samples, const unsigned char *chan_ids);
int WavpackPackInit (WavpackContext *wpc);
int WavpackAddWrapper (WavpackContext *wpc, void *data, uint32_t bcount);
int WavpackPackSamples (WavpackContext *wpc, int32_t *sample_buffer, uint32_t sample_count);
int WavpackFlushSamples (WavpackContext *wpc);
int WavpackStoreMD5Sum (WavpackContext *wpc, unsigned char data [16]);
void WavpackSeekTrailingWrapper (WavpackContext *wpc);
void WavpackUpdateNumSamples (WavpackContext *wpc, void *first_block);
void *WavpackGetWrapperLocation (void *first_block, uint32_t *size);

/////////////////////////////////// common utilities ////////////////////////////////////
// module: common_utils.c

extern const uint32_t sample_rates [16];
uint32_t WavpackGetLibraryVersion (void);
const char *WavpackGetLibraryVersionString (void);
uint32_t WavpackGetSampleRate (WavpackContext *wpc);
int WavpackGetBitsPerSample (WavpackContext *wpc);
int WavpackGetBytesPerSample (WavpackContext *wpc);
int WavpackGetNumChannels (WavpackContext *wpc);
int WavpackGetChannelMask (WavpackContext *wpc);
int WavpackGetReducedChannels (WavpackContext *wpc);
int WavpackGetFloatNormExp (WavpackContext *wpc);
uint32_t WavpackGetNumSamples (WavpackContext *wpc);
int64_t WavpackGetNumSamples64 (WavpackContext *wpc);
uint32_t WavpackGetSampleIndex (WavpackContext *wpc);
int64_t WavpackGetSampleIndex64 (WavpackContext *wpc);
char *WavpackGetErrorMessage (WavpackContext *wpc);
int WavpackGetNumErrors (WavpackContext *wpc);
int WavpackLossyBlocks (WavpackContext *wpc);
uint32_t WavpackGetWrapperBytes (WavpackContext *wpc);
unsigned char *WavpackGetWrapperData (WavpackContext *wpc);
void WavpackFreeWrapper (WavpackContext *wpc);
double WavpackGetProgress (WavpackContext *wpc);
uint32_t WavpackGetFileSize (WavpackContext *wpc);
int64_t WavpackGetFileSize64 (WavpackContext *wpc);
double WavpackGetRatio (WavpackContext *wpc);
double WavpackGetAverageBitrate (WavpackContext *wpc, int count_wvc);
double WavpackGetInstantBitrate (WavpackContext *wpc);
WavpackContext *WavpackCloseFile (WavpackContext *wpc);
void WavpackLittleEndianToNative (void *data, char *format);
void WavpackNativeToLittleEndian (void *data, char *format);
void WavpackBigEndianToNative (void *data, char *format);
void WavpackNativeToBigEndian (void *data, char *format);

void install_close_callback (WavpackContext *wpc, void cb_func (void *wpc));
void free_single_stream (WavpackStream *wps);
void free_dsd_tables (WavpackStream *wps);
void free_streams (WavpackContext *wpc);

/////////////////////////////////// tag utilities ////////////////////////////////////
// modules: tags.c, tag_utils.c

int WavpackGetNumTagItems (WavpackContext *wpc);
int WavpackGetTagItem (WavpackContext *wpc, const char *item, char *value, int size);
int WavpackGetTagItemIndexed (WavpackContext *wpc, int index, char *item, int size);
int WavpackGetNumBinaryTagItems (WavpackContext *wpc);
int WavpackGetBinaryTagItem (WavpackContext *wpc, const char *item, char *value, int size);
int WavpackGetBinaryTagItemIndexed (WavpackContext *wpc, int index, char *item, int size);
int WavpackAppendTagItem (WavpackContext *wpc, const char *item, const char *value, int vsize);
int WavpackAppendBinaryTagItem (WavpackContext *wpc, const char *item, const char *value, int vsize);
int WavpackDeleteTagItem (WavpackContext *wpc, const char *item);
int WavpackWriteTag (WavpackContext *wpc);
int load_tag (WavpackContext *wpc);
void free_tag (M_Tag *m_tag);
int valid_tag (M_Tag *m_tag);
int editable_tag (M_Tag *m_tag);

#endif


/* ===================== src/open_utils.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2024 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// open_utils.c

// This module provides all the code required to open an existing WavPack file
// for reading by using a reader callback mechanism (NOT a filename). This
// includes the code required to find and parse WavPack blocks, process any
// included metadata, and queue up the bitstreams containing the encoded audio
// data. It does not the actual code to unpack audio data and this was done so
// that programs that just want to query WavPack files for information (like,
// for example, taggers) don't need to link in a lot of unnecessary code.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// This function is identical to WavpackOpenFileInput() except that instead
// of providing a filename to open, the caller provides a pointer to a set of
// reader callbacks and instances of up to two streams. The first of these
// streams is required and contains the regular WavPack data stream; the second
// contains the "correction" file if desired. Unlike the standard open
// function which handles the correction file transparently, in this case it
// is the responsibility of the caller to be aware of correction files.

static int seek_eof_information (WavpackContext *wpc, int64_t *final_index, int get_wrapper);

WavpackContext *WavpackOpenFileInputEx64 (WavpackStreamReader64 *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset)
{
    WavpackContext *wpc = (WavpackContext *)malloc (sizeof (WavpackContext));
    WavpackStream *wps;
    int num_blocks = 0;
    unsigned char first_byte;
    uint32_t bcount;

    if (!wpc) {
        if (error) strcpy (error, "can't allocate memory");
        return NULL;
    }

    CLEAR (*wpc);
    wpc->wv_in = wv_id;
    wpc->wvc_in = wvc_id;
    wpc->reader = reader;
    wpc->total_samples = -1;
    wpc->norm_offset = norm_offset;
    wpc->max_streams = OLD_MAX_STREAMS;     // use this until overwritten with actual number
    wpc->open_flags = flags;

    wpc->filelen = wpc->reader->get_length (wpc->wv_in);

#ifndef NO_TAGS
    if ((flags & (OPEN_TAGS | OPEN_EDIT_TAGS)) && wpc->reader->can_seek (wpc->wv_in)) {
        load_tag (wpc);
        wpc->reader->set_pos_abs (wpc->wv_in, 0);

        if ((flags & OPEN_EDIT_TAGS) && !editable_tag (&wpc->m_tag)) {
            if (error) strcpy (error, "can't edit tags located at the beginning of files!");
            return WavpackCloseFile (wpc);
        }
    }
#endif

    if (wpc->reader->read_bytes (wpc->wv_in, &first_byte, 1) != 1) {
        if (error) strcpy (error, "can't read all of WavPack file!");
        return WavpackCloseFile (wpc);
    }

    wpc->reader->push_back_byte (wpc->wv_in, first_byte);

    if (first_byte == 'R') {
#ifdef ENABLE_LEGACY
        return open_file3 (wpc, error);
#else
        if (error) strcpy (error, "this legacy WavPack file is deprecated, use version 4.80.0 to transcode");
        return WavpackCloseFile (wpc);
#endif
    }

    wpc->streams = (WavpackStream **)(malloc ((wpc->num_streams = 1) * sizeof (wpc->streams [0])));
    if (!wpc->streams) {
        if (error) strcpy (error, "can't allocate memory");
        return WavpackCloseFile (wpc);
    }

    wpc->streams [0] = wps = (WavpackStream *)calloc (1, sizeof (WavpackStream));
    if (!wps) {
        if (error) strcpy (error, "can't allocate memory");
        return WavpackCloseFile (wpc);
    }

    wps->wpc = wpc;

    while (!wps->wphdr.block_samples) {

        wpc->filepos = wpc->reader->get_pos (wpc->wv_in);
        bcount = read_next_header (wpc->reader, wpc->wv_in, &wps->wphdr);

        if (bcount == (uint32_t) -1 ||
            (!wps->wphdr.block_samples && num_blocks++ > 16)) {
                if (error) strcpy (error, "not compatible with this version of WavPack file!");
                return WavpackCloseFile (wpc);
        }

        wpc->filepos += bcount;
        wps->blockbuff = (unsigned char *)malloc (wps->wphdr.ckSize + 8);
        if (!wps->blockbuff) {
            if (error) strcpy (error, "can't allocate memory");
            return WavpackCloseFile (wpc);
        }
        memcpy (wps->blockbuff, &wps->wphdr, 32);

        if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + 32, wps->wphdr.ckSize - 24) != wps->wphdr.ckSize - 24) {
            if (error) strcpy (error, "can't read all of WavPack file!");
            return WavpackCloseFile (wpc);
        }

        // if block does not verify, flag error, free buffer, and continue
        if (!WavpackVerifySingleBlock (wps->blockbuff, !(flags & OPEN_NO_CHECKSUM))) {
            wps->wphdr.block_samples = 0;
            free (wps->blockbuff);
            wps->blockbuff = NULL;
            wpc->crc_errors++;
            continue;
        }

        wps->init_done = FALSE;

        if (wps->wphdr.block_samples) {
            if (flags & OPEN_STREAMING)
                SET_BLOCK_INDEX (wps->wphdr, 0);
            else if (wpc->total_samples == -1) {
                if (GET_BLOCK_INDEX (wps->wphdr) || GET_TOTAL_SAMPLES (wps->wphdr) == -1) {
                    wpc->initial_index = GET_BLOCK_INDEX (wps->wphdr);
                    SET_BLOCK_INDEX (wps->wphdr, 0);

                    if (wpc->reader->can_seek (wpc->wv_in)) {
                        int64_t final_index = -1;

                        seek_eof_information (wpc, &final_index, FALSE);

                        if (final_index != -1)
                            wpc->total_samples = final_index - wpc->initial_index;
                    }
                }
                else
                    wpc->total_samples = GET_TOTAL_SAMPLES (wps->wphdr);
            }
        }
        else if (wpc->total_samples == -1 && !GET_BLOCK_INDEX (wps->wphdr) && GET_TOTAL_SAMPLES (wps->wphdr))
            wpc->total_samples = GET_TOTAL_SAMPLES (wps->wphdr);

        if (wpc->wvc_in && wps->wphdr.block_samples && (wps->wphdr.flags & HYBRID_FLAG)) {
            unsigned char ch;

            if (wpc->reader->read_bytes (wpc->wvc_in, &ch, 1) == 1) {
                wpc->reader->push_back_byte (wpc->wvc_in, ch);
                wpc->file2len = wpc->reader->get_length (wpc->wvc_in);
                wpc->wvc_flag = TRUE;
            }
        }

        if (wpc->wvc_flag && !read_wvc_block (wpc, 0)) {
            if (error) strcpy (error, "not compatible with this version of correction file!");
            return WavpackCloseFile (wpc);
        }

        if (!wps->init_done && !unpack_init (wpc, 0)) {
            if (error) strcpy (error, wpc->error_message [0] ? wpc->error_message :
                "not compatible with this version of WavPack file!");

            return WavpackCloseFile (wpc);
        }

        if (!wps->wphdr.block_samples) {    // free blockbuff if we're going to loop again
            free (wps->blockbuff);
            wps->blockbuff = NULL;
        }

        wps->init_done = TRUE;
    }

    wpc->config.flags &= ~0xff;
    wpc->config.flags |= wps->wphdr.flags & 0xff;

    if (!wpc->config.num_channels) {
        wpc->config.num_channels = (wps->wphdr.flags & MONO_FLAG) ? 1 : 2;
        wpc->config.channel_mask = 0x5 - wpc->config.num_channels;
    }

    if ((flags & OPEN_2CH_MAX) && !(wps->wphdr.flags & FINAL_BLOCK))
        wpc->reduced_channels = (wps->wphdr.flags & MONO_FLAG) ? 1 : 2;

    if (wps->wphdr.flags & DSD_FLAG) {
#ifdef ENABLE_DSD
        if (flags & OPEN_DSD_NATIVE) {
            wpc->config.bytes_per_sample = 1;
            wpc->config.bits_per_sample = 8;
        }
        else if (flags & OPEN_DSD_AS_PCM) {
            wpc->decimation_context = decimate_dsd_init (wpc->reduced_channels ?
                wpc->reduced_channels : wpc->config.num_channels);

            wpc->config.bytes_per_sample = 3;
            wpc->config.bits_per_sample = 24;
        }
        else {
            if (error) strcpy (error, "not configured to handle DSD WavPack files!");
            return WavpackCloseFile (wpc);
        }
#else
        if (error) strcpy (error, "not configured to handle DSD WavPack files!");
        return WavpackCloseFile (wpc);
#endif
    }
    else {
        wpc->config.bytes_per_sample = (wps->wphdr.flags & BYTES_STORED) + 1;
        wpc->config.float_norm_exp = wps->float_norm_exp;

        wpc->config.bits_per_sample = (wpc->config.bytes_per_sample * 8) -
            ((wps->wphdr.flags & SHIFT_MASK) >> SHIFT_LSB);
    }

    if (!wpc->config.sample_rate) {
        if (!wps->wphdr.block_samples || (wps->wphdr.flags & SRATE_MASK) == SRATE_MASK)
            wpc->config.sample_rate = 44100;
        else
            wpc->config.sample_rate = sample_rates [(wps->wphdr.flags & SRATE_MASK) >> SRATE_LSB];
    }

#ifdef ENABLE_THREADS
    if (!wpc->reduced_channels && (wpc->open_flags & OPEN_THREADS_MASK)) {
        wpc->num_workers = ((wpc->open_flags & OPEN_THREADS_MASK) >> OPEN_THREADS_SHFT) & 0xf;

        // for multichannel files we can limit the number of workers
        // because we only do spatial multithreading (not temporal)

        if (!(wps->wphdr.flags & FINAL_BLOCK)) {
            if (wpc->num_workers > wpc->max_streams - 1)
                wpc->num_workers = wpc->max_streams - 1;

            if (wpc->num_workers > wpc->config.num_channels - 1)
                wpc->num_workers = wpc->config.num_channels - 1;
        }
    }
#endif

    return wpc;
}

// This function returns the major version number of the WavPack program
// (or library) that created the open file. Currently, this can be 1 to 5.
// Minor versions are not recorded in WavPack files.

int WavpackGetVersion (WavpackContext *wpc)
{
    if (wpc) {
#ifdef ENABLE_LEGACY
        if (wpc->stream3)
            return get_version3 (wpc);
#endif
        return wpc->version_five ? 5 : 4;
    }

    return 0;
}

// Return the file format specified in the call to WavpackSetFileInformation()
// when the file was created. For all files created prior to WavPack 5.0 this
// will 0 (WP_FORMAT_WAV).

unsigned char WavpackGetFileFormat (WavpackContext *wpc)
{
    return wpc->file_format;
}

// Return a string representing the recommended file extension for the open
// WavPack file. For all files created prior to WavPack 5.0 this will be "wav",
// even for raw files with no RIFF into. This string is specified in the
// call to WavpackSetFileInformation() when the file was created.

char *WavpackGetFileExtension (WavpackContext *wpc)
{
    if (wpc && wpc->file_extension [0])
        return wpc->file_extension;
    else
        return (char *)"wav";
}

// This function initializes everything required to unpack a WavPack block
// and must be called before unpack_samples() is called to obtain audio data.
// It is assumed that the WavpackHeader has been read into the wps->wphdr
// (in the current WavpackStream) and that the entire block has been read at
// wps->blockbuff. If a correction file is available (wpc->wvc_flag = TRUE)
// then the corresponding correction block must be read into wps->block2buff
// and its WavpackHeader has overwritten the header at wps->wphdr. This is
// where all the metadata blocks are scanned including those that contain
// bitstream data.

static int read_metadata_buff (WavpackMetadata *wpmd, unsigned char *blockbuff, unsigned char **buffptr);
static int process_metadata (WavpackContext *wpc, WavpackMetadata *wpmd, int stream);
static void bs_open_read (Bitstream *bs, void *buffer_start, void *buffer_end);

int unpack_init (WavpackContext *wpc, int stream)
{
    WavpackStream *wps = wpc->streams [stream];
    unsigned char *blockptr, *block2ptr;
    WavpackMetadata wpmd;

    wps->num_terms = 0;
    wps->mute_error = FALSE;
    wps->crc = wps->crc_x = 0xffffffff;
    wps->dsd.ready = 0;
    CLEAR (wps->wvbits);
    CLEAR (wps->wvcbits);
    CLEAR (wps->wvxbits);
    CLEARA(wps->decorr_passes);
    CLEAR (wps->dc);
    CLEAR (wps->w);

    if (!(wps->wphdr.flags & MONO_FLAG) && wpc->config.num_channels && wps->wphdr.block_samples &&
        (wpc->reduced_channels == 1 || wpc->config.num_channels == 1)) {
            wps->mute_error = TRUE;
            return FALSE;
    }

    if ((wps->wphdr.flags & UNKNOWN_FLAGS) || (wps->wphdr.flags & MONO_DATA) == MONO_DATA) {
        wps->mute_error = TRUE;
        return FALSE;
    }

    blockptr = wps->blockbuff + sizeof (WavpackHeader);

    while (read_metadata_buff (&wpmd, wps->blockbuff, &blockptr))
        if (!process_metadata (wpc, &wpmd, stream)) {
            wps->mute_error = TRUE;
            return FALSE;
        }

    if (wps->wphdr.block_samples && wpc->wvc_flag && wps->block2buff) {
        block2ptr = wps->block2buff + sizeof (WavpackHeader);

        while (read_metadata_buff (&wpmd, wps->block2buff, &block2ptr))
            if (!process_metadata (wpc, &wpmd, stream)) {
                wps->mute_error = TRUE;
                return FALSE;
            }
    }

    if (wps->wphdr.block_samples && ((wps->wphdr.flags & DSD_FLAG) ? !wps->dsd.ready : !bs_is_open (&wps->wvbits))) {
        if (bs_is_open (&wps->wvcbits))
            strcpy (wpc->error_message, "can't unpack correction files alone!");

        wps->mute_error = TRUE;
        return FALSE;
    }

    if (wps->wphdr.block_samples && !bs_is_open (&wps->wvxbits)) {
        if ((wps->wphdr.flags & INT32_DATA) && wps->int32_sent_bits)
            wpc->lossy_blocks = TRUE;

        if ((wps->wphdr.flags & FLOAT_DATA) &&
            wps->float_flags & (FLOAT_EXCEPTIONS | FLOAT_ZEROS_SENT | FLOAT_SHIFT_SENT | FLOAT_SHIFT_SAME))
                wpc->lossy_blocks = TRUE;
    }

    if (wps->wphdr.block_samples)
        wps->sample_index = GET_BLOCK_INDEX (wps->wphdr);

    return TRUE;
}

//////////////////////////////// metadata handlers ///////////////////////////////

// These functions handle specific metadata types and are called directly
// during WavPack block parsing by process_metadata() at the bottom.

// This function initializes the main bitstream for audio samples, which must
// be in the "wv" file.

static int init_wv_bitstream (WavpackStream *wps, WavpackMetadata *wpmd)
{
    if (!wpmd->byte_length || (wpmd->byte_length & 1))
        return FALSE;

    bs_open_read (&wps->wvbits, wpmd->data, (unsigned char *) wpmd->data + wpmd->byte_length);
    return TRUE;
}

// This function initializes the "correction" bitstream for audio samples,
// which currently must be in the "wvc" file.

static int init_wvc_bitstream (WavpackStream *wps, WavpackMetadata *wpmd)
{
    if (!wpmd->byte_length || (wpmd->byte_length & 1))
        return FALSE;

    bs_open_read (&wps->wvcbits, wpmd->data, (unsigned char *) wpmd->data + wpmd->byte_length);
    return TRUE;
}

// This function initializes the "extra" bitstream for audio samples which
// contains the information required to losslessly decompress 32-bit float data
// or integer data that exceeds 24 bits. This bitstream is in the "wv" file
// for pure lossless data or the "wvc" file for hybrid lossless. This data
// would not be used for hybrid lossy mode. There is also a 32-bit CRC stored
// in the first 4 bytes of these blocks.

static int init_wvx_bitstream (WavpackStream *wps, WavpackMetadata *wpmd)
{
    unsigned char *cp = (unsigned char *)wpmd->data;

    if (wpmd->byte_length <= 4 || (wpmd->byte_length & 1))
        return FALSE;

    wps->crc_wvx = *cp++;
    wps->crc_wvx |= (uint32_t) *cp++ << 8;
    wps->crc_wvx |= (uint32_t) *cp++ << 16;
    wps->crc_wvx |= (uint32_t) *cp++ << 24;

    bs_open_read (&wps->wvxbits, cp, (unsigned char *) wpmd->data + wpmd->byte_length);

    // the new WVX bitstream format starts with one or two new 5-bit fields

    if (wpmd->id == ID_WVX_NEW_BITSTREAM) {
        if (wps->wphdr.flags & FLOAT_DATA) {
            getbits (&wps->float_min_shifted_zeros, 5, &wps->wvxbits);
            wps->float_min_shifted_zeros &= 0x1f;
            getbits (&wps->float_max_shifted_ones, 5, &wps->wvxbits);
            wps->float_max_shifted_ones &= 0x1f;
        }
        else {
            getbits (&wps->int32_max_width, 5, &wps->wvxbits);
            wps->int32_max_width &= 0x1f;
        }
    }

    return TRUE;
}

// Read the int32 data from the specified metadata into the specified stream.
// This data is used for integer data that has more than 24 bits of magnitude
// or, in some cases, used to eliminate redundant bits from any audio stream.

static int read_int32_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    char *byteptr = (char *)wpmd->data;

    if (bytecnt != 4)
        return FALSE;

    wps->int32_sent_bits = *byteptr++;
    wps->int32_zeros = *byteptr++;
    wps->int32_ones = *byteptr++;
    wps->int32_dups = *byteptr;

    return TRUE;
}

static int read_float_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    char *byteptr = (char *)wpmd->data;

    if (bytecnt != 4)
        return FALSE;

    wps->float_flags = *byteptr++;
    wps->float_shift = *byteptr++;
    wps->float_max_exp = *byteptr++;
    wps->float_norm_exp = *byteptr;
    return TRUE;
}

// Read multichannel information from metadata. The first byte is the total
// number of channels and the following bytes represent the channel_mask
// as described for Microsoft WAVEFORMATEX.

static int read_channel_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length, shift = 0, mask_bits;
    unsigned char *byteptr = (unsigned char *)wpmd->data;
    uint32_t mask = 0;

    if (!bytecnt || bytecnt > 7)
        return FALSE;

    if (!wpc->config.num_channels) {

        // if bytecnt is 6 or 7 we are using new configuration with "unlimited" streams

        if (bytecnt >= 6) {
            wpc->config.num_channels = (byteptr [0] | ((byteptr [2] & 0xf) << 8)) + 1;
            wpc->max_streams = (byteptr [1] | ((byteptr [2] & 0xf0) << 4)) + 1;

            if (wpc->config.num_channels < wpc->max_streams)
                return FALSE;

            byteptr += 3;
            mask = *byteptr++;
            mask |= (uint32_t) *byteptr++ << 8;
            mask |= (uint32_t) *byteptr++ << 16;

            if (bytecnt == 7)                           // this was introduced in 5.0
                mask |= (uint32_t) *byteptr << 24;
        }
        else {
            wpc->config.num_channels = *byteptr++;

            while (--bytecnt) {
                mask |= (uint32_t) *byteptr++ << shift;
                shift += 8;
            }

            // FFmpeg 6.1 generates non-compliant multichannel WavPack files that do not use the
            // new (2016) channel format form introduced in WavPack 5.0. This hack allows those
            // files to be decoded by the reference decoder. Surprisingly, the last time I
            // checked, FFmpeg itself can't decode many of them either!

            if (wpc->config.num_channels > OLD_MAX_STREAMS)
                wpc->max_streams = wpc->config.num_channels;
        }

        if (wpc->config.num_channels > wpc->max_streams * 2)
            return FALSE;

        wpc->config.channel_mask = mask;

        for (mask_bits = 0; mask; mask >>= 1)
            if ((mask & 1) && ++mask_bits > wpc->config.num_channels)
                return FALSE;
    }

    return TRUE;
}

// Read multichannel identity information from metadata. Data is an array of
// unsigned characters representing any channels in the file that DO NOT
// match one the 18 Microsoft standard channels (and are represented in the
// channel mask). A value of 0 is not allowed and 0xff means an unknown or
// undefined channel identity.

static int read_channel_identities (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    unsigned char *idents = (unsigned char *) wpmd->data;  /* XARCHIVE-C++ */
    int i;

    if (!wpmd->data || !wpmd->byte_length)
        return FALSE;

    for (i = 0; i < wpmd->byte_length; ++i)
        if (!idents [i])
            return FALSE;

    if (!wpc->channel_identities) {
        wpc->channel_identities = (unsigned char *)malloc (wpmd->byte_length + 1);
        memcpy (wpc->channel_identities, wpmd->data, wpmd->byte_length);
        wpc->channel_identities [wpmd->byte_length] = 0;
    }

    return TRUE;
}

// Read configuration information from metadata.

static int read_config_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    unsigned char *byteptr = (unsigned char *)wpmd->data;

    if (bytecnt >= 3) {
        wpc->config.flags &= 0xff;
        wpc->config.flags |= (uint32_t) *byteptr++ << 8;
        wpc->config.flags |= (uint32_t) *byteptr++ << 16;
        wpc->config.flags |= (uint32_t) *byteptr++ << 24;
        bytecnt -= 3;

        if (bytecnt && (wpc->config.flags & CONFIG_EXTRA_MODE)) {
            wpc->config.xmode = *byteptr++;
            bytecnt--;
        }

        // we used an extra config byte here for the 5.0.0 alpha, so still
        // honor it now (but this has been replaced with NEW_CONFIG)

        if (bytecnt) {
            wpc->config.qmode = (wpc->config.qmode & ~0xff) | *byteptr;
            wpc->version_five = 1;
        }
    }

    return TRUE;
}

// Read "new" configuration information from metadata.

static int read_new_config_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    unsigned char *byteptr = (unsigned char *)wpmd->data;

    wpc->version_five = 1;      // just having this block signals version 5.0

    wpc->file_format = wpc->config.qmode = wpc->channel_layout = 0;

    if (wpc->channel_reordering) {
        free (wpc->channel_reordering);
        wpc->channel_reordering = NULL;
    }

    // if there's any data, the first two bytes are file_format and qmode flags

    if (bytecnt >= 2) {
        wpc->file_format = *byteptr++;
        wpc->config.qmode = (wpc->config.qmode & ~0xff) | *byteptr++;
        bytecnt -= 2;

        // another byte indicates a channel layout

        if (bytecnt) {
            int nchans, i;

            wpc->channel_layout = (uint32_t) *byteptr++ << 16;
            bytecnt--;

            // another byte means we have a channel count for the layout and maybe a reordering

            if (bytecnt) {
                wpc->channel_layout += nchans = *byteptr++;
                bytecnt--;

                // any more means there's a reordering string

                if (bytecnt) {
                    if (bytecnt > nchans)
                        return FALSE;

                    wpc->channel_reordering = (unsigned char *)malloc (nchans);

                    // note that redundant reordering info is not stored, so we fill in the rest

                    if (wpc->channel_reordering) {
                        for (i = 0; i < nchans; ++i)
                            if (bytecnt) {
                                wpc->channel_reordering [i] = *byteptr++;

                                if (wpc->channel_reordering [i] >= nchans)  // make sure index is in range
                                    wpc->channel_reordering [i] = 0;

                                bytecnt--;
                            }
                            else
                                wpc->channel_reordering [i] = i;
                    }
                }
            }
            else
                wpc->channel_layout += wpc->config.num_channels;
        }
    }

    return TRUE;
}

// Read non-standard sampling rate from metadata.

static int read_sample_rate (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    unsigned char *byteptr = (unsigned char *)wpmd->data;

    if (bytecnt == 3 || bytecnt == 4) {
        wpc->config.sample_rate = (int32_t) *byteptr++;
        wpc->config.sample_rate |= (int32_t) *byteptr++ << 8;
        wpc->config.sample_rate |= (int32_t) *byteptr++ << 16;

        // for sampling rates > 16777215 (non-audio probably, or ...)

        if (bytecnt == 4)
            wpc->config.sample_rate |= (int32_t) (*byteptr & 0x7f) << 24;
    }

    return TRUE;
}

// Read wrapper data from metadata. Currently, this consists of the RIFF
// header and trailer that wav files contain around the audio data but could
// be used for other formats as well. Because WavPack files contain all the
// information required for decoding and playback, this data can probably
// be ignored except when an exact wavefile restoration is needed.

static int read_wrapper_data (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    if ((wpc->open_flags & OPEN_WRAPPER) && wpc->wrapper_bytes < MAX_WRAPPER_BYTES && wpmd->byte_length) {
        wpc->wrapper_data = (unsigned char *)realloc (wpc->wrapper_data, wpc->wrapper_bytes + wpmd->byte_length);
	if (!wpc->wrapper_data)
	    return FALSE;
        memcpy (wpc->wrapper_data + wpc->wrapper_bytes, wpmd->data, wpmd->byte_length);
        wpc->wrapper_bytes += wpmd->byte_length;
    }

    return TRUE;
}

static int read_metadata_buff (WavpackMetadata *wpmd, unsigned char *blockbuff, unsigned char **buffptr)
{
    WavpackHeader *wphdr = (WavpackHeader *) blockbuff;
    unsigned char *buffend = blockbuff + wphdr->ckSize + 8;

    if (buffend - *buffptr < 2)
        return FALSE;

    wpmd->id = *(*buffptr)++;
    wpmd->byte_length = *(*buffptr)++ << 1;

    if (wpmd->id & ID_LARGE) {
        wpmd->id &= ~ID_LARGE;

        if (buffend - *buffptr < 2)
            return FALSE;

        wpmd->byte_length += *(*buffptr)++ << 9;
        wpmd->byte_length += *(*buffptr)++ << 17;
    }

    if (wpmd->id & ID_ODD_SIZE) {
        if (!wpmd->byte_length)         // odd size and zero length makes no sense
            return FALSE;
        wpmd->id &= ~ID_ODD_SIZE;
        wpmd->byte_length--;
    }

    if (wpmd->byte_length) {
        if (buffend - *buffptr < wpmd->byte_length + (wpmd->byte_length & 1)) {
            wpmd->data = NULL;
            return FALSE;
        }

        wpmd->data = *buffptr;
        (*buffptr) += wpmd->byte_length + (wpmd->byte_length & 1);
    }
    else
        wpmd->data = NULL;

    return TRUE;
}

static int process_metadata (WavpackContext *wpc, WavpackMetadata *wpmd, int stream)
{
    WavpackStream *wps = wpc->streams [stream];

    switch (wpmd->id) {
        case ID_DUMMY:
            return TRUE;

        case ID_DECORR_TERMS:
            return read_decorr_terms (wps, wpmd);

        case ID_DECORR_WEIGHTS:
            return read_decorr_weights (wps, wpmd);

        case ID_DECORR_SAMPLES:
            return read_decorr_samples (wps, wpmd);

        case ID_ENTROPY_VARS:
            return read_entropy_vars (wps, wpmd);

        case ID_HYBRID_PROFILE:
            return read_hybrid_profile (wps, wpmd);

        case ID_SHAPING_WEIGHTS:
            return read_shaping_info (wps, wpmd);

        case ID_FLOAT_INFO:
            return read_float_info (wps, wpmd);

        case ID_INT32_INFO:
            return read_int32_info (wps, wpmd);

        case ID_CHANNEL_INFO:
            return read_channel_info (wpc, wpmd);

        case ID_CHANNEL_IDENTITIES:
            return read_channel_identities (wpc, wpmd);

        case ID_CONFIG_BLOCK:
            return read_config_info (wpc, wpmd);

        case ID_NEW_CONFIG_BLOCK:
            return read_new_config_info (wpc, wpmd);

        case ID_SAMPLE_RATE:
            return read_sample_rate (wpc, wpmd);

        case ID_WV_BITSTREAM:
            return init_wv_bitstream (wps, wpmd);

        case ID_WVC_BITSTREAM:
            return init_wvc_bitstream (wps, wpmd);

        case ID_WVX_BITSTREAM:
        case ID_WVX_NEW_BITSTREAM:
            return init_wvx_bitstream (wps, wpmd);

        case ID_DSD_BLOCK:
#ifdef ENABLE_DSD
            return init_dsd_block (wps, wpmd);
#else
            strcpy (wpc->error_message, "not configured to handle DSD WavPack files!");
            return FALSE;
#endif

        case ID_ALT_HEADER: case ID_ALT_TRAILER:
            if (!(wpc->open_flags & OPEN_ALT_TYPES))
                return TRUE;

            // fall through
        case ID_RIFF_HEADER: case ID_RIFF_TRAILER:
            return read_wrapper_data (wpc, wpmd);

        case ID_ALT_MD5_CHECKSUM:
            if (!(wpc->open_flags & OPEN_ALT_TYPES))
                return TRUE;

            // fall through
        case ID_MD5_CHECKSUM:
            if (wpmd->byte_length == 16) {
                memcpy (wpc->config.md5_checksum, wpmd->data, 16);
                wpc->config.flags |= CONFIG_MD5_CHECKSUM;
                wpc->config.md5_read = 1;
            }

            return TRUE;

        case ID_ALT_EXTENSION:
            if (wpmd->byte_length && wpmd->byte_length < sizeof (wpc->file_extension)) {
                int i, j;

                for (i = j = 0; i < wpmd->byte_length; ++i)
                    if (isalnum (((char *) wpmd->data) [i]))
                        wpc->file_extension [j++] = ((char *) wpmd->data) [i];

                wpc->file_extension [j] = 0;
            }

            return TRUE;

        // we don't actually verify the checksum here (it's done right after the
        // block is read), but it's a good indicator of version 5 files

        case ID_BLOCK_CHECKSUM:
            wpc->version_five = 1;
            return TRUE;

        default:
            return (wpmd->id & ID_OPTIONAL_DATA) ? TRUE : FALSE;
    }
}

//////////////////////////////// bitstream management ///////////////////////////////

// Open the specified BitStream and associate with the specified buffer.

static void bs_read (Bitstream *bs);

static void bs_open_read (Bitstream *bs, void *buffer_start, void *buffer_end)
{
    bs->error = bs->sr = bs->bc = 0;
    bs->ptr = (bs->buf = (decltype(bs->buf)) buffer_start) - 1;  /* XARCHIVE-C++ */
    bs->end = (decltype(bs->end)) buffer_end;  /* XARCHIVE-C++ */
    bs->wrap = bs_read;
}

// This function is only called from the getbit() and getbits() macros when
// the BitStream has been exhausted and more data is required. Since these
// bistreams no longer access files, this function simple sets an error and
// resets the buffer.

static void bs_read (Bitstream *bs)
{
    bs->ptr = bs->buf;
    bs->error = 1;
}

// This function is called to close the bitstream. It returns the number of
// full bytes actually read as bits.

uint32_t bs_close_read (Bitstream *bs)
{
    uint32_t bytes_read;

    if (bs->bc < sizeof (*(bs->ptr)) * 8)
        bs->ptr++;

    bytes_read = (uint32_t)(bs->ptr - bs->buf) * sizeof (*(bs->ptr));

    if (!(bytes_read & 1))
        ++bytes_read;

    CLEAR (*bs);
    return bytes_read;
}

// Normally the trailing wrapper will not be available when a WavPack file is first
// opened for reading because it is stored in the final block of the file. This
// function forces a seek to the end of the file to pick up any trailing wrapper
// stored there (then use WavPackGetWrapper**() to obtain). This can obviously only
// be used for seekable files (not pipes) and is not available for pre-4.0 WavPack
// files.

void WavpackSeekTrailingWrapper (WavpackContext *wpc)
{
    if ((wpc->open_flags & OPEN_WRAPPER) &&
        wpc->reader->can_seek (wpc->wv_in) && !wpc->stream3)
            seek_eof_information (wpc, NULL, TRUE);
}

// Get any MD5 checksum stored in the metadata (should be called after reading
// last sample or an extra seek will occur). A return value of FALSE indicates
// that no MD5 checksum was stored.

int WavpackGetMD5Sum (WavpackContext *wpc, unsigned char data [16])
{
    if (wpc->config.flags & CONFIG_MD5_CHECKSUM) {
        if (!wpc->config.md5_read && wpc->reader->can_seek (wpc->wv_in))
            seek_eof_information (wpc, NULL, FALSE);

        if (wpc->config.md5_read) {
            memcpy (data, wpc->config.md5_checksum, 16);
            return TRUE;
        }
    }

    return FALSE;
}

// Read from current file position until a valid 32-byte WavPack 4.0 header is
// found and read into the specified pointer. The number of bytes skipped is
// returned. If no WavPack header is found within 1 meg, then a -1 is returned
// to indicate the error. No additional bytes are read past the header and it
// is returned in the processor's native endian mode. Seeking is not required.

uint32_t read_next_header (WavpackStreamReader64 *reader, void *id, WavpackHeader *wphdr)
{
    unsigned char buffer [sizeof (*wphdr)], *sp = buffer + sizeof (*wphdr), *ep = sp;
    uint32_t bytes_skipped = 0;
    int bleft;

    while (1) {
        if (sp < ep) {
            bleft = (int)(ep - sp);
            memmove (buffer, sp, bleft);
        }
        else
            bleft = 0;

        if (reader->read_bytes (id, buffer + bleft, sizeof (*wphdr) - bleft) != sizeof (*wphdr) - bleft)
            return -1;

        sp = buffer;

        if (*sp++ == 'w' && *sp == 'v' && *++sp == 'p' && *++sp == 'k' &&
            !(*++sp & 1) && sp [2] < 16 && !sp [3] && (sp [2] || sp [1] || *sp >= 24) && sp [5] == 4 &&
            sp [4] >= (MIN_STREAM_VERS & 0xff) && sp [4] <= (MAX_STREAM_VERS & 0xff) && sp [18] < 3 && !sp [19]) {
                memcpy (wphdr, buffer, sizeof (*wphdr));
                WavpackLittleEndianToNative (wphdr, (char *)WavpackHeaderFormat);
                return bytes_skipped;
            }

        while (sp < ep && *sp != 'w')
            sp++;

        if ((bytes_skipped += (uint32_t)(sp - buffer)) > 1024 * 1024)
            return -1;
    }
}

// Compare the regular wv file block header to a potential matching wvc
// file block header and return action code based on analysis:
//
//   0 = use wvc block (assuming rest of block is readable)
//   1 = bad match; try to read next wvc block
//  -1 = bad match; ignore wvc file for this block and backup fp (if
//       possible) and try to use this block next time

static int match_wvc_header (WavpackHeader *wv_hdr, WavpackHeader *wvc_hdr)
{
    if (GET_BLOCK_INDEX (*wv_hdr) == GET_BLOCK_INDEX (*wvc_hdr) &&
        wv_hdr->block_samples == wvc_hdr->block_samples) {
            int wvi = 0, wvci = 0;

            if (wv_hdr->flags == wvc_hdr->flags)
                return 0;

            if (wv_hdr->flags & INITIAL_BLOCK)
                wvi -= 1;

            if (wv_hdr->flags & FINAL_BLOCK)
                wvi += 1;

            if (wvc_hdr->flags & INITIAL_BLOCK)
                wvci -= 1;

            if (wvc_hdr->flags & FINAL_BLOCK)
                wvci += 1;

            return (wvci - wvi < 0) ? 1 : -1;
        }

    if ((GET_BLOCK_INDEX (*wvc_hdr) - GET_BLOCK_INDEX (*wv_hdr)) & 0x8000000000LL)
        return 1;
    else
        return -1;
}

// Read the wvc block that matches the regular wv block that has been
// read for the current stream. If an exact match is not found then
// we either keep reading or back up and (possibly) use the block
// later. The skip_wvc flag is set if not matching wvc block is found
// so that we can still decode using only the lossy version (although
// we flag this as an error). A return of FALSE indicates a serious
// error (not just that we missed one wvc block).

int read_wvc_block (WavpackContext *wpc, int stream)
{
    WavpackStream *wps = wpc->streams [stream];
    int64_t bcount, file2pos;
    WavpackHeader orig_wphdr;
    WavpackHeader wphdr;
    int compare_result;

    while (1) {
        file2pos = wpc->reader->get_pos (wpc->wvc_in);
        bcount = read_next_header (wpc->reader, wpc->wvc_in, &wphdr);

        if (bcount == (uint32_t) -1) {
            wps->wvc_skip = TRUE;
            wpc->crc_errors++;
            return FALSE;
        }

        memcpy (&orig_wphdr, &wphdr, 32);       // save original header for verify step

        if (wpc->open_flags & OPEN_STREAMING)
            SET_BLOCK_INDEX (wphdr, wps->sample_index = 0);
        else
            SET_BLOCK_INDEX (wphdr, GET_BLOCK_INDEX (wphdr) - wpc->initial_index);

        if (wphdr.flags & INITIAL_BLOCK)
            wpc->file2pos = file2pos + bcount;

        compare_result = match_wvc_header (&wps->wphdr, &wphdr);

        if (!compare_result) {
            wps->block2buff = (unsigned char *)malloc (wphdr.ckSize + 8);
	    if (!wps->block2buff)
	        return FALSE;

            if (wpc->reader->read_bytes (wpc->wvc_in, wps->block2buff + 32, wphdr.ckSize - 24) !=
                wphdr.ckSize - 24) {
                    free (wps->block2buff);
                    wps->block2buff = NULL;
                    wps->wvc_skip = TRUE;
                    wpc->crc_errors++;
                    return FALSE;
            }

            memcpy (wps->block2buff, &orig_wphdr, 32);

            // don't use corrupt blocks
            if (!WavpackVerifySingleBlock (wps->block2buff, !(wpc->open_flags & OPEN_NO_CHECKSUM))) {
                free (wps->block2buff);
                wps->block2buff = NULL;
                wps->wvc_skip = TRUE;
                wpc->crc_errors++;
                return TRUE;
            }

            wps->wvc_skip = FALSE;
            memcpy (wps->block2buff, &wphdr, 32);
            memcpy (&wps->wphdr, &wphdr, 32);
            return TRUE;
        }
        else if (compare_result == -1) {
            wps->wvc_skip = TRUE;
            wpc->reader->set_pos_rel (wpc->wvc_in, -32, SEEK_CUR);
            wpc->crc_errors++;
            return TRUE;
        }
    }
}

// This function is used to seek to end of a file to obtain certain information
// that is stored there at the file creation time because it is not known at
// the start. This includes the MD5 sum and and trailing part of the file
// wrapper, and in some rare cases may include the total number of samples in
// the file (although we usually try to back up and write that at the front of
// the file). Note this function restores the file position to its original
// location (and obviously requires a seekable file). The normal return value
// is TRUE indicating no errors, although this does not actually mean that any
// information was retrieved. An error return of FALSE usually means the file
// terminated unexpectedly. Note that this could be used to get all three
// types of information in one go, but it's not actually used that way now.

static int seek_eof_information (WavpackContext *wpc, int64_t *final_index, int get_wrapper)
{
    int64_t restore_pos, last_pos = -1;
    WavpackStreamReader64 *reader = wpc->reader;
    int alt_types = wpc->open_flags & OPEN_ALT_TYPES;
    uint32_t blocks = 0, audio_blocks = 0;
    void *id = wpc->wv_in;
    WavpackHeader wphdr;

    restore_pos = reader->get_pos (id);    // we restore file position when done

    // start 1MB from the end-of-file, or from the start if the file is not that big

    if (reader->get_length (id) > (int64_t) 1048576)
        reader->set_pos_rel (id, -1048576, SEEK_END);
    else
        reader->set_pos_abs (id, 0);

    // Note that we go backward (without parsing inside blocks) until we find a block
    // with audio (careful to not get stuck in a loop). Only then do we go forward
    // parsing all blocks in their entirety.

    while (1) {
        uint32_t bcount = read_next_header (reader, id, &wphdr);
        int64_t current_pos = reader->get_pos (id);

        // if we just got to the same place as last time, we're stuck and need to give up

        if (current_pos == last_pos) {
            reader->set_pos_abs (id, restore_pos);
            return FALSE;
        }

        last_pos = current_pos;

        // We enter here if we just read 1 MB without seeing any WavPack block headers.
        // Since WavPack blocks are < 1 MB, that means we're in a big APE tag, or we got
        // to the end-of-file.

        if (bcount == (uint32_t) -1) {

            // if we have not seen any blocks at all yet, back up almost 2 MB (or to the
            // beginning of the file) and try again

            if (!blocks) {
                if (current_pos > (int64_t) 2000000)
                    reader->set_pos_rel (id, -2000000, SEEK_CUR);
                else
                    reader->set_pos_abs (id, 0);

                continue;
            }

            // if we have seen WavPack blocks, then this means we've done all we can do here

            reader->set_pos_abs (id, restore_pos);
            return TRUE;
        }

        blocks++;

        // If the block has audio samples, calculate a final index, although this is not
        // final since this may not be the last block with audio. On the other hand, if
        // this block does not have audio, and we haven't seen one with audio, we have
        // to go back some more.

        if (wphdr.block_samples) {
            if (final_index)
                *final_index = GET_BLOCK_INDEX (wphdr) + wphdr.block_samples;

            audio_blocks++;
        }
        else if (!audio_blocks) {
            if (current_pos > (int64_t) 1048576)
                reader->set_pos_rel (id, -1048576, SEEK_CUR);
            else
                reader->set_pos_abs (id, 0);

            continue;
        }

        // at this point we have seen at least one block with audio, so we parse the
        // entire block looking for MD5 metadata or (conditionally) trailing wrappers

        bcount = wphdr.ckSize - sizeof (WavpackHeader) + 8;

        while (bcount >= 2) {
            unsigned char meta_id, c1, c2;
            uint32_t meta_bc, meta_size;

            if (reader->read_bytes (id, &meta_id, 1) != 1 ||
                reader->read_bytes (id, &c1, 1) != 1) {
                    reader->set_pos_abs (id, restore_pos);
                    return FALSE;
            }

            meta_bc = c1 << 1;
            bcount -= 2;

            if (meta_id & ID_LARGE) {
                if (bcount < 2 || reader->read_bytes (id, &c1, 1) != 1 ||
                    reader->read_bytes (id, &c2, 1) != 1) {
                        reader->set_pos_abs (id, restore_pos);
                        return FALSE;
                }

                meta_bc += ((uint32_t) c1 << 9) + ((uint32_t) c2 << 17);
                bcount -= 2;
            }

            meta_size = (meta_id & ID_ODD_SIZE) ? meta_bc - 1 : meta_bc;
            meta_id &= ID_UNIQUE;

            if (get_wrapper && (meta_id == ID_RIFF_TRAILER || (alt_types && meta_id == ID_ALT_TRAILER)) && meta_bc) {
                wpc->wrapper_data = (unsigned char *)realloc (wpc->wrapper_data, wpc->wrapper_bytes + meta_bc);

                if (!wpc->wrapper_data) {
                    reader->set_pos_abs (id, restore_pos);
                    return FALSE;
                }

                if (reader->read_bytes (id, wpc->wrapper_data + wpc->wrapper_bytes, meta_bc) == meta_bc)
                    wpc->wrapper_bytes += meta_size;
                else {
                    reader->set_pos_abs (id, restore_pos);
                    return FALSE;
                }
            }
            else if (meta_id == ID_MD5_CHECKSUM || (alt_types && meta_id == ID_ALT_MD5_CHECKSUM)) {
                if (meta_bc == 16 && bcount >= 16) {
                    if (reader->read_bytes (id, wpc->config.md5_checksum, 16) == 16)
                        wpc->config.md5_read = TRUE;
                    else {
                        reader->set_pos_abs (id, restore_pos);
                        return FALSE;
                    }
                }
                else
                    reader->set_pos_rel (id, meta_bc, SEEK_CUR);
            }
            else
                reader->set_pos_rel (id, meta_bc, SEEK_CUR);

            bcount -= meta_bc;
        }
    }
}

// Quickly verify the referenced block. It is assumed that the WavPack header has been converted
// to native endian format. If a block checksum is performed, that is done in little-endian
// (file) format. It is also assumed that the caller has made sure that the block length
// indicated in the header is correct (we won't overflow the buffer). If a checksum is present,
// then it is checked, otherwise we just check that all the metadata blocks are formatted
// correctly (without looking at their contents). Returns FALSE for bad block.

int WavpackVerifySingleBlock (unsigned char *buffer, int verify_checksum)
{
    WavpackHeader *wphdr = (WavpackHeader *) buffer;
    uint32_t checksum_passed = 0, bcount, meta_bc;
    unsigned char *dp, meta_id, c1, c2;

    if (strncmp (wphdr->ckID, "wvpk", 4) || wphdr->ckSize + 8 < sizeof (WavpackHeader))
        return FALSE;

    bcount = wphdr->ckSize - sizeof (WavpackHeader) + 8;
    dp = (unsigned char *)(wphdr + 1);

    while (bcount >= 2) {
        meta_id = *dp++;
        c1 = *dp++;

        meta_bc = c1 << 1;
        bcount -= 2;

        if (meta_id & ID_LARGE) {
            if (bcount < 2)
                return FALSE;

            c1 = *dp++;
            c2 = *dp++;
            meta_bc += ((uint32_t) c1 << 9) + ((uint32_t) c2 << 17);
            bcount -= 2;
        }

        if (bcount < meta_bc)
            return FALSE;

        if (verify_checksum && (meta_id & ID_UNIQUE) == ID_BLOCK_CHECKSUM) {
#ifdef BITSTREAM_SHORTS
            uint16_t *csptr = (uint16_t*) buffer;
#else
            unsigned char *csptr = buffer;
#endif
            int wcount = (int)(dp - 2 - buffer) >> 1;
            uint32_t csum = (uint32_t) -1;

            if ((meta_id & ID_ODD_SIZE) || meta_bc < 2 || meta_bc > 4)
                return FALSE;

#ifdef BITSTREAM_SHORTS
            while (wcount--)
                csum = (csum * 3) + *csptr++;
#else
            WavpackNativeToLittleEndian ((WavpackHeader *) buffer, (char *)WavpackHeaderFormat);

            while (wcount--) {
                csum = (csum * 3) + csptr [0] + (csptr [1] << 8);
                csptr += 2;
            }

            WavpackLittleEndianToNative ((WavpackHeader *) buffer, (char *)WavpackHeaderFormat);
#endif

            if (meta_bc == 4) {
                if (*dp != (csum & 0xff) || dp[1] != ((csum >> 8) & 0xff) || dp[2] != ((csum >> 16) & 0xff) || dp[3] != ((csum >> 24) & 0xff))
                    return FALSE;
            }
            else {
                csum ^= csum >> 16;

                if (*dp != (csum & 0xff) || dp[1] != ((csum >> 8) & 0xff))
                    return FALSE;
            }

            checksum_passed++;
        }

        bcount -= meta_bc;
        dp += meta_bc;
    }

    return (bcount == 0) && (!verify_checksum || !(wphdr->flags & HAS_CHECKSUM) || checksum_passed);
}

/* ===================== src/common_utils.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// common_utils.c

// This module provides a lot of the trivial WavPack API functions and several
// functions that are common to both reading and writing WavPack files (like
// WavpackCloseFile()). Functions here are restricted to those that have few
// external dependencies and this is done so that applications that statically
// link to the WavPack library (like the command-line utilities on Windows)
// do not need to include the entire library image if they only use a subset
// of it. This module will be loaded for ANY WavPack application.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#ifndef LIBWAVPACK_VERSION_STRING
#endif

///////////////////////////// local table storage ////////////////////////////

const uint32_t sample_rates [] = { 6000, 8000, 9600, 11025, 12000, 16000, 22050,
    24000, 32000, 44100, 48000, 64000, 88200, 96000, 192000 };

///////////////////////////// executable code ////////////////////////////////

// This function obtains general information about an open input file and
// returns a mask with the following bit values:

// MODE_WVC:  a .wvc file has been found and will be used for lossless
// MODE_LOSSLESS:  file is lossless (either pure or hybrid)
// MODE_HYBRID:  file is hybrid mode (either lossy or lossless)
// MODE_FLOAT:  audio data is 32-bit ieee floating point
// MODE_VALID_TAG:  file contains a valid ID3v1 or APEv2 tag
// MODE_HIGH:  file was created in "high" mode (information only)
// MODE_FAST:  file was created in "fast" mode (information only)
// MODE_EXTRA:  file was created using "extra" mode (information only)
// MODE_APETAG:  file contains a valid APEv2 tag
// MODE_SFX:  file was created as a "self-extracting" executable
// MODE_VERY_HIGH:  file was created in the "very high" mode (or in
//                  the "high" mode prior to 4.4)
// MODE_MD5:  file contains an MD5 checksum
// MODE_XMODE:  level used for extra mode (1-6, 0=unknown)
// MODE_DNS:  dynamic noise shaping

int WavpackGetMode (WavpackContext *wpc)
{
    int mode = 0;

    if (wpc) {
        if (wpc->config.flags & CONFIG_HYBRID_FLAG)
            mode |= MODE_HYBRID;
        else if (!(wpc->config.flags & CONFIG_LOSSY_MODE))
            mode |= MODE_LOSSLESS;

        if (wpc->wvc_flag)
            mode |= (MODE_LOSSLESS | MODE_WVC);

        if (wpc->lossy_blocks)
            mode &= ~MODE_LOSSLESS;

        if (wpc->config.flags & CONFIG_FLOAT_DATA)
            mode |= MODE_FLOAT;

        if (wpc->config.flags & (CONFIG_HIGH_FLAG | CONFIG_VERY_HIGH_FLAG)) {
            mode |= MODE_HIGH;

            if ((wpc->config.flags & CONFIG_VERY_HIGH_FLAG) ||
                (wpc->streams && wpc->streams [0] && wpc->streams [0]->wphdr.version < 0x405))
                    mode |= MODE_VERY_HIGH;
        }

        if (wpc->config.flags & CONFIG_FAST_FLAG)
            mode |= MODE_FAST;

        if (wpc->config.flags & CONFIG_EXTRA_MODE)
            mode |= (MODE_EXTRA | (wpc->config.xmode << 12));

        if (wpc->config.flags & CONFIG_CREATE_EXE)
            mode |= MODE_SFX;

        if (wpc->config.flags & CONFIG_MD5_CHECKSUM)
            mode |= MODE_MD5;

        if ((wpc->config.flags & CONFIG_HYBRID_FLAG) && (wpc->config.flags & CONFIG_DYNAMIC_SHAPING) &&
            wpc->streams && wpc->streams [0] && wpc->streams [0]->wphdr.version >= 0x407)
                mode |= MODE_DNS;

#ifndef NO_TAGS
        if (valid_tag (&wpc->m_tag)) {
            mode |= MODE_VALID_TAG;

            if (valid_tag (&wpc->m_tag) == 'A')
                mode |= MODE_APETAG;
        }
#endif

        mode |= (wpc->config.qmode << 16) & 0xFF0000;
    }

    return mode;
}

// This function obtains information about specific file features that were
// added for version 5.0, specifically qualifications added to support CAF
// and DSD files. Except for indicating the presence of DSD data, these
// bits are meant to simply indicate the format of the data in the original
// source file and do NOT indicate how the library will return the data to
// the application (which is always the same). This means that in general an
// application that simply wants to play or process the audio data need not
// be concerned about these. If the file is DSD audio, then either of the
// QMDOE_DSD_LSB_FIRST or QMODE_DSD_MSB_FIRST bits will be set (but the
// DSD audio is always returned to the caller MSB first).

// QMODE_BIG_ENDIAN        0x1     // big-endian data format (opposite of WAV format)
// QMODE_SIGNED_BYTES      0x2     // 8-bit audio data is signed (opposite of WAV format)
// QMODE_UNSIGNED_WORDS    0x4     // audio data (other than 8-bit) is unsigned (opposite of WAV format)
// QMODE_REORDERED_CHANS   0x8     // source channels were not Microsoft order, so they were reordered
// QMODE_DSD_LSB_FIRST     0x10    // DSD bytes, LSB first (most Sony .dsf files)
// QMODE_DSD_MSB_FIRST     0x20    // DSD bytes, MSB first (Philips .dff files)
// QMODE_DSD_IN_BLOCKS     0x40    // DSD data is blocked by channels (Sony .dsf only)

int WavpackGetQualifyMode (WavpackContext *wpc)
{
    return wpc->config.qmode & 0xFF;
}

// This function returns a pointer to a string describing the last error
// generated by WavPack.

char *WavpackGetErrorMessage (WavpackContext *wpc)
{
    return wpc->error_message;
}

// Get total number of samples contained in the WavPack file, or -1 if unknown

uint32_t WavpackGetNumSamples (WavpackContext *wpc)
{
    return (uint32_t) WavpackGetNumSamples64 (wpc);
}

int64_t WavpackGetNumSamples64 (WavpackContext *wpc)
{
    return wpc ? wpc->total_samples : -1;
}

// Get the current sample index position, or -1 if unknown

uint32_t WavpackGetSampleIndex (WavpackContext *wpc)
{
    return (uint32_t) WavpackGetSampleIndex64 (wpc);
}

int64_t WavpackGetSampleIndex64 (WavpackContext *wpc)
{
    if (wpc) {
#ifdef ENABLE_LEGACY
        if (wpc->stream3)
            return get_sample_index3 (wpc);
        else if (wpc->streams && wpc->streams [0])
            return wpc->streams [0]->sample_index;
#else
        if (wpc->streams && wpc->streams [0])
            return wpc->streams [0]->sample_index;
#endif
    }

    return -1;
}

// Get the number of errors encountered so far

int WavpackGetNumErrors (WavpackContext *wpc)
{
    return wpc ? wpc->crc_errors : 0;
}

// return TRUE if any uncorrected lossy blocks were actually written or read

int WavpackLossyBlocks (WavpackContext *wpc)
{
    return wpc ? wpc->lossy_blocks : 0;
}

// Calculate the progress through the file as a double from 0.0 (for begin)
// to 1.0 (for done). A return value of -1.0 indicates that the progress is
// unknown.

double WavpackGetProgress (WavpackContext *wpc)
{
    if (wpc && wpc->total_samples != -1 && wpc->total_samples != 0)
        return (double) WavpackGetSampleIndex64 (wpc) / wpc->total_samples;
    else
        return -1.0;
}

// Return the total size of the WavPack file(s) in bytes.

uint32_t WavpackGetFileSize (WavpackContext *wpc)
{
    return (uint32_t) (wpc ? wpc->filelen + wpc->file2len : 0);
}

int64_t WavpackGetFileSize64 (WavpackContext *wpc)
{
    return wpc ? wpc->filelen + wpc->file2len : 0;
}

// Get the actual number of total samples, or if writing a file this returns the number of
// samples written so far. This allows WavpackGetRatio() and WavpackGetAverageBitrate() to
// work during writing and specifically in cases where the initial length was unknown.

static int64_t actual_total_samples (WavpackContext *wpc)
{
    int64_t total_samples = wpc->total_samples;

    if (wpc->wv_out && wpc->streams && wpc->streams [0] && wpc->streams [0]->sample_index)
        total_samples = wpc->streams [0]->sample_index;

    return total_samples;
}

// Calculate the ratio of the specified WavPack file size to the size of the
// original audio data as a double greater than 0.0 and (usually) smaller than
// 1.0. A value greater than 1.0 represents "negative" compression and a
// return value of 0.0 indicates that the ratio cannot be determined.

double WavpackGetRatio (WavpackContext *wpc)
{
    if (wpc && actual_total_samples (wpc) != -1 && wpc->filelen) {
        double output_size = (double) actual_total_samples (wpc) * wpc->config.num_channels *
            wpc->config.bytes_per_sample;
        double input_size = (double) wpc->filelen + wpc->file2len;

        if (output_size >= 1.0 && input_size >= 1.0)
            return input_size / output_size;
    }

    return 0.0;
}

// Calculate the average bitrate of the WavPack file in bits per second. A
// return of 0.0 indicates that the bitrate cannot be determined. An option is
// provided to use (or not use) any attendant .wvc file.

double WavpackGetAverageBitrate (WavpackContext *wpc, int count_wvc)
{
    if (wpc && actual_total_samples (wpc) != -1 && wpc->filelen && WavpackGetSampleRate (wpc)) {
        double output_time = (double) actual_total_samples (wpc) / WavpackGetSampleRate (wpc);
        double input_size = (double) wpc->filelen + (count_wvc ? wpc->file2len : 0);

        if (output_time >= 0.1 && input_size >= 1.0)
            return input_size * 8.0 / output_time;
    }

    return 0.0;
}

// Calculate the bitrate of the current WavPack file block in bits per second.
// This can be used for an "instant" bit display and gets updated from about
// 1 to 4 times per second. A return of 0.0 indicates that the bitrate cannot
// be determined.

double WavpackGetInstantBitrate (WavpackContext *wpc)
{
    if (wpc && wpc->stream3)
        return WavpackGetAverageBitrate (wpc, TRUE);

    if (wpc && wpc->streams && wpc->streams [0] && wpc->streams [0]->wphdr.block_samples && WavpackGetSampleRate (wpc)) {
        double output_time = (double) wpc->streams [0]->wphdr.block_samples / WavpackGetSampleRate (wpc);
        double input_size = 0;
        int si;

        for (si = 0; si < wpc->num_streams; ++si) {
            if (wpc->streams [si]->blockbuff)
                input_size += ((WavpackHeader *) wpc->streams [si]->blockbuff)->ckSize;

            if (wpc->streams [si]->block2buff)
                input_size += ((WavpackHeader *) wpc->streams [si]->block2buff)->ckSize;
        }

        if (output_time > 0.0 && input_size >= 1.0)
            return input_size * 8.0 / output_time;
    }

    return 0.0;
}

// This function allows retrieving the Core Audio File channel layout, many of which do not
// conform to the Microsoft ordering standard that WavPack requires internally (at least for
// those channels present in the "channel mask"). In addition to the layout tag, this function
// returns the reordering string (if stored in the file) to allow the unpacker to reorder the
// channels back to the specified layout (if it wants to restore the CAF order). The number of
// channels in the layout is determined from the lower nybble of the layout word (and should
// probably match the number of channels in the file), and if a reorder string is requested
// then that much space must be allocated. Note that all the reordering is actually done
// outside of this library, and that if reordering is done then the appropriate qmode bit
// will be set.
//
// Note: Normally this function would not be used by an application unless it specifically
// wanted to restore a non-standard channel order (to check an MD5, for example) or obtain
// the Core Audio channel layout ID. For simple file decoding for playback, the channel_mask
// should provide all the information required unless there are non-Microsoft channels
// involved, in which case WavpackGetChannelIdentities() will provide the identities of
// the other channels (if they are known).

uint32_t WavpackGetChannelLayout (WavpackContext *wpc, unsigned char *reorder)
{
    if ((wpc->channel_layout & 0xff) && wpc->channel_reordering && reorder)
        memcpy (reorder, wpc->channel_reordering, wpc->channel_layout & 0xff);

    return wpc->channel_layout;
}

// This function provides the identities of ALL the channels in the file, including the
// standard Microsoft channels (which come first, in order, and are numbered 1-18) and also
// any non-Microsoft channels (which can be in any order and have values from 33-254). The
// value 0x00 is invalid and 0xFF indicates an "unknown" or "unassigned" channel. The
// string is NULL terminated so the caller must supply enough space for the number
// of channels indicated by WavpackGetNumChannels(), plus one.
//
// Note that this function returns the actual order of the channels in the Wavpack file
// (i.e., the order returned by WavpackUnpackSamples()). If the file includes a "reordering"
// string because the source file was not in Microsoft order that is NOT taken into account
// here and really only needs to be considered if doing an MD5 verification or if it's
// required to restore the original order/file (like wvunpack does).

void WavpackGetChannelIdentities (WavpackContext *wpc, unsigned char *identities)
{
    int num_channels = wpc->config.num_channels, index = 1;
    uint32_t channel_mask = wpc->config.channel_mask;
    unsigned char *src = wpc->channel_identities;

    while (num_channels--) {
        if (channel_mask) {
            while (!(channel_mask & 1)) {
                channel_mask >>= 1;
                index++;
            }

            *identities++ = index++;
            channel_mask >>= 1;
        }
        else if (src && *src)
            *identities++ = *src++;
        else
            *identities++ = 0xff;
    }

    *identities = 0;
}

#ifdef ENABLE_THREADS

static void worker_threads_destroy (WavpackContext *wpc)
{
    if (wpc->workers) {
        int i;

        for (i = 0; i < wpc->num_workers; ++i) {
            wp_mutex_obtain (wpc->mutex);
            wpc->workers [i].state = Quit;
            wp_condvar_signal (wpc->workers [i].worker_cond);
            wp_mutex_release (wpc->mutex);
            wp_thread_join (wpc->workers [i].thread);
            wp_thread_delete (wpc->workers [i].thread);
            wp_condvar_delete (wpc->workers [i].worker_cond);
        }

        free (wpc->workers);
        wpc->workers = NULL;
        wp_mutex_delete (wpc->mutex);
    }
}

#endif

// For local use only. Install a callback to be executed when WavpackCloseFile() is called,
// usually used to dump some statistics accumulated during encode or decode.

void install_close_callback (WavpackContext *wpc, void cb_func (void *wpc))
{
    wpc->close_callback = cb_func;
}

// Close the specified WavPack file and release all resources used by it.
// Returns NULL.

WavpackContext *WavpackCloseFile (WavpackContext *wpc)
{
    if (wpc->close_callback)
        wpc->close_callback (wpc);

    if (wpc->streams) {
        free_streams (wpc);

        if (wpc->streams [0])
            free (wpc->streams [0]);

        free (wpc->streams);
    }

#ifdef ENABLE_LEGACY
    if (wpc->stream3)
        free_stream3 (wpc);
#endif

    if (wpc->reader && wpc->reader->close && wpc->wv_in)
        wpc->reader->close (wpc->wv_in);

    if (wpc->reader && wpc->reader->close && wpc->wvc_in)
        wpc->reader->close (wpc->wvc_in);

    WavpackFreeWrapper (wpc);

    if (wpc->metadata) {
        int i;

        for (i = 0; i < wpc->metacount; ++i)
            if (wpc->metadata [i].data)
                free (wpc->metadata [i].data);

        free (wpc->metadata);
    }

    if (wpc->channel_identities)
        free (wpc->channel_identities);

    if (wpc->channel_reordering)
        free (wpc->channel_reordering);

#ifndef NO_TAGS
    free_tag (&wpc->m_tag);
#endif

#ifdef ENABLE_DSD
    if (wpc->decimation_context)
        decimate_dsd_destroy (wpc->decimation_context);
#endif

#ifdef ENABLE_THREADS
    worker_threads_destroy (wpc);
#endif

    free (wpc);

    return NULL;
}

// These routines are used to access (and free) header and trailer data that
// was retrieved from the Wavpack file. The header will be available before
// the samples are decoded and the trailer will be available after all samples
// have been read.

uint32_t WavpackGetWrapperBytes (WavpackContext *wpc)
{
    return wpc ? wpc->wrapper_bytes : 0;
}

unsigned char *WavpackGetWrapperData (WavpackContext *wpc)
{
    return wpc ? wpc->wrapper_data : NULL;
}

void WavpackFreeWrapper (WavpackContext *wpc)
{
    if (wpc && wpc->wrapper_data) {
        free (wpc->wrapper_data);
        wpc->wrapper_data = NULL;
        wpc->wrapper_bytes = 0;
    }
}

// Returns the sample rate of the specified WavPack file

uint32_t WavpackGetSampleRate (WavpackContext *wpc)
{
    return wpc ? (wpc->dsd_multiplier ? wpc->config.sample_rate * wpc->dsd_multiplier : wpc->config.sample_rate) : 44100;
}

// Returns the native sample rate of the specified WavPack file
// (provides the native rate for DSD files rather than the "byte" rate that's used for
//   seeking, duration, etc. and would generally be used just for user facing reports)

uint32_t WavpackGetNativeSampleRate (WavpackContext *wpc)
{
    return wpc ? (wpc->dsd_multiplier ? wpc->config.sample_rate * wpc->dsd_multiplier * 8 : wpc->config.sample_rate) : 44100;
}

// Returns the number of channels of the specified WavPack file. Note that
// this is the actual number of channels contained in the file even if the
// OPEN_2CH_MAX flag was specified when the file was opened.

int WavpackGetNumChannels (WavpackContext *wpc)
{
    return wpc ? wpc->config.num_channels : 2;
}

// Returns the standard Microsoft channel mask for the specified WavPack
// file. A value of zero indicates that there is no speaker assignment
// information.

int WavpackGetChannelMask (WavpackContext *wpc)
{
    return wpc ? wpc->config.channel_mask : 0;
}

// Return the normalization value for floating point data (valid only
// if floating point data is present). A value of 127 indicates that
// the floating point range is +/- 1.0. Higher values indicate a
// larger floating point range. Note that if the client specified
// OPEN_NORMALIZE we return the normalized value (i.e., 127 + offset)
// rather than what's in the file (which isn't really relevant).

int WavpackGetFloatNormExp (WavpackContext *wpc)
{
    return (wpc->open_flags & OPEN_NORMALIZE) ? 127 + wpc->norm_offset : wpc->config.float_norm_exp;
}

// Returns the actual number of valid bits per sample contained in the
// original file, which may or may not be a multiple of 8. Floating data
// always has 32 bits, integers may be from 1 to 32 bits each. When this
// value is not a multiple of 8, then the "extra" bits are located in the
// LSBs of the results. That is, values are right justified when unpacked
// into ints, but are left justified in the number of bytes used by the
// original data.

int WavpackGetBitsPerSample (WavpackContext *wpc)
{
    return wpc ? wpc->config.bits_per_sample : 16;
}

// Returns the number of bytes used for each sample (1 to 4) in the original
// file. This is required information for the user of this module because the
// audio data is returned in the LOWER bytes of the long buffer and must be
// left-shifted 8, 16, or 24 bits if normalized longs are required.

int WavpackGetBytesPerSample (WavpackContext *wpc)
{
    return wpc ? wpc->config.bytes_per_sample : 2;
}

// If the OPEN_2CH_MAX flag is specified when opening the file, this function
// will return the actual number of channels decoded from the file (which may
// or may not be less than the actual number of channels, but will always be
// 1 or 2). Normally, this will be the front left and right channels of a
// multichannel file.

int WavpackGetReducedChannels (WavpackContext *wpc)
{
    if (wpc)
        return wpc->reduced_channels ? wpc->reduced_channels : wpc->config.num_channels;
    else
        return 2;
}

// Free all memory allocated for raw WavPack blocks (for all allocated streams)
// and free all additional streams. This does not free the default stream ([0])
// which is always kept around.

void free_streams (WavpackContext *wpc)
{
    int si = wpc->num_streams;

    while (si--) {
        free_single_stream (wpc->streams [si]);

        if (si) {
            wpc->num_streams--;
            free (wpc->streams [si]);
            wpc->streams [si] = NULL;
        }
    }
}

// Free all resources associated with the specified stream

void free_single_stream (WavpackStream *wps)
{
    if (wps->blockbuff) {
        free (wps->blockbuff);
        wps->blockbuff = NULL;
    }

    if (wps->block2buff) {
        free (wps->block2buff);
        wps->block2buff = NULL;
    }

    if (wps->sample_buffer) {
        free (wps->sample_buffer);
        wps->sample_buffer = NULL;
    }

    if (wps->pre_sample_buffer) {
        free (wps->pre_sample_buffer);
        wps->pre_sample_buffer = NULL;
    }

    if (wps->dc.shaping_data) {
        free (wps->dc.shaping_data);
        wps->dc.shaping_data = NULL;
    }

#ifdef ENABLE_DSD
    free_dsd_tables (wps);
#endif
}

// Free all DSD-related resources associated with the specified stream

void free_dsd_tables (WavpackStream *wps)
{
    if (wps->dsd.probabilities) {
        free (wps->dsd.probabilities);
        wps->dsd.probabilities = NULL;
    }

    if (wps->dsd.summed_probabilities) {
        free (wps->dsd.summed_probabilities);
        wps->dsd.summed_probabilities = NULL;
    }

    if (wps->dsd.lookup_buffer) {
        free (wps->dsd.lookup_buffer);
        wps->dsd.lookup_buffer = NULL;
    }

    if (wps->dsd.value_lookup) {
        free (wps->dsd.value_lookup);
        wps->dsd.value_lookup = NULL;
    }

    if (wps->dsd.ptable) {
        free (wps->dsd.ptable);
        wps->dsd.ptable = NULL;
    }
}

void WavpackFloatNormalize (int32_t *values, int32_t num_values, int delta_exp)
{
    f32 *fvalues = (f32 *) values;
    int exp;

    if (!delta_exp)
        return;

    while (num_values--) {
        if ((exp = get_exponent (*fvalues)) == 0 || exp + delta_exp <= 0)
            *fvalues = 0;
        else if (exp == 255 || (exp += delta_exp) >= 255) {
            set_exponent (*fvalues, 255);
            set_mantissa (*fvalues, 0);
        }
        else
            set_exponent (*fvalues, exp);

        fvalues++;
    }
}

void WavpackLittleEndianToNative (void *data, char *format)
{
    unsigned char *cp = (unsigned char *) data;
    int64_t temp64;
    int32_t temp32;
    int16_t temp16;

    while (*format) {
        switch (*format) {
            case 'D':
                temp64 = cp [0] + ((int64_t) cp [1] << 8) + ((int64_t) cp [2] << 16) + ((int64_t) cp [3] << 24) +
                    ((int64_t) cp [4] << 32) + ((int64_t) cp [5] << 40) + ((int64_t) cp [6] << 48) + ((uint64_t) cp [7] << 56);
                memcpy (cp, &temp64, 8);
                cp += 8;
                break;

            case 'L':
                temp32 = cp [0] + ((int32_t) cp [1] << 8) + ((int32_t) cp [2] << 16) + ((int64_t) cp [3] << 24);
                memcpy (cp, &temp32, 4);
                cp += 4;
                break;

            case 'S':
                temp16 = cp [0] + (cp [1] << 8);
                memcpy (cp, &temp16, 2);
                cp += 2;
                break;

            default:
                if (isdigit (*format))
                    cp += *format - '0';

                break;
        }

        format++;
    }
}

void WavpackNativeToLittleEndian (void *data, char *format)
{
    unsigned char *cp = (unsigned char *) data;
    int64_t temp64;
    int32_t temp32;
    int16_t temp16;

    while (*format) {
        switch (*format) {
            case 'D':
                memcpy (&temp64, cp, sizeof (temp64));
                *cp++ = (unsigned char) temp64;
                *cp++ = (unsigned char) (temp64 >> 8);
                *cp++ = (unsigned char) (temp64 >> 16);
                *cp++ = (unsigned char) (temp64 >> 24);
                *cp++ = (unsigned char) (temp64 >> 32);
                *cp++ = (unsigned char) (temp64 >> 40);
                *cp++ = (unsigned char) (temp64 >> 48);
                *cp++ = (unsigned char) (temp64 >> 56);
                break;

            case 'L':
                memcpy (&temp32, cp, sizeof (temp32));
                *cp++ = (unsigned char) temp32;
                *cp++ = (unsigned char) (temp32 >> 8);
                *cp++ = (unsigned char) (temp32 >> 16);
                *cp++ = (unsigned char) (temp32 >> 24);
                break;

            case 'S':
                memcpy (&temp16, cp, sizeof (temp16));
                *cp++ = (unsigned char) temp16;
                *cp++ = (unsigned char) (temp16 >> 8);
                break;

            default:
                if (isdigit (*format))
                    cp += *format - '0';

                break;
        }

        format++;
    }
}

void WavpackBigEndianToNative (void *data, char *format)
{
    unsigned char *cp = (unsigned char *) data;
    int64_t temp64;
    int32_t temp32;
    int16_t temp16;

    while (*format) {
        switch (*format) {
            case 'D':
                temp64 = cp [7] + ((int64_t) cp [6] << 8) + ((int64_t) cp [5] << 16) + ((int64_t) cp [4] << 24) +
                    ((int64_t) cp [3] << 32) + ((int64_t) cp [2] << 40) + ((int64_t) cp [1] << 48) + ((uint64_t) cp [0] << 56);
                memcpy (cp, &temp64, 8);
                cp += 8;
                break;

            case 'L':
                temp32 = cp [3] + ((int32_t) cp [2] << 8) + ((int32_t) cp [1] << 16) + ((int64_t) cp [0] << 24);
                memcpy (cp, &temp32, 4);
                cp += 4;
                break;

            case 'S':
                temp16 = cp [1] + (cp [0] << 8);
                memcpy (cp, &temp16, 2);
                cp += 2;
                break;

            default:
                if (isdigit (*format))
                    cp += *format - '0';

                break;
        }

        format++;
    }
}

void WavpackNativeToBigEndian (void *data, char *format)
{
    unsigned char *cp = (unsigned char *) data;
    int64_t temp64;
    int32_t temp32;
    int16_t temp16;

    while (*format) {
        switch (*format) {
            case 'D':
                memcpy (&temp64, cp, sizeof (temp64));
                *cp++ = (unsigned char) (temp64 >> 56);
                *cp++ = (unsigned char) (temp64 >> 48);
                *cp++ = (unsigned char) (temp64 >> 40);
                *cp++ = (unsigned char) (temp64 >> 32);
                *cp++ = (unsigned char) (temp64 >> 24);
                *cp++ = (unsigned char) (temp64 >> 16);
                *cp++ = (unsigned char) (temp64 >> 8);
                *cp++ = (unsigned char) temp64;
                break;

            case 'L':
                memcpy (&temp32, cp, sizeof (temp32));
                *cp++ = (unsigned char) (temp32 >> 24);
                *cp++ = (unsigned char) (temp32 >> 16);
                *cp++ = (unsigned char) (temp32 >> 8);
                *cp++ = (unsigned char) temp32;
                break;

            case 'S':
                memcpy (&temp16, cp, sizeof (temp16));
                *cp++ = (unsigned char) (temp16 >> 8);
                *cp++ = (unsigned char) temp16;
                break;

            default:
                if (isdigit (*format))
                    cp += *format - '0';

                break;
        }

        format++;
    }
}

uint32_t WavpackGetLibraryVersion (void)
{
    return (LIBWAVPACK_MAJOR<<16)
          |(LIBWAVPACK_MINOR<<8)
          |(LIBWAVPACK_MICRO<<0);
}

const char *WavpackGetLibraryVersionString (void)
{
    return LIBWAVPACK_VERSION_STRING;
}


/* ===================== src/unpack.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// unpack.c

// This module actually handles the decompression of the audio data, except for
// the entropy decoding which is handled by the read_words.c module. For better
// efficiency, the conversion is isolated to tight loops that handle an entire
// buffer.

#include <stdlib.h>
#include <string.h>


#ifdef OPT_ASM_X86
    #define DECORR_STEREO_PASS_CONT unpack_decorr_stereo_pass_cont_x86
    #define DECORR_STEREO_PASS_CONT_AVAILABLE unpack_cpu_has_feature_x86(CPU_FEATURE_MMX)
    #define DECORR_MONO_PASS_CONT unpack_decorr_mono_pass_cont_x86
#elif defined(OPT_ASM_X64) && (defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW64__) || defined(__midipix__))
    #define DECORR_STEREO_PASS_CONT unpack_decorr_stereo_pass_cont_x64win
    #define DECORR_STEREO_PASS_CONT_AVAILABLE 1
    #define DECORR_MONO_PASS_CONT unpack_decorr_mono_pass_cont_x64win
#elif defined(OPT_ASM_X64)
    #define DECORR_STEREO_PASS_CONT unpack_decorr_stereo_pass_cont_x64
    #define DECORR_STEREO_PASS_CONT_AVAILABLE 1
    #define DECORR_MONO_PASS_CONT unpack_decorr_mono_pass_cont_x64
#elif defined(OPT_ASM_ARM)
    #define DECORR_STEREO_PASS_CONT unpack_decorr_stereo_pass_cont_armv7
    #define DECORR_STEREO_PASS_CONT_AVAILABLE 1
    #define DECORR_MONO_PASS_CONT unpack_decorr_mono_pass_cont_armv7
#endif

#ifdef DECORR_STEREO_PASS_CONT
extern void ASMCALL DECORR_STEREO_PASS_CONT (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count, int32_t long_math);
extern void ASMCALL DECORR_MONO_PASS_CONT (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count, int32_t long_math);
#endif

// This flag provides the functionality of terminating the decoding and muting
// the output when a lossy sample appears to be corrupt. This is automatic
// for lossless files because a corrupt sample is unambiguous, but for lossy
// data it might be possible for this to falsely trigger (although I have never
// seen it).

#define LOSSY_MUTE

///////////////////////////// executable code ////////////////////////////////

// This monster actually unpacks the WavPack bitstream(s) into the specified
// buffer as 32-bit integers or floats (depending on original data). Lossy
// samples will be clipped to their original limits (i.e. 8-bit samples are
// clipped to -128/+127) but are still returned in longs. It is up to the
// caller to potentially reformat this for the final output including any
// multichannel distribution, block alignment or endian compensation. The
// function unpack_init() must have been called and the entire WavPack block
// must still be visible (although wps->blockbuff will not be accessed again).
// For maximum clarity, the function is broken up into segments that handle
// various modes. This makes for a few extra infrequent flag checks, but
// makes the code easier to follow because the nesting does not become so
// deep. For maximum efficiency, the conversion is isolated to tight loops
// that handle an entire buffer. The function returns the total number of
// samples unpacked, which can be less than the number requested if an error
// occurs or the end of the block is reached.

static void decorr_stereo_pass (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);
static void decorr_mono_pass (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count);
static void fixup_samples (WavpackStream *wps, int32_t *buffer, uint32_t sample_count);

int32_t unpack_samples (WavpackStream *wps, int32_t *buffer, uint32_t sample_count)
{
    uint32_t flags = wps->wphdr.flags, crc = wps->crc, i;
    int32_t mute_limit = (1L << ((flags & MAG_MASK) >> MAG_LSB)) + 2;
    int32_t correction [2], read_word, *bptr;
    struct decorr_pass *dpp;
    int tcount, m = 0;

    // don't attempt to decode past the end of the block, but watch out for overflow!

    if (wps->sample_index + sample_count > GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples &&
        (uint32_t) (GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples - wps->sample_index) < sample_count)
            sample_count = (uint32_t) (GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples - wps->sample_index);

    if (GET_BLOCK_INDEX (wps->wphdr) > wps->sample_index || wps->wphdr.block_samples < sample_count)
        wps->mute_error = TRUE;

    if (wps->mute_error) {
        if (wps->wpc->reduced_channels == 1 || wps->wpc->config.num_channels == 1 || (flags & MONO_FLAG))
            memset (buffer, 0, sample_count * 4);
        else
            memset (buffer, 0, sample_count * 8);

        wps->sample_index += sample_count;
        return sample_count;
    }

    if ((flags & HYBRID_FLAG) && !wps->block2buff)
        mute_limit = (mute_limit * 2) + 128;

    //////////////// handle lossless or hybrid lossy mono data /////////////////

    if (!wps->block2buff && (flags & MONO_DATA)) {
        int32_t *eptr = buffer + sample_count;

        if (flags & HYBRID_FLAG) {
            i = sample_count;

            for (bptr = buffer; bptr < eptr;)
                if ((*bptr++ = get_word (wps, 0, NULL)) == WORD_EOF) {
                    i = (uint32_t)(bptr - buffer);
                    break;
                }
        }
        else
            i = get_words_lossless (wps, buffer, sample_count);

        if (i != sample_count)
            goto get_word_eof;

#ifdef DECORR_MONO_PASS_CONT
        if (sample_count < 16)
            for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
                decorr_mono_pass (dpp, buffer, sample_count);
        else
            for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
                int pre_samples = (dpp->term > MAX_TERM) ? 2 : dpp->term;

                decorr_mono_pass (dpp, buffer, pre_samples);

                DECORR_MONO_PASS_CONT (dpp, buffer + pre_samples, sample_count - pre_samples,
                    ((flags & MAG_MASK) >> MAG_LSB) > 15);
            }
#else
        for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
            decorr_mono_pass (dpp, buffer, sample_count);
#endif

#ifndef LOSSY_MUTE
        if (!(flags & HYBRID_FLAG))
#endif
        for (bptr = buffer; bptr < eptr; ++bptr) {
            if (labs (bptr [0]) > mute_limit) {
                i = (uint32_t)(bptr - buffer);
                break;
            }

            crc = crc * 3 + bptr [0];
        }
#ifndef LOSSY_MUTE
        else
            for (bptr = buffer; bptr < eptr; ++bptr)
                crc = crc * 3 + bptr [0];
#endif
    }

    /////////////// handle lossless or hybrid lossy stereo data ///////////////

    else if (!wps->block2buff && !(flags & MONO_DATA)) {
        int32_t *eptr = buffer + (sample_count * 2);

        if (flags & HYBRID_FLAG) {
            i = sample_count;

            for (bptr = buffer; bptr < eptr; bptr += 2)
                if ((bptr [0] = get_word (wps, 0, NULL)) == WORD_EOF ||
                    (bptr [1] = get_word (wps, 1, NULL)) == WORD_EOF) {
                        i = (uint32_t)(bptr - buffer) / 2;
                        break;
                }
        }
        else
            i = get_words_lossless (wps, buffer, sample_count);

        if (i != sample_count)
            goto get_word_eof;

#ifdef DECORR_STEREO_PASS_CONT
        if (sample_count < 16 || !DECORR_STEREO_PASS_CONT_AVAILABLE) {
            for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
                decorr_stereo_pass (dpp, buffer, sample_count);

            m = sample_count & (MAX_TERM - 1);
        }
        else
            for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
                int pre_samples = (dpp->term < 0 || dpp->term > MAX_TERM) ? 2 : dpp->term;

                decorr_stereo_pass (dpp, buffer, pre_samples);

                DECORR_STEREO_PASS_CONT (dpp, buffer + pre_samples * 2, sample_count - pre_samples,
                    ((flags & MAG_MASK) >> MAG_LSB) >= 16);
            }
#else
        for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
            decorr_stereo_pass (dpp, buffer, sample_count);

        m = sample_count & (MAX_TERM - 1);
#endif

        if (flags & JOINT_STEREO)
            for (bptr = buffer; bptr < eptr; bptr += 2) {
                bptr [0] += (bptr [1] -= (bptr [0] >> 1));
                crc += (crc << 3) + ((uint32_t) bptr [0] << 1) + bptr [0] + bptr [1];
            }
        else
            for (bptr = buffer; bptr < eptr; bptr += 2)
                crc += (crc << 3) + ((uint32_t) bptr [0] << 1) + bptr [0] + bptr [1];

#ifndef LOSSY_MUTE
        if (!(flags & HYBRID_FLAG))
#endif
        for (bptr = buffer; bptr < eptr; bptr += 16)
            if (labs (bptr [0]) > mute_limit || labs (bptr [1]) > mute_limit) {
                i = (uint32_t)(bptr - buffer) / 2;
                break;
            }
    }

    /////////////////// handle hybrid lossless mono data ////////////////////

    else if ((flags & HYBRID_FLAG) && (flags & MONO_DATA))
        for (bptr = buffer, i = 0; i < sample_count; ++i) {

            if ((read_word = get_word (wps, 0, correction)) == WORD_EOF)
                break;

            for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
                int32_t sam, temp;
                int k;

                if (dpp->term > MAX_TERM) {
                    if (dpp->term & 1)
                        sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
                    else
                        sam = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;

                    dpp->samples_A [1] = dpp->samples_A [0];
                    k = 0;
                }
                else {
                    sam = dpp->samples_A [m];
                    k = (m + dpp->term) & (MAX_TERM - 1);
                }

                temp = apply_weight (dpp->weight_A, sam) + read_word;
                update_weight (dpp->weight_A, dpp->delta, sam, read_word);
                dpp->samples_A [k] = read_word = temp;
            }

            m = (m + 1) & (MAX_TERM - 1);

            if (flags & HYBRID_SHAPE) {
                int shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
                int32_t temp = -apply_weight (shaping_weight, wps->dc.error [0]);

                if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
                    if (temp == wps->dc.error [0])
                        temp = (temp < 0) ? temp + 1 : temp - 1;

                    wps->dc.error [0] = temp - correction [0];
                }
                else
                    wps->dc.error [0] = -correction [0];

                read_word += correction [0] - temp;
            }
            else
                read_word += correction [0];

            crc += (crc << 1) + read_word;

            if (labs (read_word) > mute_limit)
                break;

            *bptr++ = read_word;
        }

    //////////////////// handle hybrid lossless stereo data ///////////////////

    else if (wps->block2buff && !(flags & MONO_DATA))
        for (bptr = buffer, i = 0; i < sample_count; ++i) {
            int32_t left, right, left2, right2;
            int32_t left_c = 0, right_c = 0;

            if ((left = get_word (wps, 0, correction)) == WORD_EOF ||
                (right = get_word (wps, 1, correction + 1)) == WORD_EOF)
                    break;

            if (flags & CROSS_DECORR) {
                left_c = left + correction [0];
                right_c = right + correction [1];

                for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
                    int32_t sam_A, sam_B;

                    if (dpp->term > 0) {
                        if (dpp->term > MAX_TERM) {
                            if (dpp->term & 1) {
                                sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
                                sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
                            }
                            else {
                                sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
                                sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
                            }
                        }
                        else {
                            sam_A = dpp->samples_A [m];
                            sam_B = dpp->samples_B [m];
                        }

                        left_c += apply_weight (dpp->weight_A, sam_A);
                        right_c += apply_weight (dpp->weight_B, sam_B);
                    }
                    else if (dpp->term == -1) {
                        left_c += apply_weight (dpp->weight_A, dpp->samples_A [0]);
                        right_c += apply_weight (dpp->weight_B, left_c);
                    }
                    else {
                        right_c += apply_weight (dpp->weight_B, dpp->samples_B [0]);

                        if (dpp->term == -3)
                            left_c += apply_weight (dpp->weight_A, dpp->samples_A [0]);
                        else
                            left_c += apply_weight (dpp->weight_A, right_c);
                    }
                }

                if (flags & JOINT_STEREO)
                    left_c += (right_c -= (left_c >> 1));
            }

            for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
                int32_t sam_A, sam_B;

                if (dpp->term > 0) {
                    int k;

                    if (dpp->term > MAX_TERM) {
                        if (dpp->term & 1) {
                            sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
                            sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
                        }
                        else {
                            sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
                            sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
                        }

                        dpp->samples_A [1] = dpp->samples_A [0];
                        dpp->samples_B [1] = dpp->samples_B [0];
                        k = 0;
                    }
                    else {
                        sam_A = dpp->samples_A [m];
                        sam_B = dpp->samples_B [m];
                        k = (m + dpp->term) & (MAX_TERM - 1);
                    }

                    left2 = apply_weight (dpp->weight_A, sam_A) + left;
                    right2 = apply_weight (dpp->weight_B, sam_B) + right;

                    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
                    update_weight (dpp->weight_B, dpp->delta, sam_B, right);

                    dpp->samples_A [k] = left = left2;
                    dpp->samples_B [k] = right = right2;
                }
                else if (dpp->term == -1) {
                    left2 = left + apply_weight (dpp->weight_A, dpp->samples_A [0]);
                    update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], left);
                    left = left2;
                    right2 = right + apply_weight (dpp->weight_B, left2);
                    update_weight_clip (dpp->weight_B, dpp->delta, left2, right);
                    dpp->samples_A [0] = right = right2;
                }
                else {
                    right2 = right + apply_weight (dpp->weight_B, dpp->samples_B [0]);
                    update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], right);
                    right = right2;

                    if (dpp->term == -3) {
                        right2 = dpp->samples_A [0];
                        dpp->samples_A [0] = right;
                    }

                    left2 = left + apply_weight (dpp->weight_A, right2);
                    update_weight_clip (dpp->weight_A, dpp->delta, right2, left);
                    dpp->samples_B [0] = left = left2;
                }
            }

            m = (m + 1) & (MAX_TERM - 1);

            if (!(flags & CROSS_DECORR)) {
                left_c = left + correction [0];
                right_c = right + correction [1];

                if (flags & JOINT_STEREO)
                    left_c += (right_c -= (left_c >> 1));
            }

            if (flags & JOINT_STEREO)
                left += (right -= (left >> 1));

            if (flags & HYBRID_SHAPE) {
                int shaping_weight;
                int32_t temp;

                correction [0] = left_c - left;
                shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
                temp = -apply_weight (shaping_weight, wps->dc.error [0]);

                if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
                    if (temp == wps->dc.error [0])
                        temp = (temp < 0) ? temp + 1 : temp - 1;

                    wps->dc.error [0] = temp - correction [0];
                }
                else
                    wps->dc.error [0] = -correction [0];

                left = left_c - temp;
                correction [1] = right_c - right;
                shaping_weight = (wps->dc.shaping_acc [1] += wps->dc.shaping_delta [1]) >> 16;
                temp = -apply_weight (shaping_weight, wps->dc.error [1]);

                if ((flags & NEW_SHAPING) && shaping_weight < 0 && temp) {
                    if (temp == wps->dc.error [1])
                        temp = (temp < 0) ? temp + 1 : temp - 1;

                    wps->dc.error [1] = temp - correction [1];
                }
                else
                    wps->dc.error [1] = -correction [1];

                right = right_c - temp;
            }
            else {
                left = left_c;
                right = right_c;
            }

            if (labs (left) > mute_limit || labs (right) > mute_limit)
                break;

            crc += (crc << 3) + ((uint32_t) left << 1) + left + right;
            *bptr++ = left;
            *bptr++ = right;
        }
    else
        i = 0;  /* this line can't execute, but suppresses compiler warning */

get_word_eof:
    if (i != sample_count) {
        memset (buffer, 0, sample_count * (flags & MONO_FLAG ? 4 : 8));
        wps->mute_error = TRUE;
        i = sample_count;

        if (bs_is_open (&wps->wvxbits))
            bs_close_read (&wps->wvxbits);
    }

    if (m)
        for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
            if (dpp->term > 0 && dpp->term <= MAX_TERM) {
                int32_t temp_A [MAX_TERM], temp_B [MAX_TERM];
                int k;

                memcpy (temp_A, dpp->samples_A, sizeof (dpp->samples_A));
                memcpy (temp_B, dpp->samples_B, sizeof (dpp->samples_B));

                for (k = 0; k < MAX_TERM; k++) {
                    dpp->samples_A [k] = temp_A [m];
                    dpp->samples_B [k] = temp_B [m];
                    m = (m + 1) & (MAX_TERM - 1);
                }
            }

    fixup_samples (wps, buffer, i);

    if ((flags & FLOAT_DATA) && (wps->wpc->open_flags & OPEN_NORMALIZE))
        WavpackFloatNormalize (buffer, (flags & MONO_DATA) ? i : i * 2,
            127 - wps->float_norm_exp + wps->wpc->norm_offset);

    if (flags & FALSE_STEREO) {
        int32_t *dptr = buffer + i * 2;
        int32_t *sptr = buffer + i;
        int32_t c = i;

        while (c--) {
            *--dptr = *--sptr;
            *--dptr = *sptr;
        }
    }

    wps->sample_index += i;
    wps->crc = crc;

    // If we just finished this block, then it's time to check if the applicable checksums match. Like
    // other decoding errors, these are indicated by setting the mute_error flag. We also clear the
    // buffer to reduce the chances of noise bursts getting through.

    if (!wps->mute_error && wps->sample_index == GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples) {
        if (wps->crc != wps->wphdr.crc || (bs_is_open (&wps->wvxbits) && wps->crc_x != wps->crc_wvx)) {
            memset (buffer, 0, sample_count * (flags & MONO_FLAG ? 4 : 8));
            wps->mute_error = TRUE;
        }
    }

    return i;
}

// General function to perform mono decorrelation pass on specified buffer
// (although since this is the reverse function it might technically be called
// "correlation" instead). This version handles all sample resolutions and
// weight deltas. The dpp->samples_X[] data is returned normalized for term
// values 1-8.

static void decorr_mono_pass (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t delta = dpp->delta, weight_A = dpp->weight_A;
    int32_t *bptr, *eptr = buffer + sample_count, sam_A;
    int m, k;

    switch (dpp->term) {

        case 17:
            for (bptr = buffer; bptr < eptr; bptr++) {
                sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
                dpp->samples_A [1] = dpp->samples_A [0];
                dpp->samples_A [0] = apply_weight (weight_A, sam_A) + bptr [0];
                update_weight (weight_A, delta, sam_A, bptr [0]);
                bptr [0] = dpp->samples_A [0];
            }

            break;

        case 18:
            for (bptr = buffer; bptr < eptr; bptr++) {
                sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
                dpp->samples_A [1] = dpp->samples_A [0];
                dpp->samples_A [0] = apply_weight (weight_A, sam_A) + bptr [0];
                update_weight (weight_A, delta, sam_A, bptr [0]);
                bptr [0] = dpp->samples_A [0];
            }

            break;

        default:
            for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr++) {
                sam_A = dpp->samples_A [m];
                dpp->samples_A [k] = apply_weight (weight_A, sam_A) + bptr [0];
                update_weight (weight_A, delta, sam_A, bptr [0]);
                bptr [0] = dpp->samples_A [k];
                m = (m + 1) & (MAX_TERM - 1);
                k = (k + 1) & (MAX_TERM - 1);
            }

            if (m) {
                int32_t temp_samples [MAX_TERM];

                memcpy (temp_samples, dpp->samples_A, sizeof (dpp->samples_A));

                for (k = 0; k < MAX_TERM; k++, m++)
                    dpp->samples_A [k] = temp_samples [m & (MAX_TERM - 1)];
            }

            break;
    }

    dpp->weight_A = weight_A;
}

// General function to perform stereo decorrelation pass on specified buffer
// (although since this is the reverse function it might technically be called
// "correlation" instead). This version handles all sample resolutions and
// weight deltas. The dpp->samples_X[] data is *not* returned normalized for
// term values 1-8, so it should be normalized if it is going to be used to
// call this function again.

static void decorr_stereo_pass (struct decorr_pass *dpp, int32_t *buffer, int32_t sample_count)
{
    int32_t *bptr, *eptr = buffer + (sample_count * 2);
    int m, k;

    switch (dpp->term) {
        case 17:
            for (bptr = buffer; bptr < eptr; bptr += 2) {
                int32_t sam, tmp;

                sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
                dpp->samples_A [1] = dpp->samples_A [0];
                bptr [0] = dpp->samples_A [0] = apply_weight (dpp->weight_A, sam) + (tmp = bptr [0]);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);

                sam = 2 * dpp->samples_B [0] - dpp->samples_B [1];
                dpp->samples_B [1] = dpp->samples_B [0];
                bptr [1] = dpp->samples_B [0] = apply_weight (dpp->weight_B, sam) + (tmp = bptr [1]);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);
            }

            break;

        case 18:
            for (bptr = buffer; bptr < eptr; bptr += 2) {
                int32_t sam, tmp;

                sam = dpp->samples_A [0] + ((dpp->samples_A [0] - dpp->samples_A [1]) >> 1);
                dpp->samples_A [1] = dpp->samples_A [0];
                bptr [0] = dpp->samples_A [0] = apply_weight (dpp->weight_A, sam) + (tmp = bptr [0]);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);

                sam = dpp->samples_B [0] + ((dpp->samples_B [0] - dpp->samples_B [1]) >> 1);
                dpp->samples_B [1] = dpp->samples_B [0];
                bptr [1] = dpp->samples_B [0] = apply_weight (dpp->weight_B, sam) + (tmp = bptr [1]);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);
            }

            break;

        default:
            for (m = 0, k = dpp->term & (MAX_TERM - 1), bptr = buffer; bptr < eptr; bptr += 2) {
                int32_t sam;

                sam = dpp->samples_A [m];
                dpp->samples_A [k] = apply_weight (dpp->weight_A, sam) + bptr [0];
                update_weight (dpp->weight_A, dpp->delta, sam, bptr [0]);
                bptr [0] = dpp->samples_A [k];

                sam = dpp->samples_B [m];
                dpp->samples_B [k] = apply_weight (dpp->weight_B, sam) + bptr [1];
                update_weight (dpp->weight_B, dpp->delta, sam, bptr [1]);
                bptr [1] = dpp->samples_B [k];

                m = (m + 1) & (MAX_TERM - 1);
                k = (k + 1) & (MAX_TERM - 1);
            }

            break;

        case -1:
            for (bptr = buffer; bptr < eptr; bptr += 2) {
                int32_t sam;

                sam = bptr [0] + apply_weight (dpp->weight_A, dpp->samples_A [0]);
                update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
                bptr [0] = sam;
                dpp->samples_A [0] = bptr [1] + apply_weight (dpp->weight_B, sam);
                update_weight_clip (dpp->weight_B, dpp->delta, sam, bptr [1]);
                bptr [1] = dpp->samples_A [0];
            }

            break;

        case -2:
            for (bptr = buffer; bptr < eptr; bptr += 2) {
                int32_t sam;

                sam = bptr [1] + apply_weight (dpp->weight_B, dpp->samples_B [0]);
                update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
                bptr [1] = sam;
                dpp->samples_B [0] = bptr [0] + apply_weight (dpp->weight_A, sam);
                update_weight_clip (dpp->weight_A, dpp->delta, sam, bptr [0]);
                bptr [0] = dpp->samples_B [0];
            }

            break;

        case -3:
            for (bptr = buffer; bptr < eptr; bptr += 2) {
                int32_t sam_A, sam_B;

                sam_A = bptr [0] + apply_weight (dpp->weight_A, dpp->samples_A [0]);
                update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], bptr [0]);
                sam_B = bptr [1] + apply_weight (dpp->weight_B, dpp->samples_B [0]);
                update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], bptr [1]);
                bptr [0] = dpp->samples_B [0] = sam_A;
                bptr [1] = dpp->samples_A [0] = sam_B;
            }

            break;
    }
}

// This is a helper function for unpack_samples() that applies several final
// operations. First, if the data is 32-bit float data, then that conversion
// is done in the float.c module (whether lossy or lossless) and we return.
// Otherwise, if the extended integer data applies, then that operation is
// executed first. If the unpacked data is lossy (and not corrected) then
// it is clipped and shifted in a single operation. Otherwise, if it's
// lossless then the last step is to apply the final shift (if any).

static void fixup_samples (WavpackStream *wps, int32_t *buffer, uint32_t sample_count)
{
    uint32_t flags = wps->wphdr.flags;
    int lossy_flag = (flags & HYBRID_FLAG) && !wps->block2buff;
    int shift = (flags & SHIFT_MASK) >> SHIFT_LSB;

    if (flags & FLOAT_DATA) {
        float_values (wps, buffer, (flags & MONO_DATA) ? sample_count : sample_count * 2);
        return;
    }

    if (flags & INT32_DATA) {
        uint32_t count = (flags & MONO_DATA) ? sample_count : sample_count * 2;
        int sent_bits = wps->int32_sent_bits & 0x1f, zeros = wps->int32_zeros & 0x1f;
        int ones = wps->int32_ones & 0x1f, dups = wps->int32_dups & 0x1f;
        uint32_t data, mask = (1U << sent_bits) - 1;
        int32_t *dptr = buffer;

        if (bs_is_open (&wps->wvxbits)) {
            int max_width = wps->int32_max_width;
            uint32_t crc = wps->crc_x;

            while (count--) {
                if (sent_bits) {
                    if (max_width) {
                        int32_t pvalue = *dptr < 0 ? ~*dptr : *dptr;
                        int width = count_bits (pvalue) + sent_bits;
                        int bits_to_read = sent_bits;

                        if (width <= max_width || (bits_to_read -= width - max_width) > 0) {
                            getbits (&data, bits_to_read, &wps->wvxbits);
                            data &= (1U << bits_to_read) - 1;
                            *dptr = (((uint32_t) *dptr << bits_to_read) | data) << (sent_bits - bits_to_read);
                        }
                        else
                            *dptr = (uint32_t) *dptr << sent_bits;
                    }
                    else {
                        getbits (&data, sent_bits, &wps->wvxbits);
                        *dptr = ((uint32_t) *dptr << sent_bits) | (data & mask);
                    }
                }

                if (zeros)
                    *(uint32_t*)dptr <<= zeros;
                else if (ones)
                    *dptr = ((uint32_t)(*dptr + 1) << ones) - 1;
                else if (dups)
                    *dptr = ((uint32_t)(*dptr + (*dptr & 1)) << dups) - (*dptr & 1);

                crc = crc * 9 + (*dptr & 0xffff) * 3 + ((*dptr >> 16) & 0xffff);
                dptr++;
            }

            wps->crc_x = crc;
        }
        else if (!sent_bits && (zeros + ones + dups)) {
            while (lossy_flag && (flags & BYTES_STORED) == 3 && shift < 8) {
                if (zeros)
                    zeros--;
                else if (ones)
                    ones--;
                else if (dups)
                    dups--;
                else
                    break;

                shift++;
            }

            while (count--) {
                if (zeros)
                    *(uint32_t*)dptr <<= zeros;
                else if (ones)
                    *dptr = ((uint32_t)(*dptr + 1) << ones) - 1;
                else if (dups)
                    *dptr = ((uint32_t)(*dptr + (*dptr & 1)) << dups) - (*dptr & 1);

                dptr++;
            }
        }
        else
            shift += zeros + sent_bits + ones + dups;
    }

    shift &= 0x1f;

    if (lossy_flag) {
        int32_t min_value, max_value, min_shifted, max_shifted;

        switch (flags & BYTES_STORED) {
            case 0:
                min_shifted = (uint32_t)(min_value = -128 >> shift) << shift;
                max_shifted = (max_value = 127 >> shift) << shift;
                break;

            case 1:
                min_shifted = (uint32_t)(min_value = -32768 >> shift) << shift;
                max_shifted = (max_value = 32767 >> shift) << shift;
                break;

            case 2:
                min_shifted = (uint32_t)(min_value = -8388608 >> shift) << shift;
                max_shifted = (max_value = 8388607 >> shift) << shift;
                break;

            case 3: default:    /* "default" suppresses compiler warning */
                min_shifted = (uint32_t)(min_value = (int32_t) 0x80000000 >> shift) << shift;
                max_shifted = (max_value = (int32_t) 0x7fffffff >> shift) << shift;
                break;
        }

        if (!(flags & MONO_DATA))
            sample_count *= 2;

        while (sample_count--) {
            if (*buffer < min_value)
                *buffer++ = min_shifted;
            else if (*buffer > max_value)
                *buffer++ = max_shifted;
            else
                *(uint32_t*)buffer++ <<= shift;
        }
    }
    else if (shift) {
        if (!(flags & MONO_DATA))
            sample_count *= 2;

        while (sample_count--)
            *(uint32_t*)buffer++ <<= shift;
    }
}

/* ===================== src/unpack_utils.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// unpack_utils.c

// This module provides the high-level API for unpacking audio data from
// WavPack files. It manages the buffers used to interleave the data passed
// back to the application from the individual streams. The actual audio
// stream decompression is handled in the unpack.c module.

#include <stdlib.h>
#include <string.h>


#ifdef ENABLE_THREADS
static void unpack_samples_enqueue (WavpackStream *wps, int32_t *outbuf, int offset, uint32_t samcnt, int free_wps);
static void worker_threads_finish (WavpackContext *wpc);
static void worker_threads_create (WavpackContext *wpc);
static int worker_available (WavpackContext *wpc);
#endif

///////////////////////////// executable code ////////////////////////////////

// This function unpacks the specified number of samples from the given stream (which must be
// completely loaded and initialized). The samples are written (interleaved) into the given
// buffer at the specified offset. This function is threadsafe across streams, so it may be
// called directly from the main unpack code or from the worker threads.

static void unpack_samples_interleave (WavpackStream *wps, int32_t *outbuf, int offset, int32_t *tmpbuf, uint32_t samcnt)
{
    int num_channels = wps->wpc->config.num_channels;
    int32_t *src = tmpbuf, *dst = outbuf + offset;

    // if the number of channels in the stream exactly matches the channels in the file, we
    // don't actually have to interleave and can render directly without the temp buffer

    if (num_channels == (wps->wphdr.flags & MONO_FLAG ? 1 : 2)) {
#ifdef ENABLE_DSD
        if (wps->wphdr.flags & DSD_FLAG)
            unpack_dsd_samples (wps, outbuf, samcnt);
        else
#endif
            unpack_samples (wps, outbuf, samcnt);

        return;
    }

    // otherwise we do have to use the temp buffer

#ifdef ENABLE_DSD
    if (wps->wphdr.flags & DSD_FLAG)
        unpack_dsd_samples (wps, tmpbuf, samcnt);
    else
#endif
        unpack_samples (wps, tmpbuf, samcnt);

    // if the block is mono, copy the samples from the single channel into the destination
    // using num_channels as the stride

    if (wps->wphdr.flags & MONO_FLAG) {
        while (samcnt--) {
            dst [0] = *src++;
            dst += num_channels;
        }
    }

    // if the block is stereo, and we don't have room for two more channels, just copy one

    else if (offset == num_channels - 1) {
        while (samcnt--) {
            dst [0] = src [0];
            dst += num_channels;
            src += 2;
        }
    }

    // otherwise copy the stereo samples into the destination

    else {
        while (samcnt--) {
            dst [0] = *src++;
            dst [1] = *src++;
            dst += num_channels;
        }
    }
}

// Unpack the specified number of samples from the current file position.
// Note that "samples" here refers to "complete" samples, which would be
// 2 longs for stereo files or even more for multichannel files, so the
// required memory at "buffer" is 4 * samples * num_channels bytes. The
// audio data is returned right-justified in 32-bit longs in the endian
// mode native to the executing processor. So, if the original data was
// 16-bit, then the values returned would be +/-32k. Floating point data
// can also be returned if the source was floating point data (and this
// can be optionally normalized to +/-1.0 by using the appropriate flag
// in the call to WavpackOpenFileInput ()). The actual number of samples
// unpacked is returned, which should be equal to the number requested unless
// the end of file is encountered or an error occurs. After all samples have
// been unpacked then 0 will be returned.

uint32_t WavpackUnpackSamples (WavpackContext *wpc, int32_t *buffer, uint32_t samples)
{
    int num_channels = wpc->config.num_channels, file_done = FALSE;
    uint32_t bcount, samples_unpacked = 0, samples_to_unpack;
    int32_t *bptr = buffer;

    memset (buffer, 0, (wpc->reduced_channels ? wpc->reduced_channels : num_channels) * samples * sizeof (int32_t));

#ifdef ENABLE_THREADS
    if (wpc->num_workers && !wpc->workers)
        worker_threads_create (wpc);

    wpc->worker_errors = 0;
#endif

#ifdef ENABLE_LEGACY
    if (wpc->stream3)
        return unpack_samples3 (wpc, buffer, samples);
#endif

    while (samples) {
        WavpackStream *wps = wpc->streams [0];
        int stream_index = 0;

        // if the current block has no audio, or it's not the first block of a multichannel
        // sequence, or the sample we're on is past the last sample in this block...we need
        // to free up the streams and read the next block

        if (!wps->wphdr.block_samples || !(wps->wphdr.flags & INITIAL_BLOCK) ||
            wps->sample_index >= GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples) {
                int64_t nexthdrpos;

                if (wpc->wrapper_bytes >= MAX_WRAPPER_BYTES)
                    break;

                free_streams (wpc);
                nexthdrpos = wpc->reader->get_pos (wpc->wv_in);
                bcount = read_next_header (wpc->reader, wpc->wv_in, &wps->wphdr);

                if (bcount == (uint32_t) -1)
                    break;

                wpc->filepos = nexthdrpos + bcount;

                // allocate the memory for the entire raw block and read it in

                wps->blockbuff = (unsigned char *)malloc (wps->wphdr.ckSize + 8);

                if (!wps->blockbuff)
                    break;

                memcpy (wps->blockbuff, &wps->wphdr, 32);

                if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + 32, wps->wphdr.ckSize - 24) !=
                    wps->wphdr.ckSize - 24) {
                        strcpy (wpc->error_message, "can't read all of last block!");
                        wps->wphdr.block_samples = 0;
                        wps->wphdr.ckSize = 24;
                        break;
                }

                // render corrupt blocks harmless
                if (!WavpackVerifySingleBlock (wps->blockbuff, !(wpc->open_flags & OPEN_NO_CHECKSUM))) {
                    wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
                    wps->wphdr.block_samples = 0;
                    memcpy (wps->blockbuff, &wps->wphdr, 32);
                }

                // potentially adjusting block_index must be done AFTER verifying block

                if (wpc->open_flags & OPEN_STREAMING)
                    SET_BLOCK_INDEX (wps->wphdr, wps->sample_index = 0);
                else
                    SET_BLOCK_INDEX (wps->wphdr, GET_BLOCK_INDEX (wps->wphdr) - wpc->initial_index);

                memcpy (wps->blockbuff, &wps->wphdr, 32);
                wps->init_done = FALSE;     // we have not yet called unpack_init() for this block

                // if this block has audio, but not the sample index we were expecting, flag an error

                if (!wpc->reduced_channels && wps->wphdr.block_samples && wps->sample_index != GET_BLOCK_INDEX (wps->wphdr))
                    wpc->crc_errors++;

                // if this block has audio, and we're in hybrid lossless mode, read the matching wvc block

                if (wps->wphdr.block_samples && wpc->wvc_flag)
                    read_wvc_block (wpc, 0);

                // if the block does NOT have any audio, call unpack_init() to process non-audio stuff

                if (!wps->wphdr.block_samples) {
                    if (!wps->init_done && !unpack_init (wpc, 0))
                        wpc->crc_errors++;

                    wps->init_done = TRUE;
                }
        }

        // if the current block has no audio, or it's not the first block of a multichannel
        // sequence, or the sample we're on is past the last sample in this block...we need
        // to loop back and read the next block

        if (!wps->wphdr.block_samples || !(wps->wphdr.flags & INITIAL_BLOCK) ||
            wps->sample_index >= GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples)
                continue;

        // There seems to be some missing data, like a block was corrupted or something.
        // If it's not too much data, just fill in with silence here and loop back.

        if (wps->sample_index < GET_BLOCK_INDEX (wps->wphdr)) {
            int32_t zvalue = (wps->wphdr.flags & DSD_FLAG) ? 0x55 : 0;

            samples_to_unpack = (uint32_t) (GET_BLOCK_INDEX (wps->wphdr) - wps->sample_index);

            if (!samples_to_unpack || samples_to_unpack > 262144) {
                strcpy (wpc->error_message, "discontinuity found, aborting file!");
                wps->wphdr.block_samples = 0;
                wps->wphdr.ckSize = 24;
                break;
            }

            if (samples_to_unpack > samples)
                samples_to_unpack = samples;

            wps->sample_index += samples_to_unpack;
            samples_unpacked += samples_to_unpack;
            samples -= samples_to_unpack;

            samples_to_unpack *= (wpc->reduced_channels ? wpc->reduced_channels : num_channels);

            while (samples_to_unpack--)
                *bptr++ = zvalue;

            continue;
        }

        // calculate number of samples to process from this block, then initialize the decoder for
        // this block if we haven't already

        samples_to_unpack = (uint32_t) (GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples - wps->sample_index);

        if (samples_to_unpack > samples)
            samples_to_unpack = samples;

        if (!wps->init_done && !unpack_init (wpc, 0))
            wpc->crc_errors++;

        wps->init_done = TRUE;

        // if this block is not the final block of a multichannel sequence (and we're not truncating
        // to stereo), then enter this conditional block...otherwise we just unpack the samples directly

        if (!wpc->reduced_channels && !(wps->wphdr.flags & FINAL_BLOCK)) {
            int32_t *temp_buffer = (int32_t *)calloc (1, samples_to_unpack * 8);
            uint32_t offset = 0;     // offset to next channel in sequence (0 to num_channels - 1)

            // loop through all the streams...

            while (1) {

                // if the stream has not been allocated and corresponding block read, do that here...

                if (stream_index == wpc->num_streams) {
                    wpc->streams = (WavpackStream **)realloc (wpc->streams, (wpc->num_streams + 1) * sizeof (wpc->streams [0]));

                    if (!wpc->streams)
                        break;

                    wps = wpc->streams [wpc->num_streams++] = (WavpackStream *)calloc (1, sizeof (WavpackStream));

                    if (!wps)
                        break;

                    wps->wpc = wpc;
                    wps->stream_index = stream_index;
                    bcount = read_next_header (wpc->reader, wpc->wv_in, &wps->wphdr);

                    if (bcount == (uint32_t) -1) {
                        wpc->streams [0]->wphdr.block_samples = 0;
                        wpc->streams [0]->wphdr.ckSize = 24;
                        file_done = TRUE;
                        break;
                    }

                    wps->blockbuff = (unsigned char *)malloc (wps->wphdr.ckSize + 8);

                    if (!wps->blockbuff)
                        break;

                    memcpy (wps->blockbuff, &wps->wphdr, 32);

                    if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + 32, wps->wphdr.ckSize - 24) !=
                        wps->wphdr.ckSize - 24) {
                            wpc->streams [0]->wphdr.block_samples = 0;
                            wpc->streams [0]->wphdr.ckSize = 24;
                            file_done = TRUE;
                            break;
                    }

                    // render corrupt blocks harmless
                    if (!WavpackVerifySingleBlock (wps->blockbuff, !(wpc->open_flags & OPEN_NO_CHECKSUM))) {
                        wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
                        wps->wphdr.block_samples = 0;
                        memcpy (wps->blockbuff, &wps->wphdr, 32);
                    }

                    // potentially adjusting block_index must be done AFTER verifying block

                    if (wpc->open_flags & OPEN_STREAMING)
                        SET_BLOCK_INDEX (wps->wphdr, wps->sample_index = 0);
                    else
                        SET_BLOCK_INDEX (wps->wphdr, GET_BLOCK_INDEX (wps->wphdr) - wpc->initial_index);

                    memcpy (wps->blockbuff, &wps->wphdr, 32);

                    // if this block has audio, and we're in hybrid lossless mode, read the matching wvc block

                    if (wpc->wvc_flag)
                        read_wvc_block (wpc, stream_index);

                    // initialize the unpacker for this block

                    if (!unpack_init (wpc, stream_index))
                        wpc->crc_errors++;

                    wps->init_done = TRUE;
                }
                else
                    wps = wpc->streams [stream_index];

#ifdef ENABLE_THREADS
                // If there is a worker thread available, and there have been no errors, and this is not
                // the final block of the multichannel stream, then give it to a worker thread to decode.
                // To reduce context switches, we do the final block in the foreground because we have to
                // wait around anyway for all the workers to complete.

                if (worker_available (wpc) && !wps->mute_error && !(wps->wphdr.flags & FINAL_BLOCK))
                    unpack_samples_enqueue (wps, bptr, offset, samples_to_unpack, FALSE);
                else
#endif
                {
                    unpack_samples_interleave (wps, bptr, offset, temp_buffer, samples_to_unpack);

                    if (wps->sample_index == GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples && wps->mute_error)
                        wpc->crc_errors++;
                }

                if (wps->wphdr.flags & MONO_FLAG)
                    offset++;
                else if (offset == num_channels - 1) {
                    wpc->crc_errors++;
                    offset++;
                }
                else
                    offset += 2;

                // check several clues that we're done with this set of blocks and exit if we are; else do next stream

                if ((wps->wphdr.flags & FINAL_BLOCK) || stream_index == wpc->max_streams - 1 || offset == num_channels)
                    break;
                else
                    stream_index++;
            }

#ifdef ENABLE_THREADS
            worker_threads_finish (wpc);    // for multichannel, wait for all threads to finish before freeing anything
#endif

            free (temp_buffer);

            // if we didn't get all the channels we expected, mute the buffer and flag an error

            if (offset != num_channels) {
                if (wps->wphdr.flags & DSD_FLAG) {
                    int samples_to_zero = samples_to_unpack * num_channels;
                    int32_t *zptr = bptr;

                    while (samples_to_zero--)
                        *zptr++ = 0x55;
                }
                else
                    memset (bptr, 0, samples_to_unpack * num_channels * 4);

                wpc->crc_errors++;
            }

            // go back to the first stream (we're going to leave them all loaded for now because they might have more samples)

            wps = wpc->streams [stream_index = 0];
        }
        // catch the error situation where we have only one channel but run into a stereo block
        // (this avoids overwriting the caller's buffer)
        else if (!(wps->wphdr.flags & MONO_FLAG) && (num_channels == 1 || wpc->reduced_channels == 1)) {
            memset (bptr, 0, samples_to_unpack * sizeof (*bptr));
            wps->sample_index += samples_to_unpack;
            wpc->crc_errors++;
        }
#ifdef ENABLE_THREADS
        // This is where temporal multithreaded decoding occurs. If there's a worker thread available, and no errors
        // have been encountered, and we are going to finish this block, and there are samples beyond this block to
        // be decoded during this same call to WavpackUnpackSamples(), then we give this block to a worker thread to
        // decode to completion. Because we are going to continue decoding the next block in this stream before this
        // one completes, we must make a copy of the stream that can decode in isolation, and we instruct the worker
        // thread to free everything associated with the stream context when it's done.
        else if (worker_available (wpc) && !wps->mute_error &&
            wps->sample_index + samples_to_unpack == GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples &&
            wps->sample_index + samples > GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples) {
                WavpackStream *wps_copy = malloc (sizeof (WavpackStream));

                memcpy (wps_copy, wps, sizeof (WavpackStream));

                // Update the existing WavpackStream so we can use it for the next block before the current one
                // is complete. Because the worker thread will free any allocated areas, we mark those NULL here.
                // Also advance the sample index (even though they're really not decoded yet).

                wps->blockbuff = NULL;
                wps->block2buff = NULL;
                wps->sample_index += samples_to_unpack;

#ifdef ENABLE_DSD
                wps->dsd.probabilities = NULL;
                wps->dsd.summed_probabilities = NULL;
                wps->dsd.lookup_buffer = NULL;
                wps->dsd.value_lookup = NULL;
                wps->dsd.ptable = NULL;
#endif

                unpack_samples_enqueue (wps_copy, bptr, 0, samples_to_unpack, TRUE);
        }
#endif
        else {
#ifdef ENABLE_DSD
            if (wps->wphdr.flags & DSD_FLAG)
                unpack_dsd_samples (wps, bptr, samples_to_unpack);
            else
#endif
                unpack_samples (wps, bptr, samples_to_unpack);

            if (wps->sample_index == GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples && wps->mute_error)
                wpc->crc_errors++;
        }

        if (file_done) {
            strcpy (wpc->error_message, "can't read all of last block!");
            break;
        }

        if (wpc->reduced_channels)
            bptr += samples_to_unpack * wpc->reduced_channels;
        else
            bptr += samples_to_unpack * num_channels;

        samples_unpacked += samples_to_unpack;
        samples -= samples_to_unpack;

        if (wpc->total_samples != -1 && wps->sample_index == wpc->total_samples)
            break;
    }

#ifdef ENABLE_THREADS
    worker_threads_finish (wpc);    // we don't return until all decoding by worker threads is complete
#endif

#ifdef ENABLE_DSD
    if (wpc->decimation_context)    // TODO: this could be parallelized too
        decimate_dsd_run (wpc->decimation_context, buffer, samples_unpacked);
#endif

    return samples_unpacked;
}

///////////////////////////// multithreading code ////////////////////////////////

#ifdef ENABLE_THREADS

// This is the worker thread function for unpacking support, essentially allowing
// unpack_samples_interleave() to be running for multiple streams simultaneously.

#ifdef _WIN32
static unsigned WINAPI unpack_samples_worker_thread (LPVOID param)
#else
static void *unpack_samples_worker_thread (void *param)
#endif
{
    WorkerInfo *cxt = param;
    int32_t *temp_buffer = NULL;
    uint32_t temp_samples = 0;

    while (1) {
        wp_mutex_obtain (*cxt->mutex);
        cxt->state = Ready;
        (*cxt->workers_ready)++;
        wp_condvar_signal (*cxt->global_cond);      // signal that we're ready to work

        while (cxt->state == Ready)                 // wait for something to do
            wp_condvar_wait (cxt->worker_cond, *cxt->mutex);

        wp_mutex_release (*cxt->mutex);

        if (cxt->state == Quit)                     // break out if we're done
            break;

        if (cxt->samcnt > temp_samples) {           // reallocate temp buffer if not big enough
            temp_buffer = (int32_t *) realloc (temp_buffer, (temp_samples = cxt->samcnt) * 8);
            memset (temp_buffer, 0, temp_samples * 8);
        }

        // this is where the work is done
        unpack_samples_interleave (cxt->wps, cxt->outbuf, cxt->offset, temp_buffer, cxt->samcnt);

        if (cxt->wps->mute_error) {                 // this is where we pass back decoding errors
            wp_mutex_obtain (*cxt->mutex);
            (*cxt->worker_errors)++;
            wp_mutex_release (*cxt->mutex);
        }

        if (cxt->free_wps) {                        // if instructed, free the WavpackStream context
            free_single_stream (cxt->wps);
            free (cxt->wps);
        }
    }

    free (temp_buffer);
    wp_thread_exit (0);
    return 0;
}

// Send the given stream to an available worker thread. In the background, the stream will be
// unpacked and written (interleaved) to the given buffer at the specified offset. The "free_wps"
// flag indicates that the WavpackStream structure should be freed once the unpack operation is
// complete because it is a copy of the original created for this operation only.

static void unpack_samples_enqueue (WavpackStream *wps, int32_t *outbuf, int offset, uint32_t samcnt, int free_wps)
{
    WavpackContext *wpc = (WavpackContext *) wps->wpc;  // this is safe here because single-threaded
    int i;

    wp_mutex_obtain (wpc->mutex);

    while (!wpc->workers_ready)
        wp_condvar_wait (wpc->global_cond, wpc->mutex);

    for (i = 0; i < wpc->num_workers; ++i)
        if (wpc->workers [i].state == Ready) {
            wpc->workers [i].wps = wps;
            wpc->workers [i].state = Running;
            wpc->workers [i].outbuf = outbuf;
            wpc->workers [i].offset = offset;
            wpc->workers [i].samcnt = samcnt;
            wpc->workers [i].free_wps = free_wps;
            wp_condvar_signal (wpc->workers [i].worker_cond);
            wpc->workers_ready--;
            break;
        }

    wp_mutex_release (wpc->mutex);
}

static void worker_threads_finish (WavpackContext *wpc)
{
    if (wpc->workers) {
        wp_mutex_obtain (wpc->mutex);

        while (wpc->workers_ready < wpc->num_workers)
            wp_condvar_wait (wpc->global_cond, wpc->mutex);

        wp_mutex_release (wpc->mutex);
    }

    if (wpc->worker_errors) {
        wpc->crc_errors += wpc->worker_errors;
        wpc->worker_errors = 0;
    }
}

// Create the worker thread contexts and start the threads
// (which should all quickly go to the ready state)

static void worker_threads_create (WavpackContext *wpc)
{
    if (!wpc->workers) {
        int i;

        wp_mutex_init (wpc->mutex);
        wp_condvar_init (wpc->global_cond);

        wpc->workers = calloc (wpc->num_workers, sizeof (WorkerInfo));

        for (i = 0; i < wpc->num_workers; ++i) {
            wpc->workers [i].mutex = &wpc->mutex;
            wpc->workers [i].global_cond = &wpc->global_cond;
            wpc->workers [i].workers_ready = &wpc->workers_ready;
            wpc->workers [i].worker_errors = &wpc->worker_errors;
            wp_condvar_init (wpc->workers [i].worker_cond);
            wp_thread_create (wpc->workers [i].thread, unpack_samples_worker_thread, &wpc->workers [i]);

            // gracefully handle failures in creating worker threads

            if (!wpc->workers [i].thread) {
                wp_condvar_delete (wpc->workers [i].worker_cond);
                wpc->num_workers = i;
                break;
            }
        }

        if (!wpc->num_workers) {    // if we failed to start any workers, free the array
            free (wpc->workers);
            wpc->workers = NULL;
        }
    }
}

// Return TRUE if there is a worker thread available. Obviously this depends on
// whether there are worker threads enabled and running, but it also depends
// on whether there is actually a worker thread that's not busy (however we
// don't count this as a requirement if there are many workers).

static int worker_available (WavpackContext *wpc)
{
    int retval = FALSE;

    if (wpc->num_workers && wpc->workers) {
        if (wpc->num_workers < 4) {
            wp_mutex_obtain (wpc->mutex);
            retval = wpc->workers_ready;
            wp_mutex_release (wpc->mutex);
        }
        else
            retval = TRUE;
    }

    return retval;
}
#endif

/* ===================== src/read_words.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// read_words.c

// This module provides entropy word decoding functions using
// a variation on the Rice method.  This was introduced in version 3.93
// because it allows splitting the data into a "lossy" stream and a
// "correction" stream in a very efficient manner and is therefore ideal
// for the "hybrid" mode.  For 4.0, the efficiency of this method was
// significantly improved by moving away from the normal Rice restriction of
// using powers of two for the modulus divisions and now the method can be
// used for both hybrid and pure lossless encoding.

// Samples are divided by median probabilities at 5/7 (71.43%), 10/49 (20.41%),
// and 20/343 (5.83%). Each zone has 3.5 times fewer samples than the
// previous. Using standard Rice coding on this data would result in 1.4
// bits per sample average (not counting sign bit). However, there is a
// very simple encoding that is over 99% efficient with this data and
// results in about 1.22 bits per sample.

#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif


#if defined (HAVE___BUILTIN_CTZ) || defined (_WIN64)
#define USE_CTZ_OPTIMIZATION    // use ctz intrinsic (or Windows equivalent) to count trailing ones
#elif defined(__WATCOMC__) && defined(__386__)
#define USE_CTZ_OPTIMIZATION
extern __inline uint32_t _bsf_watcom (uint32_t);
#pragma aux _bsf_watcom = \
  "bsf eax, eax" \
  parm [eax] nomemory \
  value [eax] \
  modify exact [eax] nomemory;
#else
#define USE_NEXT8_OPTIMIZATION  // optimization using a table to count trailing ones
#endif

#define USE_BITMASK_TABLES      // use tables instead of shifting for certain masking operations

///////////////////////////// local table storage ////////////////////////////

#ifdef USE_NEXT8_OPTIMIZATION
static const char ones_count_table [] = {
    0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,
    0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,
    0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,
    0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,
    0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,
    0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,
    0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,
    0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,8
};
#endif

///////////////////////////// executable code ////////////////////////////////

static uint32_t __inline read_code (Bitstream *bs, uint32_t maxcode);

// Read the next word from the bitstream "wvbits" and return the value. This
// function can be used for hybrid or lossless streams, but since an
// optimized version is available for lossless this function would normally
// be used for hybrid only. If a hybrid lossless stream is being read then
// the "correction" offset is written at the specified pointer. A return value
// of WORD_EOF indicates that the end of the bitstream was reached (all 1s) or
// some other error occurred.

int32_t FASTCALL get_word (WavpackStream *wps, int chan, int32_t *correction)
{
    struct entropy_data *c = wps->w.c + chan;
    uint32_t ones_count, low, mid, high;
    int32_t value;
    int sign;

    if (!wps->wvbits.ptr)
        return WORD_EOF;

    if (correction)
        *correction = 0;

    if (!(wps->w.c [0].median [0] & ~1) && !wps->w.holding_zero && !wps->w.holding_one && !(wps->w.c [1].median [0] & ~1)) {
        uint32_t mask;
        int cbits;

        if (wps->w.zeros_acc) {
            if (--wps->w.zeros_acc) {
                c->slow_level -= (c->slow_level + SLO) >> SLS;
                return 0;
            }
        }
        else {
            for (cbits = 0; cbits < 33 && getbit (&wps->wvbits); ++cbits);

            if (cbits == 33)
                return WORD_EOF;

            if (cbits < 2)
                wps->w.zeros_acc = cbits;
            else {
                for (mask = 1, wps->w.zeros_acc = 0; --cbits; mask <<= 1)
                    if (getbit (&wps->wvbits))
                        wps->w.zeros_acc |= mask;

                wps->w.zeros_acc |= mask;
            }

            if (wps->w.zeros_acc) {
                c->slow_level -= (c->slow_level + SLO) >> SLS;
                CLEARA (wps->w.c [0].median);
                CLEARA (wps->w.c [1].median);
                return 0;
            }
        }
    }

    if (wps->w.holding_zero)
        ones_count = wps->w.holding_zero = 0;
    else {
#ifdef USE_CTZ_OPTIMIZATION
        while (wps->wvbits.bc < LIMIT_ONES) {
            if (++(wps->wvbits.ptr) == wps->wvbits.end)
                wps->wvbits.wrap (&wps->wvbits);

            wps->wvbits.sr |= *(wps->wvbits.ptr) << wps->wvbits.bc;
            wps->wvbits.bc += sizeof (*(wps->wvbits.ptr)) * 8;
        }

#ifdef _MSC_VER
        { unsigned long res; _BitScanForward (&res, (unsigned long)~wps->wvbits.sr); ones_count = (uint32_t) res; }
#elif defined(__WATCOMC__) && defined(__386__)
        ones_count = _bsf_watcom (~wps->wvbits.sr);
#else
        ones_count = __builtin_ctz (~wps->wvbits.sr);
#endif

        if (ones_count >= LIMIT_ONES) {
            wps->wvbits.bc -= ones_count;
            wps->wvbits.sr >>= ones_count;

            for (; ones_count < (LIMIT_ONES + 1) && getbit (&wps->wvbits); ++ones_count);

            if (ones_count == (LIMIT_ONES + 1))
                return WORD_EOF;

            if (ones_count == LIMIT_ONES) {
                uint32_t mask;
                int cbits;

                for (cbits = 0; cbits < 33 && getbit (&wps->wvbits); ++cbits);

                if (cbits == 33)
                    return WORD_EOF;

                if (cbits < 2)
                    ones_count = cbits;
                else {
                    for (mask = 1, ones_count = 0; --cbits; mask <<= 1)
                        if (getbit (&wps->wvbits))
                            ones_count |= mask;

                    ones_count |= mask;
                }

                ones_count += LIMIT_ONES;
            }
        }
        else {
            wps->wvbits.bc -= ones_count + 1;
            wps->wvbits.sr >>= ones_count + 1;
        }
#elif defined (USE_NEXT8_OPTIMIZATION)
        int next8;

        if (wps->wvbits.bc < 8) {
            if (++(wps->wvbits.ptr) == wps->wvbits.end)
                wps->wvbits.wrap (&wps->wvbits);

            next8 = (wps->wvbits.sr |= *(wps->wvbits.ptr) << wps->wvbits.bc) & 0xff;
            wps->wvbits.bc += sizeof (*(wps->wvbits.ptr)) * 8;
        }
        else
            next8 = wps->wvbits.sr & 0xff;

        if (next8 == 0xff) {
            wps->wvbits.bc -= 8;
            wps->wvbits.sr >>= 8;

            for (ones_count = 8; ones_count < (LIMIT_ONES + 1) && getbit (&wps->wvbits); ++ones_count);

            if (ones_count == (LIMIT_ONES + 1))
                return WORD_EOF;

            if (ones_count == LIMIT_ONES) {
                uint32_t mask;
                int cbits;

                for (cbits = 0; cbits < 33 && getbit (&wps->wvbits); ++cbits);

                if (cbits == 33)
                    return WORD_EOF;

                if (cbits < 2)
                    ones_count = cbits;
                else {
                    for (mask = 1, ones_count = 0; --cbits; mask <<= 1)
                        if (getbit (&wps->wvbits))
                            ones_count |= mask;

                    ones_count |= mask;
                }

                ones_count += LIMIT_ONES;
            }
        }
        else {
            wps->wvbits.bc -= (ones_count = ones_count_table [next8]) + 1;
            wps->wvbits.sr >>= ones_count + 1;
        }
#else
        for (ones_count = 0; ones_count < (LIMIT_ONES + 1) && getbit (&wps->wvbits); ++ones_count);

        if (ones_count >= LIMIT_ONES) {
            uint32_t mask;
            int cbits;

            if (ones_count == (LIMIT_ONES + 1))
                return WORD_EOF;

            for (cbits = 0; cbits < 33 && getbit (&wps->wvbits); ++cbits);

            if (cbits == 33)
                return WORD_EOF;

            if (cbits < 2)
                ones_count = cbits;
            else {
                for (mask = 1, ones_count = 0; --cbits; mask <<= 1)
                    if (getbit (&wps->wvbits))
                        ones_count |= mask;

                ones_count |= mask;
            }

            ones_count += LIMIT_ONES;
        }
#endif

        if (wps->w.holding_one) {
            wps->w.holding_one = ones_count & 1;
            ones_count = (ones_count >> 1) + 1;
        }
        else {
            wps->w.holding_one = ones_count & 1;
            ones_count >>= 1;
        }

        wps->w.holding_zero = ~wps->w.holding_one & 1;
    }

    if ((wps->wphdr.flags & HYBRID_FLAG) && !chan)
        update_error_limit (wps);

    if (ones_count == 0) {
        low = 0;
        high = GET_MED (0) - 1;
        DEC_MED0 ();
    }
    else {
        low = GET_MED (0);
        INC_MED0 ();

        if (ones_count == 1) {
            high = low + GET_MED (1) - 1;
            DEC_MED1 ();
        }
        else {
            low += GET_MED (1);
            INC_MED1 ();

            if (ones_count == 2) {
                high = low + GET_MED (2) - 1;
                DEC_MED2 ();
            }
            else {
                low += (ones_count - 2) * GET_MED (2);
                high = low + GET_MED (2) - 1;
                INC_MED2 ();
            }
        }
    }

    low &= 0x7fffffff;
    high &= 0x7fffffff;

    if (low > high)         // make sure high and low make sense
        high = low;

    mid = (high + low + 1) >> 1;

    if (!c->error_limit)
        mid = read_code (&wps->wvbits, high - low) + low;
    else while (high - low > c->error_limit) {
        if (getbit (&wps->wvbits))
            mid = (high + (low = mid) + 1) >> 1;
        else
            mid = ((high = mid - 1) + low + 1) >> 1;
    }

    sign = getbit (&wps->wvbits);

    if (bs_is_open (&wps->wvcbits) && c->error_limit) {
        value = read_code (&wps->wvcbits, high - low) + low;

        if (correction)
            *correction = sign ? (mid - value) : (value - mid);
    }

    if (wps->wphdr.flags & HYBRID_BITRATE) {
        c->slow_level -= (c->slow_level + SLO) >> SLS;
        c->slow_level += wp_log2 (mid);
    }

    return sign ? ~mid : mid;
}

// This is an optimized version of get_word() that is used for lossless only
// (error_limit == 0). Also, rather than obtaining a single sample, it can be
// used to obtain an entire buffer of either mono or stereo samples.

int32_t get_words_lossless (WavpackStream *wps, int32_t *buffer, int32_t nsamples)
{
    struct entropy_data *c = wps->w.c;
    uint32_t ones_count, low, high;
    Bitstream *bs = &wps->wvbits;
    int32_t csamples;
#ifdef USE_NEXT8_OPTIMIZATION
    int32_t next8;
#endif

    if (nsamples && !bs->ptr) {
        memset (buffer, 0, (wps->wphdr.flags & MONO_DATA) ? nsamples * 4 : nsamples * 8);
        return nsamples;
    }

    if (!(wps->wphdr.flags & MONO_DATA))
        nsamples *= 2;

    for (csamples = 0; csamples < nsamples; ++csamples) {
        if (!(wps->wphdr.flags & MONO_DATA))
            c = wps->w.c + (csamples & 1);

        if (wps->w.holding_zero) {
            wps->w.holding_zero = 0;
            low = read_code (bs, GET_MED (0) - 1);
            DEC_MED0 ();
            buffer [csamples] = (getbit (bs)) ? ~low : low;

            if (++csamples == nsamples)
                break;

            if (!(wps->wphdr.flags & MONO_DATA))
                c = wps->w.c + (csamples & 1);
        }

        if (wps->w.c [0].median [0] < 2 && !wps->w.holding_one && wps->w.c [1].median [0] < 2) {
            uint32_t mask;
            int cbits;

            if (wps->w.zeros_acc) {
                if (--wps->w.zeros_acc) {
                    buffer [csamples] = 0;
                    continue;
                }
            }
            else {
                for (cbits = 0; cbits < 33 && getbit (bs); ++cbits);

                if (cbits == 33)
                    break;

                if (cbits < 2)
                    wps->w.zeros_acc = cbits;
                else {
                    for (mask = 1, wps->w.zeros_acc = 0; --cbits; mask <<= 1)
                        if (getbit (bs))
                            wps->w.zeros_acc |= mask;

                    wps->w.zeros_acc |= mask;
                }

                if (wps->w.zeros_acc) {
                    CLEARA (wps->w.c [0].median);
                    CLEARA (wps->w.c [1].median);
                    buffer [csamples] = 0;
                    continue;
                }
            }
        }

#ifdef USE_CTZ_OPTIMIZATION
        while (bs->bc < LIMIT_ONES) {
            if (++(bs->ptr) == bs->end)
                bs->wrap (bs);

            bs->sr |= *(bs->ptr) << bs->bc;
            bs->bc += sizeof (*(bs->ptr)) * 8;
        }

#ifdef _MSC_VER
        { unsigned long res; _BitScanForward (&res, (unsigned long)~wps->wvbits.sr); ones_count = (uint32_t) res; }
#elif defined(__WATCOMC__) && defined(__386__)
        ones_count = _bsf_watcom (~wps->wvbits.sr);
#else
        ones_count = __builtin_ctz (~wps->wvbits.sr);
#endif

        if (ones_count >= LIMIT_ONES) {
            bs->bc -= ones_count;
            bs->sr >>= ones_count;

            for (; ones_count < (LIMIT_ONES + 1) && getbit (bs); ++ones_count);

            if (ones_count == (LIMIT_ONES + 1))
                break;

            if (ones_count == LIMIT_ONES) {
                uint32_t mask;
                int cbits;

                for (cbits = 0; cbits < 33 && getbit (bs); ++cbits);

                if (cbits == 33)
                    break;

                if (cbits < 2)
                    ones_count = cbits;
                else {
                    for (mask = 1, ones_count = 0; --cbits; mask <<= 1)
                        if (getbit (bs))
                            ones_count |= mask;

                    ones_count |= mask;
                }

                ones_count += LIMIT_ONES;
            }
        }
        else {
            bs->bc -= ones_count + 1;
            bs->sr >>= ones_count + 1;
        }
#elif defined (USE_NEXT8_OPTIMIZATION)
        if (bs->bc < 8) {
            if (++(bs->ptr) == bs->end)
                bs->wrap (bs);

            next8 = (bs->sr |= *(bs->ptr) << bs->bc) & 0xff;
            bs->bc += sizeof (*(bs->ptr)) * 8;
        }
        else
            next8 = bs->sr & 0xff;

        if (next8 == 0xff) {
            bs->bc -= 8;
            bs->sr >>= 8;

            for (ones_count = 8; ones_count < (LIMIT_ONES + 1) && getbit (bs); ++ones_count);

            if (ones_count == (LIMIT_ONES + 1))
                break;

            if (ones_count == LIMIT_ONES) {
                uint32_t mask;
                int cbits;

                for (cbits = 0; cbits < 33 && getbit (bs); ++cbits);

                if (cbits == 33)
                    break;

                if (cbits < 2)
                    ones_count = cbits;
                else {
                    for (mask = 1, ones_count = 0; --cbits; mask <<= 1)
                        if (getbit (bs))
                            ones_count |= mask;

                    ones_count |= mask;
                }

                ones_count += LIMIT_ONES;
            }
        }
        else {
            bs->bc -= (ones_count = ones_count_table [next8]) + 1;
            bs->sr >>= ones_count + 1;
        }
#else
        for (ones_count = 0; ones_count < (LIMIT_ONES + 1) && getbit (bs); ++ones_count);

        if (ones_count >= LIMIT_ONES) {
            uint32_t mask;
            int cbits;

            if (ones_count == (LIMIT_ONES + 1))
                break;

            for (cbits = 0; cbits < 33 && getbit (bs); ++cbits);

            if (cbits == 33)
                break;

            if (cbits < 2)
                ones_count = cbits;
            else {
                for (mask = 1, ones_count = 0; --cbits; mask <<= 1)
                    if (getbit (bs))
                        ones_count |= mask;

                ones_count |= mask;
            }

            ones_count += LIMIT_ONES;
        }
#endif

        low = wps->w.holding_one;
        wps->w.holding_one = ones_count & 1;
        wps->w.holding_zero = ~ones_count & 1;
        ones_count = (ones_count >> 1) + low;

        if (ones_count == 0) {
            low = 0;
            high = GET_MED (0) - 1;
            DEC_MED0 ();
        }
        else {
            low = GET_MED (0);
            INC_MED0 ();

            if (ones_count == 1) {
                high = low + GET_MED (1) - 1;
                DEC_MED1 ();
            }
            else {
                low += GET_MED (1);
                INC_MED1 ();

                if (ones_count == 2) {
                    high = low + GET_MED (2) - 1;
                    DEC_MED2 ();
                }
                else {
                    low += (ones_count - 2) * GET_MED (2);
                    high = low + GET_MED (2) - 1;
                    INC_MED2 ();
                }
            }
        }

        low += read_code (bs, high - low);
        buffer [csamples] = (getbit (bs)) ? ~low : low;
    }

    return (wps->wphdr.flags & MONO_DATA) ? csamples : (csamples / 2);
}

// Read a single unsigned value from the specified bitstream with a value
// from 0 to maxcode. If there are exactly a power of two number of possible
// codes then this will read a fixed number of bits; otherwise it reads the
// minimum number of bits and then determines whether another bit is needed
// to define the code.

static uint32_t __inline read_code (Bitstream *bs, uint32_t maxcode)
{
    unsigned long local_sr;
    uint32_t extras, code;
    int bitcount;

    if (maxcode < 2)
        return maxcode ? getbit (bs) : 0;

    bitcount = count_bits (maxcode);
#ifdef USE_BITMASK_TABLES
    extras = bitset [bitcount] - maxcode - 1;
#else
    extras = (1 << bitcount) - maxcode - 1;
#endif

    local_sr = bs->sr;

    while (bs->bc < bitcount) {
        if (++(bs->ptr) == bs->end)
            bs->wrap (bs);

        local_sr |= (long)*(bs->ptr) << bs->bc;
        bs->bc += sizeof (*(bs->ptr)) * 8;
    }

#ifdef USE_BITMASK_TABLES
    if ((code = local_sr & bitmask [bitcount - 1]) >= extras)
#else
    if ((code = local_sr & ((1 << (bitcount - 1)) - 1)) >= extras)
#endif
        code = (code << 1) - extras + ((local_sr >> (bitcount - 1)) & 1);
    else
        bitcount--;

    if (sizeof (local_sr) < 8 && bs->bc > sizeof (local_sr) * 8) {
        bs->bc -= bitcount;
        bs->sr = *(bs->ptr) >> (sizeof (*(bs->ptr)) * 8 - bs->bc);
    }
    else {
        bs->bc -= bitcount;
        bs->sr = local_sr >> bitcount;
    }

    return code;
}

/* ===================== src/entropy_utils.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// entropy_utils.c

// This module contains the functions that process metadata blocks that are
// specific to the entropy decoder; these would be called any time a WavPack
// block was parsed. Additionally, it contains tables and functions that are
// common to both entropy coding and decoding. These are in a module separate
// from the actual entropy encoder (write_words.c) and decoder (read_words.c)
// so that if applications that just do a subset of the full WavPack reading
// and writing can link with a subset of the library.

#include <stdlib.h>
#include <string.h>


///////////////////////////// local table storage ////////////////////////////

const uint32_t bitset [] = {
    1L << 0, 1L << 1, 1L << 2, 1L << 3,
    1L << 4, 1L << 5, 1L << 6, 1L << 7,
    1L << 8, 1L << 9, 1L << 10, 1L << 11,
    1L << 12, 1L << 13, 1L << 14, 1L << 15,
    1L << 16, 1L << 17, 1L << 18, 1L << 19,
    1L << 20, 1L << 21, 1L << 22, 1L << 23,
    1L << 24, 1L << 25, 1L << 26, 1L << 27,
    1L << 28, 1L << 29, 1L << 30, 1L << 31
};

const uint32_t bitmask [] = {
    (1L << 0) - 1, (1L << 1) - 1, (1L << 2) - 1, (1L << 3) - 1,
    (1L << 4) - 1, (1L << 5) - 1, (1L << 6) - 1, (1L << 7) - 1,
    (1L << 8) - 1, (1L << 9) - 1, (1L << 10) - 1, (1L << 11) - 1,
    (1L << 12) - 1, (1L << 13) - 1, (1L << 14) - 1, (1L << 15) - 1,
    (1L << 16) - 1, (1L << 17) - 1, (1L << 18) - 1, (1L << 19) - 1,
    (1L << 20) - 1, (1L << 21) - 1, (1L << 22) - 1, (1L << 23) - 1,
    (1L << 24) - 1, (1L << 25) - 1, (1L << 26) - 1, (1L << 27) - 1,
    (1L << 28) - 1, (1L << 29) - 1, (1L << 30) - 1, 0x7fffffff
};

const char nbits_table [] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,     // 0 - 15
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,     // 16 - 31
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,     // 32 - 47
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,     // 48 - 63
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,     // 64 - 79
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,     // 80 - 95
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,     // 96 - 111
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,     // 112 - 127
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // 128 - 143
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // 144 - 159
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // 160 - 175
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // 176 - 191
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // 192 - 207
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // 208 - 223
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // 224 - 239
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8      // 240 - 255
};

static const unsigned char log2_table [] = {
    0x00, 0x01, 0x03, 0x04, 0x06, 0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x14, 0x15,
    0x16, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e, 0x20, 0x21, 0x22, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2a,
    0x2c, 0x2d, 0x2e, 0x2f, 0x31, 0x32, 0x33, 0x34, 0x36, 0x37, 0x38, 0x39, 0x3b, 0x3c, 0x3d, 0x3e,
    0x3f, 0x41, 0x42, 0x43, 0x44, 0x45, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4d, 0x4e, 0x4f, 0x50, 0x51,
    0x52, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63,
    0x64, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
    0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4,
    0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb2,
    0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc0,
    0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcb, 0xcc, 0xcd, 0xce,
    0xcf, 0xd0, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd8, 0xd9, 0xda, 0xdb,
    0xdc, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe4, 0xe5, 0xe6, 0xe7, 0xe7,
    0xe8, 0xe9, 0xea, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xee, 0xef, 0xf0, 0xf1, 0xf1, 0xf2, 0xf3, 0xf4,
    0xf4, 0xf5, 0xf6, 0xf7, 0xf7, 0xf8, 0xf9, 0xf9, 0xfa, 0xfb, 0xfc, 0xfc, 0xfd, 0xfe, 0xff, 0xff
};

static const unsigned char exp2_table [] = {
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a, 0x0b,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0e, 0x0f, 0x10, 0x10, 0x11, 0x12, 0x13, 0x13, 0x14, 0x15, 0x16, 0x16,
    0x17, 0x18, 0x19, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1d, 0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x24, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3a, 0x3b, 0x3c, 0x3d,
    0x3e, 0x3f, 0x40, 0x41, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a,
    0x5b, 0x5c, 0x5d, 0x5e, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x87, 0x88, 0x89, 0x8a,
    0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad,
    0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0,
    0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc8, 0xc9, 0xca, 0xcb, 0xcd, 0xce, 0xcf, 0xd0, 0xd2, 0xd3, 0xd4,
    0xd6, 0xd7, 0xd8, 0xd9, 0xdb, 0xdc, 0xdd, 0xde, 0xe0, 0xe1, 0xe2, 0xe4, 0xe5, 0xe6, 0xe8, 0xe9,
    0xea, 0xec, 0xed, 0xee, 0xf0, 0xf1, 0xf2, 0xf4, 0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfc, 0xfd, 0xff
};

///////////////////////////// executable code ////////////////////////////////

// Read the median log2 values from the specified metadata structure, convert
// them back to 32-bit unsigned values and store them. If length is not
// exactly correct then we flag and return an error.

int read_entropy_vars (WavpackStream *wps, WavpackMetadata *wpmd)
{
    unsigned char *byteptr = (unsigned char *)wpmd->data;

    if (wpmd->byte_length != ((wps->wphdr.flags & MONO_DATA) ? 6 : 12))
        return FALSE;

    wps->w.c [0].median [0] = wp_exp2s (byteptr [0] + (byteptr [1] << 8));
    wps->w.c [0].median [1] = wp_exp2s (byteptr [2] + (byteptr [3] << 8));
    wps->w.c [0].median [2] = wp_exp2s (byteptr [4] + (byteptr [5] << 8));

    if (!(wps->wphdr.flags & MONO_DATA)) {
        wps->w.c [1].median [0] = wp_exp2s (byteptr [6] + (byteptr [7] << 8));
        wps->w.c [1].median [1] = wp_exp2s (byteptr [8] + (byteptr [9] << 8));
        wps->w.c [1].median [2] = wp_exp2s (byteptr [10] + (byteptr [11] << 8));
    }

    return TRUE;
}

// Read the hybrid related values from the specified metadata structure, convert
// them back to their internal formats and store them. The extended profile
// stuff is not implemented yet, so return an error if we get more data than
// we know what to do with.

int read_hybrid_profile (WavpackStream *wps, WavpackMetadata *wpmd)
{
    unsigned char *byteptr = (unsigned char *)wpmd->data;
    unsigned char *endptr = byteptr + wpmd->byte_length;

    if (wps->wphdr.flags & HYBRID_BITRATE) {
        if (byteptr + (wps->wphdr.flags & MONO_DATA ? 2 : 4) > endptr)
            return FALSE;

        wps->w.c [0].slow_level = wp_exp2s (byteptr [0] + (byteptr [1] << 8));
        byteptr += 2;

        if (!(wps->wphdr.flags & MONO_DATA)) {
            wps->w.c [1].slow_level = wp_exp2s (byteptr [0] + (byteptr [1] << 8));
            byteptr += 2;
        }
    }

    if (byteptr + (wps->wphdr.flags & MONO_DATA ? 2 : 4) > endptr)
        return FALSE;

    wps->w.bitrate_acc [0] = (uint32_t)(byteptr [0] + (byteptr [1] << 8)) << 16;
    byteptr += 2;

    if (!(wps->wphdr.flags & MONO_DATA)) {
        wps->w.bitrate_acc [1] = (uint32_t)(byteptr [0] + (byteptr [1] << 8)) << 16;
        byteptr += 2;
    }

    if (byteptr < endptr) {
        if (byteptr + (wps->wphdr.flags & MONO_DATA ? 2 : 4) > endptr)
            return FALSE;

        wps->w.bitrate_delta [0] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
        byteptr += 2;

        if (!(wps->wphdr.flags & MONO_DATA)) {
            wps->w.bitrate_delta [1] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
            byteptr += 2;
        }

        if (byteptr < endptr)
            return FALSE;
    }
    else
        wps->w.bitrate_delta [0] = wps->w.bitrate_delta [1] = 0;

    return TRUE;
}

// This function is called during both encoding and decoding of hybrid data to
// update the "error_limit" variable which determines the maximum sample error
// allowed in the main bitstream. In the HYBRID_BITRATE mode (which is the only
// currently implemented) this is calculated from the slow_level values and the
// bitrate accumulators. Note that the bitrate accumulators can be changing.

void update_error_limit (WavpackStream *wps)
{
    int bitrate_0 = (wps->w.bitrate_acc [0] += wps->w.bitrate_delta [0]) >> 16;

    if (wps->wphdr.flags & MONO_DATA) {
        if (wps->wphdr.flags & HYBRID_BITRATE) {
            int slow_log_0 = (wps->w.c [0].slow_level + SLO) >> SLS;

            if (slow_log_0 - bitrate_0 > -0x100)
                wps->w.c [0].error_limit = wp_exp2s (slow_log_0 - bitrate_0 + 0x100);
            else
                wps->w.c [0].error_limit = 0;
        }
        else
            wps->w.c [0].error_limit = wp_exp2s (bitrate_0);
    }
    else {
        int bitrate_1 = (wps->w.bitrate_acc [1] += wps->w.bitrate_delta [1]) >> 16;

        if (wps->wphdr.flags & HYBRID_BITRATE) {
            int slow_log_0 = (wps->w.c [0].slow_level + SLO) >> SLS;
            int slow_log_1 = (wps->w.c [1].slow_level + SLO) >> SLS;

            if (wps->wphdr.flags & HYBRID_BALANCE) {
                int balance = (slow_log_1 - slow_log_0 + bitrate_1 + 1) >> 1;

                if (balance > bitrate_0) {
                    bitrate_1 = bitrate_0 * 2;
                    bitrate_0 = 0;
                }
                else if (-balance > bitrate_0) {
                    bitrate_0 = bitrate_0 * 2;
                    bitrate_1 = 0;
                }
                else {
                    bitrate_1 = bitrate_0 + balance;
                    bitrate_0 = bitrate_0 - balance;
                }
            }

            if (slow_log_0 - bitrate_0 > -0x100)
                wps->w.c [0].error_limit = wp_exp2s (slow_log_0 - bitrate_0 + 0x100);
            else
                wps->w.c [0].error_limit = 0;

            if (slow_log_1 - bitrate_1 > -0x100)
                wps->w.c [1].error_limit = wp_exp2s (slow_log_1 - bitrate_1 + 0x100);
            else
                wps->w.c [1].error_limit = 0;
        }
        else {
            wps->w.c [0].error_limit = wp_exp2s (bitrate_0);
            wps->w.c [1].error_limit = wp_exp2s (bitrate_1);
        }
    }
}

// The concept of a base 2 logarithm is used in many parts of WavPack. It is
// a way of sufficiently accurately representing 32-bit signed and unsigned
// values storing only 16 bits (actually fewer). It is also used in the hybrid
// mode for quickly comparing the relative magnitude of large values (i.e.
// division) and providing smooth exponentials using only addition.

// These are not strict logarithms in that they become linear around zero and
// can therefore represent both zero and negative values. They have 8 bits
// of precision and in "roundtrip" conversions the total error never exceeds 1
// part in 225 except for the cases of +/-115 and +/-195 (which error by 1).


// This function returns the log2 for the specified 32-bit unsigned value.
// The maximum value allowed is about 0xff800000 and returns 8447.

int FASTCALL wp_log2 (uint32_t avalue)
{
    int dbits;

    if ((avalue += avalue >> 9) < (1 << 8)) {
        dbits = nbits_table [avalue];
        return (dbits << 8) + log2_table [(avalue << (9 - dbits)) & 0xff];
    }
    else {
        if (avalue < (1L << 16))
            dbits = nbits_table [avalue >> 8] + 8;
        else if (avalue < (1L << 24))
            dbits = nbits_table [avalue >> 16] + 16;
        else
            dbits = nbits_table [avalue >> 24] + 24;

        return (dbits << 8) + log2_table [(avalue >> (dbits - 9)) & 0xff];
    }
}

// This function scans a buffer of longs and accumulates the total log2 value
// of all the samples. This is useful for determining maximum compression
// because the bitstream storage required for entropy coding is proportional
// to the base 2 log of the samples. On some platforms there is an assembly
// version of this.

#if !defined(OPT_ASM_X86) && !defined(OPT_ASM_X64)

uint32_t log2buffer (int32_t *samples, uint32_t num_samples, int limit)
{
    uint32_t result = 0, avalue;
    int dbits;

    while (num_samples--) {
        avalue = abs (*samples++);

        if ((avalue += avalue >> 9) < (1 << 8)) {
            dbits = nbits_table [avalue];
            result += (dbits << 8) + log2_table [(avalue << (9 - dbits)) & 0xff];
        }
        else {
            if (avalue < (1L << 16))
                dbits = nbits_table [avalue >> 8] + 8;
            else if (avalue < (1L << 24))
                dbits = nbits_table [avalue >> 16] + 16;
            else
                dbits = nbits_table [avalue >> 24] + 24;

            result += dbits = (dbits << 8) + log2_table [(avalue >> (dbits - 9)) & 0xff];

            if (limit && dbits >= limit)
                return (uint32_t) -1;
        }
    }

    return result;
}

#endif

// This function returns the log2 for the specified 32-bit signed value.
// All input values are valid and the return values are in the range of
// +/- 8192.

int wp_log2s (int32_t value)
{
    return (value < 0) ? -wp_log2 (-value) : wp_log2 (value);
}

// This function returns the original integer represented by the supplied
// logarithm (at least within the provided accuracy). The log is signed,
// but since a full 32-bit value is returned this can be used for unsigned
// conversions as well (i.e. the input range is -8192 to +8447).

int32_t wp_exp2s (int log)
{
    uint32_t value;

    if (log < 0)
        return ~((uint32_t) wp_exp2s (-log) - 1);

    value = exp2_table [log & 0xff] | 0x100;

    if ((log >>= 8) <= 9)
        return value >> (9 - log);
    else
        return value << ((log - 9) & 0x1f);
}

// These two functions convert internal weights (which are normally +/-1024)
// to and from an 8-bit signed character version for storage in metadata. The
// weights are clipped here in the case that they are outside that range.

signed char store_weight (int weight)
{
    if (weight > 1024)
        weight = 1024;
    else if (weight < -1024)
        weight = -1024;

    if (weight > 0)
        weight -= (weight + 64) >> 7;

    return (weight + 4) >> 3;
}

int restore_weight (signed char weight)
{
    int result;

    if ((result = (int) weight * 8) > 0)
        result += (result + 64) >> 7;

    return result;
}

/* ===================== src/decorr_utils.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// decorr_utils.c

// This module contains the functions that process metadata blocks that are
// specific to the decorrelator. These would be called any time a WavPack
// block was parsed. These are in a module separate from the actual unpack
// decorrelation code (unpack.c) so that if an application just wants to get
// information from WavPack files (rather than actually decoding audio) then
// less code needs to be linked.

#include <stdlib.h>
#include <string.h>


///////////////////////////// executable code ////////////////////////////////

// Read decorrelation terms from specified metadata block into the
// decorr_passes array. The terms range from -3 to 8, plus 17 & 18;
// other values are reserved and generate errors for now. The delta
// ranges from 0 to 7 with all values valid. Note that the terms are
// stored in the opposite order in the decorr_passes array compared
// to packing.

int read_decorr_terms (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int termcnt = wpmd->byte_length;
    unsigned char *byteptr = (unsigned char *)wpmd->data;
    struct decorr_pass *dpp;

    if (termcnt > MAX_NTERMS)
        return FALSE;

    wps->num_terms = termcnt;

    for (dpp = wps->decorr_passes + termcnt - 1; termcnt--; dpp--) {
        dpp->term = (int)(*byteptr & 0x1f) - 5;
        dpp->delta = (*byteptr++ >> 5) & 0x7;

        if (!dpp->term || dpp->term < -3 || (dpp->term > MAX_TERM && dpp->term < 17) || dpp->term > 18 ||
            ((wps->wphdr.flags & MONO_DATA) && dpp->term < 0))
                return FALSE;
    }

    return TRUE;
}

// Read decorrelation weights from specified metadata block into the
// decorr_passes array. The weights range +/-1024, but are rounded and
// truncated to fit in signed chars for metadata storage. Weights are
// separate for the two channels and are specified from the "last" term
// (first during encode). Unspecified weights are set to zero.

int read_decorr_weights (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int termcnt = wpmd->byte_length, tcount;
    char *byteptr = (char *)wpmd->data;
    struct decorr_pass *dpp;

    if (!(wps->wphdr.flags & MONO_DATA))
        termcnt /= 2;

    if (termcnt > wps->num_terms)
        return FALSE;

    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
        dpp->weight_A = dpp->weight_B = 0;

    while (--dpp >= wps->decorr_passes && termcnt--) {
        dpp->weight_A = restore_weight (*byteptr++);

        if (!(wps->wphdr.flags & MONO_DATA))
            dpp->weight_B = restore_weight (*byteptr++);
    }

    return TRUE;
}

// Read decorrelation samples from specified metadata block into the
// decorr_passes array. The samples are signed 32-bit values, but are
// converted to signed log2 values for storage in metadata. Values are
// stored for both channels and are specified from the "last" term
// (first during encode) with unspecified samples set to zero. The
// number of samples stored varies with the actual term value, so
// those must obviously come first in the metadata.

int read_decorr_samples (WavpackStream *wps, WavpackMetadata *wpmd)
{
    unsigned char *byteptr = (unsigned char *)wpmd->data;
    unsigned char *endptr = byteptr + wpmd->byte_length;
    struct decorr_pass *dpp;
    int tcount;

    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
        CLEARA (dpp->samples_A);
        CLEARA (dpp->samples_B);
    }

    if (wps->wphdr.version == 0x402 && (wps->wphdr.flags & HYBRID_FLAG)) {
        if (byteptr + (wps->wphdr.flags & MONO_DATA ? 2 : 4) > endptr)
            return FALSE;

        wps->dc.error [0] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
        byteptr += 2;

        if (!(wps->wphdr.flags & MONO_DATA)) {
            wps->dc.error [1] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
            byteptr += 2;
        }
    }

    while (dpp-- > wps->decorr_passes && byteptr < endptr)
        if (dpp->term > MAX_TERM) {
            if (byteptr + (wps->wphdr.flags & MONO_DATA ? 4 : 8) > endptr)
                return FALSE;

            dpp->samples_A [0] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
            dpp->samples_A [1] = wp_exp2s ((int16_t)(byteptr [2] + (byteptr [3] << 8)));
            byteptr += 4;

            if (!(wps->wphdr.flags & MONO_DATA)) {
                dpp->samples_B [0] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
                dpp->samples_B [1] = wp_exp2s ((int16_t)(byteptr [2] + (byteptr [3] << 8)));
                byteptr += 4;
            }
        }
        else if (dpp->term < 0) {
            if (byteptr + 4 > endptr)
                return FALSE;

            dpp->samples_A [0] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
            dpp->samples_B [0] = wp_exp2s ((int16_t)(byteptr [2] + (byteptr [3] << 8)));
            byteptr += 4;
        }
        else {
            int m = 0, cnt = dpp->term;

            while (cnt--) {
                if (byteptr + (wps->wphdr.flags & MONO_DATA ? 2 : 4) > endptr)
                    return FALSE;

                dpp->samples_A [m] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
                byteptr += 2;

                if (!(wps->wphdr.flags & MONO_DATA)) {
                    dpp->samples_B [m] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
                    byteptr += 2;
                }

                m++;
            }
        }

    return byteptr == endptr;
}

// Read the shaping weights from specified metadata block into the
// WavpackStream structure. Note that there must be two values (even
// for mono streams) and that the values are stored in the same
// manner as decorrelation weights. These would normally be read from
// the "correction" file and are used for lossless reconstruction of
// hybrid data.

int read_shaping_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    if (wpmd->byte_length == 2) {
        char *byteptr = (char *)wpmd->data;

        wps->dc.shaping_acc [0] = (uint32_t) restore_weight (*byteptr++) << 16;
        wps->dc.shaping_acc [1] = (uint32_t) restore_weight (*byteptr++) << 16;
        return TRUE;
    }
    else if (wpmd->byte_length >= (wps->wphdr.flags & MONO_DATA ? 4 : 8)) {
        unsigned char *byteptr = (unsigned char *)wpmd->data;

        wps->dc.error [0] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
        wps->dc.shaping_acc [0] = wp_exp2s ((int16_t)(byteptr [2] + (byteptr [3] << 8)));
        byteptr += 4;

        if (!(wps->wphdr.flags & MONO_DATA)) {
            wps->dc.error [1] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));
            wps->dc.shaping_acc [1] = wp_exp2s ((int16_t)(byteptr [2] + (byteptr [3] << 8)));
            byteptr += 4;
        }

        if (wpmd->byte_length == (wps->wphdr.flags & MONO_DATA ? 6 : 12)) {
            wps->dc.shaping_delta [0] = wp_exp2s ((int16_t)(byteptr [0] + (byteptr [1] << 8)));

            if (!(wps->wphdr.flags & MONO_DATA))
                wps->dc.shaping_delta [1] = wp_exp2s ((int16_t)(byteptr [2] + (byteptr [3] << 8)));
        }

        return TRUE;
    }

    return FALSE;
}

/* ===================== src/unpack_floats.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// unpack_floats.c

// This module deals with the restoration of floating-point data. Note that no
// floating point math is involved here...the values are only processed with
// the macros that directly access the mantissa, exponent, and sign fields.
// That's why we use the f32 type instead of the built-in float type.

#include <stdlib.h>


static void float_values_nowvx (WavpackStream *wps, int32_t *values, int32_t num_values);

void float_values (WavpackStream *wps, int32_t *values, int32_t num_values)
{
    int min_shifted_zeros = wps->float_min_shifted_zeros;
    int max_shifted_ones = wps->float_max_shifted_ones;
    uint32_t crc = wps->crc_x;

    if (!bs_is_open (&wps->wvxbits)) {
        float_values_nowvx (wps, values, num_values);
        return;
    }

    while (num_values--) {
        int shift_count = 0, exp = wps->float_max_exp;
        f32 outval = 0;
        uint32_t temp;

        if (*values == 0) {
            if (wps->float_flags & FLOAT_ZEROS_SENT) {
                if (getbit (&wps->wvxbits)) {
                    getbits (&temp, 23, &wps->wvxbits);
                    set_mantissa (outval, temp);

                    if (exp >= 25) {
                        getbits (&temp, 8, &wps->wvxbits);
                        set_exponent (outval, temp);
                    }

                    set_sign (outval, getbit (&wps->wvxbits));
                }
                else if (wps->float_flags & FLOAT_NEG_ZEROS)
                    set_sign (outval, getbit (&wps->wvxbits));
            }
        }
        else {
            *(uint32_t*)values <<= (wps->float_shift & 0x1f);

            if (*values < 0) {
                *values = -*values;
                set_sign (outval, 1);
            }

            if (*values == 0x1000000) {
                if (getbit (&wps->wvxbits)) {
                    getbits (&temp, 23, &wps->wvxbits);
                    set_mantissa (outval, temp);
                }

                set_exponent (outval, 255);
            }
            else {
                if (exp)
                    while (!(*values & 0x800000) && --exp) {
                        shift_count++;
                        *(uint32_t*)values <<= 1;
                    }

                if (shift_count &= 0x1f) {
                    if ((wps->float_flags & FLOAT_SHIFT_ONES) ||
                        ((wps->float_flags & FLOAT_SHIFT_SAME) && getbit (&wps->wvxbits)))
                            *values |= ((1U << shift_count) - 1);
                    else if (wps->float_flags & FLOAT_SHIFT_SENT) {
                        int32_t mask = (1U << shift_count) - 1;
                        int num_zeros = 0;

                        if (max_shifted_ones && shift_count > max_shifted_ones)
                            num_zeros = shift_count - max_shifted_ones;

                        if (min_shifted_zeros > num_zeros)
                            num_zeros = (min_shifted_zeros > shift_count) ? shift_count : min_shifted_zeros;

                        if ((shift_count -= num_zeros) > 0) {
                            getbits (&temp, shift_count, &wps->wvxbits);
                            *values |= (temp << num_zeros) & mask;
                        }
                    }
                }

                set_mantissa (outval, *values);
                set_exponent (outval, exp);
            }
        }

        crc = crc * 27 + get_mantissa (outval) * 9 + get_exponent (outval) * 3 + get_sign (outval);
        * (f32 *) values++ = outval;
    }

    wps->crc_x = crc;
}

static void float_values_nowvx (WavpackStream *wps, int32_t *values, int32_t num_values)
{
    while (num_values--) {
        int shift_count = 0, exp = wps->float_max_exp;
        f32 outval = 0;

        if (*values) {
            *(uint32_t*)values <<= (wps->float_shift & 0x1f);

            if (*values < 0) {
                *values = -*values;
                set_sign (outval, 1);
            }

            if (*values >= 0x1000000) {
                while (*values & 0xf000000) {
                    *values >>= 1;
                    ++exp;
                }
            }
            else if (exp) {
                while (!(*values & 0x800000) && --exp) {
                    shift_count++;
                    *(uint32_t*)values <<= 1;
                }

                if ((shift_count &= 0x1f) && (wps->float_flags & FLOAT_SHIFT_ONES))
                    *values |= ((1U << shift_count) - 1);
            }

            set_mantissa (outval, *values);
            set_exponent (outval, exp);
        }

        * (f32 *) values++ = outval;
    }
}

/* ===================== src/unpack_dsd.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** DSDPACK ****                            //
//         Lossless DSD (Direct Stream Digital) Audio Compressor          //
//                Copyright (c) 2013 - 2024 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// unpack_dsd.c

// This module actually handles the uncompression of the DSD audio data.

#ifdef ENABLE_DSD

#include <stdlib.h>
#include <string.h>
#include <math.h>


///////////////////////////// executable code ////////////////////////////////

// This function initializes the main range-encoded data for DSD audio samples

static int init_dsd_block_fast (WavpackStream *wps, WavpackMetadata *wpmd);
static int init_dsd_block_high (WavpackStream *wps, WavpackMetadata *wpmd);
static int decode_fast (WavpackStream *wps, int32_t *output, int sample_count);
static int decode_high (WavpackStream *wps, int32_t *output, int sample_count);

int init_dsd_block (WavpackStream *wps, WavpackMetadata *wpmd)
{
    if (wpmd->byte_length < 2)
        return FALSE;

    wps->dsd.byteptr = (unsigned char *)wpmd->data;
    wps->dsd.endptr = wps->dsd.byteptr + wpmd->byte_length;

    if (*wps->dsd.byteptr > 31)
        return FALSE;

    // safe to cast away const on stream 0 only
    if (!wps->stream_index)
        ((WavpackContext *)wps->wpc)->dsd_multiplier = 1U << *wps->dsd.byteptr++;
    else
        wps->dsd.byteptr++;

    wps->dsd.mode = *wps->dsd.byteptr++;

    if (!wps->dsd.mode) {
        if (wps->dsd.endptr - wps->dsd.byteptr != wps->wphdr.block_samples * (wps->wphdr.flags & MONO_DATA ? 1 : 2)) {
            return FALSE;
        }

        wps->dsd.ready = 1;
        return TRUE;
    }

    if (wps->dsd.mode == 1)
        return init_dsd_block_fast (wps, wpmd);
    else if (wps->dsd.mode == 3)
        return init_dsd_block_high (wps, wpmd);
    else
        return FALSE;
}

int32_t unpack_dsd_samples (WavpackStream *wps, int32_t *buffer, uint32_t sample_count)
{
    uint32_t flags = wps->wphdr.flags;

    // don't attempt to decode past the end of the block, but watch out for overflow!

    if (wps->sample_index + sample_count > GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples &&
        (uint32_t) (GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples - wps->sample_index) < sample_count)
            sample_count = (uint32_t) (GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples - wps->sample_index);

    if (GET_BLOCK_INDEX (wps->wphdr) > wps->sample_index || wps->wphdr.block_samples < sample_count)
        wps->mute_error = TRUE;

    if (!wps->mute_error) {
        if (!wps->dsd.mode) {
            int total_samples = sample_count * ((flags & MONO_DATA) ? 1 : 2);
            int32_t *bptr = buffer;

            if (wps->dsd.endptr - wps->dsd.byteptr < total_samples)
                total_samples = (int)(wps->dsd.endptr - wps->dsd.byteptr);

            while (total_samples--)
                wps->crc += (wps->crc << 1) + (*bptr++ = *wps->dsd.byteptr++);
        }
        else if (wps->dsd.mode == 1) {
            if (!decode_fast (wps, buffer, sample_count))
                wps->mute_error = TRUE;
        }
        else if (!decode_high (wps, buffer, sample_count))
            wps->mute_error = TRUE;

        // If we just finished this block, then it's time to check if the applicable checksum matches. Like
        // other decoding errors, this is indicated by setting the mute_error flag.

        if (wps->sample_index + sample_count == GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples &&
            !wps->mute_error && wps->crc != wps->wphdr.crc)
                wps->mute_error = TRUE;
    }

    if (wps->mute_error) {
        int samples_to_null;
        if (wps->wpc->reduced_channels == 1 || wps->wpc->config.num_channels == 1 || (flags & MONO_FLAG))
            samples_to_null = sample_count;
        else
            samples_to_null = sample_count * 2;

        while (samples_to_null--)
            *buffer++ = 0x55;

        wps->sample_index += sample_count;
        return sample_count;
    }

    if (flags & FALSE_STEREO) {
        int32_t *dptr = buffer + sample_count * 2;
        int32_t *sptr = buffer + sample_count;
        int32_t c = sample_count;

        while (c--) {
            *--dptr = *--sptr;
            *--dptr = *sptr;
        }
    }

    wps->sample_index += sample_count;

    return sample_count;
}

/*------------------------------------------------------------------------------------------------------------------------*/

// #define DSD_BYTE_READY(low,high) (((low) >> 24) == ((high) >> 24))
// #define DSD_BYTE_READY(low,high) (!(((low) ^ (high)) >> 24))
#define DSD_BYTE_READY(low,high) (!(((low) ^ (high)) & 0xff000000))

static int init_dsd_block_fast (WavpackStream *wps, WavpackMetadata *wpmd)
{
    unsigned char history_bits, max_probability, *lb_ptr;
    int total_summed_probabilities = 0, bi, i;

    (void) wpmd;

    if (wps->dsd.byteptr == wps->dsd.endptr)
        return FALSE;

    history_bits = *wps->dsd.byteptr++;

    if (wps->dsd.byteptr == wps->dsd.endptr || history_bits > MAX_HISTORY_BITS)
        return FALSE;

    wps->dsd.history_bins = 1 << history_bits;

    free_dsd_tables (wps);
    lb_ptr = wps->dsd.lookup_buffer = (unsigned char *)malloc (wps->dsd.history_bins * MAX_BYTES_PER_BIN);
    wps->dsd.value_lookup = (unsigned char **)malloc (sizeof (*wps->dsd.value_lookup) * wps->dsd.history_bins);
    memset (wps->dsd.value_lookup, 0, sizeof (*wps->dsd.value_lookup) * wps->dsd.history_bins);
    wps->dsd.summed_probabilities = (uint16_t (*)[256])malloc (sizeof (*wps->dsd.summed_probabilities) * wps->dsd.history_bins);
    wps->dsd.probabilities = (unsigned char (*)[256])malloc (sizeof (*wps->dsd.probabilities) * wps->dsd.history_bins);

    max_probability = *wps->dsd.byteptr++;

    if (max_probability < 0xff) {
        unsigned char *outptr = (unsigned char *) wps->dsd.probabilities;
        unsigned char *outend = outptr + sizeof (*wps->dsd.probabilities) * wps->dsd.history_bins;

        while (outptr < outend && wps->dsd.byteptr < wps->dsd.endptr) {
            int code = *wps->dsd.byteptr++;

            if (code > max_probability) {
                int zcount = code - max_probability;

                while (outptr < outend && zcount--)
                    *outptr++ = 0;
            }
            else if (code)
                *outptr++ = (unsigned char)code;
            else
                break;
        }

        if (outptr < outend || (wps->dsd.byteptr < wps->dsd.endptr && *wps->dsd.byteptr++))
            return FALSE;
    }
    else if (wps->dsd.endptr - wps->dsd.byteptr > (int) sizeof (*wps->dsd.probabilities) * wps->dsd.history_bins) {
        memcpy (wps->dsd.probabilities, wps->dsd.byteptr, sizeof (*wps->dsd.probabilities) * wps->dsd.history_bins);
        wps->dsd.byteptr += sizeof (*wps->dsd.probabilities) * wps->dsd.history_bins;
    }
    else
        return FALSE;

    for (bi = 0; bi < wps->dsd.history_bins; ++bi) {
        int32_t sum_values;

        for (sum_values = i = 0; i < 256; ++i)
            wps->dsd.summed_probabilities [bi] [i] = (uint16_t)(sum_values += wps->dsd.probabilities [bi] [i]);

        if (sum_values) {
            if ((total_summed_probabilities += sum_values) > wps->dsd.history_bins * MAX_BYTES_PER_BIN)
                return FALSE;

            wps->dsd.value_lookup [bi] = lb_ptr;

            for (i = 0; i < 256; i++) {
                int c = wps->dsd.probabilities [bi] [i];

                while (c--)
                    *lb_ptr++ = (unsigned char)i;
            }
        }
    }

    if (wps->dsd.endptr - wps->dsd.byteptr < 4 || total_summed_probabilities > wps->dsd.history_bins * MAX_BYTES_PER_BIN)
        return FALSE;

    for (i = 4; i--;)
        wps->dsd.value = (wps->dsd.value << 8) | *wps->dsd.byteptr++;

    wps->dsd.p0 = wps->dsd.p1 = 0;
    wps->dsd.low = 0; wps->dsd.high = 0xffffffff;
    wps->dsd.ready = 1;

    return TRUE;
}

static int decode_fast (WavpackStream *wps, int32_t *output, int sample_count)
{
    int total_samples = sample_count;

    if (!(wps->wphdr.flags & MONO_DATA))
        total_samples *= 2;

    while (total_samples--) {
        unsigned int mult, index, code, i;

        if (!wps->dsd.summed_probabilities [wps->dsd.p0] [255])
            return 0;

        mult = (wps->dsd.high - wps->dsd.low) / wps->dsd.summed_probabilities [wps->dsd.p0] [255];

        if (!mult) {
            if (wps->dsd.endptr - wps->dsd.byteptr >= 4)
                for (i = 4; i--;)
                    wps->dsd.value = (wps->dsd.value << 8) | *wps->dsd.byteptr++;

            wps->dsd.low = 0;
            wps->dsd.high = 0xffffffff;
            mult = wps->dsd.high / wps->dsd.summed_probabilities [wps->dsd.p0] [255];

            if (!mult)
                return 0;
        }

        index = (wps->dsd.value - wps->dsd.low) / mult;

        if (index >= wps->dsd.summed_probabilities [wps->dsd.p0] [255])
            return 0;

        if ((*output++ = code = wps->dsd.value_lookup [wps->dsd.p0] [index]) != 0)
            wps->dsd.low += wps->dsd.summed_probabilities [wps->dsd.p0] [code-1] * mult;

        wps->dsd.high = wps->dsd.low + wps->dsd.probabilities [wps->dsd.p0] [code] * mult - 1;
        wps->crc += (wps->crc << 1) + code;

        if (wps->wphdr.flags & MONO_DATA)
            wps->dsd.p0 = code & (wps->dsd.history_bins-1);
        else {
            wps->dsd.p0 = wps->dsd.p1;
            wps->dsd.p1 = code & (wps->dsd.history_bins-1);
        }

        while (DSD_BYTE_READY (wps->dsd.high, wps->dsd.low) && wps->dsd.byteptr < wps->dsd.endptr) {
            wps->dsd.value = (wps->dsd.value << 8) | *wps->dsd.byteptr++;
            wps->dsd.high = (wps->dsd.high << 8) | 0xff;
            wps->dsd.low <<= 8;
        }
    }

    return sample_count;
}

/*------------------------------------------------------------------------------------------------------------------------*/

#define PTABLE_BITS 8
#define PTABLE_BINS (1<<PTABLE_BITS)
#define PTABLE_MASK (PTABLE_BINS-1)

#define UP   0x010000fe
#define DOWN 0x00010000
#define DECAY 8

#define PRECISION 20
#define VALUE_ONE (1 << PRECISION)
#define PRECISION_USE 12

#define RATE_S 20

static void init_ptable (int32_t *table, int rate_i, int rate_s)
{
    int value = 0x808000, rate = rate_i << 8, c, i;

    for (c = (rate + 128) >> 8; c--;)
        value += (DOWN - value) >> DECAY;

    for (i = 0; i < PTABLE_BINS/2; ++i) {
        table [i] = value;
        table [PTABLE_BINS-1-i] = 0x100ffff - value;

        if (value > 0x010000) {
            rate += (rate * rate_s + 128) >> 8;

            for (c = (rate + 64) >> 7; c--;)
                value += (DOWN - value) >> DECAY;
        }
    }
}

static int init_dsd_block_high (WavpackStream *wps, WavpackMetadata *wpmd)
{
    uint32_t flags = wps->wphdr.flags;
    int channel, rate_i, rate_s, i;

    (void) wpmd;

    if (wps->dsd.endptr - wps->dsd.byteptr < ((flags & MONO_DATA) ? 13 : 20))
        return FALSE;

    rate_i = *wps->dsd.byteptr++;
    rate_s = *wps->dsd.byteptr++;

    if (rate_s != RATE_S)
        return FALSE;

    if (!wps->dsd.ptable)
        wps->dsd.ptable = (int32_t *)malloc (PTABLE_BINS * sizeof (*wps->dsd.ptable));

    init_ptable (wps->dsd.ptable, rate_i, rate_s);

    for (channel = 0; channel < ((flags & MONO_DATA) ? 1 : 2); ++channel) {
        DSDfilters *sp = wps->dsd.filters + channel;

        sp->filter1 = *wps->dsd.byteptr++ << (PRECISION - 8);
        sp->filter2 = *wps->dsd.byteptr++ << (PRECISION - 8);
        sp->filter3 = *wps->dsd.byteptr++ << (PRECISION - 8);
        sp->filter4 = *wps->dsd.byteptr++ << (PRECISION - 8);
        sp->filter5 = *wps->dsd.byteptr++ << (PRECISION - 8);
        sp->filter6 = 0;
        sp->factor = *wps->dsd.byteptr++ & 0xff;
        sp->factor |= (*wps->dsd.byteptr++ << 8) & 0xff00;
        sp->factor = (int32_t)((uint32_t)sp->factor << 16) >> 16;
    }

    wps->dsd.high = 0xffffffff;
    wps->dsd.low = 0x0;

    for (i = 4; i--;)
        wps->dsd.value = (wps->dsd.value << 8) | *wps->dsd.byteptr++;

    wps->dsd.ready = 1;

    return TRUE;
}

static int decode_high (WavpackStream *wps, int32_t *output, int sample_count)
{
    int total_samples = sample_count, stereo = (wps->wphdr.flags & MONO_DATA) ? 0 : 1;
    DSDfilters *sp = wps->dsd.filters;

    while (total_samples--) {
        int bitcount = 8;

        sp [0].value = sp [0].filter1 - sp [0].filter5 + ((sp [0].filter6 * sp [0].factor) >> 2);

        if (stereo)
            sp [1].value = sp [1].filter1 - sp [1].filter5 + ((sp [1].filter6 * sp [1].factor) >> 2);

        while (bitcount--) {
            int32_t *pp = wps->dsd.ptable + ((sp [0].value >> (PRECISION - PRECISION_USE)) & PTABLE_MASK);
            uint32_t split = wps->dsd.low + ((wps->dsd.high - wps->dsd.low) >> 8) * (*pp >> 16);

            if (wps->dsd.value <= split) {
                wps->dsd.high = split;
                *pp += (UP - *pp) >> DECAY;
                sp [0].filter0 = -1;
            }
            else {
                wps->dsd.low = split + 1;
                *pp += (DOWN - *pp) >> DECAY;
                sp [0].filter0 = 0;
            }

            while (DSD_BYTE_READY (wps->dsd.high, wps->dsd.low) && wps->dsd.byteptr < wps->dsd.endptr) {
                wps->dsd.value = (wps->dsd.value << 8) | *wps->dsd.byteptr++;
                wps->dsd.high = (wps->dsd.high << 8) | 0xff;
                wps->dsd.low <<= 8;
            }

            sp [0].value += sp [0].filter6 * 8;
            sp [0].byte = (sp [0].byte << 1) | (sp [0].filter0 & 1);
            sp [0].factor += (((sp [0].value ^ sp [0].filter0) >> 31) | 1) & ((sp [0].value ^ (sp [0].value - (sp [0].filter6 * 16))) >> 31);
            sp [0].filter1 += ((sp [0].filter0 & VALUE_ONE) - sp [0].filter1) >> 6;
            sp [0].filter2 += ((sp [0].filter0 & VALUE_ONE) - sp [0].filter2) >> 4;
            sp [0].filter3 += (sp [0].filter2 - sp [0].filter3) >> 4;
            sp [0].filter4 += (sp [0].filter3 - sp [0].filter4) >> 4;
            sp [0].value = (sp [0].filter4 - sp [0].filter5) >> 4;
            sp [0].filter5 += sp [0].value;
            sp [0].filter6 += (sp [0].value - sp [0].filter6) >> 3;
            sp [0].value = sp [0].filter1 - sp [0].filter5 + ((sp [0].filter6 * sp [0].factor) >> 2);

            if (!stereo)
                continue;

            pp = wps->dsd.ptable + ((sp [1].value >> (PRECISION - PRECISION_USE)) & PTABLE_MASK);
            split = wps->dsd.low + ((wps->dsd.high - wps->dsd.low) >> 8) * (*pp >> 16);

            if (wps->dsd.value <= split) {
                wps->dsd.high = split;
                *pp += (UP - *pp) >> DECAY;
                sp [1].filter0 = -1;
            }
            else {
                wps->dsd.low = split + 1;
                *pp += (DOWN - *pp) >> DECAY;
                sp [1].filter0 = 0;
            }

            while (DSD_BYTE_READY (wps->dsd.high, wps->dsd.low) && wps->dsd.byteptr < wps->dsd.endptr) {
                wps->dsd.value = (wps->dsd.value << 8) | *wps->dsd.byteptr++;
                wps->dsd.high = (wps->dsd.high << 8) | 0xff;
                wps->dsd.low <<= 8;
            }

            sp [1].value += sp [1].filter6 * 8;
            sp [1].byte = (sp [1].byte << 1) | (sp [1].filter0 & 1);
            sp [1].factor += (((sp [1].value ^ sp [1].filter0) >> 31) | 1) & ((sp [1].value ^ (sp [1].value - (sp [1].filter6 * 16))) >> 31);
            sp [1].filter1 += ((sp [1].filter0 & VALUE_ONE) - sp [1].filter1) >> 6;
            sp [1].filter2 += ((sp [1].filter0 & VALUE_ONE) - sp [1].filter2) >> 4;
            sp [1].filter3 += (sp [1].filter2 - sp [1].filter3) >> 4;
            sp [1].filter4 += (sp [1].filter3 - sp [1].filter4) >> 4;
            sp [1].value = (sp [1].filter4 - sp [1].filter5) >> 4;
            sp [1].filter5 += sp [1].value;
            sp [1].filter6 += (sp [1].value - sp [1].filter6) >> 3;
            sp [1].value = sp [1].filter1 - sp [1].filter5 + ((sp [1].filter6 * sp [1].factor) >> 2);
        }

        wps->crc += (wps->crc << 1) + (*output++ = sp [0].byte & 0xff);
        sp [0].factor -= (sp [0].factor + 512) >> 10;

        if (stereo) {
            wps->crc += (wps->crc << 1) + (*output++ = wps->dsd.filters [1].byte & 0xff);
            wps->dsd.filters [1].factor -= (wps->dsd.filters [1].factor + 512) >> 10;
        }
    }

    return sample_count;
}

/*------------------------------------------------------------------------------------------------------------------------*/

#if 0

// 80 term DSD decimation filter
// < 1 dB down at 20 kHz
// > 108 dB stopband attenuation (fs/16)

static const int32_t decm_filter [] = {
    4, 17, 56, 147, 336, 693, 1320, 2359,
    4003, 6502, 10170, 15392, 22623, 32389, 45275, 61920,
    82994, 109174, 141119, 179431, 224621, 277068, 336983, 404373,
    479004, 560384, 647741, 740025, 835917, 933849, 1032042, 1128551,
    1221329, 1308290, 1387386, 1456680, 1514425, 1559128, 1589610, 1605059,
    1605059, 1589610, 1559128, 1514425, 1456680, 1387386, 1308290, 1221329,
    1128551, 1032042, 933849, 835917, 740025, 647741, 560384, 479004,
    404373, 336983, 277068, 224621, 179431, 141119, 109174, 82994,
    61920, 45275, 32389, 22623, 15392, 10170, 6502, 4003,
    2359, 1320, 693, 336, 147, 56, 17, 4,
};

#define NUM_FILTER_TERMS 80

#else

// 56 term decimation filter
// < 0.5 dB down at 20 kHz
// > 100 dB stopband attenuation (fs/12)

static const int32_t decm_filter [] = {
    4, 17, 56, 147, 336, 692, 1315, 2337,
    3926, 6281, 9631, 14216, 20275, 28021, 37619, 49155,
    62616, 77870, 94649, 112551, 131049, 149507, 167220, 183448,
    197472, 208636, 216402, 220385, 220385, 216402, 208636, 197472,
    183448, 167220, 149507, 131049, 112551, 94649, 77870, 62616,
    49155, 37619, 28021, 20275, 14216, 9631, 6281, 3926,
    2337, 1315, 692, 336, 147, 56, 17, 4,
};

#define NUM_FILTER_TERMS 56

#endif

#define HISTORY_BYTES ((NUM_FILTER_TERMS+7)/8)

typedef struct {
    unsigned char delay [HISTORY_BYTES];
} DecimationChannel;

typedef struct {
    int32_t conv_tables [HISTORY_BYTES] [256];
    DecimationChannel *chans;
    int num_channels, reset;
} DecimationContext;

static void extrapolate_pcm (int32_t *samples, int samples_to_extrapolate, int samples_visible, int num_channels);

void *decimate_dsd_init (int num_channels)
{
    DecimationContext *context = (DecimationContext *)malloc (sizeof (DecimationContext));
    double filter_sum = 0, filter_scale;
    int i, j;

    if (!context)
        return context;

    memset (context, 0, sizeof (*context));
    context->num_channels = num_channels;
    context->chans = (DecimationChannel *)malloc (num_channels * sizeof (DecimationChannel));

    if (!context->chans) {
        free (context);
        return NULL;
    }

    for (i = 0; i < NUM_FILTER_TERMS; ++i)
        filter_sum += decm_filter [i];

    filter_scale = ((1 << 23) - 1) / filter_sum * 16.0;

    for (i = 0; i < NUM_FILTER_TERMS; ++i) {
        int scaled_term = (int) floor (decm_filter [i] * filter_scale + 0.5);

        if (scaled_term) {
            for (j = 0; j < 256; ++j)
                if (j & (0x80 >> (i & 0x7)))
                    context->conv_tables [i >> 3] [j] += scaled_term;
                else
                    context->conv_tables [i >> 3] [j] -= scaled_term;
        }
    }

    decimate_dsd_reset (context);

    return context;
}

void decimate_dsd_reset (void *decimate_context)
{
    DecimationContext *context = (DecimationContext *) decimate_context;
    int chan = 0, i;

    if (!context)
        return;

    for (chan = 0; chan < context->num_channels; ++chan)
        for (i = 0; i < HISTORY_BYTES; ++i)
            context->chans [chan].delay [i] = 0x55;

    context->reset = 1;
}

void decimate_dsd_run (void *decimate_context, int32_t *samples, int num_samples)
{
    DecimationContext *context = (DecimationContext *) decimate_context;
    int chan = 0, scount = num_samples;
    int32_t *samptr = samples;

    if (!context)
        return;

    while (scount) {
        DecimationChannel *sp = context->chans + chan;
        int32_t sum = 0;

#if (HISTORY_BYTES == 10)
        sum += context->conv_tables [0] [sp->delay [0] = sp->delay [1]];
        sum += context->conv_tables [1] [sp->delay [1] = sp->delay [2]];
        sum += context->conv_tables [2] [sp->delay [2] = sp->delay [3]];
        sum += context->conv_tables [3] [sp->delay [3] = sp->delay [4]];
        sum += context->conv_tables [4] [sp->delay [4] = sp->delay [5]];
        sum += context->conv_tables [5] [sp->delay [5] = sp->delay [6]];
        sum += context->conv_tables [6] [sp->delay [6] = sp->delay [7]];
        sum += context->conv_tables [7] [sp->delay [7] = sp->delay [8]];
        sum += context->conv_tables [8] [sp->delay [8] = sp->delay [9]];
        sum += context->conv_tables [9] [sp->delay [9] = (unsigned char)*samptr];
#elif (HISTORY_BYTES == 7)
        sum += context->conv_tables [0] [sp->delay [0] = sp->delay [1]];
        sum += context->conv_tables [1] [sp->delay [1] = sp->delay [2]];
        sum += context->conv_tables [2] [sp->delay [2] = sp->delay [3]];
        sum += context->conv_tables [3] [sp->delay [3] = sp->delay [4]];
        sum += context->conv_tables [4] [sp->delay [4] = sp->delay [5]];
        sum += context->conv_tables [5] [sp->delay [5] = sp->delay [6]];
        sum += context->conv_tables [6] [sp->delay [6] = (unsigned char)*samptr];
#else
        int i;

        for (i = 0; i < HISTORY_BYTES-1; ++i)
            sum += context->conv_tables [i] [sp->delay [i] = sp->delay [i+1]];

        sum += context->conv_tables [i] [sp->delay [i] = (unsigned char)*samptr];
#endif

        *samptr++ = (sum + 8) >> 4;

        if (++chan == context->num_channels) {
            scount--;
            chan = 0;
        }
    }

    if (context->reset) {
        extrapolate_pcm (samples, HISTORY_BYTES - 1, num_samples, context->num_channels);
        context->reset = 0;
    }
}

// This function is used to linearly extrapolate some samples at the beginning of the first
// decoded frame because we don't have the previous DSD data to prefill the decimation filter.
// Currently we only extrapolate at the beginning of the file because we have an implicit
// delay in the decimation. It might be better, but more complicated, to have zero delay in
// the decimation and split the extrapolated samples between the beginning and end of the
// file.

static void extrapolate_pcm (int32_t *samples, int samples_to_extrapolate, int samples_visible, int num_channels)
{
    int scount = num_channels, min_period = 5, max_period = 10;

    if (samples_visible < samples_to_extrapolate + min_period * 2)
        return;

    if (samples_visible < samples_to_extrapolate + max_period * 2)
        max_period = (samples_visible - samples_to_extrapolate) / 2;

    while (scount--) {
        float left_value_ave = 0.0, right_value_ave = 0.0, slope;
        int period, i;

        for (period = min_period; period <= max_period; ++period) {
            float left_ratio = (samples_to_extrapolate + period / 2.0F) / period, right_ratio = (period / 2.0F) / period;
            int32_t *sam1 = samples + samples_to_extrapolate * num_channels, *sam2 = sam1 + period * num_channels;
            float ave1 = 0.0, ave2 = 0.0;

            for (i = 0; i < period; ++i) {
                ave1 += (float) sam1 [i * num_channels] / period;
                ave2 += (float) sam2 [i * num_channels] / period;
            }

            left_value_ave += ave1 + (ave1 - ave2) * left_ratio;
            right_value_ave += ave1 + (ave1 - ave2) * right_ratio;
        }

        right_value_ave /= (max_period - min_period + 1);
        left_value_ave /= (max_period - min_period + 1);
        slope = (right_value_ave - left_value_ave) / (samples_to_extrapolate - 1);

        for (i = 0; i < samples_to_extrapolate; ++i)
            samples [i * num_channels] = (int32_t) (left_value_ave + i * slope + 0.5);

        samples++;
    }
}

void decimate_dsd_destroy (void *decimate_context)
{
    DecimationContext *context = (DecimationContext *) decimate_context;

    if (!context)
        return;

    if (context->chans)
        free (context->chans);

    free (context);
}

#endif      // ENABLE_DSD

/* ===================== src/unpack_seek.c ===================== */
////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// unpack_seek.c

// This module provides the high-level API for unpacking audio data from
// a specific sample index (i.e., seeking).

#ifndef NO_SEEKING

#include <stdlib.h>
#include <string.h>


///////////////////////////// executable code ////////////////////////////////

static int64_t find_sample (WavpackContext *wpc, void *infile, int64_t header_pos, int64_t sample);

// Seek to the specified sample index, returning TRUE on success. Note that
// files generated with version 4.0 or newer will seek almost immediately.
// Older files can take quite long if required to seek through unplayed
// portions of the file, but will create a seek map so that reverse seeks
// (or forward seeks to already scanned areas) will be very fast. After a
// FALSE return the file should not be accessed again (other than to close
// it); this is a fatal error.

int WavpackSeekSample (WavpackContext *wpc, uint32_t sample)
{
    return WavpackSeekSample64 (wpc, sample);
}

int WavpackSeekSample64 (WavpackContext *wpc, int64_t sample)
{
    WavpackStream *wps = wpc->streams ? wpc->streams [0] : NULL;
    uint32_t bcount, samples_to_skip;
#ifdef ENABLE_DSD
    uint32_t samples_to_decode = 0;
#endif
    int stream_index = 0;
    int32_t *buffer;

    if (wpc->total_samples == -1 || sample >= wpc->total_samples ||
        !wpc->reader->can_seek (wpc->wv_in) || (wpc->open_flags & OPEN_STREAMING) ||
        (wpc->wvc_flag && !wpc->reader->can_seek (wpc->wvc_in)))
            return FALSE;

#ifdef ENABLE_LEGACY
    if (wpc->stream3)
        return seek_sample3 (wpc, (uint32_t) sample);
#endif

#ifdef ENABLE_DSD
    if (wpc->decimation_context) {      // the decimation code needs some context to be sample accurate
        if (sample < 16) {
            samples_to_decode = (uint32_t) sample;
            sample = 0;
        }
        else {
            samples_to_decode = 16;
            sample -= 16;
        }
    }
#endif

    if (!wps->wphdr.block_samples || !(wps->wphdr.flags & INITIAL_BLOCK) || sample < GET_BLOCK_INDEX (wps->wphdr) ||
        sample >= GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples) {

            free_streams (wpc);
            wpc->filepos = find_sample (wpc, wpc->wv_in, wpc->filepos, sample);

            if (wpc->filepos == -1)
                return FALSE;

            if (wpc->wvc_flag) {
                wpc->file2pos = find_sample (wpc, wpc->wvc_in, 0, sample);

                if (wpc->file2pos == -1)
                    return FALSE;
            }
    }

    if (!wps->blockbuff) {
        wpc->reader->set_pos_abs (wpc->wv_in, wpc->filepos);
        wpc->reader->read_bytes (wpc->wv_in, &wps->wphdr, sizeof (WavpackHeader));
        WavpackLittleEndianToNative (&wps->wphdr, (char *)WavpackHeaderFormat);

        if ((wps->wphdr.ckSize & 1) || wps->wphdr.ckSize < 24 || wps->wphdr.ckSize >= 1024 * 1024) {
            free_streams (wpc);
            return FALSE;
        }

        wps->blockbuff = (unsigned char *)malloc (wps->wphdr.ckSize + 8);
        memcpy (wps->blockbuff, &wps->wphdr, sizeof (WavpackHeader));

        if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + sizeof (WavpackHeader), wps->wphdr.ckSize - 24) !=
            wps->wphdr.ckSize - 24) {
                free_streams (wpc);
                return FALSE;
        }

        // render corrupt blocks harmless
        if (!WavpackVerifySingleBlock (wps->blockbuff, !(wpc->open_flags & OPEN_NO_CHECKSUM))) {
            wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
            wps->wphdr.block_samples = 0;
            memcpy (wps->blockbuff, &wps->wphdr, 32);
        }

        SET_BLOCK_INDEX (wps->wphdr, GET_BLOCK_INDEX (wps->wphdr) - wpc->initial_index);
        memcpy (wps->blockbuff, &wps->wphdr, sizeof (WavpackHeader));
        wps->init_done = FALSE;

        if (wpc->wvc_flag) {
            wpc->reader->set_pos_abs (wpc->wvc_in, wpc->file2pos);
            wpc->reader->read_bytes (wpc->wvc_in, &wps->wphdr, sizeof (WavpackHeader));
            WavpackLittleEndianToNative (&wps->wphdr, (char *)WavpackHeaderFormat);

            if ((wps->wphdr.ckSize & 1) || wps->wphdr.ckSize < 24 || wps->wphdr.ckSize >= 1024 * 1024) {
                free_streams (wpc);
                return FALSE;
            }

            wps->block2buff = (unsigned char *)malloc (wps->wphdr.ckSize + 8);
            memcpy (wps->block2buff, &wps->wphdr, sizeof (WavpackHeader));

            if (wpc->reader->read_bytes (wpc->wvc_in, wps->block2buff + sizeof (WavpackHeader), wps->wphdr.ckSize - 24) !=
                wps->wphdr.ckSize - 24) {
                    free_streams (wpc);
                    return FALSE;
            }

            // render corrupt blocks harmless
            if (!WavpackVerifySingleBlock (wps->block2buff, !(wpc->open_flags & OPEN_NO_CHECKSUM))) {
                wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
                wps->wphdr.block_samples = 0;
                memcpy (wps->block2buff, &wps->wphdr, 32);
            }

            SET_BLOCK_INDEX (wps->wphdr, GET_BLOCK_INDEX (wps->wphdr) - wpc->initial_index);
            memcpy (wps->block2buff, &wps->wphdr, sizeof (WavpackHeader));
        }

        if (!wps->init_done && !unpack_init (wpc, stream_index)) {
            free_streams (wpc);
            return FALSE;
        }

        wps->init_done = TRUE;
    }

    while (!wpc->reduced_channels && !(wps->wphdr.flags & FINAL_BLOCK)) {
        if (++stream_index == wpc->num_streams) {

            if (wpc->num_streams == wpc->max_streams) {
                free_streams (wpc);
                return FALSE;
            }

            wpc->streams = (WavpackStream **)realloc (wpc->streams, (wpc->num_streams + 1) * sizeof (wpc->streams [0]));
            wps = wpc->streams [wpc->num_streams] = (WavpackStream *)calloc (1, sizeof (WavpackStream));
            wps->stream_index = wpc->num_streams++;
            wps->wpc = wpc;
            bcount = read_next_header (wpc->reader, wpc->wv_in, &wps->wphdr);

            if (bcount == (uint32_t) -1) {
                free_streams (wpc);
                return FALSE;
            }

            wps->blockbuff = (unsigned char *)malloc (wps->wphdr.ckSize + 8);
            memcpy (wps->blockbuff, &wps->wphdr, 32);

            if (wpc->reader->read_bytes (wpc->wv_in, wps->blockbuff + 32, wps->wphdr.ckSize - 24) !=
                wps->wphdr.ckSize - 24) {
                    free_streams (wpc);
                    return FALSE;
            }

            // render corrupt blocks harmless
            if (!WavpackVerifySingleBlock (wps->blockbuff, !(wpc->open_flags & OPEN_NO_CHECKSUM))) {
                wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
                wps->wphdr.block_samples = 0;
                memcpy (wps->blockbuff, &wps->wphdr, 32);
            }

            wps->init_done = FALSE;

            if (wpc->wvc_flag && !read_wvc_block (wpc, stream_index)) {
                free_streams (wpc);
                return FALSE;
            }

            if (!wps->init_done && !unpack_init (wpc, stream_index)) {
                free_streams (wpc);
                return FALSE;
            }

            wps->init_done = TRUE;
        }
        else
            wps = wpc->streams [stream_index];
    }

    if (sample < wps->sample_index) {
        for (stream_index = 0; stream_index < wpc->num_streams; stream_index++)
            if (!unpack_init (wpc, stream_index))
                return FALSE;
            else
                wpc->streams [stream_index]->init_done = TRUE;
    }

    samples_to_skip = (uint32_t) (sample - wps->sample_index);

    if (samples_to_skip > 131072) {
        free_streams (wpc);
        return FALSE;
    }

    if (samples_to_skip) {
        buffer = (int32_t *)malloc (samples_to_skip * 8);

        for (stream_index = 0; stream_index < wpc->num_streams; stream_index++)
#ifdef ENABLE_DSD
            if (wpc->streams [stream_index]->wphdr.flags & DSD_FLAG)
                unpack_dsd_samples (wpc->streams [stream_index], buffer, samples_to_skip);
            else
#endif
                unpack_samples (wpc->streams [stream_index], buffer, samples_to_skip);

        free (buffer);
    }

#ifdef ENABLE_DSD
    if (wpc->decimation_context)
        decimate_dsd_reset (wpc->decimation_context);

    if (samples_to_decode) {
        buffer = (int32_t *)calloc (1, samples_to_decode * wpc->config.num_channels * 4);

        if (buffer) {
            WavpackUnpackSamples (wpc, buffer, samples_to_decode);
            free (buffer);
        }
    }
#endif

    return TRUE;
}

// Find a valid WavPack header, searching either from the current file position
// (or from the specified position if not -1) and store it (endian corrected)
// at the specified pointer. The return value is the exact file position of the
// header, although we may have actually read past it. Because this function
// is used for seeking to a specific audio sample, it only considers blocks
// that contain audio samples for the initial stream to be valid.

#define BUFSIZE 4096

static int64_t find_header (WavpackStreamReader64 *reader, void *id, int64_t filepos, WavpackHeader *wphdr)
{
    unsigned char *buffer = (unsigned char *)malloc (BUFSIZE), *sp = buffer, *ep = buffer;

    if (filepos != (uint32_t) -1 && reader->set_pos_abs (id, filepos)) {
        free (buffer);
        return -1;
    }

    while (1) {
        int bleft;

        if (sp < ep) {
            bleft = (int)(ep - sp);
            memmove (buffer, sp, bleft);
            ep -= (sp - buffer);
            sp = buffer;
        }
        else {
            if (sp > ep)
                if (reader->set_pos_rel (id, (int32_t)(sp - ep), SEEK_CUR)) {
                    free (buffer);
                    return -1;
                }

            sp = ep = buffer;
            bleft = 0;
        }

        ep += reader->read_bytes (id, ep, BUFSIZE - bleft);

        if (ep - sp < 32) {
            free (buffer);
            return -1;
        }

        while (sp + 32 <= ep)
            if (*sp++ == 'w' && *sp == 'v' && *++sp == 'p' && *++sp == 'k' &&
                !(*++sp & 1) && sp [2] < 16 && !sp [3] && (sp [2] || sp [1] || *sp >= 24) && sp [5] == 4 &&
                sp [4] >= (MIN_STREAM_VERS & 0xff) && sp [4] <= (MAX_STREAM_VERS & 0xff) && sp [18] < 3 && !sp [19]) {
                    memcpy (wphdr, sp - 4, sizeof (*wphdr));
                    WavpackLittleEndianToNative (wphdr, (char *)WavpackHeaderFormat);

                    if (wphdr->block_samples && (wphdr->flags & INITIAL_BLOCK)) {
                        int64_t retpos = reader->get_pos (id) - (ep - sp + 4);
                        free (buffer);
                        return retpos;
                    }

                    if (wphdr->ckSize > 1024)
                        sp += wphdr->ckSize - 1024;
            }
    }
}

// Find the WavPack block that contains the specified sample. If "header_pos"
// is zero, then no information is assumed except the total number of samples
// in the file and its size in bytes. If "header_pos" is non-zero then we
// assume that it is the file position of the valid header image contained in
// the first stream and we can limit our search to either the portion above
// or below that point. If a .wvc file is being used, then this must be called
// for that file also.

static int64_t find_sample (WavpackContext *wpc, void *infile, int64_t header_pos, int64_t sample)
{
    WavpackStream *wps = wpc->streams [0];
    int64_t file_pos1 = 0, file_pos2 = wpc->reader->get_length (infile);
    int64_t sample_pos1 = 0, sample_pos2 = wpc->total_samples;
    double ratio = 0.96;
    int file_skip = 0;

    if (sample >= wpc->total_samples)
        return -1;

    if (header_pos && wps->wphdr.block_samples) {
        if (GET_BLOCK_INDEX (wps->wphdr) > sample) {
            sample_pos2 = GET_BLOCK_INDEX (wps->wphdr);
            file_pos2 = header_pos;
        }
        else if (GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples <= sample) {
            sample_pos1 = GET_BLOCK_INDEX (wps->wphdr);
            file_pos1 = header_pos;
        }
        else
            return header_pos;
    }

    while (1) {
        double bytes_per_sample;
        int64_t seek_pos;

        bytes_per_sample = (double) file_pos2 - file_pos1;
        bytes_per_sample /= sample_pos2 - sample_pos1;
        seek_pos = file_pos1 + (file_skip ? 32 : 0);
        seek_pos += (int64_t)(bytes_per_sample * (sample - sample_pos1) * ratio);
        seek_pos = find_header (wpc->reader, infile, seek_pos, &wps->wphdr);

        if (seek_pos != (int64_t) -1)
            SET_BLOCK_INDEX (wps->wphdr, GET_BLOCK_INDEX (wps->wphdr) - wpc->initial_index);

        if (seek_pos == (int64_t) -1 || seek_pos >= file_pos2) {
            if (ratio > 0.0) {
                if ((ratio -= 0.24) < 0.0)
                    ratio = 0.0;
            }
            else
                return -1;
        }
        else if (GET_BLOCK_INDEX (wps->wphdr) > sample) {
            sample_pos2 = GET_BLOCK_INDEX (wps->wphdr);
            file_pos2 = seek_pos;
        }
        else if (GET_BLOCK_INDEX (wps->wphdr) + wps->wphdr.block_samples <= sample) {

            if (seek_pos == file_pos1)
                file_skip = 1;
            else {
                sample_pos1 = GET_BLOCK_INDEX (wps->wphdr);
                file_pos1 = seek_pos;
            }
        }
        else
            return seek_pos;
    }
}

#endif


