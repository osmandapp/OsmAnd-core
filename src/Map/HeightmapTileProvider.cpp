#include "HeightmapTileProvider.h"
#include "HeightmapTileProvider_P.h"

const QString OsmAnd::HeightmapTileProvider::defaultIndexFilename("heightmap.index");

OsmAnd::HeightmapTileProvider::HeightmapTileProvider( const QDir& dataPath, const QString& indexFilepath/* = QString()*/ )
    : _d(new HeightmapTileProvider_P(this, dataPath, indexFilepath))
{
}

OsmAnd::HeightmapTileProvider::~HeightmapTileProvider()
{
}

void OsmAnd::HeightmapTileProvider::rebuildTileDbIndex()
{
    _d->_tileDb.rebuildIndex();
}

uint32_t OsmAnd::HeightmapTileProvider::getTileSize() const
{
    return 32;
}

bool OsmAnd::HeightmapTileProvider::obtainTile( const TileId tileId, const ZoomLevel zoom, std::shared_ptr<MapTile>& outTile )
{
    return _d->obtainTile(tileId, zoom, outTile);
}
