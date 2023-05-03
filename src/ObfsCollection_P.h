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
#include <QMutex>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "ObfsCollection.h"
#include "ObfReader.h"

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

        static bool compareObfReaders(const std::shared_ptr<const ObfReader>& o1, const std::shared_ptr<const ObfReader>& o2)
        {
            const auto& name1 = formatObfReaderFileName(o1);
            const auto& name2 = formatObfReaderFileName(o2);
            return name1.compare(name2) > 0;
        }

        static QString formatObfReaderFileName(const std::shared_ptr<const ObfReader>& obfReader)
        {
            const auto& filePath = obfReader->obfFile->filePath;
            auto fileName = filePath.toLower();

            if (fileName.contains(QDir::separator()))
                fileName = fileName.mid(fileName.lastIndexOf(QDir::separator()) + 1);

            if (fileName.contains("."))
                fileName = fileName.mid(0, fileName.indexOf("."));
            
            if (fileName.endsWith("_2"))
                fileName = fileName.mid(0, fileName.length() - 2);

            bool timestampSpecified = false;
            for (const auto ch : fileName)
            {
                if (ch >= '0' && ch <= '9')
                {
                    timestampSpecified = true;
                    break;
                }
            }

            if (!timestampSpecified)
                fileName += "_00_00_00";

            return fileName;
        }
        
        QHash< ObfsCollection::SourceOriginId, std::shared_ptr<const SourceOrigin> > _sourcesOrigins;
        mutable QReadWriteLock _sourcesOriginsLock;
        int _lastUnusedSourceOriginId;

        mutable QMutex _indexCacheFileMutex;
        mutable QFileInfo _indexCacheFile;

        void invalidateCollectedSources();
        mutable QAtomicInt _collectedSourcesInvalidated;
        mutable QHash< ObfsCollection::SourceOriginId, QHash<QString, std::shared_ptr<ObfFile> > > _collectedSources;
        mutable QReadWriteLock _collectedSourcesLock;
        void collectSources() const;
    public:
        virtual ~ObfsCollection_P();

        ImplementationInterface<ObfsCollection> owner;

        QList<ObfsCollection::SourceOriginId> getSourceOriginIds() const;
        ObfsCollection::SourceOriginId addDirectory(const QDir& dir, bool recursive);
        ObfsCollection::SourceOriginId addFile(const QFileInfo& fileInfo);
        void setIndexCacheFile(const QFileInfo& indexCacheFile);
        bool remove(const ObfsCollection::SourceOriginId entryId);

        QList< std::shared_ptr<const ObfFile> > getObfFiles() const;
        std::shared_ptr<OsmAnd::ObfDataInterface> obtainDataInterface(
            const std::shared_ptr<const ObfFile> obfFile) const;
        std::shared_ptr<OsmAnd::ObfDataInterface> obtainDataInterface(
            const QList< std::shared_ptr<const ResourcesManager::LocalResource> > localResources) const;
        std::shared_ptr<ObfDataInterface> obtainDataInterface(
            const AreaI* const pBbox31,
            const ZoomLevel minZoomLevel,
            const ZoomLevel maxZoomLevel,
            const ObfDataTypesMask desiredDataTypes) const;

    friend class OsmAnd::ObfsCollection;
    friend class OsmAnd::ObfsCollection_P__SignalProxy;
    };
}

#endif // !defined(_OSMAND_CORE_OBFS_COLLECTION_P_H_)
