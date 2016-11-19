#ifndef _OSMAND_CORE_MAP_MARKER_BUILDER_P_H_
#define _OSMAND_CORE_MAP_MARKER_BUILDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QHash>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "MapMarker.h"

class SkBitmap;

namespace OsmAnd
{
    class MapMarkersCollection;
    class MapMarkersCollection_P;

    class MapMarkerBuilder;
    class MapMarkerBuilder_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarkerBuilder_P);

    private:
    protected:
        MapMarkerBuilder_P(MapMarkerBuilder* const owner);

        mutable QReadWriteLock _lock;

        bool _isHidden;

        int _baseOrder;

        bool _isAccuracyCircleSupported;
        bool _isAccuracyCircleVisible;
        double _accuracyCircleRadius;
        FColorRGB _accuracyCircleBaseColor;

        PointI _position;

        float _direction;

        std::shared_ptr<const SkBitmap> _pinIcon;
        MapMarker::PinIconVerticalAlignment _pinIconVerticalAlignment;
        MapMarker::PinIconHorisontalAlignment _pinIconHorisontalAlignment;
        ColorARGB _pinIconModulationColor;

        QHash< MapMarker::OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > _onMapSurfaceIcons;
    public:
        virtual ~MapMarkerBuilder_P();

        ImplementationInterface<MapMarkerBuilder> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        int getBaseOrder() const;
        void setBaseOrder(const int baseOrder);

        bool isAccuracyCircleSupported() const;
        void setIsAccuracyCircleSupported(const bool supported);
        bool isAccuracyCircleVisible() const;
        void setIsAccuracyCircleVisible(const bool visible);
        double getAccuracyCircleRadius() const;
        void setAccuracyCircleRadius(const double radius);
        FColorRGB getAccuracyCircleBaseColor() const;
        void setAccuracyCircleBaseColor(const FColorRGB baseColor);

        PointI getPosition() const;
        void setPosition(const PointI position);

        std::shared_ptr<const SkBitmap> getPinIcon() const;
        void setPinIcon(const std::shared_ptr<const SkBitmap>& bitmap);

        MapMarker::PinIconVerticalAlignment getPinIconVerticalAlignment() const;
        MapMarker::PinIconHorisontalAlignment getPinIconHorisontalAlignment() const;
        void setPinIconVerticalAlignment(const MapMarker::PinIconVerticalAlignment value);
        void setPinIconHorisontalAlignment(const MapMarker::PinIconHorisontalAlignment value);

        ColorARGB getPinIconModulationColor() const;
        void setPinIconModulationColor(const ColorARGB colorValue);

        QHash< MapMarker::OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > getOnMapSurfaceIcons() const;
        void addOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key, const std::shared_ptr<const SkBitmap>& bitmap);
        void removeOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key);
        void clearOnMapSurfaceIcons();

        std::shared_ptr<MapMarker> buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection);

    friend class OsmAnd::MapMarkerBuilder;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_BUILDER_P_H_)
