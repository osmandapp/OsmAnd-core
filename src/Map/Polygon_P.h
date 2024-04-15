#ifndef _OSMAND_CORE_MAP_LINE_P_H_
#define _OSMAND_CORE_MAP_LINE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QReadWriteLock>
#include <QHash>
#include <QVector>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "MapSymbolsGroup.h"
#include "IUpdatableMapSymbolsGroup.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "Polygon.h"

namespace OsmAnd
{
    class PolygonsCollection;
    class PolygonsCollection_P;

    class Polygon;
    class Polygon_P : public std::enable_shared_from_this<Polygon_P>
    {
        Q_DISABLE_COPY_AND_MOVE(Polygon_P);

    private:
    protected:
        Polygon_P(Polygon* const owner);

        mutable QReadWriteLock _lock;
        bool _hasUnappliedChanges;
        bool _hasUnappliedPrimitiveChanges;

        bool _isHidden;

        QVector<PointI> _points;

        PointI _circleCenter;
        double _circleRadiusInMeters;

        double _metersPerPixel;
        ZoomLevel _mapZoomLevel;
        ZoomLevel _surfaceZoomLevel;
        float _mapVisualZoom;
        float _surfaceVisualZoom;
        float _mapVisualZoomShift;
        bool _hasElevationDataProvider;

        float zoom() const;

        bool update(const MapState& mapState);

        bool applyChanges();

        bool isMapStateChanged(const MapState& mapState) const;
        void applyMapState(const MapState& mapState);
        
        std::shared_ptr<Polygon::SymbolsGroup> inflateSymbolsGroup() const;
        mutable QReadWriteLock _symbolsGroupsRegistryLock;
        mutable QHash< MapSymbolsGroup*, std::weak_ptr< MapSymbolsGroup > > _symbolsGroupsRegistry;
        void registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const;
        void unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const;

        PointD getProjection(PointD point, PointD from, PointD to ) const;
        double scalarMultiplication(double xA, double yA, double xB, double yB, double xC, double yC) const;
        
        int simplifyDouglasPeucker(std::vector<PointD>& points, uint begin, uint end, double epsilon,
                                   std::vector<bool>& include) const;
        
    public:
        virtual ~Polygon_P();

        ImplementationInterface<Polygon> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI>& points);

        void setCircle(const PointI& center, const double radiusInMeters);
        void setZoomLevel(const ZoomLevel zoomLevel, const bool hasElevationDataProvider);
        void generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol>& polygon) const;

        bool hasUnappliedChanges() const;
        bool hasUnappliedPrimitiveChanges() const;

        std::shared_ptr<Polygon::SymbolsGroup> createSymbolsGroup(const MapState& mapState);

    friend class OsmAnd::Polygon;
    friend class OsmAnd::PolygonsCollection;
    friend class OsmAnd::PolygonsCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_LINE_P_H_)
