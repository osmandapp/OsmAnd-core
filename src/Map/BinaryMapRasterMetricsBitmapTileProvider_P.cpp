#include "BinaryMapRasterMetricsBitmapTileProvider_P.h"
#include "BinaryMapRasterMetricsBitmapTileProvider.h"

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

#include "BinaryMapRasterBitmapTileProvider.h"
#include "BinaryMapRasterBitmapTileProvider_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapRasterMetricsBitmapTileProvider_P::BinaryMapRasterMetricsBitmapTileProvider_P(BinaryMapRasterMetricsBitmapTileProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapRasterMetricsBitmapTileProvider_P::~BinaryMapRasterMetricsBitmapTileProvider_P()
{
}

bool OsmAnd::BinaryMapRasterMetricsBitmapTileProvider_P::obtainData(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<MapTiledData>& outTiledData, const IQueryController* const queryController)
{
    BinaryMapRasterBitmapTileProvider_Metrics::Metric_obtainData obtainDataMetric;

    // Obtain offline map primitives tile
    std::shared_ptr<MapTiledData> rasterizedTile_;
    owner->rasterBitmapTileProvider->obtainData(tileId, zoom, rasterizedTile_, &obtainDataMetric);
    if (!rasterizedTile_)
    {
        outTiledData.reset();
        return true;
    }
    const auto rasterizedTile = std::static_pointer_cast<BinaryMapRasterizedTile>(rasterizedTile_);

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

    outTiledData.reset(new BinaryMapRasterMetricsBitmapTile(
        rasterizedTile,
        bitmap,
        owner->densityFactor,
        tileId,
        zoom));

    return true;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterMetricsBitmapTileProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterMetricsBitmapTileProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}
