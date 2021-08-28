#include "ObfMapObjectsMetricsLayerProvider_P.h"
#include "ObfMapObjectsMetricsLayerProvider.h"

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
#include "ObfMapObjectsProvider.h"
#include "ObfMapObjectsProvider_Metrics.h"
#include "ObfMapSectionReader_Metrics.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapRasterizer.h"
#include "MapPresentationEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

#define FORMAT_PRECISION 3

OsmAnd::ObfMapObjectsMetricsLayerProvider_P::ObfMapObjectsMetricsLayerProvider_P(
    ObfMapObjectsMetricsLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::ObfMapObjectsMetricsLayerProvider_P::~ObfMapObjectsMetricsLayerProvider_P()
{
}

bool OsmAnd::ObfMapObjectsMetricsLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<ObfMapObjectsMetricsLayerProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    ObfMapObjectsProvider_Metrics::Metric_obtainData obtainDataMetric;

    // Obtain offline map data tile
    std::shared_ptr<ObfMapObjectsProvider::Data> binaryData;
    owner->dataProvider->obtainTiledObfMapObjects(request, binaryData, &obtainDataMetric);
    if (!binaryData)
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
    if (const auto loadMapObjectsMetric = obtainDataMetric.findSubmetricOfType<ObfMapSectionReader_Metrics::Metric_loadMapObjects>(true))
    {
        text += QString(QLatin1String("blocks r:%1+s:%2=%3\n"))
            .arg(loadMapObjectsMetric->mapObjectsBlocksRead)
            .arg(loadMapObjectsMetric->mapObjectsBlocksReferenced)
            .arg(loadMapObjectsMetric->mapObjectsBlocksProcessed);
    }
    text += QString(QLatin1String("objects u:%1+s:%2=%3\n"))
        .arg(obtainDataMetric.uniqueObjectsCount)
        .arg(obtainDataMetric.sharedObjectsCount)
        .arg(obtainDataMetric.objectsCount);
    text += QString(QLatin1String("total r:%1+?=%2s\n"))
        .arg(QString::number(obtainDataMetric.elapsedTimeForRead, 'f', FORMAT_PRECISION))
        .arg(QString::number(obtainDataMetric.elapsedTime, 'f', FORMAT_PRECISION));
    text = text.trimmed();

    const auto fontSize = 14.0f * owner->densityFactor;

    SkPaint textPaint;
    SkFont skFontText;
    textPaint.setAntiAlias(true);
    //textPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    skFontText.setSize(fontSize);
    textPaint.setColor(SK_ColorGREEN);

    auto topOffset = fontSize;
    const auto lines = text.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    for (const auto& line : lines)
    {
        canvas.drawSimpleText(
            line.constData(), line.length()*sizeof(QChar), SkTextEncoding::kUTF16,
            5, topOffset, skFontText,
            textPaint);
        topOffset += 1.15f * fontSize;
    }

    outData.reset(new ObfMapObjectsMetricsLayerProvider::Data(
        request.tileId,
        request.zoom,
        AlphaChannelPresence::NotPresent,
        owner->densityFactor,
        bitmap,
        binaryData,
        new RetainableCacheMetadata(binaryData->retainableCacheMetadata)));

    return true;
}

OsmAnd::ZoomLevel OsmAnd::ObfMapObjectsMetricsLayerProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::ObfMapObjectsMetricsLayerProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}

OsmAnd::ObfMapObjectsMetricsLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapRetainableCacheMetadata_)
    : binaryMapRetainableCacheMetadata(binaryMapRetainableCacheMetadata_)
{
}

OsmAnd::ObfMapObjectsMetricsLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
