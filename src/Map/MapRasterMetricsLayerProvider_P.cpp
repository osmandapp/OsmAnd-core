#include "MapRasterMetricsLayerProvider_P.h"
#include "MapRasterMetricsLayerProvider.h"

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageEncoder.h>
#include "restore_internal_warnings.h"

#include "MapDataProviderHelpers.h"
#include "MapRasterLayerProvider.h"
#include "MapRasterLayerProvider_Metrics.h"
#include "ObfMapObjectsProvider_Metrics.h"
#include "MapRasterizer_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::MapRasterMetricsLayerProvider_P::MapRasterMetricsLayerProvider_P(MapRasterMetricsLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapRasterMetricsLayerProvider_P::~MapRasterMetricsLayerProvider_P()
{
}

bool OsmAnd::MapRasterMetricsLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<MapRasterMetricsLayerProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    MapRasterLayerProvider_Metrics::Metric_obtainData obtainDataMetric;

    // Obtain offline map primitives tile
    std::shared_ptr<MapRasterLayerProvider::Data> rasterizedTile;
    owner->rasterBitmapTileProvider->obtainRasterizedTile(request, rasterizedTile, &obtainDataMetric);
    if (!rasterizedTile)
    {
        outData.reset();
        return true;
    }

    // Prepare drawing canvas
    const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    if (!bitmap->tryAllocPixels(SkImageInfo::MakeN32Premul(owner->tileSize, owner->tileSize)))
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to allocate buffer for rasterization surface %dx%d",
            owner->tileSize,
            owner->tileSize);
        return false;
    }
    SkCanvas canvas(*bitmap);
    canvas.clear(SK_ColorDKGRAY);

    QString text;
    text += QString(QLatin1String("TILE   %1x%2@%3\n"))
        .arg(request.tileId.x)
        .arg(request.tileId.y)
        .arg(request.zoom);
    if (const auto obtainBinaryMapObjectsMetric = obtainDataMetric.findSubmetricOfType<ObfMapObjectsProvider_Metrics::Metric_obtainData>(true))
    {
        text += QString(QLatin1String("obf read    %1s\n"))
            .arg(QString::number(obtainBinaryMapObjectsMetric->elapsedTime, 'f', 3));
    }
    if (const auto primitiviseMetric = obtainDataMetric.findSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitivise>(true))
    {
        text += QString(QLatin1String("primitives  %1s\n"))
            .arg(QString::number(primitiviseMetric->elapsedTime, 'f', 3));
    }
    if (const auto rasterizeMetric = obtainDataMetric.findSubmetricOfType<MapRasterizer_Metrics::Metric_rasterize>(true))
    {
        text += QString(QLatin1String("rasterize   %1s\n"))
            .arg(QString::number(rasterizeMetric->elapsedTime, 'f', 3));
    }
    text += QString(QLatin1String("--total--   %1s\n"))
        .arg(QString::number(obtainDataMetric.elapsedTime, 'f', 3));
    text = text.trimmed();

    const auto fontSize = 14.0f * owner->densityFactor;

    SkPaint textPaint;
    SkFont textFont;
    textPaint.setAntiAlias(true);
//    textPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    textFont.setSize(fontSize);
    textPaint.setColor(SK_ColorGREEN);

    auto topOffset = fontSize;
    const auto lines = text.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    for (const auto& line : lines)
    {
        canvas.drawSimpleText(line.constData(), line.length()*sizeof(QChar), SkTextEncoding::kUTF16, 5, topOffset, textFont, textPaint);
        topOffset += 1.15f * fontSize;
    }

    outData.reset(new MapRasterMetricsLayerProvider::Data(
        request.tileId,
        request.zoom,
        AlphaChannelPresence::NotPresent,
        owner->densityFactor,
        bitmap, 
        rasterizedTile,
        new RetainableCacheMetadata(rasterizedTile->retainableCacheMetadata)));

    return true;
}

OsmAnd::ZoomLevel OsmAnd::MapRasterMetricsLayerProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::MapRasterMetricsLayerProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}

OsmAnd::MapRasterMetricsLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& rasterizedBinaryMapRetainableCacheMetadata_)
    : rasterizedBinaryMapRetainableCacheMetadata(rasterizedBinaryMapRetainableCacheMetadata_)
{
}

OsmAnd::MapRasterMetricsLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
