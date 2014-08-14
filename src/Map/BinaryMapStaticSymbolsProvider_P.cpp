#include "BinaryMapStaticSymbolsProvider_P.h"
#include "BinaryMapStaticSymbolsProvider.h"

#include "QtExtensions.h"
#include "QtCommon.h"

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
                billboardRasterSymbol->content = rasterizedSpriteSymbol->content;
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
            // Compose list of symbols to compute pin-points for
            QList<SymbolForPinPointsComputation> symbolsForComputation;
            symbolsForComputation.reserve(group->symbols.size());
            for (const auto& symbol : constOf(group->symbols))
            {
                if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                {
                    // Get larger bbox, to take into account possible rotation
                    const auto maxSize = qMax(billboardSymbol->size.x, billboardSymbol->size.y);
                    const auto outerCircleRadius = 0.5f * qSqrt(2 * maxSize * maxSize);
                    symbolsForComputation.push_back({ 0, 2.0f * outerCircleRadius, 0 });
                }
                else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathMapSymbol>(symbol))
                {
                    symbolsForComputation.push_back({ 0, onPathSymbol->size.x, 0 });
                }
            }

            const auto computedPinPoints = computePinPoints(
                mapObject->points31,
                0.0f,
                0.0f,
                symbolsForComputation,
                mapObject->level->minZoom,
                mapObject->level->maxZoom);

            // After pin-points were computed, assign them to symbols in the same order
            QHash< std::shared_ptr<MapSymbol>, QList<std::shared_ptr<MapSymbol>> > extraSymbolInstances;
            auto citComputedPinPoint = computedPinPoints.begin();
            const auto citComputedPinPointsEnd = computedPinPoints.end();
            for (const auto& symbol : constOf(group->symbols))
            {
                // Stop in case no more pin-points left
                if (citComputedPinPoint == citComputedPinPointsEnd)
                    break;
                const auto& computedPinPoint = *(citComputedPinPoint++);

                if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                {
                    // Create additional instance of billboard raster map symbol
                    std::shared_ptr<BillboardRasterMapSymbol> extraSymbolInstance(new BillboardRasterMapSymbol(group, group->sharableById));
                    extraSymbolInstance->order = billboardSymbol->order;
                    extraSymbolInstance->intersectionModeFlags = billboardSymbol->intersectionModeFlags;
                    extraSymbolInstance->bitmap = billboardSymbol->bitmap;
                    extraSymbolInstance->size = billboardSymbol->size;
                    extraSymbolInstance->content = billboardSymbol->content;
                    extraSymbolInstance->contentClass = billboardSymbol->contentClass;
                    extraSymbolInstance->languageId = billboardSymbol->languageId;
                    extraSymbolInstance->minDistance = billboardSymbol->minDistance;
                    extraSymbolInstance->position31 = computedPinPoint.point;
                    extraSymbolInstance->offset = billboardSymbol->offset;

                    extraSymbolInstances[billboardSymbol].push_back(extraSymbolInstance);
                }
                else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathMapSymbol>(symbol))
                {
                    OnPathMapSymbol::PinPoint pinPoint;
                    pinPoint.point = computedPinPoint.point;
                    pinPoint.basePathPointIndex = computedPinPoint.basePathPointIndex;
                    pinPoint.offsetFromBasePathPoint31 = computedPinPoint.offsetFromBasePathPoint31;
                    pinPoint.normalizedOffsetFromBasePathPoint = computedPinPoint.normalizedOffsetFromBasePathPoint;

                    onPathSymbol->pinPoints.push_back(qMove(pinPoint));
                }
            }

            // Now merge from extraSymbolInstances into group
            auto itSymbol = mutableIteratorOf(group->symbols);
            while (itSymbol.hasNext())
            {
                const auto& currentSymbol = itSymbol.next();

                // In case extraSymbolInstances have symbols derived from this one, replace current with those
                const auto itReplacementSymbols = extraSymbolInstances.find(currentSymbol);
                if (itReplacementSymbols != extraSymbolInstances.end())
                {
                    const auto& replacementSymbols = *itReplacementSymbols;

                    itSymbol.remove();
                    for (const auto& replacementSymbol : constOf(replacementSymbols))
                        itSymbol.insert(replacementSymbol);
                    extraSymbolInstances.erase(itReplacementSymbols);
                }
            }
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
                // This happens when e.g. road has 'ref'+'name*' tags, what is also a valid situation
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

QList<OsmAnd::BinaryMapStaticSymbolsProvider_P::ComputedPinPoint> OsmAnd::BinaryMapStaticSymbolsProvider_P::computePinPoints(
    const QVector<PointI>& path31,
    const float globalLeftPaddingInPixels,
    const float globalRightPaddingInPixels,
    const QList<SymbolForPinPointsComputation>& symbolsForPinPointsComputation_,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom)
{
    QList<ComputedPinPoint> computedPinPoints;

    // Compute pin-points placement starting from minZoom to maxZoom. How this works:
    //
    // Example of block instance placement assuming it fits exactly 4 times on minZoom ('bi' is block instance):
    // minZoom+0: bibibibi
    // Since each next zoom is 2x bigger, thus here's how minZoom+1 will look like without additional instances ('-' is widthOfBlockInPixels/2)
    // minZoom+1: -bi--bi--bi--bi-
    // After placing 3 additional instances it will look like
    // minZoom+1: -bibibibibibibi-
    // On next zoom without additional instances it will look like
    // minZoom+2: ---bi--bi--bi--bi--bi--bi--bi---
    // After placing additional 8 instances it will look like
    // minZoom+2: -bibibibibibibibibibibibibibibi-
    // On next zoom without additional 16 instances it will look like
    // minZoom+3: ---bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi---
    // On next zoom without additional 32 instances it will look like
    // minZoom+4: ---bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi---
    // This gives following sequence : (+4 on minZoom+0);(+3 on minZoom+1);(+8 on minZoom+2);(+16 on minZoom+3);(+32 on minZoom+4)
    //
    // Another example of block instance placement assuming only 3.5 block instances fit ('.' is widthOfBlockInPixels/4)
    // minZoom+0: .bibibi.
    // On next zoom without 4 additional instances
    // minZoom+1: --bi--bi--bi--
    // After placement additional 4 instances
    // minZoom+1: bibibibibibibi
    // On next zoom without additional 6 instances
    // minZoom+2: -bi--bi--bi--bi--bi--bi--bi-
    // minZoom+2: -bibibibibibibibibibibibibi-
    // On next zoom without additional 14 instances
    // minZoom+3: ---bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi--bi---
    // This gives following sequence : (+3 on minZoom+0);(+4 on minZoom+1);(+6 on minZoom+2);(+14 on minZoom+3)
    //
    // As clearly seen - on each next zoom level number of instances is doubled.
    // Expressing this fact as numbers here what will be the result:
    // lengthOfPathInPixelsOnBaseZoom = computePathLengthInPixels(path, minZoom)
    // baseNumberOfInstances = | lengthOfPathInPixels / widthOfBlockInPixels |
    // remainingLength = lengthOfPathInPixelsOnBaseZoom - baseNumberOfInstances * widthOfBlockInPixels
    // numberOfNewInstancesOnNextZoom = (baseNumberOfInstances - 1) + 2*|remainingLength / widthOfBlockInPixels|;
    //
    // Check for widthOfBlockInPixels=10 and lengthOfPathInPixelsOnBaseZoom=40
    // minZoom+0: bibibibi
    //  - baseNumberOfInstances = | 40/10 | -> 4
    //  - remainingLength = 40 - 4*10 -> 0
    //  - numberOfNewInstancesOnNextZoom = (4-1) + 2*|0 / 10| -> 3 + 2*0 -> 3
    // minZoom+1: -bi--bi--bi--bi-
    //  - lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnPrevZoom * 2 -> 40 * 2 -> 80
    //  - numberOfInstances = 4 + 3 -> 7
    //  - remainingLength = 80 - 7*10 -> 10
    //  - numberOfNewInstancesOnNextZoom = (7-1) + 2*|10 / 10| -> 6 + 2*1 -> 8
    // minZoom+2: ---bi--bi--bi--bi--bi--bi--bi---
    //  - lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnPrevZoom * 2 -> 80 * 2 -> 160
    //  - numberOfInstances = 7 + 8 -> 15
    //  - remainingLength = 160 - 15*10 -> 10
    //  - numberOfNewInstancesOnNextZoom = (15-1) + 2*|10 / 10| -> 14 + 2*1 -> 16
    //
    // Check for widthOfBlockInPixels=10 and lengthOfPathInPixelsOnBaseZoom=35
    // minZoom+0: .bibibi.
    //  - baseNumberOfInstances = | 35/10 | -> 3
    //  - remainingLength = 40 - 3*10 -> 10
    //  - numberOfNewInstancesOnNextZoom = (3-1) + 2*|10 / 10| -> 2 + 2*1 -> 4
    // minZoom+1: --bi--bi--bi--
    //  - lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnPrevZoom * 2 -> 35 * 2 -> 70
    //  - numberOfInstances = 3 + 4 -> 7
    //  - remainingLength = 70 - 7*10 -> 0
    //  - numberOfNewInstancesOnNextZoom = (7-1) + 2*|0 / 10| -> 6 + 2*0 -> 6
    // minZoom+2: -bi--bi--bi--bi--bi--bi--bi-
    //  - lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnPrevZoom * 2 -> 70 * 2 -> 140
    //  - numberOfInstances = 7 + 6 -> 13
    //  - remainingLength = 140 - 13*10 -> 10
    //  - numberOfNewInstancesOnNextZoom = (13-1) + 2*|10 / 10| -> 12 + 2*1 -> 14
    //
    // Placement of pin-points is aligned to center, so there's a special offset variable computed as
    // offsetOnBaseZoom = {lengthOfPathInPixelsOnBaseZoom / widthOfBlockInPixels} / 2.0
    // Example for widthOfBlockInPixels=10 and lengthOfPathInPixelsOnBaseZoom=40
    // offsetOnBaseZoom = {40 / 10} / 2.0 -> 0 / 2.0 -> 0
    // Example for widthOfBlockInPixels=10 and lengthOfPathInPixelsOnBaseZoom=35
    // offsetOnBaseZoom = {35 / 10} / 2.0 -> 0.5 / 2.0 -> 0.25
    // This offsetOnBase zoom specifies offset measured in 'widthOfBlockInPixels' to first instance start (present or not)
    //
    // On each next level:
    // offsetToFirstPresentInstance = 0.5 + offsetOnPrevZoom * 2
    // offsetOnCurrentZoom = (offsetToFirstPresentInstance > 1.0) ? offsetToFirstPresentInstance - 1.0 : offsetToFirstPresentInstance + 1.0;
    //
    // Each next instance on this level is located at offsetOnCurrentZoom + 2*instancesPlotted

    // And here's the implementation:
    auto symbolsForPinPointsComputation = symbolsForPinPointsComputation_;

    // Step 0. Initial checks
    if (symbolsForPinPointsComputation.isEmpty())
        return computedPinPoints;

    // Step 1. Get scale factor from 31 to pixels for minZoom.
    // Only display density is used, since it's not real calculation, thus tile is imagined to be 256????

    // Step 2. Compute path length, path segments length (in 31 and in pixels)
    const auto pathSize = path31.size();
    const auto pathSegmentsCount = pathSize - 1;
    float pathLengthInPixels = 0.0f;
    QVector<float> pathSegmentsLengthInPixels(pathSegmentsCount);
    auto pPathSegmentLengthInPixels = pathSegmentsLengthInPixels.data();
    double pathLength31 = 0.0;
    QVector<double> pathSegmentsLength31(pathSegmentsCount);
    auto pPathSegmentLength31 = pathSegmentsLength31.data();
    auto pPoint31 = path31.constData();
    auto pPrevPoint31 = pPoint31++;
    for (auto segmentIdx = 0; segmentIdx < pathSegmentsCount; segmentIdx++)
    {
        const auto segmentLength31 = qSqrt((*(pPoint31++) - *(pPrevPoint31++)).squareNorm());
        *(pPathSegmentLength31++) = segmentLength31;
        pathLength31 += segmentLength31;

        const auto segmentLengthInPixels = ;
        *(pPathSegmentLengthInPixels++) = segmentLengthInPixels;
        pathLengthInPixels += segmentLengthInPixels;
    }
    const auto usablePathLengthInPixels = pathLengthInPixels - globalLeftPaddingInPixels - globalRightPaddingInPixels;

    // Step 3. Compute total width of all symbols requested. This will be the block width.
    float blockWidth = 0.0f;
    bool limitToOneBlockInstance = false;
    unsigned int symbolsInBlockThatFit = 0;
    for (const auto& symbolForPinPointsComputation : constOf(symbolsForPinPointsComputation))
    {
        const auto previousBlockWidth = blockWidth;

        blockWidth += symbolForPinPointsComputation.leftPaddingInPixels;
        blockWidth += symbolForPinPointsComputation.widthInPixels;
        blockWidth += symbolForPinPointsComputation.rightPaddingInPixels;

        // If block can not fit, use shorted version
        if (usablePathLengthInPixels < blockWidth)
        {
            blockWidth = previousBlockWidth;
            limitToOneBlockInstance = true;
            symbolsForPinPointsComputation = symbolsForPinPointsComputation.mid(0, symbolsInBlockThatFit);

            break;
        }
        symbolsInBlockThatFit++;
    }
    if (symbolsForPinPointsComputation.isEmpty() || qFuzzyIsNull(blockWidth))
        return computedPinPoints;

    // Step 4. Process base zoom level
    const auto lengthOfPathInPixelsOnBaseZoom = usablePathLengthInPixels;
    const auto numberOfInstancesOnBaseZoom = limitToOneBlockInstance ? 1 : qFloor(lengthOfPathInPixelsOnBaseZoom / blockWidth);
    const auto remainingLengthOnBaseZoom = usablePathLengthInPixels - numberOfInstancesOnBaseZoom * blockWidth;
    const auto numberOfNewInstancesOnNextZoom = (numberOfInstancesOnBaseZoom - 1) + 2 * qFloor(remainingLengthOnBaseZoom / blockWidth);

    // Step 5. Process remaining zoom levels
    for (auto currentZoomLevel = minZoom + 1; currentZoomLevel <= maxZoom; currentZoomLevel++)
    {

    }

    return computedPinPoints;
}
