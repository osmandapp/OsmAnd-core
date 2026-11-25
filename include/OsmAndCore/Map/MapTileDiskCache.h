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
    //    keptEnvironments hashes stay (day/night, label language), older ones are deleted
    //    once they have not been used for 10 minutes (useEnvironment);
    //  - over 90% of the size limit: the entries of the lowest weight are deleted down to 60%.
    //
    // Weight of an entry, in half-lives (HalfLifeDays) of not being used:
    //   weight = log2(sum over visits of 2^(-(now - visit) / halfLife)) + now / halfLife   (decayed visit count)
    //          + log2(zoom weight)                       (z9-z14 x4, z12 x8)
    //          + GenerationTimeFactor * log2(1 + generation time, ms)
    // The first line is kept as one number, so neither the visit times nor now are needed to order
    // entries: a week without use costs 1, a visit adds log2(1 + decayed count). A visit is an access
    // after VisitGapMinutes without one; accesses within a visit only bring the entry to "used now".
    // Accesses are counted in memory and written every FlushSeconds and on flush().
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

        struct OSMAND_CORE_API EntryInfo
        {
            int64_t added;
            int64_t lastAccess;
            int hits;
            // CPU time of reading, styling and drawing the tile
            int generationTimeMs;
            double weight;
            int size;
        };

        enum
        {
            DefaultLimitBytes = 256 * 1024 * 1024,
            // day and night, each with two label languages (OsmAnd switches it on basemap zooms)
            DefaultKeptEnvironments = 4,
            HalfLifeDays = 7,
            VisitGapMinutes = 30,
            FlushSeconds = 10,
            // over EvictAbovePercent of the limit entries are deleted down to EvictToPercent
            EvictAbovePercent = 90,
            EvictToPercent = 60,
            // background writes waiting for encoding; more are dropped, they come again with the next render
            WriteQueueLimit = 64,
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
        // writes the counted accesses
        void flush();

        static bool isZoomCached(const ZoomLevel zoom);
        static int getZoomWeight(const ZoomLevel zoom);

#if !defined(SWIG)
        static const double GenerationTimeFactor;

        // weight of an entry added at time t
        static double getInitialWeight(const int64_t t, const ZoomLevel zoom, const int generationTimeMs);
        // weight after accesses between firstAccess and lastAccess, the previous one being at previousAccess
        static double getAccessedWeight(const double weight, const ZoomLevel zoom, const int generationTimeMs,
            const int64_t previousAccess, const int64_t firstAccess, const int64_t lastAccess);
        // CPU time of the calling thread, ms
        static double getThreadTimeMs();

        // Marks the environment as the current one and drops entries of environments beyond keptEnvironments
        void useEnvironment(const uint64_t environment);
        // Brings the stored map list in line with the current one, deleting entries of changed areas
        // Returns the number of deleted entries
        int syncMaps(const QList<MapFile>& maps);

        bool contains(const Key& key) const;
        QByteArray get(const Key& key);
        bool getEntryInfo(const Key& key, EntryInfo& outInfo);
        bool put(const Key& key, const QByteArray& data, const int generationTimeMs);
        // makes the data on the cache's own thread (encoding, compression) and stores it there
        void putLater(const Key& key, const std::function<QByteArray()>& makeData, const int generationTimeMs);

        // seconds since epoch; replaced in tests
        void setClock(const std::function<int64_t()>& clock);
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_MAP_TILE_DISK_CACHE_H_)
