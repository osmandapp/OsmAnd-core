#ifndef _OSMAND_CORE_MAP_MARKER_BUILDER_P_H_
#define _OSMAND_CORE_MAP_MARKER_BUILDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QReadWriteLock>

#include <SkBitmap.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"

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

        SkBitmap _pinIconBitmap;

        QList< QPair<const SkBitmap, bool> > _mapIconsBitmaps;
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

        float getDirection() const;
        void setDirection(const float direction);

        const SkBitmap getPinIcon() const;
        void setPinIcon(const SkBitmap& bitmap);

        QList< QPair<const SkBitmap, bool> > getMapIcons() const;
        void clearMapIcons();
        void addMapIcon(const SkBitmap& bitmap, const bool respectsDirection);

        std::shared_ptr<MapMarker> buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection);

    friend class OsmAnd::MapMarkerBuilder;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_BUILDER_P_H_)
