#ifndef _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_H_
#define _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QMutex>
#include <QSet>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>
#include <OsmAndCore/Map/MapRasterLayerProvider_Metrics.h>
#include <OsmAndCore/Map/MapPrimitivesProvider.h>

namespace OsmAnd
{
    class MapRasterLayerProvider_P;
    class OSMAND_CORE_API MapRasterLayerProvider
        : public std::enable_shared_from_this<MapRasterLayerProvider>
        , public IRasterMapLayerProvider
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
                const AlphaChannelPresence alphaChannelPresence,
                const float densityFactor,
                const sk_sp<const SkImage>& image,
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
            const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider,
            const bool fillBackground,
            const bool online);
    public:
        virtual ~MapRasterLayerProvider();

        const std::shared_ptr<MapPrimitivesProvider> primitivesProvider;
        const bool fillBackground;
        const bool online;

        virtual MapStubStyle getDesiredStubsStyle() const;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE;

        bool obtainRasterizedTile(
            const Request& request,
            std::shared_ptr<Data>& outData,
            MapRasterLayerProvider_Metrics::Metric_obtainData* const metric = nullptr);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_H_)
