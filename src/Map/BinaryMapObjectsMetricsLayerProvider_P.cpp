#include "BinaryMapObjectsMetricsLayerProvider_P.h"
#include "BinaryMapObjectsMetricsLayerProvider.h"

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

#include "ObfMapObjectsProvider.h"
#include "ObfMapObjectsProvider_Metrics.h"
#include "ObfMapSectionReader_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapRasterizer.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapObjectsMetricsLayerProvider_P::BinaryMapObjectsMetricsLayerProvider_P(BinaryMapObjectsMetricsLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapObjectsMetricsLayerProvider_P::~BinaryMapObjectsMetricsLayerProvider_P()
{
}

bool OsmAnd::BinaryMapObjectsMetricsLayerProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<BinaryMapObjectsMetricsLayerProvider::Data>& outTiledData,
    const IQueryController* const queryController)
{
    ObfMapObjectsProvider_Metrics::Metric_obtainData obtainDataMetric;

    // Obtain offline map data tile
    std::shared_ptr<ObfMapObjectsProvider::Data> binaryData;
    owner->dataProvider->obtainData(tileId, zoom, binaryData, &obtainDataMetric, nullptr);
    if (!binaryData)
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
    if (const auto loadMapObjectsMetric = obtainDataMetric.findSubmetricOfType<ObfMapSectionReader_Metrics::Metric_loadMapObjects>())
    {
        text += QString(QLatin1String("BLOCKS r:%1+s:%2=%3\n"))
            .arg(loadMapObjectsMetric->mapObjectsBlocksRead)
            .arg(loadMapObjectsMetric->mapObjectsBlocksReferenced)
            .arg(loadMapObjectsMetric->mapObjectsBlocksProcessed);
    }
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

    outTiledData.reset(new BinaryMapObjectsMetricsLayerProvider::Data(
        tileId,
        zoom,
        AlphaChannelData::NotPresent,
        owner->densityFactor,
        bitmap,
        binaryData,
        new RetainableCacheMetadata(binaryData->retainableCacheMetadata)));

    return true;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapObjectsMetricsLayerProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapObjectsMetricsLayerProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}

OsmAnd::BinaryMapObjectsMetricsLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapRetainableCacheMetadata_)
    : binaryMapRetainableCacheMetadata(binaryMapRetainableCacheMetadata_)
{
}

OsmAnd::BinaryMapObjectsMetricsLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
