#ifndef _OSMAND_CORE_MAP_TILE_DISK_CACHE_P_H_
#define _OSMAND_CORE_MAP_TILE_DISK_CACHE_P_H_

#include "stdlib_common.h"
#include <deque>
#include <functional>
#include <thread>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMutex>
#include <QWaitCondition>
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
        typedef MapTileDiskCache::EntryInfo EntryInfo;

        // bump when the table layout or the format of stored data changes: the file is created again
        static const int SchemaVersion = 2;
        // an environment used within this time is never dropped, so two live renderers do not evict each other
        static const int EnvironmentGraceSeconds = 10 * 60;
        // the use time of an environment is written at most this often
        static const int EnvironmentTouchSeconds = 10 * 60;
        // rows deleted per transaction while evicting, so readers are not held up for long
        static const int EvictionChunk = 256;
        // pages given back to the file system per step
        static const int VacuumChunkPages = 2048;

    private:
        struct Access
        {
            int64_t first;
            int64_t last;
            int hits;
        };
        struct Write
        {
            Key key;
            std::function<QByteArray()> makeData;
            int generationTimeMs;
        };

        mutable QMutex _mutex;
        sqlite3* _db;
        sqlite3_stmt* _getStmt;
        sqlite3_stmt* _containsStmt;
        sqlite3_stmt* _insertStmt;
        sqlite3_stmt* _sizeStmt;
        int64_t _limit;
        int _keptEnvironments;
        int64_t _totalBytes;
        int _entriesCount;
        // kept environments: hash -> last use
        QHash<uint64_t, int64_t> _environments;
        // accesses not written yet: rowid -> access
        QHash<int64_t, Access> _accesses;
        int64_t _lastFlush;
        std::function<int64_t()> _clock;

        QMutex _writesMutex;
        QWaitCondition _writesCondition;
        std::deque<Write> _writes;
        std::thread _writer;
        bool _writerStarted;
        bool _stopping;
        bool _maintenanceWanted;

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
        void flushLocked();
        bool isFlushDueLocked(const int64_t t) const;
        bool isOverLimitLocked() const;
        int evict(const int64_t targetBytes);
        void vacuum();
        bool bindKey(sqlite3_stmt* stmt, const Key& key) const;
        void runWriter();
        void wakeWriter();
        void startWriterLocked();
        void maintain();
    protected:
        MapTileDiskCache_P(MapTileDiskCache* const owner, const int64_t limitBytes);
    public:
        ~MapTileDiskCache_P();

        ImplementationInterface<MapTileDiskCache> owner;

        bool open();
        void stop();
        bool isOpen() const;
        void setLimit(const int64_t limitBytes);
        int64_t getLimit() const;
        void setKeptEnvironments(const int count);
        int64_t getTotalBytes() const;
        int getEntriesCount() const;
        void clear();
        void flush();

        void useEnvironment(const uint64_t environment);
        int syncMaps(const QList<MapFile>& maps);
        bool contains(const Key& key) const;
        QByteArray get(const Key& key);
        bool getEntryInfo(const Key& key, EntryInfo& outInfo);
        bool put(const Key& key, const QByteArray& data, const int generationTimeMs);
        void putLater(const Key& key, const std::function<QByteArray()>& makeData, const int generationTimeMs);
        void setClock(const std::function<int64_t()>& clock);

    friend class OsmAnd::MapTileDiskCache;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_TILE_DISK_CACHE_P_H_)
