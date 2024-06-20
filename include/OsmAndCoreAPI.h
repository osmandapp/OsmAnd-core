#pragma once

#if defined(OSMAND_CORE_EXPORTS)
#   if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_API \
            __declspec(dllexport)
#       define OSMAND_CORE_CALL \
            __stdcall
#   else
#       if __GNUC__ >= 4
#           define OSMAND_CORE_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_API
#       endif
#       define OSMAND_CORE_CALL
#   endif
#elif !defined(OSMAND_CORE_STATIC)
#   if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_API \
            __declspec(dllimport)
#       define OSMAND_CORE_API_FUNCTION \
            extern OSMAND_CORE_API
#       define OSMAND_CORE_CALL \
            __stdcall
#   else
#       if __GNUC__ >= 4
#           define OSMAND_CORE_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_API
#       endif
#       define OSMAND_CORE_CALL
#   endif
#else
#   define OSMAND_CORE_API
#   define OSMAND_CORE_CALL
#endif

#if !defined(OSMAND_DEBUG)
#   if defined(DEBUG) || defined(_DEBUG)
#       define OSMAND_DEBUG 1
#   endif
#endif
#if !defined(OSMAND_DEBUG)
#   define OSMAND_DEBUG 0
#endif


