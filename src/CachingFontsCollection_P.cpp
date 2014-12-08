#include "CachingFontsCollection_P.h"
#include "CachingFontsCollection.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkTypeface.h>
#include "restore_internal_warnings.h"

#include "SkiaUtilities.h"
#include "Logging.h"

OsmAnd::CachingFontsCollection_P::CachingFontsCollection_P(CachingFontsCollection* const owner_)
    : owner(owner_)
{
}

OsmAnd::CachingFontsCollection_P::~CachingFontsCollection_P()
{
}

SkTypeface* OsmAnd::CachingFontsCollection_P::obtainTypeface(const QString& fontName) const
{
    QMutexLocker scopedLocker(&_fontTypefacesCacheMutex);

    // Look for loaded typefaces
    const auto& citTypeface = _fontTypefacesCache.constFind(fontName);
    if (citTypeface != _fontTypefacesCache.cend())
        return *citTypeface;

    // Obtain font data
    const auto fontData = owner->obtainFont(fontName);
    if (fontData.isNull())
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to load font data for '%s'",
            qPrintable(fontName));
        return nullptr;
    }

    // Create typeface from data
    const auto typeface = SkiaUtilities::createTypefaceFromData(fontData);
    if (!typeface)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to create SkTypeface from data for '%s'",
            qPrintable(fontName));
        return nullptr;
    }

    _fontTypefacesCache.insert(fontName, typeface);

    return typeface;
}

void OsmAnd::CachingFontsCollection_P::clearFontsCache()
{
    QMutexLocker scopedLocker(&_fontTypefacesCacheMutex);

    for (const auto& typeface : _fontTypefacesCache)
        typeface->unref();
    _fontTypefacesCache.clear();
}
