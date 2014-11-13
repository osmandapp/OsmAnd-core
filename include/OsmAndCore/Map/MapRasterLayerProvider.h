#ifndef _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_H_
#define _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>
#include <OsmAndCore/Map/MapRasterLayerProvider_Metrics.h>
#include <OsmAndCore/Map/MapPrimitivesProvider.h>

namespace OsmAnd
{
    class MapRasterLayerProvider_P;
    class OSMAND_CORE_API MapRasterLayerProvider : public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapRasterLayerProvider);
    public:
        class OSMAND_CORE_API Data : public IRasterMapLayerProvider::Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                const TileId tileId,
                const ZoomLevel zoom,
                const AlphaChannelData alphaChannelData,
                const float densityFactor,
                const std::shared_ptr<const SkBitmap>& bitmap,
                const std::shared_ptr<const MapPrimitivesProvider::Data>& binaryMapData,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const MapPrimitivesProvider::Data> binaryMapData;
        };

    private:
        PrivateImplementation<MapRasterLayerProvider_P> _p;
    protected:
        MapRasterLayerProvider(
            MapRasterLayerProvider_P* const p,
            const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider);
    public:
        virtual ~MapRasterLayerProvider();

        const std::shared_ptr<MapPrimitivesProvider> primitivesProvider;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            std::shared_ptr<Metric>* pOutMetric = nullptr,
            const IQueryController* const queryController = nullptr);

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            MapRasterLayerProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_H_)
