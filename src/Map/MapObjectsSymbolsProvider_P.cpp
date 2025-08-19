#include "MapObjectsSymbolsProvider_P.h"
#include "MapObjectsSymbolsProvider.h"

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "QtCommon.h"

#include "MapDataProviderHelpers.h"
#include "MapSymbolIntersectionClassesRegistry.h"
#include "MapPrimitivesProvider.h"
#include "MapPresentationEnvironment.h"
#include "MapPrimitiviser.h"
#include "SymbolRasterizer.h"
#include "BillboardRasterMapSymbol.h"
#include "OnPathRasterMapSymbol.h"
#include "MapObject.h"
#include "ObfMapObject.h"
#include "ObfMapSectionInfo.h"
#include "Utilities.h"
#include "Stopwatch.h"
#include "MapRendererPerformanceMetrics.h"

#define MAX_PATHS_TO_ATTACH 20
#define MAX_GAP_BETWEEN_PATHS 45
#define MAX_ANGLE_BETWEEN_VECTORS M_PI_2

#define BLOCK_LENGTH_COEFF 4.0f / 3.0f

OsmAnd::MapObjectsSymbolsProvider_P::MapObjectsSymbolsProvider_P(MapObjectsSymbolsProvider* owner_)
    : owner(owner_)
{
}

OsmAnd::MapObjectsSymbolsProvider_P::~MapObjectsSymbolsProvider_P()
{
}

bool OsmAnd::MapObjectsSymbolsProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& queryController = request_.queryController;

    if (queryController->isAborted())
        return false;

    if (pOutMetric)
        pOutMetric->reset();
    const auto& request = MapDataProviderHelpers::castRequest<MapObjectsSymbolsProvider::Request>(request_);
    const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);
    const auto bbox31 = owner->isMetaTiled() && request.zoom > ZoomLevel::MinZoomLevel
        ? Utilities::roundBoundingBox31(tileBBox31, static_cast<ZoomLevel>(request.zoom - 1))
        : tileBBox31;

    assert(owner->isMetaTiled() || !request.combineTilesData);

    const Stopwatch totalTimeStopwatch(pOutMetric != nullptr);
    const Stopwatch allocateTimeStopwatch(OsmAnd::isPerformanceMetricsEnabled());
    
    QVector<TileId> tilesIds;
    if (request.combineTilesData)
        tilesIds = Utilities::getAllMetaTileIds(request.tileId);
    else
        tilesIds.push_back(request.tileId);

    std::shared_ptr<MapPrimitivesProvider::Data> primitivesTile;
    MapPrimitiviser::SymbolsGroupsCollection primitivisedSymbolsGroups;
    std::shared_ptr<const MapPresentationEnvironment> mapPresentationEnvironment;
    PointD scaleDivisor31ToPixel;
    bool anyTilePrimitivesObtained = false;

    QSet<MapObject::SharingKey> addedObjectsSymbolsKeys;
    for (const auto tileId : tilesIds)
    {
        bool originalTile = tileId == request.tileId;

        const auto requestClone = request.clone();
        auto& tileRequest = *dynamic_cast<MapObjectsSymbolsProvider::Request*>(requestClone.get());
        tileRequest.tileId = tileId;

        std::shared_ptr<MapPrimitivesProvider::Data> tilePrimitives;
        owner->primitivesProvider->obtainTiledPrimitives(tileRequest, tilePrimitives); 

        if (queryController->isAborted())
            return false;

        if (tilePrimitives)
        {
            if (!anyTilePrimitivesObtained)
            {
                anyTilePrimitivesObtained = true;
                mapPresentationEnvironment = tilePrimitives->primitivisedObjects->mapPresentationEnvironment;
                scaleDivisor31ToPixel = tilePrimitives->primitivisedObjects->scaleDivisor31ToPixel;
            }

            const auto& symbolsGroups = tilePrimitives->primitivisedObjects->symbolsGroups;
            for (const auto& symbolsGroupEntry : rangeOf(constOf(symbolsGroups)))
            {
                const auto& mapObject = symbolsGroupEntry.key();
                const auto& symbolsGroup = symbolsGroupEntry.value();

                MapObject::SharingKey key;
                bool accept = false;
                if (owner->isMetaTiled())
                {
                    const bool hasOnPathSymbol = MapObjectsSymbolsProvider_P::hasOnPathSymbol(symbolsGroup);
                    accept = accept || request.combineTilesData && (originalTile || hasOnPathSymbol);
                    accept = accept || !request.combineTilesData && !hasOnPathSymbol;
                }
                else
                    accept = true;
                accept = accept && (!mapObject->obtainSharingKey(key) || !addedObjectsSymbolsKeys.contains(key));

                if (!accept)
                    continue;

                addedObjectsSymbolsKeys.insert(key);
                primitivisedSymbolsGroups.insert(mapObject, symbolsGroup);
            }
        }

        if (originalTile)
            primitivesTile = tilePrimitives;
    }

    if (queryController->isAborted())
        return false;

    // If tile has nothing to be rasterized, mark that data is not available for it
    if (!anyTilePrimitivesObtained || primitivisedSymbolsGroups.isEmpty())
    {
        // Mark tile as empty
        outData.reset();
        return true;
    }

    const float allocationTime = allocateTimeStopwatch.elapsed();

    if (OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().textStart(request.tileId);

    CombinePathsResult combinePathsResult;
    if (!owner->isMetaTiled() || request.combineTilesData)
        combinePathsResult = combineOnPathSymbols(
            bbox31,
            primitivisedSymbolsGroups,
            mapPresentationEnvironment,
            scaleDivisor31ToPixel,
            request.queryController);

    // Rasterize symbols and create symbols groups
    QList< std::shared_ptr<const SymbolRasterizer::RasterizedSymbolsGroup> > rasterizedSymbolsGroups;
    QHash< std::shared_ptr<const MapObject>, std::shared_ptr<MapObjectSymbolsGroup> > preallocatedSymbolsGroups;
    const auto filterCallback = request.filterCallback;
    const auto rasterizationFilter =
        [this, filterCallback, &preallocatedSymbolsGroups, &combinePathsResult]
        (const std::shared_ptr<const MapPrimitiviser::SymbolsGroup>& symbolsGroup) -> bool
        {
            const auto& mapObject = symbolsGroup->sourceObject;

            if (combinePathsResult.mapObjectToSkip.contains(mapObject))
                return false;

            const std::shared_ptr<MapObjectSymbolsGroup> preallocatedGroup(new MapObjectSymbolsGroup(mapObject));

            // Do not filter groups with on-path symbols
            if (hasOnPathSymbol(symbolsGroup) || !filterCallback || filterCallback(owner, preallocatedGroup))
            {
                preallocatedSymbolsGroups.insert(mapObject, qMove(preallocatedGroup));
                return true;
            }
            return false;
        };

    owner->symbolRasterizer->rasterize(
        primitivisedSymbolsGroups,
        mapPresentationEnvironment,
        rasterizedSymbolsGroups,
        rasterizationFilter,
        queryController);

    if (queryController->isAborted())
        return false;

    int spriteSymbols = 0;
    int onPathSymbols = 0;

    // Convert results
    auto& mapSymbolIntersectionClassesRegistry = MapSymbolIntersectionClassesRegistry::globalInstance();
    QList< std::shared_ptr<MapSymbolsGroup> > symbolsGroups;
    for (const auto& rasterizedGroup : constOf(rasterizedSymbolsGroups))
    {
        if (queryController->isAborted())
            return false;

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

        const auto& citCombinedPath = combinePathsResult.combinedPaths.constFind(mapObject);
        bool isCombined = citCombinedPath != combinePathsResult.combinedPaths.cend();
        auto areaIndex = isCombined ? -1 : mapObject->startReadingArea();
        if (areaIndex >= 0 && !mapObject->vapItems[areaIndex]->area31.contains(bbox31))
        {
            mapObject->stopReadingArea(areaIndex);
            areaIndex = -1;
        }
        auto resultPoints31 = isCombined ?
            *citCombinedPath : (areaIndex >= 0 ? mapObject->vapItems[areaIndex]->points31 : mapObject->points31);
        auto path31 = resultPoints31.size() < MIN_POINTS_TO_USE_SIMPLIFIED ? qMove(resultPoints31)
            : Utilities::simplifyPathOutsideBBox(resultPoints31, bbox31);

        if (areaIndex >= 0)
            mapObject->stopReadingArea(areaIndex);

        // Convert all symbols inside group
        bool hasAtLeastOneOnPath = false;
        bool hasAtLeastOneAlongPathBillboard = false;
        bool hasAtLeastOneSimpleBillboard = false;
        QMap< std::shared_ptr<MapSymbol>, PointI> additionalOffsets;

        for (const auto& rasterizedSymbol : constOf(rasterizedGroup->symbols))
        {
            assert(static_cast<bool>(rasterizedSymbol->image));

            std::shared_ptr<MapSymbol> symbol;
            if (const auto rasterizedSpriteSymbol = std::dynamic_pointer_cast<const SymbolRasterizer::RasterizedSpriteSymbol>(rasterizedSymbol))
            {
                spriteSymbols++;
                
                if (!hasAtLeastOneAlongPathBillboard && rasterizedSpriteSymbol->drawAlongPath)
                    hasAtLeastOneAlongPathBillboard = true;
                if (!hasAtLeastOneSimpleBillboard && !rasterizedSpriteSymbol->drawAlongPath)
                    hasAtLeastOneSimpleBillboard = true;

                const auto billboardRasterSymbol = new BillboardRasterMapSymbol(group);
                billboardRasterSymbol->order = rasterizedSpriteSymbol->order;
                billboardRasterSymbol->image = rasterizedSpriteSymbol->image;
                billboardRasterSymbol->size = PointI(rasterizedSpriteSymbol->image->width(), rasterizedSpriteSymbol->image->height());
                billboardRasterSymbol->content = rasterizedSpriteSymbol->content;
                billboardRasterSymbol->languageId = rasterizedSpriteSymbol->languageId;
                billboardRasterSymbol->minDistance = rasterizedSpriteSymbol->minDistance;
                billboardRasterSymbol->ignoreClick = rasterizedSpriteSymbol->ignoreClick;
                billboardRasterSymbol->position31 = rasterizedSpriteSymbol->location31;
                billboardRasterSymbol->offset = rasterizedSpriteSymbol->offset;
                billboardRasterSymbol->drawAlongPath = rasterizedSpriteSymbol->drawAlongPath;
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
                symbol.reset(billboardRasterSymbol);

                if (rasterizedSpriteSymbol->additionalOffset)
                    additionalOffsets[symbol] = *(rasterizedSpriteSymbol->additionalOffset);
            }
            else if (const auto rasterizedOnPathSymbol = std::dynamic_pointer_cast<const SymbolRasterizer::RasterizedOnPathSymbol>(rasterizedSymbol))
            {
                onPathSymbols++;
                hasAtLeastOneOnPath = true;

                const auto shareablePath31 = std::make_shared< QVector<PointI> >(path31);

                const auto onPathSymbol = new OnPathRasterMapSymbol(group);
                onPathSymbol->order = rasterizedOnPathSymbol->order;
                onPathSymbol->image = rasterizedOnPathSymbol->image;
                onPathSymbol->size = PointI(rasterizedOnPathSymbol->image->width(), rasterizedOnPathSymbol->image->height());
                onPathSymbol->content = rasterizedOnPathSymbol->content;
                onPathSymbol->languageId = rasterizedOnPathSymbol->languageId;
                onPathSymbol->minDistance = rasterizedOnPathSymbol->minDistance;
                onPathSymbol->ignoreClick = rasterizedOnPathSymbol->ignoreClick;
                onPathSymbol->shareablePath31 = shareablePath31;
                assert(shareablePath31->size() >= 2);
                onPathSymbol->glyphsWidth = rasterizedOnPathSymbol->glyphsWidth;
                for (const auto& intersectsWithClass : constOf(rasterizedOnPathSymbol->primitiveSymbol->intersectsWith))
                {
                    onPathSymbol->intersectsWithClasses.insert(mapSymbolIntersectionClassesRegistry.getOrRegisterClassIdByName(intersectsWithClass));
                }
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
        //  - Compute pin-points for all on-path symbols and along-path billboard symbols
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
            QVector<float> symbolsWidthsInPixels(group->symbols.size());
            auto pSymbolWidthInPixels = symbolsWidthsInPixels.data();
            for (const auto& symbol : constOf(group->symbols))
            {
                if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                {
                    if (!billboardSymbol->drawAlongPath)
                        continue;

                    // Get larger bbox, to take into account possible rotation
                    const auto maxSize = qMax(billboardSymbol->size.x, billboardSymbol->size.y);
                    *(pSymbolWidthInPixels++) = static_cast<float>(qSqrt(2 * maxSize * maxSize));
                }
                else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathRasterMapSymbol>(symbol))
                {
                    *(pSymbolWidthInPixels++) = static_cast<float>(onPathSymbol->size.x);
                }
            }

            const auto& env = owner->primitivesProvider->primitiviser->environment;
            // Temp fix to avoid intersections of street names vs road shields on map
            const auto symbolSpacingInPixels = env->getDefaultSymbolPathSpacing() * 20;

            const auto& windowSize = request.mapState.windowSize;
            const auto minWindowSize = qMin(windowSize.x, windowSize.y) * 0.8f;
            const auto minBlockSpacingInPixels = minWindowSize / 10.0f;
            const auto maxBlockSpacingInPixels = minWindowSize / 2.0f;
            const auto completeBlockLengthInPixels = std::accumulate(symbolsWidthsInPixels, 0.0f)
                + (symbolsWidthsInPixels.size() - 1) * symbolSpacingInPixels;
            const auto completeBlockSpacingInPixels =
                qBound(minBlockSpacingInPixels, minWindowSize - completeBlockLengthInPixels * BLOCK_LENGTH_COEFF, maxBlockSpacingInPixels);

            const auto computedPinPoints = computePinPoints(
                path31,
                env->getGlobalPathPadding(),
                completeBlockSpacingInPixels,
                symbolSpacingInPixels,
                symbolsWidthsInPixels,
                request.zoom);

            float actualBlockLengthInPixels;
            float actualBlockSpacingInPixels;
            if (computedPinPoints.size() >= group->symbols.size())
            {
                actualBlockLengthInPixels = completeBlockLengthInPixels;
                actualBlockSpacingInPixels = completeBlockSpacingInPixels;
            }
            else
            {
                const auto pFirstSymbolWidthInPixels = symbolsWidthsInPixels.constBegin();
                const auto symbolsInBlock = computedPinPoints.size();
                actualBlockLengthInPixels = std::accumulate(pFirstSymbolWidthInPixels, pFirstSymbolWidthInPixels + symbolsInBlock, 0.0f)
                    + (symbolsInBlock - 1) * symbolSpacingInPixels;
                actualBlockSpacingInPixels =
                    qBound(minBlockSpacingInPixels, minWindowSize - actualBlockLengthInPixels * BLOCK_LENGTH_COEFF, maxBlockSpacingInPixels);
            }    

            // After pin-points were computed, assign them to symbols in the same order
            auto citComputedPinPoint = computedPinPoints.cbegin();
            const auto citComputedPinPointsEnd = computedPinPoints.cend();
            while (citComputedPinPoint != citComputedPinPointsEnd)
            {
                // Construct new additional instance of group
                std::shared_ptr<MapSymbolsGroup::AdditionalInstance> additionalGroupInstance(
                    new MapSymbolsGroup::AdditionalInstance(group));
                    
                for (int symbolIndex = 0; symbolIndex < group->symbols.size(); symbolIndex++)
                {
                    const auto& symbol = group->symbols[symbolIndex];
                    const auto symbolWidthInPixels = symbolsWidthsInPixels[symbolIndex];
                    const auto symbolMinDistance = actualBlockSpacingInPixels + actualBlockLengthInPixels - symbolWidthInPixels;

                    // Stop in case no more pin-points left
                    if (citComputedPinPoint == citComputedPinPointsEnd)
                        break;
                    const auto& computedPinPoint = *(citComputedPinPoint++);

                    // Reject symbol with pin-point outside of bbox
                    if (!bbox31.contains(computedPinPoint.point31))
                        continue;

                    std::shared_ptr<MapSymbolsGroup::AdditionalSymbolInstanceParameters> additionalSymbolInstance;
                    if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                    {
                        if (!billboardSymbol->drawAlongPath)
                            continue;

                        const auto billboardSymbolInstance =
                            new MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters(additionalGroupInstance.get());
                        billboardSymbolInstance->overridesPosition31 = true;
                        billboardSymbolInstance->position31 = computedPinPoint.point31;
                        billboardSymbolInstance->overridesMinDistance = true;
                        billboardSymbolInstance->minDistance = symbolMinDistance;
                        additionalSymbolInstance.reset(billboardSymbolInstance);
                    }
                    else if (const auto onPathSymbol = std::dynamic_pointer_cast<OnPathRasterMapSymbol>(symbol))
                    {
                        IOnPathMapSymbol::PinPoint pinPoint;
                        pinPoint.point31 = computedPinPoint.point31;
                        pinPoint.basePathPointIndex = computedPinPoint.basePathPointIndex;
                        pinPoint.normalizedOffsetFromBasePathPoint = computedPinPoint.normalizedOffsetFromBasePathPoint;

                        const auto onPathSymbolInstance =
                            new MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters(additionalGroupInstance.get());
                        onPathSymbolInstance->overridesPinPointOnPath = true;
                        onPathSymbolInstance->pinPointOnPath = pinPoint;
                        onPathSymbolInstance->overridesMinDistance = true;
                        onPathSymbolInstance->minDistance = symbolMinDistance;
                        additionalSymbolInstance.reset(onPathSymbolInstance);
                    }

                    if (additionalSymbolInstance)
                        additionalGroupInstance->symbols.insert(symbol, qMove(additionalSymbolInstance));
                }

                // This group instance needs intersection check among its symbols
                additionalGroupInstance->intersectionProcessingMode |= MapSymbolsGroup::IntersectionProcessingModeFlag::CheckIntersectionsWithinGroup;

                additionalGroupInstance->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnything;

                group->additionalInstances.push_back(qMove(additionalGroupInstance));
            }

            // Now add all simple billboard symbols
            if (hasAtLeastOneSimpleBillboard)
            {
                const auto additionalGroupInstance = std::make_shared<MapSymbolsGroup::AdditionalInstance>(group);

                for (const auto& symbol : constOf(group->symbols))
                {
                    if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                    {
                        if (billboardSymbol->drawAlongPath)
                            continue;

                        const auto billboardSymbolInstance =
                            std::make_shared<MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters>(additionalGroupInstance.get());
                        additionalGroupInstance->symbols.insert(symbol, qMove(billboardSymbolInstance));
                    }
                }

                additionalGroupInstance->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowNoneIfIconIsNotShown;
                additionalGroupInstance->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnythingUntilFirstGap;

                group->additionalInstances.push_back(qMove(additionalGroupInstance));
            }

            // Finally there's no need in original, so turn it off
            group->additionalInstancesDiscardOriginal = true;
        }
        else if (!additionalOffsets.isEmpty())
        {
            for (const auto& itEntry : rangeOf(constOf(additionalOffsets)))
            {
                const auto& symbol = itEntry.key();
                const auto& offset = itEntry.value();

                if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                {
                    std::shared_ptr<MapSymbolsGroup::AdditionalInstance> additionalGroupInstance(
                        new MapSymbolsGroup::AdditionalInstance(group));
                    std::shared_ptr<MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters> billboardSymbolInstance(
                        new MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters(additionalGroupInstance.get()));
                    billboardSymbolInstance->discardableByAnotherInstances = true;
                    billboardSymbolInstance->overridesOffset = true;
                    billboardSymbolInstance->offset = offset;
                    
                    additionalGroupInstance->symbols.insert(symbol, qMove(billboardSymbolInstance));
                    additionalGroupInstance->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowNoneIfIconIsNotShown;
                    additionalGroupInstance->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnythingUntilFirstGap;
                    group->additionalInstances.push_back(additionalGroupInstance);
                }
            }
        }

        // Configure group
        if (!group->symbols.isEmpty() && !group->additionalInstancesDiscardOriginal)
        {
            if (hasAtLeastOneSimpleBillboard && !(hasAtLeastOneOnPath || hasAtLeastOneAlongPathBillboard))
            {
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowNoneIfIconIsNotShown;
                group->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAnythingUntilFirstGap;
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
    outData.reset(new MapObjectsSymbolsProvider::Data(
        request.tileId,
        request.zoom,
        symbolsGroups,
        primitivesTile,
        primitivesTile ? new RetainableCacheMetadata(primitivesTile->retainableCacheMetadata) : nullptr));

    if (OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().textFinish(request.tileId, request.zoom, spriteSymbols, onPathSymbols, allocationTime);

    return true;
}

// On-path text symbols (mainly, street names) are drawn on path,
// that comes from map object. In some cases there is not enough
// length of path to draw full text of symbol. That's why paths
// are combined, to fit full text of symbol 
OsmAnd::MapObjectsSymbolsProvider_P::CombinePathsResult OsmAnd::MapObjectsSymbolsProvider_P::combineOnPathSymbols(
    const AreaI& bbox31,
    const MapPrimitiviser::SymbolsGroupsCollection& symbolsGroups,
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const PointD& divisor31ToPixels,
    const std::shared_ptr<const IQueryController>& queryController) const
{
    // Firstly, collect all on-path text symbols with same content and order
    QHash< QString, QList < std::shared_ptr<СombinedPath> > > combinedPathsMap;
    for (const auto& mapObjectSymbolsGroup : rangeOf(constOf(symbolsGroups)))
    {
        if (queryController && queryController->isAborted())
            return CombinePathsResult();

        const auto& mapObject = mapObjectSymbolsGroup.key();
        const auto& symbolsGroup = mapObjectSymbolsGroup.value();

        QList< std::shared_ptr<const MapPrimitiviser::TextSymbol> > onPathTextSymbols;
        for (const auto& symbol : constOf(symbolsGroup->symbols))
        {
            const auto& textSymbol = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(symbol);
            if (textSymbol && textSymbol->drawOnPath && !textSymbol->value.isEmpty())
                onPathTextSymbols.push_back(textSymbol);
        }

        if (onPathTextSymbols.isEmpty())
            continue;

        // There shouldn't be two or more on-path text symbols in group. But if so, skip
        if (onPathTextSymbols.size() >= 2) 
            continue;

        const std::shared_ptr<СombinedPath> combinedPath(new СombinedPath(bbox31, mapObject, divisor31ToPixels));

        const auto& onPathTextSymbol = onPathTextSymbols.front();
        const QString key = onPathTextSymbol->value + QString::number(onPathTextSymbol->order);
        const auto itCombinedSymbolPathList = combinedPathsMap.find(key);

        if (itCombinedSymbolPathList == combinedPathsMap.end())
        {
            QList< std::shared_ptr<СombinedPath> > combinedSymbolPathList;
            combinedSymbolPathList.push_back(combinedPath);
            combinedPathsMap.insert(key, combinedSymbolPathList);
        }
        else
        {
            auto& combinedSymbolPathList = *itCombinedSymbolPathList;
            combinedSymbolPathList.push_back(combinedPath);
        }
    }

    if (combinedPathsMap.isEmpty())
        return CombinePathsResult();

    CombinePathsResult result;

    const auto maxGapBetweenPaths = MAX_GAP_BETWEEN_PATHS * env->displayDensityFactor;

    for (auto& pathsToCombine : combinedPathsMap)
    {
        if (queryController && queryController->isAborted())
            return CombinePathsResult();

        if (pathsToCombine.size() < 2)
            continue;

        bool iterateFromStart = true;
        for (auto attachedPaths = 0; attachedPaths < MAX_PATHS_TO_ATTACH && iterateFromStart; attachedPaths++)
        {
            if (queryController && queryController->isAborted())
                return CombinePathsResult();

            iterateFromStart = false;

            for (auto pathIndex = 0; pathIndex < pathsToCombine.size() && !iterateFromStart; pathIndex++)
            {
                if (queryController && queryController->isAborted())
                    return CombinePathsResult();

                auto& path = pathsToCombine[pathIndex];

                if (path->isAttachedToAnotherPath())
                    continue;

                auto minGapBetweenPaths = maxGapBetweenPaths;
                std::shared_ptr<СombinedPath> mainPath;
                std::shared_ptr<СombinedPath> pathToAttach;

                // Find path with min distance to other path
                for (auto otherPathIndex = pathIndex + 1; otherPathIndex < pathsToCombine.size(); otherPathIndex++)
                {
                    auto& otherPath = pathsToCombine[otherPathIndex];

                    if (otherPath->isAttachedToAnotherPath())
                        continue;

                    // Try to connect 1st path to 2nd and 2nd to 1st, choose best connection
                    float gapBetweenPaths = 0.0f;
                    if (otherPath->isAttachAllowed(path, minGapBetweenPaths, gapBetweenPaths))
                    {
                        minGapBetweenPaths = gapBetweenPaths;
                        mainPath = otherPath;
                        pathToAttach = path;
                    }
                    if (path->isAttachAllowed(otherPath, minGapBetweenPaths, gapBetweenPaths))
                    {
                        minGapBetweenPaths = gapBetweenPaths;
                        mainPath = path;
                        pathToAttach = otherPath;
                    }
                }

                // If close enough path was found, connect paths
                if (mainPath && pathToAttach)
                {
                    iterateFromStart = true;
                    mainPath->attachPath(pathToAttach);
                }
            }
        }

        for (const auto& path : constOf(pathsToCombine))
        {
            if (path->isAttachedToAnotherPath())
                result.mapObjectToSkip.push_back(path->mapObject);
            else if (path->isCombined())
                result.combinedPaths.insert(path->mapObject, path->getPoints());
        }
    }

    return result;
}

QList<OsmAnd::MapObjectsSymbolsProvider_P::ComputedPinPoint> OsmAnd::MapObjectsSymbolsProvider_P::computePinPoints(
    const QVector<PointI>& path31,
    const float globalPaddingInPixels,
    const float blockSpacingInPixels,
    const float symbolSpacingInPixels,
    const QVector<float>& symbolsWidthsInPixels,
    const ZoomLevel neededZoom) const
{
    QList<ComputedPinPoint> computedPinPoints;

    if (symbolsWidthsInPixels.isEmpty())
        return computedPinPoints;

    const auto blockLengthInPixels =
        std::accumulate(symbolsWidthsInPixels, 0.0f) + (symbolsWidthsInPixels.size() - 1) * symbolSpacingInPixels;
    const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - neededZoom));
    const auto from31toPixelsScale = static_cast<double>(owner->referenceTileSizeOnScreenInPixels) / tileSize31;

    const auto symbolsCount = symbolsWidthsInPixels.count();
    const auto pathSize = path31.size();
    const auto pathSegmentsCount = pathSize - 1;

    double pathLength31 = 0.0;
    QVector<double> pathSegmentsLength31(pathSegmentsCount);
    auto pPathSegmentLength31 = pathSegmentsLength31.data();

    float pathLengthInPixels = 0.0f;
    QVector<float> pathSegmentsLengthInPixels(pathSegmentsCount);
    auto pPathSegmentLengthInPixels = pathSegmentsLengthInPixels.data();
    
    auto pPoint31 = path31.constData();
    auto pPrevPoint31 = pPoint31++;
    for (auto segmentIdx = 0; segmentIdx < pathSegmentsCount; segmentIdx++)
    {
        const auto segmentLength31 = qSqrt((*(pPoint31++) - *(pPrevPoint31++)).squareNorm());
        *(pPathSegmentLength31++) = segmentLength31;
        pathLength31 += segmentLength31;

        const auto segmentLengthInPixels = segmentLength31 * from31toPixelsScale;
        *(pPathSegmentLengthInPixels++) = segmentLengthInPixels;
        pathLengthInPixels += segmentLengthInPixels;
    }

    QVector<float> symbolsWidthsN(symbolsCount);
    auto pSymbolWidthN = symbolsWidthsN.data();
    auto pSymbolWidth = symbolsWidthsInPixels.constData();
    for (auto symbolIdx = 0; symbolIdx < symbolsCount; symbolIdx++)
        *(pSymbolWidthN++) = *(pSymbolWidth++) / pathLengthInPixels;
    const auto symbolSpacingN = symbolSpacingInPixels / pathLengthInPixels;
    const auto blockLengthN = blockLengthInPixels / pathLengthInPixels;
    QVector<float> pathSegmentsLengthN(pathSegmentsCount);
    auto pPathSegmentLengthN = pathSegmentsLengthN.data();
    pPathSegmentLengthInPixels = pathSegmentsLengthInPixels.data();
    for (auto pathSegmentIdx = 0; pathSegmentIdx < pathSegmentsCount; pathSegmentIdx++)
        *(pPathSegmentLengthN++) = *(pPathSegmentLengthInPixels++) / pathLengthInPixels;

    auto blockPinPoints = Utilities::calculateItemPointsOnPath(
        pathLengthInPixels,
        blockLengthInPixels,
        globalPaddingInPixels,
        blockSpacingInPixels);

    if (blockPinPoints.isEmpty())
    {
        const auto usablePathLengthInPixels = pathLengthInPixels - 2.0f * globalPaddingInPixels;
        auto fittedSymbolsCount = 0;
        auto fittedSymbolsWidth = 0.0f;
        while (fittedSymbolsWidth < usablePathLengthInPixels && fittedSymbolsCount < symbolsCount)
        {
            const auto symbolIdx = fittedSymbolsCount;
            const auto symbolWidth = symbolsWidthsInPixels[symbolIdx];
            if (fittedSymbolsWidth + symbolWidth > usablePathLengthInPixels)
                break;

            fittedSymbolsWidth += symbolWidth + symbolSpacingInPixels;
            if (fittedSymbolsCount < symbolsCount - 1)
            {
                fittedSymbolsCount++;
            }
        }
        if (fittedSymbolsCount == 0)
            return computedPinPoints;

        fittedSymbolsWidth -= symbolSpacingInPixels;
        auto nextSymbolStartN = 0.5f - 0.5f * fittedSymbolsWidth / pathLengthInPixels;
        computeSymbolsPinPoints(
            symbolsWidthsN,
            fittedSymbolsCount,
            nextSymbolStartN,
            pathSegmentsLengthN,
            path31,
            symbolSpacingN,
            computedPinPoints);
        
        return computedPinPoints;
    }

    const auto globalPaddingN = globalPaddingInPixels / pathLengthInPixels;
    const auto blockSpacingN = blockSpacingInPixels / pathLengthInPixels;

    // Provide more potential pin-points than in standard pin-points positioning
    int power = static_cast<int>(std::log2f(blockPinPoints.size() + 1)) + 3;
    int segments = 1 << power;
    int middleIndex = segments >> 1;

    // Try to position pin-points more tightly and less evenly (but still on negative powers of 2)
    QList<Utilities::ItemPointOnPath> alternativeBlockPinPoints;
    alternativeBlockPinPoints.push_back(blockPinPoints.first());
    for (int offset = 1; offset < middleIndex - 1; offset++)
    {
        const auto segmentIndex = middleIndex + offset;
        const auto blockCenterN = segmentIndex / static_cast<float>(segments);

        // There is no more space on path, finish positioning
        if (blockCenterN + blockLengthN / 2.0f + globalPaddingN > 1.0f)
            break;

        // Add pin-points if they are sufficiently faw away from previously added pin-points
        const auto lastBlockCenterN = alternativeBlockPinPoints.back().itemCenterN;
        const auto gap = blockCenterN - lastBlockCenterN - blockLengthN;
        if (gap >= blockSpacingN)
        {
            const auto denominator = segments / Utilities::commonDivisor(segmentIndex, segments);
            const auto priority = static_cast<int>(std::log2l(denominator) - 1);

            alternativeBlockPinPoints.push_back({priority, (1.0f - blockCenterN) * pathLengthInPixels, 1.0f - blockCenterN});
            alternativeBlockPinPoints.push_back({priority, blockCenterN * pathLengthInPixels, blockCenterN});
        }
    }

    if (alternativeBlockPinPoints.size() > blockPinPoints.size())
        blockPinPoints = alternativeBlockPinPoints;

    std::sort(blockPinPoints, Utilities::ItemPointOnPath::PriorityComparator());
    for (const auto& blockPinPoint : constOf(blockPinPoints))
    {
        const auto blockStartN = blockPinPoint.itemCenterN - 0.5f * blockLengthN;

        computeSymbolsPinPoints(
            symbolsWidthsN,
            symbolsCount,
            blockStartN,
            pathSegmentsLengthN,
            path31,
            symbolSpacingN,
            computedPinPoints);
    }

    return computedPinPoints;
}

void OsmAnd::MapObjectsSymbolsProvider_P::computeSymbolsPinPoints(
    const QVector<float>& symbolsWidthsN,
    const int symbolsCount,
    float nextSymbolStartN,
    const QVector<float>& pathSegmentsLengthN,
    const QVector<PointI>& path31,
    const float symbolSpacingN,
    QList<ComputedPinPoint> &outComputedPinPoints) const
{
    auto pSymbolWidthN = symbolsWidthsN.constData();
    for (auto symbolIdx = 0; symbolIdx < symbolsCount; symbolIdx++)
    {
        const auto symbolWidthN = *(pSymbolWidthN++);
        const auto symbolCenterN = nextSymbolStartN + 0.5f * symbolWidthN;

        auto basePathPointIndex = 0u;
        float scannedPathLengthN = 0.0f;
        while (scannedPathLengthN <= 1.0f && basePathPointIndex < pathSegmentsLengthN.size() - 1)
        {
            const auto pathSegmentLengthN = pathSegmentsLengthN[basePathPointIndex];

            if (scannedPathLengthN + pathSegmentLengthN > symbolCenterN)
                break;

            basePathPointIndex++;
            scannedPathLengthN += pathSegmentLengthN;
        }

        ComputedPinPoint computedPinPoint;
        computedPinPoint.basePathPointIndex = basePathPointIndex;
        computedPinPoint.normalizedOffsetFromBasePathPoint =
            (symbolCenterN - scannedPathLengthN) / pathSegmentsLengthN[basePathPointIndex];
        if (computedPinPoint.normalizedOffsetFromBasePathPoint < 0.0f ||
            computedPinPoint.normalizedOffsetFromBasePathPoint > 1.0f)
        {
            computedPinPoint.normalizedOffsetFromBasePathPoint = 0.5f;
        }
        assert(
            computedPinPoint.normalizedOffsetFromBasePathPoint >= 0.0f &&
            computedPinPoint.normalizedOffsetFromBasePathPoint <= 1.0f);
        const auto& segmentStartPoint31 = path31[basePathPointIndex + 0];
        const auto& segmentEndPoint31 = path31[basePathPointIndex + 1];
        const auto& vSegment31 = segmentEndPoint31 - segmentStartPoint31;
        computedPinPoint.point31 =
            segmentStartPoint31 + PointI(PointD(vSegment31) * computedPinPoint.normalizedOffsetFromBasePathPoint);

        outComputedPinPoints.push_back(qMove(computedPinPoint));

        nextSymbolStartN += symbolWidthN + symbolSpacingN;
    }
}

OsmAnd::MapObjectsSymbolsProvider_P::СombinedPath::СombinedPath(
    const AreaI& bbox31,
    const std::shared_ptr<const MapObject>& mapObject_,
    const PointD& divisor31ToPixels_)
    : _lengthInPixels(-1.0f)
    , _attachedToAnotherPath(false)
    , _combined(false)
    , mapObject(mapObject_)
    , divisor31ToPixels(divisor31ToPixels_)
{
    areaIndex = mapObject->startReadingArea();
    if (areaIndex >= 0 && !mapObject->vapItems[areaIndex]->area31.contains(bbox31))
    {
        mapObject->stopReadingArea(areaIndex);
        areaIndex = -1;
    }
    _points = detachedOf(areaIndex >= 0 ? mapObject->vapItems[areaIndex]->points31 : mapObject->points31);
    if (areaIndex >= 0)
        mapObject->stopReadingArea(areaIndex);
}

OsmAnd::MapObjectsSymbolsProvider_P::СombinedPath::~СombinedPath()
{
}

QVector<OsmAnd::PointI> OsmAnd::MapObjectsSymbolsProvider_P::СombinedPath::getPoints() const
{
    return _points;
}

bool OsmAnd::MapObjectsSymbolsProvider_P::СombinedPath::isCombined() const
{
    return _combined;
}

bool OsmAnd::MapObjectsSymbolsProvider_P::СombinedPath::isAttachedToAnotherPath() const
{
    return _attachedToAnotherPath;
}

bool OsmAnd::MapObjectsSymbolsProvider_P::СombinedPath::isAttachAllowed(
    const std::shared_ptr<const СombinedPath>& other,
    const float maxGapBetweenPaths,
    float& outGapBetweenPaths) const
{
    const auto lastPoint = _points.back();
    const auto otherFirstPoint = other->_points.front();
    const auto gapVector = otherFirstPoint - lastPoint;
    outGapBetweenPaths = qAbs(gapVector.x / divisor31ToPixels.x) + qAbs(gapVector.y / divisor31ToPixels.y);

    if (outGapBetweenPaths >= maxGapBetweenPaths)
        return false;

    const auto penultimatePoint = _points[_points.size() - 2];
    const auto otherSecondPoint = other->_points[1];

    const auto firstPathLastVectorN = static_cast<PointD>(lastPoint - penultimatePoint).normalized();
    const auto secondPathFirstVectorN = static_cast<PointD>(otherSecondPoint - otherFirstPoint).normalized();

    // Allow paths attachment only with obtuse angle between their segment of connection
    double cumulativeAngle = 0.0f;

    if (gapVector.x == 0 && gapVector.y == 0)
    {
        // Check angle between last segment of 1st path and first segment of 2nd path 
        const auto angle = Utilities::getSignedAngle(firstPathLastVectorN, secondPathFirstVectorN);
        if (qAbs(angle) >= MAX_ANGLE_BETWEEN_VECTORS)
            return false;

        cumulativeAngle += angle;
    }
    else
    {
        // Check angle between сonnecting segments and gap segment
        const auto gapVectorN = static_cast<PointD>(gapVector).normalized();
        const auto angle1 = Utilities::getSignedAngle(firstPathLastVectorN, gapVectorN);
        const auto angle2 = Utilities::getSignedAngle(gapVectorN, secondPathFirstVectorN);
        if (qAbs(angle1) >= MAX_ANGLE_BETWEEN_VECTORS || qAbs(angle2) >= MAX_ANGLE_BETWEEN_VECTORS)
            return false;

        cumulativeAngle += angle1 + angle2;
    }

    // Check angles between adjacent segments to prevent sharp U-shaped turns

    if (_points.size() >= 3)
    {
        // Compute second to last vector of first path to check angle with last vector
        const auto thirdToLastPoint = _points[_points.size() - 3];
        const auto secondToLastVectorN = static_cast<PointD>(penultimatePoint - thirdToLastPoint).normalized();

        cumulativeAngle += Utilities::getSignedAngle(secondToLastVectorN, firstPathLastVectorN);
        if (qAbs(cumulativeAngle) >= MAX_ANGLE_BETWEEN_VECTORS)
            return false;
    }

    if (other->_points.size() >= 3)
    {
        // Compute second vector of second path to check angle with first vector
        const auto thirdPoint = other->_points[2];
        const auto secondVector = static_cast<PointD>(thirdPoint - otherSecondPoint).normalized();

        cumulativeAngle += Utilities::getSignedAngle(secondPathFirstVectorN, secondVector);
        if (qAbs(cumulativeAngle) >= MAX_ANGLE_BETWEEN_VECTORS)
            return false;
    }

    return true;
}

void OsmAnd::MapObjectsSymbolsProvider_P::СombinedPath::attachPath(const std::shared_ptr<СombinedPath>& other)
{
    const auto end = _points.back();
    const auto otherStart = other->_points[0];
    const auto gapVector = otherStart - end;
    if (gapVector.x == 0 && gapVector.y == 0)
    {
        _points.append(other->_points.mid(1));
        _lengthInPixels = getLengthInPixels() + other->getLengthInPixels();
    }
    else
    {
        _points.append(other->_points);

        const auto gapLength = static_cast<PointF>(gapVector).norm();
        _lengthInPixels = getLengthInPixels() + gapLength + other->getLengthInPixels();
    }
    
    _combined = true;    
    other->_attachedToAnotherPath = true;
}

float OsmAnd::MapObjectsSymbolsProvider_P::СombinedPath::getLengthInPixels() const
{
    if (qFuzzyCompare(_lengthInPixels, -1.0f))
    {
        for (auto i = 1; i < _points.size(); i++)
        {
            const auto previousPoint = _points[i - 1];
            const auto currentPoint = _points[i];
            const auto delta31 = currentPoint - previousPoint;
            const PointD deltaInPixels(delta31.x / divisor31ToPixels.x, delta31.y / divisor31ToPixels.y);
            _lengthInPixels += qSqrt(deltaInPixels.squareNorm());
        }
    }

    return _lengthInPixels;
}

OsmAnd::MapObjectsSymbolsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata_)
    : binaryMapPrimitivesRetainableCacheMetadata(binaryMapPrimitivesRetainableCacheMetadata_)
{
}

OsmAnd::MapObjectsSymbolsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
