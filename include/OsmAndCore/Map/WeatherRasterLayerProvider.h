#ifndef _OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_
#define _OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QString>
#include <QDateTime>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>
#include <OsmAndCore/IWebClient.h>

namespace OsmAnd
{
    class WeatherRasterLayerProvider_P;
    class OSMAND_CORE_API WeatherRasterLayerProvider
        : public std::enable_shared_from_this<WeatherRasterLayerProvider>
        , public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(WeatherRasterLayerProvider);

    private:
        PrivateImplementation<WeatherRasterLayerProvider_P> _p;
    protected:
        QThreadPool *_threadPool;
        
        mutable QReadWriteLock _lock;
        ZoomLevel _lastRequestedZoom;
        int _priority;

        ZoomLevel getLastRequestedZoom() const;
        void setLastRequestedZoom(const ZoomLevel zoomLevel);
        int getAndDecreasePriority();

    public:
        WeatherRasterLayerProvider(const QDateTime& dateTime,
                                   const int bandIndex,
                                   const QString& colorProfilePath,
                                   const uint32_t tileSize = 256,
                                   const float densityFactor = 1.0f,
                                   const QString& dataCachePath = QString(),
                                   const QString& projSearchPath = QString(),
                                   const std::shared_ptr<const IWebClient> webClient = nullptr);
        virtual ~WeatherRasterLayerProvider();
        
        const QDateTime dateTime;
        const int bandIndex;
        const QString colorProfilePath;
        const uint32_t tileSize;
        const float densityFactor;
        const QString dataCachePath;
        const QString projSearchPath;

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

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMinVisibleZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxVisibleZoom() const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_)
