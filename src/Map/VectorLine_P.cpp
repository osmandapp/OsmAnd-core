#include "VectorLine_P.h"
#include "VectorLine.h"
#include "Utilities.h"

#include "ignore_warnings_on_external_includes.h"
#include "restore_internal_warnings.h"

#include <SkBitmap.h>
#include <SkPathMeasure.h>

#include "MapSymbol.h"
#include "MapSymbolsGroup.h"
#include "VectorMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "QKeyValueIterator.h"
#include "Logging.h"
#include "AtlasMapRenderer.h"
#include "SkiaUtilities.h"
#include "GeometryModifiers.h"

#include <Polyline2D/Polyline2D.h>
#include <Polyline2D/Vec2.h>

#define SPECIAL_ARROW_DISTANCE_MULTIPLIER 2.5f

#define VECTOR_LINE_SCALE_COEF 2.0f
#define LINE_WIDTH_THRESHOLD_DP 22.0f * VECTOR_LINE_SCALE_COEF

// Colorization shemes
#define COLORIZATION_NONE 0
#define COLORIZATION_GRADIENT 1
#define COLORIZATION_SOLID 2


// The smaller delta, the less line is simplified and the more time it takes to generate primitives
#define MIN_ALPHA_DELTA 0.1f
#define MIN_RGB_DELTA 0.075f

OsmAnd::VectorLine_P::VectorLine_P(VectorLine* const owner_)
    : _hasUnappliedChanges(false)
    , _hasUnappliedPrimitiveChanges(false)
    , _isHidden(false)
    , _startingDistance(0.0f)
    , _isApproximationEnabled(true)
    , _colorizationScheme(0)
    , _lineWidth(1.0)
    , _outlineWidth(0.0)
    , _pathIconStep(-1.0f)
    , _specialPathIconStep(-1.0f)
    , _metersPerPixel(1.0)
    , _mapZoomLevel(InvalidZoomLevel)
    , _surfaceZoomLevel(InvalidZoomLevel)
    , _mapVisualZoom(0.f)
    , _surfaceVisualZoom(0.f)
    , _mapVisualZoomShift(0.f)
    , _isElevatedLineVisible(true)
    , _isSurfaceLineVisible(false)
    , _elevationScaleFactor(1.0f)
    , owner(owner_)
{
}

OsmAnd::VectorLine_P::~VectorLine_P()
{
}

bool OsmAnd::VectorLine_P::isHidden() const
{
    QReadLocker scopedLocker(&_lock);

    return _isHidden;
}

void OsmAnd::VectorLine_P::setIsHidden(const bool hidden)
{
    QWriteLocker scopedLocker(&_lock);

    if (_isHidden != hidden)
    {
        _isHidden = hidden;
        _hasUnappliedChanges = true;
    }
}

float OsmAnd::VectorLine_P::getStartingDistance() const
{
    QReadLocker scopedLocker(&_lock);

    return _startingDistance;
}

void OsmAnd::VectorLine_P::setStartingDistance(const float distanceInMeters)
{
    QWriteLocker scopedLocker(&_lock);

    if (_startingDistance != distanceInMeters)
    {
        _startingDistance = distanceInMeters;
        _hasUnappliedChanges = true;
    }
}

float OsmAnd::VectorLine_P::getArrowStartingGap() const
{
    QReadLocker scopedLocker(&_lock);

    return getPointStepPx() * _metersPerPixel;
}

bool OsmAnd::VectorLine_P::showArrows() const
{
    QReadLocker scopedLocker(&_lock);

    return _showArrows;
}

void OsmAnd::VectorLine_P::setShowArrows(const bool showArrows)
{
    QWriteLocker scopedLocker(&_lock);

    if (_showArrows != showArrows)
    {
        _showArrows = showArrows;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

bool OsmAnd::VectorLine_P::isApproximationEnabled() const
{
    QReadLocker scopedLocker(&_lock);

    return _isApproximationEnabled;
}

void OsmAnd::VectorLine_P::setApproximationEnabled(const bool enabled)
{
    QWriteLocker scopedLocker(&_lock);

    if (_isApproximationEnabled != enabled)
    {
        _isApproximationEnabled = enabled;
        _hasUnappliedChanges = true;
    }
}

QVector<OsmAnd::PointI> OsmAnd::VectorLine_P::getPoints() const
{
    QReadLocker scopedLocker(&_lock);

    return _points;
}

void OsmAnd::VectorLine_P::attachMarker(const std::shared_ptr<MapMarker>& marker)
{
    QReadLocker scopedLocker(&_lock);

    _attachedMarkers.push_back(marker);
}

void OsmAnd::VectorLine_P::setPoints(const QVector<PointI>& points)
{
    QWriteLocker scopedLocker(&_lock);

    if (_points != points)
    {
        _points = points;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

QList<float> OsmAnd::VectorLine_P::getHeights() const
{
    QReadLocker scopedLocker(&_lock);

    return _heights;
}

void OsmAnd::VectorLine_P::setHeights(const QList<float>& heights)
{
    QWriteLocker scopedLocker(&_lock);

    if (_heights != heights)
    {
        _heights = heights;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

QList<OsmAnd::FColorARGB> OsmAnd::VectorLine_P::getColorizationMapping() const
{
    QReadLocker scopedLocker(&_lock);

    return _colorizationMapping;
}

void OsmAnd::VectorLine_P::setColorizationMapping(const QList<FColorARGB> &colorizationMapping)
{
    QWriteLocker scopedLocker(&_lock);

    if (_colorizationMapping != colorizationMapping)
    {
        _colorizationMapping = colorizationMapping;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

QList<OsmAnd::FColorARGB> OsmAnd::VectorLine_P::getOutlineColorizationMapping() const
{
    QReadLocker scopedLocker(&_lock);

    return _outlineColorizationMapping;
}

void OsmAnd::VectorLine_P::setOutlineColorizationMapping(const QList<FColorARGB> &colorizationMapping)
{
    QWriteLocker scopedLocker(&_lock);

    if (_outlineColorizationMapping != colorizationMapping)
    {
        _outlineColorizationMapping = colorizationMapping;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

bool OsmAnd::VectorLine_P::hasColorizationMapping() const
{
    return _colorizationMapping.size() == _points.size();
}

bool OsmAnd::VectorLine_P::hasOutlineColorizationMapping() const
{
    return _outlineColorizationMapping.size() == _points.size();
}

bool OsmAnd::VectorLine_P::hasHeights() const
{
    return _heights.size() == _points.size();
}

double OsmAnd::VectorLine_P::getLineWidth() const
{
    QReadLocker scopedLocker(&_lock);

    return _lineWidth;
}

void OsmAnd::VectorLine_P::setLineWidth(const double width)
{
    QWriteLocker scopedLocker(&_lock);

    if (_lineWidth != width)
    {
        _lineWidth = width;

        if (owner->pathIcon)
        {
            double newWidth = _lineWidth * owner->screenScale / 8.0f;
            double scale = newWidth / owner->pathIcon->width();
            auto scaledPathIcon = SkiaUtilities::scaleImage(owner->pathIcon, scale, scale);
            _scaledPathIcon = scaledPathIcon ? scaledPathIcon : owner->pathIcon;
        }
        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }

}

float OsmAnd::VectorLine_P::getPathIconStep() const
{
    QReadLocker scopedLocker(&_lock);

    return _pathIconStep;
}

void OsmAnd::VectorLine_P::setPathIconStep(const float step)
{
    QWriteLocker scopedLocker(&_lock);

    if (!qFuzzyCompare(_pathIconStep, step))
    {
        _pathIconStep = step;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

float OsmAnd::VectorLine_P::getSpecialPathIconStep() const
{
    QReadLocker scopedLocker(&_lock);

    return _specialPathIconStep;
}

void OsmAnd::VectorLine_P::setSpecialPathIconStep(const float step)
{
    QWriteLocker scopedLocker(&_lock);

    if (!qFuzzyCompare(_specialPathIconStep, step))
    {
        _specialPathIconStep = step;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

double OsmAnd::VectorLine_P::getOutlineWidth() const
{
    QReadLocker scopedLocker(&_lock);

    return _outlineWidth;
}

void OsmAnd::VectorLine_P::setOutlineWidth(const double width)
{
    QWriteLocker scopedLocker(&_lock);

    if (_outlineWidth != width)
    {
        _outlineWidth = width;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

OsmAnd::FColorARGB OsmAnd::VectorLine_P::getOutlineColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _nearOutlineColor;
}

void OsmAnd::VectorLine_P::setOutlineColor(const FColorARGB color)
{
    QWriteLocker scopedLocker(&_lock);

    if (_nearOutlineColor != color || _farOutlineColor != color)
    {
        _nearOutlineColor = color;
        _farOutlineColor = color;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

OsmAnd::FColorARGB OsmAnd::VectorLine_P::getNearOutlineColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _nearOutlineColor;
}

void OsmAnd::VectorLine_P::setNearOutlineColor(const FColorARGB color)
{
    QWriteLocker scopedLocker(&_lock);

    if (_nearOutlineColor != color)
    {
        _nearOutlineColor = color;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

OsmAnd::FColorARGB OsmAnd::VectorLine_P::getFarOutlineColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _farOutlineColor;
}

void OsmAnd::VectorLine_P::setFarOutlineColor(const FColorARGB color)
{
    QWriteLocker scopedLocker(&_lock);

    if (_farOutlineColor != color)
    {
        _farOutlineColor = color;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

void OsmAnd::VectorLine_P::setColorizationScheme(const int colorizationScheme)
{
    QWriteLocker scopedLocker(&_lock);

    if (_colorizationScheme != colorizationScheme)
    {
        _colorizationScheme = colorizationScheme;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

bool OsmAnd::VectorLine_P::getElevatedLineVisibility() const
{
    QReadLocker scopedLocker(&_lock);

    return _isElevatedLineVisible;
}

void OsmAnd::VectorLine_P::setElevatedLineVisibility(const bool visible)
{
    QWriteLocker scopedLocker(&_lock);

    if (_isElevatedLineVisible != visible)
    {
        _isElevatedLineVisible = visible;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

bool OsmAnd::VectorLine_P::getSurfaceLineVisibility() const
{
    QReadLocker scopedLocker(&_lock);

    return _isSurfaceLineVisible;
}

void OsmAnd::VectorLine_P::setSurfaceLineVisibility(const bool visible)
{
    QWriteLocker scopedLocker(&_lock);

    if (_isSurfaceLineVisible != visible)
    {
        _isSurfaceLineVisible = visible;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

float OsmAnd::VectorLine_P::getElevationScaleFactor() const
{
    QReadLocker scopedLocker(&_lock);

    return _elevationScaleFactor;
}

void OsmAnd::VectorLine_P::setElevationScaleFactor(const float scaleFactor)
{
    QWriteLocker scopedLocker(&_lock);

    if (_elevationScaleFactor != scaleFactor)
    {
        _elevationScaleFactor = scaleFactor;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

OsmAnd::FColorARGB OsmAnd::VectorLine_P::getFillColor() const
{
    QReadLocker scopedLocker(&_lock);

    return _fillColor;
}

void OsmAnd::VectorLine_P::setFillColor(const FColorARGB color)
{
    QWriteLocker scopedLocker(&_lock);

    if (_fillColor != color)
    {
        _fillColor = color;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

std::vector<double> OsmAnd::VectorLine_P::getLineDash() const
{
    QReadLocker scopedLocker(&_lock);

    return _dashPattern;
}

void OsmAnd::VectorLine_P::setLineDash(const std::vector<double> dashPattern)
{
    QWriteLocker scopedLocker(&_lock);

    if (_dashPattern != dashPattern)
    {
        _dashPattern = dashPattern;

        _hasUnappliedPrimitiveChanges = true;
        _hasUnappliedChanges = true;
    }
}

bool OsmAnd::VectorLine_P::hasUnappliedChanges() const
{
    QReadLocker scopedLocker(&_lock);

    return _hasUnappliedChanges;
}

bool OsmAnd::VectorLine_P::hasUnappliedPrimitiveChanges() const
{
    QReadLocker scopedLocker(&_lock);

    return _hasUnappliedPrimitiveChanges;
}

bool OsmAnd::VectorLine_P::isMapStateChanged(const MapState& mapState) const
{
    bool changed = _mapZoomLevel != mapState.zoomLevel;
    changed |= _surfaceZoomLevel != mapState.surfaceZoomLevel;
    changed |= qAbs(_mapVisualZoom - mapState.visualZoom) > 0.25;
    changed |= qAbs(_surfaceVisualZoom - mapState.surfaceVisualZoom) > 0.25;
    changed |= _hasElevationDataProvider != mapState.hasElevationDataProvider;
    if (!changed && _visibleBBoxShifted != mapState.visibleBBoxShifted)
    {
        const AreaI64 visibleBBoxShifted(_visibleBBoxShifted);
        auto bboxShiftPoint = visibleBBoxShifted.topLeft - mapState.visibleBBoxShifted.topLeft;
        bool bboxChanged = abs(bboxShiftPoint.x) > visibleBBoxShifted.width()
            || abs(bboxShiftPoint.y) > visibleBBoxShifted.height();
        changed |= bboxChanged;
    }

    return changed;
}

void OsmAnd::VectorLine_P::applyMapState(const MapState& mapState)
{
    _metersPerPixel = mapState.metersPerPixel;
    _visibleBBoxShifted = mapState.visibleBBoxShifted;
    _mapZoomLevel = mapState.zoomLevel;
    _mapVisualZoom = mapState.visualZoom;
    _surfaceZoomLevel = mapState.surfaceZoomLevel;
    _surfaceVisualZoom = mapState.surfaceVisualZoom;
    _mapVisualZoomShift = mapState.visualZoomShift;
    _hasElevationDataProvider = mapState.hasElevationDataProvider;
}

bool OsmAnd::VectorLine_P::update(const MapState& mapState)
{
    QWriteLocker scopedLocker(&_lock);

    bool mapStateChanged = isMapStateChanged(mapState);
    if (mapStateChanged)
        applyMapState(mapState);

    _hasUnappliedPrimitiveChanges |= mapStateChanged;
    return mapStateChanged;
}

bool OsmAnd::VectorLine_P::applyChanges()
{
    QReadLocker scopedLocker1(&_lock);

    if (!_hasUnappliedChanges && !_hasUnappliedPrimitiveChanges)
        return false;

    QReadLocker scopedLocker2(&_symbolsGroupsRegistryLock);
    for (const auto& symbolGroup_ : constOf(_symbolsGroupsRegistry))
    {
        const auto symbolGroup = symbolGroup_.lock();
        if (!symbolGroup)
            continue;

        bool needUpdatePrimitive = _hasUnappliedPrimitiveChanges && _points.size() > 1;
        for (const auto& symbol_ : constOf(symbolGroup->symbols))
        {
            symbol_->isHidden = _isHidden;

            if (const auto symbol = std::dynamic_pointer_cast<OnSurfaceVectorMapSymbol>(symbol_))
            {
                symbol->startingDistance = _startingDistance;
                if (needUpdatePrimitive)
                    generatePrimitive(symbol);
            }
        }
    }
    owner->updatedObservable.postNotify(owner);
    _hasUnappliedChanges = false;
    _hasUnappliedPrimitiveChanges = false;

    return true;
}

std::shared_ptr<OsmAnd::VectorLine::SymbolsGroup> OsmAnd::VectorLine_P::inflateSymbolsGroup()
{
    QReadLocker scopedLocker(&_lock);

    // Construct new map symbols group for this marker
    const std::shared_ptr<VectorLine::SymbolsGroup> symbolsGroup(new VectorLine::SymbolsGroup(std::const_pointer_cast<VectorLine_P>(shared_from_this())));
    symbolsGroup->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAllOrNothing;

    if (_points.size() > 1)
    {
        const auto& vectorLine = std::make_shared<OnSurfaceVectorMapSymbol>(symbolsGroup);
        generatePrimitive(vectorLine);
        vectorLine->allowFastCheckByFrustum = false;
        symbolsGroup->symbols.push_back(vectorLine);
        owner->updatedObservable.postNotify(owner);
    }

    return symbolsGroup;
}

std::shared_ptr<OsmAnd::VectorLine::SymbolsGroup> OsmAnd::VectorLine_P::createSymbolsGroup(const MapState& mapState)
{
    applyMapState(mapState);

    const auto inflatedSymbolsGroup = inflateSymbolsGroup();
    registerSymbolsGroup(inflatedSymbolsGroup);
    return inflatedSymbolsGroup;
}

void OsmAnd::VectorLine_P::registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const
{
    QWriteLocker scopedLocker(&_symbolsGroupsRegistryLock);

    _symbolsGroupsRegistry.insert(symbolsGroup.get(), symbolsGroup);
}

void OsmAnd::VectorLine_P::unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const
{
    QWriteLocker scopedLocker(&_symbolsGroupsRegistryLock);

    _symbolsGroupsRegistry.remove(symbolsGroup);
}

OsmAnd::PointD OsmAnd::VectorLine_P::findLineIntersection(PointD p1, OsmAnd::PointD p2, OsmAnd::PointD p3, OsmAnd::PointD p4) const
{
    double d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    //double atn1 = atan2(p1.x - p2.x, p1.y - p2.y);
    //double atn2 = atan2(p3.x - p4.x, p3.y - p4.y);
    //double df = qAbs(atn1 - atn2);
    // printf("\n %f %f d=%f df=%f df-PI=%f df-2PI=%f", atn1, atn2, d, df, df - M_PI, df - 2 * M_PI);
    //double THRESHOLD = M_PI / 6;
    if(d == 0 // || df < THRESHOLD  || qAbs(df - M_PI) < THRESHOLD || qAbs(df - 2 * M_PI) < THRESHOLD
       ) {
        // in case of lines connecting p2 == p3
        return p2;
    }
    OsmAnd::PointD r;
    r.x = ((p1.x* p2.y-p1.y*p2.x)*(p3.x - p4.x) - (p3.x* p4.y-p3.y*p4.x)*(p1.x - p2.x)) / d;
    r.y = ((p1.x* p2.y-p1.y*p2.x)*(p3.y - p4.y) - (p3.x* p4.y-p3.y*p4.x)*(p1.y - p2.y)) / d;

    return r;
}

OsmAnd::PointD OsmAnd::VectorLine_P::getProjection(PointD point, PointD from, PointD to ) const
{
    double mDist = (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y);
    double projection = scalarMultiplication(from.x, from.y, to.x, to.y, point.x, point.y);
    if (projection < 0)
    {
        return from;
    }
    else if (projection >= mDist)
    {
        return to;
    }
    return PointD(from.x + (to.x - from.x) * (projection / mDist),
                  from.y + (to.y - from.y) * (projection / mDist));
}

double OsmAnd::VectorLine_P::scalarMultiplication(double xA, double yA, double xB, double yB, double xC, double yC) const
{
    // Scalar multiplication between (AB, AC)
    return (xB - xA) * (xC - xA) + (yB - yA) * (yC - yA);
}

int OsmAnd::VectorLine_P::simplifyDouglasPeucker(
    const std::vector<PointI>& points,
    const uint start,
    const uint end,
    const double epsilon,
    std::vector<bool>& include) const
{
    double dmax = -1;
    int index = -1;
    for (int i = start + 1; i <= end - 1; i++)
    {
        PointD pointToProject = static_cast<PointD>(points[i]);
        PointD startPoint = static_cast<PointD>(points[start]);
        PointD endPoint = static_cast<PointD>(points[end]);
        PointD proj = getProjection(pointToProject, startPoint, endPoint);
        double d = qSqrt((points[i].x - proj.x) * (points[i].x - proj.x) +
                         (points[i].y - proj.y) * (points[i].y - proj.y));
        // calculate distance from line
        if (d > dmax)
        {
            dmax = d;
            index = i;
        }
    }
    if (dmax >= epsilon && index > -1)
    {
        int enabled1 = simplifyDouglasPeucker(points, start, index, epsilon, include);
        int enabled2 = simplifyDouglasPeucker(points, index, end, epsilon, include);
        return enabled1 + enabled2;
    }
    else
    {
        include[end] = true;
        return 1;
    }
}

bool OsmAnd::VectorLine_P::forceIncludePoint(const QList<FColorARGB>& pointsColors, const uint pointIndex) const
{
    const auto currColor = pointsColors[pointIndex];

    const auto pPrevColor = pointIndex == 0 ? nullptr : &pointsColors[pointIndex - 1];
    const auto pNextColor = pointIndex + 1 == pointsColors.size() ? nullptr : &pointsColors[pointIndex + 1];

    if (_colorizationScheme == COLORIZATION_SOLID)
        return !pPrevColor || *pPrevColor != currColor;

    if (_colorizationScheme == COLORIZATION_GRADIENT)
    {
        bool highColorDiff = false;
        highColorDiff |= pPrevColor && qAbs(pPrevColor->a - currColor.a) > MIN_ALPHA_DELTA;
        highColorDiff |= pNextColor && qAbs(pNextColor->a - currColor.a) > MIN_ALPHA_DELTA;
        highColorDiff |= pPrevColor && currColor.getRGBDelta(*pPrevColor) > MIN_RGB_DELTA;
        highColorDiff |= pNextColor && currColor.getRGBDelta(*pNextColor) > MIN_RGB_DELTA;
        return highColorDiff;
    }

    return false;
}

OsmAnd::FColorARGB OsmAnd::VectorLine_P::middleColor(
    const FColorARGB& first, const FColorARGB& last, const float factor) const
{
    return FColorARGB(
        first.a + (last.a - first.a) * factor,
        first.r + (last.r - first.r) * factor,
        first.g + (last.g - first.g) * factor,
        first.b + (last.b - first.b) * factor);
}

void OsmAnd::VectorLine_P::calculateVisibleSegments(
    std::vector<std::vector<PointI>>& segments,
    QList<QList<float>>& segmentDistances,
    QList<QList<FColorARGB>>& segmentColors,
    QList<QList<FColorARGB>>& segmentOutlineColors,
    QList<QList<float>>& segmentHeights)
{
    // Use enlarged visible area
    const AreaI64 visibleBBox64(_visibleBBoxShifted);
    auto visibleArea64 = visibleBBox64.getEnlargedBy(PointI64(visibleBBox64.width(), visibleBBox64.height()));
    visibleArea64.topLeft.x = visibleArea64.topLeft.x < INT32_MIN ? INT32_MIN : visibleArea64.topLeft.x;
    visibleArea64.topLeft.y = visibleArea64.topLeft.y < INT32_MIN ? INT32_MIN : visibleArea64.topLeft.y;
    visibleArea64.bottomRight.x = visibleArea64.bottomRight.x > INT32_MAX ? INT32_MAX : visibleArea64.bottomRight.x;
    visibleArea64.bottomRight.y = visibleArea64.bottomRight.y > INT32_MAX ? INT32_MAX : visibleArea64.bottomRight.y;
    const AreaI visibleArea(visibleArea64);

    // Calculate points unwrapped
    auto pointsCount = _points.size();
    int64_t intFull = INT32_MAX;
    intFull++;
    const auto intHalf = static_cast<int32_t>(intFull >> 1);
    const PointI shiftToCenter(intHalf, intHalf);
    auto point31 = _points[0];
    PointI64 point64(point31 - shiftToCenter);
    QVector<PointI64> points64;
    points64.reserve(pointsCount);
    points64.push_back(point64);
    QVector<int> pointIndices(pointsCount);
    QVector<float> pointDistances(pointsCount);
    pointIndices[0] = 0;
    pointDistances[0] = 0.0f;
    int nextIndex = 0;
    AreaI64 bbox(point64, point64);
    PointI nextPoint31;
    auto pointsTotal = 1;
    double distance = 0.0;
    for (int i = 1; i < pointsCount; i++)
    {
        auto offset = _points[i] - point31;
        if (offset.x >= intHalf)
            offset.x = offset.x - INT32_MAX - 1;
        else if (offset.x < -intHalf)
            offset.x = offset.x + INT32_MAX + 1;
        nextPoint31 = Utilities::normalizeCoordinates(PointI64(point31) + offset, ZoomLevel31);
        distance +=
            Utilities::calculateShortestPath(point64, point31, nextPoint31, bbox.topLeft, bbox.bottomRight, &points64);
        point64 += offset;
        points64.push_back(point64);
        bbox.enlargeToInclude(point64);
        auto pointsSize = points64.size();
        nextIndex += pointsSize - pointsTotal;
        pointIndices[i] = nextIndex;
        pointDistances[i] = static_cast<float>(distance);
        pointsTotal = pointsSize;
        point31 = nextPoint31;
    }
    
    for (auto& attachedMarker : _attachedMarkers)
    {
        attachedMarker->attachToVectorLine(points64);
    }
    
    auto minShiftX = static_cast<int32_t>(bbox.topLeft.x / intFull - (bbox.topLeft.x % intFull < 0 ? 1 : 0));
    auto minShiftY = static_cast<int32_t>(bbox.topLeft.y / intFull - (bbox.topLeft.y % intFull < 0 ? 1 : 0));
    auto maxShiftX = static_cast<int32_t>(bbox.bottomRight.x / intFull + (bbox.bottomRight.x % intFull < 0 ? 0 : 1));
    auto maxShiftY = static_cast<int32_t>(bbox.bottomRight.y / intFull + (bbox.bottomRight.y % intFull < 0 ? 0 : 1));

    // Use full map shifts to collect all visible segments
    const bool solidColors = _colorizationScheme == COLORIZATION_SOLID;
    const bool withColors = hasColorizationMapping();
    const bool withOutlineColors = hasOutlineColorizationMapping();
    const bool withHeights = hasHeights();
    pointsCount = points64.size();
    PointI64 curr, drawFrom, drawTo, inter1, inter2;
    FColorARGB colorFrom, colorTo, colorSubFrom, colorSubTo, colorInterFrom, colorInterTo;
    FColorARGB colorOutFrom, colorOutTo, colorOutSubFrom, colorOutSubTo, colorOutInterFrom, colorOutInterTo;
    float heightFrom, heightTo, heightSubFrom, heightSubTo, heightInterFrom, heightInterTo;
    float distanceFrom, distanceTo, distanceSubFrom, distanceSubTo, distanceInterFrom, distanceInterTo;
    std::vector<PointI> segment;
    QList<FColorARGB> colors;
    QList<FColorARGB> outlineColors;
    QList<float> heights;
    QList<float> distances;
    for (int shiftX = minShiftX; shiftX <= maxShiftX; shiftX++)
    {
        for (int shiftY = minShiftY; shiftY <= maxShiftY; shiftY++)
        {
            const PointI64 shift(shiftX * intFull, shiftY * intFull);
            bool segmentStarted = false;
            distanceTo = pointDistances[0];
            distanceSubTo = pointDistances[0];
            if (withColors)
            {
                colorTo = _colorizationMapping[0];
                colorSubTo = colorTo;
            }
            if (withOutlineColors)
            {
                colorOutTo = _outlineColorizationMapping[0];
                colorOutSubTo = colorOutTo;
            }
            if (withHeights)
            {
                heightTo = _heights[0];
                heightSubTo = _heights[0];
            }
            auto prev = drawFrom = points64[0] - shift;
            int prevIndex;
            nextIndex = 0;            
            int j = 0;
            bool prevIn = visibleArea64.contains(prev);
            for (int i = 1; i < pointsCount; i++)
            {
                curr = drawTo = points64[i] - shift;
                if (i > nextIndex)
                {
                    prevIndex = nextIndex;
                    nextIndex = pointIndices[++j];
                    distanceFrom = distanceTo;
                    distanceTo = pointDistances[j];
                    if (withColors)
                    {
                        colorFrom = colorTo;
                        colorTo = _colorizationMapping[j];
                    }
                    if (withOutlineColors)
                    {
                        colorOutFrom = colorOutTo;
                        colorOutTo = _outlineColorizationMapping[j];
                    }
                    if (withHeights)
                    {
                        heightFrom = heightTo;
                        heightTo = _heights[j];
                    }
                }
                const auto factor = static_cast<float>(i - prevIndex) / static_cast<float>(nextIndex - prevIndex);
                distanceSubFrom = distanceSubTo;
                distanceSubTo = distanceFrom + (distanceTo - distanceFrom) * factor;
                if (withColors)
                {
                    colorSubFrom = solidColors ? colorFrom : colorSubTo;
                    colorSubTo = solidColors ? colorTo : middleColor(colorFrom, colorTo, factor);
                }
                if (withOutlineColors)
                {
                    colorOutSubFrom = solidColors ? colorOutFrom : colorOutSubTo;
                    colorOutSubTo = solidColors ? colorOutTo : middleColor(colorOutFrom, colorOutTo, factor);
                }
                if (withHeights)
                {
                    heightSubFrom = heightSubTo;
                    heightSubTo = heightFrom + (heightTo - heightFrom) * factor;
                }
                bool currIn = visibleArea64.contains(curr);
                bool draw = false;
                if (prevIn && currIn)
                {
                    draw = true;
                    distanceInterFrom = distanceSubFrom;
                    distanceInterTo = distanceSubTo;
                    if (withColors)
                    {
                        colorInterFrom = colorSubFrom;
                        colorInterTo = colorSubTo;
                    }
                    if (withOutlineColors)
                    {
                        colorOutInterFrom = colorOutSubFrom;
                        colorOutInterTo = colorOutSubTo;
                    }
                    if (withHeights)
                    {
                        heightInterFrom = heightSubFrom;
                        heightInterTo = heightSubTo;
                    }
                }
                else
                {
                    if (Utilities::calculateIntersection(curr, prev, visibleArea, inter1))
                    {
                        draw = true;
                        if (prevIn)
                        {
                            drawTo = inter1;
                            const auto factor = static_cast<float>(
                                static_cast<double>((drawTo - prev).norm()) /
                                static_cast<double>((curr - prev).norm()));
                            distanceInterFrom = distanceSubFrom;
                            distanceInterTo = distanceSubFrom + (distanceSubTo - distanceSubFrom) * factor;
                            if (withColors)
                            {
                                colorInterFrom = colorSubFrom;
                                colorInterTo = solidColors ? colorSubFrom
                                    : middleColor(colorSubFrom, colorSubTo, factor);
                            }
                            if (withOutlineColors)
                            {
                                colorOutInterFrom = colorOutSubFrom;
                                colorOutInterTo = solidColors ? colorOutSubFrom
                                    : middleColor(colorOutSubFrom, colorOutSubTo, factor);
                            }
                            if (withHeights)
                            {
                                heightInterFrom = heightSubFrom;
                                heightInterTo = heightSubFrom + (heightSubTo - heightSubFrom) * factor;
                            }
                        }
                        else if (currIn)
                        {
                            drawFrom = inter1;
                            segmentStarted = false;
                            const auto factor = static_cast<float>(
                                static_cast<double>((drawFrom - prev).norm()) /
                                static_cast<double>((curr - prev).norm()));
                            distanceInterFrom = distanceSubFrom + (distanceSubTo - distanceSubFrom) * factor;
                            distanceInterTo = distanceSubTo;
                            if (withColors)
                            {
                                colorInterFrom = solidColors ? colorSubFrom
                                    : middleColor(colorSubFrom, colorSubTo, factor);
                                colorInterTo = colorSubTo;
                            }
                            if (withOutlineColors)
                            {
                                colorOutInterFrom = solidColors ? colorOutSubFrom
                                    : middleColor(colorOutSubFrom, colorOutSubTo, factor);
                                colorOutInterTo = colorOutSubTo;
                            }
                            if (withHeights)
                            {
                                heightInterFrom = heightSubFrom + (heightSubTo - heightSubFrom) * factor;
                                heightInterTo = heightSubTo;
                            }
                        }
                        else if (Utilities::calculateIntersection(prev, curr, visibleArea, inter2))
                        {
                            drawFrom = inter1;
                            drawTo = inter2;
                            segmentStarted = false;
                            const auto firstFactor = static_cast<float>(
                                static_cast<double>((drawFrom - prev).norm()) /
                                static_cast<double>((curr - prev).norm()));
                            const auto secondFactor = static_cast<float>(
                                static_cast<double>((drawTo - prev).norm()) /
                                static_cast<double>((curr - prev).norm()));
                            distanceInterFrom = distanceSubFrom + (distanceSubTo - distanceSubFrom) * firstFactor;
                            distanceInterTo = distanceSubFrom + (distanceSubTo - distanceSubFrom) * secondFactor;
                            if (withColors)
                            {
                                colorInterFrom = solidColors ? colorSubFrom
                                    : middleColor(colorSubFrom, colorSubTo, firstFactor);
                                colorInterTo = solidColors ? colorSubFrom
                                    : middleColor(colorSubFrom, colorSubTo, secondFactor);
                            }
                            if (withOutlineColors)
                            {
                                colorOutInterFrom = solidColors ? colorOutSubFrom
                                    : middleColor(colorOutSubFrom, colorOutSubTo, firstFactor);
                                colorOutInterTo = solidColors ? colorOutSubFrom
                                    : middleColor(colorOutSubFrom, colorOutSubTo, secondFactor);
                            }
                            if (withHeights)
                            {
                                heightInterFrom = heightSubFrom + (heightSubTo - heightSubFrom) * firstFactor;
                                heightInterTo = heightSubFrom + (heightSubTo - heightSubFrom) * secondFactor;
                            }
                        }
                        else
                            draw = false;
                    }
                }
                if (draw)
                {
                    if (!segmentStarted)
                    {
                        if (!segment.empty())
                        {
                            segments.push_back(segment);
                            segment = std::vector<PointI>();
                            segmentDistances.push_back(distances);
                            distances.clear();
                            segmentColors.push_back(colors);
                            colors.clear();
                            segmentOutlineColors.push_back(outlineColors);
                            outlineColors.clear();
                            segmentHeights.push_back(heights);
                            heights.clear();
                        }
                        segment.push_back(PointI(drawFrom));
                        distances.push_back(distanceInterFrom);
                        if (withColors)
                            colors.push_back(colorInterFrom);
                        if (withOutlineColors)
                            outlineColors.push_back(colorOutInterFrom);
                        if (withHeights)
                            heights.push_back(heightInterFrom);
                        segmentStarted = currIn;
                    }
                    PointI drawTo31(drawTo);
                    if (segment.empty() || segment.back() != drawTo31)
                    {
                        segment.push_back(drawTo31);
                        distances.push_back(distanceInterTo);
                        if (withColors)
                            colors.push_back(colorInterTo);
                        if (withOutlineColors)
                            outlineColors.push_back(colorOutInterTo);
                        if (withHeights)
                            heights.push_back(heightInterTo);
                    }
                }
                else
                    segmentStarted = false;
                prevIn = currIn;
                prev = drawFrom = curr;
            }
            if (!segment.empty())
                segments.push_back(segment);
            if (!distances.empty())
                segmentDistances.push_back(distances);
            if (!colors.empty())
                segmentColors.push_back(colors);
            if (!outlineColors.empty())
                segmentOutlineColors.push_back(outlineColors);
            if (!heights.empty())
                segmentHeights.push_back(heights);
            segment.clear();
            distances.clear();
            colors.clear();
            outlineColors.clear();
            heights.clear();
        }
    }
}

float OsmAnd::VectorLine_P::zoom() const
{
    return _mapZoomLevel + (_mapVisualZoom >= 1.0f ? _mapVisualZoom - 1.0f : (_mapVisualZoom - 1.0f) * 2.0f);
}

std::shared_ptr<OsmAnd::OnSurfaceVectorMapSymbol> OsmAnd::VectorLine_P::generatePrimitive(
    const std::shared_ptr<OnSurfaceVectorMapSymbol> vectorLine)
{
    int order = owner->baseOrder;
    const bool withHeights = _hasElevationDataProvider && hasHeights();
    float zoom = !_hasElevationDataProvider ? this->zoom() : _surfaceZoomLevel +
        (_surfaceVisualZoom >= 1.0f ? _surfaceVisualZoom - 1.0f : (_surfaceVisualZoom - 1.0f) * 2.0f);
    double scale = Utilities::getPowZoom(31 - zoom) * qSqrt(zoom) /
        (AtlasMapRenderer::TileSize3D * AtlasMapRenderer::TileSize3D); // TODO: this should come from renderer

    double visualShiftCoef = 1 / (1 + _mapVisualZoomShift);
    double thickness = _lineWidth * scale * visualShiftCoef * 2.0;
    double outlineThickness = _outlineWidth * scale * visualShiftCoef * 2.0;
    bool approximate = _isApproximationEnabled;
    double simplificationRadius = _lineWidth * scale * visualShiftCoef;

    vectorLine->order = order++;
    vectorLine->primitiveType = VectorMapSymbol::PrimitiveType::Triangles;
    vectorLine->scaleType = VectorMapSymbol::ScaleType::In31;
    vectorLine->scale = 1.0;
    vectorLine->direction = 0.f;

    const auto verticesAndIndices = std::make_shared<VectorMapSymbol::VerticesAndIndices>();
    // Line has no reusable vertices - TODO clarify
    verticesAndIndices->indices = nullptr;
    verticesAndIndices->indicesCount = 0;

    clearArrowsOnPath();

    std::vector<VectorMapSymbol::Vertex> vertices;
    VectorMapSymbol::Vertex vertex;

    std::vector<std::vector<PointI>> segments;
    QList<QList<float>> distances;
    QList<QList<FColorARGB>> colors;
    QList<QList<FColorARGB>> outlineColors;
    QList<QList<float>> heights;
    calculateVisibleSegments(segments, distances, colors, outlineColors, heights);
    const bool withColors = hasColorizationMapping();
    const bool withOutlineColors = hasOutlineColorizationMapping();

    PointD startPos;
    bool startPosDefined = false;
    for (int segmentIndex = 0; segmentIndex < segments.size(); segmentIndex++)
    {
        auto& points = segments[segmentIndex];
        const auto distancesForSegment = distances[segmentIndex];
        const auto colorsForSegment = withColors ? colors[segmentIndex] : QList<FColorARGB>();
        const auto colorsForSegmentOutline = withOutlineColors ? outlineColors[segmentIndex] : QList<FColorARGB>();
        const auto heightsForSegment = withHeights ? heights[segmentIndex] : QList<float>();
        if (points.size() < 2)
            continue;

        if (!startPosDefined)
        {
            startPosDefined = true;
            const auto startPoint = points[0];
            int64_t intFull = INT32_MAX;
            intFull++;
            const auto intHalf = intFull >> 1;
            const auto intTwo = intFull << 1;
            const PointI64 origPos = PointI64(intHalf, intHalf) + startPoint;
            const PointI location31(
                origPos.x > INT32_MAX ? origPos.x - intTwo : origPos.x,
                origPos.y > INT32_MAX ? origPos.y - intTwo : origPos.y);
            vectorLine->position31 = location31;
            verticesAndIndices->position31 = new PointI(location31);
            startPos = PointD(startPoint);
        }
        int pointsCount = (int) points.size();

        std::vector<bool> include(pointsCount, !approximate);
        if (approximate)
        {
            include[0] = true;
            simplifyDouglasPeucker(points, 0, (uint) points.size() - 1, simplificationRadius / 3, include);
        }

        // Generate base points to draw a stripe around
        std::vector<OsmAnd::PointD> original;
        original.reserve(pointsCount);
        QList<float> filteredDistances;
        QList<OsmAnd::FColorARGB> filteredColorsMap;
        QList<OsmAnd::FColorARGB> filteredOutlineColorsMap;
        QList<float> filteredHeights;
        int includedPointsCount = 0;
        for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
        {
            // If color is lost after approximation, restore it
            if (approximate && !include[pointIdx] && hasColorizationMapping())
                include[pointIdx] = forceIncludePoint(colorsForSegment, pointIdx);
            if (approximate && !include[pointIdx] && hasOutlineColorizationMapping())
                include[pointIdx] = forceIncludePoint(colorsForSegmentOutline, pointIdx);

            if (include[pointIdx])
            {
                filteredDistances.push_back(distancesForSegment[pointIdx]);
                if (withColors)
                    filteredColorsMap.push_back(colorsForSegment[pointIdx]);
                if (withOutlineColors)
                    filteredOutlineColorsMap.push_back(colorsForSegmentOutline[pointIdx]);
                if (withHeights)
                    filteredHeights.push_back(heightsForSegment[pointIdx]);
                original.push_back(PointD(points[pointIdx]) - startPos);
                includedPointsCount++;
            }
        }

        if (_showArrows && owner->pathIcon)
        {
            const auto arrowsOrigin = segments.back().back();
            addArrowsOnSegmentPath(points, distancesForSegment, heightsForSegment, include, arrowsOrigin);
        }

        int patternLength = (int) _dashPattern.size();
        // generate triangles
        if (patternLength > 0)
        {
            // Dashed line does not support distances yet, thus clear distances
            filteredDistances.clear();

            // Dashed line does not support colorization yet, thus clear colors map
            filteredColorsMap.clear();
            filteredOutlineColorsMap.clear();

            // Dashed line does not support heights yet, thus clear heightmap
            filteredHeights.clear();
            
            FColorARGB fillColor = _fillColor;
            std::vector<double> dashPattern(_dashPattern);
            double threshold = dashPattern[0] < 0 ? -dashPattern[0] : 0;
            if (threshold > 0)
            {
                dashPattern.erase(dashPattern.begin());
                patternLength--;
            }

            OsmAnd::PointD start = original[0];
            OsmAnd::PointD end = original[original.size() - 1];
            OsmAnd::PointD prevPnt = start;

            std::vector<OsmAnd::PointD> origTar;
            if (threshold == 0)
                origTar.push_back(start);

            double dashPhase = 0;
            int patternIndex = 0;
            bool firstDash = true;
            for (auto pointIdx = 1u; pointIdx < includedPointsCount; pointIdx++)
            {
                OsmAnd::PointD pnt = original[pointIdx];
                double segLength = sqrt(pow((prevPnt.x - pnt.x), 2) + pow((prevPnt.y - pnt.y), 2));
                // create a vector of direction for the segment
                OsmAnd::PointD v = pnt - prevPnt;
                // unit length
                OsmAnd::PointD u(v.x / segLength, v.y / segLength);

                double length = firstDash && threshold > 0 ? threshold * scale : dashPattern[patternIndex] * scale - 0;
                bool gap = firstDash && threshold > 0 ? true : patternIndex % 2 == 1;

                OsmAnd::PointD delta;
                double deltaLength;
                if (dashPhase == 0)
                    deltaLength = length;
                else
                    deltaLength = dashPhase;

                delta = OsmAnd::PointD(u.x * deltaLength, u.y * deltaLength);

                if (segLength <= deltaLength)
                {
                    if (!gap)
                        origTar.push_back(pnt);
                }
                else
                {
                    while (deltaLength < segLength)
                    {
                        origTar.push_back(prevPnt + delta);

                        if (!gap)
                        {
                            createVertexes(vertices, vertex, origTar, thickness, filteredDistances,
                                fillColor, filteredColorsMap, filteredOutlineColorsMap, filteredHeights);
                            origTar.clear();
                            firstDash = false;
                        }

                        if (!firstDash)
                            patternIndex++;

                        patternIndex %= patternLength;
                        gap = patternIndex % 2 == 1;
                        length = dashPattern[patternIndex] * scale - (gap ? 0 : 0);
                        delta += OsmAnd::PointD(u.x * length, u.y * length);
                        deltaLength += length;
                    }

                    if (!origTar.empty() && !gap)
                        origTar.push_back(pnt);
                }

                // calculate dash phase
                dashPhase = length - (segLength - deltaLength);
                if (dashPhase > length)
                    dashPhase -= length;

                prevPnt = pnt;
            }
            // end point
            if (threshold == 0)
            {
                if (origTar.size() == 0)
                    origTar.push_back(end);
                
                origTar.push_back(end);
                createVertexes(vertices, vertex, origTar, thickness, filteredDistances,
                    fillColor, filteredColorsMap, filteredOutlineColorsMap, filteredHeights);
            }
        }
        else
        {
            if (withHeights)
            {
                GeometryModifiers::getTesselatedPlane(
                    vertices,
                    original,
                    filteredDistances,
                    filteredHeights,
                    _fillColor, _nearOutlineColor, _farOutlineColor,
                    filteredColorsMap, filteredOutlineColorsMap,
                    _isElevatedLineVisible ? outlineThickness : 0.0f,
                    _mapZoomLevel,
                    Utilities::convert31toDouble(*(verticesAndIndices->position31), _mapZoomLevel),
                    AtlasMapRenderer::HeixelsPerTileSide - 1,
                    0.4f, false,
                    _colorizationScheme == COLORIZATION_SOLID);
                if (_isElevatedLineVisible)
                {
                    crushedpixel::Polyline2D::create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(
                        vertex,
                        vertices,
                        original, thickness, 0.0f, filteredDistances,
                        _fillColor, _nearOutlineColor, _farOutlineColor,
                        filteredColorsMap, filteredOutlineColorsMap, filteredHeights,
                        static_cast<crushedpixel::Polyline2D::JointStyle>(static_cast<int>(owner->jointStyle)),
                        static_cast<crushedpixel::Polyline2D::EndCapStyle>(static_cast<int>(owner->endCapStyle)),
                        _colorizationScheme == COLORIZATION_SOLID);
                }
                if (_isSurfaceLineVisible)
                {
                    filteredHeights.clear();
                    crushedpixel::Polyline2D::create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(
                        vertex,
                        vertices,
                        original, thickness, 0.0f, filteredDistances,
                        _fillColor, _nearOutlineColor, _farOutlineColor,
                        filteredColorsMap, filteredOutlineColorsMap, filteredHeights,
                        static_cast<crushedpixel::Polyline2D::JointStyle>(static_cast<int>(owner->jointStyle)),
                        static_cast<crushedpixel::Polyline2D::EndCapStyle>(static_cast<int>(owner->endCapStyle)),
                        _colorizationScheme == COLORIZATION_SOLID);
                }
            }
            else
            {
                crushedpixel::Polyline2D::create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(
                    vertex,
                    vertices,
                    original, thickness, (outlineThickness - thickness) / 2.0f, filteredDistances,
                    _fillColor, _nearOutlineColor, _farOutlineColor,
                    filteredColorsMap, filteredOutlineColorsMap, filteredHeights,
                    static_cast<crushedpixel::Polyline2D::JointStyle>(static_cast<int>(owner->jointStyle)),
                    static_cast<crushedpixel::Polyline2D::EndCapStyle>(static_cast<int>(owner->endCapStyle)),
                    _colorizationScheme == COLORIZATION_SOLID);
            }
        }
    }
    if (vertices.size() == 0)
    {
        vertex.positionXYZD[0] = 0;
        vertex.positionXYZD[1] = VectorMapSymbol::_absentElevation;
        vertex.positionXYZD[2] = 0;
        vertex.positionXYZD[3] = NAN;
        vertices.push_back(vertex);
        verticesAndIndices->position31 = new PointI(0, 0);
    }

    // Tesselate the line for the surface
    auto partSizes =
        std::shared_ptr<std::vector<std::pair<TileId, int32_t>>>(new std::vector<std::pair<TileId, int32_t>>);
    const auto zoomLevel = _mapZoomLevel < MaxZoomLevel ? static_cast<ZoomLevel>(_mapZoomLevel + 1) : _mapZoomLevel;
	const auto cellsPerTileSize = (AtlasMapRenderer::HeixelsPerTileSide - 1) / (1 << (zoomLevel - _mapZoomLevel));
    bool tesselated = _hasElevationDataProvider
        ? GeometryModifiers::cutMeshWithGrid(
            vertices,
            nullptr,
            vectorLine->primitiveType,
            partSizes,
            zoomLevel,
            Utilities::convert31toDouble(*(verticesAndIndices->position31), zoomLevel),
            cellsPerTileSize,
            0.5f, 0.01f,
            false, false)
        : false;
    verticesAndIndices->partSizes = tesselated ? partSizes : nullptr;
    verticesAndIndices->zoomLevel = tesselated ? zoomLevel : InvalidZoomLevel;
    verticesAndIndices->isDenseObject =
        tesselated && withHeights && _nearOutlineColor.a == 1.0f && _farOutlineColor.a == 1.0f;

    verticesAndIndices->verticesCount = (unsigned int) vertices.size();
    verticesAndIndices->vertices = new VectorMapSymbol::Vertex[vertices.size()];
    std::copy(vertices.begin(), vertices.end(), verticesAndIndices->vertices);

    vectorLine->isHidden = _isHidden;
    vectorLine->startingDistance = _startingDistance;
    vectorLine->elevationScaleFactor = _elevationScaleFactor;
    vectorLine->setVerticesAndIndices(verticesAndIndices);
    return vectorLine;
}

void OsmAnd::VectorLine_P::createVertexes(std::vector<VectorMapSymbol::Vertex> &vertices,
                  VectorMapSymbol::Vertex &vertex,
                  std::vector<OsmAnd::PointD> &original,
                  double thickness,
                  QList<float>& distances,
                  FColorARGB &fillColor,
                  QList<FColorARGB>& colorMapping,
                  QList<FColorARGB>& outlineColorMapping,
                  QList<float>& heights) const
{
    auto pointsCount = original.size();
    if (pointsCount == 0)
        return;

    crushedpixel::Polyline2D::create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(
        vertex,
        vertices,
        original, thickness, 0.0f, distances,
        fillColor, fillColor, fillColor, colorMapping, outlineColorMapping, heights,
        static_cast<crushedpixel::Polyline2D::JointStyle>(static_cast<int>(owner->jointStyle)),
        static_cast<crushedpixel::Polyline2D::EndCapStyle>(static_cast<int>(owner->endCapStyle)));
}

void OsmAnd::VectorLine_P::clearArrowsOnPath()
{
    QWriteLocker scopedLocker(&_arrowsOnPathLock);

    _arrowsOnPath.clear();
}

const QList<OsmAnd::VectorLine::OnPathSymbolData> OsmAnd::VectorLine_P::getArrowsOnPath() const
{
    QReadLocker scopedLocker(&_arrowsOnPathLock);

    return detachedOf(_arrowsOnPath);
}

void OsmAnd::VectorLine_P::addArrowsOnSegmentPath(
    const std::vector<PointI>& segmentPoints,
    const QList<float>& segmentDistances,
    const QList<float>& segmentHeights,
    const std::vector<bool>& includedPoints,
    const PointI64& origin)
{
    int64_t intFull = INT32_MAX;
    intFull++;
    const auto intHalf = intFull >> 1;
    const auto intTwo = intFull << 1;
    const PointD location(origin);
    bool withHeights = !segmentHeights.isEmpty();
    float pathIconStep = getPointStepPx();
    double step = Utilities::metersToX31(pathIconStep * _metersPerPixel);
    double halfStep = step / 2.0;
    bool ok = halfStep > 0.0;
    if (!ok)
        return;

    PointD prevPoint(PointI64(segmentPoints.back()) - origin);
    PointD nextPoint;
    float prevDistance = segmentDistances[segmentPoints.size() - 1];
    float nextDistance;
    float prevHeight = withHeights ? segmentHeights[segmentPoints.size() - 1] : NAN;
    float nextHeight;
    double gap = halfStep;

    QWriteLocker scopedLocker(&_arrowsOnPathLock);
    
    for (int i = (int) segmentPoints.size() - 2; i >= 0; i--)
    {
        if (!includedPoints[i])
            continue;

        if (gap <= 0.0)
            gap = step;

        nextPoint = PointD(PointI64(segmentPoints[i]) - origin);
        nextDistance = segmentDistances[i];
        nextHeight = withHeights ? segmentHeights[i] : NAN;
        const auto lineLength = (nextPoint - prevPoint).norm();
        auto shift = gap;
        while (lineLength >= shift && (i > 0 || lineLength >= shift + halfStep))
        {
            auto t = (nextPoint - prevPoint) / lineLength;
            auto midPoint = prevPoint + t * shift;
            auto midDistance = prevDistance + (nextDistance - prevDistance) * static_cast<float>(shift / lineLength);
            auto midHeight =
                withHeights ? prevHeight + (nextHeight - prevHeight) * static_cast<float>(shift / lineLength) : NAN;

            PointI64 origPos(
                static_cast<int64_t>(midPoint.x + location.x) + intHalf,
                static_cast<int64_t>(midPoint.y + location.y) + intHalf);
            const PointI position(
                origPos.x + (origPos.x >= intFull ? -intTwo : (origPos.x < -intFull ? intFull : 0)),
                origPos.y + (origPos.y >= intFull ? -intTwo : (origPos.y < -intFull ? intFull : 0)));
            // Get mirrored direction
            float direction = Utilities::normalizedAngleDegrees(qRadiansToDegrees(atan2(-t.x, t.y)) - 180);
            const VectorLine::OnPathSymbolData arrowSymbol(
                position, midDistance, direction, midHeight, _elevationScaleFactor);
            _arrowsOnPath.push_back(arrowSymbol);

            shift += step;
        }
        gap = shift - lineLength;
        prevPoint = nextPoint;
        prevDistance = nextDistance;
        prevHeight = nextHeight;
    }
}

bool OsmAnd::VectorLine_P::useSpecialArrow() const
{
    return _lineWidth < LINE_WIDTH_THRESHOLD_DP && owner->specialPathIcon;
}

double OsmAnd::VectorLine_P::getPointStepPx() const
{
    if (useSpecialArrow())
    {
        return _specialPathIconStep > 0
            ? _specialPathIconStep
            : owner->specialPathIcon->height() * SPECIAL_ARROW_DISTANCE_MULTIPLIER;
    }
    else
    {
        return _pathIconStep > 0 ? _pathIconStep : _scaledPathIcon->height();
    }
}

sk_sp<const SkImage> OsmAnd::VectorLine_P::getPointImage() const
{
    return useSpecialArrow() ? owner->specialPathIcon : _scaledPathIcon;
}

