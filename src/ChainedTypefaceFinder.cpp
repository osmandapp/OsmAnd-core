#include "ChainedTypefaceFinder.h"

#include "QtExtensions.h"
#include "QtCommon.h"

OsmAnd::ChainedTypefaceFinder::ChainedTypefaceFinder(const QList< std::shared_ptr<const ITypefaceFinder> >& chain_)
    : chain(detachedOf(chain_))
{
}

OsmAnd::ChainedTypefaceFinder::~ChainedTypefaceFinder()
{
}

std::shared_ptr<const OsmAnd::ITypefaceFinder::Typeface> OsmAnd::ChainedTypefaceFinder::findTypefaceForCharacterUCS4(
    const uint32_t character,
    const SkFontStyle style /*= SkFontStyle()*/) const
{
    for (const auto& typefaceFinder : constOf(chain))
    {
        const auto typeface = typefaceFinder->findTypefaceForCharacterUCS4(character, style);
        if (typeface)
            return typeface;
    }

    return nullptr;
}
