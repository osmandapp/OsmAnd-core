#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_

#include "stdlib_common.h"

#include <glm/glm.hpp>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRenderer.h"
#include "AtlasMapRendererInternalState_OpenGL.h"
#include "AtlasMapRendererSkyStage_OpenGL.h"
#include "AtlasMapRendererRasterMapStage_OpenGL.h"
#include "AtlasMapRendererSymbolsStage_OpenGL.h"
#if OSMAND_DEBUG
#   include "AtlasMapRendererDebugStage_OpenGL.h"
#endif
#include "OpenGL/GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererStage_OpenGL;
    class AtlasMapRenderer_OpenGL : public AtlasMapRenderer
    {
        Q_DISABLE_COPY(AtlasMapRenderer_OpenGL);
    public:
        // Short type aliases:
        typedef AtlasMapRendererInternalState_OpenGL InternalState;
    private:
    protected:
        const static float _zNear;

        InternalState _internalState;
        virtual const MapRenderer::InternalState* getInternalStateRef() const;
        virtual MapRenderer::InternalState* getInternalStateRef();

        void updateFrustum(InternalState* internalState, const MapRendererState& state);
        void computeVisibleTileset(InternalState* internalState, const MapRendererState& state);

        AtlasMapRendererSkyStage_OpenGL _skyStage;
        AtlasMapRendererRasterMapStage_OpenGL _rasterMapStage;
        AtlasMapRendererSymbolsStage_OpenGL _symbolsStage;
#if OSMAND_DEBUG
        AtlasMapRendererDebugStage_OpenGL _debugStage;
#endif

        virtual bool doInitializeRendering();
        virtual bool doRenderFrame();
        virtual bool doReleaseRendering();

        virtual void onValidateResourcesOfType(const MapRendererResources::ResourceType type);

        virtual bool updateInternalState(MapRenderer::InternalState* internalState, const MapRendererState& state);

        virtual bool postInitializeRendering();
        virtual bool preReleaseRendering();

        GPUAPI_OpenGL* getGPUAPI() const;

        float getReferenceTileSizeOnScreen(const MapRendererState& state);
    public:
        AtlasMapRenderer_OpenGL(GPUAPI_OpenGL* const gpuAPI);
        virtual ~AtlasMapRenderer_OpenGL();

        virtual float getReferenceTileSizeOnScreen();
        virtual float getScaledTileSizeOnScreen();
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31);
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location);

    friend class OsmAnd::AtlasMapRendererStage_OpenGL;
    friend class OsmAnd::AtlasMapRendererSkyStage_OpenGL;
    friend class OsmAnd::AtlasMapRendererSymbolsStage_OpenGL;
#if OSMAND_DEBUG
    friend class OsmAnd::AtlasMapRendererDebugStage_OpenGL;
#endif
    //friend class OsmAnd::AtlasMapRendererStage_OpenGL;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_)
