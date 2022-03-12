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
#include <OsmAndCore/Color.h>
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
        struct OSMAND_CORE_API ValueRequest
        {
            ValueRequest();
            ValueRequest(const ValueRequest& that);
            virtual ~ValueRequest();

            PointI point31;
            ZoomLevel zoom;
            BandIndex band;
            
            std::shared_ptr<const IQueryController> queryController;

            static void copy(ValueRequest& dst, const ValueRequest& src);
            virtual std::shared_ptr<ValueRequest> clone() const;
        };
        
        struct OSMAND_CORE_API TileRequest
        {
            TileRequest();
            TileRequest(const TileRequest& that);
            virtual ~TileRequest();

            WeatherType weatherType;
            TileId tileId;
            ZoomLevel zoom;
            QList<BandIndex> bands;

            int version;
            bool ignoreVersion;

            std::shared_ptr<const IQueryController> queryController;

            static void copy(TileRequest& dst, const TileRequest& src);
            virtual std::shared_ptr<TileRequest> clone() const;
        };
        
        struct OSMAND_CORE_API DownloadGeoTileRequest
        {
            DownloadGeoTileRequest();
            DownloadGeoTileRequest(const DownloadGeoTileRequest& that);
            virtual ~DownloadGeoTileRequest();

            LatLon topLeft;
            LatLon bottomRight;
            bool forceDownload;

            std::shared_ptr<const IQueryController> queryController;

            static void copy(DownloadGeoTileRequest& dst, const DownloadGeoTileRequest& src);
            virtual std::shared_ptr<DownloadGeoTileRequest> clone() const;
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

        OSMAND_CALLABLE(ObtainValueAsyncCallback,
            void,
            const bool succeeded,
            const double value,
            const std::shared_ptr<Metric>& metric);
        
        OSMAND_CALLABLE(ObtainTileDataAsyncCallback,
            void,
            const bool requestSucceeded,
            const std::shared_ptr<Data>& data,
            const std::shared_ptr<Metric>& metric);
        
        OSMAND_CALLABLE(DownloadGeoTilesAsyncCallback,
            void,
            const bool succeeded,
            const uint64_t downloadedTiles,
            const uint64_t totalTiles,
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

        virtual void obtainValueAsync(
            const ValueRequest& request,
            const ObtainValueAsyncCallback callback,
            const bool collectMetric = false);
        
        virtual void obtainDataAsync(
            const TileRequest& request,
            const ObtainTileDataAsyncCallback callback,
            const bool collectMetric = false);

        virtual void downloadGeoTilesAsync(
            const DownloadGeoTileRequest& request,
            const DownloadGeoTilesAsyncCallback callback,
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
