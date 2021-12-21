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
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    if (pOutMetric)
        pOutMetric->reset();
    const auto& request = MapDataProviderHelpers::castRequest<MapObjectsSymbolsProvider::Request>(request_);
    const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);

    // Obtain offline map primitives tile
    std::shared_ptr<MapPrimitivesProvider::Data> primitivesTile;
    owner->primitivesProvider->obtainTiledPrimitives(request, primitivesTile);

    // If tile has nothing to be rasterized, mark that data is not available for it
    if (!primitivesTile || primitivesTile->primitivisedObjects->isEmpty())
    {
        // Mark tile as empty
        outData.reset();
        return true;
    }

    // Rasterize symbols and create symbols groups
    QList< std::shared_ptr<const SymbolRasterizer::RasterizedSymbolsGroup> > rasterizedSymbolsGroups;
    QHash< std::shared_ptr<const MapObject>, std::shared_ptr<MapObjectSymbolsGroup> > preallocatedSymbolsGroups;
    const auto filterCallback = request.filterCallback;
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
            assert(static_cast<bool>(rasterizedSymbol->image));

            std::shared_ptr<MapSymbol> symbol;
            if (const auto rasterizedSpriteSymbol = std::dynamic_pointer_cast<const SymbolRasterizer::RasterizedSpriteSymbol>(rasterizedSymbol))
            {
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
                symbol.reset(billboardRasterSymbol);
            }
            else if (const auto rasterizedOnPathSymbol = std::dynamic_pointer_cast<const SymbolRasterizer::RasterizedOnPathSymbol>(rasterizedSymbol))
            {
                hasAtLeastOneOnPath = true;

                const auto onPathSymbol = new OnPathRasterMapSymbol(group);
                onPathSymbol->order = rasterizedOnPathSymbol->order;
                onPathSymbol->image = rasterizedOnPathSymbol->image;
                onPathSymbol->size = PointI(rasterizedOnPathSymbol->image->width(), rasterizedOnPathSymbol->image->height());
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
            QVector<float> symbolsWidthsInPixels(group->symbols.size());
            auto pSymbolWidthInPixels = symbolsWidthsInPixels.data();
            for (const auto& symbol : constOf(group->symbols))
            {
                if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                {
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
            const auto computedPinPoints = computePinPoints(
                mapObject->points31,
                env->getGlobalPathPadding(),
                env->getDefaultBlockPathSpacing() * 10,
                env->getDefaultSymbolPathSpacing(),
                symbolsWidthsInPixels,
                mapObject->getMinZoomLevel(),
                mapObject->getMaxZoomLevel(),
                request.zoom);

            // After pin-points were computed, assign them to symbols in the same order
            auto citComputedPinPoint = computedPinPoints.cbegin();
            const auto citComputedPinPointsEnd = computedPinPoints.cend();
            while (citComputedPinPoint != citComputedPinPointsEnd)
            {
                // Construct new additional instance of group
                std::shared_ptr<MapSymbolsGroup::AdditionalInstance> additionalGroupInstance(
                    new MapSymbolsGroup::AdditionalInstance(group));
                    
                for (const auto& symbol : constOf(group->symbols))
                {
                    // Stop in case no more pin-points left
                    if (citComputedPinPoint == citComputedPinPointsEnd)
                        break;
                    const auto& computedPinPoint = *(citComputedPinPoint++);

                    std::shared_ptr<MapSymbolsGroup::AdditionalSymbolInstanceParameters> additionalSymbolInstance;
                    if (const auto billboardSymbol = std::dynamic_pointer_cast<BillboardRasterMapSymbol>(symbol))
                    {
                        const auto billboardSymbolInstance =
                            new MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters(additionalGroupInstance.get());
                        billboardSymbolInstance->overridesPosition31 = true;
                        billboardSymbolInstance->position31 = computedPinPoint.point31;
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
                        additionalSymbolInstance.reset(onPathSymbolInstance);
                    }

                    if (additionalSymbolInstance)
                        additionalGroupInstance->symbols.insert(symbol, qMove(additionalSymbolInstance));
                }

                group->additionalInstances.push_back(qMove(additionalGroupInstance));
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
    outData.reset(new MapObjectsSymbolsProvider::Data(
        request.tileId,
        request.zoom,
        symbolsGroups,
        primitivesTile,
        new RetainableCacheMetadata(primitivesTile->retainableCacheMetadata)));

    return true;
}

QList<OsmAnd::MapObjectsSymbolsProvider_P::ComputedPinPoint> OsmAnd::MapObjectsSymbolsProvider_P::computePinPoints(
    const QVector<PointI>& path31,
    const float globalPaddingInPixels,
    const float blockSpacingInPixels,
    const float symbolSpacingInPixels,
    const QVector<float>& symbolsWidthsInPixels,
    const ZoomLevel minZoom,
    const ZoomLevel maxZoom,
    const ZoomLevel neededZoom) const
{
    Q_UNUSED(minZoom);
    Q_UNUSED(maxZoom);

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
        }
        if (fittedSymbolsCount == 0)
            return computedPinPoints;

        fittedSymbolsWidth -= symbolSpacingInPixels;
        auto nextSymbolStartN = pathLengthInPixels - 0.5f * fittedSymbolsWidth;
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
        while (scannedPathLengthN <= 1.0f)
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

OsmAnd::MapObjectsSymbolsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata_)
    : binaryMapPrimitivesRetainableCacheMetadata(binaryMapPrimitivesRetainableCacheMetadata_)
{
}

OsmAnd::MapObjectsSymbolsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
