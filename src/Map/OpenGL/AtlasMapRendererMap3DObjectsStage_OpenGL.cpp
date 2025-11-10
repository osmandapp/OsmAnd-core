#include "AtlasMapRendererMap3DObjectsStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "OpenGL/GPUAPI_OpenGL.h"
#include "AtlasMapRenderer_OpenGL.h"
#include "Utilities.h"
#include "MapRendererTiledResourcesCollection.h"
#include "MapRenderer3DObjects.h"
#include "MapRendererDebugSettings.h"
#include "Stopwatch.h"
#include "Logging.h"
#include <QSet>
#include <mapbox/earcut.hpp>
#include <cstdlib>

using namespace OsmAnd;

AtlasMapRendererMap3DObjectsStage_OpenGL::AtlasMapRendererMap3DObjectsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer)
    : AtlasMapRendererMap3DObjectsStage(renderer)
    , AtlasMapRendererStageHelper_OpenGL(this)
{
}

AtlasMapRendererMap3DObjectsStage_OpenGL::~AtlasMapRendererMap3DObjectsStage_OpenGL()
{
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initializeProgram()
{
    const auto nextInit3DobjectsType = static_cast<Init3DObjectsType>(static_cast<int>(_init3DObjectsType) + 1);
    _init3DObjectsType = Init3DObjectsType::Incomplete;

    const auto gpuAPI = getGPUAPI();
    
    QHash<QString, GPUAPI_OpenGL::GlslProgramVariable> variablesMap;
    _program.id = 0;

    if (!_program.binaryCache.isEmpty())
    {
        _program.id = gpuAPI->linkProgram(0, nullptr, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
    }

    if (!_program.id.isValid())
    {
        const QString vertexShader = R"(
            in ivec2 in_vs_location31;
            in float in_vs_height;
            in vec3 in_vs_normal;
            
            out vec4 v2f_color;
            
            uniform mat4 param_vs_mPerspectiveProjectionView;
            uniform float param_vs_metersPerUnit;
            uniform vec4 param_vs_color;
            uniform ivec2 param_vs_target31;
            uniform int param_vs_zoomLevel;
            uniform vec3 param_vs_lightDirection;
            uniform float param_vs_ambient;
            
            uniform highp sampler2D param_vs_elevation_dataSampler;
            uniform highp vec4 param_vs_elevationLayerDataPlace;
            uniform highp vec4 param_vs_elevationLayer_txOffsetAndScale;
            uniform vec4 param_vs_elevation_scale;
            uniform ivec2 param_vs_elevationTileCoords31;
            uniform int param_vs_elevationTileZoomLevel;
            
            const float TILE_SIZE_3D = 100.0;
            const int HEIXELS_PER_TILE_SIDE = 32;
            const int MAX_ZOOM_LEVEL = 31;
            const int MAX_TILE_NUMBER = (1 << MAX_ZOOM_LEVEL) - 1;
            const int MIDDLE_TILE_NUMBER = MAX_TILE_NUMBER / 2 + 1;

            vec2 convert31toFloat(ivec2 point31, int zoomLevel)
            {
                float tileSize31 = float(1 << (MAX_ZOOM_LEVEL - zoomLevel));
                return vec2(point31) / tileSize31;
            }

            ivec2 shortestVector31(ivec2 p0, ivec2 p1)
            {
                ivec2 offset = p1 - p0;
                ivec2 signOffset = sign(offset);
                ivec2 absOffset = abs(offset);
                
                ivec2 needsCorrection = ivec2(
                    absOffset.x >= MIDDLE_TILE_NUMBER ? 1 : 0,
                    absOffset.y >= MIDDLE_TILE_NUMBER ? 1 : 0
                );
                
                offset.x = offset.x - signOffset.x * needsCorrection.x * (MAX_TILE_NUMBER + 1);
                offset.y = offset.y - signOffset.y * needsCorrection.y * (MAX_TILE_NUMBER + 1);
                
                return offset;
            }

            vec3 planeWorldCoordinates(ivec2 location31, ivec2 target31, int zoomLevel, float elevation)
            {
                ivec2 offset31 = shortestVector31(target31, location31);
                vec2 offsetFromTarget = convert31toFloat(offset31, zoomLevel) * TILE_SIZE_3D;
                return vec3(offsetFromTarget.x, elevation, offsetFromTarget.y);
            }

            float interpolatedHeight(in vec2 inTexCoords)
            {
                vec2 heixelSize = param_vs_elevationLayerDataPlace.zw * 2.0;
                vec2 texCoords = (inTexCoords - param_vs_elevationLayerDataPlace.zw) / heixelSize;
                vec2 pixOffset = fract(texCoords);
                texCoords = floor(texCoords) * heixelSize + param_vs_elevationLayerDataPlace.zw;
                vec2 minCoords = param_vs_elevationLayerDataPlace.xy - heixelSize;
                vec2 maxCoords = minCoords + heixelSize * (float(HEIXELS_PER_TILE_SIDE) + 2.0);
                float blHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;
                texCoords.x += heixelSize.x;
                float brHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;
                texCoords.y += heixelSize.y;
                float trHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;
                texCoords.x -= heixelSize.x;
                float tlHeixel = SAMPLE_TEXTURE_2D(param_vs_elevation_dataSampler, clamp(texCoords, minCoords, maxCoords)).r;
                float avbPixel = mix(blHeixel, brHeixel, pixOffset.x);
                float avtPixel = mix(tlHeixel, trHeixel, pixOffset.x);
                return mix(avbPixel, avtPixel, pixOffset.y);
            }

            void main()
            {
                float baseElevation = in_vs_height / param_vs_metersPerUnit;
                float elevation = baseElevation;
                
                if (abs(param_vs_elevation_scale.w) > 0.0)
                {
                    ivec2 offset31 = shortestVector31(param_vs_elevationTileCoords31, in_vs_location31);
                    vec2 locationFloat = convert31toFloat(offset31, param_vs_elevationTileZoomLevel);
                    vec2 elevationTexCoords = locationFloat * param_vs_elevationLayer_txOffsetAndScale.zw + param_vs_elevationLayer_txOffsetAndScale.xy;
                    float elevationHeight = interpolatedHeight(elevationTexCoords);
                    float terrainElevation = (elevationHeight * param_vs_elevation_scale.w * param_vs_elevation_scale.z) / param_vs_metersPerUnit;
                    elevation = baseElevation + terrainElevation;
                }
                
                vec3 worldPos = planeWorldCoordinates(in_vs_location31, param_vs_target31, param_vs_zoomLevel, elevation);
                
                gl_Position = param_vs_mPerspectiveProjectionView * vec4(worldPos, 1.0);
                
                float ndotl = max(dot(in_vs_normal, param_vs_lightDirection), 0.0);
                float diffuse = param_vs_ambient + (1.0 - param_vs_ambient) * ndotl;
                v2f_color = vec4(param_vs_color.rgb * diffuse, param_vs_color.a);
            }
        )";

        const QString fragmentShader = R"(
            in vec4 v2f_color;
            
            void main()
            {
                FRAGMENT_COLOR_OUTPUT = v2f_color;
            }
        )";

        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        auto preprocessedFragmentShader = fragmentShader;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        _program.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _program.cacheFormat);

        if (!_program.binaryCache.isEmpty())
        {
            _program.id = gpuAPI->linkProgram(0, nullptr, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
        }
        if (_program.binaryCache.isEmpty() || !_program.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects vertex shader");
                return false;
            }

            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects fragment shader");
                return false;
            }

            const GLuint shaders[] = { vsId, fsId };
            _program.id = gpuAPI->linkProgram(2, shaders, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
            if (_program.id.isValid() && !_program.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _program.binaryCache,
                    _program.cacheFormat);
            }
        }
    }

    if (!_program.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link Map3DObjects shader program");
        return false;
    }

    const auto lookup = gpuAPI->obtainVariablesLookupContext(_program.id, variablesMap);
    bool ok = true;
    ok = ok && lookup->lookupLocation(_program.vs.in.location31, "in_vs_location31", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.height, "in_vs_height", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.normal, "in_vs_normal", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.color, "param_vs_color", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.zoomLevel, "param_vs_zoomLevel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.lightDirection, "param_vs_lightDirection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.ambient, "param_vs_ambient", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.elevation_dataSampler, "param_vs_elevation_dataSampler", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.elevationLayer.txOffsetAndScale, "param_vs_elevationLayer_txOffsetAndScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.elevationLayerDataPlace, "param_vs_elevationLayerDataPlace", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.elevation_scale, "param_vs_elevation_scale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.elevationTileCoords31, "param_vs_elevationTileCoords31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.elevationTileZoomLevel, "param_vs_elevationTileZoomLevel", GlslVariableType::Uniform);

    if (!ok)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;

        _program.id.reset();

        LogPrintf(LogSeverityLevel::Error,
            "Failed to find variable in Map3DObjects shader program");
        return false;
    }

    _init3DObjectsType = nextInit3DobjectsType;
    return true;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initialize()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glVertexAttribIPointer);
    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform4f);
    GL_CHECK_PRESENT(glUniform2i);
    GL_CHECK_PRESENT(glUniform1f);
    GL_CHECK_PRESENT(glUniform1i);
    GL_CHECK_PRESENT(glDrawElements);
    GL_CHECK_PRESENT(glEnable);
    GL_CHECK_PRESENT(glDisable);
    GL_CHECK_PRESENT(glBlendFunc);
    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindTexture);

    _vao = gpuAPI->allocateUninitializedVAO();
    gpuAPI->initializeVAO(_vao);

    _init3DObjectsType = Init3DObjectsType::Objects3D;

    return true;
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const /*metric*/)
{
    bool ok = true;

    if (_init3DObjectsType != Init3DObjectsType::Complete)
    {
        const auto init3DObjectsType = _init3DObjectsType;
        ok = ok && (init3DObjectsType != Init3DObjectsType::Objects3D || initializeProgram());

        if (!ok || _init3DObjectsType == Init3DObjectsType::Incomplete)
            return StageResult::Fail;

        return StageResult::Wait;
    }

    const auto resourcesCollection = getResources().getCollectionSnapshot(MapRendererResourceType::Map3DObjects,
        std::static_pointer_cast<IMapDataProvider>(currentState.map3DObjectsProvider));

    if (!resourcesCollection)
    {
        return StageResult::Success;
    }

    const bool debugEnabled = debugSettings && debugSettings->debugStageEnabled;
    const bool debugDetailedInfo = debugEnabled && debugSettings->enableMap3dObjectsDebugInfo;
    Stopwatch renderStopwatch(debugEnabled);
    
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();
    
    QSet<uint64_t> drawnBboxHashes;
    int tilesDrawnCount = 0;
    int totalObjectsCount = 0;
    int objectsDrawnCount = 0;
    QVector<std::pair<TileId, ZoomLevel>> drawnResources;

    glUseProgram(_program.id);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*_program.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;
    glUniform2i(*_program.vs.param.target31, currentState.target31.x, currentState.target31.y);
    GL_CHECK_RESULT;
    glUniform1i(*_program.vs.param.zoomLevel, (int)currentState.zoomLevel);
    GL_CHECK_RESULT;
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, 1.0f, -0.5f));
    glUniform3f(*_program.vs.param.lightDirection, lightDir.x, lightDir.y, lightDir.z);
    GL_CHECK_RESULT;
    glUniform1f(*_program.vs.param.ambient, 0.2f);
    GL_CHECK_RESULT;

    const auto CollectionStapshot = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection);

    auto tilesBegin = internalState.visibleTiles.cbegin();
    for (auto itTiles = internalState.visibleTiles.cend(); itTiles != tilesBegin; itTiles--)
    {
        const auto& tilesEntry = itTiles - 1;
        const auto& visibleTilesSet = internalState.visibleTilesSet.constFind(tilesEntry.key());

        if (visibleTilesSet == internalState.visibleTilesSet.cend())
        {
            continue;
        }

        for (const auto& tileId : constOf(tilesEntry.value()))
        {
            const auto tileIdN = Utilities::normalizeTileId(tileId, currentState.zoomLevel);

            if (!visibleTilesSet->contains(tileIdN))
            {
                continue;
            }

            const int maxMissingDataUnderZoomShift = currentState.map3DObjectsProvider->getMaxMissingDataUnderZoomShift();

            int desiredOffset = internalState.zoomLevelOffset;
            if (desiredOffset == 0 && !internalState.extraDetailedTiles.empty())
            {
                desiredOffset = 1;
            }

            const int appliedOffset = std::min(desiredOffset, maxMissingDataUnderZoomShift);
            const int neededZoom = std::min(static_cast<int>(MaxZoomLevel), tilesEntry.key() + appliedOffset);

            bool rendered = false;
            {
                std::shared_ptr<MapRendererBaseTiledResource> tiledResource;
                CollectionStapshot->obtainResource(tileIdN, (ZoomLevel)neededZoom, tiledResource);

                const auto object3DResource = std::static_pointer_cast<MapRenderer3DObjectsResource>(tiledResource);
                if (object3DResource && object3DResource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
                {
                    totalObjectsCount += object3DResource->getRenderableBuildings().size();
                    const auto elevationData = findElevationData(tileIdN, static_cast<ZoomLevel>(neededZoom));
                    const int drawnCount = drawResource(tileIdN, static_cast<ZoomLevel>(neededZoom), object3DResource, drawnBboxHashes, elevationData);
                    objectsDrawnCount += drawnCount;
                    object3DResource->setState(MapRendererResourceState::Uploaded);
                    rendered = true;
                    tilesDrawnCount++;
                    if (debugDetailedInfo)
                    {
                        drawnResources.append(std::make_pair(tileIdN, static_cast<ZoomLevel>(neededZoom)));
                    }
                }
            }

            // Try underscaled subtiles (higher zoom) if nothing rendered yet
            if (!rendered)
            {
                const int maxZoomShift = currentState.map3DObjectsProvider->getMaxMissingDataZoomShift();
                for (int absZoomShift = 1; absZoomShift <= maxZoomShift; absZoomShift++)
                {
                    const int underscaledZoom = static_cast<int>(tilesEntry.key()) + absZoomShift;
                    if (underscaledZoom > static_cast<int>(MaxZoomLevel) || absZoomShift > maxMissingDataUnderZoomShift)
                    {
                        break;
                    }

                    const auto subTileIds = Utilities::getTileIdsUnderscaledByZoomShift(tileIdN, absZoomShift);
                    bool atLeastOne = false;

                    for (const auto& subId : constOf(subTileIds))
                    {
                        std::shared_ptr<MapRendererBaseTiledResource> subResBase;
                        CollectionStapshot->obtainResource(subId, static_cast<ZoomLevel>(underscaledZoom), subResBase);

                        const auto subRes = std::static_pointer_cast<MapRenderer3DObjectsResource>(subResBase);
                        if (subRes && subRes->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
                        {
                            totalObjectsCount += subRes->getRenderableBuildings().size();
                            const auto elevationData = findElevationData(subId, static_cast<ZoomLevel>(underscaledZoom));
                            const int drawnCount = drawResource(subId, static_cast<ZoomLevel>(underscaledZoom), subRes, drawnBboxHashes, elevationData);
                            objectsDrawnCount += drawnCount;
                            subRes->setState(MapRendererResourceState::Uploaded);
                            atLeastOne = true;
                            tilesDrawnCount++;
                            if (debugDetailedInfo)
                            {
                                drawnResources.append(std::make_pair(subId, static_cast<ZoomLevel>(underscaledZoom)));
                            }
                        }
                    }

                    if (atLeastOne)
                    {
                        rendered = true;
                        break;
                    }
                }
            }

            // Try overscaled parent (lower zoom) if still nothing
            if (!rendered)
            {
                const int maxZoomShift = currentState.map3DObjectsProvider->getMaxMissingDataZoomShift();
                for (int absZoomShift = 1; absZoomShift <= maxZoomShift; absZoomShift++)
                {
                    const int overscaledZoom = static_cast<int>(tilesEntry.key()) - absZoomShift;
                    if (overscaledZoom < static_cast<int>(MinZoomLevel))
                    {
                        break;
                    }

                    const auto parentId = Utilities::getTileIdOverscaledByZoomShift(tileIdN, absZoomShift);
                    std::shared_ptr<MapRendererBaseTiledResource> parentBase;

                    CollectionStapshot->obtainResource(parentId, static_cast<ZoomLevel>(overscaledZoom), parentBase);
                    const auto parentRes = std::static_pointer_cast<MapRenderer3DObjectsResource>(parentBase);

                    if (parentRes && parentRes->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
                    {
                        totalObjectsCount += parentRes->getRenderableBuildings().size();
                        const auto elevationData = findElevationData(parentId, static_cast<ZoomLevel>(overscaledZoom));
                        const int drawnCount = drawResource(parentId, static_cast<ZoomLevel>(overscaledZoom), parentRes, drawnBboxHashes, elevationData);
                        objectsDrawnCount += drawnCount;
                        parentRes->setState(MapRendererResourceState::Uploaded);
                        rendered = true;
                        tilesDrawnCount++;
                        if (debugDetailedInfo)
                        {
                            drawnResources.append(std::make_pair(parentId, static_cast<ZoomLevel>(overscaledZoom)));
                        }
                        break;
                    }
                }
            }
        }
    }

    if (debugEnabled)
    {
        const float renderTime = renderStopwatch.elapsed() * 1000.0f;
        size_t totalGpuMemoryBytes = 0;
        
        if (debugDetailedInfo)
        {
            for (const auto& resourceInfo : constOf(drawnResources))
            {
                std::shared_ptr<MapRendererBaseTiledResource> tiledResource;
                CollectionStapshot->obtainResource(resourceInfo.first, resourceInfo.second, tiledResource);
                const auto object3DResource = std::static_pointer_cast<MapRenderer3DObjectsResource>(tiledResource);
                if (object3DResource)
                {
                    const auto& perfInfo = object3DResource->getPerformanceDebugInfo();
                    totalGpuMemoryBytes += perfInfo.totalGpuMemoryBytes;
                }
            }
        }
        
        const float totalGpuMemoryMB = static_cast<float>(totalGpuMemoryBytes) / (1024.0f * 1024.0f);
        QString debugString = QString("3D_OBJECTS_DEBUG <<<<< RenderTime: %1ms, TilesDrawn: %2, TotalObjects: %3, ObjectsDrawn: %4, TotalGpuMemory: %5 MB")
            .arg(renderTime, 0, 'f', 2)
            .arg(tilesDrawnCount)
            .arg(totalObjectsCount)
            .arg(objectsDrawnCount)
            .arg(totalGpuMemoryMB, 0, 'f', 3);
        
        if (debugDetailedInfo)
        {
            for (const auto& resourceInfo : constOf(drawnResources))
            {
                std::shared_ptr<MapRendererBaseTiledResource> tiledResource;
                CollectionStapshot->obtainResource(resourceInfo.first, resourceInfo.second, tiledResource);
                const auto object3DResource = std::static_pointer_cast<MapRenderer3DObjectsResource>(tiledResource);
                if (object3DResource)
                {
                    const auto& perfInfo = object3DResource->getPerformanceDebugInfo();
                    debugString += QString("\n3D_OBJECTS_DEBUG <<<<< Resource[%1x%2@%3]: GPU: %4 bytes, ObtainData: %5ms, UploadToGPU: %6ms")
                        .arg(resourceInfo.first.x)
                        .arg(resourceInfo.first.y)
                        .arg(resourceInfo.second)
                        .arg(perfInfo.totalGpuMemoryBytes)
                        .arg(perfInfo.obtainDataTimeMilliseconds, 0, 'f', 2)
                        .arg(perfInfo.uploadToGpuTimeMilliseconds, 0, 'f', 2);
                }
            }
        }
        
        LogPrintf(LogSeverityLevel::Info, "%s", qPrintable(debugString));
    }

    return StageResult::Success;
}

AtlasMapRendererMap3DObjectsStage_OpenGL::ElevationData AtlasMapRendererMap3DObjectsStage_OpenGL::findElevationData(const TileId& tileIdN, ZoomLevel buildingZoom)
{
    ElevationData result;
    result.hasData = false;
    result.zoom = buildingZoom;
    result.tileIdN = tileIdN;
    result.texCoordsOffset = PointF(0.0f, 0.0f);
    result.texCoordsScale = PointF(1.0f, 1.0f);

    if (!currentState.elevationDataProvider)
    {
        return result;
    }

    result.resource = captureElevationDataResource(tileIdN, buildingZoom);
    if (result.resource)
    {
        result.hasData = true;
        return result;
    }

    const auto maxMissingDataZoomShift = currentState.elevationDataProvider->getMaxMissingDataZoomShift();
    const auto maxUnderZoomShift = currentState.elevationDataProvider->getMaxMissingDataUnderZoomShift();
    const auto minZoom = currentState.elevationDataProvider->getMinZoom();
    const auto maxZoom = currentState.elevationDataProvider->getMaxZoom();

    for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
    {
        // Try underscaled first
        const auto underscaledZoom = static_cast<int>(buildingZoom) + absZoomShift;
        if (underscaledZoom >= minZoom && underscaledZoom <= maxZoom && 
            absZoomShift <= maxUnderZoomShift && buildingZoom >= minZoom)
        {
            result.zoom = static_cast<ZoomLevel>(underscaledZoom);
            const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(tileIdN, absZoomShift);

            if (!underscaledTileIdsN.isEmpty())
            {
                result.tileIdN = underscaledTileIdsN[0];
                result.resource = captureElevationDataResource(result.tileIdN, result.zoom);

                if (result.resource)
                {
                    result.hasData = true;
                    const auto subtilesPerSide = (1u << absZoomShift);
                    result.texCoordsScale = PointF(static_cast<float>(subtilesPerSide), static_cast<float>(subtilesPerSide));

                    uint16_t xSubtile, ySubtile;
                    Utilities::decodeMortonCode(0, xSubtile, ySubtile);
                    result.texCoordsOffset = PointF(-static_cast<float>(xSubtile), -static_cast<float>(ySubtile));

                    return result;
                }
            }
        }

        // Try overscaled
        const auto overscaledZoom = static_cast<int>(buildingZoom) - absZoomShift;
        if (overscaledZoom >= minZoom && overscaledZoom <= maxZoom)
        {
            result.zoom = static_cast<ZoomLevel>(overscaledZoom);
            result.tileIdN = Utilities::getTileIdOverscaledByZoomShift(tileIdN, absZoomShift, &result.texCoordsOffset, &result.texCoordsScale);
            result.resource = captureElevationDataResource(result.tileIdN, result.zoom);

            if (result.resource)
            {
                result.hasData = true;
                return result;
            }
        }
    }

    return result;
}

int OsmAnd::AtlasMapRendererMap3DObjectsStage_OpenGL::drawResource(const TileId& id, ZoomLevel z,
    const std::shared_ptr<MapRenderer3DObjectsResource>& res, QSet<uint64_t>& drawnBboxHashes, const ElevationData& elevationData)
{
    int drawnCount = 0;
    const auto gpuAPI = getGPUAPI();

    const int deltaZoom = static_cast<int>(z) - static_cast<int>(currentState.zoomLevel);
    int yAtCurrentZoom;
    if (deltaZoom >= 0)
    {
        yAtCurrentZoom = static_cast<int>(id.y) >> deltaZoom;
    }
    else
    {
        yAtCurrentZoom = static_cast<int>(id.y) << (-deltaZoom);
    }

    const double metersPerUnit = Utilities::getMetersPerTileUnit(
        currentState.zoomLevel, yAtCurrentZoom, AtlasMapRenderer::TileSize3D);

    glUniform1f(*_program.vs.param.metersPerUnit, static_cast<float>(metersPerUnit));
    GL_CHECK_RESULT;

    const auto tileIdN = Utilities::normalizeTileId(id, z);

    float zScaleFactor = 0.0f;
    float dataScaleFactor = 0.0f;
    double upperMetersPerUnit = metersPerUnit;
    double lowerMetersPerUnit = metersPerUnit;
    
    if (elevationData.hasData && elevationData.resource)
    {
        zScaleFactor = currentState.elevationConfiguration.zScaleFactor;
        dataScaleFactor = currentState.elevationConfiguration.dataScaleFactor;
        
        if (currentState.flatEarth)
        {
            const auto tileSize = static_cast<double>(AtlasMapRenderer::TileSize3D) *
                static_cast<double>(1ull << (currentState.zoomLevel - elevationData.zoom));

            upperMetersPerUnit = Utilities::getMetersPerTileUnit(elevationData.zoom, tileIdN.y, tileSize);
            lowerMetersPerUnit = Utilities::getMetersPerTileUnit(elevationData.zoom, tileIdN.y + 1, tileSize);
        }
        else
        {
            const auto& internalState = getInternalState();
            upperMetersPerUnit = internalState.metersPerUnit;
            lowerMetersPerUnit = internalState.metersPerUnit;
        }
        
        configureElevationData(elevationData.resource, elevationData.tileIdN, elevationData.zoom, 
            elevationData.texCoordsOffset, elevationData.texCoordsScale);
        

        const auto zoomShift = static_cast<int>(ZoomLevel::MaxZoomLevel) - static_cast<int>(elevationData.zoom);
        const PointI elevationTile31(elevationData.tileIdN.x << zoomShift, elevationData.tileIdN.y << zoomShift);

        glUniform2i(*_program.vs.param.elevationTileCoords31, elevationTile31.x, elevationTile31.y);
        GL_CHECK_RESULT;
        glUniform1i(*_program.vs.param.elevationTileZoomLevel, static_cast<int>(elevationData.zoom));
        GL_CHECK_RESULT;
    }
    else
    {
        cancelElevation();
        zScaleFactor = 0.0f;
        dataScaleFactor = 0.0f;
    }

    glUniform4f(*_program.vs.param.elevation_scale, static_cast<float>(upperMetersPerUnit),
        static_cast<float>(lowerMetersPerUnit), zScaleFactor, dataScaleFactor);
    GL_CHECK_RESULT;

    const auto& buildings = res->getRenderableBuildings();
    for (const auto& b : constOf(buildings))
    {
        if (!b.vertexBuffer || !b.indexBuffer || b.vertexCount <= 0 || b.indexCount <= 0)
        {
            continue;
        }

        if (drawnBboxHashes.contains(b.bboxHash))
        {
            continue;
        }
        
        drawnBboxHashes.insert(b.bboxHash);
        drawnCount++;

        gpuAPI->useVAO(_vao);

        glUniform4f(*_program.vs.param.color, b.color.r, b.color.g, b.color.b, b.color.a);
        GL_CHECK_RESULT;

        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(b.vertexBuffer->refInGPU)));
        GL_CHECK_RESULT;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(b.indexBuffer->refInGPU)));
        GL_CHECK_RESULT;

        glEnableVertexAttribArray(*_program.vs.in.location31);
        GL_CHECK_RESULT;
        glVertexAttribIPointer(*_program.vs.in.location31, 2, GL_INT, sizeof(BuildingVertex),
            reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, location31)));
        GL_CHECK_RESULT;

        glEnableVertexAttribArray(*_program.vs.in.height);
        GL_CHECK_RESULT;
        glVertexAttribPointer(*_program.vs.in.height, 1, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
            reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, height)));
        GL_CHECK_RESULT;

        glEnableVertexAttribArray(*_program.vs.in.normal);
        GL_CHECK_RESULT;
        glVertexAttribPointer(*_program.vs.in.normal, 3, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
            reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, normal)));
        GL_CHECK_RESULT;

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(b.indexCount), GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(0));
        GL_CHECK_RESULT;

        gpuAPI->unuseVAO();
    }
    
    return drawnCount;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::release(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    if (_vao.isValid())
    {
        gpuAPI->releaseVAO(_vao, gpuContextLost);
        _vao.reset();
    }
    if (_program.id)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;
        _program.id = 0;
    }
    return true;
}

void AtlasMapRendererMap3DObjectsStage_OpenGL::configureElevationData(
    const std::shared_ptr<const GPUAPI::ResourceInGPU>& elevationDataResource,
    const TileId tileIdN,
    const ZoomLevel zoomLevel,
    const PointF& texCoordsOffsetN,
    const PointF& texCoordsScaleN)
{
    const auto gpuAPI = getGPUAPI();

    const int elevationDataSamplerIndex = 0;

    glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
    GL_CHECK_RESULT;

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(elevationDataResource->refInGPU)));
    GL_CHECK_RESULT;

    gpuAPI->applyTextureBlockToTexture(GL_TEXTURE_2D, GL_TEXTURE0 + elevationDataSamplerIndex);

    if (elevationDataResource->type == GPUAPI::ResourceInGPU::Type::SlotOnAtlasTexture)
    {
        const auto tileOnAtlasTexture = std::static_pointer_cast<const GPUAPI::SlotOnAtlasTextureInGPU>(elevationDataResource);
        const auto& texture = tileOnAtlasTexture->atlasTexture;

        const auto rowIndex = tileOnAtlasTexture->slotIndex / texture->slotsPerSide;
        const auto colIndex = tileOnAtlasTexture->slotIndex - rowIndex * texture->slotsPerSide;

        // Must be in sync with IMapElevationDataProvider::Data::getValue
        const PointF innerSize(texture->tileSizeN - 3.0f * texture->uTexelSizeN, texture->tileSizeN - 3.0f * texture->vTexelSizeN);
        const PointF texCoordsScale(innerSize.x * texCoordsScaleN.x, innerSize.y * texCoordsScaleN.y);

        const PointF texPlace(colIndex * texture->tileSizeN + texture->uHalfTexelSizeN + texture->uTexelSizeN,
            rowIndex * texture->tileSizeN + texture->vHalfTexelSizeN + texture->vTexelSizeN);

        const PointF texCoordsOffset(texPlace.x + innerSize.x * texCoordsOffsetN.x,
            texPlace.y + innerSize.y * texCoordsOffsetN.y);

        glUniform4f(_program.vs.param.elevationLayer.txOffsetAndScale,
            texCoordsOffset.x, texCoordsOffset.y, texCoordsScale.x, texCoordsScale.y);
        GL_CHECK_RESULT;

        glUniform4f(_program.vs.param.elevationLayerDataPlace,
            texPlace.x, texPlace.y, texture->uHalfTexelSizeN, texture->vHalfTexelSizeN);
        GL_CHECK_RESULT;
    }
    else // if (elevationDataResource->type == GPUAPI::ResourceInGPU::Type::Texture)
    {
        const auto& texture = std::static_pointer_cast<const GPUAPI::TextureInGPU>(elevationDataResource);

        const PointF innerSize(
            1.0f - 3.0f * texture->uTexelSizeN,
            1.0f - 3.0f * texture->vTexelSizeN
        );

        const PointF texCoordsScale(innerSize.x * texCoordsScaleN.x, innerSize.y * texCoordsScaleN.y);
        const PointF texPlace(texture->uHalfTexelSizeN + texture->uTexelSizeN, texture->vHalfTexelSizeN + texture->vTexelSizeN);
        const PointF texCoordsOffset(texPlace.x + innerSize.x * texCoordsOffsetN.x, texPlace.y + innerSize.y * texCoordsOffsetN.y);

        glUniform4f(_program.vs.param.elevationLayer.txOffsetAndScale,
            texCoordsOffset.x, texCoordsOffset.y, texCoordsScale.x, texCoordsScale.y);
        GL_CHECK_RESULT;

        glUniform4f(_program.vs.param.elevationLayerDataPlace,
            texPlace.x, texPlace.y, texture->uHalfTexelSizeN, texture->vHalfTexelSizeN);
        GL_CHECK_RESULT;
    }

    glUniform1i(_program.vs.param.elevation_dataSampler, elevationDataSamplerIndex);
    GL_CHECK_RESULT;
}

void AtlasMapRendererMap3DObjectsStage_OpenGL::cancelElevation()
{
    const auto gpuAPI = getGPUAPI();

    GL_CHECK_PRESENT(glActiveTexture);
    GL_CHECK_PRESENT(glBindTexture);

    const int elevationDataSamplerIndex = 0;

    glActiveTexture(GL_TEXTURE0 + elevationDataSamplerIndex);
    GL_CHECK_RESULT;

    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK_RESULT;
}
