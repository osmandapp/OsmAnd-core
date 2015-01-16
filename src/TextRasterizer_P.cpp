#include "TextRasterizer_P.h"
#include "TextRasterizer.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmapDevice.h>
#include <SkTypeface.h>
#include "restore_internal_warnings.h"

#include "ICU.h"
#include "CoreResourcesEmbeddedBundle.h"

OsmAnd::TextRasterizer_P::TextRasterizer_P(TextRasterizer* const owner_)
    : owner(owner_)
{
    _defaultPaint.setAntiAlias(true);
    _defaultPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    static_assert(sizeof(QChar) == 2, "If QChar is not 2 bytes, then encoding is not kUTF16_TextEncoding");
}

OsmAnd::TextRasterizer_P::~TextRasterizer_P()
{
}

void OsmAnd::TextRasterizer_P::configurePaintForText(SkPaint& paint, const QString& text, const bool bold, const bool italic) const
{
    paint = _defaultPaint;

    // Find out what font is going to be used for specified text
    const auto fontName = owner->fontsCollection->findSuitableFont(text, bold, italic);
    const auto typeface = fontName.isEmpty() ? nullptr : owner->fontsCollection->obtainTypeface(fontName);
    if (fontName.isEmpty() || typeface == nullptr)
    {
        // No specific font was found, let SKIA decide what to do
        paint.setTypeface(nullptr);
        if (bold)
            paint.setFakeBoldText(true);
        return;
    }

    // Use found typeface
    paint.setTypeface(typeface);
    if ((typeface->style() & SkTypeface::kBold) != SkTypeface::kBold && bold)
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
    const bool ok = rasterize(
        *bitmap,
        text,
        style,
        outGlyphWidths,
        outExtraTopSpace,
        outExtraBottomSpace,
        outLineSpacing);
    if (!ok)
        return nullptr;
    return bitmap;
}

bool OsmAnd::TextRasterizer_P::rasterize(
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
    paint.setColor(style.color.toSkColor());

    // Get line spacing
    SkPaint::FontMetrics fontMetrics;
    auto fullLineHeight = paint.getFontMetrics(&fontMetrics);
    auto lineSpacing = fontMetrics.fLeading;
    auto fontMaxTop = -fontMetrics.fTop;
    auto fontMaxBottom = fontMetrics.fBottom;

    // Measure text
    SkScalar maxLineWidthInPixels = 0;
    QVector<SkRect> linesBounds(linesCount);
    auto pLineBounds = linesBounds.data();
    for (const auto& lineRef : constOf(lineRefs))
    {
        auto& lineBounds = *(pLineBounds++);

        paint.measureText(lineRef.constData(), lineRef.length()*sizeof(QChar), &lineBounds);

        const auto lineWidthInPixels = lineBounds.width();
        if (lineWidthInPixels > maxLineWidthInPixels)
            maxLineWidthInPixels = lineWidthInPixels;
    }

    // Measure glyphs
    if (outGlyphWidths)
    {
        // This is supported only for one-line text
        assert(lineRefs.size() == 1);
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
        haloPaint.setColor(style.haloColor.toSkColor());
        haloPaint.setStrokeWidth(style.haloRadius);
        
        // Get line spacing
        SkPaint::FontMetrics shadowFontMetrics;
        const auto fullShadowLineHeight = haloPaint.getFontMetrics(&shadowFontMetrics);
        fullLineHeight = qMax(fullLineHeight, fullShadowLineHeight);
        lineSpacing = qMax(lineSpacing, shadowFontMetrics.fLeading);
        fontMaxTop = qMax(fontMaxTop, -shadowFontMetrics.fTop);
        fontMaxBottom = qMax(fontMaxBottom, shadowFontMetrics.fBottom);

        // Measure text shadow bounds
        auto pLineBounds = linesBounds.data();
        for (const auto& lineRef : constOf(lineRefs))
        {
            auto& lineBounds = *(pLineBounds++);

            SkRect lineShadowBounds;
            haloPaint.measureText(lineRef.constData(), lineRef.length()*sizeof(QChar), &lineShadowBounds);

            const auto shadowLineWidthInPixels = lineShadowBounds.width();
            if (shadowLineWidthInPixels > maxLineWidthInPixels)
                maxLineWidthInPixels = shadowLineWidthInPixels;

            // Combine shadow bounds with text bounds
            lineBounds.join(lineShadowBounds);
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

    // Compute alignment offsets and apply them
    for (auto& lineBounds : linesBounds)
    {
        const auto widthDelta = maxLineWidthInPixels - lineBounds.width();

        switch (style.textAlignment)
        {
            case Style::TextAlignment::Center:
                lineBounds.offset(-widthDelta / 2.0f, 0);
                break;

            case Style::TextAlignment::Right:
                lineBounds.offset(-widthDelta, 0);
                break;

            case Style::TextAlignment::Left:
            default:
                // Do nothing here
                break;
        }
    }

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
        LogPrintf(LogSeverityLevel::Error,
            "Failed to rasterize text '%s': resulting bitmap size %dx%d is invalid",
            qPrintable(text),
            bitmapWidth,
            bitmapHeight);
        return false;
    }

    // Create a bitmap that will be hold entire symbol (if target is empty)
    if (targetBitmap.isNull())
    {
        if (!targetBitmap.tryAllocPixels(SkImageInfo::Make(
            bitmapWidth, bitmapHeight,
            SkColorType::kRGBA_8888_SkColorType,
            SkAlphaType::kUnpremul_SkAlphaType)))
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to allocate RGBA8888 bitmap of size %dx%d",
                qPrintable(text),
                bitmapWidth,
                bitmapHeight);
            return false;
        }

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
        auto pLineShadowNormalizedBounds = linesNormalizedBounds.constData();
        for (const auto& lineRef : constOf(lineRefs))
        {
            const auto& lineShadowNormalizedBounds = *(pLineShadowNormalizedBounds++);
            canvas.drawText(
                lineRef.constData(), lineRef.length()*sizeof(QChar),
                lineShadowNormalizedBounds.left(), lineShadowNormalizedBounds.top(),
                haloPaint);
        }
    }

    // Rasterize text itself
    auto pLineNormalizedBounds = linesNormalizedBounds.constData();
    for (const auto& lineRef : constOf(lineRefs))
    {
        const auto& lineNormalizedBounds = *(pLineNormalizedBounds++);
        canvas.drawText(
            lineRef.constData(), lineRef.length()*sizeof(QChar),
            lineNormalizedBounds.left(), lineNormalizedBounds.top(),
            paint);
    }

    canvas.flush();

    return true;
}
