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

OsmAnd::VectorLine_P::VectorLine_P(VectorLine* const owner_)
: _hasUnappliedChanges(false), _hasUnappliedPrimitiveChanges(false), _isHidden(false),
  _metersPerPixel(1.0), _mapZoomLevel(InvalidZoomLevel), _mapVisualZoom(0.f), _mapVisualZoomShift(0.f),
  owner(owner_)
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
    
    _isHidden = hidden;
    _hasUnappliedChanges = true;
}

QVector<OsmAnd::PointI> OsmAnd::VectorLine_P::getPoints() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _points;
}

void OsmAnd::VectorLine_P::setPoints(const QVector<PointI>& points)
{
    QWriteLocker scopedLocker(&_lock);
    
    _points = points;
    
    _hasUnappliedPrimitiveChanges = true;
    _hasUnappliedChanges = true;
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
    bool changed = qAbs(_mapZoomLevel + _mapVisualZoom - mapState.zoomLevel - mapState.visualZoom) > 0.5;
    //_mapZoomLevel != mapState.zoomLevel ||
    //_mapVisualZoom != mapState.visualZoom ||
    //_mapVisualZoomShift != mapState.visualZoomShift;
    return changed;
}

void OsmAnd::VectorLine_P::applyMapState(const MapState& mapState)
{
    _metersPerPixel = mapState.metersPerPixel;
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
        
        const auto& vectorLineSymbolGroup = std::dynamic_pointer_cast<VectorLine::SymbolsGroup>(symbolGroup);
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

    _hasUnappliedChanges = false;
    _hasUnappliedPrimitiveChanges = false;
    
    return true;
}

std::shared_ptr<OsmAnd::VectorLine::SymbolsGroup> OsmAnd::VectorLine_P::inflateSymbolsGroup() const
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
        
        // TODO: needs to be updatable
        //generateArrowsOnPath(symbolsGroup);
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
    if(d == 0) {
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

float OsmAnd::VectorLine_P::zoom() const
{
    return _mapZoomLevel + (_mapVisualZoom >= 1.0f ? _mapVisualZoom - 1.0f : (_mapVisualZoom - 1.0f) * 2.0f);
}

std::shared_ptr<OsmAnd::OnSurfaceVectorMapSymbol> OsmAnd::VectorLine_P::generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol> vectorLine) const
{
    int order = owner->baseOrder;
    int pointsCount = _points.size();
    
    vectorLine->order = order++;
    vectorLine->position31 = _points[0];
    vectorLine->primitiveType = VectorMapSymbol::PrimitiveType::TriangleStrip;

    const auto verticesAndIndexes = std::make_shared<VectorMapSymbol::VerticesAndIndexes>();
    // Line has no reusable vertices - TODO clarify
    verticesAndIndexes->indices = nullptr;
    verticesAndIndexes->indicesCount = 0;
    
    vectorLine->scaleType = VectorMapSymbol::ScaleType::In31;
    vectorLine->scale = 1.0;
    vectorLine->direction = 0.f;
    
    float zoom = this->zoom();
    double radius = owner->lineWidth * Utilities::getPowZoom( 31 - _mapZoomLevel) * qSqrt(zoom) /
                        (IAtlasMapRenderer::TileSize3D * IAtlasMapRenderer::TileSize3D);
    // generate array of points
    std::vector<OsmAnd::PointD> pointsToPlot(pointsCount);
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        pointsToPlot[pointIdx] = PointD((_points[pointIdx].x-_points[0].x), (_points[pointIdx].y-_points[0].y));
    }
    
    std::vector<bool> include(pointsCount, false);
    include[0] = true;
    int pointsSimpleCount = simplifyDouglasPeucker(pointsToPlot, 0, (uint) pointsToPlot.size() - 1, radius / 3, include) + 1;
    
    // generate base points for connecting lines with triangles
    std::vector<OsmAnd::PointD> b1(pointsSimpleCount), b2(pointsSimpleCount), e1(pointsSimpleCount), e2(pointsSimpleCount);
    double ntan = 0, nx1 = 0, ny1 = 0;
    uint prevPointIdx = 0;
    uint insertIdx = 0;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        if(!include[pointIdx])
        {
            continue;
        }
        PointD pnt = pointsToPlot[pointIdx];
        PointD prevPnt = pointsToPlot[prevPointIdx];
        if(pointIdx > 0)
        {
            ntan = atan2(_points[pointIdx].x - _points[prevPointIdx].x,
                            _points[pointIdx].y - _points[prevPointIdx].y);
            nx1 = radius * sin(M_PI_2 - ntan) ;
            ny1 = radius * cos(M_PI_2 - ntan) ;
            e1[insertIdx] = b1[insertIdx] = OsmAnd::PointD(pnt.x - nx1, pnt.y + ny1);
            e2[insertIdx] = b2[insertIdx] = OsmAnd::PointD(pnt.x + nx1, pnt.y - ny1);
            e1[insertIdx-1] = OsmAnd::PointD(prevPnt.x - nx1, prevPnt.y + ny1);
            e2[insertIdx-1] = OsmAnd::PointD(prevPnt.x + nx1, prevPnt.y - ny1);
        } else {
            b2[insertIdx] = b1[insertIdx] = pnt;
        }
        prevPointIdx = pointIdx;
        insertIdx++;
    }
    
    verticesAndIndexes->verticesCount = (pointsSimpleCount - 2) * 4 + 2 * 2;
    verticesAndIndexes->vertices = new VectorMapSymbol::Vertex[verticesAndIndexes->verticesCount];
    verticesAndIndexes->position31 = new PointI(vectorLine->position31.x, vectorLine->position31.y);
    auto pVertex = verticesAndIndexes->vertices;

    
    //OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "=== pointsCount=%d zoom=%d visualZoom=%f metersPerPixel=%f radius=%f simpleCount=%d cnt=%d", verticesAndIndexes->verticesCount,
    //  _mapZoomLevel, _mapVisualZoom, _metersPerPixel, radius, pointsSimpleCount,pointsCount);
    
    
    // generate triangles
    for (auto pointIdx = 0u; pointIdx < pointsSimpleCount; pointIdx++)
    {
        
        if (pointIdx == 0)
        {
            pVertex->positionXY[0] = e1[pointIdx].x;
            pVertex->positionXY[1] = e1[pointIdx].y;
            pVertex->color = owner->fillColor;
            pVertex += 1;
            
            pVertex->positionXY[0] = e2[pointIdx].x;
            pVertex->positionXY[1] = e2[pointIdx].y;
            pVertex->color = owner->fillColor;
            pVertex += 1;
        }
        else if (pointIdx == pointsSimpleCount - 1)
        {
            pVertex->positionXY[0] = b1[pointIdx].x;
            pVertex->positionXY[1] = b1[pointIdx].y;
            pVertex->color = owner->fillColor;
            pVertex += 1;
        
            pVertex->positionXY[0] = b2[pointIdx].x;
            pVertex->positionXY[1] = b2[pointIdx].y;
            pVertex->color = owner->fillColor;
            pVertex += 1;
        }
        else
        {
            /*
            PointD l1 = findLineIntersection(e1[pointIdx-1], b1[pointIdx], e1[pointIdx], b1[pointIdx+1]);
            PointD l2 = findLineIntersection(e2[pointIdx-1], b2[pointIdx], e1[pointIdx], b1[pointIdx+1]);
            PointD l3 = findLineIntersection(e1[pointIdx-1], b1[pointIdx], e2[pointIdx], b2[pointIdx+1]);
            PointD l4 = findLineIntersection(e2[pointIdx-1], b2[pointIdx], e2[pointIdx], b2[pointIdx+1]);
             */
            PointD l1 = b1[pointIdx];
            PointD l2 = b2[pointIdx];
            PointD l3 = e1[pointIdx];
            PointD l4 = e2[pointIdx];
            // bewel - connecting only 3 points (one excluded depends on angle)
            // miter - connecting 4 points
            // round - generating between 2-3 point triangles (in place of triangle different between bewel/miter)
            
            pVertex->positionXY[0] = l1.x;
            pVertex->positionXY[1] = l1.y;
            pVertex->color = owner->fillColor;
            pVertex += 1;
            
            pVertex->positionXY[0] = l2.x;
            pVertex->positionXY[1] = l2.y;
            pVertex->color = owner->fillColor;
            pVertex += 1;
            
            pVertex->positionXY[0] = l3.x;
            pVertex->positionXY[1] = l3.y;
            pVertex->color = owner->fillColor;
            pVertex += 1;
            
            pVertex->positionXY[0] = l4.x;
            pVertex->positionXY[1] = l4.y;
            pVertex->color = owner->fillColor;
            pVertex += 1;
            
        }
    }
    
    vectorLine->isHidden = _isHidden;
    
    vectorLine->setVerticesAndIndexes(verticesAndIndexes);
    
    return vectorLine;
}

void OsmAnd::VectorLine_P::generateArrowsOnPath(const std::shared_ptr<VectorLine::SymbolsGroup> symbolsGroup) const
{
    if (owner->pathIconStep > 0 && owner->pathIcon)
    {
        SkPath path;
        const auto& start = _points[0];
        path.moveTo(start.x, start.y);
        for (int i = 1; i < _points.size(); i++)
        {
            const auto& p = _points[i];
            path.lineTo(p.x, p.y);
        }
        SkPathMeasure pathMeasure(path, false);
        
        bool ok = false;
        const auto length = pathMeasure.getLength();
        
        float step = Utilities::metersToX31(owner->pathIconStep * _metersPerPixel);
        auto iconOffset = 0.5f * step;
        const auto iconInstancesCount = static_cast<int>((length - iconOffset) / step) + 1;
        if (iconInstancesCount > 0)
        {
            int surfOrder = owner->baseOrder - 1;
            // Set of OnSurfaceMapSymbol from onMapSurfaceIcons
            const auto& onMapSurfaceIcon = owner->pathIcon;
            
            std::shared_ptr<SkBitmap> iconClone(new SkBitmap());
            ok = onMapSurfaceIcon->deepCopyTo(iconClone.get());
            assert(ok);
            
            for (auto iconInstanceIdx = 0; iconInstanceIdx < iconInstancesCount; iconInstanceIdx++, iconOffset += step)
            {
                SkPoint p;
                SkVector t;
                ok = pathMeasure.getPosTan(iconOffset, &p, &t);
                if (!ok)
                    break;
                
                // Get direction
                float direction = Utilities::normalizedAngleDegrees(qRadiansToDegrees(atan2(-t.x(), t.y())));
                const auto arrowSymbol = std::make_shared<OnSurfaceRasterMapSymbol>(symbolsGroup);
                
                arrowSymbol->order = surfOrder;
                
                arrowSymbol->bitmap = iconClone;
                arrowSymbol->size = PointI(iconClone->width(), iconClone->height());
                arrowSymbol->content = QString().sprintf(
                                                         "markerGroup(%p:%p)->onMapSurfaceIconBitmap:%p",
                                                         this,
                                                         symbolsGroup.get(),
                                                         iconClone->getPixels());
                arrowSymbol->languageId = LanguageId::Invariant;
                arrowSymbol->position31 = PointI(p.x(), p.y());
                arrowSymbol->direction = direction;
                arrowSymbol->isHidden = _isHidden;
                symbolsGroup->symbols.push_back(arrowSymbol);
            }
        }
    }
}

