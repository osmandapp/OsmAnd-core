#include "IMapKeyedDataProvider.h"

OsmAnd::IMapKeyedDataProvider::IMapKeyedDataProvider()
{
}

OsmAnd::IMapKeyedDataProvider::~IMapKeyedDataProvider()
{
}

OsmAnd::IMapKeyedDataProvider::Data::Data(const Key key_, const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapDataProvider::Data(pRetainableCacheMetadata_)
    , key(key_)
{
}

OsmAnd::IMapKeyedDataProvider::Data::~Data()
{
    release();
}
