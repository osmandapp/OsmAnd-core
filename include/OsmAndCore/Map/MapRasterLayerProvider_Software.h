#ifndef _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_
#define _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapRasterLayerProvider.h>
#include <OsmAndCore/Map/MapRasterLayerProvider_Metrics.h>

namespace OsmAnd
{
    class MapPrimitivesProvider;
    class MapRasterLayerCache;
    class MapRasterLayerProvider_Software_P;
    class MapRenderer;
    class OSMAND_CORE_API MapRasterLayerProvider_Software : public MapRasterLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapRasterLayerProvider_Software);
    private:
        mutable QMutex _localCacheMutex;
        std::shared_ptr<MapRasterLayerCache> _localCache;
        unsigned int _rendererBindings = 0;
        bool _cacheSuspended = false;
        std::shared_ptr<MapRasterLayerCache> getLocalCache() const;
        void attachToRenderer();
        void detachFromRenderer();
        friend class MapRenderer;
        friend class MapRasterLayerProvider_Software_P;
    protected:
    public:
        MapRasterLayerProvider_Software(
            const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider,
            const bool fillBackground = true,
            const bool forceObtainDataAsync = false,
            const bool adjustToDetailedZoom = false);
        virtual ~MapRasterLayerProvider_Software();

        // Style and raster resources are fingerprinted automatically when the cache is enabled.
        // Recreate the presentation environment/provider chain when style or resource contents change.
        // maxBytes limits stored tile payloads; SQLite reuses freed pages until clearLocalCache().
        // The symbols provider should share this provider's MapPrimitivesProvider.
        // The inclusive zoom range applies to tile zoom.
        // An empty filename or a zero limit disables the cache.
        bool setLocalCache(const QString& filename,
            uint64_t maxBytes = 256ull * 1024 * 1024, ZoomLevel minZoom = ZoomLevel9, ZoomLevel maxZoom = ZoomLevel15);
        bool clearLocalCache();
        // Remove records created before time (Unix epoch milliseconds); equality is retained.
        bool removeOlderTilesFromCache(int64_t time);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_)
