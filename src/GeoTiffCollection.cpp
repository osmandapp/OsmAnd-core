#include "GeoTiffCollection.h"
#include "GeoTiffCollection_P.h"

OsmAnd::GeoTiffCollection::GeoTiffCollection(bool useFileWatcher /*= true*/)
    : _p(new GeoTiffCollection_P(this, useFileWatcher))
{
}

OsmAnd::GeoTiffCollection::~GeoTiffCollection()
{
}

QList<OsmAnd::GeoTiffCollection::SourceOriginId> OsmAnd::GeoTiffCollection::getSourceOriginIds() const
{
    return _p->getSourceOriginIds();
}

OsmAnd::GeoTiffCollection::SourceOriginId OsmAnd::GeoTiffCollection::addDirectory(
    const QString& dirPath,
    bool recursive /*= true*/)
{
    return addDirectory(QDir(dirPath), recursive);
}

OsmAnd::GeoTiffCollection::SourceOriginId OsmAnd::GeoTiffCollection::addDirectory(
    const QDir& dir,
    bool recursive /*= true*/)
{
    return _p->addDirectory(dir, recursive);
}

OsmAnd::GeoTiffCollection::SourceOriginId OsmAnd::GeoTiffCollection::addFile(
    const QString& filePath)
{
    return addFile(QFileInfo(filePath));
}

OsmAnd::GeoTiffCollection::SourceOriginId OsmAnd::GeoTiffCollection::addFile(
    const QFileInfo& fileInfo)
{
    return _p->addFile(fileInfo);
}

bool OsmAnd::GeoTiffCollection::removeFile(const QString& filePath)
{
    return _p->removeFile(QFileInfo(filePath));
}

bool OsmAnd::GeoTiffCollection::remove(const SourceOriginId entryId)
{
    return _p->remove(entryId);
}

OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection::getMaxZoom(const uint32_t tileSize) const
{
    return _p->getMaxZoom(tileSize);
}

QString OsmAnd::GeoTiffCollection::getGeoTiffFilename(const TileId& tileId, const ZoomLevel zoom,
    const uint32_t tileSize, const uint32_t overlap, const ZoomLevel minZoom /*= MinZoomLevel*/) const
{
    return _p->getGeoTiffFilename(tileId, zoom, tileSize, overlap, minZoom);
}

bool OsmAnd::GeoTiffCollection::getGeoTiffData(const QString& filename, const TileId& tileId, const ZoomLevel zoom,
    const uint32_t tileSize, const uint32_t overlap, void *pBuffer) const
{
    return _p->getGeoTiffData(filename, tileId, zoom, tileSize, overlap, pBuffer);
}
