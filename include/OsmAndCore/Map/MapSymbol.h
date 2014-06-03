#ifndef _OSMAND_CORE_MAP_SYMBOL_H_
#define _OSMAND_CORE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>
#include <QString>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Map/IMapDataProvider.h>

namespace OsmAnd
{
    class MapSymbolsGroup;

    class OSMAND_CORE_API MapSymbol
    {
        Q_DISABLE_COPY(MapSymbol);

    public:
        enum IntersectionModeFlags : uint32_t
        {
            RegularIntersectionProcessing = 0,

            IgnoredByIntersectionTest = 1 << 0,
            TransparentForIntersectionLookup = 1 << 1,
        };

    private:
    protected:
        MapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const IntersectionModeFlags intersectionModeFlags,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance);
    public:
        virtual ~MapSymbol();

        const std::weak_ptr<MapSymbolsGroup> group;
        MapSymbolsGroup* const groupPtr;

        const bool isShareable;

        std::shared_ptr<const SkBitmap> bitmap;
        const int order;
        const IntersectionModeFlags intersectionModeFlags;
        const QString content;
        const LanguageId languageId;
        const PointI minDistance;

        virtual std::shared_ptr<MapSymbol> clone() const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOL_H_)
