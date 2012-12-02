#ifndef _OSMAND_EXPAT_CONFIG_H_
#define _OSMAND_EXPAT_CONFIG_H_

// Windows Phone target
#if defined(_WIN32) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)

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

#endif // defined(_WIN32) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)

#endif // _OSMAND_EXPAT_CONFIG_H_