#include "AtlasMapRendererMap3DObjectsStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "OpenGL/GPUAPI_OpenGL.h"
#include "AtlasMapRenderer_OpenGL.h"
#include "Utilities.h"
#include "MapRendererTiledResourcesCollection.h"
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
    const auto gpuAPI = getGPUAPI();
    
    QHash< QString, GPUAPI_OpenGL::GlslProgramVariable > variablesMap;
    _program.id = 0;
    if (!_program.binaryCache.isEmpty())
        _program.id = gpuAPI->linkProgram(0, nullptr, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
    if (!_program.id.isValid())
    {
        const QString vertexShader = R"(
            in vec3 in_vs_vertexPosition;
            
            out vec4 v2f_color;
            
            uniform mat4 param_vs_mPerspectiveProjectionView;
            uniform float param_vs_metersPerUnit;
            uniform vec4 param_vs_color;

            void main()
            {
                vec4 v;
                v.x = in_vs_vertexPosition.x;
                v.y = in_vs_vertexPosition.y / param_vs_metersPerUnit;  // Normalize Y to world units
                v.z = in_vs_vertexPosition.z;
                v.w = 1.0;

                v = param_vs_mPerspectiveProjectionView * v;
                gl_Position = v;
                
                v2f_color = param_vs_color;
            }
        )";
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = R"(
            in vec4 v2f_color;
            out vec4 fragColor;
            
            void main()
            {
                fragColor = v2f_color;
            }
        )";
        auto preprocessedFragmentShader = fragmentShader;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        _program.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, "", _program.cacheFormat);

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
                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererMap3DObjectsStage_OpenGL vertex shader");
                return false;
            }
            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error,
                    "Failed to compile AtlasMapRendererMap3DObjectsStage_OpenGL fragment shader");
                return false;
            }
            const GLuint shaders[] = { vsId, fsId };
            _program.id = gpuAPI->linkProgram(
                2, shaders, _program.binaryCache, _program.cacheFormat, true, &variablesMap);
        }
    }

    const auto lookup = gpuAPI->obtainVariablesLookupContext(_program.id, variablesMap);
    bool ok = true;
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.color, "param_vs_color", GlslVariableType::Uniform);

    return ok && _program.id.isValid();
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initialize()
{
    const auto gpuAPI = getGPUAPI();

    // Initialize program first
    if (!initializeProgram())
        return false;

    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform4fv);
    GL_CHECK_PRESENT(glUniform3fv);
    GL_CHECK_PRESENT(glDrawArrays);
    GL_CHECK_PRESENT(glDrawElements);
    GL_CHECK_PRESENT(glDeleteBuffers);
    GL_CHECK_PRESENT(glEnable);
    GL_CHECK_PRESENT(glDisable);
    GL_CHECK_PRESENT(glBlendFunc);
    //GL_CHECK_PRESENT(glDrawElements);

    _vao = gpuAPI->allocateUninitializedVAO();
    gpuAPI->initializeVAO(_vao);

    return true;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const /*metric*/)
{
    const auto& currentState = getRenderer()->getState();
    const auto resourcesCollection_ = getResources().getCollectionSnapshot(MapRendererResourceType::Map3DObjects,
        std::static_pointer_cast<IMapDataProvider>(currentState.map3DObjectsProvider));

    if (!resourcesCollection_)
    {
        return false;
    }

    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    glUseProgram(_program.id);
    GL_CHECK_RESULT;

    glUniformMatrix4fv(*_program.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;

    const auto resourcesCollection = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

    const auto visibleTiles = static_cast<AtlasMapRenderer*>(getRenderer())->getVisibleTiles();
    const auto zoomLevel = currentState.zoomLevel;
    const PointI target31 = currentState.target31;
    const float tileSizeInWorld = static_cast<float>(AtlasMapRenderer::TileSize3D);

    for (const auto& tileId : constOf(visibleTiles))
    {
        const auto tileIdN = Utilities::normalizeTileId(tileId, zoomLevel);
        std::shared_ptr<MapRendererBaseTiledResource> resource_;
        if (!resourcesCollection->obtainResource(tileIdN, zoomLevel, resource_))
            continue;
        const auto r = std::static_pointer_cast<MapRenderer3DObjectsResource>(resource_);
        if (!r)
            continue;
        if (!r->setStateIf(MapRendererResourceState::Uploaded, MapRendererResourceState::IsBeingUsed))
            continue;

            const auto& buildings = r->getTestBuildings();
            for (const auto& b : constOf(buildings))
            {
                if (!b.vertexBuffer || b.vertexCount <= 0)
                    continue;

                auto debugColor = b.debugColor;

                // Generate random color for this building
                glUniform4f(*_program.vs.param.color, debugColor.r, debugColor.g, debugColor.b, 1.0f);
                GL_CHECK_RESULT;

            const int edgePointsCount = b.debugPoints31.size();
            if (edgePointsCount < 3)
                continue;


            // Generate extruded 3D mesh
            const float bottomAltitude = -1000.0f;
            const float topAltitude = b.height;

            // Create vertices for extruded mesh
            QVector<MapRenderer3DObjectsResource::Vertex> extrudedVertices;
            QVector<GLushort> extrudedIndices;

            double metersPerUnit;

            // Generate bottom face vertices (for side walls only)
            QVector<MapRenderer3DObjectsResource::Vertex> bottomVertices(edgePointsCount);
            for (int i = 0; i < edgePointsCount; ++i)
            {
                const auto& point31 = b.debugPoints31[i];
                bottomVertices[i].position = Utilities::planeWorldCoordinates(point31, target31, zoomLevel, tileSizeInWorld, bottomAltitude);

                PointF offsetInTileN;
                const auto tileId = Utilities::normalizeTileId(
                    Utilities::getTileId(point31, currentState.zoomLevel, &offsetInTileN), currentState.zoomLevel);


                // Get elevation data
                metersPerUnit = Utilities::getMetersPerTileUnit(currentState.zoomLevel, tileId.y, AtlasMapRenderer::TileSize3D);
                glUniform1f(*_program.vs.param.metersPerUnit, static_cast<float>(metersPerUnit));
                GL_CHECK_RESULT;
            }

            // Generate top face vertices
            QVector<MapRenderer3DObjectsResource::Vertex> topVertices(edgePointsCount);
            for (int i = 0; i < edgePointsCount; ++i)
            {
                const auto& point31 = b.debugPoints31[i];
                topVertices[i].position = Utilities::planeWorldCoordinates(point31, target31, zoomLevel, tileSizeInWorld, topAltitude);
            }

            // Triangulate top face using earcut
            std::vector<std::array<double, 2>> topPolygon;
            topPolygon.reserve(static_cast<size_t>(edgePointsCount));
            for (int i = 0; i < edgePointsCount; ++i)
            {
                const auto& p = topVertices[i].position;
                topPolygon.push_back({ static_cast<double>(p.x), static_cast<double>(p.z) });
            }
            std::vector< std::vector<std::array<double, 2>> > polygon;
            polygon.push_back(std::move(topPolygon));

            std::vector<uint32_t> topIndices32 = mapbox::earcut<uint32_t>(polygon);
            if (topIndices32.empty())
                continue;

            // Add top face vertices to extruded mesh
            int vertexOffset = 0;
            for (const auto& vertex : topVertices)
            {
                extrudedVertices.append(vertex);
            }

            // Add top face indices (same winding as original)
            for (uint32_t idx : topIndices32)
            {
                extrudedIndices.append(static_cast<GLushort>(idx + vertexOffset));
            }

            // Generate side wall faces
            vertexOffset = extrudedVertices.size();
            for (int i = 0; i < edgePointsCount; ++i)
            {
                int next = (i + 1) % edgePointsCount;

                // Add vertices for this side wall quad
                extrudedVertices.append(bottomVertices[i]);     // bottom-left
                extrudedVertices.append(topVertices[i]);       // top-left
                extrudedVertices.append(topVertices[next]);     // top-right
                extrudedVertices.append(bottomVertices[next]);  // bottom-right

                // Add triangle indices for the quad (two triangles)
                int baseIdx = vertexOffset + i * 4;
                // First triangle
                extrudedIndices.append(static_cast<GLushort>(baseIdx));
                extrudedIndices.append(static_cast<GLushort>(baseIdx + 1));
                extrudedIndices.append(static_cast<GLushort>(baseIdx + 2));
                // Second triangle
                extrudedIndices.append(static_cast<GLushort>(baseIdx));
                extrudedIndices.append(static_cast<GLushort>(baseIdx + 2));
                extrudedIndices.append(static_cast<GLushort>(baseIdx + 3));
            }

            const size_t vertexBufferSize = static_cast<size_t>(extrudedVertices.size() * sizeof(MapRenderer3DObjectsResource::Vertex));
            const size_t indexBufferSize = static_cast<size_t>(extrudedIndices.size() * sizeof(GLushort));

            gpuAPI->useVAO(_vao);
            GLuint vbo = 0;
            GLuint ebo = 0;
            glGenBuffers(1, &vbo);
            GL_CHECK_RESULT;
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            GL_CHECK_RESULT;
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexBufferSize), extrudedVertices.constData(), GL_DYNAMIC_DRAW);
            GL_CHECK_RESULT;
            glGenBuffers(1, &ebo);
            GL_CHECK_RESULT;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            GL_CHECK_RESULT;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indexBufferSize), extrudedIndices.constData(), GL_DYNAMIC_DRAW);
            GL_CHECK_RESULT;

            glEnableVertexAttribArray(*_program.vs.in.vertexPosition);
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_program.vs.in.vertexPosition,
                                  3, GL_FLOAT, GL_FALSE,
                                  sizeof(MapRenderer3DObjectsResource::Vertex),
                                  reinterpret_cast<const GLvoid*>(offsetof(MapRenderer3DObjectsResource::Vertex, position)));
            GL_CHECK_RESULT;

            // Transparency
            glEnable(GL_BLEND);
            GL_CHECK_RESULT;
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            GL_CHECK_RESULT;

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(extrudedIndices.size()), GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(0));
            GL_CHECK_RESULT;

            // Transparency
            glDisable(GL_BLEND);
            GL_CHECK_RESULT;

            glDeleteBuffers(1, &vbo);
            GL_CHECK_RESULT;
            glDeleteBuffers(1, &ebo);
            GL_CHECK_RESULT;
            gpuAPI->unuseVAO();
        }
        r->setState(MapRendererResourceState::Uploaded);
    }

    return true;
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


