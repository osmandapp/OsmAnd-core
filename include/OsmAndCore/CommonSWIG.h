#ifndef _OSMAND_CORE_COMMON_SWIG_H_
#define _OSMAND_CORE_COMMON_SWIG_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>

#include <boost/type_traits.hpp>

// SWIG_CASTS
#if defined(SWIG)
#   define SWIG_CASTS(thisClass, parentClass)                                                                                   \
        struct Casts                                                                                                            \
        {                                                                                                                       \
            static std::shared_ptr<thisClass> upcastFrom(const std::shared_ptr<parentClass>& input);                            \
            static std::shared_ptr<parentClass> downcastTo_##parentClass(const std::shared_ptr<thisClass>& input);              \
        }
#elif defined(OSMAND_SWIG)
#   define SWIG_CASTS(thisClass, parentClass)                                                                                   \
        struct Casts                                                                                                            \
        {                                                                                                                       \
            static std::shared_ptr<thisClass> upcastFrom(const std::shared_ptr<parentClass>& input)                             \
            {                                                                                                                   \
                return std::dynamic_pointer_cast< thisClass >(input);                                                           \
            }                                                                                                                   \
            static std::shared_ptr<const thisClass> upcastFrom(const std::shared_ptr<const parentClass>& input)                 \
            {                                                                                                                   \
                return std::dynamic_pointer_cast< const thisClass >(input);                                                     \
            }                                                                                                                   \
            static std::shared_ptr<parentClass> downcastTo_##parentClass(const std::shared_ptr<thisClass>& input)               \
            {                                                                                                                   \
                return std::static_pointer_cast< parentClass >(input);                                                          \
            }                                                                                                                   \
            static std::shared_ptr<const parentClass> downcastTo_##parentClass(const std::shared_ptr<const thisClass>& input)   \
            {                                                                                                                   \
                return std::static_pointer_cast< const parentClass >(input);                                                    \
            }                                                                                                                   \
        }
#else
#   define SWIG_CASTS(thisClass, parentClass)
#endif

// SWIG_MARK_AS_DIRECTOR
#if defined(SWIG)
#   define SWIG_MARK_AS_DIRECTOR(name) %feature("director") name
#else
#   define SWIG_MARK_AS_DIRECTOR(name)
#endif

#if defined(SWIG)
#   define SWIG_OMIT(x)
#else
#   define SWIG_OMIT(x) x
#endif

// Directors
#if defined(SWIG)
#   define SWIG_EMIT_DIRECTOR_BEGIN(name)                                                                                       \
        SWIG_MARK_AS_DIRECTOR(interface_##name);                                                                                \
        class interface_##name                                                                                                  \
        {                                                                                                                       \
        private:                                                                                                                \
        protected:                                                                                                              \
            interface_##name();                                                                                                 \
        public:                                                                                                                 \
            virtual ~interface_##name();

#   define SWIG_EMIT_DIRECTOR_METHOD(return_type, name, ...)                                                                    \
            virtual return_type name( __VA_ARGS__ ) = 0;
#   define SWIG_EMIT_DIRECTOR_CONST_METHOD(return_type, name, ...)                                                              \
            virtual return_type name( __VA_ARGS__ ) const = 0;
#   define SWIG_EMIT_DIRECTOR_VOID_METHOD(name, ...)                                                                            \
            virtual void name( __VA_ARGS__ ) = 0;
#   define SWIG_EMIT_DIRECTOR_VOID_CONST_METHOD(name, ...)                                                                      \
            virtual void name( __VA_ARGS__ ) const = 0;
#   define SWIG_EMIT_DIRECTOR_METHOD_NO_ARGS(return_type, name)                                                                 \
            virtual return_type name() = 0;
#   define SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(return_type, name)                                                           \
            virtual return_type name() const = 0;
#   define SWIG_EMIT_DIRECTOR_VOID_METHOD_NO_ARGS(name)                                                                         \
            virtual void name() = 0;
#   define SWIG_EMIT_DIRECTOR_VOID_CONST_METHOD_NO_ARGS(name)                                                                   \
            virtual void name() const = 0;

#   define SWIG_EMIT_DIRECTOR_END(name)                                                                                         \
            std::shared_ptr< name > instantiateProxy();                                                                         \
        };
#elif defined(OSMAND_SWIG)
#   define SWIG_EMIT_DIRECTOR_BEGIN(name)                                                                                       \
        class interface_##name : public name                                                                                    \
        {                                                                                                                       \
        private:                                                                                                                \
            interface_##name* const _instance;                                                                                  \
                                                                                                                                \
            interface_##name(interface_##name* const instance)                                                                  \
                : _instance(instance)                                                                                           \
            {                                                                                                                   \
            }                                                                                                                   \
        protected:                                                                                                              \
            interface_##name()                                                                                                  \
                : _instance(nullptr)                                                                                            \
            {                                                                                                                   \
            }                                                                                                                   \
        public:                                                                                                                 \
            virtual ~interface_##name()                                                                                         \
            {                                                                                                                   \
            }

#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_0(...)
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_1(...) boost::function_traits<void (__VA_ARGS__)>::arg1_type  arg0
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_2(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_1(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg2_type arg1
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_3(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_2(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg3_type arg2
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_4(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_3(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg4_type arg3
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_5(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_4(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg5_type arg4
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_6(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_5(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg6_type arg5
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_7(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_6(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg7_type arg6
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_8(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_7(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg8_type arg7
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_9(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_8(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg9_type arg8
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_10(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_9(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg10_type arg9
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_11(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_10(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg11_type arg10
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_12(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_11(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg12_type arg11
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_13(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_12(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg13_type arg12
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_14(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_13(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg14_type arg13
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_15(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_14(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg15_type arg14
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_16(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_15(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg16_type arg15
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_17(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_16(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg17_type arg16
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_18(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_17(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg18_type arg17
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_19(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_18(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg19_type arg18
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_20(...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_19(__VA_ARGS__), boost::function_traits<void (__VA_ARGS__)>::arg20_type arg19
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_HELPER2(count, ...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_##count(__VA_ARGS__)
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_HELPER1(count, ...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_HELPER2(count, __VA_ARGS__)
#   define _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS(count, ...) _SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS_HELPER1(count, __VA_ARGS__)

#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_0
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_1 arg0
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_2 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_1, arg1
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_3 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_2, arg2
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_4 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_3, arg3
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_5 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_4, arg4
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_6 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_5, arg5
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_7 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_6, arg6
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_8 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_7, arg7
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_9 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_8, arg8
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_10 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_9, arg9
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_11 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_10, arg10
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_12 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_11, arg11
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_13 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_12, arg12
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_14 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_13, arg13
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_15 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_14, arg14
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_16 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_15, arg15
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_17 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_16, arg16
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_18 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_17, arg17
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_19 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_18, arg18
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_20 _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_19, arg19
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_HELPER2(count) _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_##count
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_HELPER1(count) _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_HELPER2(count)
#   define _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS(count) _SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS_HELPER1(count)

#   define SWIG_EMIT_DIRECTOR_METHOD(return_type, name, ...)                                                                    \
            virtual return_type name(_SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS(__COUNT_VA_ARGS__(__VA_ARGS__), __VA_ARGS__))       \
            {                                                                                                                   \
                return _instance->name(_SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS( __COUNT_VA_ARGS__(__VA_ARGS__) ));                 \
            }
#   define SWIG_EMIT_DIRECTOR_CONST_METHOD(return_type, name, ...)                                                              \
            virtual return_type name(_SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS(__COUNT_VA_ARGS__(__VA_ARGS__), __VA_ARGS__)) const \
            {                                                                                                                   \
                return _instance->name(_SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS( __COUNT_VA_ARGS__(__VA_ARGS__) ));                 \
            }
#   define SWIG_EMIT_DIRECTOR_VOID_METHOD(name, ...)                                                                            \
            virtual void name(_SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS(__COUNT_VA_ARGS__(__VA_ARGS__), __VA_ARGS__))              \
            {                                                                                                                   \
                _instance->name(_SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS( __COUNT_VA_ARGS__(__VA_ARGS__) ));                        \
            }
#   define SWIG_EMIT_DIRECTOR_VOID_CONST_METHOD(name, ...)                                                                      \
            virtual void name(_SWIG_DIRECTOR_METHOD_UNWRAP_ARGUMENTS(__COUNT_VA_ARGS__(__VA_ARGS__), __VA_ARGS__)) const        \
            {                                                                                                                   \
                _instance->name(_SWIG_DIRECTOR_METHOD_PASS_ARGUMENTS( __COUNT_VA_ARGS__(__VA_ARGS__) ));                        \
            }
#   define SWIG_EMIT_DIRECTOR_METHOD_NO_ARGS(return_type, name)                                                                 \
            virtual return_type name()                                                                                          \
            {                                                                                                                   \
                return _instance->name();                                                                                       \
            }
#   define SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(return_type, name)                                                           \
            virtual return_type name() const                                                                                    \
            {                                                                                                                   \
                return _instance->name();                                                                                       \
            }
#   define SWIG_EMIT_DIRECTOR_VOID_METHOD_NO_ARGS(name)                                                                         \
            virtual void name()                                                                                                 \
            {                                                                                                                   \
                _instance->name();                                                                                              \
            }
#   define SWIG_EMIT_DIRECTOR_VOID_CONST_METHOD_NO_ARGS(name)                                                                   \
            virtual void name() const                                                                                           \
            {                                                                                                                   \
                _instance->name();                                                                                              \
            }

#   define SWIG_EMIT_DIRECTOR_END(name)                                                                                         \
            std::shared_ptr< name > instantiateProxy()                                                                          \
            {                                                                                                                   \
                return std::shared_ptr< name >( new interface_##name(this) );                                                   \
            }                                                                                                                   \
        };
#else
#   define SWIG_EMIT_DIRECTOR_BEGIN(name)
#   define SWIG_EMIT_DIRECTOR_METHOD(return_type, name, ...)
#   define SWIG_EMIT_DIRECTOR_CONST_METHOD(return_type, name, ...)
#   define SWIG_EMIT_DIRECTOR_VOID_METHOD(name, ...)
#   define SWIG_EMIT_DIRECTOR_VOID_CONST_METHOD(name, ...)
#   define SWIG_EMIT_DIRECTOR_METHOD_NO_ARGS(return_type, name)
#   define SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(return_type, name)
#   define SWIG_EMIT_DIRECTOR_VOID_METHOD_NO_ARGS(name)
#   define SWIG_EMIT_DIRECTOR_VOID_CONST_METHOD_NO_ARGS(name)
#   define SWIG_EMIT_DIRECTOR_END(name)
#endif

// SWIG_EMIT_SHARED_PTR_REFERENCE_ASSIGN
#if defined(SWIG)
#   define SWIG_EMIT_SHARED_PTR_REFERENCE_ASSIGN(type)                                                                          \
        inline void referenceAssign(const std::shared_ptr< type >& input, std::shared_ptr< type >& output);
#elif defined(OSMAND_SWIG)
#   define SWIG_EMIT_SHARED_PTR_REFERENCE_ASSIGN(type)                                                                          \
        inline void referenceAssign(const std::shared_ptr< type >& input, std::shared_ptr< type >& output)                      \
        {                                                                                                                       \
            output = input;                                                                                                     \
        }
#else
#   define SWIG_EMIT_SHARED_PTR_REFERENCE_ASSIGN(type)
#endif

#endif // !defined(_OSMAND_CORE_COMMON_SWIG_H_)
