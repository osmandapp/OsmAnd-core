#include "Polygon_P.h"
#include "Polygon.h"
#include "Utilities.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkPathMeasure.h>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <mapbox/earcut.hpp>
#include "restore_internal_warnings.h"

#include "MapSymbol.h"
#include "MapSymbolsGroup.h"
#include "VectorMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "OnSurfaceRasterMapSymbol.h"
#include "QKeyValueIterator.h"
#include "Logging.h"
#include "AtlasMapRenderer.h"

OsmAnd::Polygon_P::Polygon_P(Polygon* const owner_)
: _hasUnappliedChanges(false), _hasUnappliedPrimitiveChanges(false), _isHidden(false),
  _metersPerPixel(1.0), _mapZoomLevel(InvalidZoomLevel), _mapVisualZoom(0.f), _mapVisualZoomShift(0.f),
  owner(owner_)
{
}

OsmAnd::Polygon_P::~Polygon_P()
{
}

bool OsmAnd::Polygon_P::isHidden() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _isHidden;
}

void OsmAnd::Polygon_P::setIsHidden(const bool hidden)
{
    QWriteLocker scopedLocker(&_lock);
    
    _isHidden = hidden;
    _hasUnappliedChanges = true;
}

QVector<OsmAnd::PointI> OsmAnd::Polygon_P::getPoints() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _points;
}

void OsmAnd::Polygon_P::setPoints(const QVector<PointI>& points)
{
    QWriteLocker scopedLocker(&_lock);
    
    _points = points;
    
    _hasUnappliedPrimitiveChanges = true;
    _hasUnappliedChanges = true;
}

bool OsmAnd::Polygon_P::hasUnappliedChanges() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _hasUnappliedChanges;
}

bool OsmAnd::Polygon_P::hasUnappliedPrimitiveChanges() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _hasUnappliedPrimitiveChanges;
}

bool OsmAnd::Polygon_P::isMapStateChanged(const MapState& mapState) const
{
    return qAbs(_mapZoomLevel + _mapVisualZoom - mapState.zoomLevel - mapState.visualZoom) > 0.5;
}

void OsmAnd::Polygon_P::applyMapState(const MapState& mapState)
{
    _metersPerPixel = mapState.metersPerPixel;
    _mapZoomLevel = mapState.zoomLevel;
    _mapVisualZoom = mapState.visualZoom;
    _mapVisualZoomShift = mapState.visualZoomShift;
}

bool OsmAnd::Polygon_P::update(const MapState& mapState)
{
    QWriteLocker scopedLocker(&_lock);

    bool mapStateChanged = isMapStateChanged(mapState);
    if (mapStateChanged)
        applyMapState(mapState);

    _hasUnappliedPrimitiveChanges |= mapStateChanged;
    return mapStateChanged;
}

bool OsmAnd::Polygon_P::applyChanges()
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
                if (needUpdatePrimitive)
                    generatePrimitive(symbol);
            }
        }
    }

    _hasUnappliedChanges = false;
    _hasUnappliedPrimitiveChanges = false;
    
    return true;
}

std::shared_ptr<OsmAnd::Polygon::SymbolsGroup> OsmAnd::Polygon_P::inflateSymbolsGroup() const
{
    QReadLocker scopedLocker(&_lock);
    
    // Construct new map symbols group for this marker
    const std::shared_ptr<Polygon::SymbolsGroup> symbolsGroup(new Polygon::SymbolsGroup(std::const_pointer_cast<Polygon_P>(shared_from_this())));
    symbolsGroup->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAllOrNothing;
    
    if (_points.size() > 1)
    {
        const auto& polygon = std::make_shared<OnSurfaceVectorMapSymbol>(symbolsGroup);
        generatePrimitive(polygon);
        polygon->allowFastCheckByFrustum = false;
        symbolsGroup->symbols.push_back(polygon);
    }
    
    return symbolsGroup;
}

std::shared_ptr<OsmAnd::Polygon::SymbolsGroup> OsmAnd::Polygon_P::createSymbolsGroup(const MapState& mapState)
{
    applyMapState(mapState);
    
    const auto inflatedSymbolsGroup = inflateSymbolsGroup();
    registerSymbolsGroup(inflatedSymbolsGroup);
    return inflatedSymbolsGroup;
}

void OsmAnd::Polygon_P::registerSymbolsGroup(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup) const
{
    QWriteLocker scopedLocker(&_symbolsGroupsRegistryLock);
    
    _symbolsGroupsRegistry.insert(symbolsGroup.get(), symbolsGroup);
}

void OsmAnd::Polygon_P::unregisterSymbolsGroup(MapSymbolsGroup* const symbolsGroup) const
{
    QWriteLocker scopedLocker(&_symbolsGroupsRegistryLock);
    
    _symbolsGroupsRegistry.remove(symbolsGroup);
}

OsmAnd::PointD OsmAnd::Polygon_P::getProjection(PointD point, PointD from, PointD to ) const
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

double OsmAnd::Polygon_P::scalarMultiplication(double xA, double yA, double xB, double yB, double xC, double yC) const
{
    // Scalar multiplication between (AB, AC)
    return (xB - xA) * (xC - xA) + (yB - yA) * (yC - yA);
}

int OsmAnd::Polygon_P::simplifyDouglasPeucker(std::vector<PointD>& points, uint start, uint end, double epsilon, std::vector<bool>& include) const
{
    double dmax = -1;
    int index = -1;
    for (int i = start + 1; i <= end - 1; i++) {
        PointD proj = getProjection(points[i],points[start], points[end]);
        double d = qSqrt((points[i].x-proj.x)*(points[i].x-proj.x)+
                         (points[i].y-proj.y)*(points[i].y-proj.y));
        // calculate distance from polygon
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

float OsmAnd::Polygon_P::zoom() const
{
    return _mapZoomLevel + (_mapVisualZoom >= 1.0f ? _mapVisualZoom - 1.0f : (_mapVisualZoom - 1.0f) * 2.0f);
}

std::shared_ptr<OsmAnd::OnSurfaceVectorMapSymbol> OsmAnd::Polygon_P::generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol> polygon) const
{
    int order = owner->baseOrder;
    int pointsCount = _points.size();
    
    polygon->order = order++;
    polygon->position31 = _points[0];
    polygon->primitiveType = VectorMapSymbol::PrimitiveType::Triangles;

    const auto verticesAndIndices = std::make_shared<VectorMapSymbol::VerticesAndIndices>();
    // Line has no reusable vertices - TODO clarify
    verticesAndIndices->indices = nullptr;
    verticesAndIndices->indicesCount = 0;
    
    polygon->scaleType = VectorMapSymbol::ScaleType::In31;
    polygon->scale = 1.0;
    polygon->direction = 0.f;
    
    float zoom = this->zoom();
    double radius = Utilities::getPowZoom( 31 - _mapZoomLevel) * qSqrt(zoom) /
                        (AtlasMapRenderer::TileSize3D * AtlasMapRenderer::TileSize3D); // TODO: this should come from renderer
    // generate array of points
    std::vector<OsmAnd::PointD> pointsToPlot(pointsCount);
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        pointsToPlot[pointIdx] = PointD((_points[pointIdx].x-_points[0].x), (_points[pointIdx].y-_points[0].y));
    }
    
    std::vector<bool> include(pointsCount, false);
    include[0] = true;
    int pointsSimpleCount = simplifyDouglasPeucker(pointsToPlot, 0, (uint) pointsToPlot.size() - 1, radius / 3, include) + 1;
    
    // The number type to use for tessellation
    using Coord = double;

    // The index type. Defaults to uint32_t, but you can also pass uint16_t if you know that your
    // data won't have more than 65536 vertices.
    using N = uint32_t;

    // Create array
    using EPoint = std::array<Coord, 2>;
    
    // Check if polygon was oversimplified into one dot
    bool oversimplified = true;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        if (include[pointIdx])
        {
            const auto pointToPlot = pointsToPlot[pointIdx];
            bool startPoint = qFuzzyCompare(pointToPlot.x, 0) && qFuzzyCompare(pointToPlot.y, 0);
            if (!startPoint)
            {
                oversimplified = false;
                break;
            }
        }
    }
    
    // generate base points for connecting lines with triangles
    const auto originalSize = oversimplified ? pointsToPlot.size() : pointsSimpleCount;
    std::vector<EPoint> original(originalSize);
    uint insertIdx = 0;
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
        if (include[pointIdx] || oversimplified)
            original[insertIdx++] = { pointsToPlot[pointIdx].x, pointsToPlot[pointIdx].y };
    
    verticesAndIndices->position31 = new PointI(polygon->position31.x, polygon->position31.y);
    
    std::vector<std::vector<EPoint>> polygons;

    // Fill polygon structure with actual data. Any winding order works.
    // The first polyline defines the main polygon.
    polygons.push_back(original);
    // Following polylines define holes.
    //polygons.push_back({{75, 25}, {75, 75}, {25, 75}, {25, 25}});

    // Run tessellation
    // Returns array of indices that refer to the vertices of the input polygon.
    // Three subsequent indices form a triangle. Output triangles are clockwise.
    std::vector<N> indices = mapbox::earcut<N>(polygons);
    
    std::vector<VectorMapSymbol::Vertex> vertices;
    VectorMapSymbol::Vertex vertex;
    VectorMapSymbol::Vertex* pVertex = &vertex;
    
    // generate triangles
    for (auto pointIdx = 0u; pointIdx + 2 < indices.size(); pointIdx += 3)
    {
        pVertex->positionXYZ[1] = VectorMapSymbol::_absentElevation;
        pVertex->positionXYZ[0] = original[indices[pointIdx]][0];
        pVertex->positionXYZ[2] = original[indices[pointIdx]][1];
        pVertex->color = owner->fillColor;
        vertices.push_back(vertex);

        pVertex->positionXYZ[0] = original[indices[pointIdx + 1]][0];
        pVertex->positionXYZ[2] = original[indices[pointIdx + 1]][1];
        pVertex->color = owner->fillColor;
        vertices.push_back(vertex);
        
        pVertex->positionXYZ[0] = original[indices[pointIdx + 2]][0];
        pVertex->positionXYZ[2] = original[indices[pointIdx + 2]][1];
        pVertex->color = owner->fillColor;
        vertices.push_back(vertex);
    }
    
    verticesAndIndices->verticesCount = vertices.size();
    verticesAndIndices->vertices = new VectorMapSymbol::Vertex[vertices.size()];
    std::copy(vertices.begin(), vertices.end(), verticesAndIndices->vertices);

    polygon->isHidden = _isHidden;
    polygon->setVerticesAndIndices(verticesAndIndices);
    
    return polygon;
}
