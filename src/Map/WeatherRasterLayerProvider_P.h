#ifndef _OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_P_H_
#define _OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QDir>
#include <QMutex>
#include <QQueue>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "WeatherRasterLayerProvider.h"
#include "TileSqliteDatabase.h"

namespace OsmAnd
{
    class WeatherRasterLayerProvider_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(WeatherRasterLayerProvider_P);
    private:
        mutable QReadWriteLock _dataLock;
        mutable QMutex _dataMutex;
        TileSqliteDatabase *_geoDb;
        TileSqliteDatabase *_rasterDb;
        const std::shared_ptr<const IWebClient> _webClient;

        static QString doubleToStr(double value);

    protected:
        WeatherRasterLayerProvider_P(
            WeatherRasterLayerProvider* const owner,
            const QDateTime& dateTime,
            const int bandIndex,
            const QString& colorProfilePath,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f,
            const QString& dataCachePath = QString(),
            const QString& projSearchPath = QString(),
            const std::shared_ptr<const IWebClient> webClient = nullptr);
        
    public:
        ~WeatherRasterLayerProvider_P();

        ImplementationInterface<WeatherRasterLayerProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::WeatherRasterLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_RASTER_LAYER_PROVIDER_P_H_)
