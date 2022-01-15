#ifndef _OSMAND_CORE_TILE_SQLITE_DATABASES_INDEX_P_H_
#define _OSMAND_CORE_TILE_SQLITE_DATABASES_INDEX_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QAtomicInt>
#include <QReadWriteLock>
#include <QMutex>
#include <QDateTime>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PointsAndAreas.h"
#include "QuadTree.h"
#include "TileSqliteDatabase.h"
#include "TileSqliteDatabasesIndex.h"

namespace OsmAnd
{
    class TileSqliteDatabasesIndex;

    class TileSqliteDatabasesIndex_P Q_DECL_FINAL
    {
    public:
        typedef TileSqliteDatabasesIndex::Info Info;

    private:
        mutable QReadWriteLock _lock;
        QHash<std::shared_ptr<TileSqliteDatabase>, Info> _infos;
        QuadTree<std::shared_ptr<TileSqliteDatabase>, int32_t> _tree;

        bool shouldRefresh(const std::shared_ptr<TileSqliteDatabase>& database, const Info& info) const;
    protected:
        TileSqliteDatabasesIndex_P(TileSqliteDatabasesIndex* owner);
    public:
        ~TileSqliteDatabasesIndex_P();

        ImplementationInterface<TileSqliteDatabasesIndex> owner;

        bool add(const std::shared_ptr<TileSqliteDatabase>& database);

        bool remove(const std::shared_ptr<TileSqliteDatabase>& database);
        bool removeAll();
        
        QList<std::shared_ptr<TileSqliteDatabase>> getAll();
        QList<std::shared_ptr<const TileSqliteDatabase>> getAll() const;

        bool shouldRefresh(const std::shared_ptr<TileSqliteDatabase>& database) const;
        bool refresh(const std::shared_ptr<TileSqliteDatabase>& database, bool force = false);
        bool refreshAll(bool force = false);

        QList<std::shared_ptr<TileSqliteDatabase>> find(TileId tileId, ZoomLevel zoom);
        QList<std::shared_ptr<const TileSqliteDatabase>> find(TileId tileId, ZoomLevel zoom) const;

        friend class OsmAnd::TileSqliteDatabasesIndex;
    };
}

#endif // !defined(_OSMAND_CORE_TILE_SQLITE_DATABASES_INDEX_P_H_)
