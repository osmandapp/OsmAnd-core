#ifndef _OSMAND_CORE_COMMON_H_
#define _OSMAND_CORE_COMMON_H_

#include <cassert>
#include <memory>
#include <iostream>

#include <OsmAndCore/QtExtensions.h>

#if !defined(qPrintableRef)
#  define qPrintableRef(stringRef) stringRef.toLocal8Bit().constData()
#endif // !defined(qPrintableRef)

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

    template<typename T>
    T detachedOf(const T& instance)
    {
        T copy = instance;
        copy.detach();
        return copy;
    }

    template <typename T_>
    static int sign(T_ value)
    {
        return (T_(0) < value) - (value < T_(0));
    }
}

#endif // !defined(_OSMAND_CORE_COMMON_H_)
