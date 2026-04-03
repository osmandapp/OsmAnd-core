#include "AmenitySymbolsProvider_P.h"
#include "AmenitySymbolsProvider.h"

#include "ICoreResourcesProvider.h"
#include "ObfDataInterface.h"
#include "MapDataProviderHelpers.h"
#include "BillboardRasterMapSymbol.h"
#include "SkiaUtilities.h"
#include "Utilities.h"
#include "MapSymbolIntersectionClassesRegistry.h"
#include "ObfPoiSectionInfo.h"
#include <algorithm>

static const int kSkipTilesZoom = 13;
static const int kSkipTileDivider = 16;
static const int kTilePointsLimit = 25;
static const int kStartZoom = 5;
static const int kStartZoomRouteTrack = 11;
static const QString kRouteArticle = QStringLiteral("route_article");
static const QString kRouteArticlePoint = QStringLiteral("route_article_point");
static const QString kRouteTrack = QStringLiteral("route_track");
static const QString kRoutesPrefix = QStringLiteral("routes_");
static const QString kRouteBboxRadius = QStringLiteral("route_bbox_radius");
static const QString kRouteId = QStringLiteral("route_id");
OsmAnd::AmenitySymbolsProvider_P::AmenitySymbolsProvider_P(AmenitySymbolsProvider* owner_)
: owner(owner_)
, textRasterizer(TextRasterizer::getDefault())
{
}

OsmAnd::AmenitySymbolsProvider_P::~AmenitySymbolsProvider_P()
{
}

void OsmAnd::AmenitySymbolsProvider_P::setDataObtainedHandler(
    const std::function<void(const TileId tileId, const ZoomLevel zoom)>& handler)
{
    QWriteLocker scopedLocker(&_dataObtainedHandlerLock);
    _dataObtainedHandler = handler;
}

void OsmAnd::AmenitySymbolsProvider_P::notifyDataObtained(const TileId tileId, const ZoomLevel zoom)
{
    std::function<void(const TileId tileId, const ZoomLevel zoom)> handler;
    {
        QReadLocker scopedLocker(&_dataObtainedHandlerLock);
        handler = _dataObtainedHandler;
    }

    if (handler)
        handler(tileId, zoom);
}

uint32_t OsmAnd::AmenitySymbolsProvider_P::getTileId(const AreaI& bbox31, const PointI& point)
{
    const auto divX = bbox31.width() / kSkipTileDivider;
    const auto divY = bbox31.height() / kSkipTileDivider;
    const auto tx = static_cast<uint32_t>(floor((point.x - bbox31.left()) / divX));
    const auto ty = static_cast<uint32_t>(floor((point.y - bbox31.top()) / divY));
    return tx + ty * kSkipTileDivider;
}

OsmAnd::AreaD OsmAnd::AmenitySymbolsProvider_P::calculateRect(double x, double y, double width, double height)
{
    double left = x - width / 2.0;
    double top = y - height / 2.0;
    double right = left + width;
    double bottom = top + height;
    return AreaD(top, left, bottom, right);
}

bool OsmAnd::AmenitySymbolsProvider_P::intersects(SymbolsQuadTree& boundIntersections, double x, double y, double width, double height)
{
    QList<AreaD> result;
    const auto visibleRect = calculateRect(x, y, width, height);
    boundIntersections.query(visibleRect, result);
    for (const auto &r : result)
        if (r.intersects(visibleRect))
            return true;

    boundIntersections.insert(visibleRect, visibleRect);
    return false;
}

namespace
{
    struct AmenityCacheEntry
    {
        std::shared_ptr<const OsmAnd::Amenity> amenity;
        bool isRouteArticle = false;
        bool isRouteTrack = false;
    };

    bool hasNonEmptyAmenityValue(const QVariant& value)
    {
        switch (value.type())
        {
            case QVariant::String:
                return !value.toString().isEmpty();
            case QVariant::ByteArray:
                return !value.toByteArray().isEmpty();
            case QVariant::Int:
            case QVariant::UInt:
                return true;
            default:
                return value.isValid() && !value.isNull();
        }
    }

    bool hasRouteTrackMetadata(const std::shared_ptr<const OsmAnd::Amenity>& amenity)
    {
        const auto routeBboxRadiusIt = amenity->decodedValuesOverride.constFind(kRouteBboxRadius);
        const auto routeIdIt = amenity->decodedValuesOverride.constFind(kRouteId);
        if (!amenity->decodedValuesOverride.isEmpty())
        {
            return routeBboxRadiusIt != amenity->decodedValuesOverride.cend()
                && !routeBboxRadiusIt.value().isEmpty()
                && routeIdIt != amenity->decodedValuesOverride.cend()
                && !routeIdIt.value().isEmpty();
        }

        if (!amenity->obfSection)
            return false;

        const auto sectionSubtypes = amenity->obfSection->getSubtypes();
        if (!sectionSubtypes)
            return false;

        bool hasGeometry = false;
        bool hasRouteId = false;
        for (const auto& valueEntry : OsmAnd::rangeOf(OsmAnd::constOf(amenity->values)))
        {
            const auto subtypeIndex = valueEntry.key();
            if (subtypeIndex >= sectionSubtypes->subtypes.size())
                continue;

            const auto& subtype = sectionSubtypes->subtypes[subtypeIndex];
            if (subtype->tagName == kRouteBboxRadius)
                hasGeometry = hasNonEmptyAmenityValue(valueEntry.value());
            else if (subtype->tagName == kRouteId)
                hasRouteId = hasNonEmptyAmenityValue(valueEntry.value());

            if (hasGeometry && hasRouteId)
                return true;
        }

        return false;
    }

    AmenityCacheEntry extractAmenityCacheEntry(const std::shared_ptr<const OsmAnd::Amenity>& amenity)
    {
        AmenityCacheEntry entry;
        entry.amenity = amenity;
        entry.isRouteArticle = amenity->subType == kRouteArticlePoint || amenity->subType == kRouteArticle;

        const auto hasRouteTrackSubtype = amenity->subType.startsWith(kRoutesPrefix) || amenity->subType == kRouteTrack;
        if (hasRouteTrackSubtype)
            entry.isRouteTrack = hasRouteTrackMetadata(amenity);
        return entry;
    }

    bool shouldDraw(const AmenityCacheEntry& entry, const OsmAnd::ZoomLevel zoom)
    {
        if (entry.isRouteArticle)
            return zoom >= kStartZoom;
        if (entry.isRouteTrack)
            return zoom >= kStartZoomRouteTrack;
        return zoom >= kStartZoom;
    }
}

bool OsmAnd::AmenitySymbolsProvider_P::shouldDraw(const std::shared_ptr<const Amenity>& amenity, const ZoomLevel zoom) const
{
    return ::shouldDraw(extractAmenityCacheEntry(amenity), zoom);
}

void OsmAnd::AmenitySymbolsProvider_P::invalidateTiles()
{
    owner->cache->clear();

    QList<std::shared_ptr<TileEntry>> tileEntries;
    _tileReferences.obtainEntries(&tileEntries);
    for (const auto& tileEntry : constOf(tileEntries))
    {
        if (tileEntry->getState() != TileState::Loading)
            continue;

        tileEntry->setState(TileState::Cancelled);
        QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
        tileEntry->loadedCondition.wakeAll();
    }

    _tileReferences.removeAllEntries();
}

bool OsmAnd::AmenitySymbolsProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<AmenitySymbolsProvider::Request>(request_);
    const auto& queryController = request.queryController;

    if (pOutMetric)
        pOutMetric->reset();

    if (request.zoom > owner->getMaxZoom() || request.zoom < owner->getMinZoom())
    {
        outData.reset();
        return true;
    }

    auto& mapSymbolIntersectionClassesRegistry = MapSymbolIntersectionClassesRegistry::globalInstance();
    const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);
    const auto stepX = tileBBox31.width() / 4;
    const auto stepY = tileBBox31.height() / 4;
    auto extendedTileBBox31 = tileBBox31.getEnlargedBy(stepY, stepX, stepY, stepX);
    const auto hasMapStateScale = request.mapState.windowSize.x > 0 && request.mapState.windowSize.y > 0;
    const auto scaleZoom = hasMapStateScale ? request.mapState.zoomLevel : request.zoom;
    const auto tileSize31 = Utilities::getPowZoom(static_cast<float>(ZoomLevel::MaxZoomLevel - scaleZoom));
    const auto tileOnScreenScale = hasMapStateScale
        ? static_cast<double>(request.mapState.visualZoom) * qMax(1e-6, 1.0 + static_cast<double>(request.mapState.visualZoomShift))
        : 1.0;
    const auto from31toPixelsScale =
        static_cast<double>(owner->referenceTileSizeOnScreenInPixels) * tileOnScreenScale / tileSize31;
    const float displayDensityFactor = owner->displayDensityFactor;
    const auto requestedZoom = request.zoom;
    AmenitySymbolsProvider::ExternalAmenitiesResponse externalAmenitiesResponse;
    const auto isCancelled =
        [queryController]() -> bool
        {
            return queryController && queryController->isAborted();
        };
    if (owner->externalAmenitiesProvider)
    {
        if (!owner->externalAmenitiesProvider(request.tileId, request.zoom, isCancelled, externalAmenitiesResponse))
        {
            outData.reset();
            return false;
        }
    }
    const auto createSymbolsGroups =
        [this,
         queryController,
         requestedZoom,
         displayDensityFactor,
         from31toPixelsScale,
         &tileBBox31,
         &mapSymbolIntersectionClassesRegistry]
        (const QList<std::shared_ptr<const Amenity>>& amenities,
         QList<std::shared_ptr<MapSymbolsGroup>>& outMapSymbolsGroups) -> bool
        {
            SymbolsQuadTree boundIntersections(AreaD(tileBBox31).getEnlargedBy(tileBBox31.width() / 2), 4);

            for (const auto& amenity : constOf(amenities))
            {
                if (queryController && queryController->isAborted())
                    return false;

                auto icon = owner->amenityIconProvider->getIcon(amenity, ZoomLevel16, false);
                if (!icon)
                    continue;

                const auto pos31 = amenity->position31;
                const auto iconSize31 = icon->dimensions().width() / from31toPixelsScale;
                const auto intersectsWithOtherSymbols =
                    intersects(boundIntersections, pos31.x, pos31.y, iconSize31, iconSize31);

                int order = owner->baseOrder;
                if (intersectsWithOtherSymbols)
                {
                    icon = owner->amenityIconProvider->getIcon(amenity, ZoomLevel10, false);
                    order += 2;
                }

                const auto mapSymbolsGroup = std::make_shared<AmenitySymbolsGroup>(amenity);

                const auto mapSymbol = std::make_shared<BillboardRasterMapSymbol>(mapSymbolsGroup);
                mapSymbol->subsection = owner->subsection;
                mapSymbol->order = order;
                mapSymbol->image = icon;
                mapSymbol->size = PointI(icon->width(), icon->height());
                mapSymbol->languageId = LanguageId::Invariant;
                mapSymbol->position31 = pos31;
                mapSymbolsGroup->symbols.push_back(mapSymbol);

                outMapSymbolsGroups.push_back(mapSymbolsGroup);

                const auto caption = owner->amenityIconProvider->getCaption(amenity, requestedZoom);
                if (!intersectsWithOtherSymbols && !caption.isEmpty())
                {
                    const auto textStyle = owner->amenityIconProvider->getCaptionStyle(amenity, requestedZoom);
                    const auto textImage = textRasterizer->rasterize(caption, textStyle);
                    if (textImage)
                    {
                        const auto mapSymbolCaption = std::make_shared<BillboardRasterMapSymbol>(mapSymbolsGroup);
                        mapSymbolCaption->subsection = owner->subsection;
                        mapSymbolCaption->order = order + 1;
                        mapSymbolCaption->image = textImage;
                        mapSymbolCaption->contentClass = OsmAnd::MapSymbol::ContentClass::Caption;
                        mapSymbolCaption->intersectsWithClasses.insert(
                            mapSymbolIntersectionClassesRegistry.getOrRegisterClassIdByName(QStringLiteral("text_layer_caption")));
                        mapSymbolCaption->setOffset(
                            PointI(0, icon->height() / 2 + textImage->height() / 2 + 2 * displayDensityFactor));
                        mapSymbolCaption->size = PointI(textImage->width(), textImage->height());
                        mapSymbolCaption->languageId = LanguageId::Invariant;
                        mapSymbolCaption->position31 = pos31;
                        mapSymbolsGroup->symbols.push_back(mapSymbolCaption);
                    }
                }
            }

            return true;
        };

    const auto createDataFromAmenities =
        [this, &request, &createSymbolsGroups]
        (const QList<std::shared_ptr<const Amenity>>& amenities,
         const std::shared_ptr<TileEntry>& tileEntry,
         std::shared_ptr<AmenitySymbolsProvider::Data>& outTiledData) -> bool
        {
            QList<std::shared_ptr<MapSymbolsGroup>> mapSymbolsGroups;
            if (!createSymbolsGroups(amenities, mapSymbolsGroups))
                return false;

            outTiledData.reset(new AmenitySymbolsProvider::Data(
                request.tileId,
                request.zoom,
                mapSymbolsGroups,
                tileEntry ? new RetainableCacheMetadata(tileEntry) : nullptr));
            return true;
        };

    QList<std::shared_ptr<const Amenity>> cachedAmenities;
    if (owner->cache->obtainAmenities(request.tileId, request.zoom, cachedAmenities))
    {
        std::shared_ptr<AmenitySymbolsProvider::Data> cachedData;
        if (!createDataFromAmenities(cachedAmenities, nullptr, cachedData))
            return false;

        outData = cachedData;
        return true;
    }

    if (queryController && queryController->isAborted())
        return false;

    const auto cancelTileLoading =
        [this, &request]
        (const std::shared_ptr<TileEntry>& tileEntry)
        {
            tileEntry->setState(TileState::Cancelled);
            {
                QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
                tileEntry->loadedCondition.wakeAll();
            }
            _tileReferences.removeEntry(
                request.tileId,
                request.zoom,
                [tileEntry]
                (const std::shared_ptr<TileEntry>& entry) -> bool
                {
                    return entry == tileEntry;
                });
        };

    std::shared_ptr<TileEntry> tileEntry;
    for (;;)
    {
        _tileReferences.obtainOrAllocateEntry(tileEntry, request.tileId, request.zoom,
            []
            (const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TileEntry*
            {
                return new TileEntry(collection, tileId, zoom);
            });

        if (tileEntry->setStateIf(TileState::Undefined, TileState::Loading))
            break;

        if (tileEntry->getState() == TileState::Loading)
        {
            QReadLocker scopedLocker(&tileEntry->loadedConditionLock);
            while (tileEntry->getState() == TileState::Loading)
                REPEAT_UNTIL(tileEntry->loadedCondition.wait(&tileEntry->loadedConditionLock));
        }

        if (owner->cacheSize > 0)
        {
            if (owner->cache->obtainAmenities(request.tileId, request.zoom, cachedAmenities))
            {
                std::shared_ptr<AmenitySymbolsProvider::Data> cachedData;
                if (!createDataFromAmenities(cachedAmenities, nullptr, cachedData))
                    return false;

                outData = cachedData;
                return true;
            }

            _tileReferences.removeEntry(
                request.tileId,
                request.zoom,
                [tileEntry]
                (const std::shared_ptr<TileEntry>& entry) -> bool
                {
                    return entry == tileEntry;
                });
            tileEntry.reset();

            if (queryController && queryController->isAborted())
                return false;

            continue;
        }

        if (!tileEntry->dataIsPresent)
        {
            outData.reset();
            return true;
        }

        outData = tileEntry->dataWeakRef.lock();
        if (outData)
            return true;

        _tileReferences.removeEntry(
            request.tileId,
            request.zoom,
            [tileEntry]
            (const std::shared_ptr<TileEntry>& entry) -> bool
            {
                return entry == tileEntry;
            });
        tileEntry.reset();

        if (queryController && queryController->isAborted())
            return false;
    }

    if (queryController && queryController->isAborted())
    {
        cancelTileLoading(tileEntry);
        tileEntry.reset();
        return false;
    }

    QSet<uint32_t> offlineSkippedTiles;
    QSet<uint64_t> offlineSearchedIds;
    const bool zoomFilter = request.zoom <= kSkipTilesZoom;
    const auto dataInterface = owner->obfsCollection->obtainDataInterface(
        &extendedTileBBox31,
        request.zoom,
        request.zoom,
        ObfDataTypesMask().set(ObfDataType::POI));

    const auto processAmenityEntry =
        [this,
         queryController,
         requestedZoom,
         zoomFilter,
         &extendedTileBBox31,
         &tileBBox31]
        (const std::shared_ptr<const Amenity>& amenity,
         QSet<uint32_t>& skippedTiles,
         QSet<uint64_t>& searchedIds,
         QList<AmenityCacheEntry>& outAmenityEntries) -> bool
        {
            if (queryController && queryController->isAborted())
                return false;

            const auto amenityId = static_cast<uint64_t>(amenity->id.id);
            if (owner->amentitiesFilter && !owner->amentitiesFilter(amenity))
            {
                searchedIds.insert(amenityId);
                return true;
            }

            if (searchedIds.contains(amenityId))
                return true;

            const auto entry = extractAmenityCacheEntry(amenity);
            if (!::shouldDraw(entry, requestedZoom))
            {
                searchedIds.insert(amenityId);
                return true;
            }

            const auto pos31 = amenity->position31;
            if (zoomFilter)
            {
                const auto skipTileId = getTileId(extendedTileBBox31, pos31);
                if (skippedTiles.contains(skipTileId))
                    return true;

                skippedTiles.insert(skipTileId);
            }

            if (!tileBBox31.contains(pos31))
                return true;

            searchedIds.insert(amenityId);
            outAmenityEntries.push_back(entry);
            return true;
        };
    QList<AmenityCacheEntry> offlineAmenityEntries;
    const auto visitorFunction =
        [&processAmenityEntry,
         &offlineSkippedTiles,
         &offlineSearchedIds,
         &offlineAmenityEntries]
        (const std::shared_ptr<const Amenity>& amenity) -> bool
        {
            return processAmenityEntry(amenity, offlineSkippedTiles, offlineSearchedIds, offlineAmenityEntries);
        };

    dataInterface->loadAmenities(
        nullptr,
        &extendedTileBBox31,
        nullptr,
        request.zoom,
        owner->categoriesFilter.getValuePtrOrNullptr(),
        owner->poiAdditionalFilter.getValuePtrOrNullptr(),
        visitorFunction,
        nullptr);

    if (queryController && queryController->isAborted())
    {
        cancelTileLoading(tileEntry);
        tileEntry.reset();
        return false;
    }

    QList<AmenityCacheEntry> externalAmenityEntries;
    {
        QSet<uint32_t> skippedTiles;
        QSet<uint64_t> searchedIds;
        for (const auto& amenity : constOf(externalAmenitiesResponse.amenities))
        {
            if (!processAmenityEntry(amenity, skippedTiles, searchedIds, externalAmenityEntries))
            {
                cancelTileLoading(tileEntry);
                tileEntry.reset();
                return false;
            }
        }
    }

    QList<AmenityCacheEntry> cachedAmenityEntries;
    QSet<uint64_t> cachedAmenityIds;
    cachedAmenityEntries.reserve(externalAmenityEntries.size() + offlineAmenityEntries.size());
    for (const auto& amenityEntry : constOf(externalAmenityEntries))
    {
        cachedAmenityEntries.push_back(amenityEntry);
        cachedAmenityIds.insert(static_cast<uint64_t>(amenityEntry.amenity->id.id));
    }
    for (const auto& amenityEntry : constOf(offlineAmenityEntries))
    {
        const auto amenityId = static_cast<uint64_t>(amenityEntry.amenity->id.id);
        if (cachedAmenityIds.contains(amenityId))
            continue;

        cachedAmenityEntries.push_back(amenityEntry);
        cachedAmenityIds.insert(amenityId);
    }

    cachedAmenities.clear();
    cachedAmenities.reserve(cachedAmenityEntries.size());
    for (const auto& amenityEntry : constOf(cachedAmenityEntries))
        cachedAmenities.push_back(amenityEntry.amenity);

    while (cachedAmenities.size() > kTilePointsLimit)
        cachedAmenities.removeLast();

    if (owner->cacheSize > 0)
        owner->cache->put(request.tileId, request.zoom, cachedAmenities);

    std::shared_ptr<AmenitySymbolsProvider::Data> newData;
    if (!createDataFromAmenities(cachedAmenities, owner->cacheSize == 0 ? tileEntry : nullptr, newData))
    {
        cancelTileLoading(tileEntry);
        tileEntry.reset();
        return false;
    }

    outData = newData;

    tileEntry->dataIsPresent = true;
    tileEntry->dataWeakRef = owner->cacheSize == 0 ? newData : std::weak_ptr<AmenitySymbolsProvider::Data>();
    tileEntry->setState(TileState::Loaded);
    {
        QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
        tileEntry->loadedCondition.wakeAll();
    }

    return true;
}

OsmAnd::AmenitySymbolsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<TileEntry>& tileEntry)
    : tileEntryWeakRef(tileEntry)
{
}

OsmAnd::AmenitySymbolsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata()
{
}

OsmAnd::AmenitySymbolsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
    if (const auto tileEntry = tileEntryWeakRef.lock())
    {
        if (const auto link = tileEntry->link.lock())
            link->collection.removeEntry(tileEntry->tileId, tileEntry->zoom);
    }
}
