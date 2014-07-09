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
#include "OnSurfaceRasterMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "MapMarker.h"

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

        bool _isAccuracyCircleVisible;
        double _accuracyCircleRadius;

        PointI _position;

        QHash< MapMarker::OnSurfaceIconKey, float > _directions;

        ColorARGB _pinIconModulationColor;

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

            virtual bool update();

        friend class OsmAnd::MapMarker_P;
        };

        class KeyedOnSurfaceRasterMapSymbol : public OnSurfaceRasterMapSymbol
        {
        private:
        protected:
            KeyedOnSurfaceRasterMapSymbol(
                const MapMarker::OnSurfaceIconKey key,
                const std::shared_ptr<MapSymbolsGroup>& group,
                const bool isShareable,
                const int order,
                const IntersectionModeFlags intersectionModeFlags,
                const std::shared_ptr<const SkBitmap>& bitmap,
                const QString& content,
                const LanguageId& languageId,
                const PointI& minDistance,
                const PointI& location31);
        public:
            virtual ~KeyedOnSurfaceRasterMapSymbol();

            const MapMarker::OnSurfaceIconKey key;

        friend class OsmAnd::MapMarker_P;
        };

        class AccuracyCircleMapSymbol : public OnSurfaceVectorMapSymbol
        {
        private:
        protected:
            AccuracyCircleMapSymbol(
                const std::shared_ptr<MapSymbolsGroup>& group,
                const bool isShareable,
                const int order,
                const IntersectionModeFlags intersectionModeFlags,
                const PointI& position31);
        public:
            virtual ~AccuracyCircleMapSymbol();

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

        bool isAccuracyCircleVisible() const;
        void setIsAccuracyCircleVisible(const bool visible);
        double getAccuracyCircleRadius() const;
        void setAccuracyCircleRadius(const double radius);

        PointI getPosition() const;
        void setPosition(const PointI position);

        float getOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key) const;
        void setOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key, const float direction);

        ColorARGB getPinIconModulationColor() const;
        void setPinIconModulationColor(const ColorARGB colorValue);

        bool hasUnappliedChanges() const;
        
        std::shared_ptr<MapSymbolsGroup> createSymbolsGroup() const;

    friend class OsmAnd::MapMarker;
    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkersCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_P_H_)
