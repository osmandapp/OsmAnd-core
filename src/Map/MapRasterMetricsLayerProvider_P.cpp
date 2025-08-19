#include "MapRasterMetricsLayerProvider_P.h"
#include "MapRasterMetricsLayerProvider.h"

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkImageEncoder.h>
#include <SkFont.h>
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
    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(tileSize, tileSize)))
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to allocate buffer for rasterization surface %dx%d", tileSize, tileSize);
        return false;
    }
    SkCanvas canvas(bitmap);
    canvas.clear(SK_ColorDKGRAY);

    QString text = QString(QLatin1String("TILE %1x%2 z%3\n")).arg(request.tileId.x).arg(request.tileId.y).arg(request.zoom);
    
    QString readTime = "?";
    int readObjects = 0;
    if (const auto obtainBinaryMapObjectsMetric = obtainDataMetric.findSubmetricOfType<ObfMapObjectsProvider_Metrics::Metric_obtainData>(true))
    {
        readTime = QString::number(obtainBinaryMapObjectsMetric->elapsedTime, 'f', 1);
        readObjects = obtainBinaryMapObjectsMetric->objectsCount;
    }

    text += QString(QLatin1String("Read %1s, %2 objects,\n")).arg(readTime).arg(readObjects);
    
    QString orderTime = "0.0";
    int orderObjects = 0;
    if (const auto primitiviseMetric = obtainDataMetric.findSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitivise>(true))
    {
        orderTime = QString::number(primitiviseMetric->elapsedTimeForOrderEvaluation, 'f', 1);
        orderObjects = primitiviseMetric->orderEvaluations;
    }

    text += QString(QLatin1String("Order %1s, %2 objects,\n")).arg(orderTime).arg(orderObjects);
    
    int areaCount = 0, lineCount = 0, pointCount = 0;
    if (const auto primitiviseMetric = obtainDataMetric.findSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitivise>(true))
    {
        areaCount = primitiviseMetric->polygonPrimitives;
        lineCount = primitiviseMetric->polylinePrimitives;
        pointCount = primitiviseMetric->pointPrimitives;
    }

    text += QString(QLatin1String("Area %1, Line %2, pnts %3\n")).arg(areaCount).arg(lineCount).arg(pointCount);
    
    QString styleTime = "0.0";
    if (const auto primitiviseMetric = obtainDataMetric.findSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitivise>(true))
    {
        styleTime = QString::number(primitiviseMetric->elapsedTime, 'f', 1);
    }

    text += QString(QLatin1String("Style %1s\n")).arg(styleTime);
    
    QString rasterizationTime = "0.0";
    if (const auto rasterizeMetric = obtainDataMetric.findSubmetricOfType<MapRasterizer_Metrics::Metric_rasterize>(true))
    {
        rasterizationTime = QString::number(rasterizeMetric->elapsedTime, 'f', 1);
    }

    text += QString(QLatin1String("Rasterization %1s\n")).arg(rasterizationTime);
    text += QString(QLatin1String("Total %1s")).arg(QString::number(obtainDataMetric.elapsedTime, 'f', 1));

    const auto fontSize = 14.0f * owner->densityFactor;

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorGREEN);

    SkFont textFont;
    textFont.setSize(fontSize);

    auto topOffset = fontSize;
    const auto lines = text.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    for (const auto& line : lines)
    {
        canvas.drawSimpleText(
            line.constData(), line.length()*sizeof(QChar), SkTextEncoding::kUTF16,
            5, topOffset,
            textFont,
            textPaint);
        topOffset += 1.15f * fontSize;
    }

    outData.reset(new MapRasterMetricsLayerProvider::Data(
        request.tileId,
        request.zoom,
        AlphaChannelPresence::NotPresent,
        owner->densityFactor,
        bitmap.asImage(), 
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
