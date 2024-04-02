#ifndef _OSMAND_CORE_MAP_MARKER_P_H_
#define _OSMAND_CORE_MAP_MARKER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QReadWriteLock>
#include <QHash>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "MapSymbolsGroup.h"
#include "IUpdatableMapSymbolsGroup.h"
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
        Q_DISABLE_COPY_AND_MOVE(MapMarker_P);

    public:
        typedef MapMarker::PinIconVerticalAlignment PinIconVerticalAlignment;
        typedef MapMarker::PinIconHorisontalAlignment PinIconHorisontalAlignment;

    private:
        std::shared_ptr<const TextRasterizer> textRasterizer;
    protected:
        MapMarker_P(MapMarker* const owner);

        mutable QReadWriteLock _lock;
        bool _hasUnappliedChanges;

        bool _isHidden;

        bool _isAccuracyCircleVisible;
        double _accuracyCircleRadius;

        PointI _position;

        float _height;

        QHash< MapMarker::OnSurfaceIconKey, float > _directions;

        ColorARGB _pinIconModulationColor;

        class KeyedOnSurfaceRasterMapSymbol : public OnSurfaceRasterMapSymbol
        {
        private:
        protected:
            KeyedOnSurfaceRasterMapSymbol(
                const MapMarker::OnSurfaceIconKey key,
                const std::shared_ptr<MapSymbolsGroup>& group);
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
                const std::shared_ptr<MapSymbolsGroup>& group);
        public:
            virtual ~AccuracyCircleMapSymbol();

        friend class OsmAnd::MapMarker_P;
        };

        bool applyChanges();

        std::shared_ptr<MapMarker::SymbolsGroup> inflateSymbolsGroup() const;
        mutable QReadWriteLock _symbolsGroupsRegistryLock;
        mutable QHash< MapSymbolsGroup*, std::weak_ptr< MapSymbolsGroup > > _symbolsGroupsRegistry;
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

        float getHeight() const;
        void setHeight(const float height);

        float getOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key) const;
        void setOnMapSurfaceIconDirection(const MapMarker::OnSurfaceIconKey key, const float direction);

        ColorARGB getPinIconModulationColor() const;
        void setPinIconModulationColor(const ColorARGB colorValue);

        bool hasUnappliedChanges() const;
        
        std::shared_ptr<MapMarker::SymbolsGroup> createSymbolsGroup() const;

    friend class OsmAnd::MapMarker;
    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkersCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_P_H_)
