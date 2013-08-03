#if defined(_MSC_VER)

#ifndef _OSMAND_WINDOWS_STDBOOL_H_
#define _OSMAND_WINDOWS_STDBOOL_H_

#ifndef __cplusplus

typedef char _Bool;

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

#endif // _OSMAND_WINDOWS_STDBOOL_H_

#endif // defined(_MSC_VER)