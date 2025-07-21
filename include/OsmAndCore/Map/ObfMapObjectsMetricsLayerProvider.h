#ifndef _OSMAND_CORE_OBF_MAP_OBJECTS_METRICS_LAYER_PROVIDER_H_
#define _OSMAND_CORE_OBF_MAP_OBJECTS_METRICS_LAYER_PROVIDER_H_

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
#include <OsmAndCore/Map/ObfMapObjectsProvider.h>

namespace OsmAnd
{
    class ObfMapObjectsMetricsLayerProvider_P;
    class OSMAND_CORE_API ObfMapObjectsMetricsLayerProvider
        : public std::enable_shared_from_this<ObfMapObjectsMetricsLayerProvider>
        , public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(ObfMapObjectsMetricsLayerProvider);
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
                const std::shared_ptr<const ObfMapObjectsProvider::Data>& binaryMapData,
                const RetainableCacheMetadata* const pRetainableCacheMetadata = nullptr);
            virtual ~Data();

            std::shared_ptr<const ObfMapObjectsProvider::Data> binaryMapData;
        };

    private:
        PrivateImplementation<ObfMapObjectsMetricsLayerProvider_P> _p;
    protected:
    public:
        ObfMapObjectsMetricsLayerProvider(
            const std::shared_ptr<ObfMapObjectsProvider>& dataProvider,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f);
        virtual ~ObfMapObjectsMetricsLayerProvider();

        const std::shared_ptr<ObfMapObjectsProvider> dataProvider;
        const uint32_t _tileSize;
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

#endif // !defined(_OSMAND_CORE_OBF_MAP_OBJECTS_METRICS_LAYER_PROVIDER_H_)
