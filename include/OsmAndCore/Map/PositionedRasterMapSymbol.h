#ifndef _OSMAND_CORE_POSITIONED_RASTER_MAP_SYMBOL_H_
#define _OSMAND_CORE_POSITIONED_RASTER_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterMapSymbol.h>
#include <OsmAndCore/Map/IPositionedMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API PositionedRasterMapSymbol
        : public RasterMapSymbol
        , public IPositionedMapSymbol
    {
        Q_DISABLE_COPY(PositionedRasterMapSymbol);
    private:
    protected:
        PositionedRasterMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const int order,
            const IntersectionModeFlags intersectionModeFlags,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& position31);
    public:
        virtual ~PositionedRasterMapSymbol();

        PointI position31;

        virtual PointI getPosition31() const;
        virtual void setPosition31(const PointI position);
    };
}

#endif // !defined(_OSMAND_CORE_POSITIONED_RASTER_MAP_SYMBOL_H_)
