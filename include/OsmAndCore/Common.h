#ifndef _OSMAND_CORE_COMMON_H_
#define _OSMAND_CORE_COMMON_H_

#include <cassert>
#include <memory>
#include <iostream>

#include <OsmAndCore/QtExtensions.h>

#if OSMAND_DEBUG
#   define OSMAND_ASSERT(condition, message)                                                                \
    do {                                                                                                    \
        if (! (condition))                                                                                  \
        {                                                                                                   \
            std::cerr << "Assertion '" #condition "' failed in "                                            \
                << Q_FUNC_INFO << "(" << __FILE__ << ":" << __LINE__ << ": "                                \
                << message << std::endl;                                                                    \
            assert((condition));                                                                            \
        }                                                                                                   \
    } while (false)
#else
#   define OSMAND_ASSERT(condition, message)
#endif

#if defined(TEXT) && defined(_T)
#   define xT(x) _T(x)
#else
#   if defined(_UNICODE) || defined(UNICODE)
#       define xT(x) L##x
#   else
#       define xT(x) x
#   endif
#endif

#if defined(_UNICODE) || defined(UNICODE)
#   define QStringToStlString(x) (x).toStdWString()
#else
#   define QStringToStlString(x) (x).toStdString()
#endif

#define REPEAT_UNTIL(exp) \
    for(;!(exp);)

namespace OsmAnd
{
    template<typename T>
    Q_DECL_CONSTEXPR const T& constOf(T& value)
    {
        return value;
    }
}

#endif // !defined(_OSMAND_CORE_COMMON_H_)
