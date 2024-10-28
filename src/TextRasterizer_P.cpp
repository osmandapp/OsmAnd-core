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
            auto npNextCharacter = pNextCharacter;
            const auto characterUCS4 = SkUTF16_NextUnichar(reinterpret_cast<const uint16_t**>(&npNextCharacter));
            if (npNextCharacter > pEnd)
                npNextCharacter = pEnd;
            const auto charSize = npNextCharacter - pNextCharacter;
            pNextCharacter = npNextCharacter;
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

                pTextPaint->text = QStringRef(lineRef.string(), position, charSize);
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
                pTextPaint->text =
                    QStringRef(lineRef.string(), pTextPaint->text.position(), pTextPaint->text.size() + charSize);
            }
        }

        linePaints.push_back(qMove(linePaint));
    }

    return linePaints;
}

OsmAnd::TextRasterizer_P::GlyphBlock OsmAnd::TextRasterizer_P::preparePart(const TextPaint& textPaint,
    const QString& text, bool rtl,const std::shared_ptr<hb_buffer_t>& hbBuffer, SkPoint& origin) const
{
    GlyphBlock glyphBlock;

    hb_buffer_reset(hbBuffer.get());
    hb_buffer_add_utf16(hbBuffer.get(), reinterpret_cast<const uint16_t*>(text.constData()), text.size(), 0, -1);
    hb_buffer_set_direction(hbBuffer.get(), rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    hb_buffer_guess_segment_properties(hbBuffer.get());

    hb_shape(textPaint.hbFont.get(), hbBuffer.get(), nullptr, 0);

    const auto glyphsCount = hb_buffer_get_length(hbBuffer.get());
    if (glyphsCount == 0)
        return glyphBlock;

    const auto pGlyphInfos = hb_buffer_get_glyph_infos(hbBuffer.get(), nullptr);
    const auto pGlyphPositions = hb_buffer_get_glyph_positions(hbBuffer.get(), nullptr);

    SkTextBlobBuilder textBlobBuilder;
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
        glyphBlock.codepoints.push_back(codepoint);

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
    glyphBlock.textBlob = textBlobBuilder.make();

    return glyphBlock;
}

bool OsmAnd::TextRasterizer_P::getGlyphBlocks(QVector<LinePaint>& paints, int& outMaxGlyphCount) const
{
    outMaxGlyphCount = 0;
    const auto pHbBuffer = hb_buffer_create();
    if (pHbBuffer == hb_buffer_get_empty())
        return false;
    const std::shared_ptr<hb_buffer_t> hbBuffer(pHbBuffer, hb_buffer_destroy);
    if (!hb_buffer_allocation_successful(hbBuffer.get()))
        return false;

    for (auto& linePaint : paints)
    {
        for (auto& textPaint : linePaint.textPaints)
        {
            auto text = textPaint.text.toString();
            if (text.isEmpty())
                continue;

            // Detect parts that have different text directions
            auto origin = SkPoint::Make(0.0f, 0.0f);
            auto direction = ICU::getTextDirection(text);
            if (direction != ICU::TextDirection::MIXED)
            {
                auto block = preparePart(textPaint, text, direction == ICU::TextDirection::RTL, hbBuffer, origin);
                if (block.textBlob)
                {
                    outMaxGlyphCount = qMax(outMaxGlyphCount, block.codepoints.size());
                    textPaint.glyphBlocks.push_back(qMove(block));
                    continue;
                }
                return false;
            }
            const auto len = text.length();
            int i, j, k, l;
            i = 0;
            bool prevRtl, nextRtl;
            while (i < len)
            {
                j = i;
                while (j < len && isNotRtlChar(text[j]))
                    j++;
                if (j > i)
                {
                    auto block = preparePart(textPaint, text.mid(i, j - i), false, hbBuffer, origin);
                    if (block.textBlob)
                    {
                        outMaxGlyphCount = qMax(outMaxGlyphCount, block.codepoints.size());
                        textPaint.glyphBlocks.push_back(qMove(block));
                    }
                    else
                        return false;
                    if (j == len)
                        break;
                    i = j;
                }
                k = j;
                while (k < len)
                {
                    if (!isNotRtlChar(text[k]))
                    {
                        k++;
                        j = k;
                    }
                    else if (isNotLtrChar(text[k]) || text[k].direction() == QChar::DirEN)
                        k++;
                    else
                        break;
                }
                if (j > i)
                {
                    l = j;
                    k = j - 1;
                    while (k >= i)
                    {
                        nextRtl = isRtlChar(text[k]) || isNotLtrChar(text[k]);
                        if (k == j - 1)
                            prevRtl = nextRtl;
                        if (nextRtl != prevRtl || k == i)
                        {
                            if (nextRtl != prevRtl)
                                k++;
                            auto block = preparePart(textPaint, text.mid(k, j - k), prevRtl, hbBuffer, origin);
                            if (block.textBlob)
                            {
                                outMaxGlyphCount = qMax(outMaxGlyphCount, block.codepoints.size());
                                textPaint.glyphBlocks.push_back(qMove(block));
                            }
                            else
                                return false;
                            j = k;
                            if (k > i + 1)
                            {
                                prevRtl = nextRtl;
                                k--;
                            }
                        }
                        k--;
                    }
                    i = l;
                }
            }
        }
    }
    return true;
}

void OsmAnd::TextRasterizer_P::measureText(QVector<LinePaint>& paints, int maxGlyphCount, const Style* style,
    SkScalar& outMaxLineWidth, SkScalar& outLeftGap, SkScalar& outRightGap, QVector<SkScalar>* outGlyphWidths) const
{
    outMaxLineWidth = 0;
    outLeftGap = 0;
    outRightGap = 0;

    SkScalar widths[maxGlyphCount];
    SkRect bounds[maxGlyphCount];
    for (auto& linePaint : paints)
    {
        SkScalar lineLeftGap = 0;
        SkScalar lineRightGap = 0;
        linePaint.maxBoundsTop = 0;
        linePaint.width = 0;
        const auto textsCount = linePaint.textPaints.size();
        for (int textIdx = 0; textIdx < textsCount; textIdx++)
        {
            auto& textPaint = linePaint.textPaints[textIdx];
            auto paint = textPaint.paint;
            if (style)
                paint = getHaloPaint(paint, *style);

            textPaint.width = 0;
            textPaint.bounds = SkRect::MakeEmpty();
            const auto blocksCount = textPaint.glyphBlocks.size();
            for (int blockIdx = 0; blockIdx < blocksCount; blockIdx++)
            {
                const auto& glyphBlock = textPaint.glyphBlocks[blockIdx];
                const auto glyphsCount = glyphBlock.codepoints.size();
                textPaint.skFont.getWidthsBounds(
                    glyphBlock.codepoints.constData(), glyphsCount, widths, bounds, &paint);
                for (int glyphIdx = 0; glyphIdx < glyphsCount; glyphIdx++)
                {
                    if (glyphIdx == 0 && blockIdx == 0 && textIdx == 0)
                    {
                        lineLeftGap = -bounds[glyphIdx].fLeft;
                        if (bounds[glyphIdx].width() == 0.0f)
                            continue;
                    }
                    if (glyphIdx == glyphsCount - 1 && blockIdx == blocksCount - 1 && textIdx == textsCount - 1)
                    {
                        lineRightGap = bounds[glyphIdx].fRight;
                        if (bounds[glyphIdx].width() == 0.0f)
                            continue;
                        else
                            lineRightGap -= widths[glyphIdx];
                    }
                    textPaint.width += widths[glyphIdx];
                    textPaint.bounds.join(bounds[glyphIdx]);
                }
                if (outGlyphWidths)
                {
                    const auto previousSize = outGlyphWidths->size();
                    outGlyphWidths->resize(previousSize + glyphsCount);
                    const auto pWidth = outGlyphWidths->data() + previousSize;
                    memcpy(pWidth, widths, glyphsCount * sizeof(SkScalar));
                }
            }
            textPaint.bounds.fLeft = 0;
            textPaint.bounds.fRight = textPaint.width;
            linePaint.maxBoundsTop = qMax(linePaint.maxBoundsTop, -textPaint.bounds.top());
            linePaint.width += textPaint.width;
        }
        if (linePaint.width > outMaxLineWidth)
        {
            outMaxLineWidth = linePaint.width;
            outLeftGap = lineLeftGap;
            outRightGap = lineRightGap;
        }
    }
}

SkPaint OsmAnd::TextRasterizer_P::getHaloPaint(
    const SkPaint& paint,
    const Style& style) const
{
    auto haloPaint = paint;

    haloPaint.setStyle(SkPaint::kStroke_Style);
    haloPaint.setColor(style.haloColor.toSkColor());
    haloPaint.setStrokeWidth(style.haloRadius);

    return haloPaint;
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
        SkScalar horizontalShift = 0;
        const auto widthDelta = maxLineWidth - linePaint.width;
        switch (textAlignment)
        {
            case Style::TextAlignment::Center:
                horizontalShift += widthDelta / 2.0f;
                break;

            case Style::TextAlignment::Right:
                horizontalShift += widthDelta;
                break;

            case Style::TextAlignment::Left:
            default:
                // Do nothing here
                break;
        }
        if (verticalOffset == 0)
            verticalOffset = linePaint.maxBoundsTop;
        SkScalar horizontalOffset = 0;
        for (auto& textPaint : linePaint.textPaints)
        {
            textPaint.positionedBounds = textPaint.bounds;
            if(horizontalOffset == 0)
                horizontalOffset = horizontalShift;

            // Position text
            textPaint.positionedBounds.offset(horizontalOffset, verticalOffset);

            // Include into text area
            textArea.join(textPaint.positionedBounds);

            // Position text vertically
            textPaint.positionedBounds.offset(0, -textPaint.bounds.top());

            horizontalOffset += textPaint.width;
        }
        verticalOffset += linePaint.maxFontHeight;
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

void OsmAnd::TextRasterizer_P::drawText(SkCanvas& canvas, const TextPaint& textPaint, const SkPaint& paint) const
{
    for (auto& glyphBlock : textPaint.glyphBlocks)
    {
        canvas.drawTextBlob(
            glyphBlock.textBlob,
            textPaint.positionedBounds.left(),
            textPaint.positionedBounds.top(),
            paint
        );
    }
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

    // Obtain glyph blocks
    int maxGlyphCount;
    if (!getGlyphBlocks(paints, maxGlyphCount))
        return false;

    bool withHalo = style.haloRadius > 0;

    // Measure text
    SkScalar maxLineWidthInPixels = 0;
    SkScalar leftGap = 0;
    SkScalar rightGap = 0;
    measureText(
        paints, maxGlyphCount, withHalo ? &style : nullptr, maxLineWidthInPixels, leftGap, rightGap, outGlyphWidths);

    if (outGlyphWidths)
    {
        outGlyphWidths->front() += 1.0f + leftGap;
        outGlyphWidths->back() += 1.0f + rightGap;
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

    // Shift text for at least one pixel to avoid aliasing at the left and top edges
    float offsetLeft = 1.0f + leftGap;
    float offsetTop = 1.0f;

    // Calculate bitmap size (add extra pixels to avoid aliasing at the edges)
    auto bitmapWidth = qCeil(leftGap + textArea.width() + rightGap) + 2;
    auto bitmapHeight = qCeil(textArea.height()) + 2;
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
        offsetLeft = (bitmapWidth - qCeil(leftGap + textArea.width() + rightGap)) / 2.0f;
        offsetTop = (bitmapHeight - qCeil(textArea.height())) / 2.0f;
    }

    const auto offset = SkPoint::Make(offsetLeft, offsetTop);
    for (auto& linePaint : paints)
    {
        for (auto& textPaint : linePaint.textPaints)
        {
            textPaint.positionedBounds.offset(offset);
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

    // Create a bitmap that will hold entire symbol (if target is empty)
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

    // Enable to draw background for each glyph (for debug)
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

    // Rasterize text halo first (if enabled)
    if (withHalo)
    {
        for (const auto& linePaint : paints)
        {
            for (const auto& textPaint : linePaint.textPaints)
            {
                const auto haloPaint = getHaloPaint(textPaint.paint, style);
                drawText(canvas, textPaint, haloPaint);
            }
        }
    }

    // Rasterize text itself
    for (const auto& linePaint : paints)
    {
        for (const auto& textPaint : linePaint.textPaints)
        {
            drawText(canvas, textPaint, textPaint.paint);
        }
    }

    canvas.flush();

    return true;
}
