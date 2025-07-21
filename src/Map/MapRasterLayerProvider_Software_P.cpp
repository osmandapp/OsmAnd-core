#include "MapRasterLayerProvider_Software_P.h"
#include "MapRasterLayerProvider_Software.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkImageEncoder.h>
#include "restore_internal_warnings.h"

#include "MapPrimitivesProvider.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapRasterizer.h"
#include "MapPresentationEnvironment.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::MapRasterLayerProvider_Software_P::MapRasterLayerProvider_Software_P(MapRasterLayerProvider_Software* owner_)
    : MapRasterLayerProvider_P(owner_)
    , owner(owner_)
{
}

OsmAnd::MapRasterLayerProvider_Software_P::~MapRasterLayerProvider_Software_P()
{
}

sk_sp<SkImage> OsmAnd::MapRasterLayerProvider_Software_P::rasterize(
    const MapRasterLayerProvider::Request& request,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& primitivesTile,
    MapRasterLayerProvider_Metrics::Metric_obtainData* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    // Allocate rasterization target
    const auto tileSize = owner->getTileSize();
    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(tileSize, tileSize)))
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to allocate buffer for rasterization surface %dx%d",
            tileSize,
            tileSize);
        return nullptr;
    }

    // Create rasterization canvas
    SkCanvas canvas(bitmap);

    // Perform actual rasterization
    if (!owner->fillBackground)
        canvas.clear(SK_ColorTRANSPARENT);
    _mapRasterizer->rasterize(
        Utilities::tileBoundingBox31(request.tileId, request.zoom),
        primitivesTile->primitivisedObjects,
        canvas,
        owner->fillBackground,
        nullptr,
        metric ? metric->findOrAddSubmetricOfType<MapRasterizer_Metrics::Metric_rasterize>().get() : nullptr,
        request.queryController);

    return bitmap.asImage();
}
