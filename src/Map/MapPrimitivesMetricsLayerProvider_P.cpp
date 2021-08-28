#include "MapPrimitivesMetricsLayerProvider_P.h"
#include "MapPrimitivesMetricsLayerProvider.h"

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
#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_Metrics.h"
#include "MapPrimitiviser_Metrics.h"
#include "ObfMapObjectsProvider_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

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
    SkCanvas canvas(*bitmap);
    canvas.clear(SK_ColorDKGRAY);

    QString text;
    text += QString(QLatin1String("TILE   %1x%2@%3\n"))
        .arg(request.tileId.x)
        .arg(request.tileId.y)
        .arg(request.zoom);
    QString obtainBinaryMapObjectsElapsedTime(QLatin1String("?"));
    if (const auto obtainBinaryMapObjectsMetric = obtainDataMetric.findSubmetricOfType<ObfMapObjectsProvider_Metrics::Metric_obtainData>(true))
    {
        obtainBinaryMapObjectsElapsedTime = QString::number(obtainBinaryMapObjectsMetric->elapsedTime, 'f', 2);
    }
    QString primitiviseElapsedTime(QLatin1String("?"));
    if (const auto primitiviseMetric = obtainDataMetric.findSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitiviseWithSurface>(true))
    {
        text += QString(QLatin1String("order %1/-%2 %3s ~%4us/e\n"))
            .arg(primitiviseMetric->orderEvaluations)
            .arg(primitiviseMetric->orderRejects)
            .arg(QString::number(primitiviseMetric->elapsedTimeForOrderEvaluation, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForOrderEvaluation * 1000000.0f / primitiviseMetric->orderEvaluations));
        text += QString(QLatin1String("polyg %1/-%2(-%3) %4s ~%5us/e\n"))
            .arg(primitiviseMetric->polygonEvaluations)
            .arg(primitiviseMetric->polygonRejects)
            .arg(primitiviseMetric->polygonsRejectedByArea)
            .arg(QString::number(primitiviseMetric->elapsedTimeForPolygonEvaluation, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForPolygonEvaluation * 1000000.0f / primitiviseMetric->polygonEvaluations));
        text += QString(QLatin1String("%1s ~%2us/p\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForPolygonProcessing, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForPolygonProcessing * 1000000.0f / primitiviseMetric->polygonPrimitives));
        text += QString(QLatin1String("polyl %1/-%2(-%3) %4s ~%5us/e\n"))
            .arg(primitiviseMetric->polylineEvaluations)
            .arg(primitiviseMetric->polylineRejects)
            .arg(primitiviseMetric->polylineRejectedByDensity)
            .arg(QString::number(primitiviseMetric->elapsedTimeForPolylineEvaluation, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForPolylineEvaluation * 1000000.0f / primitiviseMetric->polylineEvaluations));
        text += QString(QLatin1String("%1s ~%2us/p\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForPolylineProcessing, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForPolylineProcessing * 1000000.0f / primitiviseMetric->polylinePrimitives));
        text += QString(QLatin1String("point %1/-%2 %3s ~%4us/e\n"))
            .arg(primitiviseMetric->pointEvaluations)
            .arg(primitiviseMetric->pointRejects)
            .arg(QString::number(primitiviseMetric->elapsedTimeForPointEvaluation, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForPointEvaluation * 1000000.0f / primitiviseMetric->pointEvaluations));
        text += QString(QLatin1String("%1s ~%2us/p\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForPointProcessing, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForPointProcessing * 1000000.0f / primitiviseMetric->pointPrimitives));
        const auto deltaGroups =
            primitiviseMetric->elapsedTimeForObtainingPrimitivesGroups -
            primitiviseMetric->elapsedTimeForOrderEvaluation -
            primitiviseMetric->elapsedTimeForOrderProcessing -
            primitiviseMetric->elapsedTimeForPolygonEvaluation -
            primitiviseMetric->elapsedTimeForPolygonProcessing -
            primitiviseMetric->elapsedTimeForPolylineEvaluation -
            primitiviseMetric->elapsedTimeForPolylineProcessing -
            primitiviseMetric->elapsedTimeForPointEvaluation -
            primitiviseMetric->elapsedTimeForPointProcessing;
        text += QString(QLatin1String("grp %1s (-^=%2s)\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesGroups, 'f', 2))
            .arg(QString::number(deltaGroups, 'f', 2));
        text += QString(QLatin1String("prim %1s+s%2s+?=%3s\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesGroups, 'f', 2))
            .arg(QString::number(primitiviseMetric->elapsedTimeForFutureSharedPrimitivesGroups, 'f', 2))
            .arg(QString::number(primitiviseMetric->elapsedTimeForPrimitives, 'f', 2));
        text += QString(QLatin1String("d/b/c %1s/%2s/%3s\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesFromDetailedmap, 'f', 2))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesFromBasemap, 'f', 2))
            .arg(QString::number(primitiviseMetric->elapsedTimeForObtainingPrimitivesFromCoastlines, 'f', 2));
        text += QString(QLatin1String("txt %1(-%2) %3s ~%4us/e ~%5us/p\n"))
            .arg(primitiviseMetric->obtainedTextSymbols)
            .arg(primitiviseMetric->rejectedTextSymbols)
            .arg(QString::number(primitiviseMetric->elapsedTimeForTextSymbolsEvaluation + primitiviseMetric->elapsedTimeForTextSymbolsProcessing, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForTextSymbolsEvaluation * 1000000.0f / primitiviseMetric->textSymbolsEvaluations))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForTextSymbolsProcessing * 1000000.0f / (primitiviseMetric->obtainedTextSymbols + primitiviseMetric->rejectedTextSymbols)));
        text += QString(QLatin1String("icn %1(-%2) %3s ~%4us/p\n"))
            .arg(primitiviseMetric->obtainedIconSymbols)
            .arg(primitiviseMetric->rejectedIconSymbols)
            .arg(QString::number(primitiviseMetric->elapsedTimeForIconSymbolsProcessing, 'f', 2))
            .arg(static_cast<int>(primitiviseMetric->elapsedTimeForIconSymbolsProcessing * 1000000.0f / (primitiviseMetric->obtainedIconSymbols + primitiviseMetric->rejectedIconSymbols)));
        const auto deltaSymbols =
            primitiviseMetric->elapsedTimeForSymbolsGroupsProcessing -
            primitiviseMetric->elapsedTimeForTextSymbolsEvaluation -
            primitiviseMetric->elapsedTimeForTextSymbolsProcessing -
            primitiviseMetric->elapsedTimeForIconSymbolsProcessing;
        text += QString(QLatin1String("sym %1s(-^=%2s) %3->%4\n"))
            .arg(QString::number(primitiviseMetric->elapsedTimeForSymbolsGroupsProcessing, 'f', 2))
            .arg(QString::number(deltaSymbols, 'f', 2))
            .arg(primitiviseMetric->symbolsGroupsProcessed)
            .arg(primitiviseMetric->obtainedTextSymbols + primitiviseMetric->obtainedIconSymbols);
        primitiviseElapsedTime = QString::number(primitiviseMetric->elapsedTime, 'f', 2);
    }
    text += QString(QLatin1String("total r%1+p%2+?=%3s\n"))
        .arg(obtainBinaryMapObjectsElapsedTime)
        .arg(primitiviseElapsedTime)
        .arg(QString::number(obtainDataMetric.elapsedTime, 'f', 2));
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
