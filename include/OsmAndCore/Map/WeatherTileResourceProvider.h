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
#include <OsmAndCore/Map/GeoContour.h>
#include <OsmAndCore/Map/GeoBandSettings.h>
#include <OsmAndCore/Map/WeatherCommonTypes.h>

static const int64_t kGeoTileExpireTime = 1000 * 60 * 60 * 6; // 6 hours

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
            bool localData;

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
            bool localData;
            bool cacheOnly;

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
            bool localData;

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
                sk_sp<const SkImage> image,
                QHash<BandIndex, QList<std::shared_ptr<GeoContour>>> contourMap = QHash<BandIndex, QList<std::shared_ptr<GeoContour>>>());
            virtual ~Data();

            TileId tileId;
            ZoomLevel zoom;
            AlphaChannelPresence alphaChannelPresence;
            float densityFactor;
            sk_sp<const SkImage> image;
            QHash<BandIndex, QList<std::shared_ptr<GeoContour>>> contourMap;
        };

        OSMAND_CALLABLE(ObtainValueAsyncCallback,
            void,
            bool succeeded,
            const QList<double>& values,
            const std::shared_ptr<Metric>& metric);

        OSMAND_CALLABLE(ObtainTileDataAsyncCallback,
            void,
            bool requestSucceeded,
            const std::shared_ptr<Data>& data,
            const std::shared_ptr<Metric>& metric);

        OSMAND_CALLABLE(DownloadGeoTilesAsyncCallback,
            void,
            bool succeeded,
            uint64_t downloadedTiles,
            uint64_t totalTiles,
            const std::shared_ptr<Metric>& metric);

    protected:
    public:
        WeatherTileResourceProvider(
            const int64_t dateTime,
            const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings,
            const QString& localCachePath,
            const QString& projResourcesPath,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f,
            const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient()));
        virtual ~WeatherTileResourceProvider();

        bool networkAccessAllowed;

        virtual void obtainValue(
            const ValueRequest& request,
            const ObtainValueAsyncCallback callback,
            const bool collectMetric = false);

        virtual void obtainValueAsync(
            const ValueRequest& request,
            const ObtainValueAsyncCallback callback,
            const bool collectMetric = false);

        virtual void obtainData(
            const TileRequest& request,
            const ObtainTileDataAsyncCallback callback,
            const bool collectMetric = false);

        virtual void obtainDataAsync(
            const TileRequest& request,
            const ObtainTileDataAsyncCallback callback,
            const bool collectMetric = false);

        virtual void downloadGeoTiles(
            const DownloadGeoTileRequest& request,
            const DownloadGeoTilesAsyncCallback callback,
            const bool collectMetric = false);

        virtual void downloadGeoTilesAsync(
            const DownloadGeoTileRequest& request,
            const DownloadGeoTilesAsyncCallback callback,
            const bool collectMetric = false);

        void setBandSettings(const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings);

        int getCurrentRequestVersion() const;

        static ZoomLevel getGeoTileZoom();
        static ZoomLevel getTileZoom(const WeatherLayer layer);
        static WeatherLayer getWeatherLayerByZoom(const ZoomLevel zoom);
        static int getMaxMissingDataZoomShift(const WeatherLayer layer);
        static int getMaxMissingDataUnderZoomShift(const WeatherLayer layer);
        int64_t getDateTime();

        bool isEmpty();

        bool isDownloadingTiles() const;
        bool isEvaluatingTiles() const;
        QList<TileId> getCurrentDownloadingTileIds() const;
        QList<TileId> getCurrentEvaluatingTileIds() const;

        uint64_t calculateTilesSize(
            const QList<TileId>& tileIds,
            const QList<TileId>& excludeTileIds,
            const ZoomLevel zoom);

        bool removeTileData(
            const QList<TileId>& tileIds,
            const QList<TileId>& excludeTileIds,
            const ZoomLevel zoom);

        bool closeProvider();
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_TILE_RESOURCE_PROVIDER_H_)
