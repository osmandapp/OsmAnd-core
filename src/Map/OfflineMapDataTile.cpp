#include "OfflineMapDataTile.h"
#include "OfflineMapDataTile_P.h"

OsmAnd::OfflineMapDataTile::OfflineMapDataTile(
    const TileId tileId_, const ZoomLevel zoom_,
    const MapFoundationType tileFoundation_, const QList< std::shared_ptr<const Model::MapObject> >& mapObjects_,
    const std::shared_ptr< const RasterizerContext >& rasterizerContext_, const bool nothingToRasterize_)
    : _d(new OfflineMapDataTile_P(this))
    , tileId(tileId_)
    , zoom(zoom_)
    , tileFoundation(tileFoundation_)
    , mapObjects(mapObjects_)
    , rasterizerContext(rasterizerContext_)
    , nothingToRasterize(nothingToRasterize_)
{
}

OsmAnd::OfflineMapDataTile::~OfflineMapDataTile()
{
    _d->cleanup();
}
