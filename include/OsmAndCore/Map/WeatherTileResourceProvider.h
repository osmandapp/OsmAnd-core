#ifndef _OSMAND_CORE_WEATHER_TILE_RESOURCE_PROVIDER_H_
#define _OSMAND_CORE_WEATHER_TILE_RESOURCE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QDir>
#include <QDateTime>

#include <SkImage.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/WebClient.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Map/GeoCommonTypes.h>
#include <OsmAndCore/Map/WeatherCommonTypes.h>

namespace OsmAnd
{
    class WeatherTileResourceProvider_P;
        
    class OSMAND_CORE_API WeatherTileResourceProvider
        : public std::enable_shared_from_this<WeatherTileResourceProvider>
    {
        Q_DISABLE_COPY_AND_MOVE(WeatherTileResourceProvider);
    private:
        PrivateImplementation<WeatherTileResourceProvider_P> _p;
        
    public:
        struct OSMAND_CORE_API TileRequest
        {
            TileRequest();
            TileRequest(const TileRequest& that);
            virtual ~TileRequest();

            TileId tileId;
            ZoomLevel zoom;
            QList<BandIndex> bands;

            int version;

            std::shared_ptr<const IQueryController> queryController;

            static void copy(TileRequest& dst, const TileRequest& src);
            virtual std::shared_ptr<TileRequest> clone() const;
        };
        
        struct OSMAND_CORE_API GeoTileRequest
        {
            GeoTileRequest();
            GeoTileRequest(const GeoTileRequest& that);
            virtual ~GeoTileRequest();

            LatLon topLeft;
            LatLon bottomRight;
            ZoomLevel zoom;

            std::shared_ptr<const IQueryController> queryController;

            static void copy(GeoTileRequest& dst, const GeoTileRequest& src);
            virtual std::shared_ptr<GeoTileRequest> clone() const;
        };
        
        class OSMAND_CORE_API Data
        {
            Q_DISABLE_COPY_AND_MOVE(Data);
        private:
        protected:
        public:
            Data(
                TileId tileId,
                ZoomLevel zoom,
                AlphaChannelPresence alphaChannelPresence,
                float densityFactor,
                sk_sp<const SkImage> image);
            virtual ~Data();

            TileId tileId;
            ZoomLevel zoom;
            AlphaChannelPresence alphaChannelPresence;
            float densityFactor;
            sk_sp<const SkImage> image;
        };

        OSMAND_CALLABLE(ObtainTileDataAsyncCallback,
            void,
            const bool requestSucceeded,
            const std::shared_ptr<Data>& data,
            const std::shared_ptr<Metric>& metric);
        
    protected:
    public:
        WeatherTileResourceProvider(
            const QDateTime& dateTime,
            const QHash<BandIndex, float>& bandOpacityMap,
            const QHash<BandIndex, QString>& bandColorProfilePaths,
            const QString& localCachePath,
            const QString& projResourcesPath,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f,
            const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient()));
        virtual ~WeatherTileResourceProvider();

        bool networkAccessAllowed;

        virtual void obtainDataAsync(
            const TileRequest& request,
            const ObtainTileDataAsyncCallback callback,
            const bool collectMetric = false);

        void setBandOpacityMap(const QHash<BandIndex, float>& bandOpacityMap);

        int getCurrentRequestVersion() const;
        
        static ZoomLevel getGeoTileZoom();
        static ZoomLevel getTileZoom(const WeatherLayer layer);
        static WeatherLayer getWeatherLayerByZoom(const ZoomLevel zoom);
        static int getMaxMissingDataZoomShift(const WeatherLayer layer);
        static int getMaxMissingDataUnderZoomShift(const WeatherLayer layer);
        
        bool closeProvider();
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_TILE_RESOURCE_PROVIDER_H_)
