#include "BinaryMapMetricsBitmapTileProvider_P.h"
#include "BinaryMapMetricsBitmapTileProvider.h"

#include "stdlib_common.h"

#include "QtExtensions.h"

#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

#include "BinaryMapDataProvider.h"
#include "BinaryMapDataProvider_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "Rasterizer.h"
#include "RasterizerContext.h"
#include "RasterizerEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapMetricsBitmapTileProvider_P::BinaryMapMetricsBitmapTileProvider_P(BinaryMapMetricsBitmapTileProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapMetricsBitmapTileProvider_P::~BinaryMapMetricsBitmapTileProvider_P()
{
}

bool OsmAnd::BinaryMapMetricsBitmapTileProvider_P::obtainData(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<MapTiledData>& outTiledData, const IQueryController* const queryController)
{
    BinaryMapDataProvider_Metrics::Metric_obtainData obtainDataMetric;

    // Obtain offline map data tile
    std::shared_ptr<MapTiledData> dataTile_;
    owner->dataProvider->obtainData(tileId, zoom, dataTile_, &obtainDataMetric);
    if (!dataTile_)
    {
        outTiledData.reset();
        return true;
    }
    const auto dataTile = std::static_pointer_cast<BinaryMapDataTile>(dataTile_);

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
    text += QString(QLatin1String("BLOCKS r:%1+s:%2=%3\n"))
        .arg(obtainDataMetric.loadMapObjectsMetric.mapObjectsBlocksRead)
        .arg(obtainDataMetric.loadMapObjectsMetric.mapObjectsBlocksReferenced)
        .arg(obtainDataMetric.loadMapObjectsMetric.mapObjectsBlocksProcessed);
    text += QString(QLatin1String("OBJS   u:%1+s:%2=%3\n"))
        .arg(obtainDataMetric.uniqueObjectsCount)
        .arg(obtainDataMetric.sharedObjectsCount)
        .arg(obtainDataMetric.objectsCount);
    text += QString(QLatin1String("TIME   r:%1+?=%2s\n"))
        .arg(QString::number(obtainDataMetric.elapsedTimeForRead, 'f', 2))
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

    outTiledData.reset(new BinaryMapMetricsTile(
        dataTile,
        bitmap,
        owner->densityFactor,
        tileId,
        zoom));

    return true;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapMetricsBitmapTileProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapMetricsBitmapTileProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}
