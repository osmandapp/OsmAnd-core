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
                : region31(AreaD(1.0, 1.0, 0.0, 0.0))
                , pixelSize31(1)
                , rasterBandCount(0)
                , bandDataType(0)
            {
            }

            GeoTiffProperties(const PointD& upperLeft31_, const PointD& lowerRight31_,
                int32_t pixelSize31_, int rasterBandCount_, int bandDataType_)
                : region31(AreaD(upperLeft31_, lowerRight31_))
                , pixelSize31(pixelSize31_)
                , rasterBandCount(rasterBandCount_)
                , bandDataType(bandDataType_)
            {
            }

            AreaD region31;
            int32_t pixelSize31;
            int rasterBandCount;
            int bandDataType;
        };

        QHash<GeoTiffCollection::SourceOriginId, std::shared_ptr<const SourceOrigin>> _sourcesOrigins;
        mutable QReadWriteLock _sourcesOriginsLock;
        int _lastUnusedSourceOriginId;

        double earthInMeters;
        double earthIn31;

        void invalidateCollectedSources();
        mutable QAtomicInt _collectedSourcesInvalidated;
        mutable QAtomicInteger<int32_t> _pixelSize31;
        mutable QHash<GeoTiffCollection::SourceOriginId, QHash<QString, GeoTiffProperties>> _collectedSources;
        mutable QReadWriteLock _collectedSourcesLock;
        void collectSources() const;
        double metersTo31(const double positionInMeters) const;
        PointD metersTo31(const PointD& locationInMeters) const;
        double metersFrom31(const double position31) const;
        PointD metersFrom31(const PointD& location31) const;
        ZoomLevel calcMaxZoom(const int32_t pixelSize31, const uint32_t tileSize) const;
        GeoTiffProperties getGeoTiffProperties(const QString& filename) const;
    public:
        virtual ~GeoTiffCollection_P();

        ImplementationInterface<GeoTiffCollection> owner;

        QList<GeoTiffCollection::SourceOriginId> getSourceOriginIds() const;
        GeoTiffCollection::SourceOriginId addDirectory(const QDir& dir, bool recursive);
        GeoTiffCollection::SourceOriginId addFile(const QFileInfo& fileInfo);
        bool removeFile(const QFileInfo& fileInfo);
        bool remove(const GeoTiffCollection::SourceOriginId entryId);

        ZoomLevel getMaxZoom(const uint32_t tileSize) const;

        QString getGeoTiffFilename(const TileId& tileId, const ZoomLevel zoom,
            const uint32_t overlap, const uint32_t tileSize, const ZoomLevel minZoom = MinZoomLevel) const;
        bool getGeoTiffData(const QString& filename, const TileId& tileId, const ZoomLevel zoom,
            const uint32_t tileSize, const uint32_t overlap, void *pBuffer) const;

    friend class OsmAnd::GeoTiffCollection;
    friend class OsmAnd::GeoTiffCollection_P__SignalProxy;
    };
}

#endif // !defined(_OSMAND_CORE_GEOTIFF_COLLECTION_P_H_)
