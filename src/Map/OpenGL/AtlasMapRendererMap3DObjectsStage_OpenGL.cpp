#include "AtlasMapRendererMap3DObjectsStage_OpenGL.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "OpenGL/GPUAPI_OpenGL.h"
#include "AtlasMapRenderer_OpenGL.h"
#include "Utilities.h"

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
            
            uniform mat4 param_mPerspectiveProjectionView;
            
            void main()
            {
                gl_Position = param_mPerspectiveProjectionView * vec4(in_vs_vertexPosition, 1.0);
            }
        )";
        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        const QString fragmentShader = R"(
                        
            out vec4 fragColor;
            
            void main()
            {
                fragColor = vec4(1.0, 0.0, 0.0, 1.0);
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
    gpuAPI->initializeVAO(_vao);

    return true;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::render(IMapRenderer_Metrics::Metric_renderFrame* const /*metric*/)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const PointI target31 = currentState.target31;
    const ZoomLevel zoomLevel = currentState.zoomLevel;
    const float tileSizeInWorld = static_cast<float>(AtlasMapRenderer::TileSize3D);

    GL_CHECK_PRESENT(glUseProgram);
    GL_CHECK_PRESENT(glUniformMatrix4fv);
    GL_CHECK_PRESENT(glUniform4fv);
    GL_CHECK_PRESENT(glUniform3fv);
    GL_CHECK_PRESENT(glDrawArrays);
    GL_CHECK_PRESENT(glGenBuffers);
    GL_CHECK_PRESENT(glBindBuffer);
    GL_CHECK_PRESENT(glBufferData);
    GL_CHECK_PRESENT(glEnableVertexAttribArray);
    GL_CHECK_PRESENT(glVertexAttribPointer);
    GL_CHECK_PRESENT(glDeleteBuffers);

    glUseProgram(_program.id);
    GL_CHECK_RESULT;

    glUniformMatrix4fv(*_program.vs.param.mPerspectiveProjectionView, 1, GL_FALSE, glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;

    const auto& currentState = getRenderer()->getState();
    const auto resourcesCollection_ = getResources().getCollectionSnapshot(
        MapRendererResourceType::Map3DObjects,
        std::static_pointer_cast<IMapDataProvider>(currentState.map3DObjectsProvider));

    if (resourcesCollection_)
    {
        const auto resourcesCollection = std::static_pointer_cast<const MapRendererTiledResourcesCollection::Snapshot>(resourcesCollection_);

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

                const int vertexCount = b.debugPoints31.size();
                const size_t vertexBufferSize = static_cast<size_t>(vertexCount * sizeof(MapRenderer3DObjectsResource::Vertex));

                QVector<MapRenderer3DObjectsResource::Vertex> vertexData(vertexCount);
                for (int i = 0; i < vertexCount; ++i)
                {
                    const auto& point31 = b.debugPoints31[i];

                    vertexData[i].position = Utilities::planeWorldCoordinates(point31, target31, zoomLevel, tileSizeInWorld, 1.0);
                }

                gpuAPI->useVAO(_vao);
                GLuint vbo = 0;
                glGenBuffers(1, &vbo);
                GL_CHECK_RESULT;
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                GL_CHECK_RESULT;
                glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexBufferSize), vertexData.constData(), GL_DYNAMIC_DRAW);
                GL_CHECK_RESULT;

                glEnableVertexAttribArray(*_program.vs.in.vertexPosition);
                GL_CHECK_RESULT;
                glVertexAttribPointer(*_program.vs.in.vertexPosition,
                    3, GL_FLOAT, GL_FALSE,
                    sizeof(MapRenderer3DObjectsResource::Vertex),
                    reinterpret_cast<const GLvoid*>(offsetof(MapRenderer3DObjectsResource::Vertex, position)));
                GL_CHECK_RESULT;

                glDrawArrays(GL_LINE_LOOP, 0, vertexCount);
                GL_CHECK_RESULT;

                glDeleteBuffers(1, &vbo);
                GL_CHECK_RESULT;
                gpuAPI->unuseVAO();
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


