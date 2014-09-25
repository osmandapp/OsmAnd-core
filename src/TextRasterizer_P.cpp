#include "TextRasterizer_P.h"
#include "TextRasterizer.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmapDevice.h>
#include <SkStream.h>
#include <SkTypeface.h>
#include "restore_internal_warnings.h"

#include "ICU.h"
#include "CoreResourcesEmbeddedBundle.h"

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

    // Compose fonts register (in priority order)

    // OpenSans
    _fontsRegistry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Italic.ttf"), false, true });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Semibold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-SemiboldItalic.ttf"), true, true });

    // Noto Kufi Arabic
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoKufi/NotoKufiArabic-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoKufi/NotoKufiArabic-Bold.ttf"), true, false });

    // Noto Naskh Arabic
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoNaskh/NotoNaskhArabic-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/NotoNaskh/NotoNaskhArabic-Bold.ttf"), true, false });

    // NotoSans family, all fonts support regular and italic
    const char* const notoSans[] =
    {
        "Thai",
        "Devanagari",
        "Tamil",
        "Malayalam",
        "Bengali",
        "Telugu",
        "Kannada",
        "Khmer",
        "Lao",
        "Armenian",
        "Cham",
        "Ethiopic",
        "Georgian",
        "Gujarati",
        "Gurmukhi",
        "Hebrew"
    };
    for (unsigned int scriptIdx = 0u; scriptIdx < sizeof(notoSans) / sizeof(notoSans[0]); scriptIdx++)
    {
        _fontsRegistry.push_back({
            QString(QLatin1String("map/fonts/NotoSans/NotoSans%1-Regular.ttf")).arg(QLatin1String(notoSans[scriptIdx])),
            false,
            false });
        _fontsRegistry.push_back({
            QString(QLatin1String("map/fonts/NotoSans/NotoSans%1-Bold.ttf")).arg(QLatin1String(notoSans[scriptIdx])),
            true,
            false });
    }

    // NotoSans extra
    const char* const notoSansExtra[] =
    {
        "Myanmar",
        "Sinhala"
    };
    for (unsigned int scriptIdx = 0u; scriptIdx < sizeof(notoSansExtra) / sizeof(notoSansExtra[0]); scriptIdx++)
    {
        _fontsRegistry.push_back({
            QString(QLatin1String("map/fonts/NotoSans-extra/NotoSans%1-Regular.ttf")).arg(QLatin1String(notoSansExtra[scriptIdx])),
            false,
            false });
            _fontsRegistry.push_back({
                QString(QLatin1String("map/fonts/NotoSans-extra/NotoSans%1-Bold.ttf")).arg(QLatin1String(notoSansExtra[scriptIdx])),
                true,
                false });
    }

    // NotoSans extra (regular only)
    const char* const notoSansExtraRegular[] =
    {
        "Avestan",
        "Balinese",
        "Bamum",
        "Batak",
        "Brahmi",
        "Buginese",
        "Buhid",
        "CanadianAboriginal",
        "Carian",
        "Cherokee",
        "Coptic",
        "Cuneiform",
        "Cypriot",
        "Deseret",
        "EgyptianHieroglyphs",
        "Glagolitic",
        "Gothic",
        "Hanunoo",
        "ImperialAramaic",
        "InscriptionalPahlavi",
        "InscriptionalParthian",
        "Javanese",
        "Kaithi",
        "KayahLi",
        "Kharoshthi",
        "Lepcha",
        "Limbu",
        "LinearB",
        "Lisu",
        "Lycian",
        "Lydian",
        "Mandaic",
        "MeeteiMayek",
        "Mongolian",
        "NKo",
        "NewTaiLue",
        "Ogham",
        "OlChiki",
        "OldItalic",
        "OldPersian",
        "OldSouthArabian",
        "OldTurkic",
        "Osmanya",
        "PhagsPa",
        "Phoenician",
        "Rejang",
        "Runic",
        "Samaritan",
        "Saurashtra",
        "Shavian",
        "Sundanese",
        "SylotiNagri",
        "SyriacEastern",
        "SyriacEstrangela",
        "SyriacWestern",
        "Tagalog",
        "Tagbanwa",
        "TaiLe",
        "TaiTham",
        "TaiViet",
        "Tifinagh",
        "Ugaritic",
        "Vai",
        "Yi"
    };
    for (unsigned int scriptIdx = 0u; scriptIdx < sizeof(notoSansExtraRegular) / sizeof(notoSansExtraRegular[0]); scriptIdx++)
    {
        _fontsRegistry.push_back({
            QString(QLatin1String("map/fonts/NotoSans-extra/NotoSans%1-Regular.ttf")).arg(QLatin1String(notoSansExtraRegular[scriptIdx])),
            false,
            false });
    }

    // Tinos (from the Noto pack)
    _fontsRegistry.push_back({ QLatin1String("map/fonts/Tinos/Tinos-Regular.ttf"), false, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/Tinos/Tinos-Italic.ttf"), false, true });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/Tinos/Tinos-Bold.ttf"), true, false });
    _fontsRegistry.push_back({ QLatin1String("map/fonts/Tinos/Tinos-BoldItalic.ttf"), true, true });

    // DroidSans
    _fontsRegistry.push_back({ QLatin1String("map/fonts/DroidSansFallback.ttf"), false, false });

    // Misc
    _fontsRegistry.push_back({ QLatin1String("map/fonts/MTLmr3m.ttf"), false, false });
}

void OsmAnd::TextRasterizer_P::clearFontsCache()
{
    QMutexLocker scopedLocker(&_fontTypefacesCacheMutex);

    for (const auto& typeface : _fontTypefacesCache)
        typeface->unref();
    _fontTypefacesCache.clear();
}

#if defined(OSMAND_TARGET_OS_android)
#include <SkString.h>
#include <SkTypefaceCache.h>
#include <SkFontDescriptor.h>
#include <SkFontHost_FreeType_common.h>

// Defined in SkFontHost_FreeType.cpp
bool find_name_and_attributes(
    SkStream* stream,
    SkString* name,
    SkTypeface::Style* style,
    bool* isFixedWidth);

namespace OsmAnd
{
    // Based on SkFontHost_linux.cpp
    class SkTypeface_StreamOnAndroidWorkaround : public SkTypeface_FreeType
    {
    public:
        SkTypeface_StreamOnAndroidWorkaround(Style style, SkStream* stream, bool isFixedPitch, const SkString familyName)
            : SkTypeface_FreeType(style, SkTypefaceCache::NewFontID(), isFixedPitch)
            , _stream(SkRef(stream))
            , _familyName(familyName)
        {
        }

        virtual ~SkTypeface_StreamOnAndroidWorkaround()
        {
        }

    protected:
        virtual void onGetFontDescriptor(SkFontDescriptor* desc, bool* isLocal) const SK_OVERRIDE
        {
            desc->setFamilyName(_familyName.c_str());
            desc->setFontFileName(nullptr);
            *isLocal = true;
        }

        virtual SkStream* onOpenStream(int* ttcIndex) const SK_OVERRIDE
        {
            *ttcIndex = 0;
            return _stream->duplicate();
        }

    private:
        SkAutoTUnref<SkStream> _stream;
        SkString _familyName;
    };
}
#endif // defined(OSMAND_TARGET_OS_android)

SkTypeface* OsmAnd::TextRasterizer_P::getTypefaceForFontResource(const QString& fontResource) const
{
    QMutexLocker scopedLocker(&_fontTypefacesCacheMutex);

    // Look for loaded typefaces
    const auto& citTypeface = _fontTypefacesCache.constFind(fontResource);
    if (citTypeface != _fontTypefacesCache.cend())
        return *citTypeface;

    // Load raw data from resource
    const auto fontData = getCoreResourcesProvider()->getResource(fontResource);
    if (fontData.isNull())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to load font data for '%s'",
            qPrintable(fontResource));
        return nullptr;
    }

    // Load typeface from font data
    const auto fontDataStream = new SkMemoryStream(fontData.constData(), fontData.length(), true);
#if defined(OSMAND_TARGET_OS_android)
    //WORKAROUND: Right now SKIA on Android lost ability to load custom fonts, since it doesn't provide SkFontMgr.
    //WORKAROUND: But it still loads system fonts internally, so just write direct code

    SkTypeface::Style typefaceStyle;
    bool typefaceIsFixedWidth = false;
    SkString typefaceName;
    const auto isValidTypeface = find_name_and_attributes(fontDataStream, &typefaceName, &typefaceStyle, &typefaceIsFixedWidth);
    SkTypeface* typeface = nullptr;
    if (isValidTypeface)
        typeface = SkNEW_ARGS(SkTypeface_StreamOnAndroidWorkaround, (typefaceStyle, fontDataStream, typefaceIsFixedWidth, typefaceName));
#else // if !defined(OSMAND_TARGET_OS_android)
    const auto typeface = SkTypeface::CreateFromStream(fontDataStream);
#endif
    fontDataStream->unref();
    if (!typeface)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to load SkTypeface from '%s'",
            qPrintable(fontResource));
        return nullptr;
    }

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
#if OSMAND_DEBUG && 0
        LogPrintf(LogSeverityLevel::Warning,
            "No embedded font found that contains all glyphs of \"%s\":",
            qPrintable(text));
        for (const auto unicodeChar : constOf(text))
        {
            QStringList matchingFonts;

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
                if (!textCoverageTestPaint.containsText(&unicodeChar, sizeof(QChar)))
                    continue;

                matchingFonts.push_back(QFileInfo(entry.resource).fileName());
            }

            LogPrintf(LogSeverityLevel::Warning,
                "\tU+%04X : %s",
                unicodeChar.unicode(),
                qPrintable(matchingFonts.isEmpty() ? QString(QLatin1String("missing!")) : matchingFonts.join(QLatin1String("; "))));
        }

#endif // OSMAND_DEBUG

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
    const bool ok = rasterize(*bitmap, text, style, outGlyphWidths, outExtraTopSpace, outExtraBottomSpace, outLineSpacing);
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
