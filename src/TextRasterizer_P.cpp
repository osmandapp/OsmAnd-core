#include "TextRasterizer_P.h"
#include "TextRasterizer.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmapDevice.h>
#include <SkStream.h>
#include <SkTypeface.h>
#include "restore_internal_warnings.h"

#include "ICU.h"
#include "EmbeddedResources.h"

OsmAnd::TextRasterizer_P::TextRasterizer_P(TextRasterizer* const owner_)
    : owner(owner_)
{
}

OsmAnd::TextRasterizer_P::~TextRasterizer_P()
{
}

void OsmAnd::TextRasterizer_P::initialize()
{
    _defaultPaint.setAntiAlias(true);
    _defaultPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    static_assert(sizeof(QChar) == 2, "If QChar is not 2 bytes, then encoding is not kUTF16_TextEncoding");

    // Fonts register (in priority order):
    _fontsRegistry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Italic.ttf"), false, true });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Semibold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-SemiboldItalic.ttf"), true, true });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansEthiopic-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansEthiopic-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansArabic.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansHebrew-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansHebrew-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansThai-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansThai-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansArmenian.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansGeorgian.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansDevanagari-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansDevanagari-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansTamil-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansTamil-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansMalayalam.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansMalayalam-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansBengali-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansBengali-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansTelugu-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansTelugu-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansKannada-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansKannada-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansKhmer-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansKhmer-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansLao-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoSansLao-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansFallback.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/MTLmr3m.ttf"), false, false });
}

void OsmAnd::TextRasterizer_P::clearFontsCache()
{
    QMutexLocker scopedLocker(&_fontTypefacesCacheMutex);

    for (const auto& typeface : _fontTypefacesCache)
        typeface->unref();
    _fontTypefacesCache.clear();
}

SkTypeface* OsmAnd::TextRasterizer_P::getTypefaceForFontResource(const QString& fontResource) const
{
    QMutexLocker scopedLocker(&_fontTypefacesCacheMutex);

    // Look for loaded typefaces
    const auto& citTypeface = _fontTypefacesCache.constFind(fontResource);
    if (citTypeface != _fontTypefacesCache.cend())
        return *citTypeface;

    // Load raw data from resource
    const auto fontData = EmbeddedResources::decompressResource(fontResource);
    if (fontData.isNull())
        return nullptr;

    // Load typeface from font data
    const auto fontDataStream = new SkMemoryStream(fontData.constData(), fontData.length(), true);
    const auto typeface = SkTypeface::CreateFromStream(fontDataStream);
    fontDataStream->unref();
    if (!typeface)
        return nullptr;

    _fontTypefacesCache.insert(fontResource, typeface);

    return typeface;
}

void OsmAnd::TextRasterizer_P::configurePaintForText(SkPaint& paint, const QString& text, const bool bold, const bool italic) const
{
    paint = _defaultPaint;

    // Lookup matching font entry
    const FontsRegistryEntry* pBestMatchEntry = nullptr;
    SkTypeface* bestMatchTypeface = nullptr;
    SkPaint textCoverageTestPaint;
    textCoverageTestPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    for (const auto& entry : constOf(_fontsRegistry))
    {
        // Get typeface for this entry
        const auto typeface = getTypefaceForFontResource(entry.resource);
        if (!typeface)
            continue;

        // Check if this typeface covers provided text
        textCoverageTestPaint.setTypeface(typeface);
        if (!textCoverageTestPaint.containsText(text.constData(), text.length()*sizeof(QChar)))
            continue;

        // Mark this as best match
        pBestMatchEntry = &entry;
        bestMatchTypeface = typeface;

        // If this entry fully matches the request, stop search
        if (entry.bold == bold && entry.italic == italic)
            break;
    }

    // If there's no best match, fallback to default typeface
    if (bestMatchTypeface == nullptr)
    {
        paint.setTypeface(nullptr);

        // Adjust to handle bold text
        if (bold)
            paint.setFakeBoldText(true);

        return;
    }

    // There was a best match, use that
    paint.setTypeface(bestMatchTypeface);

    // Adjust to handle bold text
    if (pBestMatchEntry->bold != bold && bold)
        paint.setFakeBoldText(true);
}

std::shared_ptr<SkBitmap> OsmAnd::TextRasterizer_P::rasterize(
    const QString& text,
    const Style& style,
    QVector<SkScalar>* const outGlyphWidths,
    float* const outExtraTopSpace,
    float* const outExtraBottomSpace,
    float* const outLineSpacing) const
{
    std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    rasterize(*bitmap, text, style, outGlyphWidths, outExtraTopSpace, outExtraBottomSpace, outLineSpacing);
    return bitmap;
}

void OsmAnd::TextRasterizer_P::rasterize(
    SkBitmap& targetBitmap,
    const QString& text_,
    const Style& style,
    QVector<SkScalar>* const outGlyphWidths,
    float* const outExtraTopSpace,
    float* const outExtraBottomSpace,
    float* const outLineSpacing) const
{
    const auto text = ICU::convertToVisualOrder(text_);
    const auto lineRefs =
        style.wrapWidth > 0
        ? ICU::getTextWrappingRefs(text, style.wrapWidth)
        : (QVector<QStringRef>() << QStringRef(&text));
    const auto& linesCount = lineRefs.size();

    // Get base text settings from environment
    SkPaint paint = _defaultPaint;

    // Configure paint for text
    configurePaintForText(paint, text, style.bold, style.italic);
    paint.setTextSize(style.size);
    paint.setColor(style.color);

    // Get line spacing
    SkPaint::FontMetrics fontMetrics;
    auto fullLineHeight = paint.getFontMetrics(&fontMetrics);
    auto lineSpacing = fontMetrics.fLeading;
    auto fontMaxTop = -fontMetrics.fTop;
    auto fontMaxBottom = fontMetrics.fBottom;

    // Measure text
    QVector<SkRect> linesBounds(linesCount);
    auto itLineBounds = linesBounds.begin();
    for (const auto& lineRef : constOf(lineRefs))
        paint.measureText(lineRef.constData(), lineRef.length()*sizeof(QChar), &*(itLineBounds++));

    // Measure glyphs
    if (outGlyphWidths)
    {
        const auto& lineRef = lineRefs.first();

        const auto glyphsCount = paint.countText(lineRef.constData(), lineRef.length()*sizeof(QChar));
        outGlyphWidths->resize(glyphsCount);
        paint.getTextWidths(lineRef.constData(), lineRef.length()*sizeof(QChar), outGlyphWidths->data());
    }

    // Process halo
    SkPaint haloPaint;
    if (style.haloRadius > 0)
    {
        // Configure paint for halo
        haloPaint = paint;
        haloPaint.setStyle(SkPaint::kStroke_Style);
        haloPaint.setColor(style.haloColor);
        haloPaint.setStrokeWidth(style.haloRadius);
        
        // Get line spacing
        SkPaint::FontMetrics shadowFontMetrics;
        const auto fullShadowLineHeight = haloPaint.getFontMetrics(&shadowFontMetrics);
        fullLineHeight = qMax(fullLineHeight, fullShadowLineHeight);
        lineSpacing = qMax(lineSpacing, shadowFontMetrics.fLeading);
        fontMaxTop = qMax(fontMaxTop, -shadowFontMetrics.fTop);
        fontMaxBottom = qMax(fontMaxBottom, shadowFontMetrics.fBottom);

        // Measure text shadow bounds
        auto itLineBounds = linesBounds.begin();
        for (const auto& lineRef : constOf(lineRefs))
        {
            SkRect lineShadowBounds;
            haloPaint.measureText(lineRef.constData(), lineRef.length()*sizeof(QChar), &lineShadowBounds);

            // Combine shadow bounds with text bounds
            (itLineBounds++)->join(lineShadowBounds);
        }

        // Measure glyphs, since halo is larger than text itself
        if (outGlyphWidths)
        {
            const auto& lineRef = lineRefs.first();

            haloPaint.getTextWidths(lineRef.constData(), lineRef.length()*sizeof(QChar), outGlyphWidths->data());
        }
    }

    // Set line spacing
    if (outLineSpacing)
        *outLineSpacing = lineSpacing;

    // Calculate extra top and bottom space
    if (outExtraTopSpace)
        *outExtraTopSpace = qMax(0.0f, fontMaxTop - (-linesBounds.first().fTop));
    if (outExtraBottomSpace)
        *outExtraBottomSpace = qMax(0.0f, fontMaxBottom - linesBounds.last().fBottom);

    // Shift first glyph width
    if (outGlyphWidths && !outGlyphWidths->isEmpty())
        outGlyphWidths->first() += -linesBounds.first().left();

    // Normalize line bounds (move origin top bottom-left corner of bitmap)
    QVector<SkRect> linesNormalizedBounds(linesCount);
    auto itNormalizedLineBounds = linesNormalizedBounds.begin();
    for (auto& lineBounds : linesBounds)
    {
        auto& normalizedLineBounds = *(itNormalizedLineBounds++);

        normalizedLineBounds = lineBounds;
        normalizedLineBounds.offset(-2.0f*lineBounds.left(), -2.0f*lineBounds.top());
    }

    // Calculate text area and move bounds vertically
    auto textArea = linesNormalizedBounds.first();
    auto linesHeightSum = textArea.height();
    auto citPrevLineBounds = linesBounds.cbegin();
    auto citLineBounds = citPrevLineBounds + 1;
    for (auto itNormalizedLineBounds = linesNormalizedBounds.begin() + 1, itEnd = linesNormalizedBounds.end();
        itNormalizedLineBounds != itEnd;
        ++itNormalizedLineBounds, citPrevLineBounds = citLineBounds, ++citLineBounds)
    {
        auto& lineNormalizedBounds = *itNormalizedLineBounds;
        const auto& prevLineBounds = *citPrevLineBounds;
        const auto& lineBounds = *citLineBounds;

        // Include gap between previous line and it's font-end
        const auto extraPrevGapHeight = qMax(0.0f, fontMaxBottom - prevLineBounds.fBottom);
        textArea.fBottom += extraPrevGapHeight;
        linesHeightSum += extraPrevGapHeight;

        // Include line spacing
        textArea.fBottom += lineSpacing;
        linesHeightSum += lineSpacing;

        // Include gap between current line and it's font-start
        const auto extraGapHeight = qMax(0.0f, fontMaxTop - (-lineBounds.fTop));
        textArea.fBottom += extraGapHeight;
        linesHeightSum += extraGapHeight;

        // Move current line baseline
        lineNormalizedBounds.offset(0.0f, linesHeightSum);

        // Include height of current line
        const auto& lineHeight = lineNormalizedBounds.height();
        textArea.fBottom += lineHeight;
        linesHeightSum += lineHeight;

        // This will expand left-right bounds to get proper area width
        textArea.fLeft = qMin(textArea.fLeft, lineNormalizedBounds.fLeft);
        textArea.fRight = qMax(textArea.fRight, lineNormalizedBounds.fRight);
    }

    // Calculate bitmap size
    auto bitmapWidth = qCeil(textArea.width());
    auto bitmapHeight = qCeil(textArea.height());
    if (style.backgroundBitmap)
    {
        // Clear extra spacing
        if (outExtraTopSpace)
            *outExtraTopSpace = 0.0f;
        if (outExtraBottomSpace)
            *outExtraBottomSpace = 0.0f;

        // Enlarge bitmap if shield is larger than text
        bitmapWidth = qMax(bitmapWidth, style.backgroundBitmap->width());
        bitmapHeight = qMax(bitmapHeight, style.backgroundBitmap->height());

        // Shift text area to proper position in a larger
        auto& firstLineNormalizedBounds = linesNormalizedBounds.first();
        firstLineNormalizedBounds.offset(
            (bitmapWidth - qCeil(firstLineNormalizedBounds.width())) / 2.0f,
            (bitmapHeight - qCeil(firstLineNormalizedBounds.height())) / 2.0f);
    }

    // Check if bitmap size was successfully calculated
    if (bitmapWidth <= 0 || bitmapHeight <= 0)
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to rasterize text '%s'", qPrintable(text));
        return;
    }

    // Create a bitmap that will be hold entire symbol (if target is empty)
    if (targetBitmap.isNull())
    {
        targetBitmap.setConfig(SkBitmap::kARGB_8888_Config, bitmapWidth, bitmapHeight);
        targetBitmap.allocPixels();
        targetBitmap.eraseColor(SK_ColorTRANSPARENT);
    }
    SkBitmapDevice target(targetBitmap);
    SkCanvas canvas(&target);

    // If there is background this text, rasterize it also
    if (style.backgroundBitmap)
    {
        canvas.drawBitmap(*style.backgroundBitmap,
            (bitmapWidth - style.backgroundBitmap->width()) / 2.0f,
            (bitmapHeight - style.backgroundBitmap->height()) / 2.0f,
            nullptr);
    }

    // Rasterize text halo first (if enabled)
    if (style.haloRadius > 0)
    {
        auto itLineShadowNormalizedBounds = linesNormalizedBounds.cbegin();
        for (const auto& lineRef : constOf(lineRefs))
        {
            const auto& lineShadowNormalizedBounds = *(itLineShadowNormalizedBounds++);
            canvas.drawText(
                lineRef.constData(), lineRef.length()*sizeof(QChar),
                lineShadowNormalizedBounds.left(), lineShadowNormalizedBounds.top(),
                haloPaint);
        }
    }

    // Rasterize text itself
    auto citLineNormalizedBounds = linesNormalizedBounds.cbegin();
    for (const auto& lineRef : constOf(lineRefs))
    {
        const auto& lineNormalizedBounds = *(citLineNormalizedBounds++);
        canvas.drawText(
            lineRef.constData(), lineRef.length()*sizeof(QChar),
            lineNormalizedBounds.left(), lineNormalizedBounds.top(),
            paint);
    }

    canvas.flush();
}
