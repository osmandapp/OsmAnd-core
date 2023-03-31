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
#include <gdal_priv.h>
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
                , rasterSize(PointI())
                , pixelSize31(1)
                , rasterBandCount(0)
                , bandDataType(0)
            {
            }

            GeoTiffProperties(const PointI& upperLeft31_, const PointI& lowerRight31_, const PointI& rasterSize_,
                int32_t pixelSize31_, int rasterBandCount_, int bandDataType_)
                : region31(AreaI(upperLeft31_, lowerRight31_))
                , rasterSize(rasterSize_)
                , pixelSize31(pixelSize31_)
                , rasterBandCount(rasterBandCount_)
                , bandDataType(bandDataType_)
            {
            }

            AreaI region31;
            PointI rasterSize;
            int32_t pixelSize31;
            int rasterBandCount;
            int bandDataType;
        };

        QHash<GeoTiffCollection::SourceOriginId, std::shared_ptr<const SourceOrigin>> _sourcesOrigins;
        mutable QReadWriteLock _sourcesOriginsLock;
        int _lastUnusedSourceOriginId;

        double earthInMeters;
        double earthIn31;

        mutable QMutex _localCacheDirMutex;
        mutable QDir _localCacheDir;

        mutable QAtomicInt _collectedSourcesInvalidated;
        mutable QAtomicInteger<int32_t> _minZoom;
        mutable QAtomicInteger<int32_t> _pixelSize31;
        mutable QHash<GeoTiffCollection::SourceOriginId, QHash<QString, GeoTiffProperties>> _collectedSources;
        mutable QReadWriteLock _collectedSourcesLock;

        std::shared_ptr<TileSqliteDatabase> heightmapCache;
        std::shared_ptr<TileSqliteDatabase> hillshadeCache;
        std::shared_ptr<TileSqliteDatabase> slopeCache;

        void invalidateCollectedSources();
        void clearCollectedSources() const;
        void collectSources() const;
        int32_t metersTo31(const double positionInMeters) const;
        PointI metersTo31(const PointD& locationInMeters) const;
        double metersFrom31(const double position31) const;
        PointD metersFrom31(const double positionX, const double positionY) const;
        bool containsTile(const AreaI& region31, const AreaI& area31) const;
        bool isPartOfTile(const AreaI& region31, const AreaI& area31) const;
        ZoomLevel calcMaxZoom(const int32_t pixelSize31, const uint32_t tileSize) const;
        GeoTiffProperties getGeoTiffProperties(const QString& filePath) const;
        std::shared_ptr<TileSqliteDatabase> openCacheFile(const QString filename);
        bool isDataPresent(const char* pByteOffset, const GDALDataType dataType, const double noData) const;
        uint64_t multiplyParts(const uint64_t shade, const uint64_t slope) const;
        void blendHillshade(const uint32_t tileSize, uchar* shade, uchar* slope, uchar* blend) const;
        void mergeHeights(const uint32_t tileSize, const float scaleFactor, const bool forceReplace,
            QByteArray& destination, float* source) const;
        bool postProcess(
            const char* pByteBuffer,
            const GeoTiffCollection::ProcessingParameters& procParameters,
            const uint32_t tileSize,
            const uint32_t overlap,
            const PointD& tileOrigin,
            const PointD& tileResolution,
            const int gcpCount,
            const void* gcpList,
            void* pBuffer) const;
    public:
        virtual ~GeoTiffCollection_P();

        ImplementationInterface<GeoTiffCollection> owner;

        QList<GeoTiffCollection::SourceOriginId> getSourceOriginIds() const;
        GeoTiffCollection::SourceOriginId addDirectory(const QDir& dir, bool recursive);
        GeoTiffCollection::SourceOriginId addFile(const QFileInfo& fileInfo);
        bool removeFile(const QFileInfo& fileInfo);
        bool remove(const GeoTiffCollection::SourceOriginId entryId);
        void setLocalCache(const QDir& localCacheDir);
        bool refreshTilesInCache(const GeoTiffCollection::RasterType cache);
        bool removeFileTilesFromCache(const GeoTiffCollection::RasterType cache, const QString& filePath);
        bool removeOlderTilesFromCache(const GeoTiffCollection::RasterType cache, int64_t time);
        
        void setMinZoom(const ZoomLevel zoomLevel) const;
        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom(const uint32_t tileSize) const;

        GeoTiffCollection::CallResult getGeoTiffData(
            const TileId& tileId,
            const ZoomLevel zoom,
            const uint32_t tileSize,
            const uint32_t overlap,
            const uint32_t bandCount,
            const bool toBytes,
            void* pBuffer,
            const GeoTiffCollection::ProcessingParameters* procParameters) const;

    friend class OsmAnd::GeoTiffCollection;
    friend class OsmAnd::GeoTiffCollection_P__SignalProxy;
    };
}

#endif // !defined(_OSMAND_CORE_GEOTIFF_COLLECTION_P_H_)
