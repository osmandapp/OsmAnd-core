#include "AtlasMapRendererSymbolsStage_OpenGL.h"

#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <SkColor.h>

#include "AtlasMapRenderer_OpenGL.h"
#include "MapSymbolProvidersCommon.h"
#include "QuadTree.h"
#include "QKeyValueIterator.h"
#include "ObjectWithId.h"
#include "Utilities.h"

//#define OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK 1
#if !defined(OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK)
#   define OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK 0
#endif // !defined(OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK)

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer)
    : AtlasMapRendererStage_OpenGL(renderer)
    , _maxGlyphsPerDrawCallSOP2D(-1)
    , _maxGlyphsPerDrawCallSOP3D(-1)
{
}

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::~AtlasMapRendererSymbolsStage_OpenGL()
{
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initialize()
{
    initializePinned();
    initializeOnPath();
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::render()
{
    GL_PUSH_GROUP_MARKER(QLatin1String("symbols"));

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform2f);
    GL_CHECK_PRESENT(glUniform3f);
    GL_CHECK_PRESENT(glDrawElements);

    const auto gpuAPI = getGPUAPI();
    const auto renderer = getRenderer();

    struct RenderableSymbol
    {
        virtual ~RenderableSymbol()
        {
        }

        std::shared_ptr<const MapSymbol> mapSymbol;
        std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
    };

    struct RenderablePinnedSymbol : RenderableSymbol
    {
        PointI offsetFromTarget31;
        PointF offsetFromTarget;
        glm::vec3 positionInWorld;
    };

    struct RenderableSymbolOnPath : RenderableSymbol
    {
        int subpathStartIndex;
        int subpathEndIndex;
        QVector<glm::vec2> subpathPointsInWorld;
        float offset;
        float subpathLength;
        QVector<float> segmentLengths;
        glm::vec2 subpathDirectionOnScreen;
        bool is2D;

        // 2D-only:
        QVector<glm::vec2> subpathPointsOnScreen;

        // 3D-only:
        glm::vec2 subpathDirectioInWorld;
    };

    QMutexLocker scopedLocker(&getResources().getSymbolsMapMutex());
    const auto& mapSymbolsByOrder = getResources().getMapSymbolsByOrder();
    const auto& topDownCameraView = internalState.mDistance * internalState.mAzimuth;
    const glm::vec4 viewport(
        currentState.viewport.left,
        currentState.windowSize.y - currentState.viewport.bottom,
        currentState.viewport.width(),
        currentState.viewport.height());
    const auto mPerspectiveProjectionView = internalState.mPerspectiveProjection * internalState.mCameraView;
    int lastUsedProgram = -1;

    typedef std::shared_ptr<const RenderableSymbol> RenderableSymbolEntry;
    typedef std::shared_ptr<RenderableSymbolOnPath> RenderableSymbolOnPathEntry;

    // Intersections are calculated using quad-tree, and general rule that
    // map symbol with smaller order [and further from camera] is more important.
    typedef QuadTree< std::shared_ptr<const MapSymbol>, AreaI::CoordType > IntersectionsQuadTree;
    IntersectionsQuadTree intersections(currentState.viewport, 8);

    // Iterate over symbols by "order" in ascending direction
    for(const auto& mapSymbolsLayerPair : rangeOf(constOf(mapSymbolsByOrder)))
    {
        // For each "order" value, obtain list of entries and sort them
        const auto& mapSymbolsLayer = mapSymbolsLayerPair.value();
        if (mapSymbolsLayer.isEmpty())
            continue;

        GL_PUSH_GROUP_MARKER(QString("order %1").arg(mapSymbolsLayerPair.key()));

        // Process symbols-on-path (SOPs) to get visible subpaths from them
        QList< RenderableSymbolOnPathEntry > visibleSOPSubpaths;
        visibleSOPSubpaths.reserve(mapSymbolsLayer.size());
        for(const auto& symbolEntry : rangeOf(constOf(mapSymbolsLayer)))
        {
            const auto symbol = std::dynamic_pointer_cast<const OnPathMapSymbol>(symbolEntry.key());
            if (!symbol)
                continue;
            const auto& points31 = symbol->path;
            assert(points31.size() >= 2);

            // Check first point to initialize subdivision
            auto pPoint31 = points31.constData();
            auto wasInside = internalState.frustum2D31.test(*(pPoint31++) - currentState.target31);
            int subpathStartIdx = -1;
            int subpathEndIdx = -1;
            if (wasInside)
            {
                subpathStartIdx = 0;
                subpathEndIdx = 0;
            }

            // Process rest of points one by one
            for(int pointIdx = 1, pointsCount = points31.size(); pointIdx < pointsCount; pointIdx++)
            {
                auto isInside = internalState.frustum2D31.test(*(pPoint31++) - currentState.target31);
                bool currentWasAdded = false;
                if (wasInside && !isInside)
                {
                    subpathEndIdx = pointIdx;
                    currentWasAdded = true;
                }
                else if (wasInside && isInside)
                {
                    subpathEndIdx = pointIdx;
                    currentWasAdded = true;
                }
                else if (!wasInside && isInside)
                {
                    subpathStartIdx = pointIdx - 1;
                    subpathEndIdx = pointIdx;
                    currentWasAdded = true;
                }

                if ((wasInside && !isInside) || (pointIdx == pointsCount - 1 && subpathStartIdx >= 0))
                {
                    if (!currentWasAdded)
                        subpathEndIdx = pointIdx;

                    std::shared_ptr<RenderableSymbolOnPath> renderable(new RenderableSymbolOnPath());
                    renderable->mapSymbol = symbol;
                    renderable->gpuResource = symbolEntry.value().lock();
                    renderable->subpathStartIndex = subpathStartIdx;
                    renderable->subpathEndIndex = subpathEndIdx;
                    visibleSOPSubpaths.push_back(qMove(renderable));

                    subpathStartIdx = -1;
                    subpathEndIdx = -1;
                }

                wasInside = isInside;
            }
        }

        // For each subpath, calculate it's points in world. That's needed for both 2D and 3D SOPs
        for(auto& renderable : visibleSOPSubpaths)
        {
            const auto& points = std::static_pointer_cast<const OnPathMapSymbol>(renderable->mapSymbol)->path;
            const auto subpathLength = renderable->subpathEndIndex - renderable->subpathStartIndex + 1;
            renderable->subpathPointsInWorld.resize(subpathLength);
            auto pPointInWorld = renderable->subpathPointsInWorld.data();
            const auto& pointInWorld0 = *(pPointInWorld++) =
                Utilities::convert31toFloat(points[renderable->subpathStartIndex] - currentState.target31, currentState.zoomBase) * AtlasMapRenderer::TileSize3D;
            for(int idx = renderable->subpathStartIndex + 1, endIdx = renderable->subpathEndIndex; idx <= endIdx; idx++, pPointInWorld++)
                *pPointInWorld = Utilities::convert31toFloat(points[idx] - currentState.target31, currentState.zoomBase) * AtlasMapRenderer::TileSize3D;
            renderable->subpathEndIndex -= renderable->subpathStartIndex;
            renderable->subpathStartIndex = 0;
        }

        // For each subpath, determine if it will be rendered in 2D or 3D.
        //NOTE: This is not quite correct, since it checks entire subpath instead of just occupied part.
        for(auto& renderable : visibleSOPSubpaths)
        {
            const auto& pointsInWorld = renderable->subpathPointsInWorld;

            // Calculate 'incline' of each part of path segment and compare to horizontal direction.
            // If any 'incline' is larger than 15 degrees, this segment is rendered in the map plane.
            renderable->is2D = true;
            const auto inclineThresholdSinSq = 0.0669872981f; // qSin(qDegreesToRadians(15.0f))*qSin(qDegreesToRadians(15.0f))
            auto pPointInWorld = pointsInWorld.constData();
            const auto& pointInWorld0 = *(pPointInWorld++);
            QVector<glm::vec2> pointsOnScreen(pointsInWorld.size());
            auto pPointOnScreen = pointsOnScreen.data();
            auto prevPointOnScreen = *(pPointOnScreen++) = glm::project(
                glm::vec3(pointInWorld0.x, 0.0f, pointInWorld0.y),
                internalState.mCameraView,
                internalState.mPerspectiveProjection,
                viewport).xy;
            for(auto idx = 1, count = pointsInWorld.size(); idx < count; idx++, pPointInWorld++)
            {
                const auto& pointOnScreen = *(pPointOnScreen++) = glm::project(
                    glm::vec3(pPointInWorld->x, 0.0f, pPointInWorld->y),
                    internalState.mCameraView,
                    internalState.mPerspectiveProjection,
                    viewport).xy;

                const auto vSegment = pointOnScreen - prevPointOnScreen;
                const auto d = vSegment.y;// horizont.x*vSegment.y - horizont.y*vSegment.x == 1.0f*vSegment.y - 0.0f*vSegment.x
                const auto inclineSinSq = d*d / (vSegment.x*vSegment.x + vSegment.y*vSegment.y);
                if (qAbs(inclineSinSq) > inclineThresholdSinSq)
                {
                    renderable->is2D = false;
                    break;
                }

                prevPointOnScreen = pointOnScreen;
            }

            if (renderable->is2D)
            {
                // In case SOP needs 2D mode, all points have been projected on the screen already
                renderable->subpathPointsOnScreen = qMove(pointsOnScreen);
            }
            else
            {
                //NOTE: Well, actually nothing to do here.
            }
        }

        // Adjust SOPs placement on path
        //TODO: improve significantly to place as much SOPs as possible by moving them around. Currently it just centers SOP on path
        QMutableListIterator<RenderableSymbolOnPathEntry> itRenderableSOP(visibleSOPSubpaths);
        while(itRenderableSOP.hasNext())
        {
            const auto& renderable = itRenderableSOP.next();
            const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
            const auto& is2D = renderable->is2D;

            const auto& points = is2D ? renderable->subpathPointsOnScreen : renderable->subpathPointsInWorld;
            //NOTE: Original algorithm for 3D SOPs contained a top-down projection that didn't include camera elevation angle. But this should give same results.
            const auto scale = is2D ? 1.0f : (static_cast<float>(AtlasMapRenderer::TileSize3D) / (internalState.screenTileSize*internalState.tileScaleFactor));
            const auto symbolWidth = gpuResource->width*scale;
            auto& length = renderable->subpathLength;
            auto& segmentLengths = renderable->segmentLengths;
            auto& offset = renderable->offset;

            // Calculate offset for 'center' alignment and check if symbol can fit given path
            auto pPrevPoint = points.constData();
            auto pPoint = pPrevPoint + 1;
            length = 0.0f;
            const auto& pointsCount = points.size();
            segmentLengths.resize(pointsCount - 1);
            auto pSegmentLength = segmentLengths.data();
            for(auto idx = 1; idx < pointsCount; idx++, pPoint++, pPrevPoint++)
            {
                const auto& distance = glm::distance(*pPoint, *pPrevPoint);
                *(pSegmentLength++) = distance;
                length += distance;
            }
            if (length < symbolWidth)
            {
#if OSMAND_DEBUG && 0
                {
                    QVector< glm::vec3 > debugPoints;
                    for(const auto& pointInWorld : renderable->subpathPointsInWorld)
                    {
                        debugPoints.push_back(qMove(glm::vec3(
                            pointInWorld.x,
                            0.0f,
                            pointInWorld.y)));
                    }
                    getRenderer()->_debugStage.addLine3D(debugPoints, SkColorSetA(is2D ? SK_ColorYELLOW : SK_ColorBLUE, 128));
                }
#endif // OSMAND_DEBUG

                // If length of path is not enough to contain entire symbol, remove this subpath entirely
                itRenderableSOP.remove();
                continue;
            }
            offset = (length - symbolWidth) / 2.0f;

            // Adjust subpath start index to cut off unused segments
            auto& startIndex = renderable->subpathStartIndex;
            float lengthFromStart = 0.0f;
            float prevLengthFromStart = 0.0f;
            pSegmentLength = segmentLengths.data();
            while(lengthFromStart <= offset)
            {
                prevLengthFromStart = lengthFromStart;
                lengthFromStart += *(pSegmentLength++);
                startIndex++;
            }
            startIndex--;
            assert(startIndex >= 0);

            // Adjust subpath end index to cut off unused segments
            auto& endIndex = renderable->subpathEndIndex;
            endIndex = startIndex + 1;
            while(lengthFromStart <= offset + symbolWidth)
            {
                lengthFromStart += *(pSegmentLength++);
                endIndex++;
            }
            assert(endIndex < pointsCount);
            assert(endIndex - startIndex > 0);

            // Adjust offset to reflect the changed
            offset -= prevLengthFromStart;

            // Calculate direction of used subpath
            glm::vec2 subpathDirection;
            pPrevPoint = points.data() + startIndex;
            pPoint = pPrevPoint + 1;
            for(auto idx = startIndex + 1; idx <= endIndex; idx++, pPrevPoint++, pPoint++)
                subpathDirection += (*pPoint - *pPrevPoint);

            if (is2D)
                renderable->subpathDirectionOnScreen = glm::normalize(subpathDirection);
            else
            {
                // For 3D SOPs direction is still threated as direction on screen
                const auto& p0 = points.first();
                const auto p1 = p0 + subpathDirection;
                const auto& projectedP0 = glm::project(glm::vec3(p0.x, 0.0f, p0.y), internalState.mCameraView, internalState.mPerspectiveProjection, viewport);
                const auto& projectedP1 = glm::project(glm::vec3(p1.x, 0.0f, p1.y), internalState.mCameraView, internalState.mPerspectiveProjection, viewport);
                renderable->subpathDirectioInWorld = glm::normalize(p1 - p0);
                renderable->subpathDirectionOnScreen = glm::normalize(projectedP1.xy - projectedP0.xy);
            }
        }

        QMultiMap< float, RenderableSymbolEntry > sortedRenderables;

        // Sort visible SOPs by distance to camera
        for(auto& renderable : visibleSOPSubpaths)
        {
            float maxDistanceToCamera = 0.0f;
            for(const auto& pointInWorld : constOf(renderable->subpathPointsInWorld))
            {
                const auto& distance = glm::distance(internalState.worldCameraPosition, glm::vec3(pointInWorld.x, 0.0f, pointInWorld.y));
                if (distance > maxDistanceToCamera)
                    maxDistanceToCamera = distance;
            }

            // Insert into map
            sortedRenderables.insert(maxDistanceToCamera, qMove(renderable));
        }
        visibleSOPSubpaths.clear();

        // Sort pinned symbols by distance to camera
        for(const auto& symbolEntry : rangeOf(constOf(mapSymbolsLayer)))
        {
            const auto& symbol = std::dynamic_pointer_cast<const PinnedMapSymbol>(symbolEntry.key());
            if (!symbol)
                continue;
            assert(!symbolEntry.value().expired());

            std::shared_ptr<RenderablePinnedSymbol> renderable(new RenderablePinnedSymbol());
            renderable->mapSymbol = symbol;
            renderable->gpuResource = symbolEntry.value().lock();

            // Calculate location of symbol in world coordinates.
            renderable->offsetFromTarget31 = symbol->location31 - currentState.target31;
            renderable->offsetFromTarget = Utilities::convert31toFloat(renderable->offsetFromTarget31, currentState.zoomBase);
            renderable->positionInWorld = glm::vec3(
                renderable->offsetFromTarget.x * AtlasMapRenderer::TileSize3D,
                0.0f,
                renderable->offsetFromTarget.y * AtlasMapRenderer::TileSize3D);

            // Get distance from symbol to camera
            const auto distance = glm::distance(internalState.worldCameraPosition, renderable->positionInWorld);

            // Insert into map
            sortedRenderables.insert(distance, qMove(renderable));
        }

        // Render symbols in reversed order, since sortedSymbols contains symbols by distance from camera from near->far.
        // And rendering needs to be done far->near
        QMapIterator< float, RenderableSymbolEntry > itRenderableEntry(sortedRenderables);
        itRenderableEntry.toBack();
        while(itRenderableEntry.hasPrevious())
        {
            const auto& item = itRenderableEntry.previous();
            const auto& distanceFromCamera = item.key();

            if (const auto& renderable = std::dynamic_pointer_cast<const RenderablePinnedSymbol>(item.value()))
            {
                const auto& symbol = std::dynamic_pointer_cast<const PinnedMapSymbol>(renderable->mapSymbol);
                const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
                const auto& symbolGroupPtr = symbol->groupPtr;

                // Calculate position in screen coordinates (same calculation as done in shader)
                const auto symbolOnScreen = glm::project(renderable->positionInWorld, internalState.mCameraView, internalState.mPerspectiveProjection, viewport);

                // Get bounds in screen coordinates
                auto boundsInWindow = AreaI::fromCenterAndSize(
                    static_cast<int>(symbolOnScreen.x + symbol->offset.x), static_cast<int>((currentState.windowSize.y - symbolOnScreen.y) + symbol->offset.y),
                    gpuResource->width, gpuResource->height);
                //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
                boundsInWindow.enlargeBy(PointI(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

#if !OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK
                // Check intersections
                const auto intersects = intersections.test(boundsInWindow, false,
                    [symbolGroupPtr]
                    (const std::shared_ptr<const MapSymbol>& otherSymbol, const IntersectionsQuadTree::BBox& otherBBox) -> bool
                    {
                        // Only accept intersections with symbols from other groups
                        return otherSymbol->groupPtr != symbolGroupPtr;
                    });
                if (intersects)
                {
#if OSMAND_DEBUG && 0
                    getRenderer()->_debugStage.addRect2D(boundsInWindow, SkColorSetA(SK_ColorRED, 50));
#endif // OSMAND_DEBUG
                    continue;
                }
#endif // !OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK

                // Query for similar content in area of "minDistance" to exclude duplicates, but keep if from same mapObject
                if (symbol->minDistance.x > 0 || symbol->minDistance.y > 0)
                {
                    const auto& symbolContent = symbol->content;
                    const auto hasSimilarContent = intersections.test(boundsInWindow.getEnlargedBy(symbol->minDistance), false,
                        [symbolContent, symbolGroupPtr]
                        (const std::shared_ptr<const MapSymbol>& otherSymbol, const IntersectionsQuadTree::BBox& otherBBox) -> bool
                        {
                            return
                                (otherSymbol->groupPtr != symbolGroupPtr) &&
                                (otherSymbol->content == symbolContent);
                        });
                    if (hasSimilarContent)
                    {
#if OSMAND_DEBUG && 0
                        getRenderer()->_debugStage.addRect2D(boundsInWindow.getEnlargedBy(symbol->minDistance), SkColorSetA(SK_ColorRED, 50));
                        getRenderer()->_debugStage.addRect2D(boundsInWindow, SkColorSetA(SK_ColorRED, 128));
#endif // OSMAND_DEBUG
                        continue;
                    }
                }

                // Insert into quad-tree
                if (!intersections.insert(symbol, boundsInWindow))
                {
#if OSMAND_DEBUG && 0
                    getRenderer()->_debugStage.addRect2D(boundsInWindow, SkColorSetA(SK_ColorBLUE, 50));
#endif // OSMAND_DEBUG
                    continue;
                }

#if OSMAND_DEBUG && 0
                getRenderer()->_debugStage.addRect2D(boundsInWindow, SkColorSetA(SK_ColorGREEN, 50));
#endif // OSMAND_DEBUG

                // Check if correct program is being used
                if (lastUsedProgram != *_pinnedSymbolProgram.id)
                {
                    GL_PUSH_GROUP_MARKER("use 'pinned-symbol' program");

                    // Set symbol VAO
                    gpuAPI->glBindVertexArray_wrapper(_pinnedSymbolVAO);
                    GL_CHECK_RESULT;

                    // Activate program
                    glUseProgram(_pinnedSymbolProgram.id);
                    GL_CHECK_RESULT;

                    // Set perspective projection-view matrix
                    glUniformMatrix4fv(_pinnedSymbolProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(mPerspectiveProjectionView));
                    GL_CHECK_RESULT;

                    // Set orthographic projection matrix
                    glUniformMatrix4fv(_pinnedSymbolProgram.vs.param.mOrthographicProjection, 1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
                    GL_CHECK_RESULT;

                    // Set viewport
                    glUniform4fv(_pinnedSymbolProgram.vs.param.viewport, 1, glm::value_ptr(viewport));
                    GL_CHECK_RESULT;

                    // Activate texture block for symbol textures
                    glActiveTexture(GL_TEXTURE0 + 0);
                    GL_CHECK_RESULT;

                    // Set proper sampler for texture block
                    gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + 0, GPUAPI_OpenGL::SamplerType::Symbol);

                    lastUsedProgram = _pinnedSymbolProgram.id;

                    GL_POP_GROUP_MARKER;
                }

                GL_PUSH_GROUP_MARKER(QString("[%1(%2) pinned \"%3\"]")
                    .arg(QString().sprintf("0x%p", symbol->groupPtr))
                    .arg(symbol->group.lock()->getDebugTitle())
                    .arg(qPrintable(symbol->content)));

                // Set symbol offset from target
                glUniform2f(_pinnedSymbolProgram.vs.param.symbolOffsetFromTarget, renderable->offsetFromTarget.x, renderable->offsetFromTarget.y);
                GL_CHECK_RESULT;

                // Set symbol size
                glUniform2i(_pinnedSymbolProgram.vs.param.symbolSize, gpuResource->width, gpuResource->height);
                GL_CHECK_RESULT;

                // Set distance from camera to symbol
                glUniform1f(_pinnedSymbolProgram.vs.param.distanceFromCamera, distanceFromCamera);
                GL_CHECK_RESULT;

                // Set on-screen offset
                glUniform2i(_pinnedSymbolProgram.vs.param.onScreenOffset, symbol->offset.x, -symbol->offset.y);
                GL_CHECK_RESULT;

                // Activate symbol texture
                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
                GL_CHECK_RESULT;

                // Apply settings from texture block to texture
                gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

                // Draw symbol actually
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
                GL_CHECK_RESULT;

                GL_POP_GROUP_MARKER;
            }
            else if (const auto& renderable = std::dynamic_pointer_cast<const RenderableSymbolOnPath>(item.value()))
            {
                const auto& symbol = std::dynamic_pointer_cast<const OnPathMapSymbol>(renderable->mapSymbol);
                const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
                const auto& symbolGroupPtr = symbol->groupPtr;

#if OSMAND_DEBUG && 0
                {
                    QVector< glm::vec3 > debugPoints;
                    for(const auto& pointInWorld : renderable->subpathPointsInWorld)
                    {
                        debugPoints.push_back(qMove(glm::vec3(
                            pointInWorld.x,
                            0.0f,
                            pointInWorld.y)));
                    }
                    getRenderer()->_debugStage.addLine3D(debugPoints, SkColorSetA(renderable->is2D ? SK_ColorGREEN : SK_ColorRED, 128));
                }
#endif // OSMAND_DEBUG

                const auto& is2D = renderable->is2D;
                const auto& points = is2D ? renderable->subpathPointsOnScreen : renderable->subpathPointsInWorld;
                //NOTE: Original algorithm for 3D SOPs contained a top-down projection that didn't include camera elevation angle. But this should give same results.
                const auto projectionScale = is2D ? 1.0f : (static_cast<float>(AtlasMapRenderer::TileSize3D) / (internalState.screenTileSize*internalState.tileScaleFactor));
                const auto& subpathDirectionOnScreen = renderable->subpathDirectionOnScreen;
                const glm::vec2 subpathDirectionOnScreenN(-subpathDirectionOnScreen.y, subpathDirectionOnScreen.x);
                const auto shouldInvert = (subpathDirectionOnScreenN.y /* horizont.x*dirN.y - horizont.y*dirN.x == 1.0f*dirN.y - 0.0f*dirN.x */) < 0;
                typedef std::tuple<glm::vec2, float, float, glm::vec2> GlyphLocation;
                QVector<GlyphLocation> glyphs;
                glyphs.reserve(symbol->glyphsWidth.size());

                auto nextSubpathPointIdx = renderable->subpathStartIndex;
                float lastSegmentLength = 0.0f;
                glm::vec2 vLastPoint0;
                glm::vec2 vLastPoint1;
                glm::vec2 vLastSegment;
                glm::vec2 vLastSegmentN;
                float lastSegmentAngle = 0.0f;
                float segmentsLengthSum = 0.0f;
                float prevOffset = renderable->offset;
                const auto glyphsCount = symbol->glyphsWidth.size();
                auto pGlyphWidth = symbol->glyphsWidth.constData();
                const auto glyphIterationDirection = shouldInvert ? -1 : +1;
                if (shouldInvert)
                {
                    // In case of direction inversion, start from last glyph
                    pGlyphWidth += (glyphsCount - 1);
                }
                for(int idx = 0; idx < glyphsCount; idx++, pGlyphWidth += glyphIterationDirection)
                {
                    // Get current glyph anchor offset and provide offset for next glyph
                    const auto& glyphWidth = *pGlyphWidth;
                    const auto glyphWidthScaled = glyphWidth*projectionScale;
                    const auto anchorOffset = prevOffset + glyphWidthScaled / 2.0f;
                    prevOffset += glyphWidthScaled;

                    // Get subpath segment, where anchor is located
                    while(segmentsLengthSum < anchorOffset)
                    {
                        const auto& p0 = points[nextSubpathPointIdx];
                        assert(nextSubpathPointIdx + 1 <= renderable->subpathEndIndex);
                        const auto& p1 = points[nextSubpathPointIdx + 1];
                        lastSegmentLength = renderable->segmentLengths[nextSubpathPointIdx];
                        segmentsLengthSum += lastSegmentLength;
                        nextSubpathPointIdx++;

                        vLastPoint0.x = p0.x;
                        vLastPoint0.y = p0.y;
                        vLastPoint1.x = p1.x;
                        vLastPoint1.y = p1.y;
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

                    // Add glyph location data
                    GlyphLocation glyphLocation(anchorPoint, glyphWidth, lastSegmentAngle, vLastSegmentN);
                    if (shouldInvert)
                        glyphs.push_front(qMove(glyphLocation));
                    else
                        glyphs.push_back(qMove(glyphLocation));
                }

                // Draw the glyphs
                if (renderable->is2D)
                {
                    // Calculate OOBB for 2D SOP
                    const auto directionAngle = qAtan2(renderable->subpathDirectionOnScreen.y, renderable->subpathDirectionOnScreen.x);
                    const auto negDirectionAngleCos = qCos(-directionAngle);
                    const auto negDirectionAngleSin = qSin(-directionAngle);
                    const auto directionAngleCos = qCos(directionAngle);
                    const auto directionAngleSin = qSin(directionAngle);
                    const auto halfGlyphHeight = gpuResource->height / 2.0f;
                    auto bboxInitialized = false;
                    AreaF bboxInDirection;
                    for(const auto& glyph : constOf(glyphs))
                    {
                        const auto& anchorPoint = std::get<0>(glyph);

                        const auto halfGlyphWidth = std::get<1>(glyph) / 2.0f;
                        const glm::vec2 glyphPoints[4] =
                        {
                            glm::vec2(-halfGlyphWidth, -halfGlyphHeight), // TL
                            glm::vec2( halfGlyphWidth, -halfGlyphHeight), // TR
                            glm::vec2( halfGlyphWidth,  halfGlyphHeight), // BR
                            glm::vec2(-halfGlyphWidth,  halfGlyphHeight)  // BL
                        };

                        const auto& segmentAngle = std::get<2>(glyph);
                        const auto segmentAngleCos = qCos(segmentAngle);
                        const auto segmentAngleSin = qSin(segmentAngle);

                        for(int idx = 0; idx < 4; idx++)
                        {
                            const auto& glyphPoint = glyphPoints[idx];

                            // Rotate to align with it's segment
                            glm::vec2 pointOnScreen;
                            pointOnScreen.x = glyphPoint.x*segmentAngleCos - glyphPoint.y*segmentAngleSin;
                            pointOnScreen.y = glyphPoint.x*segmentAngleSin + glyphPoint.y*segmentAngleCos;

                            // Add anchor point
                            pointOnScreen += anchorPoint;

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
                    OOBBF oobb(bboxInDirection, directionAngle);

                    //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
                    oobb.enlargeBy(PointI(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

#if !OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK
                    // Check intersections
                    const auto intersects = intersections.test(oobb, false,
                        [symbolGroupPtr]
                        (const std::shared_ptr<const MapSymbol>& otherSymbol, const IntersectionsQuadTree::BBox& otherBBox) -> bool
                        {
                            // Only accept intersections with symbols from other groups
                            return otherSymbol->groupPtr != symbolGroupPtr;
                        });
                    if (intersects)
                    {
#if OSMAND_DEBUG && 0
                        getRenderer()->_debugStage.addRect2D(oobb.unrotatedBBox, SkColorSetA(SK_ColorRED, 50), oobb.rotation);
#endif // OSMAND_DEBUG
                        continue;
                    }
#endif // !OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK

                    // Query for similar content in area of "minDistance" to exclude duplicates, but keep if from same mapObject
                    if (symbol->minDistance.x > 0 || symbol->minDistance.y > 0)
                    {
                        const auto& symbolContent = symbol->content;
                        const auto hasSimilarContent = intersections.test(oobb.getEnlargedBy(symbol->minDistance), false,
                            [symbolContent, symbolGroupPtr]
                            (const std::shared_ptr<const MapSymbol>& otherSymbol, const IntersectionsQuadTree::BBox& otherBBox) -> bool
                            {
                                return
                                    (otherSymbol->groupPtr != symbolGroupPtr) &&
                                    (otherSymbol->content == symbolContent);
                            });
                        if (hasSimilarContent)
                        {
#if OSMAND_DEBUG && 0
                            getRenderer()->_debugStage.addRect2D(oobb.getEnlargedBy(symbol->minDistance).unrotatedBBox, SkColorSetA(SK_ColorRED, 50), oobb.rotation);
                            getRenderer()->_debugStage.addRect2D(oobb.unrotatedBBox, SkColorSetA(SK_ColorRED, 128), oobb.rotation);
#endif // OSMAND_DEBUG
                            continue;
                        }
                    }

                    // Insert into quad-tree
                    if (!intersections.insert(symbol, oobb))
                    {
#if OSMAND_DEBUG && 0
                        getRenderer()->_debugStage.addRect2D(oobb.unrotatedBBox, SkColorSetA(SK_ColorBLUE, 50), oobb.rotation);
#endif // OSMAND_DEBUG
                        continue;
                    }

#if OSMAND_DEBUG && 0
                    getRenderer()->_debugStage.addRect2D(oobb.unrotatedBBox, SkColorSetA(SK_ColorGREEN, 50), oobb.rotation);
#endif // OSMAND_DEBUG

                    // Check if correct program is being used
                    if (lastUsedProgram != *_symbolOnPath2dProgram.id)
                    {
                        GL_PUSH_GROUP_MARKER("use 'symbol-on-path-2d' program");

                        // Set symbol VAO
                        gpuAPI->glBindVertexArray_wrapper(_symbolOnPath2dVAO);
                        GL_CHECK_RESULT;

                        // Activate program
                        glUseProgram(_symbolOnPath2dProgram.id);
                        GL_CHECK_RESULT;

                        // Set orthographic projection matrix
                        glUniformMatrix4fv(_symbolOnPath2dProgram.vs.param.mOrthographicProjection, 1, GL_FALSE, glm::value_ptr(internalState.mOrthographicProjection));
                        GL_CHECK_RESULT;

                        // Activate texture block for symbol textures
                        glActiveTexture(GL_TEXTURE0 + 0);
                        GL_CHECK_RESULT;

                        // Set proper sampler for texture block
                        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + 0, GPUAPI_OpenGL::SamplerType::Symbol);

                        lastUsedProgram = _symbolOnPath2dProgram.id;

                        GL_POP_GROUP_MARKER;
                    }

                    GL_PUSH_GROUP_MARKER(QString("[%1(%2) SOP-2D \"%3\"]")
                        .arg(QString().sprintf("0x%p", symbol->groupPtr))
                        .arg(symbol->group.lock()->getDebugTitle())
                        .arg(qPrintable(symbol->content)));

                    // Set glyph height
                    glUniform1f(_symbolOnPath2dProgram.vs.param.glyphHeight, gpuResource->height);
                    GL_CHECK_RESULT;

                    // Set distance from camera to symbol
                    glUniform1f(_symbolOnPath2dProgram.vs.param.distanceFromCamera, distanceFromCamera);
                    GL_CHECK_RESULT;

                    // Activate symbol texture
                    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
                    GL_CHECK_RESULT;

                    // Apply settings from texture block to texture
                    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

                    // Draw chains of glyphs
                    int glyphsDrawn = 0;
                    auto pGlyph = glyphs.constData();
                    auto pGlyphVS = _symbolOnPath2dProgram.vs.param.glyphs.constData();
                    float widthOfPreviousN = 0.0f;
                    while(glyphsDrawn < glyphsCount)
                    {
                        const auto glyphsToDraw = qMin(glyphsCount - glyphsDrawn, _maxGlyphsPerDrawCallSOP2D);

                        for(auto glyphIdx = 0; glyphIdx < glyphsToDraw; glyphIdx++)
                        {
                            const auto& glyph = *(pGlyph++);
                            const auto& vsGlyph = *(pGlyphVS++);

                            // Set anchor point of glyph
                            const auto& anchorPoint = std::get<0>(glyph);
                            glUniform2fv(vsGlyph.anchorPoint, 1, glm::value_ptr(anchorPoint));
                            GL_CHECK_RESULT;

                            // Set glyph width
                            const auto& width = std::get<1>(glyph);
                            glUniform1f(vsGlyph.width, width);
                            GL_CHECK_RESULT;

                            // Set angle
                            const auto& angle = std::get<2>(glyph);
                            glUniform1f(vsGlyph.angle, angle);
                            GL_CHECK_RESULT;

                            // Set normalized width of all previous glyphs
                            glUniform1f(vsGlyph.widthOfPreviousN, widthOfPreviousN);
                            GL_CHECK_RESULT;

                            // Set normalized width of this glyph
                            const auto widthN = width*gpuResource->uTexelSizeN;
                            glUniform1f(vsGlyph.widthN, widthN);
                            GL_CHECK_RESULT;
                            
                            widthOfPreviousN += widthN;
                        }

                        // Draw chain of glyphs actually
                        glDrawElements(GL_TRIANGLES, 6 * glyphsToDraw, GL_UNSIGNED_SHORT, nullptr);
                        GL_CHECK_RESULT;

                        glyphsDrawn += glyphsToDraw;
                    }
                    
                    GL_POP_GROUP_MARKER;

#if OSMAND_DEBUG && 0
                    for(const auto& glyph : constOf(glyphs))
                    {
                        const auto& anchorPoint = std::get<0>(glyph);
                        const auto& glyphWidth = std::get<1>(glyph);
                        const auto& angle = std::get<2>(glyph);
                        const auto& vN = std::get<3>(glyph);

                        getRenderer()->_debugStage.addRect2D(AreaF::fromCenterAndSize(
                            anchorPoint.x, currentState.windowSize.y - anchorPoint.y,
                            glyphWidth, gpuResource->height), SkColorSetA(SK_ColorGREEN, 128), angle);

                        QVector<glm::vec2> lineN;
                        const auto ln0 = anchorPoint;
                        lineN.push_back(glm::vec2(ln0.x, currentState.windowSize.y - ln0.y));
                        const auto ln1 = anchorPoint + (vN*16.0f);
                        lineN.push_back(glm::vec2(ln1.x, currentState.windowSize.y - ln1.y));
                        getRenderer()->_debugStage.addLine2D(lineN, SkColorSetA(shouldInvert ? SK_ColorMAGENTA : SK_ColorYELLOW, 128));
                    }

                    // Subpath N (start)
                    {
                        QVector<glm::vec2> lineN;
                        const auto sn0 = points[renderable->subpathStartIndex];
                        lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
                        const auto sn1 = points[renderable->subpathStartIndex] + (subpathDirectionOnScreenN*32.0f);
                        lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
                        getRenderer()->_debugStage.addLine2D(lineN, SkColorSetA(SK_ColorCYAN, 128));
                    }

                    // Subpath N (end)
                    {
                        QVector<glm::vec2> lineN;
                        const auto sn0 = points[renderable->subpathEndIndex];
                        lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
                        const auto sn1 = points[renderable->subpathEndIndex] + (subpathDirectionOnScreenN*32.0f);
                        lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
                        getRenderer()->_debugStage.addLine2D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
                    }
#endif // OSMAND_DEBUG
                }
                else
                {
                    // Calculate OOBB for 3D SOP in world
                    const auto directionAngleInWorld = qAtan2(renderable->subpathDirectioInWorld.y, renderable->subpathDirectioInWorld.x);
                    const auto negDirectionAngleInWorldCos = qCos(-directionAngleInWorld);
                    const auto negDirectionAngleInWorldSin = qSin(-directionAngleInWorld);
                    const auto directionAngleInWorldCos = qCos(directionAngleInWorld);
                    const auto directionAngleInWorldSin = qSin(directionAngleInWorld);
                    const auto halfGlyphHeight = (gpuResource->height / 2.0f) * projectionScale;
                    auto bboxInWorldInitialized = false;
                    AreaF bboxInWorldDirection;
                    for(const auto& glyph : constOf(glyphs))
                    {
                        const auto& anchorPoint = std::get<0>(glyph);

                        const auto halfGlyphWidth = (std::get<1>(glyph) / 2.0f) * projectionScale;
                        const glm::vec2 glyphPoints[4] =
                        {
                            glm::vec2(-halfGlyphWidth, -halfGlyphHeight), // TL
                            glm::vec2( halfGlyphWidth, -halfGlyphHeight), // TR
                            glm::vec2( halfGlyphWidth,  halfGlyphHeight), // BR
                            glm::vec2(-halfGlyphWidth,  halfGlyphHeight)  // BL
                        };

                        const auto& segmentAngle = std::get<2>(glyph);
                        const auto segmentAngleCos = qCos(segmentAngle);
                        const auto segmentAngleSin = qSin(segmentAngle);

                        for(int idx = 0; idx < 4; idx++)
                        {
                            const auto& glyphPoint = glyphPoints[idx];

                            // Rotate to align with it's segment
                            glm::vec2 pointInWorld;
                            pointInWorld.x = glyphPoint.x*segmentAngleCos - glyphPoint.y*segmentAngleSin;
                            pointInWorld.y = glyphPoint.x*segmentAngleSin + glyphPoint.y*segmentAngleCos;

                            // Add anchor point
                            pointInWorld += anchorPoint;

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
                        getRenderer()->_debugStage.addQuad3D((M*p0).xyz, (M*p1).xyz, (M*p2).xyz, (M*p3).xyz, SkColorSetA(SK_ColorGREEN, 50));
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
                        getRenderer()->_debugStage.addQuad3D(p0, p1, p2, p3, SkColorSetA(SK_ColorGREEN, 50));
                    }
#endif // OSMAND_DEBUG

                    // Project points of OOBB in world to screen
                    const PointF projectedRotatedBBoxInWorldP0(static_cast<glm::vec2>(
                        glm::project(glm::vec3(rotatedBBoxInWorld[0].x, 0.0f, rotatedBBoxInWorld[0].y),
                        internalState.mCameraView,
                        internalState.mPerspectiveProjection,
                        viewport).xy));
                    const PointF projectedRotatedBBoxInWorldP1(static_cast<glm::vec2>(
                        glm::project(glm::vec3(rotatedBBoxInWorld[1].x, 0.0f, rotatedBBoxInWorld[1].y),
                        internalState.mCameraView,
                        internalState.mPerspectiveProjection,
                        viewport).xy));
                    const PointF projectedRotatedBBoxInWorldP2(static_cast<glm::vec2>(
                        glm::project(glm::vec3(rotatedBBoxInWorld[2].x, 0.0f, rotatedBBoxInWorld[2].y),
                        internalState.mCameraView,
                        internalState.mPerspectiveProjection,
                        viewport).xy));
                    const PointF projectedRotatedBBoxInWorldP3(static_cast<glm::vec2>(
                        glm::project(glm::vec3(rotatedBBoxInWorld[3].x, 0.0f, rotatedBBoxInWorld[3].y),
                        internalState.mCameraView,
                        internalState.mPerspectiveProjection,
                        viewport).xy));
#if OSMAND_DEBUG && 0
                    {
                        QVector<glm::vec2> line;
                        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP0.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP0.y));
                        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP1.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP1.y));
                        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP2.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP2.y));
                        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP3.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP3.y));
                        line.push_back(glm::vec2(projectedRotatedBBoxInWorldP0.x, currentState.windowSize.y - projectedRotatedBBoxInWorldP0.y));
                        getRenderer()->_debugStage.addLine2D(line, SkColorSetA(SK_ColorRED, 50));
                    }
#endif // OSMAND_DEBUG

                    // Rotate using direction on screen
                    const auto directionAngle = qAtan2(renderable->subpathDirectionOnScreen.y, renderable->subpathDirectionOnScreen.x);
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
                    OOBBF oobb(bboxInDirection, directionAngle);

                    //TODO: use symbolExtraTopSpace & symbolExtraBottomSpace from font via Rasterizer_P
                    oobb.enlargeBy(PointI(3.0f*setupOptions.displayDensityFactor, 10.0f*setupOptions.displayDensityFactor)); /* 3dip; 10dip */

#if !OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK
                    // Check intersections
                    const auto intersects = intersections.test(oobb, false,
                        [symbolGroupPtr]
                        (const std::shared_ptr<const MapSymbol>& otherSymbol, const IntersectionsQuadTree::BBox& otherBBox) -> bool
                        {
                            // Only accept intersections with symbols from other groups
                            return otherSymbol->groupPtr != symbolGroupPtr;
                        });
                    if (intersects)
                    {
#if OSMAND_DEBUG && 0
                        getRenderer()->_debugStage.addRect2D(oobb.unrotatedBBox, SkColorSetA(SK_ColorRED, 50), oobb.rotation);
#endif // OSMAND_DEBUG
                        continue;
                    }
#endif // !OSMAND_SKIP_SYMBOLS_INTERSECTION_CHECK

                    // Query for similar content in area of "minDistance" to exclude duplicates, but keep if from same mapObject
                    if (symbol->minDistance.x > 0 || symbol->minDistance.y > 0)
                    {
                        const auto& symbolContent = symbol->content;
                        const auto hasSimilarContent = intersections.test(oobb.getEnlargedBy(symbol->minDistance), false,
                            [symbolContent, symbolGroupPtr]
                            (const std::shared_ptr<const MapSymbol>& otherSymbol, const IntersectionsQuadTree::BBox& otherBBox) -> bool
                            {
                                return
                                    (otherSymbol->groupPtr != symbolGroupPtr) &&
                                    (otherSymbol->content == symbolContent);
                            });
                        if (hasSimilarContent)
                        {
#if OSMAND_DEBUG && 0
                            getRenderer()->_debugStage.addRect2D(oobb.getEnlargedBy(symbol->minDistance).unrotatedBBox, SkColorSetA(SK_ColorRED, 50), oobb.rotation);
                            getRenderer()->_debugStage.addRect2D(oobb.unrotatedBBox, SkColorSetA(SK_ColorRED, 128), oobb.rotation);
#endif // OSMAND_DEBUG
                            continue;
                        }
                    }

                    // Insert into quad-tree
                    if (!intersections.insert(symbol, oobb))
                    {
#if OSMAND_DEBUG && 0
                        getRenderer()->_debugStage.addRect2D(oobb.unrotatedBBox, SkColorSetA(SK_ColorBLUE, 50), oobb.rotation);
#endif // OSMAND_DEBUG
                        continue;
                    }

#if OSMAND_DEBUG && 0
                    getRenderer()->_debugStage.addRect2D(oobb.unrotatedBBox, SkColorSetA(SK_ColorGREEN, 50), oobb.rotation);
#endif // OSMAND_DEBUG

                    // Check if correct program is being used
                    if (lastUsedProgram != *_symbolOnPath3dProgram.id)
                    {
                        GL_PUSH_GROUP_MARKER("use 'symbol-on-path-3d' program");

                        // Set symbol VAO
                        gpuAPI->glBindVertexArray_wrapper(_symbolOnPath3dVAO);
                        GL_CHECK_RESULT;

                        // Activate program
                        glUseProgram(_symbolOnPath3dProgram.id);
                        GL_CHECK_RESULT;

                        // Set view-projection matrix
                        const auto mViewProjection = internalState.mPerspectiveProjection * internalState.mCameraView;
                        glUniformMatrix4fv(_symbolOnPath3dProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(mViewProjection));
                        GL_CHECK_RESULT;

                        // Activate texture block for symbol textures
                        glActiveTexture(GL_TEXTURE0 + 0);
                        GL_CHECK_RESULT;

                        // Set proper sampler for texture block
                        gpuAPI->setTextureBlockSampler(GL_TEXTURE0 + 0, GPUAPI_OpenGL::SamplerType::Symbol);

                        lastUsedProgram = _symbolOnPath3dProgram.id;

                        GL_POP_GROUP_MARKER;
                    }

                    GL_PUSH_GROUP_MARKER(QString("[%1(%2) SOP-3D \"%3\"]")
                        .arg(QString().sprintf("0x%p", symbol->groupPtr))
                        .arg(symbol->group.lock()->getDebugTitle())
                        .arg(qPrintable(symbol->content)));

                    // Set glyph height
                    glUniform1f(_symbolOnPath3dProgram.vs.param.glyphHeight, gpuResource->height*projectionScale);
                    GL_CHECK_RESULT;

                    // Set distance from camera
                    const auto zDistanceFromCamera = (internalState.mOrthographicProjection * glm::vec4(0.0f, 0.0f, -distanceFromCamera, 1.0f)).z;
                    glUniform1f(_symbolOnPath3dProgram.vs.param.zDistanceFromCamera, zDistanceFromCamera);
                    GL_CHECK_RESULT;

                    // Activate symbol texture
                    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(gpuResource->refInGPU)));
                    GL_CHECK_RESULT;

                    // Apply settings from texture block to texture
                    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + 0);

                    // Draw chains of glyphs
                    int glyphsDrawn = 0;
                    auto pGlyph = glyphs.constData();
                    auto pGlyphVS = _symbolOnPath3dProgram.vs.param.glyphs.constData();
                    float widthOfPreviousN = 0.0f;
                    while(glyphsDrawn < glyphsCount)
                    {
                        const auto glyphsToDraw = qMin(glyphsCount - glyphsDrawn, _maxGlyphsPerDrawCallSOP3D);

                        for(auto glyphIdx = 0; glyphIdx < glyphsToDraw; glyphIdx++)
                        {
                            const auto& glyph = *(pGlyph++);
                            const auto& vsGlyph = *(pGlyphVS++);

                            // Set anchor point of glyph
                            const auto& anchorPoint = std::get<0>(glyph);
                            glUniform2fv(vsGlyph.anchorPoint, 1, glm::value_ptr(anchorPoint));
                            GL_CHECK_RESULT;

                            // Set glyph width
                            const auto& width = std::get<1>(glyph);
                            glUniform1f(vsGlyph.width, width*projectionScale);
                            GL_CHECK_RESULT;

                            // Set angle
                            const auto& angle = std::get<2>(glyph);
                            glUniform1f(vsGlyph.angle, Utilities::normalizedAngleRadians(angle + M_PI));
                            GL_CHECK_RESULT;

                            // Set normalized width of all previous glyphs
                            glUniform1f(vsGlyph.widthOfPreviousN, widthOfPreviousN);
                            GL_CHECK_RESULT;

                            // Set normalized width of this glyph
                            const auto widthN = width*gpuResource->uTexelSizeN;
                            glUniform1f(vsGlyph.widthN, widthN);
                            GL_CHECK_RESULT;

                            widthOfPreviousN += widthN;
                        }

                        // Draw chain of glyphs actually
                        glDrawElements(GL_TRIANGLES, 6 * glyphsToDraw, GL_UNSIGNED_SHORT, nullptr);
                        GL_CHECK_RESULT;

                        glyphsDrawn += glyphsToDraw;
                    }

                    GL_POP_GROUP_MARKER;

#if OSMAND_DEBUG && 0
                    for(const auto& glyphLocation : constOf(glyphs))
                    {
                        const auto& anchorPoint = std::get<0>(glyphLocation);
                        const auto& glyphWidth = std::get<1>(glyphLocation);
                        const auto& angle = std::get<2>(glyphLocation);
                        const auto& vN = std::get<3>(glyphLocation);

                        const auto& glyphInMapPlane = AreaF::fromCenterAndSize(
                            anchorPoint.x, anchorPoint.y, /* anchor points are specified in world coordinates already */
                            glyphWidth*projectionScale, gpuResource->height*projectionScale);
                        const auto& tl = glyphInMapPlane.topLeft;
                        const auto& tr = glyphInMapPlane.topRight();
                        const auto& br = glyphInMapPlane.bottomRight;
                        const auto& bl = glyphInMapPlane.bottomLeft();
                        const glm::vec3 pC(anchorPoint.x, 0.0f, anchorPoint.y);
                        const glm::vec4 p0(tl.x, 0.0f, tl.y, 1.0f);
                        const glm::vec4 p1(tr.x, 0.0f, tr.y, 1.0f);
                        const glm::vec4 p2(br.x, 0.0f, br.y, 1.0f);
                        const glm::vec4 p3(bl.x, 0.0f, bl.y, 1.0f);
                        const auto toCenter = glm::translate(-pC);
                        const auto rotate = glm::rotate(qRadiansToDegrees((float)Utilities::normalizedAngleRadians(angle + M_PI)), glm::vec3(0.0f, -1.0f, 0.0f));
                        const auto fromCenter = glm::translate(pC);
                        const auto M = fromCenter*rotate*toCenter;
                        getRenderer()->_debugStage.addQuad3D((M*p0).xyz, (M*p1).xyz, (M*p2).xyz, (M*p3).xyz, SkColorSetA(SK_ColorGREEN, 128));

                        QVector<glm::vec3> lineN;
                        const auto ln0 = anchorPoint;
                        lineN.push_back(glm::vec3(ln0.x, 0.0f, ln0.y));
                        const auto ln1 = anchorPoint + (vN*16.0f*projectionScale);
                        lineN.push_back(glm::vec3(ln1.x, 0.0f, ln1.y));
                        getRenderer()->_debugStage.addLine3D(lineN, SkColorSetA(shouldInvert ? SK_ColorMAGENTA : SK_ColorYELLOW, 128));
                    }

                    // Subpath N (start)
                    {
                        const auto a = points[renderable->subpathStartIndex];
                        const auto p0 = glm::project(glm::vec3(a.x, 0.0f, a.y), internalState.mCameraView, internalState.mPerspectiveProjection, viewport);

                        QVector<glm::vec2> lineN;
                        const glm::vec2 sn0 = p0.xy;
                        lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
                        const glm::vec2 sn1 = p0.xy + (subpathDirectionOnScreenN*32.0f);
                        lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
                        getRenderer()->_debugStage.addLine2D(lineN, SkColorSetA(SK_ColorCYAN, 128));
                    }

                    // Subpath N (end)
                    {
                        const auto a = points[renderable->subpathEndIndex];
                        const auto p0 = glm::project(glm::vec3(a.x, 0.0f, a.y), internalState.mCameraView, internalState.mPerspectiveProjection, viewport);

                        QVector<glm::vec2> lineN;
                        const glm::vec2 sn0 = p0.xy;
                        lineN.push_back(glm::vec2(sn0.x, currentState.windowSize.y - sn0.y));
                        const glm::vec2 sn1 = p0.xy + (subpathDirectionOnScreenN*32.0f);
                        lineN.push_back(glm::vec2(sn1.x, currentState.windowSize.y - sn1.y));
                        getRenderer()->_debugStage.addLine2D(lineN, SkColorSetA(SK_ColorMAGENTA, 128));
                    }
#endif // OSMAND_DEBUG
                }
            }
        }

        GL_POP_GROUP_MARKER;
    }

    // Deactivate any symbol texture
    glActiveTexture(GL_TEXTURE0 + 0);
    GL_CHECK_RESULT;
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;
    
    // Deactivate program
    glUseProgram(0);
    GL_CHECK_RESULT;

    // Deselect VAO
    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;

    GL_POP_GROUP_MARKER;
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::release()
{
    releasePinned();
    releaseOnPath();
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializePinned()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    // Compile vertex shader
    const QString vertexShader = QLatin1String(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoords;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
        "uniform mat4 param_vs_mOrthographicProjection;                                                                     ""\n"
        "uniform vec4 param_vs_viewport; // x, y, width, height                                                             ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform highp vec2 param_vs_symbolOffsetFromTarget;                                                                ""\n"
        "uniform ivec2 param_vs_symbolSize;                                                                                 ""\n"
        "uniform float param_vs_distanceFromCamera;                                                                         ""\n"
        "uniform ivec2 param_vs_onScreenOffset;                                                                             ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        // Calculate location of symbol in world coordinate system.
        "    vec4 symbolLocation;                                                                                           ""\n"
        "    symbolLocation.xz = param_vs_symbolOffsetFromTarget.xy * %TileSize3D%.0;                                       ""\n"
        "    symbolLocation.y = 0.0; //TODO: A height from heightmap should be used here                                    ""\n"
        "    symbolLocation.w = 1.0;                                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Project location of symbol from world coordinate system to screen
        "    vec4 projectedSymbolLocation = param_vs_mPerspectiveProjectionView * symbolLocation;                           ""\n"
        "                                                                                                                   ""\n"
        // "Normalize" location in screen coordinates to get [-1 .. 1] range
        "    vec3 symbolLocationOnScreen = projectedSymbolLocation.xyz / projectedSymbolLocation.w;                         ""\n"
        "                                                                                                                   ""\n"
        // Using viewport size, get real screen coordinates and correct depth to be [0 .. 1]
        "    symbolLocationOnScreen.xy = symbolLocationOnScreen.xy * 0.5 + 0.5;                                             ""\n"
        "    symbolLocationOnScreen.x = symbolLocationOnScreen.x * param_vs_viewport.z + param_vs_viewport.x;               ""\n"
        "    symbolLocationOnScreen.y = symbolLocationOnScreen.y * param_vs_viewport.w + param_vs_viewport.y;               ""\n"
        "    symbolLocationOnScreen.z = (1.0 + symbolLocationOnScreen.z) * 0.5;                                             ""\n"
        "                                                                                                                   ""\n"
        // Add on-screen offset
        "    symbolLocationOnScreen.xy += vec2(param_vs_onScreenOffset);                                                    ""\n"
        "                                                                                                                   ""\n"
        // symbolLocationOnScreen.xy now contains correct coordinates in viewport,
        // which can be used in orthographic projection (if it was configured to match viewport).
        //
        // So it's possible to calculate current vertex location:
        // Initially, get location of current vertex in screen coordinates
        "    vec2 vertexOnScreen;                                                                                           ""\n"
        "    vertexOnScreen.x = in_vs_vertexPosition.x * float(param_vs_symbolSize.x);                                      ""\n"
        "    vertexOnScreen.y = in_vs_vertexPosition.y * float(param_vs_symbolSize.y);                                      ""\n"
        "    vertexOnScreen = vertexOnScreen + symbolLocationOnScreen.xy;                                                   ""\n"
        "                                                                                                                   ""\n"
        // There's no need to perform unprojection into orthographic world space, just multiply these coordinates by
        // orthographic projection matrix (View and Model being identity)
        "  vec4 vertex;                                                                                                     ""\n"
        "  vertex.xy = vertexOnScreen.xy;                                                                                   ""\n"
        "  vertex.z = -param_vs_distanceFromCamera;                                                                         ""\n"
        "  vertex.w = 1.0;                                                                                                  ""\n"
        "  gl_Position = param_vs_mOrthographicProjection * vertex;                                                         ""\n"
        "                                                                                                                   ""\n"
        // Texture coordinates are simply forwarded from input
        "   v2f_texCoords = in_vs_vertexTexCoords;                                                                          ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%TileSize3D%", QString::number(AtlasMapRenderer::TileSize3D));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    assert(vsId != 0);

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec2 v2f_texCoords;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        // Parameters: per-symbol data
        "uniform lowp sampler2D param_fs_sampler;                                                                           ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = SAMPLE_TEXTURE_2D(                                                                     ""\n"
        "        param_fs_sampler,                                                                                          ""\n"
        "        v2f_texCoords);                                                                                            ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    assert(fsId != 0);

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    _pinnedSymbolProgram.id = gpuAPI->linkProgram(2, shaders);
    assert(_pinnedSymbolProgram.id);

    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_pinnedSymbolProgram.id);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GLShaderVariableType::In);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.param.mOrthographicProjection, "param_vs_mOrthographicProjection", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.param.viewport, "param_vs_viewport", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.param.symbolOffsetFromTarget, "param_vs_symbolOffsetFromTarget", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.param.symbolSize, "param_vs_symbolSize", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.param.distanceFromCamera, "param_vs_distanceFromCamera", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_pinnedSymbolProgram.vs.param.onScreenOffset, "param_vs_onScreenOffset", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_pinnedSymbolProgram.fs.param.sampler, "param_fs_sampler", GLShaderVariableType::Uniform);

#pragma pack(push, 1)
    struct Vertex
    {
        // XY coordinates. Z is assumed to be 0
        float positionXY[2];

        // UV coordinates
        float textureUV[2];
    };
#pragma pack(pop)

    // Vertex data
    Vertex vertices[4] =
    {
        // In OpenGL, UV origin is BL. But since same rule applies to uploading texture data,
        // texture in memory is vertically flipped, so swap bottom and top UVs
        { { -0.5f, -0.5f }, { 0.0f, 1.0f } },//BL
        { { -0.5f,  0.5f }, { 0.0f, 0.0f } },//TL
        { {  0.5f,  0.5f }, { 1.0f, 0.0f } },//TR
        { {  0.5f, -0.5f }, { 1.0f, 1.0f } } //BR
    };
    const auto verticesCount = 4;

    // Index data
    GLushort indices[6] =
    {
        0, 1, 2,
        0, 2, 3
    };
    const auto indicesCount = 6;

    // Create Vertex Array Object
    gpuAPI->glGenVertexArrays_wrapper(1, &_pinnedSymbolVAO);
    GL_CHECK_RESULT;
    gpuAPI->glBindVertexArray_wrapper(_pinnedSymbolVAO);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_pinnedSymbolVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _pinnedSymbolVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, verticesCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_pinnedSymbolProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_pinnedSymbolProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_pinnedSymbolProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_pinnedSymbolProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_pinnedSymbolIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _pinnedSymbolIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releasePinned()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_pinnedSymbolIBO)
    {
        glDeleteBuffers(1, &_pinnedSymbolIBO);
        GL_CHECK_RESULT;
        _pinnedSymbolIBO.reset();
    }
    if (_pinnedSymbolVBO)
    {
        glDeleteBuffers(1, &_pinnedSymbolVBO);
        GL_CHECK_RESULT;
        _pinnedSymbolVBO.reset();
    }
    if (_pinnedSymbolVAO)
    {
        gpuAPI->glDeleteVertexArrays_wrapper(1, &_pinnedSymbolVAO);
        GL_CHECK_RESULT;
        _pinnedSymbolVAO.reset();
    }

    if (_pinnedSymbolProgram.id)
    {
        glDeleteProgram(_pinnedSymbolProgram.id);
        GL_CHECK_RESULT;
        _pinnedSymbolProgram = PinnedSymbolProgram();
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath()
{
    initializeOnPath2D();
    initializeOnPath3D();
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath2D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    _maxGlyphsPerDrawCallSOP2D = (gpuAPI->maxVertexUniformVectors - 8) / 5;

    // Compile vertex shader
    const QString vertexShader = QLatin1String(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT float in_vs_glyphIndex;                                                                                       ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoords;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mOrthographicProjection;                                                                     ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform float param_vs_glyphHeight;                                                                                ""\n"
        "uniform float param_vs_distanceFromCamera;                                                                         ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-glyph data
        "struct Glyph                                                                                                       ""\n"
        "{                                                                                                                  ""\n"
        "    vec2 anchorPoint;                                                                                              ""\n"
        "    float width;                                                                                                   ""\n"
        "    float angle;                                                                                                   ""\n"
        "    float widthOfPreviousN;                                                                                        ""\n"
        "    float widthN;                                                                                                  ""\n"
        "};                                                                                                                 ""\n"
        "uniform Glyph param_vs_glyphs[%MaxGlyphsPerDrawCall%];                                                             ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    Glyph glyph = param_vs_glyphs[int(in_vs_glyphIndex)];                                                          ""\n"
        "                                                                                                                   ""\n"
        // Get on-screen vertex coordinates
        "    float cos_a = cos(glyph.angle);                                                                                ""\n"
        "    float sin_a = sin(glyph.angle);                                                                                ""\n"
        "    vec2 p;                                                                                                        ""\n"
        "    p.x = in_vs_vertexPosition.x * glyph.width;                                                                    ""\n"
        "    p.y = in_vs_vertexPosition.y * param_vs_glyphHeight;                                                           ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = glyph.anchorPoint.x + (p.x*cos_a - p.y*sin_a);                                                           ""\n"
        "    v.y = glyph.anchorPoint.y + (p.x*sin_a + p.y*cos_a);                                                           ""\n"
        "    v.z = -param_vs_distanceFromCamera;                                                                            ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "    gl_Position = param_vs_mOrthographicProjection * v;                                                            ""\n"
        "                                                                                                                   ""\n"
        // Prepare texture coordinates
        "    v2f_texCoords.s = glyph.widthOfPreviousN + in_vs_vertexTexCoords.s*glyph.widthN;                               ""\n"
        "    v2f_texCoords.t = in_vs_vertexTexCoords.t; // Height is compatible as-is                                       ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%MaxGlyphsPerDrawCall%", QString::number(_maxGlyphsPerDrawCallSOP2D));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    assert(vsId != 0);

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec2 v2f_texCoords;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        // Parameters: per-symbol data
        "uniform lowp sampler2D param_fs_sampler;                                                                           ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = SAMPLE_TEXTURE_2D(                                                                     ""\n"
        "        param_fs_sampler,                                                                                          ""\n"
        "        v2f_texCoords);                                                                                            ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    assert(fsId != 0);

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    _symbolOnPath2dProgram.id = gpuAPI->linkProgram(2, shaders);
    assert(_symbolOnPath2dProgram.id);

    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_symbolOnPath2dProgram.id);
    lookup->lookupLocation(_symbolOnPath2dProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    lookup->lookupLocation(_symbolOnPath2dProgram.vs.in.glyphIndex, "in_vs_glyphIndex", GLShaderVariableType::In);
    lookup->lookupLocation(_symbolOnPath2dProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GLShaderVariableType::In);
    lookup->lookupLocation(_symbolOnPath2dProgram.vs.param.mOrthographicProjection, "param_vs_mOrthographicProjection", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_symbolOnPath2dProgram.vs.param.glyphHeight, "param_vs_glyphHeight", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_symbolOnPath2dProgram.vs.param.distanceFromCamera, "param_vs_distanceFromCamera", GLShaderVariableType::Uniform);
    auto& glyphs = _symbolOnPath2dProgram.vs.param.glyphs;
    glyphs.resize(_maxGlyphsPerDrawCallSOP2D);
    int glyphStructIndex = 0;
    for(auto& glyph : glyphs)
    {
        const auto glyphStructPrefix =
            QString::fromLatin1("param_vs_glyphs[%glyphIndex%]").replace(QLatin1String("%glyphIndex%"), QString::number(glyphStructIndex++));

        lookup->lookupLocation(glyph.anchorPoint, glyphStructPrefix + ".anchorPoint", GLShaderVariableType::Uniform);
        lookup->lookupLocation(glyph.width, glyphStructPrefix + ".width", GLShaderVariableType::Uniform);
        lookup->lookupLocation(glyph.angle, glyphStructPrefix + ".angle", GLShaderVariableType::Uniform);
        lookup->lookupLocation(glyph.widthOfPreviousN, glyphStructPrefix + ".widthOfPreviousN", GLShaderVariableType::Uniform);
        lookup->lookupLocation(glyph.widthN, glyphStructPrefix + ".widthN", GLShaderVariableType::Uniform);
    }
    lookup->lookupLocation(_symbolOnPath2dProgram.fs.param.sampler, "param_fs_sampler", GLShaderVariableType::Uniform);

#pragma pack(push, 1)
    struct Vertex
    {
        // XY coordinates
        float positionXY[2];

        // Index of glyph
        //NOTE: Here should be int to omit conversion float->int, but it's not supported in OpenGLES 2.0
        float glyphIndex;

        // UV coordinates
        float textureUV[2];
    };
#pragma pack(pop)

    // Vertex data
    const Vertex templateVertices[4] =
    {
        // In OpenGL, UV origin is BL. But since same rule applies to uploading texture data,
        // texture in memory is vertically flipped, so swap bottom and top UVs
        { { -0.5f, -0.5f }, 0, { 0.0f, 1.0f } },//BL
        { { -0.5f,  0.5f }, 0, { 0.0f, 0.0f } },//TL
        { {  0.5f,  0.5f }, 0, { 1.0f, 0.0f } },//TR
        { {  0.5f, -0.5f }, 0, { 1.0f, 1.0f } } //BR
    };
    QVector<Vertex> vertices(4 * _maxGlyphsPerDrawCallSOP2D);
    auto pVertex = vertices.data();
    for(int glyphIdx = 0; glyphIdx < _maxGlyphsPerDrawCallSOP2D; glyphIdx++)
    {
        auto& p0 = *(pVertex++);
        p0 = templateVertices[0];
        p0.glyphIndex = glyphIdx;

        auto& p1 = *(pVertex++);
        p1 = templateVertices[1];
        p1.glyphIndex = glyphIdx;

        auto& p2 = *(pVertex++);
        p2 = templateVertices[2];
        p2.glyphIndex = glyphIdx;

        auto& p3 = *(pVertex++);
        p3 = templateVertices[3];
        p3.glyphIndex = glyphIdx;
    }

    // Index data
    QVector<GLushort> indices(6 * _maxGlyphsPerDrawCallSOP2D);
    auto pIndex = indices.data();
    for(int glyphIdx = 0; glyphIdx < _maxGlyphsPerDrawCallSOP2D; glyphIdx++)
    {
        *(pIndex++) = glyphIdx * 4 + 0;
        *(pIndex++) = glyphIdx * 4 + 1;
        *(pIndex++) = glyphIdx * 4 + 2;

        *(pIndex++) = glyphIdx * 4 + 0;
        *(pIndex++) = glyphIdx * 4 + 2;
        *(pIndex++) = glyphIdx * 4 + 3;
    }

    // Create Vertex Array Object
    gpuAPI->glGenVertexArrays_wrapper(1, &_symbolOnPath2dVAO);
    GL_CHECK_RESULT;
    gpuAPI->glBindVertexArray_wrapper(_symbolOnPath2dVAO);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_symbolOnPath2dVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _symbolOnPath2dVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_symbolOnPath2dProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_symbolOnPath2dProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_symbolOnPath2dProgram.vs.in.glyphIndex);
    GL_CHECK_RESULT;
    //NOTE: Here should be glVertexAttribIPointer to omit conversion float->int, but it's not supported in OpenGLES 2.0
    glVertexAttribPointer(*_symbolOnPath2dProgram.vs.in.glyphIndex, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, glyphIndex)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_symbolOnPath2dProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_symbolOnPath2dProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_symbolOnPath2dIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _symbolOnPath2dIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLushort), indices.constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);

    _maxGlyphsPerDrawCallSOP3D = (gpuAPI->maxVertexUniformVectors - 8) / 5;

    // Compile vertex shader
    const QString vertexShader = QLatin1String(
        // Input data
        "INPUT vec2 in_vs_vertexPosition;                                                                                   ""\n"
        "INPUT float in_vs_glyphIndex;                                                                                       ""\n"
        "INPUT vec2 in_vs_vertexTexCoords;                                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Output data to next shader stages
        "PARAM_OUTPUT vec2 v2f_texCoords;                                                                                   ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        "uniform mat4 param_vs_mPerspectiveProjectionView;                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-symbol data
        "uniform float param_vs_glyphHeight;                                                                                ""\n"
        "uniform float param_vs_zDistanceFromCamera;                                                                        ""\n"
        "                                                                                                                   ""\n"
        // Parameters: per-glyph data
        "struct Glyph                                                                                                       ""\n"
        "{                                                                                                                  ""\n"
        "    vec2 anchorPoint;                                                                                              ""\n"
        "    float width;                                                                                                   ""\n"
        "    float angle;                                                                                                   ""\n"
        "    float widthOfPreviousN;                                                                                        ""\n"
        "    float widthN;                                                                                                  ""\n"
        "};                                                                                                                 ""\n"
        "uniform Glyph param_vs_glyphs[%MaxGlyphsPerDrawCall%];                                                             ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    Glyph glyph = param_vs_glyphs[int(in_vs_glyphIndex)];                                                          ""\n"
        "                                                                                                                   ""\n"
        // Get on-screen vertex coordinates
        "    float cos_a = cos(glyph.angle);                                                                                ""\n"
        "    float sin_a = sin(glyph.angle);                                                                                ""\n"
        "    vec2 p;                                                                                                        ""\n"
        "    p.x = in_vs_vertexPosition.x * glyph.width;                                                                    ""\n"
        "    p.y = in_vs_vertexPosition.y * param_vs_glyphHeight;                                                           ""\n"
        "    vec4 v;                                                                                                        ""\n"
        "    v.x = glyph.anchorPoint.x + (p.x*cos_a - p.y*sin_a);                                                           ""\n"
        "    v.y = 0.0;                                                                                                     ""\n"
        "    v.z = glyph.anchorPoint.y + (p.x*sin_a + p.y*cos_a);                                                           ""\n"
        "    v.w = 1.0;                                                                                                     ""\n"
        "    gl_Position = param_vs_mPerspectiveProjectionView * v;                                                         ""\n"
        "    gl_Position.z = param_vs_zDistanceFromCamera;                                                                  ""\n"
        "                                                                                                                   ""\n"
        // Prepare texture coordinates
        "    v2f_texCoords.s = glyph.widthOfPreviousN + in_vs_vertexTexCoords.s*glyph.widthN;                               ""\n"
        "    v2f_texCoords.t = in_vs_vertexTexCoords.t; // Height is compatible as-is                                       ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedVertexShader = vertexShader;
    preprocessedVertexShader.replace("%MaxGlyphsPerDrawCall%", QString::number(_maxGlyphsPerDrawCallSOP3D));
    gpuAPI->preprocessVertexShader(preprocessedVertexShader);
    gpuAPI->optimizeVertexShader(preprocessedVertexShader);
    const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
    assert(vsId != 0);

    // Compile fragment shader
    const QString fragmentShader = QLatin1String(
        // Input data
        "PARAM_INPUT vec2 v2f_texCoords;                                                                                    ""\n"
        "                                                                                                                   ""\n"
        // Parameters: common data
        // Parameters: per-symbol data
        "uniform lowp sampler2D param_fs_sampler;                                                                           ""\n"
        "                                                                                                                   ""\n"
        "void main()                                                                                                        ""\n"
        "{                                                                                                                  ""\n"
        "    FRAGMENT_COLOR_OUTPUT = SAMPLE_TEXTURE_2D(                                                                     ""\n"
        "        param_fs_sampler,                                                                                          ""\n"
        "        v2f_texCoords);                                                                                            ""\n"
        "}                                                                                                                  ""\n");
    auto preprocessedFragmentShader = fragmentShader;
    QString preprocessedFragmentShader_UnrolledPerLayerProcessingCode;
    gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
    gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);
    const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
    assert(fsId != 0);

    // Link everything into program object
    GLuint shaders[] = { vsId, fsId };
    _symbolOnPath3dProgram.id = gpuAPI->linkProgram(2, shaders);
    assert(_symbolOnPath3dProgram.id);

    const auto& lookup = gpuAPI->obtainVariablesLookupContext(_symbolOnPath3dProgram.id);
    lookup->lookupLocation(_symbolOnPath3dProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    lookup->lookupLocation(_symbolOnPath3dProgram.vs.in.glyphIndex, "in_vs_glyphIndex", GLShaderVariableType::In);
    lookup->lookupLocation(_symbolOnPath3dProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GLShaderVariableType::In);
    lookup->lookupLocation(_symbolOnPath3dProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_symbolOnPath3dProgram.vs.param.glyphHeight, "param_vs_glyphHeight", GLShaderVariableType::Uniform);
    lookup->lookupLocation(_symbolOnPath3dProgram.vs.param.zDistanceFromCamera, "param_vs_zDistanceFromCamera", GLShaderVariableType::Uniform);
    auto& glyphs = _symbolOnPath3dProgram.vs.param.glyphs;
    glyphs.resize(_maxGlyphsPerDrawCallSOP3D);
    int glyphStructIndex = 0;
    for(auto& glyph : glyphs)
    {
        const auto glyphStructPrefix =
            QString::fromLatin1("param_vs_glyphs[%glyphIndex%]").replace(QLatin1String("%glyphIndex%"), QString::number(glyphStructIndex++));

        lookup->lookupLocation(glyph.anchorPoint, glyphStructPrefix + ".anchorPoint", GLShaderVariableType::Uniform);
        lookup->lookupLocation(glyph.width, glyphStructPrefix + ".width", GLShaderVariableType::Uniform);
        lookup->lookupLocation(glyph.angle, glyphStructPrefix + ".angle", GLShaderVariableType::Uniform);
        lookup->lookupLocation(glyph.widthOfPreviousN, glyphStructPrefix + ".widthOfPreviousN", GLShaderVariableType::Uniform);
        lookup->lookupLocation(glyph.widthN, glyphStructPrefix + ".widthN", GLShaderVariableType::Uniform);
    }
    lookup->lookupLocation(_symbolOnPath3dProgram.fs.param.sampler, "param_fs_sampler", GLShaderVariableType::Uniform);

#pragma pack(push, 1)
    struct Vertex
    {
        // XY coordinates
        float positionXY[2];

        // Index of glyph
        //NOTE: Here should be int to omit conversion float->int, but it's not supported in OpenGLES 2.0
        float glyphIndex;

        // UV coordinates
        float textureUV[2];
    };
#pragma pack(pop)

    // Vertex data
    const Vertex templateVertices[4] =
    {
        // In OpenGL, UV origin is BL. But since same rule applies to uploading texture data,
        // texture in memory is vertically flipped, so swap bottom and top UVs
        { { -0.5f, -0.5f }, 0, { 1.0f - 0.0f, 1.0f } },//BL
        { { -0.5f,  0.5f }, 0, { 1.0f - 0.0f, 0.0f } },//TL
        { {  0.5f,  0.5f }, 0, { 1.0f - 1.0f, 0.0f } },//TR
        { {  0.5f, -0.5f }, 0, { 1.0f - 1.0f, 1.0f } } //BR
    };
    QVector<Vertex> vertices(4 * _maxGlyphsPerDrawCallSOP3D);
    auto pVertex = vertices.data();
    for(int glyphIdx = 0; glyphIdx < _maxGlyphsPerDrawCallSOP3D; glyphIdx++)
    {
        auto& p0 = *(pVertex++);
        p0 = templateVertices[0];
        p0.glyphIndex = glyphIdx;

        auto& p1 = *(pVertex++);
        p1 = templateVertices[1];
        p1.glyphIndex = glyphIdx;

        auto& p2 = *(pVertex++);
        p2 = templateVertices[2];
        p2.glyphIndex = glyphIdx;

        auto& p3 = *(pVertex++);
        p3 = templateVertices[3];
        p3.glyphIndex = glyphIdx;
    }

    // Index data
    QVector<GLushort> indices(6 * _maxGlyphsPerDrawCallSOP3D);
    auto pIndex = indices.data();
    for(int glyphIdx = 0; glyphIdx < _maxGlyphsPerDrawCallSOP3D; glyphIdx++)
    {
        *(pIndex++) = glyphIdx * 4 + 0;
        *(pIndex++) = glyphIdx * 4 + 1;
        *(pIndex++) = glyphIdx * 4 + 2;

        *(pIndex++) = glyphIdx * 4 + 0;
        *(pIndex++) = glyphIdx * 4 + 2;
        *(pIndex++) = glyphIdx * 4 + 3;
    }

    // Create Vertex Array Object
    gpuAPI->glGenVertexArrays_wrapper(1, &_symbolOnPath3dVAO);
    GL_CHECK_RESULT;
    gpuAPI->glBindVertexArray_wrapper(_symbolOnPath3dVAO);
    GL_CHECK_RESULT;

    // Create vertex buffer and associate it with VAO
    glGenBuffers(1, &_symbolOnPath3dVBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ARRAY_BUFFER, _symbolOnPath3dVBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_symbolOnPath3dProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_symbolOnPath3dProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_symbolOnPath3dProgram.vs.in.glyphIndex);
    GL_CHECK_RESULT;
    //NOTE: Here should be glVertexAttribIPointer to omit conversion float->int, but it's not supported in OpenGLES 2.0
    glVertexAttribPointer(*_symbolOnPath3dProgram.vs.in.glyphIndex, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, glyphIndex)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(*_symbolOnPath3dProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(*_symbolOnPath3dProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
    GL_CHECK_RESULT;

    // Create index buffer and associate it with VAO
    glGenBuffers(1, &_symbolOnPath3dIBO);
    GL_CHECK_RESULT;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _symbolOnPath3dIBO);
    GL_CHECK_RESULT;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLushort), indices.constData(), GL_STATIC_DRAW);
    GL_CHECK_RESULT;

    gpuAPI->glBindVertexArray_wrapper(0);
    GL_CHECK_RESULT;
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath()
{
    releaseOnPath3D();
    releaseOnPath2D();
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath2D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_symbolOnPath2dIBO)
    {
        glDeleteBuffers(1, &_symbolOnPath2dIBO);
        GL_CHECK_RESULT;
        _symbolOnPath2dIBO.reset();
    }
    if (_symbolOnPath2dVBO)
    {
        glDeleteBuffers(1, &_symbolOnPath2dVBO);
        GL_CHECK_RESULT;
        _symbolOnPath2dVBO.reset();
    }
    if (_symbolOnPath2dVAO)
    {
        gpuAPI->glDeleteVertexArrays_wrapper(1, &_symbolOnPath2dVAO);
        GL_CHECK_RESULT;
        _symbolOnPath2dVAO.reset();
    }

    if (_symbolOnPath2dProgram.id)
    {
        glDeleteProgram(_symbolOnPath2dProgram.id);
        GL_CHECK_RESULT;
        _symbolOnPath2dProgram = SymbolOnPath2dProgram();
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath3D()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glDeleteProgram);

    if (_symbolOnPath3dIBO)
    {
        glDeleteBuffers(1, &_symbolOnPath3dIBO);
        GL_CHECK_RESULT;
        _symbolOnPath3dIBO.reset();
    }
    if (_symbolOnPath3dVBO)
    {
        glDeleteBuffers(1, &_symbolOnPath3dVBO);
        GL_CHECK_RESULT;
        _symbolOnPath3dVBO.reset();
    }
    if (_symbolOnPath3dVAO)
    {
        gpuAPI->glDeleteVertexArrays_wrapper(1, &_symbolOnPath3dVAO);
        GL_CHECK_RESULT;
        _symbolOnPath3dVAO.reset();
    }

    if (_symbolOnPath3dProgram.id)
    {
        glDeleteProgram(_symbolOnPath3dProgram.id);
        GL_CHECK_RESULT;
        _symbolOnPath3dProgram = SymbolOnPath3dProgram();
    }
}

