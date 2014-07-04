#ifndef _OSMAND_CORE_MAP_MARKER_BUILDER_H_
#define _OSMAND_CORE_MAP_MARKER_BUILDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapMarker.h>

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
        FColorRGB getPrecisionCircleBaseColor() const;
        void setPrecisionCircleBaseColor(const FColorRGB baseColor);

        PointI getPosition() const;
        void setPosition(const PointI position);

        std::shared_ptr<const SkBitmap> getPinIcon() const;
        void setPinIcon(const std::shared_ptr<const SkBitmap>& bitmap);
        ColorARGB getPinIconModulationColor() const;
        void setPinIconModulationColor(const ColorARGB colorValue);

        QHash< MapMarker::OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > getOnMapSurfaceIcons() const;
        void addOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key, const std::shared_ptr<const SkBitmap>& bitmap);
        void removeOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key);
        void clearOnMapSurfaceIcons();

        std::shared_ptr<MapMarker> buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_BUILDER_H_)
