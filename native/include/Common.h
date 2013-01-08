#ifndef _OSMAND_COMMON_H_
#define _OSMAND_COMMON_H_

// Unordered containers
#if defined(ANDROID) || defined(__ANDROID__)
#   include <unordered_map>
#   include <unordered_set>
#   define UNORDERED_NAMESPACE std
#   define UNORDERED_map unordered_map
#   define UNORDERED_set unordered_set
#elif defined(__linux__)
#   include <unordered_map>
#   include <unordered_set>
#   define UNORDERED_NAMESPACE std
#   define UNORDERED_map unordered_map
#   define UNORDERED_set unordered_set
#elif defined(__APPLE__)
#   include <unordered_map>
#   include <unordered_set>
#   define UNORDERED_NAMESPACE std
#   define UNORDERED_map unordered_map
#   define UNORDERED_set unordered_set
#elif defined(_WIN32) && defined(WINAPI_FAMILY) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)
#   include <unordered_map>
#   include <unordered_set>
#   define UNORDERED_NAMESPACE std
#   define UNORDERED_map unordered_map
#   define UNORDERED_set unordered_set
#endif
#define UNORDERED(cls) UNORDERED_NAMESPACE::UNORDERED_##cls

// Smart pointers
#if defined(ANDROID)
#   include <memory>
#   define SHARED_PTR std::shared_ptr
#elif defined(_WIN32) && defined(WINAPI_FAMILY) && (WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP)
#   include <memory>
#   define SHARED_PTR std::shared_ptr
#   define UNIQUE_PTR std::unique_ptr
#endif

#include <string>

namespace OsmAnd
{
    typedef UNORDERED(map)<std::string, float> StringToFloatMap;
    typedef UNORDERED(map)<std::string, std::string> StringToStringMap;
}

#endif // _OSMAND_COMMON_H_
