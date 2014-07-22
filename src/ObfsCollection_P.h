#ifndef _OSMAND_CORE_OBFS_COLLECTION_P_H_
#define _OSMAND_CORE_OBFS_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QDir>
#include <QHash>
#include <QSet>
#include <QReadWriteLock>
#include <QFileSystemWatcher>
#include <QEventLoop>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "ObfsCollection.h"
#include "Concurrent.h"

namespace OsmAnd
{
    class ObfFile;
    class ObfDataInterface;

    class ObfsCollection;
    class ObfsCollection_P__SignalProxy;
    class ObfsCollection_P Q_DECL_FINAL
    {
    private:
    protected:
        ObfsCollection_P(ObfsCollection* owner);

        ImplementationInterface<ObfsCollection> owner;

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
        
        QHash< ObfsCollection::SourceOriginId, std::shared_ptr<const SourceOrigin> > _sourcesOrigins;
        mutable QReadWriteLock _sourcesOriginsLock;
        int _lastUnusedSourceOriginId;

        void invalidateCollectedSources();
        mutable QAtomicInt _collectedSourcesInvalidated;
        mutable QHash< ObfsCollection::SourceOriginId, QHash<QString, std::shared_ptr<ObfFile> > > _collectedSources;
        mutable QReadWriteLock _collectedSourcesLock;
        void collectSources() const;
    public:
        virtual ~ObfsCollection_P();

        ObfsCollection::SourceOriginId addDirectory(const QDir& dir, bool recursive);
        ObfsCollection::SourceOriginId addFile(const QFileInfo& fileInfo);
        bool remove(const ObfsCollection::SourceOriginId entryId);

        QList< std::shared_ptr<const ObfFile> > getObfFiles() const;
        std::shared_ptr<ObfDataInterface> obtainDataInterface() const;

    friend class OsmAnd::ObfsCollection;
    friend class OsmAnd::ObfsCollection_P__SignalProxy;
    };
}

#endif // !defined(_OSMAND_CORE_OBFS_COLLECTION_P_H_)
