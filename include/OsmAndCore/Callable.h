#ifndef _OSMAND_CORE_CALLABLE_H_
#define _OSMAND_CORE_CALLABLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Observable.h>

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

#if !defined(SWIG)
#   define OSMAND_OBSERVER_CALLABLE(name, ...)                                                                                  \
        typedef std::function< void ( __VA_ARGS__ )> name;                                                                      \
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
            virtual void operator()(__VA_ARGS__) const = 0;                                                                     \
                                                                                                                                \
            operator name() const                                                                                               \
            {                                                                                                                   \
                return getBinding();                                                                                            \
            }                                                                                                                   \
                                                                                                                                \
            name getBinding() const                                                                                             \
            {                                                                                                                   \
                return (name)std::bind(&I##name::operator(), this                                                               \
                    _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS( __COUNT_VA_ARGS__(__VA_ARGS__) ));                                    \
            }                                                                                                                   \
                                                                                                                                \
            /* This is a required workaround, since specifying ObservableAs<> kills compiler here */                            \
            template<typename OBSERVABLE_T>                                                                                     \
            bool attachTo(                                                                                                      \
                const OBSERVABLE_T& observable,                                                                                 \
                const IObservable::Tag tag)                                                                                     \
            {                                                                                                                   \
                return observable.attach(tag, getBinding());                                                                    \
            }                                                                                                                   \
        }
#   define OSMAND_CALLABLE(name, return_type, ...)                                                                              \
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
            virtual return_type operator()(__VA_ARGS__) const = 0;                                                              \
                                                                                                                                \
            operator name() const                                                                                               \
            {                                                                                                                   \
                return getBinding();                                                                                            \
            }                                                                                                                   \
                                                                                                                                \
            name getBinding() const                                                                                             \
            {                                                                                                                   \
                return (name)std::bind(&I##name::operator(), this                                                               \
                    _OSMAND_CALLABLE_UNRWAP_PLACEHOLDERS( __COUNT_VA_ARGS__(__VA_ARGS__) ));                                    \
            }                                                                                                                   \
        }
#else
#   define OSMAND_OBSERVER_CALLABLE(name, ...)                                                                                  \
        struct name;                                                                                                            \
                                                                                                                                \
        SWIG_MARK_AS_DIRECTOR(I##name);                                                                                         \
        %rename(method) I##name::operator()(__VA_ARGS__) const;                                                                 \
        struct I##name                                                                                                          \
        {                                                                                                                       \
            I##name();                                                                                                          \
            virtual ~I##name();                                                                                                 \
                                                                                                                                \
            virtual void operator()( __VA_ARGS__ ) const = 0;                                                                   \
                                                                                                                                \
            bool attachTo(const OsmAnd::ObservableAs<name>& observable, const OsmAnd::ObservableAs<name>::Tag tag);             \
                                                                                                                                \
            name getBinding() const;                                                                                            \
        };
#   define OSMAND_CALLABLE(name, return_type, ...)                                                                              \
        struct name;                                                                                                            \
                                                                                                                                \
        SWIG_MARK_AS_DIRECTOR(I##name);                                                                                         \
        %rename(method) I##name::operator()(__VA_ARGS__) const;                                                                 \
        struct I##name                                                                                                          \
        {                                                                                                                       \
            I##name();                                                                                                          \
            virtual ~I##name();                                                                                                 \
                                                                                                                                \
            virtual return_type operator()( __VA_ARGS__ ) const = 0;                                                            \
                                                                                                                                \
            name getBinding() const;                                                                                            \
        };
#endif

#endif // !defined(_OSMAND_CORE_CALLABLE_H_)
