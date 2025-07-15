#include "MapRasterLayerProvider_P.h"
#include "MapRasterLayerProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_Metrics.h"
#include "MapPrimitiviser.h"
#include "MapRasterizer.h"
#include "Stopwatch.h"

OsmAnd::MapRasterLayerProvider_P::MapRasterLayerProvider_P(MapRasterLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapRasterLayerProvider_P::~MapRasterLayerProvider_P()
{
}

bool OsmAnd::MapRasterLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    if (pOutMetric)
    {
        if (!pOutMetric->get() || !dynamic_cast<MapRasterLayerProvider_Metrics::Metric_obtainData*>(pOutMetric->get()))
            pOutMetric->reset(new MapRasterLayerProvider_Metrics::Metric_obtainData());
        else
            pOutMetric->get()->reset();
    }

    std::shared_ptr<MapRasterLayerProvider::Data> data;
    const auto result = obtainRasterizedTile(
        MapDataProviderHelpers::castRequest<MapRasterLayerProvider::Request>(request),
        data,
        pOutMetric ? static_cast<MapRasterLayerProvider_Metrics::Metric_obtainData*>(pOutMetric->get()) : nullptr);
    outData = data;

    return result;
}

bool OsmAnd::MapRasterLayerProvider_P::obtainRasterizedTile(
    const MapRasterLayerProvider::Request& request, 
    std::shared_ptr<MapRasterLayerProvider::Data>& outData,
    MapRasterLayerProvider_Metrics::Metric_obtainData* const metric)
{
    const Stopwatch totalStopwatch(OsmAnd::performanceLogsEnabled || metric != nullptr);

    // Obtain offline map primitives tile
    std::shared_ptr<MapPrimitivesProvider::Data> primitivesTile;
    owner->primitivesProvider->obtainTiledPrimitives(
        request,
        primitivesTile,
        metric ? metric->findOrAddSubmetricOfType<MapPrimitivesProvider_Metrics::Metric_obtainData>().get() : nullptr);
    if (!primitivesTile || primitivesTile->primitivisedObjects->isEmpty())
    {
        outData.reset();

        if (metric)
            metric->elapsedTime += totalStopwatch.elapsed();

        return true;
    }

    // Perform actual rasterization
    const auto image = rasterize(request, primitivesTile, metric);
    if (!image)
    {
        if (metric)
            metric->elapsedTime += totalStopwatch.elapsed();

        return false;
    }

    // Or supply newly rasterized tile
    outData.reset(new MapRasterLayerProvider::Data(
        request.tileId,
        request.zoom,
        AlphaChannelPresence::NotPresent,
        owner->getTileDensityFactor(),
        image,
        primitivesTile,
        new RetainableCacheMetadata(primitivesTile->retainableCacheMetadata)));

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    if (OsmAnd::performanceLogsEnabled)
    {
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld RASTER %f: %dx%d@%d rasterized on CPU",
            millis, totalStopwatch.elapsed(),
            request.tileId.x,
            request.tileId.y,
            request.zoom);
        //qPrintable(metric ? metric->toString(QLatin1String("\t - ")) : QLatin1String("(null)")));
    }

    return true;
}

void OsmAnd::MapRasterLayerProvider_P::initialize()
{
    _mapRasterizer.reset(new MapRasterizer(owner->primitivesProvider->primitiviser->environment));
}

OsmAnd::ZoomLevel OsmAnd::MapRasterLayerProvider_P::getMinZoom() const
{
    return owner->primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapRasterLayerProvider_P::getMaxZoom() const
{
    return owner->primitivesProvider->getMaxZoom();
}

OsmAnd::MapRasterLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata_)
    : binaryMapPrimitivesRetainableCacheMetadata(binaryMapPrimitivesRetainableCacheMetadata_)
{
}

OsmAnd::MapRasterLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
