#ifndef _OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_
#define _OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IRasterMapLayerProvider.h>

namespace OsmAnd
{
    class WeatherRasterLayerProvider_P;
    class OSMAND_CORE_API WeatherRasterLayerProvider : public IRasterMapLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(WeatherRasterLayerProvider);

    private:
        PrivateImplementation<WeatherRasterLayerProvider_P> _p;
    protected:
    public:
        WeatherRasterLayerProvider(const QString& geotiffPath,
                                   const uint32_t tileSize = 256,
                                   const float densityFactor = 1.0f,
                                   const QString& cacheDbFilename = QString());
        virtual ~WeatherRasterLayerProvider();

        const QString geotiffPath;
        const uint32_t tileSize;
        const float densityFactor;
        const QString cacheDbFilename;
        
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
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_H_)
