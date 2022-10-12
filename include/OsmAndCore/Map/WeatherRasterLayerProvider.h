#ifndef _OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_
#define _OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QString>
#include <QDateTime>
#include <QReadWriteLock>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>
#include <OsmAndCore/Map/GeoCommonTypes.h>
#include <OsmAndCore/Map/WeatherCommonTypes.h>
#include <OsmAndCore/Map/WeatherTileResourcesManager.h>

namespace OsmAnd
{
    class OSMAND_CORE_API WeatherRasterLayerProvider
        : public std::enable_shared_from_this<WeatherRasterLayerProvider>
        , public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(WeatherRasterLayerProvider);

    private:
        const std::shared_ptr<WeatherTileResourcesManager> _resourcesManager;
        
        mutable QReadWriteLock _lock;

        int64_t _dateTime;
        QList<BandIndex> _bands;
        bool _localData;

    protected:
    public:
        WeatherRasterLayerProvider(const std::shared_ptr<WeatherTileResourcesManager> resourcesManager,
                                   const WeatherLayer weatherLayer,
                                   const int64_t dateTime,
                                   const QList<BandIndex> bands,
                                   const bool localData);
        virtual ~WeatherRasterLayerProvider();
        
        const WeatherLayer weatherLayer;

        const int64_t getDateTime() const;
        void setDateTime(int64_t dateTime);
        const QList<BandIndex> getBands() const;
        void setBands(const QList<BandIndex>& bands);
        const bool getLocalData() const;
        void setLocalData(const bool localData);

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
        
        virtual int getMaxMissingDataZoomShift() const Q_DECL_OVERRIDE;
        virtual int getMaxMissingDataUnderZoomShift() const Q_DECL_OVERRIDE;

    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_)
