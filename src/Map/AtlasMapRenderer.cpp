#include "AtlasMapRenderer.h"

#include "AtlasMapRendererConfiguration.h"
#include "AtlasMapRendererInternalState.h"
#include "AtlasMapRendererSkyStage.h"
#include "AtlasMapRendererRasterMapStage.h"
#include "AtlasMapRendererSymbolsStage.h"
#include "AtlasMapRendererDebugStage.h"
#include "Utilities.h"

OsmAnd::AtlasMapRenderer::AtlasMapRenderer(
    GPUAPI* const gpuAPI_,
    const std::unique_ptr<const MapRendererConfiguration>& baseConfiguration_,
    const std::unique_ptr<const MapRendererDebugSettings>& baseDebugSettings_)
    : MapRenderer(gpuAPI_, baseConfiguration_, baseDebugSettings_)
    , skyStage(_skyStage)
    , rasterMapStage(_rasterMapStage)
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

    const auto zoomDiff = ZoomLevel::MaxZoomLevel - state.zoomBase;

    // Get target tile id
    internalState->targetTileId.x = state.target31.x >> zoomDiff;
    internalState->targetTileId.y = state.target31.y >> zoomDiff;

    // Compute in-tile offset
    PointI targetTile31;
    targetTile31.x = internalState->targetTileId.x << zoomDiff;
    targetTile31.y = internalState->targetTileId.y << zoomDiff;

    const auto tileSize31 = 1u << zoomDiff;
    const auto inTileOffset = state.target31 - targetTile31;
    internalState->targetInTileOffsetN.x = static_cast<float>(inTileOffset.x) / tileSize31;
    internalState->targetInTileOffsetN.y = static_cast<float>(inTileOffset.y) / tileSize31;

    // Sort visible tiles by distance from target
    qSort(internalState->visibleTiles.begin(), internalState->visibleTiles.end(),
        [internalState]
        (const TileId& l, const TileId& r) -> bool
        {
            const auto lx = l.x - internalState->targetTileId.x;
            const auto ly = l.y - internalState->targetTileId.y;

            const auto rx = r.x - internalState->targetTileId.x;
            const auto ry = r.y - internalState->targetTileId.y;

            return (lx*lx + ly*ly) > (rx*rx + ry*ry);
        });

    return true;
}

bool OsmAnd::AtlasMapRenderer::prePrepareFrame()
{
    if (!MapRenderer::prePrepareFrame())
        return false;

    // Get set of tiles that are unique: visible tiles may contain same tiles, but wrapped
    const auto internalState = static_cast<AtlasMapRendererInternalState*>(getInternalStateRef());
    _uniqueTiles.clear();
    for (const auto& tileId : constOf(internalState->visibleTiles))
        _uniqueTiles.insert(Utilities::normalizeTileId(tileId, currentState.zoomBase));

    return true;
}

bool OsmAnd::AtlasMapRenderer::postPrepareFrame()
{
    if (!MapRenderer::postPrepareFrame())
        return false;

    // Notify resources manager about new active zone
    getResources().updateActiveZone(_uniqueTiles, currentState.zoomBase);

    return true;
}

QList<OsmAnd::TileId> OsmAnd::AtlasMapRenderer::getVisibleTiles() const
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

    const bool referenceTileSizeChanged = (current->referenceTileSizeOnScreenInPixels != updated->referenceTileSizeOnScreenInPixels);

    if (referenceTileSizeChanged)
        mask |= enumToBit(ConfigurationChange::ReferenceTileSize);

    return mask;
}

void OsmAnd::AtlasMapRenderer::invalidateCurrentConfiguration(const uint32_t changesMask)
{
    MapRenderer::invalidateCurrentConfiguration(changesMask);
}

bool OsmAnd::AtlasMapRenderer::preInitializeRendering()
{
    bool ok = MapRenderer::preInitializeRendering();
    if (!ok)
        return false;

    _skyStage.reset(createSkyStage());
    if (!_skyStage)
        return false;

    _rasterMapStage.reset(createRasterMapStage());
    if (!_rasterMapStage)
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

    _skyStage->initialize();
    _rasterMapStage->initialize();
    _symbolsStage->initialize();
    _debugStage->initialize();

    return true;
}

bool OsmAnd::AtlasMapRenderer::doReleaseRendering()
{
    bool ok;

    if (_skyStage)
    {
        _skyStage->release();
        _skyStage.reset();
    }

    if (_rasterMapStage)
    {
        _rasterMapStage->release();
        _rasterMapStage.reset();
    }

    if (_symbolsStage)
    {
        _symbolsStage->release();
        _symbolsStage.reset();
    }

    if (_debugStage)
    {
        _debugStage->release();
        _debugStage.reset();
    }

    ok = MapRenderer::doReleaseRendering();
    if (!ok)
        return false;

    return true;
}

QList< std::shared_ptr<const OsmAnd::MapSymbol> > OsmAnd::AtlasMapRenderer::getSymbolsAt(const PointI& screenPoint) const
{
    QList< std::shared_ptr<const MapSymbol> > result;
    if (symbolsStage)
        symbolsStage->queryLastPreparedSymbolsAt(screenPoint, result);
    return result;
}
