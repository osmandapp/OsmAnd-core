#ifndef _OSMAND_CORE_BOUND_TO_POINT_MAP_SYMBOL_H_
#define _OSMAND_CORE_BOUND_TO_POINT_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API BoundToPointMapSymbol : public MapSymbol
    {
        Q_DISABLE_COPY(BoundToPointMapSymbol);
    private:
    protected:
        BoundToPointMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& location31);
    public:
        virtual ~BoundToPointMapSymbol();

        PointI location31;
    };
}

#endif // !defined(_OSMAND_CORE_BOUND_TO_POINT_MAP_SYMBOL_H_)
