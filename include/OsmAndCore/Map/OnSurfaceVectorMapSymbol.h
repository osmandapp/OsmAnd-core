#ifndef _OSMAND_CORE_ON_SURFACE_VECTOR_MAP_SYMBOL_H_
#define _OSMAND_CORE_ON_SURFACE_VECTOR_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/VectorMapSymbol.h>
#include <OsmAndCore/Map/IOnSurfaceMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API OnSurfaceVectorMapSymbol
        : public VectorMapSymbol
        , public IOnSurfaceMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(OnSurfaceVectorMapSymbol);

    private:
    protected:
    public:
        OnSurfaceVectorMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
        virtual ~OnSurfaceVectorMapSymbol();

        float direction;
        virtual float getDirection() const;
        virtual void setDirection(const float direction);

        PointI position31;
        virtual PointI getPosition31() const;
        virtual void setPosition31(const PointI position);

        float elevationScaleFactor;
        virtual float getElevationScaleFactor() const;
        virtual void setElevationScaleFactor(const float scaleFactor);
    };
}

#endif // !defined(_OSMAND_CORE_ON_SURFACE_VECTOR_MAP_SYMBOL_H_)
