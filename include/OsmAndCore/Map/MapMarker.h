#ifndef _OSMAND_CORE_MAP_MARKER_H_
#define _OSMAND_CORE_MAP_MARKER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IUpdatableMapSymbolsGroup.h>

class SkBitmap;

namespace OsmAnd
{
    class MapMarkerBuilder;
    class MapMarkerBuilder_P;
    class MapMarkersCollection;
    class MapMarkersCollection_P;

    class MapMarker_P;
    class OSMAND_CORE_API MapMarker
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarker);

    public:
        typedef const void* OnSurfaceIconKey;
        enum PinIconAlignment : unsigned int
        {
            XAxisMask = 0x3,
            Left = 0u << 0,
            CenterHorizontal = 1u << 0,
            Right = 2u << 0,

            YAxisMask = 0xC,
            Top = 0u << 2,
            CenterVertical = 1u << 2,
            Bottom = 2u << 2,

            Center = CenterHorizontal | CenterVertical,
            CenterTop = CenterHorizontal | Top,
        };

        class SymbolsGroup
            : public MapSymbolsGroup
            , public IUpdatableMapSymbolsGroup
        {
        private:
            const std::weak_ptr<MapMarker_P> _mapMarkerP;
        protected:
            SymbolsGroup(const std::shared_ptr<MapMarker_P>& mapMarkerP);
        public:
            virtual ~SymbolsGroup();

            const MapMarker* getMapMarker() const;

            virtual bool updatesPresent();
            virtual bool update();

        friend class OsmAnd::MapMarker;
        friend class OsmAnd::MapMarker_P;
        };

    private:
        PrivateImplementation<MapMarker_P> _p;
    protected:
        MapMarker(
            const int baseOrder,
            const std::shared_ptr<const SkBitmap>& pinIcon,
            const PinIconAlignment pinIconAlignment,
            const QHash< OnSurfaceIconKey, std::shared_ptr<const SkBitmap> >& onMapSurfaceIcons,
            const bool isAccuracyCircleSupported,
            const FColorRGB accuracyCircleBaseColor);

        bool applyChanges();
    public:
        virtual ~MapMarker();

        const int baseOrder;
        const std::shared_ptr<const SkBitmap> pinIcon;
        const PinIconAlignment pinIconAlignment;
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

        std::shared_ptr<SymbolsGroup> createSymbolsGroup() const;

    friend class OsmAnd::MapMarkerBuilder;
    friend class OsmAnd::MapMarkerBuilder_P;
    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkersCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_H_)
