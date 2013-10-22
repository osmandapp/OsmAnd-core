#include "OfflineMapDataTile.h"
#include "OfflineMapDataTile_P.h"

OsmAnd::OfflineMapDataTile::OfflineMapDataTile(const MapFoundationType tileFoundation_, const QList< std::shared_ptr<const Model::MapObject> >& mapObjects_)
    : _d(new OfflineMapDataTile_P(this))
    , tileFoundation(tileFoundation_)
    , mapObjects(mapObjects_)
{
}

OsmAnd::OfflineMapDataTile::~OfflineMapDataTile()
{
    _d->cleanup();
}
