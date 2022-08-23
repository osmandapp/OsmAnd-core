#ifndef _OSMAND_CORE_WEATHER_TILE_RESOURCES_MANAGER_P_H_
#define _OSMAND_CORE_WEATHER_TILE_RESOURCES_MANAGER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QDir>
#include <QMutex>
#include <QQueue>
#include <QSet>
#include "restore_internal_warnings.h"

#include "WeatherTileResourcesManager.h"
#include "WeatherTileResourceProvider.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IWebClient.h"

namespace OsmAnd
{
    class WeatherTileResourcesManager_P Q_DECL_FINAL
        : public std::enable_shared_from_this<WeatherTileResourcesManager_P>
    {
        Q_DISABLE_COPY_AND_MOVE(WeatherTileResourcesManager_P);
    private:
        mutable QReadWriteLock _resourceProvidersLock;
        QHash<QString, std::shared_ptr<WeatherTileResourceProvider>> _resourceProviders;
        
        std::shared_ptr<WeatherTileResourceProvider> createResourceProvider(const QDateTime& dateTime);

        mutable QReadWriteLock _bandSettingsLock;
        QHash<BandIndex, std::shared_ptr<const GeoBandSettings>> _bandSettings;
        void updateProvidersBandSettings();

    protected:
        WeatherTileResourcesManager_P(
            WeatherTileResourcesManager* const owner,
            const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings,
            const QString& localCachePath,
            const QString& projResourcesPath,
            const uint32_t tileSize = 256,
            const float densityFactor = 1.0f,
            const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient())
        );
        
    public:
        ~WeatherTileResourcesManager_P();

        ImplementationInterface<WeatherTileResourcesManager> owner;

        const std::shared_ptr<const IWebClient> webClient;

        const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>> getBandSettings() const;
        void setBandSettings(const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings);

        double getConvertedBandValue(const BandIndex band, const double value) const;
        QString getFormattedBandValue(const BandIndex band, const double value, const bool precise) const;

        const QString localCachePath;
        const QString projResourcesPath;
        const uint32_t tileSize;
        const float densityFactor;

        ZoomLevel getGeoTileZoom() const;
        ZoomLevel getMinTileZoom(const WeatherType type, const WeatherLayer layer) const;
        ZoomLevel getMaxTileZoom(const WeatherType type, const WeatherLayer layer) const;
        int getMaxMissingDataZoomShift(const WeatherType type, const WeatherLayer layer) const;
        int getMaxMissingDataUnderZoomShift(const WeatherType type, const WeatherLayer layer) const;

        std::shared_ptr<WeatherTileResourceProvider> getResourceProvider(const QDateTime& dateTime);
        
        void obtainMinMaxValue(const QDateTime date,
                               const BandIndex band,
                               const QVector<TileId>& visibleTiles,
                               const ZoomLevel zoom);

        void obtainValue(
            const WeatherTileResourcesManager::ValueRequest& request,
            const WeatherTileResourcesManager::ObtainValueAsyncCallback callback,
            const bool collectMetric = false);

        void obtainValueAsync(
            const WeatherTileResourcesManager::ValueRequest& request,
            const WeatherTileResourcesManager::ObtainValueAsyncCallback callback,
            const bool collectMetric = false);
        
        void obtainData(
            const WeatherTileResourcesManager::TileRequest& request,
            const WeatherTileResourcesManager::ObtainTileDataAsyncCallback callback,
            const bool collectMetric = false);

        void obtainDataAsync(
            const WeatherTileResourcesManager::TileRequest& request,
            const WeatherTileResourcesManager::ObtainTileDataAsyncCallback callback,
            const bool collectMetric = false);

        void downloadGeoTiles(
            const WeatherTileResourcesManager::DownloadGeoTileRequest& request,
            const WeatherTileResourcesManager::DownloadGeoTilesAsyncCallback callback,
            const bool collectMetric = false);

        void downloadGeoTilesAsync(
            const WeatherTileResourcesManager::DownloadGeoTileRequest& request,
            const WeatherTileResourcesManager::DownloadGeoTilesAsyncCallback callback,
            const bool collectMetric = false);

        uint64_t calculateDbCacheSize(
            const QList<TileId>& tileIds,
            const QList<TileId>& excludeTileIds,
            const ZoomLevel zoom);

        bool clearDbCache(
            const QList<TileId>& tileIds,
            const QList<TileId>& excludeTileIds,
            const ZoomLevel zoom);

        bool clearDbCache(const QDateTime clearBeforeDateTime = QDateTime());

    friend class OsmAnd::WeatherTileResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_TILE_RESOURCES_MANAGER_P_H_)
