#include "MapPrimitivesMetricsLayerProvider_P.h"
#include "MapPrimitivesMetricsLayerProvider.h"

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

#include "MapDataProviderHelpers.h"
#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_Metrics.h"
#include "MapPrimitiviser_Metrics.h"
#include "ObfMapObjectsProvider_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

#define FORMAT_PRECISION 3

OsmAnd::MapPrimitivesMetricsLayerProvider_P::MapPrimitivesMetricsLayerProvider_P(
    MapPrimitivesMetricsLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapPrimitivesMetricsLayerProvider_P::~MapPrimitivesMetricsLayerProvider_P()
{
}

bool OsmAnd::MapPrimitivesMetricsLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<MapPrimitivesMetricsLayerProvider::Request>(request_);
    if (pOutMetric)
        pOutMetric->reset();

    MapPrimitivesProvider_Metrics::Metric_obtainData obtainDataMetric;

    // Obtain offline map primitives tile
    std::shared_ptr<MapPrimitivesProvider::Data> primitivesTile;
    owner->primitivesProvider->obtainTiledPrimitives(request, primitivesTile, &obtainDataMetric);
    if (!primitivesTile)
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
    SkBitmapDevice target(*bitmap);
    SkCanvas canvas(&target);
    canvas.clear(SK_ColorDKGRAY);

    QString text;
    text += QString(QLatin1String("TILE   %1x%2@%3\n"))
        .arg(request.tileId.x)
        .arg(request.tileId.y)
        .arg(request.zoom);
    QString obtainBinaryMapObjectsElapsedTime(QLatin1String("?"));
    if (const auto obtainBinaryMapObjectsMetric = obtainDataMetric.findSubmetricOfType<ObfMapObjectsProvider_Metrics::Metric_obtainData>(true))
    {
        obtainBinaryMapObjectsElapsedTime = QString::number(obtainBinaryMapObjectsMetric->elapsedTime, 'f', FORMAT_PRECISION);
    }
    QString primitiviseElapsedTime(QLatin1String("?"));
    if (const auto primitiviseMetric = obtainDataMetric.findSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitiviseWithSurface>(true))
    {
        text += QString(QLatin1String("order  %1/-%2 %3s\n"))
            .arg(primitiviseMetric->orderEvaluations)
            .arg(primitiviseMetric->orderRejects)
            .arg(QString::number(primitiviseMetric->elapsedTimeForOrderEvaluation, 'f', FORMAT_PRECISION));
        text += QString(QLatin1String("polyg  %1/-%2(-%3) %4s\n"))
            .arg(primitiviseMetric->polygonEvaluations)
            .arg(primitiviseMetric->polygonRejects)
            .arg(primitiviseMetric->polygonsRejectedByArea)
            .arg(QString::number(primitiviseMetric->elapsedTimeForPolygonEvaluation, 'f', FORMAT_PRECISION));
        text += QString(QLatin1String("polyl  %1/-%2(-%3) %4s\n"))
            .arg(primitiviseMetric->polylineEvaluations)
            .arg(primitiviseMetric->polylineRejects)
            .arg(primitiviseMetric->polylineRejectedByDensity)
            .arg(QString::number(primitiviseMetric->elapsedTimeForPolylineEvaluation, 'f', FORMAT_PRECISION));
        text += QString(QLatin1String("point  %1/-%2 %3s\n"))
            .arg(primitiviseMetric->pointEvaluations)
            .arg(primitiviseMetric->pointRejects)
            .arg(QString::number(primitiviseMetric->elapsedTimeForPointEvaluation, 'f', FORMAT_PRECISION));
        const auto deltaGroups =
            primitiviseMetric->elapsedTimeForObtainingPrimitivesGroups -
            primitiviseMetric->elapsedTimeForOrderEvaluation -
            primitiviseMetric->elapsedTimeForPolygonEvaluation -
            primitiviseMetric->elapsedTimeForPolylineEvaluation -
            primitiviseMetric->elapsedTimeForPointEvaluation;
        text += QString(QLatin1String("groups %1s (- ^ = %2s)\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesGroups, 'f', FORMAT_PRECISION))
            .arg(QString::number(deltaGroups, 'f', FORMAT_PRECISION));
        text += QString(QLatin1String("d/b/c  %1s/%2s/%3s\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesFromDetailedmap, 'f', FORMAT_PRECISION))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesFromBasemap, 'f', FORMAT_PRECISION))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesFromCoastlines, 'f', FORMAT_PRECISION));
        text += QString(QLatin1String("sym    %1s\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesSymbols, 'f', FORMAT_PRECISION));
        primitiviseElapsedTime = QString::number(primitiviseMetric->elapsedTime, 'f', FORMAT_PRECISION);
    }
    text += QString(QLatin1String("TIME   r%1+p%2+?=%3s\n"))
        .arg(obtainBinaryMapObjectsElapsedTime)
        .arg(primitiviseElapsedTime)
        .arg(QString::number(obtainDataMetric.elapsedTime, 'f', FORMAT_PRECISION));
    text = text.trimmed();

    const auto fontSize = 16.0f * owner->densityFactor;

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

    outData.reset(new MapPrimitivesMetricsLayerProvider::Data(
        request.tileId,
        request.zoom,
        AlphaChannelPresence::NotPresent,
        owner->densityFactor,
        bitmap,
        primitivesTile,
        new RetainableCacheMetadata(primitivesTile->retainableCacheMetadata)));

    return true;
}

OsmAnd::ZoomLevel OsmAnd::MapPrimitivesMetricsLayerProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::MapPrimitivesMetricsLayerProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}

OsmAnd::MapPrimitivesMetricsLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata_)
    : binaryMapPrimitivesRetainableCacheMetadata(binaryMapPrimitivesRetainableCacheMetadata_)
{
}

OsmAnd::MapPrimitivesMetricsLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
