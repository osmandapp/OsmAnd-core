#ifndef _OSMAND_CORE_WEATHER_TILE_RESOURCES_MANAGER_H_
#define _OSMAND_CORE_WEATHER_TILE_RESOURCES_MANAGER_H_

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

namespace OsmAnd
{
    class WeatherTileResourcesManager_P;

    class OSMAND_CORE_API WeatherTileResourcesManager
        : public std::enable_shared_from_this<WeatherTileResourcesManager>
    {
        Q_DISABLE_COPY_AND_MOVE(WeatherTileResourcesManager);
    private:
        PrivateImplementation<WeatherTileResourcesManager_P> _p;

    public:
        struct OSMAND_CORE_API ValueRequest
        {
            ValueRequest();
            ValueRequest(const ValueRequest& that);
            virtual ~ValueRequest();

            QDateTime dataTime;
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

            WeatherLayer weatherLayer;
            WeatherType weatherType;
            QDateTime dataTime;
            TileId tileId;
            ZoomLevel zoom;
            QList<BandIndex> bands;
            bool localData;

            std::shared_ptr<const IQueryController> queryController;

            static void copy(TileRequest& dst, const TileRequest& src);
            virtual std::shared_ptr<TileRequest> clone() const;
        };

        struct OSMAND_CORE_API DownloadGeoTileRequest
        {
            DownloadGeoTileRequest();
            DownloadGeoTileRequest(const DownloadGeoTileRequest& that);
            virtual ~DownloadGeoTileRequest();

            QDateTime dataTime;
            LatLon topLeft;
            LatLon bottomRight;
            bool forceDownload;
            bool localData;

            std::shared_ptr<const IQueryController> queryController;

            static void copy(DownloadGeoTileRequest& dst, const DownloadGeoTileRequest& src);
            virtual std::shared_ptr<DownloadGeoTileRequest> clone() const;
        };

        struct OSMAND_CORE_API FileRequest
        {
            FileRequest();
            FileRequest(const FileRequest& that);
            virtual ~FileRequest();

            QDateTime dataTime;
            LatLon topLeft;
            LatLon bottomRight;
            bool localData;

            std::shared_ptr<const IQueryController> queryController;

            static void copy(FileRequest& dst, const FileRequest& src);
            virtual std::shared_ptr<FileRequest> clone() const;
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
                QHash<BandIndex, QList<Ref<GeoContour>>> contourMap = QHash<BandIndex, QList<Ref<GeoContour>>>());
            virtual ~Data();

            TileId tileId;
            ZoomLevel zoom;
            AlphaChannelPresence alphaChannelPresence;
            float densityFactor;
            sk_sp<const SkImage> image;
            QHash<BandIndex, QList<Ref<GeoContour>>> contourMap;
        };

        OSMAND_CALLABLE(ObtainValueAsyncCallback,
            void,
            bool succeeded,
            double value,
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
            int downloadedTileSize,
            const std::shared_ptr<Metric>& metric);

        OSMAND_CALLABLE(FileAsyncCallback,
            void,
            bool succeeded,
            long long fileSize,
            const std::shared_ptr<Metric>& metric);

    protected:
    public:
        WeatherTileResourcesManager(
            const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings,
            const QString& localCachePath,
            const QString& projResourcesPath,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f,
            const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient()));
        virtual ~WeatherTileResourcesManager();

        bool networkAccessAllowed;

        const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>> getBandSettings() const;
        void setBandSettings(const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings);

        double getConvertedBandValue(const BandIndex band, const double value) const;
        QString getFormattedBandValue(const BandIndex band, const double value, const bool precise) const;

        QString getLocalCachePath() const;
        QString getProjResourcesPath() const;
        uint32_t getTileSize() const;
        float getDensityFactor() const;

        ZoomLevel getGeoTileZoom() const;
        ZoomLevel getMinTileZoom(const WeatherType type, const WeatherLayer layer) const;
        ZoomLevel getMaxTileZoom(const WeatherType type, const WeatherLayer layer) const;
        int getMaxMissingDataZoomShift(const WeatherType type, const WeatherLayer layer) const;
        int getMaxMissingDataUnderZoomShift(const WeatherType type, const WeatherLayer layer) const;

        static QVector<TileId> generateGeoTileIds(
                const LatLon topLeft,
                const LatLon bottomRight,
                const ZoomLevel zoom);

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

        virtual void obtainFile(
            const FileRequest& request,
            const FileAsyncCallback callback,
            const bool collectMetric = false);

        virtual void obtainFileAsync(
            const FileRequest& request,
            const FileAsyncCallback callback,
            const bool collectMetric = false);

        virtual void downloadGeoTiles(
            const DownloadGeoTileRequest& request,
            const DownloadGeoTilesAsyncCallback callback,
            const bool collectMetric = false);

        virtual void downloadGeoTilesAsync(
            const DownloadGeoTileRequest& request,
            const DownloadGeoTilesAsyncCallback callback,
            const bool collectMetric = false);

        virtual bool clearLocalDbCache(
                const QVector<TileId> tileIds,
                const ZoomLevel zoom);

        virtual bool clearDbCache(
                const bool localData = false,
                const QDateTime beforeDateTime = QDateTime());
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_TILE_RESOURCES_MANAGER_H_)
