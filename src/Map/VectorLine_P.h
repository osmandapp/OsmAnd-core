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
#include "MapMarker.h"

namespace OsmAnd
{
    class VectorLinesCollection;
    class VectorLinesCollection_P;

    class VectorLine;
    class VectorLine_P : public std::enable_shared_from_this<VectorLine_P>
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLine_P);

    private:
        void createVertices(
            std::vector<VectorMapSymbol::Vertex> &vertices,
            VectorMapSymbol::Vertex &vertex,
            std::vector<OsmAnd::PointD> &original,
            double thickness,
            double startY31,
            float flatten,
            QList<float>& distances,
            FColorARGB &fillColor,
            QList<FColorARGB>& colorMapping,
            QList<FColorARGB>& outlineColorMapping,
            QList<float>& heights) const;
                
        void addArrowsOnFlatSegmentPath(
            const std::vector<PointI>& segmentPoints,
            const QList<float>& segmentDistances,
            const QList<float>& segmentHeights,
            const std::vector<bool>& includedPoints,
            const PointI64& arrowsOrigin);

        void addArrowsOnSegmentPath(
            const std::vector<PointI>& segmentPoints,
            const QList<float>& segmentDistances,
            const QList<float>& segmentHeights,
            const std::vector<bool>& includedPoints,
            const float distance);
            
        QList<VectorLine::OnPathSymbolData> _arrowsOnPath;
        sk_sp<const SkImage> _scaledPathIcon;
        
        double getRadius(double width) const;
        bool useSpecialArrow() const;
        double getPointStepPx() const;
        
        bool hasColorizationMapping() const;
        bool hasOutlineColorizationMapping() const;
        bool hasHeights() const;
    protected:
        VectorLine_P(VectorLine* const owner);
        
        QVector<std::shared_ptr<MapMarker>> _attachedMarkers;

        mutable QReadWriteLock _lock;
        mutable QReadWriteLock _arrowsOnPathLock;
        bool _hasUnappliedChanges;
        bool _hasUnappliedPrimitiveChanges;
        bool _hasUnappliedStartingDistance;

        bool _isHidden;
        float _startingDistance;
        bool _isApproximationEnabled;
        bool _showArrows;

        QVector<PointI> _points;
        QList<float> _heights;
        QList<FColorARGB> _colorizationMapping;
        QList<FColorARGB> _outlineColorizationMapping;
        int _colorizationScheme;
        double _lineWidth;
        double _outlineWidth;
        FColorARGB _fillColor;
        FColorARGB _nearOutlineColor;
        FColorARGB _farOutlineColor;
        float _pathIconStep;
        float _specialPathIconStep;
        bool _dash;
        std::vector<double> _dashPattern;

        bool _isElevatedLineVisible;
        bool _isSurfaceLineVisible;

        float _elevationScaleFactor;

        double _metersPerPixel;
        AreaI _visibleBBoxShifted;
        PointI _target31;
        ZoomLevel _mapZoomLevel;
        ZoomLevel _surfaceZoomLevel;
        float _mapVisualZoom;
        float _surfaceVisualZoom;
        float _mapVisualZoomShift;
        bool _hasElevationDataProvider;
        bool _hasElevationDataResources;
        bool _flatEarth;
        AreaI64 _bboxShifted;

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
        
        bool generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol> vectorLine);
        
        void clearArrowsOnPath();
        const QList<OsmAnd::VectorLine::OnPathSymbolData> getArrowsOnPath() const;

        double correctDistance(double y, double startY31, double flatten, double radius) const;
        
        PointD getProjection(PointD point, PointD from, PointD to ) const;
        double scalarMultiplication(double xA, double yA, double xB, double yB, double xC, double yC) const;
        
        int simplifyDouglasPeucker(
            const std::vector<PointI>& points,
            const uint begin,
            const uint end,
            const double epsilon,
            const double startY31,
            const double flatten,
            std::vector<bool>& include) const;
        bool forceIncludePoint(const QList<FColorARGB>& pointsColors, const uint pointIndex) const;
        bool isBigDiff(const ColorARGB& firstColor, const ColorARGB& secondColor) const;
        inline FColorARGB middleColor(const FColorARGB& first, const FColorARGB& last, const float factor) const;
        PointI calculateVisibleSegments(
            const AreaI64& visibleArea64,
            std::vector<std::vector<PointI>>& segments,
            QList<QList<float>>& segmentDistances,
            QList<QList<FColorARGB>>& segmentColors,
            QList<QList<FColorARGB>>& segmentOutlineColors,
            QList<QList<float>>& segmentHeights,
            float& totalDistance);
        static bool calculateIntersection(const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX);

    public:
        virtual ~VectorLine_P();

        ImplementationInterface<VectorLine> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        float getStartingDistance() const;
        void setStartingDistance(const float distanceInMeters);

        float getArrowStartingGap() const;

        bool showArrows() const;
        void setShowArrows(const bool showArrows);

        bool isApproximationEnabled() const;
        void setApproximationEnabled(const bool enabled);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI>& points);
        
        QVector<PointI64> getShortestPathPoints() const;
        
        void attachMarker(const std::shared_ptr<MapMarker>& marker);
        void detachMarker(const std::shared_ptr<MapMarker>& marker);
        QVector<std::shared_ptr<OsmAnd::MapMarker>> getAttachedMarkers() const;

        QList<float> getHeights() const;
        void setHeights(const QList<float>& heights);
        
        QList<FColorARGB> getColorizationMapping() const;
        void setColorizationMapping(const QList<FColorARGB>& colorizationMapping);

        QList<FColorARGB> getOutlineColorizationMapping() const;
        void setOutlineColorizationMapping(const QList<FColorARGB>& colorizationMapping);

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

        FColorARGB getNearOutlineColor() const;
        void setNearOutlineColor(const FColorARGB color);

        FColorARGB getFarOutlineColor() const;
        void setFarOutlineColor(const FColorARGB color);

        void setColorizationScheme(const int colorizationScheme);

        bool getElevatedLineVisibility() const;
        void setElevatedLineVisibility(const bool visible);

        bool getSurfaceLineVisibility() const;
        void setSurfaceLineVisibility(const bool visible);

        float getElevationScaleFactor() const;
        void setElevationScaleFactor(const float scaleFactor);

        bool hasUnappliedChanges() const;
        bool hasUnappliedPrimitiveChanges() const;

        std::shared_ptr<VectorLine::SymbolsGroup> createSymbolsGroup(const MapState& mapState);

    friend class OsmAnd::VectorLine;
    friend class OsmAnd::VectorLinesCollection;
    friend class OsmAnd::VectorLinesCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_LINE_P_H_)
