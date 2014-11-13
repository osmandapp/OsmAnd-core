#ifndef _OSMAND_CORE_COMMON_H_
#define _OSMAND_CORE_COMMON_H_

#include <OsmAndCore/stdlib_common.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <type_traits>
#include <functional>

#include <OsmAndCore/QtExtraDefinitions.h>
#include <OsmAndCore/QtExtensions.h>

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

//NOTE: Even for __COUNT_VA_ARGS__() it will give 1
#define __COUNT_VA_ARGS__N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#define __COUNT_VA_ARGS__EXPAND(VALUE) VALUE
#define __COUNT_VA_ARGS__(...)                                                                                              \
    __COUNT_VA_ARGS__EXPAND(__COUNT_VA_ARGS__N(__VA_ARGS__,                                                                 \
        20, 19, 18, 17, 16, 15, 14, 13, 12, 11,                                                                             \
        10,  9,  8,  7,  6,  5,  4,  3,  2,  1))

namespace OsmAnd
{
    template<typename T>
    Q_DECL_CONSTEXPR const T& constOf(T& value)
    {
        return value;
    }

    template<typename T>
    const T& assignAndReturn(T& out, const T& value)
    {
        out = value;
        return value;
    }

    template<typename T>
    T modifyAndReturn(
        const T instance,
        const std::function<void(T& instance)> modifier)
    {
        auto copy = instance;
        modifier(copy);
        return copy;
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
