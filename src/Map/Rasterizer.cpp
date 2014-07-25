#include "Rasterizer.h"
#include "Rasterizer_P.h"

OsmAnd::Rasterizer::Rasterizer()
    : _p(new Rasterizer_P(this))
{
}

OsmAnd::Rasterizer::~Rasterizer()
{
}

void OsmAnd::Rasterizer::rasterizeMap(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    SkCanvas& canvas,
    const bool fillBackground /*= true*/,
    const AreaI* const destinationArea /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/ )
{
    _p->rasterizeMap(primitivizedArea, canvas, fillBackground, destinationArea, controller);
}

void OsmAnd::Rasterizer::rasterizeSymbolsWithoutPaths(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    std::function<bool (const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/ )
{
    _p->rasterizeSymbolsWithoutPaths(primitivizedArea, outSymbolsGroups, filter, controller);
}
