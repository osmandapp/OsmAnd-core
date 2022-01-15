#ifndef _OSMAND_CORE_I_TILE_SQLITE_DATABASES_COLLECTION_H_
#define _OSMAND_CORE_I_TILE_SQLITE_DATABASES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>

namespace OsmAnd
{
    class TileSqliteDatabase;

    class OSMAND_CORE_API ITileSqliteDatabasesCollection
    {
        Q_DISABLE_COPY_AND_MOVE(ITileSqliteDatabasesCollection);
    private:
    protected:
        ITileSqliteDatabasesCollection();
    public:
        virtual ~ITileSqliteDatabasesCollection();

        virtual QList< std::shared_ptr<const TileSqliteDatabase> > getTileSqliteDatabases() const = 0;
        virtual QList< std::shared_ptr<const TileSqliteDatabase> > getTileSqliteDatabases(
            TileId tileId, ZoomLevel zoom) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_TILE_SQLITE_DATABASES_COLLECTION_H_)
