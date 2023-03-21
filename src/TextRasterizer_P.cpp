#include "TextRasterizer_P.h"
#include "TextRasterizer.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkImage.h>
#include <SkTypeface.h>
#include <SkTextBlob.h>
#include <SkUtils.h>
#include <SkFontMetrics.h>
#include "restore_internal_warnings.h"

#include "ICU.h"
#include "CoreResourcesEmbeddedBundle.h"

//#define OSMAND_LOG_CHARACTERS_WITHOUT_GLYPHS 1
#ifndef OSMAND_LOG_CHARACTERS_WITHOUT_GLYPHS
#   define OSMAND_LOG_CHARACTERS_WITHOUT_GLYPHS 0
#endif // !defined(OSMAND_LOG_CHARACTERS_WITHOUT_GLYPHS)

// #define OSMAND_LOG_CHARACTERS_TYPEFACE 1
#ifndef OSMAND_LOG_CHARACTERS_TYPEFACE
#   define OSMAND_LOG_CHARACTERS_TYPEFACE 0
#endif // !defined(OSMAND_LOG_CHARACTERS_TYPEFACE)

OsmAnd::TextRasterizer_P::TextRasterizer_P(TextRasterizer* const owner_)
    : owner(owner_)
{
    _defaultPaint.setAntiAlias(true);
}

OsmAnd::TextRasterizer_P::~TextRasterizer_P()
{
}

QVector<OsmAnd::TextRasterizer_P::LinePaint> OsmAnd::TextRasterizer_P::evaluatePaints(
    const QVector<QStringRef>& lineRefs,
    const Style& style) const
{
    // Prepare default paint
    SkPaint paint = _defaultPaint;
    paint.setColor(style.color.toSkColor());

    // Prepare default font
    SkFont skFont = _defaultFont;
    skFont.setSize(style.size);

    // NOTE: It's not optimal to create a shared HB font w/o typeface in order to hold only scale
    // NOTE: See hb_ot_font_set_funcs() implementation
    const auto hbFontScale = qRound(style.size * HB_FONT_SCALE_FACTOR);

    // Transform text style to font style
    const SkFontStyle fontStyle(
        style.bold ? SkFontStyle::kBold_Weight : SkFontStyle::kNormal_Weight,
        SkFontStyle::kNormal_Width,
        style.italic ? SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant);

    const auto pText = lineRefs.first().string()->constData();

    QVector<LinePaint> linePaints;
    linePaints.reserve(lineRefs.count());
    for (const auto& lineRef : constOf(lineRefs))
    {
        LinePaint linePaint;
        linePaint.line = lineRef;

        std::shared_ptr<const ITypefaceFinder::Typeface> typeface;
        TextPaint* pTextPaint = nullptr;
        const auto pLine = lineRef.constData();
        const auto pEnd = pLine + lineRef.size();
        auto pNextCharacter = pLine;
        while (pNextCharacter != pEnd)
        {
            const auto position = pNextCharacter - pText;
            const auto characterUCS4 = SkUTF16_NextUnichar(reinterpret_cast<const uint16_t**>(&pNextCharacter));
            
            if (typeface)
            {
                if (typeface->skTypeface->unicharToGlyph(characterUCS4) == 0)
                    typeface = nullptr;
#if OSMAND_LOG_CHARACTERS_TYPEFACE
                else
                {
                    SkString typefaceName;
                    typeface->skTypeface->getFamilyName(&typefaceName);

                    LogPrintf(LogSeverityLevel::Warning,
                        "UCS4 character 0x%08x (%u) has been found in '%s' typeface (reused)",
                        characterUCS4,
                        characterUCS4,
                        typefaceName.c_str());
                }
#endif // OSMAND_LOG_CHARACTERS_TYPEFACE
            }
            if (!typeface)
            {
                typeface = owner->typefaceFinder->findTypefaceForCharacterUCS4(characterUCS4, fontStyle);

#if OSMAND_LOG_CHARACTERS_WITHOUT_GLYPHS
                if (!typeface)
                {
                    LogPrintf(LogSeverityLevel::Warning,
                        "UCS4 character 0x%08x (%u) has not been found in any typeface",
                        characterUCS4,
                        characterUCS4);
                }
#endif // OSMAND_LOG_CHARACTERS_WITHOUT_GLYPHS

#if OSMAND_LOG_CHARACTERS_TYPEFACE
                if (typeface)
                {
                    SkString typefaceName;
                    typeface->skTypeface->getFamilyName(&typefaceName);

                    LogPrintf(LogSeverityLevel::Warning,
                        "UCS4 character 0x%08x (%u) has been found in '%s' font",
                        characterUCS4,
                        characterUCS4,
                        typefaceName.c_str());
                }
#endif // OSMAND_LOG_CHARACTERS_TYPEFACE
            }
            if (!typeface)
            {
                 continue;
            }

            if (pTextPaint == nullptr || pTextPaint->typeface != typeface)
            {
                linePaint.textPaints.push_back(TextPaint());
                pTextPaint = &linePaint.textPaints.last();

                pTextPaint->text = QStringRef(lineRef.string(), position, 1);
                pTextPaint->paint = paint;
                pTextPaint->typeface = typeface;
                pTextPaint->skFont = skFont;
                pTextPaint->skFont.setTypeface(typeface->skTypeface);
                pTextPaint->hbFont = std::shared_ptr<hb_font_t>(
                    hb_font_create(typeface->hbFace.get()),
                    hb_font_destroy
                );
                hb_font_set_scale(pTextPaint->hbFont.get(), hbFontScale, hbFontScale);

                SkFontMetrics metrics;
                pTextPaint->skFont.getMetrics(&metrics);
                pTextPaint->height = pTextPaint->skFont.getSize() * 1.2f;
                linePaint.maxFontHeight = qMax(linePaint.maxFontHeight, pTextPaint->height);
                linePaint.minFontHeight = qMin(linePaint.minFontHeight, pTextPaint->height);
                linePaint.maxFontLineSpacing = qMax(linePaint.maxFontLineSpacing, metrics.fLeading);
                linePaint.minFontLineSpacing = qMin(linePaint.minFontLineSpacing, metrics.fLeading);
                linePaint.maxFontTop = qMax(linePaint.maxFontTop, -metrics.fTop);
                linePaint.minFontTop = qMin(linePaint.minFontTop, -metrics.fTop);
                linePaint.maxFontBottom = qMax(linePaint.maxFontBottom, metrics.fBottom);
                linePaint.minFontBottom = qMin(linePaint.minFontBottom, metrics.fBottom);
                linePaint.fontAscent = metrics.fAscent;

                if (style.bold && typeface->skTypeface->fontStyle().weight() <= SkFontStyle::kNormal_Weight)
                    pTextPaint->skFont.setEmbolden(true);
            }
            else
            {
                pTextPaint->text = QStringRef(lineRef.string(), pTextPaint->text.position(), pTextPaint->text.size() + 1);
            }
        }

        linePaints.push_back(qMove(linePaint));
    }

    return linePaints;
}

void OsmAnd::TextRasterizer_P::measureText(QVector<LinePaint>& paints, SkScalar& outMaxLineWidth) const
{
    outMaxLineWidth = 0;

    for (auto& linePaint : paints)
    {
        linePaint.maxBoundsTop = 0;
        linePaint.minBoundsTop = std::numeric_limits<SkScalar>::max();
        linePaint.width = 0;

        for (auto& textPaint : linePaint.textPaints)
        {
            textPaint.skFont.measureText(
                textPaint.text.constData(), textPaint.text.length()*sizeof(QChar), SkTextEncoding::kUTF16,
                &textPaint.bounds);

            textPaint.width = textPaint.bounds.width();

            linePaint.maxBoundsTop = qMax(linePaint.maxBoundsTop, -textPaint.bounds.top());
            linePaint.minBoundsTop = qMin(linePaint.minBoundsTop, -textPaint.bounds.top());
            linePaint.width += textPaint.width;
        }

        outMaxLineWidth = qMax(outMaxLineWidth, linePaint.width);
    }
}

void OsmAnd::TextRasterizer_P::measureGlyphs(const QVector<LinePaint>& paints, QVector<SkScalar>& outGlyphWidths) const
{
    for (const auto& linePaint : constOf(paints))
    {
        for (const auto& textPaint : constOf(linePaint.textPaints))
        {
            const auto glyphsCount = textPaint.skFont.countText(
                textPaint.text.constData(), textPaint.text.length()*sizeof(QChar), SkTextEncoding::kUTF16
            );
            if (glyphsCount < 0)
                continue;

            QVector<SkGlyphID> glyphs(glyphsCount);
            textPaint.skFont.textToGlyphs(
                textPaint.text.constData(), textPaint.text.length()*sizeof(QChar), SkTextEncoding::kUTF16,
                glyphs.data(), glyphsCount
            );

            const auto previousSize = outGlyphWidths.size();
            outGlyphWidths.resize(previousSize + glyphsCount);
            const auto pWidth = outGlyphWidths.data() + previousSize;
            textPaint.skFont.getWidths(
                glyphs.constData(), glyphsCount,
                pWidth);

            // Remove excessive width from first glyph
            *pWidth += -textPaint.bounds.left();

            // Remove excessive width from last glyph
            const auto pWidthEnd = pWidth + glyphsCount;
            const auto glyphsWidth = std::accumulate(pWidth, pWidthEnd, 0.0f);
            const auto pLastWidth = pWidthEnd - 1;
            *pLastWidth -= qMax(0.0f, glyphsWidth - textPaint.bounds.width());

            ///////
            /*
            const float totalWidth = textPaint.paint.measureText(
                textPaint.text.constData(),
                textPaint.text.length()*sizeof(QChar));

            auto widthsSum = 0.0f;
            for (int idx = 0; idx < glyphsCount; idx++)
                widthsSum += pWidth[idx];

            if (widthsSum > totalWidth)
            {
                LogPrintf(LogSeverityLevel::Error, "totalWidth = %f, widthsSum = %f", totalWidth, widthsSum);
                int i = 5;
            }
            */
            ////////
        }
    }
}

OsmAnd::TextRasterizer_P::TextPaint OsmAnd::TextRasterizer_P::getHaloTextPaint(
    const TextPaint& textPaint,
    const Style& style) const
{
    auto haloTextPaint = textPaint;

    haloTextPaint.paint.setStyle(SkPaint::kStroke_Style);
    haloTextPaint.paint.setColor(style.haloColor.toSkColor());
    haloTextPaint.paint.setStrokeWidth(style.haloRadius);

    return haloTextPaint;
}

void OsmAnd::TextRasterizer_P::measureHalo(const Style& style, QVector<LinePaint>& paints) const
{
    for (auto& linePaint : paints)
    {
        linePaint.maxBoundsTop = 0;
        linePaint.minBoundsTop = std::numeric_limits<SkScalar>::max();

        for (auto& textPaint : linePaint.textPaints)
        {
            const auto haloTextPaint = getHaloTextPaint(textPaint, style);
            /*
            SkPaint::FontMetrics metrics;
            textPaint.height = haloPaint.getFontMetrics(&metrics);
            linePaint.maxFontHeight = qMax(linePaint.maxFontHeight, textPaint.height);
            linePaint.minFontHeight = qMin(linePaint.minFontHeight, textPaint.height);
            linePaint.maxFontLineSpacing = qMax(linePaint.maxFontLineSpacing, metrics.fLeading);
            linePaint.minFontLineSpacing = qMin(linePaint.minFontLineSpacing, metrics.fLeading);
            linePaint.maxFontTop = qMax(linePaint.maxFontTop, -metrics.fTop);
            linePaint.minFontTop = qMin(linePaint.minFontTop, -metrics.fTop);
            linePaint.maxFontBottom = qMax(linePaint.maxFontBottom, metrics.fBottom);
            linePaint.minFontBottom = qMin(linePaint.minFontBottom, metrics.fBottom);
            */
            SkRect haloBounds;
            haloTextPaint.skFont.measureText(
                haloTextPaint.text.constData(), haloTextPaint.text.length()*sizeof(QChar), SkTextEncoding::kUTF16,
                &haloBounds);
            haloBounds.inset(-(float)style.haloRadius, -(float)style.haloRadius);
            textPaint.bounds.join(haloBounds);

            linePaint.maxBoundsTop = qMax(linePaint.maxBoundsTop, -textPaint.bounds.top());
            linePaint.minBoundsTop = qMin(linePaint.minBoundsTop, -textPaint.bounds.top());
        }
    }
}

void OsmAnd::TextRasterizer_P::measureHaloGlyphs(
    const Style& style,
    const QVector<LinePaint>& paints,
    QVector<SkScalar>& outGlyphWidths) const
{
    for (const auto& linePaint : constOf(paints))
    {
        for (const auto& textPaint : constOf(linePaint.textPaints))
        {
            const auto haloTextPaint = getHaloTextPaint(textPaint, style);

            const auto glyphsCount = haloTextPaint.skFont.countText(
                haloTextPaint.text.constData(), haloTextPaint.text.length()*sizeof(QChar), SkTextEncoding::kUTF16
            );
            if (glyphsCount < 0)
                continue;
            
            QVector<SkGlyphID> glyphs(glyphsCount);
            haloTextPaint.skFont.textToGlyphs(
                haloTextPaint.text.constData(), haloTextPaint.text.length()*sizeof(QChar), SkTextEncoding::kUTF16,
                glyphs.data(), glyphsCount
            );

            const auto previousSize = outGlyphWidths.size();
            outGlyphWidths.resize(previousSize + glyphsCount);
            const auto pWidth = outGlyphWidths.data() + previousSize;
            haloTextPaint.skFont.getWidths(
                glyphs.constData(), glyphsCount,
                pWidth);

            // Remove excessive width from first glyph
            *pWidth += -haloTextPaint.bounds.left();

            // Remove excessive width from last glyph
            const auto pWidthEnd = pWidth + glyphsCount;
            const auto glyphsWidth = std::accumulate(pWidth, pWidthEnd, 0.0f);
            const auto pLastWidth = pWidthEnd - 1;
            *pLastWidth -= qMax(0.0f, glyphsWidth - textPaint.bounds.width());
        }
    }
}

SkRect OsmAnd::TextRasterizer_P::positionText(
    QVector<LinePaint>& paints,
    const SkScalar maxLineWidth,
    const Style::TextAlignment textAlignment) const
{
    auto textArea = SkRect::MakeEmpty();

    SkScalar verticalOffset = 0;
    for (auto& linePaint : paints)
    {
        SkScalar horizontalOffset = 0;
        const auto widthDelta = maxLineWidth - linePaint.width;
        switch (textAlignment)
        {
            case Style::TextAlignment::Center:
                horizontalOffset += widthDelta / 2.0f;
                break;

            case Style::TextAlignment::Right:
                horizontalOffset += widthDelta;
                break;

            case Style::TextAlignment::Left:
            default:
                // Do nothing here
                break;
        }

        for (auto& textPaint : linePaint.textPaints)
        {
            textPaint.positionedBounds = textPaint.bounds;

            // Position horizontally
            textPaint.positionedBounds.offset(-2.0f*textPaint.bounds.left(), 0);
            textPaint.positionedBounds.offset(horizontalOffset, 0);
            horizontalOffset += textPaint.width;

            // Position vertically
            if(verticalOffset == 0)
            {
                textPaint.positionedBounds.offset(0, -2.0f*textPaint.bounds.top());
                verticalOffset -= textPaint.bounds.top();
            }
            else
            {
                textPaint.positionedBounds.offset(0, -textPaint.bounds.top());
                textPaint.positionedBounds.offset(0, verticalOffset);
                textPaint.positionedBounds.fBottom += textPaint.bounds.fBottom;
            }
            textPaint.positionedBounds.offset(0, linePaint.maxBoundsTop + textPaint.bounds.top());

            // Include into text area
            textArea.join(textPaint.positionedBounds);
        }

        verticalOffset += linePaint.maxFontHeight;

        //// Calculate text area and move bounds vertically
        //auto textArea = linesNormalizedBounds.first();
        //auto linesHeightSum = textArea.height();
        //auto citPrevLineBounds = linesBounds.cbegin();
        //auto citLineBounds = citPrevLineBounds + 1;
        //for (auto itNormalizedLineBounds = linesNormalizedBounds.begin() + 1, itEnd = linesNormalizedBounds.end();
        //    itNormalizedLineBounds != itEnd;
        //    ++itNormalizedLineBounds, citPrevLineBounds = citLineBounds, ++citLineBounds)
        //{
        //    auto& lineNormalizedBounds = *itNormalizedLineBounds;
        //    const auto& prevLineBounds = *citPrevLineBounds;
        //    const auto& lineBounds = *citLineBounds;

        //    // Include gap between previous line and it's font-end
        //    const auto extraPrevGapHeight = qMax(0.0f, fontMaxBottom - prevLineBounds.fBottom);
        //    textArea.fBottom += extraPrevGapHeight;
        //    linesHeightSum += extraPrevGapHeight;

        //    // Include line spacing
        //    textArea.fBottom += lineSpacing;
        //    linesHeightSum += lineSpacing;

        //    // Include gap between current line and it's font-start
        //    const auto extraGapHeight = qMax(0.0f, fontMaxTop - (-lineBounds.fTop));
        //    textArea.fBottom += extraGapHeight;
        //    linesHeightSum += extraGapHeight;

        //    // Move current line baseline
        //    lineNormalizedBounds.offset(0.0f, linesHeightSum);

        //    // Include height of current line
        //    const auto& lineHeight = lineNormalizedBounds.height();
        //    textArea.fBottom += lineHeight;
        //    linesHeightSum += lineHeight;

        //    // This will expand left-right bounds to get proper area width
        //    textArea.fLeft = qMin(textArea.fLeft, lineNormalizedBounds.fLeft);
        //    textArea.fRight = qMax(textArea.fRight, lineNormalizedBounds.fRight);
        //}

    }

    return textArea;
}

sk_sp<SkImage> OsmAnd::TextRasterizer_P::rasterize(
    const QString& text,
    const Style& style,
    QVector<SkScalar>* const outGlyphWidths,
    float* const outExtraTopSpace,
    float* const outExtraBottomSpace,
    float* const outLineSpacing,
    float* const outFontAscent) const
{
    SkBitmap target;
    const bool ok = rasterize(
        target,
        text,
        style,
        outGlyphWidths,
        outExtraTopSpace,
        outExtraBottomSpace,
        outLineSpacing,
        outFontAscent);
    if (!ok)
        return nullptr;
    return target.asImage();
}

bool OsmAnd::TextRasterizer_P::drawText(SkCanvas& canvas, const TextPaint& textPaint) const
{
    const auto text = textPaint.text.toString();
    const auto rightToLeft = ICU::isRightToLeft(text);
    if (text.isEmpty())
        return true;

    const auto pHbBuffer = hb_buffer_create();
    if (pHbBuffer == hb_buffer_get_empty())
    {
        return false;
    }
    std::shared_ptr<hb_buffer_t> hbBuffer(pHbBuffer, hb_buffer_destroy);
    if (!hb_buffer_allocation_successful(hbBuffer.get()))
    {
        return false;
    }

    hb_buffer_add_utf16(hbBuffer.get(), reinterpret_cast<const uint16_t*>(text.constData()), text.size(), 0, -1);
    hb_buffer_set_direction(hbBuffer.get(), rightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    hb_buffer_guess_segment_properties(hbBuffer.get());

    hb_shape(textPaint.hbFont.get(), hbBuffer.get(), nullptr, 0);

    const auto glyphsCount = hb_buffer_get_length(hbBuffer.get());
    if (!glyphsCount)
    {
        return false;
    }
    const auto pGlyphInfos = hb_buffer_get_glyph_infos(hbBuffer.get(), nullptr);
    const auto pGlyphPositions = hb_buffer_get_glyph_positions(hbBuffer.get(), nullptr);

    SkTextBlobBuilder textBlobBuilder;
    auto origin = SkPoint::Make(0.0f, 0.0f);
    const auto textBlobRunBuffer = textBlobBuilder.allocRunPos(textPaint.skFont, glyphsCount);
    for (auto glyphIdx = 0u; glyphIdx < glyphsCount; glyphIdx++)
    {
        auto codepoint = pGlyphInfos[glyphIdx].codepoint;

        const auto citReplacementCodepoint = textPaint.typeface->replacementCodepoints.find(codepoint);
        if (citReplacementCodepoint != textPaint.typeface->replacementCodepoints.cend())
        {
            codepoint = citReplacementCodepoint->second;
        }

        textBlobRunBuffer.glyphs[glyphIdx] = codepoint;

        const auto advance = SkPoint::Make(
            static_cast<float>(pGlyphPositions[glyphIdx].x_advance) / HB_FONT_SCALE_FACTOR,
            static_cast<float>(pGlyphPositions[glyphIdx].y_advance) / HB_FONT_SCALE_FACTOR
        );
        const auto offset = SkPoint::Make(
            static_cast<float>(pGlyphPositions[glyphIdx].x_offset) / HB_FONT_SCALE_FACTOR,
            -static_cast<float>(pGlyphPositions[glyphIdx].y_offset) / HB_FONT_SCALE_FACTOR
        );
        textBlobRunBuffer.points()[glyphIdx] = origin + offset;
        origin += advance;
    }
    canvas.drawTextBlob(
        textBlobBuilder.make(),
        textPaint.positionedBounds.left(),
        textPaint.positionedBounds.top(),
        textPaint.paint
    );

    return true;
}

bool OsmAnd::TextRasterizer_P::rasterize(
    SkBitmap& targetBitmap,
    const QString& text,
    const Style& style,
    QVector<SkScalar>* const outGlyphWidths,
    float* const outExtraTopSpace,
    float* const outExtraBottomSpace,
    float* const outLineSpacing,
    float* const outFontAscent) const
{
    // Prepare text and break by lines
    const auto lineRefs = style.wrapWidth > 0
        ? ICU::getTextWrappingRefs(text, style.wrapWidth, style.maxLines)
        : (QVector<QStringRef>() << QStringRef(&text));

    // Obtain paints from lines and style
    auto paints = evaluatePaints(lineRefs, style);

    // Measure text
    SkScalar maxLineWidthInPixels = 0;
    measureText(paints, maxLineWidthInPixels);

    // Measure glyphs (if requested and there's no halo)
    if (outGlyphWidths && style.haloRadius == 0)
    {
        measureGlyphs(paints, *outGlyphWidths);
    }

    // Process halo if exists
    if (style.haloRadius > 0)
    {
        measureHalo(style, paints);

        if (outGlyphWidths)
        {
            measureHaloGlyphs(style, paints, *outGlyphWidths);
        }
    }

    // Set output font ascent
    if (outFontAscent)
    {
        float fontAscent = 0.0f;
        for (const auto& linePaint : constOf(paints))
        {
            fontAscent = qMin(fontAscent, linePaint.fontAscent);
        }

        *outFontAscent = fontAscent;
    }

    // Set output line spacing
    if (outLineSpacing)
    {
        float lineSpacing = 0.0f;
        for (const auto& linePaint : constOf(paints))
        {
            lineSpacing = qMax(lineSpacing, linePaint.maxFontLineSpacing);
        }

        *outLineSpacing = lineSpacing;
    }

    // Calculate extra top and bottom space
    if (outExtraTopSpace)
    {
        SkScalar maxTop = 0;
        for (const auto& linePaint : constOf(paints))
        {
            maxTop = qMax(maxTop, linePaint.maxFontTop);
        }

        *outExtraTopSpace = qMax(0.0f, maxTop - paints.first().maxFontTop);
    }
    if (outExtraBottomSpace)
    {
        SkScalar maxBottom = 0;
        for (const auto& linePaint : constOf(paints))
        {
            maxBottom = qMax(maxBottom, linePaint.maxFontBottom);
        }

        *outExtraBottomSpace = qMax(0.0f, maxBottom - paints.last().maxFontBottom);
    }

    // Position text horizontally and vertically
    const auto textArea = positionText(paints, maxLineWidthInPixels, style.textAlignment);

    // Calculate bitmap size
    auto bitmapWidth = qCeil(textArea.width());
    auto bitmapHeight = qCeil(textArea.height());
    if (style.backgroundImage)
    {
        // Clear extra spacing
        if (outExtraTopSpace)
            *outExtraTopSpace = 0.0f;
        if (outExtraBottomSpace)
            *outExtraBottomSpace = 0.0f;

        // Enlarge bitmap if shield is larger than text
        bitmapWidth = qMax(bitmapWidth, style.backgroundImage->width());
        bitmapHeight = qMax(bitmapHeight, style.backgroundImage->height());

        // Shift text area to proper position in a larger
        const auto offset = SkPoint::Make(
            (bitmapWidth - qCeil(textArea.width())) / 2.0f,
            (bitmapHeight - qCeil(textArea.height())) / 2.0f);
        for (auto& linePaint : paints)
        {
            for (auto& textPaint : linePaint.textPaints)
            {
                textPaint.positionedBounds.offset(offset);
            }
        }
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
        if (!targetBitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(bitmapWidth, bitmapHeight)))
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to allocate bitmap of size %dx%d",
                qPrintable(text),
                bitmapWidth,
                bitmapHeight);
            return false;
        }

        targetBitmap.eraseColor(SK_ColorTRANSPARENT);
    }
    SkCanvas canvas(targetBitmap);

    // If there is background this text, rasterize it also
    if (style.backgroundImage)
    {
        canvas.drawImage(style.backgroundImage.get(),
            (bitmapWidth - style.backgroundImage->width()) / 2.0f,
            (bitmapHeight - style.backgroundImage->height()) / 2.0f
        );
    }

    // Enable to draw background for each glyph
    bool visualizeGlyphWidths = false;
    if (outGlyphWidths && visualizeGlyphWidths)
    {
        SkPaint glyphPaint;
        auto start = 0.0f;
        auto i = 0;
        for (auto& width : *outGlyphWidths)
        {
            const auto left = start;
            const auto right = start + width;
            const auto glyphRect = SkRect::MakeLTRB(left, 0, right, bitmapHeight);

            glyphPaint.setColor(i++ % 2 == 0 ? SK_ColorGREEN : SK_ColorRED);
            canvas.drawRect(glyphRect, glyphPaint);
            
            start = right;
        }
    }

    bool success = true;

    // Rasterize text halo first (if enabled)
    if (style.haloRadius > 0)
    {
        for (const auto& linePaint : paints)
        {
            for (const auto& textPaint : linePaint.textPaints)
            {
                const auto haloTextPaint = getHaloTextPaint(textPaint, style);

                if (!drawText(canvas, haloTextPaint))
                {
                    success = false;
                }
            }
        }
    }

    // Rasterize text itself
    for (const auto& linePaint : paints)
    {
        for (const auto& textPaint : linePaint.textPaints)
        {
            if (!drawText(canvas, textPaint))
            {
                success = false;
            }
        }
    }

    canvas.flush();

    return success;
}
