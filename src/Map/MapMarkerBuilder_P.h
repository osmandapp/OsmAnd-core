#ifndef _OSMAND_CORE_MAP_MARKER_BUILDER_P_H_
#define _OSMAND_CORE_MAP_MARKER_BUILDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QHash>
#include <QReadWriteLock>

#include <SkBitmap.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "MapMarker.h"

namespace OsmAnd
{
    class MapMarker;
    class MapMarkersCollection;
    class MapMarkersCollection_P;

    class MapMarkerBuilder;
    class MapMarkerBuilder_P
    {
        Q_DISABLE_COPY(MapMarkerBuilder_P);

    private:
    protected:
        MapMarkerBuilder_P(MapMarkerBuilder* const owner);

        mutable QReadWriteLock _lock;

        bool _isHidden;

        int _baseOrder;

        bool _isPrecisionCircleEnabled;
        double _precisionCircleRadius;
        SkColor _precisionCircleBaseColor;

        PointI _position;

        float _direction;

        std::shared_ptr<const SkBitmap> _pinIcon;

        QHash< MapMarker::OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > _onMapSurfaceIcons;
    public:
        virtual ~MapMarkerBuilder_P();

        ImplementationInterface<MapMarkerBuilder> owner;

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

        std::shared_ptr<const SkBitmap> getPinIcon() const;
        void setPinIcon(const std::shared_ptr<const SkBitmap>& bitmap);

        QHash< MapMarker::OnSurfaceIconKey, std::shared_ptr<const SkBitmap> > getOnMapSurfaceIcons() const;
        void addOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key, const std::shared_ptr<const SkBitmap>& bitmap);
        void removeOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key);
        void clearOnMapSurfaceIcons();

        std::shared_ptr<MapMarker> buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection);

    friend class OsmAnd::MapMarkerBuilder;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_BUILDER_P_H_)
