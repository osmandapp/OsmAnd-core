#ifndef _OSMAND_CORE_SPRITE_MAP_SYMBOL_H_
#define _OSMAND_CORE_SPRITE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/BoundToPointMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API SpriteMapSymbol : public BoundToPointMapSymbol
    {
        Q_DISABLE_COPY(SpriteMapSymbol);
    private:
    protected:
    public:
        SpriteMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const IntersectionModeFlags intersectionModeFlags,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& location31,
            const PointI& offset);
        virtual ~SpriteMapSymbol();

        const PointI offset;
    };
}

#endif // !defined(_OSMAND_CORE_SPRITE_MAP_SYMBOL_H_)
