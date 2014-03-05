#ifndef _OSMAND_CORE_UTILS_H_
#define _OSMAND_CORE_UTILS_H_

#if defined(OSMAND_CORE_UTILS_EXPORTS)
#   if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_UTILS_API \
            __declspec(dllexport)
#       define OSMAND_CORE_UTILS_CALL \
            __stdcall
#   else
#       if __GNUC__ >= 4
#           define OSMAND_CORE_UTILS_API \
            __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_UTILS_API
#       endif
#       define OSMAND_CORE_UTILS_CALL
#   endif
#elif !defined(OSMAND_CORE_UTILS_STATIC)
#if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_UTILS_API \
            __declspec(dllimport)
#       define OSMAND_CORE_UTILS_CALL \
            __stdcall
#   else
#       if __GNUC__ >= 4
#           define OSMAND_CORE_UTILS_API \
            __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_UTILS_API
#       endif
#       define OSMAND_CORE_UTILS_CALL
#   endif
#else
#   define OSMAND_CORE_UTILS_CALL
#   define OSMAND_CORE_UTILS_API
#endif

#endif // !defined(_OSMAND_CORE_UTILS_H_)
