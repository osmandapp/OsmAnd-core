#include "Rasterizer.h"
#include "Rasterizer_P.h"

#include "RasterizerEnvironment.h"
#include "RasterizerContext.h"

void OsmAnd::Rasterizer::prepareContext(
    const RasterizerEnvironment& env, RasterizerContext& context,
    const AreaI& area31, const ZoomLevel& zoom,
    uint32_t tileSize, float densityFactor,
    const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& objects,
    const PointF& tlOriginOffset /*= PointF()*/,
    bool* nothingToRasterize /*= nullptr*/, IQueryController* controller /*= nullptr*/ )
{
    Rasterizer_P::prepareContext(*env._d.get(), *context._d.get(), area31, zoom, tileSize, densityFactor, objects, tlOriginOffset, nothingToRasterize, controller);
}

bool OsmAnd::Rasterizer::rasterizeMap(
    const RasterizerEnvironment& env, const RasterizerContext& context,
    bool fillBackground, SkCanvas& canvas, IQueryController* controller /*= nullptr*/ )
{
    return Rasterizer_P::rasterizeMap(*env._d.get(), *context._d.get(), fillBackground, canvas, controller);
}
