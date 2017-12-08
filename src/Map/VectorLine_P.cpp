#include "VectorLine_P.h"
#include "VectorLine.h"
#include "Utilities.h"

#include "ignore_warnings_on_external_includes.h"
#include "restore_internal_warnings.h"

#include "MapSymbol.h"
#include "MapSymbolsGroup.h"
#include "VectorMapSymbol.h"
#include "OnSurfaceVectorMapSymbol.h"
#include "QKeyValueIterator.h"

OsmAnd::VectorLine_P::VectorLine_P(VectorLine* const owner_)
: owner(owner_)
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
    _hasUnappliedChanges = true;
}

bool OsmAnd::VectorLine_P::hasUnappliedChanges() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _hasUnappliedChanges;
}

bool OsmAnd::VectorLine_P::applyChanges()
{
    QReadLocker scopedLocker1(&_lock);
    
    if (!_hasUnappliedChanges)
        return false;
    
    QReadLocker scopedLocker2(&_symbolsGroupsRegistryLock);
    for (const auto& symbolGroup_ : constOf(_symbolsGroupsRegistry))
    {
        const auto symbolGroup = symbolGroup_.lock();
        if (!symbolGroup)
            continue;
        
        for (const auto& symbol_ : constOf(symbolGroup->symbols))
        {
            symbol_->isHidden = _isHidden;
            
            if (const auto symbol = std::dynamic_pointer_cast<OnSurfaceVectorMapSymbol>(symbol_))
            {
                if (!_points.empty())
                {
                    symbol->setPosition31(_points[0]);
                    //generatePrimitive(symbol);
                }
            }
        }
    }
    
    _hasUnappliedChanges = false;
    
    return true;
}

std::shared_ptr<OsmAnd::VectorLine::SymbolsGroup> OsmAnd::VectorLine_P::inflateSymbolsGroup() const
{
    QReadLocker scopedLocker(&_lock);
    
    // Construct new map symbols group for this marker
    const std::shared_ptr<VectorLine::SymbolsGroup> symbolsGroup(new VectorLine::SymbolsGroup(
                                                                                              std::const_pointer_cast<VectorLine_P>(shared_from_this())));
    symbolsGroup->presentationMode |= MapSymbolsGroup::PresentationModeFlag::ShowAllOrNothing;
    
    if (_points.size() > 1)
    {
        const std::shared_ptr<OnSurfaceVectorMapSymbol> vectorLine(new OnSurfaceVectorMapSymbol(symbolsGroup));
        generatePrimitive(vectorLine);
        symbolsGroup->symbols.push_back(vectorLine);
    }
    
    return symbolsGroup;
}

std::shared_ptr<OsmAnd::VectorLine::SymbolsGroup> OsmAnd::VectorLine_P::createSymbolsGroup() const
{
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

std::shared_ptr<OsmAnd::OnSurfaceVectorMapSymbol> OsmAnd::VectorLine_P::generatePrimitive(const std::shared_ptr<OnSurfaceVectorMapSymbol> vectorLine) const
{
    vectorLine->releaseVerticesAndIndices();
    
    int order = owner->baseOrder;
    int pointsCount = _points.size();
    
    vectorLine->order = order++;
    vectorLine->position31 = _points[0];
    vectorLine->primitiveType = VectorMapSymbol::PrimitiveType::TriangleStrip;
    
    // Line has no reusable vertices - TODO clarify
    vectorLine->indices = nullptr;
    vectorLine->indicesCount = 0;
    
    vectorLine->scaleType = VectorMapSymbol::ScaleType::InMeters;
    vectorLine->scale = 1.0;
    vectorLine->direction = 0.f;
    
    vectorLine->verticesCount = (pointsCount - 2) * 5 + 2 * 2;
    vectorLine->vertices = new VectorMapSymbol::Vertex[vectorLine->verticesCount];
    
    double lineWidth = owner->lineWidth;// + (double)(rand() % 10 + 1);
    auto pVertex = vectorLine->vertices;
    double ntan = atan2(_points[1].x - _points[0].x, _points[1].y - _points[0].y);
    double nx = lineWidth * sin(ntan) / 2;
    double ny = lineWidth * cos(ntan) / 2;
    
    double nx1 = lineWidth * sin(ntan + M_PI_4) / 2;
    double ny1 = lineWidth * cos(ntan + M_PI_4) / 2;
    double nx2 = lineWidth * sin(ntan - M_PI_4) / 2;
    double ny2 = lineWidth * cos(ntan - M_PI_4) / 2;
    
    for (auto pointIdx = 0u; pointIdx < pointsCount; pointIdx++)
    {
        
        int prevIndex = 0;
        double distX = Utilities::distance(
                                           Utilities::convert31ToLatLon(PointI(_points[pointIdx].x, _points[prevIndex].y)),
                                           Utilities::convert31ToLatLon(PointI(_points[prevIndex].x, _points[prevIndex].y)));
        
        double distY = Utilities::distance(
                                           Utilities::convert31ToLatLon(PointI(_points[prevIndex].x, _points[pointIdx].y)),
                                           Utilities::convert31ToLatLon(PointI(_points[prevIndex].x, _points[prevIndex].y)));
        
        if (_points[prevIndex].x > _points[pointIdx].x)
        {
            distX = -distX;
        }
        if (_points[prevIndex].y > _points[pointIdx].y)
        {
            distY = -distY;
        }
        pVertex->positionXY[0] = distX - nx2;
        pVertex->positionXY[1] = distY - ny2;
        pVertex->color = owner->fillColor;
        pVertex += 1;
        
        pVertex->positionXY[0] = distX - nx1;
        pVertex->positionXY[1] = distY - ny1;
        pVertex->color = owner->fillColor;
        pVertex += 1;
        
        if (pointIdx > 0 && pointIdx < pointsCount - 1)
        {
            ntan = atan2(_points[pointIdx + 1].x - _points[pointIdx].x,
                         _points[pointIdx + 1].y - _points[pointIdx].y);
            nx = lineWidth * sin(ntan) / 2;
            ny = lineWidth * cos(ntan) / 2;
            
            nx1 = lineWidth * sin(ntan + M_PI_4) / 2;
            ny1 = lineWidth * cos(ntan + M_PI_4) / 2;
            nx2 = lineWidth * sin(ntan - M_PI_4) / 2;
            ny2 = lineWidth * cos(ntan - M_PI_4) / 2;
            
            pVertex->positionXY[0] = distX;
            pVertex->positionXY[1] = distY;
            pVertex->color = owner->fillColor;
            pVertex += 1;
            
            pVertex->positionXY[0] = distX + nx1;
            pVertex->positionXY[1] = distY + ny1;
            pVertex->color = owner->fillColor;
            pVertex += 1;
            
            pVertex->positionXY[0] = distX + nx2;
            pVertex->positionXY[1] = distY + ny2;
            pVertex->color = owner->fillColor;
            pVertex += 1;
        }
    }
    
    vectorLine->isHidden = _isHidden;
    return vectorLine;
}
