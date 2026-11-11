#ifndef _OSMAND_CORE_MAP_TILE_DISK_CACHE_H_
#define _OSMAND_CORE_MAP_TILE_DISK_CACHE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>
#include <QByteArray>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class MapTileDiskCache_P;

    // Persistent cache of rendered map tiles (WebP images) and of their filtered map symbols,
    // kept in one SQLite file with a size limit.
    //
    // What makes an entry stale and how it is cleaned:
    //  - an OBF file changed (size or creation date), appeared or disappeared: entries whose tile
    //    intersects that file's bbox are deleted at once (syncMaps);
    //  - rendering settings changed (style, its settings, density, text scale, language, ...): the
    //    caller gives each set of settings a 64-bit environment hash; entries of the last
    //    keptEnvironments hashes stay (day/night, label language), older ones are deleted at once,
    //    unless used within the last 10 minutes (useEnvironment);
    //  - over the size limit: entries are evicted by idle time divided by the zoom weight, so tiles
    //    of z9-z14 (and z12 most of all) live longer than z15 ones (put);
    //  - a broken or older-format file is deleted and created again (open).
    class OSMAND_CORE_API MapTileDiskCache
    {
        Q_DISABLE_COPY_AND_MOVE(MapTileDiskCache);
    public:
        enum class DataKind : int32_t
        {
            RasterImage = 1,
            Symbols = 2,
        };

        struct OSMAND_CORE_API MapFile
        {
            QString path;
            int64_t size;
            int64_t timestamp;
            AreaI bbox31;
        };

        struct OSMAND_CORE_API Key
        {
            uint64_t environment;
            DataKind kind;
            TileId tileId;
            ZoomLevel zoom;
            // everything else that changes the tile content: detailed zoom, background fill, ...
            int32_t variant;
        };

        enum
        {
            DefaultLimitBytes = 128 * 1024 * 1024,
            // day and night, each with two label languages (OsmAnd switches it on basemap zooms)
            DefaultKeptEnvironments = 4,
            // a hit refreshes the access time only when it is older than this, so reads stay reads
            AccessTimeResolutionSeconds = 10 * 60,
        };
        static const ZoomLevel MaxCachedZoom = ZoomLevel15;

    private:
        PrivateImplementation<MapTileDiskCache_P> _p;
    protected:
    public:
        MapTileDiskCache(const QString& filePath, const int64_t limitBytes = DefaultLimitBytes);
        virtual ~MapTileDiskCache();

        const QString filePath;

        bool isOpen() const;
        void setLimit(const int64_t limitBytes);
        int64_t getLimit() const;
        void setKeptEnvironments(const int count);
        int64_t getTotalBytes() const;
        int getEntriesCount() const;
        void clear();

        static bool isZoomCached(const ZoomLevel zoom);
        // eviction weight: an entry of weight w is kept w times longer than an entry of weight 1
        static int getZoomWeight(const ZoomLevel zoom);

#if !defined(SWIG)
        // Marks the environment as the current one and drops entries of environments beyond keptEnvironments
        void useEnvironment(const uint64_t environment);
        // Brings the stored map list in line with the current one, deleting entries of changed areas
        // Returns the number of deleted entries
        int syncMaps(const QList<MapFile>& maps);

        bool contains(const Key& key) const;
        QByteArray get(const Key& key);
        bool put(const Key& key, const QByteArray& data);

        // seconds since epoch; replaced in tests
        void setClock(const std::function<int64_t()>& clock);
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_MAP_TILE_DISK_CACHE_H_)
