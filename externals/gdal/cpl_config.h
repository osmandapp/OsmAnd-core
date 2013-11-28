#ifndef _OSMAND_GDAL_CPL_CONFIG_H_
#define _OSMAND_GDAL_CPL_CONFIG_H_

#define HAVE_LIBZ 1

#if defined(_WIN32)
#   include "upstream.patched/port/cpl_config.h.vc"
#elif defined(ANDROID) || defined(__ANDROID__) || defined(__APPLE__) || defined(__linux__) || defined(__QNXNTO__)

    /* Define if you want to use pthreads based multiprocessing support */
#   define CPL_MULTIPROC_PTHREAD 1

    /* Define to 1 if you have the `PTHREAD_MUTEX_RECURSIVE' constant. */
#   define HAVE_PTHREAD_MUTEX_RECURSIVE 1

    /* Define to 1 if you have the <assert.h> header file. */
#   define HAVE_ASSERT_H 1

    /* Define to 1 if you have the `atoll' function. */
#   define HAVE_ATOLL 1

    /* Define to 1 if you have the <csf.h> header file. */
#   undef HAVE_CSF_H

    /* Define to 1 if you have the <dbmalloc.h> header file. */
#   undef HAVE_DBMALLOC_H

    /* Define to 1 if you have the declaration of `strtof', and to 0 if you don't.
       */
#   define HAVE_DECL_STRTOF 1

    /* Define to 1 if you have the <direct.h> header file. */
#   undef HAVE_DIRECT_H

    /* Define to 1 if you have the <dlfcn.h> header file. */
#   define HAVE_DLFCN_H 1

    /* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
#   undef HAVE_DOPRNT

    /* Define to 1 if you have the <errno.h> header file. */
#   define HAVE_ERRNO_H 1

    /* Define to 1 if you have the <fcntl.h> header file. */
#   define HAVE_FCNTL_H 1

    /* Define to 1 if you have the <float.h> header file. */
#   define HAVE_FLOAT_H 1

    /* Define to 1 if you have the `getcwd' function. */
#   define HAVE_GETCWD 1

    /* Define if you have the iconv() function and it works. */
#   undef HAVE_ICONV

    /* Define as 0 or 1 according to the floating point format suported by the
       machine */
#   define HAVE_IEEEFP 1

    /* Define to 1 if the system has the type `int16'. */
#   undef HAVE_INT16

    /* Define to 1 if the system has the type `int32'. */
#   undef HAVE_INT32

    /* Define to 1 if the system has the type `int8'. */
#   undef HAVE_INT8

    /* Define to 1 if you have the <inttypes.h> header file. */
#   define HAVE_INTTYPES_H 1

    /* Define to 1 if you have the <jpeglib.h> header file. */
#   undef HAVE_JPEGLIB_H

    /* Define to 1 if you have the `dl' library (-ldl). */
#   define HAVE_LIBDL 1

    /* Define to 1 if you have the `m' library (-lm). */
#   define HAVE_LIBM 1

    /* Define to 1 if you have the `pq' library (-lpq). */
#   undef HAVE_LIBPQ

    /* Define to 1 if you have the `rt' library (-lrt). */
#   undef HAVE_LIBRT

    /* Define to 1 if you have the <limits.h> header file. */
#   define HAVE_LIMITS_H 1

    /* Define to 1 if you have the <locale.h> header file. */
#   define HAVE_LOCALE_H 1

    /* Define to 1, if your compiler supports long long data type */
#   define HAVE_LONG_LONG 1

    /* Define to 1 if you have the <memory.h> header file. */
#   define HAVE_MEMORY_H 1

    /* Define to 1 if you have the <png.h> header file. */
#   define HAVE_PNG_H 1

    /* Define to 1 if you have the `snprintf' function. */
#   define HAVE_SNPRINTF 1

    /* Define to 1 if you have the <stdint.h> header file. */
#   define HAVE_STDINT_H 1

    /* Define to 1 if you have the <stdlib.h> header file. */
#   define HAVE_STDLIB_H 1

    /* Define to 1 if you have the <strings.h> header file. */
#   define HAVE_STRINGS_H 1

    /* Define to 1 if you have the <string.h> header file. */
#   define HAVE_STRING_H 1

    /* Define to 1 if you have the `strtof' function. */
#   define HAVE_STRTOF 1

    /* Define to 1 if you have the <sys/stat.h> header file. */
#   define HAVE_SYS_STAT_H 1

    /* Define to 1 if you have the <sys/types.h> header file. */
#   define HAVE_SYS_TYPES_H 1

    /* Define to 1 if you have the <unistd.h> header file. */
#   define HAVE_UNISTD_H 1

    /* Define to 1 if you have the <values.h> header file. */
#   undef HAVE_VALUES_H

    /* Define to 1 if you have the `vprintf' function. */
#   define HAVE_VPRINTF 1

    /* Define to 1 if you have the `vsnprintf' function. */
#   define HAVE_VSNPRINTF 1

    /* Define to 1 if you have the `readlink' function. */
#   define HAVE_READLINK 1

    /* Define to 1 if you have the `posix_spawnp' function. */
#   undef HAVE_POSIX_SPAWNP

    /* Define to 1 if you have the `vfork' function. */
#   define HAVE_VFORK 1

    /* Define to 1 if you have the `lstat' function. */
#   define HAVE_LSTAT 1

    /* Set the native cpu bit order (FILLORDER_LSB2MSB or FILLORDER_MSB2LSB) */
#   ifdef __BIG_ENDIAN__
#       define HOST_FILLORDER FILLORDER_MSB2LSB
#   else
#       define HOST_FILLORDER FILLORDER_LSB2MSB
#   endif

    /* Define as const if the declaration of iconv() needs const. */
#   define ICONV_CONST

    /* For .cpp files, define as const if the declaration of iconv() needs const. */
#   define ICONV_CPP_CONST

    /* Define to the sub-directory in which libtool stores uninstalled libraries.
       */
#   undef LT_OBJDIR

    /* Define for Mac OSX Framework build */
#   undef MACOSX_FRAMEWORK

    /* The size of `int', as computed by sizeof. */
#   define SIZEOF_INT __SIZEOF_INT__

    /* The size of `long', as computed by sizeof. */
#   define SIZEOF_LONG __SIZEOF_LONG__

    /* The size of `unsigned long', as computed by sizeof. */
#   define SIZEOF_UNSIGNED_LONG __SIZEOF_LONG__

    /* The size of `void*', as computed by sizeof. */
#   define SIZEOF_VOIDP __SIZEOF_POINTER__

    /* Define to 1 if you have the ANSI C header files. */
#   define STDC_HEADERS 1

    /* Define to 1 if you have fseek64, ftell64 */
#   define UNIX_STDIO_64 1

    /* Define to 1 if you want to use the -fvisibility GCC flag */
#   define USE_GCC_VISIBILITY_FLAG 1

    /* Define to 1 if GCC atomic builtins are available */
#   undef HAVE_GCC_ATOMIC_BUILTINS

    /* Define to name of 64bit fopen function */
#   define VSI_FOPEN64 fopen

    /* Define to name of 64bit ftruncate function */
#   define VSI_FTRUNCATE64 ftruncate

    /* Define to name of 64bit fseek func */
#   define VSI_FSEEK64 fseeko

    /* Define to name of 64bit ftell func */
#   define VSI_FTELL64 ftello

    /* Define to 1, if you have 64 bit STDIO API */
#   define VSI_LARGE_API_SUPPORTED 1

    /* Define to 1, if you have LARGEFILE64_SOURCE */
#   define VSI_NEED_LARGEFILE64_SOURCE 1

    /* Define to name of 64bit stat function */
#   define VSI_STAT64 stat

    /* Define to name of 64bit stat structure */
#   define VSI_STAT64_T stat

    /* Define to 1 if your processor stores words with the most significant byte
       first (like Motorola and SPARC, unlike Intel and VAX). */
#   ifdef __BIG_ENDIAN__
#       define WORDS_BIGENDIAN 1
#   else
#       undef WORDS_BIGENDIAN
#   endif

    /* Define to 1 if you have the `getaddrinfo' function. */
#   define HAVE_GETADDRINFO 1

#endif

#endif // _OSMAND_GDAL_CPL_CONFIG_H_
