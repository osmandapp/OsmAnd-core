#include "BinaryMapStaticSymbolsProvider_P.h"
#include "BinaryMapStaticSymbolsProvider.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include "restore_internal_warnings.h"

#include "BinaryMapPrimitivesProvider.h"
#include "MapPresentationEnvironment.h"
#include "Primitiviser.h"
#include "SymbolRasterizer.h"
#include "BillboardRasterMapSymbol.h"
#include "OnPathMapSymbol.h"
#include "BinaryMapObject.h"
#include "ObfMapSectionInfo.h"
#include "Utilities.h"

OsmAnd::BinaryMapStaticSymbolsProvider_P::BinaryMapStaticSymbolsProvider_P(BinaryMapStaticSymbolsProvider* owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapStaticSymbolsProvider_P::~BinaryMapStaticSymbolsProvider_P()
{
}

bool OsmAnd::BinaryMapStaticSymbolsProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const FilterCallback filterCallback,
    const IQueryController* const queryController)
{
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Obtain offline map primitives tile
    std::shared_ptr<MapTiledData> primitivesTile_;
    owner->primitivesProvider->obtainData(tileId, zoom, primitivesTile_);
    const auto primitivesTile = std::static_pointer_cast<BinaryMapPrimitivesTile>(primitivesTile_);

    // If tile has nothing to be rasterized, mark that data is not available for it
    if (!primitivesTile_ || primitivesTile->primitivisedArea->isEmpty())
    {
        // Mark tile as empty
        outTiledData.reset();
        return true;
    }

    // Rasterize symbols and create symbols groups
    QList< std::shared_ptr<const SymbolRasterizer::RasterizedSymbolsGroup> > rasterizedSymbolsGroups;
    QHash< uint64_t, std::shared_ptr<MapSymbolsGroup> > preallocatedSymbolsGroups;
    const auto rasterizationFilter =
        [this, tileBBox31, filterCallback, &preallocatedSymbolsGroups]
        (const std::shared_ptr<const Model::BinaryMapObject>& mapObject) -> bool
        {
            const auto isShareable =
                (mapObject->section != owner->primitivesProvider->primitiviser->environment->dummyMapSection) &&
                !tileBBox31.contains(mapObject->bbox31);
            const std::shared_ptr<MapSymbolsGroup> preallocatedGroup(new BinaryMapObjectSymbolsGroup(mapObject, isShareable));

            if (!filterCallback || filterCallback(owner, preallocatedGroup))
            {
                preallocatedSymbolsGroups.insert(mapObject->id, qMove(preallocatedGroup));
                return true;
            }
            return false;
        };
    SymbolRasterizer().rasterize(primitivesTile->primitivisedArea, rasterizedSymbolsGroups, rasterizationFilter, nullptr);

    // Convert results
    QList< std::shared_ptr<MapSymbolsGroup> > symbolsGroups;
    for (const auto& rasterizedGroup : constOf(rasterizedSymbolsGroups))
    {
        const auto& mapObject = rasterizedGroup->mapObject;
        
        // Get preallocated group
        const auto citPreallocatedGroup = preallocatedSymbolsGroups.constFind(mapObject->id);
        assert(citPreallocatedGroup != preallocatedSymbolsGroups.cend());
        const auto group = std::static_pointer_cast<BinaryMapObjectSymbolsGroup>(*citPreallocatedGroup);

        // Convert all symbols inside group
        bool hasAtLeastOneBillboard = false;
        bool hasAtLeastOneOnPath = false;
        for (const auto& rasterizedSymbol : constOf(rasterizedGroup->symbols))
        {
            assert(static_cast<bool>(rasterizedSymbol->bitmap));

            std::shared_ptr<MapSymbol> symbol;
            if (const auto rasterizedSpriteSymbol = std::dynamic_pointer_cast<const SymbolRasterizer::RasterizedSpriteSymbol>(rasterizedSymbol))
            {
                hasAtLeastOneBillboard = true;

                const auto billboardRasterSymbol = new BillboardRasterMapSymbol(group, group->sharableById);
                billboardRasterSymbol->order = rasterizedSpriteSymbol->order;
                billboardRasterSymbol->intersectionModeFlags = MapSymbol::RegularIntersectionProcessing;
                billboardRasterSymbol->bitmap = rasterizedSpriteSymbol->bitmap;
                billboardRasterSymbol->size = PointI(rasterizedSpriteSymbol->bitmap->width(), rasterizedSpriteSymbol->bitmap->height());
                billboardRasterSymbol->content = rasterizedSpriteSymbol->content,
                billboardRasterSymbol->languageId = rasterizedSpriteSymbol->languageId;
                billboardRasterSymbol->minDistance = rasterizedSpriteSymbol->minDistance;
                billboardRasterSymbol->position31 = rasterizedSpriteSymbol->location31;
                billboardRasterSymbol->offset = rasterizedSpriteSymbol->offset;
                symbol.reset(billboardRasterSymbol);
            }
            else if (const auto rasterizedOnPathSymbol = std::dynamic_pointer_cast<const SymbolRasterizer::RasterizedOnPathSymbol>(rasterizedSymbol))
            {
                hasAtLeastOneOnPath = true;

                const auto onPathSymbol = new OnPathMapSymbol(group, group->sharableById);
                onPathSymbol->order = rasterizedOnPathSymbol->order;
                onPathSymbol->intersectionModeFlags = MapSymbol::RegularIntersectionProcessing;
                onPathSymbol->bitmap = rasterizedOnPathSymbol->bitmap;
                onPathSymbol->size = PointI(rasterizedOnPathSymbol->bitmap->width(), rasterizedOnPathSymbol->bitmap->height());
                onPathSymbol->content = rasterizedOnPathSymbol->content;
                onPathSymbol->languageId = rasterizedOnPathSymbol->languageId;
                onPathSymbol->minDistance = rasterizedOnPathSymbol->minDistance;
                onPathSymbol->path = mapObject->points31;
                onPathSymbol->glyphsWidth = rasterizedOnPathSymbol->glyphsWidth;
                symbol.reset(onPathSymbol);
            }
            else
            {
                LogPrintf(LogSeverityLevel::Error, "BinaryMapObject #%" PRIu64 " (%" PRIi64 ") produced unsupported symbol type",
                    mapObject->id,
                    static_cast<int64_t>(mapObject->id) / 2);
            }

            if (rasterizedSymbol->contentType == SymbolRasterizer::RasterizedSymbol::ContentType::Icon)
                symbol->contentClass = MapSymbol::ContentClass::Icon;
            else if (rasterizedSymbol->contentType == SymbolRasterizer::RasterizedSymbol::ContentType::Text)
                symbol->contentClass = MapSymbol::ContentClass::Caption;

            group->symbols.push_back(qMove(symbol));
        }

        // If there's at least one on-path symbol, this group needs special post-processing:
        //  - Compute pin-points for all symbols in group (including billboard ones)
        //  - Split path between them
        if (hasAtLeastOneOnPath)
        {
            QList<SymbolForPinPointsComputation> symbolsForComputation;
            symbolsForComputation.reserve(group->symbols.size());
            for (const auto& symbol : constOf(group->symbols))
            {
                if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                {
                    symbolsForComputation.push_back({ 0, billboardSymbol->size.x, 0 });
                }
                else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathMapSymbol>(symbol))
                {
                    symbolsForComputation.push_back({ 0, onPathSymbol->size.x, 0 });
                }
            }

            //TODO: blaaa
            int i = 5;
        }

        // Configure group
        if (!group->symbols.isEmpty())
        {
            if (hasAtLeastOneBillboard && !hasAtLeastOneOnPath)
            {
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowNoneIfIconIsNotShown;
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAllCaptionsOrNoCaptions;
            }
            else if (!hasAtLeastOneBillboard && hasAtLeastOneOnPath)
            {
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnything;
            }
            else
            {
                // This happens when road has 'ref'+'name*' tags, what is also a valid situation
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnything;
            }
        }

        // Add constructed group to output
        symbolsGroups.push_back(qMove(group));
    }

    // Create output tile
    outTiledData.reset(new BinaryMapStaticSymbolsTile(primitivesTile, symbolsGroups, tileId, zoom));

    return true;
}

QVector<OsmAnd::BinaryMapStaticSymbolsProvider_P::ComputedPinPoint> OsmAnd::BinaryMapStaticSymbolsProvider_P::computePinPoints(
    const QVector<PointI>& path31,
    const float globalLeftPaddingInPixels,
    const float globalRightPaddingInPixels,
    const QVector<SymbolForPinPointsComputation>& symbolsForPinPointsComputation,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom)
{
    QVector<ComputedPinPoint> computedPinPoints;

    // Compute pin-points placement starting from minZoom to maxZoom.
    
    // Example of symbol instance placement assuming it fits exactly 4 times on minZoom ('si' is symbol instance):
    // minZoom+0: sisisisi
    // Since each next zoom is 2x bigger, thus here's how minZoom+1 will look like without additional instances ('-' is widthOfSymbolInPixels/2)
    // minZoom+1: -si--si--si--si-
    // After placing 3 additional instances it will look like
    // minZoom+1: -sisisisisisisi-
    // On next zoom without additional instances it will look like
    // minZoom+2: ---si--si--si--si--si--si--si---
    // After placing additional 8 instances it will look like
    // minZoom+2: -sisisisisisisisisisisisisisisi-
    // On next zoom without additional 16 instances it will look like
    // minZoom+3: ---si--si--si--si--si--si--si--si--si--si--si--si--si--si--si---
    // On next zoom without additional 32 instances it will look like
    // minZoom+4: ---si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si--si---
    // This gives following sequence : (+4 on minZoom+0);(+3 on minZoom+1);(+8 on minZoom+2);(+16 on minZoom+3);(+32 on minZoom+4)

    // Another example of symbol instance placement assuming only 3.5 symbol instances fit ('.' is widthOfSymbolInPixels/4)
    // minZoom+0: .sisisi.
    // On next zoom without 4 additional instances
    // minZoom+1: --si--si--si--
    // After placement additional 4 instances
    // minZoom+1: sisisisisisisi
    // On next zoom without additional 6 instances
    // minZoom+2: -si--si--si--si--si--si--si-
    // minZoom+2: -sisisisisisisisisisisisisi-
    // On next zoom without additional 14 instances
    // minZoom+3: ---si--si--si--si--si--si--si--si--si--si--si--si--si---
    // This gives following sequence : (+3 on minZoom+0);(+4 on minZoom+1);(+6 on minZoom+2);(+14 on minZoom+3)

    // As clearly seen - on each next zoom level number of instances is doubled.
    // Expressing this fact as numbers here what will be the result:
    // lengthOfPathInPixelsOnBaseZoom = computePathLengthInPixels(path, minZoom)
    // baseNumberOfInstances = | lengthOfPathInPixels / widthOfSymbolInPixels |
    // remainingLength = lengthOfPathInPixelsOnBaseZoom - baseNumberOfInstances * widthOfSymbolInPixels
    // numberOfNewInstancesOnNextZoom = (baseNumberOfInstances - 1) + 2*|remainingLength / widthOfSymbolInPixels|;

    // Check for widthOfSymbolInPixels=10 and lengthOfPathInPixelsOnBaseZoom=40
    // minZoom+0: sisisisi
    //  - baseNumberOfInstances = | 40/10 | -> 4
    //  - remainingLength = 40 - 4*10 -> 0
    //  - numberOfNewInstancesOnNextZoom = (4-1) + 2*|0 / 10| -> 3 + 2*0 -> 3
    // minZoom+1: -si--si--si--si-
    //  - lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnPrevZoom * 2 -> 40 * 2 -> 80
    //  - numberOfInstances = 4 + 3 -> 7
    //  - remainingLength = 80 - 7*10 -> 10
    //  - numberOfNewInstancesOnNextZoom = (7-1) + 2*|10 / 10| -> 6 + 2*1 -> 8
    // minZoom+2: ---si--si--si--si--si--si--si---
    //  - lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnPrevZoom * 2 -> 80 * 2 -> 160
    //  - numberOfInstances = 7 + 8 -> 15
    //  - remainingLength = 160 - 15*10 -> 10
    //  - numberOfNewInstancesOnNextZoom = (15-1) + 2*|10 / 10| -> 14 + 2*1 -> 16

    // Check for widthOfSymbolInPixels=10 and lengthOfPathInPixelsOnBaseZoom=35
    // minZoom+0: .sisisi.
    //  - baseNumberOfInstances = | 35/10 | -> 3
    //  - remainingLength = 40 - 3*10 -> 10
    //  - numberOfNewInstancesOnNextZoom = (3-1) + 2*|10 / 10| -> 2 + 2*1 -> 4
    // minZoom+1: --si--si--si--
    //  - lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnPrevZoom * 2 -> 35 * 2 -> 70
    //  - numberOfInstances = 3 + 4 -> 7
    //  - remainingLength = 70 - 7*10 -> 0
    //  - numberOfNewInstancesOnNextZoom = (7-1) + 2*|0 / 10| -> 6 + 2*0 -> 6
    // minZoom+2: -si--si--si--si--si--si--si-
    //  - lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnPrevZoom * 2 -> 70 * 2 -> 140
    //  - numberOfInstances = 7 + 6 -> 13
    //  - remainingLength = 140 - 13*10 -> 10
    //  - numberOfNewInstancesOnNextZoom = (13-1) + 2*|10 / 10| -> 12 + 2*1 -> 14

    // Placement of pin-points is aligned to center, so there's a special offset variable computed as
    // offsetOnBaseZoom = {lengthOfPathInPixelsOnBaseZoom / widthOfSymbolInPixels} / 2.0
    // Example for widthOfSymbolInPixels=10 and lengthOfPathInPixelsOnBaseZoom=40
    // offsetOnBaseZoom = {40 / 10} / 2.0 -> 0 / 2.0 -> 0
    // Example for widthOfSymbolInPixels=10 and lengthOfPathInPixelsOnBaseZoom=35
    // offsetOnBaseZoom = {35 / 10} / 2.0 -> 0.5 / 2.0 -> 0.25
    // This offsetOnBase zoom specifies offset measured in 'widthOfSymbolInPixels' to first instance start (present or not)

    // On each next level:
    // offsetToFirstPresentInstance = 0.5 + offsetOnPrevZoom * 2
    // offsetOnCurrentZoom = (offsetToFirstPresentInstance > 1.0) ? offsetToFirstPresentInstance - 1.0 : offsetToFirstPresentInstance + 1.0;

    return computedPinPoints;
}
