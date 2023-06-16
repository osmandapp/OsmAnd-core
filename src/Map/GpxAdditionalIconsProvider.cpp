#include "GpxAdditionalIconsProvider.h"

#include "MapDataProviderHelpers.h"
#include "Utilities.h"
#include "BillboardRasterMapSymbol.h"
#include "LatLon.h"
#include "GpxDocument.h"
#include "TextRasterizer.h"

#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkImage.h>
#include <SkRect.h>
#include <SkColor.h>

OsmAnd::GpxAdditionalIconsProvider::GpxAdditionalIconsProvider(
    const int baseOrder_,
    const double screenScale_,
    const QList<PointI>& startFinishPoints_,
    const QList<SplitLabel>& splitLabels_,
    const sk_sp<const SkImage>& startIcon_,
    const sk_sp<const SkImage>& finishIcon_,
    const sk_sp<const SkImage>& startFinishIcon_)
    : _cachedZoomLevel(MinZoomLevel)
    , _textRasterizer(TextRasterizer::getDefault())
    , baseOrder(baseOrder_)
    , screenScale(screenScale_)
    , startFinishPoints(startFinishPoints_)
    , splitLabels(splitLabels_)
    , startIcon(startIcon_)
    , finishIcon(finishIcon_)
    , startFinishIcon(startFinishIcon_)
{
    _captionStyle
        .setWrapWidth(100)
        .setMaxLines(1)
        .setBold(false)
        .setItalic(false)
        .setSize(12.0 * screenScale);
}

OsmAnd::GpxAdditionalIconsProvider::~GpxAdditionalIconsProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::GpxAdditionalIconsProvider::getMinZoom() const
{
    return OsmAnd::ZoomLevel5;
}

OsmAnd::ZoomLevel OsmAnd::GpxAdditionalIconsProvider::getMaxZoom() const
{
    return OsmAnd::MaxZoomLevel;
}

bool OsmAnd::GpxAdditionalIconsProvider::supportsNaturalObtainData() const
{
    return true;
}

sk_sp<SkImage> OsmAnd::GpxAdditionalIconsProvider::getSplitIconForValue(const SplitLabel& label)
{
    auto backgroundColor = label.gpxColor;
    ColorARGB textColor = Utilities::isColorBright(backgroundColor) ? ColorARGB(0xFF000000) : ColorARGB(0xFFFFFFFF);
    _captionStyle.setColor(textColor);
    _captionStyle.setBold(true);
    
    const auto textBmp = _textRasterizer->rasterize(label.text, _captionStyle);
    if (textBmp)
    {
        SkBitmap bitmap;
        double bitmapWidth = textBmp->width() + (20 * screenScale);
        double bitmapHeight = textBmp->height() + (17 * screenScale);
        double strokeWidth = 2.5 * screenScale;
        if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(bitmapWidth, bitmapHeight)))
        {
            LogPrintf(OsmAnd::LogSeverityLevel::Error,
                      "Failed to allocate bitmap of size %dx%d",
                      bitmapWidth,
                      bitmapHeight);
            return nullptr;
        }
        
        bitmap.eraseColor(SK_ColorTRANSPARENT);

        SkCanvas canvas(bitmap);
        SkPaint paint;
        paint.setStyle(SkPaint::Style::kStroke_Style);
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(255, backgroundColor.r, backgroundColor.g, backgroundColor.b));
        paint.setStrokeWidth(strokeWidth);
        SkRect rect;
        rect.setXYWH(strokeWidth, strokeWidth, bitmapWidth - (strokeWidth * 2), bitmapHeight - (strokeWidth * 2));
        canvas.drawRoundRect(rect, 40, 40, paint);
        
        paint.reset();
        paint.setStyle(SkPaint::Style::kFill_Style);
        paint.setColor(SkColorSetARGB(200, backgroundColor.r, backgroundColor.g, backgroundColor.b));
        rect.setXYWH(strokeWidth, strokeWidth, rect.width(), rect.height());
        canvas.drawRoundRect(rect, 36, 36, paint);
        
        canvas.drawImage(textBmp,
                         (bitmapWidth - textBmp->width()) / 2.0f,
                         (bitmapHeight - textBmp->height()) / 2.0f);
        
        canvas.flush();
        
        return bitmap.asImage();
    }
    return textBmp;
}

void OsmAnd::GpxAdditionalIconsProvider::buildSplitIntervalsSymbolsGroup(
    const AreaI& bbox31,
    const QList<SplitLabel>& labels,
    QList<std::shared_ptr<MapSymbolsGroup>>& mapSymbolsGroups)
{
    const auto mapSymbolsGroup = std::make_shared<OsmAnd::MapSymbolsGroup>();
    for (const auto& label : constOf(labels))
    {
        if (!bbox31.contains(label.pos31))
            continue;
        
        const auto bitmap = getSplitIconForValue(label);
        if (bitmap)
        {
            const auto mapSymbol = std::make_shared<OsmAnd::BillboardRasterMapSymbol>(mapSymbolsGroup);
            mapSymbol->order = baseOrder;
            mapSymbol->image = bitmap;
            mapSymbol->size = OsmAnd::PointI(bitmap->width(), bitmap->height());
            mapSymbol->languageId = OsmAnd::LanguageId::Invariant;
            mapSymbol->position31 = label.pos31;
            mapSymbolsGroup->symbols.push_back(mapSymbol);
        }
    }
    mapSymbolsGroups.push_back(mapSymbolsGroup);
}

void OsmAnd::GpxAdditionalIconsProvider::buildStartFinishSymbolsGroup(
    const AreaI &bbox31,
    double metersPerPixel,
    QList<std::shared_ptr<MapSymbolsGroup>>& mapSymbolsGroups)
{
    float iconSize = 14.0;
    
    const auto mapSymbolsGroup = std::make_shared<OsmAnd::MapSymbolsGroup>();
    for (int i = 0; i < startFinishPoints.size() - 1; i += 2)
    {
        const auto startPos31 = startFinishPoints[i];
        const auto finishPos31 = startFinishPoints[i + 1];
        
        bool containsStart = bbox31.contains(startPos31);
        bool containsFinish = bbox31.contains(finishPos31);
        if (containsStart && containsFinish)
        {
            double distance = ((iconSize * screenScale) * metersPerPixel) / 2;
            const auto startIconArea = Utilities::boundingBox31FromAreaInMeters(distance, startPos31);
            const auto finishIconArea = Utilities::boundingBox31FromAreaInMeters(distance, finishPos31);
            
            if (startIconArea.intersects(finishIconArea))
            {
                const auto mapSymbol = std::make_shared<OsmAnd::BillboardRasterMapSymbol>(mapSymbolsGroup);
                mapSymbol->order = baseOrder;
                mapSymbol->image = startFinishIcon;
                mapSymbol->size = PointI(startFinishIcon->width(), startFinishIcon->height());
                mapSymbol->languageId = LanguageId::Invariant;
                mapSymbol->position31 = startPos31;
                mapSymbolsGroup->symbols.push_back(mapSymbol);
                continue;
            }
        }
        if (containsStart)
        {
            const auto mapSymbol = std::make_shared<OsmAnd::BillboardRasterMapSymbol>(mapSymbolsGroup);
            mapSymbol->order = baseOrder;
            mapSymbol->image = startIcon;
            mapSymbol->size = OsmAnd::PointI(startIcon->width(), startIcon->height());
            mapSymbol->languageId = OsmAnd::LanguageId::Invariant;
            mapSymbol->position31 = startPos31;
            mapSymbolsGroup->symbols.push_back(mapSymbol);
        }
        if (containsFinish)
        {
            const auto mapSymbol = std::make_shared<OsmAnd::BillboardRasterMapSymbol>(mapSymbolsGroup);
            mapSymbol->order = baseOrder;
            mapSymbol->image = finishIcon;
            mapSymbol->size = OsmAnd::PointI(finishIcon->width(), finishIcon->height());
            mapSymbol->languageId = OsmAnd::LanguageId::Invariant;
            mapSymbol->position31 = finishPos31;
            mapSymbolsGroup->symbols.push_back(mapSymbol);
        }
    }
    mapSymbolsGroups.push_back(mapSymbolsGroup);
}

void OsmAnd::GpxAdditionalIconsProvider::buildVisibleSplits(
    const double metersPerPixel,
    QList<SplitLabel>& visibleSplits)
{
    if (splitLabels.empty())
        return;
    
    double distance = (50.0 * screenScale * metersPerPixel) / 2;
    const auto& firstLabel = splitLabels.first();
    auto prevIconArea = Utilities::boundingBox31FromAreaInMeters(distance, firstLabel.pos31);
    visibleSplits.append(firstLabel);
    for (int i = 1; i < splitLabels.size(); i++)
    {
        const auto& label = splitLabels[i];
        auto currentIconArea = Utilities::boundingBox31FromAreaInMeters(distance, label.pos31);
        if (!currentIconArea.intersects(prevIconArea))
        {
            visibleSplits.append(label);
            prevIconArea = currentIconArea;
        }
    }
}

QList<std::shared_ptr<OsmAnd::MapSymbolsGroup>> OsmAnd::GpxAdditionalIconsProvider::buildMapSymbolsGroups(const OsmAnd::AreaI &bbox31, const double metersPerPixel)
{
    QReadLocker scopedLocker(&_lock);

    QList<std::shared_ptr<OsmAnd::MapSymbolsGroup>> mapSymbolsGroups;
    buildStartFinishSymbolsGroup(bbox31, metersPerPixel, mapSymbolsGroups);
    buildSplitIntervalsSymbolsGroup(bbox31, _visibleSplitLabels, mapSymbolsGroups);
    return mapSymbolsGroups;
}

bool OsmAnd::GpxAdditionalIconsProvider::obtainData(const IMapDataProvider::Request& request,
                                            std::shared_ptr<IMapDataProvider::Data>& outData,
                                            std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    const auto& req = OsmAnd::MapDataProviderHelpers::castRequest<GpxAdditionalIconsProvider::Request>(request);
    if (pOutMetric)
        pOutMetric->reset();
    
    if (req.zoom > getMaxZoom() || req.zoom < getMinZoom())
    {
        outData.reset();
        return true;
    }
    
    if (req.mapState.zoomLevel != _cachedZoomLevel)
    {
        QWriteLocker scopedLocker(&_lock);
        
        _cachedZoomLevel = req.mapState.zoomLevel;
        _visibleSplitLabels.clear();
        buildVisibleSplits(req.mapState.metersPerPixel, _visibleSplitLabels);
    }
    
    const auto tileId = req.tileId;
    const auto zoom = req.zoom;
    const auto tileBBox31 = OsmAnd::Utilities::tileBoundingBox31(tileId, zoom);
    const auto mapSymbolsGroups = buildMapSymbolsGroups(tileBBox31, req.mapState.metersPerPixel);
    outData.reset(new Data(tileId, zoom, mapSymbolsGroups));
    
    return true;
}

bool OsmAnd::GpxAdditionalIconsProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::GpxAdditionalIconsProvider::obtainDataAsync(const IMapDataProvider::Request& request,
                                                 const IMapDataProvider::ObtainDataAsyncCallback callback,
                                                 const bool collectMetric /*= false*/)
{
    OsmAnd::MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

OsmAnd::GpxAdditionalIconsProvider::Data::Data(const OsmAnd::TileId tileId_,
                                       const ZoomLevel zoom_,
                                       const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_,
                                       const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
: IMapTiledSymbolsProvider::Data(tileId_, zoom_, symbolsGroups_, pRetainableCacheMetadata_)
{
}

OsmAnd::GpxAdditionalIconsProvider::Data::~Data()
{
    release();
}

OsmAnd::GpxAdditionalIconsProvider::SplitLabel::SplitLabel(
    const PointI& pos31_,
    const QString& text_,
    const ColorARGB& gpxColor_)
    : pos31(pos31_)
    , text(text_)
    , gpxColor(gpxColor_)
{
}

OsmAnd::GpxAdditionalIconsProvider::SplitLabel::~SplitLabel()
{
}
