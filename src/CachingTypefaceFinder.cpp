#include "CachingTypefaceFinder.h"

OsmAnd::CachingTypefaceFinder::CachingTypefaceFinder(const std::shared_ptr<const ITypefaceFinder>& typefaceFinder_)
    : typefaceFinder(typefaceFinder_)
{
}

OsmAnd::CachingTypefaceFinder::~CachingTypefaceFinder()
{
}

std::shared_ptr<const OsmAnd::ITypefaceFinder::Typeface> OsmAnd::CachingTypefaceFinder::findTypefaceForCharacterUCS4(
    const uint32_t character,
    const SkFontStyle style /*= SkFontStyle()*/) const
{
    QMutexLocker scopedLocker(&_lock);

    CacheKey cacheKey;
    cacheKey.styleId = *reinterpret_cast<const StyleId*>(&style);
    cacheKey.character = character;

    const auto citTypeface = _cache.constFind(cacheKey);
    if (citTypeface != _cache.cend())
        return *citTypeface;

    const auto typeface = typefaceFinder->findTypefaceForCharacterUCS4(character, style);
    _cache.insert(cacheKey, typeface);
    return typeface;
}
