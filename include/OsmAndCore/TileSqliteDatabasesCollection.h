#ifndef _OSMAND_CORE_TILE_SQLITE_DATABASES_COLLECTION_H_
#define _OSMAND_CORE_TILE_SQLITE_DATABASES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QFileInfo>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/ITileSqliteDatabasesCollection.h>

namespace OsmAnd
{
    class TileSqliteDatabasesCollection_P;
    class OSMAND_CORE_API TileSqliteDatabasesCollection : public ITileSqliteDatabasesCollection
    {
        Q_DISABLE_COPY_AND_MOVE(TileSqliteDatabasesCollection);
    public:
        typedef int SourceOriginId;

    private:
    protected:
        PrivateImplementation<TileSqliteDatabasesCollection_P> _p;
    public:
        TileSqliteDatabasesCollection(bool useFileWatcher = true, bool buildIndexes = true);
        virtual ~TileSqliteDatabasesCollection();

        QList<SourceOriginId> getSourceOriginIds() const;
        SourceOriginId addDirectory(const QDir& dir, bool recursive = true);
        SourceOriginId addDirectory(const QString& dirPath, bool recursive = true);
        SourceOriginId addFile(const QFileInfo& fileInfo);
        SourceOriginId addFile(const QString& filePath);
        bool removeFile(const QString& filePath);
        bool remove(const SourceOriginId entryId);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual QList< std::shared_ptr<const TileSqliteDatabase> > getTileSqliteDatabases() const;
        virtual QList< std::shared_ptr<const TileSqliteDatabase> > getTileSqliteDatabases(
            TileId tileId, ZoomLevel zoom) const;
        virtual std::shared_ptr<TileSqliteDatabase> getTileSqliteDatabase(const QString& filePath) const;
    };
}

#endif // !defined(_OSMAND_CORE_TILE_SQLITE_DATABASES_COLLECTION_H_)
