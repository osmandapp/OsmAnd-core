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
#include "IAtlasMapRenderer.h"
#include "SkiaUtilities.h"

#include <Polyline2D/Polyline2D.h>
#include <Polyline2D/Vec2.h>

#define TRACK_WIDTH_THRESHOLD 8.0f
#define ARROW_DISTANCE_MULTIPLIER 1.5f
#define SPECIAL_ARROW_DISTANCE_MULTIPLIER 2.5f

// Colorization shemes
#define COLORIZATION_NONE 0
#define COLORIZATION_GRADIENT 1
#define COLORIZATION_SOLID 2

OsmAnd::VectorLine_P::VectorLine_P(VectorLine* const owner_)
: _hasUnappliedChanges(false)
, _hasUnappliedPrimitiveChanges(false)
, _isHidden(false)
, _isApproximationEnabled(true)
, _colorizationSceme(0)
, _lineWidth(1.0)
, _outlineWidth(0.0)
, _metersPerPixel(1.0)
, _mapZoomLevel(InvalidZoomLevel)
, _mapVisualZoom(0.f)
, _mapVisualZoomShift(0.f)
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

bool OsmAnd::VectorLine_P::hasColorizationMapping() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _colorizationMapping.size() == _points.size();
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
            double newWidth = _lineWidth / 2;
            double scale = newWidth / owner->pathIcon->width();
            _scaledBitmap = SkiaUtilities::scaleBitmap(owner->pathIcon, scale, 1);
        }
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

void OsmAnd::VectorLine_P::setColorizationScheme(const int colorizationScheme)
{
    QWriteLocker scopedLocker(&_lock);
    
    if (_colorizationSceme != colorizationScheme)
    {
        _colorizationSceme = colorizationScheme;
        
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
    bool changed = qAbs(_mapZoomLevel + _mapVisualZoom - mapState.zoomLevel - mapState.visualZoom) > 0.1;
    if (!changed && _visibleBBox31 != mapState.visibleBBox31)
    {
        auto bboxShiftPoint = _visibleBBox31.topLeft - mapState.visibleBBox31.topLeft;
        bool bboxChanged = abs(bboxShiftPoint.x) > _visibleBBox31.width() || abs(bboxShiftPoint.y) > _visibleBBox31.height();
        changed |= bboxChanged;
    }
    
    //_mapZoomLevel != mapState.zoomLevel ||
    //_mapVisualZoom != mapState.visualZoom ||
    //_mapVisualZoomShift != mapState.visualZoomShift;
    return changed;
}

void OsmAnd::VectorLine_P::applyMapState(const MapState& mapState)
{
    _metersPerPixel = mapState.metersPerPixel;
    _visibleBBox31 = mapState.visibleBBox31;
    _mapZoomLevel = mapState.zoomLevel;
    _mapVisualZoom = mapState.visualZoom;
    _mapVisualZoomShift = mapState.visualZoomShift;
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
        
        //const auto& vectorLineSymbolGroup = std::dynamic_pointer_cast<VectorLine::SymbolsGroup>(symbolGroup);
        bool needUpdatePrimitive = _hasUnappliedPrimitiveChanges && _points.size() > 1;
        for (const auto& symbol_ : constOf(symbolGroup->symbols))
        {
            symbol_->isHidden = _isHidden;
            
            if (const auto symbol = std::dynamic_pointer_cast<OnSurfaceVectorMapSymbol>(symbol_))
            {
                if (needUpdatePrimitive)
                    generatePrimitive(symbol);
            }
        }
    }
    owner->lineUpdatedObservable.postNotify(owner);
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
        owner->lineUpdatedObservable.postNotify(owner);
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
//    double d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    //double atn1 = atan2(p1.x - p2.x, p1.y - p2.y);
    //double atn2 = atan2(p3.x - p4.x, p3.y - p4.y);
    //double df = qAbs(atn1 - atn2);
    // printf("\n %f %f d=%f df=%f df-PI=%f df-2PI=%f", atn1, atn2, d, df, df - M_PI, df - 2 * M_PI);
    //double THRESHOLD = M_PI / 6;
//    if(d == 0 // || df < THRESHOLD  || qAbs(df - M_PI) < THRESHOLD || qAbs(df - 2 * M_PI) < THRESHOLD
//       ) {
        // in case of lines connecting p2 == p3
//        return p2;
//    }
    OsmAnd::PointD Pout;
//    r.x = ((p1.x* p2.y-p1.y*p2.x)*(p3.x - p4.x) - (p3.x* p4.y-p3.y*p4.x)*(p1.x - p2.x)) / d;
//    r.y = ((p1.x* p2.y-p1.y*p2.x)*(p3.y - p4.y) - (p3.x* p4.y-p3.y*p4.x)*(p1.y - p2.y)) / d;
    
    //Determine the intersection point of two line segments
      //http://paulbourke.net/geometry/lineline2d/
        double mua,mub;
        double denom,numera,numerb;
        const double eps = 0.000000000001;

        denom  = (p4.y-p3.y) * (p2.x-p1.x) - (p4.x-p3.x) * (p2.y-p1.y);
        numera = (p4.x-p3.x) * (p1.y-p3.y) - (p4.y-p3.y) * (p1.x-p3.x);
        numerb = (p2.x-p1.x) * (p1.y-p3.y) - (p2.y-p1.y) * (p1.x-p3.x);

        if ( (-eps < numera && numera < eps) &&
                 (-eps < numerb && numerb < eps) &&
                 (-eps < denom  && denom  < eps) ) {
            Pout.x = (p1.x + p2.x) * 0.5;
            Pout.y = (p1.y + p2.y) * 0.5;
            return Pout; //meaning the lines coincide
        }

        if (-eps < denom  && denom < eps) {
            Pout.x = 0;
            Pout.y = 0;
            return Pout; //meaning lines are parallel
        }

        mua = numera / denom;
        mub = numerb / denom;
        Pout.x = p1.x + mua * (p2.x - p1.x);
        Pout.y = p1.y + mua * (p2.y - p1.y);
        bool out1 = mua < 0 || mua > 1;
        bool out2 = mub < 0 || mub > 1;

//        if (out1 & out2) {
//            return Pout; //the intersection lies outside both segments
//        } else if (out1) {
//            return p1; //the intersection lies outside segment 1
//        } else if (out2) {
//            return p3; //the intersection lies outside segment 2
//        } else {
//            return Pout; //the intersection lies inside both segments
//        }
    
    return Pout;
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

int OsmAnd::VectorLine_P::simplifyDouglasPeucker(std::vector<PointD>& points, uint start, uint end, double epsilon, std::vector<bool>& include) const
{
    double dmax = -1;
    int index = -1;
    for (int i = start + 1; i <= end - 1; i++) {
        PointD proj = getProjection(points[i],points[start], points[end]);
        double d = qSqrt((points[i].x-proj.x)*(points[i].x-proj.x)+
                         (points[i].y-proj.y)*(points[i].y-proj.y));
        // calculate distance from line
        if (d > dmax) {
            dmax = d;
            index = i;
        }
    }
    if (dmax >= epsilon) {
        int enabled1 = simplifyDouglasPeucker(points, start, index, epsilon, include);
        int enabled2 = simplifyDouglasPeucker(points, index, end, epsilon, include);
        return enabled1 + enabled2 ;
    } else {
        include[end] = true;
        return 1;
    }
}

bool OsmAnd::VectorLine_P::calculateIntersection(const PointI& p1, const PointI& p0, const AreaI& bbox, PointI& pX)
{
    // Calculates intersection between line and bbox in clockwise manner.
    const auto& px = p0.x;
    const auto& py = p0.y;
    const auto& x = p1.x;
    const auto& y = p1.y;
    const auto& leftX = bbox.left();
    const auto& rightX = bbox.right();
    const auto& topY = bbox.top();
    const auto& bottomY = bbox.bottom();

    // firstly try to search if the line goes in
    if (py < topY && y >= topY) {
        int tx = (int)(px + ((double)(x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;
            pX.y = topY;
            return true;
        }
    }
    if (py > bottomY && y <= bottomY) {
        int tx = (int)(px + ((double)(x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;
            pX.y = bottomY;
            return true;
        }
    }
    if (px < leftX && x >= leftX) {
        int ty = (int)(py + ((double)(y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;
            pX.y = ty;
            return true;
        }

    }
    if (px > rightX && x <= rightX) {
        int ty = (int)(py + ((double)(y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;
            pX.y = ty;
            return true;
        }

    }

    // try to search if point goes out
    if (py > topY && y <= topY) {
        int tx = (int)(px + ((double)(x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;
            pX.y = topY;
            return true;
        }
    }
    if (py < bottomY && y >= bottomY) {
        int tx = (int)(px + ((double)(x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;
            pX.y = bottomY;
            return true;
        }
    }
    if (px > leftX && x <= leftX) {
        int ty = (int)(py + ((double)(y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;
            pX.y = ty;
            return true;
        }

    }
    if (px < rightX && x >= rightX) {
        int ty = (int)(py + ((double)(y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;
            pX.y = ty;
            return true;
        }

    }

    if (px == rightX || px == leftX) {
        if (py >= topY && py <= bottomY) {
            pX.x = px;
            pX.y = py;
            return true;
        }
    }

    if (py == topY || py == bottomY) {
        if (leftX <= px && px <= rightX) {
            pX.x = px;
            pX.y = py;
            return true;
        }
    }
    
    /*
    if (px == rightX || px == leftX || py == topY || py == bottomY) {
        pX = p0;//b.first = px; b.second = py;
        //        return true;
        // Is it right? to not return anything?
    }
     */
    return false;
}

void OsmAnd::VectorLine_P::calculateVisibleSegments(std::vector<std::vector<PointI>>& segments, QList<QList<FColorARGB>>& segmentColors) const
{
    bool segmentStarted = false;
    auto visibleArea = _visibleBBox31.getEnlargedBy(PointI(_visibleBBox31.width() * 3, _visibleBBox31.height() * 3));
    PointI curr, drawFrom, drawTo, inter1, inter2;
    auto prev = drawFrom = _points[0];
    int drawFromIdx = 0, drawToIdx = 0;
    std::vector<PointI> segment;
    QList<FColorARGB> colors;
    bool prevIn = visibleArea.contains(prev);
    for (int i = 1; i < _points.size(); i++)
    {
        curr = drawTo = _points[i];
        drawToIdx = i;
        bool currIn = visibleArea.contains(curr);
        bool draw = false;
        if (prevIn && currIn)
        {
            draw = true;
        }
        else
        {
            if (calculateIntersection(curr, prev, visibleArea, inter1))
            {
                draw = true;
                if (prevIn)
                {
                    drawTo = inter1;
                    drawToIdx = i;
                }
                else if (currIn)
                {
                    drawFrom = inter1;
                    drawFromIdx = i;
                    segmentStarted = false;
                }
                else if (calculateIntersection(prev, curr, visibleArea, inter2))
                {
                    drawFrom = inter1;
                    drawTo = inter2;
                    drawFromIdx = drawToIdx;
                    drawToIdx = i;
                    segmentStarted = false;
                }
                else
                {
                    draw = false;
                }
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
                    
                    segmentColors.push_back(colors);
                    colors.clear();
                }
                segment.push_back(drawFrom);
                if (hasColorizationMapping())
                    colors.push_back(_colorizationMapping[drawFromIdx]);
                
                segmentStarted = currIn;
            }
            if (segment.empty() || segment.back() != drawTo)
            {
                segment.push_back(drawTo);
                if (hasColorizationMapping())
                    colors.push_back(_colorizationMapping[drawToIdx]);
                
            }
        }
        else
        {
            segmentStarted = false;
        }
        prevIn = currIn;
        prev = drawFrom = curr;
        drawFromIdx = i;
    }
    if (!segment.empty())
        segments.push_back(segment);
    
    if (!colors.empty())
        segmentColors.push_back(colors);
}

float OsmAnd::VectorLine_P::zoom() const
{
    return _mapZoomLevel + (_mapVisualZoom >= 1.0f ? _mapVisualZoom - 1.0f : (_mapVisualZoom - 1.0f) * 2.0f);
}

std::shared_ptr<OsmAnd::OnSurfaceVectorMapSymbol> OsmAnd::VectorLine_P::generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol> vectorLine)
{
    int order = owner->baseOrder;
    float zoom = this->zoom();
    double scale = Utilities::getPowZoom(31 - zoom) * qSqrt(zoom) / (IAtlasMapRenderer::TileSize3D * IAtlasMapRenderer::TileSize3D);

    double radius = _lineWidth * scale * owner->screenScale;
    double simplificationRadius = radius - (_outlineWidth * scale);

    vectorLine->order = order++;
    vectorLine->primitiveType = _dashPattern.size() > 0 ? VectorMapSymbol::PrimitiveType::TriangleStrip : VectorMapSymbol::PrimitiveType::Triangles;
    vectorLine->scaleType = VectorMapSymbol::ScaleType::In31;
    vectorLine->scale = 1.0;
    vectorLine->direction = 0.f;
        
    const auto verticesAndIndexes = std::make_shared<VectorMapSymbol::VerticesAndIndexes>();
    // Line has no reusable vertices - TODO clarify
    verticesAndIndexes->indices = nullptr;
    verticesAndIndexes->indicesCount = 0;

    std::vector<VectorMapSymbol::Vertex> vertices;
    VectorMapSymbol::Vertex vertex;

    std::vector<std::vector<PointI>> segments;
    QList<QList<FColorARGB>> colors;
    calculateVisibleSegments(segments, colors);
    // Generate new arrow paths for visible segments
    _arrowsOnPath.clear();
    generateArrowsOnPath(_arrowsOnPath, segments);
    
    PointI startPos;
    bool startPosDefined = false;
    for (int segmentIndex = 0; segmentIndex < segments.size(); segmentIndex++)
    {
        auto& points = segments[segmentIndex];
        auto colorsForSegment = hasColorizationMapping() ? colors[segmentIndex] : QList<FColorARGB>();
        if (points.size() < 2)
            continue;
        
        if (!startPosDefined)
        {
            startPosDefined = true;
            startPos = points[0];
            vectorLine->position31 = startPos;
            verticesAndIndexes->position31 = new PointI(startPos);
        }
        int pointsCount = (int) points.size();
        // generate array of points
        std::vector<OsmAnd::PointD> pointsToPlot(pointsCount);
        QSet<uint> colorChangeIndexes;
        uint prevPointIdx = 0;
        for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
        {
            pointsToPlot[pointIdx] = PointD((points[pointIdx].x - startPos.x), (points[pointIdx].y - startPos.y));
            if (hasColorizationMapping() && _colorizationSceme == COLORIZATION_SOLID)
            {
                FColorARGB prevColor = colorsForSegment[prevPointIdx];
                if (prevColor != colorsForSegment[pointIdx])
                {
                    colorChangeIndexes.insert(pointIdx);
                    prevColor = colorsForSegment[pointIdx];
                }
                prevPointIdx = pointIdx;
            }
        }
        // Do not simplify colorization
        std::vector<bool> include(pointsCount, !_isApproximationEnabled || hasColorizationMapping());
        include[0] = true;
        int pointsSimpleCount = _isApproximationEnabled && !hasColorizationMapping()
            ? simplifyDouglasPeucker(pointsToPlot, 0, (uint) pointsToPlot.size() - 1, simplificationRadius / 3, include) + 1
            : pointsCount;
        
        // generate base points for connecting lines with triangles
        std::vector<OsmAnd::PointD> b1(pointsSimpleCount), b2(pointsSimpleCount), e1(pointsSimpleCount), e2(pointsSimpleCount), original(pointsSimpleCount);
        double ntan = 0, nx1 = 0, ny1 = 0;
        prevPointIdx = 0;
        uint insertIdx = 0;
        
        QVector<uint> solidColorChangeIndexes;
        QList<OsmAnd::FColorARGB> filteredColorsMap;
        for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
        {
            if (!include[pointIdx])
                continue;
            
            if (hasColorizationMapping())
            {
                const auto& color = colorsForSegment[pointIdx];
                filteredColorsMap.push_back(color);
                if (_colorizationSceme == COLORIZATION_SOLID && colorChangeIndexes.contains(pointIdx))
                    solidColorChangeIndexes.push_back(insertIdx);
            }
            
            PointD pnt = pointsToPlot[pointIdx];
            PointD prevPnt = pointsToPlot[prevPointIdx];
            original[insertIdx] = pnt;
            if (pointIdx > 0)
            {
                ntan = atan2(points[pointIdx].x - points[prevPointIdx].x, points[pointIdx].y - points[prevPointIdx].y);
                nx1 = radius * sin(M_PI_2 - ntan) ;
                ny1 = radius * cos(M_PI_2 - ntan) ;
                e1[insertIdx] = b1[insertIdx] = OsmAnd::PointD(pnt.x - nx1, pnt.y + ny1);
                e2[insertIdx] = b2[insertIdx] = OsmAnd::PointD(pnt.x + nx1, pnt.y - ny1);
                e1[insertIdx - 1] = OsmAnd::PointD(prevPnt.x - nx1, prevPnt.y + ny1);
                e2[insertIdx - 1] = OsmAnd::PointD(prevPnt.x + nx1, prevPnt.y - ny1);
            }
            else
            {
                b2[insertIdx] = b1[insertIdx] = pnt;
            }
            prevPointIdx = pointIdx;
            insertIdx++;
        }
                
        //OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "=== pointsCount=%d zoom=%d visualZoom=%f metersPerPixel=%f radius=%f simpleCount=%d cnt=%d", verticesAndIndexes->verticesCount,
        //  _mapZoomLevel, _mapVisualZoom, _metersPerPixel, radius, pointsSimpleCount,pointsCount);
        //FColorARGB fillColor = FColorARGB(0.6, 1.0, 0.0, 0.0);
        FColorARGB fillColor = _fillColor;
        int patternLength = (int) _dashPattern.size();
        // generate triangles
        if (patternLength > 0)
        {
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
            OsmAnd::PointD pe1 = e1[0];
            OsmAnd::PointD pe2 = e2[0];
            OsmAnd::PointD de1 = pe1;
            OsmAnd::PointD de2 = pe2;
            
            std::vector<OsmAnd::PointD> b1tar, b2tar, e1tar, e2tar, origTar;
            if (threshold == 0)
            {
                b1tar.push_back(b1[0]);
                b2tar.push_back(b2[0]);
                e1tar.push_back(e1[0]);
                e2tar.push_back(e2[0]);
                origTar.push_back(start);
            }
            double dashPhase = 0;
            int patternIndex = 0;
            bool firstDash = true;
            for (auto pointIdx = 1u; pointIdx < pointsSimpleCount; pointIdx++)
            {
                OsmAnd::PointD pnt = original[pointIdx];
                double segLength = sqrt(pow((prevPnt.x - pnt.x), 2) + pow((prevPnt.y - pnt.y), 2));
                // create a vector of direction for the segment
                OsmAnd::PointD v = pnt - prevPnt;
                // unit length
                OsmAnd::PointD u(v.x / segLength, v.y / segLength);
                
                double length = (firstDash && threshold > 0 ? threshold : dashPattern[patternIndex]) * scale;
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
                    {
                        b1tar.push_back(b1[pointIdx]);
                        b2tar.push_back(b2[pointIdx]);
                        e1tar.push_back(e1[pointIdx]);
                        e2tar.push_back(e2[pointIdx]);
                        origTar.push_back(pnt);
                    }
                }
                else
                {
                    while (deltaLength < segLength)
                    {
                        OsmAnd::PointD b1e1 = pe1 + delta;
                        OsmAnd::PointD b2e2 = pe2 + delta;
                        b1tar.push_back(b1e1);
                        b2tar.push_back(b2e2);
                        e1tar.push_back(b1e1);
                        e2tar.push_back(b2e2);
                        origTar.push_back(prevPnt + delta);
                        
                        if (!gap)
                        {
                            createVertexes(vertices, vertex, b1tar, b2tar, e1tar, e2tar, origTar, radius, fillColor, filteredColorsMap, !firstDash || segmentIndex > 0);
                            de1 = e1tar.back();
                            de2 = e2tar.back();
                            b1tar.clear();
                            b2tar.clear();
                            e1tar.clear();
                            e2tar.clear();
                            origTar.clear();
                            firstDash = false;
                        }
                        
                        if (!firstDash)
                            patternIndex++;
                        
                        patternIndex %= patternLength;
                        length = dashPattern[patternIndex] * scale;
                        gap = patternIndex % 2 == 1;
                        delta += OsmAnd::PointD(u.x * length, u.y * length);
                        deltaLength += length;
                    }
                    
                    if (!origTar.empty() && !gap)
                    {
                        b1tar.push_back(b1[pointIdx]);
                        b2tar.push_back(b2[pointIdx]);
                        e1tar.push_back(e1[pointIdx]);
                        e2tar.push_back(e2[pointIdx]);
                        origTar.push_back(pnt);
                    }
                }
                
                // calculate dash phase
                dashPhase = length - (segLength - deltaLength);
                if (dashPhase > length)
                    dashPhase -= length;
                
                pe1 = e1[pointIdx];
                pe2 = e2[pointIdx];
                prevPnt = pnt;
            }
            // end point
            if (threshold == 0)
            {
                if (origTar.size() == 0)
                {
                    b1tar.push_back(de1);
                    b2tar.push_back(de2);
                    e1tar.push_back(de1);
                    e2tar.push_back(de2);
                    origTar.push_back(end);
                }
                b1tar.push_back(b1[pointsSimpleCount - 1]);
                b2tar.push_back(b2[pointsSimpleCount - 1]);
                e1tar.push_back(b1tar.back());
                e2tar.push_back(b2tar.back());
                origTar.push_back(end);
                createVertexes(vertices, vertex, b1tar, b2tar, e1tar, e2tar, origTar, radius, fillColor, filteredColorsMap, true);
            }
        }
        else if (_colorizationSceme == COLORIZATION_SOLID && !solidColorChangeIndexes.isEmpty())
        {
            uint prevIdx = 1;
            for (const uint idx : solidColorChangeIndexes)
            {
                const auto begin = original.begin() + prevIdx;
                const auto end = original.begin() + idx;
                std::vector<PointD> subvector(begin, end);
                const auto fillColor = filteredColorsMap[idx - 1];
                QList<FColorARGB> colors;
                for (int i = 0; i < idx - prevIdx; i++)
                    colors.push_back(fillColor);
                
                const auto segmentVertices = crushedpixel::Polyline2D::create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(vertex,
                                                                                                               subvector, radius * owner->screenScale, _fillColor,
                                                                                                               colors,
                                                                                                               crushedpixel::Polyline2D::JointStyle::ROUND,
                                                                                                               crushedpixel::Polyline2D::EndCapStyle::BUTT);
                vertices.insert(vertices.end(), segmentVertices.begin(), segmentVertices.end());
                prevIdx = idx - 1;
            }
            const auto begin = original.begin() + prevIdx;
            const auto end = original.end();
            std::vector<PointD> subvector(begin, end);
            const auto colors = filteredColorsMap.mid(prevIdx, filteredColorsMap.size() - prevIdx);
            crushedpixel::Polyline2D::create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(vertex,
                                                                                                           vertices,
                                                                                                           subvector, radius * owner->screenScale,
                                                                                                           _fillColor, colors,
                                                                                                           crushedpixel::Polyline2D::JointStyle::ROUND,
                                                                                                           crushedpixel::Polyline2D::EndCapStyle::ROUND);
        }
        else
        {
            crushedpixel::Polyline2D::create<OsmAnd::VectorMapSymbol::Vertex, std::vector<OsmAnd::PointD>>(vertex,
                                                                                                           vertices,
                                                                                                           original, radius * owner->screenScale,
                                                                                                           _fillColor, filteredColorsMap,
                                                                                                           crushedpixel::Polyline2D::JointStyle::ROUND,
                                                                                                           crushedpixel::Polyline2D::EndCapStyle::ROUND);
        }
    }
    if (vertices.size() == 0)
    {
        vertex.positionXY[0] = 0;
        vertex.positionXY[1] = 0;
        vertices.push_back(vertex);
        verticesAndIndexes->position31 = new PointI(0, 0);
    }
    //verticesAndIndexes->verticesCount = (pointsSimpleCount - 2) * 2 + 2 * 2;
    verticesAndIndexes->verticesCount = (unsigned int) vertices.size();
    verticesAndIndexes->vertices = new VectorMapSymbol::Vertex[vertices.size()];
    std::copy(vertices.begin(), vertices.end(), verticesAndIndexes->vertices);

    vectorLine->isHidden = _isHidden;
    vectorLine->setVerticesAndIndexes(verticesAndIndexes);
    return vectorLine;
}

void OsmAnd::VectorLine_P::createVertexes(std::vector<VectorMapSymbol::Vertex> &vertices,
                  VectorMapSymbol::Vertex &vertex,
                  std::vector<OsmAnd::PointD> &b1,
                  std::vector<OsmAnd::PointD> &b2,
                  std::vector<OsmAnd::PointD> &e1,
                  std::vector<OsmAnd::PointD> &e2,
                  std::vector<OsmAnd::PointD> &original,
                  double radius,
                  FColorARGB &fillColor,
                  QList<FColorARGB>& colorMapping,
                  bool gap) const
{
    VectorMapSymbol::Vertex* pVertex = &vertex;
    bool direction = true;
    auto pointsCount = original.size();
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        const auto& mappingColor = colorMapping.isEmpty() ? fillColor : colorMapping[pointIdx];
        if (pointIdx == 0)
        {
            if (pointIdx == pointsCount - 1)
                return;
            
            if (gap)
                vertices.push_back(vertex);
            
            pVertex->positionXY[0] = e2[pointIdx].x;
            pVertex->positionXY[1] = e2[pointIdx].y;
            pVertex->color = mappingColor;
            vertices.push_back(vertex);
            
            if (gap)
                vertices.push_back(vertex);
            
            pVertex->positionXY[0] = e1[pointIdx].x;
            pVertex->positionXY[1] = e1[pointIdx].y;
            pVertex->color = mappingColor;
            vertices.push_back(vertex);
            
            direction = true;
        }
        else if (pointIdx == pointsCount - 1)
        {
            if (!direction) {
                pVertex->positionXY[0] = b1[pointIdx].x;
                pVertex->positionXY[1] = b1[pointIdx].y;
                pVertex->color = mappingColor;
                vertices.push_back(vertex);
            }
            
            pVertex->positionXY[0] = b2[pointIdx].x;
            pVertex->positionXY[1] = b2[pointIdx].y;
            pVertex->color = mappingColor;
            vertices.push_back(vertex);
            if (direction) {
                pVertex->positionXY[0] = b1[pointIdx].x;
                pVertex->positionXY[1] = b1[pointIdx].y;
                pVertex->color = mappingColor;
                vertices.push_back(vertex);
            }
        }
        else
        {
            int smoothLevel = 1; // could be constant & could be dynamic depends on the angle
            PointD l1 = findLineIntersection(e1[pointIdx - 1], b1[pointIdx], e1[pointIdx], b1[pointIdx + 1]);
            PointD l2 = findLineIntersection(e2[pointIdx - 1], b2[pointIdx], e2[pointIdx], b2[pointIdx + 1]);
            bool l1Intersects = (l1.x >= qMin(e1[pointIdx - 1].x, b1[pointIdx].x) && l1.x <= qMax(e1[pointIdx - 1].x, b1[pointIdx].x)) ||
            (l1.x >= qMin(e1[pointIdx].x, b1[pointIdx + 1].x) && l1.x <= qMax(e1[pointIdx].x, b1[pointIdx + 1].x));
            bool l2Intersects = (l2.x >= qMin(e2[ pointIdx - 1].x, b2[pointIdx].x) && l2.x <= qMax(e2[pointIdx - 1].x, b2[pointIdx].x)) ||
            (l2.x >= qMin(e2[pointIdx].x, b2[pointIdx + 1].x) && l2.x <= qMax(e2[pointIdx].x, b2[pointIdx + 1].x));
            
            // bewel - connecting only 3 points (one excluded depends on angle)
            // miter - connecting 4 points
            // round - generating between 2-3 point triangles (in place of triangle different between bewel/miter)
            
            if (!l2Intersects && !l1Intersects)
            {
                // skip point
            }
            else
            {
                bool startDirection = l2Intersects;
                const PointD& lp = startDirection ? l2 : l1;
                const std::vector<OsmAnd::PointD>& bp = startDirection ? b1 : b2;
                const std::vector<OsmAnd::PointD>& ep = startDirection ? e1 : e2;
                int phase = direction == startDirection ? 0 : 2;
                if (phase % 3 == 0) {
                    pVertex->positionXY[0] = lp.x;
                    pVertex->positionXY[1] = lp.y;
                    pVertex->color = mappingColor;
                    vertices.push_back(vertex);
                    phase++;
                }
                
                pVertex->positionXY[0] = bp[pointIdx].x;
                pVertex->positionXY[1] = bp[pointIdx].y;
                pVertex->color = mappingColor;
                vertices.push_back(vertex);
                phase++;
                if (phase % 3 == 0)
                {
                    pVertex->positionXY[0] = lp.x;
                    pVertex->positionXY[1] = lp.y;
                    pVertex->color = mappingColor;
                    vertices.push_back(vertex);
                    phase++;
                }
                
                if (smoothLevel > 0)
                {
                    double dv = 1.0 / (1 << smoothLevel);
                    double nt = dv;
                    while (nt < 1)
                    {
                        double rx = bp[pointIdx].x * nt + ep[pointIdx].x * (1 - nt);
                        double ry = bp[pointIdx].y * nt + ep[pointIdx].y * (1 - nt);
                        double ld = (rx - original[pointIdx].x) * (rx - original[pointIdx].x) + (ry - original[pointIdx].y) * (ry - original[pointIdx].y);
                        pVertex->positionXY[0] = original[pointIdx].x + radius / sqrt(ld) * (rx - original[pointIdx].x);
                        pVertex->positionXY[1] = original[pointIdx].y + radius / sqrt(ld) * (ry - original[pointIdx].y);
                        pVertex->color = mappingColor;
                        vertices.push_back(vertex);
                        phase++;
                        if (phase % 3 == 0)
                        {
                            // avoid overlap
                            vertices.push_back(vertex);
                            
                            pVertex->positionXY[0] = lp.x;
                            pVertex->positionXY[1] = lp.y;
                            pVertex->color = mappingColor;
                            vertices.push_back(vertex);
                            phase++;
                        }
                        nt += dv;
                    }
                }
                
                pVertex->positionXY[0] = ep[pointIdx].x;
                pVertex->positionXY[1] = ep[pointIdx].y;
                pVertex->color = mappingColor;
                vertices.push_back(vertex);
                phase++;
                if (phase % 3 == 0)
                {
                    // avoid overlap
                    vertices.push_back(vertex);
                    
                    pVertex->positionXY[0] = lp.x;
                    pVertex->positionXY[1] = lp.y;
                    pVertex->color = mappingColor;
                    vertices.push_back(vertex);
                    phase++;
                }
                if (smoothLevel > 0) {
                    //direction = direction;
                } else {
                    direction = !direction;
                }
            }
        }
    }
}

QVector<SkPath> OsmAnd::VectorLine_P::calculateLinePath(const std::vector<std::vector<PointI>>& visibleSegments) const
{
    QVector<SkPath> paths;
    if (!visibleSegments.empty())
    {
        for (const auto& segment : visibleSegments)
        {
            SkPath path;
            const auto& start = segment.back();
            path.moveTo(start.x, start.y);
            for (int i = (int) segment.size() - 2; i >= 0; i--)
            {
                const auto& p = segment[i];
                path.lineTo(p.x, p.y);
            }
            paths.push_back(path);
        }
    }
    return paths;
}

const QList<OsmAnd::VectorLine::OnPathSymbolData> OsmAnd::VectorLine_P::getArrowsOnPath() const
{
    return _arrowsOnPath;
}

void OsmAnd::VectorLine_P::generateArrowsOnPath(QList<OsmAnd::VectorLine::OnPathSymbolData>& symbolsData, const std::vector<std::vector<PointI>>& visibleSegments) const
{
    if (_showArrows && owner->pathIcon)
    {
        const auto paths = calculateLinePath(visibleSegments);
        for (const auto& path : paths)
        {
            SkPathMeasure pathMeasure(path, false);
            bool ok = false;
            const auto length = pathMeasure.getLength();

            float pathIconStep = owner->pathIconStep > 0 ? owner->pathIconStep : getPointStepPx();
            
            float step = Utilities::metersToX31(pathIconStep * _metersPerPixel * owner->screenScale);
            auto iconOffset = 0.5f * step;
            const auto iconInstancesCount = static_cast<int>((length - iconOffset) / step) + 1;
            if (iconInstancesCount > 0)
            {
                for (auto iconInstanceIdx = 0; iconInstanceIdx < iconInstancesCount; iconInstanceIdx++, iconOffset += step)
                {
                    SkPoint p;
                    SkVector t;
                    ok = pathMeasure.getPosTan(iconOffset, &p, &t);
                    if (!ok)
                        break;

                    const auto position = PointI(p.x(), p.y());
                    // Get mirrored direction
                    float direction = Utilities::normalizedAngleDegrees(qRadiansToDegrees(atan2(-t.x(), t.y())) - 180);
                    const VectorLine::OnPathSymbolData arrowSymbol(position, direction);
                    symbolsData.push_back(arrowSymbol);
                }
            }
        }
    }
}

bool OsmAnd::VectorLine_P::useSpecialArrow() const
{
    return _lineWidth <= TRACK_WIDTH_THRESHOLD * owner->screenScale && owner->specialPathIcon != nullptr;
}

double OsmAnd::VectorLine_P::getPointStepPx() const
{
    return useSpecialArrow() ?
    getPointBitmap()->height() * SPECIAL_ARROW_DISTANCE_MULTIPLIER :
    getPointBitmap()->height() * ARROW_DISTANCE_MULTIPLIER;
}

const std::shared_ptr<const SkBitmap> OsmAnd::VectorLine_P::getPointBitmap() const
{
    return useSpecialArrow() ? owner->specialPathIcon : _scaledBitmap;
}

