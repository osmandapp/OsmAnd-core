#include "AtlasMapRendererMap3DObjectsStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "OpenGL/GPUAPI_OpenGL.h"
#include "AtlasMapRenderer_OpenGL.h"
#include "Utilities.h"

//// Generate cube geometry with current zoom level
//struct Vertex
//{
//   GLfloat position[3];
//   GLfloat color[4];
//};
//
//// Define a small area around the current camera target for the cube
//static PointI staticTarget = currentState.target31;
//const PointI target31 = currentState.target31;
//const ZoomLevel zoomLevel = currentState.zoomLevel;
//const float tileSizeInWorld = static_cast<float>(AtlasMapRenderer::TileSize3D);
//
//int32_t offset31 = 1000;
//
//// Calculate world coordinates for the 4 corners of the base rectangle
//const PointI bottomLeft31 = PointI(staticTarget.x - offset31, staticTarget.y - offset31);
//const PointI bottomRight31 = PointI(staticTarget.x + offset31, staticTarget.y - offset31);
//const PointI topRight31 = PointI(staticTarget.x + offset31, staticTarget.y + offset31);
//const PointI topLeft31 = PointI(staticTarget.x - offset31, staticTarget.y + offset31);
//
//const glm::vec3 bottomLeft = Utilities::planeWorldCoordinates(
//   bottomLeft31, target31, zoomLevel, tileSizeInWorld, 0.0);
//const glm::vec3 bottomRight = Utilities::planeWorldCoordinates(
//   bottomRight31, target31, zoomLevel, tileSizeInWorld, 0.0);
//const glm::vec3 topRight = Utilities::planeWorldCoordinates(
//   topRight31, target31, zoomLevel, tileSizeInWorld, 0.0);
//const glm::vec3 topLeft = Utilities::planeWorldCoordinates(
//   topLeft31, target31, zoomLevel, tileSizeInWorld, 0.0);
//
//float altitude = 50;
//
//const Vertex vertices[] = {
//   // Bottom face vertices (ground level)
//   {{bottomLeft.x, bottomLeft.y, bottomLeft.z}, {1.0f, 0.0f, 0.0f, 1.0f}},
//   {{bottomRight.x, bottomRight.y, bottomRight.z}, {1.0f, 0.0f, 0.0f, 1.0f}},
//   {{topRight.x, topRight.y, topRight.z}, {1.0f, 0.0f, 0.0f, 1.0f}},
//   {{topLeft.x, topLeft.y, topLeft.z}, {1.0f, 0.0f, 0.0f, 1.0f}},
//
//   // Top face vertices (elevated at 10m)
//   {{bottomLeft.x, altitude, bottomLeft.z}, {0.0f, 1.0f, 0.0f, 1.0f}},
//   {{bottomRight.x, altitude, bottomRight.z}, {0.0f, 1.0f, 0.0f, 1.0f}},
//   {{topRight.x, altitude, topRight.z}, {0.0f, 1.0f, 0.0f, 1.0f}},
//   {{topLeft.x, altitude, topLeft.z}, {0.0f, 1.0f, 0.0f, 1.0f}},
//};
//
//const GLushort indices[] = {
//   // Bottom face (ground level)
//   0, 1, 2,  2, 3, 0,
//   // Top face (elevated)
//   4, 6, 5,  6, 4, 7,
//   // Left face
//   4, 0, 3,  3, 7, 4,
//   // Right face
//   1, 5, 6,  6, 2, 1,
//   // Front face
//   0, 4, 5,  5, 1, 0,
//   // Back face
//   3, 2, 6,  6, 7, 3,
//};

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
            in vec4 in_vs_vertexColor;
            
            uniform mat4 param_mPerspectiveProjectionView;
            
            out vec4 v2f_pointColor;
            
            void main()
            {      
                gl_Position = param_mPerspectiveProjectionView * vec4(in_vs_vertexPosition, 1.0);
                v2f_pointColor = in_vs_vertexColor;
            }
        )";
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = R"(
            
            in vec4 v2f_pointColor;
            
            out vec4 fragColor;
            
            void main()
            {
                fragColor = v2f_pointColor;
            }
        )";
        auto preprocessedFragmentShader = fragmentShader;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        // Read precompiled shaders if available or otherwise compile them and put the binary code in cache if possible
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

    // Get variable locations
    const auto lookup = gpuAPI->obtainVariablesLookupContext(_program.id, variablesMap);
    bool ok = true;
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexPosition, "in_vs_vertexPosition", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.vertexColor, "in_vs_vertexColor", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.param.mPerspectiveProjectionView, "param_mPerspectiveProjectionView", GlslVariableType::Uniform);
    
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

    _vao = gpuAPI->allocateUninitializedVAO();

    return true;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const /*metric*/)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform4fv);
    GL_CHECK_PRESENT(glUniform3fv);
    GL_CHECK_PRESENT(glDrawElements);

    // Activate program
    glUseProgram(_program.id);
    GL_CHECK_RESULT;

    // Set MVP matrix uniform
    glUniformMatrix4fv(*_program.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;

        // Pull 3D objects resources snapshot and render
        const auto& currentState = getRenderer()->getState();
        const auto resourcesCollection_ = getResources().getCollectionSnapshot(
            MapRendererResourceType::Map3DObjects,
            std::static_pointer_cast<IMapDataProvider>(currentState.map3DObjectsProvider));

    if (resourcesCollection_)
    {
        const auto resourcesCollection = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

        // Iterate visible tiles and obtain resource per tile id and zoom
        const auto visibleTiles = static_cast<AtlasMapRenderer*>(getRenderer())->getVisibleTiles();
        const auto zoomLevel = currentState.zoomLevel;

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

                glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(b.vertexBuffer->refInGPU)));
                GL_CHECK_RESULT;
                glEnableVertexAttribArray(*_program.vs.in.vertexPosition);
                GL_CHECK_RESULT;
                glVertexAttribPointer(*_program.vs.in.vertexPosition, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<const GLvoid*>(0));
                GL_CHECK_RESULT;

                glDrawArrays(GL_LINE_LOOP, 0, b.vertexCount);
                GL_CHECK_RESULT;
            }
            r->setState(MapRendererResourceState::Uploaded);
        }
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


