#include "HeightmapTileProvider.h"
#include "HeightmapTileProvider_P.h"

const QString OsmAnd::HeightmapTileProvider::defaultIndexFilename("heightmap.index");

OsmAnd::HeightmapTileProvider::HeightmapTileProvider( const QDir& dataPath, const QString& indexFilepath/* = QString()*/ )
    : _p(new HeightmapTileProvider_P(this, dataPath, indexFilepath))
{
}

OsmAnd::HeightmapTileProvider::~HeightmapTileProvider()
{
}

void OsmAnd::HeightmapTileProvider::rebuildTileDbIndex()
{
    _p->_tileDb.rebuildIndex();
}

uint32_t OsmAnd::HeightmapTileProvider::getTileSize() const
{
    return 32;
}

bool OsmAnd::HeightmapTileProvider::obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController)
{
    return _p->obtainTile(tileId, zoom, outTile, queryController);
}

OsmAnd::ZoomLevel OsmAnd::HeightmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::HeightmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}
