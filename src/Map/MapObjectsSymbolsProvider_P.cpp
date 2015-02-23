#include "MapObjectsSymbolsProvider_P.h"
#include "MapObjectsSymbolsProvider.h"

#include "QtExtensions.h"
#include "QtCommon.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include "restore_internal_warnings.h"

#include "MapSymbolIntersectionClassesRegistry.h"
#include "MapPrimitivesProvider.h"
#include "MapPresentationEnvironment.h"
#include "MapPrimitiviser.h"
#include "SymbolRasterizer.h"
#include "BillboardRasterMapSymbol.h"
#include "OnPathRasterMapSymbol.h"
#include "MapObject.h"
#include "ObfMapSectionInfo.h"
#include "Utilities.h"

OsmAnd::MapObjectsSymbolsProvider_P::MapObjectsSymbolsProvider_P(MapObjectsSymbolsProvider* owner_)
    : owner(owner_)
{
}

OsmAnd::MapObjectsSymbolsProvider_P::~MapObjectsSymbolsProvider_P()
{
}

bool OsmAnd::MapObjectsSymbolsProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapObjectsSymbolsProvider::Data>& outTiledData,
    const IQueryController* const queryController,
    const FilterCallback filterCallback)
{
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Obtain offline map primitives tile
    std::shared_ptr<IMapTiledDataProvider::Data> primitivesTile_;
    owner->primitivesProvider->obtainData(tileId, zoom, primitivesTile_);
    const auto primitivesTile = std::static_pointer_cast<MapPrimitivesProvider::Data>(primitivesTile_);

    // If tile has nothing to be rasterized, mark that data is not available for it
    if (!primitivesTile_ || primitivesTile->primitivisedObjects->isEmpty())
    {
        // Mark tile as empty
        outTiledData.reset();
        return true;
    }

    // Rasterize symbols and create symbols groups
    QList< std::shared_ptr<const SymbolRasterizer::RasterizedSymbolsGroup> > rasterizedSymbolsGroups;
    QHash< std::shared_ptr<const MapObject>, std::shared_ptr<MapObjectSymbolsGroup> > preallocatedSymbolsGroups;
    const auto rasterizationFilter =
        [this, tileBBox31, filterCallback, &preallocatedSymbolsGroups]
        (const std::shared_ptr<const MapObject>& mapObject) -> bool
        {
            const std::shared_ptr<MapObjectSymbolsGroup> preallocatedGroup(new MapObjectSymbolsGroup(mapObject));

            if (!filterCallback || filterCallback(owner, preallocatedGroup))
            {
                preallocatedSymbolsGroups.insert(mapObject, qMove(preallocatedGroup));
                return true;
            }
            return false;
        };
    owner->symbolRasterizer->rasterize(
        primitivesTile->primitivisedObjects,
        rasterizedSymbolsGroups,
        rasterizationFilter,
        nullptr);

    // Convert results
    auto& mapSymbolIntersectionClassesRegistry = MapSymbolIntersectionClassesRegistry::globalInstance();
    QList< std::shared_ptr<MapSymbolsGroup> > symbolsGroups;
    for (const auto& rasterizedGroup : constOf(rasterizedSymbolsGroups))
    {
        const auto& mapObject = rasterizedGroup->mapObject;

        //////////////////////////////////////////////////////////////////////////
        //if ((mapObject->id >> 1) == 189600735u)
        //{
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        // Get preallocated group
        const auto citPreallocatedGroup = preallocatedSymbolsGroups.constFind(mapObject);
        assert(citPreallocatedGroup != preallocatedSymbolsGroups.cend());
        const auto group = *citPreallocatedGroup;

        // Create shareable path
        const std::shared_ptr< const QVector<PointI> > shareablePath31(new QVector<PointI>(mapObject->points31));

        // Convert all symbols inside group
        bool hasAtLeastOneOnPath = false;
        bool hasAtLeastOneAlongPathBillboard = false;
        bool hasAtLeastOneSimpleBillboard = false;
        for (const auto& rasterizedSymbol : constOf(rasterizedGroup->symbols))
        {
            assert(static_cast<bool>(rasterizedSymbol->bitmap));

            std::shared_ptr<MapSymbol> symbol;
            if (const auto rasterizedSpriteSymbol = std::dynamic_pointer_cast<const SymbolRasterizer::RasterizedSpriteSymbol>(rasterizedSymbol))
            {
                if (!hasAtLeastOneAlongPathBillboard && rasterizedSpriteSymbol->drawAlongPath)
                    hasAtLeastOneAlongPathBillboard = true;
                if (!hasAtLeastOneSimpleBillboard && !rasterizedSpriteSymbol->drawAlongPath)
                    hasAtLeastOneSimpleBillboard = true;

                const auto billboardRasterSymbol = new BillboardRasterMapSymbol(group);
                billboardRasterSymbol->order = rasterizedSpriteSymbol->order;
                billboardRasterSymbol->bitmap = rasterizedSpriteSymbol->bitmap;
                billboardRasterSymbol->size = PointI(rasterizedSpriteSymbol->bitmap->width(), rasterizedSpriteSymbol->bitmap->height());
                billboardRasterSymbol->content = rasterizedSpriteSymbol->content;
                billboardRasterSymbol->languageId = rasterizedSpriteSymbol->languageId;
                billboardRasterSymbol->minDistance = rasterizedSpriteSymbol->minDistance;
                billboardRasterSymbol->position31 = rasterizedSpriteSymbol->location31;
                billboardRasterSymbol->offset = rasterizedSpriteSymbol->offset;
                if (rasterizedSpriteSymbol->intersectionBBox.width() > 0 || rasterizedSpriteSymbol->intersectionBBox.height() > 0)
                {
                    const auto halfWidth = billboardRasterSymbol->size.x / 2;
                    const auto halfHeight = billboardRasterSymbol->size.y / 2;

                    billboardRasterSymbol->margin.top() = -rasterizedSpriteSymbol->intersectionBBox.top() - halfHeight;
                    billboardRasterSymbol->margin.left() = -rasterizedSpriteSymbol->intersectionBBox.left() - halfWidth;
                    billboardRasterSymbol->margin.bottom() = rasterizedSpriteSymbol->intersectionBBox.bottom() - halfHeight;
                    billboardRasterSymbol->margin.right() = rasterizedSpriteSymbol->intersectionBBox.right() - halfWidth;

                    // Collect intersection classes
                    for (const auto& intersectsWithClass : constOf(rasterizedSpriteSymbol->primitiveSymbol->intersectsWith))
                    {
                        billboardRasterSymbol->intersectsWithClasses.insert(mapSymbolIntersectionClassesRegistry.getOrRegisterClassIdByName(intersectsWithClass));
                    }
                }
                billboardRasterSymbol->pathPaddingLeft = rasterizedSpriteSymbol->pathPaddingLeft;
                billboardRasterSymbol->pathPaddingRight = rasterizedSpriteSymbol->pathPaddingRight;
                symbol.reset(billboardRasterSymbol);
            }
            else if (const auto rasterizedOnPathSymbol = std::dynamic_pointer_cast<const SymbolRasterizer::RasterizedOnPathSymbol>(rasterizedSymbol))
            {
                hasAtLeastOneOnPath = true;

                const auto onPathSymbol = new OnPathRasterMapSymbol(group);
                onPathSymbol->order = rasterizedOnPathSymbol->order;
                onPathSymbol->bitmap = rasterizedOnPathSymbol->bitmap;
                onPathSymbol->size = PointI(rasterizedOnPathSymbol->bitmap->width(), rasterizedOnPathSymbol->bitmap->height());
                onPathSymbol->content = rasterizedOnPathSymbol->content;
                onPathSymbol->languageId = rasterizedOnPathSymbol->languageId;
                onPathSymbol->minDistance = rasterizedOnPathSymbol->minDistance;
                onPathSymbol->shareablePath31 = shareablePath31;
                assert(shareablePath31->size() >= 2);
                onPathSymbol->glyphsWidth = rasterizedOnPathSymbol->glyphsWidth;
                for (const auto& intersectsWithClass : constOf(rasterizedOnPathSymbol->primitiveSymbol->intersectsWith))
                {
                    onPathSymbol->intersectsWithClasses.insert(mapSymbolIntersectionClassesRegistry.getOrRegisterClassIdByName(intersectsWithClass));
                }
                onPathSymbol->pathPaddingLeft = rasterizedOnPathSymbol->pathPaddingLeft;
                onPathSymbol->pathPaddingRight = rasterizedOnPathSymbol->pathPaddingRight;
                symbol.reset(onPathSymbol);
            }
            else
            {
                LogPrintf(LogSeverityLevel::Error, "MapObject %s produced unsupported symbol type",
                    qPrintable(mapObject->toString()));
            }

            if (rasterizedSymbol->contentType == SymbolRasterizer::RasterizedSymbol::ContentType::Icon)
                symbol->contentClass = MapSymbol::ContentClass::Icon;
            else if (rasterizedSymbol->contentType == SymbolRasterizer::RasterizedSymbol::ContentType::Text)
                symbol->contentClass = MapSymbol::ContentClass::Caption;

            group->symbols.push_back(qMove(symbol));
        }

        // If there's at least one on-path symbol or along-path symbol, this group needs special post-processing:
        //  - Compute pin-points for all symbols in group (including billboard ones)
        //  - Split path between them
        if (hasAtLeastOneOnPath || hasAtLeastOneAlongPathBillboard)
        {
            //////////////////////////////////////////////////////////////////////////
            //if ((mapObject->id >> 1) == 7381701u)
            //{
            //    int i = 5;
            //}
            //////////////////////////////////////////////////////////////////////////

            // Compose list of symbols to compute pin-points for
            QList<SymbolForPinPointsComputation> symbolsForComputation;
            symbolsForComputation.reserve(group->symbols.size());
            for (const auto& symbol : constOf(group->symbols))
            {
                if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                {
                    // Get larger bbox, to take into account possible rotation
                    const auto maxSize = qMax(billboardSymbol->size.x, billboardSymbol->size.y);
                    const auto outerCircleRadius = 0.5f * static_cast<float>(qSqrt(2 * maxSize * maxSize));
                    symbolsForComputation.push_back({
                        billboardSymbol->pathPaddingLeft,
                        2.0f * outerCircleRadius,
                        billboardSymbol->pathPaddingRight });
                }
                else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathRasterMapSymbol>(symbol))
                {
                    symbolsForComputation.push_back({
                        onPathSymbol->pathPaddingLeft,
                        static_cast<float>(onPathSymbol->size.x),
                        onPathSymbol->pathPaddingRight });
                }
            }

            const auto& env = owner->primitivesProvider->primitiviser->environment;
            float globalLeftPaddingInPixels = 0.0f;
            float globalRightPaddingInPixels = 0.0f;
            env->obtainGlobalPathPadding(globalLeftPaddingInPixels, globalRightPaddingInPixels);
            const auto globalSpacingBetweenBlocksInPixels = env->getGlobalPathSymbolsBlockSpacing();
            const auto computedPinPointsByLayer = computePinPoints(
                mapObject->points31,
                globalLeftPaddingInPixels, // global left padding in pixels
                globalRightPaddingInPixels, // global right padding in pixels
                globalSpacingBetweenBlocksInPixels, // global spacing between blocks in pixels
                symbolsForComputation,
                mapObject->getMinZoomLevel(),
                mapObject->getMaxZoomLevel(),
                zoom);

            // After pin-points were computed, assign them to symbols in the same order
            for (const auto& computedPinPoints : constOf(computedPinPointsByLayer))
            {
                auto citComputedPinPoint = computedPinPoints.cbegin();
                const auto citComputedPinPointsEnd = computedPinPoints.cend();
                while (citComputedPinPoint != citComputedPinPointsEnd)
                {
                    // Construct new additional instance of group
                    std::shared_ptr<MapSymbolsGroup::AdditionalInstance> additionalGroupInstance(new MapSymbolsGroup::AdditionalInstance(group));
                    
                    for (const auto& symbol : constOf(group->symbols))
                    {
                        // Stop in case no more pin-points left
                        if (citComputedPinPoint == citComputedPinPointsEnd)
                            break;
                        const auto& computedPinPoint = *(citComputedPinPoint++);

                        std::shared_ptr<MapSymbolsGroup::AdditionalSymbolInstanceParameters> additionalSymbolInstance;
                        if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                        {
                            const auto billboardSymbolInstance = new MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters(additionalGroupInstance.get());
                            billboardSymbolInstance->overridesPosition31 = true;
                            billboardSymbolInstance->position31 = computedPinPoint.point31;
                            additionalSymbolInstance.reset(billboardSymbolInstance);
                        }
                        else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathRasterMapSymbol>(symbol))
                        {
                            IOnPathMapSymbol::PinPoint pinPoint;
                            pinPoint.point31 = computedPinPoint.point31;
                            pinPoint.basePathPointIndex = computedPinPoint.basePathPointIndex;
                            pinPoint.offsetFromBasePathPoint31 = computedPinPoint.offsetFromBasePathPoint31;
                            pinPoint.normalizedOffsetFromBasePathPoint = computedPinPoint.normalizedOffsetFromBasePathPoint;

                            const auto onPathSymbolInstance = new MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters(additionalGroupInstance.get());
                            onPathSymbolInstance->overridesPinPointOnPath = true;
                            onPathSymbolInstance->pinPointOnPath = pinPoint;
                            additionalSymbolInstance.reset(onPathSymbolInstance);
                        }

                        if (additionalSymbolInstance)
                            additionalGroupInstance->symbols.insert(symbol, qMove(additionalSymbolInstance));
                    }

                    group->additionalInstances.push_back(qMove(additionalGroupInstance));
                }
            }

            // This group needs intersection check inside group
            group->intersectionProcessingMode |= MapSymbolsGroup::IntersectionProcessingModeFlag::CheckIntersectionsWithinGroup;

            // Finally there's no need in original, so turn it off
            group->additionalInstancesDiscardOriginal = true;
        }

        // Configure group
        if (!group->symbols.isEmpty())
        {
            if (hasAtLeastOneSimpleBillboard && !(hasAtLeastOneOnPath || hasAtLeastOneAlongPathBillboard))
            {
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowNoneIfIconIsNotShown;
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnythingUntilFirstGap;
            }
            else if (!hasAtLeastOneSimpleBillboard && (hasAtLeastOneOnPath || hasAtLeastOneAlongPathBillboard))
            {
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnything;
            }
            else
            {
                LogPrintf(LogSeverityLevel::Error,
                    "%s produced incompatible set of map symbols to compute presentation mode",
                    qPrintable(group->toString()));
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnything;
            }
        }

        // Add constructed group to output
        symbolsGroups.push_back(qMove(group));
    }

    // Create output tile
    outTiledData.reset(new MapObjectsSymbolsProvider::Data(
        tileId,
        zoom,
        symbolsGroups,
        primitivesTile,
        new RetainableCacheMetadata(primitivesTile->retainableCacheMetadata)));

    return true;
}

//#define OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION 1
#ifndef OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION
#   define OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION 0
#endif // !defined(OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION)

QList< QList<OsmAnd::MapObjectsSymbolsProvider_P::ComputedPinPoint> > OsmAnd::MapObjectsSymbolsProvider_P::computePinPoints(
    const QVector<PointI>& path31,
    const float globalLeftPaddingInPixels,
    const float globalRightPaddingInPixels,
    const float globalSpacingBetweenBlocksInPixels,
    const QList<SymbolForPinPointsComputation>& symbolsForPinPointsComputation,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const ZoomLevel neededZoom) const
{
    QList< QList<ComputedPinPoint> > computedPinPointsByLayer;

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
    //

    // Step 0. Initial checks
    if (symbolsForPinPointsComputation.isEmpty())
        return computedPinPointsByLayer;

    // Step 1. Get scale factor from 31 to pixels for minZoom.
    // Length on path in pixels depends on tile size in pixels, and density
    const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - minZoom));
    const auto from31toPixelsScale = static_cast<double>(owner->referenceTileSizeOnScreenInPixels) / tileSize31;

    // Step 2. Compute path length, path segments length (in 31 and in pixels)
    const auto pathSize = path31.size();
    const auto pathSegmentsCount = pathSize - 1;
    float pathLengthInPixels = 0.0f;
    QVector<float> pathSegmentsLengthInPixelsOnBaseZoom(pathSegmentsCount);
    auto pPathSegmentLengthInPixelsOnBaseZoom = pathSegmentsLengthInPixelsOnBaseZoom.data();
    double pathLength31 = 0.0;
    QVector<double> pathSegmentsLength31(pathSegmentsCount);
    auto pPathSegmentLength31 = pathSegmentsLength31.data();
    auto pPoint31 = path31.constData();
    auto pPrevPoint31 = pPoint31++;
    auto basePathPointIndex = 0;
    auto globalPaddingFromBasePathPoint = 0.0f;
    bool capturedBasePathPointIndex = false;
    for (auto segmentIdx = 0; segmentIdx < pathSegmentsCount; segmentIdx++)
    {
        const auto segmentLength31 = qSqrt((*(pPoint31++) - *(pPrevPoint31++)).squareNorm());
        *(pPathSegmentLength31++) = segmentLength31;
        pathLength31 += segmentLength31;

        const auto segmentLengthInPixels = segmentLength31 * from31toPixelsScale;
        *(pPathSegmentLengthInPixelsOnBaseZoom++) = segmentLengthInPixels;
        pathLengthInPixels += segmentLengthInPixels;

        if (pathLength31 > globalLeftPaddingInPixels && !capturedBasePathPointIndex)
        {
            basePathPointIndex = segmentIdx;
            if (!qFuzzyIsNull(globalLeftPaddingInPixels))
                globalPaddingFromBasePathPoint = globalLeftPaddingInPixels - (pathLengthInPixels - segmentLengthInPixels);

            capturedBasePathPointIndex = true;
        }
    }
    const auto usablePathLengthInPixels = pathLengthInPixels - globalLeftPaddingInPixels - globalRightPaddingInPixels;
    if (usablePathLengthInPixels <= 0.0f)
        return computedPinPointsByLayer;

    // Step 3. Compute total width of all symbols requested. This will be the block width.
    const auto symbolsCount = symbolsForPinPointsComputation.size();
    QVector<float> symbolsFullSizesInPixels(symbolsCount);
    auto pSymbolFullSizeInPixels = symbolsFullSizesInPixels.data();
    float blockWidthWithoutSpacing = 0.0f;
    for (const auto& symbolForPinPointsComputation : constOf(symbolsForPinPointsComputation))
    {
        auto symbolWidth = 0.0f;
        symbolWidth += symbolForPinPointsComputation.leftPaddingInPixels;
        symbolWidth += symbolForPinPointsComputation.widthInPixels;
        symbolWidth += symbolForPinPointsComputation.rightPaddingInPixels;
        *(pSymbolFullSizeInPixels++) = symbolWidth;
        blockWidthWithoutSpacing += symbolWidth;
    }
    if (symbolsForPinPointsComputation.isEmpty() || qFuzzyIsNull(blockWidthWithoutSpacing))
        return computedPinPointsByLayer;
    const auto blockWidthWithSpacing = blockWidthWithoutSpacing + globalSpacingBetweenBlocksInPixels;

    // Step 4. Process values for base zoom level
    const auto lengthOfPathInPixelsOnBaseZoom = usablePathLengthInPixels;

    // Step 5. Process by zoom levels
    auto lengthOfPathInPixelsOnCurrentZoom = lengthOfPathInPixelsOnBaseZoom;
    auto pathSegmentsLengthInPixelsOnCurrentZoom = detachedOf(pathSegmentsLengthInPixelsOnBaseZoom);
    auto totalNumberOfCompleteBlocks = 0;
    auto remainingPathLengthOnPrevZoom = 0.0f;
    auto kOffsetToFirstBlockOnPrevZoom = 0.0f;
    auto globalPaddingInPixelsFromBasePathPointOnCurrentZoom = globalPaddingFromBasePathPoint;
    for (int currentZoomLevel = minZoom; currentZoomLevel <= maxZoom; currentZoomLevel++)
    {
        // Compute how many new blocks will fit, where and how to place them
        auto blocksToInstantiate = 0;
        auto kOffsetToFirstNewBlockOnCurrentZoom = 0.0f;
        auto kOffsetToFirstPresentBlockOnCurrentZoom = 0.0f;
        auto fullSizeOfSymbolsThatFit = 0.0f; // This is used only in case even 1 block doesn't fit
        auto numberOfSymbolsThatFit = 0; // This is used only in case even 1 block doesn't fit
        if (totalNumberOfCompleteBlocks == 0)
        {
            if (lengthOfPathInPixelsOnCurrentZoom >= blockWidthWithoutSpacing)
            {
                const auto numberOfBlocksThatFit = lengthOfPathInPixelsOnCurrentZoom / blockWidthWithoutSpacing;
                blocksToInstantiate = qMax(qFloor(numberOfBlocksThatFit), 1);
                kOffsetToFirstNewBlockOnCurrentZoom = (numberOfBlocksThatFit - static_cast<int>(numberOfBlocksThatFit)) / 2.0f;
                kOffsetToFirstPresentBlockOnCurrentZoom = -1.0f;
                //if (lengthOfPathInPixelsOnCurrentZoom >= blockWidth + globalSpacingBetweenBlocksInPixels + blockWidth)
                //{
                //    // If more than 2 blocks + spacing between them
                //    const auto numberOfBlocksThatFit = lengthOfPathInPixelsOnCurrentZoom / blockWidthWithSpacing;
                //    blocksToInstantiate = qMax(qFloor(numberOfBlocksThatFit), 1);
                //    kOffsetToFirstNewBlockOnCurrentZoom = (numberOfBlocksThatFit - static_cast<int>(numberOfBlocksThatFit)) / 2.0f;
                //}
                //else
                //{
                //    // If only 1 block (with or without spacing)
                //    blocksToInstantiate = 1;
                //    kOffsetToFirstNewBlockOnCurrentZoom = (numberOfBlocksThatFit - static_cast<int>(numberOfBlocksThatFit)) / 2.0f;
                //}
                //kOffsetToFirstPresentBlockOnCurrentZoom = -1.0f;
            }
            else
            {
                blocksToInstantiate = 0;
                for (auto symbolIdx = 0; symbolIdx < symbolsCount; symbolIdx++)
                {
                    const auto& symbolFullSize = symbolsFullSizesInPixels[symbolIdx];

                    if (fullSizeOfSymbolsThatFit + symbolFullSize > lengthOfPathInPixelsOnCurrentZoom)
                        break;

                    fullSizeOfSymbolsThatFit += symbolFullSize;
                    numberOfSymbolsThatFit++;
                }

                // Actually offset to incomplete block
                kOffsetToFirstNewBlockOnCurrentZoom = ((lengthOfPathInPixelsOnCurrentZoom - fullSizeOfSymbolsThatFit) / blockWidthWithoutSpacing) / 2.0f;
                kOffsetToFirstPresentBlockOnCurrentZoom = -1.0f;
            }
        }
        else
        {
            kOffsetToFirstPresentBlockOnCurrentZoom = kOffsetToFirstBlockOnPrevZoom * 2.0f + 0.5f; // 0.5f represents shift of present instance due to x2 scale-up
            blocksToInstantiate = qRound((lengthOfPathInPixelsOnCurrentZoom / blockWidthWithoutSpacing) - 2.0f * kOffsetToFirstPresentBlockOnCurrentZoom) - totalNumberOfCompleteBlocks;
            assert(blocksToInstantiate >= 0);
            if (kOffsetToFirstPresentBlockOnCurrentZoom > 1.0f)
            {
                // Insert new block before present. This will add 2 extra blocks on both sides
                kOffsetToFirstNewBlockOnCurrentZoom = kOffsetToFirstPresentBlockOnCurrentZoom - 1.0f;
                blocksToInstantiate += 2;
            }
            else
            {
                // Insert new block after present
                kOffsetToFirstNewBlockOnCurrentZoom = kOffsetToFirstPresentBlockOnCurrentZoom + 1.0f;
            }
            if (blocksToInstantiate == 0)
                kOffsetToFirstNewBlockOnCurrentZoom = -1.0f;
        }
        const auto remainingPathLengthOnCurrentZoom = lengthOfPathInPixelsOnCurrentZoom - (totalNumberOfCompleteBlocks + blocksToInstantiate) * blockWidthWithoutSpacing;
        const auto offsetToFirstNewBlockInPixels = kOffsetToFirstNewBlockOnCurrentZoom * blockWidthWithoutSpacing;
        const auto eachNewBlockAfterFirstOffsetInPixels = (totalNumberOfCompleteBlocks > 0 ? 2.0f : 1.0f) * blockWidthWithoutSpacing;

#if OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION
        LogPrintf(LogSeverityLevel::Debug,
            "%d->%f/(%d of [%d;%d]):"
            "\n\ttotalNumberOfCompleteBlocks=%d"
            "\n\tremainingPathLengthOnPrevZoom=%f"
            "\n\tkOffsetToFirstBlockOnPrevZoom=%f"
            "\n\tblocksToInstantiate=%d"
            "\n\tkOffsetToFirstNewBlockOnCurrentZoom=%f"
            "\n\tkOffsetToFirstPresentBlockOnCurrentZoom=%f"
            "\n\tfullSizeOfSymbolsThatFit=%f"
            "\n\tnumberOfSymbolsThatFit=%d"
            "\n\tlengthOfPathInPixelsOnCurrentZoom=%f"
            "\n\tblockWidth=%f"
            /*"\n\t=%f"*/,
            symbolsForPinPointsComputation.size(),
            pathLength31,
            currentZoomLevel,
            minZoom,
            maxZoom,
            totalNumberOfCompleteBlocks,
            remainingPathLengthOnPrevZoom,
            kOffsetToFirstBlockOnPrevZoom,
            blocksToInstantiate,
            kOffsetToFirstNewBlockOnCurrentZoom,
            kOffsetToFirstPresentBlockOnCurrentZoom,
            fullSizeOfSymbolsThatFit,
            numberOfSymbolsThatFit,
            lengthOfPathInPixelsOnCurrentZoom,
            blockWidthWithoutSpacing);
#endif // OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION

        // Compute actual pin-points only for zoom levels less detained that needed, including needed
        if (currentZoomLevel <= neededZoom)
        {
            // In case at least 1 block fits, only complete blocks are being used.
            // Otherwise, plot only part of symbols (virtually, smaller block)
            if (blocksToInstantiate > 0)
            {
                QList<ComputedPinPoint> computedPinPoints;
                unsigned int scanOriginPathPointIndex = basePathPointIndex;
                float scanOriginPathPointOffsetInPixels = globalPaddingInPixelsFromBasePathPointOnCurrentZoom;
                for (auto blockIdx = 0; blockIdx < blocksToInstantiate; blockIdx++)
                {
                    // Compute base pin-point of block. Actually blocks get pinned,
                    // symbols inside block just receive offset from base pin-point
                    ComputedPinPoint computedBlockPinPoint;
                    unsigned int nextScanOriginPathPointIndex;
                    float nextScanOriginPathPointOffsetInPixels;
                    bool fits = computeBlockPinPoint(
                        pathSegmentsLengthInPixelsOnCurrentZoom,
                        lengthOfPathInPixelsOnCurrentZoom,
                        pathSegmentsLength31,
                        path31,
                        blockWidthWithoutSpacing,
                        offsetToFirstNewBlockInPixels + blockIdx * eachNewBlockAfterFirstOffsetInPixels,
                        scanOriginPathPointIndex,
                        scanOriginPathPointOffsetInPixels,
                        nextScanOriginPathPointIndex,
                        nextScanOriginPathPointOffsetInPixels,
                        computedBlockPinPoint);
                    if (!fits)
                    {
                        assert(false);
                        break;
                    }
                    scanOriginPathPointIndex = nextScanOriginPathPointIndex;
                    scanOriginPathPointOffsetInPixels = nextScanOriginPathPointOffsetInPixels;

                    float symbolPinPointOffset = -blockWidthWithoutSpacing / 2.0f;
                    for (auto symbolIdx = 0u; symbolIdx < symbolsCount; symbolIdx++)
                    {
                        const auto& symbol = symbolsForPinPointsComputation[symbolIdx];

                        const auto offsetFromBlockPinPointInPixelOnCurrentZoom = symbolPinPointOffset + symbol.leftPaddingInPixels + symbol.widthInPixels / 2.0f;
                        const auto zoomShiftScaleFactor = qPow(2.0f, neededZoom - currentZoomLevel);
                        const auto offsetFromBlockPinPointInPixelOnNeededZoom = offsetFromBlockPinPointInPixelOnCurrentZoom * zoomShiftScaleFactor;
                        symbolPinPointOffset += symbolsFullSizesInPixels[symbolIdx];

                        ComputedPinPoint computedSymbolPinPoint;
                        fits = computeSymbolPinPoint(
                            pathSegmentsLengthInPixelsOnCurrentZoom,
                            lengthOfPathInPixelsOnCurrentZoom,
                            pathSegmentsLength31,
                            path31,
                            computedBlockPinPoint,
                            zoomShiftScaleFactor,
                            offsetFromBlockPinPointInPixelOnNeededZoom,
                            computedSymbolPinPoint);
                        if (!fits)
                        {
                            assert(false);
                            continue;
                        }
                        
                        computedPinPoints.push_back(qMove(computedSymbolPinPoint));
                    }
                }
                computedPinPointsByLayer.push_front(qMove(computedPinPoints));
            }
            else  if (numberOfSymbolsThatFit > 0)
            {
                QList<ComputedPinPoint> computedPinPoints;
                unsigned int scanOriginPathPointIndex = basePathPointIndex;
                float scanOriginPathPointOffsetInPixels = globalPaddingInPixelsFromBasePathPointOnCurrentZoom;

                // Compute base pin-point of this virtual block. Actually blocks get pinned,
                // symbols inside block just receive offset from base pin-point
                ComputedPinPoint computedBlockPinPoint;
                unsigned int nextScanOriginPathPointIndex;
                float nextScanOriginPathPointOffsetInPixels;
                bool fits = computeBlockPinPoint(
                    pathSegmentsLengthInPixelsOnCurrentZoom,
                    lengthOfPathInPixelsOnCurrentZoom,
                    pathSegmentsLength31,
                    path31,
                    fullSizeOfSymbolsThatFit,
                    offsetToFirstNewBlockInPixels,
                    scanOriginPathPointIndex,
                    scanOriginPathPointOffsetInPixels,
                    nextScanOriginPathPointIndex,
                    nextScanOriginPathPointOffsetInPixels,
                    computedBlockPinPoint);
                if (!fits)
                {
                    assert(false);
                    break;
                }
                scanOriginPathPointIndex = nextScanOriginPathPointIndex;
                scanOriginPathPointOffsetInPixels = nextScanOriginPathPointOffsetInPixels;

                float symbolPinPointOffset = -fullSizeOfSymbolsThatFit / 2.0f;
                for (auto symbolIdx = 0u; symbolIdx < numberOfSymbolsThatFit; symbolIdx++)
                {
                    const auto& symbol = symbolsForPinPointsComputation[symbolIdx];

                    const auto offsetFromBlockPinPointInPixelOnCurrentZoom = symbolPinPointOffset + symbol.leftPaddingInPixels + symbol.widthInPixels / 2.0f;
                    const auto zoomShiftScaleFactor = qPow(2.0f, neededZoom - currentZoomLevel);
                    const auto offsetFromBlockPinPointInPixelOnNeededZoom = offsetFromBlockPinPointInPixelOnCurrentZoom * zoomShiftScaleFactor;
                    symbolPinPointOffset += symbolsFullSizesInPixels[symbolIdx];

                    ComputedPinPoint computedSymbolPinPoint;
                    fits = computeSymbolPinPoint(
                        pathSegmentsLengthInPixelsOnCurrentZoom,
                        lengthOfPathInPixelsOnCurrentZoom,
                        pathSegmentsLength31,
                        path31,
                        computedBlockPinPoint,
                        zoomShiftScaleFactor,
                        offsetFromBlockPinPointInPixelOnNeededZoom,
                        computedSymbolPinPoint);
                    if (!fits)
                    {
                        assert(false);
                        continue;
                    }
                    
                    computedPinPoints.push_back(qMove(computedSymbolPinPoint));
                }
                computedPinPointsByLayer.push_back(qMove(computedPinPoints));
            }
        }

        // Move to next zoom level
        lengthOfPathInPixelsOnCurrentZoom *= 2.0f;
        for (auto& pathSegmentLengthInPixelsOnCurrentZoom : pathSegmentsLengthInPixelsOnCurrentZoom)
            pathSegmentLengthInPixelsOnCurrentZoom *= 2.0f;
        globalPaddingInPixelsFromBasePathPointOnCurrentZoom *= 2.0f;
        remainingPathLengthOnPrevZoom = remainingPathLengthOnCurrentZoom;
        if (blocksToInstantiate > 0 || totalNumberOfCompleteBlocks > 0)
        {
            if (blocksToInstantiate > 0 && totalNumberOfCompleteBlocks > 0)
            {
                // In case new blocks added and there was previous blocks, use block offset closest to start
                kOffsetToFirstBlockOnPrevZoom = qMin(kOffsetToFirstNewBlockOnCurrentZoom, kOffsetToFirstPresentBlockOnCurrentZoom);
            }
            else if (blocksToInstantiate > 0 && totalNumberOfCompleteBlocks == 0)
            {
                // In case blocks were added and they are first ones, use first new block offset
                kOffsetToFirstBlockOnPrevZoom = kOffsetToFirstNewBlockOnCurrentZoom;
            }
            else if (blocksToInstantiate == 0 && totalNumberOfCompleteBlocks > 0)
            {
                // In case no blocks were added, but there was previously blocks, use offset to first present block
                kOffsetToFirstBlockOnPrevZoom = kOffsetToFirstPresentBlockOnCurrentZoom;
            }
        }
        totalNumberOfCompleteBlocks += blocksToInstantiate;
    }

    return computedPinPointsByLayer;
}

bool OsmAnd::MapObjectsSymbolsProvider_P::computeBlockPinPoint(
    const QVector<float>& pathSegmentsLengthInPixels,
    const float pathLengthInPixels,
    const QVector<double>& pathSegmentsLength31,
    const QVector<PointI>& path31,
    const float blockWidthInPixels,
    const float offsetFromPathStartInPixels,
    const unsigned int scanOriginPathPointIndex,
    const float scanOriginPathPointOffsetInPixels,
    unsigned int& outNextScanOriginPathPointIndex,
    float& outNextScanOriginPathPointOffsetInPixels,
    ComputedPinPoint& outComputedBlockPinPoint) const
{
    const auto pathSegmentsCount = pathSegmentsLengthInPixels.size();
    const auto startOffset = offsetFromPathStartInPixels;
    const auto pinPointOffset = startOffset + blockWidthInPixels / 2.0f;
    const auto endOffset = startOffset + blockWidthInPixels;
    if (endOffset > pathLengthInPixels)
        return false;

    bool blockPinPointFound = false;
    auto testPathPointIndex = scanOriginPathPointIndex;
    auto scannedLengthInPixels = scanOriginPathPointOffsetInPixels;
    while (scannedLengthInPixels <= pinPointOffset)
    {
        if (testPathPointIndex >= pathSegmentsCount)
        {
            assert(false);
            return false;
        }
        const auto& segmentLengthInPixels = pathSegmentsLengthInPixels[testPathPointIndex];
        if (scannedLengthInPixels + segmentLengthInPixels >= pinPointOffset)
        {
            const auto nOffsetFromPoint = (pinPointOffset - scannedLengthInPixels) / segmentLengthInPixels;
            const auto& segmentStartPoint31 = path31[testPathPointIndex + 0];
            const auto& segmentEndPoint31 = path31[testPathPointIndex + 1];
            const auto& vSegment31 = segmentEndPoint31 - segmentStartPoint31;

            // Compute block pin-point
            outComputedBlockPinPoint.point31 = segmentStartPoint31 + PointI(PointD(vSegment31) * nOffsetFromPoint);
            outComputedBlockPinPoint.basePathPointIndex = testPathPointIndex;
            outComputedBlockPinPoint.offsetFromBasePathPoint31 = pathSegmentsLength31[testPathPointIndex] * nOffsetFromPoint;
            outComputedBlockPinPoint.normalizedOffsetFromBasePathPoint = nOffsetFromPoint;
            blockPinPointFound = true;

            outNextScanOriginPathPointIndex = testPathPointIndex;
            outNextScanOriginPathPointOffsetInPixels = scannedLengthInPixels;
            break;
        }
        scannedLengthInPixels += segmentLengthInPixels;
        testPathPointIndex++;
    }
    while (scannedLengthInPixels < endOffset)
    {
        if (testPathPointIndex >= pathSegmentsCount)
        {
            assert(false);
            return false;
        }
        const auto& segmentLengthInPixels = pathSegmentsLengthInPixels[testPathPointIndex];
        if (scannedLengthInPixels + segmentLengthInPixels > endOffset)
        {
            outNextScanOriginPathPointIndex = testPathPointIndex;
            outNextScanOriginPathPointOffsetInPixels = scannedLengthInPixels;
            break;
        }
        scannedLengthInPixels += segmentLengthInPixels;
        testPathPointIndex++;
    }

    return blockPinPointFound;
}

bool OsmAnd::MapObjectsSymbolsProvider_P::computeSymbolPinPoint(
    const QVector<float>& pathSegmentsLengthInPixels,
    const float pathLengthInPixels,
    const QVector<double>& pathSegmentsLength31,
    const QVector<PointI>& path31,
    const ComputedPinPoint& blockPinPoint,
    const float neededZoomPixelScaleFactor,
    const float offsetFromBlockPinPointInPixelsOnNeededZoom,
    ComputedPinPoint& outComputedSymbolPinPoint) const
{
    const auto pathSegmentsCount = pathSegmentsLengthInPixels.size();
    const auto offsetFromOriginPathPoint =
        (pathSegmentsLengthInPixels[blockPinPoint.basePathPointIndex] * blockPinPoint.normalizedOffsetFromBasePathPoint * neededZoomPixelScaleFactor) +
        offsetFromBlockPinPointInPixelsOnNeededZoom;

    if (offsetFromOriginPathPoint >= 0.0f)
    {
        // In case start point is located after origin point ('on the right'), usual search is used

        auto testPathPointIndex = blockPinPoint.basePathPointIndex;
        auto scannedLength = 0.0f;
        while (scannedLength <= offsetFromOriginPathPoint)
        {
            if (testPathPointIndex >= pathSegmentsCount)
                return false;
            const auto segmentLength = pathSegmentsLengthInPixels[testPathPointIndex] * neededZoomPixelScaleFactor;
            if (scannedLength + segmentLength >= offsetFromOriginPathPoint)
            {
                const auto pathPointIndex = testPathPointIndex;
                const auto offsetFromPathPoint = offsetFromOriginPathPoint - scannedLength;
                const auto nOffsetFromPoint = offsetFromPathPoint / segmentLength;
                const auto& segmentStartPoint31 = path31[testPathPointIndex + 0];
                const auto& segmentEndPoint31 = path31[testPathPointIndex + 1];
                const auto& vSegment31 = segmentEndPoint31 - segmentStartPoint31;

                outComputedSymbolPinPoint.point31 = segmentStartPoint31 + PointI(PointD(vSegment31) * nOffsetFromPoint);
                outComputedSymbolPinPoint.basePathPointIndex = pathPointIndex;
                outComputedSymbolPinPoint.offsetFromBasePathPoint31 = pathSegmentsLength31[testPathPointIndex] * nOffsetFromPoint;
                outComputedSymbolPinPoint.normalizedOffsetFromBasePathPoint = nOffsetFromPoint;

                return true;
            }
            scannedLength += segmentLength;
            testPathPointIndex++;
        }
    }
    else
    {
        // In case start point is located before origin point ('on the left'), reversed search is used
        if (blockPinPoint.basePathPointIndex == 0)
            return false;

        auto testPathPointIndex = blockPinPoint.basePathPointIndex - 1;
        auto scannedLength = 0.0f;
        while (scannedLength >= offsetFromOriginPathPoint)
        {
            const auto& segmentLength = pathSegmentsLengthInPixels[testPathPointIndex] * neededZoomPixelScaleFactor;
            if (scannedLength - segmentLength <= offsetFromOriginPathPoint)
            {
                const auto pathPointIndex = testPathPointIndex;
                const auto offsetFromPathPoint = segmentLength + (offsetFromOriginPathPoint - scannedLength);
                const auto nOffsetFromPoint = offsetFromPathPoint / segmentLength;
                const auto& segmentStartPoint31 = path31[testPathPointIndex + 0];
                const auto& segmentEndPoint31 = path31[testPathPointIndex + 1];
                const auto& vSegment31 = segmentEndPoint31 - segmentStartPoint31;

                outComputedSymbolPinPoint.point31 = segmentStartPoint31 + PointI(PointD(vSegment31) * nOffsetFromPoint);
                outComputedSymbolPinPoint.basePathPointIndex = pathPointIndex;
                outComputedSymbolPinPoint.offsetFromBasePathPoint31 = pathSegmentsLength31[testPathPointIndex] * nOffsetFromPoint;
                outComputedSymbolPinPoint.normalizedOffsetFromBasePathPoint = nOffsetFromPoint;

                return true;
            }
            scannedLength -= segmentLength;
            if (testPathPointIndex == 0)
                return false;
            testPathPointIndex--;
        }
    }

    return false;
}

OsmAnd::MapObjectsSymbolsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata_)
    : binaryMapPrimitivesRetainableCacheMetadata(binaryMapPrimitivesRetainableCacheMetadata_)
{
}

OsmAnd::MapObjectsSymbolsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
