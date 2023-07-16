#include "AtlasMapRendererSymbolsStage.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QSet>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkMath.h>
#include <SkMathPriv.h>
#include <SkPathMeasure.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "AtlasMapRenderer.h"
#include "AtlasMapRenderer_Metrics.h"
#include "AtlasMapRendererDebugStage.h"
#include "MapSymbol.h"
#include "VectorMapSymbol.h"
#include "BillboardVectorMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "OnPathRasterMapSymbol.h"
#include "BillboardRasterMapSymbol.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "MapSymbolsGroup.h"
#include "QKeyValueIterator.h"
#include "MapSymbolIntersectionClassesRegistry.h"
#include "Stopwatch.h"
#include "GlmExtensions.h"
#include "MapMarker.h"
#include "VectorLine.h"

// Maximum distance from camera to visible symbols (per camera-to-target distance)
#define VISIBLE_DISTANCE_FACTOR 1.4f

#define SHORT_GLYPHS_COUNT 4
#define MEDIUM_GLYPHS_COUNT 10
#define LONG_GLYPHS_COUNT 15

#define MEDIUM_GLYPHS_MAX_ANGLE 5.0f
#define LONG_GLYPHS_MAX_ANGLE 2.5f
#define EXTRA_LONG_GLYPHS_MAX_ANGLE 1.0f

OsmAnd::AtlasMapRendererSymbolsStage::AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer_)
    : AtlasMapRendererStage(renderer_)
{
    _lastResumeSymbolsUpdateTime = std::chrono::high_resolution_clock::now();
}

OsmAnd::AtlasMapRendererSymbolsStage::~AtlasMapRendererSymbolsStage() = default;

void OsmAnd::AtlasMapRendererSymbolsStage::prepare(AtlasMapRenderer_Metrics::Metric_renderFrame* const metric)
{
    Stopwatch stopwatch(metric != nullptr);

    ScreenQuadTree intersections;
    float timeSinceLastUpdate = std::chrono::duration<float>(
        std::chrono::high_resolution_clock::now() - _lastResumeSymbolsUpdateTime).count();
    // Don't invalidate this possibly last frame if symbols are quite fresh
    if (timeSinceLastUpdate < 0.5f)
        renderer->setSymbolsUpdated();
    bool forceSymbolsUpdate = false;
    int symbolsUpdateInterval = renderer->getSymbolsUpdateInterval();
    if (symbolsUpdateInterval > 0)
        forceSymbolsUpdate = timeSinceLastUpdate * 1000.0 > symbolsUpdateInterval;
    if (!obtainRenderableSymbols(renderableSymbols, intersections, metric, forceSymbolsUpdate))
    {
        // In case obtain failed due to lock, schedule another frame
        invalidateFrame();

        if (metric)
            metric->elapsedTimeForPreparingSymbols = stopwatch.elapsed();

        return;
    }

    Stopwatch preparedSymbolsPublishingStopwatch(metric != nullptr);

    ScreenQuadTree visibleSymbols(intersections.getRootArea(), intersections.maxDepth);
    visibleSymbols.insertFrom(renderableSymbols,
        []
        (const std::shared_ptr<const RenderableSymbol>& item, ScreenQuadTree::BBox& outBbox) -> bool
        {
            outBbox = item->visibleBBox;
            return true;
        });

    {
        QWriteLocker scopedLocker(&_lastPreparedIntersectionsLock);
        _lastPreparedIntersections = qMove(intersections);
    }
    {
        QWriteLocker scopedLocker(&_lastVisibleSymbolsLock);
        _lastVisibleSymbols = qMove(visibleSymbols);
    }
    if (metric)
        metric->elapsedTimeForPublishingPreparedSymbols = preparedSymbolsPublishingStopwatch.elapsed();

    if (metric)
        metric->elapsedTimeForPreparingSymbols = stopwatch.elapsed();
}

bool OsmAnd::AtlasMapRendererSymbolsStage::withTerrainFilter()
{
    const auto result = currentState.elevationDataProvider
        && currentState.zoomLevel >= currentState.elevationDataProvider->getMinZoom()
        && currentState.elevationAngle < 5.0f * currentState.zoomLevel;

    return result;
}

void OsmAnd::AtlasMapRendererSymbolsStage::convertRenderableSymbolsToMapSymbolInformation(
    const QList< std::shared_ptr<const RenderableSymbol> >& input,
    QList<IMapRenderer::MapSymbolInformation>& output)
{
    for (const auto& renderable : constOf(input))
    {
        IMapRenderer::MapSymbolInformation mapSymbolInfo;

        mapSymbolInfo.mapSymbol = renderable->mapSymbol;
        mapSymbolInfo.instanceParameters = renderable->genericInstanceParameters;

        output.push_back(qMove(mapSymbolInfo));
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage::queryLastPreparedSymbolsAt(
    const PointI screenPoint,
    QList<IMapRenderer::MapSymbolInformation>& outMapSymbols) const
{
    QList< std::shared_ptr<const RenderableSymbol> > selectedRenderables;

    {
        QReadLocker scopedLocker(&_lastPreparedIntersectionsLock);
        _lastPreparedIntersections.select(screenPoint, selectedRenderables);
    }

    convertRenderableSymbolsToMapSymbolInformation(selectedRenderables, outMapSymbols);
}

void OsmAnd::AtlasMapRendererSymbolsStage::queryLastPreparedSymbolsIn(
    const AreaI& screenArea,
    QList<IMapRenderer::MapSymbolInformation>& outMapSymbols,
    const bool strict /*= false*/) const
{
    QList< std::shared_ptr<const RenderableSymbol> > selectedRenderables;

    {
        QReadLocker scopedLocker(&_lastPreparedIntersectionsLock);
        _lastPreparedIntersections.query(screenArea, selectedRenderables, strict);
    }

    convertRenderableSymbolsToMapSymbolInformation(selectedRenderables, outMapSymbols);
}

void OsmAnd::AtlasMapRendererSymbolsStage::queryLastVisibleSymbolsAt(const PointI& screenPoint, QList<IMapRenderer::MapSymbolInformation>& outMapSymbols) const
{
    QList< std::shared_ptr<const RenderableSymbol> > selectedRenderables;

    {
        QReadLocker scopedLocker(&_lastVisibleSymbolsLock);
        _lastVisibleSymbols.select(screenPoint, selectedRenderables);
    }

    convertRenderableSymbolsToMapSymbolInformation(selectedRenderables, outMapSymbols);
}

void OsmAnd::AtlasMapRendererSymbolsStage::queryLastVisibleSymbolsIn(
    const AreaI& screenArea, QList<IMapRenderer::MapSymbolInformation>& outMapSymbols, const bool strict /*= false*/) const
{
    QList< std::shared_ptr<const RenderableSymbol> > selectedRenderables;

    {
        QReadLocker scopedLocker(&_lastVisibleSymbolsLock);
        _lastVisibleSymbols.query(screenArea, selectedRenderables, strict);
    }

    convertRenderableSymbolsToMapSymbolInformation(selectedRenderables, outMapSymbols);
}

//#define OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE 1
#ifndef OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE
#   define OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE 0
#endif // !defined(OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE)

bool OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderableSymbols(
    QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
    ScreenQuadTree& outIntersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric,
    bool forceUpdate /*= false*/)
{
    Stopwatch stopwatch(metric != nullptr);

    // Overscaled/underscaled symbol resources were removed
    const auto needUpdatedSymbols = renderer->needUpdatedSymbols();

    // In case symbols update was not suspended, process published symbols
    if (!renderer->isSymbolsUpdateSuspended() || forceUpdate || needUpdatedSymbols)
    {
        if (!publishedMapSymbolsByOrderLock.tryLockForRead())
            return false;

        _lastAcceptedMapSymbolsByOrder.clear();
        const auto result = obtainRenderableSymbols(
            publishedMapSymbolsByOrder,
            outRenderableSymbols,
            outIntersections,
            &_lastAcceptedMapSymbolsByOrder,
            metric);

        publishedMapSymbolsByOrderLock.unlock();

        if (result)
        {
            if (needUpdatedSymbols)
                renderer->dontNeedUpdatedSymbols();

            // Don't invalidate this possibly last frame with fresh symbols
            renderer->setSymbolsUpdated();

            _lastResumeSymbolsUpdateTime = std::chrono::high_resolution_clock::now();
        }

        if (metric)
        {
            metric->elapsedTimeForObtainingRenderableSymbolsWithLock = stopwatch.elapsed();
            metric->elapsedTimeForObtainingRenderableSymbolsOnlyLock =
                metric->elapsedTimeForObtainingRenderableSymbolsWithLock - metric->elapsedTimeForObtainingRenderableSymbols;
        }

        return result;
    }

    // Do not suspend MapMarker and VectorLine objects

    if (!publishedMapSymbolsByOrderLock.tryLockForRead())
        return false;

    // Acquire fresh MapMarker and VectorLine objects from publishedMapSymbolsByOrder
    MapRenderer::PublishedMapSymbolsByOrder filteredPublishedMapSymbols;
    for (const auto& mapSymbolsByOrderEntry : rangeOf(constOf(publishedMapSymbolsByOrder)))
    {
        const auto order = mapSymbolsByOrderEntry.key();
        const auto& mapSymbols = mapSymbolsByOrderEntry.value();
        MapRenderer::PublishedMapSymbolsByGroup acceptedMapSymbols;
        for (const auto& mapSymbolsEntry : constOf(mapSymbols))
        {
            const auto& mapSymbolsGroup = mapSymbolsEntry.first;
            const auto& mapSymbolsFromGroup = mapSymbolsEntry.second;

            if (std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(mapSymbolsGroup)
                || std::dynamic_pointer_cast<const VectorLine::SymbolsGroup>(mapSymbolsGroup))
            {
                acceptedMapSymbols[mapSymbolsGroup] = mapSymbolsFromGroup;
            }
        }
        if (!acceptedMapSymbols.empty())
            filteredPublishedMapSymbols[order] = acceptedMapSymbols;
    }

    // Filter out old MapMarker and VectorLine objects from _lastAcceptedMapSymbolsByOrder
    MapRenderer::PublishedMapSymbolsByOrder filteredLastAcceptedMapSymbols;
    for (const auto& mapSymbolsByOrderEntry : rangeOf(constOf(_lastAcceptedMapSymbolsByOrder)))
    {
        const auto order = mapSymbolsByOrderEntry.key();
        const auto& mapSymbols = mapSymbolsByOrderEntry.value();
        MapRenderer::PublishedMapSymbolsByGroup acceptedMapSymbols;
        for (const auto& mapSymbolsEntry : constOf(mapSymbols))
        {
            const auto& mapSymbolsGroup = mapSymbolsEntry.first;
            const auto& mapSymbolsFromGroup = mapSymbolsEntry.second;

            if (!std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(mapSymbolsGroup)
                && !std::dynamic_pointer_cast<const VectorLine::SymbolsGroup>(mapSymbolsGroup))
            {
                acceptedMapSymbols[mapSymbolsGroup] = mapSymbolsFromGroup;
            }
        }
        filteredLastAcceptedMapSymbols[order] = acceptedMapSymbols;
    }

    // Append new MapMarker and VectorLine objects to filteredLastAcceptedMapSymbols
    for (const auto& mapSymbolsByOrderEntry : rangeOf(constOf(filteredPublishedMapSymbols)))
    {
        const auto order = mapSymbolsByOrderEntry.key();
        const auto& mapSymbols = mapSymbolsByOrderEntry.value();

        auto itAcceptedMapSymbols = filteredLastAcceptedMapSymbols.find(order);
        if (itAcceptedMapSymbols != filteredLastAcceptedMapSymbols.end())
        {
            auto& acceptedMapSymbols = *itAcceptedMapSymbols;
            acceptedMapSymbols.insert(mapSymbols.begin(), mapSymbols.end());
        }
        else
        {
            filteredLastAcceptedMapSymbols[order] = mapSymbols;
        }
    }

    // Otherwise, use last accepted map symbols by order
    const auto result = obtainRenderableSymbols(
        filteredLastAcceptedMapSymbols,
        outRenderableSymbols,
        outIntersections,
        nullptr,
        metric);

    publishedMapSymbolsByOrderLock.unlock();

    return result;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderableSymbols(
    const MapRenderer::PublishedMapSymbolsByOrder& mapSymbolsByOrder,
    QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
    ScreenQuadTree& outIntersections,
    MapRenderer::PublishedMapSymbolsByOrder* pOutAcceptedMapSymbolsByOrder,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric)
{
    Stopwatch stopwatch(metric != nullptr);

    typedef std::list< std::shared_ptr<const RenderableSymbol> > PlottedSymbols;
    PlottedSymbols plottedSymbols;

    struct PlottedSymbolRef
    {
        PlottedSymbols::iterator iterator;
        std::shared_ptr<const RenderableSymbol> renderable;
    };
    struct PlottedSymbolsRefGroupInstance
    {
        QList< PlottedSymbolRef > symbolsRefs;

        void discard(const AtlasMapRendererSymbolsStage* const stage, PlottedSymbols& plottedSymbols, ScreenQuadTree& intersections)
        {
            // Discard entire group
            for (auto& symbolRef : symbolsRefs)
            {
                if (Q_UNLIKELY(stage->debugSettings->showSymbolsBBoxesRejectedByPresentationMode))
                    stage->addIntersectionDebugBox(symbolRef.renderable, ColorARGB::fromSkColor(SK_ColorYELLOW).withAlpha(50));

#if !OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE
                bool intersectsWithOthers = !symbolRef.renderable->mapSymbol->intersectsWithClasses.isEmpty();
                if (intersectsWithOthers)
                {
                    const auto removed = intersections.removeOne(symbolRef.renderable, symbolRef.renderable->intersectionBBox);
                    assert(removed);
                }
#endif // !OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE
                plottedSymbols.erase(symbolRef.iterator);
            }
            symbolsRefs.clear();
        }

        void discardSpecific(
            const AtlasMapRendererSymbolsStage* const stage,
            PlottedSymbols& plottedSymbols,
            ScreenQuadTree& intersections,
            const std::function<bool(const std::shared_ptr<const RenderableSymbol>&)> acceptor)
        {
            auto itSymbolRef = mutableIteratorOf(symbolsRefs);
            while (itSymbolRef.hasNext())
            {
                const auto& symbolRef = itSymbolRef.next();

                if (!acceptor(symbolRef.renderable))
                    continue;

                if (Q_UNLIKELY(stage->debugSettings->showSymbolsBBoxesRejectedByPresentationMode))
                    stage->addIntersectionDebugBox(symbolRef.renderable, ColorARGB::fromSkColor(SK_ColorYELLOW).withAlpha(50));

#if !OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE
                const auto removed = intersections.removeOne(symbolRef.renderable, symbolRef.renderable->intersectionBBox);
                assert(removed);
#endif // !OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE
                plottedSymbols.erase(symbolRef.iterator);
                itSymbolRef.remove();
            }
        }

        void discardAllOf(
            const AtlasMapRendererSymbolsStage* const stage,
            PlottedSymbols& plottedSymbols,
            ScreenQuadTree& intersections,
            const MapSymbol::ContentClass contentClass)
        {
            auto itSymbolRef = mutableIteratorOf(symbolsRefs);
            while (itSymbolRef.hasNext())
            {
                const auto& symbolRef = itSymbolRef.next();

                if (symbolRef.renderable->mapSymbol->contentClass != contentClass)
                    continue;

                if (Q_UNLIKELY(stage->debugSettings->showSymbolsBBoxesRejectedByPresentationMode))
                    stage->addIntersectionDebugBox(symbolRef.renderable, ColorARGB::fromSkColor(SK_ColorYELLOW).withAlpha(50));

#if !OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE
                const auto removed = intersections.removeOne(symbolRef.renderable, symbolRef.renderable->intersectionBBox);
                assert(removed);
#endif // !OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE
                plottedSymbols.erase(symbolRef.iterator);
                itSymbolRef.remove();
            }
        }
    };
    struct PlottedSymbolsRefGroupInstances
    {
        QHash< std::shared_ptr<const MapSymbolsGroup::AdditionalInstance>, PlottedSymbolsRefGroupInstance > instancesRefs;

        void discard(const AtlasMapRendererSymbolsStage* const stage, PlottedSymbols& plottedSymbols, ScreenQuadTree& intersections)
        {
            // Discard all instances
            for (auto& instanceRef : instancesRefs)
                instanceRef.discard(stage, plottedSymbols, intersections);
            instancesRefs.clear();
        }
    };
    QHash< std::shared_ptr<const MapSymbolsGroup>, PlottedSymbolsRefGroupInstances> plottedSymbolsMapByGroupAndInstance;

    // Iterate over map symbols layer sorted by "order" in ascending direction.
    // This means that map symbols with smaller order value are more important than map symbols with larger order value.
    // Also this means that map symbols with smaller order value are rendered after map symbols with larger order value.
    // Tree depth should satisfy following condition:
    // (max(width, height) / 2^depth) >= 64
    const auto viewportMaxDimension = qMax(currentState.viewport.height(), currentState.viewport.width());
    const auto treeDepth = 32u - SkCLZ(viewportMaxDimension >> 6);
    outIntersections = ScreenQuadTree(currentState.viewport, qMax(treeDepth, 1u));
    ComputedPathsDataCache computedPathsDataCache;
    bool applyFiltering = pOutAcceptedMapSymbolsByOrder != nullptr;
    if (!withTerrainFilter())
    {
        // Simplified one-pass cycle if all symbols are visible on terrain
        for (const auto& mapSymbolsByOrderEntry : rangeOf(constOf(mapSymbolsByOrder)))
        {
            const auto order = mapSymbolsByOrderEntry.key();
            const auto& mapSymbols = mapSymbolsByOrderEntry.value();
            MapRenderer::PublishedMapSymbolsByGroup* pAcceptedMapSymbols = nullptr;

            const auto itPlottedSymbolsInsertPosition = plottedSymbols.begin();

            // Iterate over all groups in proper order (proper order is maintained during publishing)
            for (const auto& mapSymbolsEntry : constOf(mapSymbols))
            {
                const auto& mapSymbolsGroup = mapSymbolsEntry.first;
                const auto& mapSymbolsFromGroup = mapSymbolsEntry.second;

                // Debug: showTooShortOnPathSymbolsRenderablesPaths
                if (Q_UNLIKELY(debugSettings->showTooShortOnPathSymbolsRenderablesPaths) &&
                    mapSymbolsGroup->additionalInstancesDiscardOriginal &&
                    mapSymbolsGroup->additionalInstances.isEmpty())
                {
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (const auto onPathMapSymbol = std::dynamic_pointer_cast<IOnPathMapSymbol>(mapSymbol))
                        {
                            addPathDebugLine(onPathMapSymbol->getPath31(), ColorARGB::fromSkColor(SK_ColorYELLOW));
                        }
                    }
                }

                // Check if this symbol can be skipped
                if (mapSymbolsGroup->additionalInstancesDiscardOriginal
                    && mapSymbolsGroup->additionalInstances.isEmpty())
                    continue;

                // Debug: showAllPaths
                if (Q_UNLIKELY(debugSettings->showAllPaths))
                {
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (const auto onPathMapSymbol = std::dynamic_pointer_cast<IOnPathMapSymbol>(mapSymbol))
                        {
                            addPathDebugLine(onPathMapSymbol->getPath31(), ColorARGB::fromSkColor(SK_ColorGRAY));
                        }
                    }
                }

                // Check if original was discarded
                if (!mapSymbolsGroup->additionalInstancesDiscardOriginal)
                {
                    // Process symbols from this group in order as they are stored in group
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (mapSymbol->isHidden)
                            continue;

                        // If this map symbol is not published yet or is located at different order, skip
                        const auto citReferencesOrigins = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citReferencesOrigins == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = *citReferencesOrigins;

                        QList< std::shared_ptr<RenderableSymbol> > renderableSymbols;
                        obtainRenderablesFromSymbol(
                            mapSymbolsGroup,
                            mapSymbol,
                            nullptr,
                            referencesOrigins,
                            computedPathsDataCache,
                            renderableSymbols,
                            outIntersections,
                            applyFiltering,
                            metric);

                        bool atLeastOnePlotted = false;
                        for (const auto& renderableSymbol : constOf(renderableSymbols))
                        {
                            if (!plotSymbol(renderableSymbol, outIntersections, applyFiltering, metric))
                                continue;

                            if (!atLeastOnePlotted)
                            {
                                // In case renderable symbol was obtained and accepted map symbols were requested,
                                // add this symbol to accepted
                                if (pOutAcceptedMapSymbolsByOrder)
                                {
                                    if (pAcceptedMapSymbols == nullptr)
                                        pAcceptedMapSymbols = &(*pOutAcceptedMapSymbolsByOrder)[order];

                                    (*pAcceptedMapSymbols)[mapSymbolsGroup].insert(mapSymbol, referencesOrigins);
                                }
                                atLeastOnePlotted = true;
                            }

                            const auto itPlottedSymbol =
                                plottedSymbols.insert(itPlottedSymbolsInsertPosition, renderableSymbol);
                            PlottedSymbolRef plottedSymbolRef = { itPlottedSymbol, renderableSymbol };

                            plottedSymbolsMapByGroupAndInstance[mapSymbolsGroup]
                                .instancesRefs[nullptr]
                                .symbolsRefs.push_back(qMove(plottedSymbolRef));
                        }
                    }
                }

                // Now process rest of the instances in order as they are defined in original group
                for (const auto& additionalGroupInstance : constOf(mapSymbolsGroup->additionalInstances))
                {
                    // Process symbols from this group in order as they are stored in original group
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (mapSymbol->isHidden)
                            continue;

                        // If this map symbol is not published yet or is located at different order, skip
                        const auto citReferencesOrigins = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citReferencesOrigins == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = *citReferencesOrigins;

                        // If symbol is not references in additional group reference, also skip
                        const auto citAdditionalSymbolInstance = additionalGroupInstance->symbols.constFind(mapSymbol);
                        if (citAdditionalSymbolInstance == additionalGroupInstance->symbols.cend())
                            continue;
                        const auto& additionalSymbolInstance = *citAdditionalSymbolInstance;

                        QList< std::shared_ptr<RenderableSymbol> > renderableSymbols;
                        obtainRenderablesFromSymbol(
                            mapSymbolsGroup,
                            mapSymbol,
                            additionalSymbolInstance,
                            referencesOrigins,
                            computedPathsDataCache,
                            renderableSymbols,
                            outIntersections,
                            applyFiltering,
                            metric);

                        bool atLeastOnePlotted = false;
                        for (const auto& renderableSymbol : constOf(renderableSymbols))
                        {
                            if (!plotSymbol(renderableSymbol, outIntersections, applyFiltering, metric))
                                continue;

                            if (!atLeastOnePlotted)
                            {
                                // In case renderable symbol was obtained and accepted map symbols were requested,
                                // add this symbol to accepted
                                if (pOutAcceptedMapSymbolsByOrder)
                                {
                                    if (pAcceptedMapSymbols == nullptr)
                                        pAcceptedMapSymbols = &(*pOutAcceptedMapSymbolsByOrder)[order];

                                    (*pAcceptedMapSymbols)[mapSymbolsGroup].insert(mapSymbol, referencesOrigins);
                                }
                                atLeastOnePlotted = true;
                            }

                            const auto itPlottedSymbol =
                                plottedSymbols.insert(itPlottedSymbolsInsertPosition, renderableSymbol);
                            PlottedSymbolRef plottedSymbolRef = { itPlottedSymbol, renderableSymbol };

                            plottedSymbolsMapByGroupAndInstance[mapSymbolsGroup]
                                .instancesRefs[additionalGroupInstance]
                                .symbolsRefs.push_back(qMove(plottedSymbolRef));
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Two-pass algorithm if symbol visibility on terrain should be verified

        typedef std::pair<int, std::shared_ptr<QList<std::shared_ptr<RenderableSymbol>>>> SymbolRenderables;
        QList<SymbolRenderables> originalRenderableSymbols;
        QList<SymbolRenderables> additionalRenderableSymbols;
        clearTerrainVisibilityFiltering();

        // Collect renderable symbols for rendering
        int originalSymbolIndex = 0;
        int additionalSymbolIndex = 0;
        for (const auto& mapSymbolsByOrderEntry : rangeOf(constOf(mapSymbolsByOrder)))
        {
            const auto& mapSymbols = mapSymbolsByOrderEntry.value();

            // Iterate over all groups in proper order (proper order is maintained during publishing)
            for (const auto& mapSymbolsEntry : constOf(mapSymbols))
            {
                const auto& mapSymbolsGroup = mapSymbolsEntry.first;
                const auto& mapSymbolsFromGroup = mapSymbolsEntry.second;

                // Debug: showTooShortOnPathSymbolsRenderablesPaths
                if (Q_UNLIKELY(debugSettings->showTooShortOnPathSymbolsRenderablesPaths) &&
                    mapSymbolsGroup->additionalInstancesDiscardOriginal &&
                    mapSymbolsGroup->additionalInstances.isEmpty())
                {
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (const auto onPathMapSymbol = std::dynamic_pointer_cast<IOnPathMapSymbol>(mapSymbol))
                        {
                            addPathDebugLine(onPathMapSymbol->getPath31(), ColorARGB::fromSkColor(SK_ColorYELLOW));
                        }
                    }
                }

                // Check if this symbol can be skipped
                if (mapSymbolsGroup->additionalInstancesDiscardOriginal && mapSymbolsGroup->additionalInstances.isEmpty())
                    continue;

                // Debug: showAllPaths
                if (Q_UNLIKELY(debugSettings->showAllPaths))
                {
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (const auto onPathMapSymbol = std::dynamic_pointer_cast<IOnPathMapSymbol>(mapSymbol))
                        {
                            addPathDebugLine(onPathMapSymbol->getPath31(), ColorARGB::fromSkColor(SK_ColorGRAY));
                        }
                    }
                }

                // Check if original was discarded
                if (!mapSymbolsGroup->additionalInstancesDiscardOriginal)
                {
                    // Process symbols from this group in order as they are stored in group
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (mapSymbol->isHidden)
                            continue;

                        // If this map symbol is not published yet or is located at different order, skip
                        const auto citReferencesOrigins = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citReferencesOrigins == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = *citReferencesOrigins;

                        std::shared_ptr<QList<std::shared_ptr<RenderableSymbol>>> renderableSymbols;
                        renderableSymbols.reset(new QList<std::shared_ptr<RenderableSymbol>>);
                        obtainRenderablesFromSymbol(
                            mapSymbolsGroup,
                            mapSymbol,
                            nullptr,
                            referencesOrigins,
                            computedPathsDataCache,
                            *renderableSymbols,
                            outIntersections,
                            applyFiltering,
                            metric);

                        if (renderableSymbols->size() > 0)
                            originalRenderableSymbols.push_back(SymbolRenderables(originalSymbolIndex, renderableSymbols));
                        originalSymbolIndex++;
                    }
                }

                // Now process rest of the instances in order as they are defined in original group
                for (const auto& additionalGroupInstance : constOf(mapSymbolsGroup->additionalInstances))
                {
                    // Process symbols from this group in order as they are stored in original group
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (mapSymbol->isHidden)
                            continue;

                        // If this map symbol is not published yet or is located at different order, skip
                        const auto citReferencesOrigins = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citReferencesOrigins == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = *citReferencesOrigins;

                        // If symbol is not references in additional group reference, also skip
                        const auto citAdditionalSymbolInstance = additionalGroupInstance->symbols.constFind(mapSymbol);
                        if (citAdditionalSymbolInstance == additionalGroupInstance->symbols.cend())
                            continue;
                        const auto& additionalSymbolInstance = *citAdditionalSymbolInstance;

                        std::shared_ptr<QList<std::shared_ptr<RenderableSymbol>>> renderableSymbols;
                        renderableSymbols.reset(new QList<std::shared_ptr<RenderableSymbol>>);
                        obtainRenderablesFromSymbol(
                            mapSymbolsGroup,
                            mapSymbol,
                            additionalSymbolInstance,
                            referencesOrigins,
                            computedPathsDataCache,
                            *renderableSymbols,
                            outIntersections,
                            applyFiltering,
                            metric);

                        if (renderableSymbols->size() > 0)
                        {
                            additionalRenderableSymbols.push_back(
                                SymbolRenderables(additionalSymbolIndex, renderableSymbols));
                        }
                        additionalSymbolIndex++;
                    }
                }
            }
        }

        // Process renderable symbols before rendering
        originalSymbolIndex = 0;
        additionalSymbolIndex = 0;
        int originalSymbolListIndex = 0;
        int additionalSymbolListIndex = 0;
        auto currentOriginalRenderables = SymbolRenderables(0, originalRenderableSymbols.isEmpty()
            ?  nullptr : originalRenderableSymbols.at(0).second);
        auto currentAdditionalRenderables = SymbolRenderables(0, additionalRenderableSymbols.isEmpty()
            ?  nullptr : additionalRenderableSymbols.at(0).second);
        for (const auto& mapSymbolsByOrderEntry : rangeOf(constOf(mapSymbolsByOrder)))
        {
            const auto order = mapSymbolsByOrderEntry.key();
            const auto& mapSymbols = mapSymbolsByOrderEntry.value();
            MapRenderer::PublishedMapSymbolsByGroup* pAcceptedMapSymbols = nullptr;

            const auto itPlottedSymbolsInsertPosition = plottedSymbols.begin();

            // Iterate over all groups in proper order (proper order is maintained during publishing)
            for (const auto& mapSymbolsEntry : constOf(mapSymbols))
            {
                const auto& mapSymbolsGroup = mapSymbolsEntry.first;
                const auto& mapSymbolsFromGroup = mapSymbolsEntry.second;

                // Check if this symbol can be skipped
                if (mapSymbolsGroup->additionalInstancesDiscardOriginal && mapSymbolsGroup->additionalInstances.isEmpty())
                    continue;

                // Check if original was discarded
                if (!mapSymbolsGroup->additionalInstancesDiscardOriginal)
                {
                    // Process symbols from this group in order as they are stored in group
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (mapSymbol->isHidden)
                            continue;

                        // If this map symbol is not published yet or is located at different order, skip
                        const auto citReferencesOrigins = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citReferencesOrigins == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = *citReferencesOrigins;

                        if (originalSymbolIndex > currentOriginalRenderables.first)
                        {
                            originalSymbolListIndex++;
                            if (originalSymbolListIndex < originalRenderableSymbols.size())
                                currentOriginalRenderables = originalRenderableSymbols.at(originalSymbolListIndex);
                            else
                                currentOriginalRenderables = SymbolRenderables(INT32_MAX, nullptr);
                        }

                        const auto pRenderableSymbols = originalSymbolIndex == currentOriginalRenderables.first
                            ? currentOriginalRenderables.second : nullptr;
                        const auto& renderableSymbols =
                            pRenderableSymbols ? *pRenderableSymbols : QList<std::shared_ptr<RenderableSymbol>>();

                        originalSymbolIndex++;

                        bool atLeastOnePlotted = false;
                        for (const auto& renderableSymbol : constOf(renderableSymbols))
                        {
                            if (!plotSymbol(renderableSymbol, outIntersections, applyFiltering, metric))
                                continue;

                            if (!atLeastOnePlotted)
                            {
                                // In case renderable symbol was obtained and accepted map symbols were requested,
                                // add this symbol to accepted
                                if (pOutAcceptedMapSymbolsByOrder)
                                {
                                    if (pAcceptedMapSymbols == nullptr)
                                        pAcceptedMapSymbols = &(*pOutAcceptedMapSymbolsByOrder)[order];

                                    (*pAcceptedMapSymbols)[mapSymbolsGroup].insert(mapSymbol, referencesOrigins);
                                }
                                atLeastOnePlotted = true;
                            }

                            const auto itPlottedSymbol = plottedSymbols.insert(itPlottedSymbolsInsertPosition, renderableSymbol);
                            PlottedSymbolRef plottedSymbolRef = { itPlottedSymbol, renderableSymbol };

                            plottedSymbolsMapByGroupAndInstance[mapSymbolsGroup]
                                .instancesRefs[nullptr]
                                .symbolsRefs.push_back(qMove(plottedSymbolRef));
                        }
                    }
                }

                // Now process rest of the instances in order as they are defined in original group
                for (const auto& additionalGroupInstance : constOf(mapSymbolsGroup->additionalInstances))
                {
                    // Process symbols from this group in order as they are stored in original group
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (mapSymbol->isHidden)
                            continue;

                        // If this map symbol is not published yet or is located at different order, skip
                        const auto citReferencesOrigins = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citReferencesOrigins == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = *citReferencesOrigins;

                        // If symbol is not references in additional group reference, also skip
                        const auto citAdditionalSymbolInstance = additionalGroupInstance->symbols.constFind(mapSymbol);
                        if (citAdditionalSymbolInstance == additionalGroupInstance->symbols.cend())
                            continue;

                        if (additionalSymbolIndex > currentAdditionalRenderables.first)
                        {
                            additionalSymbolListIndex++;
                            if (additionalSymbolListIndex < additionalRenderableSymbols.size())
                                currentAdditionalRenderables = additionalRenderableSymbols.at(additionalSymbolListIndex);
                            else
                                currentAdditionalRenderables = SymbolRenderables(INT32_MAX, nullptr);
                        }

                        const auto pRenderableSymbols = additionalSymbolIndex == currentAdditionalRenderables.first
                            ? currentAdditionalRenderables.second : nullptr;
                        const auto& renderableSymbols =
                            pRenderableSymbols ? *pRenderableSymbols : QList<std::shared_ptr<RenderableSymbol>>();

                        additionalSymbolIndex++;

                        bool atLeastOnePlotted = false;
                        for (const auto& renderableSymbol : constOf(renderableSymbols))
                        {
                            if (!plotSymbol(renderableSymbol, outIntersections, applyFiltering, metric))
                                continue;

                            if (!atLeastOnePlotted)
                            {
                                // In case renderable symbol was obtained and accepted map symbols were requested,
                                // add this symbol to accepted
                                if (pOutAcceptedMapSymbolsByOrder)
                                {
                                    if (pAcceptedMapSymbols == nullptr)
                                        pAcceptedMapSymbols = &(*pOutAcceptedMapSymbolsByOrder)[order];

                                    (*pAcceptedMapSymbols)[mapSymbolsGroup].insert(mapSymbol, referencesOrigins);
                                }
                                atLeastOnePlotted = true;
                            }

                            const auto itPlottedSymbol = plottedSymbols.insert(itPlottedSymbolsInsertPosition, renderableSymbol);
                            PlottedSymbolRef plottedSymbolRef = { itPlottedSymbol, renderableSymbol };

                            plottedSymbolsMapByGroupAndInstance[mapSymbolsGroup]
                                .instancesRefs[additionalGroupInstance]
                                .symbolsRefs.push_back(qMove(plottedSymbolRef));
                        }
                    }
                }
            }
        }
    }

    if (Q_LIKELY(!debugSettings->skipSymbolsPresentationModeCheck))
    {
        Stopwatch symbolsPresentationModeCheckStopwatch(metric != nullptr);

        // Remove those plotted symbols that do not conform to presentation rules
        auto itPlottedSymbolsGroupInstancesEntry = mutableIteratorOf(plottedSymbolsMapByGroupAndInstance);
        while (itPlottedSymbolsGroupInstancesEntry.hasNext())
        {
            const auto& plottedSymbolsGroupInstancesEntry = itPlottedSymbolsGroupInstancesEntry.next();
            const auto& mapSymbolsGroup = plottedSymbolsGroupInstancesEntry.key();
            auto& plottedSymbolsGroupInstances = plottedSymbolsGroupInstancesEntry.value();

            if (!mapSymbolsGroup)
            {
                plottedSymbolsGroupInstances.discard(this, plottedSymbols, outIntersections);
                itPlottedSymbolsGroupInstancesEntry.remove();
                continue;
            }

            // Each instance of group is treated as separated group
            {
                auto itPlottedSymbolsGroupInstanceEntry = mutableIteratorOf(plottedSymbolsGroupInstances.instancesRefs);
                while (itPlottedSymbolsGroupInstanceEntry.hasNext())
                {
                    const auto& plottedSymbolsGroupInstanceEntry = itPlottedSymbolsGroupInstanceEntry.next();
                    const auto& mapSymbolsGroupInstance = plottedSymbolsGroupInstanceEntry.key();
                    auto& plottedSymbolsGroupInstance = plottedSymbolsGroupInstanceEntry.value();

                    // Just skip all rules
                    if (mapSymbolsGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAnything)
                        continue;

                    // Rule: show all symbols or no symbols
                    if (mapSymbolsGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAllOrNothing)
                    {
                        const auto symbolsCount = mapSymbolsGroupInstance
                            ? mapSymbolsGroupInstance->symbols.size()
                            : mapSymbolsGroup->symbols.size();

                        if (symbolsCount != plottedSymbolsGroupInstance.symbolsRefs.size())
                        {
                            // Discard entire group instance
                            plottedSymbolsGroupInstance.discard(
                                this,
                                plottedSymbols,
                                outIntersections);
                            itPlottedSymbolsGroupInstanceEntry.remove();
                            continue;
                        }
                    }

                    // Rule: if there's icon, icon must always be visible. Otherwise discard entire group
                    if (mapSymbolsGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowNoneIfIconIsNotShown)
                    {
                        const auto symbolWithIconContentClass = mapSymbolsGroupInstance
                            ? mapSymbolsGroupInstance->getFirstSymbolWithContentClass(MapSymbol::ContentClass::Icon)
                            : mapSymbolsGroup->getFirstSymbolWithContentClass(MapSymbol::ContentClass::Icon);

                        if (symbolWithIconContentClass)
                        {
                            bool iconPlotted = false;
                            for (const auto& plottedGroupSymbol : constOf(plottedSymbolsGroupInstance.symbolsRefs))
                            {
                                if (plottedGroupSymbol.renderable->mapSymbol != symbolWithIconContentClass)
                                    continue;

                                iconPlotted = true;
                                break;
                            }

                            if (!iconPlotted)
                            {
                                // Discard entire group instance
                                plottedSymbolsGroupInstance.discard(
                                    this,
                                    plottedSymbols,
                                    outIntersections);
                                itPlottedSymbolsGroupInstanceEntry.remove();
                                continue;
                            }
                        }
                    }

                    // Rule: if at least one caption was not shown, discard all other captions
                    if (mapSymbolsGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAllCaptionsOrNoCaptions)
                    {
                        const auto captionsCount = mapSymbolsGroupInstance
                            ? mapSymbolsGroupInstance->numberOfSymbolsWithContentClass(MapSymbol::ContentClass::Caption)
                            : mapSymbolsGroup->numberOfSymbolsWithContentClass(MapSymbol::ContentClass::Caption);

                        if (captionsCount > 0)
                        {
                            unsigned int captionsPlotted = 0;
                            for (const auto& plottedGroupSymbol : constOf(plottedSymbolsGroupInstance.symbolsRefs))
                            {
                                if (plottedGroupSymbol.renderable->mapSymbol->contentClass == MapSymbol::ContentClass::Caption)
                                    captionsPlotted++;
                            }

                            if (captionsCount != captionsPlotted)
                            {
                                // Discard all captions since at least one was not shown
                                plottedSymbolsGroupInstance.discardAllOf(
                                    this,
                                    plottedSymbols,
                                    outIntersections,
                                    MapSymbol::ContentClass::Caption);
                                if (plottedSymbolsGroupInstance.symbolsRefs.isEmpty())
                                {
                                    itPlottedSymbolsGroupInstanceEntry.remove();
                                    continue;
                                }
                            }
                        }
                    }

                    // Rule: show anything until first map symbol from group was not plotted
                    if (mapSymbolsGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAnythingUntilFirstGap)
                    {
                        const auto& mapSymbols = mapSymbolsGroupInstance
                            ? mapSymbolsGroupInstance->symbols.keys()
                            : mapSymbolsGroup->symbols;

                        auto itMapSymbol = iteratorOf(mapSymbols);
                        while (itMapSymbol.hasNext())
                        {
                            const auto& mapSymbol = itMapSymbol.next();

                            // Check if this symbol is plotted
                            bool symbolPlotted = false;
                            for (const auto& plottedGroupSymbol : constOf(plottedSymbolsGroupInstance.symbolsRefs))
                            {
                                if (plottedGroupSymbol.renderable->mapSymbol == mapSymbol)
                                {
                                    symbolPlotted = true;
                                    break;
                                }
                            }
                            if (symbolPlotted)
                                continue;

                            // In case this symbol was not plotted, discard all remaining symbol including this
                            itMapSymbol.previous();
                            break;
                        }
                        while (itMapSymbol.hasNext())
                        {
                            const auto& mapSymbol = itMapSymbol.next();

                            plottedSymbolsGroupInstance.discardSpecific(
                                this,
                                plottedSymbols,
                                outIntersections,
                                [mapSymbol](const std::shared_ptr<const RenderableSymbol>& renderableSymbol) -> bool
                                {
                                    return (renderableSymbol->mapSymbol == mapSymbol);
                                });
                        }
                        if (plottedSymbolsGroupInstance.symbolsRefs.isEmpty())
                        {
                            itPlottedSymbolsGroupInstanceEntry.remove();
                            continue;
                        }
                    }
                }
            }

            // In case all instances of group was removed, erase the group
            if (plottedSymbolsGroupInstances.instancesRefs.isEmpty())
                itPlottedSymbolsGroupInstancesEntry.remove();
        }

        if (metric)
            metric->elapsedTimeForSymbolsPresentationModeCheck = symbolsPresentationModeCheckStopwatch.elapsed();
    }

    // Publish the result
    outRenderableSymbols.clear();
    outRenderableSymbols.reserve(plottedSymbols.size());
    for (const auto& plottedSymbol : constOf(plottedSymbols))
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesAcceptedByIntersectionCheck))
            addIntersectionDebugBox(plottedSymbol, ColorARGB::fromSkColor(SK_ColorGREEN).withAlpha(50));

        outRenderableSymbols.push_back(qMove(plottedSymbol));
    }

    if (metric)
        metric->elapsedTimeForObtainingRenderableSymbols = stopwatch.elapsed();

    return true;
}

void OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderablesFromSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
    const std::shared_ptr<const MapSymbol>& mapSymbol,
    const std::shared_ptr<const MapSymbolsGroup::AdditionalSymbolInstanceParameters>& instanceParameters_,
    const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
    ComputedPathsDataCache& computedPathsDataCache,
    QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
    ScreenQuadTree& intersections,
    bool allowFastCheckByFrustum /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
{
    Stopwatch stopwatch(metric != nullptr);

    if (const auto onPathMapSymbol = std::dynamic_pointer_cast<const OnPathRasterMapSymbol>(mapSymbol))
    {
        if (Q_UNLIKELY(debugSettings->excludeOnPathSymbolsFromProcessing))
            return;

        const auto instanceParameters =
            std::static_pointer_cast<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters>(instanceParameters_);
        obtainRenderablesFromOnPathSymbol(
            mapSymbolGroup,
            onPathMapSymbol,
            instanceParameters,
            referenceOrigins,
            computedPathsDataCache,
            outRenderableSymbols,
            intersections,
            allowFastCheckByFrustum,
            metric);
    }
    else if (const auto onSurfaceMapSymbol = std::dynamic_pointer_cast<const IOnSurfaceMapSymbol>(mapSymbol))
    {
        if (Q_UNLIKELY(debugSettings->excludeOnSurfaceSymbolsFromProcessing))
            return;

        const auto instanceParameters =
            std::static_pointer_cast<const MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters>(instanceParameters_);
        obtainRenderablesFromOnSurfaceSymbol(
            mapSymbolGroup,
            onSurfaceMapSymbol,
            instanceParameters,
            referenceOrigins,
            outRenderableSymbols,
            allowFastCheckByFrustum,
            metric);
    }
    else if (const auto billboardMapSymbol = std::dynamic_pointer_cast<const IBillboardMapSymbol>(mapSymbol))
    {
        if (Q_UNLIKELY(debugSettings->excludeBillboardSymbolsFromProcessing))
            return;

        const auto instanceParameters =
            std::static_pointer_cast<const MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters>(instanceParameters_);
        obtainRenderablesFromBillboardSymbol(
            mapSymbolGroup,
            billboardMapSymbol,
            instanceParameters,
            referenceOrigins,
            outRenderableSymbols,
            intersections,
            allowFastCheckByFrustum,
            metric);
    }
    else
    {
        assert(false);
    }

    if (metric)
    {
        metric->elapsedTimeForObtainRenderableSymbolCalls += stopwatch.elapsed();
        metric->obtainRenderableSymbolCalls++;
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotSymbol(
    const std::shared_ptr<RenderableSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/) const
{
    Stopwatch stopwatch(metric != nullptr);

    bool plotted = false;
    if (const auto& renderableBillboard = std::dynamic_pointer_cast<RenderableBillboardSymbol>(renderable))
    {
        plotted = plotBillboardSymbol(renderableBillboard, intersections, applyFiltering, metric);
    }
    else if (const auto& renderableOnPath = std::dynamic_pointer_cast<RenderableOnPathSymbol>(renderable))
    {
        plotted = plotOnPathSymbol(renderableOnPath, intersections, applyFiltering, metric);
    }
    else if (const auto& renderableOnSurface = std::dynamic_pointer_cast<RenderableOnSurfaceSymbol>(renderable))
    {
        plotted = plotOnSurfaceSymbol(renderableOnSurface, intersections, applyFiltering, metric);
    }

    if (metric)
    {
        metric->elapsedTimeForPlotSymbolCalls += stopwatch.elapsed();
        metric->plotSymbolCalls++;
        if (plotted)
            metric->plotSymbolCallsSucceeded++;
        else
            metric->plotSymbolCallsFailed++;
    }

    return plotted;
}

void OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderablesFromBillboardSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
    const std::shared_ptr<const IBillboardMapSymbol>& billboardMapSymbol,
    const std::shared_ptr<const MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters>& instanceParameters,
    const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
    QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
    ScreenQuadTree& intersections,
    bool allowFastCheckByFrustum /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
{
    const auto& internalState = getInternalState();
    const auto mapSymbol = std::dynamic_pointer_cast<const MapSymbol>(billboardMapSymbol);

    const auto& position31 =
        (instanceParameters && instanceParameters->overridesPosition31)
        ? instanceParameters->position31
        : billboardMapSymbol->getPosition31();

    // Test against visible frustum area (if allowed)
    if (!debugSettings->disableSymbolsFastCheckByFrustum &&
        allowFastCheckByFrustum && mapSymbol->allowFastCheckByFrustum &&
        !internalState.globalFrustum2D31.test(position31) &&
        !internalState.extraFrustum2D31.test(position31))
    {
        if (metric)
            metric->billboardSymbolsRejectedByFrustum++;
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    /*
    {
        if (const auto symbolsGroupWithId = std::dynamic_pointer_cast<MapSymbolsGroupWithId>(mapSymbol->group.lock()))
        {
            if ((symbolsGroupWithId->id >> 1) != 244622303)
                return;
        }
    }
    */
    //////////////////////////////////////////////////////////////////////////

    // Get target tile and offset
    PointF offsetInTileN;
    const auto tileId = Utilities::normalizeTileId(Utilities::getTileId(position31, currentState.zoomLevel, &offsetInTileN), currentState.zoomLevel);

    // Get GPU resource
    const auto gpuResource = captureGpuResource(referenceOrigins, mapSymbol);
    if (!gpuResource)
        return;

    // Calculate location of symbol in world coordinates.
    auto offset31 = Utilities::shortestVector31(currentState.target31, position31);

    const auto offsetFromTarget = Utilities::convert31toFloat(offset31, currentState.zoomLevel);
    auto positionInWorld = glm::vec3(offsetFromTarget.x * AtlasMapRenderer::TileSize3D, 0.0f, offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

    // Get elevation data
    float elevationInMeters = 0.0f;
    std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
    PointF offsetInScaledTileN = offsetInTileN;
    if (getElevationData(tileId, currentState.zoomLevel, offsetInScaledTileN, &elevationData) != InvalidZoomLevel &&
        elevationData)
    {
        if (elevationData->getValue(offsetInScaledTileN, elevationInMeters))
        {
            const auto scaledElevationInMeters = elevationInMeters * currentState.elevationConfiguration.dataScaleFactor;

            const auto upperMetersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y, AtlasMapRenderer::TileSize3D);
            const auto lowerMetersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y + 1, AtlasMapRenderer::TileSize3D);
            const auto metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);

            positionInWorld.y = static_cast<float>((scaledElevationInMeters / metersPerUnit) * currentState.elevationConfiguration.zScaleFactor);
        }
    }

    const auto& symbol = std::static_pointer_cast<const BillboardRasterMapSymbol>(mapSymbol);

    const auto& offsetOnScreen =
        (instanceParameters && instanceParameters->overridesOffset)
        ? instanceParameters->offset
        : symbol->offset;

    // Calculate position on-screen coordinates (must correspond to calculation in shader)
    const auto symbolOnScreen = glm_extensions::project(
        positionInWorld,
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport);

    // Get bounds in screen coordinates
    auto visibleBBox = AreaI::fromCenterAndSize(
        static_cast<int>(symbolOnScreen.x + offsetOnScreen.x),
        static_cast<int>((currentState.windowSize.y - symbolOnScreen.y) + offsetOnScreen.y),
        symbol->size.x,
        symbol->size.y);

    if (!applyOnScreenVisibilityFiltering(visibleBBox, intersections, metric))
        return;

    const auto cameraVectorN = internalState.worldCameraPosition / internalState.distanceFromCameraToTarget;
    const auto distanceToPoint = glm::dot(internalState.worldCameraPosition - positionInWorld, cameraVectorN);
    if (distanceToPoint / internalState.distanceFromCameraToTarget > VISIBLE_DISTANCE_FACTOR)
        return;

    // Don't render fully transparent symbols
    const auto opacityFactor = getSubsectionOpacityFactor(mapSymbol);
    if (opacityFactor > 0.0f)
    {
        std::shared_ptr<RenderableBillboardSymbol> renderable(new RenderableBillboardSymbol());
        renderable->mapSymbolGroup = mapSymbolGroup;
        renderable->mapSymbol = mapSymbol;
        renderable->genericInstanceParameters = instanceParameters;
        renderable->instanceParameters = instanceParameters;
        renderable->gpuResource = gpuResource;
        renderable->positionInWorld = positionInWorld;
        renderable->elevationInMeters = elevationInMeters;
        renderable->tileId = tileId;
        renderable->offsetInTileN = offsetInTileN;
        renderable->opacityFactor = opacityFactor;
        renderable->visibleBBox = visibleBBox;

        if (allowFastCheckByFrustum)
        {
            renderable->queryIndex = startTerrainVisibilityFiltering(PointF(symbolOnScreen.x, symbolOnScreen.y),
                positionInWorld, positionInWorld, positionInWorld, positionInWorld);
        }
        else
            renderable->queryIndex = -1;

        outRenderableSymbols.push_back(renderable);
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/) const
{
    bool plotted = false;
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        plotted = plotBillboardRasterSymbol(renderable, intersections, applyFiltering, metric);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        plotted = plotBillboardVectorSymbol(renderable, intersections, applyFiltering, metric);
    }

    return plotted;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardRasterSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/) const
{
    auto visibleBBox = renderable->visibleBBox.asAABB;

    const auto& symbol = std::static_pointer_cast<const BillboardRasterMapSymbol>(renderable->mapSymbol);

    auto intersectionBBox = visibleBBox.getEnlargedBy(
        symbol->margin.top(),
        symbol->margin.left(),
        symbol->margin.bottom(),
        symbol->margin.right());
    renderable->intersectionBBox = intersectionBBox;

    if (Q_UNLIKELY(debugSettings->showBillboardSymbolBBoxes))
    {
        QVector<glm::vec2> debugBbox;
        debugBbox.push_back(glm::vec2(visibleBBox.left(), visibleBBox.top()));
        debugBbox.push_back(glm::vec2(visibleBBox.right(), visibleBBox.top()));
        debugBbox.push_back(glm::vec2(visibleBBox.right(), visibleBBox.bottom()));
        debugBbox.push_back(glm::vec2(visibleBBox.left(), visibleBBox.bottom()));
        debugBbox.push_back(glm::vec2(visibleBBox.left(), visibleBBox.top()));
        getRenderer()->debugStage->addLine2D(debugBbox, SK_ColorRED);

        debugBbox.clear();
        debugBbox.push_back(glm::vec2(intersectionBBox.left(), intersectionBBox.top()));
        debugBbox.push_back(glm::vec2(intersectionBBox.right(), intersectionBBox.top()));
        debugBbox.push_back(glm::vec2(intersectionBBox.right(), intersectionBBox.bottom()));
        debugBbox.push_back(glm::vec2(intersectionBBox.left(), intersectionBBox.bottom()));
        debugBbox.push_back(glm::vec2(intersectionBBox.left(), intersectionBBox.top()));
        getRenderer()->debugStage->addLine2D(debugBbox, SK_ColorGREEN);
    }

    if (applyFiltering)
    {
        if (renderable->queryIndex > -1 && !applyTerrainVisibilityFiltering(renderable->queryIndex, metric))
            return false;

        if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
            return false;

        if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
            return false;
    }
    return addToIntersections(renderable, intersections, metric);
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardVectorSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/) const
{
    assert(false);
    return false;
}

void OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderablesFromOnSurfaceSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
    const std::shared_ptr<const IOnSurfaceMapSymbol>& onSurfaceMapSymbol,
    const std::shared_ptr<const MapSymbolsGroup::AdditionalOnSurfaceSymbolInstanceParameters>& instanceParameters,
    const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
    QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
    bool allowFastCheckByFrustum /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
{
    const auto& internalState = getInternalState();
    const auto mapSymbol = std::dynamic_pointer_cast<const MapSymbol>(onSurfaceMapSymbol);

    auto position31 =
        (instanceParameters && instanceParameters->overridesPosition31)
        ? instanceParameters->position31
        : onSurfaceMapSymbol->getPosition31();
    const auto& direction =
        (instanceParameters && instanceParameters->overridesDirection)
        ? instanceParameters->direction
        : onSurfaceMapSymbol->getDirection();

    // Test against visible frustum area (if allowed)
    if (!debugSettings->disableSymbolsFastCheckByFrustum &&
        allowFastCheckByFrustum && mapSymbol->allowFastCheckByFrustum)
    {
        const auto height = getRenderer()->getHeightOfLocation(currentState, position31);
        if (const auto vectorMapSymbol = std::dynamic_pointer_cast<const VectorMapSymbol>(onSurfaceMapSymbol))
        {
            int64_t mapSize31 = INT32_MAX;
            mapSize31++;
            AreaI symbolArea;
            AreaI64 symbolArea64;
            PointI64 topLeft;
            PointI64 bottomRight;
            QVector<PointI> symbolRect;
            switch (vectorMapSymbol->scaleType)
            {
                case VectorMapSymbol::ScaleType::Raw:
                    symbolRect << position31;
                    break;
                case VectorMapSymbol::ScaleType::In31:
                    topLeft = PointI64(static_cast<int64_t>(position31.x) - vectorMapSymbol->scale,
                        static_cast<int64_t>(position31.y) - vectorMapSymbol->scale);
                    bottomRight = PointI64(static_cast<int64_t>(position31.x) + vectorMapSymbol->scale,
                        static_cast<int64_t>(position31.y) + vectorMapSymbol->scale);
                    if (bottomRight.x >= mapSize31)
                    {
                        topLeft.x -= mapSize31;
                        bottomRight.x -= mapSize31;
                    }
                    if (bottomRight.y >= mapSize31)
                    {
                        topLeft.y -= mapSize31;
                        bottomRight.y -= mapSize31;
                    }
                    symbolArea = AreaI(topLeft.y, topLeft.x, bottomRight.y, bottomRight.x);
                    symbolRect << symbolArea.topLeft << symbolArea.topRight() << symbolArea.bottomRight << symbolArea.bottomLeft();
                    break;
                case VectorMapSymbol::ScaleType::InMeters:
                    symbolArea64 = Utilities::boundingBox31FromAreaInMeters(vectorMapSymbol->scale, position31);
                    if (symbolArea64.right() >= mapSize31)
                    {
                        symbolArea64.left() -= mapSize31;
                        symbolArea64.right() -= mapSize31;
                    }
                    if (symbolArea64.bottom() >= mapSize31)
                    {
                        symbolArea64.top() -= mapSize31;
                        symbolArea64.bottom() -= mapSize31;
                    }
                    symbolArea = (AreaI)symbolArea64;
                    symbolRect << symbolArea.topLeft << symbolArea.topRight() << symbolArea.bottomRight << symbolArea.bottomLeft();
                    break;
            }
            if (height != 0.0f)
            {
                PointI elevatedPoint;
                for (auto& symbolPoint : symbolRect)
                {
                    if (getRenderer()->getProjectedLocation(
                        internalState, currentState, symbolPoint, height, elevatedPoint))
                    {
                        symbolPoint = elevatedPoint;
                    }
                }
            }
            if (!internalState.globalFrustum2D31.test(symbolRect) && !internalState.extraFrustum2D31.test(symbolRect))
            {
                if (metric)
                    metric->onSurfaceSymbolsRejectedByFrustum++;
                return;
            }
        }
        else
        {
            PointI testPoint;
            testPoint = Utilities::normalizeCoordinates(position31, ZoomLevel31);
            if (height != 0.0f)
                getRenderer()->getProjectedLocation(internalState, currentState, position31, height, testPoint);
            if (!internalState.globalFrustum2D31.test(testPoint) && !internalState.extraFrustum2D31.test(testPoint))
            {
                if (metric)
                    metric->onSurfaceSymbolsRejectedByFrustum++;
                return;
            }
        }
    }

    // Get GPU resource
    const auto gpuResource = captureGpuResource(referenceOrigins, mapSymbol);
    if (!gpuResource)
        return;

    if (const auto& gpuMeshResource = std::dynamic_pointer_cast<const GPUAPI::MeshInGPU>(gpuResource))
    {
        if (gpuMeshResource->position31 != nullptr)
            position31 = PointI(gpuMeshResource->position31->x, gpuMeshResource->position31->y);
    }

    PointF offsetInTileN;
    const auto tileId = Utilities::normalizeTileId(Utilities::getTileId(Utilities::normalizeCoordinates(
        position31, ZoomLevel31), currentState.zoomLevel, &offsetInTileN), currentState.zoomLevel);

    // Get elevation data
    float elevationInMeters = 0.0f;
    float elevationInWorld = 0.0f;
    std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
    PointF offsetInScaledTileN = offsetInTileN;
    if (getElevationData(tileId, currentState.zoomLevel, offsetInScaledTileN, &elevationData) != InvalidZoomLevel &&
        elevationData)
    {
        if (elevationData->getValue(offsetInScaledTileN, elevationInMeters))
        {
            const auto scaledElevationInMeters =
                elevationInMeters * currentState.elevationConfiguration.dataScaleFactor;

            const auto upperMetersPerUnit =
                Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y, AtlasMapRenderer::TileSize3D);
            const auto lowerMetersPerUnit =
                Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y + 1, AtlasMapRenderer::TileSize3D);
            const auto metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);

            elevationInWorld =
                (scaledElevationInMeters / metersPerUnit) * currentState.elevationConfiguration.zScaleFactor;
        }
    }

    // Don't render fully transparent symbols
    const auto opacityFactor = getSubsectionOpacityFactor(mapSymbol);
    if (opacityFactor > 0.0f)
    {
        std::shared_ptr<RenderableOnSurfaceSymbol> renderable(new RenderableOnSurfaceSymbol());
        renderable->mapSymbolGroup = mapSymbolGroup;
        renderable->mapSymbol = mapSymbol;
        renderable->genericInstanceParameters = instanceParameters;
        renderable->instanceParameters = instanceParameters;
        renderable->gpuResource = gpuResource;
        renderable->elevationInMeters = elevationInMeters;
        renderable->tileId = tileId;
        renderable->offsetInTileN = offsetInTileN;        
        renderable->opacityFactor = opacityFactor;
        outRenderableSymbols.push_back(renderable);

        // Calculate location of symbol in world coordinates.
        int64_t intFull = INT32_MAX;
        intFull++;
        const auto intHalf = intFull >> 1;
        const auto intTwo = intFull << 1;
        PointI64 position = position31;
        position.x += position.x < -intHalf ? intTwo : 0;
        position.y += position.y < -intHalf ? intTwo : 0;
        const PointI64 offset = position - currentState.target31;
        renderable->offsetFromTarget31 = Utilities::wrapCoordinates(offset);
        renderable->offsetFromTarget = PointF(Utilities::convert31toDouble(offset, currentState.zoomLevel));
        renderable->positionInWorld = glm::vec3(
            renderable->offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
            elevationInWorld,
            renderable->offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

        // Get direction
        if (IOnSurfaceMapSymbol::isAzimuthAlignedDirection(direction))
            renderable->direction = Utilities::normalizedAngleDegrees(currentState.azimuth + 180.0f);
        else
            renderable->direction = direction;

        // Get distance from symbol to camera
        renderable->distanceToCamera = glm::distance(internalState.worldCameraPosition, renderable->positionInWorld);
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/) const
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return plotOnSurfaceRasterSymbol( renderable, intersections, applyFiltering, metric);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        return plotOnSurfaceVectorSymbol(renderable, intersections, applyFiltering, metric);
    }

    assert(false);
    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceRasterSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceRasterMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceVectorSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceVectorMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    return true;
}

void OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderablesFromOnPathSymbol(
    const std::shared_ptr<const MapSymbolsGroup>& mapSymbolGroup,
    const std::shared_ptr<const OnPathRasterMapSymbol>& onPathMapSymbol,
    const std::shared_ptr<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters>& instanceParameters,
    const MapRenderer::MapSymbolReferenceOrigins& referenceOrigins,
    ComputedPathsDataCache& computedPathsDataCache,
    QList< std::shared_ptr<RenderableSymbol> >& outRenderableSymbols,
    ScreenQuadTree& intersections,
    bool allowFastCheckByFrustum /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
{
    // Path must have at least 2 points and there must be at least one pin-point
    if (Q_UNLIKELY(onPathMapSymbol->shareablePath31->size() < 2))
    {
        assert(false);
        return;
    }

    const auto& internalState = getInternalState();

    const auto& pinPointOnPath =
        (instanceParameters && instanceParameters->overridesPinPointOnPath)
        ? instanceParameters->pinPointOnPath
        : onPathMapSymbol->pinPointOnPath;

    // Test against visible frustum area (if allowed)
    if (!debugSettings->disableSymbolsFastCheckByFrustum &&
        allowFastCheckByFrustum &&
        onPathMapSymbol->allowFastCheckByFrustum &&
        !internalState.globalFrustum2D31.test(pinPointOnPath.point31) &&
        !internalState.extraFrustum2D31.test(pinPointOnPath.point31))
    {
        if (metric)
            metric->onPathSymbolsRejectedByFrustum++;
        return;
    }

    ////////////////////////////////////////////////////////////////////////////
    /*
    {
        if (const auto symbolsGroupWithId = std::dynamic_pointer_cast<MapSymbolsGroupWithId>(onPathMapSymbol->group.lock()))
        {
            if ((symbolsGroupWithId->id >> 1) != 244622303)
                return;
        }
    }
    */
    ////////////////////////////////////////////////////////////////////////////

    // Get GPU resource for this map symbol, since it's useless to perform any calculations unless it's possible to draw it
    const auto gpuResource = std::dynamic_pointer_cast<const GPUAPI::TextureInGPU>(captureGpuResource(referenceOrigins, onPathMapSymbol));
    if (!gpuResource)
        return;

    // Processing pin-point needs path in world and path on screen, as well as lengths of all segments. This may have already been computed
    auto itComputedPathData = computedPathsDataCache.find(onPathMapSymbol->shareablePath31);
    if (itComputedPathData == computedPathsDataCache.end())
    {
        const auto& path31 = *onPathMapSymbol->shareablePath31;

        ComputedPathData computedPathData;

        //TODO: optimize by using lazy computation
        computedPathData.pathInWorld = convertPoints31ToWorld(path31);
        computedPathData.pathSegmentsLengthsInWorld = computePathSegmentsLengths(computedPathData.pathInWorld);
        computedPathData.pathOnScreen = projectFromWorldToScreen(computedPathData.pathInWorld);
        computedPathData.pathSegmentsLengthsOnScreen = computePathSegmentsLengths(computedPathData.pathOnScreen);

        itComputedPathData = computedPathsDataCache.insert(onPathMapSymbol->shareablePath31, computedPathData);
    }
    const auto& computedPathData = *itComputedPathData;

    // Pin-point represents center of symbol
    const auto halfSizeInPixels = onPathMapSymbol->size.x / 2.0f;
    bool fits = true;
    bool is2D = true;

    // Check if this symbol instance can be rendered in 2D mode
    glm::vec2 exactStartPointOnScreen(0.0f, 0.0f);
    glm::vec2 exactEndPointOnScreen(0.0f, 0.0f);
    unsigned int startPathPointIndex2D = 0;
    float offsetFromStartPathPoint2D = 0.0f;
    fits = fits && computePointIndexAndOffsetFromOriginAndOffset(
        computedPathData.pathSegmentsLengthsOnScreen,
        pinPointOnPath.basePathPointIndex,
        pinPointOnPath.normalizedOffsetFromBasePathPoint,
        -halfSizeInPixels,
        startPathPointIndex2D,
        offsetFromStartPathPoint2D);
    unsigned int endPathPointIndex2D = 0;
    float offsetFromEndPathPoint2D = 0.0f;
    fits = fits && computePointIndexAndOffsetFromOriginAndOffset(
        computedPathData.pathSegmentsLengthsOnScreen,
        pinPointOnPath.basePathPointIndex,
        pinPointOnPath.normalizedOffsetFromBasePathPoint,
        halfSizeInPixels,
        endPathPointIndex2D,
        offsetFromEndPathPoint2D);
    if (fits)
    {
        exactStartPointOnScreen = computeExactPointFromOriginAndOffset(
            computedPathData.pathOnScreen,
            computedPathData.pathSegmentsLengthsOnScreen,
            startPathPointIndex2D,
            offsetFromStartPathPoint2D);
        exactEndPointOnScreen = computeExactPointFromOriginAndOffset(
            computedPathData.pathOnScreen,
            computedPathData.pathSegmentsLengthsOnScreen,
            endPathPointIndex2D,
            offsetFromEndPathPoint2D);

        is2D = pathRenderableAs2D(
            computedPathData.pathOnScreen,
            startPathPointIndex2D,
            exactStartPointOnScreen,
            endPathPointIndex2D,
            exactEndPointOnScreen);
    }

    // If 2D failed, check if renderable as 3D
    glm::vec2 exactStartPointInWorld(0.0f, 0.0f);
    glm::vec2 exactEndPointInWorld(0.0f, 0.0f);
    unsigned int startPathPointIndex3D = 0;
    float offsetFromStartPathPoint3D = 0.0f;
    unsigned int endPathPointIndex3D = 0;
    float offsetFromEndPathPoint3D = 0.0f;
    if (!fits || !is2D)
    {
        is2D = false;
        const auto halfSizeInWorld = halfSizeInPixels * internalState.pixelInWorldProjectionScale;

        fits = true;
        fits = fits && computePointIndexAndOffsetFromOriginAndOffset(
            computedPathData.pathSegmentsLengthsInWorld,
            pinPointOnPath.basePathPointIndex,
            pinPointOnPath.normalizedOffsetFromBasePathPoint,
            -halfSizeInWorld,
            startPathPointIndex3D,
            offsetFromStartPathPoint3D);
        fits = fits && computePointIndexAndOffsetFromOriginAndOffset(
            computedPathData.pathSegmentsLengthsInWorld,
            pinPointOnPath.basePathPointIndex,
            pinPointOnPath.normalizedOffsetFromBasePathPoint,
            halfSizeInWorld,
            endPathPointIndex3D,
            offsetFromEndPathPoint3D);

        if (fits)
        {
            exactStartPointInWorld = computeExactPointFromOriginAndOffset(
                computedPathData.pathInWorld,
                computedPathData.pathSegmentsLengthsInWorld,
                startPathPointIndex3D,
                offsetFromStartPathPoint3D);
            exactEndPointInWorld = computeExactPointFromOriginAndOffset(
                computedPathData.pathInWorld,
                computedPathData.pathSegmentsLengthsInWorld,
                endPathPointIndex3D,
                offsetFromEndPathPoint3D);
        }
    }

    // If this symbol instance doesn't fit in both 2D and 3D, skip it
    if (!fits)
        return;

    // Compute exact points
    if (is2D)
    {
        // Get 3D exact points from 2D
        exactStartPointInWorld = computeExactPointFromOriginAndNormalizedOffset(
            computedPathData.pathInWorld,
            startPathPointIndex2D,
            offsetFromStartPathPoint2D / computedPathData.pathSegmentsLengthsOnScreen[startPathPointIndex2D]);
        exactEndPointInWorld = computeExactPointFromOriginAndNormalizedOffset(
            computedPathData.pathInWorld,
            endPathPointIndex2D,
            offsetFromEndPathPoint2D / computedPathData.pathSegmentsLengthsOnScreen[endPathPointIndex2D]);
    }
    else
    {
        // Get 2D exact points from 3D
        exactStartPointOnScreen = computeExactPointFromOriginAndNormalizedOffset(
            computedPathData.pathOnScreen,
            startPathPointIndex3D,
            offsetFromStartPathPoint3D / computedPathData.pathSegmentsLengthsInWorld[startPathPointIndex3D]);
        exactEndPointOnScreen = computeExactPointFromOriginAndNormalizedOffset(
            computedPathData.pathOnScreen,
            endPathPointIndex3D,
            offsetFromEndPathPoint3D / computedPathData.pathSegmentsLengthsInWorld[endPathPointIndex3D]);
    }

    // Compute direction of subpath on screen and in world
    const auto subpathStartIndex = is2D ? startPathPointIndex2D : startPathPointIndex3D;
    const auto subpathEndIndex = is2D ? endPathPointIndex2D : endPathPointIndex3D;
    assert(subpathEndIndex >= subpathStartIndex);

    const auto originaPathDirectionOnScreen = glm::normalize(exactEndPointOnScreen - exactStartPointOnScreen);
    // Compute path to place glyphs on
    const auto path = computePathForGlyphsPlacement(
        is2D,
        computedPathData.pathOnScreen,
        computedPathData.pathSegmentsLengthsOnScreen,
        computedPathData.pathInWorld,
        computedPathData.pathSegmentsLengthsInWorld,
        subpathStartIndex,
        is2D ? offsetFromStartPathPoint2D : offsetFromStartPathPoint3D,
        subpathEndIndex,
        originaPathDirectionOnScreen,
        onPathMapSymbol->glyphsWidth);

    if (path.countPoints() < 2)
        return;

    bool ok = true;
    const auto pathInWorld = is2D ? convertPathOnScreenToWorld(path, ok) : path;
    if (!ok)
        return;

    const auto pathOnScreen = is2D ? path : projectPathInWorldToScreen(path);

    const auto directionInWorld = computePathDirection(pathInWorld);
    const auto directionOnScreen = computePathDirection(pathOnScreen);

    // Don't render fully transparent symbols
    const auto opacityFactor = getSubsectionOpacityFactor(onPathMapSymbol);
    if (opacityFactor > 0.0f)
    {
        QVector<RenderableOnPathSymbol::GlyphPlacement> glyphsPlacement;
        QVector<glm::vec3> rotatedElevatedBBoxInWorld;
        bool ok = computePlacementOfGlyphsOnPath(
            path,
            is2D,
            directionInWorld,
            directionOnScreen,
            onPathMapSymbol->glyphsWidth,
            onPathMapSymbol->size.y,
            glyphsPlacement,
            rotatedElevatedBBoxInWorld);
        
        if (ok)
        {
            std::shared_ptr<RenderableOnPathSymbol> renderable(new RenderableOnPathSymbol());
            renderable->mapSymbolGroup = mapSymbolGroup;
            renderable->mapSymbol = onPathMapSymbol;
            renderable->genericInstanceParameters = instanceParameters;
            renderable->instanceParameters = instanceParameters;
            renderable->gpuResource = gpuResource;
            renderable->is2D = is2D;
            renderable->distanceToCamera = computeDistanceFromCameraToPath(pathInWorld);
            renderable->directionInWorld = directionInWorld;
            renderable->directionOnScreen = directionOnScreen;
            renderable->glyphsPlacement = glyphsPlacement;
            renderable->rotatedElevatedBBoxInWorld = rotatedElevatedBBoxInWorld;
            renderable->opacityFactor = opacityFactor;

            // Calculate OOBB for 2D SOP
            const auto oobb = calculateOnPath2dOOBB(renderable);
            renderable->visibleBBox = renderable->intersectionBBox = (OOBBI)oobb;

            if (!applyOnScreenVisibilityFiltering(renderable->visibleBBox, intersections, metric))
                return;

            const auto pointsCount = rotatedElevatedBBoxInWorld.size();
            const auto p0 = rotatedElevatedBBoxInWorld[0];
            auto p1 = p0;
            auto p2 = p0;
            const auto p3 = rotatedElevatedBBoxInWorld[pointsCount - 1];
            if (pointsCount > 3)
            {
                const auto gap = pointsCount / 3;
                p1 = rotatedElevatedBBoxInWorld[gap];
                p2 = rotatedElevatedBBoxInWorld[gap * 2];
            }
            else if (pointsCount > 2)
                p2 = rotatedElevatedBBoxInWorld[1];

            const auto cameraVectorN = internalState.worldCameraPosition / internalState.distanceFromCameraToTarget;
            if (glm::dot(internalState.worldCameraPosition - p0, cameraVectorN) /
                internalState.distanceFromCameraToTarget > VISIBLE_DISTANCE_FACTOR)
                return;
            if (glm::dot(internalState.worldCameraPosition - p1, cameraVectorN) /
                internalState.distanceFromCameraToTarget > VISIBLE_DISTANCE_FACTOR)
                return;
            if (glm::dot(internalState.worldCameraPosition - p2, cameraVectorN) /
                internalState.distanceFromCameraToTarget > VISIBLE_DISTANCE_FACTOR)
                return;
            if (glm::dot(internalState.worldCameraPosition - p3, cameraVectorN) /
                internalState.distanceFromCameraToTarget > VISIBLE_DISTANCE_FACTOR)
                return;

            if (allowFastCheckByFrustum)
                renderable->queryIndex = startTerrainVisibilityFiltering(PointF(-1.0f, -1.0f), p0, p1, p2, p3);
            else
                renderable->queryIndex = -1;

            outRenderableSymbols.push_back(renderable);
        }
    }

    if (Q_UNLIKELY(debugSettings->showOnPathSymbolsRenderablesPaths))
    {
        const glm::vec2 directionOnScreenN(-directionOnScreen.y, directionOnScreen.x);

        {
            QVector< glm::vec3 > debugPathPoints;
            for (auto i = 0; i < pathInWorld.countPoints(); i++)
            {
                const auto skPoint = pathInWorld.getPoint(i);
                const glm::vec3 point(skPoint.x(), 0.0f, skPoint.y());
                debugPathPoints.push_back(point);
            }
            getRenderer()->debugStage->addLine3D(debugPathPoints, is2D ? SK_ColorGREEN : SK_ColorRED);
        }

        // Subpath N (start)
        {
            QVector<glm::vec2> lineN;
            const auto skStartPoint = pathOnScreen.getPoint(0);
            const glm::vec2 start(skStartPoint.x(), skStartPoint.y());
            const glm::vec2 end = start + directionOnScreenN * 24.0f;
            lineN.push_back(glm::vec2(start.x, currentState.windowSize.y - start.y));
            lineN.push_back(glm::vec2(end.x, currentState.windowSize.y - end.y));
            getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorCYAN, 128));
        }

        // Subpath N (end)
        {
            QVector<glm::vec2> lineN;
            const auto skStartPoint = pathOnScreen.getPoint(pathOnScreen.countPoints() - 1);
            const glm::vec2 start(skStartPoint.x(), skStartPoint.y());
            const glm::vec2 end = start + directionOnScreenN * 24.0f;
            lineN.push_back(glm::vec2(start.x, currentState.windowSize.y - start.y));
            lineN.push_back(glm::vec2(end.x, currentState.windowSize.y - end.y));
            getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
        }

        // Pin-point location
        {
            const auto pinPointInWorld = Utilities::convert31toFloat(
                pinPointOnPath.point31 - currentState.target31,
                currentState.zoomLevel) * static_cast<float>(AtlasMapRenderer::TileSize3D);
            const auto pinPointOnScreen = glm_extensions::project(
                glm::vec3(pinPointInWorld.x, 0.0f, pinPointInWorld.y),
                internalState.mPerspectiveProjectionView,
                internalState.glmViewport).xy();

            {
                QVector<glm::vec2> lineN;
                const auto sn0 = pinPointOnScreen;
                lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
                const auto sn1 = sn0 + (directionOnScreenN*32.0f);
                lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
                getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorWHITE, 128));
            }

            {
                QVector<glm::vec2> lineN;
                const auto sn0 = pinPointOnScreen;
                lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
                const auto sn1 = sn0 + (directionOnScreen*32.0f);
                lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
                getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorGREEN, 128));
            }
        }
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnPathSymbol(
    const std::shared_ptr<RenderableOnPathSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Draw the glyphs
    if (renderable->is2D)
    {
        if (applyFiltering)
        {
            if(renderable->queryIndex > -1 && !applyTerrainVisibilityFiltering(renderable->queryIndex, metric))
                return false;

            //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
            //        oobb.enlargeBy(PointF(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

            if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
                return false;

            if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
                return false;

            if (!addToIntersections(renderable, intersections, metric))
                return false;
        }

        if (Q_UNLIKELY(debugSettings->showOnPath2dSymbolGlyphDetails))
        {
            for (const auto& glyph : constOf(renderable->glyphsPlacement))
            {
                getRenderer()->debugStage->addRect2D(AreaF::fromCenterAndSize(
                    glyph.anchorPoint.x, currentState.windowSize.y - glyph.anchorPoint.y,
                    glyph.width, symbol->size.y), SkColorSetA(SK_ColorGREEN, 128), glyph.angleY);

                QVector<glm::vec2> lineN;
                const auto ln0 = glyph.anchorPoint;
                lineN.push_back(glm::vec2(ln0.x, currentState.windowSize.y - ln0.y));
                const auto ln1 = glyph.anchorPoint + (glyph.vNormal*16.0f);
                lineN.push_back(glm::vec2(ln1.x, currentState.windowSize.y - ln1.y));
                getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
            }
        }
    }
    else
    {
        if (applyFiltering)
        {
            if(renderable->queryIndex > -1 && !applyTerrainVisibilityFiltering(renderable->queryIndex, metric))
                return false;

            //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
            //        oobb.enlargeBy(PointF(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

            if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
                return false;

            if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
                return false;

            if (!addToIntersections(renderable, intersections, metric))
                return false;
        }

        if (Q_UNLIKELY(debugSettings->showOnPath3dSymbolGlyphDetails))
        {
            for (const auto& glyph : constOf(renderable->glyphsPlacement))
            {
                const auto elevation = glyph.elevation;
                const auto& glyphInMapPlane = AreaF::fromCenterAndSize(
                    glyph.anchorPoint.x, glyph.anchorPoint.y, /* anchor points are specified in world coordinates already */
                    glyph.width*internalState.pixelInWorldProjectionScale, symbol->size.y*internalState.pixelInWorldProjectionScale);
                const auto& tl = glyphInMapPlane.topLeft;
                const auto& tr = glyphInMapPlane.topRight();
                const auto& br = glyphInMapPlane.bottomRight;
                const auto& bl = glyphInMapPlane.bottomLeft();
                const glm::vec3 pC(glyph.anchorPoint.x, elevation, glyph.anchorPoint.y);
                const glm::vec4 p0(tl.x, elevation, tl.y, 1.0f);
                const glm::vec4 p1(tr.x, elevation, tr.y, 1.0f);
                const glm::vec4 p2(br.x, elevation, br.y, 1.0f);
                const glm::vec4 p3(bl.x, elevation, bl.y, 1.0f);
                const auto toCenter = glm::translate(-pC);
                const auto rotateY = glm::rotate(
                    (float)Utilities::normalizedAngleRadians(glyph.angleY + M_PI),
                    glm::vec3(0.0f, -1.0f, 0.0f));
                const auto rotateXZ = glm::rotate(glyph.angleXZ, glm::vec3(glyph.rotationX, 0.0f, glyph.rotationZ));
                const auto fromCenter = glm::translate(pC);
                const auto M = fromCenter * rotateXZ * rotateY * toCenter;
                getRenderer()->debugStage->addQuad3D(
                    (M*p0).xyz(),
                    (M*p1).xyz(),
                    (M*p2).xyz(),
                    (M*p3).xyz(),
                    SkColorSetA(SK_ColorGREEN, 128));

                QVector<glm::vec3> lineN;
                const auto ln0 = glyph.anchorPoint;
                lineN.push_back(glm::vec3(ln0.x, elevation, ln0.y));
                const auto ln1 = glyph.anchorPoint + (glyph.vNormal*16.0f*internalState.pixelInWorldProjectionScale);
                lineN.push_back(glm::vec3(ln1.x, elevation, ln1.y));
                getRenderer()->debugStage->addLine3D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
            }
        }
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::applyOnScreenVisibilityFiltering(
    const ScreenQuadTree::BBox& visibleBBox,
    const ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    Stopwatch stopwatch(metric != nullptr);

    const auto mayBeVisible =
        visibleBBox.isContainedBy(intersections.rootArea()) ||
        visibleBBox.isIntersectedBy(intersections.rootArea()) ||
        visibleBBox.contains(intersections.rootArea());

    if (metric)
    {
        metric->elapsedTimeForApplyVisibilityFilteringCalls += stopwatch.elapsed();
        metric->applyVisibilityFilteringCalls++;
        if (!mayBeVisible)
            metric->rejectedByVisibilityFiltering++;
        else
            metric->acceptedByVisibilityFiltering++;
    }

    return mayBeVisible;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::applyIntersectionWithOtherSymbolsFiltering(
    const std::shared_ptr<const RenderableSymbol>& renderable,
    const ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    if (Q_UNLIKELY(debugSettings->skipSymbolsIntersectionCheck))
        return true;

    const auto& symbol = renderable->mapSymbol;

    if (symbol->intersectsWithClasses.isEmpty())
        return true;

    Stopwatch stopwatch(metric != nullptr);

    // Check intersections
    const auto checkIntersectionsWithinGroup = renderable->mapSymbolGroup->intersectionProcessingMode.isSet(
        MapSymbolsGroup::IntersectionProcessingModeFlag::CheckIntersectionsWithinGroup);
    const auto& intersectionClassesRegistry = MapSymbolIntersectionClassesRegistry::globalInstance();
    const auto& symbolIntersectsWithClasses = symbol->intersectsWithClasses;
    const auto anyIntersectionClass = intersectionClassesRegistry.anyClass;
    const auto symbolIntersectsWithAnyClass = symbolIntersectsWithClasses.contains(intersectionClassesRegistry.anyClass);
    const auto symbolGroupPtr = symbol->groupPtr;
    const auto symbolGroupInstancePtr = renderable->genericInstanceParameters
        ? renderable->genericInstanceParameters->groupInstancePtr
        : nullptr;
    const auto intersects = intersections.test(renderable->intersectionBBox, false,
        [symbolGroupPtr, symbolIntersectsWithClasses, symbolIntersectsWithAnyClass, anyIntersectionClass, symbolGroupInstancePtr, checkIntersectionsWithinGroup]
        (const std::shared_ptr<const RenderableSymbol>& otherRenderable, const ScreenQuadTree::BBox& otherBBox) -> bool
        {
            const auto& otherSymbol = otherRenderable->mapSymbol;

            // Symbols never intersect anything inside own group (different instances are treated as different groups)
            if (!checkIntersectionsWithinGroup && symbolGroupPtr == otherSymbol->groupPtr)
            {
                const auto otherSymbolGroupInstancePtr = otherRenderable->genericInstanceParameters
                    ? otherRenderable->genericInstanceParameters->groupInstancePtr
                    : nullptr;
                if (symbolGroupInstancePtr == otherSymbolGroupInstancePtr)
                    return false;
            }

            // Special case: tested symbol intersects any other symbol with at least 1 any class
            if (symbolIntersectsWithAnyClass && !otherSymbol->intersectsWithClasses.isEmpty())
                return true;

            // Special case: other symbol intersects tested symbol with at least 1 any class (which is true already)
            if (otherSymbol->intersectsWithClasses.contains(anyIntersectionClass))
                return true;

            // General case:
            const auto commonIntersectionClasses = symbolIntersectsWithClasses & otherSymbol->intersectsWithClasses;
            const auto hasCommonIntersectionClasses = !commonIntersectionClasses.isEmpty();
            return hasCommonIntersectionClasses;
        });

    if (metric)
    {
        metric->elapsedTimeForApplyIntersectionWithOtherSymbolsFilteringCalls += stopwatch.elapsed();
        metric->applyIntersectionWithOtherSymbolsFilteringCalls++;
        if (intersects)
            metric->rejectedByIntersectionWithOtherSymbolsFiltering++;
        else
            metric->acceptedByIntersectionWithOtherSymbolsFiltering++;
    }

    if (intersects)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesRejectedByIntersectionCheck))
            addIntersectionDebugBox(renderable, ColorARGB::fromSkColor(SK_ColorRED).withAlpha(50));
        return false;
    }
    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::applyMinDistanceToSameContentFromOtherSymbolFiltering(
    const std::shared_ptr<const RenderableSymbol>& renderable,
    const ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    if (Q_UNLIKELY(debugSettings->skipSymbolsMinDistanceToSameContentFromOtherSymbolCheck))
        return true;

    const auto& symbol = std::static_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol);

    if (symbol->minDistance <= 0.0f || symbol->content.isNull())
        return true;

    Stopwatch stopwatch(metric != nullptr);

    // Query for similar content from other groups in area of "minDistance" to exclude duplicates
    const auto symbolGroupPtr = symbol->groupPtr;
    const auto symbolGroupInstancePtr = renderable->genericInstanceParameters
        ? renderable->genericInstanceParameters->groupInstancePtr
        : nullptr;
    const auto& symbolContent = symbol->content;
    const auto hasSimilarContent = intersections.test(renderable->intersectionBBox.getEnlargedBy(symbol->minDistance), false,
        [symbolContent, symbolGroupPtr, symbolGroupInstancePtr]
        (const std::shared_ptr<const RenderableSymbol>& otherRenderable, const ScreenQuadTree::BBox& otherBBox) -> bool
        {
            const auto otherSymbol = std::dynamic_pointer_cast<const RasterMapSymbol>(otherRenderable->mapSymbol);
            if (!otherSymbol)
                return false;

            if (symbolGroupPtr == otherSymbol->groupPtr)
            {
                const auto otherSymbolGroupInstancePtr = otherRenderable->genericInstanceParameters
                    ? otherRenderable->genericInstanceParameters->groupInstancePtr
                    : nullptr;
                if (symbolGroupInstancePtr == otherSymbolGroupInstancePtr)
                    return false;
            }

            const auto shouldCheck = (otherSymbol->content == symbolContent);
            return shouldCheck;
        });

    if (metric)
    {
        metric->elapsedTimeForApplyMinDistanceToSameContentFromOtherSymbolFilteringCalls += stopwatch.elapsed();
        metric->applyMinDistanceToSameContentFromOtherSymbolFilteringCalls ++;
        if (hasSimilarContent)
            metric->rejectedByMinDistanceToSameContentFromOtherSymbolFiltering++;
        else
            metric->acceptedByMinDistanceToSameContentFromOtherSymbolFiltering++;
    }

    if (hasSimilarContent)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck))
            addIntersectionDebugBox(renderable, ColorARGB::fromSkColor(SK_ColorRED).withAlpha(128));

        if (Q_UNLIKELY(debugSettings->showSymbolsCheckBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck))
            addIntersectionDebugBox(renderable->intersectionBBox.getEnlargedBy(symbol->minDistance), ColorARGB::fromSkColor(SK_ColorRED).withAlpha(128), false);

        return false;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::addToIntersections(
    const std::shared_ptr<const RenderableSymbol>& renderable,
    ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    if (Q_UNLIKELY(debugSettings->allSymbolsTransparentForIntersectionLookup))
        return true;

    if (renderable->mapSymbol->intersectsWithClasses.isEmpty())
        return true;

    Stopwatch stopwatch(metric != nullptr);

    const auto inserted = intersections.insert(renderable, renderable->intersectionBBox);

    if (metric)
    {
        metric->elapsedTimeForAddToIntersectionsCalls += stopwatch.elapsed();
        metric->addToIntersectionsCalls++;
        if (!inserted)
            metric->rejectedByAddToIntersections++;
        else
            metric->acceptedByAddToIntersections++;
    }

    if (!inserted)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesRejectedByIntersectionCheck))
            addIntersectionDebugBox(renderable, ColorARGB::fromSkColor(SK_ColorBLUE).withAlpha(50));

        return false;
    }

    return true;
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::convertPoints31ToWorld(
    const QVector<PointI>& points31) const
{
    return convertPoints31ToWorld(points31, 0, points31.size() - 1);
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::convertPoints31ToWorld(
    const QVector<PointI>& points31,
    const unsigned int startIndex,
    const unsigned int endIndex) const
{
    assert(endIndex >= startIndex);
    const auto count = endIndex - startIndex + 1;
    QVector<glm::vec2> result(count);
    auto pPointInWorld = result.data();
    auto pPoint31 = points31.constData() + startIndex;

    for (auto idx = 0u; idx < count; idx++)
    {
        *(pPointInWorld++) =
            Utilities::convert31toFloat(*(pPoint31++) - currentState.target31, currentState.zoomLevel) *
            static_cast<float>(AtlasMapRenderer::TileSize3D);
    }

    return result;
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::projectFromWorldToScreen(
    const QVector<glm::vec2>& pointsInWorld) const
{
    return projectFromWorldToScreen(pointsInWorld, 0, pointsInWorld.size() - 1);
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::projectFromWorldToScreen(
    const QVector<glm::vec2>& pointsInWorld,
    const unsigned int startIndex,
    const unsigned int endIndex) const
{
    const auto& internalState = getInternalState();

    assert(endIndex >= startIndex);
    const auto count = endIndex - startIndex + 1;
    QVector<glm::vec2> result(count);
    auto pPointOnScreen = result.data();
    auto pPointInWorld = pointsInWorld.constData() + startIndex;

    for (auto idx = 0u; idx < count; idx++)
    {
        *(pPointOnScreen++) = glm_extensions::project(
            glm::vec3(pPointInWorld->x, 0.0f, pPointInWorld->y),
            internalState.mPerspectiveProjectionView,
            internalState.glmViewport).xy();
        pPointInWorld++;
    }

    return result;
}

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::AtlasMapRendererSymbolsStage::captureGpuResource(
    const MapRenderer::MapSymbolReferenceOrigins& resources,
    const std::shared_ptr<const MapSymbol>& mapSymbol)
{
    for (auto& resource : constOf(resources))
    {
        std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
        if (resource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
        {
            if (const auto tiledResource = std::dynamic_pointer_cast<MapRendererTiledSymbolsResource>(resource))
                gpuResource = tiledResource->getGpuResourceFor(mapSymbol);
            else if (const auto keyedResource = std::dynamic_pointer_cast<MapRendererKeyedSymbolsResource>(resource))
                gpuResource = keyedResource->getGpuResourceFor(mapSymbol);

            resource->setState(MapRendererResourceState::Uploaded);
        }
        else if (resource->isRenewing())
        {
            if (const auto keyedResource = std::dynamic_pointer_cast<MapRendererKeyedSymbolsResource>(resource))
                gpuResource = keyedResource->getCachedGpuResourceFor(mapSymbol);
        }

        // Stop as soon as GPU resource found
        if (gpuResource)
            return gpuResource;
    }
    return nullptr;
}

QVector<float> OsmAnd::AtlasMapRendererSymbolsStage::computePathSegmentsLengths(const QVector<glm::vec2>& path)
{
    const auto segmentsCount = path.size() - 1;
    QVector<float> lengths(segmentsCount);

    auto pPathPoint = path.constData();
    auto pPrevPathPoint = pPathPoint++;
    auto pLength = lengths.data();
    for (auto segmentIdx = 0; segmentIdx < segmentsCount; segmentIdx++)
        *(pLength++) = glm::distance(*(pPathPoint++), *(pPrevPathPoint++));

    return lengths;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::computePointIndexAndOffsetFromOriginAndOffset(
    const QVector<float>& pathSegmentsLengths,
    const unsigned int originPathPointIndex,
    const float nOffsetFromOriginPathPoint,
    const float offsetToPoint,
    unsigned int& outPathPointIndex,
    float& outOffsetFromPathPoint)
{
    const auto pathSegmentsCount = pathSegmentsLengths.size();
    const auto offsetFromOriginPathPoint =
        (pathSegmentsLengths[originPathPointIndex] * nOffsetFromOriginPathPoint) + offsetToPoint;

    if (offsetFromOriginPathPoint >= 0.0f)
    {
        // In case start point is located after origin point ('on the right'), usual search is used

        auto testPathPointIndex = originPathPointIndex;
        auto scannedLength = 0.0f;
        while (scannedLength < offsetFromOriginPathPoint)
        {
            if (testPathPointIndex >= pathSegmentsCount)
                return false;
            const auto& segmentLength = pathSegmentsLengths[testPathPointIndex];
            if (scannedLength + segmentLength > offsetFromOriginPathPoint)
            {
                outPathPointIndex = testPathPointIndex;
                outOffsetFromPathPoint = offsetFromOriginPathPoint - scannedLength;
                assert(outOffsetFromPathPoint >= 0.0f);
            }
            scannedLength += segmentLength;
            testPathPointIndex++;
        }
    }
    else
    {
        // In case start point is located before origin point ('on the left'), reversed search is used
        if (originPathPointIndex == 0)
            return false;

        auto testPathPointIndex = originPathPointIndex - 1;
        auto scannedLength = 0.0f;
        while (scannedLength > offsetFromOriginPathPoint)
        {
            const auto& segmentLength = pathSegmentsLengths[testPathPointIndex];
            if (scannedLength - segmentLength < offsetFromOriginPathPoint)
            {
                outPathPointIndex = testPathPointIndex;
                outOffsetFromPathPoint = segmentLength + (offsetFromOriginPathPoint - scannedLength);
                assert(outOffsetFromPathPoint >= 0.0f);
            }
            scannedLength -= segmentLength;
            if (testPathPointIndex == 0 && scannedLength > offsetFromOriginPathPoint)
                return false;
            testPathPointIndex--;
        }
    }

    return true;
}

glm::vec2 OsmAnd::AtlasMapRendererSymbolsStage::computeExactPointFromOriginAndOffset(
    const QVector<glm::vec2>& path,
    const QVector<float>& pathSegmentsLengths,
    const unsigned int originPathPointIndex,
    const float offsetFromOriginPathPoint)
{
    assert(offsetFromOriginPathPoint >= 0.0f);

    const auto& originPoint = path[originPathPointIndex + 0];
    const auto& nextPoint = path[originPathPointIndex + 1];
    const auto& length = pathSegmentsLengths[originPathPointIndex];
    assert(offsetFromOriginPathPoint <= length);

    const auto exactPoint = originPoint + (nextPoint - originPoint) * (offsetFromOriginPathPoint / length);

    return exactPoint;
}

glm::vec2 OsmAnd::AtlasMapRendererSymbolsStage::computeExactPointFromOriginAndNormalizedOffset(
    const QVector<glm::vec2>& path,
    const unsigned int originPathPointIndex,
    const float nOffsetFromOriginPathPoint)
{
    assert(nOffsetFromOriginPathPoint >= 0.0f && nOffsetFromOriginPathPoint <= 1.0f);

    const auto& originPoint = path[originPathPointIndex + 0];
    const auto& nextPoint = path[originPathPointIndex + 1];

    const auto exactPoint = originPoint + (nextPoint - originPoint) * nOffsetFromOriginPathPoint;

    return exactPoint;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::pathRenderableAs2D(
    const QVector<glm::vec2>& pathOnScreen,
    const unsigned int startPathPointIndex,
    const glm::vec2& exactStartPointOnScreen,
    const unsigned int endPathPointIndex,
    const glm::vec2& exactEndPointOnScreen)
{
    assert(endPathPointIndex >= startPathPointIndex);

    if (endPathPointIndex > startPathPointIndex)
    {
        const auto segmentsCount = endPathPointIndex - startPathPointIndex + 1;

        // First check segment between exact start point (which is after startPathPointIndex)
        // and next point (startPathPointIndex + 1)
        if (!segmentValidFor2D(pathOnScreen[startPathPointIndex + 1] - exactStartPointOnScreen))
            return false;

        auto pPathPoint = pathOnScreen.constData() + startPathPointIndex + 1;
        auto pPrevPathPoint = pPathPoint++;
        for (auto segmentIdx = 1; segmentIdx < segmentsCount - 1; segmentIdx++)
        {
            const auto vSegment = (*(pPathPoint++) - *(pPrevPathPoint++));
            if (!segmentValidFor2D(vSegment))
                return false;
        }

        // Last check is between pathOnScreen[endPathPointIndex] and exact end point,
        // which is always after endPathPointIndex
        assert(pPrevPathPoint == &pathOnScreen[endPathPointIndex]);
        return segmentValidFor2D(exactEndPointOnScreen - *pPrevPathPoint);
    }
    else
    {
        // In case both exact points are on the same segment, check is simple
        return segmentValidFor2D(exactEndPointOnScreen - exactStartPointOnScreen);
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage::segmentValidFor2D(const glm::vec2& vSegment)
{
    // Calculate 'incline' of line and compare to horizontal direction.
    // If any 'incline' is larger than 15 degrees, this line can not be rendered as 2D

    const static float inclineThresholdSinSq = 0.0669872981f; // qSin(qDegreesToRadians(15.0f))*qSin(qDegreesToRadians(15.0f))
    const auto d = vSegment.y;// horizont.x*vSegment.y - horizont.y*vSegment.x == 1.0f*vSegment.y - 0.0f*vSegment.x
    const auto inclineSinSq = d*d / (vSegment.x*vSegment.x + vSegment.y*vSegment.y);
    if (qAbs(inclineSinSq) > inclineThresholdSinSq)
        return false;
    return true;
}

SkPath OsmAnd::AtlasMapRendererSymbolsStage::computePathForGlyphsPlacement(
    const bool is2D,
    const QVector<glm::vec2>& pathOnScreen,
    const QVector<float>& pathSegmentsLengthsOnScreen,
    const QVector<glm::vec2>& pathInWorld,
    const QVector<float>& pathSegmentsLengthsInWorld,
    const unsigned int startPathPointIndex,
    const float offsetFromStartPathPoint,
    const unsigned int endPathPointIndex,
    const glm::vec2& directionOnScreen,
    const QVector<float>& glyphsWidths) const
{
    const auto& internalState = getInternalState();

    assert(endPathPointIndex >= startPathPointIndex);
    const auto projectionScale = is2D ? 1.0f : internalState.pixelInWorldProjectionScale;

    const auto glyphsCount = glyphsWidths.size();

    const auto atLeastTwoSegments = endPathPointIndex > startPathPointIndex;
    const auto forceDrawGlyphsOnStaright = glyphsCount <= 4 && atLeastTwoSegments;

    if (forceDrawGlyphsOnStaright)
    {
        const auto originalPathSegmentsCount = endPathPointIndex - startPathPointIndex + 1; 
        const auto originalPointsCount = originalPathSegmentsCount + 1;

        // New path intersects points on 1/3 and 2/3 of original path
        const auto firstPointOnPathIndex = (startPathPointIndex + originalPointsCount / 3) - 1;
        const auto secondPointOnPathIndex = (startPathPointIndex + originalPointsCount / 3 * 2) - 1;

        const auto firstPointOnPath = is2D ? pathOnScreen[firstPointOnPathIndex] : pathInWorld[firstPointOnPathIndex];
        const auto secondPointOnPath = is2D ? pathOnScreen[secondPointOnPathIndex] : pathInWorld[secondPointOnPathIndex];

        const auto pathInitialVector = secondPointOnPath - firstPointOnPath;
        const auto pathInitialLength = glm::distance(firstPointOnPath, secondPointOnPath);
        if (pathInitialLength == 0.0f)
            return SkPath();
        
        const auto pathDirection = pathInitialVector / pathInitialLength;

        const auto pathCenter = firstPointOnPath + pathInitialVector / 2.0f;

        const auto totalGlyphsWidth = std::accumulate(glyphsWidths.begin(), glyphsWidths.end(), 0.0f) * projectionScale;
        // Make sure new path will accomodate all glyphs
        const auto pathScale = totalGlyphsWidth / pathInitialLength;

        // Compute start/end of new path
        const auto pathStart = pathCenter - (pathInitialVector / 2.0f) * pathScale;
        const auto pathEnd = pathCenter + (pathInitialVector / 2.0f) * pathScale;

        SkPath straightPath;
        straightPath.moveTo(pathStart.x, pathStart.y);
        straightPath.lineTo(pathEnd.x, pathEnd.y);

        return straightPath;
    }

    const glm::vec2 directionOnScreenN(-directionOnScreen.y, directionOnScreen.x);
    const auto shouldInvert = directionOnScreenN.y < 0; // For text readability

    auto pGlyphWidth = glyphsWidths.constData();
    if (shouldInvert)
    {
        // In case of direction inversion, start from last glyph
        pGlyphWidth += glyphsCount - 1;
    }
    const auto glyphWidthIncrement = shouldInvert ? -1 : +1;

    // Compute start with offset on original path
    const auto startNoOffset = is2D ? pathOnScreen[startPathPointIndex] : pathInWorld[startPathPointIndex];
    const auto firstSegmentEnd = is2D ? pathOnScreen[startPathPointIndex + 1] : pathInWorld[startPathPointIndex + 1];
    const auto firstSegmentVector = firstSegmentEnd - startNoOffset;
    const auto firstSegmentLength = is2D
        ? pathSegmentsLengthsOnScreen[startPathPointIndex]
        : pathSegmentsLengthsInWorld[startPathPointIndex];
    const auto firstSegmentDirection = firstSegmentVector / firstSegmentLength;
    const auto start = startNoOffset + firstSegmentDirection * offsetFromStartPathPoint;

    // Fill original path
    SkPath originalPath;
    originalPath.moveTo(start.x, start.y);
    for (auto i = startPathPointIndex + 1; i <= endPathPointIndex + 1; i++)
    {
        const auto point = is2D ? pathOnScreen[i] : pathInWorld[i];
        originalPath.lineTo(point.x, point.y);
    }
    SkPathMeasure originalPathMeasure(originalPath, false);

    // Compute start of each glyph on straight line
    QVector<float> glyphsStartOnStraight;
    auto glyphsTotalWidth = 0.0f;
    for (int glyphIdx = 0; glyphIdx < glyphsCount; glyphIdx++)
    {
        glyphsStartOnStraight.push_back(glyphsTotalWidth);
        const auto glyphWidth = *(pGlyphWidth + glyphIdx * glyphWidthIncrement);
        const auto scaledGlyphWidth = glyphWidth * projectionScale;
        glyphsTotalWidth += scaledGlyphWidth;
    }

    // Add start point to new path
    SkPath pathToDrawOn;
    pathToDrawOn.moveTo(start.x, start.y);

    // This will build such path, that in each triplet of glyphs 1st and 2nd glyph will
    // be drawn on one segment, and 3rd glyph will be drawn on separate segment
    for (auto i = 0; i < glyphsCount; i += 3)
    {
        // Find start of 1st and end of 2nd (or 1st, if there are no more glyphs) glyph in triplet
        const auto start = glyphsStartOnStraight[i];
        const auto endIndex = qMin(i + 1, glyphsCount - 1);
        const auto endGlyphWidth = *(pGlyphWidth + endIndex * glyphWidthIncrement);
        const auto endGlyphScaledWidth = endGlyphWidth * projectionScale;
        const auto end = glyphsStartOnStraight[endIndex] + endGlyphScaledWidth;
        
        // Find such segment on original path, so that 1st and 2nd glyphs of triplet will fit
        SkPath glyphsGroupPath;
        originalPathMeasure.getSegment(start, end, &glyphsGroupPath, true);
        const auto pointsCount = glyphsGroupPath.countPoints();

        if (pointsCount < 2)
            return SkPath();

        // Add founded segment to new path 
        pathToDrawOn.lineTo(glyphsGroupPath.getPoint(0));
        pathToDrawOn.lineTo(glyphsGroupPath.getPoint(pointsCount - 1));
    }

    // Add last point to new path
    const auto lastPoint = is2D ? pathOnScreen[endPathPointIndex + 1] : pathInWorld[endPathPointIndex + 1]; 
    pathToDrawOn.lineTo(lastPoint.x, lastPoint.y);

    // Check that glyphs won't overlap each other too much
    SkPathMeasure pathToDrawOnMeasure(pathToDrawOn, false);
    auto textUnreadable = pathToDrawOnMeasure.getLength() < glyphsTotalWidth;
    if (textUnreadable)
        return SkPath();

    return pathToDrawOn;
}

glm::vec2 OsmAnd::AtlasMapRendererSymbolsStage::computePathDirection(const SkPath& path) const
{
    const auto skPathStart = path.getPoint(0);
    const auto skPathEnd = path.getPoint(path.countPoints() - 1);

    const glm::vec2 pathStart(skPathStart.x(), skPathStart.y());
    const glm::vec2 pathEnd(skPathEnd.x(), skPathEnd.y());

    return glm::normalize(pathEnd - pathStart);
}

double OsmAnd::AtlasMapRendererSymbolsStage::computeDistanceFromCameraToPath(const SkPath& pathInWorld) const
{
    const auto& internalState = getInternalState();

    auto distancesSum = 0.0;
    for (auto i = 0; i < pathInWorld.countPoints(); i++)
    {
        const auto skPoint = pathInWorld.getPoint(i);
        const auto distance = glm::distance(
            internalState.worldCameraPosition,
            glm::vec3(skPoint.x(), 0.0f, skPoint.y()));
        distancesSum += distance;
    }

    return distancesSum / pathInWorld.countPoints();
}

SkPath OsmAnd::AtlasMapRendererSymbolsStage::convertPathOnScreenToWorld(const SkPath& pathOnScreen, bool& outOk) const
{
    const auto& internalState = getInternalState();
    SkPath pathInWorld;

    for (auto i = 0; i < pathOnScreen.countPoints(); i++)
    {
        const auto skPoint = pathOnScreen.getPoint(i);
        const PointI point(skPoint.x(), currentState.windowSize.y - skPoint.y());
        PointF worldPoint;
        const auto ok = getRenderer()->getWorldPointFromScreenPoint(internalState, currentState, point, worldPoint);

        if (!ok)
        {
            outOk = false;
            return SkPath();
        }
        
        if (pathInWorld.countPoints() == 0)
            pathInWorld.moveTo(worldPoint.x, worldPoint.y);
        else
            pathInWorld.lineTo(worldPoint.x, worldPoint.y);
    }

    outOk = true;
    return pathInWorld;
}

SkPath OsmAnd::AtlasMapRendererSymbolsStage::projectPathInWorldToScreen(const SkPath& pathInWorld) const
{
    const auto& internalState = getInternalState();

    SkPath pathOnScreen;

    for (auto i = 0; i < pathInWorld.countPoints(); i++)
    {
        const auto skPoint = pathInWorld.getPoint(i);
        const auto pointOnScreen = glm_extensions::project(
            glm::vec3(skPoint.x(), 0.0f, skPoint.y()),
            internalState.mPerspectiveProjectionView,
            internalState.glmViewport).xy();

        if (pathOnScreen.countPoints() == 0)
            pathOnScreen.moveTo(pointOnScreen.x, pointOnScreen.y);
        else
            pathOnScreen.lineTo(pointOnScreen.x, pointOnScreen.y);
    }

    return pathOnScreen;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::computePlacementOfGlyphsOnPath(
    const SkPath& path,
    const bool is2D,
    const glm::vec2& directionInWorld,
    const glm::vec2& directionOnScreen,
    const QVector<float>& glyphsWidths,
    const float glyphHeight,
    QVector<RenderableOnPathSymbol::GlyphPlacement>& outGlyphsPlacement,
    QVector<glm::vec3>& outRotatedElevatedBBoxInWorld) const
{
    const auto& internalState = getInternalState();

    assert(path.countPoints() >= 2);

    const auto projectionScale = is2D ? 1.0f : internalState.pixelInWorldProjectionScale;
    const glm::vec2 directionOnScreenN(-directionOnScreen.y, directionOnScreen.x);
    const auto shouldInvert = directionOnScreenN.y < 0; // For readability

    // Initialize glyph input and output pointers
    const auto glyphsCount = glyphsWidths.size();
    outGlyphsPlacement.resize(glyphsCount);
    auto pGlyphPlacement = outGlyphsPlacement.data();
    if (shouldInvert)
    {
        // In case of direction inversion, fill from end
        pGlyphPlacement += glyphsCount - 1;
    }
    auto pGlyphWidth = glyphsWidths.constData();
    if (shouldInvert)
    {
        // In case of direction inversion, start from last glyph
        pGlyphWidth += glyphsCount - 1;
    }
    const auto glyphWidthIncrement = (shouldInvert ? -1 : +1);

    SkPathMeasure pathToDrawOnMeasure(path, false);

    auto glyphOffsetOnStraight = 0.0f;

    for (auto glyphIdx = 0; glyphIdx < glyphsCount; glyphIdx++)
    {
        const auto glyphWidth = *(pGlyphWidth + glyphIdx * glyphWidthIncrement);
        const auto glyphWidthScaled = glyphWidth * projectionScale;

        const auto glyphStartOnStraight = glyphOffsetOnStraight;
        const auto glyphEndOnStraight = glyphStartOnStraight + glyphWidthScaled;

        // Find such segment on path, so that current glyph will fit 
        SkPath glyphPath;
        pathToDrawOnMeasure.getSegment(glyphStartOnStraight, glyphEndOnStraight, &glyphPath, true);

        const auto glyphPathPointsCount = glyphPath.countPoints();
        if (glyphPathPointsCount < 2)
            return false;

        const auto skGlyphStart = glyphPath.getPoint(0);
        const auto skGlyphEnd = glyphPath.getPoint(glyphPathPointsCount - 1);
        
        const glm::vec2 glyphStart(skGlyphStart.x(), skGlyphStart.y());
        const glm::vec2 glyphEnd(skGlyphEnd.x(), skGlyphEnd.y());

        const auto glyphVector = glyphEnd - glyphStart;
        const auto glyphDirection = glyphVector / glyphWidthScaled;

        glm::vec2 glyphNormal;
        if (is2D)
        {
            // CCW 90 degrees rotation of Y is up
            glyphNormal.x = -glyphDirection.y;
            glyphNormal.y = glyphDirection.x;
        }
        else
        {
            // CCW 90 degrees rotation of Y is down
            glyphNormal.x = glyphDirection.y;
            glyphNormal.y = -glyphDirection.x;
        }

        //TODO: maybe for 3D a -y should be passed (see -1 rotation axis)
        float glyphAngle = qAtan2(glyphDirection.y, glyphDirection.x);
        if (shouldInvert)
            glyphAngle = Utilities::normalizedAngleRadians(glyphAngle + M_PI);

        // Position glyph anchor point without elevation
        const auto anchorPointNoElevation = glyphStart + glyphVector / 2.0f;
        
        glm::vec2 anchorPoint;
        float glyphDepth;

        if (is2D)
        {
            // If elevation data is provided, elevate glyph anchor point
            glm::vec3 elevatedAnchorPoint;
            glm::vec3 elevatedAnchorInWorld;
            auto ok = elevateGlyphAnchorPointIn2D(anchorPointNoElevation, elevatedAnchorPoint, elevatedAnchorInWorld);
            if (!ok)
                return false;
            anchorPoint = elevatedAnchorPoint.xy();
            glyphDepth = elevatedAnchorPoint.z;
            outRotatedElevatedBBoxInWorld.push_back(elevatedAnchorInWorld);
        }
        else
        {
            // Glyph anchor point will be elevated later
            anchorPoint = anchorPointNoElevation;
            glyphDepth = NAN;
        }


        (shouldInvert ? *(pGlyphPlacement--) : *(pGlyphPlacement++)) = RenderableOnPathSymbol::GlyphPlacement(
            anchorPoint,
            glyphWidth,
            glyphAngle,
            glyphDepth,
            glyphNormal);

        glyphOffsetOnStraight += glyphWidthScaled;
    }

    if (!is2D)
    {
        // Try to elevate all glyph anchor points
        bool ok = elevateGlyphAnchorPointsIn3D(outGlyphsPlacement, outRotatedElevatedBBoxInWorld, glyphHeight, directionInWorld);
        if (!ok)
            return false;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::elevateGlyphAnchorPointIn2D(
    const glm::vec2& anchorPoint,
    glm::vec3& outElevatedAnchorPoint,
    glm::vec3& outElevatedAnchorInWorld) const
{
    const auto& internalState = getInternalState();
    const auto zoom = currentState.zoomLevel;
    
    // Convert screen point to world
    const PointI anchorOnScreenNoElevation(anchorPoint.x, currentState.windowSize.y - anchorPoint.y);
    PointF anchorInWorldNoElevation;
    auto ok = getRenderer()->getWorldPointFromScreenPoint(
        internalState,
        currentState,
        anchorOnScreenNoElevation,
        anchorInWorldNoElevation);

    if (!ok)
        return false;

    // Find height of anchor point
    const auto anchorPointFloat = anchorInWorldNoElevation / AtlasMapRenderer::TileSize3D;
    const auto anchorPoint31 = Utilities::convertFloatTo31(anchorPointFloat, currentState.target31, zoom);
    const auto anchorHeightInWorld = getRenderer()->getHeightOfLocation(currentState, anchorPoint31);

    // Project point back to screen, but now with elevation
    outElevatedAnchorInWorld = glm::vec3(anchorInWorldNoElevation.x, anchorHeightInWorld, anchorInWorldNoElevation.y);
    outElevatedAnchorPoint = glm_extensions::project(
        outElevatedAnchorInWorld,
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport);

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::elevateGlyphAnchorPointsIn3D(
    QVector<RenderableOnPathSymbol::GlyphPlacement>& glyphsPlacement,
    QVector<glm::vec3>& outRotatedElevatedBBoxInWorld,
    const float glyphHeight,
    const glm::vec2& directionInWorld) const
{
    const auto& bboxInWorld = calculateOnPath3DRotatedBBox(glyphsPlacement, glyphHeight, directionInWorld);

    // Elevate bbox vertices and center
    glm::vec3 elevatedVertices[4];
    glm::vec3 elevatedBboxCenter(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; i++)
    {
        const auto vertex = bboxInWorld[i];
        const auto bboxVertex31 = Utilities::convertFloatTo31(
            vertex / AtlasMapRenderer::TileSize3D,
            currentState.target31,
            currentState.zoomLevel);
        const auto vertexElevation = getRenderer()->getHeightOfLocation(currentState, bboxVertex31);
        const glm::vec3 elevatedVertex(vertex.x, vertexElevation, vertex.y);

        elevatedVertices[i] = elevatedVertex;
        elevatedBboxCenter += elevatedVertex;
    }
    elevatedBboxCenter /= 4.0f;

    // Calculate normals to all 4 planes of elevated bbox and normal to approximate common bbox plane 
    glm::vec3 bboxPlanesN[4];
    glm::vec3 approximatedBBoxPlaneN(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; i++)
    {
        const auto vertex0 = elevatedVertices[(i + 0) % 4];
        const auto vertex1 = elevatedVertices[(i + 1) % 4];
        const auto vertex2 = elevatedVertices[(i + 2) % 4];
        const auto planeN = Utilities::calculatePlaneN(vertex0, vertex1, vertex2);
        bboxPlanesN[(i + 1) % 4] = planeN;
        approximatedBBoxPlaneN += planeN;
    }
    approximatedBBoxPlaneN /= 4.0f;
    const auto planeLength = glm::length(approximatedBBoxPlaneN);
    if (planeLength > 0.0f)
        approximatedBBoxPlaneN /= planeLength;
    else
        approximatedBBoxPlaneN = glm::vec3(1.0f, -1.0f, 0.0f);


    // Check if angle between planes not too big. The longer text the less angle allowed between planes.
    // Short texts are displayed unconditionally
    int glyphsCount = glyphsPlacement.size();
    if (glyphsCount > SHORT_GLYPHS_COUNT)
    {
        float maxAngle;
        if (glyphsCount <= MEDIUM_GLYPHS_COUNT)
            maxAngle = MEDIUM_GLYPHS_MAX_ANGLE;
        else if (glyphsCount <= LONG_GLYPHS_COUNT)
            maxAngle = LONG_GLYPHS_MAX_ANGLE;      
        else
            maxAngle = EXTRA_LONG_GLYPHS_MAX_ANGLE;

        for (int i = 0; i < 3; i++)
        {
            for (int j = i + 1; j < 4; j++)
            {
                const auto planeN1 = bboxPlanesN[i];
                const auto planeN2 = bboxPlanesN[j];
                const auto angleBetweenPlanes = qAcos(planeN1.x * planeN2.x + planeN1.y * planeN2.y);
                if (angleBetweenPlanes > qDegreesToRadians(maxAngle))
                    return false;
            }
        }
    }

    // Calculate rotation angle from approximated bbox plane
    const auto angle = static_cast<float>(qAcos(-approximatedBBoxPlaneN.y));
    const auto rotationN = glm::normalize(angle > 0.0f ?
        glm::vec3(approximatedBBoxPlaneN.z, 0.0f, -approximatedBBoxPlaneN.x) : glm::vec3(1.0f, 0.0f, 0.0f));
    const auto mRotate = glm::rotate(angle, rotationN);

    const auto mTranslateToStart = glm::translate(-glm::vec3(elevatedBboxCenter.x, 0.0f, elevatedBboxCenter.z));
    const auto mTranslateToElevatedCenter = glm::translate(elevatedBboxCenter);

    // Project glyphs onto plane of elevated bbox
    for (auto& glyph : glyphsPlacement)
    {
        const glm::vec4 originalAnchorPoint(glyph.anchorPoint.x, 0.0f, glyph.anchorPoint.y, 1.0f);
        const auto elevatedAnchorPoint = mTranslateToElevatedCenter * mRotate * mTranslateToStart * originalAnchorPoint;

        glyph.anchorPoint = elevatedAnchorPoint.xz();
        glyph.elevation = elevatedAnchorPoint.y;
        glyph.angleXZ = -angle;
        glyph.rotationX = rotationN.x;
        glyph.rotationZ = rotationN.z;
    }

    outRotatedElevatedBBoxInWorld.resize(4);
    for (int i = 0; i < 4; i++)
    {
        const auto vertex = bboxInWorld[i];
        const glm::vec4 originalVertex(vertex.x, 0.0f, vertex.y, 1.0f);
        const auto elevatedVertex = mTranslateToElevatedCenter * mRotate * mTranslateToStart * originalVertex;
        outRotatedElevatedBBoxInWorld[i] = elevatedVertex;
    }

    return true;
}

OsmAnd::OOBBF OsmAnd::AtlasMapRendererSymbolsStage::calculateOnPath2dOOBB(
    const std::shared_ptr<RenderableOnPathSymbol>& renderable) const
{
    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);

    const auto directionAngle = qAtan2(renderable->directionOnScreen.y, renderable->directionOnScreen.x);
    const auto negDirectionAngleCos = qCos(-directionAngle);
    const auto negDirectionAngleSin = qSin(-directionAngle);
    const auto directionAngleCos = qCos(directionAngle);
    const auto directionAngleSin = qSin(directionAngle);
    const auto halfGlyphHeight = symbol->size.y / 2.0f;
    auto bboxInitialized = false;
    AreaF bboxInDirection;
    for (const auto& glyph : constOf(renderable->glyphsPlacement))
    {
        const auto halfGlyphWidth = glyph.width / 2.0f;
        const glm::vec2 glyphPoints[4] =
        {
            glm::vec2(-halfGlyphWidth, -halfGlyphHeight), // TL
            glm::vec2( halfGlyphWidth, -halfGlyphHeight), // TR
            glm::vec2( halfGlyphWidth,  halfGlyphHeight), // BR
            glm::vec2(-halfGlyphWidth,  halfGlyphHeight)  // BL
        };

        const auto segmentAngleCos = qCos(glyph.angleY);
        const auto segmentAngleSin = qSin(glyph.angleY);

        for (int idx = 0; idx < 4; idx++)
        {
            const auto& glyphPoint = glyphPoints[idx];

            // Rotate to align with it's segment
            glm::vec2 pointOnScreen;
            pointOnScreen.x = glyphPoint.x*segmentAngleCos - glyphPoint.y*segmentAngleSin;
            pointOnScreen.y = glyphPoint.x*segmentAngleSin + glyphPoint.y*segmentAngleCos;

            // Add anchor point
            pointOnScreen += glyph.anchorPoint;

            // Rotate to align with direction
            PointF alignedPoint;
            alignedPoint.x = pointOnScreen.x*negDirectionAngleCos - pointOnScreen.y*negDirectionAngleSin;
            alignedPoint.y = pointOnScreen.x*negDirectionAngleSin + pointOnScreen.y*negDirectionAngleCos;
            if (Q_LIKELY(bboxInitialized))
                bboxInDirection.enlargeToInclude(alignedPoint);
            else
            {
                bboxInDirection.topLeft = bboxInDirection.bottomRight = alignedPoint;
                bboxInitialized = true;
            }
        }
    }
    const auto alignedCenter = bboxInDirection.center();
    bboxInDirection -= alignedCenter;
    PointF centerOnScreen;
    centerOnScreen.x = alignedCenter.x*directionAngleCos - alignedCenter.y*directionAngleSin;
    centerOnScreen.y = alignedCenter.x*directionAngleSin + alignedCenter.y*directionAngleCos;
    bboxInDirection = AreaF::fromCenterAndSize(
        centerOnScreen.x, currentState.windowSize.y - centerOnScreen.y,
        bboxInDirection.width(), bboxInDirection.height());

    return OOBBF(bboxInDirection, -directionAngle);
}

OsmAnd::OOBBF OsmAnd::AtlasMapRendererSymbolsStage::calculateOnPath3dOOBB(
    const std::shared_ptr<RenderableOnPathSymbol>& renderable) const
{
    const auto& internalState = getInternalState();
    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& rotatedBBoxInWorld = renderable->rotatedElevatedBBoxInWorld;
#if OSMAND_DEBUG && 0
        {
            getRenderer()->debugStage->addQuad3D(
                rotatedBBoxInWorld[0],
                rotatedBBoxInWorld[1],
                rotatedBBoxInWorld[2],
                rotatedBBoxInWorld[3],
                SkColorSetA(SK_ColorGREEN, 50));
        }
#endif // OSMAND_DEBUG

    // Project points of OOBB in world to screen
    const PointF projectedRotatedBBoxInWorldP0(static_cast<glm::vec2>(
        glm_extensions::project(rotatedBBoxInWorld[0],
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy()));
    const PointF projectedRotatedBBoxInWorldP1(static_cast<glm::vec2>(
        glm_extensions::project(rotatedBBoxInWorld[1],
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy()));
    const PointF projectedRotatedBBoxInWorldP2(static_cast<glm::vec2>(
        glm_extensions::project(rotatedBBoxInWorld[2],
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy()));
    const PointF projectedRotatedBBoxInWorldP3(static_cast<glm::vec2>(
        glm_extensions::project(rotatedBBoxInWorld[3],
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy()));
#if OSMAND_DEBUG && 0
    {
        QVector<glm::vec2> line;
        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP0.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP0.y));
        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP1.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP1.y));
        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP2.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP2.y));
        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP3.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP3.y));
        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP0.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP0.y));
        getRenderer()->debugStage->addLine2D(line, SkColorSetA(SK_ColorRED, 50));
    }
#endif // OSMAND_DEBUG

    // Rotate using direction on screen
    const auto directionAngle = qAtan2(renderable->directionOnScreen.y, renderable->directionOnScreen.x);
    const auto negDirectionAngleCos = qCos(-directionAngle);
    const auto negDirectionAngleSin = qSin(-directionAngle);
    PointF bboxOnScreenP0;
    bboxOnScreenP0.x = projectedRotatedBBoxInWorldP0.x*negDirectionAngleCos - projectedRotatedBBoxInWorldP0.y*negDirectionAngleSin;
    bboxOnScreenP0.y = projectedRotatedBBoxInWorldP0.x*negDirectionAngleSin + projectedRotatedBBoxInWorldP0.y*negDirectionAngleCos;
    PointF bboxOnScreenP1;
    bboxOnScreenP1.x = projectedRotatedBBoxInWorldP1.x*negDirectionAngleCos - projectedRotatedBBoxInWorldP1.y*negDirectionAngleSin;
    bboxOnScreenP1.y = projectedRotatedBBoxInWorldP1.x*negDirectionAngleSin + projectedRotatedBBoxInWorldP1.y*negDirectionAngleCos;
    PointF bboxOnScreenP2;
    bboxOnScreenP2.x = projectedRotatedBBoxInWorldP2.x*negDirectionAngleCos - projectedRotatedBBoxInWorldP2.y*negDirectionAngleSin;
    bboxOnScreenP2.y = projectedRotatedBBoxInWorldP2.x*negDirectionAngleSin + projectedRotatedBBoxInWorldP2.y*negDirectionAngleCos;
    PointF bboxOnScreenP3;
    bboxOnScreenP3.x = projectedRotatedBBoxInWorldP3.x*negDirectionAngleCos - projectedRotatedBBoxInWorldP3.y*negDirectionAngleSin;
    bboxOnScreenP3.y = projectedRotatedBBoxInWorldP3.x*negDirectionAngleSin + projectedRotatedBBoxInWorldP3.y*negDirectionAngleCos;

    // Build bbox from that and subtract center
    AreaF bboxInDirection;
    bboxInDirection.topLeft = bboxInDirection.bottomRight = bboxOnScreenP0;
    bboxInDirection.enlargeToInclude(bboxOnScreenP1);
    bboxInDirection.enlargeToInclude(bboxOnScreenP2);
    bboxInDirection.enlargeToInclude(bboxOnScreenP3);
    const auto alignedCenter = bboxInDirection.center();
    bboxInDirection -= alignedCenter;

    // Rotate center and add it
    const auto directionAngleCos = qCos(directionAngle);
    const auto directionAngleSin = qSin(directionAngle);
    PointF centerOnScreen;
    centerOnScreen.x = alignedCenter.x*directionAngleCos - alignedCenter.y*directionAngleSin;
    centerOnScreen.y = alignedCenter.x*directionAngleSin + alignedCenter.y*directionAngleCos;
    bboxInDirection = AreaF::fromCenterAndSize(
        centerOnScreen.x, currentState.windowSize.y - centerOnScreen.y,
        bboxInDirection.width(), bboxInDirection.height());

    return OOBBF(bboxInDirection, -directionAngle);
}

QVector<OsmAnd::PointF> OsmAnd::AtlasMapRendererSymbolsStage::calculateOnPath3DRotatedBBox(
    const QVector<RenderableOnPathSymbol::GlyphPlacement>& glyphsPlacement,
    const float glyphHeight,
    const glm::vec2& directionInWorld) const
{
    const auto& internalState = getInternalState();

    const auto directionAngleInWorld = qAtan2(directionInWorld.y, directionInWorld.x);
    const auto negDirectionAngleInWorldCos = qCos(-directionAngleInWorld);
    const auto negDirectionAngleInWorldSin = qSin(-directionAngleInWorld);
    const auto directionAngleInWorldCos = qCos(directionAngleInWorld);
    const auto directionAngleInWorldSin = qSin(directionAngleInWorld);
    const auto halfGlyphHeight = (glyphHeight / 2.0f) * internalState.pixelInWorldProjectionScale;
    auto bboxInWorldInitialized = false;
    AreaF bboxInWorldDirection;
    for (const auto& glyph : constOf(glyphsPlacement))
    {
        const auto halfGlyphWidth = (glyph.width / 2.0f) * internalState.pixelInWorldProjectionScale;
        const glm::vec2 glyphPoints[4] =
        {
            glm::vec2(-halfGlyphWidth, -halfGlyphHeight), // TL
            glm::vec2( halfGlyphWidth, -halfGlyphHeight), // TR
            glm::vec2( halfGlyphWidth,  halfGlyphHeight), // BR
            glm::vec2(-halfGlyphWidth,  halfGlyphHeight)  // BL
        };

        const auto segmentAngleCos = qCos(glyph.angleY);
        const auto segmentAngleSin = qSin(glyph.angleY);

        for (int idx = 0; idx < 4; idx++)
        {
            const auto& glyphPoint = glyphPoints[idx];

            // Rotate to align with its segment
            glm::vec2 pointInWorld;
            pointInWorld.x = glyphPoint.x*segmentAngleCos - glyphPoint.y*segmentAngleSin;
            pointInWorld.y = glyphPoint.x*segmentAngleSin + glyphPoint.y*segmentAngleCos;

            // Add anchor point
            pointInWorld += glyph.anchorPoint;

            // Rotate to align with direction
            PointF alignedPoint;
            alignedPoint.x = pointInWorld.x*negDirectionAngleInWorldCos - pointInWorld.y*negDirectionAngleInWorldSin;
            alignedPoint.y = pointInWorld.x*negDirectionAngleInWorldSin + pointInWorld.y*negDirectionAngleInWorldCos;
            if (Q_LIKELY(bboxInWorldInitialized))
                bboxInWorldDirection.enlargeToInclude(alignedPoint);
            else
            {
                bboxInWorldDirection.topLeft = bboxInWorldDirection.bottomRight = alignedPoint;
                bboxInWorldInitialized = true;
            }
        }
    }
    const auto alignedCenterInWorld = bboxInWorldDirection.center();
    bboxInWorldDirection -= alignedCenterInWorld;

    QVector<PointF> rotatedBBoxInWorld(4);

    const auto& tl = bboxInWorldDirection.topLeft;
    rotatedBBoxInWorld[0].x = tl.x*directionAngleInWorldCos - tl.y*directionAngleInWorldSin;
    rotatedBBoxInWorld[0].y = tl.x*directionAngleInWorldSin + tl.y*directionAngleInWorldCos;
    const auto& tr = bboxInWorldDirection.topRight();
    rotatedBBoxInWorld[1].x = tr.x*directionAngleInWorldCos - tr.y*directionAngleInWorldSin;
    rotatedBBoxInWorld[1].y = tr.x*directionAngleInWorldSin + tr.y*directionAngleInWorldCos;
    const auto& br = bboxInWorldDirection.bottomRight;
    rotatedBBoxInWorld[2].x = br.x*directionAngleInWorldCos - br.y*directionAngleInWorldSin;
    rotatedBBoxInWorld[2].y = br.x*directionAngleInWorldSin + br.y*directionAngleInWorldCos;
    const auto& bl = bboxInWorldDirection.bottomLeft();
    rotatedBBoxInWorld[3].x = bl.x*directionAngleInWorldCos - bl.y*directionAngleInWorldSin;
    rotatedBBoxInWorld[3].y = bl.x*directionAngleInWorldSin + bl.y*directionAngleInWorldCos;

    PointF centerInWorld;
    centerInWorld.x = alignedCenterInWorld.x*directionAngleInWorldCos - alignedCenterInWorld.y*directionAngleInWorldSin;
    centerInWorld.y = alignedCenterInWorld.x*directionAngleInWorldSin + alignedCenterInWorld.y*directionAngleInWorldCos;
    bboxInWorldDirection += centerInWorld;
    rotatedBBoxInWorld[0] += centerInWorld;
    rotatedBBoxInWorld[1] += centerInWorld;
    rotatedBBoxInWorld[2] += centerInWorld;
    rotatedBBoxInWorld[3] += centerInWorld;

#if OSMAND_DEBUG && 0
    {
        const auto& cc = bboxInWorldDirection.center();
        const auto& tl = bboxInWorldDirection.topLeft;
        const auto& tr = bboxInWorldDirection.topRight();
        const auto& br = bboxInWorldDirection.bottomRight;
        const auto& bl = bboxInWorldDirection.bottomLeft();

        const glm::vec3 pC(cc.x, 0.0f, cc.y);
        const glm::vec4 p0(tl.x, 0.0f, tl.y, 1.0f);
        const glm::vec4 p1(tr.x, 0.0f, tr.y, 1.0f);
        const glm::vec4 p2(br.x, 0.0f, br.y, 1.0f);
        const glm::vec4 p3(bl.x, 0.0f, bl.y, 1.0f);
        const auto toCenter = glm::translate(-pC);
        const auto rotate = glm::rotate(
            (float)Utilities::normalizedAngleRadians(directionAngleInWorld + M_PI),
            glm::vec3(0.0f, -1.0f, 0.0f));
        const auto fromCenter = glm::translate(pC);
        const auto M = fromCenter*rotate*toCenter;
        getRenderer()->debugStage->addQuad3D(
            (M*p0).xyz(),
            (M*p1).xyz(),
            (M*p2).xyz(),
            (M*p3).xyz(),
            SkColorSetA(SK_ColorGREEN, 50));
    }
#endif // OSMAND_DEBUG

    return rotatedBBoxInWorld;
}

float OsmAnd::AtlasMapRendererSymbolsStage::getSubsectionOpacityFactor(
    const std::shared_ptr<const MapSymbol>& mapSymbol) const
{
    const auto citSubsectionConfiguration =
        currentState.symbolSubsectionConfigurations.constFind(mapSymbol->subsection);
    if (citSubsectionConfiguration == currentState.symbolSubsectionConfigurations.cend())
        return 1.0f;
    else
    {
        const auto& subsectionConfiguration = *citSubsectionConfiguration;
        return subsectionConfiguration.opacityFactor;
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage::addPathDebugLine(const QVector<PointI>& path31, const ColorARGB color) const
{
    QVector< glm::vec3 > debugPoints;
    for (const auto& pointInWorld : convertPoints31ToWorld(path31))
    {
        debugPoints.push_back(qMove(glm::vec3(
            pointInWorld.x,
            0.0f,
            pointInWorld.y)));
    }
    getRenderer()->debugStage->addLine3D(debugPoints, color.argb);
}

void OsmAnd::AtlasMapRendererSymbolsStage::addIntersectionDebugBox(
    const std::shared_ptr<const RenderableSymbol>& renderable,
    const ColorARGB color,
    const bool drawBorder /* = true*/) const
{
    addIntersectionDebugBox(renderable->intersectionBBox, color, drawBorder);
}

void OsmAnd::AtlasMapRendererSymbolsStage::addIntersectionDebugBox(
    const ScreenQuadTree::BBox intersectionBBox,
    const ColorARGB color,
    const bool drawBorder /*= true*/) const
{
    if (intersectionBBox.type == ScreenQuadTree::BBoxType::AABB)
    {
        const auto& boundsInWindow = intersectionBBox.asAABB;

        getRenderer()->debugStage->addRect2D((AreaF)boundsInWindow, color.argb);
        if (drawBorder)
        {
            getRenderer()->debugStage->addLine2D({
                (glm::ivec2)boundsInWindow.topLeft,
                (glm::ivec2)boundsInWindow.topRight(),
                (glm::ivec2)boundsInWindow.bottomRight,
                (glm::ivec2)boundsInWindow.bottomLeft(),
                (glm::ivec2)boundsInWindow.topLeft
            }, color.withAlpha(255).argb);
        }
    }
    else /* if (intersectionBBox.type == IntersectionsQuadTree::BBoxType::OOBB) */
    {
        const auto& oobb = intersectionBBox.asOOBB;

        getRenderer()->debugStage->addRect2D((AreaF)oobb.unrotatedBBox(), color.argb, -oobb.rotation());
        if (drawBorder)
        {
            getRenderer()->debugStage->addLine2D({
                (PointF)oobb.pointInGlobalSpace0(),
                (PointF)oobb.pointInGlobalSpace1(),
                (PointF)oobb.pointInGlobalSpace2(),
                (PointF)oobb.pointInGlobalSpace3(),
                (PointF)oobb.pointInGlobalSpace0(),
            }, color.withAlpha(255).argb);
        }
    }
}

OsmAnd::AtlasMapRendererSymbolsStage::RenderableSymbol::~RenderableSymbol()
{
}

OsmAnd::AtlasMapRendererSymbolsStage::RenderableBillboardSymbol::~RenderableBillboardSymbol()
{
}

OsmAnd::AtlasMapRendererSymbolsStage::RenderableOnPathSymbol::~RenderableOnPathSymbol()
{
}

OsmAnd::AtlasMapRendererSymbolsStage::RenderableOnSurfaceSymbol::~RenderableOnSurfaceSymbol()
{
}
