#include "MapRendererDebugSettings.h"

OsmAnd::MapRendererDebugSettings::MapRendererDebugSettings()
    : debugStageEnabled(false)
    , excludeOnPathSymbols(false)
    , excludeBillboardSymbols(false)
    , excludeOnSurfaceSymbols(false)
    , skipSymbolsIntersectionCheck(false)
    , showSymbolsBBoxesAcceptedByIntersectionCheck(false)
    , showSymbolsBBoxesRejectedByIntersectionCheck(false)
    , skipMinDistanceToSameContentFromOtherSymbolCheck(false)
    , showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck(false)
    , showOnPathSubpaths(false)
    , showOnPath2dSymbolGlyphDetails(false)
    , showOnPath3dSymbolGlyphDetails(false)
    , allTransparentForIntersectionLookup(false)
{
}

OsmAnd::MapRendererDebugSettings::~MapRendererDebugSettings()
{
}

void OsmAnd::MapRendererDebugSettings::copyTo(MapRendererDebugSettings& other) const
{
    other.debugStageEnabled = debugStageEnabled;
    other.excludeOnPathSymbols = excludeOnPathSymbols;
    other.excludeBillboardSymbols = excludeBillboardSymbols;
    other.excludeOnSurfaceSymbols = excludeOnSurfaceSymbols;
    other.skipSymbolsIntersectionCheck = skipSymbolsIntersectionCheck;
    other.showSymbolsBBoxesRejectedByIntersectionCheck = showSymbolsBBoxesRejectedByIntersectionCheck;
    other.showSymbolsBBoxesAcceptedByIntersectionCheck = showSymbolsBBoxesAcceptedByIntersectionCheck;
    other.skipMinDistanceToSameContentFromOtherSymbolCheck = skipMinDistanceToSameContentFromOtherSymbolCheck;
    other.showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck = showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck;
    other.showOnPathSubpaths = showOnPathSubpaths;
    other.showOnPath2dSymbolGlyphDetails = showOnPath2dSymbolGlyphDetails;
    other.showOnPath3dSymbolGlyphDetails = showOnPath3dSymbolGlyphDetails;
    other.allTransparentForIntersectionLookup = allTransparentForIntersectionLookup;
}

std::shared_ptr<OsmAnd::MapRendererDebugSettings> OsmAnd::MapRendererDebugSettings::createCopy() const
{
    std::shared_ptr<MapRendererDebugSettings> copy(new MapRendererDebugSettings());
    copyTo(*copy);
    return copy;
}
