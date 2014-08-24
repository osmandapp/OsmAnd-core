#ifndef _OSMAND_CORE_TOOLS_H_
#define _OSMAND_CORE_TOOLS_H_

#if defined(OSMAND_CORE_TOOLS_EXPORTS)
#   if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_TOOLS_API \
            __declspec(dllexport)
#       define OSMAND_CORE_TOOLS_CALL \
            __stdcall
#   else
#       if __GNUC__ >= 4
#           define OSMAND_CORE_TOOLS_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_TOOLS_API
#       endif
#       define OSMAND_CORE_TOOLS_CALL
#   endif
#elif !defined(OSMAND_CORE_TOOLS_STATIC)
#   if defined(_WIN32) || defined(__CYGWIN__)
#       define OSMAND_CORE_TOOLS_API \
            __declspec(dllimport)
#       define OSMAND_CORE_TOOLS_API_FUNCTION \
            extern OSMAND_CORE_TOOLS_API
#       define OSMAND_CORE_TOOLS_CALL \
            __stdcall
#   else
#       if __GNUC__ >= 4
#           define OSMAND_CORE_TOOLS_API \
                __attribute__ ((visibility ("default")))
#       else
#           define OSMAND_CORE_TOOLS_API
#       endif
#       define OSMAND_CORE_TOOLS_CALL
#   endif
#else
#   define OSMAND_CORE_TOOLS_API
#   define OSMAND_CORE_TOOLS_CALL
#endif

#endif // !defined(_OSMAND_CORE_TOOLS_H_)
