#ifndef _OSMAND_CORE_MAP_MARKER_BUILDER_H_
#define _OSMAND_CORE_MAP_MARKER_BUILDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Map/MapMarker.h>

class SkBitmap;

namespace OsmAnd
{
    class MapMarkersCollection;

    class MapMarkerBuilder_P;
    class OSMAND_CORE_API MapMarkerBuilder
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarkerBuilder);

    private:
        PrivateImplementation<MapMarkerBuilder_P> _p;
    protected:
    public:
        MapMarkerBuilder();
        virtual ~MapMarkerBuilder();

        bool isHidden() const;
        MapMarkerBuilder& setIsHidden(const bool hidden);

        int getBaseOrder() const;
        MapMarkerBuilder& setBaseOrder(const int baseOrder);

        bool isAccuracyCircleSupported() const;
        MapMarkerBuilder& setIsAccuracyCircleSupported(const bool supported);
        bool isAccuracyCircleVisible() const;
        MapMarkerBuilder& setIsAccuracyCircleVisible(const bool visible);
        double getAccuracyCircleRadius() const;
        MapMarkerBuilder& setAccuracyCircleRadius(const double radius);
        FColorRGB getAccuracyCircleBaseColor() const;
        MapMarkerBuilder& setAccuracyCircleBaseColor(const FColorRGB baseColor);

        PointI getPosition() const;
        MapMarkerBuilder& setPosition(const PointI position);

        std::shared_ptr<const SkBitmap> getPinIcon() const;
        MapMarkerBuilder& setPinIcon(const std::shared_ptr<const SkBitmap>& bitmap);
        MapMarker::PinIconVerticalAlignment getPinIconVerticalAlignment() const;
        MapMarker::PinIconHorisontalAlignment getPinIconHorisontalAlignment() const;
        MapMarkerBuilder& setPinIconVerticalAlignment(const MapMarker::PinIconVerticalAlignment value);
        MapMarkerBuilder& setPinIconHorisontalAlignment(const MapMarker::PinIconHorisontalAlignment value);
        ColorARGB getPinIconModulationColor() const;
        MapMarkerBuilder& setPinIconModulationColor(const ColorARGB colorValue);

        QHash< MapMarker::OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > getOnMapSurfaceIcons() const;
        MapMarkerBuilder& addOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key, const std::shared_ptr<const SkBitmap>& bitmap);
        MapMarkerBuilder& removeOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key);
        MapMarkerBuilder& clearOnMapSurfaceIcons();

        std::shared_ptr<MapMarker> buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_BUILDER_H_)
