#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRenderer.h"
#include "IAtlasMapRenderer.h"
#include "MapRendererResourcesManager.h"
#include "QuadTree.h"

namespace OsmAnd
{
    class AtlasMapRendererSkyStage;
    class AtlasMapRendererMapLayersStage;
    class AtlasMapRendererSymbolsStage;
    class AtlasMapRendererDebugStage;

    class AtlasMapRenderer
        : public MapRenderer
        , public IAtlasMapRenderer
    {
        Q_DISABLE_COPY_AND_MOVE(AtlasMapRenderer);

    public:

        enum {
            TileSize3D = 100,
        };

    private:
    protected:
        AtlasMapRenderer(
            GPUAPI* const gpuAPI,
            const std::unique_ptr<const MapRendererConfiguration>& baseConfiguration,
            const std::unique_ptr<const MapRendererDebugSettings>& baseDebugSettings);

        // Configuration-related:
        enum class ConfigurationChange
        {
            ReferenceTileSize = static_cast<int>(MapRenderer::ConfigurationChange::__LAST),

            __LAST
        };
        enum {
            RegisteredConfigurationChangesCount = static_cast<unsigned int>(ConfigurationChange::__LAST)
        };
        virtual uint32_t getConfigurationChangeMask(
            const std::shared_ptr<const MapRendererConfiguration>& current,
            const std::shared_ptr<const MapRendererConfiguration>& updated) const Q_DECL_OVERRIDE;
        virtual void validateConfigurationChange(const MapRenderer::ConfigurationChange& change) Q_DECL_OVERRIDE;

        // State-related:
        virtual bool updateInternalState(
            MapRendererInternalState& outInternalState,
            const MapRendererState& state,
            const MapRendererConfiguration& configuration,
            const bool skipTiles = false, const bool sortTiles = false) const Q_DECL_OVERRIDE;

        // Debug-related:

        // Customization points:
        bool preInitializeRendering() override;
        bool doInitializeRendering(bool reinitialize) override;

        bool prePrepareFrame() override;
        bool postPrepareFrame() override;

        bool doReleaseRendering(bool gpuContextLost) override;
        bool postReleaseRendering(bool gpuContextLost) override;

        // Stages:
        std::shared_ptr<AtlasMapRendererSkyStage> _skyStage;
        virtual AtlasMapRendererSkyStage* createSkyStage() = 0;
        std::shared_ptr<AtlasMapRendererMapLayersStage> _mapLayersStage;
        virtual AtlasMapRendererMapLayersStage* createMapLayersStage() = 0;
        std::shared_ptr<AtlasMapRendererSymbolsStage> _symbolsStage;
        virtual AtlasMapRendererSymbolsStage* createSymbolsStage() = 0;
        std::shared_ptr<AtlasMapRendererDebugStage> _debugStage;
        virtual AtlasMapRendererDebugStage* createDebugStage() = 0;
    public:
        virtual ~AtlasMapRenderer();

        // Stages:
        const std::shared_ptr<AtlasMapRendererSkyStage>& skyStage;
        const std::shared_ptr<AtlasMapRendererMapLayersStage>& mapLayersStage;
        const std::shared_ptr<AtlasMapRendererSymbolsStage>& symbolsStage;
        const std::shared_ptr<AtlasMapRendererDebugStage>& debugStage;

        virtual QVector<TileId> getVisibleTiles() const Q_DECL_OVERRIDE;
        virtual unsigned int getVisibleTilesCount() const Q_DECL_OVERRIDE;
        virtual unsigned int getAllTilesCount() const Q_DECL_OVERRIDE;
        virtual unsigned int getDetailLevelsCount() const Q_DECL_OVERRIDE;
        virtual LatLon getCameraCoordinates() const Q_DECL_OVERRIDE;
        virtual double getCameraHeight() const Q_DECL_OVERRIDE;

        virtual int getTileSize3D() const Q_DECL_OVERRIDE;

        // Symbols-related
        virtual QList<MapSymbolInformation> getSymbolsAt(
            const PointI& screenPoint) const Q_DECL_OVERRIDE;
        virtual QList<MapSymbolInformation> getSymbolsIn(
            const AreaI& screenPoint,
            const bool strict = false) const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_H_)
