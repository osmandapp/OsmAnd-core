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

        QHash<BandIndex, float> _bandOpacityMap;
        
        void updateProvidersBandOpacityMap();
        
    protected:
        WeatherTileResourcesManager_P(
            WeatherTileResourcesManager* const owner,
            const QHash<BandIndex, float>& bandOpacityMap,
            const QHash<BandIndex, QString>& bandColorProfilePaths,
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

        const QHash<BandIndex, float> getBandOpacityMap() const;
        void setBandOpacityMap(const QHash<BandIndex, float>& bandOpacityMap);

        const QHash<BandIndex, QString> bandColorProfilePaths;
        const QString localCachePath;
        const QString projResourcesPath;
        const uint32_t tileSize;
        const float densityFactor;

        ZoomLevel getGeoTileZoom() const;
        ZoomLevel getTileZoom(const WeatherLayer layer) const;
        int getMaxMissingDataZoomShift(const WeatherLayer layer) const;
        int getMaxMissingDataUnderZoomShift(const WeatherLayer layer) const;
        
        std::shared_ptr<WeatherTileResourceProvider> getResourceProvider(const QDateTime& dateTime);

        void obtainDataAsync(
            const WeatherTileResourcesManager::TileRequest& request,
            const WeatherTileResourcesManager::ObtainTileDataAsyncCallback callback,
            const bool collectMetric = false);
        
        void downloadGeoTilesAsync(
            const WeatherTileResourcesManager::DownloadGeoTileRequest& request,
            const WeatherTileResourcesManager::DownloadGeoTilesAsyncCallback callback,
            const bool collectMetric = false);

        bool clearDbCache(const bool clearGeoCache, const bool clearRasterCache);
        
    friend class OsmAnd::WeatherTileResourcesManager;
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_TILE_RESOURCES_MANAGER_P_H_)
