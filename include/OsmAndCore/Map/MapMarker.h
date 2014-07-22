#ifndef _OSMAND_CORE_MAP_MARKER_H_
#define _OSMAND_CORE_MAP_MARKER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>

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

    public:
        typedef uintptr_t OnSurfaceIconKey;

    private:
        PrivateImplementation<MapMarker_P> _p;
    protected:
        MapMarker(
            const int baseOrder,
            const std::shared_ptr<const SkBitmap>& pinIcon,
            const QHash< OnSurfaceIconKey, std::shared_ptr<const SkBitmap> >& onMapSurfaceIcons,
            const bool isAccuracyCircleSupported,
            const FColorRGB accuracyCircleBaseColor);

        bool applyChanges();
    public:
        virtual ~MapMarker();

        const int baseOrder;
        const std::shared_ptr<const SkBitmap> pinIcon;
        const QHash< OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > onMapSurfaceIcons;
        const bool isAccuracyCircleSupported;
        const FColorRGB accuracyCircleBaseColor;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        bool isAccuracyCircleVisible() const;
        void setIsAccuracyCircleVisible(const bool visible);
        double getAccuracyCircleRadius() const;
        void setAccuracyCircleRadius(const double radius);

        PointI getPosition() const;
        void setPosition(const PointI position);

        float getOnMapSurfaceIconDirection(const OnSurfaceIconKey key) const;
        void setOnMapSurfaceIconDirection(const OnSurfaceIconKey key, const float direction);

        ColorARGB getPinIconModulationColor() const;
        void setPinIconModulationColor(const ColorARGB colorValue);

        bool hasUnappliedChanges() const;

        std::shared_ptr<MapSymbolsGroup> createSymbolsGroup() const;

    friend class OsmAnd::MapMarkerBuilder;
    friend class OsmAnd::MapMarkerBuilder_P;
    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkersCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_H_)
