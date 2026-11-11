#ifndef _OSMAND_CORE_MAP_TILE_DISK_CACHE_P_H_
#define _OSMAND_CORE_MAP_TILE_DISK_CACHE_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMutex>
#include <QHash>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <sqlite3.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapTileDiskCache.h"

namespace OsmAnd
{
    class MapTileDiskCache_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapTileDiskCache_P);
    public:
        typedef MapTileDiskCache::Key Key;
        typedef MapTileDiskCache::MapFile MapFile;

        // bump when the table layout or the format of stored data changes: the file is created again
        static const int SchemaVersion = 1;
        // an environment used within this time is never dropped, so two live renderers do not evict each other
        static const int EnvironmentGraceSeconds = 10 * 60;

    private:
        mutable QMutex _mutex;
        sqlite3* _db;
        sqlite3_stmt* _getStmt;
        sqlite3_stmt* _containsStmt;
        sqlite3_stmt* _insertStmt;
        sqlite3_stmt* _sizeStmt;
        sqlite3_stmt* _touchStmt;
        int64_t _limit;
        int _keptEnvironments;
        int64_t _totalBytes;
        int _entriesCount;
        // kept environments: hash -> last use
        QHash<uint64_t, int64_t> _environments;
        std::function<int64_t()> _clock;

        int64_t now() const;
        bool openLocked(const bool allowRecreate);
        void closeLocked();
        bool recreateLocked();
        bool exec(const char* sql);
        void checkError(const int rc);
        void recountLocked();
        void loadEnvironmentsLocked();
        int dropEnvironmentsLocked();
        int deleteAreaLocked(const AreaI& bbox31);
        int evictLocked(const int64_t targetBytes);
        void vacuumLocked();
        bool bindKey(sqlite3_stmt* stmt, const Key& key) const;
    protected:
        MapTileDiskCache_P(MapTileDiskCache* const owner, const int64_t limitBytes);
    public:
        ~MapTileDiskCache_P();

        ImplementationInterface<MapTileDiskCache> owner;

        bool open();
        bool isOpen() const;
        void setLimit(const int64_t limitBytes);
        int64_t getLimit() const;
        void setKeptEnvironments(const int count);
        int64_t getTotalBytes() const;
        int getEntriesCount() const;
        void clear();

        void useEnvironment(const uint64_t environment);
        int syncMaps(const QList<MapFile>& maps);
        bool contains(const Key& key) const;
        QByteArray get(const Key& key);
        bool put(const Key& key, const QByteArray& data);
        void setClock(const std::function<int64_t()>& clock);

    friend class OsmAnd::MapTileDiskCache;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_TILE_DISK_CACHE_P_H_)
