#include "AtlasMapRenderer.h"

#include "AtlasMapRendererConfiguration.h"
#include "AtlasMapRendererInternalState.h"
#include "AtlasMapRendererSkyStage.h"
#include "AtlasMapRendererMapLayersStage.h"
#include "AtlasMapRendererSymbolsStage.h"
#include "AtlasMapRendererDebugStage.h"
#include "Utilities.h"

OsmAnd::AtlasMapRenderer::AtlasMapRenderer(
    GPUAPI* const gpuAPI_,
    const std::unique_ptr<const MapRendererConfiguration>& baseConfiguration_,
    const std::unique_ptr<const MapRendererDebugSettings>& baseDebugSettings_)
    : MapRenderer(gpuAPI_, baseConfiguration_, baseDebugSettings_)
    , skyStage(_skyStage)
    , mapLayersStage(_mapLayersStage)
    , symbolsStage(_symbolsStage)
    , debugStage(_debugStage)
{
}

OsmAnd::AtlasMapRenderer::~AtlasMapRenderer()
{
}

bool OsmAnd::AtlasMapRenderer::updateInternalState(
    MapRendererInternalState& outInternalState_,
    const MapRendererState& state,
    const MapRendererConfiguration& configuration) const
{
    const auto internalState = static_cast<AtlasMapRendererInternalState*>(&outInternalState_);

    const auto zoomLevelDiff = ZoomLevel::MaxZoomLevel - state.zoomLevel;

    // Get target tile id
    internalState->targetTileId.x = state.target31.x >> zoomLevelDiff;
    internalState->targetTileId.y = state.target31.y >> zoomLevelDiff;

    // Compute in-tile offset
    PointI targetTile31;
    targetTile31.x = internalState->targetTileId.x << zoomLevelDiff;
    targetTile31.y = internalState->targetTileId.y << zoomLevelDiff;

    const auto tileSize31 = 1u << zoomLevelDiff;
    const auto inTileOffset = state.target31 - targetTile31;
    internalState->targetInTileOffsetN.x = static_cast<float>(inTileOffset.x) / tileSize31;
    internalState->targetInTileOffsetN.y = static_cast<float>(inTileOffset.y) / tileSize31;

    return true;
}

bool OsmAnd::AtlasMapRenderer::prePrepareFrame()
{
    // const auto internalState = static_cast<AtlasMapRendererInternalState*>(getInternalStateRef());

    if (!MapRenderer::prePrepareFrame())
        return false;

    return true;
}

bool OsmAnd::AtlasMapRenderer::postPrepareFrame()
{
    const auto internalState = static_cast<AtlasMapRendererInternalState*>(getInternalStateRef());

    if (!MapRenderer::postPrepareFrame())
        return false;

    // Notify resources manager about new active zone
    getResources().updateActiveZone(internalState->targetTileId, internalState->uniqueTiles, currentState.zoomLevel);

    return true;
}

QVector<OsmAnd::TileId> OsmAnd::AtlasMapRenderer::getVisibleTiles() const
{
    QReadLocker scopedLocker(&_internalStateLock);
    const auto internalState = static_cast<const AtlasMapRendererInternalState*>(getInternalStateRef());
    
    return detachedOf(internalState->visibleTiles);
}

unsigned int OsmAnd::AtlasMapRenderer::getVisibleTilesCount() const
{
    QReadLocker scopedLocker(&_internalStateLock);
    const auto internalState = static_cast<const AtlasMapRendererInternalState*>(getInternalStateRef());

    return internalState->visibleTiles.size();
}

uint32_t OsmAnd::AtlasMapRenderer::getConfigurationChangeMask(
    const std::shared_ptr<const MapRendererConfiguration>& current_,
    const std::shared_ptr<const MapRendererConfiguration>& updated_) const
{
    auto mask = MapRenderer::getConfigurationChangeMask(current_, updated_);

    const auto current = std::dynamic_pointer_cast<const AtlasMapRendererConfiguration>(current_);
    const auto updated = std::dynamic_pointer_cast<const AtlasMapRendererConfiguration>(updated_);

    const bool referenceTileSizeChanged = !qFuzzyCompare(
        current->referenceTileSizeOnScreenInPixels,
        updated->referenceTileSizeOnScreenInPixels);

    if (referenceTileSizeChanged)
        mask |= enumToBit(ConfigurationChange::ReferenceTileSize);

    return mask;
}

void OsmAnd::AtlasMapRenderer::validateConfigurationChange(const MapRenderer::ConfigurationChange& change_)
{
    const auto change = static_cast<ConfigurationChange>(change_);

    bool invalidateSymbols = false;
    invalidateSymbols = invalidateSymbols || (change == ConfigurationChange::ReferenceTileSize);

    if (invalidateSymbols)
        getResources().invalidateResourcesOfType(MapRendererResourceType::Symbols);

    MapRenderer::validateConfigurationChange(static_cast<MapRenderer::ConfigurationChange>(change));
}

bool OsmAnd::AtlasMapRenderer::preInitializeRendering()
{
    if (!MapRenderer::preInitializeRendering())
        return false;

    _skyStage.reset(createSkyStage());
    if (!_skyStage)
        return false;

    _mapLayersStage.reset(createMapLayersStage());
    if (!_mapLayersStage)
        return false;

    _symbolsStage.reset(createSymbolsStage());
    if (!_symbolsStage)
        return false;

    _debugStage.reset(createDebugStage());
    if (!_debugStage)
        return false;

    return true;
}

bool OsmAnd::AtlasMapRenderer::doInitializeRendering()
{
    bool ok = MapRenderer::doInitializeRendering();
    if (!ok)
        return false;

    if (!_skyStage->initialize())
        ok = false;

    if (!_mapLayersStage->initialize())
        ok = false;

    if (!_symbolsStage->initialize())
        ok = false;

    if (!_debugStage->initialize())
        ok = false;

    return ok;
}

bool OsmAnd::AtlasMapRenderer::doReleaseRendering(const bool gpuContextLost)
{
    bool ok = true;

    if (_skyStage)
    {
        if (!_skyStage->release(gpuContextLost))
            ok = false;
        _skyStage.reset();
    }

    if (_mapLayersStage)
    {
        if (!_mapLayersStage->release(gpuContextLost))
            ok = false;
        _mapLayersStage.reset();
    }

    if (_symbolsStage)
    {
        if (!_symbolsStage->release(gpuContextLost))
            ok = false;
        _symbolsStage.reset();
    }

    if (_debugStage)
    {
        if (!_debugStage->release(gpuContextLost))
            ok = false;
        _debugStage.reset();
    }

    if (!MapRenderer::doReleaseRendering(gpuContextLost))
        ok = false;

    return ok;
}

QList<OsmAnd::IMapRenderer::MapSymbolInformation> OsmAnd::AtlasMapRenderer::getSymbolsAt(const PointI& screenPoint) const
{
    QList<MapSymbolInformation> result;
    if (symbolsStage)
        symbolsStage->queryLastVisibleSymbolsAt(screenPoint, result);
    return result;
}

QList<OsmAnd::IMapRenderer::MapSymbolInformation> OsmAnd::AtlasMapRenderer::getSymbolsIn(
    const AreaI& screenArea,
    const bool strict /*= false*/) const
{
    QList<MapSymbolInformation> result;
    if (symbolsStage)
        symbolsStage->queryLastVisibleSymbolsIn(screenArea, result, strict);
    return result;
}
