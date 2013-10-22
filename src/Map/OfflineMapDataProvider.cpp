#include "OfflineMapDataProvider.h"
#include "OfflineMapDataProvider_P.h"

OsmAnd::OfflineMapDataProvider::OfflineMapDataProvider( const std::shared_ptr<const ObfsCollection>& obfsCollection_, const std::shared_ptr<const MapStyle>& mapStyle_ )
    : _d(new OfflineMapDataProvider_P(this))
    , obfsCollection(obfsCollection_)
    , mapStyle(mapStyle_)
{
}

OsmAnd::OfflineMapDataProvider::~OfflineMapDataProvider()
{
}

bool OsmAnd::OfflineMapDataProvider::isBasemapAvailable() const
{
    return _d->isBasemapAvailable();
}

void OsmAnd::OfflineMapDataProvider::obtainTile( const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile ) const
{
    _d->obtainTile(tileId, zoom, outTile);
}
