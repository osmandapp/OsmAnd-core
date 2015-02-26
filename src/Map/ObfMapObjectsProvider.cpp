#include "ObfMapObjectsProvider.h"
#include "ObfMapObjectsProvider_P.h"

#include "ObfMapObjectsProvider_Metrics.h"

OsmAnd::ObfMapObjectsProvider::ObfMapObjectsProvider(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const Mode mode_ /*= Mode::BinaryMapObjectsAndRoads*/)
    : _p(new ObfMapObjectsProvider_P(this))
    , obfsCollection(obfsCollection_)
    , mode(mode_)
{
}

OsmAnd::ObfMapObjectsProvider::~ObfMapObjectsProvider()
{
}

bool OsmAnd::ObfMapObjectsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapObjectsProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    if (pOutMetric)
    {
        if (!pOutMetric->get() || !dynamic_cast<ObfMapObjectsProvider_Metrics::Metric_obtainData*>(pOutMetric->get()))
            pOutMetric->reset(new ObfMapObjectsProvider_Metrics::Metric_obtainData());
        else
            pOutMetric->get()->reset();
    }

    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(
        tileId,
        zoom,
        tiledData,
        pOutMetric ? static_cast<ObfMapObjectsProvider_Metrics::Metric_obtainData*>(pOutMetric->get()) : nullptr,
        queryController);
    outTiledData = tiledData;

    return result;
}

OsmAnd::IMapDataProvider::SourceType OsmAnd::ObfMapObjectsProvider::getSourceType() const
{
    return IMapDataProvider::SourceType::LocalDirect;
}

bool OsmAnd::ObfMapObjectsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<Data>& outTiledData,
    ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
}

OsmAnd::ZoomLevel OsmAnd::ObfMapObjectsProvider::getMinZoom() const
{
    return MinZoomLevel;//TODO: invalid
}

OsmAnd::ZoomLevel OsmAnd::ObfMapObjectsProvider::getMaxZoom() const
{
    return MaxZoomLevel;//TODO: invalid
}
