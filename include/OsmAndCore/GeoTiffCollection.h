#ifndef _OSMAND_CORE_GEOTIFF_COLLECTION_H_
#define _OSMAND_CORE_GEOTIFF_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QFileInfo>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IGeoTiffCollection.h>

namespace OsmAnd
{
    class GeoTiffCollection_P;
    class OSMAND_CORE_API GeoTiffCollection : public IGeoTiffCollection
    {
        Q_DISABLE_COPY_AND_MOVE(GeoTiffCollection);
    public:
        typedef int SourceOriginId;

    private:
    protected:
        PrivateImplementation<GeoTiffCollection_P> _p;
    public:
        GeoTiffCollection(bool useFileWatcher = true);
        virtual ~GeoTiffCollection();

        QList<SourceOriginId> getSourceOriginIds() const;
        SourceOriginId addDirectory(const QDir& dir, bool recursive = true);
        SourceOriginId addDirectory(const QString& dirPath, bool recursive = true);
        SourceOriginId addFile(const QFileInfo& fileInfo);
        SourceOriginId addFile(const QString& filePath);
        bool removeFile(const QString& filePath);
        bool remove(const SourceOriginId entryId);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual QString getGeoTiffFilename(
            const TileId& tileId, const ZoomLevel zoom, const uint32_t tileSize) const;
        virtual bool getGeoTiffData(const QString& filename, const TileId& tileId, const ZoomLevel zoom,
            const uint32_t tileSize, void *pBuffer) const;
    };
}

#endif // !defined(_OSMAND_CORE_GEOTIFF_COLLECTION_H_)
