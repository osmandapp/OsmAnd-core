#ifndef _OSMAND_CORE_CALLABLE_H_
#define _OSMAND_CORE_CALLABLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>

//NOTE: Even for __COUNT_VA_ARGS__() it will give 1
#define __COUNT_VA_ARGS__N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#define __COUNT_VA_ARGS__(...)                                              \
    __COUNT_VA_ARGS__N(__VA_ARGS__,                                         \
        20, 19, 18, 17, 16, 15, 14, 13, 12, 11,                             \
        10,  9,  8,  7,  6,  5,  4,  3,  2,  1)

#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_0
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_1 , std::placeholders::_1
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_2 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_1, std::placeholders::_2
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_3 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_2, std::placeholders::_3
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_4 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_3, std::placeholders::_4
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_5 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_4, std::placeholders::_5
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_6 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_5, std::placeholders::_6
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_7 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_6, std::placeholders::_7
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_8 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_7, std::placeholders::_8
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_9 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_8, std::placeholders::_9
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_10 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_9, std::placeholders::_10
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_11 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_10, std::placeholders::_11
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_12 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_11, std::placeholders::_12
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_13 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_12, std::placeholders::_13
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_14 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_13, std::placeholders::_14
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_15 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_14, std::placeholders::_15
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_16 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_15, std::placeholders::_16
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_17 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_16, std::placeholders::_17
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_18 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_17, std::placeholders::_18
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_19 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_18, std::placeholders::_19
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_20 _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_19, std::placeholders::_20
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_HELPER2(count) _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_##count
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_HELPER1(count) _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_HELPER2(count)
#define _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS(count) _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS_HELPER1(count)

//NOTE: This will be needed for SWIG
/*
#define OSMAND_CALLABLE(name, return_type, ...)                                                                             \
    typedef std::function< return_type ( __VA_ARGS__ )> name;                                                               \
                                                                                                                            \
    struct I##name                                                                                                          \
    {                                                                                                                       \
        I##name()                                                                                                           \
        {                                                                                                                   \
        }                                                                                                                   \
                                                                                                                            \
        virtual ~I##name()                                                                                                  \
        {                                                                                                                   \
        }                                                                                                                   \
                                                                                                                            \
        operator name() const                                                                                               \
        {                                                                                                                   \
            return getBinding();                                                                                            \
        }                                                                                                                   \
                                                                                                                            \
        name getBinding() const                                                                                             \
        {                                                                                                                   \
            return (name)std::bind(&I##name::method, this                                                                   \
                _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS( __COUNT_VA_ARGS__(__VA_ARGS__) ));                                    \
        }                                                                                                                   \
                                                                                                                            \
        virtual return_type method( __VA_ARGS__ ) const = 0;                                                                \
    }
*/
#define OSMAND_CALLABLE(name, return_type, ...)                                                                             \
    typedef std::function< return_type ( __VA_ARGS__ )> name;

#endif // !defined(_OSMAND_CORE_CALLABLE_H_)
