#include "CachingFontFinder.h"

#include <SkTypeface.h>

OsmAnd::CachingFontFinder::CachingFontFinder(const std::shared_ptr<const IFontFinder>& fontFinder_)
    : fontFinder(fontFinder_)
{
}

OsmAnd::CachingFontFinder::~CachingFontFinder()
{
}

sk_sp<SkTypeface> OsmAnd::CachingFontFinder::findFontForCharacterUCS4(
    const uint32_t character,
    const SkFontStyle style /*= SkFontStyle()*/) const
{
    QMutexLocker scopedLocker(&_lock);

    CacheKey cacheKey;
    cacheKey.styleId = *reinterpret_cast<const StyleId*>(&style);
    cacheKey.character = character;

    const auto citFont = _cache.constFind(cacheKey);
    if (citFont != _cache.cend())
        return *citFont;

    const auto font = fontFinder->findFontForCharacterUCS4(character, style);
    if (font)
        font->ref();
    _cache.insert(cacheKey, font);
    return font;
}
