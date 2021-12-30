#include "AmenitySymbolsProvider.h"
#include "AmenitySymbolsProvider_P.h"

#include "MapDataProviderHelpers.h"
#include "Amenity.h"

OsmAnd::AmenitySymbolsProvider::AmenitySymbolsProvider(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const float displayDensityFactor_,
    const float referenceTileSizeOnScreenInPixels_,
    const QHash<QString, QStringList>* const categoriesFilter_ /*= nullptr*/,
    const ObfPoiSectionReader::VisitorFunction amentitiesFilter_ /*= nullptr*/,
    const std::shared_ptr<const IAmenityIconProvider>& amenityIconProvider_ /*= std::make_shared<CoreResourcesAmenityIconProvider>()*/,
    const int baseOrder_ /*= 10000*/)
    : _p(new AmenitySymbolsProvider_P(this))
    , obfsCollection(obfsCollection_)
    , displayDensityFactor(displayDensityFactor_)
    , referenceTileSizeOnScreenInPixels(referenceTileSizeOnScreenInPixels_)
    , categoriesFilter(categoriesFilter_)
    , amentitiesFilter(amentitiesFilter_)
    , amenityIconProvider(amenityIconProvider_)
    , baseOrder(baseOrder_)
{
}

OsmAnd::AmenitySymbolsProvider::~AmenitySymbolsProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::AmenitySymbolsProvider::getMinZoom() const
{
    return ZoomLevel6;
}

OsmAnd::ZoomLevel OsmAnd::AmenitySymbolsProvider::getMaxZoom() const
{
    return MaxZoomLevel;
}

bool OsmAnd::AmenitySymbolsProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::AmenitySymbolsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::AmenitySymbolsProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::AmenitySymbolsProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}

OsmAnd::AmenitySymbolsProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledSymbolsProvider::Data(tileId_, zoom_, symbolsGroups_, pRetainableCacheMetadata_)
{
}

OsmAnd::AmenitySymbolsProvider::Data::~Data()
{
    release();
}

OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup::AmenitySymbolsGroup(
    const std::shared_ptr<const Amenity>& amenity_)
    : amenity(amenity_)
{
}

OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup::~AmenitySymbolsGroup()
{
}

bool OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup::obtainSharingKey(SharingKey& outKey) const
{
    return false;
}

bool OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup::obtainSortingKey(SortingKey& outKey) const
{
    outKey = static_cast<SharingKey>(amenity->id);
    return true;
}

QString OsmAnd::AmenitySymbolsProvider::AmenitySymbolsGroup::toString() const
{
    return {};
}
