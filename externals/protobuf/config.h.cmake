/* the name of <hash_map> */
#cmakedefine HASH_MAP_CLASS @HASH_MAP_CLASS@

/* the location of <unordered_map> or <hash_map> */
#cmakedefine HASH_MAP_H @HASH_MAP_H@

/* the namespace of hash_map/hash_set */
#cmakedefine HASH_NAMESPACE @HASH_NAMESPACE@

/* the name of <hash_set> */
#cmakedefine HASH_SET_CLASS @HASH_SET_CLASS@

/* the location of <unordered_set> or <hash_set> */
#cmakedefine HASH_SET_H @HASH_SET_H@

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the `ftruncate' function. */
#cmakedefine HAVE_FTRUNCATE 1

/* define if the compiler has hash_map */
#cmakedefine HAVE_HASH_MAP

/* define if the compiler has hash_set */
#cmakedefine HAVE_HASH_SET

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#cmakedefine HAVE_LIMITS_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#cmakedefine HAVE_MEMSET 1

/* Define to 1 if you have the `mkdir' function. */
#cmakedefine HAVE_MKDIR 1

/* Define if you have POSIX threads libraries and header files. */
#cmakedefine HAVE_PTHREAD 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#cmakedefine HAVE_STRCHR 1

/* Define to 1 if you have the `strerror' function. */
#cmakedefine HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the `strtol' function. */
#cmakedefine HAVE_STRTOL 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Enable classes using zlib compression. */
#cmakedefine HAVE_ZLIB

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
#cmakedefine PTHREAD_CREATE_JOINABLE

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define to 1 if you need to in order for `stat' and other things to work. */
#ifndef _POSIX_SOURCE
#   define _POSIX_SOURCE 1
#endif
