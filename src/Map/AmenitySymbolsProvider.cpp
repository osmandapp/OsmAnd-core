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
    const std::shared_ptr<IAmenityIconProvider>& amenityIconProvider_ /*= std::make_shared<CoreResourcesAmenityIconProvider>()*/,
    const int baseOrder_ /*= 10000*/,
    const uint32_t cacheSize_ /*= 0*/,
    const ExternalAmenitiesProvider externalAmenitiesProvider_ /*= nullptr*/)
    : _p(new AmenitySymbolsProvider_P(this))
    , obfsCollection(obfsCollection_)
    , displayDensityFactor(displayDensityFactor_)
    , referenceTileSizeOnScreenInPixels(referenceTileSizeOnScreenInPixels_)
    , categoriesFilter(categoriesFilter_)
    , amentitiesFilter(amentitiesFilter_)
    , amenityIconProvider(amenityIconProvider_)
    , baseOrder(baseOrder_)
    , cacheSize(cacheSize_)
    , cache(std::make_shared<Cache>(cacheSize_))
    , externalAmenitiesProvider(externalAmenitiesProvider_)
{
}

OsmAnd::AmenitySymbolsProvider::~AmenitySymbolsProvider()
{
}

void OsmAnd::AmenitySymbolsProvider::invalidateTiles()
{
    _p->invalidateTiles();
}

void OsmAnd::AmenitySymbolsProvider::setDataObtainedHandler(
    const std::function<void(const TileId tileId, const ZoomLevel zoom)>& handler)
{
    _p->setDataObtainedHandler(handler);
}

OsmAnd::ZoomLevel OsmAnd::AmenitySymbolsProvider::getMinZoom() const
{
    return ZoomLevel6;
}

OsmAnd::ZoomLevel OsmAnd::AmenitySymbolsProvider::getMaxZoom() const
{
    return MaxZoomLevel;
}

// Сommented as fix of the issue: https://github.com/osmandapp/OsmAnd-iOS/issues/3690
//int OsmAnd::AmenitySymbolsProvider::getMaxMissingDataUnderZoomShift() const
//{
//    return 0;
//}

bool OsmAnd::AmenitySymbolsProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::AmenitySymbolsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    const auto requestSucceeded = _p->obtainData(request, outData, pOutMetric);
    if (requestSucceeded && outData)
    {
        const auto& tiledRequest = MapDataProviderHelpers::castRequest<AmenitySymbolsProvider::Request>(request);
        _p->notifyDataObtained(tiledRequest.tileId, tiledRequest.zoom);
    }

    return requestSucceeded;
}

bool OsmAnd::AmenitySymbolsProvider::supportsNaturalObtainDataAsync() const
{
    return true;
}

bool OsmAnd::AmenitySymbolsProvider::waitForLoading() const
{
    return false;
}

void OsmAnd::AmenitySymbolsProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
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

OsmAnd::AmenitySymbolsProvider::Cache::Entry::Entry()
{
}

OsmAnd::AmenitySymbolsProvider::Cache::Entry::Entry(const QList<std::shared_ptr<const Amenity>>& amenities_)
    : amenities(amenities_)
{
}

OsmAnd::AmenitySymbolsProvider::Cache::Cache(const uint32_t capacity_)
    : _nextGeneration(0)
    , capacity(capacity_)
{
}

OsmAnd::AmenitySymbolsProvider::Cache::~Cache()
{
}

void OsmAnd::AmenitySymbolsProvider::Cache::setDataChangedHandler(const std::function<void()>& handler)
{
    QWriteLocker scopedLocker(&_lock);
    _dataChangedHandler = handler;
}

uint32_t OsmAnd::AmenitySymbolsProvider::Cache::getSize() const
{
    QReadLocker scopedLocker(&_lock);

    uint32_t size = 0;
    for (const auto& storage : constOf(_storage))
        size += static_cast<uint32_t>(storage.size());
    return size;
}

bool OsmAnd::AmenitySymbolsProvider::Cache::contains(const TileId tileId, const ZoomLevel zoom) const
{
    QReadLocker scopedLocker(&_lock);

    const auto& storage = _storage[zoom];
    return storage.contains(tileId);
}

bool OsmAnd::AmenitySymbolsProvider::Cache::obtainEntry(
    const TileId tileId,
    const ZoomLevel zoom,
    Entry& outEntry)
{
    QWriteLocker scopedLocker(&_lock);

    auto& storage = _storage[zoom];
    auto itEntry = storage.find(tileId);
    if (itEntry == storage.end())
        return false;

    auto& cacheEntry = itEntry.value();
    cacheEntry.generation = ++_nextGeneration;
    _lru.push_back({ tileId, zoom, cacheEntry.generation });
    outEntry = Entry(cacheEntry.amenities);
    return true;
}

bool OsmAnd::AmenitySymbolsProvider::Cache::obtainAmenities(
    const TileId tileId,
    const ZoomLevel zoom,
    QList<std::shared_ptr<const Amenity>>& outAmenities)
{
    Entry entry;
    if (!obtainEntry(tileId, zoom, entry))
        return false;

    outAmenities = entry.amenities;
    return true;
}

void OsmAnd::AmenitySymbolsProvider::Cache::put(
    const TileId tileId,
    const ZoomLevel zoom,
    const QList<std::shared_ptr<const Amenity>>& amenities)
{
    if (capacity == 0)
        return;

    std::function<void()> handler;
    {
        QWriteLocker scopedLocker(&_lock);

        auto& storage = _storage[zoom];
        auto& entry = storage[tileId];
        entry.amenities = amenities;
        entry.generation = ++_nextGeneration;
        _lru.push_back({ tileId, zoom, entry.generation });

        shrinkToCapacityUnlocked();
        handler = _dataChangedHandler;
    }
    
    if (handler)
        handler();
}

void OsmAnd::AmenitySymbolsProvider::Cache::remove(const TileId tileId, const ZoomLevel zoom)
{
    std::function<void()> handler;
    {
        QWriteLocker scopedLocker(&_lock);
        removeEntryUnlocked(tileId, zoom);
        handler = _dataChangedHandler;
    }
    
    if (handler)
        handler();
}

void OsmAnd::AmenitySymbolsProvider::Cache::clear()
{
    std::function<void()> handler;
    {
        QWriteLocker scopedLocker(&_lock);

        for (auto& storage : _storage)
            storage.clear();
        _lru.clear();
        handler = _dataChangedHandler;
    }
    
    if (handler)
        handler();
}

void OsmAnd::AmenitySymbolsProvider::Cache::removeEntryUnlocked(const TileId tileId, const ZoomLevel zoom)
{
    auto& storage = _storage[zoom];
    auto itEntry = storage.find(tileId);
    if (itEntry == storage.end())
        return;

    storage.erase(itEntry);
}

void OsmAnd::AmenitySymbolsProvider::Cache::shrinkToCapacityUnlocked()
{
    uint32_t size = 0;
    for (const auto& storage : constOf(_storage))
        size += static_cast<uint32_t>(storage.size());

    while (size > capacity && !_lru.isEmpty())
    {
        const auto lruEntry = _lru.front();
        _lru.pop_front();

        auto& storage = _storage[lruEntry.zoom];
        const auto itEntry = storage.find(lruEntry.tileId);
        if (itEntry == storage.end())
            continue;
        if (itEntry.value().generation != lruEntry.generation)
            continue;

        storage.erase(itEntry);
        --size;
    }

    if (capacity == 0)
        _lru.clear();
    else if (static_cast<uint32_t>(_lru.size()) > size * 2u + capacity)
    {
        QList<LruEntry> compactedLru;
        compactedLru.reserve(static_cast<int>(size));
        for (const auto& lruEntry : constOf(_lru))
        {
            const auto& storage = _storage[lruEntry.zoom];
            const auto itEntry = storage.constFind(lruEntry.tileId);
            if (itEntry == storage.cend())
                continue;
            if (itEntry.value().generation != lruEntry.generation)
                continue;

            compactedLru.push_back(lruEntry);
        }
        _lru.swap(compactedLru);
    }
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
