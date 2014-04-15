#include "OfflineMapDataProvider.h"
#include "OfflineMapDataProvider_P.h"

#include "RasterizerEnvironment.h"
#include "RasterizerSharedContext.h"

OsmAnd::OfflineMapDataProvider::OfflineMapDataProvider(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const std::shared_ptr<const MapStyle>& mapStyle_,
    const float displayDensityFactor,
    const std::shared_ptr<const IExternalResourcesProvider>& externalResourcesProvider /*= nullptr*/ )
    : _p(new OfflineMapDataProvider_P(this))
    , obfsCollection(obfsCollection_)
    , mapStyle(mapStyle_)
    , rasterizerEnvironment(new RasterizerEnvironment(mapStyle_, displayDensityFactor, externalResourcesProvider))
    , rasterizerSharedContext(new RasterizerSharedContext())
{
}

OsmAnd::OfflineMapDataProvider::~OfflineMapDataProvider()
{
}

void OsmAnd::OfflineMapDataProvider::obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile) const
{
    _p->obtainTile(tileId, zoom, outTile);
}
