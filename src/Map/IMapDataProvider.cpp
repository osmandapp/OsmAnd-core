#include "IMapDataProvider.h"

OsmAnd::IMapDataProvider::IMapDataProvider()
{
}

OsmAnd::IMapDataProvider::~IMapDataProvider()
{
}

OsmAnd::IMapDataProvider::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}

OsmAnd::IMapDataProvider::Data::Data(const RetainableCacheMetadata* const pRetainableCacheMetadata /*= nullptr*/)
{
}

OsmAnd::IMapDataProvider::Data::~Data()
{
    release();
}

void OsmAnd::IMapDataProvider::Data::release()
{
    if (!retainableCacheMetadata)
        return;

    retainableCacheMetadata.reset();
}
