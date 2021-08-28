#include "AtlasMapRendererSymbolsStage.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QLinkedList>
#include <QSet>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkMath.h>
#include <SkMathPriv.h>
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

OsmAnd::AtlasMapRendererSymbolsStage::AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer_)
    : AtlasMapRendererStage(renderer_)
{
}

OsmAnd::AtlasMapRendererSymbolsStage::~AtlasMapRendererSymbolsStage()
{
}

void OsmAnd::AtlasMapRendererSymbolsStage::prepare(AtlasMapRenderer_Metrics::Metric_renderFrame* const metric)
{
    Stopwatch stopwatch(metric != nullptr);

    ScreenQuadTree intersections;
    if (!obtainRenderableSymbols(renderableSymbols, intersections, metric))
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
    const AreaI screenArea,
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

void OsmAnd::AtlasMapRendererSymbolsStage::queryLastVisibleSymbolsAt(
    const PointI screenPoint,
    QList<IMapRenderer::MapSymbolInformation>& outMapSymbols) const
{
    QList< std::shared_ptr<const RenderableSymbol> > selectedRenderables;

    {
        QReadLocker scopedLocker(&_lastVisibleSymbolsLock);
        _lastVisibleSymbols.select(screenPoint, selectedRenderables);
    }

    convertRenderableSymbolsToMapSymbolInformation(selectedRenderables, outMapSymbols);
}

void OsmAnd::AtlasMapRendererSymbolsStage::queryLastVisibleSymbolsIn(
    const AreaI screenArea,
    QList<IMapRenderer::MapSymbolInformation>& outMapSymbols,
    const bool strict /*= false*/) const
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    Stopwatch stopwatch(metric != nullptr);

    // In case symbols update was not suspended, process published symbols
    if (!renderer->isSymbolsUpdateSuspended())
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

        if (metric)
        {
            metric->elapsedTimeForObtainingRenderableSymbolsWithLock = stopwatch.elapsed();
            metric->elapsedTimeForObtainingRenderableSymbolsOnlyLock =
                metric->elapsedTimeForObtainingRenderableSymbolsWithLock - metric->elapsedTimeForObtainingRenderableSymbols;
        }

        return result;
    }
    
    // Otherwise, use last accepted map symbols by order
    const auto result = obtainRenderableSymbols(
        _lastAcceptedMapSymbolsByOrder,
        outRenderableSymbols,
        outIntersections,
        nullptr,
        metric);
    return result;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderableSymbols(
    const MapRenderer::PublishedMapSymbolsByOrder& mapSymbolsByOrder,
    QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
    ScreenQuadTree& outIntersections,
    MapRenderer::PublishedMapSymbolsByOrder* pOutAcceptedMapSymbolsByOrder,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    Stopwatch stopwatch(metric != nullptr);

    typedef QLinkedList< std::shared_ptr<const RenderableSymbol> > PlottedSymbols;
    PlottedSymbols plottedSymbols;
    
    struct PlottedSymbolRef
    {
        PlottedSymbols::iterator iterator;
        std::shared_ptr<const RenderableSymbol> renderable;
    };
    struct PlottedSymbolsRefGroupInstance
    {
        QList< PlottedSymbolRef > symbolsRefs;

        void discard(
            const AtlasMapRendererSymbolsStage* const stage,
            PlottedSymbols& plottedSymbols,
            ScreenQuadTree& intersections)
        {
            // Discard entire group
            for (auto& symbolRef : symbolsRefs)
            {
                if (Q_UNLIKELY(stage->debugSettings->showSymbolsBBoxesRejectedByPresentationMode))
                    stage->addIntersectionDebugBox(symbolRef.renderable, ColorARGB::fromSkColor(SK_ColorYELLOW).withAlpha(50));

#if !OSMAND_KEEP_DISCARDED_SYMBOLS_IN_QUAD_TREE
                const auto removed = intersections.removeOne(symbolRef.renderable, symbolRef.renderable->intersectionBBox);
                //assert(removed);
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

        void discard(
            const AtlasMapRendererSymbolsStage* const stage,
            PlottedSymbols& plottedSymbols,
            ScreenQuadTree& intersections)
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
    outIntersections = qMove(ScreenQuadTree(currentState.viewport, qMax(treeDepth, 1u)));
    ComputedPathsDataCache computedPathsDataCache;
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

                    QList< std::shared_ptr<RenderableSymbol> > renderableSymbols;
                    obtainRenderablesFromSymbol(
                        mapSymbolsGroup,
                        mapSymbol,
                        nullptr,
                        referencesOrigins,
                        computedPathsDataCache,
                        renderableSymbols,
                        metric);

                    bool atLeastOnePlotted = false;
                    for (const auto& renderableSymbol : constOf(renderableSymbols))
                    {
                        if (!plotSymbol(renderableSymbol, outIntersections, metric))
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
                    const auto& additionalSymbolInstance = *citAdditionalSymbolInstance;

                    QList< std::shared_ptr<RenderableSymbol> > renderableSymbols;
                    obtainRenderablesFromSymbol(
                        mapSymbolsGroup,
                        mapSymbol,
                        additionalSymbolInstance,
                        referencesOrigins,
                        computedPathsDataCache,
                        renderableSymbols,
                        metric);

                    bool atLeastOnePlotted = false;
                    for (const auto& renderableSymbol : constOf(renderableSymbols))
                    {
                        if (!plotSymbol(renderableSymbol, outIntersections, metric))
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
                                [mapSymbol]
                                (const std::shared_ptr<const RenderableSymbol>& renderableSymbol) -> bool
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    Stopwatch stopwatch(metric != nullptr);

    bool plotted = false;
    if (const auto& renderableBillboard = std::dynamic_pointer_cast<RenderableBillboardSymbol>(renderable))
    {
        plotted = plotBillboardSymbol(
            renderableBillboard,
            intersections,
            metric);
    }
    else if (const auto& renderableOnPath = std::dynamic_pointer_cast<RenderableOnPathSymbol>(renderable))
    {
        plotted = plotOnPathSymbol(
            renderableOnPath,
            intersections,
            metric);
    }
    else if (const auto& renderableOnSurface = std::dynamic_pointer_cast<RenderableOnSurfaceSymbol>(renderable))
    {
        plotted = plotOnSurfaceSymbol(
            renderableOnSurface,
            intersections,
            metric);
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    const auto& internalState = getInternalState();
    const auto mapSymbol = std::dynamic_pointer_cast<const MapSymbol>(billboardMapSymbol);

    const auto& position31 =
        (instanceParameters && instanceParameters->overridesPosition31)
        ? instanceParameters->position31
        : billboardMapSymbol->getPosition31();

    // Test against visible frustum area (if allowed)
    if (!debugSettings->disableSymbolsFastCheckByFrustum &&
        mapSymbol->allowFastCheckByFrustum &&
        !internalState.globalFrustum2D31.test(position31))
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

    // Get GPU resource
    const auto gpuResource = captureGpuResource(referenceOrigins, mapSymbol);
    if (!gpuResource)
        return;

    std::shared_ptr<RenderableBillboardSymbol> renderable(new RenderableBillboardSymbol());
    renderable->mapSymbolGroup = mapSymbolGroup;
    renderable->mapSymbol = mapSymbol;
    renderable->genericInstanceParameters = instanceParameters;
    renderable->instanceParameters = instanceParameters;
    renderable->gpuResource = gpuResource;
    outRenderableSymbols.push_back(renderable);

    // Calculate location of symbol in world coordinates.
    renderable->offsetFromTarget31 = position31 - currentState.target31;
    renderable->offsetFromTarget = Utilities::convert31toFloat(renderable->offsetFromTarget31, currentState.zoomLevel);
    renderable->positionInWorld = glm::vec3(
        renderable->offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
        0.0f,
        renderable->offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

    // Get distance from symbol to camera
    renderable->distanceToCamera = glm::distance(internalState.worldCameraPosition, renderable->positionInWorld);
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    bool plotted = false;
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        plotted = plotBillboardRasterSymbol(
            renderable,
            intersections,
            metric);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        plotted = plotBillboardVectorSymbol(
            renderable,
            intersections,metric);
    }

    return plotted;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardRasterSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const BillboardRasterMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    const auto& offsetOnScreen =
        (renderable->instanceParameters && renderable->instanceParameters->overridesOffset)
        ? renderable->instanceParameters->offset
        : symbol->offset;

    // Calculate position in screen coordinates (same calculation as done in shader)
    const auto symbolOnScreen = glm_extensions::fastProject(
        renderable->positionInWorld,
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport);

    // Get bounds in screen coordinates
    auto boundsInWindow = AreaI::fromCenterAndSize(
        static_cast<int>(symbolOnScreen.x + offsetOnScreen.x),
        static_cast<int>((currentState.windowSize.y - symbolOnScreen.y) + offsetOnScreen.y),
        symbol->size.x,
        symbol->size.y);
    renderable->visibleBBox = boundsInWindow;
    boundsInWindow.top() -= symbol->margin.top();
    boundsInWindow.left() -= symbol->margin.left();
    boundsInWindow.right() += symbol->margin.right();
    boundsInWindow.bottom() += symbol->margin.bottom();
    renderable->intersectionBBox = boundsInWindow;

    if (!applyVisibilityFiltering(renderable->visibleBBox, intersections, metric))
        return false;

    if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
        return false;

    if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
        return false;

    return addToIntersections(renderable, intersections, metric);
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardVectorSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
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
    if (!debugSettings->disableSymbolsFastCheckByFrustum && mapSymbol->allowFastCheckByFrustum)
    {
        if (const auto vectorMapSymbol = std::dynamic_pointer_cast<const VectorMapSymbol>(onSurfaceMapSymbol))
        {
            AreaI symbolArea;
            QVector<PointI> symbolRect;
            switch (vectorMapSymbol->scaleType)
            {
                case VectorMapSymbol::ScaleType::Raw:
                    symbolRect << position31;
                    break;
                case VectorMapSymbol::ScaleType::In31:
                    symbolArea = AreaI(position31.y - vectorMapSymbol->scale, position31.x - vectorMapSymbol->scale, position31.y + vectorMapSymbol->scale, position31.x + vectorMapSymbol->scale);
                    symbolRect << symbolArea.topLeft << symbolArea.topRight() << symbolArea.bottomRight << symbolArea.bottomLeft();
                    break;
                case VectorMapSymbol::ScaleType::InMeters:
                    symbolArea = (AreaI)Utilities::boundingBox31FromAreaInMeters(vectorMapSymbol->scale, position31);
                    symbolRect << symbolArea.topLeft << symbolArea.topRight() << symbolArea.bottomRight << symbolArea.bottomLeft();
                    break;
            }
            if (!internalState.globalFrustum2D31.test(symbolRect))
            {
                if (metric)
                    metric->onSurfaceSymbolsRejectedByFrustum++;
                return;
            }
        }
        else if (!internalState.globalFrustum2D31.test(position31))
        {
            if (metric)
                metric->onSurfaceSymbolsRejectedByFrustum++;
            return;
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
    
    std::shared_ptr<RenderableOnSurfaceSymbol> renderable(new RenderableOnSurfaceSymbol());
    renderable->mapSymbolGroup = mapSymbolGroup;
    renderable->mapSymbol = mapSymbol;
    renderable->genericInstanceParameters = instanceParameters;
    renderable->instanceParameters = instanceParameters;
    renderable->gpuResource = gpuResource;
    outRenderableSymbols.push_back(renderable);

    // Calculate location of symbol in world coordinates.
    renderable->offsetFromTarget31 = position31 - currentState.target31;
    renderable->offsetFromTarget = Utilities::convert31toFloat(renderable->offsetFromTarget31, currentState.zoomLevel);
    renderable->positionInWorld = glm::vec3(
        renderable->offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
        0.0f,
        renderable->offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

    // Get direction
    if (IOnSurfaceMapSymbol::isAzimuthAlignedDirection(direction))
        renderable->direction = Utilities::normalizedAngleDegrees(currentState.azimuth + 180.0f);
    else
        renderable->direction = direction;

    // Get distance from symbol to camera
    renderable->distanceToCamera = glm::distance(internalState.worldCameraPosition, renderable->positionInWorld);
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return plotOnSurfaceRasterSymbol(
            renderable,
            intersections,
            metric);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        return plotOnSurfaceVectorSymbol(
            renderable,
            intersections,
            metric);
    }

    assert(false);
    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceRasterSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceRasterMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceVectorSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    ScreenQuadTree& intersections,
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
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
        onPathMapSymbol->allowFastCheckByFrustum &&
        !internalState.globalFrustum2D31.test(pinPointOnPath.point31))
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
    glm::vec2 exactStartPointOnScreen;
    glm::vec2 exactEndPointOnScreen;
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
    glm::vec2 exactStartPointInWorld;
    glm::vec2 exactEndPointInWorld;
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
    const auto directionInWorld = computePathDirection(
        computedPathData.pathInWorld,
        subpathStartIndex,
        exactStartPointInWorld,
        subpathEndIndex,
        exactEndPointInWorld);
    const auto directionOnScreen = computePathDirection(
        computedPathData.pathOnScreen,
        subpathStartIndex,
        exactStartPointOnScreen,
        subpathEndIndex,
        exactEndPointOnScreen);

    // Plot symbol instance.
    std::shared_ptr<RenderableOnPathSymbol> renderable(new RenderableOnPathSymbol());
    renderable->mapSymbolGroup = mapSymbolGroup;
    renderable->mapSymbol = onPathMapSymbol;
    renderable->genericInstanceParameters = instanceParameters;
    renderable->instanceParameters = instanceParameters;
    renderable->gpuResource = gpuResource;
    renderable->is2D = is2D;
    renderable->distanceToCamera = computeDistanceBetweenCameraToPath(
        computedPathData.pathInWorld,
        subpathStartIndex,
        exactStartPointInWorld,
        subpathEndIndex,
        exactEndPointOnScreen);
    renderable->directionInWorld = directionInWorld;
    renderable->directionOnScreen = directionOnScreen;
    renderable->glyphsPlacement = computePlacementOfGlyphsOnPath(
        is2D,
        is2D ? computedPathData.pathOnScreen : computedPathData.pathInWorld,
        is2D ? computedPathData.pathSegmentsLengthsOnScreen : computedPathData.pathSegmentsLengthsInWorld,
        subpathStartIndex,
        is2D ? offsetFromStartPathPoint2D : offsetFromStartPathPoint3D,
        subpathEndIndex,
        directionOnScreen,
        onPathMapSymbol->glyphsWidth);
    outRenderableSymbols.push_back(renderable);

    if (Q_UNLIKELY(debugSettings->showOnPathSymbolsRenderablesPaths))
    {
        const glm::vec2 directionOnScreenN(-directionOnScreen.y, directionOnScreen.x);

        // Path itself
        QVector< glm::vec3 > debugPoints;
        debugPoints.push_back(qMove(glm::vec3(
            exactStartPointInWorld.x,
            0.0f,
            exactStartPointInWorld.y)));
        auto pPointInWorld = computedPathData.pathInWorld.constData() + subpathStartIndex + 1;
        for (auto idx = subpathStartIndex + 1; idx <= subpathEndIndex; idx++)
        {
            const auto& pointInWorld = *(pPointInWorld++);
            debugPoints.push_back(qMove(glm::vec3(
                pointInWorld.x,
                0.0f,
                pointInWorld.y)));
        }
        debugPoints.push_back(qMove(glm::vec3(
            exactEndPointInWorld.x,
            0.0f,
            exactEndPointInWorld.y)));
        getRenderer()->debugStage->addLine3D(debugPoints, is2D ? SK_ColorGREEN : SK_ColorRED);

        // Subpath N (start)
        {
            QVector<glm::vec2> lineN;
            const auto sn0 = exactStartPointOnScreen;
            lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
            const auto sn1 = sn0 + (directionOnScreenN*24.0f);
            lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
            getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorCYAN, 128));
        }

        // Subpath N (end)
        {
            QVector<glm::vec2> lineN;
            const auto sn0 = exactEndPointOnScreen;
            lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
            const auto sn1 = sn0 + (directionOnScreenN*24.0f);
            lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
            getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
        }

        // Pin-point location
        {
            const auto pinPointInWorld = Utilities::convert31toFloat(
                pinPointOnPath.point31 - currentState.target31,
                currentState.zoomLevel) * static_cast<float>(AtlasMapRenderer::TileSize3D);
            const auto pinPointOnScreen = glm_extensions::fastProject(
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
    AtlasMapRenderer_Metrics::Metric_renderFrame* const metric) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathRasterMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Draw the glyphs
    if (renderable->is2D)
    {
        // Calculate OOBB for 2D SOP
        const auto oobb = calculateOnPath2dOOBB(renderable);
        renderable->visibleBBox = renderable->intersectionBBox = (OOBBI)oobb;

        if (!applyVisibilityFiltering(renderable->visibleBBox, intersections, metric))
            return false;

        //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
        //        oobb.enlargeBy(PointF(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

        if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
            return false;

        if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
            return false;

        if (!addToIntersections(renderable, intersections, metric))
            return false;

        if (Q_UNLIKELY(debugSettings->showOnPath2dSymbolGlyphDetails))
        {
            for (const auto& glyph : constOf(renderable->glyphsPlacement))
            {
                getRenderer()->debugStage->addRect2D(AreaF::fromCenterAndSize(
                    glyph.anchorPoint.x, currentState.windowSize.y - glyph.anchorPoint.y,
                    glyph.width, symbol->size.y), SkColorSetA(SK_ColorGREEN, 128), glyph.angle);

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
        // Calculate OOBB for 3D SOP in world
        const auto oobb = calculateOnPath3dOOBB(renderable);
        renderable->visibleBBox = renderable->intersectionBBox = (OOBBI)oobb;

        if (!applyVisibilityFiltering(renderable->visibleBBox, intersections, metric))
            return false;

        //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
        //        oobb.enlargeBy(PointF(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

        if (!applyIntersectionWithOtherSymbolsFiltering(renderable, intersections, metric))
            return false;

        if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(renderable, intersections, metric))
            return false;

        if (!addToIntersections(renderable, intersections, metric))
            return false;

        if (Q_UNLIKELY(debugSettings->showOnPath3dSymbolGlyphDetails))
        {
            for (const auto& glyph : constOf(renderable->glyphsPlacement))
            {
                const auto& glyphInMapPlane = AreaF::fromCenterAndSize(
                    glyph.anchorPoint.x, glyph.anchorPoint.y, /* anchor points are specified in world coordinates already */
                    glyph.width*internalState.pixelInWorldProjectionScale, symbol->size.y*internalState.pixelInWorldProjectionScale);
                const auto& tl = glyphInMapPlane.topLeft;
                const auto& tr = glyphInMapPlane.topRight();
                const auto& br = glyphInMapPlane.bottomRight;
                const auto& bl = glyphInMapPlane.bottomLeft();
                const glm::vec3 pC(glyph.anchorPoint.x, 0.0f, glyph.anchorPoint.y);
                const glm::vec4 p0(tl.x, 0.0f, tl.y, 1.0f);
                const glm::vec4 p1(tr.x, 0.0f, tr.y, 1.0f);
                const glm::vec4 p2(br.x, 0.0f, br.y, 1.0f);
                const glm::vec4 p3(bl.x, 0.0f, bl.y, 1.0f);
                const auto toCenter = glm::translate(-pC);
                const auto rotate = glm::rotate(qRadiansToDegrees((float)Utilities::normalizedAngleRadians(glyph.angle + M_PI)), glm::vec3(0.0f, -1.0f, 0.0f));
                const auto fromCenter = glm::translate(pC);
                const auto M = fromCenter*rotate*toCenter;
                getRenderer()->debugStage->addQuad3D((M*p0).xyz, (M*p1).xyz, (M*p2).xyz, (M*p3).xyz, SkColorSetA(SK_ColorGREEN, 128));

                QVector<glm::vec3> lineN;
                const auto ln0 = glyph.anchorPoint;
                lineN.push_back(glm::vec3(ln0.x, 0.0f, ln0.y));
                const auto ln1 = glyph.anchorPoint + (glyph.vNormal*16.0f*internalState.pixelInWorldProjectionScale);
                lineN.push_back(glm::vec3(ln1.x, 0.0f, ln1.y));
                getRenderer()->debugStage->addLine3D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
            }
        }
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::applyVisibilityFiltering(
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
        *(pPointOnScreen++) = glm_extensions::fastProject(
            glm::vec3(pPointInWorld->x, 0.0f, pPointInWorld->y),
            internalState.mPerspectiveProjectionView,
            internalState.glmViewport).xy;
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
            if (testPathPointIndex == 0)
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

glm::vec2 OsmAnd::AtlasMapRendererSymbolsStage::computePathDirection(
    const QVector<glm::vec2>& path,
    const unsigned int startPathPointIndex,
    const glm::vec2& exactStartPoint,
    const unsigned int endPathPointIndex,
    const glm::vec2& exactEndPoint)
{
    assert(endPathPointIndex >= startPathPointIndex);

    glm::vec2 subpathDirection;
    if (endPathPointIndex > startPathPointIndex)
    {
        const auto segmentsCount = endPathPointIndex - startPathPointIndex + 1;

        // First compute segment between exact start point (which is after startPathPointIndex)
        // and next point (startPathPointIndex + 1)
        subpathDirection += path[startPathPointIndex + 1] - exactStartPoint;

        auto pPathPoint = path.constData() + startPathPointIndex + 1;
        auto pPrevPathPoint = pPathPoint++;
        for (auto segmentIdx = 1; segmentIdx < segmentsCount - 1; segmentIdx++)
            subpathDirection += *(pPathPoint++) - *(pPrevPathPoint++);

        // Last check is between path[endPathPointIndex] and exact end point,
        // which is always after endPathPointIndex
        assert(pPrevPathPoint == &path[endPathPointIndex]);
        subpathDirection += exactEndPoint - *pPrevPathPoint;
    }
    else
    {
        // In case both exact points are on the same segment,
        // computation is simple
        subpathDirection = exactEndPoint - exactStartPoint;
    }

    return glm::normalize(subpathDirection);
}

double OsmAnd::AtlasMapRendererSymbolsStage::computeDistanceBetweenCameraToPath(
    const QVector<glm::vec2>& pathInWorld,
    const unsigned int startPathPointIndex,
    const glm::vec2& exactStartPointInWorld,
    const unsigned int endPathPointIndex,
    const glm::vec2& exactEndPointInWorld) const
{
    const auto& internalState = getInternalState();

    assert(endPathPointIndex >= startPathPointIndex);

    auto distanceToCamera = 0.0;

    // First process distance to exactStartPointInWorld
    {
        const auto distance = glm::distance(
            internalState.worldCameraPosition,
            glm::vec3(exactStartPointInWorld.x, 0.0f, exactStartPointInWorld.y));
        distanceToCamera += distance;
    }

    // Process distances to inner points
    auto pPathPointInWorld = pathInWorld.constData() + 1;
    for (auto pathPointIdx = startPathPointIndex + 1; pathPointIdx <= endPathPointIndex; pathPointIdx++)
    {
        const auto& pathPointInWorld = *(pPathPointInWorld++);

        const auto distance = glm::distance(
            internalState.worldCameraPosition,
            glm::vec3(pathPointInWorld.x, 0.0f, pathPointInWorld.y));
        distanceToCamera += distance;
    }

    // At last process distance to exactEndPointInWorld
    {
        const auto distance = glm::distance(
            internalState.worldCameraPosition,
            glm::vec3(exactEndPointInWorld.x, 0.0f, exactEndPointInWorld.y));
        distanceToCamera += distance;
    }

    // Normalize result
    distanceToCamera /= endPathPointIndex - startPathPointIndex + 2;

    return distanceToCamera;
}

QVector<OsmAnd::AtlasMapRendererSymbolsStage::RenderableOnPathSymbol::GlyphPlacement>
OsmAnd::AtlasMapRendererSymbolsStage::computePlacementOfGlyphsOnPath(
    const bool is2D,
    const QVector<glm::vec2>& path,
    const QVector<float>& pathSegmentsLengths,
    const unsigned int startPathPointIndex,
    const float offsetFromStartPathPoint,
    const unsigned int endPathPointIndex,
    const glm::vec2& directionOnScreen,
    const QVector<float>& glyphsWidths) const
{
    const auto& internalState = getInternalState();

    assert(endPathPointIndex >= startPathPointIndex);
    const auto projectionScale = is2D ? 1.0f : internalState.pixelInWorldProjectionScale;
    const glm::vec2 directionOnScreenN(-directionOnScreen.y, directionOnScreen.x);
    const auto shouldInvert = (directionOnScreenN.y /* == horizont.x*dirN.y - horizont.y*dirN.x == 1.0f*dirN.y - 0.0f*dirN.x */) < 0;

    // Initialize glyph input and output pointers
    const auto glyphsCount = glyphsWidths.size();
    QVector<RenderableOnPathSymbol::GlyphPlacement> glyphsPlacement(glyphsCount);
    auto pGlyphPlacement = glyphsPlacement.data();
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

    // Plot glyphs one by one
    auto segmentScanIndex = startPathPointIndex;
    auto scannedSegmentsLength = 0.0f;
    auto consumedSegmentsLength = 0.0f;
    auto prevGlyphOffset = offsetFromStartPathPoint;
    glm::vec2 currentSegmentStartPoint;
    glm::vec2 currentSegmentDirection;
    glm::vec2 currentSegmentN;
    auto currentSegmentAngle = 0.0f;
    for (int glyphIdx = 0; glyphIdx < glyphsCount; glyphIdx++, pGlyphWidth += glyphWidthIncrement)
    {
        // Get current glyph anchor offset and provide offset for next glyph
        const auto glyphWidth = *pGlyphWidth;
        const auto glyphWidthScaled = glyphWidth * projectionScale;
        const auto anchorOffset = prevGlyphOffset + glyphWidthScaled / 2.0f;
        prevGlyphOffset += glyphWidthScaled;

        // Find path segment where this glyph should be placed
        while (anchorOffset > scannedSegmentsLength)
        {
            if (segmentScanIndex > endPathPointIndex)
            {
                // Wow! This shouldn't happen ever, since it means that glyphs doesn't fit into the provided path!
                // And this means that path calculation above gave error!
                LogPrintf(LogSeverityLevel::Error,
                    "computePlacementOfGlyphsOnPath() failed:\n"
                    "\t%d glyphs;\n"
                    "\tfailed on glyph #%d;\n"
                    "\tstartPathPointIndex = %d;\n"
                    "\tendPathPointIndex = %d;\n"
                    "\tsegmentScanIndex = %d;\n"
                    "\tanchorOffset = %f;\n"
                    "\tscannedSegmentsLength = %f;\n"
                    "\tprojectionScale = %f;\n"
                    "\toffsetFromStartPathPoint = %f;\n"
                    "\tpathSegmentsLengths = %s;\n"
                    "\tglyphsWidths = %s;\n"
                    "\tglyphsWidths (scaled) = %s;",
                    glyphsCount,
                    glyphIdx,
                    startPathPointIndex,
                    endPathPointIndex,
                    segmentScanIndex,
                    anchorOffset,
                    scannedSegmentsLength,
                    projectionScale,
                    offsetFromStartPathPoint,
                    qPrintable(qTransform<QStringList>(
                        pathSegmentsLengths,
                        []
                        (const float& value) -> QStringList
                        {
                            return QStringList() << QString::number(value);
                        }).join(QLatin1String(", "))),
                    qPrintable(qTransform<QStringList>(
                        glyphsWidths,
                        []
                        (const float& value) -> QStringList
                        {
                            return QStringList() << QString::number(value);
                        }).join(QLatin1String(", "))),
                    qPrintable(qTransform<QStringList>(
                        glyphsWidths,
                        [projectionScale]
                        (const float& value) -> QStringList
                        {
                            return QStringList() << QString::number(value * projectionScale);
                        }).join(QLatin1String(", "))));
                glyphsPlacement.clear();
                return glyphsPlacement;
            }

            // Check this segment
            const auto& segmentLength = pathSegmentsLengths[segmentScanIndex];
            consumedSegmentsLength = scannedSegmentsLength;
            scannedSegmentsLength += segmentLength;
            segmentScanIndex++;
            if (anchorOffset > scannedSegmentsLength)
                continue;

            // Get points for this segment
            const auto& segmentStartPoint = path[segmentScanIndex - 1];
            const auto& segmentEndPoint = path[segmentScanIndex - 0];
            currentSegmentStartPoint = segmentStartPoint;

            // Get segment direction and normal
            currentSegmentDirection = (segmentEndPoint - segmentStartPoint) / segmentLength;
            if (is2D)
            {
                // CCW 90 degrees rotation of Y is up
                currentSegmentN.x = -currentSegmentDirection.y;
                currentSegmentN.y = currentSegmentDirection.x;
            }
            else
            {
                // CCW 90 degrees rotation of Y is down
                currentSegmentN.x = currentSegmentDirection.y;
                currentSegmentN.y = -currentSegmentDirection.x;
            }
            currentSegmentAngle = qAtan2(currentSegmentDirection.y, currentSegmentDirection.x);//TODO: maybe for 3D a -y should be passed (see -1 rotation axis)
            if (shouldInvert)
                currentSegmentAngle = Utilities::normalizedAngleRadians(currentSegmentAngle + M_PI);
        }

        // Compute anchor point
        const auto anchorOffsetFromSegmentStartPoint = (anchorOffset - consumedSegmentsLength);
        const auto anchorPoint = currentSegmentStartPoint + anchorOffsetFromSegmentStartPoint * currentSegmentDirection;

        // Add glyph location data.
        // In case inverted, filling is performed from back-to-front. Otherwise from front-to-back
        (shouldInvert ? *(pGlyphPlacement--) : *(pGlyphPlacement++)) = RenderableOnPathSymbol::GlyphPlacement(
            anchorPoint,
            glyphWidth,
            currentSegmentAngle,
            currentSegmentN);
    }

    return glyphsPlacement;
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

        const auto segmentAngleCos = qCos(glyph.angle);
        const auto segmentAngleSin = qSin(glyph.angle);

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

    const auto directionAngleInWorld = qAtan2(renderable->directionInWorld.y, renderable->directionInWorld.x);
    const auto negDirectionAngleInWorldCos = qCos(-directionAngleInWorld);
    const auto negDirectionAngleInWorldSin = qSin(-directionAngleInWorld);
    const auto directionAngleInWorldCos = qCos(directionAngleInWorld);
    const auto directionAngleInWorldSin = qSin(directionAngleInWorld);
    const auto halfGlyphHeight = (symbol->size.y / 2.0f) * internalState.pixelInWorldProjectionScale;
    auto bboxInWorldInitialized = false;
    AreaF bboxInWorldDirection;
    for (const auto& glyph : constOf(renderable->glyphsPlacement))
    {
        const auto halfGlyphWidth = (glyph.width / 2.0f) * internalState.pixelInWorldProjectionScale;
        const glm::vec2 glyphPoints[4] =
        {
            glm::vec2(-halfGlyphWidth, -halfGlyphHeight), // TL
            glm::vec2( halfGlyphWidth, -halfGlyphHeight), // TR
            glm::vec2( halfGlyphWidth,  halfGlyphHeight), // BR
            glm::vec2(-halfGlyphWidth,  halfGlyphHeight)  // BL
        };

        const auto segmentAngleCos = qCos(glyph.angle);
        const auto segmentAngleSin = qSin(glyph.angle);

        for (int idx = 0; idx < 4; idx++)
        {
            const auto& glyphPoint = glyphPoints[idx];

            // Rotate to align with it's segment
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

    PointF rotatedBBoxInWorld[4];
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
        const auto rotate = glm::rotate(qRadiansToDegrees((float)Utilities::normalizedAngleRadians(directionAngleInWorld + M_PI)), glm::vec3(0.0f, -1.0f, 0.0f));
        const auto fromCenter = glm::translate(pC);
        const auto M = fromCenter*rotate*toCenter;
        getRenderer()->debugStage->addQuad3D((M*p0).xyz, (M*p1).xyz, (M*p2).xyz, (M*p3).xyz, SkColorSetA(SK_ColorGREEN, 50));
    }
#endif // OSMAND_DEBUG
#if OSMAND_DEBUG && 0
        {
            const auto& tl = rotatedBBoxInWorld[0];
            const auto& tr = rotatedBBoxInWorld[1];
            const auto& br = rotatedBBoxInWorld[2];
            const auto& bl = rotatedBBoxInWorld[3];

            const glm::vec3 p0(tl.x, 0.0f, tl.y);
            const glm::vec3 p1(tr.x, 0.0f, tr.y);
            const glm::vec3 p2(br.x, 0.0f, br.y);
            const glm::vec3 p3(bl.x, 0.0f, bl.y);
            getRenderer()->debugStage->addQuad3D(p0, p1, p2, p3, SkColorSetA(SK_ColorGREEN, 50));
        }
#endif // OSMAND_DEBUG

    // Project points of OOBB in world to screen
    const PointF projectedRotatedBBoxInWorldP0(static_cast<glm::vec2>(
        glm_extensions::fastProject(glm::vec3(rotatedBBoxInWorld[0].x, 0.0f, rotatedBBoxInWorld[0].y),
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy));
    const PointF projectedRotatedBBoxInWorldP1(static_cast<glm::vec2>(
        glm_extensions::fastProject(glm::vec3(rotatedBBoxInWorld[1].x, 0.0f, rotatedBBoxInWorld[1].y),
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy));
    const PointF projectedRotatedBBoxInWorldP2(static_cast<glm::vec2>(
        glm_extensions::fastProject(glm::vec3(rotatedBBoxInWorld[2].x, 0.0f, rotatedBBoxInWorld[2].y),
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy));
    const PointF projectedRotatedBBoxInWorldP3(static_cast<glm::vec2>(
        glm_extensions::fastProject(glm::vec3(rotatedBBoxInWorld[3].x, 0.0f, rotatedBBoxInWorld[3].y),
        internalState.mPerspectiveProjectionView,
        internalState.glmViewport).xy));
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
