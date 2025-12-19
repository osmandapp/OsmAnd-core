#include "AtlasMapRendererMap3DObjectsStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <QSet>
#include <QHash>
#include <QPair>

#include "OpenGL/GPUAPI_OpenGL.h"
#include "AtlasMapRenderer_OpenGL.h"
#include "Utilities.h"
#include "MapRendererTiledResourcesCollection.h"
#include "MapRenderer3DObjects.h"
#include "MapRendererDebugSettings.h"
#include <OsmAndCore/Map/Map3DObjectsProvider.h>
#include "Stopwatch.h"
#include "Logging.h"

using namespace OsmAnd;

namespace
{
    const QString vertexShaderBase = QString(R"(
            INPUT ivec2 in_vs_location31;
            INPUT float in_vs_height;
            
            %ColorInOutDeclaration%
            
            uniform mat4 param_vs_mPerspectiveProjectionView;
            uniform vec4 param_vs_resultScale;
            uniform ivec2 param_vs_target31;
            uniform int param_vs_zoomLevel;
            uniform float param_vs_metersPerUnit;
            
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

            void main()
            {
                float elevation = in_vs_height / param_vs_metersPerUnit;
                vec3 worldPos = planeWorldCoordinates(in_vs_location31, param_vs_target31, param_vs_zoomLevel, elevation);
                
                vec4 v = param_vs_mPerspectiveProjectionView * vec4(worldPos, 1.0);
                gl_Position = v * param_vs_resultScale;
                
                %ColorCalculation%
            }
        )");
}

AtlasMapRendererMap3DObjectsStage_OpenGL::AtlasMapRendererMap3DObjectsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer)
    : AtlasMapRendererMap3DObjectsStage(renderer)
    , AtlasMapRendererStageHelper_OpenGL(this)
{
}

AtlasMapRendererMap3DObjectsStage_OpenGL::~AtlasMapRendererMap3DObjectsStage_OpenGL()
{
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initializeColorProgram()
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
        const QString colorInOutDeclaration = QString(R"(
            INPUT vec3 in_vs_normal;
            INPUT vec3 in_vs_color;
        
            PARAM_OUTPUT vec4 v2f_color;
        
            uniform float param_vs_alpha;
            uniform vec3 param_vs_lightDirection;
            uniform float param_vs_ambient;
        )");
        const QString colorCalculation = QString(R"(
                float ndotl = max(dot(in_vs_normal, param_vs_lightDirection), 0.0);
                float diffuse = param_vs_ambient + (1.0 - param_vs_ambient) * ndotl;
                v2f_color = vec4(in_vs_color * diffuse, param_vs_alpha);
        )");

        auto vertexShader = vertexShaderBase;
        vertexShader.replace("%ColorInOutDeclaration%", colorInOutDeclaration);
        vertexShader.replace("%ColorCalculation%", colorCalculation);

        const QString fragmentShader = R"(
            PARAM_INPUT vec4 v2f_color;
            
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
    ok = ok && lookup->lookupLocation(_program.vs.in.color, "in_vs_color", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.alpha, "param_vs_alpha", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.zoomLevel, "param_vs_zoomLevel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.lightDirection, "param_vs_lightDirection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.ambient, "param_vs_ambient", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);

    if (!ok)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;

        _program.id.reset();

        LogPrintf(LogSeverityLevel::Error,
            "Failed to find variable in Map3DObjects shader program");
        return false;
    }

    if (_vao.isValid())
    {
        gpuAPI->useVAO(_vao);
        
        glEnableVertexAttribArray(*_program.vs.in.location31);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_program.vs.in.height);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_program.vs.in.normal);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_program.vs.in.color);
        GL_CHECK_RESULT;
        
        gpuAPI->initializeVAO(_vao);
        gpuAPI->unuseVAO();
    }

    _init3DObjectsType = nextInit3DobjectsType;
    return true;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initializeDepthProgram()
{
    const auto nextInit3DobjectsType = static_cast<Init3DObjectsType>(static_cast<int>(_init3DObjectsType) + 1);
    _init3DObjectsType = Init3DObjectsType::Incomplete;

    const auto gpuAPI = getGPUAPI();
    
    QHash<QString, GPUAPI_OpenGL::GlslProgramVariable> variablesMap;
    _depthProgram.id = 0;

    if (!_depthProgram.binaryCache.isEmpty())
    {
        _depthProgram.id = gpuAPI->linkProgram(0, nullptr, _depthProgram.binaryCache, _depthProgram.cacheFormat, true, &variablesMap);
    }

    if (!_depthProgram.id.isValid())
    {
        auto vertexShader = vertexShaderBase;
        vertexShader.replace("%ColorInOutDeclaration%", "");
        vertexShader.replace("%ColorCalculation%", "");

        const QString fragmentShader = R"(
            void main()
            {
            }
        )";

        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        auto preprocessedFragmentShader = fragmentShader;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        _depthProgram.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _depthProgram.cacheFormat);

        if (!_depthProgram.binaryCache.isEmpty())
        {
            _depthProgram.id = gpuAPI->linkProgram(0, nullptr, _depthProgram.binaryCache, _depthProgram.cacheFormat, true, &variablesMap);
        }
        if (_depthProgram.binaryCache.isEmpty() || !_depthProgram.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects depth vertex shader");
                return false;
            }

            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects depth fragment shader");
                return false;
            }

            const GLuint shaders[] = { vsId, fsId };
            _depthProgram.id = gpuAPI->linkProgram(2, shaders, _depthProgram.binaryCache, _depthProgram.cacheFormat, true, &variablesMap);
            if (_depthProgram.id.isValid() && !_depthProgram.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _depthProgram.binaryCache,
                    _depthProgram.cacheFormat);
            }
        }
    }

    if (!_depthProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link Map3DObjects depth shader program");
        return false;
    }

    const auto lookup = gpuAPI->obtainVariablesLookupContext(_depthProgram.id, variablesMap);
    bool ok = true;
    ok = ok && lookup->lookupLocation(_depthProgram.vs.in.location31, "in_vs_location31", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.in.height, "in_vs_height", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.zoomLevel, "param_vs_zoomLevel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);

    if (!ok)
    {
        glDeleteProgram(_depthProgram.id);
        GL_CHECK_RESULT;

        _depthProgram.id.reset();

        LogPrintf(LogSeverityLevel::Error,
            "Failed to find variable in Map3DObjects depth shader program");
        return false;
    }

    if (_depthVao.isValid())
    {
        gpuAPI->useVAO(_depthVao);
        
        glEnableVertexAttribArray(*_depthProgram.vs.in.location31);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_depthProgram.vs.in.height);
        GL_CHECK_RESULT;
        
        gpuAPI->initializeVAO(_depthVao);
        gpuAPI->unuseVAO();
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
    
    _depthVao = gpuAPI->allocateUninitializedVAO();
    gpuAPI->initializeVAO(_depthVao);

    _init3DObjectsType = Init3DObjectsType::Objects3DDepth;

    return true;
}

void AtlasMapRendererMap3DObjectsStage_OpenGL::prepareDrawObjects(QSet<std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData>>& collectedResources)
{
    const auto resourcesCollection = getResources().getCollectionSnapshot(MapRendererResourceType::Map3DObjects,
        std::static_pointer_cast<IMapDataProvider>(currentState.map3DObjectsProvider));

    if (!resourcesCollection)
    {
        return;
    }

    const auto& internalState = getInternalState();
    
    float buildingAlpha = renderer->get3DBuildingsAlpha();

    const auto CollectionStapshot = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection);
    
    QVector<ZoomLevel> sortedZoomLevels;
    for (auto itTiles = internalState.visibleTiles.cbegin(); itTiles != internalState.visibleTiles.cend(); itTiles++)
    {
        sortedZoomLevels.append(itTiles.key());
    }
    
    if (buildingAlpha < 1.0f)
    {
        std::sort(sortedZoomLevels.begin(), sortedZoomLevels.end(), std::greater<ZoomLevel>());
    }
    else
    {
        std::sort(sortedZoomLevels.begin(), sortedZoomLevels.end());
    }

    for (const auto& zoomLevel : constOf(sortedZoomLevels))
    {
        const auto& visibleTilesSet = internalState.visibleTilesSet.constFind(zoomLevel);
        if (visibleTilesSet == internalState.visibleTilesSet.cend())
        {
            continue;
        }
        
        const auto& tilesForZoom = internalState.visibleTiles.value(zoomLevel);
        for (const auto& tileId : constOf(tilesForZoom))
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
            const int neededZoom = std::min(static_cast<int>(MaxZoomLevel), static_cast<int>(zoomLevel) + appliedOffset);

            bool collected = false;

            {
                std::shared_ptr<MapRendererBaseTiledResource> tiledResource;
                CollectionStapshot->obtainResource(tileIdN, (ZoomLevel)neededZoom, tiledResource);

                const auto object3DResource = std::static_pointer_cast<MapRenderer3DObjectsResource>(tiledResource);
                if (object3DResource && object3DResource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
                {
                    for (const auto& resource : object3DResource->getRenderableBuildings().buildingResources)
                    {
                        collectedResources.insert(resource);
                    }

                    object3DResource->setState(MapRendererResourceState::Uploaded);
                    collected = true;
                }
            }

            if (!collected)
            {
                const int maxZoomShift = currentState.map3DObjectsProvider->getMaxMissingDataZoomShift();
                for (int absZoomShift = 1; absZoomShift <= maxZoomShift; absZoomShift++)
                {
                    const int underscaledZoom = static_cast<int>(zoomLevel) + absZoomShift;
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
                            for (const auto& resource : subRes->getRenderableBuildings().buildingResources)
                            {
                                collectedResources.insert(resource);
                            }

                            subRes->setState(MapRendererResourceState::Uploaded);
                            collected = true;
                        }
                    }

                    if (atLeastOne)
                    {
                        collected = true;
                        break;
                    }
                }
            }

            if (!collected)
            {
                const int maxZoomShift = currentState.map3DObjectsProvider->getMaxMissingDataZoomShift();
                for (int absZoomShift = 1; absZoomShift <= maxZoomShift; absZoomShift++)
                {
                    const int overscaledZoom = static_cast<int>(zoomLevel) - absZoomShift;
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
                        for (const auto& resource : parentRes->getRenderableBuildings().buildingResources)
                        {
                            collectedResources.insert(resource);
                        }

                        parentRes->setState(MapRendererResourceState::Uploaded);
                        break;
                    }
                }
            }

            if (!collected)
            {
                std::shared_ptr<MapRendererBaseTiledResource> tiledResource;
                CollectionStapshot->obtainResource(tileIdN, (ZoomLevel)zoomLevel, tiledResource);

                const auto object3DResource = std::static_pointer_cast<MapRenderer3DObjectsResource>(tiledResource);
                if (object3DResource && object3DResource->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
                {
                    for (const auto& resource : object3DResource->getRenderableBuildings().buildingResources)
                    {
                        collectedResources.insert(resource);
                    }

                    object3DResource->setState(MapRendererResourceState::Uploaded);
                    collected = true;
                }
            }
        }
    }
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::renderDepth(QSet<std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData>>& collectedResources)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    glUseProgram(_depthProgram.id);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*_depthProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;
    glUniform4f(*_depthProgram.vs.param.resultScale, 1.0f, currentState.flip ? -1.0f : 1.0f, 1.0f, 1.0f);
    GL_CHECK_RESULT;
    glUniform2i(*_depthProgram.vs.param.target31, currentState.target31.x, currentState.target31.y);
    GL_CHECK_RESULT;
    glUniform1i(*_depthProgram.vs.param.zoomLevel, (int)currentState.zoomLevel);
    GL_CHECK_RESULT;
    glUniform1f(*_depthProgram.vs.param.metersPerUnit, static_cast<float>(internalState.metersPerUnit));
    GL_CHECK_RESULT;

    gpuAPI->useVAO(_depthVao);

    for (const auto& resource : collectedResources)
    {
        if (resource->indexBuffer && resource->indexBuffer->itemsCount > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(resource->vertexBuffer->refInGPU)));
            GL_CHECK_RESULT;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(resource->indexBuffer->refInGPU)));
            GL_CHECK_RESULT;

            glVertexAttribIPointer(*_depthProgram.vs.in.location31, 2, GL_INT, sizeof(BuildingVertex),
                                   reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, location31)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_depthProgram.vs.in.height, 1, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                                  reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, height)));
            GL_CHECK_RESULT;

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(resource->indexBuffer->itemsCount), GL_UNSIGNED_SHORT, 0);
            GL_CHECK_RESULT;
        }
    }

    gpuAPI->unuseVAO();

    return StageResult::Success;
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::renderColor(QSet<std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData>>& collectedResources)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    float buildingAlpha = renderer->get3DBuildingsAlpha();

    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, 1.0f, -0.5f));

    glUseProgram(_program.id);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*_program.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;
    glUniform4f(*_program.vs.param.resultScale, 1.0f, currentState.flip ? -1.0f : 1.0f, 1.0f, 1.0f);
    GL_CHECK_RESULT;
    glUniform2i(*_program.vs.param.target31, currentState.target31.x, currentState.target31.y);
    GL_CHECK_RESULT;
    glUniform1i(*_program.vs.param.zoomLevel, (int)currentState.zoomLevel);
    GL_CHECK_RESULT;
    glUniform1f(*_program.vs.param.metersPerUnit, static_cast<float>(internalState.metersPerUnit));
    GL_CHECK_RESULT;
    glUniform3f(*_program.vs.param.lightDirection, lightDir.x, lightDir.y, lightDir.z);
    GL_CHECK_RESULT;
    glUniform1f(*_program.vs.param.ambient, 0.2f);
    GL_CHECK_RESULT;
    glUniform1f(*_program.vs.param.alpha, buildingAlpha);
    GL_CHECK_RESULT;

    gpuAPI->useVAO(_vao);

    for (const auto& resource : collectedResources)
    {
        if (resource->indexBuffer && resource->indexBuffer->itemsCount > 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(resource->vertexBuffer->refInGPU)));
            GL_CHECK_RESULT;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(resource->indexBuffer->refInGPU)));
            GL_CHECK_RESULT;

            glVertexAttribIPointer(*_program.vs.in.location31, 2, GL_INT, sizeof(BuildingVertex),
                                   reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, location31)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_program.vs.in.height, 1, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                                  reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, height)));
            GL_CHECK_RESULT;

            glVertexAttribPointer(*_program.vs.in.normal, 3, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                                  reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, normal)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_program.vs.in.color, 3, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                                  reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, color)));
            GL_CHECK_RESULT;

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(resource->indexBuffer->itemsCount), GL_UNSIGNED_SHORT, 0);
            GL_CHECK_RESULT;
        }
    }

    gpuAPI->unuseVAO();

    return StageResult::Success;
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const metric)
{
    bool ok = true;

    if (_init3DObjectsType != Init3DObjectsType::Complete)
    {
        const auto init3DObjectsType = _init3DObjectsType;
        ok = ok && (init3DObjectsType != Init3DObjectsType::Objects3DDepth || initializeDepthProgram());
        ok = ok && (init3DObjectsType != Init3DObjectsType::Objects3DColor || initializeColorProgram());

        if (!ok || _init3DObjectsType == Init3DObjectsType::Incomplete)
        {
            return StageResult::Fail;
        }

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

    QSet<std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData>> collectedResources;
    prepareDrawObjects(collectedResources);

    if (collectedResources.isEmpty())
    {
        return StageResult::Success;
    }

    const bool needsDepthPrepass = isDepthPrepassRequired();

    glEnable(GL_CULL_FACE);
    GL_CHECK_RESULT;

    glCullFace(currentState.flip ? GL_BACK : GL_FRONT);
    GL_CHECK_RESULT;

    StageResult depthPrepassResult = StageResult::Success;
    if (needsDepthPrepass)
    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        GL_CHECK_RESULT;

        glDisable(GL_BLEND);
        GL_CHECK_RESULT;

        depthPrepassResult = renderDepth(collectedResources);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        GL_CHECK_RESULT;

        glDepthMask(GL_FALSE);
        GL_CHECK_RESULT;

        glEnable(GL_BLEND);
        GL_CHECK_RESULT;
    }

    const auto colorPassResult = renderColor(collectedResources);

    if (needsDepthPrepass)
    {
        glDepthMask(GL_TRUE);
        GL_CHECK_RESULT;
    }

    glDisable(GL_CULL_FACE);
    GL_CHECK_RESULT;

    if (debugEnabled)
    {
        const auto CollectionStapshot = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection);
        const float renderTime = renderStopwatch.elapsed() * 1000.0f;
        size_t totalGpuMemoryBytes = 0;

        QString debugString;

        if (debugDetailedInfo)
        {
            for (const auto& resourceInfo : constOf(collectedResources))
            {
                const uint64_t tileTotalGpuMemoryBytes = resourceInfo->indexBuffer->itemsCount * sizeof(uint16_t)
                    + resourceInfo->vertexBuffer->itemsCount * sizeof(BuildingVertex);
                totalGpuMemoryBytes += tileTotalGpuMemoryBytes;

                if (debugDetailedInfo)
                {
                    debugString += QString("\n3D_OBJECTS_DEBUG <<<<< Resource[%1x%2@%3]: GPU: %4 bytes, ObtainData: %5ms, UploadToGPU: %6ms")
                        .arg(resourceInfo->tileId.x)
                        .arg(resourceInfo->tileId.y)
                        .arg(resourceInfo->zoom)
                        .arg(tileTotalGpuMemoryBytes)
                        .arg(resourceInfo->_performanceDebugInfo.obtainDataTimeMilliseconds, 0, 'f', 2)
                        .arg(resourceInfo->_performanceDebugInfo.uploadToGpuTimeMilliseconds, 0, 'f', 2);
                }
            }
        }
        
        const float totalGpuMemoryMB = static_cast<float>(totalGpuMemoryBytes) / (1024.0f * 1024.0f);
        debugString += QString("3D_OBJECTS_DEBUG <<<<< RenderTime: %1ms, TilesDrawn: %2, TotalGpuMemory: %3 MB")
            .arg(renderTime, 0, 'f', 2)
            .arg(collectedResources.size())
            .arg(totalGpuMemoryMB, 0, 'f', 3);

        LogPrintf(LogSeverityLevel::Info, "%s", qPrintable(debugString));
    }

    if (depthPrepassResult == StageResult::Fail || colorPassResult == StageResult::Fail)
    {
        return StageResult::Fail;
    }

    if (depthPrepassResult == StageResult::Wait || colorPassResult == StageResult::Wait)
    {
        return StageResult::Wait;
    }

    return StageResult::Success;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::release(bool gpuContextLost)
{
    const auto gpuAPI = getGPUAPI();

    if (_vao.isValid())
    {
        gpuAPI->releaseVAO(_vao, gpuContextLost);
        _vao.reset();
    }
    
    if (_depthVao.isValid())
    {
        gpuAPI->releaseVAO(_depthVao, gpuContextLost);
        _depthVao.reset();
    }

    if (_program.id)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;
        _program.id = 0;
    }

    if (_depthProgram.id)
    {
        glDeleteProgram(_depthProgram.id);
        GL_CHECK_RESULT;
        _depthProgram.id = 0;
    }
    return true;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::isDepthPrepassRequired() const
{
    const float buildingAlpha = renderer->get3DBuildingsAlpha();
    return buildingAlpha < 1.0f;
}
