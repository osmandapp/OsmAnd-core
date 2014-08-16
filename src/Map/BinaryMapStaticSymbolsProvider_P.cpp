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
            //////////////////////////////////////////////////////////////////////////
           /* {
                if ((mapObject->id >> 1) == 28191391)
                {
                    int i = 5;
                }
            }*/
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
                    const auto outerCircleRadius = 0.5f * qSqrt(2 * maxSize * maxSize);
                    symbolsForComputation.push_back({ 0, 2.0f * outerCircleRadius, 0 });
                }
                else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathMapSymbol>(symbol))
                {
                    symbolsForComputation.push_back({ 0, onPathSymbol->size.x, 0 });
                }
            }

            const auto computedPinPointsByLayer = computePinPoints(
                mapObject->points31,
                0.0f,
                0.0f,
                symbolsForComputation,
                mapObject->level->minZoom,
                mapObject->level->maxZoom,
                zoom);

            // After pin-points were computed, assign them to symbols in the same order
            QHash< std::shared_ptr<MapSymbol>, QList<std::shared_ptr<MapSymbol>> > extraSymbolInstances;
            for (const auto& computedPinPoints : constOf(computedPinPointsByLayer))
            {
                auto citComputedPinPoint = computedPinPoints.cbegin();
                const auto citComputedPinPointsEnd = computedPinPoints.cend();
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
                        extraSymbolInstance->position31 = computedPinPoint.point31;
                        extraSymbolInstance->offset = billboardSymbol->offset;

                        extraSymbolInstances[billboardSymbol].push_back(extraSymbolInstance);
                    }
                    else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathMapSymbol>(symbol))
                    {
                        OnPathMapSymbol::PinPoint pinPoint;
                        pinPoint.point31 = computedPinPoint.point31;
                        pinPoint.basePathPointIndex = computedPinPoint.basePathPointIndex;
                        pinPoint.offsetFromBasePathPoint31 = computedPinPoint.offsetFromBasePathPoint31;
                        pinPoint.normalizedOffsetFromBasePathPoint = computedPinPoint.normalizedOffsetFromBasePathPoint;

                        onPathSymbol->pinPoints.push_back(qMove(pinPoint));
                    }
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

#define OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION 1
#ifndef OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION
#   define OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION 0
#endif // !defined(OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION)

QList< QList<OsmAnd::BinaryMapStaticSymbolsProvider_P::ComputedPinPoint> > OsmAnd::BinaryMapStaticSymbolsProvider_P::computePinPoints(
    const QVector<PointI>& path31,
    const float globalLeftPaddingInPixels,
    const float globalRightPaddingInPixels,
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
    const auto from31toPixelsScale = static_cast<double>(owner->referenceTileSizeInPixels) / tileSize31;

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
    float blockWidth = 0.0f;
    for (const auto& symbolForPinPointsComputation : constOf(symbolsForPinPointsComputation))
    {
        auto symbolWidth = 0.0f;
        symbolWidth += symbolForPinPointsComputation.leftPaddingInPixels;
        symbolWidth += symbolForPinPointsComputation.widthInPixels;
        symbolWidth += symbolForPinPointsComputation.rightPaddingInPixels;
        *(pSymbolFullSizeInPixels++) = symbolWidth;
        blockWidth += symbolWidth;
    }
    if (symbolsForPinPointsComputation.isEmpty() || qFuzzyIsNull(blockWidth))
        return computedPinPointsByLayer;

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
            const auto numberOfBlocksThatFit = lengthOfPathInPixelsOnCurrentZoom / blockWidth;
            blocksToInstantiate = qFloor(numberOfBlocksThatFit);
            if (blocksToInstantiate > 0)
            {
                kOffsetToFirstNewBlockOnCurrentZoom = (numberOfBlocksThatFit - static_cast<int>(numberOfBlocksThatFit)) / 2.0f;
                kOffsetToFirstPresentBlockOnCurrentZoom = -1.0f;
            }
            else
            {
                for (auto symbolIdx = 0; symbolIdx < symbolsCount; symbolIdx++)
                {
                    const auto& symbolFullSize = symbolsFullSizesInPixels[symbolIdx];

                    if (fullSizeOfSymbolsThatFit + symbolFullSize > lengthOfPathInPixelsOnCurrentZoom)
                        break;

                    fullSizeOfSymbolsThatFit += symbolFullSize;
                    numberOfSymbolsThatFit++;
                }

                // Actually offset to incomplete block
                kOffsetToFirstNewBlockOnCurrentZoom = ((lengthOfPathInPixelsOnCurrentZoom - fullSizeOfSymbolsThatFit) / blockWidth) / 2.0f;
                kOffsetToFirstPresentBlockOnCurrentZoom = -1.0f;
            }
        }
        else
        {
            kOffsetToFirstPresentBlockOnCurrentZoom = kOffsetToFirstBlockOnPrevZoom * 2.0f + 0.5f; // 0.5f represents shift of present instance due to x2 scale-up
            blocksToInstantiate = qRound((lengthOfPathInPixelsOnCurrentZoom / blockWidth) - 2.0f * kOffsetToFirstPresentBlockOnCurrentZoom) - totalNumberOfCompleteBlocks;
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
        const auto remainingPathLengthOnCurrentZoom = lengthOfPathInPixelsOnCurrentZoom - (totalNumberOfCompleteBlocks + blocksToInstantiate) * blockWidth;
        const auto offsetToFirstNewBlockInPixels = kOffsetToFirstNewBlockOnCurrentZoom * blockWidth;
        const auto eachNewBlockAfterFirstOffsetInPixels = (totalNumberOfCompleteBlocks > 0 ? 2.0f : 1.0f) * blockWidth;

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
            blockWidth);
#endif // OSMAND_LOG_SYMBOLS_PIN_POINTS_COMPUTATION

        // Compute actual pin-points only for zoom levels less detained that needed, including needed + 1
        if (currentZoomLevel <= neededZoom + 1)
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
                        blockWidth,
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

                    float symbolPinPointOffset = -blockWidth / 2.0f;
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
                computedPinPointsByLayer.push_back(qMove(computedPinPoints));
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

bool OsmAnd::BinaryMapStaticSymbolsProvider_P::computeBlockPinPoint(
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

    auto testPathPointIndex = scanOriginPathPointIndex;
    auto scannedLengthInPixels = scanOriginPathPointOffsetInPixels;
    while (scannedLengthInPixels < pinPointOffset)
    {
        if (testPathPointIndex >= pathSegmentsCount)
        {
            assert(false);
            return false;
        }
        const auto& segmentLengthInPixels = pathSegmentsLengthInPixels[testPathPointIndex];
        if (scannedLengthInPixels + segmentLengthInPixels > pinPointOffset)
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

    return true;
}

bool OsmAnd::BinaryMapStaticSymbolsProvider_P::computeSymbolPinPoint(
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
        while (scannedLength < offsetFromOriginPathPoint)
        {
            if (testPathPointIndex >= pathSegmentsCount)
                return false;
            const auto segmentLength = pathSegmentsLengthInPixels[testPathPointIndex] * neededZoomPixelScaleFactor;
            if (scannedLength + segmentLength > offsetFromOriginPathPoint)
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
        while (scannedLength > offsetFromOriginPathPoint)
        {
            const auto& segmentLength = pathSegmentsLengthInPixels[testPathPointIndex] * neededZoomPixelScaleFactor;
            if (scannedLength - segmentLength < offsetFromOriginPathPoint)
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
