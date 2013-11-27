#include "Rasterizer.h"
#include "Rasterizer_P.h"

#include "RasterizerEnvironment.h"
#include "RasterizerContext.h"

OsmAnd::Rasterizer::Rasterizer(const std::shared_ptr<const RasterizerContext>& context_)
    : _d(new Rasterizer_P(this, *context_->environment->_d, *context_->_d))
    , context(context_)
{
}

OsmAnd::Rasterizer::~Rasterizer()
{
}

void OsmAnd::Rasterizer::prepareContext(
    RasterizerContext& context,
    const AreaI& area31,
    const ZoomLevel zoom,
    const MapFoundationType foundation,
    const QList< std::shared_ptr<const Model::MapObject> >& objects,
    bool* nothingToRasterize /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/,
    Rasterizer_Metrics::Metric_prepareContext* const metric /*= nullptr*/)
{
    Rasterizer_P::prepareContext(*context.environment->_d, *context._d, area31, zoom, foundation, objects, nothingToRasterize, controller, metric);
}

void OsmAnd::Rasterizer::rasterizeMap(
    SkCanvas& canvas,
    const bool fillBackground /*= true*/,
    const AreaI* const destinationArea /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/ )
{
    _d->rasterizeMap(canvas, fillBackground, destinationArea, controller);
}

void OsmAnd::Rasterizer::rasterizeSymbolsWithoutPaths(
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    std::function<bool (const std::shared_ptr<const Model::MapObject>& mapObject)> filter /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/ )
{
    _d->rasterizeSymbolsWithoutPaths(outSymbolsGroups, filter, controller);
}
