#include "BinaryMapPrimitivesProvider.h"
#include "BinaryMapPrimitivesProvider_P.h"

#include "BinaryMapDataProvider.h"

OsmAnd::BinaryMapPrimitivesProvider::BinaryMapPrimitivesProvider(
    const std::shared_ptr<BinaryMapDataProvider>& dataProvider_,
    const std::shared_ptr<Primitiviser>& primitiviser_,
    const unsigned int tileSize_ /*= 256*/)
    : _p(new BinaryMapPrimitivesProvider_P(this))
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
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, nullptr, queryController);
    outTiledData = tiledData;
    return result;
}

bool OsmAnd::BinaryMapPrimitivesProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<Data>& outTiledData,
    BinaryMapPrimitivesProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
}

OsmAnd::BinaryMapPrimitivesProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const std::shared_ptr<const BinaryMapDataProvider::Data>& binaryMapData_,
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivisedArea_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledDataProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , binaryMapData(binaryMapData_)
    , primitivisedArea(primitivisedArea_)
{
}

OsmAnd::BinaryMapPrimitivesProvider::Data::~Data()
{
    release();
}
