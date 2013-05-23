#if !defined(__COMMON_H_)
#define __COMMON_H_

#include <assert.h>
#include <iostream>

#ifndef NDEBUG
#   define OSMAND_ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            assert((condition)); \
        } \
    } while (false)
#else
#   define OSMAND_ASSERT(condition, message)
#endif

#if defined(TEXT) && defined(_T)
#   define _L(x) _T(x)
#else
#   if defined(_UNICODE) || defined(UNICODE)
#       define _L(x) L ##x
#   else
#       define _L(x) x
#   endif
#endif

#if defined(_UNICODE) || defined(UNICODE)
#   define QStringToStdXString(x) (x).toStdWString()
#else
#   define QStringToStdXString(x) (x).toStdString()
#endif

#endif // __COMMON_H_
