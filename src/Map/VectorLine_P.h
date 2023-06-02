#ifndef _OSMAND_CORE_MAP_LINE_P_H_
#define _OSMAND_CORE_MAP_LINE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QReadWriteLock>
#include <QHash>
#include <QVector>
#include <SkPath.h>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "MapSymbolsGroup.h"
#include "IUpdatableMapSymbolsGroup.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "VectorLine.h"

namespace OsmAnd
{
    class VectorLinesCollection;
    class VectorLinesCollection_P;

    class VectorLine;
    class VectorLine_P : public std::enable_shared_from_this<VectorLine_P>
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLine_P);

    private:
        void createVertexes(
            std::vector<VectorMapSymbol::Vertex> &vertices,
            VectorMapSymbol::Vertex &vertex,
            std::vector<OsmAnd::PointD> &original,
            double radius,
            FColorARGB &fillColor,
            QList<FColorARGB>& colorMapping) const;
                
        void addArrowsOnSegmentPath(
            const std::vector<PointI>& segmentPoints,
            const std::vector<bool>& includedPoints,
            const PointI& arrowsOrigin);
        
        QList<VectorLine::OnPathSymbolData> _arrowsOnPath;
        sk_sp<const SkImage> _scaledPathIcon;
        
        double getRadius(double width) const;
        bool useSpecialArrow() const;
        double getPointStepPx() const;
        
        bool hasColorizationMapping() const;
    protected:
        VectorLine_P(VectorLine* const owner);

        mutable QReadWriteLock _lock;
        bool _hasUnappliedChanges;
        bool _hasUnappliedPrimitiveChanges;

        bool _isHidden;
        bool _isApproximationEnabled;
        bool _showArrows;

        QVector<PointI> _points;
        QList<FColorARGB> _colorizationMapping;
        int _colorizationSceme;
        double _lineWidth;
        double _outlineWidth;
        FColorARGB _fillColor;
        FColorARGB _outlineColor;
        float _pathIconStep;
        float _specialPathIconStep;
        bool _dash;
        std::vector<double> _dashPattern;

        double _metersPerPixel;
        AreaI _visibleBBoxShifted;
        ZoomLevel _mapZoomLevel;
        float _mapVisualZoom;
        float _mapVisualZoomShift;
        bool _hasElevationDataProvider;

        float zoom() const;

        bool update(const MapState& mapState);

        bool applyChanges();

        bool isMapStateChanged(const MapState& mapState) const;
        void applyMapState(const MapState& mapState);
        
        std::shared_ptr<VectorLine::SymbolsGroup> inflateSymbolsGroup();
        mutable QReadWriteLock _symbolsGroupsRegistryLock;
        mutable QHash< MapSymbolsGroup*, std::weak_ptr< MapSymbolsGroup > > _symbolsGroupsRegistry;
        void registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const;
        void unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const;

        sk_sp<const SkImage> getPointImage() const;
        
        std::shared_ptr<OnSurfaceVectorMapSymbol> generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol> vectorLine);
        
        const QList<OsmAnd::VectorLine::OnPathSymbolData> getArrowsOnPath() const;

        PointD findLineIntersection(PointD p1, PointD p2, PointD p3, PointD p4) const;
        
        PointD getProjection(PointD point, PointD from, PointD to ) const;
        double scalarMultiplication(double xA, double yA, double xB, double yB, double xC, double yC) const;
        
        int simplifyDouglasPeucker(
            const std::vector<PointI>& points,
            const uint begin,
            const uint end,
            const double epsilon,
            std::vector<bool>& include) const;
        bool forceIncludePoint(const QList<FColorARGB>& pointsColors, const uint pointIndex) const;
        bool isBigDiff(const ColorARGB& firstColor, const ColorARGB& secondColor) const;
        inline FColorARGB middleColor(const FColorARGB& first, const FColorARGB& last, const float factor) const;
        void calculateVisibleSegments(std::vector<std::vector<PointI>>& segments, QList<QList<FColorARGB>>& segmentColors) const;
        static bool calculateIntersection(const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX);

    public:
        virtual ~VectorLine_P();

        ImplementationInterface<VectorLine> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);
        
        bool showArrows() const;
        void setShowArrows(const bool showArrows);

        bool isApproximationEnabled() const;
        void setApproximationEnabled(const bool enabled);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI>& points);
        
        QList<FColorARGB> getColorizationMapping() const;
        void setColorizationMapping(const QList<FColorARGB>& colorizationMapping);
        
        double getLineWidth() const;
        void setLineWidth(const double width);

        float getPathIconStep() const;
        void setPathIconStep(const float step);

        float getSpecialPathIconStep() const;
        void setSpecialPathIconStep(const float step);

        FColorARGB getFillColor() const;
        void setFillColor(const FColorARGB color);

        std::vector<double> getLineDash() const;
        void setLineDash(const std::vector<double> dashPattern);
        
        double getOutlineWidth() const;
        void setOutlineWidth(const double width);

        FColorARGB getOutlineColor() const;
        void setOutlineColor(const FColorARGB color);

        void setColorizationScheme(const int colorizationScheme);

        bool hasUnappliedChanges() const;
        bool hasUnappliedPrimitiveChanges() const;

        std::shared_ptr<VectorLine::SymbolsGroup> createSymbolsGroup(const MapState& mapState);

    friend class OsmAnd::VectorLine;
    friend class OsmAnd::VectorLinesCollection;
    friend class OsmAnd::VectorLinesCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_LINE_P_H_)
