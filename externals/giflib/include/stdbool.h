#if defined(_WIN32) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)

#ifndef _OSMAND_WP_STDBOOL_H_
#define _OSMAND_WP_STDBOOL_H_

#ifndef __cplusplus

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
# if !defined(__GNUC__) ||(__GNUC__ < 3)
  typedef char _Bool;		/* For C compilers without _Bool */
# endif
#endif

#define bool  _Bool
#define true  1
#define false 0

#else

/* C++ */
#define bool  bool
#define true  true
#define false false

#endif

#define __bool_true_false_are_defined 1

#endif // _OSMAND_WP_STDBOOL_H_

#endif // defined(_WIN32) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)