#include "HeightmapTileProvider.h"
#include "HeightmapTileProvider_P.h"

const QString OsmAnd::HeightmapTileProvider::defaultIndexFilename(QLatin1String("heightmap.index"));

OsmAnd::HeightmapTileProvider::HeightmapTileProvider(const QString& dataPath_, const QString& indexFilename_ /*= QString::null*/)
    : _p(new HeightmapTileProvider_P(this, dataPath_, indexFilename_))
    , dataPath(dataPath_)
    , indexFilename(indexFilename_)
{
}

OsmAnd::HeightmapTileProvider::~HeightmapTileProvider()
{
}

void OsmAnd::HeightmapTileProvider::rebuildTileDbIndex()
{
    _p->rebuildTileDbIndex();
}

uint32_t OsmAnd::HeightmapTileProvider::getTileSize() const
{
    return _p->getTileSize();
}

OsmAnd::ZoomLevel OsmAnd::HeightmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::HeightmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

bool OsmAnd::HeightmapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<const MapTiledData>& outTiledData,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}
