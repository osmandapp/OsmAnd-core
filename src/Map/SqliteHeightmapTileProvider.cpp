#include "SqliteHeightmapTileProvider.h"
#include "SqliteHeightmapTileProvider_P.h"

#include "MapDataProviderHelpers.h"
#include "MapRenderer.h"

OsmAnd::SqliteHeightmapTileProvider::SqliteHeightmapTileProvider()
    : _p(new SqliteHeightmapTileProvider_P(this))
    , sourcesCollection(nullptr)
    , filesCollection(nullptr)
    , outputTileSize(OsmAnd::MapRenderer::ElevationDataTileSize)
{
}

OsmAnd::SqliteHeightmapTileProvider::SqliteHeightmapTileProvider(
    const std::shared_ptr<const ITileSqliteDatabasesCollection>& sourcesCollection_,
    uint32_t outputTileSize_)
    : _p(new SqliteHeightmapTileProvider_P(this))
    , sourcesCollection(sourcesCollection_)
    , filesCollection(nullptr)
    , outputTileSize(outputTileSize_)
{
}

OsmAnd::SqliteHeightmapTileProvider::SqliteHeightmapTileProvider(
    const std::shared_ptr<const IGeoTiffCollection>& filesCollection_,
    uint32_t outputTileSize_)
    : _p(new SqliteHeightmapTileProvider_P(this))
    , sourcesCollection(nullptr)
    , filesCollection(filesCollection_)
    , outputTileSize(outputTileSize_)
{
}

OsmAnd::SqliteHeightmapTileProvider::SqliteHeightmapTileProvider(
    const std::shared_ptr<const ITileSqliteDatabasesCollection>& sourcesCollection_,
    const std::shared_ptr<const IGeoTiffCollection>& filesCollection_,
    uint32_t outputTileSize_)
    : _p(new SqliteHeightmapTileProvider_P(this))
    , sourcesCollection(sourcesCollection_)
    , filesCollection(filesCollection_)
    , outputTileSize(outputTileSize_)
{
}

OsmAnd::SqliteHeightmapTileProvider::~SqliteHeightmapTileProvider()
{
}

bool OsmAnd::SqliteHeightmapTileProvider::hasDataResources() const
{
    return _p->hasDataResources();
}

OsmAnd::ZoomLevel OsmAnd::SqliteHeightmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::SqliteHeightmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

int OsmAnd::SqliteHeightmapTileProvider::getMaxMissingDataZoomShift() const
{
    return _p->getMaxMissingDataZoomShift(MapRenderer::MaxMissingDataZoomShift);
}

uint32_t OsmAnd::SqliteHeightmapTileProvider::getTileSize() const
{
    return outputTileSize;
}

bool OsmAnd::SqliteHeightmapTileProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::SqliteHeightmapTileProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::SqliteHeightmapTileProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::SqliteHeightmapTileProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}
