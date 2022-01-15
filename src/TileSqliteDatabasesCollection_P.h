#ifndef _OSMAND_CORE_TILE_SQLITE_DATABASES_COLLECTION_P_H_
#define _OSMAND_CORE_TILE_SQLITE_DATABASES_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QDir>
#include <QHash>
#include <QSet>
#include <QReadWriteLock>
#include <QFileSystemWatcher>
#include <QEventLoop>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "TileSqliteDatabasesIndex.h"
#include "TileSqliteDatabasesCollection.h"

namespace OsmAnd
{
    class TileSqliteDatabasesCollection;
    class TileSqliteDatabasesCollection_P__SignalProxy;
    class TileSqliteDatabasesCollection_P Q_DECL_FINAL
    {
    private:
    protected:
        TileSqliteDatabasesCollection_P(TileSqliteDatabasesCollection* owner);

        QFileSystemWatcher* const _fileSystemWatcher;
        QMetaObject::Connection _onDirectoryChangedConnection;
        void onDirectoryChanged(const QString& path);
        QMetaObject::Connection _onFileChangedConnection;
        void onFileChanged(const QString& path);

        enum class SourceOriginType
        {
            Directory,
            File,
        };
        struct SourceOrigin
        {
            SourceOrigin(SourceOriginType type_)
                : type(type_)
            {
            }

            const SourceOriginType type;
        };
        struct DirectoryAsSourceOrigin : SourceOrigin
        {
            DirectoryAsSourceOrigin()
                : SourceOrigin(SourceOriginType::Directory)
            {
            }

            QDir directory;
            bool isRecursive;

            mutable QSet<QString> watchedSubdirectories;
        };
        struct FileAsSourceOrigin : SourceOrigin
        {
            FileAsSourceOrigin()
                : SourceOrigin(SourceOriginType::File)
            {
            }

            QFileInfo fileInfo;
        };
        
        QHash< TileSqliteDatabasesCollection::SourceOriginId, std::shared_ptr<const SourceOrigin> > _sourcesOrigins;
        mutable QReadWriteLock _sourcesOriginsLock;
        int _lastUnusedSourceOriginId;

        void invalidateCollectedSources();
        mutable QAtomicInt _collectedSourcesInvalidated;
        mutable QHash< TileSqliteDatabasesCollection::SourceOriginId, QHash<QString, std::shared_ptr<TileSqliteDatabase> > > _collectedSources;
        mutable TileSqliteDatabasesIndex _collectedSourcesIndex;
        mutable QReadWriteLock _collectedSourcesLock;
        void collectSources() const;
    public:
        virtual ~TileSqliteDatabasesCollection_P();

        ImplementationInterface<TileSqliteDatabasesCollection> owner;

        QList<TileSqliteDatabasesCollection::SourceOriginId> getSourceOriginIds() const;
        TileSqliteDatabasesCollection::SourceOriginId addDirectory(const QDir& dir, bool recursive);
        TileSqliteDatabasesCollection::SourceOriginId addFile(const QFileInfo& fileInfo);
        bool remove(const TileSqliteDatabasesCollection::SourceOriginId entryId);

        QList< std::shared_ptr<const TileSqliteDatabase> > getTileSqliteDatabases() const;
        QList< std::shared_ptr<const TileSqliteDatabase> > getTileSqliteDatabases(
            TileId tileId, ZoomLevel zoom) const;

    friend class OsmAnd::TileSqliteDatabasesCollection;
    friend class OsmAnd::TileSqliteDatabasesCollection_P__SignalProxy;
    };
}

#endif // !defined(_OSMAND_CORE_TILE_SQLITE_DATABASES_COLLECTION_P_H_)
