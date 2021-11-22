#include "EmbeddedFontFinder.h"
#include "EmbeddedFontFinder_internal.h"

#include "QtExtensions.h"
#include "QtCommon.h"

#include <SkTypeface.h>
#include <SkFont.h>

#include "ICoreResourcesProvider.h"
#include "SkiaUtilities.h"
#include "Logging.h"

OsmAnd::EmbeddedFontFinder::EmbeddedFontFinder(
    const std::shared_ptr<const ICoreResourcesProvider>& coreResourcesProvider_ /*= getCoreResourcesProvider()*/)
    : coreResourcesProvider(coreResourcesProvider_)
{
    for (const auto& embeddedFontResource : constOf(resources))
    {
        const auto fontData = coreResourcesProvider->getResource(embeddedFontResource);
        if (fontData.isNull())
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to load embedded font data for '%s'",
                qPrintable(embeddedFontResource));
            continue;
        }

        const auto font = SkiaUtilities::createTypefaceFromData(fontData);
        if (!font)
        {
            LogPrintf(LogSeverityLevel::Error,
                "Failed to create SkTypeface from embedded font data for '%s'",
                qPrintable(embeddedFontResource));
            continue;
        }

        _fonts.push_back(font);
    }
}

OsmAnd::EmbeddedFontFinder::~EmbeddedFontFinder()
{
}

sk_sp<SkTypeface> OsmAnd::EmbeddedFontFinder::findFontForCharacterUCS4(
    const uint32_t character,
    const SkFontStyle style /*= SkFontStyle()*/) const
{
    sk_sp<SkTypeface> bestMatch = nullptr;
    auto bestMatchDifference = std::numeric_limits<float>::quiet_NaN();
    for (const auto& font : constOf(_fonts))
    {
        // If font doesn't contain requested character, it should be completely ignored
        if (font->unicharToGlyph(character) == 0)
            continue;

        // Calculate difference between this font style and requested style
        auto difference = 0.0f;
        const auto fontStyle = font->fontStyle();
        if (fontStyle.slant() != style.slant())
            difference += 1.0f;
        if (fontStyle.width() != style.width())
            difference += static_cast<float>(qAbs(fontStyle.width() - style.width())) / SkFontStyle::kUltraExpanded_Width;
        if (fontStyle.weight() != style.weight())
            difference += static_cast<float>(qAbs(fontStyle.weight() - style.weight())) / SkFontStyle::kBlack_Weight;

        // If there was previous best match, check if this match is better
        if (bestMatch && bestMatchDifference < difference)
            continue;

        bestMatch = font;
        bestMatchDifference = difference;

        // In case difference is 0, there won't be better match
        if (qFuzzyIsNull(bestMatchDifference))
            break;
    }

    return bestMatch;
}

static std::shared_ptr<OsmAnd::EmbeddedFontFinder> s_embeddedFontFinderDefaultInstance;
std::shared_ptr<const OsmAnd::EmbeddedFontFinder> OsmAnd::EmbeddedFontFinder::getDefaultInstance()
{
    return s_embeddedFontFinderDefaultInstance;
}

void OsmAnd::EmbeddedFontFinder_initialize()
{
    s_embeddedFontFinderDefaultInstance.reset(new EmbeddedFontFinder());
}

void OsmAnd::EmbeddedFontFinder_release()
{
    s_embeddedFontFinderDefaultInstance.reset();
}

const QStringList OsmAnd::EmbeddedFontFinder::resources = QStringList()

     << QLatin1String("map/fonts/05_NotoSans-Regular.ttf")
     << QLatin1String("map/fonts/10_NotoSans-Bold.ttf")
     << QLatin1String("map/fonts/15_NotoSans-Italic.ttf")
     << QLatin1String("map/fonts/20_NotoSans-BoldItalic.ttf")
//     << QLatin1String("map/fonts/25_NotoSansArabic-Regular.ttf")
//     << QLatin1String("map/fonts/30_NotoSansArabic-Bold.ttf")
     << QLatin1String("map/fonts/35_NotoSansSouthAsian-Regular.ttf")
     << QLatin1String("map/fonts/40_NotoSansSouthAsian-Bold.ttf")
     << QLatin1String("map/fonts/45_NotoSansSoutheastAsian-Regular.ttf")
     << QLatin1String("map/fonts/50_NotoSansSoutheastAsian-Bold.ttf")
     << QLatin1String("map/fonts/55_NotoSansTibetan-Regular.ttf")
     << QLatin1String("map/fonts/60_NotoSansTibetan-Bold.ttf")
     << QLatin1String("map/fonts/65_NotoSansNastaliqUrdu-Regular.ttf")

    // Noto Kufi Arabic
//     << QLatin1String("map/fonts/NotoKufi/NotoKufiArabic-Regular.ttf")
//     << QLatin1String("map/fonts/NotoKufi/NotoKufiArabic-Bold.ttf")

    // Noto Naskh Arabic
//     << QLatin1String("map/fonts/NotoNaskh/NotoNaskhArabic-Regular.ttf")
//     << QLatin1String("map/fonts/NotoNaskh/NotoNaskhArabic-Bold.ttf")

    // NotoSans family, all fonts support regular and italic
//     << qTransform<QStringList>(
//         QStringList()
//             << QLatin1String("Thai")
//             << QLatin1String("Devanagari")
//             << QLatin1String("Tamil")
//             << QLatin1String("Malayalam")
//             << QLatin1String("Bengali")
//             << QLatin1String("Telugu")
//             << QLatin1String("Kannada")
//             << QLatin1String("Khmer")
//             << QLatin1String("Lao")
//             << QLatin1String("Armenian")
//             << QLatin1String("Cham")
//             << QLatin1String("Ethiopic")
//             << QLatin1String("Georgian")
//             << QLatin1String("Gujarati")
//             << QLatin1String("Gurmukhi")
//             << QLatin1String("Hebrew"),
//         []
//         (const QString& input) -> QStringList
//         {
//             return QStringList()
//                 << QString(QLatin1String("map/fonts/NotoSans/NotoSans%1-Regular.ttf")).arg(input)
//                 << QString(QLatin1String("map/fonts/NotoSans/NotoSans%1-Bold.ttf")).arg(input);
//         })

    // NotoSans extra
//     << qTransform<QStringList>(
//         QStringList()
//             << QLatin1String("Myanmar")
//             << QLatin1String("Sinhala"),
//         []
//         (const QString& input) -> QStringList
//         {
//             return QStringList()
//                 << QString(QLatin1String("map/fonts/NotoSans-extra/NotoSans%1-Regular.ttf")).arg(input)
//                 << QString(QLatin1String("map/fonts/NotoSans-extra/NotoSans%1-Bold.ttf")).arg(input);
//         })

    // NotoSans extra (regular only)
//     << qTransform<QStringList>(
//         QStringList()
//             << QLatin1String("Avestan")
//             << QLatin1String("Balinese")
//             << QLatin1String("Bamum")
//             << QLatin1String("Batak")
//             << QLatin1String("Brahmi")
//             << QLatin1String("Buginese")
//             << QLatin1String("Buhid")
//             << QLatin1String("CanadianAboriginal")
//             << QLatin1String("Carian")
//             << QLatin1String("Cherokee")
//             << QLatin1String("Coptic")
//             << QLatin1String("Cuneiform")
//             << QLatin1String("Cypriot")
//             << QLatin1String("Deseret")
//             << QLatin1String("EgyptianHieroglyphs")
//             << QLatin1String("Glagolitic")
//             << QLatin1String("Gothic")
//             << QLatin1String("Hanunoo")
//             << QLatin1String("ImperialAramaic")
//             << QLatin1String("InscriptionalPahlavi")
//             << QLatin1String("InscriptionalParthian")
//             << QLatin1String("Javanese")
//             << QLatin1String("Kaithi")
//             << QLatin1String("KayahLi")
//             << QLatin1String("Kharoshthi")
//             << QLatin1String("Lepcha")
//             << QLatin1String("Limbu")
//             << QLatin1String("LinearB")
//             << QLatin1String("Lisu")
//             << QLatin1String("Lycian")
//             << QLatin1String("Lydian")
//             << QLatin1String("Mandaic")
//             << QLatin1String("MeeteiMayek")
//             << QLatin1String("Mongolian")
//             << QLatin1String("NKo")
//             << QLatin1String("NewTaiLue")
//             << QLatin1String("Ogham")
//             << QLatin1String("OlChiki")
//             << QLatin1String("OldItalic")
//             << QLatin1String("OldPersian")
//             << QLatin1String("OldSouthArabian")
//             << QLatin1String("OldTurkic")
//             << QLatin1String("Osmanya")
//             << QLatin1String("PhagsPa")
//             << QLatin1String("Phoenician")
//             << QLatin1String("Rejang")
//             << QLatin1String("Runic")
//             << QLatin1String("Samaritan")
//             << QLatin1String("Saurashtra")
//             << QLatin1String("Shavian")
//             << QLatin1String("Sundanese")
//             << QLatin1String("SylotiNagri")
//             << QLatin1String("SyriacEastern")
//             << QLatin1String("SyriacEstrangela")
//             << QLatin1String("SyriacWestern")
//             << QLatin1String("Tagalog")
//             << QLatin1String("Tagbanwa")
//             << QLatin1String("TaiLe")
//             << QLatin1String("TaiTham")
//             << QLatin1String("TaiViet")
//             << QLatin1String("Tifinagh")
//             << QLatin1String("Ugaritic")
//             << QLatin1String("Vai")
//             << QLatin1String("Yi"),
//         []
//         (const QString& input) -> QStringList
//         {
//             return QStringList()
//                 << QString(QLatin1String("map/fonts/NotoSans-extra/NotoSans%1-Regular.ttf")).arg(input);
//         })

    // DroidSans
    << QLatin1String("map/fonts/DroidSansFallback.ttf");

    // Misc
    //<< QLatin1String("map/fonts/MTLmr3m.ttf");
