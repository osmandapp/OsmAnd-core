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
#include "AtlasMapRendererSymbolsStageModel3D.h"
#include "MapSymbol.h"
#include "VectorMapSymbol.h"
#include "BillboardVectorMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "OnPathRasterMapSymbol.h"
#include "BillboardRasterMapSymbol.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "Model3DMapSymbol.h"
#include "MapSymbolsGroup.h"
#include "QKeyValueIterator.h"
#include "MapSymbolIntersectionClassesRegistry.h"
#include "Stopwatch.h"
#include "GlmExtensions.h"
#include "MapMarker.h"
#include "VectorLine.h"

# define GRID_ITER_LIMIT 3
# define GRID_ITER_PRECISION 100.0

// Set maximum incline angle for using onpath-2D symbols instead of 3D-ones (20 deg)
const float OsmAnd::AtlasMapRendererSymbolsStage::_inclineThresholdOnPath2D =
    qPow(qSin(qDegreesToRadians(20.0f)), 2.0f);

// Set maximum viewing angle when onpath-3D symbols can be displayed (60 deg)
const float OsmAnd::AtlasMapRendererSymbolsStage::_tiltThresholdOnPath3D = qCos(qDegreesToRadians(60.0f));

OsmAnd::AtlasMapRendererSymbolsStage::AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer_)
    : AtlasMapRendererStage(renderer_)
{
    _lastResumeSymbolsUpdateTime = std::chrono::high_resolution_clock::now();
}

OsmAnd::AtlasMapRendererSymbolsStage::~AtlasMapRendererSymbolsStage() = default;

bool OsmAnd::AtlasMapRendererSymbolsStage::release(bool gpuContextLost)
{
    {
        QWriteLocker scopedLocker(&_lastPreparedIntersectionsLock);

        _lastPreparedIntersections.clear();
    }

    {
        QWriteLocker scopedLocker(&_lastVisibleSymbolsLock);

        _lastVisibleSymbols.clear();
    }

    _lastAcceptedMapSymbolsByOrder.clear();
    denseSymbols.clear();
    renderableSymbols.clear();
    return true;
}

void OsmAnd::AtlasMapRendererSymbolsStage::prepare(AtlasMapRenderer_Metrics::Metric_renderFrame* const metric)
{
    Stopwatch stopwatch(metric != nullptr);

    float timeSinceLastUpdate = std::chrono::duration<float>(
        std::chrono::high_resolution_clock::now() - _lastResumeSymbolsUpdateTime).count() * 1000.0f;
    bool forceSymbolsUpdate = false;
    const auto symbolsUpdateInterval = static_cast<float>(renderer->getSymbolsUpdateInterval());
    if (symbolsUpdateInterval > 0.0f)
    {
        forceSymbolsUpdate = timeSinceLastUpdate > symbolsUpdateInterval;

        // Don't invalidate this possibly last frame if symbols are quite fresh
        if (timeSinceLastUpdate < symbolsUpdateInterval / 2.0f)
            renderer->setSymbolsUpdated();
    }
    else if (timeSinceLastUpdate < 1000.0f)
    {
        // Don't invalidate this possibly last frame if symbols aren't older than 1s
        renderer->setSymbolsUpdated();
    }

    ScreenQuadTree intersections;
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
    bool result = false;
    
    if (!publishedMapSymbolsByOrderLock.tryLockForRead())
    {
        preRender(denseSymbols, metric);
        return false;
    }

    // Obtain volumetric symbols to pre-render their depths
    ScreenQuadTree dummyIntersections;
    result = obtainRenderableSymbols(
            publishedMapSymbolsByOrder,
            true,
            denseSymbols,
            dummyIntersections,
            nullptr,
            metric);

    preRender(denseSymbols, metric);
    
    Stopwatch stopwatch(metric != nullptr);

    // Overscaled/underscaled symbol resources were removed
    const auto needUpdatedSymbols = renderer->needUpdatedSymbols();

    // In case symbols update was not suspended, process published symbols
    if (!renderer->isSymbolsUpdateSuspended() || forceUpdate || needUpdatedSymbols)
    {
        _lastAcceptedMapSymbolsByOrder.clear();
        result = obtainRenderableSymbols(
            publishedMapSymbolsByOrder,
            false,
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

    // Do not suspend VectorLine objects

    // Acquire fresh VectorLine objects from publishedMapSymbolsByOrder
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

            if (std::dynamic_pointer_cast<const VectorLine::SymbolsGroup>(mapSymbolsGroup))
            {
                acceptedMapSymbols[mapSymbolsGroup] = mapSymbolsFromGroup;
            }
        }
        if (!acceptedMapSymbols.empty())
            filteredPublishedMapSymbols[order] = acceptedMapSymbols;
    }

    // Filter out old VectorLine objects from _lastAcceptedMapSymbolsByOrder
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

            if (!std::dynamic_pointer_cast<const VectorLine::SymbolsGroup>(mapSymbolsGroup))
            {
                acceptedMapSymbols[mapSymbolsGroup] = mapSymbolsFromGroup;
            }
        }
        filteredLastAcceptedMapSymbols[order] = acceptedMapSymbols;
    }

    // Append new VectorLine objects to filteredLastAcceptedMapSymbols
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
    result = obtainRenderableSymbols(
        filteredLastAcceptedMapSymbols,
        false,
        outRenderableSymbols,
        outIntersections,
        nullptr,
        metric);

    publishedMapSymbolsByOrderLock.unlock();

    return result;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderableSymbols(
    const MapRenderer::PublishedMapSymbolsByOrder& mapSymbolsByOrder,
    const bool preRenderDenseSymbolsDepth,
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
    clearTerrainVisibilityFiltering();
    bool applyFiltering = pOutAcceptedMapSymbolsByOrder != nullptr;
    if (!applyFiltering || !currentState.elevationDataProvider)
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

                const bool skipNotDenseSymbolGroup = preRenderDenseSymbolsDepth
                    && !std::dynamic_pointer_cast<const VectorLine::SymbolsGroup>(mapSymbolsGroup)
                    && !std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(mapSymbolsGroup);

                if (skipNotDenseSymbolGroup)
                    continue;

                const auto freshlyPublishedGroup =
                    std::dynamic_pointer_cast<const VectorLine::SymbolsGroup>(mapSymbolsGroup);

                const bool canSkip = !applyFiltering && !freshlyPublishedGroup && !preRenderDenseSymbolsDepth;

                const auto groupWithoutFiltering =
                    std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(mapSymbolsGroup);

                const bool withFiltering = applyFiltering && !groupWithoutFiltering;

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

                QList< std::shared_ptr<const MapSymbol> > plottedMapSymbolsFromGroup;

                // Check if original was discarded
                if (!mapSymbolsGroup->additionalInstancesDiscardOriginal)
                {
                    // Process symbols from this group in order as they are stored in group
                    for (const auto& mapSymbol : constOf(mapSymbolsGroup->symbols))
                    {
                        if (mapSymbol->isHidden)
                            continue;

                        // If this map symbol is not published yet or is located at different order, skip
                        const auto citSymbolInfo = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citSymbolInfo == mapSymbolsFromGroup.cend())
                            continue;

                        if (canSkip)
                        {
                            const auto& plottedSymbolInstances = citSymbolInfo->plottedInstances;
                            if (!plottedSymbolInstances.contains(nullptr))
                                continue;
                        }

                        const auto& referencesOrigins = citSymbolInfo->referenceOrigins;

                        QList< std::shared_ptr<RenderableSymbol> > renderableSymbols;
                        obtainRenderablesFromSymbol(
                            mapSymbolsGroup,
                            mapSymbol,
                            nullptr,
                            referencesOrigins,
                            computedPathsDataCache,
                            renderableSymbols,
                            outIntersections,
                            applyFiltering || preRenderDenseSymbolsDepth,
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

                                    auto& plottedSymbolInfo = (*pAcceptedMapSymbols)[mapSymbolsGroup][mapSymbol];
                                    plottedSymbolInfo.referenceOrigins = referencesOrigins;
                                    plottedSymbolInfo.plottedInstances.push_back(nullptr);
                                }
                                plottedMapSymbolsFromGroup.push_back(mapSymbol);
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
                        const auto citSymbolInfo = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citSymbolInfo == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = citSymbolInfo->referenceOrigins;

                        // If symbol is not references in additional group reference, also skip
                        const auto citAdditionalSymbolInstance = additionalGroupInstance->symbols.constFind(mapSymbol);
                        if (citAdditionalSymbolInstance == additionalGroupInstance->symbols.cend())
                            continue;
                        const auto& additionalSymbolInstance = *citAdditionalSymbolInstance;

                        if (canSkip)
                        {
                            const auto& plottedSymbolInstances = citSymbolInfo->plottedInstances;
                            if (!plottedSymbolInstances.contains(additionalSymbolInstance))
                                continue;
                        }

                        if (additionalSymbolInstance->discardableByAnotherInstances && plottedMapSymbolsFromGroup.contains(mapSymbol))
                            continue;

                        QList< std::shared_ptr<RenderableSymbol> > renderableSymbols;
                        obtainRenderablesFromSymbol(
                            mapSymbolsGroup,
                            mapSymbol,
                            additionalSymbolInstance,
                            referencesOrigins,
                            computedPathsDataCache,
                            renderableSymbols,
                            outIntersections,
                            applyFiltering || preRenderDenseSymbolsDepth,
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

                                    auto& plottedSymbolInfo = (*pAcceptedMapSymbols)[mapSymbolsGroup][mapSymbol];
                                    plottedSymbolInfo.referenceOrigins = referencesOrigins;
                                    plottedSymbolInfo.plottedInstances.push_back(additionalSymbolInstance);
                                }
                                plottedMapSymbolsFromGroup.push_back(mapSymbol);
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

        typedef std::pair<const std::shared_ptr<MapSymbolsGroup::AdditionalInstance>*,
            std::shared_ptr<QList<std::shared_ptr<RenderableSymbol>>>> SymbolRenderables;
        QList<SymbolRenderables> renderables;

        // Collect renderable symbols for rendering
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
                        const auto citSymbolInfo = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citSymbolInfo == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = citSymbolInfo->referenceOrigins;

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
                            renderables.push_back(SymbolRenderables(nullptr, renderableSymbols));
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
                        const auto citSymbolInfo = mapSymbolsFromGroup.constFind(mapSymbol);
                        if (citSymbolInfo == mapSymbolsFromGroup.cend())
                            continue;
                        const auto& referencesOrigins = citSymbolInfo->referenceOrigins;

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
                            renderables.push_back(SymbolRenderables(&additionalGroupInstance, renderableSymbols));
                    }
                }
            }
        }

        // Process renderable symbols before rendering
        int order = 0;
        MapRenderer::PublishedMapSymbolsByGroup* pAcceptedMapSymbols = nullptr;
        auto itPlottedSymbolsInsertPosition = plottedSymbols.begin();
        bool firstOrder = true;
        int prevOrder;
        QList< std::shared_ptr<const MapSymbol> > plottedMapSymbolsFromGroup;
        for (const auto& symbolRenderable : constOf(renderables))
        {
            if (firstOrder)
            {
                firstOrder = false;
                prevOrder = symbolRenderable.second->front()->mapSymbol->order;
            }
            else if (symbolRenderable.second->front()->mapSymbol->order != prevOrder)
            {
                itPlottedSymbolsInsertPosition = plottedSymbols.begin();
                prevOrder = symbolRenderable.second->front()->mapSymbol->order;
            }
            if (!symbolRenderable.first)
            {
                bool atLeastOnePlotted = false;
                for (const auto& renderableSymbol : constOf(*symbolRenderable.second))
                {
                    if (!plotSymbol(renderableSymbol, outIntersections, applyFiltering, metric))
                        continue;

                    const auto mapSymbolsGroup = renderableSymbol->mapSymbolGroup;
                    const auto mapSymbol = renderableSymbol->mapSymbol;

                    if (!atLeastOnePlotted)
                    {
                        // In case renderable symbol was obtained and accepted map symbols were requested,
                        // add this symbol to accepted
                        if (pOutAcceptedMapSymbolsByOrder)
                        {
                            if (pAcceptedMapSymbols == nullptr || order != mapSymbol->order)
                            {
                                order = mapSymbol->order;
                                pAcceptedMapSymbols = &(*pOutAcceptedMapSymbolsByOrder)[order];
                            }

                            auto& plottedSymbolInfo = (*pAcceptedMapSymbols)[mapSymbolsGroup][mapSymbol];
                            plottedSymbolInfo.referenceOrigins = *renderableSymbol->referenceOrigins;
                            plottedSymbolInfo.plottedInstances.push_back(nullptr);
                        }
                        plottedMapSymbolsFromGroup.push_back(mapSymbol);
                        atLeastOnePlotted = true;
                    }

                    const auto itPlottedSymbol = plottedSymbols.insert(itPlottedSymbolsInsertPosition, renderableSymbol);
                    PlottedSymbolRef plottedSymbolRef = { itPlottedSymbol, renderableSymbol };

                    plottedSymbolsMapByGroupAndInstance[mapSymbolsGroup]
                        .instancesRefs[nullptr]
                        .symbolsRefs.push_back(qMove(plottedSymbolRef));
                }
            }
            else
            {
                bool atLeastOnePlotted = false;
                for (const auto& renderableSymbol : constOf(*symbolRenderable.second))
                {
                    const auto mapSymbolsGroup = renderableSymbol->mapSymbolGroup;
                    const auto mapSymbol = renderableSymbol->mapSymbol;

                    const auto& instanceParameters = renderableSymbol->genericInstanceParameters;
                    const bool discardableByAnotherInstances = instanceParameters && !instanceParameters->discardableByAnotherInstances;
                    if (discardableByAnotherInstances && plottedMapSymbolsFromGroup.contains(mapSymbol))
                        continue;

                    if (!plotSymbol(renderableSymbol, outIntersections, applyFiltering, metric))
                        continue;

                    if (!atLeastOnePlotted)
                    {
                        // In case renderable symbol was obtained and accepted map symbols were requested,
                        // add this symbol to accepted
                        if (pOutAcceptedMapSymbolsByOrder)
                        {
                            if (pAcceptedMapSymbols == nullptr || order != mapSymbol->order)
                            {
                                order = mapSymbol->order;
                                pAcceptedMapSymbols = &(*pOutAcceptedMapSymbolsByOrder)[order];
                            }

                            auto& plottedSymbolInfo = (*pAcceptedMapSymbols)[mapSymbolsGroup][mapSymbol];
                            plottedSymbolInfo.referenceOrigins = *renderableSymbol->referenceOrigins;
                            plottedSymbolInfo.plottedInstances.push_back(instanceParameters);
                        }
                        plottedMapSymbolsFromGroup.push_back(mapSymbol);
                        atLeastOnePlotted = true;
                    }

                    const auto itPlottedSymbol = plottedSymbols.insert(itPlottedSymbolsInsertPosition, renderableSymbol);
                    PlottedSymbolRef plottedSymbolRef = { itPlottedSymbol, renderableSymbol };

                    plottedSymbolsMapByGroupAndInstance[mapSymbolsGroup]
                        .instancesRefs[*symbolRenderable.first]
                        .symbolsRefs.push_back(qMove(plottedSymbolRef));
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

                    const auto presentationMode = mapSymbolsGroupInstance
                        ? mapSymbolsGroupInstance->presentationMode
                        : mapSymbolsGroup->presentationMode;

                    // Just skip all rules
                    if (presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAnything)
                        continue;

                    // Rule: show all symbols or no symbols
                    if (presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAllOrNothing)
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
                    if (presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowNoneIfIconIsNotShown)
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
                    if (presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAllCaptionsOrNoCaptions)
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
                    if (presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAnythingUntilFirstGap)
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
    else if (const auto model3DMapSymbol = std::dynamic_pointer_cast<const Model3DMapSymbol>(mapSymbol))
    {
        if (Q_UNLIKELY(debugSettings->excludeModel3DSymbolsFromProcessing))
            return;

        _model3DSubstage->obtainRenderables(
            mapSymbolGroup,
            model3DMapSymbol,
            referenceOrigins,
            outRenderableSymbols,
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
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
    else if (const auto& renderableModel3D = std::dynamic_pointer_cast<RenderableModel3DSymbol>(renderable))
    {
        plotted = _model3DSubstage->plotSymbol(renderableModel3D, intersections, applyFiltering, metric);
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

    auto position31 =
        (instanceParameters && instanceParameters->overridesPosition31)
        ? instanceParameters->position31
        : billboardMapSymbol->getPosition31();

    if (const auto& rasterMapSymbol = std::dynamic_pointer_cast<const BillboardRasterMapSymbol>(mapSymbol))
    {
        const auto positionType = rasterMapSymbol->getPositionType();
        if (positionType != BillboardRasterMapSymbol::PositionType::Coordinate31)
        {
            bool isPrimary = positionType == BillboardRasterMapSymbol::PositionType::PrimaryGridX
                || positionType == BillboardRasterMapSymbol::PositionType::PrimaryGridY;
            bool isAxisY = positionType == BillboardRasterMapSymbol::PositionType::PrimaryGridY
                || positionType == BillboardRasterMapSymbol::PositionType::SecondaryGridY;
            auto coordinate = rasterMapSymbol->getAdditionalPosition();
            PointD point(isAxisY ? 0.0 : coordinate, isAxisY ? coordinate : 0.0);
            auto pos31 = isPrimary ? currentState.gridConfiguration.getPrimaryGridLocation31(point)
                : currentState.gridConfiguration.getSecondaryGridLocation31(point);
            if (pos31.x < 0)
            {
                int64_t intFull = INT32_MAX;
                intFull++;
                const auto intHalf = intFull >> 1;
                AreaI64 area64(currentState.visibleBBoxShifted);
                area64 += PointI64(intHalf, intHalf);
                area64 += PointI64(intFull, intFull);
                PointI point1(
                    isAxisY ? currentState.target31.x : static_cast<int32_t>(area64.topLeft.x & INT32_MAX),
                    isAxisY ? static_cast<int32_t>(area64.topLeft.y & INT32_MAX) : currentState.target31.y);
                PointI point2(
                    isAxisY ? currentState.target31.x : static_cast<int32_t>(area64.bottomRight.x & INT32_MAX),
                    isAxisY ? static_cast<int32_t>(area64.bottomRight.y & INT32_MAX) : currentState.target31.y);
                auto refLon = isPrimary ? currentState.gridConfiguration.getPrimaryGridReference(currentState.target31)
                    : currentState.gridConfiguration.getSecondaryGridReference(currentState.target31);
                auto refLon1 = isPrimary ? currentState.gridConfiguration.getPrimaryGridReference(point1)
                    : currentState.gridConfiguration.getSecondaryGridReference(point1);
                auto refLon2 = isPrimary ? currentState.gridConfiguration.getPrimaryGridReference(point2)
                    : currentState.gridConfiguration.getSecondaryGridReference(point2);
                if (refLon != refLon1 && refLon != refLon2)
                    return;
                if (refLon != refLon1)
                    point1 = currentState.target31;
                if (refLon != refLon2)
                    point2 = currentState.target31;
                auto location1 = isPrimary ? currentState.gridConfiguration.getPrimaryGridLocation(point1, &refLon)
                    : currentState.gridConfiguration.getSecondaryGridLocation(point1, &refLon);
                auto location2 = isPrimary ? currentState.gridConfiguration.getPrimaryGridLocation(point2, &refLon)
                    : currentState.gridConfiguration.getSecondaryGridLocation(point2, &refLon);
                auto coord1 = isAxisY ? location1.y : location1.x;
                auto coord2 = isAxisY ? location2.y : location2.x;
                const bool inside = coordinate > std::min(coord1, coord2) && coordinate < std::max(coord1, coord2);
                if (!inside)
                    return;
                auto pos1 = isAxisY ? point1.y : point1.x;
                auto pos2 = isAxisY ? point2.y : point2.x;
                int iterCount = 0;
                auto pos =
                    getApproximate31(coordinate, coord1, coord2, pos1, pos2, isPrimary, isAxisY, &refLon, iterCount);
                if (isAxisY)
                    pos31.y = pos;
                else
                    pos31.x = pos;
            }
            position31.x = isAxisY ? currentState.target31.x : pos31.x;
            position31.y = isAxisY ? pos31.y : currentState.target31.y;
        }
    }

    const auto renderer = getRenderer();

    // Get target tile and offset
    PointF offsetInTileN;
    const auto tileId = Utilities::normalizeTileId(
        Utilities::getTileId(position31, currentState.zoomLevel, &offsetInTileN), currentState.zoomLevel);

    // Calculate location of symbol in world coordinates.
    auto offset31 = Utilities::shortestVector31(currentState.target31, position31);

    const auto offsetFromTarget =
        Utilities::convert31toFloat(offset31, currentState.zoomLevel) * AtlasMapRenderer::TileSize3D;
    auto positionInWorld = glm::vec3(offsetFromTarget.x, 0.0f, offsetFromTarget.y);

    // Get elevation scale factor (affects distance from the surface)
    float elevationFactor = 1.0f;
    if (const auto& rasterMapSymbol = std::dynamic_pointer_cast<const BillboardRasterMapSymbol>(mapSymbol))
        elevationFactor = rasterMapSymbol->getElevationScaleFactor();

    // Get elevation data
    const auto upperMetersPerUnit =
            Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y, AtlasMapRenderer::TileSize3D);
    const auto lowerMetersPerUnit =
            Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y + 1, AtlasMapRenderer::TileSize3D);
    const auto metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);
    float surfaceInMeters = 0.0f;
    float surfaceInWorld = 0.0f;
    std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
    PointF offsetInScaledTileN = offsetInTileN;
    if (getElevationData(tileId, currentState.zoomLevel, offsetInScaledTileN, &elevationData) != InvalidZoomLevel
            && elevationData && elevationData->getValue(offsetInScaledTileN, surfaceInMeters))
    {
        float scaleFactor =
            currentState.elevationConfiguration.dataScaleFactor * currentState.elevationConfiguration.zScaleFactor;
        surfaceInWorld = scaleFactor * surfaceInMeters / metersPerUnit;
    }
    float elevationInWorld = surfaceInWorld;
    float elevationInMeters = NAN;
    if (const auto& rasterMapSymbol = std::dynamic_pointer_cast<const BillboardRasterMapSymbol>(mapSymbol))
        elevationInMeters = rasterMapSymbol->getElevation();
    if (!qIsNaN(elevationInMeters))
        elevationInWorld += elevationFactor * (elevationInMeters - surfaceInMeters) / metersPerUnit;
    positionInWorld.y = elevationInWorld;

    // Test against visible frustum area (if allowed)
    const bool isntMarker = !std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(mapSymbolGroup);
    if (isntMarker && !debugSettings->disableSymbolsFastCheckByFrustum &&
        allowFastCheckByFrustum && mapSymbol->allowFastCheckByFrustum)
    {
        if (!renderer->isPointVisible(internalState, positionInWorld))
        {
            if (metric)
                metric->billboardSymbolsRejectedByFrustum++;
            return;
        }
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

    // Get GPU resource
    const auto gpuResource = captureGpuResource(referenceOrigins, mapSymbol);
    if (!gpuResource)
        return;

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

    if (isntMarker && allowFastCheckByFrustum && !applyOnScreenVisibilityFiltering(visibleBBox, intersections, metric))
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
        renderable->referenceOrigins = const_cast<MapRenderer::MapSymbolReferenceOrigins*>(&referenceOrigins);
        renderable->gpuResource = gpuResource;
        renderable->positionInWorld = positionInWorld;
        renderable->position31 = position31;
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
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

        if (!std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(renderable->mapSymbolGroup))
        {
            if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
                return false;

            if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
                return false;
        }
    }
    return addToIntersections(renderable, intersections, metric);
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardVectorSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
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
    const bool isntMarker = !std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(mapSymbolGroup);
    if (isntMarker && !debugSettings->disableSymbolsFastCheckByFrustum &&
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

    // Get elevation scale factor (affects distance from the surface)
    float elevationFactor = 1.0f;
    if (const auto& rasterMapSymbol = std::dynamic_pointer_cast<const OnSurfaceRasterMapSymbol>(mapSymbol))
        elevationFactor = rasterMapSymbol->getElevationScaleFactor();
    else if (const auto& vectorMapSymbol = std::dynamic_pointer_cast<const OnSurfaceVectorMapSymbol>(mapSymbol))
        elevationFactor = vectorMapSymbol->getElevationScaleFactor();

    // Get elevation data
    const auto upperMetersPerUnit =
            Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y, AtlasMapRenderer::TileSize3D);
    const auto lowerMetersPerUnit =
            Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y + 1, AtlasMapRenderer::TileSize3D);
    const auto metersPerUnit = glm::mix(upperMetersPerUnit, lowerMetersPerUnit, offsetInTileN.y);
    float surfaceInMeters = 0.0f;
    float surfaceInWorld = 0.0f;
    std::shared_ptr<const IMapElevationDataProvider::Data> elevationData;
    PointF offsetInScaledTileN = offsetInTileN;
    if (getElevationData(tileId, currentState.zoomLevel, offsetInScaledTileN, &elevationData) != InvalidZoomLevel
            && elevationData && elevationData->getValue(offsetInScaledTileN, surfaceInMeters))
    {
        float scaleFactor =
            currentState.elevationConfiguration.dataScaleFactor * currentState.elevationConfiguration.zScaleFactor;
        surfaceInWorld = scaleFactor * surfaceInMeters / metersPerUnit;
    }
    float elevationInWorld = surfaceInWorld;
    float elevationInMeters = NAN;
    if (const auto& rasterMapSymbol = std::dynamic_pointer_cast<const OnSurfaceRasterMapSymbol>(mapSymbol))
        elevationInMeters = rasterMapSymbol->getElevation();
    if (!qIsNaN(elevationInMeters))
        elevationInWorld += elevationFactor * (elevationInMeters - surfaceInMeters) / metersPerUnit;

    // Don't render fully transparent symbols
    const auto opacityFactor = getSubsectionOpacityFactor(mapSymbol);
    if (opacityFactor > 0.0f)
    {
        std::shared_ptr<RenderableOnSurfaceSymbol> renderable(new RenderableOnSurfaceSymbol());
        renderable->mapSymbolGroup = mapSymbolGroup;
        renderable->mapSymbol = mapSymbol;
        renderable->genericInstanceParameters = instanceParameters;
        renderable->instanceParameters = instanceParameters;
        renderable->referenceOrigins = const_cast<MapRenderer::MapSymbolReferenceOrigins*>(&referenceOrigins);
        renderable->gpuResource = gpuResource;
        renderable->elevationInMeters = elevationInMeters;
        renderable->elevationFactor = elevationFactor;
        renderable->tileId = tileId;
        renderable->offsetInTileN = offsetInTileN;
        renderable->opacityFactor = opacityFactor;
        renderable->target31 = currentState.target31;
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
        const auto positionInWorld = glm::vec3(
            renderable->offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
            elevationInWorld,
            renderable->offsetFromTarget.y * AtlasMapRenderer::TileSize3D);
        renderable->positionInWorld = positionInWorld;
        const auto point = glm_extensions::project(
            positionInWorld,
            internalState.mPerspectiveProjectionView,
            internalState.glmViewport).xy();
        if (allowFastCheckByFrustum)
        {
            renderable->queryIndex = startTerrainVisibilityFiltering(PointF(point.x, point.y),
                positionInWorld, positionInWorld, positionInWorld, positionInWorld);
        }
        else
            renderable->queryIndex = -1;

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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
{
    const bool result = !applyFiltering || renderable->queryIndex < 0 || applyTerrainVisibilityFiltering(renderable->queryIndex, metric);

    return result;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceVectorSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    ScreenQuadTree& intersections,
    bool applyFiltering /*= true*/,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
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

    const auto renderer = getRenderer();
    const auto pinPoint =
        Utilities::convert31toFloat(Utilities::shortestVector31(currentState.target31, pinPointOnPath.point31),
        currentState.zoomLevel) * static_cast<float>(AtlasMapRenderer::TileSize3D);
    const auto pinPointInWorld =
        glm::vec3(pinPoint.x, renderer->getHeightOfLocation(currentState, pinPointOnPath.point31), pinPoint.y);

    // Test against visible frustum (if allowed)
    const bool isntMarker = !std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(mapSymbolGroup);
    bool checkVisibility = isntMarker && !debugSettings->disableSymbolsFastCheckByFrustum && allowFastCheckByFrustum
        && onPathMapSymbol->allowFastCheckByFrustum;
    if (checkVisibility)
    {
        if (!renderer->isPointVisible(internalState, pinPointInWorld))
        {
            if (metric)
                metric->onPathSymbolsRejectedByFrustum++;
            return;
        }
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
        projectFromWorldToScreen(computedPathData, &path31);

        itComputedPathData = computedPathsDataCache.insert(onPathMapSymbol->shareablePath31, computedPathData);
    }
    const auto& computedPathData = *itComputedPathData;

    // Calculate pixel scale factor for 3D
    glm::vec2 pinPointOnScreen;
    float pathPixelSizeInWorld = getWorldPixelSize(pinPointInWorld, pinPointOnScreen);
    bool pixelSizeOk = pathPixelSizeInWorld > 0.0f;
    if (!pixelSizeOk)
        return;

    // Pin-point represents center of symbol
    const auto halfSizeInPixels = onPathMapSymbol->size.x / 2.0f;
    bool halfSizeOk = halfSizeInPixels > 0.0f;
    if (!halfSizeOk)
        return;

    bool fits = true;
    bool is2D = true;

    // Check if this symbol instance can be rendered in 2D mode
    glm::vec2 exactStartPointOnScreen(0.0f, 0.0f);
    glm::vec2 exactEndPointOnScreen(0.0f, 0.0f);
    unsigned int startPathPointIndex2D = 0;
    float offsetFromStartPathPoint2D = 0.0f;
    const auto lengthOnScreen = computedPathData.pathSegmentsLengthsOnScreen[pinPointOnPath.basePathPointIndex];
    fits = fits && !std::isnan(lengthOnScreen);
    const auto distanceToPinPointOnScreen =
        glm::distance(computedPathData.pathOnScreen[pinPointOnPath.basePathPointIndex], pinPointOnScreen);
    fits = fits && distanceToPinPointOnScreen >= 0.0f && distanceToPinPointOnScreen <= lengthOnScreen;
    fits = fits && computePointIndexAndOffsetFromOriginAndOffset(
        computedPathData.pathSegmentsLengthsOnScreen,
        pinPointOnPath.basePathPointIndex,
        distanceToPinPointOnScreen,
        -halfSizeInPixels,
        startPathPointIndex2D,
        offsetFromStartPathPoint2D);
    unsigned int endPathPointIndex2D = startPathPointIndex2D;
    float offsetFromEndPathPoint2D = offsetFromStartPathPoint2D;
    const auto extraSpaceToFitStaightenedPath =
        qMax(onPathMapSymbol->glyphsWidth.first(), onPathMapSymbol->glyphsWidth.last());
    fits = fits && computePointIndexAndOffsetFromOriginAndOffset(
        computedPathData.pathSegmentsLengthsOnScreen,
        pinPointOnPath.basePathPointIndex,
        distanceToPinPointOnScreen,
        halfSizeInPixels + extraSpaceToFitStaightenedPath,
        endPathPointIndex2D,
        offsetFromEndPathPoint2D);
    if (fits)
    {
        exactStartPointOnScreen = computeExactPointFromOriginAndOffset(
            computedPathData.pathOnScreen,
            computedPathData.vectorsOnScreen,
            computedPathData.pathSegmentsLengthsOnScreen,
            startPathPointIndex2D,
            offsetFromStartPathPoint2D);
        exactEndPointOnScreen = computeExactPointFromOriginAndOffset(
            computedPathData.pathOnScreen,
            computedPathData.vectorsOnScreen,
            computedPathData.pathSegmentsLengthsOnScreen,
            endPathPointIndex2D,
            offsetFromEndPathPoint2D);

        is2D = pathRenderableAs2D(
            computedPathData.pathOnScreen,
            computedPathData.vectorsOnScreen,
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
        const auto halfSizeInWorld = halfSizeInPixels * pathPixelSizeInWorld;

        const auto lengthInWorld = computedPathData.pathSegmentsLengthsInWorld[pinPointOnPath.basePathPointIndex];
        fits = !std::isnan(lengthInWorld);
        const auto distanceToPinPointInWorld =
            glm::distance(computedPathData.pathInWorld[pinPointOnPath.basePathPointIndex], pinPointInWorld.xz());
        fits = fits && distanceToPinPointInWorld >= 0.0f && distanceToPinPointInWorld <= lengthInWorld;
        fits = fits && computePointIndexAndOffsetFromOriginAndOffset(
            computedPathData.pathSegmentsLengthsInWorld,
            pinPointOnPath.basePathPointIndex,
            distanceToPinPointInWorld,
            -halfSizeInWorld,
            startPathPointIndex3D,
            offsetFromStartPathPoint3D);
        endPathPointIndex3D = startPathPointIndex3D;
        offsetFromEndPathPoint3D = offsetFromStartPathPoint3D;
        fits = fits && computePointIndexAndOffsetFromOriginAndOffset(
            computedPathData.pathSegmentsLengthsInWorld,
            pinPointOnPath.basePathPointIndex,
            distanceToPinPointInWorld,
            halfSizeInWorld + extraSpaceToFitStaightenedPath * pathPixelSizeInWorld,
            endPathPointIndex3D,
            offsetFromEndPathPoint3D);

        if (fits)
        {
            exactStartPointInWorld = computeExactPointFromOriginAndOffset(
                computedPathData.pathInWorld,
                computedPathData.vectorsInWorld,
                computedPathData.pathSegmentsLengthsInWorld,
                startPathPointIndex3D,
                offsetFromStartPathPoint3D);
            exactEndPointInWorld = computeExactPointFromOriginAndOffset(
                computedPathData.pathInWorld,
                computedPathData.vectorsInWorld,
                computedPathData.pathSegmentsLengthsInWorld,
                endPathPointIndex3D,
                offsetFromEndPathPoint3D);
        }
    }

    // If this symbol instance doesn't fit in both 2D and 3D, skip it
    if (!fits)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsMarksRejectedByViewpoint))
            getRenderer()->debugStage->addPoint2D(
                PointF(pinPointOnScreen.x, pinPointOnScreen.y), SkColorSetARGB(255, 128, 0, 0));
        return;
    }

    // Compute exact points
    if (!is2D)
    {
        // Get 2D exact points from 3D
        exactStartPointOnScreen = glm_extensions::project(
            glm::vec3(exactStartPointInWorld.x, 0.0f, exactStartPointInWorld.y),
            internalState.mPerspectiveProjectionView,
            internalState.glmViewport).xy();
        exactEndPointOnScreen = glm_extensions::project(
            glm::vec3(exactEndPointInWorld.x, 0.0f, exactEndPointInWorld.y),
            internalState.mPerspectiveProjectionView,
            internalState.glmViewport).xy();
    }

    // Compute direction of subpath on screen and in world
    const auto subpathStartIndex = is2D ? startPathPointIndex2D : startPathPointIndex3D;
    const auto subpathEndIndex = is2D ? endPathPointIndex2D : endPathPointIndex3D;
    assert(subpathEndIndex >= subpathStartIndex);

    const auto originalPathDirectionOnScreen = glm::normalize(exactEndPointOnScreen - exactStartPointOnScreen);
    // Compute path to place glyphs on
    QVector<float> pathOffsets;
    float symmetricOffset;
    auto path = computePathForGlyphsPlacement(
        pathPixelSizeInWorld,
        is2D,
        computedPathData,
        subpathStartIndex,
        is2D ? offsetFromStartPathPoint2D : offsetFromStartPathPoint3D,
        subpathEndIndex,
        originalPathDirectionOnScreen,
        onPathMapSymbol->glyphsWidth,
        pathOffsets,
        symmetricOffset);

    if (path.size() < 2)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsMarksRejectedByViewpoint))
            getRenderer()->debugStage->addPoint2D(
                PointF(pinPointOnScreen.x, pinPointOnScreen.y), SkColorSetARGB(255, 255, 0, 0));
        return;
    }

    ComputedPathData symbolPathData;

    //Calculate glyph path nodes only
    symbolPathData.pathInWorld = is2D
        ? convertPathOnScreenToWorld(
            internalState.sizeOfPixelInWorld,
            path,
            pathOffsets,
            computedPathData.pathDistancesOnScreen,
            computedPathData.pathAnglesOnScreen,
            computedPathData.pathInWorld,
            computedPathData.vectorsInWorld,
            computedPathData.pathSegmentsLengthsOnRelief,
            computedPathData.pathDistancesInWorld,
            computedPathData.pathAnglesInWorld)
        : getPathInWorldToWorld(
            path,
            pathOffsets,
            computedPathData.pathInWorld,
            computedPathData.vectorsInWorld,
            computedPathData.pathSegmentsLengthsInWorld);

    //Calculate glyph path
    projectFromWorldToScreen(symbolPathData);

    if (symmetricOffset != 0.0f)
    {
        //Calculate straigth path
        path.clear();
        path.push_back(0);
        path.push_back(0);
        pathOffsets.clear();
        pathOffsets.push_back(symmetricOffset);
        pathOffsets.push_back(
            (is2D ? symbolPathData.pathSegmentsLengthsOnScreen[0] : symbolPathData.pathSegmentsLengthsInWorld[0])
            - symmetricOffset);
        QVector<glm::vec2> tempPath;
        tempPath.push_back(symbolPathData.pathInWorld[0]);
        tempPath.push_back(symbolPathData.pathInWorld[1]);
        symbolPathData.pathInWorld = is2D
            ? convertPathOnScreenToWorld(
                internalState.sizeOfPixelInWorld,
                path,
                pathOffsets,
                symbolPathData.pathDistancesOnScreen,
                symbolPathData.pathAnglesOnScreen,
                tempPath,
                symbolPathData.vectorsInWorld,
                symbolPathData.pathSegmentsLengthsOnRelief,
                symbolPathData.pathDistancesInWorld,
                symbolPathData.pathAnglesInWorld)
            : getPathInWorldToWorld(
                path,
                pathOffsets,
                tempPath,
                symbolPathData.vectorsInWorld,
                symbolPathData.pathSegmentsLengthsInWorld);
        projectFromWorldToScreen(symbolPathData);
    }

    const auto directionInWorld = computePathDirection(symbolPathData.pathInWorld);
    const auto directionOnScreen = computePathDirection(symbolPathData.pathOnScreen);

    // Don't render fully transparent symbols
    const auto opacityFactor = getSubsectionOpacityFactor(onPathMapSymbol);
    if (opacityFactor > 0.0f)
    {
        QVector<RenderableOnPathSymbol::GlyphPlacement> glyphsPlacement;
        QVector<glm::vec3> rotatedElevatedBBoxInWorld;
        bool ok = computePlacementOfGlyphsOnPath(
            pathPixelSizeInWorld,
            symbolPathData,
            is2D,
            directionInWorld,
            directionOnScreen,
            onPathMapSymbol->glyphsWidth,
            onPathMapSymbol->size.y,
            checkVisibility,
            glyphsPlacement,
            rotatedElevatedBBoxInWorld);

        if (ok)
        {
            std::shared_ptr<RenderableOnPathSymbol> renderable(new RenderableOnPathSymbol());
            renderable->mapSymbolGroup = mapSymbolGroup;
            renderable->mapSymbol = onPathMapSymbol;
            renderable->genericInstanceParameters = instanceParameters;
            renderable->instanceParameters = instanceParameters;
            renderable->referenceOrigins = const_cast<MapRenderer::MapSymbolReferenceOrigins*>(&referenceOrigins);
            renderable->gpuResource = gpuResource;
            renderable->is2D = is2D;
            renderable->target31 = currentState.target31;
            renderable->pinPointOnScreen = pinPointOnScreen;
            renderable->pinPointInWorld = pinPointInWorld;
            renderable->distanceToCamera = computeDistanceFromCameraToPath(symbolPathData.pathInWorld);
            renderable->directionInWorld = directionInWorld;
            renderable->directionOnScreen = directionOnScreen;
            renderable->pixelSizeInWorld = pathPixelSizeInWorld;
            renderable->glyphsPlacement = glyphsPlacement;
            renderable->rotatedElevatedBBoxInWorld = rotatedElevatedBBoxInWorld;
            renderable->opacityFactor = opacityFactor;

            if (is2D)
            {
                // Calculate OOBB for 2D SOP
                const auto oobb = calculateOnPath2dOOBB(renderable);
                renderable->visibleBBox = renderable->intersectionBBox = (OOBBI)oobb;
            }
            else
            {
                // Calculate OOBB for 3D SOP in world
                const auto oobb = calculateOnPath3dOOBB(renderable);
                renderable->visibleBBox = renderable->intersectionBBox = (OOBBI)oobb;
            }

            if (isntMarker && allowFastCheckByFrustum)
            {
                // Make sure the symbol is at least two-thirds visible
                if (glyphsPlacement.size() > 1)
                {
                    glm::vec3 firstPoint, lastPoint;
                    if (is2D)
                    {
                        firstPoint = rotatedElevatedBBoxInWorld[0];
                        lastPoint = rotatedElevatedBBoxInWorld[rotatedElevatedBBoxInWorld.size() - 1];
                    }
                    else
                    {
                        auto placement = glyphsPlacement[0];
                        firstPoint = glm::vec3(placement.anchorPoint.x, placement.elevation, placement.anchorPoint.y);
                        placement = glyphsPlacement[glyphsPlacement.size() - 1];
                        lastPoint = glm::vec3(placement.anchorPoint.x, placement.elevation, placement.anchorPoint.y);
                    }
                    glm::vec2 pointsOnScreen[4];
                    pointsOnScreen[0] = glm_extensions::project(
                        firstPoint,
                        internalState.mPerspectiveProjectionView,
                        internalState.glmViewport).xy();
                    pointsOnScreen[3] = glm_extensions::project(
                        lastPoint,
                        internalState.mPerspectiveProjectionView,
                        internalState.glmViewport).xy();
                    const auto oneThird = (pointsOnScreen[3] - pointsOnScreen[0]) / 3.0f;
                    pointsOnScreen[1] = pointsOnScreen[0] + oneThird;
                    pointsOnScreen[2] = pointsOnScreen[1] + oneThird;
                    int countPointsOnScreen = 0;
                    for (int i = 0; i < 4; i++)
                    {
                        if (pointsOnScreen[i].x >= 0 && pointsOnScreen[i].x <= currentState.windowSize.x
                            && pointsOnScreen[i].y >= 0 && pointsOnScreen[i].y <= currentState.windowSize.y)
                            countPointsOnScreen++;
                    }
                    if (countPointsOnScreen < 3)
                        return;
                }

                if (!applyOnScreenVisibilityFiltering(renderable->visibleBBox, intersections, metric))
                    return;
            }

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

            if (allowFastCheckByFrustum)
                renderable->queryIndex = startTerrainVisibilityFiltering(PointF(-1.0f, -1.0f), p0, p1, p2, p3);
            else
                renderable->queryIndex = -1;

            outRenderableSymbols.push_back(renderable);
        }
        else
        {
            if (Q_UNLIKELY(debugSettings->showSymbolsMarksRejectedByViewpoint))
                getRenderer()->debugStage->addPoint2D(
                    PointF(pinPointOnScreen.x, pinPointOnScreen.y), SkColorSetARGB(255, 255, 128, 64));
            return;
        }
    }
    else
        return;

    if (Q_UNLIKELY(debugSettings->showOnPathSymbolsRenderablesPaths))
    {
        const glm::vec2 directionOnScreenN(-directionOnScreen.y, directionOnScreen.x);

        {
            QVector< glm::vec3 > debugPathPoints;
            for (auto i = 0; i < symbolPathData.pathInWorld.size(); i++)
            {
                const auto pointInWorld = symbolPathData.pathInWorld[i];
                const auto point31 = Utilities::convertFloatTo31(PointF(pointInWorld.x, pointInWorld.y) /
                    AtlasMapRenderer::TileSize3D, currentState.target31, currentState.zoomLevel);
                const glm::vec3 point(
                    pointInWorld.x,
                    getRenderer()->getHeightOfLocation(currentState, point31),
                    pointInWorld.y);
                debugPathPoints.push_back(point);
            }
            getRenderer()->debugStage->addLine3D(debugPathPoints, is2D ? SK_ColorGREEN : SK_ColorRED);
        }

        // Subpath N (start)
        {
            QVector<glm::vec2> lineN;
            const auto startPoint = symbolPathData.pathOnScreen[0];
            const glm::vec2 start(startPoint.x, startPoint.y);
            const glm::vec2 end = start + directionOnScreenN * 24.0f;
            lineN.push_back(glm::vec2(start.x, currentState.windowSize.y - start.y));
            lineN.push_back(glm::vec2(end.x, currentState.windowSize.y - end.y));
            getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorCYAN, 128));
        }

        // Subpath N (end)
        {
            QVector<glm::vec2> lineN;
            const auto startPoint = symbolPathData.pathOnScreen[symbolPathData.pathOnScreen.size() - 1];
            const glm::vec2 start(startPoint.x, startPoint.y);
            const glm::vec2 end = start + directionOnScreenN * 24.0f;
            lineN.push_back(glm::vec2(start.x, currentState.windowSize.y - start.y));
            lineN.push_back(glm::vec2(end.x, currentState.windowSize.y - end.y));
            getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
        }

        // Pin-point location
        {
            const auto pinPointInWorld = Utilities::convert31toFloat(Utilities::shortestVector31(
                currentState.target31, pinPointOnPath.point31),
                currentState.zoomLevel) * static_cast<float>(AtlasMapRenderer::TileSize3D);
            const auto pinPointOnScreen = glm_extensions::project(
                glm::vec3(
                    pinPointInWorld.x,
                    getRenderer()->getHeightOfLocation(currentState, pinPointOnPath.point31),
                    pinPointInWorld.y),
                internalState.mPerspectiveProjectionView,
                internalState.glmViewport).xy();

            {
                QVector<glm::vec2> lineN;
                const auto sn0 = pinPointOnScreen;
                lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
                const auto sn1 = sn0 + (directionOnScreenN*32.0f);
                lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
                getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorBLACK, 128));
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric /*= nullptr*/)
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Draw the glyphs
    if (renderable->is2D)
    {
        if (applyFiltering)
        {
            if (renderable->queryIndex > -1 && !applyTerrainVisibilityFiltering(renderable->queryIndex, metric))
                return false;

            //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
            //        oobb.enlargeBy(PointF(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

            if (!std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(renderable->mapSymbolGroup))
            {
                if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
                    return false;

                if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
                    return false;

                if (!addToIntersections(renderable, intersections, metric))
                    return false;
            }
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
            if (renderable->queryIndex > -1 && !applyTerrainVisibilityFiltering(renderable->queryIndex, metric))
                return false;

            //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
            //        oobb.enlargeBy(PointF(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

            if (!std::dynamic_pointer_cast<const MapMarker::SymbolsGroup>(renderable->mapSymbolGroup))
            {
                if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
                    return false;

                if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
                    return false;

                if (!addToIntersections(renderable, intersections, metric))
                    return false;
            }
        }

        if (Q_UNLIKELY(debugSettings->showOnPath3dSymbolGlyphDetails))
        {
            for (const auto& glyph : constOf(renderable->glyphsPlacement))
            {
                const auto elevation = glyph.elevation;
                const auto& glyphInMapPlane = AreaF::fromCenterAndSize(
                    glyph.anchorPoint.x, glyph.anchorPoint.y, /* anchor points are specified in world coordinates already */
                    glyph.width * renderable->pixelSizeInWorld, symbol->size.y * renderable->pixelSizeInWorld);
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
                const auto ln1 = glyph.anchorPoint + (glyph.vNormal * 16.0f * renderable->pixelSizeInWorld);
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
    const auto symbolGroupInstancePtr = renderable->genericInstanceParameters
        ? renderable->genericInstanceParameters->groupInstancePtr
        : nullptr;
    const auto intersectionProcessingMode = symbolGroupInstancePtr
        ? symbolGroupInstancePtr->intersectionProcessingMode
        : renderable->mapSymbolGroup->intersectionProcessingMode;
    const auto checkIntersectionsWithinGroup = intersectionProcessingMode.isSet(
        MapSymbolsGroup::IntersectionProcessingModeFlag::CheckIntersectionsWithinGroup);
    const auto& intersectionClassesRegistry = MapSymbolIntersectionClassesRegistry::globalInstance();
    const auto& symbolIntersectsWithClasses = symbol->intersectsWithClasses;
    const auto anyIntersectionClass = intersectionClassesRegistry.anyClass;
    const auto symbolIntersectsWithAnyClass = symbolIntersectsWithClasses.contains(intersectionClassesRegistry.anyClass);
    const auto symbolGroupPtr = symbol->groupPtr;
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

    auto symbolMinDistance = symbol->minDistance;
    if (const auto billboardInstanceParameters
            = std::dynamic_pointer_cast<const MapSymbolsGroup::AdditionalBillboardSymbolInstanceParameters>(renderable->genericInstanceParameters))
    {
        if (billboardInstanceParameters->overridesMinDistance)
            symbolMinDistance = billboardInstanceParameters->minDistance;
    }
    else if (const auto onPathInstanceParameters
            = std::dynamic_pointer_cast<const MapSymbolsGroup::AdditionalOnPathSymbolInstanceParameters>(renderable->genericInstanceParameters))
    {
        if (onPathInstanceParameters->overridesMinDistance)
            symbolMinDistance = onPathInstanceParameters->minDistance;
    }
    const auto& symbolContent = symbol->content;

    const auto hasSimilarContent = intersections.test(renderable->intersectionBBox.getEnlargedBy(symbolMinDistance), false,
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
            addIntersectionDebugBox(renderable, ColorARGB(50, 160, 0, 255));

        if (Q_UNLIKELY(debugSettings->showSymbolsCheckBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck))
            addIntersectionDebugBox(renderable->intersectionBBox.getEnlargedBy(symbol->minDistance), ColorARGB(50, 160, 0, 255), false);

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
            Utilities::convert31toFloat(Utilities::shortestVector31(currentState.target31, *(pPoint31++)),
            currentState.zoomLevel) * static_cast<float>(AtlasMapRenderer::TileSize3D);
    }

    return result;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::projectFromWorldToScreen(
    ComputedPathData& computedPathData,
    const QVector<PointI>* points31 /* = nullptr */) const
{
    const auto& internalState = getInternalState();

    const auto pointsCount = points31 ? points31->size() : computedPathData.pathInWorld.size();
    const auto segmentsCount = pointsCount - 1;

    if (points31)
        computedPathData.pathInWorld.resize(pointsCount);

    computedPathData.pathOnScreen.resize(pointsCount);

    computedPathData.vectorsInWorld.resize(segmentsCount);
    computedPathData.vectorsOnScreen.resize(segmentsCount);
    computedPathData.pathSegmentsLengthsOnRelief.resize(segmentsCount);
    computedPathData.pathSegmentsLengthsInWorld.resize(segmentsCount);
    computedPathData.pathSegmentsLengthsOnScreen.resize(segmentsCount);
    computedPathData.pathDistancesInWorld.resize(segmentsCount);
    computedPathData.pathDistancesOnScreen.resize(segmentsCount);
    computedPathData.pathAnglesInWorld.resize(segmentsCount);
    computedPathData.pathAnglesOnScreen.resize(segmentsCount);

    glm::vec3 pointInWorld, pointNearInWorld, prevPointInWorld, prevPointNearInWorld;
    glm::vec3 origPointInWorld, intersectionPoint;
    glm::vec2 pointOnScreen, prevPointOnScreen;
    float worldLength, screenLength, worldLengthOnPlane, screenLengthInPixels;
    float worldDistance, screenDistance, prevWorldDistance, prevScreenDistance;
    float worldAngle, screenAngle;
    PointI point31;
    PointF sourcePoint;
    unsigned int prevIdx;
    bool visible = true;
    bool prevVisible = true;
    const auto r = getRenderer();
    for (auto idx = 0u; idx < pointsCount; idx++)
    {
        prevIdx = idx - 1;
        if (points31)
        {
            point31 = (*points31)[idx];
            sourcePoint = Utilities::convert31toFloat(Utilities::shortestVector31(currentState.target31, point31),
                currentState.zoomLevel) * static_cast<float>(AtlasMapRenderer::TileSize3D);
        }
        else
        {
            sourcePoint.x = computedPathData.pathInWorld[idx].x;
            sourcePoint.y = computedPathData.pathInWorld[idx].y;
            point31 = Utilities::convertFloatTo31(sourcePoint / AtlasMapRenderer::TileSize3D,
                currentState.target31, currentState.zoomLevel);
        }
        pointInWorld = glm::vec3(sourcePoint.x, r->getHeightOfLocation(currentState, point31), sourcePoint.y);
        pointOnScreen = glm_extensions::project(
            pointInWorld,
            internalState.mPerspectiveProjectionView,
            internalState.glmViewport).xy();
        if (points31)
        {
            visible = r->isPointProjectable(internalState, pointInWorld);
            if (!visible && (idx == 0 || !prevVisible))
            {
                if (idx > 0)
                {
                    computedPathData.pathSegmentsLengthsInWorld[prevIdx] = NAN;
                    computedPathData.pathSegmentsLengthsOnScreen[prevIdx] = NAN;
                }
                origPointInWorld = pointInWorld;
                prevPointInWorld = origPointInWorld;
                prevVisible = false;
                continue;
            }
            if (idx > 0)
            {
                if (prevVisible && !visible)
                {
                    origPointInWorld = pointInWorld;
                    if (r->getLastProjectablePoint(internalState, prevPointInWorld, origPointInWorld, pointInWorld))
                    {
                        pointOnScreen = glm_extensions::project(
                            pointInWorld,
                            internalState.mPerspectiveProjectionView,
                            internalState.glmViewport).xy();
                    }
                    else
                        visible = true;
                }
                else if (!prevVisible && visible)
                {
                    origPointInWorld = pointInWorld;
                    if (r->getLastProjectablePoint(internalState, prevPointInWorld, origPointInWorld, pointInWorld))
                    {
                        pointOnScreen = glm_extensions::project(
                            pointInWorld,
                            internalState.mPerspectiveProjectionView,
                            internalState.glmViewport).xy();
                        worldDistance = glm::distance(internalState.worldCameraPosition, pointInWorld);
                        pointNearInWorld = glm::unProject(
                            glm::vec3(pointOnScreen.x, currentState.windowSize.y - pointOnScreen.y, 0.0f),
                            internalState.mCameraView,
                            internalState.mPerspectiveProjection,
                            internalState.glmViewport);
                        screenDistance = glm::distance(internalState.worldCameraPosition, pointNearInWorld);
                        computedPathData.pathInWorld[prevIdx] = pointInWorld.xz();
                        computedPathData.pathOnScreen[prevIdx] = pointOnScreen;
                        computedPathData.pathDistancesInWorld[prevIdx] = worldDistance;
                        computedPathData.pathDistancesOnScreen[prevIdx] = screenDistance;
                        prevPointInWorld = pointInWorld;
                        prevPointOnScreen = pointOnScreen;
                        prevPointNearInWorld = pointNearInWorld;
                        prevWorldDistance = worldDistance;
                        prevScreenDistance = screenDistance;
                    }
                    else
                    {
                        origPointInWorld = prevPointInWorld;
                        visible = false;
                    }
                }
            }
        }
        worldDistance = glm::distance(internalState.worldCameraPosition, pointInWorld);
        pointNearInWorld = glm::unProject(
            glm::vec3(pointOnScreen.x, currentState.windowSize.y - pointOnScreen.y, 0.0f),
            internalState.mCameraView,
            internalState.mPerspectiveProjection,
            internalState.glmViewport);
        screenDistance = glm::distance(internalState.worldCameraPosition, pointNearInWorld);
        if (idx < segmentsCount)
        {
            computedPathData.pathDistancesInWorld[idx] = worldDistance;
            computedPathData.pathDistancesOnScreen[idx] = screenDistance;
        }
        if (idx > 0)
        {
            computedPathData.vectorsInWorld[prevIdx] = pointInWorld.xz() - prevPointInWorld.xz();
            computedPathData.vectorsOnScreen[prevIdx] = pointOnScreen - prevPointOnScreen;
            worldLength = glm::distance(prevPointInWorld, pointInWorld);
            screenLength = glm::distance(prevPointNearInWorld, pointNearInWorld);
            worldLengthOnPlane = glm::distance(
                glm::vec2(prevPointInWorld.x, prevPointInWorld.z), glm::vec2(pointInWorld.x, pointInWorld.z));
            screenLengthInPixels = glm::distance(prevPointOnScreen, pointOnScreen);
            worldAngle = qAcos(qBound(-1.0f, (prevWorldDistance * prevWorldDistance + worldLength * worldLength
                - worldDistance * worldDistance) / (2.0f * prevWorldDistance * worldLength), 1.0f));
            screenAngle = qAcos(qBound(-1.0f, (prevScreenDistance * prevScreenDistance + screenLength * screenLength
                - screenDistance * screenDistance) / (2.0f * prevScreenDistance * screenLength), 1.0f));
            computedPathData.pathSegmentsLengthsOnRelief[prevIdx] = worldLength;
            computedPathData.pathSegmentsLengthsInWorld[prevIdx] = worldLengthOnPlane;
            computedPathData.pathSegmentsLengthsOnScreen[prevIdx] = screenLengthInPixels;
            computedPathData.pathAnglesInWorld[prevIdx] = worldAngle;
            computedPathData.pathAnglesOnScreen[prevIdx] = screenAngle;
        }
        if (points31)
            computedPathData.pathInWorld[idx] = pointInWorld.xz();
        prevPointInWorld = visible ? pointInWorld : origPointInWorld;
        computedPathData.pathOnScreen[idx] = pointOnScreen;
        prevPointOnScreen = pointOnScreen;
        prevPointNearInWorld = pointNearInWorld;
        prevWorldDistance = worldDistance;
        prevScreenDistance = screenDistance;
        prevVisible = visible;
    }

    return true;
}

float OsmAnd::AtlasMapRendererSymbolsStage::getWorldPixelSize(
    const glm::vec3& pinPointInWorld, glm::vec2& pinPointOnScreen) const
{
    const auto& internalState = getInternalState();

    const auto renderer = getRenderer();
    if (!renderer->isPointProjectable(internalState, pinPointInWorld))
        return 0.0f;
    const auto worldDistance = glm::distance(internalState.worldCameraPosition, pinPointInWorld);
    const auto pointOnScreen = glm_extensions::project(
        pinPointInWorld,
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy();
    pinPointOnScreen = pointOnScreen;
    const auto nearPointInWorld = glm::unProject(
        glm::vec3(pointOnScreen.x, currentState.windowSize.y - pointOnScreen.y, 0.0f),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport);
    const auto screenDistance = glm::distance(internalState.worldCameraPosition, nearPointInWorld);
    const auto pixelSize = worldDistance * internalState.sizeOfPixelInWorld / screenDistance;
    return pixelSize;
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

bool OsmAnd::AtlasMapRendererSymbolsStage::computePointIndexAndOffsetFromOriginAndOffset(
    const QVector<float>& pathSegmentsLengths,
    const unsigned int originPathPointIndex,
    const float distanceFromOriginPathPoint,
    const float offsetToPoint,
    unsigned int& outPathPointIndex,
    float& outOffsetFromPathPoint)
{
    const auto pathSegmentsCount = pathSegmentsLengths.size();
    const auto offsetFromOriginPathPoint = distanceFromOriginPathPoint + offsetToPoint;

    if (offsetFromOriginPathPoint >= 0.0f)
    {
        // In case start point is located after origin point ('on the right'), usual search is used

        auto testPathPointIndex = originPathPointIndex;
        auto scannedLength = 0.0f;
        while (scannedLength < offsetFromOriginPathPoint)
        {
            if (testPathPointIndex >= pathSegmentsCount)
                return false;
            const auto segmentLength = pathSegmentsLengths[testPathPointIndex];
            if (std::isnan(segmentLength))
                return false;
            if (scannedLength + segmentLength >= offsetFromOriginPathPoint)
            {
                outPathPointIndex = testPathPointIndex;
                outOffsetFromPathPoint =
                    qBound(0.0f, offsetFromOriginPathPoint - scannedLength, segmentLength);
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
            const auto segmentLength = pathSegmentsLengths[testPathPointIndex];
            if (std::isnan(segmentLength))
                return false;
            if (scannedLength - segmentLength <= offsetFromOriginPathPoint)
            {
                outPathPointIndex = testPathPointIndex;
                outOffsetFromPathPoint =
                    qBound(0.0f, segmentLength + offsetFromOriginPathPoint - scannedLength, segmentLength);
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
    const QVector<glm::vec2>& vectors,
    const QVector<float>& pathSegmentsLengths,
    const unsigned int originPathPointIndex,
    const float offsetFromOriginPathPoint)
{
    assert(offsetFromOriginPathPoint >= 0.0f);

    const auto& originPoint = path[originPathPointIndex];
    const auto& vector = vectors[originPathPointIndex];
    const auto& length = pathSegmentsLengths[originPathPointIndex];
    assert(offsetFromOriginPathPoint <= length);

    const auto exactPoint = originPoint + vector * (offsetFromOriginPathPoint / length);

    return exactPoint;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::pathRenderableAs2D(
    const QVector<glm::vec2>& pathOnScreen,
    const QVector<glm::vec2>& vectorsOnScreen,
    const unsigned int startPathPointIndex,
    const glm::vec2& exactStartPointOnScreen,
    const unsigned int endPathPointIndex,
    const glm::vec2& exactEndPointOnScreen)
{
    assert(endPathPointIndex >= startPathPointIndex);

    if (endPathPointIndex > startPathPointIndex)
    {
        const auto segmentsCount = endPathPointIndex - startPathPointIndex;

        auto pVector = vectorsOnScreen.constData() + startPathPointIndex;
        for (auto segmentIdx = 0; segmentIdx <= segmentsCount; segmentIdx++)
        {
            if (!segmentValidFor2D(*(pVector++)))
                return false;
        }

        return true;
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

    const auto d = vSegment.y;// horizont.x*vSegment.y - horizont.y*vSegment.x == 1.0f*vSegment.y - 0.0f*vSegment.x
    const auto inclineSinSq = d*d / (vSegment.x*vSegment.x + vSegment.y*vSegment.y);
    if (qAbs(inclineSinSq) > _inclineThresholdOnPath2D)
        return false;
    return true;
}

glm::vec2 OsmAnd::AtlasMapRendererSymbolsStage::computeCorrespondingPoint(
    const float sourcePointOffset,
    const float sourceDistance,
    const float sourceAngle,
    const glm::vec2& destinationStartPoint,
    const glm::vec2& destinationVector,
    const float destinationLength,
    const float destinationDistance,
    const float destinationAngle)
{
    const float startAngle =
        qAtan(qSin(sourceAngle) / (sourceDistance / sourcePointOffset - qCos(sourceAngle)));
    const float factor =
        (destinationDistance * qSin(startAngle) / qSin(startAngle + destinationAngle)) / destinationLength;
    const auto destinationPoint = destinationStartPoint + destinationVector * factor;
    return destinationPoint;
}

float OsmAnd::AtlasMapRendererSymbolsStage::findOffsetInSegmentForDistance(
    const float distance,
    const QVector<float>& pathSegmentsLengths,
    const unsigned int startPathPointIndex,
    const float offsetFromStartPathPoint,
    const unsigned int endPathPointIndex,
    unsigned int& segmentIndex)
{
    auto remained = distance + offsetFromStartPathPoint - pathSegmentsLengths[startPathPointIndex];
    if (remained < 0.0f)
    {
        segmentIndex = startPathPointIndex;
        return distance + offsetFromStartPathPoint;
    }
    for (auto index = startPathPointIndex + 1; index <= endPathPointIndex; index++)
    {
        const auto length = pathSegmentsLengths[index];
        remained -= length;
        if (remained < 0.0f)
        {
            segmentIndex = index;
            return remained + length;
        }
    }
    return -1.0f;
}

QVector<unsigned int> OsmAnd::AtlasMapRendererSymbolsStage::computePathForGlyphsPlacement(
    const float pathPixelSizeInWorld,
    const bool is2D,
    const ComputedPathData& computedPathData,
    const unsigned int startPathPointIndex,
    const float offsetFromStartPathPoint,
    const unsigned int endPathPointIndex,
    const glm::vec2& directionOnScreen,
    const QVector<float>& glyphsWidths,
    QVector<float>& pathOffsets,
    float& symmetricOffset) const
{
    const auto& internalState = getInternalState();

    assert(endPathPointIndex >= startPathPointIndex);
    const auto projectionScale = is2D ? 1.0f : pathPixelSizeInWorld;

    const auto glyphsCount = glyphsWidths.size();

    const auto atLeastTwoSegments = endPathPointIndex > startPathPointIndex;
    const auto forceDrawGlyphsOnStaright = glyphsCount <= 4 && atLeastTwoSegments;

    QVector<unsigned int> result;

    if (forceDrawGlyphsOnStaright)
    {
        const auto originalPathSegmentsCount = endPathPointIndex - startPathPointIndex + 1;
        const auto originalPointsCount = originalPathSegmentsCount + 1;

        // New path intersects points on 1/3 and 2/3 of original path
        const auto firstPointOnPathIndex = (startPathPointIndex + originalPointsCount / 3) - 1;
        const auto secondPointOnPathIndex = (startPathPointIndex + originalPointsCount / 3 * 2) - 1;

        const auto firstPointOnPath = is2D ? computedPathData.pathOnScreen[firstPointOnPathIndex]
            : computedPathData.pathInWorld[firstPointOnPathIndex];
        const auto secondPointOnPath = is2D ? computedPathData.pathOnScreen[secondPointOnPathIndex]
            : computedPathData.pathInWorld[secondPointOnPathIndex];

        const auto pathInitialVector = secondPointOnPath - firstPointOnPath;
        const auto pathInitialLength = glm::distance(firstPointOnPath, secondPointOnPath);
        if (pathInitialLength == 0.0f)
            return result;
        result.push_back(firstPointOnPathIndex);
        result.push_back(secondPointOnPathIndex);
        pathOffsets.push_back(0.0f);
        pathOffsets.push_back(0.0f);

        const auto totalGlyphsWidth = std::accumulate(glyphsWidths.begin(), glyphsWidths.end(), 0.0f) * projectionScale;
        // Make sure new path will accomodate all glyphs
        symmetricOffset = (pathInitialLength - totalGlyphsWidth * 1.1f) / 2.0f;
        return result;
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

    // Compute start of each glyph on straight line
    QVector<float> glyphsEndOnStraight;
    auto glyphsTotalWidth = 0.0f;
    for (int glyphIdx = 0; glyphIdx < glyphsCount; glyphIdx++)
    {
        const auto glyphWidth = *(pGlyphWidth + glyphIdx * glyphWidthIncrement);
        const auto scaledGlyphWidth = glyphWidth * projectionScale;
        glyphsTotalWidth += scaledGlyphWidth;
        glyphsEndOnStraight.push_back(glyphsTotalWidth);
    }

    // This will build such path, that in each triplet of glyphs 1st and 2nd glyph will
    // be drawn on one segment, and 3rd glyph will be drawn on separate segment
    auto totalLength = 0.0f;
    glm::vec2 startPoint, endPoint;
    unsigned int startPointIndex = 0;
    unsigned int endPointIndex = 0;
    float startPointDistance = 0.0f;
    float endPointDistance = 0.0f;
    for (auto i = 0; i < glyphsCount; i += 3)
    {
        // Find start of 1st and end of 2nd (or 1st, if there are no more glyphs) glyph in triplet
        const auto start = i > 0 ? glyphsEndOnStraight[i - 1] : 0.0f;
        const auto endIndex = qMin(i + 1, glyphsCount - 1);
        const auto end = glyphsEndOnStraight[endIndex];

        // Find such segment on original path, so that 1st and 2nd glyphs of triplet will fit
        startPointDistance = findOffsetInSegmentForDistance(
            start,
            is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
            startPathPointIndex,
            offsetFromStartPathPoint,
            endPathPointIndex,
            startPointIndex);
        if (startPointDistance < 0.0f)
        {
            result.clear();
            return result;
        }
        endPointDistance = findOffsetInSegmentForDistance(
            end,
            is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
            startPathPointIndex,
            offsetFromStartPathPoint,
            endPathPointIndex,
            endPointIndex);
        if (endPointDistance < 0.0f)
        {
            result.clear();
            return result;
        }
        result.push_back(startPointIndex);
        result.push_back(endPointIndex);
        pathOffsets.push_back(startPointDistance);
        pathOffsets.push_back(endPointDistance);
        startPoint = computeExactPointFromOriginAndOffset(
            is2D ? computedPathData.pathOnScreen : computedPathData.pathInWorld,
            is2D ? computedPathData.vectorsOnScreen : computedPathData.vectorsInWorld,
            is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
            startPointIndex,
            startPointDistance);
        if (i > 0)
            totalLength += glm::distance(endPoint, startPoint);
        endPoint = computeExactPointFromOriginAndOffset(
            is2D ? computedPathData.pathOnScreen : computedPathData.pathInWorld,
            is2D ? computedPathData.vectorsOnScreen : computedPathData.vectorsInWorld,
            is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
            endPointIndex,
            endPointDistance);
        totalLength += glm::distance(startPoint, endPoint);
    }

    symmetricOffset = 0.0f;

    // Leave room for a possible last glyph
    const auto lastWidth = qMax((glyphsTotalWidth - totalLength) * 1.1f, glyphsEndOnStraight[0] * 0.1f);
    unsigned int lastIndex;
    float lastDistance;
    lastDistance = findOffsetInSegmentForDistance(
        lastWidth,
        is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
        endPointIndex,
        endPointDistance,
        endPathPointIndex,
        lastIndex);
    if (lastDistance < 0.0f)
    {
        result.clear();
        return result;
    }
    result.push_back(lastIndex);
    pathOffsets.push_back(lastDistance);
    return result;
}

glm::vec2 OsmAnd::AtlasMapRendererSymbolsStage::computePathDirection(const QVector<glm::vec2>& path) const
{
    const auto pathStart = path[0];
    const auto pathEnd = path[path.size() - 1];

    return glm::normalize(pathEnd - pathStart);
}

double OsmAnd::AtlasMapRendererSymbolsStage::computeDistanceFromCameraToPath(
    const QVector<glm::vec2>& pathInWorld) const
{
    const auto& internalState = getInternalState();

    auto distancesSum = 0.0;
    const auto count = pathInWorld.size();
    for (auto i = 0; i < count; i++)
    {
        const auto point = pathInWorld[i];
        const auto distance = glm::distance(
            internalState.worldCameraPosition,
            glm::vec3(point.x, 0.0f, point.y));
        distancesSum += distance;
    }

    return distancesSum / count;
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::convertPathOnScreenToWorld(
    const float screenToWorldFlatFactor,
    const QVector<unsigned int>& pointIndices,
    const QVector<float>& pointOffsets,
    const QVector<float>& pathDistancesOnScreen,
    const QVector<float>& pathAnglesOnScreen,
    const QVector<glm::vec2>& pathInWorld,
    const QVector<glm::vec2>& vectorsInWorld,
    const QVector<float>& pathSegmentsLengthsInWorld,
    const QVector<float>& pathDistancesInWorld,
    const QVector<float>& pathAnglesInWorld) const
{
    const auto count = pointIndices.size();
    QVector<glm::vec2> result(count);
    auto pPointInWorld = result.data();

    const auto segmentsCount = pathSegmentsLengthsInWorld.size();
    for (auto idx = 0u; idx < count; idx++)
    {
        const auto pointIndex = pointIndices[idx];
        *(pPointInWorld++) = pointIndex < segmentsCount ? computeCorrespondingPoint(
            pointOffsets[idx] * screenToWorldFlatFactor,
            pathDistancesOnScreen[pointIndex],
            pathAnglesOnScreen[pointIndex],
            pathInWorld[pointIndex],
            vectorsInWorld[pointIndex],
            pathSegmentsLengthsInWorld[pointIndex],
            pathDistancesInWorld[pointIndex],
            pathAnglesInWorld[pointIndex]) : pathInWorld[pointIndex];
    }

    return result;
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::getPathInWorldToWorld(
    const QVector<unsigned int>& pointIndices,
    const QVector<float>& pointOffsets,
    const QVector<glm::vec2>& pathInWorld,
    const QVector<glm::vec2>& vectorsInWorld,
    const QVector<float>& pathSegmentsLengthsInWorld) const
{
    const auto count = pointIndices.size();
    QVector<glm::vec2> result(count);
    auto pPointInWorld = result.data();

    const auto segmentsCount = pathSegmentsLengthsInWorld.size();
    for (auto idx = 0u; idx < count; idx++)
    {
        const auto pointIndex = pointIndices[idx];
        const auto startPoint = pathInWorld[pointIndex];
        *(pPointInWorld++) = pointIndex < segmentsCount ? startPoint +
            vectorsInWorld[pointIndex] * pointOffsets[idx] / pathSegmentsLengthsInWorld[pointIndex]
            : startPoint;
    }

    return result;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::computePlacementOfGlyphsOnPath(
    const float pathPixelSizeInWorld,
    const ComputedPathData& computedPathData,
    const bool is2D,
    const glm::vec2& directionInWorld,
    const glm::vec2& directionOnScreen,
    const QVector<float>& glyphsWidths,
    const float glyphHeight,
    bool checkVisibility,
    QVector<RenderableOnPathSymbol::GlyphPlacement>& outGlyphsPlacement,
    QVector<glm::vec3>& outRotatedElevatedBBoxInWorld) const
{
    const auto& internalState = getInternalState();

    const auto screenToWorldFlatFactor = internalState.sizeOfPixelInWorld;

    const auto projectionScale = is2D ? 1.0f : pathPixelSizeInWorld;
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

    auto glyphOffsetOnStraight = 0.0f;

    for (auto glyphIdx = 0; glyphIdx < glyphsCount; glyphIdx++)
    {
        const auto glyphWidth = *(pGlyphWidth + glyphIdx * glyphWidthIncrement);
        const auto glyphWidthScaled = glyphWidth * projectionScale;

        const auto glyphStartOnStraight = glyphOffsetOnStraight;
        const auto glyphEndOnStraight = glyphStartOnStraight + glyphWidthScaled;

        // Find such segment on path, so that current glyph will fit
        unsigned int startPointIndex, endPointIndex;
        const auto lastPathIndex = (is2D ? computedPathData.pathSegmentsLengthsOnScreen.size()
            : computedPathData.pathSegmentsLengthsInWorld.size()) - 1;
        const auto startPointDistance = findOffsetInSegmentForDistance(
            glyphStartOnStraight,
            is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
            0,
            0.0f,
            lastPathIndex,
            startPointIndex);
        if (startPointDistance < 0.0f)
            return false;
        const auto endPointDistance = findOffsetInSegmentForDistance(
            glyphEndOnStraight,
            is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
            0,
            0.0f,
            lastPathIndex,
            endPointIndex);
        if (endPointDistance < 0.0f)
            return false;
        const auto glyphStart = computeExactPointFromOriginAndOffset(
            is2D ? computedPathData.pathOnScreen : computedPathData.pathInWorld,
            is2D ? computedPathData.vectorsOnScreen : computedPathData.vectorsInWorld,
            is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
            startPointIndex,
            startPointDistance);
        const auto glyphEnd = computeExactPointFromOriginAndOffset(
            is2D ? computedPathData.pathOnScreen : computedPathData.pathInWorld,
            is2D ? computedPathData.vectorsOnScreen : computedPathData.vectorsInWorld,
            is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
            endPointIndex,
            endPointDistance);

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

        glm::vec2 anchorPoint;
        float glyphDepth;

        if (is2D)
        {
            // If elevation data is provided, elevate glyph anchor point
            glm::vec3 elevatedAnchorPoint;
            glm::vec3 elevatedAnchorInWorld;

            const auto anchorPointNoElevation = computeCorrespondingPoint(
                (startPointDistance + glyphWidthScaled * 0.5f) * screenToWorldFlatFactor,
                computedPathData.pathDistancesOnScreen[startPointIndex],
                computedPathData.pathAnglesOnScreen[startPointIndex],
                computedPathData.pathInWorld[startPointIndex],
                computedPathData.vectorsInWorld[startPointIndex],
                computedPathData.pathSegmentsLengthsOnRelief[startPointIndex],
                computedPathData.pathDistancesInWorld[startPointIndex],
                computedPathData.pathAnglesInWorld[startPointIndex]);

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
            anchorPoint = glyphStart + glyphVector / 2.0f;
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
        bool ok = elevateGlyphAnchorPointsIn3D(outGlyphsPlacement, outRotatedElevatedBBoxInWorld,
            glyphHeight, pathPixelSizeInWorld, directionInWorld, checkVisibility);
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

    // Find height of anchor point
    const PointF anchorInWorldNoElevation(anchorPoint.x, anchorPoint.y);
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
    const float pathPixelSizeInWorld,
    const glm::vec2& directionInWorld,
    bool checkVisibility) const
{
    const auto& bboxInWorld = calculateOnPath3DRotatedBBox(
        glyphsPlacement, glyphHeight, pathPixelSizeInWorld, directionInWorld);

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
    {
        approximatedBBoxPlaneN /= planeLength;
        if (checkVisibility)
        {
            const auto& internalState = getInternalState();
            const auto viewRayN = glm::normalize(elevatedBboxCenter - internalState.worldCameraPosition);
            const auto viewAngleCos = static_cast<float>(glm::dot(viewRayN, approximatedBBoxPlaneN));
            if (viewAngleCos < _tiltThresholdOnPath3D)
                return false;
        }
    }
    else
        approximatedBBoxPlaneN = glm::vec3(0.0f, -1.0f, -1.0f);

    // Calculate rotation angle from approximated bbox plane
    const auto angle = static_cast<float>(qAcos(-approximatedBBoxPlaneN.y));
    const auto rotationN = angle > 0.0f ? glm::normalize(
        glm::vec3(-approximatedBBoxPlaneN.z, 0.0f, approximatedBBoxPlaneN.x)) : glm::vec3(-1.0f, 0.0f, 0.0f);
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
        glyph.angleXZ = angle;
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
    const float pathPixelSizeInWorld,
    const glm::vec2& directionInWorld) const
{
    const auto& internalState = getInternalState();

    const auto directionAngleInWorld = qAtan2(directionInWorld.y, directionInWorld.x);
    const auto negDirectionAngleInWorldCos = qCos(-directionAngleInWorld);
    const auto negDirectionAngleInWorldSin = qSin(-directionAngleInWorld);
    const auto directionAngleInWorldCos = qCos(directionAngleInWorld);
    const auto directionAngleInWorldSin = qSin(directionAngleInWorld);
    const auto halfGlyphHeight = (glyphHeight / 2.0f) * pathPixelSizeInWorld;
    auto bboxInWorldInitialized = false;
    AreaF bboxInWorldDirection;
    for (const auto& glyph : constOf(glyphsPlacement))
    {
        const auto halfGlyphWidth = (glyph.width / 2.0f) * pathPixelSizeInWorld;
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

int32_t OsmAnd::AtlasMapRendererSymbolsStage::getApproximate31(
    const double coordinate, const double coord1, const double coord2, const int32_t pos1, const int32_t pos2,
    const bool isPrimary, const bool isAxisY, const double* pRefLon, int32_t& iteration) const
{
    const auto delta = coordinate - coord1;
    const auto gap = coord2 - coord1;
    const auto distance = static_cast<double>(pos2 - pos1);
    auto pos = static_cast<int32_t>(distance * delta / gap + static_cast<double>(pos1));
    const PointI pos31(isAxisY ? currentState.target31.x : pos, isAxisY ? pos : currentState.target31.y);
    const auto point = isPrimary ? currentState.gridConfiguration.getPrimaryGridLocation(pos31, pRefLon)
        : currentState.gridConfiguration.getSecondaryGridLocation(pos31, pRefLon);
    const auto newCoord = isAxisY ? point.y : point.x;
    if (iteration < GRID_ITER_LIMIT && std::abs(newCoord - coordinate) > std::abs(gap) / GRID_ITER_PRECISION)
    {
        iteration++;
        const bool s = std::abs(newCoord - coord1) > std::abs(delta);
        pos = getApproximate31(coordinate, s ? newCoord : coord1, s ? coord2 : newCoord,
            s ? pos : pos1, s ? pos2 : pos, isPrimary, isAxisY, pRefLon, iteration);
    }
    return pos;
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

OsmAnd::AtlasMapRendererSymbolsStage::RenderableModel3DSymbol::~RenderableModel3DSymbol()
{
}

OsmAnd::AtlasMapRendererSymbolsStage::RenderableOnSurfaceSymbol::~RenderableOnSurfaceSymbol()
{
}
