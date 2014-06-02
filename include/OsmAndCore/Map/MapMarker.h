#ifndef _OSMAND_CORE_MAP_MARKER_H_
#define _OSMAND_CORE_MAP_MARKER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <SkBitmap.h>
#include <SkColor.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class MapMarkerBuilder;
    class MapMarkerBuilder_P;
    class MapMarkersCollection;
    class MapMarkersCollection_P;
    class MapSymbolsGroup;
    
    class MapMarker_P;
    class OSMAND_CORE_API MapMarker
    {
        Q_DISABLE_COPY(MapMarker);

    private:
        PrivateImplementation<MapMarker_P> _p;
    protected:
        MapMarker(
            const int baseOrder,
            const std::shared_ptr<const SkBitmap>& pinIcon);

        bool applyChanges();
    public:
        virtual ~MapMarker();

        const int baseOrder;
        const std::shared_ptr<const SkBitmap> pinIcon;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

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

        bool hasUnappliedChanges() const;

        std::shared_ptr<MapSymbolsGroup> createSymbolsGroup() const;

    friend class OsmAnd::MapMarkerBuilder;
    friend class OsmAnd::MapMarkerBuilder_P;
    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkersCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_H_)
