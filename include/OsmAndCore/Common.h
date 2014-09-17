#ifndef _OSMAND_CORE_COMMON_H_
#define _OSMAND_CORE_COMMON_H_

#include <OsmAndCore/stdlib_common.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <type_traits>

#include <OsmAndCore/QtExtraDefinitions.h>
#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/QtCommon.h>

#if defined(TEXT) && defined(_T)
#   define xT(x) _T(x)
#else
#   if defined(_UNICODE) || defined(UNICODE)
#       define xT(x) L##x
#   else
#       define xT(x) x
#   endif
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
    const T& assignAndReturn(T& var, const T& value)
    {
        var = value;
        return value;
    }

    template <typename FROM, typename TO>
    struct static_caster
    {
        TO operator()(const FROM& input)
        {
            return static_cast<TO>(input);
        }
    };

    template<typename T_OUT, typename T_IN>
    auto copyAs(const T_IN& input)
        -> typename std::enable_if<
            std::is_same< typename std::iterator_traits< decltype(std::begin(input)) >::value_type, typename T_IN::value_type>::value,
            T_OUT >::type
    {
        T_OUT copy;
        std::transform(
            std::begin(input), std::end(input),
            std::back_inserter(copy),
            static_caster<typename T_IN::value_type, typename T_OUT::value_type>());
        return copy;
    }

    template <typename T>
    static int sign(T value)
    {
        return (T(0) < value) - (value < T(0));
    }

    template <typename T>
    static unsigned int enumToBit(const T value)
    {
        assert(static_cast<unsigned int>(value) <= sizeof(unsigned int) * 8);
        return (1u << static_cast<unsigned int>(value));
    }

    template <typename C, typename T>
    static bool hasEnumBit(const C mask, const T value)
    {
        assert(static_cast<unsigned int>(value) <= sizeof(C) * 8);
        return (mask & enumToBit(value)) != 0;
    }
}

#endif // !defined(_OSMAND_CORE_COMMON_H_)
