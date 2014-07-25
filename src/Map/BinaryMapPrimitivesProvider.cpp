#include "BinaryMapPrimitivesProvider.h"
#include "BinaryMapPrimitivesProvider_P.h"

#include "BinaryMapDataProvider.h"

OsmAnd::BinaryMapPrimitivesProvider::BinaryMapPrimitivesProvider(
    const std::shared_ptr<BinaryMapDataProvider>& dataProvider_,
    const std::shared_ptr<Primitiviser>& primitiviser_,
    const unsigned int tileSize_ /*= 256*/)
    : IMapTiledDataProvider(DataType::BinaryMapPrimitivesTile)
    , _p(new BinaryMapPrimitivesProvider_P(this))
    , dataProvider(dataProvider_)
    , primitiviser(primitiviser_)
    , tileSize(tileSize_)
{
}

OsmAnd::BinaryMapPrimitivesProvider::~BinaryMapPrimitivesProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapPrimitivesProvider::getMinZoom() const
{
    return dataProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapPrimitivesProvider::getMaxZoom() const
{
    return dataProvider->getMaxZoom();
}

bool OsmAnd::BinaryMapPrimitivesProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, nullptr, queryController);
}

bool OsmAnd::BinaryMapPrimitivesProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    BinaryMapPrimitivesProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
}

OsmAnd::BinaryMapPrimitivesTile::BinaryMapPrimitivesTile(
    const std::shared_ptr<const BinaryMapDataTile> dataTile_,
    const std::shared_ptr<const Primitiviser::PrimitivisedArea> primitivisedArea_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapTiledData(DataType::BinaryMapPrimitivesTile, tileId_, zoom_)
    , _p(new BinaryMapPrimitivesTile_P(this))
    , dataTile(dataTile_)
    , primitivisedArea(primitivisedArea_)
{
}

OsmAnd::BinaryMapPrimitivesTile::~BinaryMapPrimitivesTile()
{
    _p->cleanup();
}

void OsmAnd::BinaryMapPrimitivesTile::releaseConsumableContent()
{
    // There's no consumable data

    MapTiledData::releaseConsumableContent();
}
