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

bool OsmAnd::VectorLineBuilder::isDashed() const
{
    return _p->isDashed();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setDashed(const bool dashed)
{
    _p->setDashed(dashed);

    return *this;
}

double OsmAnd::VectorLineBuilder::getDashWidth() const
{
    return _p->getDashWidth();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setDashWidth(const double dashWidth)
{
    _p->setDashWidth(dashWidth);

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

std::shared_ptr<const SkBitmap> OsmAnd::VectorLineBuilder::getPathIcon() const
{
    return _p->getPathIcon();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setPathIcon(const std::shared_ptr<const SkBitmap>& bitmap)
{
    _p->setPathIcon(bitmap);

    return *this;    
}

float OsmAnd::VectorLineBuilder::getPathIconStep() const
{
    return _p->getPathIconStep();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setPathIconStep(const float step)
{
    _p->setPathIconStep(step);

    return *this;
}

std::shared_ptr<OsmAnd::VectorLine> OsmAnd::VectorLineBuilder::buildAndAddToCollection(const std::shared_ptr<VectorLinesCollection>& collection)
{
    return _p->buildAndAddToCollection(collection);
}

std::shared_ptr<OsmAnd::VectorLine> OsmAnd::VectorLineBuilder::build()
{
    return _p->build();
}
