#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_H_

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
#include <OsmAndCore/Map/BinaryMapRasterLayerProvider_Metrics.h>
#include <OsmAndCore/Map/BinaryMapPrimitivesProvider.h>

namespace OsmAnd
{
    class BinaryMapRasterLayerProvider_P;
    class OSMAND_CORE_API BinaryMapRasterLayerProvider : public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapRasterLayerProvider);
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
                const std::shared_ptr<const BinaryMapPrimitivesProvider::Data>& binaryMapData,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const BinaryMapPrimitivesProvider::Data> binaryMapData;
        };

    private:
        PrivateImplementation<BinaryMapRasterLayerProvider_P> _p;
    protected:
        BinaryMapRasterLayerProvider(
            BinaryMapRasterLayerProvider_P* const p,
            const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider);
    public:
        virtual ~BinaryMapRasterLayerProvider();

        const std::shared_ptr<BinaryMapPrimitivesProvider> primitivesProvider;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
            const IQueryController* const queryController = nullptr);

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<Data>& outTiledData,
            BinaryMapRasterLayerProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_H_)
