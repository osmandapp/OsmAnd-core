#include "MapObjectsSymbolsProvider.h"
#include "MapObjectsSymbolsProvider_P.h"

#include "MapPrimitivesProvider.h"
#include "SymbolRasterizer.h"

OsmAnd::MapObjectsSymbolsProvider::MapObjectsSymbolsProvider(
    const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_,
    const float referenceTileSizeOnScreenInPixels_,
    const std::shared_ptr<const SymbolRasterizer>& symbolRasterizer_ /*= std::shared_ptr<const SymbolRasterizer>(new SymbolRasterizer())*/)
    : _p(new MapObjectsSymbolsProvider_P(this))
    , primitivesProvider(primitivesProvider_)
    , referenceTileSizeOnScreenInPixels(referenceTileSizeOnScreenInPixels_)
    , symbolRasterizer(symbolRasterizer_)
{
}

OsmAnd::MapObjectsSymbolsProvider::~MapObjectsSymbolsProvider()
{
}

bool OsmAnd::MapObjectsSymbolsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledSymbolsProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/,
    const FilterCallback filterCallback /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();

    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, queryController, filterCallback);
    outTiledData = tiledData;

    return result;
}

OsmAnd::IMapDataProvider::SourceType OsmAnd::MapObjectsSymbolsProvider::getSourceType() const
{
    const auto underlyingSourceType = primitivesProvider->getSourceType();

    switch (underlyingSourceType)
    {
        case IMapDataProvider::SourceType::LocalDirect:
        case IMapDataProvider::SourceType::LocalGenerated:
            return IMapDataProvider::SourceType::LocalGenerated;

        case IMapDataProvider::SourceType::NetworkDirect:
        case IMapDataProvider::SourceType::NetworkGenerated:
            return IMapDataProvider::SourceType::NetworkGenerated;

        case IMapDataProvider::SourceType::MiscDirect:
        case IMapDataProvider::SourceType::MiscGenerated:
        default:
            return IMapDataProvider::SourceType::MiscGenerated;
    }
}

OsmAnd::ZoomLevel OsmAnd::MapObjectsSymbolsProvider::getMinZoom() const
{
    return primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapObjectsSymbolsProvider::getMaxZoom() const
{
    return primitivesProvider->getMaxZoom();
}

OsmAnd::MapObjectsSymbolsProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& binaryMapPrimitivisedData_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledSymbolsProvider::Data(tileId_, zoom_, symbolsGroups_, pRetainableCacheMetadata_)
    , mapPrimitivesData(binaryMapPrimitivisedData_)
{
}

OsmAnd::MapObjectsSymbolsProvider::Data::~Data()
{
    release();
}

OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup::MapObjectSymbolsGroup(
    const std::shared_ptr<const MapObject>& mapObject_)
    : mapObject(mapObject_)
{
}

OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup::~MapObjectSymbolsGroup()
{
}

bool OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup::obtainSharingKey(SharingKey& outKey) const
{
    MapObject::SharingKey mapObjectSharingKey;
    if (!mapObject->obtainSharingKey(mapObjectSharingKey))
        return false;

    outKey = static_cast<SharingKey>(mapObjectSharingKey);

    return true;
}

bool OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup::obtainSortingKey(SortingKey& outKey) const
{
    MapObject::SortingKey mapObjectSortingKey;
    if (!mapObject->obtainSortingKey(mapObjectSortingKey))
        return false;

    outKey = static_cast<SortingKey>(mapObjectSortingKey);

    return true;
}

QString OsmAnd::MapObjectsSymbolsProvider::MapObjectSymbolsGroup::toString() const
{
    return mapObject->toString();
}
