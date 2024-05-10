#ifndef _OSMAND_CORE_OSMAND_CORE_H_
#define _OSMAND_CORE_OSMAND_CORE_H_

#include "OsmAndCoreAPI.h"

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

    OSMAND_CORE_API bool OSMAND_CORE_CALL InitializeCore(
        const std::shared_ptr<const ICoreResourcesProvider>& coreResourcesProvider,
        const char* appFontsPath = nullptr);
    OSMAND_CORE_API const std::shared_ptr<const ICoreResourcesProvider>& OSMAND_CORE_CALL getCoreResourcesProvider();
    OSMAND_CORE_API const QString& OSMAND_CORE_CALL getFontDirectory();
    OSMAND_CORE_API IMemoryManager* OSMAND_CORE_CALL getMemoryManager();
    OSMAND_CORE_API void OSMAND_CORE_CALL ReleaseCore();
}

#endif // !defined(_OSMAND_CORE_OSMAND_CORE_H_)
