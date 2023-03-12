#include "MapObjectsSymbolsProvider.h"
#include "MapObjectsSymbolsProvider_P.h"

#include "MapDataProviderHelpers.h"
#include "MapPrimitivesProvider.h"
#include "SymbolRasterizer.h"

OsmAnd::MapObjectsSymbolsProvider::MapObjectsSymbolsProvider(
    const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_,
    const float referenceTileSizeOnScreenInPixels_,
    const std::shared_ptr<const SymbolRasterizer>& symbolRasterizer_ /*= std::shared_ptr<const SymbolRasterizer>(new SymbolRasterizer())*/,
    const bool forceObtainDataAsync_ /* = false */)
    : _p(new MapObjectsSymbolsProvider_P(this))
    , primitivesProvider(primitivesProvider_)
    , referenceTileSizeOnScreenInPixels(referenceTileSizeOnScreenInPixels_)
    , symbolRasterizer(symbolRasterizer_ ? symbolRasterizer_ : std::shared_ptr<const SymbolRasterizer>(new SymbolRasterizer()))
    , forceObtainDataAsync(forceObtainDataAsync_)
{
}

OsmAnd::MapObjectsSymbolsProvider::~MapObjectsSymbolsProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::MapObjectsSymbolsProvider::getMinZoom() const
{
    return primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapObjectsSymbolsProvider::getMaxZoom() const
{
    return primitivesProvider->getMaxZoom();
}


bool OsmAnd::MapObjectsSymbolsProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::MapObjectsSymbolsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::MapObjectsSymbolsProvider::supportsNaturalObtainDataAsync() const
{
    return forceObtainDataAsync;
}

void OsmAnd::MapObjectsSymbolsProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
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
