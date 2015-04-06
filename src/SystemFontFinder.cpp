#include "SystemFontFinder.h"

OsmAnd::SystemFontFinder::SystemFontFinder(const SkFontMgr* const fontManager_ /*= SkFontMgr::RefDefault()*/)
    : fontManager(fontManager_)
{
}

OsmAnd::SystemFontFinder::~SystemFontFinder()
{
    fontManager->unref();
}

SkTypeface* OsmAnd::SystemFontFinder::findFontForCharacterUCS4(
    const uint32_t character,
    const SkFontStyle style /*= SkFontStyle()*/) const
{
    return fontManager->matchFamilyStyleCharacter(nullptr, style, nullptr, 0, character);
}
