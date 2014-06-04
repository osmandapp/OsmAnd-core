#ifndef _OSMAND_CORE_SPRITE_MAP_SYMBOL_H_
#define _OSMAND_CORE_SPRITE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/PositionedRasterMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API SpriteMapSymbol : public PositionedRasterMapSymbol
    {
        Q_DISABLE_COPY(SpriteMapSymbol);
    private:
    protected:
    public:
        SpriteMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const int order,
            const IntersectionModeFlags intersectionModeFlags,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& position31,
            const PointI& offset);
        virtual ~SpriteMapSymbol();

        PointI offset;
    };
}

#endif // !defined(_OSMAND_CORE_SPRITE_MAP_SYMBOL_H_)
