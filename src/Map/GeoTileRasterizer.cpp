#include "GeoTileRasterizer.h"
#include "GeoTileRasterizer_P.h"

OsmAnd::GeoTileRasterizer::GeoTileRasterizer(
    const QByteArray& geoTileData_,
    const TileId tileId_,
    const ZoomLevel zoom_,
    const QList<BandIndex>& bands_,
    const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const QString& projSearchPath_ /*= QString()*/)
    : _p(new GeoTileRasterizer_P(this))
    , geoTileData(geoTileData_)
    , tileId(tileId_)
    , zoom(zoom_)
    , bands(bands_)
    , bandSettings(bandSettings_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
    , projSearchPath(projSearchPath_)
{
}

OsmAnd::GeoTileRasterizer::~GeoTileRasterizer()
{
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer::rasterize(
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    return _p->rasterize(pOutMetric, queryController);
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer::rasterize(
    QHash<BandIndex, QByteArray>& outEncImgData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    return _p->rasterize(outEncImgData, pOutMetric, queryController);
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer::rasterizeContours(
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    return _p->rasterizeContours(pOutMetric, queryController);
}

QHash<OsmAnd::BandIndex, sk_sp<const SkImage>> OsmAnd::GeoTileRasterizer::rasterizeContours(
    QHash<BandIndex, QByteArray>& outEncImgData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    return _p->rasterizeContours(outEncImgData, pOutMetric, queryController);
}

QHash<OsmAnd::BandIndex, QList<OsmAnd::Ref<OsmAnd::GeoContour>>> OsmAnd::GeoTileRasterizer::evaluateContours(
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    return _p->evaluateContours(pOutMetric, queryController);
}

sk_sp<SkImage> OsmAnd::GeoTileRasterizer::rasterizeBandContours(
    const QList<Ref<GeoContour>>& contours,
    const TileId tileId,
    const ZoomLevel zoom,
    const int width,
    const int height)
{
    return GeoTileRasterizer_P::rasterizeBandContours(contours, tileId, zoom, width, height);
}
