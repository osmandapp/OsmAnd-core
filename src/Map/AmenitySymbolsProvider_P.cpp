#include "AmenitySymbolsProvider_P.h"
#include "AmenitySymbolsProvider.h"

#include "ICoreResourcesProvider.h"
#include "ObfDataInterface.h"
#include "MapDataProviderHelpers.h"
#include "BillboardRasterMapSymbol.h"
#include "SkiaUtilities.h"
#include "Utilities.h"
#include "MapSymbolIntersectionClassesRegistry.h"

static const int kSkipTilesZoom = 13;
static const int kSkipTileDivider = 16;

OsmAnd::AmenitySymbolsProvider_P::AmenitySymbolsProvider_P(AmenitySymbolsProvider* owner_)
: owner(owner_)
, textRasterizer(TextRasterizer::getDefault())
{
}

OsmAnd::AmenitySymbolsProvider_P::~AmenitySymbolsProvider_P()
{
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

bool OsmAnd::AmenitySymbolsProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<AmenitySymbolsProvider::Request>(request_);

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
    QSet<uint32_t> skippedTiles;
    bool zoomFilter = request.zoom <= kSkipTilesZoom;
    const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - request.zoom));
    const auto from31toPixelsScale = static_cast<double>(owner->referenceTileSizeOnScreenInPixels) / tileSize31;
    SymbolsQuadTree boundIntersections(AreaD(tileBBox31).getEnlargedBy(tileBBox31.width() / 2), 4);

    const auto dataInterface = owner->obfsCollection->obtainDataInterface(
        &extendedTileBBox31,
        request.zoom,
        request.zoom,
        ObfDataTypesMask().set(ObfDataType::POI));

    QList< std::shared_ptr<MapSymbolsGroup> > mapSymbolsGroups;
    QSet<ObfObjectId> searchedIds;
    const float displayDensityFactor = owner->displayDensityFactor;
    const auto requestedZoom = request.zoom;
    const auto visitorFunction =
        [this, requestedZoom, displayDensityFactor, &mapSymbolsGroups, &searchedIds, &mapSymbolIntersectionClassesRegistry,
         &extendedTileBBox31, &skippedTiles, zoomFilter, from31toPixelsScale, &boundIntersections, &tileBBox31]
        (const std::shared_ptr<const OsmAnd::Amenity>& amenity) -> bool
        {
            if (owner->amentitiesFilter && !owner->amentitiesFilter(amenity))
            {
                searchedIds << amenity->id;
                return false;
            }
            
            if (searchedIds.contains(amenity->id))
                return false;
            
            const auto pos31 = amenity->position31;
            if (zoomFilter)
            {
                const auto tileId = getTileId(extendedTileBBox31, pos31);
                if (!skippedTiles.contains(tileId))
                    skippedTiles.insert(tileId);
                else
                    return false;
            }
            
            searchedIds << amenity->id;
            auto icon = owner->amenityIconProvider->getIcon(amenity, ZoomLevel16, false);
            if (!icon)
                return false;

            const auto iconSize31 = icon->dimensions().width() / from31toPixelsScale;
            bool intr = intersects(boundIntersections, pos31.x, pos31.y, iconSize31, iconSize31);
            
            if (!tileBBox31.contains(pos31))
                return false;

            int order = owner->baseOrder;
            if (intr)
            {
                icon = owner->amenityIconProvider->getIcon(amenity, ZoomLevel10, false);
                order += 2;
            }
            
            const auto mapSymbolsGroup = std::make_shared<AmenitySymbolsGroup>(amenity);

            const auto mapSymbol = std::make_shared<BillboardRasterMapSymbol>(mapSymbolsGroup);
            mapSymbol->order = order;
            mapSymbol->image = icon;
            mapSymbol->size = PointI(icon->width(), icon->height());
            mapSymbol->languageId = LanguageId::Invariant;
            mapSymbol->position31 = pos31;
            mapSymbolsGroup->symbols.push_back(mapSymbol);

            mapSymbolsGroups.push_back(mapSymbolsGroup);

            const auto caption = owner->amenityIconProvider->getCaption(amenity, requestedZoom);
            if (!intr && !caption.isEmpty())
            {
                const auto textStyle = owner->amenityIconProvider->getCaptionStyle(amenity, requestedZoom);
                const auto textImage = textRasterizer->rasterize(caption, textStyle);
                if (textImage)
                {
                    const auto mapSymbolCaption = std::make_shared<BillboardRasterMapSymbol>(mapSymbolsGroup);
                    mapSymbolCaption->order = order + 1;
                    mapSymbolCaption->image = textImage;
                    mapSymbolCaption->contentClass = OsmAnd::MapSymbol::ContentClass::Caption;
                    mapSymbolCaption->intersectsWithClasses.insert(
                        mapSymbolIntersectionClassesRegistry.getOrRegisterClassIdByName(QStringLiteral("text_layer_caption")));
                    mapSymbolCaption->setOffset(PointI(0, icon->height() / 2 + textImage->height() / 2 + 2 * displayDensityFactor));
                    mapSymbolCaption->size = PointI(textImage->width(), textImage->height());
                    mapSymbolCaption->languageId = LanguageId::Invariant;
                    mapSymbolCaption->position31 = pos31;
                    mapSymbolsGroup->symbols.push_back(mapSymbolCaption);
                }
            }
            
            return true;
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

    outData.reset(new AmenitySymbolsProvider::Data(
        request.tileId,
        request.zoom,
        mapSymbolsGroups));
    return true;
}

OsmAnd::AmenitySymbolsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata()
{
}

OsmAnd::AmenitySymbolsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
