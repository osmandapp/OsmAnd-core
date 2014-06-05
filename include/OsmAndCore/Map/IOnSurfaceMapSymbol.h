#ifndef _OSMAND_CORE_I_ON_SURFACE_MAP_SYMBOL_H_
#define _OSMAND_CORE_I_ON_SURFACE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IOnSurfaceMapSymbol
    {
        Q_DISABLE_COPY(IOnSurfaceMapSymbol);
    private:
    protected:
        IOnSurfaceMapSymbol();
    public:
        virtual ~IOnSurfaceMapSymbol();

        // NaN value is considered as "aligned to azimuth"
        virtual float getDirection() const = 0;
        virtual void setDirection(const float direction) = 0;
        bool isAzimuthAlignedDirection() const;

        virtual PointI getPosition31() const = 0;
        virtual void setPosition31(const PointI position) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_ON_SURFACE_MAP_SYMBOL_H_)
