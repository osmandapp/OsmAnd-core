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
            INPUT vec2 in_vs_heights;
            
            %ColorInOutDeclaration%
            
            uniform mat4 param_vs_mPerspectiveProjectionView;
            uniform vec4 param_vs_resultScale;
            uniform ivec2 param_vs_target31;
            uniform int param_vs_zoomLevel;
            uniform float param_vs_metersPerUnit;
            uniform float param_vs_zScaleFactor;
            
            const int MAX_TILE_NUMBER = (1 << 31) - 1;
            const int MIDDLE_TILE_NUMBER = MAX_TILE_NUMBER / 2 + 1;
            
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

            void main()
            {
                float vertexHeight = in_vs_heights.x / param_vs_metersPerUnit;
                float terrainElevation = in_vs_heights.y * param_vs_zScaleFactor / param_vs_metersPerUnit;
                float elevation = terrainElevation + vertexHeight;
        
                ivec2 offset31 = shortestVector31(param_vs_target31, in_vs_location31);
                float tileFactor = 100.0 / float(1 << (31 - param_vs_zoomLevel));
                vec2 offsetFromTarget = vec2(offset31) * tileFactor;
                vec3 worldPos = vec3(offsetFromTarget.x, elevation, offsetFromTarget.y);

                %ColorCalculation%
                
                vec4 v = param_vs_mPerspectiveProjectionView * vec4(worldPos, 1.0);
                gl_Position = v * param_vs_resultScale;
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
            INPUT vec4 in_vs_color;
            INPUT vec4 in_vs_sizes;

            PARAM_OUTPUT highp vec3 v2f_pointPosition;
            PARAM_OUTPUT highp vec3 v2f_pointNormal;
            PARAM_OUTPUT highp vec4 v2f_pointColor;
            PARAM_OUTPUT highp vec4 v2f_sizes;
            PARAM_OUTPUT highp float v2f_height;
        )");
        const QString colorCalculation = QString(R"(
                v2f_sizes = in_vs_sizes;
                v2f_sizes.z = in_vs_sizes.z * tileFactor;
                v2f_sizes.w = in_vs_sizes.w / param_vs_metersPerUnit;
                v2f_height = in_vs_heights.x;
                v2f_pointPosition = worldPos;
                v2f_pointNormal = in_vs_normal;
                v2f_pointColor = in_vs_color;
        )");

        auto vertexShader = vertexShaderBase;
        vertexShader.replace("%ColorInOutDeclaration%", colorInOutDeclaration);
        vertexShader.replace("%ColorCalculation%", colorCalculation);

        const QString fragmentShader = R"(
            PARAM_INPUT highp vec3 v2f_pointPosition;
            PARAM_INPUT highp vec3 v2f_pointNormal;
            PARAM_INPUT highp vec4 v2f_pointColor;
            PARAM_INPUT highp vec4 v2f_sizes;
            PARAM_INPUT highp float v2f_height;
            
            uniform float param_fs_alpha;
            uniform vec3 param_fs_cameraPosition;
            uniform vec3 param_fs_lightDirection;
            
            void main()
            {
                vec3 v = normalize(param_fs_cameraPosition - v2f_pointPosition);
                vec3 n = normalize(v2f_pointNormal);
                float a = atan(n.x, n.z);
                vec2 p = abs(v2f_sizes.zw) + 10.0;
                vec2 g = (pow(v2f_sizes.xy, p) - pow(1.0 - v2f_sizes.xy, p)) * 0.4;
                g.x = dot(-param_fs_lightDirection, n) < 0.0 ? -g.x : g.x;
                bool top = abs(n.y) > abs(n.x) && abs(n.y) > abs(n.z);
                n = top ? n : normalize(vec3(sin(a + g.x), sin(g.y), cos(a + g.x)));
                vec3 r = reflect(param_fs_lightDirection, n);
                float h = pow((clamp(dot(r, v), 0.5, 1.0) - 0.5) * 2.0, 3.0) * 0.2;
                float qa = floor(a * 2.0) * 0.5;
                vec2 s = top ? vec2(0.0, 0.0) : vec2(sin(qa), cos(qa)) + v2f_pointColor.a;
                float d = (dot(-param_fs_lightDirection, n) + 1.0) * 0.5 + 0.25;
                d *= v2f_sizes.z > 0.0 ? fract(sin(dot(s, vec2(12.9898, 78.233))) * 43758.5453) * 0.2 + 0.9 : 1.0;
                d /= 1.0 + exp(-v2f_height * 0.05) * 0.25;
                vec3 dc = clamp(v2f_pointColor.rgb * d, 0.0, 1.0);
                FRAGMENT_COLOR_OUTPUT = vec4(mix(dc, vec3(1.0), h), param_fs_alpha);
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
            _program.id = gpuAPI->linkProgram(
                0, nullptr, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
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
            _program.id = gpuAPI->linkProgram(
                2, shaders, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
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
    ok = ok && lookup->lookupLocation(_program.vs.in.heights, "in_vs_heights", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.normal, "in_vs_normal", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.color, "in_vs_color", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.sizes, "in_vs_sizes", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.zoomLevel, "param_vs_zoomLevel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.zScaleFactor, "param_vs_zScaleFactor", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.alpha, "param_fs_alpha", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.cameraPosition, "param_fs_cameraPosition", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.lightDirection, "param_fs_lightDirection", GlslVariableType::Uniform);

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
        glEnableVertexAttribArray(*_program.vs.in.heights);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_program.vs.in.sizes);
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
        _depthProgram.id = gpuAPI->linkProgram(
            0, nullptr, _depthProgram.binaryCache, _depthProgram.cacheFormat, true, &variablesMap);
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
            _depthProgram.id = gpuAPI->linkProgram(
                0, nullptr, _depthProgram.binaryCache, _depthProgram.cacheFormat, true, &variablesMap);
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
            _depthProgram.id = gpuAPI->linkProgram(
                2, shaders, _depthProgram.binaryCache, _depthProgram.cacheFormat, true, &variablesMap);
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
    ok = ok && lookup->lookupLocation(_depthProgram.vs.in.heights, "in_vs_heights", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.zoomLevel, "param_vs_zoomLevel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_depthProgram.vs.param.zScaleFactor, "param_vs_zScaleFactor", GlslVariableType::Uniform);

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
        glEnableVertexAttribArray(*_depthProgram.vs.in.heights);
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

void AtlasMapRendererMap3DObjectsStage_OpenGL::occupySpace(TileId tileIdN, int zoomLevel, int minZoomLevel,
    QMap<int, QSet<TileId>>& presentTiles, QMap<int, QSet<TileId>>& occupiedSpace) const
{
    if (zoomLevel < minZoomLevel)
        return;
    presentTiles[zoomLevel].insert(tileIdN);
    int zoomShift = zoomLevel - minZoomLevel;
    for (int zoom = minZoomLevel; zoom <= zoomLevel; zoom++)
    {
        occupiedSpace[zoom].insert(TileId::fromXY(tileIdN.x >> zoomShift, tileIdN.y >> zoomShift));
        zoomShift--;
    }
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::spaceAlreadyOccupied(TileId tileIdN, int zoomLevel,
    QMap<int, QSet<TileId>>& presentTiles, QMap<int, QSet<TileId>>& occupiedSpace) const
{
    if (presentTiles.isEmpty())
        return false;
    const auto startZoom = presentTiles.firstKey();
    if (zoomLevel < startZoom)
        return false;
    int zoomShift = zoomLevel - startZoom + 1;
    for (int zoom = startZoom; zoom <= zoomLevel; zoom++)
    {
        zoomShift--;
        if (!presentTiles.contains(zoom))
            continue;
        if (presentTiles[zoom].contains(TileId::fromXY(tileIdN.x >> zoomShift, tileIdN.y >> zoomShift)))
            return true;
    }
    const auto endZoom = occupiedSpace.lastKey();
    for (int zoom = zoomLevel; zoom <= endZoom; zoom++)
    {
        if (occupiedSpace[zoom].contains(tileIdN))
            return true;
        zoomShift--;
    }

    return false;
}

void AtlasMapRendererMap3DObjectsStage_OpenGL::getResourcesInGPU(
    const std::shared_ptr<const IMapRendererResourcesCollection>& resourcesCollection, int detalizationLevel)
{
    const auto& internalState = getInternalState();

    auto minZoomLevel = static_cast<int>(currentState.map3DObjectsProvider->getMinZoom());
    auto maxMissingDataZoomShift = currentState.map3DObjectsProvider->getMaxMissingDataZoomShift();
    auto maxMissingDataUnderZoomShift = currentState.map3DObjectsProvider->getMaxMissingDataUnderZoomShift();

    QMap<int, QSet<TileId>> presentTiles, occupiedSpace;

    bool isLessDetailedLevel = false;
    auto tilesBegin = internalState.visibleTiles.cbegin();
    for (auto itTiles = internalState.visibleTiles.cend(); itTiles != tilesBegin; itTiles--)
    {
        if (isLessDetailedLevel && detalizationLevel < 2)
            break;
        const auto& tilesEntry = itTiles - 1;
        const auto zoomLevel = tilesEntry.key();
        const auto& tiles = tilesEntry.value();
        const auto& citVisibleTilesSet = internalState.visibleTilesSet.constFind(zoomLevel);
        if (citVisibleTilesSet == internalState.visibleTilesSet.cend())
            break;

        // Try to obtain more detailed resource (of higher zoom level) if needed and possible
        int detZoom = internalState.zoomLevelOffset == 0 ? zoomLevel : std::min(static_cast<int>(MaxZoomLevel),
            zoomLevel + std::min(internalState.zoomLevelOffset, maxMissingDataUnderZoomShift));
        for (const auto& tileId : constOf(tiles))
        {
            const auto tileIdN = Utilities::normalizeTileId(tileId, zoomLevel);

            // Don't render invisible tiles
            if (!citVisibleTilesSet.value().contains(tileIdN))
                continue;

            int neededZoom = isLessDetailedLevel || internalState.zoomLevelOffset != 0
                || internalState.extraDetailedTiles.empty() ? detZoom
                : std::min(static_cast<int>(MaxZoomLevel), zoomLevel + std::min(1, maxMissingDataUnderZoomShift));
            if (neededZoom != detZoom && !internalState.extraDetailedTiles.contains(tileIdN))
                neededZoom = detZoom;
            bool haveMatch = false;
            while (neededZoom > zoomLevel && neededZoom >= minZoomLevel)
            {
                const int absZoomShift = neededZoom - zoomLevel;
                const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                    tileIdN,
                    absZoomShift);
                const auto subtilesCount = underscaledTileIdsN.size();

                int count = 0;
                bool allPresent = true;
                auto pUnderscaledTileIdN = underscaledTileIdsN.constData();
                for (auto tileIdx = 0; tileIdx < subtilesCount; tileIdx++)
                {
                    const auto& underscaledTileId = *(pUnderscaledTileIdN++);
                    if (spaceAlreadyOccupied(underscaledTileId, neededZoom, presentTiles, occupiedSpace))
                    {
                        allPresent = false;
                        break;
                    }
                    auto meshInGPU = captureResourceInGPU(
                        resourcesCollection,
                        underscaledTileId,
                        static_cast<ZoomLevel>(neededZoom));
                    if (meshInGPU)
                    {
                        resourcesInGPU.append(qMove(meshInGPU));
                        count++;
                    }
                    else
                    {
                        allPresent = false;
                        break;
                    }
                }
                if (allPresent)
                {
                    for (auto tileIdx = 0; tileIdx < subtilesCount; tileIdx++)
                    {
                        const auto& underscaledTileId = *(pUnderscaledTileIdN++);
                        occupySpace(underscaledTileId, neededZoom, minZoomLevel, presentTiles, occupiedSpace);
                    }
                    haveMatch = true;
                    break;
                }
                else
                {
                    for (; count > 0; count--)
                    {
                        resourcesInGPU.removeLast();
                    }
                }
                neededZoom--;
            }

            if (!haveMatch && zoomLevel >= minZoomLevel
                && !spaceAlreadyOccupied(tileIdN, zoomLevel, presentTiles, occupiedSpace))
            {
                // Try to obtain exact match resource
                auto meshInGPU = captureResourceInGPU(
                    resourcesCollection,
                    tileIdN,
                    zoomLevel);
                if (meshInGPU)
                {
                    occupySpace(tileIdN, zoomLevel, minZoomLevel, presentTiles, occupiedSpace);
                    resourcesInGPU.append(qMove(meshInGPU));
                    haveMatch = true;
                }
            }
            if (!haveMatch)
            {
                // Exact match was not found, so now try to look for overscaled/underscaled resources,
                // giving preference to underscaled resources
                for (int absZoomShift = 1; absZoomShift <= maxMissingDataZoomShift; absZoomShift++)
                {
                    // Look for underscaled first. Only full match is accepted
                    const auto underscaledZoom = static_cast<int>(zoomLevel) + absZoomShift;
                    if (underscaledZoom <= static_cast<int>(MaxZoomLevel) && zoomLevel >= minZoomLevel)
                    {
                        const auto underscaledTileIdsN = Utilities::getTileIdsUnderscaledByZoomShift(
                            tileIdN,
                            absZoomShift);
                        const auto subtilesCount = underscaledTileIdsN.size();

                        bool atLeastOnePresent = false;
                        auto pUnderscaledTileIdN = underscaledTileIdsN.constData();
                        for (auto tileIdx = 0; tileIdx < subtilesCount; tileIdx++)
                        {
                            const auto& underscaledTileId = *(pUnderscaledTileIdN++);
                            if (spaceAlreadyOccupied(underscaledTileId, underscaledZoom, presentTiles, occupiedSpace))
                                continue;
                            auto meshInGPU = captureResourceInGPU(
                                resourcesCollection,
                                underscaledTileId,
                                static_cast<ZoomLevel>(underscaledZoom));
                            if (meshInGPU)
                            {
                                occupySpace(
                                    underscaledTileId, underscaledZoom, minZoomLevel, presentTiles, occupiedSpace);
                                resourcesInGPU.append(qMove(meshInGPU));
                                atLeastOnePresent = true;
                            }
                        }

                        if (atLeastOnePresent)
                            break;
                    }

                    // If underscaled was not found, look for overscaled (surely, if such zoom level exists at all)
                    const auto overscaleZoom = static_cast<int>(zoomLevel) - absZoomShift;
                    if (overscaleZoom >= minZoomLevel)
                    {
                        PointF texCoordsOffset;
                        PointF texCoordsScale;
                        const auto overscaledTileIdN = Utilities::getTileIdOverscaledByZoomShift(
                            tileIdN,
                            absZoomShift,
                            &texCoordsOffset,
                            &texCoordsScale);
                        if (spaceAlreadyOccupied(overscaledTileIdN, overscaleZoom, presentTiles, occupiedSpace))
                            continue;
                        auto meshInGPU = captureResourceInGPU(
                            resourcesCollection,
                            overscaledTileIdN,
                            static_cast<ZoomLevel>(overscaleZoom));
                        if (meshInGPU)
                        {
                            occupySpace(overscaledTileIdN, overscaleZoom, minZoomLevel, presentTiles, occupiedSpace);
                            resourcesInGPU.append(qMove(meshInGPU));
                            break;
                        }
                    }
                }
            }
        }
        isLessDetailedLevel = true;
    }
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::renderDepth(bool primaryOnly)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    glUseProgram(_depthProgram.id);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*_depthProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE,
        glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;
    glUniform4f(*_depthProgram.vs.param.resultScale, 1.0f, currentState.flip ? -1.0f : 1.0f, 1.0f, 1.0f);
    GL_CHECK_RESULT;
    glUniform2i(*_depthProgram.vs.param.target31, currentState.target31.x, currentState.target31.y);
    GL_CHECK_RESULT;
    glUniform1i(*_depthProgram.vs.param.zoomLevel, (int)currentState.zoomLevel);
    GL_CHECK_RESULT;
    glUniform1f(*_depthProgram.vs.param.metersPerUnit, static_cast<float>(internalState.metersPerUnit));
    GL_CHECK_RESULT;
    glUniform1f(*_depthProgram.vs.param.zScaleFactor, currentState.elevationConfiguration.zScaleFactor);
    GL_CHECK_RESULT;

    gpuAPI->useVAO(_depthVao);

    for (const auto& resource : resourcesInGPU)
    {
        if (resource->indexBuffer && resource->indexBuffer->itemsCount > 0)
        {
            int startIndex = 0;
            int indexCount = resource->indexBuffer->itemsCount;
            if (resource->partSizes->size() > 0)
            {
                if (primaryOnly)
                {
                    startIndex = resource->partSizes->front().second;
                    indexCount -= startIndex;
                }
                else
                    indexCount = resource->partSizes->front().second;
            }
            else if (primaryOnly)
                continue;

            glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(
                resource->vertexBuffer->refInGPU)));
            GL_CHECK_RESULT;
            auto asd = reinterpret_cast<uintptr_t>(resource->indexBuffer->refInGPU);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(
                resource->indexBuffer->refInGPU)));
            GL_CHECK_RESULT;

            glVertexAttribIPointer(*_depthProgram.vs.in.location31, 2, GL_INT,
                sizeof(BuildingVertex), reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, location31)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_depthProgram.vs.in.heights, 2, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, heights)));
            GL_CHECK_RESULT;

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                GL_UNSIGNED_SHORT, (void*) (startIndex * sizeof(uint16_t)));
            GL_CHECK_RESULT;
        }
    }
    gpuAPI->unuseVAO();

    return StageResult::Success;
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::renderColor(bool primaryOnly)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    float buildingAlpha = renderer->get3DBuildingsAlpha();

//    const auto zenith = glm::radians(currentState.elevationConfiguration.hillshadeSunAngle);
//    const auto cosZenith = qCos(zenith);
//    const auto sinZenith = qSin(zenith);
//    const auto azimuth = M_PI - glm::radians(currentState.elevationConfiguration.hillshadeSunAzimuth);
//    const auto cosAzimuth = qCos(azimuth);
//    const auto sinAzimuth = qSin(azimuth);
//
//    const glm::vec3 lightDir = glm::normalize(glm::vec3(sinAzimuth * cosZenith, -sinZenith, cosAzimuth * cosZenith));

    const auto zenith = glm::radians(60.0);
    const auto cosZenith = qCos(zenith);
    const auto sinZenith = qSin(zenith);
    const auto azimuth = glm::radians(45.0);
    const auto cosAzimuth = qCos(azimuth);
    const auto sinAzimuth = qSin(azimuth);
    
    const glm::vec3 lightDir = glm::normalize(glm::vec3(sinAzimuth * cosZenith, -sinZenith, cosAzimuth * cosZenith));

    glUseProgram(_program.id);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*_program.vs.param.mPerspectiveProjectionView, 1, GL_FALSE,
        glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;
    glUniform4f(*_program.vs.param.resultScale, 1.0f, currentState.flip ? -1.0f : 1.0f, 1.0f, 1.0f);
    GL_CHECK_RESULT;
    glUniform2i(*_program.vs.param.target31, currentState.target31.x, currentState.target31.y);
    GL_CHECK_RESULT;
    glUniform1i(*_program.vs.param.zoomLevel, (int)currentState.zoomLevel);
    GL_CHECK_RESULT;
    glUniform1f(*_program.vs.param.metersPerUnit, static_cast<float>(internalState.metersPerUnit));
    GL_CHECK_RESULT;
    glUniform1f(*_program.vs.param.zScaleFactor, currentState.elevationConfiguration.zScaleFactor);
    GL_CHECK_RESULT;
    glUniform1f(*_program.fs.param.alpha, buildingAlpha);
    GL_CHECK_RESULT;
    glUniform3f(*_program.fs.param.cameraPosition,
        internalState.worldCameraPosition.x,
        internalState.worldCameraPosition.y,
        internalState.worldCameraPosition.z);
    GL_CHECK_RESULT;
    glUniform3f(*_program.fs.param.lightDirection, lightDir.x, lightDir.y, lightDir.z);
    GL_CHECK_RESULT;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    gpuAPI->useVAO(_vao);

    for (const auto& resource : resourcesInGPU)
    {
        if (resource->indexBuffer && resource->indexBuffer->itemsCount > 0)
        {
            int startIndex = 0;
            int indexCount = resource->indexBuffer->itemsCount;
            if (resource->partSizes->size() > 0)
            {
                if (primaryOnly)
                {
                    startIndex = resource->partSizes->front().second;
                    indexCount -= startIndex;
                }
                else
                    indexCount = resource->partSizes->front().second;
            }
            else if (primaryOnly)
                continue;

            glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(
                resource->vertexBuffer->refInGPU)));
            GL_CHECK_RESULT;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(
                resource->indexBuffer->refInGPU)));
            GL_CHECK_RESULT;

            glVertexAttribIPointer(*_program.vs.in.location31, 2, GL_INT, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, location31)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_program.vs.in.heights, 2, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, heights)));
            GL_CHECK_RESULT;

            glVertexAttribPointer(*_program.vs.in.normal, 3, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, normal)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_program.vs.in.color, 4, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, color)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_program.vs.in.sizes, 4, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, sizes)));
            GL_CHECK_RESULT;

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                GL_UNSIGNED_SHORT, (void*) (startIndex * sizeof(uint16_t)));
            GL_CHECK_RESULT;
        }
    }
    gpuAPI->unuseVAO();

    return StageResult::Success;
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::render(
    IMapRenderer_Metrics::Metric_renderFrame* const metric)
{
    bool ok = true;

    if (_init3DObjectsType != Init3DObjectsType::Complete)
    {
        const auto init3DObjectsType = _init3DObjectsType;
        ok = ok && (init3DObjectsType != Init3DObjectsType::Objects3DDepth || initializeDepthProgram());
        ok = ok && (init3DObjectsType != Init3DObjectsType::Objects3DColor || initializeColorProgram());

        if (!ok || _init3DObjectsType == Init3DObjectsType::Incomplete)
            return StageResult::Fail;

        return StageResult::Wait;
    }

    const auto resourcesCollection = getResources().getCollectionSnapshot(MapRendererResourceType::Map3DObjects,
        std::static_pointer_cast<IMapDataProvider>(currentState.map3DObjectsProvider));

    if (!resourcesCollection)
        return StageResult::Success;

    const float buildingsAlpha = renderer->get3DBuildingsAlpha();
    const int detalizationLevel = renderer->get3DBuildingsDetalization();

    resourcesInGPU.clear();

    getResourcesInGPU(resourcesCollection, detalizationLevel);

    if (resourcesInGPU.isEmpty())
    {
        return StageResult::Success;
    }

    const bool needsDepthPrepass = buildingsAlpha > 0.0 && buildingsAlpha < 1.0f;

    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;
    glDepthMask(GL_TRUE);
    GL_CHECK_RESULT;
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

        depthPrepassResult = renderDepth(true);

        glEnable(GL_BLEND);
        GL_CHECK_RESULT;

        glDepthMask(GL_FALSE);
        GL_CHECK_RESULT;
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    GL_CHECK_RESULT;

    auto colorPassResult = renderColor(true);

    if (needsDepthPrepass)
    {
        glDepthMask(GL_TRUE);
        GL_CHECK_RESULT;
    }

    bool failed = depthPrepassResult == StageResult::Fail || colorPassResult == StageResult::Fail;

    if (needsDepthPrepass && !failed)
    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        GL_CHECK_RESULT;

        glDisable(GL_BLEND);
        GL_CHECK_RESULT;

        depthPrepassResult = renderDepth(false);

        glEnable(GL_BLEND);
        GL_CHECK_RESULT;

        glDepthMask(GL_FALSE);
        GL_CHECK_RESULT;
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    GL_CHECK_RESULT;

    if (!failed)
        colorPassResult = renderColor(false);

    if (needsDepthPrepass)
    {
        glDepthMask(GL_TRUE);
        GL_CHECK_RESULT;
    }

    glDisable(GL_CULL_FACE);
    GL_CHECK_RESULT;

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

std::shared_ptr<const GPUAPI::MeshInGPU> AtlasMapRendererMap3DObjectsStage_OpenGL::captureResourceInGPU(
    const std::shared_ptr<const IMapRendererResourcesCollection>& resourcesCollection_,
    TileId normalizedTileId,
    ZoomLevel zoomLevel) const
{
    const auto& resourcesCollection =
        std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

    // Obtain tile entry by normalized tile coordinates, since tile may repeat several times
    std::shared_ptr<MapRendererBaseTiledResource> resource_;
    if (resourcesCollection->obtainResource(normalizedTileId, zoomLevel, resource_))
    {
        const auto resource = std::static_pointer_cast<MapRenderer3DObjectsResource>(resource_);

        // Check state and obtain GPU resource
        auto state = resource->getState();
        if (state == MapRendererResourceState::Uploaded
            || state == MapRendererResourceState::PreparingRenew
            || state == MapRendererResourceState::PreparedRenew
            || state == MapRendererResourceState::Outdated
            || state == MapRendererResourceState::Renewing
            || state == MapRendererResourceState::Updating
            || state == MapRendererResourceState::RequestedUpdate
            || state == MapRendererResourceState::ProcessingUpdate
            || state == MapRendererResourceState::ProcessingUpdateWhileRenewing
            || state == MapRendererResourceState::UpdatingCancelledWhileBeingProcessed)
        {
            // Capture GPU resource
            std::shared_ptr<const GPUAPI::MeshInGPU> meshInGPU;
            resource->captureResourceInGPU(meshInGPU);
            return meshInGPU;
        }
    }

    return nullptr;
}
