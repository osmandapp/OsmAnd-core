#ifndef _OSMAND_EXPAT_CONFIG_H_
#define _OSMAND_EXPAT_CONFIG_H_

#if defined(_WIN32)

	/* Force expat to be static */
	#define XML_STATIC

	/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
	#define BYTEORDER 1234

	/* Define to 1 if you have the `bcopy' function. */
	#undef HAVE_BCOPY

	/* Define to 1 if you have the `memmove' function. */
	#define HAVE_MEMMOVE 1

	/* Define to 1 if you have the <stdlib.h> header file. */
	#define HAVE_STDLIB_H 1

	/* Define to specify how much context to retain around the current parse
	   point. */
	#define XML_CONTEXT_BYTES 1024

	/* Define to make parameter entity parsing functionality available. */
	#define XML_DTD

	/* Define to make XML Namespaces functionality available. */
	#define XML_NS

	/* Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C. */
	#ifdef _MSC_VER
	# define __func__ __FUNCTION__
	#endif

	/* Define to `long' if <sys/types.h> does not define. */
	#undef off_t

	/* Define to `unsigned' if <sys/types.h> does not define. */
	#undef size_t

#elif defined(ANDROID) || defined(__ANDROID__)

	/* Force expat to be static */
	#define XML_STATIC

	/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
	#define BYTEORDER 1234

	/* Define to 1 if you have the `bcopy' function. */
	#define HAVE_BCOPY 1

	/* Define to 1 if you have the <dlfcn.h> header file. */
	#define HAVE_DLFCN_H 1

	/* Define to 1 if you have the <fcntl.h> header file. */
	#define HAVE_FCNTL_H 1

	/* Define to 1 if you have the `getpagesize' function. */
	#define HAVE_GETPAGESIZE 1

	/* Define to 1 if you have the <inttypes.h> header file. */
	#define HAVE_INTTYPES_H 1

	/* Define to 1 if you have the `memmove' function. */
	#define HAVE_MEMMOVE 1

	/* Define to 1 if you have the <memory.h> header file. */
	#define HAVE_MEMORY_H 1

	/* Define to 1 if you have a working `mmap' system call. */
	#define HAVE_MMAP 1

	/* Define to 1 if you have the <stdint.h> header file. */
	#define HAVE_STDINT_H 1

	/* Define to 1 if you have the <stdlib.h> header file. */
	#define HAVE_STDLIB_H 1

	/* Define to 1 if you have the <strings.h> header file. */
	#define HAVE_STRINGS_H 1

	/* Define to 1 if you have the <string.h> header file. */
	#define HAVE_STRING_H 1

	/* Define to 1 if you have the <sys/stat.h> header file. */
	#define HAVE_SYS_STAT_H 1

	/* Define to 1 if you have the <sys/types.h> header file. */
	#define HAVE_SYS_TYPES_H 1

	/* Define to 1 if you have the <unistd.h> header file. */
	#define HAVE_UNISTD_H 1

	/* Define to 1 if you have the ANSI C header files. */
	#define STDC_HEADERS 1

	/* whether byteorder is bigendian */
	/* #undef WORDS_BIGENDIAN */

	/* Define to specify how much context to retain around the current parse
	   point. */
	#define XML_CONTEXT_BYTES 1024

	/* Define to make parameter entity parsing functionality available. */
	#define XML_DTD 1

	/* Define to make XML Namespaces functionality available. */
	#define XML_NS 1

	/* Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C. */
	/* #undef __func__ */

	/* Define to empty if `const' does not conform to ANSI C. */
	/* #undef const */

	/* Define to `long' if <sys/types.h> does not define. */
	/* #undef off_t */

	/* Define to `unsigned' if <sys/types.h> does not define. */
	/* #undef size_t */

#elif defined(__APPLE__)

	/* Force expat to be static */
	#define XML_STATIC

	/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
	#define BYTEORDER 1234

	/* Define to 1 if you have the `bcopy' function. */
	#define HAVE_BCOPY 1

	/* Define to 1 if you have the <dlfcn.h> header file. */
	#define HAVE_DLFCN_H 1

	/* Define to 1 if you have the <fcntl.h> header file. */
	#define HAVE_FCNTL_H 1

	/* Define to 1 if you have the `getpagesize' function. */
	#define HAVE_GETPAGESIZE 1

	/* Define to 1 if you have the <inttypes.h> header file. */
	#define HAVE_INTTYPES_H 1

	/* Define to 1 if you have the `memmove' function. */
	#define HAVE_MEMMOVE 1

	/* Define to 1 if you have the <memory.h> header file. */
	#define HAVE_MEMORY_H 1

	/* Define to 1 if you have a working `mmap' system call. */
	#define HAVE_MMAP 1

	/* Define to 1 if you have the <stdint.h> header file. */
	#define HAVE_STDINT_H 1

	/* Define to 1 if you have the <stdlib.h> header file. */
	#define HAVE_STDLIB_H 1

	/* Define to 1 if you have the <strings.h> header file. */
	#define HAVE_STRINGS_H 1

	/* Define to 1 if you have the <string.h> header file. */
	#define HAVE_STRING_H 1

	/* Define to 1 if you have the <sys/stat.h> header file. */
	#define HAVE_SYS_STAT_H 1

	/* Define to 1 if you have the <sys/types.h> header file. */
	#define HAVE_SYS_TYPES_H 1

	/* Define to 1 if you have the <unistd.h> header file. */
	#define HAVE_UNISTD_H 1

	/* Define to 1 if you have the ANSI C header files. */
	#define STDC_HEADERS 1

	/* whether byteorder is bigendian */
	/* #undef WORDS_BIGENDIAN */

	/* Define to specify how much context to retain around the current parse
	   point. */
	#define XML_CONTEXT_BYTES 1024

	/* Define to make parameter entity parsing functionality available. */
	#define XML_DTD 1

	/* Define to make XML Namespaces functionality available. */
	#define XML_NS 1

	/* Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C. */
	/* #undef __func__ */

	/* Define to empty if `const' does not conform to ANSI C. */
	/* #undef const */

	/* Define to `long' if <sys/types.h> does not define. */
	/* #undef off_t */

	/* Define to `unsigned' if <sys/types.h> does not define. */
	/* #undef size_t */

#elif defined(__linux__)

	/* Force expat to be static */
	#define XML_STATIC

	/* 1234 = LIL_ENDIAN, 4321 = BIGENDIAN */
	#define BYTEORDER 1234

	/* Define to 1 if you have the `bcopy' function. */
	#define HAVE_BCOPY 1

	/* Define to 1 if you have the <dlfcn.h> header file. */
	#define HAVE_DLFCN_H 1

	/* Define to 1 if you have the <fcntl.h> header file. */
	#define HAVE_FCNTL_H 1

	/* Define to 1 if you have the `getpagesize' function. */
	#define HAVE_GETPAGESIZE 1

	/* Define to 1 if you have the <inttypes.h> header file. */
	#define HAVE_INTTYPES_H 1

	/* Define to 1 if you have the `memmove' function. */
	#define HAVE_MEMMOVE 1

	/* Define to 1 if you have the <memory.h> header file. */
	#define HAVE_MEMORY_H 1

	/* Define to 1 if you have a working `mmap' system call. */
	#define HAVE_MMAP 1

	/* Define to 1 if you have the <stdint.h> header file. */
	#define HAVE_STDINT_H 1

	/* Define to 1 if you have the <stdlib.h> header file. */
	#define HAVE_STDLIB_H 1

	/* Define to 1 if you have the <strings.h> header file. */
	#define HAVE_STRINGS_H 1

	/* Define to 1 if you have the <string.h> header file. */
	#define HAVE_STRING_H 1

	/* Define to 1 if you have the <sys/stat.h> header file. */
	#define HAVE_SYS_STAT_H 1

	/* Define to 1 if you have the <sys/types.h> header file. */
	#define HAVE_SYS_TYPES_H 1

	/* Define to 1 if you have the <unistd.h> header file. */
	#define HAVE_UNISTD_H 1

	/* Define to 1 if you have the ANSI C header files. */
	#define STDC_HEADERS 1

	/* whether byteorder is bigendian */
	/* #undef WORDS_BIGENDIAN */

	/* Define to specify how much context to retain around the current parse
	   point. */
	#define XML_CONTEXT_BYTES 1024

	/* Define to make parameter entity parsing functionality available. */
	#define XML_DTD 1

	/* Define to make XML Namespaces functionality available. */
	#define XML_NS 1

	/* Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C. */
	/* #undef __func__ */

	/* Define to empty if `const' does not conform to ANSI C. */
	/* #undef const */

	/* Define to `long' if <sys/types.h> does not define. */
	/* #undef off_t */

	/* Define to `unsigned' if <sys/types.h> does not define. */
	/* #undef size_t */

#endif

#endif // _OSMAND_EXPAT_CONFIG_H_