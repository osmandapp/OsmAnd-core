#include "MapRendererDebugSettings.h"

OsmAnd::MapRendererDebugSettings::MapRendererDebugSettings()
    : debugStageEnabled(false)
    , excludeOnPathSymbolsFromProcessing(false)
    , excludeBillboardSymbolsFromProcessing(false)
    , excludeModel3DSymbolsFromProcessing(false)
    , excludeOnSurfaceSymbolsFromProcessing(false)
    , showSymbolsMarksRejectedByViewpoint(false)
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
    , showBillboardSymbolBBoxes(false)
    , allSymbolsTransparentForIntersectionLookup(false)
    , showTooShortOnPathSymbolsRenderablesPaths(false)
    , showAllPaths(false)
    , rasterLayersOverscaleForbidden(false)
    , rasterLayersUnderscaleForbidden(false)
    , mapLayersBatchingForbidden(false)
    , disableJunkResourcesCleanup(false)
    , disableNeededResourcesRequests(false)
    , disableSymbolsFastCheckByFrustum(false)
    , disableSkyStage(false)
    , disableMapLayersStage(false)
    , disableSymbolsStage(false)
    , disable3DMapObjectsStage(false)
    , enableMap3dObjectsDebugInfo(false)
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
    other.excludeModel3DSymbolsFromProcessing = excludeModel3DSymbolsFromProcessing;
    other.excludeOnSurfaceSymbolsFromProcessing = excludeOnSurfaceSymbolsFromProcessing;
    other.showSymbolsMarksRejectedByViewpoint = showSymbolsMarksRejectedByViewpoint;
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
    other.showBillboardSymbolBBoxes = showBillboardSymbolBBoxes;
    other.allSymbolsTransparentForIntersectionLookup = allSymbolsTransparentForIntersectionLookup;
    other.showTooShortOnPathSymbolsRenderablesPaths = showTooShortOnPathSymbolsRenderablesPaths;
    other.showAllPaths = showAllPaths;
    other.rasterLayersOverscaleForbidden = rasterLayersOverscaleForbidden;
    other.rasterLayersUnderscaleForbidden = rasterLayersUnderscaleForbidden;
    other.mapLayersBatchingForbidden = mapLayersBatchingForbidden;
    other.disableJunkResourcesCleanup = disableJunkResourcesCleanup;
    other.disableNeededResourcesRequests = disableNeededResourcesRequests;
    other.disableSymbolsFastCheckByFrustum = disableSymbolsFastCheckByFrustum;
    other.disableSkyStage = disableSkyStage;
    other.disableMapLayersStage = disableMapLayersStage;
    other.disableSymbolsStage = disableSymbolsStage;
    other.enableMap3dObjectsDebugInfo = enableMap3dObjectsDebugInfo;
    other.disable3DMapObjectsStage = disable3DMapObjectsStage;
}

std::shared_ptr<OsmAnd::MapRendererDebugSettings> OsmAnd::MapRendererDebugSettings::createCopy() const
{
    std::shared_ptr<MapRendererDebugSettings> copy(new MapRendererDebugSettings());
    copyTo(*copy);
    return copy;
}
