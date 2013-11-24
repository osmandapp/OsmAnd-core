#include "OfflineMapDataProvider.h"
#include "OfflineMapDataProvider_P.h"

#include "RasterizerEnvironment.h"
#include "RasterizerSharedContext.h"

OsmAnd::OfflineMapDataProvider::OfflineMapDataProvider( const std::shared_ptr<ObfsCollection>& obfsCollection_, const std::shared_ptr<const MapStyle>& mapStyle_, const float displayDensityFactor )
    : _d(new OfflineMapDataProvider_P(this))
    , obfsCollection(obfsCollection_)
    , mapStyle(mapStyle_)
    , rasterizerEnvironment(new RasterizerEnvironment(mapStyle_, displayDensityFactor))
    , rasterizerSharedContext(new RasterizerSharedContext())
{
}

OsmAnd::OfflineMapDataProvider::~OfflineMapDataProvider()
{
}

void OsmAnd::OfflineMapDataProvider::obtainTile( const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile ) const
{
    _d->obtainTile(tileId, zoom, outTile);
}
