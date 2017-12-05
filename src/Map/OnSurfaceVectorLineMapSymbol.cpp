#include "OnSurfaceVectorLineMapSymbol.h"

OsmAnd::OnSurfaceVectorLineMapSymbol::OnSurfaceVectorLineMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : OnSurfaceVectorMapSymbol(group_)
    , _lineWidth(3.0)
{
}

OsmAnd::OnSurfaceVectorLineMapSymbol::~OnSurfaceVectorLineMapSymbol()
{
}

double OsmAnd::OnSurfaceVectorLineMapSymbol::getLineWidth() const
{
    return _lineWidth;
}

void OsmAnd::OnSurfaceVectorLineMapSymbol::setLineWidth(const double width)
{
    _lineWidth = width;
}

QVector<OsmAnd::PointI> OsmAnd::OnSurfaceVectorLineMapSymbol::getPoints31() const
{
    return _points31;
}

void OsmAnd::OnSurfaceVectorLineMapSymbol::setPoints31(const QVector<PointI>& newPoints31)
{
    _points31 = newPoints31;
    
    if (_points31.size() > 0) 
    {
    	this->setPosition31(_points31[0]);
	}	
}

