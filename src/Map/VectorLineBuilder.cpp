#include "VectorLineBuilder.h"
#include "VectorLineBuilder_P.h"

OsmAnd::VectorLineBuilder::VectorLineBuilder()
    : _p(new VectorLineBuilder_P(this))
{
}

OsmAnd::VectorLineBuilder::~VectorLineBuilder()
{
}

bool OsmAnd::VectorLineBuilder::isHidden() const
{
    return _p->isHidden();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setIsHidden(const bool hidden)
{
    _p->setIsHidden(hidden);

    return *this;
}

int OsmAnd::VectorLineBuilder::getLineId() const
{
    return _p->getLineId();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setLineId(const int lineId)
{
    _p->setLineId(lineId);
    
    return *this;
}

int OsmAnd::VectorLineBuilder::getBaseOrder() const
{
    return _p->getBaseOrder();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setBaseOrder(const int baseOrder)
{
    _p->setBaseOrder(baseOrder);

    return *this;
}

double OsmAnd::VectorLineBuilder::getLineWidth() const
{
    return _p->getLineWidth();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setLineWidth(const double width)
{
    _p->setLineWidth(width);

    return *this;
}

OsmAnd::FColorARGB OsmAnd::VectorLineBuilder::getFillColor() const
{
    return _p->getFillColor();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setFillColor(const FColorARGB fillColor)
{
    _p->setFillColor(fillColor);

    return *this;
}

QVector<OsmAnd::PointI> OsmAnd::VectorLineBuilder::getPoints() const
{
    return _p->getPoints();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setPoints(const QVector<OsmAnd::PointI>& points)
{
    _p->setPoints(points);

    return *this;
}

std::shared_ptr<OsmAnd::VectorLine> OsmAnd::VectorLineBuilder::buildAndAddToCollection(const std::shared_ptr<VectorLinesCollection>& collection)
{
    return _p->buildAndAddToCollection(collection);
}
