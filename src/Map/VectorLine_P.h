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
#include "VectorLine.h"

class SkBitmap;

namespace OsmAnd
{
    class VectorLinesCollection;
    class VectorLinesCollection_P;

    class VectorLine;
    class VectorLine_P : public std::enable_shared_from_this<VectorLine_P>
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLine_P);

    private:
        void createVertexes(std::vector<VectorMapSymbol::Vertex> &vertices,
                            VectorMapSymbol::Vertex &vertex,
                            std::vector<OsmAnd::PointD> &b1,
                            std::vector<OsmAnd::PointD> &b2,
                            std::vector<OsmAnd::PointD> &e1,
                            std::vector<OsmAnd::PointD> &e2,
                            std::vector<OsmAnd::PointD> &original,
                            double radius,
                            FColorARGB &fillColor,
                            bool gap) const;
    protected:
        VectorLine_P(VectorLine* const owner);

        mutable QReadWriteLock _lock;
        bool _hasUnappliedChanges;
        bool _hasUnappliedPrimitiveChanges;

        bool _isHidden;

        QVector<PointI> _points;
        FColorARGB _fillColor;
        bool _dash;
        std::vector<double> _dashPattern;

        double _metersPerPixel;
        ZoomLevel _mapZoomLevel;
        float _mapVisualZoom;
        float _mapVisualZoomShift;

        float zoom() const;

        bool update(const MapState& mapState);

        bool applyChanges();

        bool isMapStateChanged(const MapState& mapState) const;
        void applyMapState(const MapState& mapState);
        
        std::shared_ptr<VectorLine::SymbolsGroup> inflateSymbolsGroup() const;
        mutable QReadWriteLock _symbolsGroupsRegistryLock;
        mutable QHash< MapSymbolsGroup*, std::weak_ptr< MapSymbolsGroup > > _symbolsGroupsRegistry;
        void registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const;
        void unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const;

        std::shared_ptr<OnSurfaceVectorMapSymbol> generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol> vectorLine) const;
        void generateArrowsOnPath(const std::shared_ptr<VectorLine::SymbolsGroup> symbolsGroup) const;

        PointD findLineIntersection(PointD p1, PointD p2, PointD p3, PointD p4) const;
        
        PointD getProjection(PointD point, PointD from, PointD to ) const;
        double scalarMultiplication(double xA, double yA, double xB, double yB, double xC, double yC) const;
        
        int simplifyDouglasPeucker(std::vector<PointD>& points, uint begin, uint end, double epsilon,
                                   std::vector<bool>& include) const;
        
    public:
        virtual ~VectorLine_P();

        ImplementationInterface<VectorLine> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI>& points);
        
        FColorARGB getFillColor() const;
        void setFillColor(const FColorARGB color);

        std::vector<double> getLineDash() const;
        void setLineDash(const std::vector<double> dashPattern);

        bool hasUnappliedChanges() const;
        bool hasUnappliedPrimitiveChanges() const;

        std::shared_ptr<VectorLine::SymbolsGroup> createSymbolsGroup(const MapState& mapState);

    friend class OsmAnd::VectorLine;
    friend class OsmAnd::VectorLinesCollection;
    friend class OsmAnd::VectorLinesCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_LINE_P_H_)
