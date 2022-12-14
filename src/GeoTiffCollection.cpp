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

void OsmAnd::GeoTiffCollection::setLocalCachePath(const QString& localCachePath)
{
    _p->setLocalCachePath(localCachePath);
}

OsmAnd::ZoomLevel OsmAnd::GeoTiffCollection::getMaxZoom(const uint32_t tileSize) const
{
    return _p->getMaxZoom(tileSize);
}

QList<QString> OsmAnd::GeoTiffCollection::getGeoTiffFilePaths(const TileId& tileId, const ZoomLevel zoom,
    const uint32_t tileSize, const uint32_t overlap, const uint32_t bandCount,
    const ZoomLevel minZoom /*= MinZoomLevel*/) const
{
    return _p->getGeoTiffFilePaths(tileId, zoom, tileSize, overlap, bandCount, minZoom);
}

bool OsmAnd::GeoTiffCollection::getGeoTiffData(const QList<QString>& filePaths,
    const TileId& tileId, const ZoomLevel zoom, const uint32_t tileSize, const uint32_t overlap,
    const uint32_t bandCount, const bool toBytes, void *pBuffer, const ZoomLevel minZoom /*= MinZoomLevel*/) const
{
    return _p->getGeoTiffData(filePaths, tileId, zoom, tileSize, overlap, bandCount, toBytes, pBuffer, minZoom);
}
