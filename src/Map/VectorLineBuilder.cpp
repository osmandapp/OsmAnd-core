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

bool OsmAnd::VectorLineBuilder::shouldShowArrows() const
{
    return _p->shouldShowArrows();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setShouldShowArrows(const bool showArrows)
{
    _p->setShouldShowArrows(showArrows);

    return *this;
}

bool OsmAnd::VectorLineBuilder::isApproximationEnabled() const
{
    return _p->isApproximationEnabled();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setApproximationEnabled(const bool enabled)
{
    _p->setApproximationEnabled(enabled);

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

std::vector<double> OsmAnd::VectorLineBuilder::getLineDash() const
{
    return _p->getLineDash();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setLineDash(const std::vector<double> dashPattern)
{
    _p->setLineDash(dashPattern);

    return *this;
}

QList<OsmAnd::FColorARGB> OsmAnd::VectorLineBuilder::getColorizationMapping() const
{
    return _p->getColorizationMapping();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setColorizationMapping(const QList<OsmAnd::FColorARGB>& colorizationMapping)
{
    _p->setColorizationMapping(colorizationMapping);

    return *this;
}

QList<OsmAnd::FColorARGB> OsmAnd::VectorLineBuilder::getOutlineColorizationMapping() const
{
    return _p->getOutlineColorizationMapping();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setOutlineColorizationMapping(
    const QList<OsmAnd::FColorARGB>& colorizationMapping)
{
    _p->setOutlineColorizationMapping(colorizationMapping);

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

QVector<float> OsmAnd::VectorLineBuilder::getHeights() const
{
    return _p->getHeights();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setHeights(const QVector<float>& heights)
{
    _p->setHeights(heights);

    return *this;
}

sk_sp<const SkImage> OsmAnd::VectorLineBuilder::getPathIcon() const
{
    return _p->getPathIcon();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setPathIcon(const SingleSkImage& image)
{
    _p->setPathIcon(image);

    return *this;    
}

sk_sp<const SkImage> OsmAnd::VectorLineBuilder::getSpecialPathIcon() const
{
    return _p->getSpecialPathIcon();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setSpecialPathIcon(const SingleSkImage& image)
{
    _p->setSpecialPathIcon(image);

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

float OsmAnd::VectorLineBuilder::getSpecialPathIconStep() const
{
    return _p->getSpecialPathIconStep();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setSpecialPathIconStep(const float step)
{
    _p->setSpecialPathIconStep(step);

    return *this;
}

bool OsmAnd::VectorLineBuilder::isPathIconOnSurface() const
{
    return _p->isPathIconOnSurface();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setPathIconOnSurface(const bool onSurface)
{
    _p->setPathIconOnSurface(onSurface);

    return *this;
}

float OsmAnd::VectorLineBuilder::getScreenScale() const
{
    return _p->getScreenScale();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setScreenScale(const float screenScale)
{
    _p->setScreenScale(screenScale);

    return *this;
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setEndCapStyle(const VectorLine::EndCapStyle endCapStyle)
{
    _p->setEndCapStyle(endCapStyle);
    
    return *this;
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setEndCapStyle(const int endCapStyle)
{
    _p->setEndCapStyle(static_cast<VectorLine::EndCapStyle>(endCapStyle));
    
    return *this;
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setJointStyle(const VectorLine::JointStyle jointStyle)
{
    _p->setJointStyle(jointStyle);
    
    return *this;
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setJointStyle(const int jointStyle)
{
    _p->setJointStyle(static_cast<VectorLine::JointStyle>(jointStyle));
    
    return *this;
}

int OsmAnd::VectorLineBuilder::getColorizationScheme() const
{
    return _p->getColorizationScheme();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setColorizationScheme(const int colorizationScheme)
{
    _p->setColorizationScheme(colorizationScheme);
    
    return *this;
}

double OsmAnd::VectorLineBuilder::getOutlineWidth() const
{
    return _p->getOutlineWidth();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setOutlineWidth(const double width)
{
    _p->setOutlineWidth(width);
    
    return *this;
}

OsmAnd::FColorARGB OsmAnd::VectorLineBuilder::getOutlineColor() const
{
    return _p->getOutlineColor();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setOutlineColor(const FColorARGB color)
{
    _p->setOutlineColor(color);

    return *this;
}

OsmAnd::FColorARGB OsmAnd::VectorLineBuilder::getNearOutlineColor() const
{
    return _p->getNearOutlineColor();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setNearOutlineColor(const FColorARGB color)
{
    _p->setNearOutlineColor(color);

    return *this;
}

OsmAnd::FColorARGB OsmAnd::VectorLineBuilder::getFarOutlineColor() const
{
    return _p->getFarOutlineColor();
}

OsmAnd::VectorLineBuilder& OsmAnd::VectorLineBuilder::setFarOutlineColor(const FColorARGB color)
{
    _p->setFarOutlineColor(color);

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
