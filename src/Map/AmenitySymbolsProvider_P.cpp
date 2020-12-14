#include "AmenitySymbolsProvider_P.h"
#include "AmenitySymbolsProvider.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include "restore_internal_warnings.h"

#include "ICoreResourcesProvider.h"
#include "ObfDataInterface.h"
#include "MapDataProviderHelpers.h"
#include "BillboardRasterMapSymbol.h"
#include "SkiaUtilities.h"
#include "Utilities.h"
#include "MapSymbolIntersectionClassesRegistry.h"

OsmAnd::AmenitySymbolsProvider_P::AmenitySymbolsProvider_P(AmenitySymbolsProvider* owner_)
: owner(owner_)
, textRasterizer(TextRasterizer::getDefault())
{
}

OsmAnd::AmenitySymbolsProvider_P::~AmenitySymbolsProvider_P()
{
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
    const auto dataInterface = owner->obfsCollection->obtainDataInterface(
        &tileBBox31,
        request.zoom,
        request.zoom,
        ObfDataTypesMask().set(ObfDataType::POI));

    QList< std::shared_ptr<MapSymbolsGroup> > mapSymbolsGroups;
    QSet<ObfObjectId> searchedIds;
    const float displayDensityFactor = 3.0f;
    const auto requestedZoom = request.zoom;
    const auto visitorFunction =
        [this, requestedZoom, displayDensityFactor, &mapSymbolsGroups, &searchedIds, &mapSymbolIntersectionClassesRegistry]
        (const std::shared_ptr<const OsmAnd::Amenity>& amenity) -> bool
        {
            if (owner->amentitiesFilter && !owner->amentitiesFilter(amenity))
            {
                searchedIds << amenity->id;
                return false;
            }
            
            if (searchedIds.contains(amenity->id))
                return false;
            
            searchedIds << amenity->id;
            const auto icon = owner->amenityIconProvider->getIcon(amenity, requestedZoom, false);
            if (!icon)
                return false;

            const auto mapSymbolsGroup = std::make_shared<AmenitySymbolsGroup>(amenity);

            const auto mapSymbol = std::make_shared<BillboardRasterMapSymbol>(mapSymbolsGroup);
            mapSymbol->order = 100000;
            mapSymbol->bitmap = icon;
            mapSymbol->size = PointI(icon->width(), icon->height());
            mapSymbol->languageId = LanguageId::Invariant;
            mapSymbol->position31 = amenity->position31;
            mapSymbolsGroup->symbols.push_back(mapSymbol);

            mapSymbolsGroups.push_back(mapSymbolsGroup);

            const auto caption = owner->amenityIconProvider->getCaption(amenity, requestedZoom);
            if (!caption.isEmpty())
            {
                const auto textStyle = owner->amenityIconProvider->getCaptionStyle(amenity, requestedZoom);
                const auto textBmp = textRasterizer->rasterize(caption, textStyle);
                if (textBmp)
                {
                    const auto mapSymbolCaption = std::make_shared<BillboardRasterMapSymbol>(mapSymbolsGroup);
                    mapSymbolCaption->order = 100001;
                    mapSymbolCaption->bitmap = textBmp;
                    mapSymbolCaption->contentClass = OsmAnd::MapSymbol::ContentClass::Caption;
                    mapSymbolCaption->intersectsWithClasses.insert(mapSymbolIntersectionClassesRegistry.getOrRegisterClassIdByName(QStringLiteral("text_layer_caption")));
                    mapSymbolCaption->setOffset(PointI(0, icon->height() / 2 + textBmp->height() / 2 + 2 * displayDensityFactor));
                    mapSymbolCaption->size = PointI(textBmp->width(), textBmp->height());
                    mapSymbolCaption->languageId = LanguageId::Invariant;
                    mapSymbolCaption->position31 = amenity->position31;
                    mapSymbolsGroup->symbols.push_back(mapSymbolCaption);
                }
            }
            
            return true;
        };
    dataInterface->loadAmenities(
        nullptr,
        &tileBBox31,
        nullptr,
        request.zoom,
        owner->categoriesFilter.getValuePtrOrNullptr(),
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
