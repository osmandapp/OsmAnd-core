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
    if (pRetainableCacheMetadata != nullptr)
        retainableCacheMetadata = std::shared_ptr<const RetainableCacheMetadata>(pRetainableCacheMetadata);
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


OsmAnd::IMapDataProvider::Request::Request()
{
}

OsmAnd::IMapDataProvider::Request::Request(const Request& that)
{
    copy(*this, that);
}

OsmAnd::IMapDataProvider::Request::~Request()
{
}

std::shared_ptr<OsmAnd::IMapDataProvider::Request> OsmAnd::IMapDataProvider::Request::clone() const
{
    return std::shared_ptr<IMapDataProvider::Request>(new Request(*this));
}

void OsmAnd::IMapDataProvider::Request::copy(Request& dst, const Request& src)
{
    dst.queryController = src.queryController;
}

void OsmAnd::IMapDataProvider::applyMapChanges(IMapRenderer* renderer)
{
}
