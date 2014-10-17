#include "BinaryMapRasterMetricsLayerProvider_P.h"
#include "BinaryMapRasterMetricsLayerProvider.h"

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>
#include "restore_internal_warnings.h"

#include "BinaryMapRasterLayerProvider.h"
#include "BinaryMapRasterLayerProvider_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapRasterMetricsLayerProvider_P::BinaryMapRasterMetricsLayerProvider_P(BinaryMapRasterMetricsLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapRasterMetricsLayerProvider_P::~BinaryMapRasterMetricsLayerProvider_P()
{
}

bool OsmAnd::BinaryMapRasterMetricsLayerProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<BinaryMapRasterMetricsLayerProvider::Data>& outTiledData,
    const IQueryController* const queryController)
{
    BinaryMapRasterLayerProvider_Metrics::Metric_obtainData obtainDataMetric;

    // Obtain offline map primitives tile
    std::shared_ptr<BinaryMapRasterLayerProvider::Data> rasterizedTile;
    owner->rasterBitmapTileProvider->obtainData(tileId, zoom, rasterizedTile, &obtainDataMetric, nullptr);
    if (!rasterizedTile)
    {
        outTiledData.reset();
        return true;
    }

    // Prepare drawing canvas
    const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    bitmap->setConfig(SkBitmap::kARGB_8888_Config, owner->tileSize, owner->tileSize);
    if (!bitmap->allocPixels())
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to allocate buffer for ARGB8888 rasterization surface %dx%d", owner->tileSize, owner->tileSize);
        return false;
    }
    SkBitmapDevice target(*bitmap);
    SkCanvas canvas(&target);
    canvas.clear(SK_ColorDKGRAY);

    QString text;
    text += QString(QLatin1String("TILE   %1x%2@%3\n"))
        .arg(tileId.x)
        .arg(tileId.y)
        .arg(zoom);
    text += QString(QLatin1String("obf read    %1s\n"))
        .arg(QString::number(obtainDataMetric.obtainBinaryMapPrimitivesMetric.obtainBinaryMapDataMetric.elapsedTime, 'f', 3));
    text += QString(QLatin1String("primitives  %1s\n"))
        .arg(QString::number(obtainDataMetric.obtainBinaryMapPrimitivesMetric.primitiviseMetric.elapsedTime, 'f', 3));
    text += QString(QLatin1String("rasterize   %1s\n"))
        .arg(QString::number(obtainDataMetric.rasterizeMetric.elapsedTime, 'f', 3));
    text += QString(QLatin1String("--total--   %1s\n"))
        .arg(QString::number(obtainDataMetric.elapsedTime, 'f', 3));
    text = text.trimmed();

    const auto fontSize = 16.0f;

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    textPaint.setTextSize(fontSize);
    textPaint.setColor(SK_ColorGREEN);

    auto topOffset = fontSize;
    const auto lines = text.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    for (const auto& line : lines)
    {
        canvas.drawText(
            line.constData(), line.length()*sizeof(QChar),
            5, topOffset,
            textPaint);
        topOffset += 1.25f * fontSize;
    }

    outTiledData.reset(new BinaryMapRasterMetricsLayerProvider::Data(
        tileId,
        zoom,
        AlphaChannelData::NotPresent,
        owner->densityFactor,
        bitmap, 
        rasterizedTile,
        new RetainableCacheMetadata(rasterizedTile->retainableCacheMetadata)));

    return true;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterMetricsLayerProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterMetricsLayerProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}

OsmAnd::BinaryMapRasterMetricsLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& rasterizedBinaryMapRetainableCacheMetadata_)
    : rasterizedBinaryMapRetainableCacheMetadata(rasterizedBinaryMapRetainableCacheMetadata_)
{
}

OsmAnd::BinaryMapRasterMetricsLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
