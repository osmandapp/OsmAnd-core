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

#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

// Ensure that SKIA is using RGBA order
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkColor.h>
#include <OsmAndCore/restore_internal_warnings.h>
#if !SK_PMCOLOR_BYTE_ORDER(R,G,B,A)
#   error SKIA must be configured to use RGBA color order
#endif

namespace OsmAnd
{
    class ICoreResourcesProvider;
    class IMemoryManager;
    struct MapRendererPerformanceMetrics;

    OSMAND_CORE_API MapRendererPerformanceMetrics& OSMAND_CORE_CALL getPerformanceMetrics();
    OSMAND_CORE_API bool OSMAND_CORE_CALL isPerformanceMetricsEnabled();
    OSMAND_CORE_API void OSMAND_CORE_CALL enablePerformanceMetrics(const bool enable);

    OSMAND_CORE_API int OSMAND_CORE_CALL InitializeCore(
        const std::shared_ptr<const ICoreResourcesProvider>& coreResourcesProvider, 
        const char* appFontsPath = nullptr);
    OSMAND_CORE_API const std::shared_ptr<const ICoreResourcesProvider>& OSMAND_CORE_CALL getCoreResourcesProvider();
    OSMAND_CORE_API const QString& OSMAND_CORE_CALL getFontDirectory();
    OSMAND_CORE_API IMemoryManager* OSMAND_CORE_CALL getMemoryManager();
    OSMAND_CORE_API void OSMAND_CORE_CALL ReleaseCore();
}

#endif // !defined(_OSMAND_CORE_OSMAND_CORE_H_)
