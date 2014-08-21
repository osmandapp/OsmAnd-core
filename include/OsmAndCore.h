#ifndef _OSMAND_CORE_OSMAND_CORE_H_
#define _OSMAND_CORE_OSMAND_CORE_H_

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

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

// Ensure that SKIA is using RGBA order
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkColor.h>
#include <OsmAndCore/restore_internal_warnings.h>
#if defined(SK_CPU_LENDIAN)
#   if (24 != SK_A32_SHIFT) || ( 0 != SK_R32_SHIFT) || \
       ( 8 != SK_G32_SHIFT) || (16 != SK_B32_SHIFT)
#       error SKIA must be configured to use RGBA color order
#   endif
#elif defined(SK_CPU_BENDIAN)
#   if ( 0 != SK_A32_SHIFT) || (24 != SK_R32_SHIFT) || \
       (16 != SK_G32_SHIFT) || ( 8 != SK_B32_SHIFT)
#       error SKIA must be configured to use RGBA color order
#   endif
#endif

#include <memory>

namespace OsmAnd
{
    OSMAND_CORE_API void OSMAND_CORE_CALL InitializeCore();
    OSMAND_CORE_API void OSMAND_CORE_CALL ReleaseCore();
}

#endif // !defined(_OSMAND_CORE_OSMAND_CORE_H_)
