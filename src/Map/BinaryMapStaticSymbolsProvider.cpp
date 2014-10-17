#include "BinaryMapStaticSymbolsProvider.h"
#include "BinaryMapStaticSymbolsProvider_P.h"

#include "BinaryMapPrimitivesProvider.h"

OsmAnd::BinaryMapStaticSymbolsProvider::BinaryMapStaticSymbolsProvider(
    const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_,
    const unsigned int referenceTileSizeInPixels_)
    : _p(new BinaryMapStaticSymbolsProvider_P(this))
    , primitivesProvider(primitivesProvider_)
    , referenceTileSizeInPixels(referenceTileSizeInPixels_)
{
}

OsmAnd::BinaryMapStaticSymbolsProvider::~BinaryMapStaticSymbolsProvider()
{
}

bool OsmAnd::BinaryMapStaticSymbolsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledSymbolsProvider::Data>& outTiledData,
    const FilterCallback filterCallback /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, filterCallback, queryController);
    outTiledData = tiledData;
    return result;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapStaticSymbolsProvider::getMinZoom() const
{
    return primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapStaticSymbolsProvider::getMaxZoom() const
{
    return primitivesProvider->getMaxZoom();
}

OsmAnd::BinaryMapStaticSymbolsProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_,
    const std::shared_ptr<const BinaryMapPrimitivesProvider::Data>& binaryMapPrimitivisedData_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledSymbolsProvider::Data(tileId_, zoom_, symbolsGroups_, pRetainableCacheMetadata_)
    , binaryMapPrimitivisedData(binaryMapPrimitivisedData_)
{
}

OsmAnd::BinaryMapStaticSymbolsProvider::Data::~Data()
{
    release();
}
