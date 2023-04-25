/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
#cmakedefine BYTEORDER @BYTEORDER@

/* Define to 1 if you have the `bcopy' function. */
#cmakedefine HAVE_BCOPY 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the `getpagesize' function. */
#cmakedefine HAVE_GETPAGESIZE 1

/* Define to 1 if you have the `arc4random' function. */
#cmakedefine HAVE_ARC4RANDOM 1

/* Define to 1 if you have the `arc4random_buf' function. */
#cmakedefine HAVE_ARC4RANDOM_BUF 1

/* Define to 1 if you have the `getrandom' function. */
#cmakedefine HAVE_GETRANDOM 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the `memmove' function. */
#cmakedefine HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have a working `mmap' system call. */
#cmakedefine HAVE_MMAP 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* whether byteorder is bigendian */
#cmakedefine WORDS_BIGENDIAN

/* Define to specify how much context to retain around the current parse
   point. */
#cmakedefine XML_CONTEXT_BYTES @XML_CONTEXT_BYTES@

/* Define to make parameter entity parsing functionality available. */
#cmakedefine XML_DTD

/* Define to make XML Namespaces functionality available. */
#cmakedefine XML_NS

/* Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C. */
#ifdef _MSC_VER
# define __func__ __FUNCTION__
#endif

/* Define to `long' if <sys/types.h> does not define. */
#cmakedefine off_t @OFF_T@

/* Define to `unsigned' if <sys/types.h> does not define. */
#cmakedefine size_t @SIZE_T@
