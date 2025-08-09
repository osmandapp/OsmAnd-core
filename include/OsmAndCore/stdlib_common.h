#ifndef _OSMAND_CORE_STDLIB_COMMON_H_
#define _OSMAND_CORE_STDLIB_COMMON_H_

#include <cstdint>
#include <cinttypes>
#include <memory>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <type_traits>

namespace std
{
    template<class CONTAINER_TYPE, class ITEM_TYPE>
    inline auto find(CONTAINER_TYPE& container, const ITEM_TYPE& itemValue) -> decltype(begin(container))
    {
        return find(std::begin(container), std::end(container), itemValue);
    }

    template<class CONTAINER_TYPE, class ITEM_TYPE>
    inline bool contains(const CONTAINER_TYPE& container, const ITEM_TYPE& itemValue)
    {
        const auto itEnd = std::end(container);
        return find(std::begin(container), std::end(container), itemValue) != itEnd;
    }

    template<class CONTAINER_TYPE, class PREDICATE>
    inline auto find_if(CONTAINER_TYPE& container, PREDICATE predicate) -> decltype(begin(container))
    {
        return find_if(std::begin(container), std::end(container), predicate);
    }

    template<class CONTAINER_TYPE, class PREDICATE>
    inline bool contains_if(const CONTAINER_TYPE& container, PREDICATE predicate)
    {
        const auto itEnd = std::end(container);
        return find_if(std::begin(container), std::end(container), predicate) != itEnd;
    }

    template<class CONTAINER_TYPE, class PREDICATE>
    inline bool any_of(const CONTAINER_TYPE& container, PREDICATE predicate)
    {
        return any_of(std::begin(container), std::end(container), predicate);
    }

    template<class CONTAINER_TYPE, class T>
    inline T accumulate(const CONTAINER_TYPE& container, T initialValue)
    {
        return accumulate(std::begin(container), std::end(container), initialValue);
    }

    template<class CONTAINER_TYPE, class T, class OPERATION>
    inline T accumulate(const CONTAINER_TYPE& container, T initialValue, OPERATION operation)
    {
        return accumulate(std::begin(container), std::end(container), initialValue, operation);
    }

    template<class CONTAINER_TYPE>
    inline void sort(CONTAINER_TYPE& container)
    {
     //   return sort(std::begin(container), std::end(container));
	return sort(container.begin(), container.end());
    }

    template<class CONTAINER_TYPE, class PREDICATE>
    inline void sort(CONTAINER_TYPE& container, PREDICATE predicate)
    {
     //   return sort(std::begin(container), std::end(container), predicate);
	return sort(container.begin(), container.end(), predicate);
    }
}

#endif // !defined(_OSMAND_CORE_STDLIB_COMMON_H_)
