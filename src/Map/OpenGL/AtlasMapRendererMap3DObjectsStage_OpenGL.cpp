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

#define BUILDINGS_FADE_ANIMATION_PERIOD 500
// Shadows fade in while the sun rises from the horizon up to this angle (degrees)
#define BUILDINGS_SHADOW_FADE_SUN_ANGLE 6.0f
#define BUILDINGS_SHADOW_MAX_ALPHA 0.3f
// Soft shadow edges: the shadow is drawn several times with the sun shifted around a small disc
#define BUILDINGS_SHADOW_SAMPLES 10
#define BUILDINGS_SHADOW_SPREAD_ANGLE 2.5f
// Shadows of buildings on buildings: depth texture seen from the sun
#define BUILDINGS_SHADOW_MAP_SIZE 2048
#define BUILDINGS_SHADOW_MAP_RANGE_FACTOR 1.5f

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

            %ShadowMapDeclaration%

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

                %ShadowMapCalculation%
                
                vec4 v = param_vs_mPerspectiveProjectionView * vec4(worldPos, 1.0);
                gl_Position = v * param_vs_resultScale;
            }
        )");
}

namespace
{
    const QString shadowMapVertexDeclaration = QString(R"(
            uniform mat4 param_vs_lightMatrix;
            uniform float param_vs_shadowNormalOffset;
            PARAM_OUTPUT highp vec4 v2f_shadowCoord;
        )");
    const QString shadowMapVertexCalculation = QString(R"(
                // Normal offset keeps a surface from shadowing itself in the coarse shadow map
                v2f_shadowCoord = param_vs_lightMatrix
                    * vec4(worldPos + normalize(in_vs_normal) * param_vs_shadowNormalOffset, 1.0);
        )");
    const QString shadowMapFragmentDeclaration = QString(R"(
            PARAM_INPUT highp vec4 v2f_shadowCoord;
            uniform float param_fs_shadowStrength;
        #if __VERSION__ >= 130
            uniform highp sampler2D param_fs_shadowMap;

            float sunVisibilityAt(highp vec2 uv, highp float depth)
            {
                return depth <= texture(param_fs_shadowMap, uv).r ? 1.0 : 0.0;
            }

            // 1.0 when the sun sees this point, 0.0 when another building covers it
            float sunVisibility()
            {
                highp vec3 c = v2f_shadowCoord.xyz;
                if (param_fs_shadowStrength <= 0.0 || c.x <= 0.0 || c.x >= 1.0 || c.y <= 0.0 || c.y >= 1.0 || c.z >= 1.0)
                    return 1.0;
                highp float t = 1.0 / %ShadowMapSize%.0;
                float v = sunVisibilityAt(c.xy + vec2(-t, -t), c.z);
                v += sunVisibilityAt(c.xy + vec2(t, -t), c.z);
                v += sunVisibilityAt(c.xy + vec2(-t, t), c.z);
                v += sunVisibilityAt(c.xy + vec2(t, t), c.z);
                // Fade out at the border of the shadow map
                float edge = min(min(c.x, 1.0 - c.x), min(c.y, 1.0 - c.y));
                return mix(1.0, v * 0.25, clamp(edge * 20.0, 0.0, 1.0));
            }
        #else
            float sunVisibility()
            {
                return 1.0;
            }
        #endif
        )");
}

AtlasMapRendererMap3DObjectsStage_OpenGL::AtlasMapRendererMap3DObjectsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer)
    : AtlasMapRendererMap3DObjectsStage(renderer)
    , AtlasMapRendererStageHelper_OpenGL(this)
    , actualTiles(&_firstTiles)
    , actualSpace(&_firstSpace)
    , oldTiles(&_secondTiles)
    , oldSpace(&_secondSpace)
    , _shadowMapTexture(0)
    , _shadowMapFramebuffer(0)
    , _shadowMapFailed(false)
    , _shadowMapStrength(0.0f)
    , _shadowMapNormalOffset(0.0f)
{
}

AtlasMapRendererMap3DObjectsStage_OpenGL::~AtlasMapRendererMap3DObjectsStage_OpenGL()
{
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initializeSimpleProgram()
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

            PARAM_OUTPUT highp vec3 v2f_pointNormal;
            PARAM_OUTPUT highp vec4 v2f_pointColor;
        )");
        const QString colorCalculation = QString(R"(
                v2f_pointNormal = in_vs_normal;
                vec3 color = in_vs_color.rgb;
                float b = dot(color, vec3(0.2126, 0.7152, 0.0722)) / 0.2;
                color = b > 1.0 ? color : (b > 0.0 ? color / b : vec3(0.2));
                v2f_pointColor = vec4(clamp(color, 0.0, 1.0), in_vs_color.a);
        )");

        auto vertexShader = vertexShaderBase;
        vertexShader.replace("%ColorInOutDeclaration%", colorInOutDeclaration);
        vertexShader.replace("%ColorCalculation%", colorCalculation);
        vertexShader.replace("%ShadowMapDeclaration%", shadowMapVertexDeclaration);
        vertexShader.replace("%ShadowMapCalculation%", shadowMapVertexCalculation);

        QString fragmentShader = R"(
            PARAM_INPUT highp vec3 v2f_pointNormal;
            PARAM_INPUT highp vec4 v2f_pointColor;
            
            uniform float param_fs_alpha;
            uniform vec3 param_fs_lightDirection;

            %ShadowMapDeclaration%
            
            void main()
            {
                vec3 n = normalize(v2f_pointNormal);
                float d = dot(-param_fs_lightDirection, n);
                // A sunlit side hidden behind another building gets the light of a side turned away
                d = d > 0.0 ? d * mix(1.0, sunVisibility(), param_fs_shadowStrength) : d;
                d = ((d < 0.0 ? -(d * d) : d * d) + 1.0) * 0.5 + 0.1;
                vec3 color = v2f_pointColor.rgb * d;
                FRAGMENT_COLOR_OUTPUT = vec4(clamp(color, 0.0, 1.0), param_fs_alpha);
            }
        )";

        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        fragmentShader.replace("%ShadowMapDeclaration%", shadowMapFragmentDeclaration);
        fragmentShader.replace("%ShadowMapSize%", QString::number(BUILDINGS_SHADOW_MAP_SIZE));
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
                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects simple vertex shader");
                return false;
            }

            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects simple fragment shader");
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
            "Failed to link Map3DObjects simple shader program");
        return false;
    }

    const auto lookup = gpuAPI->obtainVariablesLookupContext(_program.id, variablesMap);
    bool ok = true;
    ok = ok && lookup->lookupLocation(_program.vs.in.location31, "in_vs_location31", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.heights, "in_vs_heights", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.normal, "in_vs_normal", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.in.color, "in_vs_color", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_program.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.zoomLevel, "param_vs_zoomLevel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.zScaleFactor, "param_vs_zScaleFactor", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.alpha, "param_fs_alpha", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.lightDirection, "param_fs_lightDirection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.lightMatrix, "param_vs_lightMatrix", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.vs.param.shadowNormalOffset, "param_vs_shadowNormalOffset", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_program.fs.param.shadowStrength, "param_fs_shadowStrength", GlslVariableType::Uniform);
    // Absent without GLSL ES 3.0: the shaders then ignore shadows of buildings on buildings
    _program.withShadowMap = ok && lookup->lookupLocation(_program.fs.param.shadowMap, "param_fs_shadowMap", GlslVariableType::Uniform);

    if (!ok)
    {
        glDeleteProgram(_program.id);
        GL_CHECK_RESULT;

        _program.id.reset();

        LogPrintf(LogSeverityLevel::Error,
            "Failed to find variable in Map3DObjects simple shader program");
        return false;
    }

    if (_vao.isValid())
    {
        gpuAPI->useVAO(_vao);
        
        glEnableVertexAttribArray(*_program.vs.in.location31);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_program.vs.in.heights);
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

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initializeColorProgram()
{
    const auto nextInit3DobjectsType = static_cast<Init3DObjectsType>(static_cast<int>(_init3DObjectsType) + 1);
    _init3DObjectsType = Init3DObjectsType::Incomplete;

    const auto gpuAPI = getGPUAPI();
    
    QHash<QString, GPUAPI_OpenGL::GlslProgramVariable> variablesMap;
    _colorProgram.id = 0;

    if (!_colorProgram.binaryCache.isEmpty())
    {
        _colorProgram.id = gpuAPI->linkProgram(
            0, nullptr, _colorProgram.binaryCache, _colorProgram.cacheFormat, true, &variablesMap);
    }

    if (!_colorProgram.id.isValid())
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
                v2f_sizes.xy = in_vs_sizes.xy;
                v2f_sizes.z = in_vs_sizes.z * tileFactor;
                v2f_sizes.w = in_vs_sizes.w / param_vs_metersPerUnit;
                v2f_height = in_vs_heights.x;
                v2f_pointPosition = worldPos;
                v2f_pointNormal = in_vs_normal;
                vec3 color = in_vs_color.rgb;
                float b = dot(color, vec3(0.2126, 0.7152, 0.0722)) / 0.2;
                color = b > 1.0 ? color : (b > 0.0 ? color / b : vec3(0.2));
                v2f_pointColor = vec4(clamp(color, 0.0, 1.0), in_vs_color.a);
        )");

        auto vertexShader = vertexShaderBase;
        vertexShader.replace("%ColorInOutDeclaration%", colorInOutDeclaration);
        vertexShader.replace("%ColorCalculation%", colorCalculation);
        vertexShader.replace("%ShadowMapDeclaration%", shadowMapVertexDeclaration);
        vertexShader.replace("%ShadowMapCalculation%", shadowMapVertexCalculation);

        QString fragmentShader = R"(
            PARAM_INPUT highp vec3 v2f_pointPosition;
            PARAM_INPUT highp vec3 v2f_pointNormal;
            PARAM_INPUT highp vec4 v2f_pointColor;
            PARAM_INPUT highp vec4 v2f_sizes;
            PARAM_INPUT highp float v2f_height;
            
            uniform float param_fs_alpha;
            uniform float param_fs_fadeHeight;
            uniform vec3 param_fs_cameraPosition;
            uniform vec3 param_fs_lightDirection;

            %ShadowMapDeclaration%
            
            void main()
            {
                float sunVisible = mix(1.0, sunVisibility(), param_fs_shadowStrength);
                vec3 v = normalize(param_fs_cameraPosition - v2f_pointPosition);
                vec3 n = normalize(v2f_pointNormal);
                bool top = abs(n.y) > 0.0;
                float a = atan(n.x, n.z);
                vec2 p = abs(v2f_sizes.zw) + 10.0;
                vec2 g = clamp(pow(v2f_sizes.xy, p) - pow(1.0 - v2f_sizes.xy, p), 0.0, 1.0) * 0.4;
                g.x = dot(-param_fs_lightDirection, n) < 0.0 ? -g.x : g.x;
                n = top ? n : normalize(vec3(sin(a + g.x), sin(g.y), cos(a + g.x)));
                vec3 r = reflect(param_fs_lightDirection, n);
                float h = pow((clamp(dot(r, v), 0.5, 1.0) - 0.5) * 2.0, 3.0) * 0.2 * sunVisible;
                float qa = floor(a * 2.0) * 0.5;
                vec2 s = top ? vec2(0.0, 0.0) : vec2(sin(qa), cos(qa)) + v2f_pointColor.a;
                float d = dot(-param_fs_lightDirection, n);
                // A sunlit side hidden behind another building gets the light of a side turned away
                d = d > 0.0 ? d * sunVisible : d;
                d = ((d < 0.0 ? -(d * d) : d * d) + 1.0) * 0.5 + 0.1;
                d *= !top && v2f_sizes.z > 0.0 ? fract(sin(dot(s, vec2(12.9898, 78.233))) * 43758.5453) * 0.14 + 0.93 : 1.0;
                d /= 1.0 + exp(-v2f_height * 0.05) * 0.2;
                d *= 1.0 + pow(1.0 - clamp(dot(n, v), 0.0, 1.0), 3.0) * 0.2;
                vec3 color = mix(v2f_pointColor.rgb * d, vec3(1.0), h);
                float alpha = param_fs_alpha * min(param_fs_fadeHeight / v2f_height, 1.0);
                FRAGMENT_COLOR_OUTPUT = vec4(clamp(color, 0.0, 1.0), alpha);
            }
        )";

        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        fragmentShader.replace("%ShadowMapDeclaration%", shadowMapFragmentDeclaration);
        fragmentShader.replace("%ShadowMapSize%", QString::number(BUILDINGS_SHADOW_MAP_SIZE));
        auto preprocessedFragmentShader = fragmentShader;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        _colorProgram.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _colorProgram.cacheFormat);

        if (!_colorProgram.binaryCache.isEmpty())
        {
            _colorProgram.id = gpuAPI->linkProgram(
                0, nullptr, _colorProgram.binaryCache, _colorProgram.cacheFormat, true, &variablesMap);
        }
        if (_colorProgram.binaryCache.isEmpty() || !_colorProgram.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects color vertex shader");
                return false;
            }

            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects color fragment shader");
                return false;
            }

            const GLuint shaders[] = { vsId, fsId };
            _colorProgram.id = gpuAPI->linkProgram(
                2, shaders, _colorProgram.binaryCache, _colorProgram.cacheFormat, true, &variablesMap);
            if (_colorProgram.id.isValid() && !_colorProgram.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _colorProgram.binaryCache,
                    _colorProgram.cacheFormat);
            }
        }
    }

    if (!_colorProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link Map3DObjects color shader program");
        return false;
    }

    const auto lookup = gpuAPI->obtainVariablesLookupContext(_colorProgram.id, variablesMap);
    bool ok = true;
    ok = ok && lookup->lookupLocation(_colorProgram.vs.in.location31, "in_vs_location31", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.in.heights, "in_vs_heights", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.in.normal, "in_vs_normal", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.in.color, "in_vs_color", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.in.sizes, "in_vs_sizes", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.param.zoomLevel, "param_vs_zoomLevel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.param.zScaleFactor, "param_vs_zScaleFactor", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.fs.param.alpha, "param_fs_alpha", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.fs.param.fadeHeight, "param_fs_fadeHeight", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.fs.param.cameraPosition, "param_fs_cameraPosition", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.fs.param.lightDirection, "param_fs_lightDirection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.param.lightMatrix, "param_vs_lightMatrix", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.vs.param.shadowNormalOffset, "param_vs_shadowNormalOffset", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_colorProgram.fs.param.shadowStrength, "param_fs_shadowStrength", GlslVariableType::Uniform);
    // Absent without GLSL ES 3.0: the shaders then ignore shadows of buildings on buildings
    _colorProgram.withShadowMap = ok && lookup->lookupLocation(_colorProgram.fs.param.shadowMap, "param_fs_shadowMap", GlslVariableType::Uniform);

    if (!ok)
    {
        glDeleteProgram(_colorProgram.id);
        GL_CHECK_RESULT;

        _colorProgram.id.reset();

        LogPrintf(LogSeverityLevel::Error,
            "Failed to find variable in Map3DObjects color shader program");
        return false;
    }

    if (_colorVao.isValid())
    {
        gpuAPI->useVAO(_colorVao);
        
        glEnableVertexAttribArray(*_colorProgram.vs.in.location31);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_colorProgram.vs.in.heights);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_colorProgram.vs.in.sizes);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_colorProgram.vs.in.normal);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_colorProgram.vs.in.color);
        GL_CHECK_RESULT;
        
        gpuAPI->initializeVAO(_colorVao);
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
        vertexShader.replace("%ShadowMapDeclaration%", "");
        vertexShader.replace("%ShadowMapCalculation%", "");

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

bool AtlasMapRendererMap3DObjectsStage_OpenGL::initializeShadowProgram()
{
    const auto nextInit3DobjectsType = static_cast<Init3DObjectsType>(static_cast<int>(_init3DObjectsType) + 1);
    _init3DObjectsType = Init3DObjectsType::Incomplete;

    const auto gpuAPI = getGPUAPI();

    QHash<QString, GPUAPI_OpenGL::GlslProgramVariable> variablesMap;
    _shadowProgram.id = 0;

    if (!_shadowProgram.binaryCache.isEmpty())
    {
        _shadowProgram.id = gpuAPI->linkProgram(
            0, nullptr, _shadowProgram.binaryCache, _shadowProgram.cacheFormat, true, &variablesMap);
    }

    if (!_shadowProgram.id.isValid())
    {
        const QString shadowInOutDeclaration = QString(R"(
            uniform vec3 param_vs_lightDirection;
        )");
        // Slide every vertex down along the sun rays until it reaches the ground under the building.
        // Sun elevation is limited to ~3 degrees, so a shadow is never longer than 20 building heights.
        const QString shadowCalculation = QString(R"(
                float sunSin = max(-param_vs_lightDirection.y, 0.05);
                worldPos.xz += param_vs_lightDirection.xz * (vertexHeight / sunSin);
                worldPos.y = terrainElevation;
        )");

        auto vertexShader = vertexShaderBase;
        vertexShader.replace("%ColorInOutDeclaration%", shadowInOutDeclaration);
        vertexShader.replace("%ColorCalculation%", shadowCalculation);
        vertexShader.replace("%ShadowMapDeclaration%", "");
        vertexShader.replace("%ShadowMapCalculation%", "");

        const QString fragmentShader = R"(
            uniform float param_fs_shadowAlpha;
            uniform highp float param_fs_noiseSeed;

            void main()
            {
                // Per-sample grain: it averages out inside the shadow and dithers the soft edge
                highp vec2 p = gl_FragCoord.xy + vec2(param_fs_noiseSeed * 37.0, param_fs_noiseSeed * 91.0);
                highp float noise = fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
                // Cool blue-grey, so the shadow does not merge with the neutral grey of the buildings
                FRAGMENT_COLOR_OUTPUT = vec4(0.10, 0.14, 0.30, param_fs_shadowAlpha * (0.4 + 1.2 * noise));
            }
        )";

        auto preprocessedVertexShader = vertexShader;
        gpuAPI->preprocessVertexShader(preprocessedVertexShader);
        gpuAPI->optimizeVertexShader(preprocessedVertexShader);

        auto preprocessedFragmentShader = fragmentShader;
        gpuAPI->preprocessFragmentShader(preprocessedFragmentShader);
        gpuAPI->optimizeFragmentShader(preprocessedFragmentShader);

        _shadowProgram.binaryCache = gpuAPI->readProgramBinary(preprocessedVertexShader,
            preprocessedFragmentShader, setupOptions.pathToOpenGLShadersCache, _shadowProgram.cacheFormat);

        if (!_shadowProgram.binaryCache.isEmpty())
        {
            _shadowProgram.id = gpuAPI->linkProgram(
                0, nullptr, _shadowProgram.binaryCache, _shadowProgram.cacheFormat, true, &variablesMap);
        }
        if (_shadowProgram.binaryCache.isEmpty() || !_shadowProgram.id.isValid())
        {
            const auto vsId = gpuAPI->compileShader(GL_VERTEX_SHADER, qPrintable(preprocessedVertexShader));
            if (vsId == 0)
            {
                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects shadow vertex shader");
                return false;
            }

            const auto fsId = gpuAPI->compileShader(GL_FRAGMENT_SHADER, qPrintable(preprocessedFragmentShader));
            if (fsId == 0)
            {
                glDeleteShader(vsId);
                GL_CHECK_RESULT;

                LogPrintf(LogSeverityLevel::Error, "Failed to compile Map3DObjects shadow fragment shader");
                return false;
            }

            const GLuint shaders[] = { vsId, fsId };
            _shadowProgram.id = gpuAPI->linkProgram(
                2, shaders, _shadowProgram.binaryCache, _shadowProgram.cacheFormat, true, &variablesMap);
            if (_shadowProgram.id.isValid() && !_shadowProgram.binaryCache.isEmpty())
            {
                gpuAPI->writeProgramBinary(
                    preprocessedVertexShader,
                    preprocessedFragmentShader,
                    setupOptions.pathToOpenGLShadersCache,
                    _shadowProgram.binaryCache,
                    _shadowProgram.cacheFormat);
            }
        }
    }

    if (!_shadowProgram.id.isValid())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to link Map3DObjects shadow shader program");
        return false;
    }

    const auto lookup = gpuAPI->obtainVariablesLookupContext(_shadowProgram.id, variablesMap);
    bool ok = true;
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.in.location31, "in_vs_location31", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.in.heights, "in_vs_heights", GlslVariableType::In);
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.param.mPerspectiveProjectionView, "param_vs_mPerspectiveProjectionView", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.param.resultScale, "param_vs_resultScale", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.param.target31, "param_vs_target31", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.param.zoomLevel, "param_vs_zoomLevel", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.param.metersPerUnit, "param_vs_metersPerUnit", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.param.zScaleFactor, "param_vs_zScaleFactor", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_shadowProgram.vs.param.lightDirection, "param_vs_lightDirection", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_shadowProgram.fs.param.shadowAlpha, "param_fs_shadowAlpha", GlslVariableType::Uniform);
    ok = ok && lookup->lookupLocation(_shadowProgram.fs.param.noiseSeed, "param_fs_noiseSeed", GlslVariableType::Uniform);

    if (!ok)
    {
        glDeleteProgram(_shadowProgram.id);
        GL_CHECK_RESULT;

        _shadowProgram.id.reset();

        LogPrintf(LogSeverityLevel::Error,
            "Failed to find variable in Map3DObjects shadow shader program");
        return false;
    }

    if (_shadowVao.isValid())
    {
        gpuAPI->useVAO(_shadowVao);

        glEnableVertexAttribArray(*_shadowProgram.vs.in.location31);
        GL_CHECK_RESULT;
        glEnableVertexAttribArray(*_shadowProgram.vs.in.heights);
        GL_CHECK_RESULT;

        gpuAPI->initializeVAO(_shadowVao);
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
    
    _colorVao = gpuAPI->allocateUninitializedVAO();

    _depthVao = gpuAPI->allocateUninitializedVAO();

    _shadowVao = gpuAPI->allocateUninitializedVAO();

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
    QMap<int, QSet<TileId>>& presentTiles, QMap<int, QSet<TileId>>& occupiedSpace, bool* exact /* = nullptr */) const
{
    if (presentTiles.isEmpty())
        return false;
    const auto startZoom = presentTiles.firstKey();
    int zoomShift = 0;
    for (int zoom = zoomLevel; zoom >= startZoom; zoom--)
    {
        if (presentTiles.contains(zoom)
            && presentTiles[zoom].contains(TileId::fromXY(tileIdN.x >> zoomShift, tileIdN.y >> zoomShift)))
        {
            if (exact && zoom == zoomLevel)
                *exact = true;
            return true;
        }
        zoomShift++;
    }
    if (occupiedSpace.isEmpty())
        return false;
    const auto endZoom = occupiedSpace.lastKey();
    for (int zoom = std::max(zoomLevel, occupiedSpace.firstKey()); zoom <= endZoom; zoom++)
    {
        if (occupiedSpace[zoom].contains(tileIdN))
            return true;
    }

    return false;
}

void AtlasMapRendererMap3DObjectsStage_OpenGL::getResourcesInGPU(
    const std::shared_ptr<const IMapRendererResourcesCollection>& resourcesCollection,
    const int viewableDetalizationLevel,
    bool& highDetalizationLevel,
    const int64_t appearTime,
    bool& shouldInvalidateFrame)
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
        if (isLessDetailedLevel && viewableDetalizationLevel < 2)
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
        int risenZoom = isLessDetailedLevel || internalState.zoomLevelOffset != 0
            || internalState.extraDetailedTiles.empty() ? detZoom
            : std::min(static_cast<int>(MaxZoomLevel), zoomLevel + std::min(1, maxMissingDataUnderZoomShift));
        for (const auto& tileId : constOf(tiles))
        {
            const auto tileIdN = Utilities::normalizeTileId(tileId, zoomLevel);

            // Don't render invisible tiles
            if (!citVisibleTilesSet.value().contains(tileIdN))
                continue;

            int neededZoom = risenZoom;
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
                        highDetalizationLevel = highDetalizationLevel || meshInGPU->isDenseObject;
                        if (meshInGPU->creationTime > appearTime)
                        {
                            bool exact = false;
                            if (spaceAlreadyOccupied(underscaledTileId, neededZoom, *oldTiles, *oldSpace, &exact)
                                && !exact)
                                meshInGPU->creationTime = appearTime;
                            else
                                shouldInvalidateFrame = true;
                        }
                        occupySpace(underscaledTileId, neededZoom, minZoomLevel, *actualTiles, *actualSpace);
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
                    pUnderscaledTileIdN = underscaledTileIdsN.constData();
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
                    highDetalizationLevel = highDetalizationLevel || meshInGPU->isDenseObject;
                    if (meshInGPU->creationTime > appearTime)
                    {
                        bool exact = false;
                        if (spaceAlreadyOccupied(
                            tileIdN, zoomLevel, *oldTiles, *oldSpace, &exact) && !exact)
                            meshInGPU->creationTime = appearTime;
                        else
                            shouldInvalidateFrame = true;
                    }
                    occupySpace(tileIdN, zoomLevel, minZoomLevel, *actualTiles, *actualSpace);
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
                                highDetalizationLevel = highDetalizationLevel || meshInGPU->isDenseObject;
                                if (meshInGPU->creationTime > appearTime)
                                {
                                    bool exact = false;
                                    if (spaceAlreadyOccupied(
                                        underscaledTileId, underscaledZoom, *oldTiles, *oldSpace, &exact) && !exact)
                                        meshInGPU->creationTime = appearTime;
                                    else
                                        shouldInvalidateFrame = true;
                                }
                                occupySpace(
                                    underscaledTileId, underscaledZoom, minZoomLevel, *actualTiles, *actualSpace);
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
                            highDetalizationLevel = highDetalizationLevel || meshInGPU->isDenseObject;
                            if (meshInGPU->creationTime > appearTime)
                            {
                                bool exact = false;
                                if (spaceAlreadyOccupied(
                                    overscaledTileIdN, overscaleZoom, *oldTiles, *oldSpace, &exact) && !exact)
                                    meshInGPU->creationTime = appearTime;
                                else
                                    shouldInvalidateFrame = true;
                            }
                            occupySpace(overscaledTileIdN, overscaleZoom, minZoomLevel, *actualTiles, *actualSpace);
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

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::renderShadows()
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    const float sunAngle = currentState.elevationConfiguration.hillshadeSunAngle;
    const float sunFactor = qBound(0.0f, sunAngle / BUILDINGS_SHADOW_FADE_SUN_ANGLE, 1.0f);
    const float shadowAlpha = BUILDINGS_SHADOW_MAX_ALPHA * sunFactor;
    if (shadowAlpha <= 0.0f)
        return StageResult::Success;

    glUseProgram(_shadowProgram.id);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*_shadowProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE,
        glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;
    glUniform4f(*_shadowProgram.vs.param.resultScale, 1.0f, currentState.flip ? -1.0f : 1.0f, 1.0f, 1.0f);
    GL_CHECK_RESULT;
    glUniform2i(*_shadowProgram.vs.param.target31, currentState.target31.x, currentState.target31.y);
    GL_CHECK_RESULT;
    glUniform1i(*_shadowProgram.vs.param.zoomLevel, (int)currentState.zoomLevel);
    GL_CHECK_RESULT;
    glUniform1f(*_shadowProgram.vs.param.metersPerUnit, static_cast<float>(internalState.metersPerUnit));
    GL_CHECK_RESULT;
    glUniform1f(*_shadowProgram.vs.param.zScaleFactor, currentState.elevationConfiguration.zScaleFactor);
    GL_CHECK_RESULT;
    // Every sample adds the same share, so the umbra where all of them overlap gets the full shadowAlpha
    const float sampleAlpha = 1.0f - std::pow(1.0f - shadowAlpha, 1.0f / BUILDINGS_SHADOW_SAMPLES);
    glUniform1f(*_shadowProgram.fs.param.shadowAlpha, sampleAlpha);
    GL_CHECK_RESULT;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    // Projected triangles of walls and roofs overlap: every sample darkens a pixel once using stencil bit 0x02
    glStencilMask(0x02);
    glStencilFunc(GL_NOTEQUAL, 0x02, 0x02);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Shadows lie on the ground, so pull them towards the camera to win the depth test against it
    glEnable(GL_POLYGON_OFFSET_FILL);
    GL_CHECK_RESULT;
    glPolygonOffset(-2.0f, -4.0f);
    GL_CHECK_RESULT;

    gpuAPI->useVAO(_shadowVao);

    for (int sample = 0; sample < BUILDINGS_SHADOW_SAMPLES; sample++)
    {
        const auto angle = 2.0f * static_cast<float>(M_PI) * sample / BUILDINGS_SHADOW_SAMPLES;
        const float zenith = glm::radians(qMax(1.0f, sunAngle + BUILDINGS_SHADOW_SPREAD_ANGLE * std::sin(angle)));
        const float azimuth = glm::radians(currentState.elevationConfiguration.hillshadeSunAzimuth
            + BUILDINGS_SHADOW_SPREAD_ANGLE * std::cos(angle));
        const auto cosZenith = qCos(zenith);
        glUniform3f(*_shadowProgram.vs.param.lightDirection,
            -qSin(azimuth) * cosZenith,
            -qSin(zenith),
            qCos(azimuth) * cosZenith);
        GL_CHECK_RESULT;
        glUniform1f(*_shadowProgram.fs.param.noiseSeed, static_cast<float>(sample + 1));
        GL_CHECK_RESULT;

        if (sample > 0)
        {
            glClear(GL_STENCIL_BUFFER_BIT);
            GL_CHECK_RESULT;
        }

        for (const auto& resource : resourcesInGPU)
        {
            if (!resource->indexBuffer || resource->indexBuffer->itemsCount <= 0)
                continue;

            glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(
                resource->vertexBuffer->refInGPU)));
            GL_CHECK_RESULT;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(
                resource->indexBuffer->refInGPU)));
            GL_CHECK_RESULT;

            glVertexAttribIPointer(*_shadowProgram.vs.in.location31, 2, GL_INT,
                sizeof(BuildingVertex), reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, location31)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_shadowProgram.vs.in.heights, 2, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, heights)));
            GL_CHECK_RESULT;

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(resource->indexBuffer->itemsCount),
                GL_UNSIGNED_SHORT, nullptr);
            GL_CHECK_RESULT;
        }
    }
    gpuAPI->unuseVAO();

    glDisable(GL_POLYGON_OFFSET_FILL);
    GL_CHECK_RESULT;

    // Drop the shadow bit so that it does not leak into buildings and symbols
    glClear(GL_STENCIL_BUFFER_BIT);
    GL_CHECK_RESULT;
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00);

    return StageResult::Success;
}

bool AtlasMapRendererMap3DObjectsStage_OpenGL::renderShadowMap()
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    if (_shadowMapFailed)
        return false;

    if (_shadowMapTexture == 0)
    {
        glGenTextures(1, &_shadowMapTexture);
        GL_CHECK_RESULT;
        glBindTexture(GL_TEXTURE_2D, _shadowMapTexture);
        GL_CHECK_RESULT;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, BUILDINGS_SHADOW_MAP_SIZE, BUILDINGS_SHADOW_MAP_SIZE, 0,
            GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        GL_CHECK_RESULT;
        // Plain depth reads compared in the shader: hardware depth comparison is not reliable on every driver
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        GL_CHECK_RESULT;
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &_shadowMapFramebuffer);
        GL_CHECK_RESULT;
    }

    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, _shadowMapFramebuffer);
    GL_CHECK_RESULT;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowMapTexture, 0);
    GL_CHECK_RESULT;
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LogPrintf(LogSeverityLevel::Error, "Map3DObjects shadow map framebuffer is incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
        _shadowMapFailed = true;
        return false;
    }

    // Sun looks along its rays at the square around the map target that the camera sees best
    const float sunAngle = currentState.elevationConfiguration.hillshadeSunAngle;
    const auto zenith = glm::radians(qBound(1.0f, sunAngle, 89.0f));
    const auto azimuth = glm::radians(currentState.elevationConfiguration.hillshadeSunAzimuth);
    const glm::vec3 lightDirection(
        -std::sin(azimuth) * std::cos(zenith), -std::sin(zenith), std::cos(azimuth) * std::cos(zenith));
    const float range = internalState.distanceFromCameraToTarget * BUILDINGS_SHADOW_MAP_RANGE_FACTOR;
    const float maxBuildingHeight = 600.0f / static_cast<float>(internalState.metersPerUnit);
    const float depth = range + maxBuildingHeight;
    const glm::vec3 center(0.0f);
    const auto mLightView = glm::lookAt(center - lightDirection * depth, center, glm::vec3(0.0f, 1.0f, 0.0f));
    const auto mLightProjection = glm::ortho(-range, range, -range, range, 0.0f, 2.0f * depth);
    const auto mLightProjectionView = mLightProjection * mLightView;
    // From clip space [-1, 1] to texture coordinates and depth [0, 1], with a small bias against self-shadowing
    const float depthBias = (0.5f / static_cast<float>(internalState.metersPerUnit)) / (2.0f * depth);
    _shadowMapMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f - depthBias))
        * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * mLightProjectionView;

    _shadowMapNormalOffset = 1.5f * 2.0f * range / BUILDINGS_SHADOW_MAP_SIZE;
    glViewport(0, 0, BUILDINGS_SHADOW_MAP_SIZE, BUILDINGS_SHADOW_MAP_SIZE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_DEPTH_BUFFER_BIT);
    GL_CHECK_RESULT;
    glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    glUseProgram(_depthProgram.id);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*_depthProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE,
        glm::value_ptr(mLightProjectionView));
    glUniform4f(*_depthProgram.vs.param.resultScale, 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform2i(*_depthProgram.vs.param.target31, currentState.target31.x, currentState.target31.y);
    glUniform1i(*_depthProgram.vs.param.zoomLevel, (int)currentState.zoomLevel);
    glUniform1f(*_depthProgram.vs.param.metersPerUnit, static_cast<float>(internalState.metersPerUnit));
    glUniform1f(*_depthProgram.vs.param.zScaleFactor, currentState.elevationConfiguration.zScaleFactor);
    GL_CHECK_RESULT;

    gpuAPI->useVAO(_depthVao);
    for (const auto& resource : resourcesInGPU)
    {
        if (!resource->indexBuffer || resource->indexBuffer->itemsCount <= 0)
            continue;

        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(
            resource->vertexBuffer->refInGPU)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(reinterpret_cast<uintptr_t>(
            resource->indexBuffer->refInGPU)));
        glVertexAttribIPointer(*_depthProgram.vs.in.location31, 2, GL_INT,
            sizeof(BuildingVertex), reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, location31)));
        glVertexAttribPointer(*_depthProgram.vs.in.heights, 2, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
            reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, heights)));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(resource->indexBuffer->itemsCount),
            GL_UNSIGNED_SHORT, nullptr);
        GL_CHECK_RESULT;
    }
    gpuAPI->unuseVAO();
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    GL_CHECK_RESULT;
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_BLEND);
    GL_CHECK_RESULT;

    return true;
}

void AtlasMapRendererMap3DObjectsStage_OpenGL::setupShadowMapSampling(const Model3DProgram& program, float strength)
{
    const bool withShadowMap = program.withShadowMap && strength > 0.0f && _shadowMapTexture != 0;
    glUniform1f(*program.fs.param.shadowStrength, withShadowMap ? strength : 0.0f);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*program.vs.param.lightMatrix, 1, GL_FALSE, glm::value_ptr(_shadowMapMatrix));
    GL_CHECK_RESULT;
    glUniform1f(*program.vs.param.shadowNormalOffset, _shadowMapNormalOffset);
    GL_CHECK_RESULT;
    if (program.withShadowMap)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, withShadowMap ? _shadowMapTexture : 0);
        glUniform1i(*program.fs.param.shadowMap, 0);
        GL_CHECK_RESULT;
    }
}

void AtlasMapRendererMap3DObjectsStage_OpenGL::releaseShadowMap(bool gpuContextLost)
{
    if (!gpuContextLost)
    {
        if (_shadowMapFramebuffer != 0)
            glDeleteFramebuffers(1, &_shadowMapFramebuffer);
        if (_shadowMapTexture != 0)
            glDeleteTextures(1, &_shadowMapTexture);
    }
    _shadowMapFramebuffer = 0;
    _shadowMapTexture = 0;
    _shadowMapFailed = false;
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::renderSimple(bool primaryOnly)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    float buildingAlpha = renderer->get3DBuildingsAlpha();

    const auto zenith = glm::radians(currentState.elevationConfiguration.hillshadeSunAngle);
    const auto azimuth = glm::radians(currentState.elevationConfiguration.hillshadeSunAzimuth);
    const auto cosZenith = qCos(zenith);

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
    setupShadowMapSampling(_program, _shadowMapStrength);
    glUniform3f(*_program.fs.param.lightDirection,
        -qSin(azimuth) * cosZenith,
        -qSin(zenith),
        qCos(azimuth) * cosZenith);
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

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                GL_UNSIGNED_SHORT, (void*) (startIndex * sizeof(uint16_t)));
            GL_CHECK_RESULT;
        }
    }
    gpuAPI->unuseVAO();

    return StageResult::Success;
}

MapRendererStage::StageResult AtlasMapRendererMap3DObjectsStage_OpenGL::renderColor(
    bool primaryOnly, int64_t currentTime)
{
    const auto gpuAPI = getGPUAPI();
    const auto& internalState = getInternalState();

    float buildingAlpha = renderer->get3DBuildingsAlpha();

    const auto zenith = glm::radians(currentState.elevationConfiguration.hillshadeSunAngle);
    const auto azimuth = glm::radians(currentState.elevationConfiguration.hillshadeSunAzimuth);
    const auto cosZenith = qCos(zenith);

    glUseProgram(_colorProgram.id);
    GL_CHECK_RESULT;
    glUniformMatrix4fv(*_colorProgram.vs.param.mPerspectiveProjectionView, 1, GL_FALSE,
        glm::value_ptr(internalState.mPerspectiveProjectionView));
    GL_CHECK_RESULT;
    glUniform4f(*_colorProgram.vs.param.resultScale, 1.0f, currentState.flip ? -1.0f : 1.0f, 1.0f, 1.0f);
    GL_CHECK_RESULT;
    glUniform2i(*_colorProgram.vs.param.target31, currentState.target31.x, currentState.target31.y);
    GL_CHECK_RESULT;
    glUniform1i(*_colorProgram.vs.param.zoomLevel, (int)currentState.zoomLevel);
    GL_CHECK_RESULT;
    glUniform1f(*_colorProgram.vs.param.metersPerUnit, static_cast<float>(internalState.metersPerUnit));
    GL_CHECK_RESULT;
    glUniform1f(*_colorProgram.vs.param.zScaleFactor, currentState.elevationConfiguration.zScaleFactor);
    GL_CHECK_RESULT;
    glUniform1f(*_colorProgram.fs.param.alpha, buildingAlpha);
    GL_CHECK_RESULT;
    setupShadowMapSampling(_colorProgram, _shadowMapStrength);
    glUniform3f(*_colorProgram.fs.param.cameraPosition,
        internalState.worldCameraPosition.x,
        internalState.worldCameraPosition.y,
        internalState.worldCameraPosition.z);
    GL_CHECK_RESULT;
    glUniform3f(*_colorProgram.fs.param.lightDirection,
        -qSin(azimuth) * cosZenith,
        -qSin(zenith),
        qCos(azimuth) * cosZenith);
    GL_CHECK_RESULT;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_RESULT;

    gpuAPI->useVAO(_colorVao);

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

            glVertexAttribIPointer(*_colorProgram.vs.in.location31, 2, GL_INT, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, location31)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_colorProgram.vs.in.heights, 2, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, heights)));
            GL_CHECK_RESULT;

            glVertexAttribPointer(*_colorProgram.vs.in.normal, 3, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, normal)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_colorProgram.vs.in.color, 4, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, color)));
            GL_CHECK_RESULT;
            glVertexAttribPointer(*_colorProgram.vs.in.sizes, 4, GL_FLOAT, GL_FALSE, sizeof(BuildingVertex),
                reinterpret_cast<const GLvoid*>(offsetof(BuildingVertex, sizes)));
            GL_CHECK_RESULT;

            const auto fadeHeight = qMin(10000.0, exp(10.0
                * static_cast<double>(currentTime - resource->creationTime) / BUILDINGS_FADE_ANIMATION_PERIOD) - 1.0);
            glUniform1f(*_colorProgram.fs.param.fadeHeight, static_cast<float>(fadeHeight));
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
        ok = ok && (init3DObjectsType != Init3DObjectsType::Objects3DSimple || initializeSimpleProgram());
        ok = ok && (init3DObjectsType != Init3DObjectsType::Objects3DColor || initializeColorProgram());
        ok = ok && (init3DObjectsType != Init3DObjectsType::Objects3DShadow || initializeShadowProgram());

        if (!ok || _init3DObjectsType == Init3DObjectsType::Incomplete)
            return StageResult::Fail;

        return StageResult::Wait;
    }

    const auto resourcesCollection = getResources().getCollectionSnapshot(MapRendererResourceType::Map3DObjects,
        std::static_pointer_cast<IMapDataProvider>(currentState.map3DObjectsProvider));

    if (!resourcesCollection)
        return StageResult::Success;

    const float buildingsAlpha = renderer->get3DBuildingsAlpha();
    const int viewableDetalizationLevel = renderer->get3DBuildingsDetalization();
    bool highDetalizationLevel = false;
    bool shouldInvalidateFrame = false;

    resourcesInGPU.clear();

    const int64_t currentTime = QDateTime::currentMSecsSinceEpoch();

    actualTiles->clear();
    actualSpace->clear();
    getResourcesInGPU(
        resourcesCollection,
        viewableDetalizationLevel,
        highDetalizationLevel,
        currentTime - BUILDINGS_FADE_ANIMATION_PERIOD,
        shouldInvalidateFrame);

    if (actualTiles == &_firstTiles)
    {
        actualTiles = &_secondTiles;
        actualSpace = &_secondSpace;
        oldTiles = &_firstTiles;
        oldSpace = &_firstSpace;
    }
    else
    {
        actualTiles = &_firstTiles;
        actualSpace = &_firstSpace;
        oldTiles = &_secondTiles;
        oldSpace = &_secondSpace;
    }

    if (resourcesInGPU.isEmpty())
    {
        return StageResult::Success;
    }

    if (shouldInvalidateFrame)
        invalidateFrame();

    const bool needsDepthPrepass = buildingsAlpha > 0.0 && buildingsAlpha < 1.0f;

    // Only draw where the stencil value is NOT 1 to make certain important symbols could be clearly seen through
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    glDepthFunc(GL_LEQUAL);
    GL_CHECK_RESULT;

    StageResult shadowsResult = StageResult::Success;
    _shadowMapStrength = 0.0f;
    if (renderer->get3DBuildingsShadows() && currentState.elevationConfiguration.hillshadeSunAngle > 0.0f
        && (_program.withShadowMap || _colorProgram.withShadowMap) && renderShadowMap())
    {
        _shadowMapStrength = qBound(0.0f,
            currentState.elevationConfiguration.hillshadeSunAngle / BUILDINGS_SHADOW_FADE_SUN_ANGLE, 1.0f);
    }
    if (renderer->get3DBuildingsShadows())
    {
        // Buildings depth goes first, so that shadows stay on the open ground
        // and never show through semi-transparent buildings
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        GL_CHECK_RESULT;
        glDisable(GL_BLEND);
        GL_CHECK_RESULT;
        glEnable(GL_CULL_FACE);
        GL_CHECK_RESULT;
        glCullFace(currentState.flip ? GL_BACK : GL_FRONT);
        GL_CHECK_RESULT;
        glDepthMask(GL_TRUE);
        GL_CHECK_RESULT;
        shadowsResult = renderDepth(true);
        if (shadowsResult != StageResult::Fail)
            shadowsResult = renderDepth(false);
        glDisable(GL_CULL_FACE);
        GL_CHECK_RESULT;
        glEnable(GL_BLEND);
        GL_CHECK_RESULT;
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        GL_CHECK_RESULT;

        glDepthMask(GL_FALSE);
        GL_CHECK_RESULT;
        if (shadowsResult != StageResult::Fail)
            shadowsResult = renderShadows();
        // Back to the building-hole test for important symbols
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    }

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

    auto colorPassResult = highDetalizationLevel ? renderColor(true, currentTime) : renderSimple(true);

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
        colorPassResult = highDetalizationLevel ? renderColor(false, currentTime) : renderSimple(false);

    if (needsDepthPrepass)
    {
        glDepthMask(GL_TRUE);
        GL_CHECK_RESULT;
    }

    glDisable(GL_CULL_FACE);
    GL_CHECK_RESULT;

    if (_shadowMapStrength > 0.0f)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Disable testing stencil buffer
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    if (depthPrepassResult == StageResult::Fail || colorPassResult == StageResult::Fail
        || shadowsResult == StageResult::Fail)
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

    if (_colorVao.isValid())
    {
        gpuAPI->releaseVAO(_colorVao, gpuContextLost);
        _colorVao.reset();
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

    if (_colorProgram.id)
    {
        glDeleteProgram(_colorProgram.id);
        GL_CHECK_RESULT;
        _colorProgram.id = 0;
    }

    if (_depthProgram.id)
    {
        glDeleteProgram(_depthProgram.id);
        GL_CHECK_RESULT;
        _depthProgram.id = 0;
    }

    releaseShadowMap(gpuContextLost);

    if (_shadowVao.isValid())
    {
        gpuAPI->releaseVAO(_shadowVao, gpuContextLost);
        _shadowVao.reset();
    }

    if (_shadowProgram.id)
    {
        glDeleteProgram(_shadowProgram.id);
        GL_CHECK_RESULT;
        _shadowProgram.id = 0;
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
