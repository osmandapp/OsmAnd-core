#include "MapRendererDebugSettings.h"

OsmAnd::MapRendererDebugSettings::MapRendererDebugSettings()
    : debugStageEnabled(false)
    , excludeOnPathSymbols(false)
    , excludeBillboardSymbols(false)
    , excludeOnSurfaceSymbols(false)
    , skipSymbolsIntersectionCheck(false)
    , showSymbolsBBoxesRejectedByIntersectionCheck(false)
    , showSymbolsBBoxesAcceptedByIntersectionCheck(false)
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
}

std::shared_ptr<OsmAnd::MapRendererDebugSettings> OsmAnd::MapRendererDebugSettings::createCopy() const
{
    std::shared_ptr<MapRendererDebugSettings> copy(new MapRendererDebugSettings());
    copyTo(*copy);
    return copy;
}
