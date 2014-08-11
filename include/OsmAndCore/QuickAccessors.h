#ifndef _OSMAND_CORE_QUICK_ACCESSORS_H
#define _OSMAND_CORE_QUICK_ACCESSORS_H

#include <type_traits>

#define RO_ACCESSOR_EX(name, var_name)                                                                      \
    inline auto name() const                                                                                \
        -> typename std::add_lvalue_reference< typename std::add_const<decltype(var_name)>::type >::type    \
    {                                                                                                       \
        return var_name;                                                                                    \
    }

#define RO_ACCESSOR(name) RO_ACCESSOR_EX(name, _##name)

#define RW_ACCESSOR_EX(name, var_name)                                                                      \
    inline auto name() const                                                                                \
        -> typename std::add_lvalue_reference< typename std::add_const<decltype(var_name)>::type >::type    \
    {                                                                                                       \
        return var_name;                                                                                    \
    }                                                                                                       \
    inline auto name()                                                                                      \
        -> typename std::add_lvalue_reference<decltype(var_name)>::type                                     \
    {                                                                                                       \
        return var_name;                                                                                    \
    }

#define RW_ACCESSOR(name) RW_ACCESSOR_EX(name, _##name)

#endif // !defined(_OSMAND_CORE_QUICK_ACCESSORS_H)
