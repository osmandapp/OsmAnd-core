#include "MapRasterizer.h"
#include "MapRasterizer_P.h"

OsmAnd::MapRasterizer::MapRasterizer(const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment_)
    : _p(new MapRasterizer_P(this))
    , mapPresentationEnvironment(mapPresentationEnvironment_)
{
    _p->initialize();
}

OsmAnd::MapRasterizer::~MapRasterizer()
{
}

void OsmAnd::MapRasterizer::rasterize(
    const AreaI area31,
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
    SkCanvas& canvas,
    const bool fillBackground /*= true*/,
    const AreaI* const destinationArea /*= nullptr*/,
    MapRasterizer_Metrics::Metric_rasterize* const metric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/ )
{
    _p->rasterize(area31, primitivisedObjects, canvas, fillBackground, destinationArea, metric, queryController);
}
