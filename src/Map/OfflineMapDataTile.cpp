#include "OfflineMapDataTile.h"
#include "OfflineMapDataTile_P.h"

OsmAnd::OfflineMapDataTile::OfflineMapDataTile(
    const TileId tileId_, const ZoomLevel zoom_,
    const MapFoundationType tileFoundation_, const QList< std::shared_ptr<const Model::MapObject> >& mapObjects_,
    const std::shared_ptr< const RasterizerContext >& rasterizerContext_, const bool nothingToRasterize_)
    : _p(new OfflineMapDataTile_P(this))
    , tileId(tileId_)
    , zoom(zoom_)
    , tileFoundation(tileFoundation_)
    , mapObjects(_p->_mapObjects)
    , rasterizerContext(_p->_rasterizerContext)
    , nothingToRasterize(nothingToRasterize_)
{
    _p->_mapObjects = mapObjects_;
    _p->_rasterizerContext = rasterizerContext_;
}

OsmAnd::OfflineMapDataTile::~OfflineMapDataTile()
{
    _p->cleanup();
}
