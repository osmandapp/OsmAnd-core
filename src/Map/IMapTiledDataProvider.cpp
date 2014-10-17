#include "IMapTiledDataProvider.h"

OsmAnd::IMapTiledDataProvider::IMapTiledDataProvider()
{
}

OsmAnd::IMapTiledDataProvider::~IMapTiledDataProvider()
{
}

OsmAnd::IMapTiledDataProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapDataProvider::Data(pRetainableCacheMetadata_)
    , tileId(tileId_)
    , zoom(zoom_)
{
}

OsmAnd::IMapTiledDataProvider::Data::~Data()
{
    release();
}
