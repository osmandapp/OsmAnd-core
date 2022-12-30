#ifndef _OSMAND_CORE_GEOTIFF_COLLECTION_P_H_
#define _OSMAND_CORE_GEOTIFF_COLLECTION_P_H_

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
#include "GeoTiffCollection.h"
#include "TileSqliteDatabase.h"
#include <OsmAndCore/PointsAndAreas.h>

namespace OsmAnd
{
    class GeoTiffCollection;
    class GeoTiffCollection_P__SignalProxy;
    class GeoTiffCollection_P Q_DECL_FINAL
    {
    private:
    protected:
        GeoTiffCollection_P(GeoTiffCollection* owner, bool useFileWatcher = true);

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

        struct GeoTiffProperties
        {
            GeoTiffProperties()
                : region31(AreaI())
                , pixelSize31(1)
                , rasterBandCount(0)
                , bandDataType(0)
                , cacheDatabase(nullptr)
            {
            }

            GeoTiffProperties(const PointI& upperLeft31_, const PointI& lowerRight31_,
                int32_t pixelSize31_, int rasterBandCount_, int bandDataType_,
                std::shared_ptr<TileSqliteDatabase> cacheDatabase_)
                : region31(AreaI(upperLeft31_, lowerRight31_))
                , pixelSize31(pixelSize31_)
                , rasterBandCount(rasterBandCount_)
                , bandDataType(bandDataType_)
                , cacheDatabase(cacheDatabase_)
            {
            }

            AreaI region31;
            int32_t pixelSize31;
            int rasterBandCount;
            int bandDataType;
            std::shared_ptr<TileSqliteDatabase> cacheDatabase;
        };

        QHash<GeoTiffCollection::SourceOriginId, std::shared_ptr<const SourceOrigin>> _sourcesOrigins;
        mutable QReadWriteLock _sourcesOriginsLock;
        int _lastUnusedSourceOriginId;

        double earthInMeters;
        double earthIn31;

        mutable QMutex _localCacheDirMutex;
        mutable QDir _localCacheDir;

        void invalidateCollectedSources();
        void clearCollectedSources() const;
        mutable QAtomicInt _collectedSourcesInvalidated;
        mutable QAtomicInteger<int32_t> _minZoom;
        mutable QAtomicInteger<int32_t> _pixelSize31;
        mutable QHash<GeoTiffCollection::SourceOriginId, QHash<QString, GeoTiffProperties>> _collectedSources;
        mutable QReadWriteLock _collectedSourcesLock;
        void collectSources() const;
        int32_t metersTo31(const double positionInMeters) const;
        PointI metersTo31(const PointD& locationInMeters) const;
        double metersFrom31(const double position31) const;
        PointD metersFrom31(const double positionX, const double positionY) const;
        ZoomLevel calcMaxZoom(const int32_t pixelSize31, const uint32_t tileSize) const;
        GeoTiffProperties getGeoTiffProperties(const QString& filePath) const;
    public:
        virtual ~GeoTiffCollection_P();

        ImplementationInterface<GeoTiffCollection> owner;

        QList<GeoTiffCollection::SourceOriginId> getSourceOriginIds() const;
        GeoTiffCollection::SourceOriginId addDirectory(const QDir& dir, bool recursive);
        GeoTiffCollection::SourceOriginId addFile(const QFileInfo& fileInfo);
        bool removeFile(const QFileInfo& fileInfo);
        bool remove(const GeoTiffCollection::SourceOriginId entryId);
        void setLocalCache(const QDir& localCacheDir);
        
        void setMinZoom(const ZoomLevel zoomLevel) const;
        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom(const uint32_t tileSize) const;

        bool getGeoTiffData(const TileId& tileId, const ZoomLevel zoom, const uint32_t tileSize,
            const uint32_t overlap, const uint32_t bandCount, const bool toBytes, void *pBuffer) const;

    friend class OsmAnd::GeoTiffCollection;
    friend class OsmAnd::GeoTiffCollection_P__SignalProxy;
    };
}

#endif // !defined(_OSMAND_CORE_GEOTIFF_COLLECTION_P_H_)
