#include "OfflineMapDataProvider.h"

OsmAnd::OfflineMapDataProvider::OfflineMapDataProvider( const std::shared_ptr<const ObfsCollection>& obfsCollection_, const std::shared_ptr<const MapStyle>& mapStyle_ )
    : obfsCollection(obfsCollection_)
    , mapStyle(mapStyle_)
{
}

OsmAnd::OfflineMapDataProvider::~OfflineMapDataProvider()
{
}
