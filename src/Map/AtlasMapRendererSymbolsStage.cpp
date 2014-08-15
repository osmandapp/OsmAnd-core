#include "AtlasMapRendererSymbolsStage.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QLinkedList>
#include <QSet>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "AtlasMapRenderer.h"
#include "AtlasMapRendererDebugStage.h"
#include "MapSymbol.h"
#include "VectorMapSymbol.h"
#include "BillboardVectorMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "OnPathMapSymbol.h"
#include "BillboardRasterMapSymbol.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "MapSymbolsGroup.h"
#include "MapSymbolsGroupWithId.h"
#include "QKeyValueIterator.h"
#include "ObjectWithId.h"

OsmAnd::AtlasMapRendererSymbolsStage::AtlasMapRendererSymbolsStage(AtlasMapRenderer* const renderer_)
    : AtlasMapRendererStage(renderer_)
{
}

OsmAnd::AtlasMapRendererSymbolsStage::~AtlasMapRendererSymbolsStage()
{
}

void OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderableSymbols(
    QList< std::shared_ptr<const RenderableSymbol> >& outRenderableSymbols,
    IntersectionsQuadTree& outIntersections) const
{
    typedef QLinkedList< std::shared_ptr<const RenderableSymbol> > PlottedSymbols;
    struct PlottedSymbolRef
    {
        PlottedSymbols::iterator iterator;
        std::shared_ptr<const MapSymbol> mapSymbol;
    };

    QReadLocker scopedLocker(&publishedMapSymbolsLock);

    const auto& internalState = getInternalState();

    // Iterate over map symbols layer sorted by "order" in ascending direction
    outIntersections = IntersectionsQuadTree(currentState.viewport, 8);
    PlottedSymbols plottedSymbols;
    QHash< const MapSymbolsGroup*, QList< PlottedSymbolRef > > plottedMapSymbolsByGroup;
    for (const auto& publishedMapSymbols : constOf(publishedMapSymbols))
    {
        // Obtain renderables in order how they should be rendered
        QMultiMap< float, std::shared_ptr<RenderableSymbol> > sortedRenderables;
        if (!debugSettings->excludeOnPathSymbolsFromProcessing)
            processOnPathSymbols(publishedMapSymbols, sortedRenderables);
        if (!debugSettings->excludeBillboardSymbolsFromProcessing)
            processBillboardSymbols(publishedMapSymbols, sortedRenderables);
        if (!debugSettings->excludeOnSurfaceSymbolsFromProcessing)
            processOnSurfaceSymbols(publishedMapSymbols, sortedRenderables);

        // Plot symbols in reversed order, since sortedSymbols contains symbols by distance from camera from near->far.
        // And rendering needs to be done far->near, as well as plotting
        auto itRenderableEntry = iteratorOf(sortedRenderables);
        itRenderableEntry.toBack();
        while (itRenderableEntry.hasPrevious())
        {
            const auto& entry = itRenderableEntry.previous();
            const auto renderable = entry.value();

            bool plotted = false;
            if (const auto& renderable_ = std::dynamic_pointer_cast<RenderableBillboardSymbol>(renderable))
            {
                plotted = plotBillboardSymbol(
                    renderable_,
                    outIntersections);
            }
            else if (const auto& renderable_ = std::dynamic_pointer_cast<RenderableOnPathSymbol>(renderable))
            {
                plotted = plotOnPathSymbol(
                    renderable_,
                    outIntersections);
            }
            else if (const auto& renderable_ = std::dynamic_pointer_cast<RenderableOnSurfaceSymbol>(renderable))
            {
                plotted = plotOnSurfaceSymbol(
                    renderable_,
                    outIntersections);
            }

            if (plotted)
            {
                const auto itPlottedSymbol = plottedSymbols.insert(plottedSymbols.end(), renderable);
                PlottedSymbolRef plottedSymbolRef = { itPlottedSymbol, renderable->mapSymbol };

                plottedMapSymbolsByGroup[renderable->mapSymbol->groupPtr].push_back(qMove(plottedSymbolRef));
            }
        }
    }

    //TODO: also remove from outIntersections!!!!!

    // Remove those plotted symbols that do not conform to presentation rules
    auto itPlottedSymbolsGroup = mutableIteratorOf(plottedMapSymbolsByGroup);
    while (itPlottedSymbolsGroup.hasNext())
    {
        auto& plottedGroupSymbols = itPlottedSymbolsGroup.next().value();

        const auto mapSymbolGroup = plottedGroupSymbols.first().mapSymbol->group.lock();
        if (!mapSymbolGroup)
        {
            // Discard entire group
            for (const auto& plottedGroupSymbol : constOf(plottedGroupSymbols))
                plottedSymbols.erase(plottedGroupSymbol.iterator);

            itPlottedSymbolsGroup.remove();
            continue;
        }

        // Just skip all rules
        if (mapSymbolGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAnything)
            continue;

        // Rule: show all symbols or no symbols
        if (mapSymbolGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAllOrNothing)
        {
            if (mapSymbolGroup->symbols.size() != plottedGroupSymbols.size())
            {
                // Discard entire group
                for (const auto& plottedGroupSymbol : constOf(plottedGroupSymbols))
                    plottedSymbols.erase(plottedGroupSymbol.iterator);

                itPlottedSymbolsGroup.remove();
                continue;
            }
        }

        // Rule: if there's icon, icon must always be visible. Otherwise discard entire group
        if (mapSymbolGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowNoneIfIconIsNotShown)
        {
            const auto symbolWithIconContentClass = mapSymbolGroup->getFirstSymbolWithContentClass(MapSymbol::ContentClass::Icon);
            if (symbolWithIconContentClass)
            {
                bool iconPlotted = false;
                for (const auto& plottedGroupSymbol : constOf(plottedGroupSymbols))
                {
                    if (plottedGroupSymbol.mapSymbol == symbolWithIconContentClass)
                    {
                        iconPlotted = true;
                        break;
                    }
                }

                if (!iconPlotted)
                {
                    // Discard entire group
                    for (const auto& plottedGroupSymbol : constOf(plottedGroupSymbols))
                        plottedSymbols.erase(plottedGroupSymbol.iterator);

                    itPlottedSymbolsGroup.remove();
                    continue;
                }
            }
        }

        // Rule: if at least one caption was not shown, discard all other captions
        if (mapSymbolGroup->presentationMode & MapSymbolsGroup::PresentationModeFlag::ShowAllCaptionsOrNoCaptions)
        {
            const auto captionsCount = mapSymbolGroup->numberOfSymbolsWithContentClass(MapSymbol::ContentClass::Caption);
            if (captionsCount > 0)
            {
                unsigned int captionsPlotted = 0;
                for (const auto& plottedGroupSymbol : constOf(plottedGroupSymbols))
                {
                    if (plottedGroupSymbol.mapSymbol->contentClass == MapSymbol::ContentClass::Caption)
                        captionsPlotted++;
                }

                if (captionsCount != captionsPlotted)
                {
                    // Discard all plotted captions from group
                    auto itPlottedGroupSymbol = mutableIteratorOf(plottedGroupSymbols);
                    while (itPlottedGroupSymbol.hasNext())
                    {
                        const auto& plottedGroupSymbol = itPlottedGroupSymbol.next();

                        if (plottedGroupSymbol.mapSymbol->contentClass != MapSymbol::ContentClass::Caption)
                            continue;

                        plottedSymbols.erase(plottedGroupSymbol.iterator);
                        itPlottedGroupSymbol.remove();
                    }
                }
            }
        }
    }

    // Publish the result
    outRenderableSymbols.clear();
    outRenderableSymbols.reserve(plottedSymbols.size());
    for (const auto& plottedSymbol : constOf(plottedSymbols))
        outRenderableSymbols.push_back(plottedSymbol);
}

void OsmAnd::AtlasMapRendererSymbolsStage::processOnPathSymbols(
    const MapRenderer::PublishedMapSymbols& input,
    QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const
{
    // Process on path symbols to get set of renderables
    QList< std::shared_ptr<RenderableOnPathSymbol> > renderables;
    obtainRenderablesFromOnPathSymbols(input, renderables);

    // Sort visible SOPs by distance to camera
    sortRenderablesFromOnPathSymbols(renderables, output);
}

void OsmAnd::AtlasMapRendererSymbolsStage::obtainRenderablesFromOnPathSymbols(
    const MapRenderer::PublishedMapSymbols& input,
    QList< std::shared_ptr<RenderableOnPathSymbol> >& output) const
{
    const auto& internalState = getInternalState();

    for (const auto& symbolEntry : rangeOf(constOf(input)))
    {
        const auto& currentSymbol_ = symbolEntry.key();
        if (currentSymbol_->isHidden)
            continue;
        const auto currentSymbol = std::dynamic_pointer_cast<const OnPathMapSymbol>(currentSymbol_);
        if (!currentSymbol)
            continue;

        const auto& path31 = currentSymbol->path;
        const auto pathSize = path31.size();
        const auto& symbolPinPoints = currentSymbol->pinPoints;

        // Path must have at least 2 points and there must be at least one pin-point
        if (Q_UNLIKELY(pathSize < 2))
        {
            assert(false);
            continue;
        }

        // Skip if there are no pin-points
        if (Q_UNLIKELY(symbolPinPoints.isEmpty()))
        {
            if (debugSettings->showAllPaths)
            {
                bool seenOnPathSymbolInGroup = false;
                bool thisIsFirstOnPathSymbolInGroup = false;
                bool allSymbolsHaveNoPinPoints = true;
                const auto symbolsGroup = currentSymbol->group.lock();
                for (const auto& otherSymbol_ : constOf(symbolsGroup->symbols))
                {
                    const auto otherSymbol = std::dynamic_pointer_cast<const OnPathMapSymbol>(otherSymbol_);
                    if (!otherSymbol)
                        continue;

                    if (!seenOnPathSymbolInGroup)
                    {
                        if (otherSymbol == currentSymbol)
                        {
                            thisIsFirstOnPathSymbolInGroup = true;
                            continue;
                        }
                        else
                            break;
                        seenOnPathSymbolInGroup = true;
                    }

                    if (!otherSymbol->pinPoints.isEmpty())
                    {
                        allSymbolsHaveNoPinPoints = false;
                        break;
                    }
                }

                if (thisIsFirstOnPathSymbolInGroup && allSymbolsHaveNoPinPoints)
                {
                    QVector< glm::vec3 > debugPoints;
                    for (const auto& pointInWorld : convertPoints31ToWorld(path31))
                    {
                        debugPoints.push_back(qMove(glm::vec3(
                            pointInWorld.x,
                            0.0f,
                            pointInWorld.y)));
                    }
                    getRenderer()->debugStage->addLine3D(debugPoints, SK_ColorYELLOW);
                }
            }

            continue;
        }

        // Get GPU resource for this map symbol, since it's useless to perform any calculations unless it's possible to draw it
        const auto gpuResource = std::dynamic_pointer_cast<const GPUAPI::TextureInGPU>(captureGpuResource(symbolEntry.value(), currentSymbol_));
        if (!gpuResource)
        {
            if (Q_UNLIKELY(debugSettings->showAllPaths))
            {
                QVector< glm::vec3 > debugPoints;
                for (const auto& pointInWorld : convertPoints31ToWorld(path31))
                {
                    debugPoints.push_back(qMove(glm::vec3(
                        pointInWorld.x,
                        0.0f,
                        pointInWorld.y)));
                }
                getRenderer()->debugStage->addLine3D(debugPoints, SK_ColorCYAN);
            }

            continue;
        }

        if (debugSettings->showAllPaths)
        {
            bool thisIsFirstOnPathSymbolInGroup = false;
            const auto symbolsGroup = currentSymbol->group.lock();
            for (const auto& otherSymbol_ : constOf(symbolsGroup->symbols))
            {
                const auto otherSymbol = std::dynamic_pointer_cast<const OnPathMapSymbol>(otherSymbol_);
                if (!otherSymbol)
                    continue;

                if (otherSymbol != currentSymbol)
                    break;
                thisIsFirstOnPathSymbolInGroup = true;
                break;
            }

            if (thisIsFirstOnPathSymbolInGroup)
            {
                QVector< glm::vec3 > debugPoints;
                for (const auto& pointInWorld : convertPoints31ToWorld(path31))
                {
                    debugPoints.push_back(qMove(glm::vec3(
                        pointInWorld.x,
                        0.0f,
                        pointInWorld.y)));
                }
                getRenderer()->debugStage->addLine3D(debugPoints, SK_ColorGRAY);
            }
        }

        // Processing pin-points needs path in world and path on screen, as well as lengths of all segments
        const auto pathInWorld = convertPoints31ToWorld(path31);
        const auto pathOnScreen = projectFromWorldToScreen(pathInWorld);
        for (const auto& pinPoint : constOf(symbolPinPoints))
        {
            // pin point represents cennter.
            // so, find endpint and start point in 2d and see if it's ok
            // if it doesn't, find endpoint and startpoint in 3D and plot
        }

        //// For each pin point generate an instance of current symbol
        //for (const auto& pinPoint : pinPoints)
        //{
        //    // pin points represents center of subpath!!!!

        //    // first try 2D mode : calculate center in 2D, then try left and right half to check if it's still 2D
        //    // otherwise use 3D. 
        //}

        //// Calculate widths of entire on-path-symbols in group and width of symbols before current symbol
        //float totalWidth = 0.0f;
        //float widthBeforeCurrentSymbol = 0.0f;
        //for (const auto& otherSymbol_ : constOf(mapSymbolsGroup->symbols))
        //{
        //    // Verify that other symbol is also OnPathSymbol
        //    const auto otherSymbol = std::dynamic_pointer_cast<const OnPathMapSymbol>(otherSymbol_);
        //    if (!otherSymbol)
        //        continue;

        //    if (otherSymbol == currentSymbol)
        //        widthBeforeCurrentSymbol = totalWidth;
        //    totalWidth += otherSymbol->size.x;
        //}

        //// Plot multiple renderables using only world coordinates as in top-down viewmode.
        //// This will produce start point index and offset
        //
        //
        //unsigned int originPointIndex = 0;
        //float originOffset = 0.0f;
        //if (widthBeforeCurrentSymbol > 0.0f)
        //{
        //    unsigned int offsetEndPointIndex = 0;
        //    float nextOffset = 0.0f;
        //    const auto offsetBeforeCurrentSymbolFits = computeEndPointIndexAndNextOffsetIn3D(
        //        pathSize,
        //        pathInWorld,
        //        widthBeforeCurrentSymbol,
        //        0, // Origin is the first point of path
        //        0.0f, // There's no offset
        //        offsetEndPointIndex,
        //        nextOffset);
        //    
        //    // In case even offset failed to fit, nothing can be done
        //    if (!offsetBeforeCurrentSymbolFits)
        //        continue;

        //    // Since computeEndPointIndexAndNextOffsetIn3D() returns end-point-index, which includes optionally half-used segment,
        //    // origin points index should point to previous point
        //    originPointIndex = offsetEndPointIndex - 1;
        //    originOffset = nextOffset;
        //}
        //unsigned int symbolInstancesFitted = 0;
        //for (;;)
        //{
        //    // Find start point index of new instance and offset of next instance
        //    unsigned int currentInstanceEndPointIndex = 0;
        //    float nextOffset = 0.0f;
        //    const auto currentSymbolInstanceFits = computeEndPointIndexAndNextOffsetIn3D(
        //        pathSize,
        //        pathInWorld,
        //        currentSymbol->size.x,
        //        originPointIndex,
        //        originOffset,
        //        currentInstanceEndPointIndex,
        //        nextOffset);

        //    // Stop in case current symbol doesn't fit anymore
        //    if (!currentSymbolInstanceFits)
        //    {
        //        // If current symbol is the first one and it doesn't fit, show it
        //        if (Q_UNLIKELY(debugSettings->showTooShortOnPathSymbolsRenderablesPaths) &&
        //            currentSymbol_ == mapSymbolsGroup->symbols.first() &&
        //            symbolInstancesFitted == 0)
        //        {
        //            QVector< glm::vec3 > debugPoints;
        //            for (const auto& pointInWorld : pathInWorld)
        //            {
        //                debugPoints.push_back(qMove(glm::vec3(
        //                    pointInWorld.x,
        //                    0.0f,
        //                    pointInWorld.y)));
        //            }
        //            getRenderer()->debugStage->addLine3D(debugPoints, SkColorSetA(SK_ColorYELLOW, 128));
        //        }

        //        break;
        //    }

        //    // Plot symbol instance.
        //    // During this actually determine 2D or 3D mode, and compute glyph placement.
        //    const auto subpathStartIndex = originPointIndex;
        //    const auto subpathEndIndex = currentInstanceEndPointIndex;
        //    const auto subpathPointsCount = subpathEndIndex - subpathStartIndex + 1;
        //    std::shared_ptr<RenderableOnPathSymbol> renderable(new RenderableOnPathSymbol());
        //    renderable->mapSymbol = currentSymbol_;
        //    renderable->gpuResource = gpuResource;
        //    // Since to check if symbol instance can be rendered in 2D mode entire 2D points are needed,
        //    // compute them only for subpath
        //    const auto subpathOnScreen = projectFromWorldToScreen(pathInWorld, subpathStartIndex, subpathEndIndex);
        //    renderable->is2D = pathRenderableAs2D(subpathOnScreen);
        //    renderable->distanceToCamera = computeDistanceBetweenCameraToPath(
        //        pathInWorld,
        //        subpathStartIndex,
        //        subpathEndIndex);
        //    glm::vec2 exactStartPointInWorld;
        //    glm::vec2 exactEndPointInWorld;
        //    renderable->directionInWorld = computeSubpathDirectionInWorld(
        //        pathInWorld,
        //        originOffset,
        //        nextOffset,
        //        subpathStartIndex,
        //        subpathEndIndex,
        //        &exactStartPointInWorld,
        //        &exactEndPointInWorld);
        //    glm::vec2 exactStartPointOnScreen;
        //    glm::vec2 exactEndPointOnScreen;
        //    renderable->directionOnScreen = computePathDirectionOnScreen(
        //        subpathOnScreen,
        //        exactStartPointInWorld,
        //        exactEndPointInWorld,
        //        &exactStartPointOnScreen,
        //        &exactEndPointOnScreen);
        //    renderable->glyphsPlacement = computePlacementOfGlyphsOnPath(
        //        renderable->is2D,
        //        pathInWorld,
        //        subpathStartIndex,
        //        subpathEndIndex,
        //        exactStartPointInWorld,
        //        exactEndPointInWorld,
        //        subpathOnScreen,
        //        exactStartPointOnScreen,
        //        exactEndPointOnScreen,
        //        renderable->directionOnScreen,
        //        currentSymbol->glyphsWidth);
        //    output.push_back(qMove(renderable));
        //    if (Q_UNLIKELY(debugSettings->showOnPathSymbolsRenderablesPaths))
        //    {
        //        const glm::vec2 directionOnScreenN(-renderable->directionOnScreen.y, renderable->directionOnScreen.x);

        //        // Path itself
        //        QVector< glm::vec3 > debugPoints;
        //        debugPoints.push_back(qMove(glm::vec3(
        //            exactStartPointInWorld.x,
        //            0.0f,
        //            exactStartPointInWorld.y)));
        //        auto pPointInWorld = pathInWorld.constData() + subpathStartIndex + 1;
        //        for (auto idx = subpathStartIndex + 1; idx < subpathEndIndex; idx++, pPointInWorld++)
        //        {
        //            debugPoints.push_back(qMove(glm::vec3(
        //                pPointInWorld->x,
        //                0.0f,
        //                pPointInWorld->y)));
        //        }
        //        debugPoints.push_back(qMove(glm::vec3(
        //            exactEndPointInWorld.x,
        //            0.0f,
        //            exactEndPointInWorld.y)));
        //        getRenderer()->debugStage->addLine3D(debugPoints, SkColorSetA(renderable->is2D ? SK_ColorGREEN : SK_ColorRED, 128));

        //        // Subpath N (start)
        //        {
        //            QVector<glm::vec2> lineN;
        //            const auto sn0 = exactStartPointOnScreen;
        //            lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
        //            const auto sn1 = sn0 + (directionOnScreenN*32.0f);
        //            lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
        //            getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorCYAN, 128));
        //        }

        //        // Subpath N (end)
        //        {
        //            QVector<glm::vec2> lineN;
        //            const auto sn0 = exactEndPointOnScreen;
        //            lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
        //            const auto sn1 = sn0 + (directionOnScreenN*32.0f);
        //            lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
        //            getRenderer()->debugStage->addLine2D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
        //        }
        //    }
        //    symbolInstancesFitted++;

        //    // Since computeEndPointIndexAndNextOffsetIn3D() returns end-point-index, which includes optionally half-used segment,
        //    // origin points index should point to previous point
        //    originPointIndex = currentInstanceEndPointIndex - 1;
        //    originOffset = nextOffset;

        //    // And now compute next instance origin and offset
        //    unsigned int spaceBetweenInstancedEndPointIndex = 0;
        //    const auto spaceBetweenEndOfCurrentInstanceAndStartOfNextInstance = totalWidth - currentSymbol->size.x;
        //    const auto spaceBetweenInstancesFits = computeEndPointIndexAndNextOffsetIn3D(
        //        pathSize,
        //        pathInWorld,
        //        spaceBetweenEndOfCurrentInstanceAndStartOfNextInstance,
        //        originPointIndex,
        //        originOffset,
        //        spaceBetweenInstancedEndPointIndex,
        //        nextOffset);
        //    if (!spaceBetweenInstancesFits)
        //        break;

        //    // Since computeEndPointIndexAndNextOffsetIn3D() returns end-point-index, which includes optionally half-used segment,
        //    // origin points index should point to previous point
        //    originPointIndex = spaceBetweenInstancedEndPointIndex - 1;
        //    originOffset = nextOffset;
        //}
    }
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::convertPoints31ToWorld(const QVector<PointI>& points31) const
{
    return convertPoints31ToWorld(points31, 0, points31.size() - 1);
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::convertPoints31ToWorld(const QVector<PointI>& points31, unsigned int startIndex, unsigned int endIndex) const
{
    const auto& internalState = getInternalState();

    const auto count = endIndex - startIndex + 1;
    QVector<glm::vec2> result(count);
    auto pPointInWorld = result.data();
    auto pPoint31 = points31.constData() + startIndex;

    for (auto idx = 0u; idx < count; idx++)
    {
        *(pPointInWorld++) = Utilities::convert31toFloat(
                *(pPoint31++) - currentState.target31,
                currentState.zoomBase) * static_cast<float>(AtlasMapRenderer::TileSize3D);
    }

    return result;
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::projectFromWorldToScreen(const QVector<glm::vec2>& pointsInWorld) const
{
    return projectFromWorldToScreen(pointsInWorld, 0, pointsInWorld.size() - 1);
}

QVector<glm::vec2> OsmAnd::AtlasMapRendererSymbolsStage::projectFromWorldToScreen(const QVector<glm::vec2>& pointsInWorld, unsigned int startIndex, unsigned int endIndex) const
{
    const auto& internalState = getInternalState();

    const auto count = endIndex - startIndex + 1;
    QVector<glm::vec2> result(count);
    auto pPointOnScreen = result.data();
    auto pPointInWorld = pointsInWorld.constData() + startIndex;

    for (auto idx = 0u; idx < count; idx++)
    {
        *(pPointOnScreen++) = glm::project(
            glm::vec3(pPointInWorld->x, 0.0f, pPointInWorld->y),
            internalState.mCameraView,
            internalState.mPerspectiveProjection,
            internalState.glmViewport).xy;
        pPointInWorld++;
    }

    return result;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::pathRenderableAs2D(const QVector<glm::vec2>& pathOnScreen) const
{
    // Calculate 'incline' of line and compare to horizontal direction.
    // If any 'incline' is larger than 15 degrees, this line can not be rendered as 2D

    const static float inclineThresholdSinSq = 0.0669872981; // qSin(qDegreesToRadians(15.0f))*qSin(qDegreesToRadians(15.0f))

    auto pPointOnScreen = pathOnScreen.data();
    auto pPrevPointOnScreen = pPointOnScreen++;
    for (auto idx = 1, count = pathOnScreen.size(); idx < count; idx++)
    {
        const auto vSegment = *(pPointOnScreen++) - *(pPrevPointOnScreen++);
        const auto d = vSegment.y;// horizont.x*vSegment.y - horizont.y*vSegment.x == 1.0f*vSegment.y - 0.0f*vSegment.x
        const auto inclineSinSq = d*d / (vSegment.x*vSegment.x + vSegment.y*vSegment.y);
        if (qAbs(inclineSinSq) > inclineThresholdSinSq)
            return false;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::computeEndPointIndexAndNextOffsetIn3D(
    const unsigned int pathSize,
    const QVector<glm::vec2>& pathInWorld,
    const float requestedLengthInPixels,
    const unsigned int startPointIndex,
    const float offset,
    unsigned int& outEndPointIndex,
    float& outNextOffset) const
{
    const auto& internalState = getInternalState();

    const auto requestedLength = requestedLengthInPixels * internalState.pixelInWorldProjectionScale;

    auto testPointIndex = startPointIndex + 1;
    float entireSegmentLength = 0.0f;
    while (testPointIndex < pathSize)
    {
        // Add length of last segment [testPointIndex - 1, testPointIndex]
        const auto currentSegmentLength = glm::distance(
            pathInWorld[testPointIndex - 1],
            pathInWorld[testPointIndex]);
        entireSegmentLength += currentSegmentLength;

        // If segment length with subtracted "already-occupied portion" can fit requested length,
        // then plot a renderable there
        if (entireSegmentLength - offset >= requestedLength)
        {
            outEndPointIndex = testPointIndex;
            outNextOffset = currentSegmentLength - (entireSegmentLength - offset - requestedLength);
            assert(outNextOffset <= currentSegmentLength);
            assert(outNextOffset >= 0.0f);

            return true;
        }

        // Move to next text point
        testPointIndex++;
    }

    return false;
}

glm::vec2 OsmAnd::AtlasMapRendererSymbolsStage::computeSubpathDirectionInWorld(
    const QVector<glm::vec2>& pathInWorld,
    const float offsetFromStart,
    const float offsetToEnd,
    const unsigned int startPointIndex,
    const unsigned int endPointIndex,
    glm::vec2* outExactStartPoint /*= nullptr*/,
    glm::vec2* outExactEndPoint /*= nullptr*/) const
{
    const auto pathSize = endPointIndex - startPointIndex + 1;

    const auto& startPoint = pathInWorld[startPointIndex];
    const auto vFromStart = pathInWorld[startPointIndex + 1] - startPoint;
    glm::vec2 exactStartPoint = startPoint + glm::normalize(vFromStart) * offsetFromStart;
    if (outExactStartPoint)
        *outExactStartPoint = exactStartPoint;

    const auto& endPoint = pathInWorld[endPointIndex];
    const auto& lastPoint = pathInWorld[endPointIndex - 1];
    const auto vFromEnd = endPoint - lastPoint;
    glm::vec2 exactEndPoint = lastPoint + glm::normalize(vFromEnd) * offsetToEnd;
    if (outExactEndPoint)
        *outExactEndPoint = exactEndPoint;

    glm::vec2 subpathDirection;
    if (pathSize > 2)
    {
        subpathDirection += vFromStart;
        auto pPrevPoint = &startPoint + 1;
        auto pPoint = pPrevPoint + 1;
        for (auto idx = startPointIndex + 2; idx < endPointIndex; idx++)
            subpathDirection += (*(pPoint++) - *(pPrevPoint++));
        subpathDirection += vFromEnd;
    }
    else
    {
        subpathDirection = exactEndPoint - exactStartPoint;
    }

    return glm::normalize(subpathDirection);
}

glm::vec2 OsmAnd::AtlasMapRendererSymbolsStage::computePathDirectionOnScreen(
    const QVector<glm::vec2>& pathOnScreen,
    const glm::vec2& exactStartPointInWorld,
    const glm::vec2& exactEndPointInWorld,
    glm::vec2* outExactStartPointOnScreen /*= nullptr*/,
    glm::vec2* outExactEndPointOnScreen /*= nullptr*/) const
{
    const auto& internalState = getInternalState();

    const auto pathSize = pathOnScreen.size();

    glm::vec2 exactStartPointOnScreen = glm::project(
        glm::vec3(exactStartPointInWorld.x, 0.0f, exactStartPointInWorld.y),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport).xy;
    if (outExactStartPointOnScreen)
        *outExactStartPointOnScreen = exactStartPointOnScreen;

    glm::vec2 exactEndPointOnScreen = glm::project(
        glm::vec3(exactEndPointInWorld.x, 0.0f, exactEndPointInWorld.y),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport).xy;
    if (outExactEndPointOnScreen)
        *outExactEndPointOnScreen = exactEndPointOnScreen;

    glm::vec2 subpathDirection;
    if (pathSize > 2)
    {
        subpathDirection += pathOnScreen[1] - exactStartPointOnScreen;
        auto pPrevPoint = pathOnScreen.constData() + 1;
        auto pPoint = pPrevPoint + 1;
        for (auto idx = 2; idx < pathSize - 1; idx++)
            subpathDirection += (*(pPoint++) - *(pPrevPoint++));
        subpathDirection += exactEndPointOnScreen - pathOnScreen[pathSize - 2];
    }
    else
    {
        subpathDirection = exactEndPointOnScreen - exactStartPointOnScreen;
    }

    return glm::normalize(subpathDirection);
}

QVector<OsmAnd::AtlasMapRendererSymbolsStage::RenderableOnPathSymbol::GlyphPlacement>
OsmAnd::AtlasMapRendererSymbolsStage::computePlacementOfGlyphsOnPath(
    const bool is2D,
    const QVector<glm::vec2>& pathInWorld,
    const unsigned int startPointIndex,
    const unsigned int endPointIndex,
    const glm::vec2& exactStartPointInWorld,
    const glm::vec2& exactEndPointInWorld,
    const QVector<glm::vec2>& subpathOnScreen,
    const glm::vec2& exactStartPointOnScreen,
    const glm::vec2& exactEndPointOnScreen,
    const glm::vec2& directionOnScreen,
    const QVector<float>& glyphsWidths) const
{
    const auto& internalState = getInternalState();

    const auto pathSize = endPointIndex - startPointIndex + 1;
    const auto projectionScale = is2D ? 1.0f : internalState.pixelInWorldProjectionScale;
    const glm::vec2 directionOnScreenN(-directionOnScreen.y, directionOnScreen.x);
    const auto shouldInvert = (directionOnScreenN.y /* == horizont.x*dirN.y - horizont.y*dirN.x == 1.0f*dirN.y - 0.0f*dirN.x */) < 0;

    // Compute lengths
    QVector<float> lengths(pathSize - 1);
    if (pathSize > 2)
    {
        auto pPoint = is2D ? (subpathOnScreen.constData() + 1) : (pathInWorld.constData() + startPointIndex + 1);
        auto pPrevPoint = pPoint;

        auto pLength = lengths.data();

        *(pLength++) = glm::distance(is2D ? exactStartPointOnScreen : exactStartPointInWorld, *(pPoint++));
        for (auto idx = 1, count = lengths.size() - 1; idx < count; idx++)
            *(pLength++) = glm::distance(*(pPrevPoint++), *(pPoint++));
        *pLength = glm::distance(*pPrevPoint, is2D ? exactEndPointOnScreen : exactEndPointInWorld);
    }
    else
    {
        *lengths.data() = glm::distance(
            is2D ? exactStartPointOnScreen : exactStartPointInWorld,
            is2D ? exactEndPointOnScreen : exactEndPointInWorld);
    }

    typedef std::function<const glm::vec2& (unsigned int)> PointGetter;
    const PointGetter getPoint = is2D
        ? (PointGetter)[startPointIndex, endPointIndex, subpathOnScreen, exactStartPointOnScreen, exactEndPointOnScreen]
        (const unsigned int pointIndex) -> const glm::vec2&
        {
            if (pointIndex == startPointIndex)
                return exactStartPointOnScreen;
            else if (pointIndex == endPointIndex)
                return exactEndPointOnScreen;
            else
                return subpathOnScreen[pointIndex - startPointIndex];
        }
        : (PointGetter)[startPointIndex, endPointIndex, pathInWorld, exactStartPointInWorld, exactEndPointInWorld]
        (const unsigned int pointIndex) -> const glm::vec2&
        {
            if (pointIndex == startPointIndex)
                return exactStartPointInWorld;
            else if (pointIndex == endPointIndex)
                return exactEndPointInWorld;
            else
                return pathInWorld[pointIndex];
        };

    const auto glyphsCount = glyphsWidths.size();
    QVector<RenderableOnPathSymbol::GlyphPlacement> glyphsPlacement(glyphsCount);
    auto pGlyphPlacement = glyphsPlacement.data();
    if (shouldInvert)
    {
        // In case of direction inversion, fill from end
        pGlyphPlacement += glyphsCount - 1;
    }

    const auto pointsCount = endPointIndex - startPointIndex + 1;
    auto testPointIndex = startPointIndex + 1;
    auto lengthIndex = 0u;
    auto pGlyphWidth = glyphsWidths.constData();
    if (shouldInvert)
    {
        // In case of direction inversion, start from last glyph
        pGlyphWidth += glyphsCount - 1;
    }
    float lastSegmentLength = 0.0f;
    glm::vec2 vLastPoint0;
    glm::vec2 vLastPoint1;
    glm::vec2 vLastSegment;
    glm::vec2 vLastSegmentN;
    float lastSegmentAngle = 0.0f;
    float segmentsLengthSum = 0.0f;
    float prevOffset = 0.0f;
    auto glyphsPlotted = 0u;
    for (int idx = 0; idx < glyphsCount; idx++, pGlyphWidth += (shouldInvert ? -1 : +1))
    {
        // Get current glyph anchor offset and provide offset for next glyph
        const auto& glyphWidth = *pGlyphWidth;
        const auto glyphWidthScaled = glyphWidth*projectionScale;
        const auto anchorOffset = prevOffset + glyphWidthScaled / 2.0f;
        prevOffset += glyphWidthScaled;

        // Get subpath segment, where anchor is located
        while (segmentsLengthSum < anchorOffset)
        {
            if (testPointIndex > endPointIndex)
            {
                // Wow! This shouldn't happen ever, since it means that glyphs doesn't fit into the provided path!
                // And this means that path calculation above gave error!
                //assert(false);
                glyphsPlacement.resize(/*glyphsPlotted*/3);
                return glyphsPlacement;
            }
            const auto& p0 = getPoint(testPointIndex - 1);
            const auto& p1 = getPoint(testPointIndex);
            lastSegmentLength = lengths[lengthIndex];
            segmentsLengthSum += lastSegmentLength;
            testPointIndex++;
            lengthIndex++;

            vLastPoint0 = p0;
            vLastPoint1 = p1;
            vLastSegment = (vLastPoint1 - vLastPoint0) / lastSegmentLength;
            if (is2D)
            {
                // CCW 90 degrees rotation of Y is up
                vLastSegmentN.x = -vLastSegment.y;
                vLastSegmentN.y = vLastSegment.x;
            }
            else
            {
                // CCW 90 degrees rotation of Y is down
                vLastSegmentN.x = vLastSegment.y;
                vLastSegmentN.y = -vLastSegment.x;
            }
            lastSegmentAngle = qAtan2(vLastSegment.y, vLastSegment.x);//TODO: maybe for 3D a -y should be passed (see -1 rotation axis)
            if (shouldInvert)
                lastSegmentAngle = Utilities::normalizedAngleRadians(lastSegmentAngle + M_PI);
        }

        // Calculate anchor point
        const auto anchorPoint = vLastPoint0 + (anchorOffset - (segmentsLengthSum - lastSegmentLength))*vLastSegment;

        // Add glyph location data.
        // In case inverted, filling is performed from back-to-front. Otherwise from front-to-back
        (shouldInvert ? *(pGlyphPlacement--) : *(pGlyphPlacement++)) = RenderableOnPathSymbol::GlyphPlacement(
            anchorPoint,
            glyphWidth,
            lastSegmentAngle,
            vLastSegmentN);
        glyphsPlotted++;
    }

    return glyphsPlacement;
}

double OsmAnd::AtlasMapRendererSymbolsStage::computeDistanceBetweenCameraToPath(
    const QVector<glm::vec2>& pathInWorld,
    const unsigned int startPointIndex,
    const unsigned int endPointIndex) const
{
    const auto& internalState = getInternalState();

    auto distanceToCamera = 0.0;
    for (const auto& pointInWorld : constOf(pathInWorld))
    {
        const auto& distance = glm::distance(internalState.worldCameraPosition, glm::vec3(pointInWorld.x, 0.0f, pointInWorld.y));
        if (distance > distanceToCamera)
            distanceToCamera = distance;
    }

    return distanceToCamera;
}

void OsmAnd::AtlasMapRendererSymbolsStage::sortRenderablesFromOnPathSymbols(
    const QList< std::shared_ptr<RenderableOnPathSymbol> >& entries,
    QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const
{
    // Sort visible SOPs by distance to camera
    for (auto& renderable : entries)
    {
        // Insert into map
        const auto distanceToCamera = renderable->distanceToCamera;
        output.insert(distanceToCamera, qMove(renderable));
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage::processBillboardSymbols(
    const MapRenderer::PublishedMapSymbols& input,
    QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const
{
    obtainAndSortRenderablesFromBillboardSymbols(input, output);
}

void OsmAnd::AtlasMapRendererSymbolsStage::obtainAndSortRenderablesFromBillboardSymbols(
    const MapRenderer::PublishedMapSymbols& input,
    QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const
{
    const auto& internalState = getInternalState();

    // Sort sprite symbols by distance to camera
    for (const auto& symbolEntry : rangeOf(constOf(input)))
    {
        const auto& symbol_ = symbolEntry.key();
        if (symbol_->isHidden)
            continue;
        const auto& symbol = std::dynamic_pointer_cast<const IBillboardMapSymbol>(symbol_);
        if (!symbol)
            continue;

        // Get GPU resource
        const auto gpuResource = captureGpuResource(symbolEntry.value(), symbol_);
        if (!gpuResource)
            continue;

        std::shared_ptr<RenderableBillboardSymbol> renderable(new RenderableBillboardSymbol());
        renderable->mapSymbol = symbol_;
        renderable->gpuResource = gpuResource;

        // Calculate location of symbol in world coordinates.
        renderable->offsetFromTarget31 = symbol->getPosition31() - currentState.target31;
        renderable->offsetFromTarget = Utilities::convert31toFloat(renderable->offsetFromTarget31, currentState.zoomBase);
        renderable->positionInWorld = glm::vec3(
            renderable->offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
            0.0f,
            renderable->offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

        // Get distance from symbol to camera
        const auto distanceToCamera = renderable->distanceToCamera = glm::distance(internalState.worldCameraPosition, renderable->positionInWorld);

        // Insert into map
        output.insert(distanceToCamera, qMove(renderable));
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage::processOnSurfaceSymbols(
    const MapRenderer::PublishedMapSymbols& input,
    QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const
{
    obtainAndSortRenderablesFromOnSurfaceSymbols(input, output);
}

void OsmAnd::AtlasMapRendererSymbolsStage::obtainAndSortRenderablesFromOnSurfaceSymbols(
    const MapRenderer::PublishedMapSymbols& input,
    QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output) const
{
    const auto& internalState = getInternalState();

    // Sort on-surface symbols by distance to camera
    for (const auto& symbolEntry : rangeOf(constOf(input)))
    {
        const auto& symbol_ = symbolEntry.key();
        if (symbol_->isHidden)
            continue;
        const auto& symbol = std::dynamic_pointer_cast<const IOnSurfaceMapSymbol>(symbol_);
        if (!symbol)
            continue;

        // Get GPU resource
        const auto gpuResource = captureGpuResource(symbolEntry.value(), symbol_);
        if (!gpuResource)
            continue;

        std::shared_ptr<RenderableOnSurfaceSymbol> renderable(new RenderableOnSurfaceSymbol());
        renderable->mapSymbol = symbol_;
        renderable->gpuResource = gpuResource;

        // Calculate location of symbol in world coordinates.
        renderable->offsetFromTarget31 = symbol->getPosition31() - currentState.target31;
        renderable->offsetFromTarget = Utilities::convert31toFloat(renderable->offsetFromTarget31, currentState.zoomBase);
        renderable->positionInWorld = glm::vec3(
            renderable->offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
            0.0f,
            renderable->offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

        // Get direction
        if (symbol->isAzimuthAlignedDirection())
            renderable->direction = Utilities::normalizedAngleDegrees(currentState.azimuth + 180.0f);
        else
            renderable->direction = symbol->getDirection();

        // Get distance from symbol to camera
        const auto distanceToCamera = renderable->distanceToCamera = glm::distance(internalState.worldCameraPosition, renderable->positionInWorld);

        // Insert into map
        output.insert(distanceToCamera, qMove(renderable));
    }
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return plotBillboardRasterSymbol(
            renderable,
            intersections);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        return plotBillboardVectorSymbol(
            renderable,
            intersections);
    }

    assert(false);
    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardRasterSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const BillboardRasterMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Calculate position in screen coordinates (same calculation as done in shader)
    const auto symbolOnScreen = glm::project(
        renderable->positionInWorld,
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport);

    // Get bounds in screen coordinates
    auto boundsInWindow = AreaI::fromCenterAndSize(
        static_cast<int>(symbolOnScreen.x + symbol->offset.x), static_cast<int>((currentState.windowSize.y - symbolOnScreen.y) + symbol->offset.y),
        symbol->size.x, symbol->size.y);
    //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
//    boundsInWindow.enlargeBy(PointI(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

    if (!applyIntersectionWithOtherSymbolsFiltering(boundsInWindow, renderable, intersections))
        return false;

    if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(boundsInWindow, renderable, intersections))
        return false;

    return plotRenderable(boundsInWindow, renderable, intersections);
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotBillboardVectorSymbol(
    const std::shared_ptr<RenderableBillboardSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    assert(false);
    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnPathSymbol(
    const std::shared_ptr<RenderableOnPathSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnPathMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    // Draw the glyphs
    if (renderable->is2D)
    {
        // Calculate OOBB for 2D SOP
        const auto oobb = calculateOnPath2dOOBB(renderable);

        //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
//        oobb.enlargeBy(PointF(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

        if (!applyIntersectionWithOtherSymbolsFiltering(oobb, renderable, intersections))
            return false;

        if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(oobb, renderable, intersections))
            return false;

        if (!plotRenderable(oobb, renderable, intersections))
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

        //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
//        oobb.enlargeBy(PointF(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

        if (!applyIntersectionWithOtherSymbolsFiltering(oobb, renderable, intersections))
            return false;

        if (!applyMinDistanceToSameContentFromOtherSymbolFiltering(oobb, renderable, intersections))
            return false;

        if (!plotRenderable(oobb, renderable, intersections))
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

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    if (std::dynamic_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol))
    {
        return plotOnSurfaceRasterSymbol(
            renderable,
            intersections);
    }
    else if (std::dynamic_pointer_cast<const VectorMapSymbol>(renderable->mapSymbol))
    {
        return plotOnSurfaceVectorSymbol(
            renderable,
            intersections);
    }

    assert(false);
    return false;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceRasterSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceRasterMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotOnSurfaceVectorSymbol(
    const std::shared_ptr<RenderableOnSurfaceSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    const auto& internalState = getInternalState();

    const auto& symbol = std::static_pointer_cast<const OnSurfaceVectorMapSymbol>(renderable->mapSymbol);
    const auto& symbolGroupPtr = symbol->groupPtr;
    
    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::applyIntersectionWithOtherSymbolsFiltering(
    const AreaI boundsInWindow,
    const std::shared_ptr<const RenderableSymbol>& renderable,
    const IntersectionsQuadTree& intersections) const
{
    const auto& symbol = renderable->mapSymbol;

    if (symbol->intersectionModeFlags & MapSymbol::IgnoredByIntersectionTest)
        return true;

    if (Q_UNLIKELY(debugSettings->skipSymbolsIntersectionCheck))
        return true;
    
    // Check intersections
    const auto symbolGroupPtr = symbol->groupPtr;
    const auto intersects = intersections.test(boundsInWindow, false,
        [symbolGroupPtr]
        (const std::shared_ptr<const RenderableSymbol>& otherRenderable, const IntersectionsQuadTree::BBox& otherBBox) -> bool
        {
            const auto& otherSymbol = otherRenderable->mapSymbol;

            // Only accept intersections with symbols from other groups
            return otherSymbol->groupPtr != symbolGroupPtr;
        });
    if (intersects)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesRejectedByIntersectionCheck))
        {
            getRenderer()->debugStage->addRect2D((AreaF)boundsInWindow, SkColorSetA(SK_ColorRED, 50));
            getRenderer()->debugStage->addLine2D({
                (glm::ivec2)boundsInWindow.topLeft,
                (glm::ivec2)boundsInWindow.topRight(),
                (glm::ivec2)boundsInWindow.bottomRight,
                (glm::ivec2)boundsInWindow.bottomLeft(),
                (glm::ivec2)boundsInWindow.topLeft
            }, SK_ColorRED);
        }
        return false;
    }
   
    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::applyIntersectionWithOtherSymbolsFiltering(
    const OOBBF oobb,
    const std::shared_ptr<const RenderableSymbol>& renderable,
    const IntersectionsQuadTree& intersections) const
{
    const auto& symbol = renderable->mapSymbol;

    if (symbol->intersectionModeFlags & MapSymbol::IgnoredByIntersectionTest)
        return true;

    if (Q_UNLIKELY(debugSettings->skipSymbolsIntersectionCheck))
        return true;

    // Check intersections
    const auto symbolGroupPtr = symbol->groupPtr;
    const auto intersects = intersections.test(OOBBI(oobb), false,
        [symbolGroupPtr]
        (const std::shared_ptr<const RenderableSymbol>& otherRenderable, const IntersectionsQuadTree::BBox& otherBBox) -> bool
        {
            const auto& otherSymbol = otherRenderable->mapSymbol;

            // Only accept intersections with symbols from other groups
            return otherSymbol->groupPtr != symbolGroupPtr;
        });
    if (intersects)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesRejectedByIntersectionCheck))
        {
            getRenderer()->debugStage->addRect2D(oobb.unrotatedBBox(), SkColorSetA(SK_ColorRED, 50), -oobb.rotation());
            getRenderer()->debugStage->addLine2D({
                oobb.pointInGlobalSpace0(),
                oobb.pointInGlobalSpace1(),
                oobb.pointInGlobalSpace2(),
                oobb.pointInGlobalSpace3(),
                oobb.pointInGlobalSpace0()
            }, SK_ColorRED);
        }
        return false;
    }
    
    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::applyMinDistanceToSameContentFromOtherSymbolFiltering(
    const AreaI boundsInWindow,
    const std::shared_ptr<const RenderableSymbol>& renderable,
    const IntersectionsQuadTree& intersections) const
{
    const auto& symbol = std::static_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol);

    if ((symbol->minDistance.x <= 0 && symbol->minDistance.y <= 0) || symbol->content.isNull())
        return true;

    if (Q_UNLIKELY(debugSettings->skipSymbolsMinDistanceToSameContentFromOtherSymbolCheck))
        return true;

    // Query for similar content in area of "minDistance" to exclude duplicates, but keep if from same mapObject
    const auto symbolGroupPtr = symbol->groupPtr;
    const auto& symbolContent = symbol->content;
    const auto hasSimilarContent = intersections.test(boundsInWindow.getEnlargedBy(symbol->minDistance), false,
        [symbolContent, symbolGroupPtr]
        (const std::shared_ptr<const RenderableSymbol>& otherRenderable, const IntersectionsQuadTree::BBox& otherBBox) -> bool
        {
            const auto otherSymbol = std::dynamic_pointer_cast<const RasterMapSymbol>(otherRenderable->mapSymbol);
            if (!otherSymbol)
                return false;

            return
                (otherSymbol->groupPtr != symbolGroupPtr) &&
                (otherSymbol->content == symbolContent);
        });
    if (hasSimilarContent)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck))
        {
            getRenderer()->debugStage->addRect2D(AreaF(boundsInWindow.getEnlargedBy(symbol->minDistance)), SkColorSetA(SK_ColorRED, 50));
            getRenderer()->debugStage->addRect2D(AreaF(boundsInWindow), SkColorSetA(SK_ColorRED, 128));
            getRenderer()->debugStage->addLine2D({
                (glm::ivec2)boundsInWindow.topLeft,
                (glm::ivec2)boundsInWindow.topRight(),
                (glm::ivec2)boundsInWindow.bottomRight,
                (glm::ivec2)boundsInWindow.bottomLeft(),
                (glm::ivec2)boundsInWindow.topLeft
            }, SK_ColorRED);
        }
        return false;
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::applyMinDistanceToSameContentFromOtherSymbolFiltering(
    const OOBBF oobb,
    const std::shared_ptr<const RenderableSymbol>& renderable,
    const IntersectionsQuadTree& intersections) const
{
    const auto& symbol = std::static_pointer_cast<const RasterMapSymbol>(renderable->mapSymbol);

    if ((symbol->minDistance.x <= 0 && symbol->minDistance.y <= 0) || symbol->content.isNull())
        return true;

    if (Q_UNLIKELY(debugSettings->skipSymbolsMinDistanceToSameContentFromOtherSymbolCheck))
        return true;

    // Query for similar content in area of "minDistance" to exclude duplicates, but keep if from same mapObject
    const auto symbolGroupPtr = symbol->groupPtr;
    const auto& symbolContent = symbol->content;
    const auto hasSimilarContent = intersections.test(OOBBI(oobb).getEnlargedBy(symbol->minDistance), false,
        [symbolContent, symbolGroupPtr]
        (const std::shared_ptr<const RenderableSymbol>& otherRenderable, const IntersectionsQuadTree::BBox& otherBBox) -> bool
        {
            const auto otherSymbol = std::dynamic_pointer_cast<const RasterMapSymbol>(otherRenderable->mapSymbol);
            if (!otherSymbol)
                return false;

            return
                (otherSymbol->groupPtr != symbolGroupPtr) &&
                (otherSymbol->content == symbolContent);
        });
    if (hasSimilarContent)
    {
        if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesRejectedByMinDistanceToSameContentFromOtherSymbolCheck))
        {
            getRenderer()->debugStage->addRect2D(oobb.getEnlargedBy(PointF(symbol->minDistance)).unrotatedBBox(), SkColorSetA(SK_ColorRED, 50), -oobb.rotation());
            getRenderer()->debugStage->addRect2D(oobb.unrotatedBBox(), SkColorSetA(SK_ColorRED, 128), -oobb.rotation());
            getRenderer()->debugStage->addLine2D({
                oobb.pointInGlobalSpace0(),
                oobb.pointInGlobalSpace1(),
                oobb.pointInGlobalSpace2(),
                oobb.pointInGlobalSpace3(),
                oobb.pointInGlobalSpace0()
            }, SK_ColorRED);
        }
        return false;
    }

    return true;
}

OsmAnd::OOBBF OsmAnd::AtlasMapRendererSymbolsStage::calculateOnPath2dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const
{
    const auto& symbol = std::static_pointer_cast<const OnPathMapSymbol>(renderable->mapSymbol);

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

OsmAnd::OOBBF OsmAnd::AtlasMapRendererSymbolsStage::calculateOnPath3dOOBB(const std::shared_ptr<RenderableOnPathSymbol>& renderable) const
{
    const auto& internalState = getInternalState();
    const auto& symbol = std::static_pointer_cast<const OnPathMapSymbol>(renderable->mapSymbol);

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
        glm::project(glm::vec3(rotatedBBoxInWorld[0].x, 0.0f, rotatedBBoxInWorld[0].y),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport).xy));
    const PointF projectedRotatedBBoxInWorldP1(static_cast<glm::vec2>(
        glm::project(glm::vec3(rotatedBBoxInWorld[1].x, 0.0f, rotatedBBoxInWorld[1].y),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport).xy));
    const PointF projectedRotatedBBoxInWorldP2(static_cast<glm::vec2>(
        glm::project(glm::vec3(rotatedBBoxInWorld[2].x, 0.0f, rotatedBBoxInWorld[2].y),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
        internalState.glmViewport).xy));
    const PointF projectedRotatedBBoxInWorldP3(static_cast<glm::vec2>(
        glm::project(glm::vec3(rotatedBBoxInWorld[3].x, 0.0f, rotatedBBoxInWorld[3].y),
        internalState.mCameraView,
        internalState.mPerspectiveProjection,
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

bool OsmAnd::AtlasMapRendererSymbolsStage::plotRenderable(
    const AreaI boundsInWindow,
    const std::shared_ptr<const RenderableSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    if (!renderable->mapSymbol->intersectionModeFlags.isSet(MapSymbol::TransparentForIntersectionLookup) &&
        !Q_UNLIKELY(debugSettings->allSymbolsTransparentForIntersectionLookup))
    {
        // Insert into quad-tree
        if (!intersections.insert(renderable, boundsInWindow))
        {
            if (debugSettings->showSymbolsBBoxesRejectedByIntersectionCheck)
            {
                getRenderer()->debugStage->addRect2D((AreaF)boundsInWindow, SkColorSetA(SK_ColorBLUE, 50));
                getRenderer()->debugStage->addLine2D({
                    (glm::ivec2)boundsInWindow.topLeft,
                    (glm::ivec2)boundsInWindow.topRight(),
                    (glm::ivec2)boundsInWindow.bottomRight,
                    (glm::ivec2)boundsInWindow.bottomLeft(),
                    (glm::ivec2)boundsInWindow.topLeft
                }, SK_ColorBLUE);
            }
            return false;
        }
    }

    if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesAcceptedByIntersectionCheck))
    {
        getRenderer()->debugStage->addRect2D((AreaF)boundsInWindow, SkColorSetA(SK_ColorGREEN, 50));
        getRenderer()->debugStage->addLine2D({
            (glm::ivec2)boundsInWindow.topLeft,
            (glm::ivec2)boundsInWindow.topRight(),
            (glm::ivec2)boundsInWindow.bottomRight,
            (glm::ivec2)boundsInWindow.bottomLeft(),
            (glm::ivec2)boundsInWindow.topLeft
        }, SK_ColorGREEN);
    }

    return true;
}

bool OsmAnd::AtlasMapRendererSymbolsStage::plotRenderable(
    const OOBBF oobb,
    const std::shared_ptr<const RenderableSymbol>& renderable,
    IntersectionsQuadTree& intersections) const
{
    if (!renderable->mapSymbol->intersectionModeFlags.isSet(MapSymbol::TransparentForIntersectionLookup) &&
        !Q_UNLIKELY(debugSettings->allSymbolsTransparentForIntersectionLookup))
    {
        // Insert into quad-tree
        if (!intersections.insert(renderable, OOBBI(oobb)))
        {
            if (debugSettings->showSymbolsBBoxesRejectedByIntersectionCheck)
            {
                getRenderer()->debugStage->addRect2D(oobb.unrotatedBBox(), SkColorSetA(SK_ColorBLUE, 50), -oobb.rotation());
                getRenderer()->debugStage->addLine2D({
                    oobb.pointInGlobalSpace0(),
                    oobb.pointInGlobalSpace1(),
                    oobb.pointInGlobalSpace2(),
                    oobb.pointInGlobalSpace3(),
                    oobb.pointInGlobalSpace0()
                }, SK_ColorBLUE);
            }
            return false;
        }
    }

    if (Q_UNLIKELY(debugSettings->showSymbolsBBoxesAcceptedByIntersectionCheck))
    {
        getRenderer()->debugStage->addRect2D(oobb.unrotatedBBox(), SkColorSetA(SK_ColorGREEN, 50), -oobb.rotation());
        getRenderer()->debugStage->addLine2D({
            oobb.pointInGlobalSpace0(),
            oobb.pointInGlobalSpace1(),
            oobb.pointInGlobalSpace2(),
            oobb.pointInGlobalSpace3(),
            oobb.pointInGlobalSpace0(),
        }, SK_ColorGREEN);
    }

    return true;
}

std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU> OsmAnd::AtlasMapRendererSymbolsStage::captureGpuResource(
    const MapRenderer::MapSymbolreferenceOrigins& resources,
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

        // Stop as soon as GPU resource found
        if (gpuResource)
            return gpuResource;
    }
    return nullptr;
}

void OsmAnd::AtlasMapRendererSymbolsStage::prepare()
{
    IntersectionsQuadTree intersections;
    obtainRenderableSymbols(renderableSymbols, intersections);

    {
        QWriteLocker scopedLocker(&_lastPreparedIntersectionsLock);
        _lastPreparedIntersections = qMove(intersections);
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage::queryLastPreparedSymbolsAt(
    const PointI screenPoint,
    QList< std::shared_ptr<const MapSymbol> >& outMapSymbols) const
{
    QList< std::shared_ptr<const RenderableSymbol> > selectedRenderables;
    
    {
        QReadLocker scopedLocker(&_lastPreparedIntersectionsLock);
        _lastPreparedIntersections.select(screenPoint, selectedRenderables);
    }

    QSet< std::shared_ptr<const MapSymbol> > mapSymbolsSet;
    for (const auto& renderable : constOf(selectedRenderables))
        mapSymbolsSet.insert(renderable->mapSymbol);
    outMapSymbols = mapSymbolsSet.toList();
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
