#ifndef _OSMAND_CORE_ON_SURFACE_RASTER_MAP_SYMBOL_H_
#define _OSMAND_CORE_ON_SURFACE_RASTER_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterMapSymbol.h>
#include <OsmAndCore/Map/IOnSurfaceMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API OnSurfaceRasterMapSymbol
        : public RasterMapSymbol
        , public IOnSurfaceMapSymbol
    {
        Q_DISABLE_COPY(OnSurfaceRasterMapSymbol);
    private:
    protected:
    public:
        OnSurfaceRasterMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable);
        virtual ~OnSurfaceRasterMapSymbol();

        float direction;
        virtual float getDirection() const;
        virtual void setDirection(const float direction);

        PointI position31;
        virtual PointI getPosition31() const;
        virtual void setPosition31(const PointI position);
    };
}

#endif // !defined(_OSMAND_CORE_ON_SURFACE_RASTER_MAP_SYMBOL_H_)
