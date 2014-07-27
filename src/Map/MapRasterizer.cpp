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
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const bool fillBackground /*= true*/,
    const AreaI* const destinationArea /*= nullptr*/,
    MapRasterizer_Metrics::Metric_rasterize* const metric /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/ )
{
    _p->rasterize(primitivizedArea, canvas, fillBackground, destinationArea, metric, controller);
}
