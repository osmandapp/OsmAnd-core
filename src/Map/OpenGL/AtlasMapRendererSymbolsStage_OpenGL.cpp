#include "AtlasMapRendererSymbolsStage_OpenGL.h"

#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <SkColor.h>

#include "AtlasMapRenderer_OpenGL.h"
#include "IMapSymbolProvider.h"
#include "QuadTree.h"
#include "Utilities.h"

OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer)
    : AtlasMapRendererStage_OpenGL(renderer)
    , _pinnedSymbolVAO(0)
    , _pinnedSymbolVBO(0)
    , _pinnedSymbolIBO(0)
    , _symbolOnPathVAO(0)
    , _symbolOnPathVBO(0)
    , _symbolOnPathIBO(0)
{
    memset(&_pinnedSymbolProgram, 0, sizeof(_pinnedSymbolProgram));
    memset(&_symbolOnPathProgram, 0, sizeof(_symbolOnPathProgram));
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
        QVector<PointF> subpathPointsInWorld;
        float subpathLength;
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
    QuadTree< std::shared_ptr<const MapSymbol>, AreaI::CoordType > intersections(currentState.viewport, 8);

    // Iterate over symbols by "order" in ascending direction
    for(auto itMapSymbolsLayer = mapSymbolsByOrder.cbegin(), itEnd = mapSymbolsByOrder.cend(); itMapSymbolsLayer != itEnd; ++itMapSymbolsLayer)
    {
        // For each "order" value, obtain list of entries and sort them
        const auto& mapSymbolsLayer = *itMapSymbolsLayer;
        if(mapSymbolsLayer.isEmpty())
            continue;

        GL_PUSH_GROUP_MARKER(QString("order %1").arg(itMapSymbolsLayer.key()));

        // Process symbols-on-path (SOPs) to get visible subpaths from them
        QList< RenderableSymbolOnPathEntry > visibleSOPSubpaths;
        visibleSOPSubpaths.reserve(mapSymbolsLayer.size());
        for(auto itSymbolEntry = mapSymbolsLayer.cbegin(), itEnd = mapSymbolsLayer.cend(); itSymbolEntry != itEnd; ++itSymbolEntry)
        {
            const auto symbol = std::dynamic_pointer_cast<const MapSymbolOnPath>(itSymbolEntry.key());
            if(!symbol)
                continue;
            const auto& points31 = symbol->mapObject->points31;
            assert(points31.size() >= 2);

            // Check first point to initialize subdivision
            auto pPoint31 = points31.constData();
            auto wasInside = internalState.frustum2D31.test(*(pPoint31++) - currentState.target31);
            int subpathStartIdx = -1;
            int subpathEndIdx = -1;
            if(wasInside)
            {
                subpathStartIdx = 0;
                subpathEndIdx = 0;
            }

            // Process rest of points one by one
            for(int pointIdx = 1, pointsCount = points31.size(); pointIdx < pointsCount; pointIdx++)
            {
                auto isInside = internalState.frustum2D31.test(*(pPoint31++) - currentState.target31);
                bool currentWasAdded = false;
                if(wasInside && !isInside)
                {
                    subpathEndIdx = pointIdx;
                    currentWasAdded = true;
                }
                else if(wasInside && isInside)
                {
                    subpathEndIdx = pointIdx;
                    currentWasAdded = true;
                }
                else if(!wasInside && isInside)
                {
                    subpathStartIdx = pointIdx - 1;
                    subpathEndIdx = pointIdx;
                    currentWasAdded = true;
                }

                if((wasInside && !isInside) || (pointIdx == pointsCount - 1 && subpathStartIdx >= 0))
                {
                    if(!currentWasAdded)
                        subpathEndIdx = pointIdx;

                    std::shared_ptr<RenderableSymbolOnPath> renderable(new RenderableSymbolOnPath());
                    renderable->mapSymbol = symbol;
                    renderable->gpuResource = itSymbolEntry.value().lock();
                    renderable->subpathStartIndex = subpathStartIdx;
                    renderable->subpathEndIndex = subpathEndIdx;
                    visibleSOPSubpaths.push_back(qMove(renderable));

                    subpathStartIdx = -1;
                    subpathEndIdx = -1;
                }

                wasInside = isInside;
            }
        }

        // Remove all SOP subpaths that can not hold entire content by width.
        // This check is only useful for SOPs rendered in 3D mode
        QMutableListIterator<RenderableSymbolOnPathEntry> itSOPSubpathEntry(visibleSOPSubpaths);
        while(itSOPSubpathEntry.hasNext())
        {
            const auto& renderable = itSOPSubpathEntry.next();
            const auto& gpuResource = std::dynamic_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);

            const auto& points = renderable->mapSymbol->mapObject->points31;
            const auto subpathLength = renderable->subpathEndIndex - renderable->subpathStartIndex + 1;
            renderable->subpathPointsInWorld.resize(subpathLength);
            PointF* pPointInWorld = renderable->subpathPointsInWorld.data();
            const auto& pointInWorld0 = *(pPointInWorld++) =
                Utilities::convert31toFloat(points[renderable->subpathStartIndex] - currentState.target31, currentState.zoomBase) * AtlasMapRenderer::TileSize3D;
            glm::vec2 prevProjectedP = glm::project(
                glm::vec3(pointInWorld0.x, 0.0f, pointInWorld0.y),
                topDownCameraView,
                internalState.mPerspectiveProjection,
                viewport).xy;
            float length = 0.0f;
            for(int idx = renderable->subpathStartIndex + 1, endIdx = renderable->subpathEndIndex; idx <= endIdx; idx++, pPointInWorld++)
            {
                *pPointInWorld = Utilities::convert31toFloat(points[idx] - currentState.target31, currentState.zoomBase) * AtlasMapRenderer::TileSize3D;
                const glm::vec2& projectedP = glm::project(
                    glm::vec3(pPointInWorld->x, 0.0f, pPointInWorld->y),
                    topDownCameraView,
                    internalState.mPerspectiveProjection,
                    viewport).xy;
                length += glm::distance(prevProjectedP, projectedP);
                prevProjectedP = projectedP;
            }
            renderable->subpathEndIndex -= renderable->subpathStartIndex;
            renderable->subpathStartIndex = 0;

            // If projected length is not enough to hold entire texture width, skip it
            if(length < gpuResource->width)
            {
#if OSMAND_DEBUG && 0
                QVector< glm::vec3 > debugPoints;
                for(const auto& pointInWorld : renderable->subpathPointsInWorld)
                {
                    debugPoints.push_back(qMove(glm::vec3(
                        pointInWorld.x,
                        0.0f,
                        pointInWorld.y)));
                }
                addDebugLine3D(debugPoints, SkColorSetA(SK_ColorRED, 128));
#endif // OSMAND_DEBUG

                itSOPSubpathEntry.remove();
                continue;
            }
#if OSMAND_DEBUG && 0
            QVector< glm::vec3 > debugPoints;
            for(const auto& pointInWorld : renderable->subpathPointsInWorld)
            {
                debugPoints.push_back(qMove(glm::vec3(
                    pointInWorld.x,
                    0.0f,
                    pointInWorld.y)));
            }
            addDebugLine3D(debugPoints, SkColorSetA(SK_ColorGREEN, 128));
#endif // OSMAND_DEBUG
        }

        //TODO: for sure, gluing is needed often
        // - 3. ADJUST OFFSET TO INCLUDE AS MUCH NON-INTERSECTING
        //      calculate offsets for each subSOP, to ensure no intersection.
        //      [optional] Without this step there going to be intersections of street names with themselves
        //	  [shrink or move active zone]
        //    Victor: simply renders in the middle

        QMultiMap< float, RenderableSymbolEntry > sortedRenderables;

        // Sort visible SOPs by distance to camera
        for(auto& renderable : visibleSOPSubpaths)
        {
            float maxDistanceToCamera = 0.0f;
            for(const auto& pointInWorld : constOf(renderable->subpathPointsInWorld))
            {
                const auto& distance = glm::distance(internalState.worldCameraPosition, glm::vec3(pointInWorld.x, 0.0f, pointInWorld.y));
                if(distance > maxDistanceToCamera)
                    maxDistanceToCamera = distance;
            }

            // Insert into map
            sortedRenderables.insert(maxDistanceToCamera, qMove(renderable));
        }
        visibleSOPSubpaths.clear();

        // Sort pinned symbols by distance to camera
        for(auto itSymbolEntry = mapSymbolsLayer.cbegin(), itEnd = mapSymbolsLayer.cend(); itSymbolEntry != itEnd; ++itSymbolEntry)
        {
            const auto& symbol = std::dynamic_pointer_cast<const MapPinnedSymbol>(itSymbolEntry.key());
            if(!symbol)
                continue;
            assert(!itSymbolEntry.value().expired());

            std::shared_ptr<RenderablePinnedSymbol> renderable(new RenderablePinnedSymbol());
            renderable->mapSymbol = symbol;
            renderable->gpuResource = itSymbolEntry.value().lock();

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

            if(const auto& renderable = std::dynamic_pointer_cast<const RenderablePinnedSymbol>(item.value()))
            {
                const auto& symbol = std::dynamic_pointer_cast<const MapPinnedSymbol>(renderable->mapSymbol);
                const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);
                const auto& mapObjectId = symbol->mapObject->id;

                // Calculate position in screen coordinates (same calculation as done in shader)
                const auto symbolOnScreen = glm::project(renderable->positionInWorld, internalState.mCameraView, internalState.mPerspectiveProjection, viewport);

                // Check intersections
                const auto boundsInWindow = AreaI::fromCenterAndSize(
                    static_cast<int>(symbolOnScreen.x + symbol->offset.x), static_cast<int>((currentState.windowSize.y - symbolOnScreen.y) + symbol->offset.y),
                    gpuResource->width, gpuResource->height);//TODO: use MapSymbol bounds
                const auto intersects = intersections.test(boundsInWindow, false,
                    [mapObjectId](const std::shared_ptr<const MapSymbol>& otherSymbol, const AreaI& otherArea) -> bool
                {
                    return otherSymbol->mapObject->id != mapObjectId;
                });
                if(intersects || !intersections.insert(symbol, boundsInWindow))
                {
#if OSMAND_DEBUG && 0
                    getRenderer()->_debugStage.addRect2D(boundsInWindow, SkColorSetA(SK_ColorRED, 50));
#endif // OSMAND_DEBUG
                    continue;
                }

#if OSMAND_DEBUG && 0
                getRenderer()->_debugStage.addRect2D(boundsInWindow, SkColorSetA(SK_ColorGREEN, 50));
#endif // OSMAND_DEBUG

                // Check if correct program is being used
                if(lastUsedProgram != _pinnedSymbolProgram.id)
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

                GL_PUSH_GROUP_MARKER(QString("[MO %1(%2) \"%3\"]")
                    .arg(symbol->mapObject->id >> 1)
                    .arg(static_cast<int64_t>(symbol->mapObject->id) / 2)
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
            else if(const auto& renderable = std::dynamic_pointer_cast<const RenderableSymbolOnPath>(item.value()))
            {
                const auto& symbol = std::dynamic_pointer_cast<const MapSymbolOnPath>(renderable->mapSymbol);
                const auto& gpuResource = std::static_pointer_cast<const GPUAPI::TextureInGPU>(renderable->gpuResource);

                // Project all world points into the screen
                QVector<glm::vec2> pointsOnScreen(renderable->subpathPointsInWorld.size());
                auto* pPointOnScreen = pointsOnScreen.data();
                for(const auto& pointInWorld : constOf(renderable->subpathPointsInWorld))
                {
                    *(pPointOnScreen++) = glm::project(
                        glm::vec3(pointInWorld.x, 0.0f, pointInWorld.y),
                        internalState.mCameraView,
                        internalState.mPerspectiveProjection,
                        viewport).xy;
                }
                
                // Calculate 'incline' of each part of path segment and compare to horizontal direction.
                // If any 'incline' is larger than 15 degrees, this segment is rendered in the map plane.
                auto is2D = true;
                const auto inclineThresholdSinSq = 0.0669872981f; // qSin(qDegreesToRadians(15.0f))*qSin(qDegreesToRadians(15.0f))
                for(auto itPrevP = pointsOnScreen.cbegin(), itP = itPrevP + 1, itEnd = pointsOnScreen.cend(); itP != itEnd; itPrevP = itP, ++itP)
                {
                    const auto& prevP = *itPrevP;
                    const auto& p = *itP;
                
                    const auto vP = p - prevP;
                    const auto d = vP.y;// horizont.x*vP.y - horizont.y*vP.x == 1.0f*vP.y - 0.0f*vP.x
                    const auto inclineSinSq = d*d / (vP.x*vP.x + vP.y*vP.y);
                    if(qAbs(inclineSinSq) > inclineThresholdSinSq)
                    {
                        is2D = false;
                        break;
                    }
                }

#if OSMAND_DEBUG && 1
                {
                    QVector< glm::vec3 > debugPoints;
                    for(const auto& pointInWorld : renderable->subpathPointsInWorld)
                    {
                        debugPoints.push_back(qMove(glm::vec3(
                            pointInWorld.x,
                            0.0f,
                            pointInWorld.y)));
                    }
                    getRenderer()->_debugStage.addLine3D(debugPoints, SkColorSetA(is2D ? SK_ColorGREEN : SK_ColorRED, 128));
                }
#endif // OSMAND_DEBUG

                if(is2D)
                {
                    bool doesntFit = false;
                    //const auto& subpathPoints = renderable->subpathPointsInWorld;
                    auto nextSubpathPointIdx = renderable->subpathStartIndex;
                    float lastSegmentLength = 0.0f;
                    float segmentsLengthSum = 0.0f;
                    float prevOffset = 0.0f;
                    auto pGlyphWidth = symbol->glyphsWidth.constData();
                    for(int glyphIdx = 0, glyphsCount = symbol->glyphsWidth.size(); glyphIdx < glyphsCount; glyphIdx++, pGlyphWidth++)
                    {
                        // Get current glyph anchor offset and provide offset for next glyph
                        const auto& glyphWidth = *pGlyphWidth;
                        float anchorOffset = prevOffset + glyphWidth / 2.0f;
                        prevOffset += glyphWidth;

                        // Get subpath segment, where anchor is located
                        while(segmentsLengthSum < anchorOffset)
                        {
                            const auto& p0 = pointsOnScreen[nextSubpathPointIdx];
                            if(nextSubpathPointIdx + 1 > renderable->subpathEndIndex)
                            {
                                doesntFit = true;
                                break;
                            }
                            const auto& p1 = pointsOnScreen[nextSubpathPointIdx + 1];
                            lastSegmentLength = glm::distance(p1, p0);
                            segmentsLengthSum += lastSegmentLength;
                            nextSubpathPointIdx++;
                        }
                        if(doesntFit)
                            break;

                        // Calculate anchor point
                        const auto& p0 = pointsOnScreen[nextSubpathPointIdx - 1];
                        const auto& p1 = pointsOnScreen[nextSubpathPointIdx];
                        glm::vec2 vp0(p0.x, p0.y);
                        glm::vec2 vp1(p1.x, p1.y);
                        const auto vSegment = (vp1 - vp0) / lastSegmentLength;
                        const auto anchorPoint = vp0 + (anchorOffset - (segmentsLengthSum - lastSegmentLength))*vSegment;

                        //TODO: Calculate rotation angle via direction of segment perpendicular
                        glm::vec2 vSegmentN(-vSegment.y, vSegment.x);
                        vSegmentN /= lastSegmentLength;

                        // at this point there's anchor point location and direction of glyph, which is enough for passing to renderer
                        int i = 5;
#if OSMAND_DEBUG && 1
                        getRenderer()->_debugStage.addRect2D(AreaI::fromCenterAndSize(
                            anchorPoint.x, currentState.windowSize.y - anchorPoint.y,
                            glyphWidth, gpuResource->height), SkColorSetA(SK_ColorGREEN, 128));
#endif // OSMAND_DEBUG
                    }

                    // Do actual draw-call only if symbol fits
                    if(!doesntFit)
                    {
                        //TODO: do draw call
                    }
                }
                else
                {

                }
                //TODO: activate and render symbol!!!!
                int i = 5;
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
    assert(_pinnedSymbolProgram.id != 0);

    gpuAPI->clearVariablesLookup();
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.in.vertexPosition, "in_vs_vertexPosition", GLShaderVariableType::In);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.in.vertexTexCoords, "in_vs_vertexTexCoords", GLShaderVariableType::In);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.param.mOrthographicProjection, "param_vs_mOrthographicProjection", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.param.viewport, "param_vs_viewport", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.param.symbolOffsetFromTarget, "param_vs_symbolOffsetFromTarget", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.param.symbolSize, "param_vs_symbolSize", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.param.distanceFromCamera, "param_vs_distanceFromCamera", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.vs.param.onScreenOffset, "param_vs_onScreenOffset", GLShaderVariableType::Uniform);
    gpuAPI->findVariableLocation(_pinnedSymbolProgram.id, _pinnedSymbolProgram.fs.param.sampler, "param_fs_sampler", GLShaderVariableType::Uniform);
    gpuAPI->clearVariablesLookup();

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
    glEnableVertexAttribArray(_pinnedSymbolProgram.vs.in.vertexPosition);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_pinnedSymbolProgram.vs.in.vertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, positionXY)));
    GL_CHECK_RESULT;
    glEnableVertexAttribArray(_pinnedSymbolProgram.vs.in.vertexTexCoords);
    GL_CHECK_RESULT;
    glVertexAttribPointer(_pinnedSymbolProgram.vs.in.vertexTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, textureUV)));
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

    if(_pinnedSymbolIBO != 0)
    {
        glDeleteBuffers(1, &_pinnedSymbolIBO);
        GL_CHECK_RESULT;
        _pinnedSymbolIBO = 0;
    }
    if(_pinnedSymbolVBO != 0)
    {
        glDeleteBuffers(1, &_pinnedSymbolVBO);
        GL_CHECK_RESULT;
        _pinnedSymbolVBO = 0;
    }
    if(_pinnedSymbolVAO != 0)
    {
        gpuAPI->glDeleteVertexArrays_wrapper(1, &_pinnedSymbolVAO);
        GL_CHECK_RESULT;
        _pinnedSymbolVAO = 0;
    }

    if(_pinnedSymbolProgram.id)
    {
        glDeleteProgram(_pinnedSymbolProgram.id);
        GL_CHECK_RESULT;
        memset(&_pinnedSymbolProgram, 0, sizeof(_pinnedSymbolProgram));
    }
}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::initializeOnPath()
{

}

void OsmAnd::AtlasMapRendererSymbolsStage_OpenGL::releaseOnPath()
{

}

