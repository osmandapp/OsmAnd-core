#ifndef _OSMAND_CORE_MAP_MARKER_P_H_
#define _OSMAND_CORE_MAP_MARKER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QReadWriteLock>
#include <QHash>

#include <SkBitmap.h>
#include <SkColor.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "MapSymbolsGroup.h"

namespace OsmAnd
{
    class MapMarkersCollection;
    class MapMarkersCollection_P;

    class MapMarker;
    class MapMarker_P : public std::enable_shared_from_this<MapMarker_P>
    {
        Q_DISABLE_COPY(MapMarker_P);

    private:
    protected:
        MapMarker_P(MapMarker* const owner);

        mutable QReadWriteLock _lock;
        bool _hasUnappliedChanges;

        bool _isHidden;

        bool _isPrecisionCircleEnabled;
        double _precisionCircleRadius;
        SkColor _precisionCircleBaseColor;

        PointI _position;

        float _direction;

        class LinkedMapSymbolsGroup
            : public MapSymbolsGroup
            , public IUpdatableMapSymbolsGroup
        {
        private:
        protected:
            LinkedMapSymbolsGroup(const std::shared_ptr<MapMarker_P>& mapMarkerP);
        public:
            virtual ~LinkedMapSymbolsGroup();

            const std::weak_ptr<MapMarker_P> mapMarkerP;

            virtual void update();

        friend class OsmAnd::MapMarker_P;
        };

        bool applyChanges();

        std::shared_ptr<MapSymbolsGroup> inflateSymbolsGroup() const;
        mutable QReadWriteLock _symbolsGroupsRegisterLock;
        mutable QHash< MapSymbolsGroup*, std::weak_ptr< MapSymbolsGroup > > _symbolsGroupsRegister;
        void registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const;
        void unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const;
    public:
        virtual ~MapMarker_P();

        ImplementationInterface<MapMarker> owner;

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

    friend class OsmAnd::MapMarker;
    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkersCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_P_H_)
