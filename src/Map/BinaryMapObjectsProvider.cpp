#include "BinaryMapObjectsProvider.h"
#include "BinaryMapObjectsProvider_P.h"

#include "BinaryMapObjectsProvider_Metrics.h"

OsmAnd::BinaryMapObjectsProvider::BinaryMapObjectsProvider(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : _p(new BinaryMapObjectsProvider_P(this))
    , obfsCollection(obfsCollection_)
{
}

OsmAnd::BinaryMapObjectsProvider::~BinaryMapObjectsProvider()
{
}

bool OsmAnd::BinaryMapObjectsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapObjectsProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    if (pOutMetric)
    {
        if (!pOutMetric->get() || !dynamic_cast<BinaryMapObjectsProvider_Metrics::Metric_obtainData*>(pOutMetric->get()))
            pOutMetric->reset(new BinaryMapObjectsProvider_Metrics::Metric_obtainData());
        else
            pOutMetric->get()->reset();
    }

    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(
        tileId,
        zoom,
        tiledData,
        pOutMetric ? static_cast<BinaryMapObjectsProvider_Metrics::Metric_obtainData*>(pOutMetric->get()) : nullptr,
        queryController);
    outTiledData = tiledData;

    return result;
}

bool OsmAnd::BinaryMapObjectsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<Data>& outTiledData,
    BinaryMapObjectsProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapObjectsProvider::getMinZoom() const
{
    return MinZoomLevel;//TODO: invalid
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapObjectsProvider::getMaxZoom() const
{
    return MaxZoomLevel;//TODO: invalid
}
