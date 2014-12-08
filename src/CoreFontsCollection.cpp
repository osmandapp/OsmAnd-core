#include "CoreFontsCollection.h"
#include "CoreFontsCollection_private.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QStringList>
#include <QFileInfo>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkPaint.h>
#include "restore_internal_warnings.h"

#include "ICoreResourcesProvider.h"
#include "Logging.h"

QList<OsmAnd::CoreFontsCollection::CoreResourcesFont> OsmAnd::CoreFontsCollection::fonts =
    []() -> QList<OsmAnd::CoreFontsCollection::CoreResourcesFont>
    {
        QList<OsmAnd::CoreFontsCollection::CoreResourcesFont> registry;

        // OpenSans
        registry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Regular.ttf"), false, false });
        registry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Italic.ttf"), false, true });
        registry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-Semibold.ttf"), true, false });
        registry.push_back({ QLatin1String("map/fonts/OpenSans/OpenSans-SemiboldItalic.ttf"), true, true });

        // Noto Kufi Arabic
        registry.push_back({ QLatin1String("map/fonts/NotoKufi/NotoKufiArabic-Regular.ttf"), false, false });
        registry.push_back({ QLatin1String("map/fonts/NotoKufi/NotoKufiArabic-Bold.ttf"), true, false });

        // Noto Naskh Arabic
        registry.push_back({ QLatin1String("map/fonts/NotoNaskh/NotoNaskhArabic-Regular.ttf"), false, false });
        registry.push_back({ QLatin1String("map/fonts/NotoNaskh/NotoNaskhArabic-Bold.ttf"), true, false });

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
            registry.push_back({
                QString(QLatin1String("map/fonts/NotoSans/NotoSans%1-Regular.ttf")).arg(QLatin1String(notoSans[scriptIdx])),
                false,
                false });
            registry.push_back({
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
            registry.push_back({
                QString(QLatin1String("map/fonts/NotoSans-extra/NotoSans%1-Regular.ttf")).arg(QLatin1String(notoSansExtra[scriptIdx])),
                false,
                false });
            registry.push_back({
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
            registry.push_back({
                QString(QLatin1String("map/fonts/NotoSans-extra/NotoSans%1-Regular.ttf")).arg(QLatin1String(notoSansExtraRegular[scriptIdx])),
                false,
                false });
        }

        // Tinos (from the Noto pack)
        registry.push_back({ QLatin1String("map/fonts/Tinos/Tinos-Regular.ttf"), false, false });
        registry.push_back({ QLatin1String("map/fonts/Tinos/Tinos-Italic.ttf"), false, true });
        registry.push_back({ QLatin1String("map/fonts/Tinos/Tinos-Bold.ttf"), true, false });
        registry.push_back({ QLatin1String("map/fonts/Tinos/Tinos-BoldItalic.ttf"), true, true });

        // DroidSans
        registry.push_back({ QLatin1String("map/fonts/DroidSansFallback.ttf"), false, false });

        // Misc
        registry.push_back({ QLatin1String("map/fonts/MTLmr3m.ttf"), false, false });

        return registry;
    }();

OsmAnd::CoreFontsCollection::CoreFontsCollection()
{
}

OsmAnd::CoreFontsCollection::~CoreFontsCollection()
{
}

QString OsmAnd::CoreFontsCollection::findSuitableFont(const QString& text, const bool isBold, const bool isItalic) const
{
    // Lookup matching font entry
    const CoreResourcesFont* pBestMatchFont = nullptr;
    SkPaint textCoverageTestPaint;
    textCoverageTestPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    for (const auto& entry : constOf(fonts))
    {
        // Get typeface for this entry
        const auto typeface = obtainTypeface(entry.resource);
        if (!typeface)
            continue;

        // Check if this typeface covers provided text
        textCoverageTestPaint.setTypeface(typeface);
        if (!textCoverageTestPaint.containsText(text.constData(), text.length()*sizeof(QChar)))
            continue;

        // Mark this as best match
        pBestMatchFont = &entry;

        // If this entry fully matches the request, stop search
        if (entry.bold == isBold && entry.italic == isItalic)
            return entry.resource;
    }

    if (pBestMatchFont != nullptr)
        return pBestMatchFont->resource;

    // If there's no best match, fallback to default typeface
#if OSMAND_DEBUG && 1
    LogPrintf(LogSeverityLevel::Warning,
        "No embedded font found that contains all glyphs of \"%s\":",
        qPrintable(text));
    for (const auto unicodeChar : constOf(text))
    {
        QStringList matchingFonts;

        SkPaint textCoverageTestPaint;
        textCoverageTestPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
        for (const auto& entry : constOf(fonts))
        {
            // Get typeface for this entry
            const auto typeface = obtainTypeface(entry.resource);
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

    return QString::null;
}

QByteArray OsmAnd::CoreFontsCollection::obtainFont(const QString& fontName) const
{
    return getCoreResourcesProvider()->getResource(fontName);
}

std::shared_ptr<OsmAnd::CoreFontsCollection> s_coreFontsCollectionDefaultInstance;
std::shared_ptr<const OsmAnd::CoreFontsCollection> OsmAnd::CoreFontsCollection::getDefaultInstance()
{
    return s_coreFontsCollectionDefaultInstance;
}

void OsmAnd::CoreFontsCollection_initialize()
{
    s_coreFontsCollectionDefaultInstance.reset(new CoreFontsCollection());
}

void OsmAnd::CoreFontsCollection_release()
{
    s_coreFontsCollectionDefaultInstance.reset();
}
