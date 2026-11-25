#ifndef _OSMAND_CORE_MAP_RASTER_LAYER_CACHE_H_
#define _OSMAND_CORE_MAP_RASTER_LAYER_CACHE_H_

#include "MapPrimitivesProvider.h"
#include "TileSqliteDatabase.h"
#include <QReadWriteLock>
#include <QThreadPool>
#include <SkImage.h>

namespace OsmAnd
{
    class ObfMapObjectsProvider;
    // Private, optional cache shared by the software raster and primitives providers.
    class MapRasterLayerCache
    {
        Q_DISABLE_COPY_AND_MOVE(MapRasterLayerCache);
        const std::weak_ptr<MapPrimitivesProvider> _provider;
        const std::shared_ptr<const MapPresentationEnvironment> _environment;
        const std::shared_ptr<const ObfMapObjectsProvider> _obfProvider;
        const unsigned int _tileSize;
        const MapPrimitivesProvider::Mode _mode;
        const QByteArray _environmentKey;
        const bool _fillBackground;
        const uint64_t _maxBytes;
        const ZoomLevel _minZoom;
        const ZoomLevel _maxZoom;
        TileSqliteDatabase _database;
        std::shared_ptr<QReadWriteLock> _databaseLock;
        QReadWriteLock _writeAdmissionLock;
        QThreadPool _writePool;
        quint64 _generation = 0;
        // Modified under both locks. Either lock is sufficient for reading.
        bool _suspended = false;

        // The caller holds _databaseLock and has already checked the key length.
        bool isCurrentKey(const QByteArray& key) const;
        template<class Decode> bool readPayload(const IMapTiledDataProvider::Request& request,
            const QByteArray& key, TileSqliteDatabase::RasterPayload payload, Decode decode);
        void writeNow(const IMapTiledDataProvider::Request& request, const QByteArray& key,
            const sk_sp<SkImage>& image, const std::shared_ptr<const MapPrimitivesProvider::Data>& data);
    public:
        MapRasterLayerCache(const std::shared_ptr<MapPrimitivesProvider>& provider,
            const QString& filename, bool fillBackground,
            uint64_t maxBytes, ZoomLevel minZoom, ZoomLevel maxZoom);
        ~MapRasterLayerCache();

        bool isOpened() const;
        void setSuspended(bool suspended);
        QByteArray key(const IMapTiledDataProvider::Request& request) const;
        bool readImage(const IMapTiledDataProvider::Request& request, const QByteArray& key, sk_sp<SkImage>& image);
        bool readPrimitives(const IMapTiledDataProvider::Request& request, const QByteArray& key,
            std::shared_ptr<IMapObjectsProvider::Data>& objects,
            std::shared_ptr<MapPrimitiviser::PrimitivisedObjects>& primitives);
        void write(const IMapTiledDataProvider::Request& request, const QByteArray& key,
            const sk_sp<SkImage>& image, const std::shared_ptr<const MapPrimitivesProvider::Data>& data);
        bool clear();
        bool removeOlderTiles(int64_t time);
    };
}

#endif
