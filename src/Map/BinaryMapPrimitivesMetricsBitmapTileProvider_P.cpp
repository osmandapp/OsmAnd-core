#include "BinaryMapPrimitivesMetricsBitmapTileProvider_P.h"
#include "BinaryMapPrimitivesMetricsBitmapTileProvider.h"

#include "stdlib_common.h"

#include "QtExtensions.h"

#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

#include "BinaryMapPrimitivesProvider.h"
#include "BinaryMapPrimitivesProvider_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider_P::BinaryMapPrimitivesMetricsBitmapTileProvider_P(BinaryMapPrimitivesMetricsBitmapTileProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider_P::~BinaryMapPrimitivesMetricsBitmapTileProvider_P()
{
}

bool OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider_P::obtainData(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<MapTiledData>& outTiledData, const IQueryController* const queryController)
{
    BinaryMapPrimitivesProvider_Metrics::Metric_obtainData obtainDataMetric;

    // Obtain offline map primitives tile
    std::shared_ptr<MapTiledData> primitivesTile_;
    owner->primitivesProvider->obtainData(tileId, zoom, primitivesTile_, &obtainDataMetric);
    if (!primitivesTile_)
    {
        outTiledData.reset();
        return true;
    }
    const auto primitivesTile = std::static_pointer_cast<BinaryMapPrimitivesTile>(primitivesTile_);

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
    text = text.trimmed();
    text += QString(QLatin1String("order  %1/-%2 %3s\n"))
        .arg(obtainDataMetric.primitiviseMetric.orderEvaluations)
        .arg(obtainDataMetric.primitiviseMetric.orderRejects)
        .arg(QString::number(obtainDataMetric.primitiviseMetric.elapsedTimeForOrderEvaluation, 'f', 2));
    text += QString(QLatin1String("polyg  %1/-%2 %3s\n"))
        .arg(obtainDataMetric.primitiviseMetric.polygonEvaluations)
        .arg(obtainDataMetric.primitiviseMetric.polygonsRejectedByArea)
        .arg(QString::number(obtainDataMetric.primitiviseMetric.elapsedTimeForPolygonEvaluation, 'f', 2));
    text += QString(QLatin1String("polyl  %1 %2s\n"))
        .arg(obtainDataMetric.primitiviseMetric.polylineEvaluations)
        .arg(QString::number(obtainDataMetric.primitiviseMetric.elapsedTimeForPolylineEvaluation, 'f', 2));
    text += QString(QLatin1String("point  %1 %2s\n"))
        .arg(obtainDataMetric.primitiviseMetric.pointEvaluations)
        .arg(QString::number(obtainDataMetric.primitiviseMetric.elapsedTimeForPointEvaluation, 'f', 2));
    text += QString(QLatin1String("TIME   r%1+p%2+?=%3s\n"))
        .arg(QString::number(obtainDataMetric.obtainBinaryMapDataMetric.elapsedTime, 'f', 2))
        .arg(QString::number(obtainDataMetric.primitiviseMetric.elapsedTime, 'f', 2))
        .arg(QString::number(obtainDataMetric.elapsedTime, 'f', 2));
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

    outTiledData.reset(new BinaryMapPrimitivesMetricsTile(
        primitivesTile,
        bitmap,
        owner->densityFactor,
        tileId,
        zoom));

    return true;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}
