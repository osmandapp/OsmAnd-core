#ifndef _OSMAND_CORE_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_H_
#define _OSMAND_CORE_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_H_

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
#include <OsmAndCore/Map/MapPrimitivesProvider.h>

namespace OsmAnd
{
    class MapPrimitivesMetricsLayerProvider_P;
    class OSMAND_CORE_API MapPrimitivesMetricsLayerProvider Q_DECL_FINAL : public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapPrimitivesMetricsLayerProvider);
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
                const std::shared_ptr<const MapPrimitivesProvider::Data>& binaryMapPrimitives,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const MapPrimitivesProvider::Data> binaryMapPrimitives;
        };

    private:
        PrivateImplementation<MapPrimitivesMetricsLayerProvider_P> _p;
    protected:
    public:
        MapPrimitivesMetricsLayerProvider(
            const std::shared_ptr<MapPrimitivesProvider>& privitivesProvider,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f);
        virtual ~MapPrimitivesMetricsLayerProvider();

        const std::shared_ptr<MapPrimitivesProvider> primitivesProvider;
        const uint32_t tileSize;
        const float densityFactor;

        virtual MapStubStyle getDesiredStubsStyle() const Q_DECL_OVERRIDE;

        virtual float getTileDensityFactor() const Q_DECL_OVERRIDE;
        virtual uint32_t getTileSize() const Q_DECL_OVERRIDE;

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

        bool obtainMetricsTile(
            const Request& request,
            std::shared_ptr<Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr);

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVES_METRICS_LAYER_PROVIDER_H_)
