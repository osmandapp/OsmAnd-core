#ifndef _OSMAND_CORE_TILE_SQLITE_DATABASES_INDEX_H_
#define _OSMAND_CORE_TILE_SQLITE_DATABASES_INDEX_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>
#include <QDateTime>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/TileSqliteDatabase.h>

namespace OsmAnd
{
    class TileSqliteDatabasesIndex_P;
    class OSMAND_CORE_API TileSqliteDatabasesIndex
    {
        Q_DISABLE_COPY_AND_MOVE(TileSqliteDatabasesIndex);

    public:
        struct OSMAND_CORE_API Info Q_DECL_FINAL
        {
            explicit Info(
                QDateTime timestamp = QDateTime::currentDateTimeUtc(),
                TileSqliteDatabase::Meta meta = TileSqliteDatabase::Meta()
            );
            ~Info();

            QDateTime timestamp;
            TileSqliteDatabase::Meta meta;
        };

    private:
        PrivateImplementation<TileSqliteDatabasesIndex_P> _p;
    protected:
    public:
        TileSqliteDatabasesIndex();
        virtual ~TileSqliteDatabasesIndex();

        bool add(const std::shared_ptr<TileSqliteDatabase>& database);

        bool remove(const std::shared_ptr<TileSqliteDatabase>& database);
        bool removeAll();

        QList<std::shared_ptr<TileSqliteDatabase>> getAll();
        QList<std::shared_ptr<const TileSqliteDatabase>> getAll() const;

        bool shouldRefresh(const std::shared_ptr<TileSqliteDatabase>& database);
        bool refresh(const std::shared_ptr<TileSqliteDatabase>& database, bool force = false);
        bool refreshAll(bool force = false);

        QList<std::shared_ptr<TileSqliteDatabase>> find(TileId tileId, ZoomLevel zoom);
        QList<std::shared_ptr<const TileSqliteDatabase>> find(TileId tileId, ZoomLevel zoom) const;
    };

}

#endif // !defined(_OSMAND_CORE_TILE_SQLITE_DATABASES_INDEX_H_)
