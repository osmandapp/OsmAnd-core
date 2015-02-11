#include "MapRendererDebugSettings.h"

OsmAnd::MapRendererDebugSettings::MapRendererDebugSettings()
    : debugStageEnabled(false)
    , excludeOnPathSymbolsFromProcessing(false)
    , excludeBillboardSymbolsFromProcessing(false)
    , excludeOnSurfaceSymbolsFromProcessing(false)
    , skipSymbolsIntersectionCheck(false)
    , showSymbolsBBoxesAcceptedByIntersectionCheck(false)
    , showSymbolsBBoxesRejectedByIntersectionCheck(false)
    , skipSymbolsMinDistanceToSameContentFromOtherSymbolCheck(false)
    , showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck(false)
    , showSymbolsCheckBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck(false)
    , skipSymbolsPresentationModeCheck(false)
    , showSymbolsBBoxesRejectedByPresentationMode(false)
    , showOnPathSymbolsRenderablesPaths(false)
    , showOnPath2dSymbolGlyphDetails(false)
    , showOnPath3dSymbolGlyphDetails(false)
    , allSymbolsTransparentForIntersectionLookup(false)
    , showTooShortOnPathSymbolsRenderablesPaths(false)
    , showAllPaths(false)
    , rasterLayersOverscaleForbidden(false)
    , rasterLayersUnderscaleForbidden(false)
    , mapLayersBatchingForbidden(false)
{
}

OsmAnd::MapRendererDebugSettings::~MapRendererDebugSettings()
{
}

void OsmAnd::MapRendererDebugSettings::copyTo(MapRendererDebugSettings& other) const
{
    other.debugStageEnabled = debugStageEnabled;
    other.excludeOnPathSymbolsFromProcessing = excludeOnPathSymbolsFromProcessing;
    other.excludeBillboardSymbolsFromProcessing = excludeBillboardSymbolsFromProcessing;
    other.excludeOnSurfaceSymbolsFromProcessing = excludeOnSurfaceSymbolsFromProcessing;
    other.skipSymbolsIntersectionCheck = skipSymbolsIntersectionCheck;
    other.showSymbolsBBoxesRejectedByIntersectionCheck = showSymbolsBBoxesRejectedByIntersectionCheck;
    other.showSymbolsBBoxesAcceptedByIntersectionCheck = showSymbolsBBoxesAcceptedByIntersectionCheck;
    other.skipSymbolsMinDistanceToSameContentFromOtherSymbolCheck = skipSymbolsMinDistanceToSameContentFromOtherSymbolCheck;
    other.showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck = showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck;
    other.showSymbolsCheckBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck = showSymbolsCheckBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck;
    other.skipSymbolsPresentationModeCheck = skipSymbolsPresentationModeCheck;
    other.showSymbolsBBoxesRejectedByPresentationMode = showSymbolsBBoxesRejectedByPresentationMode;
    other.showOnPathSymbolsRenderablesPaths = showOnPathSymbolsRenderablesPaths;
    other.showOnPath2dSymbolGlyphDetails = showOnPath2dSymbolGlyphDetails;
    other.showOnPath3dSymbolGlyphDetails = showOnPath3dSymbolGlyphDetails;
    other.allSymbolsTransparentForIntersectionLookup = allSymbolsTransparentForIntersectionLookup;
    other.showTooShortOnPathSymbolsRenderablesPaths = showTooShortOnPathSymbolsRenderablesPaths;
    other.showAllPaths = showAllPaths;
    other.rasterLayersOverscaleForbidden = rasterLayersOverscaleForbidden;
    other.rasterLayersUnderscaleForbidden = rasterLayersUnderscaleForbidden;
    other.mapLayersBatchingForbidden = mapLayersBatchingForbidden;
}

std::shared_ptr<OsmAnd::MapRendererDebugSettings> OsmAnd::MapRendererDebugSettings::createCopy() const
{
    std::shared_ptr<MapRendererDebugSettings> copy(new MapRendererDebugSettings());
    copyTo(*copy);
    return copy;
}
