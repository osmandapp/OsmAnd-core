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
        Q_DISABLE_COPY_AND_MOVE(OnSurfaceRasterMapSymbol);
    private:
    protected:
    public:
        OnSurfaceRasterMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
        virtual ~OnSurfaceRasterMapSymbol();

        float direction;
        virtual float getDirection() const;
        virtual void setDirection(const float direction);

        PointI position31;
        virtual PointI getPosition31() const;
        virtual void setPosition31(const PointI position);

        float elevation;
        float getElevation() const;
        void setElevation(const float elevation);

        float elevationScaleFactor;
        float getElevationScaleFactor() const;
        void setElevationScaleFactor(const float scaleFactor);
    };
}

#endif // !defined(_OSMAND_CORE_ON_SURFACE_RASTER_MAP_SYMBOL_H_)
