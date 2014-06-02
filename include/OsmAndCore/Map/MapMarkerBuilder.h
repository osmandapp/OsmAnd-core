#ifndef _OSMAND_CORE_MAP_MARKER_BUILDER_H_
#define _OSMAND_CORE_MAP_MARKER_BUILDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class MapMarker;
    class MapMarkersCollection;

    class MapMarkerBuilder_P;
    class OSMAND_CORE_API MapMarkerBuilder
    {
        Q_DISABLE_COPY(MapMarkerBuilder);

    private:
        PrivateImplementation<MapMarkerBuilder_P> _p;
    protected:
    public:
        MapMarkerBuilder();
        virtual ~MapMarkerBuilder();

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        int getBaseOrder() const;
        void setBaseOrder(const int baseOrder);

        bool isPrecisionCircleEnabled() const;
        void setIsPrecisionCircleEnabled(const bool enabled);
        double getPrecisionCircleRadius() const;
        void setPrecisionCircleRadius(const double radius);
        SkColor getPrecisionCircleBaseColor() const;
        void setPrecisionCircleBaseColor(const SkColor baseColor);

        PointI getPosition() const;
        void setPosition(const PointI position);

        float getDirection() const;
        void setDirection(const float direction);
        
        std::shared_ptr<const SkBitmap> getPinIcon() const;
        void setPinIcon(const std::shared_ptr<const SkBitmap>& bitmap);

        std::shared_ptr<MapMarker> buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_BUILDER_H_)
