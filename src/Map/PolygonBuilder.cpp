#include "PolygonBuilder.h"
#include "PolygonBuilder_P.h"

OsmAnd::PolygonBuilder::PolygonBuilder()
    : _p(new PolygonBuilder_P(this))
{
}

OsmAnd::PolygonBuilder::~PolygonBuilder()
{
}

bool OsmAnd::PolygonBuilder::isHidden() const
{
    return _p->isHidden();
}

OsmAnd::PolygonBuilder& OsmAnd::PolygonBuilder::setIsHidden(const bool hidden)
{
    _p->setIsHidden(hidden);

    return *this;
}

int OsmAnd::PolygonBuilder::getPolygonId() const
{
    return _p->getPolygonId();
}

OsmAnd::PolygonBuilder& OsmAnd::PolygonBuilder::setPolygonId(const int polygonId)
{
    _p->setPolygonId(polygonId);
    
    return *this;
}

int OsmAnd::PolygonBuilder::getBaseOrder() const
{
    return _p->getBaseOrder();
}

OsmAnd::PolygonBuilder& OsmAnd::PolygonBuilder::setBaseOrder(const int baseOrder)
{
    _p->setBaseOrder(baseOrder);

    return *this;
}

OsmAnd::FColorARGB OsmAnd::PolygonBuilder::getFillColor() const
{
    return _p->getFillColor();
}

OsmAnd::PolygonBuilder& OsmAnd::PolygonBuilder::setFillColor(const FColorARGB fillColor)
{
    _p->setFillColor(fillColor);

    return *this;
}

QVector<OsmAnd::PointI> OsmAnd::PolygonBuilder::getPoints() const
{
    return _p->getPoints();
}

OsmAnd::PolygonBuilder& OsmAnd::PolygonBuilder::setPoints(const QVector<OsmAnd::PointI>& points)
{
    _p->setPoints(points);

    return *this;
}

OsmAnd::PolygonBuilder& OsmAnd::PolygonBuilder::setCircle(const OsmAnd::PointI& center, const double radiusInMeters)
{
    _p->setCircle(center, radiusInMeters);

    return *this;
}

std::shared_ptr<OsmAnd::Polygon> OsmAnd::PolygonBuilder::buildAndAddToCollection(const std::shared_ptr<PolygonsCollection>& collection)
{
    return _p->buildAndAddToCollection(collection);
}

std::shared_ptr<OsmAnd::Polygon> OsmAnd::PolygonBuilder::build()
{
    return _p->build();
}
