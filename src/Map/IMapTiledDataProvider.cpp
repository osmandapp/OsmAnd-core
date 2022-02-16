#include "IMapTiledDataProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapRenderer.h"

OsmAnd::IMapTiledDataProvider::IMapTiledDataProvider()
{
}

OsmAnd::IMapTiledDataProvider::~IMapTiledDataProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::IMapTiledDataProvider::getMinVisibleZoom() const
{
    return getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::IMapTiledDataProvider::getMaxVisibleZoom() const
{
    return getMaxZoom();
}

int OsmAnd::IMapTiledDataProvider::getMaxMissingDataZoomShift() const
{
    return MapRenderer::MaxMissingDataZoomShift;
}

int OsmAnd::IMapTiledDataProvider::getMaxMissingDataUnderZoomShift() const
{
    return MapRenderer::MaxMissingDataUnderZoomShift;
}

bool OsmAnd::IMapTiledDataProvider::obtainTiledData(
    const Request& request,
    std::shared_ptr<Data>& outTiledData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outTiledData, pOutMetric);
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

OsmAnd::IMapTiledDataProvider::Request::Request()
    : tileId(TileId::zero())
    , zoom(InvalidZoomLevel)
{
}

OsmAnd::IMapTiledDataProvider::Request::Request(const IMapDataProvider::Request& that)
    : IMapDataProvider::Request(that)
{
    copy(*this, that);
}

OsmAnd::IMapTiledDataProvider::Request::Request(const Request& that)
    : IMapDataProvider::Request(that)
{
    copy(*this, that);
}

OsmAnd::IMapTiledDataProvider::Request::~Request()
{
}

void OsmAnd::IMapTiledDataProvider::Request::copy(Request& dst, const IMapDataProvider::Request& src_)
{
    const auto& src = MapDataProviderHelpers::castRequest<Request>(src_);

    dst.tileId = src.tileId;
    dst.zoom = src.zoom;
}

std::shared_ptr<OsmAnd::IMapDataProvider::Request> OsmAnd::IMapTiledDataProvider::Request::clone() const
{
    return std::shared_ptr<IMapDataProvider::Request>(new Request(*this));
}
