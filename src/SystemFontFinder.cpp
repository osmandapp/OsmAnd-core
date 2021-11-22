#include "SystemFontFinder.h"

OsmAnd::SystemFontFinder::SystemFontFinder(const sk_sp<SkFontMgr> fontManager_ /*= SkFontMgr::RefDefault()*/)
    : fontManager(fontManager_)
{
}

OsmAnd::SystemFontFinder::~SystemFontFinder()
{
}

sk_sp<SkTypeface> OsmAnd::SystemFontFinder::findFontForCharacterUCS4(
    const uint32_t character,
    const SkFontStyle style /*= SkFontStyle()*/) const
{
    const auto pTypeface = fontManager->matchFamilyStyleCharacter(nullptr, style, nullptr, 0, character);
    if (!pTypeface)
        return nullptr;

    return pTypeface->makeClone({});
}
