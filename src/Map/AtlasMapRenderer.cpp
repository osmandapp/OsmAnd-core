#include "AtlasMapRenderer.h"

#include "AtlasMapRendererConfiguration.h"
#include "AtlasMapRendererInternalState.h"
#include "AtlasMapRendererSkyStage.h"
#include "AtlasMapRendererMapLayersStage.h"
#include "AtlasMapRendererSymbolsStage.h"
#include "AtlasMapRendererDebugStage.h"
#include "AtlasMapRendererMap3DObjectsStage.h"
#include "Utilities.h"

OsmAnd::AtlasMapRenderer::AtlasMapRenderer(
    GPUAPI* const gpuAPI_,
    const std::unique_ptr<const MapRendererConfiguration>& baseConfiguration_,
    const std::unique_ptr<const MapRendererDebugSettings>& baseDebugSettings_)
    : MapRenderer(gpuAPI_, baseConfiguration_, baseDebugSettings_)
    , skyStage(_skyStage)
    , mapLayersStage(_mapLayersStage)
    , symbolsStage(_symbolsStage)
    , map3DObjectsStage(_map3DObjectsStage)
    , debugStage(_debugStage)
{
}

OsmAnd::AtlasMapRenderer::~AtlasMapRenderer()
{
}

bool OsmAnd::AtlasMapRenderer::updateInternalState(
    MapRendererInternalState& outInternalState_,
    const MapRendererState& state,
    const MapRendererConfiguration& configuration,
    const CalculationSteps neededSteps /* = Complete */) const
{
    const auto internalState = static_cast<AtlasMapRendererInternalState*>(&outInternalState_); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

    internalState->targetTileId = Utilities::getTileId(state.target31, state.zoomLevel, &internalState->targetInTileOffsetN);

    return true;
}

bool OsmAnd::AtlasMapRenderer::prePrepareFrame()
{
    const auto internalState = static_cast<AtlasMapRendererInternalState*>(getInternalStateRef());

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
    getResources().updateActiveZone(internalState->uniqueTiles, internalState->uniqueTilesTargets,
        internalState->visibleTilesSet, internalState->extraDetailedTiles, internalState->zoomLevelOffset);

    return true;
}

QVector<OsmAnd::TileId> OsmAnd::AtlasMapRenderer::getVisibleTiles() const
{
    QReadLocker scopedLocker(&_internalStateLock);
    const auto internalState = static_cast<const AtlasMapRendererInternalState*>(getInternalStateRef());

    const auto tiles = internalState->visibleTiles.cend();
    return detachedOf(tiles != internalState->visibleTiles.cbegin() ?
        (tiles - 1).value() : QVector<OsmAnd::TileId>());
}

unsigned int OsmAnd::AtlasMapRenderer::getVisibleTilesCount() const
{
    QReadLocker scopedLocker(&_internalStateLock);
    const auto internalState = static_cast<const AtlasMapRendererInternalState*>(getInternalStateRef());
    int tilesCount = 0;
    const auto tiles = internalState->visibleTiles.cend();
    if (tiles != internalState->visibleTiles.cbegin())
        tilesCount = (tiles - 1)->size();

    return tilesCount;
}

unsigned int OsmAnd::AtlasMapRenderer::getAllTilesCount() const
{
    QReadLocker scopedLocker(&_internalStateLock);
    const auto internalState = static_cast<const AtlasMapRendererInternalState*>(getInternalStateRef());
    return internalState->visibleTilesCount;
}

unsigned int OsmAnd::AtlasMapRenderer::getDetailLevelsCount() const
{
    QReadLocker scopedLocker(&_internalStateLock);
    const auto internalState = static_cast<const AtlasMapRendererInternalState*>(getInternalStateRef());

    return internalState->visibleTiles.size();
}

OsmAnd::LatLon OsmAnd::AtlasMapRenderer::getCameraCoordinates() const
{
    QReadLocker scopedLocker(&_internalStateLock);
    const auto internalState = static_cast<const AtlasMapRendererInternalState*>(getInternalStateRef());

    return internalState->cameraCoordinates;
}

double OsmAnd::AtlasMapRenderer::getCameraHeight() const
{
    QReadLocker scopedLocker(&_internalStateLock);
    const auto internalState = static_cast<const AtlasMapRendererInternalState*>(getInternalStateRef());

    return internalState->distanceFromCameraToGroundInMeters;
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

    _map3DObjectsStage.reset(createMap3DObjectsStage());
    if (!_map3DObjectsStage)
        return false;

    _symbolsStage.reset(createSymbolsStage());
    if (!_symbolsStage)
        return false;

    _debugStage.reset(createDebugStage());
    if (!_debugStage)
        return false;

    return true;
}

bool OsmAnd::AtlasMapRenderer::doInitializeRendering(bool reinitialize)
{
    bool ok = MapRenderer::doInitializeRendering(reinitialize);
    if (!ok)
        return false;

    if (!_skyStage->initialize())
        ok = false;

    if (!_mapLayersStage->initialize())
        ok = false;

    if (!_symbolsStage->initialize())
        ok = false;

    if (!_map3DObjectsStage->initialize())
        ok = false;

    if (!_debugStage->initialize())
        ok = false;

    return ok;
}

bool OsmAnd::AtlasMapRenderer::doReleaseRendering(bool gpuContextLost)
{
    bool ok = true;

    if (_skyStage)
    {
        if (!_skyStage->release(gpuContextLost))
            ok = false;
    }

    if (_mapLayersStage)
    {
        if (!_mapLayersStage->release(gpuContextLost))
            ok = false;
    }

    if (_map3DObjectsStage)
    {
        if (!_map3DObjectsStage->release(gpuContextLost))
            ok = false;
    }

    if (_symbolsStage)
    {
        if (!_symbolsStage->release(gpuContextLost))
            ok = false;
    }

    if (_debugStage)
    {
        if (!_debugStage->release(gpuContextLost))
            ok = false;
    }

    if (!MapRenderer::doReleaseRendering(gpuContextLost))
        ok = false;

    return ok;
}

bool OsmAnd::AtlasMapRenderer::postReleaseRendering(bool gpuContextLost)
{
    bool ok = true;

    if (_skyStage)
        _skyStage.reset();

    if (_mapLayersStage)
        _mapLayersStage.reset();

    if (_map3DObjectsStage)
        _map3DObjectsStage.reset();

    if (_symbolsStage)
        _symbolsStage.reset();

    if (_debugStage)
        _debugStage.reset();

    if (!MapRenderer::postReleaseRendering(gpuContextLost))
        ok = false;

    return ok;
}

int OsmAnd::AtlasMapRenderer::getTileSize3D() const
{
    return TileSize3D;
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
