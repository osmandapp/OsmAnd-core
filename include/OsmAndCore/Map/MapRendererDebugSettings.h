#ifndef _OSMAND_CORE_MAP_RENDERER_DEBUG_SETTINGS_H_
#define _OSMAND_CORE_MAP_RENDERER_DEBUG_SETTINGS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    struct OSMAND_CORE_API MapRendererDebugSettings
    {
        MapRendererDebugSettings();
        virtual ~MapRendererDebugSettings();

        bool debugStageEnabled;
        bool excludeOnPathSymbolsFromProcessing;
        bool excludeBillboardSymbolsFromProcessing;
        bool excludeOnSurfaceSymbolsFromProcessing;
        bool skipSymbolsIntersectionCheck;
        bool showSymbolsBBoxesAcceptedByIntersectionCheck;
        bool showSymbolsBBoxesRejectedByIntersectionCheck;
        bool skipSymbolsMinDistanceToSameContentFromOtherSymbolCheck;
        bool showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck;
        bool showSymbolsCheckBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck;
        bool skipSymbolsPresentationModeCheck;
        bool showSymbolsBBoxesRejectedByPresentationMode;
        bool showOnPathSymbolsRenderablesPaths;
        bool showOnPath2dSymbolGlyphDetails;
        bool showOnPath3dSymbolGlyphDetails;
        bool allSymbolsTransparentForIntersectionLookup;
        bool showTooShortOnPathSymbolsRenderablesPaths;
        bool showAllPaths;
        bool rasterLayersOverscaleForbidden;
        bool rasterLayersUnderscaleForbidden;
        bool mapLayersBatchingForbidden;
        bool disableJunkResourcesCleanup;
        bool disableNeededResourcesRequests;
        bool disableSymbolsFastCheckByFrustum;
        bool disableSkyStage;
        bool disableMapLayersStage;
        bool disableSymbolsStage;
        bool disable3DModelsStage;
        
        virtual void copyTo(MapRendererDebugSettings& other) const;
        virtual std::shared_ptr<MapRendererDebugSettings> createCopy() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_DEBUG_SETTINGS_H_)
