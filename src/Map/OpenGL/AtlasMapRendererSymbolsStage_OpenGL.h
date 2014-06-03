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
        GLname _pinnedSymbolVAO;
        GLname _pinnedSymbolVBO;
        GLname _pinnedSymbolIBO;
        struct PinnedSymbolProgram {
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
        } _pinnedSymbolProgram;
        void initializePinned();
        void releasePinned();

        struct Glyph
        {
            GLlocation anchorPoint;
            GLlocation width;
            GLlocation angle;
            GLlocation widthOfPreviousN;
            GLlocation widthN;
        };
        GLname _symbolOnPath2dVAO;
        GLname _symbolOnPath2dVBO;
        GLname _symbolOnPath2dIBO;
        struct SymbolOnPath2dProgram {
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
        } _symbolOnPath2dProgram;
        GLint _maxGlyphsPerDrawCallSOP2D;
        void initializeOnPath2D();
        void releaseOnPath2D();

        GLname _symbolOnPath3dVAO;
        GLname _symbolOnPath3dVBO;
        GLname _symbolOnPath3dIBO;
        struct SymbolOnPath3dProgram {
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
        } _symbolOnPath3dProgram;
        GLint _maxGlyphsPerDrawCallSOP3D;
        void initializeOnPath3D();
        void releaseOnPath3D();
        
        void initializeOnPath();
        void releaseOnPath();

        struct RenderableSymbol
        {
            virtual ~RenderableSymbol();

            std::shared_ptr<const MapSymbol> mapSymbol;
            std::shared_ptr<const GPUAPI::ResourceInGPU> gpuResource;
        };
        typedef std::shared_ptr<const RenderableSymbol> RenderableSymbolEntry;

        struct RenderableSpriteSymbol : RenderableSymbol
        {
            PointI offsetFromTarget31;
            PointF offsetFromTarget;
            glm::vec3 positionInWorld;
        };
        
        struct RenderableOnPathSymbol : RenderableSymbol
        {
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
        typedef std::shared_ptr<RenderableOnPathSymbol> RenderableSymbolOnPathEntry;

        void processOnPathSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap<float, RenderableSymbolEntry>& output,
            const glm::vec4& viewport,
            QSet< std::shared_ptr<MapRendererBaseResource> > &updatedMapSymbolsResources);

        // Obtains visible portions of each OnPathSymbol
        void obtainRenderableEntriesForOnPathSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QList< RenderableSymbolOnPathEntry >& output,
            QSet< std::shared_ptr<MapRendererBaseResource> > &updatedMapSymbolsResources);

        // Calculates renderable OnPathSymbols to in world
        void calculateRenderableOnPathSymbolsInWorld(QList< RenderableSymbolOnPathEntry >& entries);

        // Determines if each renderable OnPathSymbol is 2D-mode or 3D-mode
        void determineRenderableOnPathSymbolsMode(QList< RenderableSymbolOnPathEntry >& entries, const glm::vec4& viewport);

        // Adjusts renderable OnPathSymbol bitmap placement on entire path
        void adjustPlacementOfGlyphsOnPath(QList< RenderableSymbolOnPathEntry >& entries, const glm::vec4& viewport);

        void sortRenderableOnPathSymbols(const QList< RenderableSymbolOnPathEntry >& entries, QMultiMap< float, RenderableSymbolEntry >& output);

        void processSpriteSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap<float, RenderableSymbolEntry>& output,
            QSet< std::shared_ptr<MapRendererBaseResource> >& updatedMapSymbolsResources);

        void obtainAndSortSpriteSymbols(
            const MapRendererResourcesManager::MapSymbolsByOrderRegisterLayer& input,
            QMultiMap<float, RenderableSymbolEntry>& output,
            QSet< std::shared_ptr<MapRendererBaseResource> >& updatedMapSymbolsResources);

        typedef QuadTree< std::shared_ptr<const MapSymbol>, AreaI::CoordType > IntersectionsQuadTree;

        bool renderSpriteSymbol(
            const std::shared_ptr<const RenderableSpriteSymbol>& renderable,
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
    public:
        AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererSymbolsStage_OpenGL();

        virtual void initialize();
        virtual void render();
        virtual void release();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_)
