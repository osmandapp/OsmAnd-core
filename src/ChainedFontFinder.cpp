#include "ChainedFontFinder.h"

#include "QtExtensions.h"
#include "QtCommon.h"

OsmAnd::ChainedFontFinder::ChainedFontFinder(const QList< std::shared_ptr<const IFontFinder> >& chain_)
    : chain(detachedOf(chain_))
{
}

OsmAnd::ChainedFontFinder::~ChainedFontFinder()
{
}

sk_sp<SkTypeface> OsmAnd::ChainedFontFinder::findFontForCharacterUCS4(
    const uint32_t character,
    const SkFontStyle style /*= SkFontStyle()*/) const
{
    for (const auto& fontFinder : constOf(chain))
    {
        const auto font = fontFinder->findFontForCharacterUCS4(character, style);
        if (font)
            return font;
    }

    return nullptr;
}
