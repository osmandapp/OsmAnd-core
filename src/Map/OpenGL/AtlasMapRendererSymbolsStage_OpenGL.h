#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include <glm/glm.hpp>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererStage_OpenGL.h"
#include "GPUAPI_OpenGL.h"
#include "QuadTree.h"

namespace OsmAnd
{
    class AtlasMapRendererSymbolsStage_OpenGL : public AtlasMapRendererStage_OpenGL
    {
    private:
    protected:
        GLname _billboardRasterSymbolVAO;
        GLname _billboardRasterSymbolVBO;
        GLname _billboardRasterSymbolIBO;
        struct BillboardRasterSymbolProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mPerspectiveProjectionView;
                    GLlocation mOrthographicProjection;
                    GLlocation viewport;

                    // Per-symbol data
                    GLlocation symbolOffsetFromTarget;
                    GLlocation symbolSize;
                    GLlocation distanceFromCamera;
                    GLlocation onScreenOffset;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation sampler;
                } param;
            } fs;
        } _billboardRasterProgram;
        void initializeBillboardRaster();
        void releaseBillboardRaster();

        struct Glyph
        {
            GLlocation anchorPoint;
            GLlocation width;
            GLlocation angle;
            GLlocation widthOfPreviousN;
            GLlocation widthN;
        };
        GLname _onPathSymbol2dVAO;
        GLname _onPathSymbol2dVBO;
        GLname _onPathSymbol2dIBO;
        struct OnPathSymbol2dProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation glyphIndex;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mOrthographicProjection;

                    // Per-symbol data
                    GLlocation glyphHeight;
                    GLlocation distanceFromCamera;

                    // Per-glyph data
                    QVector<Glyph> glyphs;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation sampler;
                } param;
            } fs;
        } _onPath2dProgram;
        GLint _onPathSymbol2dMaxGlyphsPerDrawCall;
        void initializeOnPath2D();
        void releaseOnPath2D();

        GLname _onPathSymbol3dVAO;
        GLname _onPathSymbol3dVBO;
        GLname _onPathSymbol3dIBO;
        struct OnPathSymbol3dProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation glyphIndex;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mPerspectiveProjectionView;

                    // Per-symbol data
                    GLlocation glyphHeight;
                    GLlocation zDistanceFromCamera;

                    // Per-glyph data
                    QVector<Glyph> glyphs;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation sampler;
                } param;
            } fs;
        } _onPath3dProgram;
        GLint _onPathSymbol3dMaxGlyphsPerDrawCall;
        void initializeOnPath3D();
        void releaseOnPath3D();
        
        void initializeOnPath();
        void releaseOnPath();

        GLname _onSurfaceRasterSymbolVAO;
        GLname _onSurfaceRasterSymbolVBO;
        GLname _onSurfaceRasterSymbolIBO;
        struct OnSurfaceSymbolProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mPerspectiveProjectionView;

                    // Per-symbol data
                    GLlocation symbolOffsetFromTarget;
                    GLlocation direction;
                    GLlocation symbolSize;
                    GLlocation zDistanceFromCamera;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation sampler;
                } param;
            } fs;
        } _onSurfaceRasterProgram;
        void initializeOnSurfaceRaster();
        void releaseOnSurfaceRaster();

        struct OnSurfaceVectorProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexColor;
                } in;

                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation mModelViewProjection;
                    GLlocation zDistanceFromCamera;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                } param;
            } fs;
        } _onSurfaceVectorProgram;
        void initializeOnSurfaceVector();
        void releaseOnSurfaceVector();

        struct RenderableSymbol
        {
            virtual ~RenderableSymbol();

            std::shared_ptr<const MapSymbol> mapSymbol;
            std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
        };

        struct RenderableBillboardSymbol : RenderableSymbol
        {
            virtual ~RenderableBillboardSymbol();

            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;
        };
        
        struct RenderableOnPathSymbol : RenderableSymbol
        {
            virtual ~RenderableOnPathSymbol();

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

        struct RenderableOnSurfaceSymbol : RenderableSymbol
        {
            virtual ~RenderableOnSurfaceSymbol();

            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;

            float direction;
        };

        void processOnPathSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output,
            const glm::vec4& viewport,
            QSet< std::shared_ptr<MapRendererBaseResource> > &updatedMapSymbolsResources);

        // Obtains visible portions of each OnPathSymbol
        void obtainRenderableEntriesForOnPathSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QList< std::shared_ptr<RenderableOnPathSymbol> >& output,
            QSet< std::shared_ptr<MapRendererBaseResource> > &updatedMapSymbolsResources);

        // Calculates renderable OnPathSymbols to in world
        void calculateRenderableOnPathSymbolsInWorld(QList< std::shared_ptr<RenderableOnPathSymbol> >& entries);

        // Determines if each renderable OnPathSymbol is 2D-mode or 3D-mode
        void determineRenderableOnPathSymbolsMode(QList< std::shared_ptr<RenderableOnPathSymbol> >& entries, const glm::vec4& viewport);

        // Adjusts renderable OnPathSymbol bitmap placement on entire path
        void adjustPlacementOfGlyphsOnPath(QList< std::shared_ptr<RenderableOnPathSymbol> >& entries, const glm::vec4& viewport);

        void sortRenderableOnPathSymbols(const QList< std::shared_ptr<RenderableOnPathSymbol> >& entries, QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output);

        void processBillboardSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output,
            QSet< std::shared_ptr<MapRendererBaseResource> >& updatedMapSymbolsResources);

        void obtainAndSortBillboardSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output,
            QSet< std::shared_ptr<MapRendererBaseResource> >& updatedMapSymbolsResources);

        void processOnSurfaceSymbols(const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output,
            QSet< std::shared_ptr<MapRendererBaseResource> > &updatedMapSymbolsResources);

        void obtainAndSortOnSurfaceSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap< float, std::shared_ptr<RenderableSymbol> >& output,
            QSet< std::shared_ptr<MapRendererBaseResource> >& updatedMapSymbolsResources);

        typedef QuadTree< std::shared_ptr<const MapSymbol>, AreaI::CoordType > IntersectionsQuadTree;

        bool renderBillboardSymbol(
            const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
            const glm::vec4& viewport,
            IntersectionsQuadTree& intersections,
            int& lastUsedProgram,
            const glm::mat4x4& mPerspectiveProjectionView,
            const float distanceFromCamera);
        bool renderBillboardRasterSymbol(
            const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
            const glm::vec4& viewport,
            IntersectionsQuadTree& intersections,
            int& lastUsedProgram,
            const glm::mat4x4& mPerspectiveProjectionView,
            const float distanceFromCamera);
        bool renderBillboardVectorSymbol(
            const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
            const glm::vec4& viewport,
            IntersectionsQuadTree& intersections,
            int& lastUsedProgram,
            const glm::mat4x4& mPerspectiveProjectionView,
            const float distanceFromCamera);

        bool renderOnPathSymbol(
            const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
            const glm::vec4& viewport,
            IntersectionsQuadTree& intersections,
            int& lastUsedProgram,
            const glm::mat4x4& mPerspectiveProjectionView,
            const float distanceFromCamera);

        bool renderOnSurfaceSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            const glm::vec4& viewport,
            IntersectionsQuadTree& intersections,
            int& lastUsedProgram,
            const glm::mat4x4& mPerspectiveProjectionView,
            const float distanceFromCamera);
        bool renderOnSurfaceRasterSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            const glm::vec4& viewport,
            IntersectionsQuadTree& intersections,
            int& lastUsedProgram,
            const glm::mat4x4& mPerspectiveProjectionView,
            const float distanceFromCamera);
        bool renderOnSurfaceVectorSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            const glm::vec4& viewport,
            IntersectionsQuadTree& intersections,
            int& lastUsedProgram,
            const glm::mat4x4& mPerspectiveProjectionView,
            const float distanceFromCamera);
    public:
        AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererSymbolsStage_OpenGL();

        virtual void initialize();
        virtual void render();
        virtual void release();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_)
