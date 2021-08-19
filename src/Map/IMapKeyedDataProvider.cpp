#include "IMapKeyedDataProvider.h"

#include "MapDataProviderHelpers.h"

OsmAnd::IMapKeyedDataProvider::IMapKeyedDataProvider()
{
}

OsmAnd::IMapKeyedDataProvider::~IMapKeyedDataProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::IMapKeyedDataProvider::getMinZoom() const
{
    return OsmAnd::MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::IMapKeyedDataProvider::getMaxZoom() const
{
    return OsmAnd::MaxZoomLevel;
}

bool OsmAnd::IMapKeyedDataProvider::obtainKeyedData(
    const Request& request,
    std::shared_ptr<Data>& outKeyedData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outKeyedData, pOutMetric);
}

OsmAnd::IMapKeyedDataProvider::Data::Data(
    const Key key_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapDataProvider::Data(pRetainableCacheMetadata_)
    , key(key_)
{
}

OsmAnd::IMapKeyedDataProvider::Data::~Data()
{
    release();
}

OsmAnd::IMapKeyedDataProvider::Request::Request()
    : key(nullptr)
{
}

OsmAnd::IMapKeyedDataProvider::Request::Request(const IMapDataProvider::Request& that)
    : IMapDataProvider::Request(that)
{
    copy(*this, that);
}

OsmAnd::IMapKeyedDataProvider::Request::Request(const Request& that)
    : IMapDataProvider::Request(that)
{
    copy(*this, that);
}

OsmAnd::IMapKeyedDataProvider::Request::~Request()
{
}

void OsmAnd::IMapKeyedDataProvider::Request::copy(Request& dst, const IMapDataProvider::Request& src_)
{
    const auto& src = MapDataProviderHelpers::castRequest<Request>(src_);

    dst.key = src.key;    
    dst.mapState = src.mapState;
}

std::shared_ptr<OsmAnd::IMapDataProvider::Request> OsmAnd::IMapKeyedDataProvider::Request::clone() const
{
    return std::shared_ptr<IMapDataProvider::Request>(new Request(*this));
}
