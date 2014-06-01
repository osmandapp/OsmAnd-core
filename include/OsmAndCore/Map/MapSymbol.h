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
    private:
    protected:
        MapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance);
    public:
        virtual ~MapSymbol();

        const std::weak_ptr<const MapSymbolsGroup> group;
        const MapSymbolsGroup* const groupPtr;

        const bool isShareable;

        const std::shared_ptr<const SkBitmap> bitmap;
        const int order;
        const QString content;
        const LanguageId languageId;
        const PointI minDistance;

        virtual MapSymbol* cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_SYMBOL_H_)
